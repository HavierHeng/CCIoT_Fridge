#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "freertos/idf_additions.h"
#include "http_operations.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "protocol_examples_common.h"
#include "lcd_operations.h"
#include "button_operations.h"
#include "file_operations.h"


void upload_task(void *pvParameters) {
    // Example task - here you could place your file upload logic
    while (1) {
        char* json_info = "{ \"protocol\": \"http\", \"hostname\": \"fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com\", \"path\": \"/audio/expiry/95491898-a0e3-4857-8b9f-79bedc06f217.wav\"}";
        upload_file_to_s3(json_info, "/sdcard/suggested-recipes.wav");
        lcd_write_screen(&lcd_handle, "Uploaded to S3");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
    }
}

void download_task(void *pvParameters) {
    while (1) {
        // On double click -> Download file
        char* json_info = "{ \"protocol\": \"http\", \"hostname\": \"fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com\", \"path\": \"/tts/suggested-recipes.wav\"}";
        download_file(json_info, "/sdcard/test.wav");
        lcd_write_screen(&lcd_handle, "Test.wav downloaded");
        play_wav("/sdcard/test.wav");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
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
    

    ESP_ERROR_CHECK(init_lcd());
    ESP_ERROR_CHECK(init_sdcard());
    ESP_ERROR_CHECK(init_i2s_spkr());

    ESP_ERROR_CHECK(init_buttons());
    ESP_ERROR_CHECK(register_btn_cb());
    ESP_ERROR_CHECK(init_semaphore());
    // xTaskCreate(upload_task, "UploadTask", 8096, NULL, 5, NULL);
    xTaskCreate(download_task, "DownloadTask", 8096, NULL, 5, NULL);

}
