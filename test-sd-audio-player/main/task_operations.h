#pragma once

// Defines all tasks and queues
// In this example, the main task is to check for buttons
// Meanwhile the secondary task is to play audio if there is anything in queue to be played

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "esp_log.h"
#include <pthread.h>
#include "lcd_operations.h"
#include "audio_operations.h"
#include "sdkconfig.h"

static bool isPlaying = false;  // Potentially a shared state
static int fileIdx = 0;  // Shared state between button task and audio task
static pthread_rwlock_t fileIdxRWLock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_rwlock_t playingRWLock = PTHREAD_RWLOCK_INITIALIZER;  // Init rwlock, no need function since static
QueueHandle_t xPlaybackDisplayQueue;

// Function to safely get the `isPlaying` state (using read lock)
bool get_isPlaying() {
    bool currentState = true;  // so even if I can't read lock, no other thread can start
    if (pthread_rwlock_rdlock(&playingRWLock) == 0) {
        currentState = isPlaying;  // Read the state
        pthread_rwlock_unlock(&playingRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire read lock");
    }
    return currentState;
}

// Function to safely set the `isPlaying` state (using write lock)
void set_isPlaying(bool state) {
    if (pthread_rwlock_wrlock(&playingRWLock) == 0) {
        isPlaying = state;  // Write the state
        pthread_rwlock_unlock(&playingRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
    }
}

// Function to safely get the `fileIdx` state
// This is shared across both the button callbacks and the audio task itself
int get_fileIdx() {
    int currentIdx = -1;
    if (pthread_rwlock_rdlock(&fileIdxRWLock) == 0) {
        currentIdx = fileIdx;  // Read the state
        pthread_rwlock_unlock(&fileIdxRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire read lock");
    }
    return currentIdx;
}

void set_fileIdx(int newIdx) {
    if (pthread_rwlock_wrlock(&fileIdxRWLock) == 0) {
        fileIdx = newIdx;  // Read the state
        pthread_rwlock_unlock(&fileIdxRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
    }
}

// Define queue message
typedef struct playbackDisplayMsg {
    char filePath[128];
    char displayMsg[LCD_ROWS * LCD_COLUMNS + 1];  // The extra space is for null terminator
} playbackDisplayMsg_t;


esp_err_t init_queue() {
    xPlaybackDisplayQueue = xQueueCreate(10, sizeof(playbackDisplayMsg_t));  // Screw storing in terms of pointers, it just makes it more unpredictable due to memory lifetimes
    if (xPlaybackDisplayQueue == 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void playback_display_task(void *pvParameters) {
    playbackDisplayMsg_t curr_msg;
    while (1) {
        if (!get_isPlaying()) {
            ESP_LOGI("subtask", "Not playing - will try to get");
            if (xQueueReceive(xPlaybackDisplayQueue, &curr_msg, portMAX_DELAY) == pdPASS) {
                if (curr_msg.filePath[0] != '\0') {
                    set_isPlaying(true);
                    play_wav(curr_msg.filePath);
                    set_isPlaying(false);
                }
                lcd_write_screen(&lcd_handle, curr_msg.displayMsg);
            }
        }
    }
}

esp_err_t init_tasks() {
    // xTaskCreate
    xTaskCreate(playback_display_task, 
                "DisplayBack", 
                5000, 
                NULL, 
                2 | portPRIVILEGE_BIT,
                NULL);
    return ESP_OK;
}
