# Description

The fridge magnet is to take in recordings of the user voice on a button press, and then store it into SD Card. It will then send the file to a S3 Bucket. 
The fridge magnet also subscribes to a MQTT channel for URL for MP3 of Text to Speech wav files.

# Setup 
Board used is an ESP32-S3. To flash it, plug into the port labelled COM or UART (not the USB port as that is for USB slave purposes).
Get ESP-IDF v5.2, and activate the IDF environment.

Be careful to not plug the ESP32 in while the speaker amplifier is connected, just in case it draws current from your USB port.

# Firmware and Pinouts

## GPIO Button

|  GPIO | Purpose |
| --- | --- |
| 21  | Input |

Firmware: [UncleRus/esp-idf-lib:Button](https://github.com/UncleRus/esp-idf-lib/tree/master/components/button)

## QSPI SD Card

Run as 1-line mode. i.e with SCK, MOSI, MISO, CS

|  GPIO | Purpose |
| --- | ---|
| 37  | MISO |
| 35  | MOSI|
| 36  | CLK |
| 39  | CS |

Firmware: 
- [FAT FS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/fatfs.html)
- [SDMMC Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/sdmmc.html)


## 16x2 LCD Screen

|  GPIO | Purpose |
| --- | ---|
| 48 | SDA|
| 2  | SCL|

Firmware: [Bradkeifer/esp32-HD44780](https://github.com/bradkeifer/esp32-HD44780/tree/main)

I2C Address: 0x27
LCD Rows: 2
LCD Columns: 16


## MAX98357A Speaker

|  GPIO | Purpose |
| --- | ---|
| 15  | Serial Data (DIN) |
| 16  | Serial Clock (BCLK) |
| 17  | WS/Left Right CLK (LRC) |

WAV files have to be strictly 16 bit depth, 44100Hz to be played. Otherwise, something might go wrong.

Firmware Reference: [Parabuzzle/esp-idf-simple-audio-player](https://github.com/parabuzzle/esp-idf-simple-audio-player/tree/main)

## INMP441 Microphone
|  GPIO | Purpose |
| --- | ---|
| 5  | WS/Left Right CLK|
| 6  | Serial Data (SD) |
| 7  | Serial Clock (SCK) |


Firmware Reference: [ThatProject/ESP32_MICROPHONE/Speech_to_text](https://github.com/0015/ThatProject/blob/master/ESP32_MICROPHONE/ESP32_INMP441_SPEECH_TO_TEXT/ESP32_INMP441_RECORDING_UPLOAD_TO_SERVER/ESP32_INMP441_RECORDING_UPLOAD_TO_SERVER.ino)

For ESP-IDF v5: [Espressif Docs for I2S](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/peripherals/i2s.html)

## Built in LED WS2812B

|  GPIO | Purpose |
| --- | ---|
| 38  | Default LED on PCB|


# Other Firmware
ESP-MQTT: [ESP-MQTT](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/protocols/mqtt.html)
- ESP-IDF MQTT SSL Mutual Auth Example: https://github.com/espressif/esp-idf/tree/v5.2.3/examples/protocols/mqtt/ssl_mutual_auth
- ESP-AWS-IOT MQTT TLS Mutal Auth Example: https://github.com/espressif/esp-aws-iot/tree/master/examples/mqtt/tls_mutual_auth



# Other notes
## Submodules

To add a submodule component:
`git submodule add <git repo>`

To update submodules:
`git submodule update --init --recursive --progress`

## Setting details
Run `idf.py menuconfig` - Fridge Configuration defines most of the pub/sub topics and other misc details.
Remember to go to example_connections and change out the configurations as well for WiFi SSID and Password.

## Certificates
AmazonRootCA1.pem -> root_cert_auth.crt
xxx-certificate.pem -> client.crt
xxx-private.pem.key -> client.key


