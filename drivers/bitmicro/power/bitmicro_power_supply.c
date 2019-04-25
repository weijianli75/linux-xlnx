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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/err.h>


static struct class *bitmicro_power_supply_class;
static struct device_type bitmicro_power_supply_dev_type;

static void bitmicro_power_supply_changed_work(struct work_struct *work)
{
    struct bitmicro_power_supply *psy = container_of(work, struct bitmicro_power_supply,
                        changed_work);

    dev_dbg(&psy->dev, "%s\n", __func__);

}

static void bitmicro_power_supply_dev_release(struct device *dev)
{
    struct bitmicro_power_supply *psy = container_of(dev, struct bitmicro_power_supply, dev);
    pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
    kfree(psy);
}

int bitmicro_power_supply_get_property(struct bitmicro_power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
    if (!psy->initialized)
        return -EAGAIN;

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
bitmicro_power_supply_register(struct device *parent,
                   const struct power_supply_desc *desc)
{
    static int inited = 0;
    struct device *dev;
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

    dev = &psy->dev;

    device_initialize(dev);

    dev->class = bitmicro_power_supply_class;
    dev->type = &bitmicro_power_supply_dev_type;
    dev->parent = parent;
    dev->release = bitmicro_power_supply_dev_release;
    dev_set_drvdata(dev, psy);
    psy->desc = desc;

    rc = dev_set_name(dev, "%s", "power");
    if (rc)
        goto dev_set_name_failed;

    INIT_WORK(&psy->changed_work, bitmicro_power_supply_changed_work);

    rc = device_add(dev);
    if (rc)
        goto device_add_failed;

    psy->initialized = true;

    return psy;

device_add_failed:
dev_set_name_failed:
    put_device(dev);
    return ERR_PTR(rc);
}
EXPORT_SYMBOL_GPL(bitmicro_power_supply_register);

static int __init bitmicro_power_supply_class_init(void)
{
    bitmicro_power_supply_class = class_create(THIS_MODULE, "bitmicro_power_supply");

    if (IS_ERR(bitmicro_power_supply_class))
        return PTR_ERR(bitmicro_power_supply_class);

    bitmicro_power_supply_init_attrs(&bitmicro_power_supply_dev_type);
    return 0;
}

static void __exit bitmicro_power_supply_class_exit(void)
{
    class_destroy(bitmicro_power_supply_class);
}

subsys_initcall(bitmicro_power_supply_class_init);
module_exit(bitmicro_power_supply_class_exit);

