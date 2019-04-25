#ifndef __HUNTKEY_POWER_H__
#define __HUNTKEY_POWER_H__
#include "../bitmicro_power_supply.h"

#define BUF_SIZE    1024

struct huntkey_power {
    struct i2c_client *client;
    struct mutex mutex;
    unsigned char version;
    int common_error;

    int vout;
    int vout_set;
    int iout;
    int pout;
    int status;
    int enable;
    int errors;
    int fan;
    int temp[3];
    char hw_version[128];
    char sw_version[128];
    char buf[BUF_SIZE];
};

int huntkey_power_get_enable(struct huntkey_power *power);
int huntkey_power_set_enable(struct huntkey_power *power, int enable);
int huntkey_power_get_vout(struct huntkey_power *power);
int huntkey_power_get_vout_set(struct huntkey_power *power);
int huntkey_power_set_vout_set(struct huntkey_power *power, int vol);
int huntkey_power_get_iout(struct huntkey_power *power);
int huntkey_power_get_status(struct huntkey_power *power);
int huntkey_power_get_errors(struct huntkey_power *power);
int huntkey_power_get_fan(struct huntkey_power *power);
int huntkey_power_get_temp(struct huntkey_power *power);
int huntkey_power_get_hw_version(struct huntkey_power *power, char *buf);
int huntkey_power_get_sw_version(struct huntkey_power *power, char *buf);
int huntkey_power_get_common_status(struct huntkey_power *power);
int huntkey_power_get_common_error(struct huntkey_power *power);

#endif
