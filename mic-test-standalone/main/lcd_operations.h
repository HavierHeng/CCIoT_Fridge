#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "lcd.h"
#include "driver/i2c.h"

lcd_handle_t lcd_handle = LCD_HANDLE_DEFAULT_CONFIG();

static esp_err_t init_lcd(void)
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    // Initialise i2c
    ESP_LOGD("lcd", "Installing i2c driver in master mode on channel %d", I2C_MASTER_NUM);
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGD("lcd",
             "Configuring i2c parameters.\n\tMode: %d\n\tSDA pin:%d\n\tSCL pin:%d\n\tSDA pullup:%d\n\tSCL pullup:%d\n\tClock speed:%.3fkHz",
             i2c_config.mode, i2c_config.sda_io_num, i2c_config.scl_io_num,
             i2c_config.sda_pullup_en, i2c_config.scl_pullup_en,
             i2c_config.master.clk_speed / 1000.0);
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_config));

    // Modify default lcd_handle details
    lcd_handle.i2c_port = I2C_MASTER_NUM;
    lcd_handle.address = LCD_ADDR;
    lcd_handle.columns = LCD_COLUMNS;
    lcd_handle.rows = LCD_ROWS;
    lcd_handle.backlight = LCD_BACKLIGHT;

    // Initialise LCD
    ESP_ERROR_CHECK(lcd_init(&lcd_handle));

    return ESP_OK;
}

esp_err_t lcd_write_screen(lcd_handle_t *handle, char *str) {
    lcd_clear_screen(handle);

    int current_row = 0;
    int current_col = 0;
    lcd_home(handle);
    int max_rows = handle->rows;
    int max_columns = handle->columns;

    // Temporary buffer to hold the string to be written
    char buffer[max_columns + 1]; // +1 for null-terminator
    int buffer_index = 0;

    while (*str) {
        char c = *str++;

        if (c == '\n' || buffer_index == max_columns) {
            // Write the current buffer content
            buffer[buffer_index] = '\0'; // Null-terminate the string
            lcd_write_str(handle, buffer);

            // Move to the next line if newline is encountered or buffer is full
            current_row++;
            current_col = 0; // Reset column for new line

            // Check for out of bounds
            if (current_row >= max_rows){
                return ESP_FAIL;
            }
            lcd_set_cursor(handle, current_col, current_row);

            // Reset buffer and index for next line
            buffer_index = 0;
        }

        // Add character to the buffer if not end of screen
        if (buffer_index < max_columns) {
            buffer[buffer_index++] = c;
        }
    }

    // If there's any leftover text in the buffer, write it
    if (buffer_index > 0) {
        lcd_write_str(handle, buffer);
    }
    return ESP_OK;
}

