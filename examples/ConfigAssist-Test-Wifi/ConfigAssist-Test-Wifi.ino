#include <Arduino.h>
#include <Arduino.h>
#include <LittleFS.h>
#if defined(ESP32)
  #include <WebServer.h>
  #include "SPIFFS.h"
  #include <ESPmDNS.h>
  #include <SD_MMC.h>
#else
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include "TZ.h"
#endif
#include <FS.h>

#include <configAssist.h>

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

#define CONNECT_TIMEOUT 8000
#define MAX_SSID_ARR_NO 2

#define APP_NAME "ConfigAssistTestWifi"

const char* appConfigDict_json PROGMEM = R"~(
[{
   "seperator": "Wifi settings"
  },{
    "name": "st_ssid1",
     "label": "Name for WLAN1",
   "default": ""
  },{
      "name": "st_pass1",
     "label": "Password for WLAN1",
   "default": ""
  },{
    "name": "st_ssid2",
     "label": "Name for WLAN2",
   "default": ""
  },{
      "name": "st_pass2",
     "label": "Password for WLAN2",
   "default": ""
  },{
      "name": "host_name",
     "label": "Host name to use for MDNS and AP<br>{mac} will be replaced with device's mac id",
   "default": "configAssist_{mac}"
  },{
 "seperator": "Application settings"
  },{
      "name": "app_name",
     "label": "Name your application",
   "default": "ConfigAssist"
  },{
      "name": "led_pin",
     "label": "Enter the pin that the led is connected",
   "default": "4",
   "attribs": "min=\"2\" max=\"16\" step=\"1\" "
  },{
 "seperator": "Other settings"
  },{
      "name": "float_val",
     "label": "Enter a float val",
   "default": "3.14159",
   "attribs": "min=\"2.0\" max=\"5\" step=\".001\" "
   },{
      "name": "debug",
     "label": "Check to enable debug",
   "checked": "False"
   },{
      "name": "sensor_type",
     "label": "Enter the sensor type",
   "options": "'BMP280', 'DHT12', 'DHT21', 'DHT22'",
   "default": "DHT22"
   },{
      "name": "refresh_rate",
     "label": "Enter the sensor update refresh rate",
     "range": "10, 50, 1",
   "default": "30"
   },{
      "name": "time_zone",
     "label": "Needs to be a valid time zone string",
   "default": "EET-2EEST,M3.5.0/3,M10.5.0/4",    
  "datalist": "
'Etc/GMT,GMT0'
'Etc/GMT-0,GMT0'
'Etc/GMT-1,<+01>-1'
'Etc/GMT-2,<+02>-2'
'Etc/GMT-3,<+03>-3'
'Etc/GMT-4,<+04>-4'
'Etc/GMT-5,<+05>-5'
'Etc/GMT-6,<+06>-6'
'Etc/GMT-7,<+07>-7'
'Etc/GMT-8,<+08>-8'
'Etc/GMT-9,<+09>-9'
'Etc/GMT-10,<+10>-10'
'Etc/GMT-11,<+11>-11'
'Etc/GMT-12,<+12>-12'
'Etc/GMT-13,<+13>-13'
'Etc/GMT-14,<+14>-14'
'Etc/GMT0,GMT0'
'Etc/GMT+0,GMT0'
'Etc/GMT+1,<-01>1'
'Etc/GMT+2,<-02>2'
'Etc/GMT+3,<-03>3'
'Etc/GMT+4,<-04>4'
'Etc/GMT+5,<-05>5'
'Etc/GMT+6,<-06>6'
'Etc/GMT+7,<-07>7'
'Etc/GMT+8,<-08>8'
'Etc/GMT+9,<-09>9'
'Etc/GMT+10,<-10>10'
'Etc/GMT+11,<-11>11'
'Etc/GMT+12,<-12>12'"  
},{
   "name": "cal_data",
  "label": "Enter data for 2 Point calibration.</br>Data will be saved to /calibration.ini",
   "file": "/calibration.ini",
"default": "X1=222, Y1=1.22
X2=900, Y2=3.24"}
])~";

ConfigAssist conf;
String hostName;
unsigned long pingMillis = millis();

void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_DBG("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_DBG("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif
}

void handleRoot() {
  digitalWrite(conf["led_pin"].toInt(), 0);

  String out("<h2>Hello from {name}</h2>");
  out += "<h4>Device time: " + conf.getLocalTime() +"</h4>";
  out += "<a href='/cfg'>Edit config</a>";

  #if defined(ESP32)
    out.replace("{name}", "ESP32");
  #else
    out.replace("{name}", "ESP8266!");
  #endif
  out += "<script>" + conf.getTimeSyncScript() + "</script>";
  server.send(200, "text/html", out);
  digitalWrite(conf["led_pin"].toInt(), 1);
}

void handleNotFound() {
  digitalWrite(conf["led_pin"].toInt(), 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(conf["led_pin"].toInt(), 0);
}


bool connectToNetwork(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(conf["host_name"].c_str());
  String st_ssid ="";
  String st_pass="";
  for (int i = 1; i < MAX_SSID_ARR_NO + 1; i++){
    st_ssid = conf["st_ssid" + String(i)];
    if(st_ssid=="") continue;
    st_pass = conf["st_pass" + String(i)];


    LOG_INF("Wifi ST connecting to: %s, %s \n",st_ssid.c_str(), st_pass.c_str());

    WiFi.begin(st_ssid.c_str(), st_pass.c_str());
    int col = 0;
    uint32_t startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < CONNECT_TIMEOUT) {
      Serial.printf(".");
      if (++col >= 60){
        col = 0;
        Serial.printf("\n");
      }
      Serial.flush();
      delay(500);
    }
    Serial.printf("\n");
    if(WiFi.status() == WL_CONNECTED) break;
  }

  if (WiFi.status() != WL_CONNECTED){
    LOG_ERR("Wifi connect fail\n");
    WiFi.disconnect();
    return false;
  }else{
    LOG_INF("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", st_ssid.c_str(), WiFi.localIP().toString().c_str());
  }
  return true;
}

void setup(void) {

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_INF("Starting..\n");
  debugMemory("setup");

  //Uncomment to remove ini file for other examples
  //and re-built it fron dictionary
  //conf.deleteConfig();

  #if defined(ESP32)
    if(!STORAGE.begin(true)) Serial.println("ESP32 Storage failed!");
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 Storage failed!");
  #endif

  conf.initJsonDict(appConfigDict_json);
  
  //Connect to any available network
  bool bConn = connectToNetwork();
  digitalWrite(conf["led_pin"].toInt(), 1);

  if(!bConn){
    LOG_ERR("Connect failed.");
    conf.setup(server, true);
    return;
  }

  if (MDNS.begin(conf["host_name"].c_str())) {
    LOG_INF("MDNS responder started\n");
  }

  server.on("/", handleRoot);
  conf.setup(server);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
    conf.dump();
  });

  server.onNotFound(handleNotFound);
  server.begin();
  LOG_INF("HTTP server started\n");

}

void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif

  if (millis() - pingMillis >= 10000){
    if(conf["debug"].toInt()) debugMemory("Loop");
    pingMillis = millis();
  }

  delay(2);
}