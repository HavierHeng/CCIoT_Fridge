#pragma once

// This stupid file is where I define all my getter and setters for thread safe access and modification

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include <pthread.h>
#include <string.h>
#include "esp_log.h"
#include "lcd_operations.h"
#include "file_operations.h"

/* Playing state */
static bool isPlaying = false;  // If isPlaying, then do not record
pthread_rwlock_t playingRWLock = PTHREAD_RWLOCK_INITIALIZER;

// Function to safely get the `isPlaying` state (using read lock) and store into a state buffer
esp_err_t get_isPlaying(bool *state) {
    if (pthread_rwlock_rdlock(&playingRWLock) == 0) {
        *state = isPlaying;  // Read the state
        pthread_rwlock_unlock(&playingRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire read lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Function to safely set the `isPlaying` state (using write lock)
esp_err_t set_isPlaying(bool state) {
    if (pthread_rwlock_wrlock(&playingRWLock) == 0) {
        isPlaying = state;  // Write the state
        pthread_rwlock_unlock(&playingRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t toggle_isPlaying() {
    if (pthread_rwlock_wrlock(&playingRWLock) == 0) {
        isPlaying = !isPlaying;  // Write the state
        pthread_rwlock_unlock(&playingRWLock);  // Release the lock
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* Audio File states */
SemaphoreHandle_t record_buf_semaphore_1 = NULL;
SemaphoreHandle_t record_buf_semaphore_2 = NULL;
SemaphoreHandle_t playback_buf_semaphore_1 = NULL;
SemaphoreHandle_t playback_buf_semaphore_2 = NULL;

esp_err_t init_i2s_semaphores() {
    // For file operations, read/write protection
    // These protect WAV_BUFFER_1/2 and PLAY_BUFFER1/2 files
    record_buf_semaphore_1 = xSemaphoreCreateMutex();
    record_buf_semaphore_2 = xSemaphoreCreateMutex();
    playback_buf_semaphore_1 = xSemaphoreCreateMutex();
    playback_buf_semaphore_2 = xSemaphoreCreateMutex();
    if (record_buf_semaphore_1 == NULL 
        || record_buf_semaphore_2 == NULL
        || playback_buf_semaphore_1 == NULL
        || playback_buf_semaphore_2 == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

pthread_rwlock_t record_to_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t play_to_rwlock = PTHREAD_RWLOCK_INITIALIZER;

bool recordTo = true;  // True means record to WAV_BUFFER_1, while uploading WAV_BUFFER_2 
bool playTo = true;  // True means play PLAY_BUFFER_1, while downloading to PLAY_BUFFER_2

esp_err_t get_recordTo(bool *state) {
  if (pthread_rwlock_rdlock(&record_to_rwlock) == 0) {
      *state = recordTo;  // Read the state
      pthread_rwlock_unlock(&record_to_rwlock);  // Release the lock
  } else {
      ESP_LOGE("Lock", "Failed to acquire read lock");
      return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t set_recordTo(bool state) {
    if (pthread_rwlock_wrlock(&record_to_rwlock) == 0) {
        recordTo = state;
        pthread_rwlock_unlock(&record_to_rwlock);
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Most of the time will just be toggling states
esp_err_t toggle_recordTo() {
    if (pthread_rwlock_wrlock(&record_to_rwlock) == 0) {
        recordTo = !recordTo;
        pthread_rwlock_unlock(&record_to_rwlock);
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t get_playTo(bool *state) {
  if (pthread_rwlock_rdlock(&record_to_rwlock) == 0) {
      *state = playTo;  // Read the state
      pthread_rwlock_unlock(&record_to_rwlock);  // Release the lock
  } else {
      ESP_LOGE("Lock", "Failed to acquire read lock");
      return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t set_playTo(bool state) {
    if (pthread_rwlock_wrlock(&record_to_rwlock) == 0) {
        playTo = state;
        pthread_rwlock_unlock(&record_to_rwlock);
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Most of the time will just be toggling states
esp_err_t toggle_playTo() {
    if (pthread_rwlock_wrlock(&record_to_rwlock) == 0) {
        playTo = !playTo;
        pthread_rwlock_unlock(&record_to_rwlock);
    } else {
        ESP_LOGE("Lock", "Failed to acquire write lock");
        return ESP_FAIL;
    }
    return ESP_OK;
}


/* Task queues - Playback and Display */
QueueHandle_t xPlaybackDisplayQueue;

typedef struct playbackDisplayMsg {
    bool playback_file;  // True is PLAY_BUFFER_1, false is PLAY_BUFFER_2
    char displayMsg[LCD_ROWS * LCD_COLUMNS + 1];  // The extra space is for null terminator
} playbackDisplayMsg_t;


/* Task queue - Uploading Recorded Wav */
QueueHandle_t xUploadQueue;

typedef struct uploadMsg {
    char filePath[MAX_CHAR_SIZE]; 
} uploadMsg_t;

esp_err_t init_queues() {
    xPlaybackDisplayQueue = xQueueCreate(10, sizeof(playbackDisplayMsg_t));
    xUploadQueue = xQueueCreate(10, sizeof(uploadMsg_t));
    if (xPlaybackDisplayQueue == 0 || xUploadQueue == 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* State machines */
