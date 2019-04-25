#include "p6_power.h"

static void debug(struct p6_data *data)
{
    int offset = 0;
    int i;

    data->status = p6_power_get_status(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "status\t\t= 0x%02x\n", data->status);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "enable\t\t= %d\n", data->status & 0x02 ? 1 : 0);

    data->errors = p6_power_get_errors(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "errors\t\t= 0x%08x\n", data->errors);

    /* voltage (0.01V) */
    data->voltage_v12 = p6_power_get_voltage_v12(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "vout_v12\t= %d\n", data->voltage_v12);

    data->voltage = p6_power_get_voltage(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "vout\t\t= %d\n", data->voltage);

    data->voltage_set = p6_power_get_voltage_set(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "vout_set\t= %d\n", data->voltage_set);

    /* current (A) */
    data->current = p6_power_get_current(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "iout\t\t= %d\n", data->current);

    /* power (W) */
    data->power_output = p6_power_get_power(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "pout\t\t= %d\n", data->power_output);

    /* temperature (0.1℃) */
    for (i = 0; i < 3; i++) {
        data->temp[i] = p6_power_get_temp(data, i);
        offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
                "temp%d\t\t= %d\n", i, data->temp[i]);
    }

    /* fan speed (RPM) */
    data->fan_speed = p6_power_get_fan(data);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "fan\t\t= %d\n", data->fan_speed);

    if (data->version == P20_VERSION) {
        for (i = 0; i < 3; i++) {
            data->ios[i] = p6_power_get_ios(data, i);
            offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
                    "iso%c\t\t= %d\n", 'A'+i, data->temp[i]);
        }
    }

    p6_power_get_sw_version(data, data->sw_version);
    offset += snprintf(data->buf + offset, sizeof(data->buf) - offset,
            "sw_version\t= %s\n", data->sw_version);
}

static int p6_power_get_property(struct bitmicro_power_supply *psy,
                      enum power_supply_property psp,
                      union power_supply_propval *val)
{
    int ret = 0;
    struct p6_data *data = psy->drv_data;

    mutex_lock(&data->update_mutex);

    switch (psp) {
    case POWER_SUPPLY_UPGRADE:
        val->intval = data->auto_update;
        break;
    case POWER_SUPPLY_VOUT:
        val->intval = p6_power_get_voltage(data);
        break;
    case POWER_SUPPLY_VOUT_SET:
        val->intval = p6_power_get_voltage_set(data);
        break;
    case POWER_SUPPLY_IOUT:
        val->intval = p6_power_get_current(data);
        break;
    case POWER_SUPPLY_POUT:
        val->intval = p6_power_get_power(data);
        break;
    case POWER_SUPPLY_STATUS:
        val->intval = p6_power_get_status(data);
        break;
    case POWER_SUPPLY_ENABLE:
        val->intval = p6_power_get_enable(data);
        break;
    case POWER_SUPPLY_ERRORS:
        val->intval = p6_power_get_errors(data);
        break;
    case POWER_SUPPLY_COMMON_STATUS:
        val->intval = p6_power_get_common_status(data);
        break;
    case POWER_SUPPLY_COMMON_ERRORS:
        val->intval = p6_power_get_common_errors(data);
        break;
    case POWER_SUPPLY_FAN_SPEED0:
        val->intval = p6_power_get_fan(data);
        break;
    case POWER_SUPPLY_TEMP0:
        val->intval = p6_power_get_temp(data, 0);
        break;
    case POWER_SUPPLY_TEMP1:
        val->intval = p6_power_get_temp(data, 1);
        break;
    case POWER_SUPPLY_TEMP2:
        val->intval = p6_power_get_temp(data, 2);
        break;
    case POWER_SUPPLY_SW_VERSION:
        p6_power_get_sw_version(data, data->sw_version);
        val->strval = data->sw_version;
        break;
    case POWER_SUPPLY_VERSION:
        val->intval = p6_power_get_version(data);
        break;
    case POWER_SUPPLY_DEBUG:
        debug(data);
        val->strval = data->buf;
        break;
    default:
        ret =  -EINVAL;
    }

    mutex_unlock(&data->update_mutex);

    return ret;
}

static int p6_power_set_property(struct bitmicro_power_supply *psy,
        enum power_supply_property psp,
        const union power_supply_propval *val)
{
    int ret = 0;
    struct p6_data *data = psy->drv_data;

    mutex_lock(&data->update_mutex);

    switch (psp) {
    case POWER_SUPPLY_UPGRADE:
        data->auto_update = val->intval;
        break;
    case POWER_SUPPLY_VOUT_SET:
        p6_power_set_voltage_set(data, val->intval);
        break;
    case POWER_SUPPLY_ENABLE:
        p6_power_set_enable(data, val->intval);
        break;
    default:
        ret = -EINVAL;
    }

    mutex_unlock(&data->update_mutex);
    return ret;
}

static enum power_supply_property test_power_ac_props[] = {
        POWER_SUPPLY_VOUT,
        POWER_SUPPLY_VOUT_SET,
        POWER_SUPPLY_IOUT,
        POWER_SUPPLY_POUT,
        POWER_SUPPLY_STATUS,
        POWER_SUPPLY_ENABLE,
        POWER_SUPPLY_ERRORS,
        POWER_SUPPLY_COMMON_STATUS,
        POWER_SUPPLY_COMMON_ERRORS,
        POWER_SUPPLY_FAN_SPEED0,
        POWER_SUPPLY_TEMP0,
        POWER_SUPPLY_TEMP1,
        POWER_SUPPLY_TEMP2,
        POWER_SUPPLY_VERSION,
        POWER_SUPPLY_SW_VERSION,
        POWER_SUPPLY_DEBUG,
};

static struct power_supply_desc power_desc[] = {
    [0] = {
        .vender = GOSPOWER,
        .name = "PX",
        .properties = test_power_ac_props,
        .num_properties = ARRAY_SIZE(test_power_ac_props),
        .get_property = p6_power_get_property,
        .set_property = p6_power_set_property,
    },
};

int p6_power_supply_init(struct p6_data *data) {
    struct bitmicro_power_supply *psy;
    switch (data->version) {
    case P10_VERSION:
        power_desc[0].name = "P10";
        break;
    case P11_VERSION:
        power_desc[0].name = "P11";
        break;
    case P20_VERSION:
        power_desc[0].name = "P20";
        break;
    case P21_VERSION:
        power_desc[0].name = "P21";
        break;
    default:
        /* default name: PX */
        break;
    }

    psy = bitmicro_power_supply_register(NULL, power_desc);
    if (psy == NULL)
        return -1;

    psy->drv_data = data;
    return 0;
}
