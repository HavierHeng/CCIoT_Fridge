#pragma once 
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "../pinouts.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#define STRINGIFY(x) #x
#ifdef CONFIG_MOUNT_POINT
    #define MOUNT_POINT STRINGIFY(CONFIG_MOUNT_POINT)
#endif
#define IS_UHS1    (CONFIG_SDMMC_SPEED_UHS_I_SDR50 || CONFIG_SDMMC_SPEED_UHS_I_DDR50)

extern sdmmc_card_t *card;
esp_err_t init_sdcard(void); 
