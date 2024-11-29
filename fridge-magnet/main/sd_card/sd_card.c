#include "sd_card.h"

const char *TAG = "sdcard";
esp_err_t init_sdcard(void)
{

  #ifdef CONFIG_DEBUG_PIN_CONNECTIONS
  const char* names[] = {"CLK", "CMD", "D0", "D1", "D2", "D3"};
  const int pins[] = {CONFIG_PIN_CLK,
                      CONFIG_PIN_CMD,
                      CONFIG_PIN_D0
                      #ifdef CONFIG_SDMMC_BUS_WIDTH_4
                      ,CONFIG_PIN_D1,
                      CONFIG_PIN_D2,
                      CONFIG_PIN_D3
                      #endif
                      };

  const int pin_count = sizeof(pins)/sizeof(pins[0]);

#if CONFIG_ENABLE_ADC_FEATURE
  const int adc_channels[] = {CONFIG_ADC_PIN_CLK,
                              CONFIG_ADC_PIN_CMD,
                              CONFIG_ADC_PIN_D0
                              #ifdef CONFIG_SDMMC_BUS_WIDTH_4
                              ,CONFIG_ADC_PIN_D1,
                              CONFIG_ADC_PIN_D2,
                              CONFIG_ADC_PIN_D3
                              #endif
                              };
#endif //CONFIG_ENABLE_ADC_FEATURE

  pin_configuration_t config = {
      .names = names,
      .pins = pins,
#if CONFIG_ENABLE_ADC_FEATURE
      .adc_channels = adc_channels,
#endif
  };
  #endif //CONFIG_DEBUG_PIN_CONNECTIONS


  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  #if CONFIG_SDMMC_SPEED_HS
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
#elif CONFIG_SDMMC_SPEED_UHS_I_SDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_SDR50;
    host.flags &= ~SDMMC_HOST_FLAG_DDR;
#elif CONFIG_SDMMC_SPEED_UHS_I_DDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DDR50;
#endif

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.

#if CONFIG_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  #if IS_UHS1
    slot_config.flags |= SDMMC_SLOT_FLAG_UHS1;
#endif

    // Set bus width to use:
#ifdef CONFIG_SDMMC_BUS_WIDTH_4
    slot_config.width = 4;
#else
    slot_config.width = 1;
#endif

#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = CONFIG_PIN_CLK;
    slot_config.cmd = CONFIG_PIN_CMD;
    slot_config.d0 = CONFIG_PIN_D0;
#ifdef CONFIG_SDMMC_BUS_WIDTH_4
    slot_config.d1 = CONFIG_PIN_D1;
    slot_config.d2 = CONFIG_PIN_D2;
    slot_config.d3 = CONFIG_PIN_D3;
#endif  // CONFIG_SDMMC_BUS_WIDTH_4
#endif  // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    #ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
      .format_if_mount_failed = true,
    #else
      .format_if_mount_failed = false,
    #endif 
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Initializing SD card as SDMMC Peripheral");
  ESP_LOGI(TAG, "Mounting Filesystem");
  esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
    }
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Filesystem mounted");
  sdmmc_card_print_info(stdout, card);
  return ESP_OK;
}
