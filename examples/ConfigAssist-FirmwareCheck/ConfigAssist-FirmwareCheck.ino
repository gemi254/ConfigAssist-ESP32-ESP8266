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

#define LOGGER_LOG_LEVEL 5
#include <ConfigAssist.h>  // Config assist class

#if defined(ESP32)
  #include "firmCheckESP32PMem.h"
  WebServer server(80);
#else
  #include "firmCheckESP8266PMem.h"
  ESP8266WebServer  server(80);
#endif

// Define application name
#define APP_NAME "FirmwareCheck"
#define INI_FILE "/FirmwareCheck.ini"

char FIRMWARE_VERSION[] = "1.0.0";    // Firmware version to check

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
 
  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary
  
  // Store readonly firmware version
  conf.put("firmware_ver", FIRMWARE_VERSION, true);
 
  // Register handlers for web server    
  server.on("/", handleRoot);  
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });
  server.onNotFound(handleNotFound);  // Append not found handler

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
  // Start web server
  server.begin();
  LOG_V("HTTP server started\n");
  conf.dump();
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
