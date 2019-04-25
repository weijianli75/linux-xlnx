#include "meic_power.h"

#define MSG_HEAD        0xa5
#define CHECKSUM_SIZE   3

char checksum(char *buf, int size) {
    int i;
    char sum = 0;

    for (i = 0; i < size; i++) {
        sum += buf[i];
    }

    return sum;
}

void dump_buf(char *buf, int size, int line) {
    int i;

    printk("=============%s:%d============\n", __func__, __LINE__);

    for (i = 0; i < size; i++) {
        printk("0x%x, ", buf[i]);
        if (!((i+1) % 8))
            printk("\n");
    }

    printk("\n=============end===============\n");
}

int meic_power_i2c_write_read(struct i2c_client *client,
                            void *wbuf, int wsize, void *rbuf, int rsize)
{
    int ret;
    struct i2c_msg wmsg = {
        .addr = client->addr,
        .flags = 0,
        .buf = wbuf,
        .len = wsize,
    };
    struct i2c_msg rmsg = {
        .addr = client->addr,
        .flags = I2C_M_RD,
        .buf = rbuf,
        .len = rsize,
    };

//    printk("\n==== write: ");
//    dump_buf(wbuf, wsize, __LINE__);

    ret = i2c_transfer(client->adapter, &wmsg, 1);
    if (ret != 1) {
        printk("%s: write reg error, ret:%d !", __func__, ret);
        return -1;
    }

    msleep(50);

    ret = i2c_transfer(client->adapter, &rmsg, 1);
    if (ret != 1) {
        printk("%s: read reg error, ret:%d !", __func__, ret);
        return -1;
    }

//    printk("\n==== read: ");
//    dump_buf(rbuf, rsize, __LINE__);
    return 0;
}

int meic_power_i2c_write(struct i2c_client *client, void *buf, int size)
{
    int ret;
    struct i2c_msg wmsg = {
        .addr = client->addr,
        .flags = 0,
        .buf = buf,
        .len = size,
    };

//    printk("\n==== write: ");
//    dump_buf(buf, size, __LINE__);

    ret = i2c_transfer(client->adapter, &wmsg, 1);
    if (ret != 1) {
        printk("%s: write buf error, ret: %d !\n", __func__, ret);
        return -1;
    }
    return 0;
}

int meic_power_i2c_read(struct i2c_client *client, void *buf, int size)
{
    int ret;
    struct i2c_msg rmsg = {
        .addr = client->addr,
        .flags = I2C_M_RD,
        .buf = buf,
        .len = size,
    };

    ret = i2c_transfer(client->adapter, &rmsg, 1);
    if (ret != 1) {
        printk("%s: write buf error, ret: %d !\n", __func__, ret);
        return -1;
    }

//    printk("\n==== read: ");
//    dump_buf(buf, size, __LINE__);
    return 0;
}

int meic_power_read(struct meic_power *power, char reg, void *buf, int size)
{
    int ret;
    int retrys = 3;
    char wbuf[CHECKSUM_SIZE+1];
    char rbuf[CHECKSUM_SIZE+1+size];

    wbuf[0] = MSG_HEAD;
    wbuf[1] = reg;
    wbuf[2] = 0;
    wbuf[3] = checksum(wbuf, 3);

    do {
        ret = meic_power_i2c_write_read(power->client, wbuf, sizeof(wbuf), rbuf, sizeof(rbuf));
        if (ret >= 0) {
//            printk("\n==== read: ");
//            dump_buf(rbuf, sizeof(rbuf), __LINE__);

            if (rbuf[0] == MSG_HEAD
                && rbuf[1] == reg
                && rbuf[2] == size
                && checksum(rbuf, sizeof(rbuf) - 1) != rbuf[sizeof(rbuf)-1]) {
                printk("%s: read reg 0x%x checksum error !", __func__, reg);
                ret = -1;
            }
        }
        else
            power->common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        power->common_error |= BIT(POWER_ERROR_XFER_ERROR);
        printk("%s: failed to read reg: 0x%x\n", __func__, reg);
        return ret;
    }

    memcpy(buf, rbuf + 3, size);

    return ret;
}

int meic_power_write(struct meic_power *power, char reg, void *buf, int size)
{
    int i;
    int ret;
    int retrys = 3;
    char wbuf[CHECKSUM_SIZE+1+size];

    wbuf[0] = MSG_HEAD;
    wbuf[1] = reg;
    wbuf[2] = size;
    for (i = 0; i < size; i++) {
        wbuf[CHECKSUM_SIZE+i] = ((char *)buf)[i];
    }
    wbuf[sizeof(wbuf)-1] = checksum(wbuf, sizeof(wbuf)-1);

    do {
        ret = meic_power_i2c_write(power->client, wbuf, sizeof(wbuf));
        if (ret < 0)
            power->common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        power->common_error |= BIT(POWER_ERROR_XFER_ERROR);
        printk("%s: failed to write reg: 0x%x\n", __func__, reg);
    }

    return ret;
}

char meic_power_read_byte(struct meic_power *power, char reg)
{
    int ret;
    char data;

    ret = meic_power_read(power, reg, &data, sizeof(data));
    if (ret < 0)
        return -1;

    return data;
}

short meic_power_read_word(struct meic_power *power, char reg)
{
    int ret;
    char data[2];

    ret = meic_power_read(power, reg, data, sizeof(data));
    if (ret < 0)
        return -1;

    return swab16(*(short *)data);
}

short meic_power_write_word(struct meic_power *power, char reg, short data)
{
    short tmp = swab16(data);
    return meic_power_write(power, reg, &tmp, sizeof(tmp));
}


int meic_power_get_enable(struct meic_power *power)
{
    int ret;
    char enable;

    ret = meic_power_read(power, MEIC_REG_ENABLE, &enable, 1);
    if (ret < 0)
        return ret;

    return enable;
}

int meic_power_set_enable(struct meic_power *power, int enable)
{
    if (enable)
        enable = 0x80;
   return meic_power_write(power, MEIC_REG_ENABLE, &enable, 1);
}

int meic_power_get_voltage(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_VOL_V12);
}

int meic_power_get_voltage_set(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_VOLTAGE_SET);
}

int meic_power_set_voltage(struct meic_power *power, int vol)
{
    return meic_power_write_word(power, MEIC_REG_VOLTAGE_SET, vol);
}

int meic_power_get_current(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_CUR_V12) / 100;
}

int meic_power_get_status(struct meic_power *power)
{
    int ret;
    int status = 0;

    ret = meic_power_read(power, MEIC_REG_STATUS, &status, sizeof(status));
    if (ret < 0)
        return ret;

    return status;
}

int meic_power_get_fan_speed(struct meic_power *power)
{
    int ret;
    int fan = 0;

    ret = meic_power_read(power, MEIC_REG_FAN, &fan, sizeof(fan));
    if (ret < 0)
        return ret;

    return fan;
}

int meic_power_get_temp(struct meic_power *power, int i)
{
    return meic_power_read_byte(power, MEIC_REG_TEMP + i);
}

int meic_power_get_hw_version(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_HW_VERSION);
}

int meic_power_get_sw_version(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_SW_VERSION);
}

int meic_power_get_vout_max(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_OVER_VOL_V12);
}

int meic_power_set_vout_max(struct meic_power *power, int vol)
{
    return meic_power_write_word(power, MEIC_REG_OVER_VOL_V12, vol);
}

int meic_power_get_vout_min(struct meic_power *power)
{
    return meic_power_read_word(power, MEIC_REG_UNDER_VOL_V12);
}

int meic_power_set_vout_min(struct meic_power *power, int vol)
{
    return meic_power_write_word(power, MEIC_REG_UNDER_VOL_V12, vol);
}

int meic_power_get_common_status(struct meic_power *power)
{
    return 0;
}

int meic_power_get_common_error(struct meic_power *power)
{
    return power->common_error;
}

int meci_power_is_upgrading(struct meic_power *power)
{
    char cmd[3];
    unsigned short crc;
    char data[1], enable;

    cmd[0] = MEIC_REG_UPGRADE_ENABLE;
    crc = meic_crc16(cmd, 1);
    cmd[1] = (crc >> 8) & 0xff;
    cmd[2] = (crc >> 0) & 0xff;
    meic_power_i2c_write_read(power->client, cmd, sizeof(cmd), data, sizeof(data));
    if (data[0] == 0xaa) {
        power->is_upgrading = 1;
        return 1;
    }

    enable = meic_power_read_byte(power, MEIC_REG_UPGRADE_ENABLE);
    if (enable == 0x00) {
        power->is_upgrading = 0;
        return 0;
    }

    return -1;
}
