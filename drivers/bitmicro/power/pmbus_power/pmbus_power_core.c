#include "pmbus_power.h"

/*
 * Convert linear sensor values to milli- or micro-units
 * depending on sensor type.
 */
static long pmbus_reg2data_linear(struct pmbus_power *power,
        struct pmbus_sensor *sensor)
{
    s16 exponent;
    s32 mantissa;
    long val;

    if (sensor->class == PSC_VOLTAGE_OUT) { /* LINEAR16 */
        exponent = power->exponent[sensor->page];
        mantissa = (u16) sensor->data;
    } else { 			/* LINEAR11 */
        exponent = ((s16) sensor->data) >> 11;
        mantissa = ((s16) ((sensor->data & 0x7ff) << 5)) >> 5;
    }

    val = mantissa;

    /* scale voltage in & out */
    if (sensor->class == PSC_VOLTAGE_IN || sensor->class == PSC_VOLTAGE_OUT)
        val = val * 1000L;

    /* scale temperature */
    if (sensor->class == PSC_TEMPERATURE)
        val = val * 10L;

    if (exponent >= 0)
        val <<= exponent;
    else
        val >>= -exponent;

    return val;
}

/*
 * Convert direct sensor values to milli- or micro-units
 * depending on sensor type.
 */
static long pmbus_reg2data_direct(struct pmbus_power *power,
        struct pmbus_sensor *sensor)
{
    long val = (s16) sensor->data;
    long m, b, R;

    m = power->drv_info->m[sensor->class];
    b = power->drv_info->b[sensor->class];
    R = power->drv_info->R[sensor->class];

    if (m == 0)
        return 0;

    /* X = 1/m * (Y * 10^-R - b) */
    R = -R;
    /* scale result to milli-units for everything but fans */
    if (sensor->class != PSC_FAN) {
        R += 3;
        b *= 1000;
    }

    /* scale result to micro-units for power sensors */
    if (sensor->class == PSC_POWER) {
        R += 3;
        b *= 1000;
    }

    while (R > 0) {
        val *= 10;
        R--;
    }
    while (R < 0) {
        val = DIV_ROUND_CLOSEST(val, 10);
        R++;
    }

    return (val - b) / m;
}

/*
 * Convert VID sensor values to milli- or micro-units
 * depending on sensor type.
 */
static long pmbus_reg2data_vid(struct pmbus_power *power,
        struct pmbus_sensor *sensor)
{
    long val = sensor->data;
    long rv = 0;

    switch (power->drv_info->vrm_version) {
    case vr11:
        if (val >= 0x02 && val <= 0xb2)
            rv = DIV_ROUND_CLOSEST(160000 - (val - 2) * 625, 100);
        break;
    case vr12:
        if (val >= 0x01)
            rv = 250 + (val - 1) * 5;
        break;
    }
    return rv;
}

long pmbus_reg2data(struct pmbus_power *power, struct pmbus_sensor *sensor)
{
    long val;

    switch (power->drv_info->format[sensor->class]) {
    case direct:
        val = pmbus_reg2data_direct(power, sensor);
        break;
    case vid:
        val = pmbus_reg2data_vid(power, sensor);
        break;
    case linear:
    default:
        val = pmbus_reg2data_linear(power, sensor);
        break;
    }
    return val;
}

#define MAX_MANTISSA	(1023 * 1000)
#define MIN_MANTISSA	(511 * 1000)

static u16 pmbus_data2reg_linear(struct pmbus_power *power,
        struct pmbus_sensor *sensor, long val)
{
//	s16 exponent = 0, mantissa;
//	bool negative = false;

    /* simple case */
    if (val == 0)
        return 0;

    if (sensor->class == PSC_VOLTAGE_OUT) {
        /* LINEAR16 does not support negative voltages */
        if (val < 0)
            return 0;

        /*
         * For a static exponents, we don't have a choice
         * but to adjust the value to it.
         */
        if (power->exponent[sensor->page] < 0)
            val <<= -power->exponent[sensor->page];
        else
            val >>= power->exponent[sensor->page];
        val = DIV_ROUND_CLOSEST(val, 1000);
        return val & 0xffff;
    }

    printk("pmbus_data2reg_linear: unsupported sensor class %d\n", sensor->class);
    return 0;
}

static u16 pmbus_data2reg_direct(struct pmbus_power *power,
        struct pmbus_sensor *sensor, long val)
{
    long m, b, R;

    m = power->drv_info->m[sensor->class];
    b = power->drv_info->b[sensor->class];
    R = power->drv_info->R[sensor->class];

    /* Power is in uW. Adjust R and b. */
    if (sensor->class == PSC_POWER) {
        R -= 3;
        b *= 1000;
    }

    /* Calculate Y = (m * X + b) * 10^R */
    if (sensor->class != PSC_FAN) {
        R -= 3; /* Adjust R and b for data in milli-units */
        b *= 1000;
    }
    val = val * m + b;

    while (R > 0) {
        val *= 10;
        R--;
    }
    while (R < 0) {
        val = DIV_ROUND_CLOSEST(val, 10);
        R++;
    }

    return val;
}

static u16 pmbus_data2reg_vid(struct pmbus_power *power,
        struct pmbus_sensor *sensor, long val)
{
    val = clamp_val(val, 500, 1600);

    return 2 + DIV_ROUND_CLOSEST((1600 - val) * 100, 625);
}

u16 pmbus_data2reg(struct pmbus_power *power, struct pmbus_sensor *sensor,
        long val)
{
    u16 regval;

    switch (power->drv_info->format[sensor->class]) {
    case direct:
        regval = pmbus_data2reg_direct(power, sensor, val);
        break;
    case vid:
        regval = pmbus_data2reg_vid(power, sensor, val);
        break;
    case linear:
    default:
        regval = pmbus_data2reg_linear(power, sensor, val);
        break;
    }
    return regval;
}
