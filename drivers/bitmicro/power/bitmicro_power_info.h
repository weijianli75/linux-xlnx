#ifndef __BITMICRO_POWER_INFO_H__
#define __BITMICRO_POWER_INFO_H__

enum power_vender {
    VENDER_UNKNOW,
    VENDER_GOSPOWER,       //高斯宝
    VENDER_MEIC,           //玛司特
    VENDER_HUNTKEY,        //航嘉
    VENDER_RSPOWER,        //瀚强
    VENDER_ATSTEK,         //安托山
    VENDER_DCPOWER,         //DC2DC
};

struct power_info {
    int vender;
    char *name;
    char *model;
    char *vender_str;
};

#endif
