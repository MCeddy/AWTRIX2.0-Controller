#include <FS.h> // include the SPIFFS library
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <Fonts/TomThumb.h>
#include <LightDependentResistor.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
//#include "SoftwareSerial.h"
#include <WiFiManager.h>
#include <DoubleResetDetect.h>
#include <EasyButton.h>
#include "SmartDisplay-conf.h"

String version = "0.13.0 beta";

// settings
bool USBConnection = false;
bool MatrixType2 = false;
char mqtt_server[16];
uint16_t mqtt_port;
char mqtt_user[16];
char mqtt_password[16];

bool configLoaded = false;
bool connectedWithServer = false;
bool shouldSaveConfig = false;
bool updating = false;
bool powerOn = true;
bool poweringOff = false;
bool isMqttConnecting = false;

unsigned long myTime; // need for animation
int myCounter;

// timers
unsigned long lastBrightnessCheck = 0;
unsigned long lastMessageFromServer = 0;
unsigned long lastInfoSend = 0;

AsyncMqttClient mqttClient;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

WiFiManager wifiManager;

ESP8266WebServer server(80);
const char *serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

LightDependentResistor photocell(LDR_PIN, LDR_RESISTOR, LDR_PHOTOCELL);

DHT dht(DHT_PIN, DHT_TYPE);
EasyButton button(BUTTON_PIN);
DoubleResetDetect drd(DRD_TIMEOUT, DRD_ADDRESS);

CRGB leds[256];
FastLED_NeoMatrix *matrix;

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR IRAM_ATTR
#endif

void log(const String &message)
{
	if (USBConnection)
	{
		return;
	}

	Serial.println(message);
}

void connectToMqtt()
{
	log("Connecting to MQTT...");

	isMqttConnecting = true;
	mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
	isMqttConnecting = false;

	log("connected to MQTT broker");

	/*Serial.print("Session present: ");
	Serial.println(sessionPresent);

	uint16_t packetIdSub = mqttClient.subscribe("test/lol", 2);
	Serial.print("Subscribing at QoS 2, packetId: ");
	Serial.println(packetIdSub);

	mqttClient.publish("test/lol", 0, true, "test 1");
	Serial.println("Publishing at QoS 0");

	uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
	Serial.print("Publishing at QoS 1, packetId: ");
	Serial.println(packetIdPub1);

	uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
	Serial.print("Publishing at QoS 2, packetId: ");
	Serial.println(packetIdPub2);*/

	mqttClient.subscribe("smartDisplay/client/in/#", 0);
	mqttClient.publish("smartDisplay/client/out/connected", 0, true, "");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
	isMqttConnecting = false;

	log("Disconnected from MQTT.");

	/*if (WiFi.isConnected())
	{
		mqttReconnectTimer.once(2, connectToMqtt);
	}*/
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
	isMqttConnecting = false;

	/*Serial.println("Publish received.");
	Serial.print("  topic: ");
	Serial.println(topic);
	Serial.print("  qos: ");
	Serial.println(properties.qos);
	Serial.print("  dup: ");
	Serial.println(properties.dup);
	Serial.print("  retain: ");
	Serial.println(properties.retain);
	Serial.print("  len: ");
	Serial.println(len);
	Serial.print("  index: ");
	Serial.println(index);
	Serial.print("  total: ");
	Serial.println(total);*/

	String s_payload = String(payload);
	String s_topic = String(topic);
	int last = s_topic.lastIndexOf("/") + 1;
	String channel = s_topic.substring(last);

	DynamicJsonDocument doc(1024);
	deserializeJson(doc, s_payload);

	log("MQTT topic: " + s_topic);
	log("MQTT payload: " + s_payload);

	processing(channel, doc);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
	log("Connected to WiFi.");

	connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
	log("Disconnected from WiFi.");

	//mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
	//wifiReconnectTimer.once(2, connectToWifi);
}

static byte c1; // Last character buffer
byte utf8ascii(byte ascii)
{
	if (ascii < 128) // Standard ASCII-set 0..0x7F handling
	{
		c1 = 0;
		return (ascii);
	}
	// get previous input
	byte last = c1; // get last char
	c1 = ascii;		// remember actual character
	switch (last)	// conversion depending on first UTF8-character
	{
	case 0xC2:
		return (ascii)-34;
		break;
	case 0xC3:
		return (ascii | 0xC0) - 34;
		break;
	case 0x82:
		if (ascii == 0xAC)
			return (0xEA);
	}
	return (0);
}

String utf8ascii(String s)
{
	String r = "";
	char c;
	for (int i = 0; i < s.length(); i++)
	{
		c = utf8ascii(s.charAt(i));
		if (c != 0)
			r += c;
	}
	return r;
}

void utf8ascii(char *s)
{
	int k = 0;
	char c;
	for (int i = 0; i < strlen(s); i++)
	{
		c = utf8ascii(s[i]);
		if (c != 0)
			s[k++] = c;
	}
	s[k] = 0;
}

int GetRSSIasQuality(int rssi)
{
	int quality = 0;

	if (rssi <= -100)
	{
		quality = 0;
	}
	else if (rssi >= -50)
	{
		quality = 100;
	}
	else
	{
		quality = 2 * (rssi + 100);
	}
	return quality;
}

void processing(String type, DynamicJsonDocument doc)
{
	lastMessageFromServer = millis();

	log("MQTT type: " + type);

	String jsonOutput;
	serializeJson(doc, jsonOutput);
	log("MQTT json: " + jsonOutput);

	if (type.equals("show"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->show();
	}
	else if (type.equals("clear"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->clear();
	}
	else if (type.equals("drawText"))
	{
		if (!powerOn)
		{
			return;
		}

		if (doc["font"].as<String>().equals("big"))
		{
			matrix->setFont();
			matrix->setCursor(doc["x"].as<int16_t>(), doc["y"].as<int16_t>() - 1);
		}
		else
		{
			matrix->setFont(&TomThumb);
			matrix->setCursor(doc["x"].as<int16_t>(), doc["y"].as<int16_t>() + 5);
		}
		matrix->setTextColor(matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));

		String text = doc["text"];
		matrix->print(utf8ascii(text));
	}
	else if (type.equals("drawBMP"))
	{
		if (!powerOn)
		{
			return;
		}

		int16_t h = doc["height"].as<int16_t>();
		int16_t w = doc["width"].as<int16_t>();
		int16_t x = doc["x"].as<int16_t>();
		int16_t y = doc["y"].as<int16_t>();

		for (int16_t j = 0; j < h; j++, y++)
		{
			for (int16_t i = 0; i < w; i++)
			{
				matrix->drawPixel(x + i, y, doc["bmp"][j * w + i].as<int16_t>());
			}
		}
	}
	else if (type.equals("drawLine"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->drawLine(doc["x0"].as<int16_t>(), doc["y0"].as<int16_t>(), doc["x1"].as<int16_t>(), doc["y1"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
	}
	else if (type.equals("drawCircle"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->drawCircle(doc["x0"].as<int16_t>(), doc["y0"].as<int16_t>(), doc["r"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
	}
	else if (type.equals("drawRect"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->drawRect(doc["x"].as<int16_t>(), doc["y"].as<int16_t>(), doc["w"].as<int16_t>(), doc["h"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
	}
	else if (type.equals("fill"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->fillScreen(matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
	}
	else if (type.equals("drawPixel"))
	{
		if (!powerOn)
		{
			return;
		}

		matrix->drawPixel(doc["x"].as<int16_t>(), doc["y"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
	}
	else if (type.equals("reset"))
	{
		ESP.reset();
	}
	else if (type.equals("resetSettings"))
	{
		wifiManager.resetSettings();
		ESP.reset();
	}
	else if (type.equals("changeSettings"))
	{
		if (doc.containsKey("mqtt_server"))
		{
			strcpy(mqtt_server, doc["mqtt_server"]);
		}

		if (doc.containsKey("mqtt_port"))
		{
			mqtt_port = doc["mqtt_port"].as<int>();
		}

		if (doc.containsKey("mqtt_user"))
		{
			strcpy(mqtt_user, doc["mqtt_user"]);
		}

		if (doc.containsKey("mqtt_password"))
		{
			strcpy(mqtt_password, doc["mqtt_password"]);
		}

		matrix->clear();
		matrix->setCursor(6, 6);
		matrix->setTextColor(matrix->Color(0, 255, 50));
		matrix->print("SAVE");
		matrix->show();

		delay(2000);

		if (saveConfig())
		{
			ESP.reset();
		}
	}
	else if (type.equals("power"))
	{
		bool oldValue = powerOn;
		powerOn = doc["on"].as<bool>();

		if (oldValue && !powerOn)
		{
			// switched power off
			poweringOff = true;
		}
	}
	else if (type.equals("ping"))
	{
		if (USBConnection)
		{
			Serial.println("ping");
		}
		else
		{
			mqttClient.publish("smartDisplay/client/out/ping", 0, true, "pong");
		}
	}
}

void hardwareAnimatedSearch(int typ, int x, int y)
{
	for (int i = 0; i < 4; i++)
	{
		matrix->clear();
		matrix->setTextColor(0xFFFF);

		if (typ == 0)
		{
			matrix->setCursor(7, 6);
			matrix->print("WiFi");
		}
		else if (typ == 1)
		{
			matrix->setCursor(1, 6);
			matrix->print("Server");
		}

		switch (i)
		{
		case 3:
			matrix->drawPixel(x, y, 0x22ff);
			matrix->drawPixel(x + 1, y + 1, 0x22ff);
			matrix->drawPixel(x + 2, y + 2, 0x22ff);
			matrix->drawPixel(x + 3, y + 3, 0x22ff);
			matrix->drawPixel(x + 2, y + 4, 0x22ff);
			matrix->drawPixel(x + 1, y + 5, 0x22ff);
			matrix->drawPixel(x, y + 6, 0x22ff);
		case 2:
			matrix->drawPixel(x - 1, y + 2, 0x22ff);
			matrix->drawPixel(x, y + 3, 0x22ff);
			matrix->drawPixel(x - 1, y + 4, 0x22ff);
		case 1:
			matrix->drawPixel(x - 3, y + 3, 0x22ff);
		case 0:
			break;
		}

		matrix->show();

		delay(100);
	}
}

void hardwareAnimatedCheck(int typ, int x, int y)
{
	int wifiCheckTime = millis();
	int wifiCheckPoints = 0;

	while (millis() - wifiCheckTime < 2000)
	{
		while (wifiCheckPoints < 7)
		{
			matrix->clear();

			switch (typ)
			{
			case 0:
				matrix->setCursor(7, 6);
				matrix->print("WiFi");
				break;
			case 1:
				matrix->setCursor(1, 6);
				matrix->print("Server");
				break;
			case 2:
				matrix->setCursor(7, 6);
				matrix->print("Temp");
				break;
			case 3:
				matrix->setCursor(3, 6);
				matrix->print("Audio");
				break;
			case 4:
				matrix->setCursor(3, 6);
				matrix->print("Gest.");
				break;
			case 5:
				matrix->setCursor(7, 6);
				matrix->print("LDR");
				break;
			}

			switch (wifiCheckPoints)
			{
			case 6:
				matrix->drawPixel(x, y, 0x07E0);
			case 5:
				matrix->drawPixel(x - 1, y + 1, 0x07E0);
			case 4:
				matrix->drawPixel(x - 2, y + 2, 0x07E0);
			case 3:
				matrix->drawPixel(x - 3, y + 3, 0x07E0);
			case 2:
				matrix->drawPixel(x - 4, y + 4, 0x07E0);
			case 1:
				matrix->drawPixel(x - 5, y + 3, 0x07E0);
			case 0:
				matrix->drawPixel(x - 6, y + 2, 0x07E0);
				break;
			}

			wifiCheckPoints++;

			matrix->show();

			delay(100);
		}
	}
}

void serverSearch(int rounds, int typ, int x, int y)
{
	matrix->clear();
	matrix->setTextColor(0xFFFF);
	matrix->setCursor(1, 6);
	matrix->print("Server");

	if (typ == 0)
	{
		switch (rounds)
		{
		case 3:
			matrix->drawPixel(x, y, 0x22ff);
			matrix->drawPixel(x + 1, y + 1, 0x22ff);
			matrix->drawPixel(x + 2, y + 2, 0x22ff);
			matrix->drawPixel(x + 3, y + 3, 0x22ff);
			matrix->drawPixel(x + 2, y + 4, 0x22ff);
			matrix->drawPixel(x + 1, y + 5, 0x22ff);
			matrix->drawPixel(x, y + 6, 0x22ff);
		case 2:
			matrix->drawPixel(x - 1, y + 2, 0x22ff);
			matrix->drawPixel(x, y + 3, 0x22ff);
			matrix->drawPixel(x - 1, y + 4, 0x22ff);
		case 1:
			matrix->drawPixel(x - 3, y + 3, 0x22ff);
		case 0:
			break;
		}
	}
	else if (typ == 1)
	{

		switch (rounds)
		{
		case 12:
			//matrix->drawPixel(x+3, y+2, 0x22ff);
			matrix->drawPixel(x + 3, y + 3, 0x22ff);
			//matrix->drawPixel(x+3, y+4, 0x22ff);
			matrix->drawPixel(x + 3, y + 5, 0x22ff);
			//matrix->drawPixel(x+3, y+6, 0x22ff);
		case 11:
			matrix->drawPixel(x + 2, y + 2, 0x22ff);
			matrix->drawPixel(x + 2, y + 3, 0x22ff);
			matrix->drawPixel(x + 2, y + 4, 0x22ff);
			matrix->drawPixel(x + 2, y + 5, 0x22ff);
			matrix->drawPixel(x + 2, y + 6, 0x22ff);
		case 10:
			matrix->drawPixel(x + 1, y + 3, 0x22ff);
			matrix->drawPixel(x + 1, y + 4, 0x22ff);
			matrix->drawPixel(x + 1, y + 5, 0x22ff);
		case 9:
			matrix->drawPixel(x, y + 4, 0x22ff);
		case 8:
			matrix->drawPixel(x - 1, y + 4, 0x22ff);
		case 7:
			matrix->drawPixel(x - 2, y + 4, 0x22ff);
		case 6:
			matrix->drawPixel(x - 3, y + 4, 0x22ff);
		case 5:
			matrix->drawPixel(x - 3, y + 5, 0x22ff);
		case 4:
			matrix->drawPixel(x - 3, y + 6, 0x22ff);
		case 3:
			matrix->drawPixel(x - 3, y + 7, 0x22ff);
		case 2:
			matrix->drawPixel(x - 4, y + 7, 0x22ff);
		case 1:
			matrix->drawPixel(x - 5, y + 7, 0x22ff);
		case 0:
			break;
		}
	}

	matrix->show();
}

bool saveConfig()
{
	DynamicJsonDocument doc(1024);
	doc["mqtt_server"] = mqtt_server;
	doc["mqtt_port"] = mqtt_port;
	doc["mqtt_user"] = mqtt_user;
	doc["mqtt_password"] = mqtt_password;
	doc["MatrixType"] = MatrixType2;
	doc["usbWifi"] = USBConnection;
	//doc["matrixCorrection"] = matrixTempCorrection;

	File configFile = SPIFFS.open(CONFIG_FILE, "w");
	if (!configFile)
	{
		log("saveConfig: failed to open config file for writing");
		delay(1000);

		return false;
	}

	serializeJson(doc, configFile);

	configFile.close();

	return true;
}

void loadConfig(DynamicJsonDocument doc)
{
	strcpy(mqtt_server, doc["mqtt_server"]);
	mqtt_port = doc["mqtt_port"].as<int>();
	strcpy(mqtt_user, doc["mqtt_user"]);
	strcpy(mqtt_password, doc["mqtt_password"]);
	USBConnection = doc["usbWifi"].as<bool>();
	MatrixType2 = doc["MatrixType"].as<bool>();
	//matrixTempCorrection = doc["matrixCorrection"].as<int>();

	configLoaded = true;
}

void sendInfo()
{
	DynamicJsonDocument root(1024);
	root["version"] = version;
	root["chipID"] = String(ESP.getChipId());
	root["lux"] = static_cast<int>(round(photocell.getCurrentLux()));
	root["powerOn"] = powerOn;

	// network
	JsonObject network = root.createNestedObject("network");
	network["wifirssi"] = WiFi.RSSI();
	network["wifiquality"] = GetRSSIasQuality(WiFi.RSSI());
	network["wifissid"] = WiFi.SSID();
	network["ip"] = WiFi.localIP().toString();

	// room weather
	JsonObject roomWeather = root.createNestedObject("roomWeather");
	roomWeather["humidity"] = dht.readHumidity();
	roomWeather["temperature"] = dht.readTemperature();

	String JS;
	serializeJson(root, JS);

	if (USBConnection)
	{
		Serial.println(String(JS));
	}
	else
	{
		mqttClient.publish("smartDisplay/client/out/info", 0, true, JS.c_str());
	}

	lastInfoSend = millis();
}

void reconnect()
{
	if (isMqttConnecting)
	{
		return;
	}

	log("reconnecting to " + String(mqtt_server) + ":" + String(mqtt_port));

	String clientId = "SmartDisplay-";
	clientId += String(random(0xffff), HEX);

	//hardwareAnimatedSearch(1, 28, 0);

	// connect to MQTT broker
	isMqttConnecting = true;
	mqttClient.connect();
}

uint32_t Wheel(byte WheelPos, int pos)
{
	if (WheelPos < 85)
	{
		return matrix->Color((WheelPos * 3) - pos, (255 - WheelPos * 3) - pos, 0);
	}
	else if (WheelPos < 170)
	{
		WheelPos -= 85;
		return matrix->Color((255 - WheelPos * 3) - pos, 0, (WheelPos * 3) - pos);
	}
	else
	{
		WheelPos -= 170;
		return matrix->Color(0, (WheelPos * 3) - pos, (255 - WheelPos * 3) - pos);
	}
}

void hardReset()
{
	matrix->clear();
	matrix->setTextColor(matrix->Color(255, 0, 0));
	matrix->setCursor(6, 6);
	matrix->print("RESET!");
	matrix->show();

	delay(1000);

	// remove config file
	SPIFFS.remove(CONFIG_FILE);
	log("settings removed");

	delay(1000);

	// reset settings
	wifiManager.resetSettings();
	ESP.reset();
}

void flashProgress(unsigned int progress, unsigned int total)
{
	matrix->setBrightness(100);
	long num = 32 * 8 * progress / total;

	for (unsigned char y = 0; y < 8; y++)
	{
		for (unsigned char x = 0; x < 32; x++)
		{
			if (num-- > 0)
				matrix->drawPixel(x, 8 - y - 1, Wheel((num * 16) & 255, 0));
		}
	}

	matrix->setCursor(0, 6);
	matrix->setTextColor(matrix->Color(255, 255, 255));
	matrix->print("FLASHING");
	matrix->show();
}

void onButtonPressed()
{
	if (!mqttClient.connected())
	{
		return;
	}

	mqttClient.publish("smartDisplay/client/out/button", 0, true, "pressed");
}

void onButtonPressedForDuration()
{
	hardReset();
}

void saveConfigCallback()
{
	log("Should save config");
	shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
	log("Entered config mode");
	log(myWiFiManager->getConfigPortalSSID());

	matrix->clear();
	matrix->setCursor(3, 6);
	matrix->setTextColor(matrix->Color(0, 255, 50));
	matrix->print("Hotspot");
	matrix->show();
}

void checkBrightness()
{
	if (millis() - lastBrightnessCheck >= 10000) // check every 10 sec
	{
		return;
	}

	float lux = photocell.getCurrentLux();
	long brightness = 255;

	if (lux < 50)
	{
		brightness = map(lux, 0, 50, 20, 255);
	}

	matrix->setBrightness(brightness);
	lastBrightnessCheck = millis();
}

void checkServerIsOnline()
{
	if (!powerOn)
	{
		return;
	}

	if (millis() - lastMessageFromServer >= 60000) // more than one minute no message from server
	{
		matrix->clear();
		matrix->drawLine(0, 3, 31, 3, matrix->Color(255, 0, 0));
		matrix->show();
	}
}

void setup()
{
	delay(2000);

	Serial.setRxBufferSize(1024);
	Serial.begin(115200);

	log("setup");
	log(version);

	// try to read settings
	if (SPIFFS.begin())
	{
		// if file not exists
		if (!SPIFFS.exists(CONFIG_FILE))
		{
			SPIFFS.open(CONFIG_FILE, "w+");
			log("make File...");
		}

		File configFile = SPIFFS.open(CONFIG_FILE, "r");

		if (configFile)
		{
			DynamicJsonDocument doc(1024);
			DeserializationError error = deserializeJson(doc, configFile);

			bool isSuccessfulParsed = error.code() == DeserializationError::Ok;

			if (isSuccessfulParsed)
			{
				log("parsed json");

				loadConfig(doc);
			}

			configFile.close();
		}
	}
	else // not possible to read settings file
	{
		log("mounting not possible");
	}

	log("Matrix Type");

	if (MatrixType2)
	{
		matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);
	}
	else
	{
		matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
	}

	// TODO maybe we can implement setTemperature ("matrixTempCorrection")
	FastLED
		.addLeds<NEOPIXEL, MATRIX_DATA_PIN>(leds, 256)
		.setCorrection(TypicalLEDStrip);

	matrix->begin();
	matrix->setTextWrap(false);
	matrix->setBrightness(80);
	matrix->setFont(&TomThumb);

	if (drd.detect())
	{
		// double reset -> hard reset

		log("** Double reset boot **");

		hardReset();
	}

	wifiManager.setAPStaticIPConfig(IPAddress(172, 217, 28, 1), IPAddress(172, 217, 28, 1), IPAddress(255, 255, 255, 0));

	WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, 16);
	WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", "1883", 6);
	WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user, 16);
	WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password, 16);
	WiFiManagerParameter p_MatrixType2("MatrixType2", "MatrixType 2", "T", 2, "type=\"checkbox\" ", WFM_LABEL_BEFORE);
	WiFiManagerParameter p_USBConnection("USBConnection", "Serial Connection", "T", 2, "type=\"checkbox\" ", WFM_LABEL_BEFORE);

	// Just a quick hint
	WiFiManagerParameter p_hint("<small>Please configure your SmartDisplay Server IP (without Port), and check MatrixType 2 if you cant read anything on the Matrix<br></small><br><br>");
	WiFiManagerParameter p_lineBreak_notext("<p></p>");

	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.setHostname(HOST_NAME);
	wifiManager.addParameter(&p_hint);
	wifiManager.addParameter(&custom_mqtt_server);
	wifiManager.addParameter(&p_lineBreak_notext);
	wifiManager.addParameter(&custom_mqtt_port);
	wifiManager.addParameter(&custom_mqtt_user);
	wifiManager.addParameter(&custom_mqtt_password);
	wifiManager.addParameter(&p_lineBreak_notext);
	wifiManager.addParameter(&p_MatrixType2);
	wifiManager.addParameter(&p_USBConnection);
	wifiManager.addParameter(&p_lineBreak_notext);

	wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
	wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

	if (!USBConnection)
	{
		mqttClient.onConnect(onMqttConnect);
		mqttClient.onDisconnect(onMqttDisconnect);
		mqttClient.onMessage(onMqttMessage);

		mqttClient.setServer(mqtt_server, mqtt_port);
		// TODO maybe check user and password was set
		mqttClient.setCredentials(mqtt_user, mqtt_password);
	}

	hardwareAnimatedSearch(0, 24, 0);

	if (!wifiManager.autoConnect(HOTSPOT_SSID, HOTSPOT_PASSWORD))
	{
		log("failed to connect and hit timeout");
		delay(3000);

		// reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(5000);
	}

	//log("connected to server: " + String(mqtt_server) + ":" + String(mqtt_port));

	server.on("/", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.send(200, "text/html", serverIndex);
	});
	server.on(
		"/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart(); }, []() {
      HTTPUpload& upload = server.upload();
	  
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
		if(!USBConnection){
			Serial.printf("Update: %s\n", upload.filename.c_str());	
		}
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
		  matrix->clear();
		  flashProgress((int)upload.currentSize,(int)upload.buf);
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
		  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		  if(!USBConnection){
			Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
			}
          
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield(); });

	server.begin();

	if (shouldSaveConfig)
	{
		log("saving config");

		strcpy(mqtt_server, custom_mqtt_server.getValue());
		mqtt_port = strtoul(custom_mqtt_port.getValue(), NULL, 10);
		strcpy(mqtt_user, custom_mqtt_user.getValue());
		strcpy(mqtt_password, custom_mqtt_password.getValue());
		MatrixType2 = (strncmp(p_MatrixType2.getValue(), "T", 1) == 0);
		USBConnection = (strncmp(p_USBConnection.getValue(), "T", 1) == 0);

		saveConfig();
		ESP.reset();
	}

	hardwareAnimatedCheck(0, 27, 2);

	delay(1000);

	ArduinoOTA.onStart([&]() {
		updating = true;
		matrix->clear();
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		flashProgress(progress, total);
	});

	ArduinoOTA.begin();

	// init sensors

	photocell.setPhotocellPositionOnGround(false);

	dht.begin();

	button.begin();
	button.onPressed(onButtonPressed);
	button.onPressedFor(12000, onButtonPressedForDuration);

	matrix->clear();
	matrix->setCursor(6, 6);
	matrix->setTextColor(matrix->Color(0, 255, 0));
	matrix->print("Ready!");
	matrix->show();

	myTime = millis() - 500;
	myCounter = 0;
}

void loop()
{
	server.handleClient(); // TODO disable server after init setup finished
	ArduinoOTA.handle();

	checkBrightness();

	if (!connectedWithServer)
	{
		if (USBConnection)
		{
			if (millis() - myTime > 100)
			{
				serverSearch(myCounter, 1, 28, 0);
				myCounter++;

				if (myCounter == 13)
				{
					myCounter = 0;
				}

				myTime = millis();
			}
		}
		else
		{
			if (millis() - myTime > 500)
			{
				serverSearch(myCounter, 0, 28, 0);
				myCounter++;

				if (myCounter == 4)
				{
					myCounter = 0;
				}

				myTime = millis();
			}
		}
	}

	if (!updating)
	{
		if (USBConnection)
		{
			while (Serial.available() > 0)
			{
				connectedWithServer = true;
				String message = Serial.readStringUntil('}') + "}";

				DynamicJsonDocument doc(1024);
				deserializeJson(doc, message);

				String type = doc["type"];
				processing(type, doc);
			};
		}
		else // wifi
		{
			if (mqttClient.connected())
			{
				connectedWithServer = true;

				if (lastInfoSend == 0 || millis() - lastInfoSend >= 45000) // every 45 seconds
				{
					sendInfo();
				}
			}
			else
			{
				reconnect();
			}
		}

		checkServerIsOnline();

		if (poweringOff)
		{
			// clear screen
			matrix->clear();
			matrix->show();

			poweringOff = false;
		}

		button.read();
	}
}
