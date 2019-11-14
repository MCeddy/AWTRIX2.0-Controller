///////////////////////// AWTRIX CONFIG /////////////////////////

#define HOST_NAME "AWTRIX-Controller"

#define HOTSPOT_SSID "AWTRIX"
#define HOTSPOT_PASSWORD "awtrixxx"

#define CONFIG_FILE "/awtrix.json" // should start with "/"

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