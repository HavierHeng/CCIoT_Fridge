#pragma once

#include "driver/i2s_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "pinouts.h"
#include "../sd_card/sd_card.h"
#include "driver/i2s_std.h" // i2s setup
#include "driver/gpio.h"

esp_err_t spkr_i2s_setup(void);
esp_err_t spkr_play_wav(char *fp);


#define SPKR_AUDIO_FILE CONFIG_SPKR_AUDIO_FILE MOUNT_POINT
#define SPKR_BUFFER_SIZE CONFIG_SPKR_BUFFER_SIZE
extern i2s_chan_handle_t spkr_tx_handle;
