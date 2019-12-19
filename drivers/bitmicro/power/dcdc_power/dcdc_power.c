#include "dcdc_power.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>

char hw_version[] = "isl23315";
char sw_version[] = "10";

struct dc_opt opt[NODE_TOTAL] = {
    [0] = {
        .pdat = NULL,
        .addr = 0x50,
        .vol = 255,
        .io = 934,
        .en = false,
    },

    [1] = {
        .pdat = NULL,
        .addr = 0x51,
        .vol = 255,
        .io = 939,
        .en = false,
    },

    [2] = {
        .pdat = NULL,
        .addr = 0x52,
        .vol = 255,
        .io = 937,
        .en = false,
    },
};

static int i2c_read_bytes(struct i2c_client *client, unsigned char addr,
                          unsigned char *buf, unsigned char len)
{
    struct i2c_msg msgs[2];

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr = addr; //client->addr;
    msgs[0].len = 1;
    msgs[0].buf = buf;

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = addr; //client->addr;
    msgs[1].len = len - 1;
    msgs[1].buf = buf + 1;

    return i2c_transfer(client->adapter, msgs, 2);
}

static int i2c_write_bytes(struct i2c_client *client, unsigned char addr,
                           const unsigned char *buf, unsigned char len)
{
    struct i2c_msg msg;

    msg.flags = !I2C_M_RD;
    msg.addr = addr; //client->addr;
    msg.len = len;
    msg.buf = (unsigned char *)buf;

    return i2c_transfer(client->adapter, &msg, 1);
}

int dc_read_reg_one_byte(struct dc_data *power, unsigned char addr,
                         unsigned char reg, unsigned char *val)
{
    int ret = -1;
    int retry = 3;
    unsigned char buf[2];

    buf[0] = reg;
    while (retry--)
    {
        ret = i2c_read_bytes(power->client, addr, buf, 1 + 1);
        if (ret > 0)
        {
            *val = buf[1];
            break;
        }
        power->err |= BIT(POWER_ERROR_XFER_WARNIGN);
        msleep(1);
    }

    if (ret < 0)
    {
        power->err |= BIT(POWER_ERROR_XFER_ERROR);
        printk("dev:0x%x,dc_read_reg_one_byte failed ret = 0x%x\n", addr, ret);
    }

    return ret;
}

int dc_write_reg_one_byte(struct dc_data *power, unsigned char addr,
                          unsigned char reg, unsigned char val)
{
    int ret = -1;
    int retry = 3;
    unsigned char buf[2];
    buf[0] = reg;
    buf[1] = val;

    while (retry--)
    {
        ret = i2c_write_bytes(power->client, addr, buf, 1 + 1);
        if (ret > 0)
        {
            break;
        }
        power->err |= BIT(POWER_ERROR_XFER_WARNIGN);
        msleep(1);
    }

    if (ret < 0)
    {
        power->err |= BIT(POWER_ERROR_XFER_ERROR);
        printk("dev:0x%x,dc_write_reg_one_byte failed ret = 0x%x\n", addr, ret);
    }

    return 0;
}

static int dc_init_client(struct dc_data *data)
{
    int ret;
    unsigned char buf[8];

    mutex_lock(&data->mutex);

    buf[0] = 0; //start addr
    ret = i2c_read_bytes(data->client, data->client->addr, buf, 1 + 2);
    if (ret < 0)
    {
        printk("i2c_read_bytes 1 failed addr: %x ret = %d\n", data->client->addr, ret);
        goto exit_err;
    }
    if ((buf[1] == EEPROM_TAG1) && (buf[2] == EEPROM_TAG2))
    {
        printk("0x%x is eeprom device\n", data->client->addr);
        goto exit_err;
    }

    buf[0] = 1; //start addr
    buf[1] = 0x55;
    buf[2] = 0xaa;
    ret = i2c_write_bytes(data->client, data->client->addr, buf, 1 + 2);
    if (ret < 0)
    {
        printk("dev:0x%x,i2c_write_bytes failed ret = 0x%x\n", data->client->addr, ret);
        goto exit_err;
    }
    buf[0] = 1; //start addr
    ret = i2c_read_bytes(data->client, data->client->addr, buf, 1 + 2);
    if (ret < 0)
    {
        printk("i2c_read_bytes 2 failed ret = 0x%x\n", ret);
        goto exit_err;
    }
    if ((buf[1] == 0x55) && (buf[2] == 0xaa))
    {
        printk("0x%x is eeprom device\n", data->client->addr);
        goto exit_err;
    }

    mutex_unlock(&data->mutex);
    return 0;
exit_err:
    mutex_unlock(&data->mutex);
    return -1;
}

/*
 * I2C init/probing/exit functions
 */
static int dc_power_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct dc_data *data;
    static int inited = 0;
    int err, i;

    dev_info(&client->dev, "Power supply dc-dc probe start\n");

    data = kzalloc(sizeof(struct dc_data), GFP_KERNEL);
    if (!data)
    {
        return -ENOMEM;
    }
    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->mutex);

    if (dc_init_client(data) < 0)
    {
        err = -ENODEV;
        goto exit_kfree;
    }

    if (!inited)
    {
        if (dc_power_supply_init(opt) < 0)
        {
            err = -ENODEV;
            goto exit_kfree;
        }
        inited = 1;
    }

    err = -ENODEV;
    for (i = 0; i < NODE_TOTAL; i++)
    {
        if (client->addr == opt[i].addr)
        {
            gpio_direction_output(opt[i].io, opt[1].en ? 1 : 0);
            data->err = 0;
            opt[i].pdat = data;

            dev_info(&client->dev, "Power supply dc-dc probed ok.\n");
            return 0;
        }
    }

exit_kfree:
    kfree(data);
    return err;
}

static int dc_power_remove(struct i2c_client *client)
{
    struct dc_data *data = (struct dc_data *)i2c_get_clientdata(client);
    kfree(data);
    return 0;
}

static const struct i2c_device_id dc_power_id[] = {
    {"dc-dc"},
    {}};

MODULE_DEVICE_TABLE(i2c, dc_power_id);

static struct i2c_driver dc_power_driver = {
    .driver = {
        .name = "dc-dc",
        .owner = THIS_MODULE,
    },
    .probe = dc_power_probe,
    .remove = dc_power_remove,
    .id_table = dc_power_id,
};

module_i2c_driver(dc_power_driver);

MODULE_AUTHOR("passby");
MODULE_DESCRIPTION("dcdc power supply driver");
MODULE_LICENSE("GPL");
