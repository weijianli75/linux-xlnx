#include "keepalive.h"


static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr,
              char *buf)
{
    return sprintf(buf, "%d\n", s_keepalive.enable);
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
               const char *buf, size_t count)
{
    int var, ret;

    ret = kstrtoint(buf, 0, &var);
    if (ret < 0)
        return ret;

    s_keepalive.enable = var;

    return count;
}

static struct kobj_attribute bar_attribute =
    __ATTR(enable, 0644, enable_show, enable_store);

static struct attribute *attrs[] = {
    &bar_attribute.attr,
    NULL,   /* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *keepalive_kobj;

int keepalive_sys_init(void)
{
    int retval;

    keepalive_kobj = kobject_create_and_add("keepalive", kernel_kobj);
    if (!keepalive_kobj)
        return -ENOMEM;

    retval = sysfs_create_group(keepalive_kobj, &attr_group);
    if (retval)
        kobject_put(keepalive_kobj);

    return retval;
}

void keepalive_sys_exit(void)
{
    kobject_put(keepalive_kobj);
}

