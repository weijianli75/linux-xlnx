#include "keepalive.h"

struct keepalive s_keepalive;

int getline(char *buf, int offset, int len, char *line) {
    int i = offset;

    do {
        if (buf[i] == '\n' || buf[i] == '\0')
            break;
    } while (++i < len);

    if (i == offset && buf[i] == '\0')
        return -1;

    memcpy(line, buf+offset, i-offset);
    line[i-offset] = '\0';

    return i + 1;
}

static void keepalive_work(struct work_struct *w)
{
    int ret;
    struct keepalive *keepalive = container_of(to_delayed_work(w), struct keepalive, work);

    if (!keepalive->enable) {
        schedule_delayed_work(to_delayed_work(w), msecs_to_jiffies(10000));
        return ;
    }

    ret = call_usermodehelper(KEEPALIVE_PATH, NULL, NULL, UMH_WAIT_PROC);
    if (ret < 0) {
        pr_err("%s-%d: failed to exec cmd: %s, %d\n",
                __func__, __LINE__, KEEPALIVE_PATH, ret);
    }

    schedule_delayed_work(to_delayed_work(w), msecs_to_jiffies(10000));
}

static int keepalive_init(void) {
    s_keepalive.enable = 1;

    INIT_DELAYED_WORK(&s_keepalive.work, keepalive_work);
    schedule_delayed_work(&s_keepalive.work, msecs_to_jiffies(10000));

    return 0;
}

static int __init keepalive_model_init(void)
{
    int ret;
    printk("==============%s============\n", __func__);
    ret = keepalive_sys_init();
    if (ret)
        return ret;

    keepalive_init();

    return ret;
}

module_init(keepalive_model_init);

