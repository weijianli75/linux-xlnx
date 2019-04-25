#ifndef __BITMICRO_POWER_INFO_H__
#define __BITMICRO_POWER_INFO_H__

enum power_vender {
    UNKNOW,
    GOSPOWER,       //高斯宝
    MEIC,           //玛司特
    HUNTKEY,        //航嘉
    RSPOWER,        //瀚强
};

struct power_info {
    int vender;
    char *name;
    char *model;
    char *vender_str;
};

#endif
