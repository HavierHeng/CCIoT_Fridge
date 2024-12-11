/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.
// The example has been extended to do a few simple things
// On short button press -> Move forward through files in SD Card, and display on i2c screen
// On double button -> Move backwards through files in SD Card, and display on i2c screen
// On long button press -> Plays the wav file and display "Playing \n<file_name>" on i2c screen. Figure out how to split to new line on either newlines or if the number of characters exceed the number of columns set
// This is display and playing is done on another task - and if its finished it will safely unset a state variable protected by a pthread rw mutex

#include <string.h>
#include "audio_operations.h"
#include "esp_err.h"
#include "file_operations.h"
#include "lcd_operations.h"
#include "button_operations.h"
#include "task_operations.h"

void app_main(void) {
    ESP_ERROR_CHECK(init_lcd());
    ESP_ERROR_CHECK(init_i2s_spkr());
    ESP_ERROR_CHECK(init_buttons());
    ESP_ERROR_CHECK(init_sdcard());
    ESP_ERROR_CHECK(init_queue());
    load_file_list();
    register_btn_cb();
    ESP_ERROR_CHECK(init_tasks());
    // All done, unmount partition and disable SDMMC peripheral
    // esp_vfs_fat_sdcard_unmount(mount_point, card);
    // ESP_LOGI(TAG, "Card unmounted");
    lcd_write_screen(&lcd_handle, "WOLOLOLOLOLOOLOLOLOLO");
}
