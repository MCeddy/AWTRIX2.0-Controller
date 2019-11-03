///////////////////////// AWTRIX CONFIG /////////////////////////

// Wifi Config
const char *ssid = "YourSSID";
const char *password = "YourPassword";
char *awtrix_server = "192.168.178.39";

//#define USB_CONNECTION
//#define MATRIX_MODEV2

/// LDR Config
#define LDR_RESISTOR 10000 //ohms
#define LDR_PIN A0
#define LDR_PHOTOCELL LightDependentResistor::GL5516

#define MATRIX_DATA_PIN D3

#define BUTTON_PIN D4

#define DHT_TYPE DHT22
#define DHT_PIN D5