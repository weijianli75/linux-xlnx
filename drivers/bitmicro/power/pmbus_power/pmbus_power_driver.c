#include "pmbus_power.h"

static struct pmbus_driver_info general_driver_info = {
        .pages = 1,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_FAN] = linear,
};

/*
 * Identify chip parameters.
 */
static int pmbus_identify_common(struct pmbus_power *power, int page)
{
    int vout_mode = -1;

    // VOUT_MODE
    vout_mode = pmbus_power_read_byte(power, PMBUS_VOUT_MODE);
    printk("pmbus_identify_common: page = %d, vout_mode = 0x%x\n", page, vout_mode);

    if (vout_mode >= 0 && vout_mode != 0xff) {
        /*
         * Not all chips support the VOUT_MODE command,
         * so a failure to read it is not an error.
         */
        switch (vout_mode >> 5) {
        case 0: /* linear mode      */
            if (power->drv_info->format[PSC_VOLTAGE_OUT] != linear)
                return -ENODEV;
            power->exponent[page] = ((s8) (vout_mode << 3)) >> 3;
            printk("pmbus_identify_common: exponent = %d\n", power->exponent[page]);
            break;
        case 1: /* VID mode         */
            if (power->drv_info->format[PSC_VOLTAGE_OUT] != vid)
                break;
        case 2: /* direct mode      */
            if (power->drv_info->format[PSC_VOLTAGE_OUT] != direct)
                break;
        default:
            return -ENODEV;
        }
    }

    return 0;
}

int pmbus_init_power(struct pmbus_power *power)
{
    struct device *dev = &power->client->dev;
    int page, ret;

    power->drv_info = &general_driver_info;

    if (power->drv_info == NULL) {
        dev_err(dev, "power->drv_info is NULL\n");
        return -ENODEV;
    }

    if (power->power_desc == NULL) {
        dev_err(dev, "power->power_desc is NULL\n");
        return -ENODEV;
    }

    if (power->drv_info->pages <= 0 || power->drv_info->pages > PMBUS_PAGES) {
        dev_err(dev, "Bad number of PMBus pages: %d\n", power->drv_info->pages);
        return -ENODEV;
    }

    // Read manufacturer info
    pmbus_power_block_read(power, PMBUS_MFR_ID, sizeof(power->mfr_id), power->mfr_id);
    pmbus_power_block_read(power, PMBUS_MFR_MODEL, sizeof(power->mfr_model), power->mfr_model);
    pmbus_power_block_read(power, PMBUS_MFR_REVISION, sizeof(power->mfr_revision), power->mfr_revision);
    pmbus_power_block_read(power, PMBUS_MFR_LOCATION, sizeof(power->mfr_location), power->mfr_location);
    pmbus_power_block_read(power, PMBUS_MFR_DATE, sizeof(power->mfr_date), power->mfr_date);
    pmbus_power_block_read(power, PMBUS_MFR_SERIAL, sizeof(power->mfr_serial), power->mfr_serial);

    if (strcmp(power->mfr_id, "GOSPOWER") == 0) {
        power_supply_desc_init(power->power_desc, power->mfr_model, power->mfr_id);
    } else {
        dev_err(dev, "Unsupported power mfr_id (%s)\n", power->mfr_id);
        return -ENODEV;
    }
    // Read hw & sw versions
    if (power->power_desc->vender == VENDER_GOSPOWER) {
        char hw_ver[5];
        pmbus_power_block_read(power, 0xDD, sizeof(hw_ver), hw_ver);
        sprintf(power->hw_version, "%02x%02x%02x%02x%02x", hw_ver[0], hw_ver[1], hw_ver[2], hw_ver[3], hw_ver[4]);

        pmbus_power_block_read(power, 0xDC, sizeof(power->sw_version), power->sw_version);
    }
    else {
        strcpy(power->hw_version, "Unknown");
        strcpy(power->sw_version, "Unknown");
    }

    power->pmbus_revision = pmbus_power_read_byte(power, PMBUS_REVISION);

    // Identify chip capabilities
    for (page = 0; page < power->drv_info->pages; page++) {
        ret = pmbus_identify_common(power, page);
        if (ret < 0) {
            dev_err(dev, "Failed to identify chip capabilities\n");
            return ret;
        }
    }

    return 0;
}
