#ifndef __general_power_H__
#define __general_power_H__
#include "../bitmicro_power_supply.h"

#define BUF_SIZE 1024

//0x01    电源开关（开：0x01, 关：0x00）    1Byte R/W
//0x02    主输出电压（单位：mv）    2Byte R
//0x03    主输出电流（单位：A） 2Byte R
//0x04    设置主输出电压（单位：mv）  2Byte R/W
//0x05    错误码（详见2.2.1错误码定义）   4Bbyte R/W(写操作清除错误)
//0x06-0x0A   温度传感器（可选）（单位：0.1°C） 2Byte R
//0x0B-0x0C   风扇转速（可选）    2Byte R
//0x0D    输入过压保护点（可选） 2Byte R
//0x0E    输入过流保护点（可选） 2Byte R
//0x0F    输入欠压保护点（可选） 2Byte R
//0x10    清除错误码   1byte W
//0x11    主输出功率   2byte R
//0x70    电源状态（详见2.2.2 状态定义）  4Byte R
//0x80    软件版本    16Byte R
//0x81    硬件版本    16Byte R
//0x82    型号  16Byte R
//0x83    供应商 16Byte R
//0x84    生产地址（可选）    16Byte R
//0x85    生产日期（可选）    16Byte R
//0xA0    升级  R/W

enum {
    REG_ENABLE = 0x01,
    REG_VOUT,
    REG_IOUT,
    REG_VOUT_SET,
    REG_ERRORS,
    REG_TEMP0,
    REG_TEMP1,
    REG_TEMP2,
    REG_TEMP3,
    REG_TEMP4,
    REG_FAN_SPEED0,
    REG_FAN_SPEED1,
    REG_OVER_VIN,
    REG_OVER_IIN,
    REG_UNDER_VIN,
    REG_CLEAR_ERROR     = 0x10,
    REG_POWER,
    REG_STATUS          = 0x70,
    REG_SW_VERSION      = 0x80,
    REG_HW_VERSION,
    REG_MODEL,
    REG_VENDER,
    REG_PRODUCT_ADDR,
    REG_PROTECT_DATE,
    REG_UPGRADE         = 0xA0,
};

struct general_power {
    struct i2c_client *client;
    struct mutex mutex;
    unsigned char version;
    int common_error;

    int vender_id;
    int is_upgrating;
    char *upgrade_path;
    int voltage;
    int voltage_v12;
    int voltage_set;
    int current;
    int power_input;
    int status;
    int enable;
    int errors;
    int fan_speed;
    char temp[BUF_SIZE];
    int ios[3];
    char hw_version[17];
    char sw_version[17];
    char vender[17];
    char model[17];
    char buf[BUF_SIZE];
};

int general_power_i2c_write(struct general_power *power, void *buf, int size);
int general_power_i2c_read(struct general_power *power, char reg, void *rbuf, int rsize);
int general_power_read(struct general_power *power, char reg, void *buf, int size);
int general_power_write(struct general_power *power, char reg, void *buf, int size);
short general_power_read_word(struct general_power *power, char reg);

int general_power_get_enable(struct general_power *power);
int general_power_set_enable(struct general_power *power, int enable);
int general_power_get_voltage(struct general_power *power);
int general_power_get_voltage_set(struct general_power *power);
int general_power_set_voltage(struct general_power *power, int vol);
int general_power_get_current(struct general_power *power);
int general_power_get_power(struct general_power *power);
int general_power_get_status(struct general_power *power);
int general_power_get_errors(struct general_power *power);
int general_power_set_errors(struct general_power *power, int val);
int general_power_get_fan_speed(struct general_power *power, int id);
int general_power_get_temp(struct general_power *power, int i);
int general_power_get_model(struct general_power *power, char *buf);
int general_power_get_vender(struct general_power *power, char *buf);
int general_power_get_hw_version(struct general_power *power, char *buf);
int general_power_get_sw_version(struct general_power *power, char *buf);
int general_power_get_vout_max(struct general_power *power);
int general_power_set_vout_max(struct general_power *power, int vol);
int general_power_get_vout_min(struct general_power *power);
int general_power_set_vout_min(struct general_power *power, int vol);
int general_power_get_common_status(struct general_power *power);
int general_power_get_common_error(struct general_power *power);
int general_power_set_upgrade(struct general_power *power, const char *file);
int general_power_upgrade(struct general_power *power, const char *file);

#endif
