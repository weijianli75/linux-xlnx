#include "bitmicro_eeprom.h"
#include <bitmicro.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/err.h>

int __must_check bitmicro_eeprom_link(struct kobject *kobj, unsigned short addr)
{
    static struct kobject *kobj_dir = NULL;
    static int init = 0;
    unsigned char adr = 0;
    char name[64];

    if (!init)
    {
        kobj_dir = kobject_create_and_add("eeprom", bitmicro_kobj);
        if (!kobj_dir)
            return -1;
        init = 1;
    }

    switch(addr)
    {
        case 0x50:adr = 0;
            break;
        case 0x51:adr = 1;
            break;
        case 0x52:adr = 2;
            break;
        case 0x54:adr = 0;
            break;
        case 0x55:adr = 1;
            break;
        case 0x56:adr = 2;
            break;        
        default: return -1;
            break;                                                   
    }
    snprintf(name, sizeof(name), "eeprom%d", adr);
    return sysfs_create_link(kobj_dir, kobj, name);
}
EXPORT_SYMBOL_GPL(bitmicro_eeprom_link);
