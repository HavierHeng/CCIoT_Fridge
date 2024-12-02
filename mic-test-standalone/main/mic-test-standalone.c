#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "file_operations.h"
#include "lcd_operations.h"
#include "audio_operations.h"
#include "button_operations.h"
#include "shared_states.h"
#include "task_operations.h"


void app_main(void)
{
    ESP_LOGI("app", "[APP] Startup..");
    ESP_LOGI("app", "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI("app", "[APP] IDF version: %s", esp_get_idf_version());

    // In app main the order of inits are:
    // 1) Drivers (e.g SDMMC, I2C, SPI, WIFI and so on)
    // 2) Queues
    // 3) Callbacks
    // 4) Tasks (which depend on most other inits first)

    /* For WIFI */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());  // Too lazy to make wifi connect myself, this comes with a kconfig
    
    /* Drivers */
    ESP_ERROR_CHECK(init_lcd());
    ESP_ERROR_CHECK(init_sdcard());
    ESP_ERROR_CHECK(init_i2s_mic());
    ESP_ERROR_CHECK(init_i2s_spkr());
    ESP_ERROR_CHECK(init_buttons());

    /* Queues and sync */
    ESP_ERROR_CHECK(init_i2s_semaphores());
    ESP_ERROR_CHECK(init_queues());

    /* Callback */
    ESP_ERROR_CHECK(register_btn_cb());

    /* Tasks */
    ESP_ERROR_CHECK(init_tasks());
}
