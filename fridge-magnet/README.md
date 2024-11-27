# Firmware and Pinouts

## GPIO Button

|  GPIO | Purpose |
| --- | ---|
|4  | Input|

Firmware: ![UncleRus/esp-idf-lib:Button](https://github.com/UncleRus/esp-idf-lib/tree/master/components/button)

## QSPI SD Card

|  GPIO | Purpose |
| --- | ---|
| 35  | MOSI|
| 37  | MISO |
| 36  | CLK |
| 39  | CS |

Firmware: ![Joltwallet/littlefs](https://components.espressif.com/components/joltwallet/littlefs)

## 16x2 LCD Screen

|  GPIO | Purpose |
| --- | ---|
| 48 | SDA|
| 2  | SCL|

Firmware: ![Bradkeifer/esp32-HD44780](https://github.com/bradkeifer/esp32-HD44780/tree/main)

I2C Address: 0x27
LCD Rows: 2
LCD Columns: 16


## MAX98357A Speaker


|  GPIO | Purpose |
| --- | ---|
| 5  | WS/Left Right CLK|
| 6  | Serial Data (SD) |
| 7  | Serial Clock (SCK) |

Firmware Reference: ![ThatProject/ESP32_MICROPHONE/Wiretap](https://github.com/0015/ThatProject/blob/master/ESP32_MICROPHONE/Wiretap_INMP441_MAX98357A_via_websocket/Server_MAX98357A_TTGO/Server_MAX98357A_TTGO.ino)

For ESP-IDF v5: ![Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/peripherals/i2s.html)

## INMP441 Microphone

|  GPIO | Purpose |
| --- | ---|
| 15  | WS/Left Right CLK|
| 16  | Serial Data (SD) |
| 17  | Serial Clock (SCK) |

Firmware Reference: ![ThatProject/ESP32_MICROPHONE/Speech_to_text](https://github.com/0015/ThatProject/blob/master/ESP32_MICROPHONE/ESP32_INMP441_SPEECH_TO_TEXT/ESP32_INMP441_RECORDING_UPLOAD_TO_SERVER/ESP32_INMP441_RECORDING_UPLOAD_TO_SERVER.ino)

For ESP-IDF v5: ![Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/peripherals/i2s.html)

## Built in LED WS2812B

|  GPIO | Purpose |
| --- | ---|
| 38  | Default LED on PCB|


# Other Firmware
ESP-MQTT: ![ESP-MQTT](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/protocols/mqtt.html)



