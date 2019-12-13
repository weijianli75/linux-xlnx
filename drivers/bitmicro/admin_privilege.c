/*
 *  Copyright (C) 2019 MicroBT Electronics Technology Co.,Ltd
 *  Huanglihong <huanglihong@microbt.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <uapi/linux/stat.h> /* S_IRUSR, S_IWUSR  */
#include <linux/uidgid.h>
#include <linux/sched.h>
#include <asm/current.h>


static int admin_privilege;

int has_privilege(void)
{
	const struct cred *cred = current_cred();

	if (admin_privilege || uid_eq(cred->uid, GLOBAL_ROOT_UID))
		return 1;

	return 0;
}

static ssize_t admin_privilege_store(struct  kobject *kobj, struct kobj_attribute *attr,
        const char *buff, size_t count)
{
	unsigned long val;
	ssize_t result;
	result = sscanf(buff, "%lu", &val);
	if (result != 1)
		return -EINVAL;
	admin_privilege = val;
	pr_info("Set admin_privilege to %d\n", admin_privilege);

	return count;
}

static struct kobj_attribute admin_privilege_attribute =
    __ATTR(admin_privilege, S_IRUGO | S_IWUSR, NULL, admin_privilege_store);

static struct attribute *attrs[] = {
    &admin_privilege_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *kobj;

static int __init admin_privilege_init(void)
{
    int ret;

    kobj = kobject_create_and_add("bitmicro", kernel_kobj);
    if (!kobj)
        return -ENOMEM;
    ret = sysfs_create_group(kobj, &attr_group);
    if (ret)
        kobject_put(kobj);
    return ret;
}

static void __exit admin_privilege_exit(void)
{
    kobject_put(kobj);
}

module_init(admin_privilege_init);
module_exit(admin_privilege_exit);

MODULE_AUTHOR("Huang Lihong <huanglihong@microbt.com>");
MODULE_DESCRIPTION("Admin privilege");
MODULE_LICENSE("GPL");
