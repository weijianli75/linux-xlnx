#include "pmbus_power.h"

#define MAX_BUF_SIZE    256
#define FW_HEADER_SIZE  32
#define RETRY_COUNT     5

static int pmbus_power_enter_upload_mode(struct pmbus_power *power)
{
    int i;

    for (i = 0; i < RETRY_COUNT; i++) {
        pmbus_power_write_byte(power, 0xD6, 0x1);
        mdelay(80);
        if (pmbus_power_read_byte(power, 0xD6) == 0x1)
            break;
    }

    if (i == RETRY_COUNT)
        return -1;

    return 0;
}

static int pmbus_power_exit_upload_mode(struct pmbus_power *power)
{
    return pmbus_power_write_byte(power, 0xD6, 0x0);
}

//static void dump_data(char *pdata, int size)
//{
//    int i;
//
//    printk("data: ");
//    for (i = 0; i < size; i++)
//        printk("%02x ", *(pdata + i));
//
//    printk("\n");
//}

static int write_image_block(struct pmbus_power *power, char *pdata,
        int block_size, int block_offset, int delay_ms)
{
    int i;
    char data[block_size + 4]; // 1-byte length + 3-byte offset + block_size-byte image data

    // Re-compact data
    data[0] = block_size + 3;   // Add 3-byte block offset
    data[1] = (block_offset >> 16) & 0xff;
    data[2] = (block_offset >> 8) & 0xff;
    data[3] = block_offset & 0xff;

    for (i = 0; i < block_size; i++)
        data[i + 4] = *(pdata + i);

    //dump_data(data, block_size + 4);

    for (i = 0; i < RETRY_COUNT; i++) {
        pmbus_power_write(power, 0xD7, data, block_size + 4);
        mdelay(delay_ms);
        if (pmbus_power_read_word(power, 0xD8) == 0x0)
            break;
    }

    printk("%s: block_offset = %d delay_ms = %d i = %d pmbus_power_read_word(power, 0xD8) = 0x%x\n",
            __func__, block_offset, delay_ms, i, pmbus_power_read_word(power, 0xD8));

    if (i == RETRY_COUNT)
        return -1;

    return 0;
}

int pmbus_power_upgrade(struct pmbus_power *power, const char *filename)
{
    char buf[MAX_BUF_SIZE];
    int file_size, offset = 0, ret;
    int block_size, write_time, da_data;
    struct file *fp;
    struct kstat stat;

    fp = filp_open(filename, O_RDONLY, 0600);
    if (IS_ERR(fp)) {
        printk("%s: failed to open file %s\n", __func__, filename);
        return PTR_ERR(fp);
    }

    ret = vfs_getattr(&fp->f_path, &stat);
    if (ret < 0) {
        printk("%s: failed to get file %s stat\n", __func__, filename);
        filp_close(fp, NULL);
        return ret;
    }
    file_size = stat.size;
    printk("%s: get file %s, size: %d\n", __func__, filename, file_size);

    if (file_size < FW_HEADER_SIZE) {
        printk("%s: file size %d is less than %d\n", __func__, file_size, FW_HEADER_SIZE);
        filp_close(fp, NULL);
        return -EINVAL;
    }

    // Read and parse firmware header first
    ret = kernel_read(fp, offset, buf, FW_HEADER_SIZE);
    if (ret < 0) {
        printk("%s: read file %s error\n", __func__, filename);
        filp_close(fp, NULL);
        return ret;
    }

    if (buf[6] == 0x55 && buf[7] == 0x00)
        da_data = 0xAAAA;   // update primary mcu fw
    else
        da_data = 0x5555;   // update secondary mcu fw

    block_size = (buf[29] << 8) | buf[28];
    write_time = (buf[31] << 8) | buf[30];

    printk("Parsing firmware header:\n");
    printk("file_size=%d\n", file_size);
    printk("block_size=%d\n", block_size);
    printk("write_time=%d\n", write_time);
    printk("da_data=0x%x\n", da_data);

    if (block_size > MAX_BUF_SIZE) {
        printk("%s: block_size (%d) > MAX_BUF_SIZE (%d)\n", __func__, block_size, MAX_BUF_SIZE);
        filp_close(fp, NULL);
        return -EINVAL;
    }

    //
    // Start to upgrade
    //

    // Select Secondary (0x5555) / Primary (0xAAAA) MCU
    pmbus_power_write_word(power, 0xDA, da_data);

    // Enter firmware upload mode
    if (pmbus_power_enter_upload_mode(power) < 0) {
        filp_close(fp, NULL);
        return -EIO;
    }

    // Loop file
    while (offset < file_size) {
        int delay_ms = (offset == 0) ? 100 : write_time;

        ret = kernel_read(fp, offset, buf, block_size);
        if (ret < 0) {
            printk("%s:%d read file %s error\n", __func__, __LINE__, filename);
            filp_close(fp, NULL);
            return ret;
        } else if (ret == 0) {
            printk("%s:%d read file %s done\n", __func__, __LINE__, filename);
            break;
        }

        // Send image block
        if (write_image_block(power, buf, block_size, offset, delay_ms) < 0) {
            filp_close(fp, NULL);
            return -EIO;
        }

        offset += block_size;
    }

    filp_close(fp, NULL);

    mdelay(1000 * 10);  // delay 10s

    if (pmbus_power_read_word(power, 0xD8) != 0x0)
        return -EIO;

    // Exit firmware upload mode
    pmbus_power_exit_upload_mode(power);

    mdelay(1000 * 3);  // delay 3s

    if ((pmbus_power_read_word(power, 0xD8) & 0xFFFF) != 0xFFFF)
        return -EIO;

    printk("upgrade success\n");

    return 0;
}
