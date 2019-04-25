#include "bitmicro_power_supply.h"

#define POWER_SUPPLY_ATTR(_name)                    \
{                                   \
    .attr = { .name = #_name },                 \
    .show = power_supply_show_property,             \
    .store = power_supply_store_property,               \
}

static struct device_attribute power_supply_attrs[];

static ssize_t power_supply_show_property(struct device *dev,
                      struct device_attribute *attr,
                      char *buf)
{
    ssize_t ret = 0;
    struct bitmicro_power_supply *psy = dev_get_drvdata(dev);
    const ptrdiff_t off = attr - power_supply_attrs;
    union power_supply_propval value;

    if (off == POWER_SUPPLY_NAME)
        return sprintf(buf, "%s\n", psy->desc->name);
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

static ssize_t power_supply_store_property(struct device *dev,
                      struct device_attribute *attr,
                      const char *buf, size_t count)
{
    ssize_t ret = 0;
    struct bitmicro_power_supply *psy = dev_get_drvdata(dev);
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

static struct device_attribute power_supply_attrs[] = {
        POWER_SUPPLY_ATTR(vout),
        POWER_SUPPLY_ATTR(vout_set),
        POWER_SUPPLY_ATTR(vout_max),
        POWER_SUPPLY_ATTR(vout_min),
        POWER_SUPPLY_ATTR(vin),
        POWER_SUPPLY_ATTR(iout),
        POWER_SUPPLY_ATTR(iin),
        POWER_SUPPLY_ATTR(pout),
        POWER_SUPPLY_ATTR(pin),
        POWER_SUPPLY_ATTR(enable),

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
        POWER_SUPPLY_ATTR(model),
        POWER_SUPPLY_ATTR(debug),
        POWER_SUPPLY_ATTR(upgrade),

        POWER_SUPPLY_ATTR(name),
};

static umode_t power_supply_attr_is_visible(struct kobject *kobj,
                       struct attribute *attr,
                       int attrno)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct bitmicro_power_supply *psy = dev_get_drvdata(dev);
    umode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;
    int i;

    if (attrno == POWER_SUPPLY_NAME
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

static const struct attribute_group *power_supply_attr_groups[] = {
    &power_supply_attr_group,
    NULL,
};

void bitmicro_power_supply_init_attrs(struct device_type *dev_type)
{
    int i;

    dev_type->groups = power_supply_attr_groups;

    for (i = 0; i < ARRAY_SIZE(power_supply_attrs); i++)
        __power_supply_attrs[i] = &power_supply_attrs[i].attr;
}
