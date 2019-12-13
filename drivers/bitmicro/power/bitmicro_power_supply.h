#ifndef __BITMICRO_POWER_SUPPLY_H__
#define __BITMICRO_POWER_SUPPLY_H__
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/swab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "bitmicro_power_info.h"

#ifdef current
#undef current
#endif

struct bitmicro_power_supply;

enum power_supply_property {
    POWER_SUPPLY_VOUT,
    POWER_SUPPLY_VOUT0,
    POWER_SUPPLY_VOUT1,
    POWER_SUPPLY_VOUT2,
    POWER_SUPPLY_VOUT_SET,
    POWER_SUPPLY_VOUT_SET0,
    POWER_SUPPLY_VOUT_SET1,
    POWER_SUPPLY_VOUT_SET2,
    POWER_SUPPLY_VOUT_MAX,
    POWER_SUPPLY_VOUT_MIN,
    POWER_SUPPLY_VIN,
    POWER_SUPPLY_IOUT,
    POWER_SUPPLY_IIN,
    POWER_SUPPLY_POUT,
    POWER_SUPPLY_PIN,
    POWER_SUPPLY_ENABLE,
    POWER_SUPPLY_ENABLE0,
    POWER_SUPPLY_ENABLE1,
    POWER_SUPPLY_ENABLE2,

    POWER_SUPPLY_FAN_SPEED0,
    POWER_SUPPLY_FAN_SPEED1,
    POWER_SUPPLY_TEMP0,
    POWER_SUPPLY_TEMP1,
    POWER_SUPPLY_TEMP2,
    POWER_SUPPLY_TEMP3,
    POWER_SUPPLY_TEMP4,
    POWER_SUPPLY_VENDER,
    
    /**
     * hex type
     */
    POWER_SUPPLY_STATUS,
    POWER_SUPPLY_VERSION,
    POWER_SUPPLY_ERRORS,
    POWER_SUPPLY_COMMON_STATUS,
    POWER_SUPPLY_COMMON_ERRORS,

    /**
     * string type
     */
    POWER_SUPPLY_HW_VERSION,
    POWER_SUPPLY_SW_VERSION,
    POWER_SUPPLY_SERIAL_NO,
    POWER_SUPPLY_MODEL,
    POWER_SUPPLY_DEBUG,
    POWER_SUPPLY_UPGRADE,

    POWER_SUPPLY_NAME,
};
/*
0   输出过温保护0
1   输出过温保护1
2   输出过温保护2
3   输出过流保护0
4   输出过流保护1
5   输出过流保护2
6   输出过压保护
7   输出低压保护
10  输入过温保护0
11  输入过温保护1
12  输入过温保护2
13  输入过流保护0
14  输入过流保护1
15  输入过压保护0
16  输入过压保护1
17  输入欠压保护0
18  输入欠压保护1
20  风扇错误0
21  风扇错误1
 */
enum {
    POWER_ERROR_OVER_TOUT0,             // 输出过温保护0
    POWER_ERROR_OVER_TOUT1,             // 输出过温保护1
    POWER_ERROR_OVER_TOUT2,             // 输出过温保护2
    POWER_ERROR_OVER_IOUT0,             // 输出过流保护0
    POWER_ERROR_OVER_IOUT1,             // 输出过流保护1
    POWER_ERROR_OVER_IOUT2,             // 输出过流保护2
    POWER_ERROR_OVER_VOUT,              // 输出过压保护
    POWER_ERROR_UNDER_VOUT,             // 输出低压保护
    POWER_ERROR_UNBALANCE_IOUT,         // 输出电流不平衡
    POWER_ERROR_OVER_TIN0 = 10,         // 输入过温保护0
    POWER_ERROR_OVER_TIN1,              // 输入过温保护1
    POWER_ERROR_OVER_TIN2,              // 输入过温保护2
    POWER_ERROR_OVER_IIN0,              // 输入过流保护0
    POWER_ERROR_OVER_IIN1,              // 输入过流保护1
    POWER_ERROR_OVER_VIN0,              // 输入过压保护0
    POWER_ERROR_OVER_VIN1,              // 输入过压保护1
    POWER_ERROR_UNDER_VIN0,             // 输入欠压保护0
    POWER_ERROR_UNDER_VIN1,             // 输入欠压保护1
    POWER_ERROR_FAN0 = 20,              // 风扇错误0
    POWER_ERROR_FAN1,                   // 风扇错误1

    POWER_ERROR_XFER_WARNIGN     = 30,     // 传输告警
    POWER_ERROR_XFER_ERROR       = 31,     // 传输错误
};

enum {
    POWER_STATUS_ENABLE,            // 开关状态
    POWER_STATUS_PROTECTED          // 看门狗保护状态
};

union power_supply_propval {
    int intval;
    const char *strval;
    int64_t int64val;
};

struct power_supply_desc {
    int vender;
    const char* name;
    const char* model;
    int vout;
    int pout_max;
    char efficiency;
    enum power_supply_property *properties;
    size_t num_properties;

    int (*get_property)(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val);
    int (*set_property)(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                const union power_supply_propval *val);
};

struct bitmicro_power_supply {
    struct kobject kobj;
    const struct power_supply_desc *desc;

    char **supplied_to;
    size_t num_supplicants;

    /* Driver private data */
    void *drv_data;
};

int power_supply_desc_init(struct power_supply_desc *desc,
        char model[16], char vender[16]);
int bitmicro_power_supply_init_attrs(struct kobject *kobj);
struct bitmicro_power_supply *
bitmicro_power_supply_register(const struct power_supply_desc *desc);

int bitmicro_power_supply_get_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val);

int bitmicro_power_supply_set_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                const union power_supply_propval *val);

#endif
