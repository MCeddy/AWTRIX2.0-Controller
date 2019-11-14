# AWTRIX

## Description

AWRTIX is a smart LED matrix display who communicate with a server via MQTT protocol.

## Features

- smart dot matrix
- MQTT communication (easy interaction with [node-red](https://nodered.org/))
- easy setup (first boot in hotspot mode and configure to access your wifi)
- internal temperature and humidity sensor
- over the air (OTA) updates

## Parts

- Wemos D1 mini/ Wemos D1 R2 or other ESP8266 dev board
- light resistor
- 2x 10k resistors
- many wires in different colors
- push button
- 32x8 WS2812 LED matrix
- DHT11 or DHT22 (better) sensor
- min 6A 5V power supply

## Wiring

![sketch](/sketch_bb.png)

## Case

Be creative! :wink:

## Server

- currently: 100% compatible with [offical AWTRIX Server](https://docs.blueforcer.de/#/v2/host) (Version: 01619)
