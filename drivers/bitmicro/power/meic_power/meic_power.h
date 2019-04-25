#ifndef __MEIC_POWER_H__
#define __MEIC_POWER_H__
#include "../bitmicro_power_supply.h"

#define BUF_SIZE 1024

//0x01    开关机 W/R 1Byte    0x80 开机  00 关机
#define MEIC_REG_ENABLE             0x01
//0x02    清状态寄存器故障    W   N/A
#define MEIC_REG_CLEAR_ERR          0x02
//0x03    读取电源状态  R 4Byte 返回数据间“附录A”
#define MEIC_REG_STATUS             0x03
//0x04    读取5路NTC温度值  R 5Byte 温度值以8位有符号值表示（+/-127°C）每路温度一个Byte.
#define MEIC_REG_TEMP               0x04
//0x05    设置5路过温故障门限  W/R 5BYTE   温度值以8位有符号值表示（+/-127°C）每路温度一个Byte.
#define MEIC_REG_TEMP_LIMIT         0x05
//0x06    读取12V电压 R 2Byte 实际值*100   高位在前，低位在后
#define MEIC_REG_VOL_V12            0x06
//0x07    读取12V电流 R 2Byte 实际值*100   高位在前，低位在后
#define MEIC_REG_CUR_V12            0x07
//0x08    读取12VSB电压   R 2Byte 实际值*100   高位在前，低位在后
#define MEIC_REG_VOL_V12SB          0x08
//0x09    读取12VSB电流   R 2Byte 实际值*100   高位在前，低位在后
#define MEIC_REG_CUR_V12SB          0x09
//0x0C    读取软件版本  R  2Byte    两个字节
#define MEIC_REG_SW_VERSION         0x0c
//0x0D    读写取电源版本 W/R 2Byte   两个字节
#define MEIC_REG_HW_VERSION            0x0d
//0x0E    获取风扇转速  R  2Byte    高位在前，低位在后(每分钟转速)
#define MEIC_REG_FAN                0x0e
//0x0F    校准使能    W   2Byte   0xAA 使能
#define MEIC_REG_CORRECT_ENABLE     0x0f
//0x10    12V采样校准K、B值 W/R 4Byte   K*10000以unsigned short下发， B保持原值以signed short下发，K在前B在后，低字节在后高字节在前, B默认为0
//0x11    12V电流采样校准K、B值   W/R 4Byte   K*10000以unsigned short下发， B保持原值以signed short下发，K在前B在后，低字节在后高字节在前, B默认为0
//0x12    12VSB采样校准K、B值   W/R 4Byte   K*10000以unsigned short下发， B保持原值以signed short下发，K在前B在后，低字节在后高字节在前, B默认为0
//0x13    12VSB采样校准K、B值   W/R 4Byte   K*10000以unsigned short下发， B保持原值以signed short下发，K在前B在后，低字节在后高字节在前, B默认为0
//0x14    12V过压保护点    W/R 2Byte   实际值*100   高位在前，低位在后
#define MEIC_REG_OVER_VOL_V12      0x14
//0x15    12V电流过流保护点  W/R 2Byte   实际值*100   高位在前，低位在后
#define MEIC_REG_OVER_CUR_V12      0x15
//0x16    12VSB过压保护点  W/R 2Byte   实际值*100   高位在前，低位在后
#define MEIC_REG_OVER_VOL_V12SB    0x16
//0x17    12VSB电流过流保护点    W/R 2Byte   实际值*100   高位在前，低位在后
#define MEIC_REG_OVER_CUR_V12SB    0x17
//0x18    12V欠压保护点    W/R 2Byte   实际值*100   高位在前，低位在后
#define MEIC_REG_UNDER_VOL_V12      0x18
//0x19    12VSB欠压保护点  W/R 2Byte   实际值*100   高位在前，低位在后
#define MEIC_REG_UNDER_CUR_V12      0x19
//0x1B    设置电源I2C从机地址 W/R 1Byte   Slave Address
#define MEIC_REG_I2C_SLAVE_ADDR     0x1B
//0x1C    设置主路输出电压值   W/R 2Byte   实际电压*100 转换为U16 ，高位在前，低位在后。
#define MEIC_REG_VOLTAGE_SET        0x1C
//0x20    启动BootLoader    W 1Byte N/A
#define MEIC_REG_BOOTLOADER_ENABLE  0x20

#define MEIC_REG_UPGRADE_ENABLE  0x21

struct meic_power {
    struct i2c_client *client;
    struct mutex mutex;
    unsigned char version;
    int common_error;

    int auto_update;
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
    char sw_version[128];
    char buf[BUF_SIZE];
    int is_upgrading;
};

int meic_power_i2c_write_read(struct i2c_client *client,
                            void *wbuf, int wsize, void *rbuf, int rsize);
int meic_power_i2c_write(struct i2c_client *client, void *buf, int size);
int meic_power_i2c_read(struct i2c_client *client, void *buf, int size);

int meic_power_read(struct meic_power *power, char reg, void *buf, int size);
int meic_power_write(struct meic_power *power, char reg, void *buf, int size);
short meic_power_read_word(struct meic_power *power, char reg);

int meic_power_get_enable(struct meic_power *power);
int meic_power_set_enable(struct meic_power *power, int enable);
int meic_power_get_voltage(struct meic_power *power);
int meic_power_get_voltage_set(struct meic_power *power);
int meic_power_set_voltage(struct meic_power *power, int vol);
int meic_power_get_current(struct meic_power *power);
int meic_power_get_status(struct meic_power *power);
int meic_power_get_fan_speed(struct meic_power *power);
int meic_power_get_temp(struct meic_power *power, int i);
int meic_power_get_hw_version(struct meic_power *power);
int meic_power_get_sw_version(struct meic_power *power);
int meic_power_get_vout_max(struct meic_power *power);
int meic_power_set_vout_max(struct meic_power *power, int vol);
int meic_power_get_vout_min(struct meic_power *power);
int meic_power_set_vout_min(struct meic_power *power, int vol);
int meic_power_get_common_status(struct meic_power *power);
int meic_power_get_common_error(struct meic_power *power);
int meic_power_upgrade(struct meic_power *power, const char *file);
int meci_power_is_upgrading(struct meic_power *power);
unsigned short meic_crc16(unsigned char * buf, unsigned char len);

#endif
