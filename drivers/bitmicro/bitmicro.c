#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/kexec.h>
#include <linux/profile.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/capability.h>
#include <linux/compiler.h>

struct kobject *bitmicro_kobj;
EXPORT_SYMBOL_GPL(bitmicro_kobj);

int __init bitmicro_sys_init(void)
{
    bitmicro_kobj = kobject_create_and_add("bitmicro", NULL);
    if (!bitmicro_kobj) {
        return -ENOMEM;
    }

    return 0;
}

core_initcall(bitmicro_sys_init);
