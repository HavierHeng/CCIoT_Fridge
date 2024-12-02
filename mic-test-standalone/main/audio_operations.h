#pragma once
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "file_operations.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "hal/i2s_types.h"
#include <pthread.h>
#include "shared_states.h"
#include "file_operations.h"

#define AUDIO_BUFFER 2048

// Speaker is hard coded since I know that 44100, 16 bit depth, stereo, is the perfect setting
// Double buffering - when one is being downloaded, the other is being played
#define PLAY_BUFFER_1 MOUNT_POINT "/playback_1.wav"
#define PLAY_BUFFER_2 MOUNT_POINT "/playback_2.wav"

// For testing mic
#define MIC_SAMPLE_RATE 44100  // Sample rate
#define MIC_NUM_CHANNELS 1  // 1 for mono, 2 for stereo
#define MIC_BITS_PER_SAMPLE 16  // Actually needa convert this to I2S values
#define RECORD_DURATION 5       // Record for 5 seconds
#define MAX_BYTES_TO_RECORD (RECORD_DURATION * MIC_SAMPLE_RATE * sizeof(int16_t))

// Double buffering - when one is written to, the other is being sent by HTTP PUT
// These two are being protected by their own semaphores just in case the simultaneous write/send is performed since its done by two different tasks
#define WAV_BUFFER_1 MOUNT_POINT "/recording_1.wav"
#define WAV_BUFFER_2 MOUNT_POINT "/recording_2.wav"

i2s_chan_handle_t tx_handle;  // Speaker handle
i2s_chan_handle_t rx_handle;  // Microphone handle

esp_err_t init_i2s_spkr() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),                                                    // the wav file sample rate
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO), 
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = 16,
          .ws = 17,
          .dout = 15,
          .din = I2S_GPIO_UNUSED,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };
  return i2s_channel_init_std_mode(tx_handle, &std_cfg);
}

esp_err_t init_i2s_mic() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
  i2s_std_config_t std_rx_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(MIC_BITS_PER_SAMPLE, MIC_NUM_CHANNELS),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = GPIO_NUM_7,
          .ws = GPIO_NUM_5,
          .dout = I2S_GPIO_UNUSED,
          .din = GPIO_NUM_6,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };
  return i2s_channel_init_std_mode(rx_handle, &std_rx_cfg);
}

// Wav has to be 44.1kHz, stereo, 16 bit depth.
esp_err_t play_wav(char* fp) {
  FILE *fh = fopen(fp, "rb");
  if (fh == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for playing");
    return ESP_ERR_INVALID_ARG;
  }

  // skip the header which is precisely 44 bytes long - by right parsing it would mean picking out 16 bits or 32 bits depth and dealing with the audio accordingly
  fseek(fh, 44, SEEK_SET);

  // create a writer buffer - Hard coded to be 16 bits - for reasoning to avoid the swapping every 2 frames
  int16_t *buf = calloc(AUDIO_BUFFER, sizeof(int16_t));
  size_t bytes_read = 0;
  size_t bytes_written = 0;

  bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);

  i2s_channel_enable(tx_handle);

  while (bytes_read > 0)
  {
    // write the buffer to the i2s
    i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);
    ESP_LOGV(TAG, "Bytes read: %d", (int) bytes_read);
  }

  i2s_channel_disable(tx_handle);
  fclose(fh);
  free(buf);

  return ESP_OK;
}

esp_err_t stream_audio_to_wav(FILE *file) {
    int16_t *buf = calloc(AUDIO_BUFFER, sizeof(int16_t));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for buffer.");
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = 0;
    size_t bytes_written = 0;
    size_t total_bytes_written = 0;

    // Enable the I2S channel for RX (assuming `rx_handle` is properly initialized)
    i2s_channel_enable(rx_handle);

    // Calculate the maximum number of bytes to record based on the duration
    size_t max_bytes_to_record = MAX_BYTES_TO_RECORD;
    size_t total_bytes_read = 0;

    // Keep a pointer to PlaybackDisplayMsg
    playbackDisplayMsg_t playbackMsg;

    while (total_bytes_read < max_bytes_to_record) {
        // Peek to see if there is things to play, and if so preterminate
        if (xQueuePeek(xPlaybackDisplayQueue, &playbackMsg, 0) == pdPASS) {
            ESP_LOGE(TAG, "Preterminating recording due to playback on Queue");
            toggle_isPlaying();
            break;
        }

        // Read audio data from the I2S channel
        esp_err_t ret = i2s_channel_read(rx_handle, buf, AUDIO_BUFFER * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read failed: %d", ret);
            break;
        }

        // If no data read, continue (avoid unnecessary writes)
        if (bytes_read == 0) {
            continue;
        }

        // Write the audio data to the file
        bytes_written = fwrite(buf, 1, bytes_read, file);
        if (bytes_written != bytes_read) {
            ESP_LOGE(TAG, "Failed to write all data to file. Written: %d, Expected: %d", (int)bytes_written, (int)bytes_read);
            break;
        }

        total_bytes_read += bytes_written;

        ESP_LOGV(TAG, "Bytes read: %d, Bytes written: %d", (int)bytes_read, (int)bytes_written);
    }

    // Disable the I2S channel after recording
    i2s_channel_disable(rx_handle);
    // Free the allocated buffer
    free(buf);

    ESP_LOGI(TAG, "Audio recording complete. Total bytes written: %d", (int)total_bytes_written);

    return ESP_OK;
}

// 44 Byte WAV header
typedef __attribute__((packed)) struct {
    char riff[4];             // "RIFF"
    uint32_t file_size;       // File size - 8
    char wave[4];             // "WAVE"
    char fmt_chunk[4];        // "fmt "
    uint32_t fmt_chunk_size;  // Format chunk size (16 for PCM)
    uint16_t audio_format;    // Audio format (1 for PCM)
    uint16_t num_channels;    // Number of channels
    uint32_t sample_rate;     // Sample rate
    uint32_t byte_rate;       // Byte rate = SampleRate * NumChannels * BitsPerSample / 8
    uint16_t block_align;     // Block align = NumChannels * BitsPerSample / 8
    uint16_t bits_per_sample; // Bits per sample (usually 16)
    char data_chunk[4];       // "data"
    uint32_t data_size;       // Size of the data
} wav_header_t;


// Calculate WAV Header fields
esp_err_t write_wav_header(FILE *file) {
    wav_header_t header;
    int placeholder_data_size = 0;  // This will be updated later on
    size_t object_written = 0;

    // Initialize the header with appropriate values
    memcpy(header.riff, "RIFF", 4);
    header.file_size = 36 + placeholder_data_size; // Total file size - 8 (header size)
    memcpy(header.wave, "WAVE", 4);

    memcpy(header.fmt_chunk, "fmt ", 4);
    header.fmt_chunk_size = 16; // PCM format size
    header.audio_format = 1;    // PCM format
    header.num_channels = MIC_NUM_CHANNELS;
    header.sample_rate = MIC_SAMPLE_RATE;
    header.byte_rate = MIC_SAMPLE_RATE * MIC_NUM_CHANNELS * (MIC_BITS_PER_SAMPLE / 8);
    header.block_align = MIC_NUM_CHANNELS * (MIC_BITS_PER_SAMPLE / 8);
    header.bits_per_sample = MIC_BITS_PER_SAMPLE;

    memcpy(header.data_chunk, "data", 4);
    header.data_size = placeholder_data_size;

    // Write the header to the file
    object_written = fwrite(&header, sizeof(wav_header_t), 1, file);
    ESP_LOGI("wav", "wav_header written: %zu bytes\n", object_written * sizeof(wav_header_t));  // I need to validate it actually packed to 44 bytes, no more, no less

    if (object_written != 1) {
        return ESP_FAIL;
    }
    ESP_LOGI("wav", "Size of wav_header: %zu bytes\n", sizeof(wav_header_t));  // I need to validate it actually packed to 44 bytes, no more, no less
    return ESP_OK;
}


esp_err_t save_wav_file(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error opening file for writing.\n");
        return ESP_FAIL;
    }

    // First, write the WAV header, with placeholders for data_size and file_size 
    ESP_ERROR_CHECK(write_wav_header(file));

    ESP_ERROR_CHECK(stream_audio_to_wav(file));

    // Rewind the file and update the header with the correct data size
    fseek(file, 0, SEEK_SET);  // Rewind to the start of the file
    long file_size = ftell(file);  // Get the total file size

    // Write the correct data size in the header (calculate it after writing all data)
    fseek(file, 4, SEEK_SET);  // Seek to the "file_size" position in the header
    uint32_t final_file_size = file_size - 8;  // File size is without RIFF and WAV Header
    fwrite(&final_file_size, sizeof(final_file_size), 1, file);  // Write the corrected file size

    // Update the data size in the header
    fseek(file, 40, SEEK_SET);  // Seek to the "data_size" position in the header
    uint32_t data_size = file_size - sizeof(wav_header_t);
    fwrite(&data_size, sizeof(data_size), 1, file);  // Write the data chunk size

    // Cleanup - Close the file
    fclose(file);
    printf("WAV file saved successfully: %s\n", filename);
    return ESP_OK;
}
