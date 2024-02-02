#include <ConfigAssist.h>        // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

#define APP_NAME "TestWifi"      // Define application name
#define INI_FILE "/TestWifi.ini" // Define SPIFFS storage file

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22
#endif
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi settings:
  - st_ssid1:
      label: Name for WLAN
      default:
  - st_pass1:
      label: Password for WLAN
      default:
  - st_ip1:
      label: Static ip setup (ip mask gateway) (192.168.1.100 255.255.255.0 192.168.1.1)
      default:
  - st_ssid2:
      label: Name for WLAN
      default:
  - st_pass2:
      label: Password for WLAN
      default:
  - st_ip2:
      label: Static ip setup (ip mask gateway) (192.168.1.100 255.255.255.0 192.168.1.1)
      default:

  - host_name: 
      label: >-
        Host name to use for MDNS and AP<br>{mac} will be replaced with device's mac id
      default: configAssist_{mac}

Application settings:
  - app_name:
      label: Name your application
      default: TestWifi  
  - led_buildin:
      label: Enter the pin that the build in led is connected. Leave blank for auto.
      attribs: "min='4' max='23' step='1'"
      default:
)~";

ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);

String hostName;
unsigned long pingMillis = millis();

// Print memory info
void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_D("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_D("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif
}

// Web server root handler
void handleRoot() {
  String out("<h2>Hello from {name}</h2>");
  out += "<h4>Device time: " + conf.getLocalTime() +"</h4>";
  out += "<a href='/cfg'>Edit config</a>";

  #if defined(ESP32)
    out.replace("{name}", "ESP32");
  #else
    out.replace("{name}", "ESP8266!");
  #endif
  // Send browser time sync script
  #ifdef CA_USE_TESTWIFI
    out += "<script>" + conf.getTimeSyncScript() + "</script>";
  #endif  
  server.send(200, "text/html", out);  
}

// Page not found handler
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

//Setup function
void setup(void) {

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");
  debugMemory("setup");

   //conf.deleteConfig();  //Uncomment to remove ini file and re-built it fron json
    
  // Setup web server 
  server.on("/", handleRoot);  
  server.on("/d", []() {    // Append dump handler
    conf.dump(&server);
  });

  server.onNotFound(handleNotFound);
  // Setup led on empty string
  if(conf["led_buildin"]=="") conf.put("led_buildin", LED_BUILTIN);  

  // Define a ConfigAssist helper class to connect wifi
  ConfigAssistHelper confHelper(conf);
  
  // Connect to any available network  
  bool bConn = confHelper.connectToNetwork(15000, "led_buildin");
  
  // Append config assist handlers to web server, setup ap on no connection
  conf.setup(server, !bConn); 
  if(!bConn) LOG_E("Connect failed.\n");

  if (MDNS.begin(conf[CA_HOSTNAME_KEY].c_str())) {
    LOG_I("MDNS responder started\n");
  }
  
  server.begin();
  LOG_I("HTTP server started\n");
}

//Loop function
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