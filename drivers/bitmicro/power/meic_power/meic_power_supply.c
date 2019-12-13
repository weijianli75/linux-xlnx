#include "meic_power.h"


static int meic_power_get_property(struct bitmicro_power_supply *psy,
                      enum power_supply_property psp,
                      union power_supply_propval *val)
{
    int ret = 0;
    struct meic_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    if (power->is_upgrading)
        return -EBUSY;

    switch (psp) {
    case POWER_SUPPLY_VOUT:
        val->intval = meic_power_get_voltage(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT_SET:
        val->intval = meic_power_get_voltage_set(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT_MAX:
        val->intval = meic_power_get_vout_max(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT_MIN:
        val->intval = meic_power_get_vout_min(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_IOUT:
        val->intval = meic_power_get_current(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_STATUS:
        val->intval = meic_power_get_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ENABLE:
        val->intval = meic_power_get_enable(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ERRORS:
        val->intval = meic_power_get_status(power);
        if (val->intval < 0)
            ret = -1;
        val->intval &= ~(1);   //clear power enable bit
        break;
    case POWER_SUPPLY_COMMON_ERRORS:
        val->intval = meic_power_get_common_error(power);
        break;
    case POWER_SUPPLY_COMMON_STATUS:
        val->intval = meic_power_get_common_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_FAN_SPEED0:
        val->intval = meic_power_get_fan_speed(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_HW_VERSION:
        val->intval = meic_power_get_hw_version(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_SW_VERSION: {
        int version = meic_power_get_sw_version(power);
        if (version < 0)
            ret = -1;
        sprintf(power->sw_version, "%d", version);
        val->strval = power->sw_version;
        break;
    }
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static int meic_power_set_property(struct bitmicro_power_supply *psy,
        enum power_supply_property psp,
        const union power_supply_propval *val)
{
    int ret = 0;
    struct meic_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
        break;
    case POWER_SUPPLY_VOUT_SET:
        ret = meic_power_set_voltage(power, val->intval);
        break;
    case POWER_SUPPLY_VOUT_MAX:
        ret = meic_power_set_vout_max(power, val->intval);
        break;
    case POWER_SUPPLY_VOUT_MIN:
        ret = meic_power_set_vout_min(power, val->intval);
        break;
    case POWER_SUPPLY_ENABLE:
        ret = meic_power_set_enable(power, val->intval);
        break;
    case POWER_SUPPLY_UPGRADE:
        ret = meic_power_upgrade(power, val->strval);
        break;
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static enum power_supply_property meic_power_props[] = {
        POWER_SUPPLY_VOUT,
        POWER_SUPPLY_VOUT_SET,
        POWER_SUPPLY_VOUT_MAX,
        POWER_SUPPLY_VOUT_MIN,
        POWER_SUPPLY_IOUT,
        POWER_SUPPLY_POUT,
        POWER_SUPPLY_STATUS,
        POWER_SUPPLY_ENABLE,
        POWER_SUPPLY_ERRORS,
        POWER_SUPPLY_COMMON_STATUS,
        POWER_SUPPLY_COMMON_ERRORS,
        POWER_SUPPLY_FAN_SPEED0,
        POWER_SUPPLY_HW_VERSION,
        POWER_SUPPLY_SW_VERSION,
        POWER_SUPPLY_UPGRADE,
};

static struct power_supply_desc power_desc[] = {
    [0] = {
        .vender = VENDER_MEIC,
        .name = "P10",
        .model = "P10-12-2200-C",
        .properties = meic_power_props,
        .num_properties = ARRAY_SIZE(meic_power_props),
        .get_property = meic_power_get_property,
        .set_property = meic_power_set_property,
    },
};

int meic_power_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct meic_power *power;
    struct bitmicro_power_supply *psy;

    power = kzalloc(sizeof(struct meic_power), GFP_KERNEL);
    if (!power) {
        return -ENOMEM;
    }

    power->client = client;
    i2c_set_clientdata(client, power);
    mutex_init(&power->mutex);

//    ret = meci_power_is_upgrading(power);
//    if (ret < 0) {
//        printk("%s: read power upgrade state error, ret: 0x%x\n", __func__, ret);
////        goto exit;
//    }

    ret = meic_power_get_sw_version(power);
    if (ret < 0) {
        printk("%s: read power version error, ret: 0x%x\n", __func__, ret);
        goto exit;
    }

    psy = bitmicro_power_supply_register(power_desc);
    if (psy == NULL)
        goto exit;
    psy->drv_data = power;

    printk("%s: probe success, version: 0x%x\n", __func__, ret);


    return 0;
exit:
    kfree(power);
    return -1;
}

static int meic_power_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id meic_power_id[] = {
    { "meic-power" },
    { }
};
MODULE_DEVICE_TABLE(i2c, meic_power_id);

static struct i2c_driver meic_power_driver = {
    .driver = {
        .name   = "meic-power",
        .owner  = THIS_MODULE,
    },
    .probe  = meic_power_probe,
    .remove = meic_power_remove,
    .id_table = meic_power_id,
};

module_i2c_driver(meic_power_driver);
