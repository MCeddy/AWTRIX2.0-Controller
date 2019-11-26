///////////////////////// AWTRIX CONFIG /////////////////////////

#define HOST_NAME "SmartDisplay"

#define HOTSPOT_SSID "SmartDisplay"
#define HOTSPOT_PASSWORD "displayyy"

#define CONFIG_FILE "/settings.json" // should start with "/"

// LDR Config
#define LDR_RESISTOR 10000 //ohms
#define LDR_PIN A0
#define LDR_PHOTOCELL LightDependentResistor::GL5516

#define MATRIX_DATA_PIN D3

#define BUTTON_PIN D4

#define DHT_TYPE DHT22
#define DHT_PIN D5

// reset detector
#define DRD_TIMEOUT 5.0
#define DRD_ADDRESS 0x00