#include <FS.h> // include the SPIFFS library
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
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

String version = "0.10.0 beta";

// settings
bool USBConnection = false;
bool MatrixType2 = false;
char mqtt_server[16];
uint16_t mqtt_port;
char mqtt_user[16];
char mqtt_password[16];

bool configLoaded = false;
bool firstStart = true;
bool shouldSaveConfig = false;
bool updating = false;

unsigned long myTime; // need for animation
int myCounter;

unsigned long lastBrightnessCheck = 0;

// for speed-test
unsigned long startTime = 0;
unsigned long endTime = 0;
unsigned long duration;

WiFiClient espClient;
PubSubClient client(espClient);

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
	switch (last)   // conversion depending on first UTF8-character
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

void log(const String &message)
{
	if (USBConnection)
	{
		return;
	}

	Serial.println(message);
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
	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.createObject();

	json["mqtt_server"] = mqtt_server;
	json["mqtt_port"] = mqtt_port;
	json["mqtt_user"] = mqtt_user;
	json["mqtt_password"] = mqtt_password;
	json["MatrixType"] = MatrixType2;
	json["usbWifi"] = USBConnection;
	//json["matrixCorrection"] = matrixTempCorrection;

	File configFile = SPIFFS.open(CONFIG_FILE, "w");
	if (!configFile)
	{
		log("saveConfig: failed to open config file for writing");
		delay(1000);

		return false;
	}

	json.printTo(configFile);
	configFile.close();

	return true;
}

void loadConfig(JsonObject &json)
{
	strcpy(mqtt_server, json["mqtt_server"]);
	mqtt_port = json["mqtt_port"].as<int>();
	strcpy(mqtt_user, json["mqtt_user"]);
	strcpy(mqtt_password, json["mqtt_password"]);
	USBConnection = json["usbWifi"].as<bool>();
	MatrixType2 = json["MatrixType"].as<bool>();
	//matrixTempCorrection = json["matrixCorrection"].as<int>();

	configLoaded = true;
}

void processing(String type, JsonObject &json)
{
	if (firstStart)
	{
		firstStart = false;
	}

	if (type.equals("show"))
	{
		matrix->show();
	}
	else if (type.equals("clear"))
	{
		matrix->clear();
	}
	else if (type.equals("drawText"))
	{
		if (json["font"].as<String>().equals("big"))
		{
			matrix->setFont();
			matrix->setCursor(json["x"].as<int16_t>(), json["y"].as<int16_t>() - 1);
		}
		else
		{
			matrix->setFont(&TomThumb);
			matrix->setCursor(json["x"].as<int16_t>(), json["y"].as<int16_t>() + 5);
		}
		matrix->setTextColor(matrix->Color(json["color"][0].as<int16_t>(), json["color"][1].as<int16_t>(), json["color"][2].as<int16_t>()));
		String text = json["text"];

		matrix->print(utf8ascii(text));
	}
	else if (type.equals("drawBMP"))
	{
		int16_t h = json["height"].as<int16_t>();
		int16_t w = json["width"].as<int16_t>();
		int16_t x = json["x"].as<int16_t>();
		int16_t y = json["y"].as<int16_t>();

		for (int16_t j = 0; j < h; j++, y++)
		{
			for (int16_t i = 0; i < w; i++)
			{
				matrix->drawPixel(x + i, y, json["bmp"][j * w + i].as<int16_t>());
			}
		}
	}
	else if (type.equals("drawLine"))
	{
		matrix->drawLine(json["x0"].as<int16_t>(), json["y0"].as<int16_t>(), json["x1"].as<int16_t>(), json["y1"].as<int16_t>(), matrix->Color(json["color"][0].as<int16_t>(), json["color"][1].as<int16_t>(), json["color"][2].as<int16_t>()));
	}
	else if (type.equals("drawCircle"))
	{
		matrix->drawCircle(json["x0"].as<int16_t>(), json["y0"].as<int16_t>(), json["r"].as<int16_t>(), matrix->Color(json["color"][0].as<int16_t>(), json["color"][1].as<int16_t>(), json["color"][2].as<int16_t>()));
	}
	else if (type.equals("drawRect"))
	{
		matrix->drawRect(json["x"].as<int16_t>(), json["y"].as<int16_t>(), json["w"].as<int16_t>(), json["h"].as<int16_t>(), matrix->Color(json["color"][0].as<int16_t>(), json["color"][1].as<int16_t>(), json["color"][2].as<int16_t>()));
	}
	else if (type.equals("fill"))
	{
		matrix->fillScreen(matrix->Color(json["color"][0].as<int16_t>(), json["color"][1].as<int16_t>(), json["color"][2].as<int16_t>()));
	}
	else if (type.equals("drawPixel"))
	{
		matrix->drawPixel(json["x"].as<int16_t>(), json["y"].as<int16_t>(), matrix->Color(json["color"][0].as<int16_t>(), json["color"][1].as<int16_t>(), json["color"][2].as<int16_t>()));
	}
	/*else if (type.equals("brightness"))
	{
		matrix->setBrightness(json["brightness"].as<int16_t>());
	}*/
	else if (type.equals("speedtest")) // TODO not working
	{
		matrix->setFont(&TomThumb);
		matrix->setCursor(0, 7);

		endTime = millis();
		duration = endTime - startTime;
		if (duration > 85 || duration < 75)
		{
			matrix->setTextColor(matrix->Color(255, 0, 0));
		}
		else
		{
			matrix->setTextColor(matrix->Color(0, 255, 0));
		}
		matrix->print(duration);
		startTime = millis();
	}
	else if (type.equals("info"))
	{
		StaticJsonBuffer<200> jsonBuffer;
		JsonObject &root = jsonBuffer.createObject();

		root["version"] = version;
		root["wifirssi"] = String(WiFi.RSSI());
		root["wifiquality"] = GetRSSIasQuality(WiFi.RSSI());
		root["wifissid"] = WiFi.SSID();
		root["ip"] = WiFi.localIP().toString();
		root["chipID"] = String(ESP.getChipId());

		String JS;
		root.printTo(JS);

		if (USBConnection)
		{
			Serial.println(String(JS));
		}
		else
		{
			client.publish("smartDisplay/client/out/info", JS.c_str());
		}
	}
	else if (type.equals("lux"))
	{
		if (USBConnection)
		{
			StaticJsonBuffer<200> jsonBuffer;
			JsonObject &root = jsonBuffer.createObject();
			root["LUX"] = photocell.getCurrentLux();

			String JS;
			root.printTo(JS);
			Serial.println(String(JS));
		}
		else
		{
			client.publish("smartDisplay/client/out/lux", String(photocell.getCurrentLux()).c_str());
		}
	}
	else if (type.equals("roomWeather"))
	{
		StaticJsonBuffer<200> jsonBuffer;
		JsonObject &root = jsonBuffer.createObject();

		root["humidity"] = String(dht.readHumidity());
		root["temperature"] = String(dht.readTemperature());

		String JS;
		root.printTo(JS);

		if (USBConnection)
		{
			Serial.println(String(JS));
		}
		else
		{
			client.publish("smartDisplay/client/out/roomWeather", JS.c_str());
		}
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
		if (json.containsKey("mqtt_server"))
		{
			strcpy(mqtt_server, json["mqtt_server"]);
		}

		if (json.containsKey("mqtt_port"))
		{
			mqtt_port = json["mqtt_port"].as<int>();
		}

		if (json.containsKey("mqtt_user"))
		{
			strcpy(mqtt_user, json["mqtt_user"]);
		}

		if (json.containsKey("mqtt_password"))
		{
			strcpy(mqtt_password, json["mqtt_password"]);
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
	else if (type.equals("ping"))
	{
		if (USBConnection)
		{
			Serial.println("ping");
		}
		else
		{
			client.publish("smartDisplay/client/out/ping", "");
		}
	}
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
	String s_payload = String((char *)payload);
	String s_topic = String(topic);
	int last = s_topic.lastIndexOf("/") + 1;
	String channel = s_topic.substring(last);

	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.parseObject(s_payload);

	processing(channel, json);
}

void reconnect()
{
	while (!client.connected())
	{
		log("reconnecting to " + String(mqtt_server) + ":" + String(mqtt_port));

		String clientId = "SmartDisplay-Client-";
		clientId += String(random(0xffff), HEX);

		hardwareAnimatedSearch(1, 28, 0);

		// connect to MQTT broker
		if (client.connect(clientId.c_str(), mqtt_user, mqtt_password))
		{
			log("connected to server");

			client.subscribe("smartDisplay/client/in/#");
			client.publish("smartDisplay/client/out", "connected");
		}
		else
		{
			delay(10000);
		}
	}
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
	if (!client.connected())
	{
		return;
	}

	client.publish("smartDisplay/client/out/button", "pressed");
}

void onButtonPressedForDuration()
{
	if (!client.connected())
	{
		return;
	}

	client.publish("smartDisplay/client/out/button", "pressed long");
}

void onButtonSequence()
{
	if (!client.connected())
	{
		return;
	}

	client.publish("smartDisplay/client/out/button", "pressed for hard reset");

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
	if (millis() - lastBrightnessCheck < 10000)
	{
		return;
	}

	float lux = photocell.getCurrentLux();
	long brightness = 255;

	if (lux < 50)
	{
		brightness = map(lux, 0, 50, 0, 255);
	}

	matrix->setBrightness(brightness);
	lastBrightnessCheck = millis();
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
			size_t size = configFile.size();
			// Allocate a buffer to store contents of the file.
			std::unique_ptr<char[]> buf(new char[size]);
			configFile.readBytes(buf.get(), size);
			DynamicJsonBuffer jsonBuffer;
			JsonObject &json = jsonBuffer.parseObject(buf.get());

			if (json.success())
			{
				log("parsed json");

				loadConfig(json);
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
	FastLED.addLeds<NEOPIXEL, MATRIX_DATA_PIN>(leds, 256).setCorrection(TypicalLEDStrip);

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
	server.on("/update", HTTP_POST, []() {
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
	button.onPressedFor(2000, onButtonPressedForDuration);
	button.onSequence(8, 2000, onButtonSequence);

	matrix->clear();
	matrix->setCursor(6, 6);
	matrix->setTextColor(matrix->Color(0, 255, 0));
	matrix->print("Ready!");
	matrix->show();

	myTime = millis() - 500;
	myCounter = 0;

	if (!USBConnection)
	{
		client.setServer(mqtt_server, mqtt_port);
		client.setCallback(mqttCallback);
	}
}

void loop()
{
	server.handleClient(); // TODO disable server after init setup finished
	ArduinoOTA.handle();

	checkBrightness();

	if (firstStart)
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
				String message = Serial.readStringUntil('}') + "}";

				DynamicJsonBuffer jsonBuffer;
				JsonObject &json = jsonBuffer.parseObject(message);
				String type = json["type"];

				processing(type, json);
			};
		}
		else // wifi
		{
			if (!client.connected())
			{
				reconnect();
			}

			client.loop();
		}

		button.read();
	}
}
