#include "speaker.h"

const char *TAG = "speaker";

esp_err_t spkr_i2s_setup(void)
{
  // setup a standard config and the channel
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &spkr_tx_handle, NULL));

  // setup the i2s config
  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),                                                    // the wav file sample rate
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO), // the wav faile bit and channel config
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,  
          .bclk = SPKR_BCLK,  // Bit clock for syncing bits
          .ws = SPKR_LRC,  // Left Right clock
          .dout = SPKR_DATA,  // Data out 
          .din = I2S_GPIO_UNUSED,  // Not used
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };
  return i2s_channel_init_std_mode(spkr_tx_handle, &std_cfg);
}

esp_err_t spkr_play_wav(char *fp)
{
  FILE *fh = fopen(fp, "rb");
  if (fh == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file");
    return ESP_ERR_INVALID_ARG;
  }

  // skip the header...
  fseek(fh, 44, SEEK_SET);

  // create a writer buffer
  int16_t *buf = calloc(SPKR_BUFFER_SIZE, sizeof(int16_t));
  size_t bytes_read = 0;
  size_t bytes_written = 0;

  bytes_read = fread(buf, sizeof(int16_t), SPKR_BUFFER_SIZE, fh);

  i2s_channel_enable(spkr_tx_handle);

  while (bytes_read > 0)
  {
    // write the buffer to the i2s
    i2s_channel_write(spkr_tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    bytes_read = fread(buf, sizeof(int16_t), SPKR_BUFFER_SIZE, fh);
    ESP_LOGV(TAG, "Bytes read: %d", (int) bytes_read);
  }

  i2s_channel_disable(spkr_tx_handle);
  free(buf);

  return ESP_OK;
}
