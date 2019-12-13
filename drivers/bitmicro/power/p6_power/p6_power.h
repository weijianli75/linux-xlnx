#ifndef __p6_POWER_SUPPLY_H__
#define __p6_POWER_SUPPLY_H__
#include "../bitmicro_power_supply.h"

#define BUF_SIZE      1024
#ifdef current
#undef current
#endif

/*
 * P6 Operations (Read)
 */
#define CMD_READ_VERSION        0x00    /* Power version */
#define CMD_READ_ERRORCODE      0x05    /* Error code */
#define CMD_READ_VOUT_12V       0x06    /* Control loop voltage(0.01V) */
#define CMD_READ_VOUT           0x07    /* Main loop voltage(0.01V) */
#define CMD_READ_IOUT           0x08    /* Total output current(A) */
#define CMD_READ_POUT           0x09    /* Power(W) */
#define CMD_READ_TEMP0          0x0A    /* Cooling fin 0's temperature(0.1℃) */
#define CMD_READ_TEMP1          0x0B    /* Cooling fin 1's temperature(0.1℃)  */
#define CMD_READ_TEMP2          0x0C    /* Internal environment temperature(0.1℃) */
#define CMD_READ_FAN_SPEED      0x0D    /* Fan speed(RPM) */
#define CMD_READ_SW_REVISION    0x17    /* SW Revision */
#define CMD_READ_HW_REVISION    0x19    /* HW Revision */
#define CMD_PROTECT_ENABLE      0x18    /* protect enable */
#define CMD_READ_IOSA           0xD0    /* IOSA Current(A) */
#define CMD_READ_IOSB           0xD1    /* IOSB Current(A) */
#define CMD_READ_IOSC           0xD2    /* IOSC Current(A) */
#define CMD_READ_SERIAL_NO      0xD9    /* Serial number */

/*
 * P6 Operations (Write/Read)
 */
#define CMD_ON_OFF              0x02    /* Power on/off, Data Byte: 1, Write 1: on, Write 0: off */
#define CMD_CLEAR_FAULT         0x03    /* Clear fault to restore function */
#define CMD_SET_VOUT            0x12    /* Set Power VOUT, Data Byte: 2, Unit: 0.01V, Range: 12-17V */

#define P10_VERSION             0x21
#define P11_VERSION             0x31
#define P20_VERSION             0x22
#define P21_VERSION_OLD         0x32
#define P21_VERSION             0x42

#define P6_ERROR_UNDER_VIN     0x0001  //输入欠压
#define P6_ERROR_OVER_TOUT0 0x0002  //过温
#define P6_ERROR_OVER_TOUT1 0x0004  //过温
#define P6_ERROR_OVER_TOUT2 0x0008  //过温
#define P6_ERROR_OVER_IIN 0x0010  //原边保护
#define P6_ERROR_UNDER_VOUT 0x0020  //输出欠压
#define P6_ERROR_OVER_IOUT0 0x0040  //输出过流
#define P6_ERROR_UNBALANCE_VOUT 0x0080  //原边保护
//0x0100  //输出过流
//0x0200  //输出过流
//0x0400  //输出过流
#define P6_ERROR_FAN0 0x0800  //风扇错误

struct boot_data {
    u8 version;
    u8 sw_version;
    u8 hw_version;
    u8 boot_version;
};

struct p6_data {
    struct i2c_client *client;
    struct mutex update_mutex;
    unsigned char version;
    int common_errors;

    int auto_update;
    int voltage;
    int voltage_v12;
    int voltage_set;
    int current;
    int power_output;
    int status;
    int enable;
    int errors;
    int fan_speed;
    int temp[3];
    int ios[3];
    int has_sw_version;
    int has_hw_version;
    char sw_version[128];
    char hw_version[128];
    char serial_no[128];
    char buf[BUF_SIZE];
    int in_boot;
};

int p6_get_boot_info(struct p6_data *pdata, struct boot_data *boot_data);
int p6_power_supply_init(struct p6_data *data);

int p6_power_get_version(struct p6_data *data);
int p6_power_get_status(struct p6_data *data);
int p6_power_get_enable(struct p6_data *data);
int p6_power_set_enable(struct p6_data *data, int enable);
int p6_power_set_protect_enable(struct p6_data *data, int enable);
int p6_power_get_errors(struct p6_data *data);
int p6_power_set_errors(struct p6_data *data);
int p6_power_get_voltage_v12(struct p6_data *data);
int p6_power_get_voltage(struct p6_data *data);
int p6_power_set_voltage_set(struct p6_data *data, int vol);
int p6_power_get_voltage_set(struct p6_data *data);
int p6_power_get_current(struct p6_data *data);
int p6_power_get_power(struct p6_data *data);
int p6_power_get_temp(struct p6_data *data, int id);
int p6_power_get_fan(struct p6_data *data);
int p6_power_get_ios(struct p6_data *data, int id);
int p6_power_get_sw_version(struct p6_data *data, char *buf);
int p6_power_get_hw_version(struct p6_data *data, char *buf);
int p6_power_get_serial_no(struct p6_data *data, char *buf);
int p6_power_get_common_status(struct p6_data *data);
int p6_power_get_common_errors(struct p6_data *data);

#endif
