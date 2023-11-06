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

#define LOGGER_LOG_LEVEL 5
#include <ConfigAssist.h>

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

#define CONNECT_TIMEOUT 8000
#define MAX_SSID_ARR_NO 2

#define APP_NAME "ConfigAssistTestWifi"
#define INI_FILE "/ConfigAssistTestWifi.ini"

const char* VARIABLES_DEF_JSON PROGMEM = R"~(
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
    "name": "st_ip1",
    "label": "Static ip setup (ip mask gateway) (192.168.1.100 255.255.255.0 192.168.1.1)",
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
    "name": "st_ip2",
    "label": "Static ip setup (ip mask gateway) (192.168.4.2 255.255.255.0 192.168.1.1)",
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

ConfigAssist conf(INI_FILE, VARIABLES_DEF_JSON);
String hostName;
unsigned long pingMillis = millis();


void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_D("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_D("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
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
#ifdef CA_USE_TESTWIFI
  out += "<script>" + conf.getTimeSyncScript() + "</script>";
#endif  
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

// Set static ip if defined
bool setStaticIP(String st_ip){
  if(st_ip.length() <= 0) return false;

  IPAddress ip, mask, gw;

  int ndx = st_ip.indexOf(' ');
  String s = st_ip.substring(0, ndx);
  s.trim();
  if(!ip.fromString(s)){
    LOG_E("Error parsing static ip: %s\n",s.c_str());
    return false;
  }

  st_ip = st_ip.substring(ndx + 1, st_ip.length() );
  ndx = st_ip.indexOf(' ');
  s = st_ip.substring(0, ndx);
  s.trim();
  if(!mask.fromString(s)){
    LOG_E("Error parsing static ip mask: %s\n",s.c_str());
    return false;
  }

  st_ip = st_ip.substring(ndx + 1, st_ip.length() );
  s = st_ip;
  s.trim();
  if(!gw.fromString(s)){
    LOG_E("Error parsing static ip gw: %s\n",s.c_str());
    return false;
  }
  LOG_I("Wifi ST setting static ip: %s, mask: %s  gw: %s \n", ip.toString().c_str(), mask.toString().c_str(), gw.toString().c_str());
  WiFi.config(ip, gw, mask);
  return true;  
}

bool connectToNetwork(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(conf["host_name"].c_str());
  String st_ssid ="";
  String st_pass = "";
  String st_ip = "";
  for (int i = 1; i < MAX_SSID_ARR_NO + 1; i++){
    st_ssid = conf["st_ssid" + String(i)];
    if(st_ssid=="") continue;
    st_pass = conf["st_pass" + String(i)];
    
    //Set static ip if defined
    st_ip = conf["st_ip" + String(i)];
    setStaticIP(st_ip);  
    
    LOG_I("Wifi ST connecting to: %s, %s \n",st_ssid.c_str(), st_pass.c_str());

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
    if (WiFi.status() != WL_CONNECTED){
      LOG_E("Wifi connect fail\n");
      WiFi.disconnect();
    }else{
      LOG_I("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", st_ssid.c_str(), WiFi.localIP().toString().c_str());
      break;
    }
  }  

  if (WiFi.status() == WL_CONNECTED)  return true;
  else return false;
}

void setup(void) {

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");
  debugMemory("setup");

  #if defined(ESP32)
    if(!STORAGE.begin(true)) Serial.println("ESP32 storage init failed!");
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 storage init failed!");
  #endif

  //conf.deleteConfig();  //Uncomment to remove ini file and re-built it fron json
  
  //Connect to any available network
  bool bConn = connectToNetwork();
  digitalWrite(conf["led_pin"].toInt(), 1);

  server.on("/", handleRoot);


  if(!bConn){
    LOG_E("Connect failed.\n");
    conf.setup(server, true);
    return;
  }

  if (MDNS.begin(conf["host_name"].c_str())) {
    LOG_V("MDNS responder started\n");
  }
  //Setup control assist handlers
  conf.setup(server);
  //Append dump handler
  server.on("/d", []() {
    //server.send(200, "text/plain", "ConfigAssist dump");
    conf.dump(server);
  });

  server.onNotFound(handleNotFound);
  server.begin();
  LOG_V("HTTP server started\n");
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