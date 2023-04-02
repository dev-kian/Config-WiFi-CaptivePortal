#define DEBUG_BAUD 115200
#define LED_PIN LED_BUILTIN

//==== Config Web Server ====//
#define WEBSERVER_PORT 80

//==== Config DNS Server ====//
#define DNS_PORT 53

//==== Config Access Point ====//
#define SSIDNAME "Captive-Portal"
#define PASSWORD "11111111"
#define UNIQUE_PASSWORD false
#define HOSTNAME "CP-Device"
#define CHANNEL 2
#define MAX_CONNECTION 1
#define APIP IPAddress(172,217,28,1)

#define MAXIMUM_DELAY 5 //maximum delay for try to connect wifi

//==== Config EEPROM ====//
#define EEPROM_SIZE 512
#define SSID_SIZE 32
#define PASS_SIZE 64
