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

#include "bitmicro_power_info.h"

#ifdef current
#undef current
#endif

struct bitmicro_power_supply;

enum power_supply_property {
    POWER_SUPPLY_VOUT,
    POWER_SUPPLY_VOUT_SET,
    POWER_SUPPLY_VOUT_MAX,
    POWER_SUPPLY_VOUT_MIN,
    POWER_SUPPLY_VIN,
    POWER_SUPPLY_IOUT,
    POWER_SUPPLY_IIN,
    POWER_SUPPLY_POUT,
    POWER_SUPPLY_PIN,
    POWER_SUPPLY_ENABLE,

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
    POWER_SUPPLY_MODEL,
    POWER_SUPPLY_DEBUG,
    POWER_SUPPLY_UPGRADE,

    POWER_SUPPLY_NAME,
};

enum {
    POWER_ERROR_OVER_TOUT,          // 输出过温保护
    POWER_ERROR_OVER_IOUT,          // 输出过流保护
    POWER_ERROR_OVER_VOUT,          // 输出过压保护
    POWER_ERROR_OVER_TIN,           // 输入过温保护
    POWER_ERROR_OVER_IIN,           // 输入过流保护
    POWER_ERROR_OVER_VIN,           // 输入过压保护
    POWER_ERROR_UNDER_VIN,          // 输入欠压保护
    POWER_ERROR_FAN,                // 风扇错误

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
    const char *name;
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
    const struct power_supply_desc *desc;

    char **supplied_to;
    size_t num_supplicants;

    /* Driver private data */
    void *drv_data;

    /* private */
    struct device dev;
    struct work_struct changed_work;
    bool changed;
    bool initialized;
};

void bitmicro_power_supply_init_attrs(struct device_type *dev_type);
struct bitmicro_power_supply *
bitmicro_power_supply_register(struct device *parent,
                               const struct power_supply_desc *desc);

int bitmicro_power_supply_get_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val);

int bitmicro_power_supply_set_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                const union power_supply_propval *val);

#endif
