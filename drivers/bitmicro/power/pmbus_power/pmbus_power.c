#include "pmbus_power.h"

static int pmbus_power_get_property(struct bitmicro_power_supply *psy,
        enum power_supply_property psp, union power_supply_propval *val)
{
    int ret = 0;
    struct pmbus_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
    case POWER_SUPPLY_ENABLE:
        val->intval = pmbus_power_get_enable(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VIN:
        val->intval = pmbus_power_get_voltage_in(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_IIN:
        val->intval = pmbus_power_get_current_in(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_PIN:
        val->intval = pmbus_power_get_power_in(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT:
        val->intval = pmbus_power_get_voltage_out(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT_SET:
        val->intval = pmbus_power_get_voltage_out_set(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_IOUT:
        val->intval = pmbus_power_get_current_out(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_POUT:
        val->intval = pmbus_power_get_power_out(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_FAN_SPEED0:
        val->intval = pmbus_power_get_fan_speed(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_TEMP0:
        val->intval = pmbus_power_get_temp0(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_TEMP1:
        val->intval = pmbus_power_get_temp1(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_TEMP2:
        val->intval = pmbus_power_get_temp2(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_HW_VERSION:
        val->strval = power->hw_version;
        break;
    case POWER_SUPPLY_SW_VERSION:
        val->strval = power->sw_version;
        break;
    case POWER_SUPPLY_STATUS:
        val->intval = pmbus_power_get_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_COMMON_STATUS:
        val->intval = pmbus_power_get_common_status(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ERRORS:
        val->intval = pmbus_power_get_errors(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_COMMON_ERRORS:
        val->intval = pmbus_power_get_common_errors(power);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_UPGRADE:
        //pmbus_power_upgrade_read_status(power);
        break;
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static int pmbus_power_set_property(struct bitmicro_power_supply *psy,
        enum power_supply_property psp, const union power_supply_propval *val)
{
    int ret = 0;
    struct pmbus_power *power = psy->drv_data;

    mutex_lock(&power->mutex);

    switch (psp) {
    case POWER_SUPPLY_VOUT_SET:
        ret = pmbus_power_set_voltage(power, val->intval);
        break;
    case POWER_SUPPLY_ENABLE:
        ret = pmbus_power_set_enable(power, val->intval);
        break;
    case POWER_SUPPLY_UPGRADE:
        ret = pmbus_power_set_upgrade(power, val->strval);
        break;
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&power->mutex);
    return ret;
}

static enum power_supply_property pmbus_power_props[] = {
    POWER_SUPPLY_UPGRADE,
    POWER_SUPPLY_ENABLE,
    POWER_SUPPLY_VIN,
    POWER_SUPPLY_IIN,
    POWER_SUPPLY_PIN,
    POWER_SUPPLY_VOUT,
    POWER_SUPPLY_VOUT_SET,
    POWER_SUPPLY_IOUT,
    POWER_SUPPLY_POUT,
    POWER_SUPPLY_FAN_SPEED0,
    POWER_SUPPLY_TEMP0,
    POWER_SUPPLY_TEMP1,
    POWER_SUPPLY_TEMP2,
    POWER_SUPPLY_HW_VERSION,
    POWER_SUPPLY_SW_VERSION,
    POWER_SUPPLY_STATUS,
    POWER_SUPPLY_COMMON_STATUS,
    POWER_SUPPLY_ERRORS,
    POWER_SUPPLY_COMMON_ERRORS,
};

static struct power_supply_desc pmbus_power_desc[] = {
    [0] = {
    	.properties = pmbus_power_props,
        .num_properties = ARRAY_SIZE(pmbus_power_props),
        .get_property = pmbus_power_get_property,
        .set_property = pmbus_power_set_property,
    },
};

static int pmbus_power_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct pmbus_power *power;
    struct bitmicro_power_supply *psy;
    int ret;

    power = kzalloc(sizeof(struct pmbus_power), GFP_KERNEL);
    if (!power) {
        return -ENOMEM;
    }

    power->client = client;
    i2c_set_clientdata(client, power);
    mutex_init(&power->mutex);
    power->power_desc = pmbus_power_desc;

    ret = pmbus_init_power(power);
    if (ret < 0)
        goto exit;

    psy = bitmicro_power_supply_register(pmbus_power_desc);
    if (psy == NULL)
        goto exit;
    psy->drv_data = power;

    printk("%s probe success (vendor: 0x%x, name: %s)\n", __FUNCTION__,
            power->power_desc->vender, power->power_desc->name);

    printk("    mfr_id:         %s\n", power->mfr_id);
    printk("    mfr_model:      %s\n", power->mfr_model);
    printk("    mfr_revision:   %s\n", power->mfr_revision);
    printk("    mfr_location:   %s\n", power->mfr_location);
    printk("    mfr_date:       %s\n", power->mfr_date);
    printk("    mfr_serial:     %s\n", power->mfr_serial);
    printk("    hw version:     %s\n", power->hw_version);
    printk("    sw version:     %s\n", power->sw_version);
    printk("    pmbus_revision: 0x%02x\n", power->pmbus_revision);

    return 0;
exit:
    kfree(power);
    return -1;
}

static int pmbus_power_remove(struct i2c_client *client)
{
    struct pmbus_power *power = (struct pmbus_power*) i2c_get_clientdata(client);

    kfree(power);

    return 0;
}

static const struct i2c_device_id pmbus_power_id[] = {
    { "pmbus-power", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, pmbus_power_id);

static struct i2c_driver pmbus_power_driver = {
    .driver = {
    		.name = "pmbus-power",
			.owner = THIS_MODULE,
    },
    .probe = pmbus_power_probe,
    .remove = pmbus_power_remove,
    .id_table = pmbus_power_id,
};

module_i2c_driver( pmbus_power_driver);
