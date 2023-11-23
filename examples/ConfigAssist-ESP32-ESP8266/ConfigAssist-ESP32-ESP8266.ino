#include <ConfigAssist.h>  // Config assist class

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif

// Define application name
#define APP_NAME "ConfigAssistDemo"
#define INI_FILE "/ConfigAssistDemo.ini"

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

// Config class
ConfigAssist conf(INI_FILE, VARIABLES_DEF_JSON);
                   
String hostName;                      // Default Host name
unsigned long pingMillis = millis();  // Ping 

// *********** Helper funcions ************
void debugMemory(const char* caller) {      
  #if defined(ESP32)
    LOG_I("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_I("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif   
}
// List storage file system
void ListDir(const char * dirname) {
  LOG_I("Listing directory: %s\n", dirname);
  // List details of files on file system
  File root = STORAGE.open(dirname, "r");
  File file = root.openNextFile();
  while (file) {
    #if defined(ESP32)
      LOG_I("File: %s, size: %u B\n", file.path(), file.size());
    #else
      LOG_I("File: %s, size: %u B\n", file.fullName(), file.size());
    #endif
    file = root.openNextFile();
  }
  Serial.println("");
}
// Handler function for Home page
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
  #ifdef CA_USE_TIMESYNC 
  out += "<script>" + conf.getTimeSyncScript() + "</script>";
  #endif
  server.send(200, "text/html", out);
  digitalWrite(conf["led_pin"].toInt(), 1);  
}

// Handler for page not found
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
  
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  
  // Start local storage
  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 storage init failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 storage init failed!"); 
  #endif
  ListDir("/");
    
  LOG_I("Starting..\n");
  debugMemory("setup");
 
  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary

  // Register handlers for web server    
  server.on("/", handleRoot);         // Add root handler
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

  debugMemory("Loaded config");
  pinMode(conf["led_pin"].toInt(), OUTPUT);
     
  // Connect to Wifi station with ssid from conf file
  uint32_t startAttemptTime = millis();
  WiFi.setAutoReconnect(false);
  WiFi.setAutoConnect(false);
  WiFi.mode(WIFI_STA);
  LOG_I("Wifi Station starting, connecting to: %s\n", conf["st_ssid"].c_str());
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
    LOG_I("MDNS responder started\n");
  }

  // Get int/bool value
  bool debug = conf["debug"].toInt();
  LOG_I("Boolean value: %i\n", debug);  
  
  // Get float value
  float float_value = atof(conf["float_val"].c_str());
  LOG_I("Float value: %1.5f\n", float_value);
  
  // Change a value
  //conf.put("led_pin","3");
  
  // Also change an int/bool value
  //conf.put("led_pin", 4);
  
  // Append config assist handlers to web server 
  conf.setup(server);

  server.begin();
  LOG_V("HTTP server started\n");
  
  // On the fly generate an ini info file on SPIFFS
  {
    if(debug) STORAGE.remove("/info.ini");
    ConfigAssist info("/info.ini");
    // Add a key even if not exists. It will be not editable
    if(!info.valid()){
      info.put("var1", "test1", true);
      info.put("var2", 1234, true);
      info.saveConfigFile();
    }else{
      LOG_D("Info file: var1:  %s, var2: %s\n", info["var1"].c_str(), info["var2"].c_str() );      
    }
  }  
}
// App main loop 
void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif  
  
  // Display info
  if (millis() - pingMillis >= 10000){
    // if debug is enabled in config display memory debug messages    
    if(conf["debug"].toInt()) debugMemory("Loop");
    pingMillis = millis();
  } 
  
  // Allow the cpu to switch to other tasks  
  delay(2);
}
