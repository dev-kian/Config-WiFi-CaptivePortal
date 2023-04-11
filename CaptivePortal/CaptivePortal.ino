#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <EEPROM.h>
#include "config.h"
#include "webfiles.h"

ESP8266WebServer server(WEBSERVER_PORT);
DNSServer dns;
bool continueLoop = true, sign = false;//Sign is connect to WiFi is successful or not

void setup() {
  Serial.begin(DEBUG_BAUD);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  EEPROM.begin(EEPROM_SIZE);
  if(!LittleFS.begin()) {
    Serial.println("Failed to mount SPIFFS.\r\nThe microcontroller could not be initialized. Please try changing the Tools>Flash Size.");
    continueLoop = false;
    return;
  }
  delay(10);
  saveWebFilesSPIFFS();
  String ssid = "", pass = "";
  //Get ssid and password from EEPROM
  getCredentials(ssid, pass);
  Serial.println(ssid + " " + pass);
  if(!ssid.isEmpty() && pass.length() >= 8){
    Serial.printf("Trying to connect to %s Wi-Fi\n", ssid);
    bool isConnected = tryConnect(ssid, pass);
    if(isConnected){
      Serial.printf("The connection to the %s Wi-Fi was successfully completed.\r\n", WiFi.SSID());
      sign = true;
      return;//Exit from setup method
    }
    else{
      Serial.printf("Unfortunately, a network with the name of the %s was not found.\r\nIt will redirect to the captive portal.\r\n",ssid);
    }
  }
  else{
    Serial.println("No Wi-Fi credentials in EEPROM.");
  }

  createAccessPoint();
  startDNS();
  configWebServer();

  Serial.println("Captive Portal is Ready!");
}

void loop(){
  if(!continueLoop){
    Serial.println("Failed to mount");
    blink(5,400);//(400 * 2)= 800*5= 4s
    return;
  }
  else if(sign){
    Serial.println("In this condition you can write the code that need network:)");
    blink(1,1000);
  }
  else{
    dns.processNextRequest();
    server.handleClient();
    delay(10);
  }
}

void createAccessPoint(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  if(UNIQUE_PASSWORD){
    String pass = getUniquePassword();
    Serial.printf("Unique Password: %s", pass);
    WiFi.softAP(SSIDNAME, pass, CHANNEL, false ,MAX_CONNECTION);
  }
  else
    WiFi.softAP(SSIDNAME, PASSWORD, CHANNEL, false ,MAX_CONNECTION);
}

void startDNS(){
  dns.start(DNS_PORT, "*", APIP);  
}

void configWebServer(){
  server.on("/", [](){
    //The X-XSS-Protection header is a security feature that helps protect against cross-site scripting (XSS) attacks
    server.sendHeader("X-XSS-Protection","1; mode=block");
    auto file = readStream(index_html_route);
    server.streamFile(file, "text/html");
    file.close();
  });
  server.on("/style.css", [](){
    auto file = readStream(style_css_route);
    server.streamFile(file, "text/css");
    file.close();
  });
    server.on("/script.js", [](){
      auto file = readStream(script_js_route);
      server.streamFile(file, "application/json");
      file.close();
    });

  server.on("/networks.json", HTTP_GET, [](){
    int networkCount = WiFi.scanNetworks(false, true);
    size_t capacity = JSON_OBJECT_SIZE((networkCount + 1) * 12);
    DynamicJsonDocument doc(capacity);
    for(int i=0; i<networkCount; ++i){
      JsonObject obj = doc.createNestedObject();
      obj["ssid"] = WiFi.SSID(i);
      obj["rssi"] = WiFi.RSSI(i);
      obj["bssid"] = WiFi.BSSIDstr(i);
      obj["hidden"] = WiFi.isHidden(i);
      obj["authmode"] = encType(WiFi.encryptionType(i));
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    doc.clear();
    server.send(200, "application/json", jsonStr);  
    blink(1,250);
  });
  
  server.on("/connect", HTTP_GET, [](){
    if(!server.hasArg("p")){ //payload has value in query or not
      server.send(400, "text/plain", "Bad Request");
    }
    
    String payload = server.arg("p");
    char in[payload.length() + 1];
    char out[128];
    payload.toCharArray(in, payload.length()+1);
    //Decode base64
    decode_base64(in, out);
    payload = String(out);
    Serial.println(payload);
    StaticJsonDocument<128> doc;
    deserializeJson(doc, payload);
    String ssidName = doc["s"], password = doc["p"];
    doc.clear();
    Serial.println(ssidName + " " + password);
    //If ssid is empty or password is empty or short return Bad Request.
    if(ssidName.isEmpty() || password.isEmpty() || password.length() < 8){
      server.send(400, "text/plain", "Bad Request");
    }
    else{
      bool isConnected = tryConnect(ssidName, password);
      size_t capacity = 18 + (isConnected ? 300 : 0);
      DynamicJsonDocument doc(capacity);
      doc["success"] = isConnected;
      if(isConnected){
         doc["ssid"] = WiFi.SSID(); 
         doc["ip"] = WiFi.localIP();
         doc["subnet"] = WiFi.subnetMask();
         doc["gateway"] = WiFi.gatewayIP();
         doc["mac"] = WiFi.macAddress();
      }
      String jsonStr;
      serializeJson(doc, jsonStr);
      doc.clear();
      server.send(200, "application/json", jsonStr);

     if(isConnected){
       //Save ssid and password in EEPROM
       saveCredentials(ssidName, password);
       delay(5000);
       //Restart Module
       ESP.restart();
     }
    }
  });

  server.on("/settings/clearEEPROM", HTTP_GET, [](){
      for(int index=0; index <= EEPROM_SIZE; index++){
        EEPROM.write(index, 0x00);
      }
      EEPROM.commit();
      //EEPROM.end();
      server.send(200);
      blink(2,150);
  });
  
  //If the web server cannot find any route, it will be redirected to the index page.
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  
  server.begin();
  //The X-Content-Type-Options header is a security header that is used to prevent MIME type sniffing
  server.sendHeader("X-Content-Type-Options", "nosniff");
}


bool tryConnect(String ssid, String password){
  //Before trying to connect to a network, disconnect the module from other networks.
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int counter = 0;
  while(counter < MAXIMUM_DELAY){
    counter++;
    if(WiFi.status() == WL_CONNECTED){
      WiFi.hostname(HOSTNAME);
      return true;
    }
    blink(4, 125);//(4*125)*2= 1000ms
  }
  return false;
}


void blink(int count, int miliSeconds){
  for(int c=0; c<count; c++){
    digitalWrite(LED_PIN, LOW);
    delay(miliSeconds);
    digitalWrite(LED_PIN, HIGH);
    delay(miliSeconds);
  }
}

void decode_base64(const char* encoded_data, char* decoded_data) {
    const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    char buffer[4], c;
    auto base64_decode = [&](const char c) -> char {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    while (*encoded_data) {
        c = base64_decode(*encoded_data++);
        if (c == -1) break;
        buffer[i++] = c;
        if (i == 4) {
            decoded_data[j++] = (buffer[0] << 2) | (buffer[1] >> 4);
            decoded_data[j++] = (buffer[1] << 4) | (buffer[2] >> 2);
            decoded_data[j++] = (buffer[2] << 6) | buffer[3];
            i = 0;
        }
    }
    if (i > 1) decoded_data[j++] = (buffer[0] << 2) | (buffer[1] >> 4);
    if (i > 2) decoded_data[j++] = (buffer[1] << 4) | (buffer[2] >> 2);
    decoded_data[j] = '\0'; // Add null terminator
}

void saveWebFilesSPIFFS(){
  writeOnSPIFFS(index_html_route, index_html_gz, index_html_gz_len);
  writeOnSPIFFS(style_css_route, style_css_gz, style_css_gz_len);
  writeOnSPIFFS(script_js_route, script_js_gz, script_js_gz_len);
}

void writeOnSPIFFS(String fileName, const uint8_t* data, int len){
  //if(LittleFS.exists(fileName)) return;
  File file = LittleFS.open(fileName, "w");
  file.write(data, 2000);
  file.close();
  blink(1, 25);
}

File readStream(String filename){
  if(!LittleFS.exists(filename)){
    return File();
  }
  File file = LittleFS.open(filename, "r");
  if(!file || file.isDirectory()){
    return File();
  }
  return file;  
}


void saveCredentials(String ssid, String pass) {
  char ssidBuf[SSID_SIZE];
  char passBuf[PASS_SIZE];
  //Convert the SSID and password strings to char arrays
  ssid.toCharArray(ssidBuf, SSID_SIZE);
  pass.toCharArray(passBuf, PASS_SIZE);
  //Write the SSID and password to the EEPROM
  for (int i = 0; i < SSID_SIZE; i++) {
    EEPROM.write(i, ssidBuf[i]);
  }
  for (int i = 0; i < PASS_SIZE; i++) {
    EEPROM.write(i + SSID_SIZE, passBuf[i]);
  }
  EEPROM.commit();
}

void getCredentials(String &ssid, String &pass) {
  char ssidBuf[SSID_SIZE];
  char passBuf[PASS_SIZE];
  //Read the SSID and password from the EEPROM
  for (int i = 0; i < SSID_SIZE; i++) {
    ssidBuf[i] = EEPROM.read(i);
  }
    for (int i = 0; i < PASS_SIZE; i++) {
    passBuf[i] = EEPROM.read(i + SSID_SIZE);
  }
  //Convert the SSID and password char arrays to strings
  ssid = String(ssidBuf);
  pass = String(passBuf);
}

String getUniquePassword(){
  //Generate unique password with chip id module
  int chipId = ESP.getChipId();
  char uuid[9];
  snprintf(uuid, sizeof(uuid), "%08x", chipId);
  return String(uuid);
}

String encType(int i){
    switch (i) {
    case ENC_TYPE_NONE:
      return "Open";
    case ENC_TYPE_WEP:
      return "WEP";
    case ENC_TYPE_TKIP:
      return "WPA/TKIP";
    case ENC_TYPE_CCMP:
      return "WPA2/AES";
    case ENC_TYPE_AUTO:
      return "WPA/WPA2/TKIP+AES";
    default:
      return "Unknown";
  }
}
