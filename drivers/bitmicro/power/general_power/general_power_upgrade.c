#include "general_power.h"

static void dump_buf(char *buf, int size, int line) {
    int i;

    printk("=============%s:%d============\n", __func__, __LINE__);

    for (i = 0; i < size; i++) {
        printk("0x%x, ", buf[i]);
        if (!((i+1) % 8))
            printk("\n");
    }

    printk("\n=============end===============\n");
}

static int general_power_read_without_checksum(struct general_power *power, char reg, void *buf, int size)
{
    int ret;
    int retrys = 3;
    char rbuf[size+1];

    do {
        ret = general_power_i2c_read(power, reg, rbuf, sizeof(rbuf));
        if (ret >= 0) {
//            printk("\n==== read: ");
//            dump_buf(rbuf, sizeof(rbuf), __LINE__);
        }
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        printk("%s: failed to read reg: 0x%x\n", __func__, reg);
    } else
        memcpy(buf, rbuf, size);

    return ret;
}

int general_power_send_package(struct general_power *power, char *wbuf, int size)
{
    int ret;
    char rbuf[size+1];

    ret = general_power_write(power, REG_UPGRADE, wbuf, size);
    if (ret < 0) {
        printk("%s: failed to send package, id: 0x%x\n", __func__, wbuf[0]);
        return ret;
    }

    msleep(800);

    ret = general_power_read_without_checksum(power, REG_UPGRADE, rbuf, sizeof(rbuf));
    if (ret < 0) {
        printk("%s: failed to get package respond, id: 0x%x\n", __func__, wbuf[0]);
        return ret;
    }

    if (memcmp(wbuf, rbuf+1, size)) {

        printk("%s: get error package respond, id: 0x%x\n", __func__, wbuf[0]);
        dump_buf(wbuf, size, __LINE__);
        dump_buf(rbuf+1, size, __LINE__);
//        return -1;
    }

    return 0;
}

int general_power_send_upgrade_start(struct general_power *power, int file_size)
{
    char wbuf[5];

    file_size = swab32(file_size);
    wbuf[0] = 0xA5;
    memcpy(wbuf+ 1, &file_size, sizeof(file_size));

    return general_power_send_package(power, wbuf, sizeof(wbuf));
}

int general_power_send_upgrade_end(struct general_power *power)
{
    char wbuf[2];

    wbuf[0] = 0xFF;
    wbuf[1] = 0x5A;

    return general_power_send_package(power, wbuf, sizeof(wbuf));
}

int general_power_upgrade(struct general_power *power, const char *file)
{
    int ret, retry, len, id, progress;
    int off = 0;
    char buf[128], wbuf[255+2];
    struct kstat stat;

    struct file *fp = filp_open(file, O_RDONLY, 0600);
    if (IS_ERR(fp)) {
        printk("%s: failed to open file %s\n", __func__, file);
        return PTR_ERR(fp);
    }

    ret = vfs_getattr(&fp->f_path, &stat);
    if (ret < 0) {
        printk("%s: failed to get file %s stat\n", __func__, file);
        return ret;
    }
    printk("%s: get file %s, size: %lld\n", __func__, file, stat.size);

    /**
     * Step 1: send upgrade start
     */
    ret = general_power_send_upgrade_start(power, stat.size);
    if (ret < 0) {
        printk("%s: Step 1 send upgrade start error\n", __func__);
        return -1;
    }

    /**
     * Step 2: send upgrade file
     */
    progress = -1;
    id = 0;
    do {
        // read buf from file
        len = kernel_read(fp, off, buf, sizeof(buf));
        if (len < 0) {
            printk("%s: read file %s error\n", __func__, file);
            return len;
        } else if (len == 0) {
            printk("%s: read file %s done\n", __func__, file);
            break;
        }
        off += len;

        wbuf[0] = id;
        wbuf[1] = len;
        memcpy(wbuf + 2, buf, len);

        retry = 30;
        do {
            ret = general_power_send_package(power, wbuf, len+2);
            msleep(10);
        } while (ret < 0 && retry-- > 0);

        if (progress != off * 100 / (int)stat.size) {
            progress = off * 100 / (int)stat.size;
            printk("%s: upgrade power %d%%\n", __func__, progress);
        }

        id = (id + 1) % 0xff;
    } while (ret >= 0);

    if (ret < 0) {
        printk("%s: Step 2 send upgrade file error\n", __func__);
        return -1;
    }

    /**
     * Step 3: send upgrade file
     */
    ret = general_power_send_upgrade_end(power);
    if (ret < 0) {
        printk("%s: Step 3 send upgrade end error\n", __func__);
        return -1;
    }

    printk("%s: upgrade power success!!\n", __func__);
    return 0;
}
