#ifndef __dc_POWER_SUPPLY_H__
#define __dc_POWER_SUPPLY_H__
#include "../bitmicro_power_supply.h"
#include <linux/gpio.h>
#include <linux/of.h>

enum EDC_TYPE{
        TYPE_NONE = 0,
        TYPE_ISL23315,
        TYPE_CAT5140,
        TYPE_TOTAL,
};

#define EEPROM_TAG1             0xfa
#define EEPROM_TAG2             0x5a
//isl23315
#define REG_VAL                 0x0
#define REG_EN                  0x10
#define DCDC_ENABLE             0x40
#define DCDC_DISABLE            0x00
//cat5140
#define REG_IVR                 0x00
#define REG_DEV_ID              0x01
#define REG_ACR                 0x08
#define CAT_DEVICE_ID           0xd0

#if 1
#define CAT_GPIO_SEL_DISABLE        1
#define CAT_GPIO_SEL_ENABLE         0
#else
#define CAT_GPIO_SEL_DISABLE        0
#define CAT_GPIO_SEL_ENABLE         1
#endif

#define POWER_GPIO_DISABLE          0
#define POWER_GPIO_ENABLE           1

#define NODE_TOTAL              3

struct dc_data
{
        struct i2c_client *client;
        struct mutex mutex;
        unsigned int err;
};

struct dc_opt
{        
        char *name;
        struct dc_data *pdat;
        int io;        //enable gpio
        int selio;
        unsigned char type;     //dcdc ic type
        unsigned char vol;
        bool en;
};

extern char sw_version[];
extern char hw_version[];
extern struct dc_opt opt[NODE_TOTAL];

int dc_read_reg_one_byte(struct dc_data *power, unsigned char reg, unsigned char *val);
int dc_write_reg_one_byte(struct dc_data *power, unsigned char reg, unsigned char val);
int dc_power_supply_init(struct dc_opt *data);
void cat5410_sel_switch(int selio);
int dc_power_set_vout(struct dc_opt *opt);

#endif
