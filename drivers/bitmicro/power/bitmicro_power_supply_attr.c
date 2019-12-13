#include "bitmicro_power_supply.h"
#include <linux/sysfs.h>
#include <bitmicro.h>

#define POWER_SUPPLY_ATTR(_name)                    \
{                                   \
    .name = #_name,                 \
}

static struct attribute power_supply_attrs[];

static ssize_t power_supply_show_property(struct kobject *kobj,
                      struct attribute *attr,
                      char *buf)
{
    ssize_t ret = 0;
    struct bitmicro_power_supply *psy =
            container_of(kobj, struct bitmicro_power_supply, kobj);
    const ptrdiff_t off = attr - power_supply_attrs;
    union power_supply_propval value;

    if (off == POWER_SUPPLY_NAME)
        return sprintf(buf, "%s\n", psy->desc->name);
    if (off == POWER_SUPPLY_MODEL)
        return sprintf(buf, "%s\n", psy->desc->model);
    else if (off == POWER_SUPPLY_VENDER)
        return sprintf(buf, "%d\n", psy->desc->vender);

    ret = bitmicro_power_supply_get_property(psy, off, &value);
    if (ret < 0) {
        return ret;
    }

    if (off < POWER_SUPPLY_STATUS)
        return sprintf(buf, "%d\n", value.intval);
    else if (off < POWER_SUPPLY_HW_VERSION)
        return sprintf(buf, "0x%x\n", value.intval);
    else
        return sprintf(buf, "%s\n", value.strval);

    return ret;
}

static ssize_t power_supply_store_property(struct kobject *kobj,
                      struct attribute *attr,
                      const char *buf, size_t count)
{
    ssize_t ret = 0;
    struct bitmicro_power_supply *psy =
            container_of(kobj, struct bitmicro_power_supply, kobj);
    const ptrdiff_t off = attr - power_supply_attrs;
    union power_supply_propval value;
    long long_val;
    char strval[count];

    if (off < POWER_SUPPLY_HW_VERSION) {
        ret = kstrtol(buf, 10, &long_val);
        if (ret < 0)
            return ret;

        value.intval = long_val;
    }
    else {
        memcpy(strval, buf, strlen(buf));
        if (strval[strlen(buf)-1] == '\n')
            strval[strlen(buf)-1] = '\0';
        value.strval = strval;
    }

    ret = bitmicro_power_supply_set_property(psy, off, &value);
    if (ret < 0)
        return ret;

    return count;
}

static struct attribute power_supply_attrs[] = {
        POWER_SUPPLY_ATTR(vout),
        POWER_SUPPLY_ATTR(vout0),
        POWER_SUPPLY_ATTR(vout1),
        POWER_SUPPLY_ATTR(vout2),
        POWER_SUPPLY_ATTR(vout_set),
        POWER_SUPPLY_ATTR(vout_set0),
        POWER_SUPPLY_ATTR(vout_set1),
        POWER_SUPPLY_ATTR(vout_set2),
        POWER_SUPPLY_ATTR(vout_max),
        POWER_SUPPLY_ATTR(vout_min),
        POWER_SUPPLY_ATTR(vin),
        POWER_SUPPLY_ATTR(iout),
        POWER_SUPPLY_ATTR(iin),
        POWER_SUPPLY_ATTR(pout),
        POWER_SUPPLY_ATTR(pin),
        POWER_SUPPLY_ATTR(enable),
        POWER_SUPPLY_ATTR(enable0),
        POWER_SUPPLY_ATTR(enable1),
        POWER_SUPPLY_ATTR(enable2),

        POWER_SUPPLY_ATTR(fan_speed0),
        POWER_SUPPLY_ATTR(fan_speed1),
        POWER_SUPPLY_ATTR(temp0),
        POWER_SUPPLY_ATTR(temp1),
        POWER_SUPPLY_ATTR(temp2),
        POWER_SUPPLY_ATTR(temp3),
        POWER_SUPPLY_ATTR(temp4),
        POWER_SUPPLY_ATTR(vender),

        POWER_SUPPLY_ATTR(status),
        POWER_SUPPLY_ATTR(version),
        POWER_SUPPLY_ATTR(errors),
        POWER_SUPPLY_ATTR(common_status),
        POWER_SUPPLY_ATTR(common_errors),

        POWER_SUPPLY_ATTR(hw_version),
        POWER_SUPPLY_ATTR(sw_version),
        POWER_SUPPLY_ATTR(serial_no),
        POWER_SUPPLY_ATTR(model),
        POWER_SUPPLY_ATTR(debug),
        POWER_SUPPLY_ATTR(upgrade),

        POWER_SUPPLY_ATTR(name),
};

static umode_t power_supply_attr_is_visible(struct kobject *kobj,
                       struct attribute *attr,
                       int attrno)
{
    struct bitmicro_power_supply *psy =
            container_of(kobj, struct bitmicro_power_supply, kobj);
    umode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;
    int i;

    if (attrno == POWER_SUPPLY_NAME
        || attrno == POWER_SUPPLY_MODEL
        || attrno == POWER_SUPPLY_VENDER)
        return mode;

    for (i = 0; i < psy->desc->num_properties; i++) {
        int property = psy->desc->properties[i];

        if (property == attrno) {
            return mode;
        }
    }

    return 0;
}

static struct attribute *
__power_supply_attrs[ARRAY_SIZE(power_supply_attrs) + 1];

static struct attribute_group power_supply_attr_group = {
    .attrs = __power_supply_attrs,
    .is_visible = power_supply_attr_is_visible,
};

static const struct sysfs_ops bitmicro_power_sysfs_ops = {
    .show = power_supply_show_property,
    .store = power_supply_store_property,
};

static struct kobj_type bitmicro_power_ktype = {
    .sysfs_ops = &bitmicro_power_sysfs_ops,
//    .default_attrs = &power_supply_attrs,
};

int bitmicro_power_supply_init_attrs(struct kobject *kobj)
{
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(power_supply_attrs); i++)
        __power_supply_attrs[i] = &power_supply_attrs[i];

    ret = kobject_init_and_add(kobj, &bitmicro_power_ktype, bitmicro_kobj, "power");
    if (ret)
        return ret;

    return sysfs_create_group(kobj, &power_supply_attr_group);
}
