#ifndef __KEEPALIVE_H__
#define __KEEPALIVE_H__
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/swab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#define KEEPALIVE_PATH       "/usr/bin/keepalive.sh"

struct keepalive {
    struct delayed_work work;
    int enable;
};

extern struct keepalive s_keepalive;

int keepalive_sys_init(void);

#endif
