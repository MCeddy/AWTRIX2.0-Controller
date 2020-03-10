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

- [SmartDisplay Server](https://github.com/MCeddy/SmartDisplay-Server)

## Setup progress

### on Raspberry Pi (Model >= 3)

* Setup lastest [Raspbian](https://www.raspberrypi.org/downloads/raspbian/)
* install [Mosquitto](https://mosquitto.org/download/)
* install [NodeJS](https://tecadmin.net/install-latest-nodejs-npm-on-ubuntu/)
* install git
* checkout [SmartDisplay Server](https://github.com/MCeddy/SmartDisplay-Server) on your home directory: `git clone https://github.com/MCeddy/SmartDisplay-Server.git`
* setup as a service (use `smartdisplay.service` from the repository)

### on display/controller

* power on the display
* upload newest firmare from [SmartDisplay Controller Master Branch](https://github.com/MCeddy/SmartDisplay-Controller/tree/master)
* wait until the flashing progress is finished 
* you should see "HOTSPOT" on the display
* use your smartphone to connect to the SmartDisplay (wifi ssid "SmartDisplay"; password "displayyy").
* use the browser to setup your personal wifi settings, the MQTT settings and save all data.
* the SmartDisplay should now restart and after that it should work.

## Roadmap

- [x] support of username and password login on MQTT servers
- [x] command to change settings
- [x] move brightness control from server to the controller
- [] Support of MQTT over SSL/TSL
