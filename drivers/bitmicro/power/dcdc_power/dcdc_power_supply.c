#include "dcdc_power.h"

static int dc_power_get_vout(struct dc_opt *opt)
{
    int ret;
    unsigned char val;
    ret = dc_read_reg_one_byte(opt->pdat, opt->addr, REG_VAL, &val);
    if (ret < 0)
        return ret;

    return (int)val;
}

static int dc_power_set_vout(struct dc_opt *opt)
{
    int rret, wret;
    unsigned char val;
    wret = dc_write_reg_one_byte(opt->pdat, opt->addr, REG_VAL, opt->vol);
    rret = dc_read_reg_one_byte(opt->pdat, opt->addr, REG_VAL, &val);
    if ((rret < 0) || (opt->vol != val))
    {
        opt->pdat->err |= BIT(POWER_ERROR_XFER_WARNIGN);
        return rret;
    }
    return wret;
}

static int dc_power_set_enable(struct dc_opt *opt)
{
    int rret, wret;
    unsigned char get;
    unsigned char set = opt->en ? DCDC_ENABLE : DCDC_DISABLE;
    wret = dc_write_reg_one_byte(opt->pdat, opt->addr, REG_EN, set);
    rret = dc_read_reg_one_byte(opt->pdat, opt->addr, REG_EN, &get);
    if ((rret < 0) || (get != set))
    {
        opt->pdat->err |= BIT(POWER_ERROR_XFER_WARNIGN);
        return rret;        
    }
    return wret;
}

static enum power_supply_property dc_power_props[] = {
    POWER_SUPPLY_IOUT,
    POWER_SUPPLY_VOUT,
    POWER_SUPPLY_VOUT0,
    POWER_SUPPLY_VOUT1,
    POWER_SUPPLY_VOUT2,
    POWER_SUPPLY_VOUT_SET,
    POWER_SUPPLY_VOUT_SET0,
    POWER_SUPPLY_VOUT_SET1,
    POWER_SUPPLY_VOUT_SET2,
    POWER_SUPPLY_ENABLE,
    POWER_SUPPLY_ENABLE0,
    POWER_SUPPLY_ENABLE1,
    POWER_SUPPLY_ENABLE2,
    POWER_SUPPLY_ERRORS,
    POWER_SUPPLY_COMMON_ERRORS,
    POWER_SUPPLY_COMMON_STATUS,
    POWER_SUPPLY_HW_VERSION,
    POWER_SUPPLY_SW_VERSION,
};

static int dc_power_get_property(struct bitmicro_power_supply *psy,
                                 enum power_supply_property psp,
                                 union power_supply_propval *val)
{
    int ret = 0, i, cnt;
    struct dc_opt *opt = psy->drv_data;

    for (i = 0; i < NODE_TOTAL; i++)
        if (opt[i].pdat != NULL)
            mutex_lock(&opt[i].pdat->mutex);

    switch (psp)
    {
    case POWER_SUPPLY_IOUT:
        val->intval = -1;
        break;
    case POWER_SUPPLY_VOUT:
    case POWER_SUPPLY_VOUT_SET:
        ret = -1;
        cnt = 0;
        val->intval = 0;
        for (i = 0; i < NODE_TOTAL; i++)
        {
            if (opt[i].pdat != NULL)
            {
                int tmp = dc_power_get_vout(&opt[i]);
                if (tmp >= 0)
                {                    
                    val->intval += tmp;
                    cnt++;
                    ret = 0;
                }
            }
        }
        if (cnt > 0)
            val->intval /= cnt;  
        break;
    case POWER_SUPPLY_VOUT0:
    case POWER_SUPPLY_VOUT_SET0:
        if (opt[0].pdat == NULL)
        {
            ret = -1;
            break;
        }
        val->intval = dc_power_get_vout(&opt[0]);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT1:
    case POWER_SUPPLY_VOUT_SET1:
        if (opt[1].pdat == NULL)
        {
            ret = -1;
            break;
        }
        val->intval = dc_power_get_vout(&opt[1]);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_VOUT2:
    case POWER_SUPPLY_VOUT_SET2:
        if (opt[2].pdat == NULL)
        {
            ret = -1;
            break;
        }
        val->intval = dc_power_get_vout(&opt[2]);
        if (val->intval < 0)
            ret = -1;
        break;
    case POWER_SUPPLY_ENABLE:
        val->intval = 0;
        for (i = 0; i < NODE_TOTAL; i++)
            if (opt[i].en == true)
                val->intval = 1;
        break;
    case POWER_SUPPLY_ENABLE0:
        if (opt[0].pdat == NULL)
        {
            ret = -1;
            break;
        }
        val->intval = opt[0].en ? 1 : 0;
        break;
    case POWER_SUPPLY_ENABLE1:
        if (opt[1].pdat == NULL)
        {
            ret = -1;
            break;
        }
        val->intval = opt[1].en ? 1 : 0;
        break;
    case POWER_SUPPLY_ENABLE2:
        if (opt[2].pdat == NULL)
        {
            ret = -1;
            break;
        }
        val->intval = opt[2].en ? 1 : 0;
        break;
    case POWER_SUPPLY_COMMON_ERRORS:
        val->intval = 0;
        for (i = 0; i < NODE_TOTAL; i++)
        {
            if (opt[i].pdat == NULL)
                continue;
            if (opt[i].pdat->err)
                val->intval = opt[i].pdat->err;
            opt[i].pdat->err = 0;
        }
        break;
    case POWER_SUPPLY_ERRORS:
    case POWER_SUPPLY_COMMON_STATUS:
        val->intval = 0;
        break;
    case POWER_SUPPLY_HW_VERSION:
        val->strval = hw_version;
        break;
    case POWER_SUPPLY_SW_VERSION:
        val->strval = sw_version;
        break;
    default:
        ret = -EINVAL;
        break;
    }

    for (i = 0; i < NODE_TOTAL; i++)
        if (opt[i].pdat != NULL)
            mutex_unlock(&opt[i].pdat->mutex);

    return ret;
}

static int dc_power_set_property(struct bitmicro_power_supply *psy,
                                 enum power_supply_property psp,
                                 const union power_supply_propval *val)
{
    int ret = 0, i;
    struct dc_opt *opt = psy->drv_data;

    for (i = 0; i < NODE_TOTAL; i++)
        if (opt[i].pdat != NULL)
            mutex_lock(&opt[i].pdat->mutex);

    switch (psp)
    {
    case POWER_SUPPLY_VOUT_SET:
        ret = -1;
        for (i = 0; i < NODE_TOTAL; i++)
        {
            if (opt[i].pdat != NULL)
            {
                opt[i].vol = (unsigned char)val->intval;
                if (dc_power_set_vout(&opt[i]) >= 0)
                    ret = 0;
            }
        }
        break;
    case POWER_SUPPLY_VOUT_SET0:
        if (opt[0].pdat == NULL)
        {
            ret = -1;
            break;
        }
        opt[0].vol = (unsigned char)val->intval;
        ret = dc_power_set_vout(&opt[0]);
        break;
    case POWER_SUPPLY_VOUT_SET1:
        if (opt[1].pdat == NULL)
        {
            ret = -1;
            break;
        }
        opt[1].vol = (unsigned char)val->intval;
        ret = dc_power_set_vout(&opt[1]);
        break;
    case POWER_SUPPLY_VOUT_SET2:
        if (opt[2].pdat == NULL)
        {
            ret = -1;
            break;
        }
        opt[2].vol = (unsigned char)val->intval;
        ret = dc_power_set_vout(&opt[2]);
        break;
    case POWER_SUPPLY_ENABLE:
        for (i = 0; i < NODE_TOTAL; i++)
            if (opt[i].pdat != NULL)
            {
                int tmp;
                opt[i].en = val->intval ? true : false;
                gpio_direction_output(opt[i].io, opt[i].en ? 1 : 0);
                tmp = dc_power_set_enable(&opt[i]);
                if (tmp > 0)
                    ret = 0;
            }
        break;
    case POWER_SUPPLY_ENABLE0:
        if (opt[0].pdat == NULL)
        {
            ret = -1;
            break;
        }
        opt[0].en = val->intval ? true : false;
        ret = dc_power_set_enable(&opt[0]);
        gpio_direction_output(opt[0].io, opt[0].en ? 1 : 0);
        break;
    case POWER_SUPPLY_ENABLE1:
        if (opt[1].pdat == NULL)
        {
            ret = -1;
            break;
        }
        opt[1].en = val->intval ? true : false;
        ret = dc_power_set_enable(&opt[1]);
        gpio_direction_output(opt[1].io, opt[1].en ? 1 : 0);
        break;
    case POWER_SUPPLY_ENABLE2:
        if (opt[2].pdat == NULL)
        {
            ret = -1;
            break;
        }
        opt[2].en = val->intval ? true : false;
        ret = dc_power_set_enable(&opt[2]);
        gpio_direction_output(opt[2].io, opt[2].en ? 1 : 0);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    for (i = 0; i < NODE_TOTAL; i++)
        if (opt[i].pdat != NULL)
            mutex_unlock(&opt[i].pdat->mutex);

    return ret;
}

static struct power_supply_desc power_desc[] = {
    [0] = {
        .vender = VENDER_DCPOWER,
        .name = "DCDC",
        .model = "DCDC-12-2200-C",
        .properties = dc_power_props,
        .num_properties = ARRAY_SIZE(dc_power_props),
        .get_property = dc_power_get_property,
        .set_property = dc_power_set_property,
    },
};

int dc_power_supply_init(struct dc_opt *opt)
{
    struct bitmicro_power_supply *psy;
    psy = bitmicro_power_supply_register(power_desc);
    if (psy == NULL)
        return -1;

    psy->drv_data = opt;
    return 0;
}
