#ifndef __dc_POWER_SUPPLY_H__
#define __dc_POWER_SUPPLY_H__
#include "../bitmicro_power_supply.h"
#include <linux/gpio.h>

#define EEPROM_TAG1     0xfa
#define EEPROM_TAG2     0x5a
#define REG_VAL	        0x0
#define REG_EN	        0x10
#define DCDC_ENABLE     0x40
#define DCDC_DISABLE    0x00

#define NODE_TOTAL      3

struct dc_data {
        struct i2c_client *client;
        struct mutex mutex;  
        unsigned int err;         
};

struct dc_opt {
        struct dc_data * pdat;          
        unsigned char addr;
        unsigned char vol;
        unsigned int  io;       //enable gpio        
        bool en;
};

extern char sw_version[];
extern char hw_version[];
extern struct dc_opt opt[NODE_TOTAL];

int dc_read_reg_one_byte(struct dc_data *power, unsigned char addr,
                         unsigned char reg, unsigned char *val);
int dc_write_reg_one_byte(struct dc_data *power, unsigned char addr,
                          unsigned char reg, unsigned char val);     
int dc_power_supply_init(struct dc_opt *data);

#endif

