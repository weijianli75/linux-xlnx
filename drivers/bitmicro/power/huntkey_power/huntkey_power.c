#include "huntkey_power.h"

enum {
    REG_ENABLE = 0x01,
    REG_VOUT,
    REG_IOUT,
    REG_VOUT_SET,
    REG_ERRORS,
    REG_TEMP0,
    REG_TEMP1,
    REG_TEMP2,
    REG_TEMP3,
    REG_TEMP4,
    REG_FAN_SPEED0,
    REG_FAN_SPEED1,
    REG_OVER_VIN,
    REG_OVER_IIN,
    REG_UNDER_VIN,
    REG_STATUS          = 0x70,
    REG_SW_VERSION      = 0x80,
    REG_HW_VERSION,
    REG_MODEL,
    REG_VENDER,
    REG_PRODUCT_ADDR,
    REG_PROTECT_DATE,
    REG_UPGRADE         = 0xA0,
};

char huntkey_checksum(char extra, char *buf, int size) {
    int i;
    char sum = extra;

    for (i = 0; i < size; i++) {
        sum += buf[i];
    }

    return sum;
}

static void dump_buf(char *buf, int size, int line) {
    int i;

    printk("\n=============%s:%d============\n", __func__, line);

    for (i = 0; i < size; i++) {
        printk("0x%x, ", buf[i]);
        if (!((i+1) % 8))
            printk("\n");
    }

    printk("\n=============end===============\n");
}

int huntkey_power_i2c_write_read(struct i2c_client *client,
                            void *wbuf, int wsize, void *rbuf, int rsize)
{
    int ret;
    struct i2c_msg msg[2] = {
        {
            .addr = client->addr,
            .flags = 0,
            .buf = wbuf,
            .len = wsize,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .buf = rbuf,
            .len = rsize,
        },
    };

//    printk("\n==== write: ");
//    dump_buf(wbuf, wsize, __LINE__);

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret != 2) {
        printk("%s: read reg error, ret:%d !", __func__, ret);
        return -1;
    }
//
//    printk("\n==== read: ");
//    dump_buf(rbuf, rsize, __LINE__);
    return 0;
}

int huntkey_power_i2c_write(struct i2c_client *client, void *buf, int size)
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

int huntkey_power_read(struct huntkey_power *power, char reg, void *buf, int size)
{
    int ret;
    int retrys = 3;
    char rbuf[size+1];

    do {
        ret = huntkey_power_i2c_write_read(power->client, &reg, sizeof(reg), rbuf, sizeof(rbuf));
        if (ret >= 0) {
//            printk("\n==== read: ");
//            dump_buf(rbuf, sizeof(rbuf), __LINE__);
            int extra = reg+(power->client->addr<<1) + ((power->client->addr<<1)+1);
            if (huntkey_checksum(extra, rbuf, sizeof(rbuf) - 1) != rbuf[sizeof(rbuf)-1]) {
                printk("%s: read reg 0x%x checksum error !", __func__, reg);
                ret = -1;
                dump_buf(rbuf, sizeof(rbuf), __LINE__);
            }
        }
        else
            power->common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        power->common_error |= BIT(POWER_ERROR_XFER_ERROR);
        printk("%s: failed to read reg: 0x%x\n", __func__, reg);
    }

    memcpy(buf, rbuf, size);

    return ret;
}

int huntkey_power_write(struct huntkey_power *power, char reg, void *buf, int size)
{
    int ret;
    int retrys = 3;
    char wbuf[size+2];
    wbuf[0] = reg;
    memcpy(wbuf + 1, buf, size);
    wbuf[sizeof(wbuf)-1] = huntkey_checksum(power->client->addr << 1, wbuf, sizeof(wbuf)-1);

    do {
        ret = huntkey_power_i2c_write(power->client, wbuf, sizeof(wbuf));
        if (ret < 0)
            power->common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        power->common_error |= BIT(POWER_ERROR_XFER_ERROR);
        printk("%s: failed to write reg: 0x%x\n", __func__, reg);
    }

    return ret;
}

short huntkey_power_read_word(struct huntkey_power *power, char reg)
{
    int ret;
    short data;

    ret = huntkey_power_read(power, reg, &data, sizeof(data));
    if (ret < 0)
        return -1;

    return data;
}

short huntkey_power_write_word(struct huntkey_power *power, char reg, short data)
{
    short tmp = data;
    return huntkey_power_write(power, reg, &tmp, sizeof(tmp));
}

int huntkey_power_get_enable(struct huntkey_power *power)
{
    int ret;
    char enable;

    ret = huntkey_power_read(power, REG_ENABLE, &enable, sizeof(enable));
    if (ret < 0)
        return ret;

    return enable;
}

int huntkey_power_set_enable(struct huntkey_power *power, int enable)
{
    int ret;
    char buf[1] = {!!enable};
    if (enable) {
        ret = huntkey_power_set_vout_set(power, power->vout_set);
        if (ret < 0)
            return ret;
    }
    return huntkey_power_write(power, REG_ENABLE, buf, sizeof(buf));
}

int huntkey_power_get_vout(struct huntkey_power *power)
{
    return huntkey_power_read_word(power, REG_VOUT);
}

int huntkey_power_get_vout_set(struct huntkey_power *power)
{
    int vol = huntkey_power_read_word(power, REG_VOUT_SET);
    if (vol < 0)
        return vol;

    return vol;
}

int huntkey_power_set_vout_set(struct huntkey_power *power, int vol)
{
    return huntkey_power_write_word(power, REG_VOUT_SET, vol);
}

int huntkey_power_get_iout(struct huntkey_power *power)
{
    return huntkey_power_read_word(power, REG_IOUT);
}

int huntkey_power_get_status(struct huntkey_power *power)
{
    int ret;
    int status = 0;

    ret = huntkey_power_read(power, REG_STATUS, &status, sizeof(status));
    if (ret < 0)
        return ret;

    return status;
}

int huntkey_power_get_errors(struct huntkey_power *power)
{
    int ret;
    int status = 0;

    ret = huntkey_power_read(power, REG_ERRORS, &status, sizeof(status));
    if (ret < 0)
        return ret;

    return status;
}

int huntkey_power_get_fan(struct huntkey_power *power)
{
    return huntkey_power_read_word(power, REG_FAN_SPEED0);
}

int huntkey_power_get_temp(struct huntkey_power *power)
{
    return huntkey_power_read_word(power, REG_TEMP0);
}

int huntkey_power_get_hw_version(struct huntkey_power *power, char *buf)
{
    return huntkey_power_read(power, REG_HW_VERSION, buf, 16);
}

int huntkey_power_get_sw_version(struct huntkey_power *power, char *buf)
{
    return huntkey_power_read(power, REG_SW_VERSION, buf, 16);
}

int huntkey_power_get_common_status(struct huntkey_power *power)
{
    return 0;
}

int huntkey_power_get_common_error(struct huntkey_power *power)
{
    return power->common_error;
}

