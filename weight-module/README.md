# Weight module code

## Design

Code for MQTT is based off Ameba MQTT code for Arduino: https://github.com/Ameba-AIoT/ameba-arduino-1/blob/dev/Arduino_package/hardware/libraries/MQTTClient/examples/amazon_awsiot_basic/amazon_awsiot_basic.ino


HX711 Code is based off Bodge's HX711 library for Arduino: https://github.com/bogde/HX711

## Hardware

HX711 pinouts are 

| HX711 | ESP32 |
| --- | ---|
| VCC | 5V |
| GND | GND|
| SCK|  4|
| DT | 16 |

