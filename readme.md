# SmartDisplay Controller

## Description

SmartDisplay is a smart LED matrix display who communicate with a server via MQTT protocol.

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

- [SmartDisplay Server](https://github.com/MCeddy/SmartDisplay-Server) :see_no_evil: work in progress :see_no_evil:

## Roadmap

- [x] support of username and password login on MQTT servers
- [x] command to change settings
- [ ] move brightness control from server to the controller
