#include "dcdc_power.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>

extern int is_s9_zynq(void);

char *type_name[] = {
    "Non",
    "isl23315",
    "cat5140",  //upgrade-whatsminer->detect-platform.sh
};

char hw_version[HW_DESC_NAME_LEN];
char sw_version[] = "10";

struct dc_opt opt[NODE_TOTAL] = {
    [0] = {
        .name = "Non",
        .pdat = NULL,
        .type = TYPE_NONE,
        .vol = 255,
        .io = -1,
        .io_req = -1,
        .selio = -1,
        .sel_req = -1,
        .en = false,
    },

    [1] = {
        .name = "Non",
        .pdat = NULL,
        .type = TYPE_NONE,
        .vol = 255,
        .io = -1,
        .io_req = -1,
        .selio = -1,
        .sel_req = -1,
        .en = false,
    },

    [2] = {
        .name = "Non",
        .pdat = NULL,
        .type = TYPE_NONE,
        .vol = 255,
        .io = -1,
        .io_req = -1,
        .selio = -1,
        .sel_req = -1,
        .en = false,
    },
};

static int i2c_read_bytes(struct i2c_client *client, unsigned char *buf,
        unsigned char len)
{
    struct i2c_msg msgs[2];

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr = client->addr;
    msgs[0].len = 1;
    msgs[0].buf = buf;

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = client->addr;
    msgs[1].len = len - 1;
    msgs[1].buf = buf + 1;

    return i2c_transfer(client->adapter, msgs, 2);
}

static int i2c_write_bytes(struct i2c_client *client, const unsigned char *buf,
        unsigned char len)
{
    struct i2c_msg msg;

    msg.flags = !I2C_M_RD;
    msg.addr = client->addr;
    msg.len = len;
    msg.buf = (unsigned char *)buf;

    return i2c_transfer(client->adapter, &msg, 1);
}

int dc_read_reg_one_byte(struct dc_data *power, unsigned char reg,
        unsigned char *val)
{
    int ret = -1;
    int retry = 3;
    unsigned char buf[2];

    buf[0] = reg;
    while (retry--)
    {
        ret = i2c_read_bytes(power->client, buf, 1 + 1);
        if (ret > 0)
        {
            *val = buf[1];
            break;
        }
        if (retry < 2)
            power->err |= BIT(POWER_ERROR_XFER_WARNIGN);
        msleep(1);      
    }

    if (ret <= 0)
    {
        ret = -1;
        power->err |= BIT(POWER_ERROR_XFER_ERROR);
        printk("dev:0x%x,dc_read_reg_one_byte failed ret = 0x%x\n",
                power->client->addr, ret);
    }

    return ret;
}

int dc_write_reg_one_byte(struct dc_data *power, unsigned char reg,
        unsigned char val)
{
    int ret = -1;
    int retry = 3;
    unsigned char buf[2];
    buf[0] = reg;
    buf[1] = val;

    while (retry--)
    {
        ret = i2c_write_bytes(power->client, buf, 1 + 1);
        if (ret > 0)
        {
            break;
        }

        if (retry < 2)
            power->err |= BIT(POWER_ERROR_XFER_WARNIGN);
        msleep(1);           
    }

    if (ret <= 0)
    {
        ret = -1;
        power->err |= BIT(POWER_ERROR_XFER_ERROR);
        printk("dev:0x%x,dc_write_reg_one_byte failed ret = 0x%x\n",
                power->client->addr, ret);
    }

    return ret;
}

void cat5410_sel_switch(int selio)
{
    int i;
    for (i = 0; i < NODE_TOTAL; i++)
    {
        if (opt[i].sel_req != -1)
        {
            gpio_direction_output(opt[i].selio, CAT_GPIO_SEL_DISABLE);
        }
    }

    if (opt[selio].sel_req != -1)
        gpio_direction_output(selio, CAT_GPIO_SEL_ENABLE);
}

static int cat5410_get_dts_node(struct dc_data *data)
{
    int i;
    char tmpbuf[16];
    const __be32 *sel_be, *io_be;

    for (i = 0; i< NODE_TOTAL; i++)
    {
        if (is_s9_zynq())
            snprintf(tmpbuf,sizeof(tmpbuf),"s9_sel%d",i);
        else
            snprintf(tmpbuf,sizeof(tmpbuf),"sel%d",i);
        sel_be = of_get_property(data->client->dev.of_node, tmpbuf, NULL);
        if (!sel_be)
            return -1;
        opt[i].selio = be32_to_cpup(sel_be);

        if (is_s9_zynq())
            snprintf(tmpbuf,sizeof(tmpbuf),"s9_io%d",i);
        else
            snprintf(tmpbuf,sizeof(tmpbuf),"io%d",i);
        io_be = of_get_property(data->client->dev.of_node, tmpbuf, NULL);
        if (!io_be)
            return -1;
        opt[i].io = be32_to_cpup(io_be);
           
    }
    return 0;
}

static int request_io(int io)
{
    char name[8];
    snprintf(name, sizeof(name), "io%d", io);
    if (gpio_request(io, name) < 0)
        return -1;
    return 0;
}

static void free_io(int io)
{
    gpio_free(io);
}

static int dc_isl23315_init(struct dc_data *data)
{
    const __be32 *slot_be, *io_be;
    int slot, io;
    char *name;

    // get dts node data
    if (is_s9_zynq())
        name = "s9_slot";
    else
        name = "slot";
    slot_be = of_get_property(data->client->dev.of_node, name, NULL);
    if (!slot_be)
        return -1;
    slot = be32_to_cpup(slot_be);

    if (is_s9_zynq())
        name = "s9_io";
    else
        name = "io";
    io_be = of_get_property(data->client->dev.of_node, name, NULL);
    if (!io_be)
        return -1;
    io = be32_to_cpup(io_be);

    if (slot >= NODE_TOTAL)
        return -1;

    if (opt[slot].io_req != -1)
        return -1;
    if (request_io(io) < 0)
    {
        dev_info(&data->client->dev, "dc-dc isl23315 request io(%d) fail\n", io);
        return -1;
    }
    opt[slot].io_req = 1;

    data->err = 0;
    opt[slot].name = type_name[TYPE_ISL23315];
    opt[slot].pdat = data;
    opt[slot].type = TYPE_ISL23315;
    opt[slot].io = io;
    dc_power_set_vout(&opt[slot]);
    gpio_direction_output(opt[slot].io, 
            opt[slot].en ? POWER_GPIO_ENABLE : POWER_GPIO_DISABLE);
    return 0;
}

static int request_sel(int sel)
{
    char name[8];
    snprintf(name, sizeof(name), "sw%d", sel);
    if (gpio_request(sel, name) < 0)
        return -1;
    return 0;
}

static void free_sel(int sel)
{
    gpio_free(sel);
}

static int dc_detect_and_init(struct dc_data *data)
{
    int this_req[NODE_TOTAL];
    int ret, slot, cat_cnt, i;
    int retry = 2;
    unsigned char buf[8];

    mutex_lock(&data->mutex);

l_retry:

    buf[0] = 0; //start addr
    ret = i2c_read_bytes(data->client, buf, 1 + 2);
    if (ret < 0)
    {
        printk("i2c_read_bytes 1 failed addr: %x ret = %d\n",
                data->client->addr, ret);
        goto test_cat5140;
    }
    if ((buf[1] == EEPROM_TAG1) && (buf[2] == EEPROM_TAG2))
        goto exit_eeprom;

    buf[0] = 0x20; //detect addr, cat5140 can't write 0x20
    buf[1] = 0x55;
    buf[2] = 0xaa;
    ret = i2c_write_bytes(data->client, buf, 1 + 2);
    if (ret < 0)
    {
        printk("dev:0x%x,i2c_write_bytes failed ret = 0x%x\n",
                data->client->addr, ret);
        goto test_cat5140;
    }

    buf[0] = 0x20; //start addr
    ret = i2c_read_bytes(data->client, buf, 1 + 2);
    if (ret < 0)
    {
        printk("i2c_read_bytes 2 failed ret = 0x%x\n", ret);
        goto test_cat5140;
    }
    if ((buf[1] == 0x55) && (buf[2] == 0xaa))
        goto exit_eeprom;

    goto exit_isl;

test_cat5140:   //detect and init

    if (cat5410_get_dts_node(data) < 0)
        goto exit_err;

    memset(this_req, 0, sizeof(this_req));
    for (i = 0; i < NODE_TOTAL; i++)
    {
        if (opt[i].sel_req == -1)
        {
            if (request_sel(opt[i].selio) < 0)
            {
                dev_info(&data->client->dev, "dc-dc cat5410 request sel(%d) fail\n", opt[i].selio);
                continue;
            }
            opt[i].sel_req = 1;
            this_req[i] = 1;
        } 
    }

    for (slot = 0, cat_cnt = 0; slot < NODE_TOTAL; slot++)
    {     
        if (this_req[slot] == 0)
            continue;
        cat5410_sel_switch(opt[slot].selio);
        buf[0] = REG_DEV_ID; //device id reg
        if (i2c_read_bytes(data->client, buf, 1 + 1) < 0)//dc_read_reg_one_byte
            goto L_release;
        if (buf[1] == CAT_DEVICE_ID)
        {
            #define GENERAL_REG_TOTAL   5
            //test general purpose reg
            buf[0] = 0x02; //start addr
            if (i2c_read_bytes(data->client, buf, 1 + GENERAL_REG_TOTAL) < 0)
                goto L_release;
            for (i = 0; i < GENERAL_REG_TOTAL; i++)
                if (buf[i+1] != 0x0)
                    break;
            if (i < GENERAL_REG_TOTAL)
                goto L_release;
            if (opt[slot].io_req == -1)
            {
                if (request_io(opt[slot].io) < 0)
                {
                    dev_info(&data->client->dev, "dc-dc cat5410 request io(%d) fail\n", opt[slot].io);
                    goto L_release;            
                }
                opt[slot].io_req = 1;
            }
            else
                goto L_release;
            data->err = 0;
            opt[slot].name = type_name[TYPE_CAT5140];
            opt[slot].type = TYPE_CAT5140;
            opt[slot].pdat = data;//3 cat5140 opt share 1 dc_data        
            dc_write_reg_one_byte(opt[slot].pdat, REG_ACR, CAT_VOL);
            dc_power_set_vout(&opt[slot]);
            gpio_direction_output(opt[slot].io, 
                    opt[slot].en ? POWER_GPIO_ENABLE : POWER_GPIO_DISABLE);
            cat_cnt++;          
            continue;
        }        
        L_release:    
        free_sel(opt[slot].selio);      
        opt[slot].sel_req = -1;
    }

    if (cat_cnt > 0)
    {
        printk("0x%x is cat5140 dcdc device (%d)\n", data->client->addr, cat_cnt);
        mutex_unlock(&data->mutex);
        return TYPE_CAT5140;        
    }
    goto exit_err;
exit_isl:
    dev_info(&data->client->dev, " is isl23315 device .\n");
    mutex_unlock(&data->mutex);
    ret = dc_isl23315_init(data);
    if (ret < 0)
        goto exit_err;
    return TYPE_ISL23315;  
exit_eeprom:
    printk("0x%x is eeprom device\n", data->client->addr);
    mutex_unlock(&data->mutex);
    return -1;
exit_err:   //eeprom or dev error
    if (retry-- > 0)
        goto l_retry;
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
    int err;

    data = kzalloc(sizeof(struct dc_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->mutex);

    err = -ENODEV;
    if (dc_detect_and_init(data) < 0)
        goto exit_kfree;

    if (!inited)
    {
        if (dc_power_supply_init(opt) < 0)
            goto exit_kfree;
        inited = 1;
    }

    return 0;
exit_kfree:
    kfree(data);
    return err;
}

static int dc_power_remove(struct i2c_client *client)
{
    int i;
    struct dc_data *data = (struct dc_data *)i2c_get_clientdata(client);
    for (i = 0; i< NODE_TOTAL; i++)
    {
        if (opt[i].sel_req != -1)
            free_sel(opt[i].selio);
        if (opt[i].io_req != -1)
            free_io(opt[i].io);
    }
    kfree(data);
    return 0;
}

static const struct i2c_device_id dc_power_id[] = {
    {"dc-dc"},
    {}
};

static const struct of_device_id dc_power_of_match[] = {
    {.compatible = "isl,dc-dc"},
    {}
};


MODULE_DEVICE_TABLE(i2c, dc_power_id);

static struct i2c_driver dc_power_driver = {
    .driver = {
        .name = "dc-dc",
        .owner = THIS_MODULE,
        .of_match_table = dc_power_of_match,
    },
    .probe = dc_power_probe,
    .remove = dc_power_remove,
    .id_table = dc_power_id,
};

module_i2c_driver(dc_power_driver);

MODULE_AUTHOR("passby");
MODULE_DESCRIPTION("dcdc power supply driver");
MODULE_LICENSE("GPL");
