#ifndef __BITMICRO_EEPROM_H__
#define __BITMICRO_EEPROM_H__
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

int bitmicro_eeprom_link(struct i2c_client *client);

#endif
