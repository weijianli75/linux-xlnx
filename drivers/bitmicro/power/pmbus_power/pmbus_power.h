#ifndef __PMBUS_POWER_H__
#define __PMBUS_POWER_H__

#include "../bitmicro_power_supply.h"
#include "../pmbus_power/pmbus.h"

/* GosPower Status Bits */
#define PMBUS_STATUS_VOUT_UV_PROTECTED          BIT(4)  /* 输出低压保护 */
#define PMBUS_STATUS_VOUT_OV_PROTECTED          BIT(7)  /* 输出过压保护 */

#define PMBUS_STATUS_IOUT_OP_WARNING            BIT(0)  /* 输出功率警告 */
#define PMBUS_STATUS_IOUT_OP_PROTECTED          BIT(1)  /* 输出功率保护 */
#define PMBUS_STATUS_IOUT_OC_WARNING            BIT(5)  /* 输出过流警告 */
#define PMBUS_STATUS_IOUT_OC_PROTECTED          BIT(7)  /* 输出过流保护 */

#define PMBUS_STATUS_IN_OP_WARNING              BIT(0)  /* 输入功率警告 */
#define PMBUS_STATUS_IN_OC_WARNING              BIT(1)  /* 输入过流警告 */
#define PMBUS_STATUS_IN_OC_PROTECTED            BIT(2)  /* 输入过流保护 */
#define PMBUS_STATUS_IN_UV_PROTECTED1           BIT(3)  /* 输入欠压导致输出关闭 */
#define PMBUS_STATUS_IN_UV_PROTECTED2           BIT(4)  /* 输入欠压保护 */
#define PMBUS_STATUS_IN_UV_WARNING              BIT(5)  /* 输入欠压警告 */
#define PMBUS_STATUS_IN_OV_WARNING              BIT(6)  /* 输入过压警告 */
#define PMBUS_STATUS_IN_OV_PROTECTED            BIT(7)  /* 输入过压保护 */

#define PMBUS_STATUS_TEMP_OT_WARNING            BIT(6)  /* 温度过高警告 */
#define PMBUS_STATUS_TEMP_OT_PROTECTED          BIT(7)  /* 温度过高保护 */

#define PMBUS_STATUS_FAN_LOW_WARNING            BIT(5)  /* 风扇转速过低警告 */
#define PMBUS_STATUS_FAN_LOW_PROTECTED          BIT(7)  /* 风扇转速过低保护 */

/* GosPower Customized Commands */
enum {
    GPSPOWER_WATCHDOG_STATUS = 0x07,
    GOSPOWER_MFR_HW_COMPATIBILITY = 0xD4,
    GOSPOWER_MFR_FWUPLOAD_CAPABILITY = 0xD5,
    GOSPOWER_MFR_FWUPLOAD_MODE = 0xD6,
    GOSPOWER_MFR_FWUPLOAD = 0xD7,
    GOSPOWER_MFR_FWUPLOAD_STATUS = 0xD8,
    GOSPOWER_PRI_FW_REVISION = 0xDB,
    GOSPOWER_SEC_FW_REVISION = 0xDC,
    GOSPOWER_ISP_PSU_HW_REV = 0xDD,
};

/* PMBUS power data structure */
struct pmbus_power
{
    struct i2c_client *client;
    struct mutex mutex;

    struct power_supply_desc *power_desc;
    struct pmbus_driver_info *drv_info;
    struct pmbus_sensor sensor;
    int exponent[PMBUS_PAGES];

    char mfr_id[16];
    char mfr_model[16];
    char mfr_revision[16];
    char mfr_location[16];
    char mfr_date[16];
    char mfr_serial[16];
    char vendor[16];
    char hw_version[16];
    char sw_version[16];
    int pmbus_revision;
    int common_errors;
    int is_upgrading;
};

long pmbus_reg2data(struct pmbus_power *power, struct pmbus_sensor *sensor);
u16 pmbus_data2reg(struct pmbus_power *power, struct pmbus_sensor *sensor, long val);

s32 pmbus_i2c_smbus_read_i2c_block_data(struct pmbus_power *power,
        u8 command, u8 length, u8 *values);
s32 pmbus_power_write(struct pmbus_power *power, u8 command, void *buf, int size);
char pmbus_power_read_byte(struct pmbus_power *power, char reg);
short pmbus_power_read_word(struct pmbus_power *power, char reg);
int pmbus_power_write_byte(struct pmbus_power *power, char reg, char data);
int pmbus_power_write_word(struct pmbus_power *power, char reg, short data);
int pmbus_power_block_read(struct pmbus_power *power, int reg, int len, char *buf);

int pmbus_power_get_enable(struct pmbus_power *power);
int pmbus_power_set_enable(struct pmbus_power *power, int enable);
int pmbus_power_get_voltage_in(struct pmbus_power *power);
int pmbus_power_get_current_in(struct pmbus_power *power);
int pmbus_power_get_power_in(struct pmbus_power *power);
int pmbus_power_get_voltage_out(struct pmbus_power *power);
int pmbus_power_get_voltage_out_set(struct pmbus_power *power);
int pmbus_power_set_voltage(struct pmbus_power *power, int vol);
int pmbus_power_get_current_out(struct pmbus_power *power);
int pmbus_power_get_power_out(struct pmbus_power *power);
int pmbus_power_get_fan_speed(struct pmbus_power *power);
int pmbus_power_get_temp0(struct pmbus_power *power);
int pmbus_power_get_temp1(struct pmbus_power *power);
int pmbus_power_get_temp2(struct pmbus_power *power);
int pmbus_power_get_hw_version(struct pmbus_power *power, char *buf);
int pmbus_power_get_sw_version(struct pmbus_power *power, char *buf);
int pmbus_power_get_status(struct pmbus_power *power);
int pmbus_power_get_common_status(struct pmbus_power *power);
int pmbus_power_get_errors(struct pmbus_power *power);
int pmbus_power_get_common_errors(struct pmbus_power *power);
int pmbus_power_set_upgrade(struct pmbus_power *power, const char *filename);

int pmbus_init_power(struct pmbus_power *power);
int pmbus_power_upgrade(struct pmbus_power *power, const char *filename);

#endif /* __PMBUS_POWER_H__ */
