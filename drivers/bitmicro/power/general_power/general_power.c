#include "general_power.h"

//------------------------------------------------------------------------------//

// Translate reg data to real value
static int ats_reg2data(short regval)
{
	int data = ((((regval >> 8) & 0xff) * 256) + (regval & 0xff)) / 10;
	return data;
}

// Translate real value to reg data
static short ats_data2reg(int data)
{
	short regval;
	regval = ((data * 10) & 0xff) | ((((data * 10) >> 8) & 0xff) << 8);
	return regval;
}

//------------------------------------------------------------------------------//

static char general_checksum(char reg, char *buf, int size) {
    int i;
    char sum = reg;

    for (i = 0; i < size; i++) {
        sum += buf[i];
    }

    return sum;
}

//static void dump_buf(char *buf, int size, int line) {
//    int i;
//
//    printk("=============%s:%d============\n", __func__, __LINE__);
//
//    for (i = 0; i < size; i++) {
//        printk("0x%x, ", buf[i]);
//        if (!((i+1) % 8))
//            printk("\n");
//    }
//
//    printk("\n=============end===============\n");
//}

int general_power_i2c_read(struct general_power *power, char reg, void *rbuf, int rsize)
{
    int ret;
    struct i2c_client *client = power->client;

//    printk("\n==== write: ");
//    dump_buf(wbuf, wsize, __LINE__);

    if (power->vender_id == VENDER_ATSTEK) {
        struct i2c_msg msg[2] = {
        	{
        		.addr = client->addr,
    			.flags = 0,
    			.buf = &reg,
    			.len = sizeof(reg),
        	},
    		{
    			.addr = client->addr,
    			.flags = I2C_M_RD,
    			.buf = rbuf,
    			.len = rsize,
    		}
        };

		ret = i2c_transfer(client->adapter, msg, 2);
		if (ret != 2) {
			printk("%s: write reg error, ret:%d !", __func__, ret);
			return -1;
		}
    }
    else {
        struct i2c_msg wmsg = {
            .addr = client->addr,
            .flags = 0,
            .buf = &reg,
            .len = sizeof(reg),
        };
        struct i2c_msg rmsg = {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .buf = rbuf,
            .len = rsize,
        };

		ret = i2c_transfer(client->adapter, &wmsg, 1);
		if (ret != 1) {
			printk("%s: write reg error, ret:%d !", __func__, ret);
			return -1;
		}

		ret = i2c_transfer(client->adapter, &rmsg, 1);
		if (ret != 1) {
			printk("%s: read reg error, ret:%d !", __func__, ret);
			return -1;
		}
    }
//    printk("\n==== read: ");
//    dump_buf(rbuf, rsize, __LINE__);
    return 0;
}

int general_power_i2c_write(struct general_power *power, void *buf, int size)
{
    int ret;
    struct i2c_client *client = power->client;
    struct i2c_msg wmsg = {
        .addr = client->addr,
        .flags = 0,
        .buf = buf,
        .len = size,
    };

//    printk("\n==== write: ");
//    dump_buf(buf, size, __LINE__);

    ret = i2c_transfer(client->adapter, &wmsg, 1);
    if (ret != 1) {
        printk("%s: write buf error, ret: %d !\n", __func__, ret);
        return -1;
    }
    return 0;
}

int general_power_read(struct general_power *power, char reg, void *buf, int size)
{
    int ret;
    int retrys = 3;
    char rbuf[size+1];
    int common_error = 0;

    do {
        ret = general_power_i2c_read(power, reg, rbuf, sizeof(rbuf));
        if (ret >= 0) {
//            printk("\n==== read: ");
//            dump_buf(rbuf, sizeof(rbuf), __LINE__);

            if (general_checksum(reg, rbuf, sizeof(rbuf) - 1) != rbuf[sizeof(rbuf)-1]) {
                printk("%s: read reg 0x%x general_checksum error !\n", __func__, reg);
                ret = -1;
            }
        }
        else
            common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        common_error |= BIT(POWER_ERROR_XFER_ERROR);
        printk("%s: failed to read reg: 0x%x\n", __func__, reg);
    } else
        memcpy(buf, rbuf, size);

    power->common_error = common_error;

    return ret;
}

int general_power_write(struct general_power *power, char reg, void *buf, int size)
{
    int i;
    int ret;
    int retrys = 3;
    char wbuf[size+2];
    int common_error = 0;

    wbuf[0] = reg;
    for (i = 0; i < size; i++) {
        wbuf[1+i] = ((char *)buf)[i];
    }
    wbuf[sizeof(wbuf)-1] = general_checksum(0, wbuf, sizeof(wbuf)-1);

    do {
        ret = general_power_i2c_write(power, wbuf, sizeof(wbuf));
        if (ret < 0)
            common_error |= BIT(POWER_ERROR_XFER_WARNIGN);
    } while (ret < 0 && retrys-- > 0);

    if (ret < 0) {
        common_error |= BIT(POWER_ERROR_XFER_ERROR);
        printk("%s: failed to write reg: 0x%x\n", __func__, reg);
    }

    power->common_error = common_error;

    return ret;
}

char general_power_read_byte(struct general_power *power, char reg)
{
    int ret;
    char data;

    ret = general_power_read(power, reg, &data, sizeof(data));
    if (ret < 0)
        return -1;

    return data;
}

short general_power_read_word(struct general_power *power, char reg)
{
    int ret;
    char data[2];

    ret = general_power_read(power, reg, data, sizeof(data));
    if (ret < 0)
        return -1;

    if (power->vender_id == VENDER_ATSTEK)
    	return *(short *)data;
    else
    	return swab16(*(short *)data);
}

short general_power_write_word(struct general_power *power, char reg, short data)
{
    short tmp;
    if (power->vender_id == VENDER_ATSTEK)
    	tmp = data;
    else
    	tmp = swab16(data);
    return general_power_write(power, reg, &tmp, sizeof(tmp));
}

//------------------------------------------------------------------------------//

int general_power_get_enable(struct general_power *power)
{
    int ret;
    char enable;

    ret = general_power_read(power, REG_ENABLE, &enable, 1);
    if (ret < 0)
        return ret;

    return enable;
}

int general_power_set_enable(struct general_power *power, int enable)
{
   enable = !!enable;
   return general_power_write(power, REG_ENABLE, &enable, 1);
}

int general_power_get_voltage(struct general_power *power)
{
	short regval = general_power_read_word(power, REG_VOUT);
	if (regval < 0)
		return -1;

	if (power->vender_id == VENDER_ATSTEK)
		return ats_reg2data(regval);
	else
		return regval;
}

int general_power_get_voltage_set(struct general_power *power)
{
	short regval = general_power_read_word(power, REG_VOUT_SET);
	if (regval < 0)
		return -1;

	if (power->vender_id == VENDER_ATSTEK)
		return ats_reg2data(regval);
	else
		return regval;
}

int general_power_set_voltage(struct general_power *power, int vol)
{
	int ret;
	short regval;

	if (power->vender_id == VENDER_ATSTEK)
		regval = ats_data2reg(vol);
	else
		regval = vol;
    ret = general_power_write_word(power, REG_VOUT_SET, regval);
    msleep(5);

    return ret;
}

int general_power_get_current(struct general_power *power)
{
    return general_power_read_word(power, REG_IOUT);
}

int general_power_get_power(struct general_power *power)
{
    return general_power_read_word(power, REG_POWER);
}

int general_power_get_status(struct general_power *power)
{
    int ret;
    int status;

    ret = general_power_read(power, REG_STATUS, &status, sizeof(status));
    if (ret < 0)
        return ret;

    return status;
}

int general_power_get_errors(struct general_power *power)
{
    int ret;
    int errors;

    ret = general_power_read(power, REG_ERRORS, &errors, sizeof(errors));
    if (ret < 0)
        return ret;

    return errors;
}

int general_power_set_errors(struct general_power *power, int val)
{
    int ret;

    ret = general_power_write(power, REG_ERRORS, &val, sizeof(val));
    if (ret < 0)
        return ret;

    return 0;
}

int general_power_get_fan_speed(struct general_power *power, int id)
{
    return general_power_read_word(power, REG_FAN_SPEED0 + id);
}

int general_power_get_temp(struct general_power *power, int id)
{
    return general_power_read_word(power, REG_TEMP0 + id);
}

int general_power_get_model(struct general_power *power, char *buf)
{
    return general_power_read(power, REG_MODEL, buf, 16);
}

int general_power_get_vender(struct general_power *power, char *buf)
{
    return general_power_read(power, REG_VENDER, buf, 16);
}

int general_power_get_hw_version(struct general_power *power, char *buf)
{
    return general_power_read(power, REG_HW_VERSION, buf, 16);
}

int general_power_get_sw_version(struct general_power *power, char *buf)
{
    return general_power_read(power, REG_SW_VERSION, buf, 16);
}

int general_power_get_common_status(struct general_power *power)
{
    return 0;
}

int general_power_get_common_error(struct general_power *power)
{
    return general_power_get_errors(power) | power->common_error;
}

int general_power_set_upgrade(struct general_power *power, const char *file)
{
    return general_power_upgrade(power, file);
}
