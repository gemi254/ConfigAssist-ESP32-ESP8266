#include <Arduino.h>
#include <sstream>
#include <vector>
#include <ArduinoJson.h>
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
#endif
#include <FS.h>

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif

//Define application name
#define APP_NAME "ConfigAssistDemo"

// Default application config dictionary
// Modify the file with the params for you application
// Then you can use then then by val = config[name];
const char* appConfigDict_json PROGMEM = R"~(
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
     "label": "Host name to use for MDNS and AP",
   "default": "SetupAssist01"
  },{
 "seperator": "Application settings"
  },{
      "name": "app_name",
     "label": "Name your application",
   "default": "ConfigAssist"
  },{
      "name": "led_pin",
     "label": "Enter the pin that the led is connected",
   "default": "33"
  },{
 "seperator": "Other settings"
  },{
      "name": "float_val",
     "label": "Enter a float val",
   "default": "3.14159"
   },{
      "name": "debug",
     "label": "Check to enable debug",
   "checked": "False"
}]
)~";

#include "configAssist.h"  // Setup assistant class
ConfigAssist conf;         // Config class
String hostName;           // Default Host name

// *********** Helper funcions ************
unsigned long pingMillis = millis();  // Ping 
void debugMemory(const char* caller) {      
  #if defined(ESP32)
    Serial.printf("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    Serial.printf("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif
   pingMillis = millis();
}
// List storage file system
void ListDir(const char * dirname) {
  Serial.printf("Listing directory: %s\n", dirname);
  // ist details of files on file system
  #if defined(ESP32)
    File root = STORAGE.open(dirname);
  #else
    File root = STORAGE.open(dirname,"r");
  #endif
  File file = root.openNextFile();
  while (file) {
    #if defined(ESP32)
      Serial.printf("File: %s, size: %u B\n", file.path(), file.size());
    #else
      Serial.printf("File: %s, size: %u B\n", file.fullName(), file.size());
    #endif
    file = root.openNextFile();
  }
  Serial.println("");
}
// Handler function for ST config form
void handleRoot() {
  digitalWrite(conf["led_pin"].toInt(), 0); 
  String out("<h3>hello from {name}<h3><a href='/config'>Edit config</a>"); 
  #if defined(ESP32)
    out.replace("{name}", "ESP32");
  #else 
    out.replace("{name}", "ESP8266!");
  #endif    
  server.send(200, "text/html", out);
  digitalWrite(conf["led_pin"].toInt(), 1);  
}

// Handler function for AP config form
static void handleAssistRoot() { 
  conf.handleFormRequest(&server); 
}
// Handle page not found
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

// *********** Main application funcions ************
void setup(void) {
  
  Serial.begin(230400);
  Serial.print("\n\n\n\n");
  Serial.flush();
  Serial.print("Starting..\n");
  
  //Start local storage
  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 Storage failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 Storage failed!"); 
  #endif
  ListDir("/");
  //Initialize ConfigAssist json dictionary
  //If ini file is valid wil not be used
  conf.init(appConfigDict_json);  

  //Failed to load config or ssid empty
  if(!conf.valid() || conf["st_ssid"]=="" ){ 
    //Start Access point server and edit config
    //Will reboot for saved data to be loaded
    conf.setup(server, handleAssistRoot);
    return;
  }
  
  pinMode(conf["led_pin"].toInt(), OUTPUT);
     
  //Connect to Wifi station with ssid from conf file
  uint32_t startAttemptTime = millis();
  WiFi.mode(WIFI_STA);
  Serial.printf("Wifi Station starting, connecting to: %s\n", conf["st_ssid"].c_str());
  WiFi.begin(conf["st_ssid"].c_str(), conf["st_pass"].c_str());
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000)  {
    digitalWrite(conf["led_pin"].toInt(), 0);
    Serial.print(".");
    delay(50);
    digitalWrite(conf["led_pin"].toInt(), 1);
    delay(500);
    Serial.flush();
  }  
  Serial.println();
  
  //Check connection
  if(WiFi.status() == WL_CONNECTED ){
    Serial.printf("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", conf["st_ssid"].c_str(), WiFi.localIP().toString().c_str()); 
  }else{
    //Fall back to Access point for editing config
    Serial.println("Connect failed.");
    conf.setup(server, handleAssistRoot);
    return;
  }
  
  if (MDNS.begin(conf["host_name"].c_str())) {
    Serial.println("MDNS responder started");
  }

  //Get float value
  float float_value = atof(conf["float_val"].c_str());
  Serial.printf("Float value: %1.5f\n", float_value);
  
  //Change a value
  //conf.put("led_pin","4");

  //Register handlers for web server    
  server.on("/config", handleAssistRoot);
  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif  
  
  //Display info
  if (millis() - pingMillis >= 5000) debugMemory("Loop");
  
  //allow the cpu to switch to other tasks  
  delay(2);
}
