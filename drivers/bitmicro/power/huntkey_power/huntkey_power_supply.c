#include "huntkey_power.h"


static int huntkey_power_get_property(struct bitmicro_power_supply *psy,
                      enum power_supply_property psp,
                      union power_supply_propval *val)
{
    int ret = 0;
    struct huntkey_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
    case POWER_SUPPLY_VOUT:
        val->intval = huntkey_power_get_vout(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT_SET:
        val->intval = huntkey_power_get_vout_set(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_IOUT:
        val->intval = huntkey_power_get_iout(power) / 100;
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_STATUS:
        val->intval = huntkey_power_get_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_COMMON_ERRORS:
        val->intval = huntkey_power_get_common_error(power);
        break;
    case POWER_SUPPLY_COMMON_STATUS:
        val->intval = huntkey_power_get_common_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ENABLE:
        val->intval = huntkey_power_get_enable(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ERRORS:
        val->intval = huntkey_power_get_errors(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_FAN_SPEED0:
        val->intval = huntkey_power_get_fan(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_HW_VERSION:
        ret = huntkey_power_get_hw_version(power, power->hw_version);
        if (ret < 0)
            ret = -1;
        val->strval = power->hw_version;
        break;
    case POWER_SUPPLY_SW_VERSION: {
        ret = huntkey_power_get_sw_version(power, power->sw_version);
        if (ret < 0)
            break;
        val->strval = power->sw_version;
        break;
    }
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static int huntkey_power_set_property(struct bitmicro_power_supply *psy,
        enum power_supply_property psp,
        const union power_supply_propval *val)
{
    int ret = 0;
    struct huntkey_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
        break;
    case POWER_SUPPLY_VOUT_SET:
        power->vout_set = val->intval;
        ret = huntkey_power_set_vout_set(power, val->intval);
        break;
    case POWER_SUPPLY_ENABLE:
        ret = huntkey_power_set_enable(power, val->intval);
        break;
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static enum power_supply_property huntkey_power_props[] = {
        POWER_SUPPLY_VOUT,
        POWER_SUPPLY_VOUT_SET,
        POWER_SUPPLY_IOUT,
//        POWER_SUPPLY_POUT,
        POWER_SUPPLY_STATUS,
        POWER_SUPPLY_ENABLE,
        POWER_SUPPLY_ERRORS,
        POWER_SUPPLY_COMMON_ERRORS,
        POWER_SUPPLY_COMMON_STATUS,
//        POWER_SUPPLY_FAN_SPEED0,
        POWER_SUPPLY_HW_VERSION,
        POWER_SUPPLY_SW_VERSION,
};

static struct power_supply_desc power_desc[] = {
    [0] = {
        .vender = HUNTKEY,
        .name = "P10",
        .properties = huntkey_power_props,
        .num_properties = ARRAY_SIZE(huntkey_power_props),
        .get_property = huntkey_power_get_property,
        .set_property = huntkey_power_set_property,
    },
};

int huntkey_power_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct huntkey_power *power;
    struct bitmicro_power_supply *psy;

    power = kzalloc(sizeof(struct huntkey_power), GFP_KERNEL);
    if (!power) {
        return -ENOMEM;
    }

    power->client = client;
    i2c_set_clientdata(client, power);
    mutex_init(&power->mutex);

    ret = huntkey_power_get_enable(power);
    if (ret < 0) {
        printk("%s: read power error, ret: 0x%x\n", __func__, ret);
        goto exit;
    }

    psy = bitmicro_power_supply_register(NULL, power_desc);
    if (psy == NULL)
        return -1;
    psy->drv_data = power;

    printk("%s: probe success\n", __func__);

    return 0;
exit:
    kfree(power);
    return -1;
}

static int huntkey_power_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id huntkey_power_id[] = {
    { "huntkey-power" },
    { }
};
MODULE_DEVICE_TABLE(i2c, huntkey_power_id);

static struct i2c_driver huntkey_power_driver = {
    .driver = {
        .name   = "huntkey-power",
        .owner  = THIS_MODULE,
    },
    .probe  = huntkey_power_probe,
    .remove = huntkey_power_remove,
    .id_table = huntkey_power_id,
};

module_i2c_driver(huntkey_power_driver);
