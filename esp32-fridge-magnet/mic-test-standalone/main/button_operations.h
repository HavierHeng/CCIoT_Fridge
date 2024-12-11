#pragma once

#include "audio_operations.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "iot_button.h"
#include "esp_log.h"
#include "task_operations.h"
#include "shared_states.h"
#include <string.h>

#define BUTTON_GPIO 21
#define MAX_FILES 100 

button_handle_t gpio_btn;

static void button_single_click_cb(void *arg, void *usr_data) {
    // On short button press -> Play audio - which stops the recording as well
    // This simulates what will happen on MQTT Subscribe for sound
    playbackDisplayMsg_t taskMsg;
    bool playTo;
    ESP_LOGI("button", "Single Click");
    // taskMsg.filePath[0] = '\0';
    // snprintf(taskMsg.displayMsg, sizeof(taskMsg.displayMsg), "File:\n%s", file_list[get_fileIdx()]);
    if (get_playTo(&playTo) == ESP_FAIL) {
        ESP_LOGE("button", "Could not get playTo");
        return;
    }
    taskMsg.playback_file = playTo;
    snprintf(taskMsg.displayMsg, sizeof(taskMsg.displayMsg), "File:\n%s", playTo ? WAV_BUFFER_1 : WAV_BUFFER_2); 
    if (xQueueSend(xPlaybackDisplayQueue, (void *) &taskMsg, (TickType_t) 0) != pdPASS) {
        ESP_LOGE("button", "Failed to queue task");
    }
};

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
    ESP_LOGI("button", "Button callbacks registered");
    return ESP_OK;
}
