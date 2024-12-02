#pragma once

// Defines all tasks and queues
// In this example, the main task is to check for buttons
// Meanwhile the secondary task is to play audio if there is anything in queue to be played

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "esp_log.h"
#include <pthread.h>
#include "lcd_operations.h"
#include "audio_operations.h"
#include "shared_states.h"
#include "http_operations.h"

// Handles playback and display of message on screen 
void playback_display_task(void *pvParameters) {
    playbackDisplayMsg_t curr_msg;
    bool isPlaying;
    while (1) {
        if (get_isPlaying(&isPlaying) == ESP_FAIL) {
            continue;
        }
        if (isPlaying) {  // If playing mode
            if (xQueueReceive(xPlaybackDisplayQueue, &curr_msg, portMAX_DELAY) == pdPASS) {
                if (curr_msg.playback_file) { // True means first playback
                    if (xSemaphoreTake(playback_buf_semaphore_1, (TickType_t) 0) == pdTRUE) {
                        play_wav(PLAY_BUFFER_1);
                        toggle_playTo();  // Swap front and back buffers
                        xSemaphoreGive(playback_buf_semaphore_1);
                    }
                } else {
                    if (xSemaphoreTake(playback_buf_semaphore_2, (TickType_t) 0) == pdTRUE) {
                        play_wav(PLAY_BUFFER_2);
                        toggle_playTo();  // Swap front and back buffers
                        xSemaphoreGive(playback_buf_semaphore_2);
                    }
                }
                lcd_write_screen(&lcd_handle, curr_msg.displayMsg);
                toggle_isPlaying();  // Indicate that recording can continue
            }
        }
    }
}

// Handles recording audio to SD Card
// Will immediately try to save current recording and give up its turn if it see that xPlaybackDisplayQueue has an available audio file to play
void record_audio_task(void *pvParameters) {
    bool isPlaying;
    bool recordTo;
    while (1) {
        if (get_isPlaying(&isPlaying) == ESP_FAIL) {
            continue;
        }
        if (!isPlaying) {  // If recording mode
            // If there is stuff to play, then stop recording, save to file and let it play by toggling isPlaying - its all handled in stream_audio_to_wav (in save_wav_file)
            if (get_recordTo(&recordTo) == ESP_FAIL) {
                continue;
            }
            uploadMsg_t uploadMsg;

            if (recordTo) {
                if (xSemaphoreTake(record_buf_semaphore_1, (TickType_t) 10) == pdTRUE) {
                    save_wav_file(WAV_BUFFER_1);
                    snprintf(uploadMsg.filePath, sizeof(uploadMsg.filePath), WAV_BUFFER_1);
                    if (xQueueSend(xUploadQueue, (void *) &uploadMsg, (TickType_t) 0) != pdPASS) {
                        ESP_LOGE("task", "Failed to queue task");
                    }
                    xSemaphoreGive(record_buf_semaphore_1);
                    toggle_recordTo();
                } 
            } else {
                if (xSemaphoreTake(record_buf_semaphore_2, (TickType_t) 10) == pdTRUE) {
                    save_wav_file(WAV_BUFFER_2);
                    snprintf(uploadMsg.filePath, sizeof(uploadMsg.filePath), WAV_BUFFER_2);
                    if (xQueueSend(xUploadQueue, (void *) &uploadMsg, (TickType_t) 0) != pdPASS) {
                        ESP_LOGE("task", "Failed to queue task");
                    }
                    xSemaphoreGive(record_buf_semaphore_2);
                    toggle_recordTo();
                }
            }
        }
    }
}

// Handles uploading of audio from SD Card via HTTP PUT to S3 Bucket
void upload_audio_task(void *pvParameters) {
    uploadMsg_t uploadMsg;
    while (1) {
        if (xQueueReceive(xUploadQueue, &uploadMsg, portMAX_DELAY) == pdPASS) {
            // Perform HTTP PUT, but to what file? I think I should increment but by right the signed URL is the one who decides the path
            if (strcmp(uploadMsg.filePath, WAV_BUFFER_1) == 0) {
                if (xSemaphoreTake(record_buf_semaphore_1, (TickType_t) 0) == pdTRUE) {
                    upload_file_from_sd(UPLOAD_URL, uploadMsg.filePath);
                    xSemaphoreGive(record_buf_semaphore_1);
                }
            } 
            if (strcmp(uploadMsg.filePath, WAV_BUFFER_2) == 0) {
                if (xSemaphoreTake(record_buf_semaphore_2, (TickType_t) 0 ) == pdTRUE) {
                    upload_file_from_sd(UPLOAD_URL, uploadMsg.filePath);
                    xSemaphoreGive(record_buf_semaphore_2);
                }
            }
        }
    }
}

// Handles downloading of audio file from S3 Bucket via HTTP GET to SD Card
// By right this one is what the callback button should trigger, then it will put things into xPlaybackTaskQueue once its downloaded
// The thing is in ESP-IDF, I can only send the GET, but my receive is via an event callback
void download_audio_task(void *pvParameters);

esp_err_t init_tasks() {
    // xTaskCreate
    xTaskCreate(playback_display_task, 
                "DisplayBack", 
                5000, 
                NULL, 
                3 | portPRIVILEGE_BIT,
                NULL);
    xTaskCreate(record_audio_task,
                "Record",
                5000,
                NULL,
                2 | portPRIVILEGE_BIT,
                NULL);
    xTaskCreatePinnedToCore(upload_audio_task,
                "Upload",
                5000,
                NULL,
                1 | portPRIVILEGE_BIT,
                NULL,
                1);

    return ESP_OK;
}
