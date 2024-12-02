# SD Card (SDMMC), Button, I2C screen, FreeRTOS, Amplifier all in one test

This example is a modified version of the SDMMC example for me to read and play wav files.

The things it does so far is:
- Use a button and callbacks for next (Single click), back (double click), and select (Long Press)
- I2C 16x2 screen - shows the current file
- SD Card on 1 line mode - to play audio out of
- Audio amplifier - deafens people with the audio
- Tasks and Queues - to display and play audio. Can queue multiple plays as well.
- Synchronized Index states with RWMutex

The other things to test is
- HTTP Client - PUT and GET via another 2 tasks, and sync blocking of using downloading files
- JSMN json parsing
- MQTT - this one in main project
- Moar queues
- Async i2s microphones

## To configure in menuconfig

Configure screen pinout
Configure SD card pinout
Configure Long file names


## Some notes from this playtest

Set yo configs - so many things are off if you don't do so. e.g screen pins, sd card pin and so on.

For breakout boards, resistors are optional for the MAX98357A SD pin, it will play in stereo by default. Pullup Resistors are also optional on the I2C 16x2 screen for SDA and SCL, this is configured in software.

The stupid LCD Handle should use the LCD_HANDLE_DEFAULT_CONFIG - otherwise, the screen will not initialize correctly - in my case it was using 1 row, with a stupid high contrast.

Queues block - which is good. So my task after checking if not playing will block out.


Remember to prepend the mount path before fopen else files will have error. 

snprintf() null terminates automatically, strncpy doesn't if the length to copy is less than the full string. Use snprintf() as much as possible to reduce chances of trouble.

## Hardware

This example requires an ESP32-S3 development board with an SD card slot and an SD card.

Although it is possible to connect an SD card breakout adapter, keep in mind that connections using breakout cables are often unreliable and have poor signal integrity. You may need to use lower clock frequency when working with SD card breakout adapters.

This example doesn't utilize card detect (CD) and write protect (WP) signals from SD card slot.
