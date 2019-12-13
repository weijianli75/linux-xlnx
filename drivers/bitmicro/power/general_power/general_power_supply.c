#include "general_power.h"


static int general_power_get_property(struct bitmicro_power_supply *psy,
                      enum power_supply_property psp,
                      union power_supply_propval *val)
{
    int ret = 0;
    struct general_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
    case POWER_SUPPLY_VOUT:
        val->intval = general_power_get_voltage(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT_SET:
        val->intval = general_power_get_voltage_set(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_IOUT:
        val->intval = general_power_get_current(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_POUT:
        val->intval = general_power_get_power(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_STATUS:
        val->intval = general_power_get_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ENABLE:
        val->intval = general_power_get_enable(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ERRORS:
        val->intval = general_power_get_errors(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_COMMON_ERRORS:
        val->intval = general_power_get_common_error(power);
        break;
    case POWER_SUPPLY_COMMON_STATUS:
        val->intval = general_power_get_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_TEMP0:
        val->intval = general_power_get_temp(power, 0);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_TEMP1:
        val->intval = general_power_get_temp(power, 1);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_TEMP2:
        val->intval = general_power_get_temp(power, 2);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_FAN_SPEED0:
        val->intval = general_power_get_fan_speed(power, 0);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_HW_VERSION:
        ret = general_power_get_hw_version(power, power->hw_version);
        if (ret < 0)
            break;
        val->strval = power->hw_version;
        break;
    case POWER_SUPPLY_SW_VERSION:
        ret = general_power_get_sw_version(power, power->sw_version);
        if (ret < 0)
            break;
        val->strval = power->sw_version;
        break;
    case POWER_SUPPLY_MODEL:
        ret = general_power_get_model(power, power->model);
        if (ret < 0)
            break;
        val->strval = power->model;
        break;
    case POWER_SUPPLY_VENDER:
        ret = general_power_get_vender(power, power->vender);
        if (ret < 0)
            break;
        val->strval = power->vender;
        break;
    default:
        ret = -EINVAL;
        break;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static int general_power_set_property(struct bitmicro_power_supply *psy,
        enum power_supply_property psp,
        const union power_supply_propval *val)
{
    int ret = 0;
    struct general_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
    case POWER_SUPPLY_UPGRADE:
        ret = general_power_set_upgrade(power, val->strval);
        break;
    case POWER_SUPPLY_VOUT_SET:
        ret = general_power_set_voltage(power, val->intval);
        break;
    case POWER_SUPPLY_ENABLE:
        ret = general_power_set_enable(power, val->intval);
        break;
    case POWER_SUPPLY_ERRORS:
        ret = general_power_set_errors(power, val->intval);
        break;
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static enum power_supply_property general_power_props[] = {
        POWER_SUPPLY_UPGRADE,
        POWER_SUPPLY_VOUT,
        POWER_SUPPLY_VOUT_SET,
        POWER_SUPPLY_IOUT,
        POWER_SUPPLY_POUT,
        POWER_SUPPLY_TEMP0,
        POWER_SUPPLY_TEMP1,
        POWER_SUPPLY_TEMP2,
        POWER_SUPPLY_STATUS,
        POWER_SUPPLY_ENABLE,
        POWER_SUPPLY_ERRORS,
        POWER_SUPPLY_COMMON_STATUS,
        POWER_SUPPLY_COMMON_ERRORS,
        POWER_SUPPLY_FAN_SPEED0,
        POWER_SUPPLY_HW_VERSION,
        POWER_SUPPLY_SW_VERSION,
        POWER_SUPPLY_SERIAL_NO,
        POWER_SUPPLY_MODEL,
        POWER_SUPPLY_VENDER,
};

static struct power_supply_desc power_desc[] = {
    [0] = {
        .properties = general_power_props,
        .num_properties = ARRAY_SIZE(general_power_props),
        .get_property = general_power_get_property,
        .set_property = general_power_set_property,
    },
};

int general_power_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct general_power *power;
    struct bitmicro_power_supply *psy;

    power = kzalloc(sizeof(struct general_power), GFP_KERNEL);
    if (!power) {
        return -ENOMEM;
    }

    power->client = client;
    i2c_set_clientdata(client, power);
    mutex_init(&power->mutex);

    ret = general_power_get_sw_version(power, power->sw_version);
    if (ret < 0) {
        printk("%s: read power sw_version error, ret: %d\n", __func__, ret);
        goto exit;
    }

    ret = general_power_get_hw_version(power, power->hw_version);
    if (ret < 0) {
        printk("%s: read power hw_version error, ret: %d\n", __func__, ret);
        goto exit;
    }

    ret = general_power_get_vender(power, power->vender);
    if (ret < 0) {
        printk("%s: read power vender error, ret: %d\n", __func__, ret);
        goto exit;
    }

    ret = general_power_get_model(power, power->model);
    if (ret < 0) {
        printk("%s: read power model error, ret: %d\n", __func__, ret);
        goto exit;
    }

    if (strcmp(power->model, "B-3600A") == 0) {
        sprintf(power->model, "P21-12-3600-D");
    } else if (strcmp(power->model, "P21") == 0) {
        sprintf(power->model, "P21-12-3600-C");
    }

    power_supply_desc_init(power_desc, power->model, power->vender);
    power->vender_id = power_desc->vender;

    psy = bitmicro_power_supply_register(power_desc);
    if (psy == NULL)
        goto exit;
    psy->drv_data = power;

    printk("==============%s============\n"
            "name: %s\n"
            "vender: %s\n"
            "model: %s\n"
            "hw_version: %s\n"
            "sw_version: %s\n"
            "vendor id: %d\n"
            "====================================\n",
            __func__, power_desc->name, power->vender, power->model,
            power->hw_version, power->sw_version, power->vender_id);

    return 0;
exit:
    printk("%s: probe failed !\n", __func__);
    kfree(power);
    return -1;
}

static int general_power_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id general_power_id[] = {
    { "general_power" },
    { }
};
MODULE_DEVICE_TABLE(i2c, general_power_id);

static struct i2c_driver general_power_driver = {
    .driver = {
        .name   = "general_power",
        .owner  = THIS_MODULE,
    },
    .probe  = general_power_probe,
    .remove = general_power_remove,
    .id_table = general_power_id,
};

module_i2c_driver(general_power_driver);
