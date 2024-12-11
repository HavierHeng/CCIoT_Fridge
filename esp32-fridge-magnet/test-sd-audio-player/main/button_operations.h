#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "iot_button.h"
#include "esp_log.h"
#include "task_operations.h"
#include "file_operations.h"
#include <string.h>

#define BUTTON_GPIO 21
#define MAX_FILES 100 

button_handle_t gpio_btn;
// extern QueueHandle_t xPlaybackDisplayQueue;

char *file_list[MAX_FILES];  // Store the file names
int file_count = 0;

void load_file_list() {
    file_count = 0;
    char **files = list_dir(MOUNT_POINT, &file_count);
    
    // Save files to the global list (assuming files are null-terminated)
    for (int i = 0; i < file_count; i++) {
        file_list[i] = files[i];  // Store the file name in global array
    }
}

static void button_single_click_cb(void *arg, void *usr_data) {
    // On short button press -> Move forward through files in SD Card, and display on i2c screen
    playbackDisplayMsg_t taskMsg;
    ESP_LOGI("button", "Single Click");
    if (file_count > 0) {
        int currIdx = get_fileIdx();
        if (currIdx != -1) {
            set_fileIdx((currIdx + 1) % file_count);
            if (xPlaybackDisplayQueue != 0) {
                taskMsg.filePath[0] = '\0';
                snprintf(taskMsg.displayMsg, sizeof(taskMsg.displayMsg), "File:\n%s", file_list[get_fileIdx()]);
                if (xQueueSend(xPlaybackDisplayQueue, (void *) &taskMsg, (TickType_t) 10) != pdPASS) {
                    ESP_LOGE("button", "Failed to queue task");
                }
            }
        }
    }
};

static void button_double_click_cb(void *arg, void *usr_data) {
    // On double button -> Move backwards through files in SD Card, and display on i2c screen
    playbackDisplayMsg_t taskMsg;
    ESP_LOGI("button", "Double Click");
    if (file_count > 0) {
        int currIdx = get_fileIdx();

        if (currIdx != -1) {
            if (currIdx - 1 < 0) {
                set_fileIdx(file_count);
            } else {
                set_fileIdx((currIdx - 1) % file_count);
            }

            if (xPlaybackDisplayQueue != 0) {
                taskMsg.filePath[0] = '\0';
                snprintf(taskMsg.displayMsg, sizeof(taskMsg.displayMsg), "File:\n%s", file_list[get_fileIdx()]);
                if (xQueueSend(xPlaybackDisplayQueue, (void *) &taskMsg, (TickType_t) 10) != pdPASS) {
                    ESP_LOGE("button", "Failed to queue task");
                }
            }
        }
    }
}

static void button_long_press_cb(void *arg, void *usr_data) {
    // On long button press -> Plays the wav file and display "Playing \n<file_name>" on i2c screen. This is actually done on another task. 
    // This is display and playing is done on another task - and if its finished it will safely unset a state variable for recording/playing protected by a pthread rw mutex
    playbackDisplayMsg_t taskMsg;
    ESP_LOGI("button", "Long Press");
    if (xPlaybackDisplayQueue != 0) {
        int currIdx = get_fileIdx();
        if (currIdx != -1) { 
            // strncpy(taskMsg.filePath, file_list[currIdx], sizeof(taskMsg.filePath) - 1);
            snprintf(taskMsg.filePath, sizeof(taskMsg.filePath), "%s/%s", MOUNT_POINT, file_list[currIdx]);
            taskMsg.filePath[sizeof(taskMsg.filePath) - 1] = '\0';
            snprintf(taskMsg.displayMsg, sizeof(taskMsg.displayMsg), "Playing:\n%s", file_list[currIdx]);
            if(xQueueSend(xPlaybackDisplayQueue, (void *) &taskMsg, (TickType_t) 10 ) != pdPASS ) {
                ESP_LOGE("button", "Failed to queue task");
            }
        }
    }
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
    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_UP, button_long_press_cb, NULL));
    ESP_LOGI("button", "Button callbacks registered");
    return ESP_OK;
}
