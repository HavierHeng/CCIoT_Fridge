#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "driver/i2s_common.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"  // For example_connect() to configure Wi-Fi or Ethernet
//

/* Own Component Headers */
#include "mqtt/mqtt_auth.h"
#include "sd_card//sd_card.h"
#include "speaker/speaker.h"


#include "esp_log.h"    

static const char *TAG = "fridge-app";

enum fridge_states {
    CONSUMPTION,
    STOCKING
};

enum consumption_substate {
    ITEM_DETAIL,
    EXPIRY_DETAIL,
    WEIGHT_DETAIL
};

esp_err_t handle_item_detail(void);
esp_err_t handle_expiry_detail(void);
esp_err_t handle_weight_detail(void);


void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("sdcard", ESP_LOG_VERBOSE);
    esp_log_level_set("speaker", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt-fridge", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();

    // Open SD Card
    ESP_ERROR_CHECK(init_sdcard());

    // Create handle for spkr - as a singleton pattern
    ESP_ERROR_CHECK(spkr_i2s_setup());
    // Play wav using spkr
    ESP_ERROR_CHECK(spkr_play_wav(SPKR_AUDIO_FILE));
    
    // testing
    // i2s_del_channel(spkr_tx_handle);
}
