# Description

The fridge magnet is to take in recordings of the user voice on a button press, and then store it into SD Card. It will then send the file to a S3 Bucket. 
The fridge magnet also subscribes to a MQTT channel for URL for MP3 of Text to Speech wav files.

# Setup 
Board used is an ESP32-S3. To flash it, plug into the port labelled COM or UART (not the USB port as that is for USB slave purposes).
Get ESP-IDF v5.2, and activate the IDF environment.

Be careful to not plug the ESP32 in while the speaker amplifier is connected, just in case it draws current from your USB port.

# Firmware and Pinouts

## GPIO Button

Button is pulled down. 
|  GPIO | Purpose |
| --- | --- |
| 21  | Input |

Firmware: [espressif/esp-iot-solution/button](https://github.com/espressif/esp-iot-solution/blob/c683b16a/README.md)
- Example of how to download this dependency from IDF Component Manager: `idf.py add-dependency espressif/button`
- To download as managed_component (it helps your linter/language server pick up the headers without doing a build): `idf.py update-dependencies`

## QSPI SD Card

Run as 1-line mode with SDMMC (not SDSPI). i.e with SCK, MOSI, MISO(D0), CS. 

You may run as 4-line mode if your SD Card PCB has extra pins for D1, D2, D3. Otherwise, 1-Line mode involves pulling up D3/Chip Select (CS). Chip select is normally pulled high, which disconnects it from the SPI bus, but pulled low to activate the peripheral.

For generic reference of how the naming of the Pins work (just in case your SD card reader doesn't match the ESP docs), you may refer to this [link](https://www.pololu.com/product/2597) or the table below. All pull up/downs are 10k resistors unless otherwise specified. 1 and 4 Line are the same SD Card PCB, its just that the bindings of the pins are treated slightly differently.

It is also possible for some SD Card PCBs to come with a nice Card Detect (CD) pin for quality of life, with pull up resistors, this pin can indicate if a card is inserted (high) or not inserted (low).


| 1 Line PCB with 6 Pins (including GND and VCC) | 4 Line Pololu (SPI Mode) | 4 Line Polulu (SDMMC Mode) | ESP SDMMC Docs (1/4 Line)                     |
| :--------------- | :----------------------- | :---------------------------- | :------------------------------------------ |
| VCC              | VCC                      | VCC                       | VCC                                         |
| GND              | GND                      | GND                           | GND                                         |
| MISO             | DO                       | DAT0                          | D0                                          |
| MOSI             | DI                       | CMD                           | CMD                                         |
| SCK              | SCLK                     | CLK                           | CLK                                         |
| CS               | CS (active low)          | DAT3                          | D3 (Pulled up in 1-Line, not actually used) |
|                  | IRQ (active low)         | DAT1                          | D1 (Pulled up in 4-Line, else NC)           |
|                  |                          | DAT2                          | D2 (Pulled up in 4-Line, else NC)           |
|                  | Card Detect (CD)         | Card Detect (CD)              |                                             |


ESP32-S3 uses a GPIO matrix, so technically all these pins are just a suggestion for SDMMC which uses the onboard MMC controller instead of bit banging like in SDSPI, and is not hardcoded in any way.

|  GPIO | Purpose |
| --- | ---|
| 35  | MOSI/DI/CMD|
| 36  | MISO/DO |
| 37  | CLK/SCLK |
| 38  | CS - Needs to be pulled up in 1-Line mode |

The bigger the SD card, the longer the .wav files can be stored for recording and playback. For a mono audio, 16 bit depth, 44.1kHz sample rate, this takes up to 882000 bytes/s of recording, so for a 128MB SD Card, its approximately 152 seconds of recordings. For a bigger SD card, e.g 4GB partition (which is the upper bound for Fat32), you can squeeze in approx 13.5 hours of recordings. Though since the recordings and playback audio are constantly being deleted, its more so the max length that can be recorded or played back.

Wav files are all using PCM Format, 16 bit depth, mono recordings. For recordings, sample rate will either be 16000Hz, due to the limitations of recording over I2S on ESP32. For playback, sample rate will be 41000Hz. The reason for why its so strict is just because it is hardcoded (^_-)-☆, the wav files header is hardcoded and so is the playback wav file parsing! 

One last gimmick is the length of file name - its default unless set in components is 16 chars + 3 chars extension. Otherwise, you cannot open the file. This can be edited in menuconfig > Component config > FAT Filesystem support.

Firmware: 
- [FAT FS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/fatfs.html)
- [SDMMC Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/sdmmc.html)
- [Wav file format](https://docs.fileformat.com/audio/wav/)


## 16x2 HD44780 LCD Screen

For this screen, there is an optional backlight which can only take up to 30mA at max. While this backlight (on/off) is controlled via commands, its brightness solely dependent on a resistor in series with the power pins. At 3.3V, a resistor of around 100-150 Ohm is a good first pick. The LED power pin is quite unlabelled, but from testing it appears that you only have to plug the 3.3V power (in series with the resistor) into the top LED pin (idk what the bottom one is for).

If the screen text cannot be read, use a small screwdriver on the potentiometer on the back, it adjusts contrast. 

|  GPIO | Purpose |
| --- | ---|
| 1 | SDA|
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

WAV files have to be strictly stereo, 16 bit depth, 44100Hz to be played. Otherwise, something might go wrong. Stereo actually makes it easier to play since it ensures that every 2 bytes are not flipped.

To get stereo output from one amplifier:
- If SD is connected to ground directly (voltage is under 0.16V) then the amp is shut down
- If the voltage on SD is between 0.16V and 0.77V then the output is (Left + Right)/2, that is the stereo average. 
- If the voltage on SD is between 0.77V and 1.4V then the output is just the Right channel
- If the voltage on SD is higher than 1.4V then the output is the Left channel.

For this rule to work, calculate the voltage divider considering the internal 100K pulldown resistor on SD pin. Aiming for 0.5V on SD pin for stereo, then a good value is around 1 mega Ohm based on [this calculator](https://ohmslawcalculator.com/voltage-divider-calculator). This is already accounted for in the breakout board though.

Firmware Reference: [Parabuzzle/esp-idf-simple-audio-player](https://github.com/parabuzzle/esp-idf-simple-audio-player/tree/main)

## INMP441 Microphone
|  GPIO | Purpose |
| --- | ---|
| 5  | WS/Left Right CLK|
| 6  | Serial Data (SD) |
| 7  | Serial Clock (SCK) |

For the microphone, its goal is to record mono audio, hence L/R pin is also connected to GND. 

Firmware Reference: [ThatProject/ESP32_MICROPHONE/Speech_to_text - For the Wav file header calculations](https://github.com/0015/ThatProject/blob/master/ESP32_MICROPHONE/ESP32_INMP441_SPEECH_TO_TEXT/ESP32_INMP441_RECORDING_UPLOAD_TO_SERVER/ESP32_INMP441_RECORDING_UPLOAD_TO_SERVER.ino)

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


