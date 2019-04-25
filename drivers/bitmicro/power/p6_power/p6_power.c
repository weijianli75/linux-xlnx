/*
 * Power supply driver for P6.
 *
 * Copyright (C) 2018 Peter Wei <peter@microbt.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "p6_power.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>

/*
 * P6 CRC & I2C Operations
 */
static u8 basic_crc(u8 remainder, unsigned char nbyte)
{
    char i = 0;
    remainder ^= nbyte;
    for (i = 8; i > 0; --i) {
        if (remainder & 0x80) {
            remainder = (remainder << 1) ^ 0x7;
        } else {
            remainder = (remainder << 1);
        }
    }
    return remainder;
}

static u8 calc_crc(unsigned char i2c_addr, unsigned char i2c_command,
        unsigned char *data, unsigned int len)
{
    int i;
    unsigned char crc = 0;
    crc = basic_crc(crc, (i2c_addr << 1) + 0);
    crc = basic_crc(crc, i2c_command);
    crc = basic_crc(crc, (i2c_addr << 1) + 1);
    for (i = 0; i < len; i++) {
        crc = basic_crc(crc, *data++);
    }
    return crc;
}

static s32 p6_i2c_smbus_read_i2c_block_data(struct p6_data *data,
        u8 command, u8 length, u8 *values)
{
    struct i2c_client *client = data->client;
    int retry_count = 10;
    s32 status = 0;
    u8 crc_calc = 0;
    int common_errors = 0;

    while (retry_count--) {
        status = i2c_smbus_read_i2c_block_data(client, command, length, values);
        if (status >= 0) {
            crc_calc = calc_crc(client->addr, command, values, length - 1);
            if (crc_calc == *(values + length - 1))
                break;
            printk(
                    "%s: crc checking error, command = 0x%x, crc_read = 0x%02x, crc_calc = 0x%02x\n",
                    __func__, command, *(values + length - 1), crc_calc);
            common_errors |= BIT(POWER_ERROR_XFER_WARNIGN);
        }
        msleep(1);
    }

    if (retry_count < 0) {
        common_errors |= BIT(POWER_ERROR_XFER_ERROR);
        status = -1;
    }

    data->common_errors = common_errors;

    return status;
}

static s32 p6_i2c_smbus_read_word_data(struct p6_data *data,
        u8 command)
{
    struct i2c_client *client = data->client;
    int retry_count = 10;
    s32 status = 0;
    u8 crc_calc = 0;
    int common_errors = 0;

    while (retry_count--) {
        status = i2c_smbus_read_word_data(client, command);
        if (status >= 0) {
            crc_calc = calc_crc(client->addr, command,
                    (unsigned char *) &status, 1);
            if (crc_calc == ((status >> 8) & 0xff))
                break;
            printk("%s: crc checking error, command = 0x%x, crc_read = 0x%02x, crc_calc = 0x%02x\n",
                    __func__, command, (status >> 8) & 0xff, crc_calc);
           common_errors |= BIT(POWER_ERROR_XFER_WARNIGN);
        }
        msleep(1);
    }

    if (retry_count < 0) {
        common_errors |= BIT(POWER_ERROR_XFER_ERROR);
        status = -1;
    }

    data->common_errors = common_errors;

    return status;
}

int p6_power_get_version(struct p6_data *data)
{
    int ret = p6_i2c_smbus_read_word_data(data, CMD_READ_VERSION);
    if (ret < 0)
        return ret;

    return ret & 0xff;
}

int p6_power_get_status(struct p6_data *data)
{
    int ret = p6_i2c_smbus_read_word_data(data, CMD_ON_OFF);
    if (ret < 0)
        return ret;

    return ret & 0xff;
}

int p6_power_get_enable(struct p6_data *data)
{
    int ret = p6_i2c_smbus_read_word_data(data, CMD_ON_OFF);
    if (ret < 0)
        return ret;

    return ret & 0x02 ? 1 : 0;
}

int p6_power_set_enable(struct p6_data *data, int enable) {
    return i2c_smbus_write_byte_data(data->client, CMD_ON_OFF, !!enable);
}

int p6_power_get_protect_enable(struct p6_data *data) {
    int ret = p6_i2c_smbus_read_word_data(data, CMD_PROTECT_ENABLE);
    if (ret < 0)
        return ret;

    if (ret & (1 << 7))
        return 1;

    return 0;
}

int p6_power_set_protect_enable(struct p6_data *data, int enable) {
    if (enable)
        enable = 1;
    else
        enable = 2;
    return i2c_smbus_write_byte_data(data->client, CMD_PROTECT_ENABLE, enable);
}

int p6_power_get_errors(struct p6_data *data)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data,
            CMD_READ_ERRORCODE, 3, (u8 *) &reg);
    if (ret < 0)
        return 0;

    return reg;
}

int p6_power_get_voltage_v12(struct p6_data *data)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_VOUT_12V, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_voltage(struct p6_data *data)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_VOUT, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_set_voltage_set(struct p6_data *data, int vol) {
    return i2c_smbus_write_word_data(data->client, CMD_SET_VOUT, vol & 0xffff);
}

int p6_power_get_voltage_set(struct p6_data *data) {
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_SET_VOUT, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_current(struct p6_data *data)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_IOUT, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_power(struct p6_data *data)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_POUT, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_temp(struct p6_data *data, int id)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_TEMP0 + id, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_fan(struct p6_data *data)
{
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_FAN_SPEED, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_ios(struct p6_data *data, int id) {
    u16 reg;
    int ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_IOSA + id, 3, (u8 *) &reg);
    if (ret < 0)
        return ret;
    return reg;
}

int p6_power_get_sw_version(struct p6_data *data, char *buf) {
    int ret;

    if (!data->has_sw_version)
        return 0;

    memset(buf, 0, 12);
    ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_SW_REVISION, 12, buf);
    if (ret < 0)
        return ret;

    buf[11] = '\0';    /* remove the crc byte */
    memmove(buf, buf + 1, 12); /* skip the first length byte */
    return 0;
}

int p6_power_get_common_status(struct p6_data *data) {
    int status = 0;

    int ret = p6_power_get_status(data);
    if (ret != -1) {
        if (ret & 0x02)
            status |= BIT(POWER_STATUS_ENABLE);

        if (ret & 0x80)
            status |= BIT(POWER_STATUS_PROTECTED);
    }

    return status;
}

int p6_power_get_common_errors(struct p6_data *data) {
    return data->common_errors;
}

static ssize_t read_temperatures(struct p6_data *data, char *buf, ssize_t count)
{
    s32 ret;
    u32 reg;
    int i;
    for (i = 0; i < 3; i++) {
        ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_TEMP0 + i, 3,
                (u8 *) &reg);
        if (ret < 0)
            continue;
        count -= snprintf(buf + PAGE_SIZE - count, count,
                "Power TEMP%d        = %d\n", i, reg & 0xffff);
    }
    return count;
}

static ssize_t read_ios(struct p6_data *data, char *buf, ssize_t count)
{
    int i;
    int current_val;
    s32 ret;
    u32 reg;
    for (i = 0; i < 3; i++) {
        ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_IOSA + i, 3,
                (u8 *) &reg);
        if (ret < 0)
            continue;
        current_val = reg & 0xffff;
        count -= snprintf(buf + PAGE_SIZE - count, count,
                "Power IOS%c         = %d\n", 'A' + i, current_val);
    }
    return count;
}

static ssize_t read_swrevision(struct p6_data *data, char *buf, ssize_t count, int add_prefix)
{
    int ret;
    char read_buf[16] = { 0 };
    char *swrevision = NULL;

    /* [0]: length, [1~10]: revision, [11]: crc */
    ret = p6_i2c_smbus_read_i2c_block_data(data, CMD_READ_SW_REVISION, 12, (u8 *) read_buf);

    read_buf[11] = '\0';       /* remove the crc byte */
    swrevision = &read_buf[1]; /* skip the length byte */

    if (add_prefix)
        count -= snprintf(buf + PAGE_SIZE - count, count, "Power SW_REVISION  = %s\n", swrevision);
    else
        count -= snprintf(buf + PAGE_SIZE - count, count, "%s\n", swrevision);

    return count;
}

/*
 * Management functions
 */
static ssize_t show_list_all(struct device *dev,
        struct device_attribute *devattr, char *buf)
{
    struct p6_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    ssize_t c = PAGE_SIZE;
    s32 ret = 0;

    mutex_lock(&data->update_mutex);

    c -= snprintf(buf + PAGE_SIZE - c, c, "Power Address      = 0x%02x\n",
            client->addr);

    ret = p6_power_get_version(data);
    if (ret >= 0)
        c -= snprintf(buf + PAGE_SIZE - c, c, "Power Version      = 0x%02x\n",
                ret & 0xff);

    ret = p6_power_get_status(data);
    if (ret >= 0) {
        c -= snprintf(buf + PAGE_SIZE - c, c, "Power Status       = 0x%02x\n", ret);
        c -= snprintf(buf + PAGE_SIZE - c, c, "Power ON_OFF       = %s\n",
                (ret & 0x02) ? "on" : "off");
    }

    ret = p6_power_get_errors(data);
    c -= snprintf(buf + PAGE_SIZE - c, c, "Power ErrorCode    = 0x%04x\n", ret);

    /* voltage (0.01V) */
    ret = p6_power_get_voltage_v12(data);
    if (ret > 0)
        c -= snprintf(buf + PAGE_SIZE - c, c,
                "Power VOUT_12V     = %d\n", ret);

    ret = p6_power_get_voltage(data);
    if (ret > 0)
        c -= snprintf(buf + PAGE_SIZE - c, c,
                "Power VOUT         = %d\n", ret);

    ret = p6_power_get_voltage_set(data);
    if (ret > 0)
        c -= snprintf(buf + PAGE_SIZE - c, c,
                "Power VOUT_SET     = %d\n", ret);

    /* current (A) */
    ret = p6_power_get_current(data);
    if (ret > 0)
        c -= snprintf(buf + PAGE_SIZE - c, c,
                "Power IOUT         = %d\n", ret);

    /* power (W) */
    ret = p6_power_get_power(data);
    if (ret > 0)
        c -= snprintf(buf + PAGE_SIZE - c, c,
                "Power POUT         = %d\n", ret);

    /* temperature (0.1℃) */
    c = read_temperatures(data, buf, c);

    /* fan speed (RPM) */
    ret = p6_power_get_fan(data);
    if (ret > 0)
        c -= snprintf(buf + PAGE_SIZE - c, c,
                "Power FAN_SPEED    = %d\n", ret);

    if (data->version == P20_VERSION)
        c = read_ios(data, buf, c);

    c = read_swrevision(data, buf, c, 1);

    mutex_unlock(&data->update_mutex);

    return PAGE_SIZE - c;
}

static ssize_t show_version(struct device *dev,
        struct device_attribute *devattr, char *buf)
{
    struct p6_data *data = dev_get_drvdata(dev);
    ssize_t c = PAGE_SIZE;
    s32 reg;

    mutex_lock(&data->update_mutex);

    reg = p6_i2c_smbus_read_word_data(data, CMD_READ_VERSION);
    if (reg >= 0)
        c -= snprintf(buf + PAGE_SIZE - c, c, "0x%02x\n", reg & 0xff);

    mutex_unlock(&data->update_mutex);

    return PAGE_SIZE - c;
}

static ssize_t show_swrevision(struct device *dev,
                               struct device_attribute *devattr, char *buf)
{
    struct p6_data *data = dev_get_drvdata(dev);
    ssize_t c = PAGE_SIZE;

    mutex_lock(&data->update_mutex);

    c = read_swrevision(data, buf, c, 0);

    mutex_unlock(&data->update_mutex);

    return PAGE_SIZE - c;
}

static ssize_t show_on_off(struct device *dev, struct device_attribute *da,
        char *buf)
{
    struct p6_data *data = dev_get_drvdata(dev);
    ssize_t c = PAGE_SIZE;
    s32 reg;

    mutex_lock(&data->update_mutex);

    reg = p6_i2c_smbus_read_word_data(data, CMD_ON_OFF);
    if (reg >= 0)
        c -= snprintf(buf + PAGE_SIZE - c, c, "%s\n", (reg & 0x02) ? "on" : "off");

    mutex_unlock(&data->update_mutex);

    return PAGE_SIZE - c;
}

static ssize_t store_on_off(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    unsigned long val;

    if (kstrtol(buf, 10, &val) < 0)
        return -EINVAL;

    mutex_lock(&data->update_mutex);
    i2c_smbus_write_byte_data(client, CMD_ON_OFF, val & 0x1);
    mutex_unlock(&data->update_mutex);

    return count;
}

static ssize_t show_command_returning_2bytes(struct device *dev, struct device_attribute *da,
        u8 cmd, char *buf)
{
    struct p6_data *data = dev_get_drvdata(dev);
    ssize_t c = PAGE_SIZE;
    u32 reg = 0;
    s32 ret;

    mutex_lock(&data->update_mutex);

    ret = p6_i2c_smbus_read_i2c_block_data(data, cmd, 3, (u8 *) &reg);
    if (ret > 0) {
        if (cmd == CMD_READ_ERRORCODE)
            c -= snprintf(buf + PAGE_SIZE - c, c, "0x%04x\n", reg & 0xffff);
        else
            c -= snprintf(buf + PAGE_SIZE - c, c, "%d\n", reg & 0xffff);
    }

    mutex_unlock(&data->update_mutex);

    return PAGE_SIZE - c;
}

static ssize_t show_error_code(struct device *dev, struct device_attribute *da,
        char *buf)
{
    return show_command_returning_2bytes(dev, da, CMD_READ_ERRORCODE, buf);
}

static ssize_t show_vout(struct device *dev, struct device_attribute *da,
        char *buf)
{
    return show_command_returning_2bytes(dev, da, CMD_READ_VOUT, buf);
}

static ssize_t store_vout(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    unsigned long val;

    if (kstrtol(buf, 10, &val) < 0)
        return -EINVAL;

    mutex_lock(&data->update_mutex);
    i2c_smbus_write_word_data(client, CMD_SET_VOUT, val & 0xffff);
    mutex_unlock(&data->update_mutex);

    return count;
}

static ssize_t show_vout_set(struct device *dev, struct device_attribute *da,
        char *buf)
{
    return show_command_returning_2bytes(dev, da, CMD_SET_VOUT, buf);
}

static ssize_t show_iout(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    return show_command_returning_2bytes(dev, da, CMD_READ_IOUT, buf);
}

static ssize_t show_pout(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    return show_command_returning_2bytes(dev, da, CMD_READ_POUT, buf);
}

static ssize_t show_fan_speed(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    return show_command_returning_2bytes(dev, da, CMD_READ_FAN_SPEED, buf);
}

static ssize_t store_clear_fault(struct device *dev, struct device_attribute *da,
        const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_mutex);
    i2c_smbus_write_byte(client, CMD_CLEAR_FAULT);
    mutex_unlock(&data->update_mutex);

    return count;
}

/*------------------------------------------------------------------------------
 * Upgrade routines
 ------------------------------------------------------------------------------*/

#include "p6_upgrade.c"

static ssize_t show_upgrade_retries(struct device *dev, struct device_attribute *da,
                                    char *buf)
{
    struct p6_data *data = dev_get_drvdata(dev);
    ssize_t c = PAGE_SIZE;

    mutex_lock(&data->update_mutex);
    c -= snprintf(buf + PAGE_SIZE - c, c, "%d\n", upgrade_retries);
    mutex_unlock(&data->update_mutex);

    return PAGE_SIZE - c;;
}

static ssize_t store_upgrade_retries(struct device *dev, struct device_attribute *da,
                                     const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtol(buf, 10, &val) < 0)
        return -EINVAL;

    mutex_lock(&data->update_mutex);
    upgrade_retries = (val > 0)? val : 0;
    mutex_unlock(&data->update_mutex);

    return count;
}

static ssize_t store_upgrade(struct device *dev, struct device_attribute *da,
                             const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtol(buf, 16, &val) < 0)
        return -EINVAL;

    if (val != UPGRADE_ID)
        return -EINVAL;

    mutex_lock(&data->update_mutex);

    upgrade_power(data, buf, count);

    mutex_unlock(&data->update_mutex);

    return count;
}

static ssize_t show_upgrade_data(struct device *dev,
                                 struct device_attribute *devattr, char *buf)
{
    ssize_t c = PAGE_SIZE;

    c -= snprintf(buf + PAGE_SIZE - c, c, "%d\n", hex_data_size);

    return PAGE_SIZE - c;
}

static ssize_t store_upgrade_data(struct device *dev, struct device_attribute *da,
                                  const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);

    mutex_lock(&data->update_mutex);

    if (hex_data_size + count > UPGRADE_DATA_BUFFER_SIZE) {
        return count;
    }

    memcpy(hex_data_buf + hex_data_size, buf, count);
    hex_data_size += count;

    mutex_unlock(&data->update_mutex);

    return count;
}

static ssize_t store_upgrade_init(struct device *dev, struct device_attribute *da,
                                  const char *buf, size_t count)
{
    struct p6_data *data = dev_get_drvdata(dev);
    unsigned long val;

    if (kstrtol(buf, 16, &val) < 0)
        return -EINVAL;

    if (val != UPGRADE_INIT_ID)
        return -EINVAL;

    mutex_lock(&data->update_mutex);

    init_upgrade_state();

    mutex_unlock(&data->update_mutex);

    return count;
}

static ssize_t show_upgrade_status(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    ssize_t c = PAGE_SIZE;

    c -= snprintf(buf + PAGE_SIZE - c, c, "%s\n", upgrade_status);

    return PAGE_SIZE - c;
}

//------------------------------------------------------------------------------

static DEVICE_ATTR(list_all, S_IRUGO, show_list_all, NULL);
static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);
static DEVICE_ATTR(swrevision, S_IRUGO, show_swrevision, NULL);
static DEVICE_ATTR(on_off, S_IWUSR | S_IRUGO, show_on_off, store_on_off);
static DEVICE_ATTR(error_code, S_IRUGO, show_error_code, NULL);
static DEVICE_ATTR(vout, S_IWUSR | S_IRUGO, show_vout, store_vout);
static DEVICE_ATTR(vout_set, S_IRUGO, show_vout_set, NULL);
static DEVICE_ATTR(iout, S_IRUGO, show_iout, NULL);
static DEVICE_ATTR(pout, S_IRUGO, show_pout, NULL);
static DEVICE_ATTR(fan_speed, S_IRUGO, show_fan_speed, NULL);
static DEVICE_ATTR(clear_fault, S_IWUSR, NULL, store_clear_fault);

static DEVICE_ATTR(upgrade, S_IWUSR | S_IRUGO, NULL, store_upgrade);
static DEVICE_ATTR(upgrade_retries, S_IWUSR | S_IRUGO, show_upgrade_retries, store_upgrade_retries);
static DEVICE_ATTR(upgrade_data, S_IWUSR | S_IRUGO, show_upgrade_data, store_upgrade_data);
static DEVICE_ATTR(upgrade_init, S_IWUSR | S_IRUGO, NULL, store_upgrade_init);
static DEVICE_ATTR(upgrade_status, S_IRUGO, show_upgrade_status, NULL);

static struct attribute *p6_attributes[] = {
    &dev_attr_list_all.attr,
    &dev_attr_version.attr,
    &dev_attr_swrevision.attr,
    &dev_attr_on_off.attr,
    &dev_attr_error_code.attr,
    &dev_attr_vout.attr,
    &dev_attr_vout_set.attr,
    &dev_attr_iout.attr,
    &dev_attr_pout.attr,
    &dev_attr_fan_speed.attr,
    &dev_attr_clear_fault.attr,
    &dev_attr_upgrade.attr,
    &dev_attr_upgrade_retries.attr,
    &dev_attr_upgrade_data.attr,
    &dev_attr_upgrade_init.attr,
    &dev_attr_upgrade_status.attr,
    NULL
};

static const struct attribute_group p6_attr_group = {
    .attrs = p6_attributes,
};

/*
 * Initialization function
 */

static int p6_init_client(struct p6_data *data)
{
    int ret;
    int retry = 3;
    s32 status;

    status = p6_i2c_smbus_read_word_data(data, CMD_READ_VERSION);
    if (status < 0) {
        printk("p6_init_client failed status = 0x%x\n", status);
        return -1;
    }
    data->version = status & 0xff;

    retry = 3;
    data->has_sw_version = 1;
    do {
        ret = p6_power_get_sw_version(data, data->sw_version);
        if (ret == 0) {
            data->has_sw_version = 1;
            break;
        }

        printk("%s: failed to set read sw_version, ret: 0x%x!\n", __func__, ret);
    } while (--retry > 0);

    if (retry <= 0)
        data->has_sw_version = 0;

    retry = 3;
    do {
        ret = p6_power_set_protect_enable(data, 1);
        if (ret < 0) {
            printk("%s: failed to set protect enable !\n", __func__);
            return -1;
        }
        ret = p6_power_get_protect_enable(data);
        if (ret == 1)
            break;

        printk("%s: failed to set protect enable, ret: 0x%x!\n", __func__, ret);
    } while (--retry > 0);

    if (retry <= 0) {
        printk("%s: try 3 times to set protect enable failed\n", __func__);
    }

    hex_data_buf = kzalloc(UPGRADE_DATA_BUFFER_SIZE, GFP_KERNEL);
    if (hex_data_buf == NULL)
        return -1;
    init_upgrade_state();

    return 0;
}

/*
 * I2C init/probing/exit functions
 */
static int p6_power_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct p6_data *data;
    int err = 0;

    data = kzalloc(sizeof(struct p6_data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }
    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_mutex);

    err = p6_init_client(data);
    if (err)
        goto exit_kfree;

    /* Register sysfs hooks */
    err = sysfs_create_group(&client->dev.kobj, &p6_attr_group);
    if (err)
        goto exit_kfree;

    dev_info(&client->dev, "Power supply %s(version: 0x%02x) probed.\n", client->name,
            data->version);

    p6_power_supply_init(data);

    return 0;

exit_kfree:
    kfree(data);
    return err;
}

static int p6_power_remove(struct i2c_client *client)
{
    struct p6_data *data = (struct p6_data *)i2c_get_clientdata(client);

    sysfs_remove_group(&client->dev.kobj, &p6_attr_group);

    if (hex_data_buf) {
        kfree(hex_data_buf);
    }

    kfree(data);

    return 0;
}

static const struct i2c_device_id p6_power_id[] = {
    { "p6-power" },
    { }
};
MODULE_DEVICE_TABLE(i2c, p6_power_id);

static struct i2c_driver p6_power_driver = {
    .driver = {
        .name   = "p6-power",
        .owner  = THIS_MODULE,
    },
    .probe  = p6_power_probe,
    .remove = p6_power_remove,
    .id_table = p6_power_id,
};

module_i2c_driver(p6_power_driver);

MODULE_AUTHOR("Peter Wei <peter@microbt.com>");
MODULE_DESCRIPTION("P6 power supply driver");
MODULE_LICENSE("GPL");
