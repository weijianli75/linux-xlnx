#include "pmbus_power.h"

static void pmbus_set_power_sensor(struct pmbus_power *power, char page,
        int class, short reg, int data)
{
    power->sensor.page = page;
    power->sensor.class = class;
    power->sensor.reg = reg;
    power->sensor.data = data;
}

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
        unsigned char *data, unsigned int len, int for_read)
{
    int i;
    unsigned char crc = 0;
    crc = basic_crc(crc, (i2c_addr << 1) + 0);
    crc = basic_crc(crc, i2c_command);
    if (for_read)
        crc = basic_crc(crc, (i2c_addr << 1) + 1);
    for (i = 0; i < len; i++) {
        crc = basic_crc(crc, *data++);
    }
    return crc;
}

static u8 calc_read_crc(unsigned char i2c_addr, unsigned char i2c_command,
        unsigned char *data, unsigned int len)
{
    return calc_crc(i2c_addr, i2c_command, data, len, 1);
}


static u8 calc_write_crc(unsigned char i2c_addr, unsigned char i2c_command,
        unsigned char *data, unsigned int len)
{
    return calc_crc(i2c_addr, i2c_command, data, len, 0);
}

s32 pmbus_i2c_smbus_read_i2c_block_data(struct pmbus_power *power,
        u8 command, u8 length, u8 *values)
{
    struct i2c_client *client = power->client;
    int retry_count = 10;
    s32 status = 0;
    char rbuf[length + 1];
    u8 crc_calc = 0;
    int common_errors = 0;

    while (retry_count--) {
        status = i2c_smbus_read_i2c_block_data(client, command, sizeof(rbuf), rbuf);
        if (status >= 0) {
            crc_calc = calc_read_crc(client->addr, command, rbuf, length);
            if (crc_calc == rbuf[length])
                break;
            printk("%s: crc checking error, command = 0x%x, crc_read = 0x%02x, crc_calc = 0x%02x\n",
                    __func__, command, rbuf[length], crc_calc);
            common_errors |= BIT(POWER_ERROR_XFER_WARNIGN);
        }
        msleep(1);
    }

    if (retry_count < 0) {
        common_errors |= BIT(POWER_ERROR_XFER_ERROR);
        status = -1;
    } else
        memcpy(values, rbuf, length);

    power->common_errors = common_errors;

    return status;
}

static s32 pmbus_i2c_smbus_read_word_data(struct pmbus_power *power, u8 command)
{
    struct i2c_client *client = power->client;
    int retry_count = 10;
    s32 status = 0;
    u8 crc_calc = 0;
    int common_errors = 0;

    while (retry_count--) {
        status = i2c_smbus_read_word_data(client, command);
        if (status >= 0) {
            crc_calc = calc_read_crc(client->addr, command,
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

    power->common_errors = common_errors;

    return status;
}

static s32 pmbus_power_i2c_write(struct i2c_client *client, void *buf, int size)
{
    int ret;
    struct i2c_msg wmsg = {
        .addr = client->addr,
        .flags = 0,
        .buf = buf,
        .len = size,
    };

    ret = i2c_transfer(client->adapter, &wmsg, 1);
    if (ret != 1) {
        pr_err("%s: write buf error, ret: %d!\n", __func__, ret);
        return -1;
    }
    return 0;
}

s32 pmbus_power_write(struct pmbus_power *power, u8 command, void *buf, int size)
{
    int i;
    int ret;
    int retrys = 3;
    char wbuf[size + 2]; /* 2 bytes are command address and crc */
    int common_error = 0;
    struct i2c_client *client = power->client;

    wbuf[0] = command;
    for (i = 0; i < size; i++)
        wbuf[1 + i] = ((char*) buf)[i];

    wbuf[size + 1] = calc_write_crc(client->addr, command, (unsigned char*) buf, size);
    do {
        ret = pmbus_power_i2c_write(client, wbuf, sizeof(wbuf));
        if (ret == 0)
            break;
        common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (retrys-- > 0);

    if (ret < 0) {
        common_error |= BIT(POWER_ERROR_XFER_ERROR);
        pr_err("%s: failed to write command: 0x%x\n", __func__, command);
    }

    power->common_errors = common_error;

    return ret;
}

//------------------------------------------------------------------------------//

char pmbus_power_read_byte(struct pmbus_power *power, char reg)
{
    int ret;

    ret = pmbus_i2c_smbus_read_word_data(power, reg);
    if (ret < 0)
        return ret;

    return ret & 0xff;
}

short pmbus_power_read_word(struct pmbus_power *power, char reg)
{
    u16 data;

    int ret = pmbus_i2c_smbus_read_i2c_block_data(power, reg, sizeof(data), (u8*) &data);
    if (ret < 0)
        return ret;

    return data;
}

int pmbus_power_write_byte(struct pmbus_power *power, char reg, char data)
{
    return pmbus_power_write(power, reg, &data, 1);
}

int pmbus_power_write_word(struct pmbus_power *power, char reg, short data)
{
    char wbuf[2];
    wbuf[0] = data & 0xff;
    wbuf[1] = data >> 8;
    return pmbus_power_write(power, reg, wbuf, 2);
}

// Block Read means that PSU return first byte is data length.
int pmbus_power_block_read(struct pmbus_power *power, int reg, int len, char *buf)
{
    int ret;
    int actual_len;
    char data[len + 1];

    // Read actual data length first
    actual_len = i2c_smbus_read_byte_data(power->client, reg);
    if (actual_len < 0)
        return actual_len;

    if (actual_len > len) {
        printk("pmbus_power_block_read warning: reg = 0x%x, actual data len (%d) > buffer length (%d)\n",
                reg, actual_len, len);
        return 0;
    }

    memset(buf, 0, len);
    memset(data, 0, len  + 1);
    ret = pmbus_i2c_smbus_read_i2c_block_data(power, reg, actual_len + 1, data); // Add the first length byte
    if (ret < 0)
        return ret;

    memmove(buf, data + 1, actual_len); /* skip the first length byte */

    return actual_len;
}

//-------------------------------------------------------------------------------//

int pmbus_power_get_enable(struct pmbus_power *power)
{
    u8 data = pmbus_power_read_byte(power, PMBUS_OPERATION);
    return (data == 0x80) ? 1 : 0;
}

int pmbus_power_set_enable(struct pmbus_power *power, int enable)
{
    u8 data = (enable > 0) ? 0x80 : 0x00;
    return pmbus_power_write_byte(power, PMBUS_OPERATION, data);
}

int pmbus_power_get_voltage_in(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_VIN);
    pmbus_set_power_sensor(power, 0, PSC_VOLTAGE_IN, PMBUS_READ_VIN, data);
    return DIV_ROUND_CLOSEST(pmbus_reg2data(power, &power->sensor), 10);
}

int pmbus_power_get_current_in(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_IIN);
    pmbus_set_power_sensor(power, 0, PSC_CURRENT_IN, PMBUS_READ_IIN, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_power_in(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_PIN);
    pmbus_set_power_sensor(power, 0, PSC_POWER, PMBUS_READ_PIN, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_voltage_out(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_VOUT);
    pmbus_set_power_sensor(power, 0, PSC_VOLTAGE_OUT, PMBUS_READ_VOUT, data);
    return DIV_ROUND_CLOSEST(pmbus_reg2data(power, &power->sensor), 10);
}

int pmbus_power_get_voltage_out_set(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_VOUT_COMMAND);
    pmbus_set_power_sensor(power, 0, PSC_VOLTAGE_OUT, PMBUS_VOUT_COMMAND, data);
    return DIV_ROUND_CLOSEST(pmbus_reg2data(power, &power->sensor), 10);
}

int pmbus_power_set_voltage(struct pmbus_power *power, int vol)
{
    u16 regval;

    vol *= 10;
    pmbus_set_power_sensor(power, 0, PSC_VOLTAGE_OUT, PMBUS_VOUT_COMMAND, vol);
    regval = pmbus_data2reg(power, &power->sensor, vol);
    return pmbus_power_write_word(power, PMBUS_VOUT_COMMAND, regval);
}

int pmbus_power_get_current_out(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_IOUT);
    pmbus_set_power_sensor(power, 0, PSC_CURRENT_OUT, PMBUS_READ_IOUT, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_power_out(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_POUT);
    pmbus_set_power_sensor(power, 0, PSC_POWER, PMBUS_READ_POUT, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_fan_speed(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_FAN_SPEED_1);
    pmbus_set_power_sensor(power, 0, PSC_FAN, PMBUS_READ_FAN_SPEED_1, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_temp0(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_TEMPERATURE_1);
    pmbus_set_power_sensor(power, 0, PSC_TEMPERATURE, PMBUS_READ_TEMPERATURE_1, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_temp1(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_TEMPERATURE_2);
    pmbus_set_power_sensor(power, 0, PSC_TEMPERATURE, PMBUS_READ_TEMPERATURE_2, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_temp2(struct pmbus_power *power)
{
    short data = pmbus_power_read_word(power, PMBUS_READ_TEMPERATURE_3);
    pmbus_set_power_sensor(power, 0, PSC_TEMPERATURE, PMBUS_READ_TEMPERATURE_3, data);
    return pmbus_reg2data(power, &power->sensor);
}

int pmbus_power_get_hw_version(struct pmbus_power *power, char *buf)
{
    memcpy(buf, power->hw_version, (int) strlen(power->hw_version));
    return strlen(power->hw_version);
}

int pmbus_power_get_sw_version(struct pmbus_power *power, char *buf)
{
    memcpy(buf, power->sw_version, (int) strlen(power->sw_version));
    return strlen(power->sw_version);
}

int pmbus_power_get_status(struct pmbus_power *power)
{
    return 0;
}

int pmbus_power_get_common_status(struct pmbus_power *power)
{
    int common_status = 0;
    u8 data;

    data = pmbus_power_read_byte(power, GPSPOWER_WATCHDOG_STATUS);
    if (data == 0x01)
        common_status |= BIT(POWER_STATUS_PROTECTED);

    return common_status;
}

int pmbus_power_get_errors(struct pmbus_power *power)
{
    return 0;
}

int pmbus_power_get_common_errors(struct pmbus_power *power)
{
    u8 status_vout = pmbus_power_read_byte(power, PMBUS_STATUS_VOUT);
    u8 status_iout = pmbus_power_read_byte(power, PMBUS_STATUS_IOUT);
    u8 status_input = pmbus_power_read_byte(power, PMBUS_STATUS_INPUT);
    u8 status_temp = pmbus_power_read_byte(power, PMBUS_STATUS_TEMPERATURE);
    u8 status_fan_12 = pmbus_power_read_byte(power, PMBUS_STATUS_FAN_12);

    int common_errors = power->common_errors;

    if (status_vout & PMBUS_STATUS_VOUT_UV_PROTECTED)
        common_errors |= BIT(POWER_ERROR_UNDER_VOUT);
    if (status_vout & PMBUS_STATUS_VOUT_OV_PROTECTED)
        common_errors |= BIT(POWER_ERROR_OVER_VOUT);

    if (status_iout & PMBUS_STATUS_IOUT_OP_PROTECTED)
        common_errors |= BIT(POWER_ERROR_OVER_IOUT0);
    if (status_iout & PMBUS_STATUS_IOUT_OC_WARNING)
        common_errors |= BIT(POWER_ERROR_OVER_IOUT1);
    if (status_iout & PMBUS_STATUS_IOUT_OC_PROTECTED)
        common_errors |= BIT(POWER_ERROR_OVER_IOUT2);

    if (status_input & PMBUS_STATUS_IN_OC_WARNING)
        common_errors |= BIT(POWER_ERROR_OVER_IIN0);
    if (status_input & PMBUS_STATUS_IN_OC_PROTECTED)
        common_errors |= BIT(POWER_ERROR_OVER_IIN1);
    if (status_input & PMBUS_STATUS_IN_UV_PROTECTED1)
        common_errors |= BIT(POWER_ERROR_UNDER_VIN0);
    if (status_input & PMBUS_STATUS_IN_UV_PROTECTED2)
        common_errors |= BIT(POWER_ERROR_UNDER_VIN1);
    if (status_input & PMBUS_STATUS_IN_OV_WARNING)
        common_errors |= BIT(POWER_ERROR_OVER_VIN0);
    if (status_input & PMBUS_STATUS_IN_OV_PROTECTED)
        common_errors |= BIT(POWER_ERROR_OVER_VIN1);

    if (status_temp & PMBUS_STATUS_TEMP_OT_WARNING)
        common_errors |= BIT(POWER_ERROR_OVER_TOUT0);
    if (status_temp & PMBUS_STATUS_TEMP_OT_PROTECTED)
        common_errors |= BIT(POWER_ERROR_OVER_TOUT1);

    if (status_fan_12 & PMBUS_STATUS_FAN_LOW_WARNING)
        common_errors |= BIT(POWER_ERROR_FAN0);
    if (status_fan_12 & PMBUS_STATUS_FAN_LOW_PROTECTED)
        common_errors |= BIT(POWER_ERROR_FAN1);

    return common_errors;
}

int pmbus_power_set_upgrade(struct pmbus_power *power, const char *filename)
{
    return pmbus_power_upgrade(power, filename);
}
