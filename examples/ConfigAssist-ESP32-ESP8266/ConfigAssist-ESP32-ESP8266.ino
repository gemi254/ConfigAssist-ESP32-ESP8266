#include <ConfigAssist.h>  // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22
#endif

#define APP_NAME "ConfigAssistDemo"       // Define application name
#define INI_FILE "/ConfigAssistDemo.ini"  // Define SPIFFS storage file

unsigned long pingMillis = millis();             // Ping 

// Default application config dictionary
// Modify the file with the params for you application
// Then you can use then then by val = config[name];
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi settings:
  - st_ssid:
      label: Name for WLAN
      default:
  - st_pass:
      label: Password for WLAN
      default:
  - host_name: 
      label: >-
        Host name to use for MDNS and AP
        {mac} will be replaced with device's mac id
      default: configAssist_{mac}

Application settings:
  - app_name:
      label: Name your application
      default: ConfigAssistDemo  
  - led_buildin:
      label: Enter the pin that the build in led is connected. Leave blank for auto.
      attribs: "min='4' max='23' step='1'"
      default: 4
      
ConfigAssist settings:
  - display_style:
      label: Choose how the config sections are displayed. 
        Must reboot to apply
      options: 
        - AllOpen: 0
        - AllClosed: 1 
        - Accordion : 2
        - AccordionToggleClosed : 3
      default: AccordionToggleClosed
  - work_mode:
      label: Application Work mode. Must reboot to apply
      options: "'MeasureUpload': '0' ,'MeasureBatchUpload':'1', 'MeasureUploadSleep':'2', 'MeasureBatchUploadSleep':'3', 'MeasureTimeoutUploadSleep':'4'"
      default: 2

Other settings:
  - float_val:
      label: Enter a float val
      default: 3.14159
      attribs: min="2.0" max="5" step=".001" 
  - debug:
      label: Check to enable debug
      checked: False
  - sensor_type:
      label: Enter the sensor type
      options: 'BMP280', 'DHT12', 'DHT21', 'DHT22'
      default: DHT22  
  - refresh_rate:
      label: Enter the sensor update refresh rate
      range: 10, 50, 1
      default: 30
  - time_zone:
      label: Needs to be a valid time zone string
      default: EET-2EEST,M3.5.0/3,M10.5.0/4 
      datalist: 
        - Etc/GMT,GMT0
        - Etc/GMT-0,GMT0
        - Etc/GMT-1,<+01>-1
        - Etc/GMT-2,<+02>-2
        - Etc/GMT-3,<+03>-3
        - Etc/GMT-4,<+04>-4
        - Etc/GMT-5,<+05>-5
        - Etc/GMT-6,<+06>-6
        - Etc/GMT-7,<+07>-7
        - Etc/GMT-8,<+08>-8
        - Etc/GMT-9,<+09>-9
        - Etc/GMT-10,<+10>-10
        - Etc/GMT-11,<+11>-11
        - Etc/GMT-12,<+12>-12
        - Etc/GMT-13,<+13>-13
        - Etc/GMT-14,<+14>-14
        - Etc/GMT0,GMT0
        - Etc/GMT+0,GMT0
        - Etc/GMT+1,<-01>1
        - Etc/GMT+2,<-02>2
        - Etc/GMT+3,<-03>3
        - Etc/GMT+4,<-04>4
        - Etc/GMT+5,<-05>5
        - Etc/GMT+6,<-06>6
        - Etc/GMT+7,<-07>7
        - Etc/GMT+8,<-08>8
        - Etc/GMT+9,<-09>9
        - Etc/GMT+10,<-10>10
        - Etc/GMT+11,<-11>11
        - Etc/GMT+12,<-12>12
  - cal_data:
      label: Enter data for 2 Point calibration.
        Data will be saved to /calibration.ini
      file: "/calibration.ini"
      default:
        X1=222, Y1=1.22
        X2=900, Y2=3.24
)~"; 

ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML); // Config assist class

// Setup led
bool b = (conf["led_buildin"] == "") ? conf.put("led_buildin", LED_BUILTIN) : false;

// Print memory info
void debugMemory(const char* caller) {      
  #if defined(ESP32)
    LOG_I("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_I("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif   
}

// List the storage file system
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

// CallBack function when conf remotely changes
void onDataChanged(String key){
  LOG_I("Data changed: %s = %s \n", key.c_str(), conf[key].c_str());
  if(key == "display_style")
    conf.setDisplayType((ConfigAssistDisplayType)conf["display_style"].toInt());

}
// Setup function
void setup(void) {
  
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
    
  LOG_I("Starting..\n");
  debugMemory("setup");
 
  ListDir("/");
  
  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary

  // Register handlers for web server    
  server.on("/", handleRoot);         // Add root handler
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });

  server.onNotFound(handleNotFound);  // Append not found handler
  
  debugMemory("Loaded config");
  pinMode(conf["led_buildin"].toInt(), OUTPUT);

  // Will be called when portal is updating a key
  conf.setRemotUpdateCallback(onDataChanged);

  // Define a ConfigAssist helper
  ConfigAssistHelper confHelper(conf);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  
  // Connect to any available network  
  bool bConn = confHelper.connectToNetwork(15000, "led_buildin");
  
  // Append config assist handlers to web server, setup ap on no connection
  conf.setup(server, !bConn); 
  if(!bConn) LOG_E("Connect failed.\n");

  if (MDNS.begin(conf[CA_HOSTNAME_KEY].c_str())) {
    LOG_I("MDNS responder started\n");
  }

  // Get int/bool value
  bool debug = conf["debug"].toInt();
  LOG_I("Boolean value: %i\n", debug);  
  
  // Get float value
  float float_value = atof(conf["float_val"].c_str());
  LOG_I("Float value: %1.5f\n", float_value);
 
  // Set the defined display type
  conf.setDisplayType((ConfigAssistDisplayType)conf["display_style"].toInt());
  
  server.begin();
  LOG_I("HTTP server started, display type: %s\n", conf["display_style"].c_str());
  
  // On the fly generate an ini info file on SPIFFS
  {
    if(debug) STORAGE.remove("/info.ini");
    ConfigAssist info("/info.ini", NULL);
    // Add a key even if not exists. It will be not editable
    if(!info.valid()){
      LOG_D("Info file not exists\n"); 
      info.put("bootCnt", 1, true);
      info.put("lastRSSI", WiFi.RSSI(), true);
    }else{
      LOG_D("Info file: bootCnt:  %s, lastRSSI: %s\n", info["bootCnt"].c_str(), info["lastRSSI"].c_str() );      
      info.put("bootCnt", info["bootCnt"].toInt() + 1, true);
      info.put("lastRSSI", WiFi.RSSI(), true);
    }
    info.saveConfigFile();
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
