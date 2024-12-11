#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "audio_operations.h"
#include "iot_button.h"
#include "esp_log.h"
#include "http_operations.h"
#include "lcd_operations.h"
#include <string.h>

#define BUTTON_GPIO 21

button_handle_t gpio_btn;

static void button_single_click_cb(void *arg, void *usr_data) {
    // On short button press -> Upload file using the presigned url from a JSON
    char* json_info = "{ \"protocol\": \"http\", \"domain\": \"fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com\", \"path\": \"/audio/expiry/95491898-a0e3-4857-8b9f-79bedc06f217.wav\"}";
    upload_file_to_s3(json_info, "/sdcard/suggested-recipes.wav");
    lcd_write_screen(&lcd_handle, "Uploaded to S3");
};

static void button_double_click_cb(void *arg, void *usr_data) {
    // On double click -> Download file
    char* json_info = "{ \"protocol\": \"http\", \"domain\": \"fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com\", \"path\": \"/tts/suggested-recipes.wav\"}";
    download_file(json_info, "/sdcard/test.wav");
    lcd_write_screen(&lcd_handle, "Test.wav downloaded");
    play_wav("/sdcard/test.wav");
}

esp_err_t init_buttons() {
    button_config_t gpio_btn_cfg = {
    .type = BUTTON_TYPE_GPIO,
    .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config = {
        .gpio_num = 21,
        .active_level = 0,  
        },
    };
    gpio_btn = iot_button_create(&gpio_btn_cfg);
    if(NULL == gpio_btn) {
        ESP_LOGE("button", "Button create failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t register_btn_cb() {
    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL));
    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL));
    ESP_LOGI("button", "Button callbacks registered");
    return ESP_OK;
}
