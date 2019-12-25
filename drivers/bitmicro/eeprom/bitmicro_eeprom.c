#include "bitmicro_eeprom.h"
#include <bitmicro.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/err.h>

int bitmicro_eeprom_link(struct i2c_client *client)
{
    static struct kobject *kobj_dir = NULL;
    static int init = 0;
    const __be32 *slot_be;
    char name[64];
    int slot;

    if (!init)
    {
        kobj_dir = kobject_create_and_add("eeprom", bitmicro_kobj);
        if (!kobj_dir)
            return -1;
        init = 1;
    }

    slot_be = of_get_property(client->dev.of_node, "slot", NULL);
    if (slot_be)
        slot = be32_to_cpup(slot_be);
    else
        return -1;

    snprintf(name, sizeof(name), "eeprom%d", slot);
    if (0 == sysfs_create_link(kobj_dir, &client->dev.kobj, name))
    {
        printk("dev:0x%x : bitmicro eeprom%d link\n",client->addr, slot);
        return 0;
    }
    return -1;
}
EXPORT_SYMBOL_GPL(bitmicro_eeprom_link);
