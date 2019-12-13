/*
 *  Universal power supply monitor class
 *
 *  Copyright © 2007  Anton Vorontsov <cbou@mail.ru>
 *  Copyright © 2004  Szabolcs Gyurko
 *  Copyright © 2003  Ian Molton <spyro@f2s.com>
 *
 *  Modified: 2004, Oct     Szabolcs Gyurko
 *
 *  You may use this code as per GPL version 2
 */

#include "bitmicro_power_supply.h"
#include <bitmicro.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/err.h>

int power_supply_desc_init(struct power_supply_desc *desc,
        char model[16], char vender[16])
{
    char *begin;
    char *end = model;
    char name[17] = {0};

    desc->model = kstrdup(model, GFP_KERNEL);

    if (strcmp(vender, "BrT") == 0) {
        desc->vender =  VENDER_ATSTEK;
    }
    else if (strcmp(model, "GOSPOWER")) {
        desc->vender = VENDER_GOSPOWER;
    }
    else {
        desc->vender = VENDER_GOSPOWER;
    }

    desc->efficiency = 'C';
    begin = strsep(&end, "-");
    memcpy(name, begin, end-begin);

    sscanf(end, "%d-%d-%c",
            &desc->vout, &desc->pout_max, &desc->efficiency);
    if ((strcmp(name, "PX") != 0)
        && desc->efficiency != 'C'
        && strlen(name) < sizeof(name) - 1)
        name[strlen(name)] = desc->efficiency;
    desc->name = kstrdup(name, GFP_KERNEL);

    printk("=============%s==================\n", __func__);
    printk("name: %s\n", desc->name);
    printk("vender: %d\n", desc->vender);
    printk("model: %s\n", desc->model);
    printk("vout: %d\n", desc->vout);
    printk("pout_max: %d\n", desc->pout_max);
    printk("efficiency: %c\n", desc->efficiency);
    printk("================================\n");

    return 0;
}

int bitmicro_power_supply_get_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
    return psy->desc->get_property(psy, psp, val);
}
EXPORT_SYMBOL_GPL(bitmicro_power_supply_get_property);

int bitmicro_power_supply_set_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                const union power_supply_propval *val)
{
    if (!psy->desc->set_property)
        return -ENODEV;

    return psy->desc->set_property(psy, psp, val);
}
EXPORT_SYMBOL_GPL(bitmicro_power_supply_set_property);

struct bitmicro_power_supply *__must_check
bitmicro_power_supply_register(const struct power_supply_desc *desc)
{
    static int inited = 0;
    struct bitmicro_power_supply *psy;
    int rc;

    if (inited) {
        pr_warn("%s: bitmicro power device already registered, register %s failed\n",
            __func__, desc->name);
        return NULL;
    }
    inited = 1;

    psy = kzalloc(sizeof(*psy), GFP_KERNEL);
    if (!psy)
        return ERR_PTR(-ENOMEM);

    psy->desc = desc;

    rc = bitmicro_power_supply_init_attrs(&psy->kobj);
    if (rc) {
        kfree(psy);
        return ERR_PTR(rc);
    }

    return psy;
}
EXPORT_SYMBOL_GPL(bitmicro_power_supply_register);

