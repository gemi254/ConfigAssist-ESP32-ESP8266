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
#include <ConfigAssist.h>  // Config assist class

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif

// Define application name
#define APP_NAME "FirmwareCheck"
#define INI_FILE "/FirmwareCheck.ini"
char FIRMWARE_VERSION[] = "1.0.0";
// Default application config dictionary
// Modify the file with the params for you application
// Then you can use then then by val = config[name];
const char* VARIABLES_DEF_JSON PROGMEM = R"~(
[{
   "seperator": "Wifi settings"
  },{
    "name": "st_ssid",
     "label": "Name for WLAN",
   "default": ""
  },{
      "name": "st_pass",
     "label": "Password for WLAN",
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
   "default": "FirmwareCheck"
  },{
      "name": "firmware_url",
     "label": "Firmware upgrade url with version and info",
   "default": "https://raw.githubusercontent.com/gemi254/ConfigAssist-ESP32-ESP8266/main/examples/ConfigAssist-FirmwareCheck/firmware/lastest.json"
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
}
])~"; 

// Config class
ConfigAssist conf(INI_FILE, VARIABLES_DEF_JSON);
                   
String hostName;                      // Default Host name
unsigned long pingMillis = millis();  // Ping 

// *********** Helper funcions ************

// Handler function for Home page
void handleRoot() {
  
  String out("<h2>Hello from {name}</h2>");
  out += "<h4>Device time: " + conf.getLocalTime() +"</h4>";
  out += "<a href='/cfg'>Edit config</a>";   

  #if defined(ESP32)
    out.replace("{name}", "ESP32");
  #else 
    out.replace("{name}", "ESP8266!");
  #endif
  #ifdef CA_USE_TIMESYNC 
  out += "<script>" + conf.getTimeSyncScript() + "</script>";
  #endif
  server.send(200, "text/html", out);  
}

// Handler for page not found
void handleNotFound() {  
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
}

// *********** Main application funcions ************
void setup(void) {
  
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  
  // Start local storage
  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 storage init failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 storage init failed!"); 
  #endif
    
  LOG_I("Starting.. ver: %s\n", FIRMWARE_VERSION);

  // Uncomment to remove old ini file and re-built it fron dictionary
  //conf.deleteConfig();

  // Register handlers for web server    
  server.on("/", handleRoot);

  // Failed to load config or ssid empty
  if(conf["st_ssid"]=="" ){ 
    // Start Access point server and edit config
    // Data will be availble instantly 
    conf.setup(server, true);
    return;
  }
    
  // Connect to Wifi station with ssid from conf file
  uint32_t startAttemptTime = millis();
  WiFi.setAutoReconnect(false);
  WiFi.setAutoConnect(false);
  WiFi.mode(WIFI_STA);
  LOG_D("Wifi Station starting, connecting to: %s\n", conf["st_ssid"].c_str());
  LOG_N("Wifi Station starting, connecting to ssid: %s, pass: %s\n", conf["st_ssid"].c_str(), conf["st_pass"].c_str());
  WiFi.begin(conf["st_ssid"].c_str(), conf["st_pass"].c_str());
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000)  {
    Serial.print(".");
    delay(500);
    Serial.flush();
  }  
  Serial.println();
  
  // Check connection
  if(WiFi.status() == WL_CONNECTED ){
    LOG_I("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", conf["st_ssid"].c_str(), WiFi.localIP().toString().c_str()); 
  }else{
    // Fall back to Access point for editing config
    LOG_E("Connect failed.\n");
    conf.setup(server, true);
    return;
  }
  
  if (MDNS.begin(conf["host_name"].c_str())) {
    LOG_V("MDNS responder started\n");
  }

  // Add handlers to web server 
  conf.setup(server);
  
  // Append dump handler
  server.on("/d", []() {
    conf.dump(server);
  });

  server.onNotFound(handleNotFound);
  server.begin();
  LOG_V("HTTP server started\n");
}
// App main loop 
void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif  
  
  // Display info
  if (millis() - pingMillis >= 10000){

    pingMillis = millis();
  } 
  
  // Allow the cpu to switch to other tasks  
  delay(2);
}
