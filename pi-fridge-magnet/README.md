# Pi Fridge Magnet

## Description

Given the difficulties in implementing the original fridge magnet in ESP32 using ESP-IDF, due to the hardware limitations and software complexity, this reimplementation aims to do so via a Raspberry Pi instead. While that project aims to 

This provides more WiFi bandwidth and memory to upload and download audio files as well as aggregate data from the weight module which uses an ESP32. It also provides an abstraction over hardware, file formats (e.g WAV files) and string parsing (e.g for JSON and URLs), which makes development easier.

## Design

### Hardware

Hardware design is virtually the same as the ESP32-S3 configuration. 
- INMP441 I2S Microhpone
- MAX98357A I2S Amplifier and 8 Ohm Speaker Speakers
- 16x2 HD44780 LCD Screen with PC8574 I2C IO expander 
- GPIO Button for user to click

Components that are now abstracted nicely by the Linux kernel are:
- Storage (From SD card initialization to filesystem)
- Networking


### Software design

Also the same state machine as what was designed to be written in ESP-IDF. 

Using a system with a full Linux kernel also enables the ability to containize the application using rootful docker containers. Use of rootful containers is important as many of these peripherals require hardware level access.

The code is mostly written in Python with some minor setup to modify the Linux drivers via loading new device trees for I2S to work. This is a lot easier as well as Python has robust REST libraries, and can even run AWS Client libraries if needed.

## Setup
The parts that need the most setup are the I2S interfaces and I2C interface. These require custom kernel modules and device trees to get working.

I2S in general: https://learn.adafruit.com/adafruit-i2s-mems-microphone-breakout/raspberry-pi-wiring-test

INMP441 Microphone: https://makersportal.com/blog/recording-stereo-audio-on-a-raspberry-pi

MAX98357A Amplifier: https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/raspberry-pi-usage

Allowing I2C for the LCD screen: https://tutorials-raspberrypi.com/control-a-raspberry-pi-hd44780-lcd-display-via-i2c/

Library for I2C: https://github.com/dhylands/python_lcd/tree/master
