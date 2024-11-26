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

#define INI_FILE "/ConfigAssistWiFiAsync.ini"

unsigned long pingMillis = millis();             // Ping

// Default application config dictionary
// Modify the file with the params for you application
// Then you can use then then by val = config[name];
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi settings:
  - st_ssid:
      label: Name for WLAN
  - st_pass:
      label: Password for WLAN
  - host_name:
      label: Host name to use for MDNS and AP
        {mac} will be replaced with device's mac id
      default: configAssist_{mac}

Application settings:
  - app_name:
      label: Name your application
      default: ConfigAssistDemo
  - led_buildin:
      label: Enter the pin that the build in led is connected.
        Leave blank for auto.
      attribs: "min='2' max='23' step='1'"
  - debug:
      label: Debug application
      checked: true      

ConfigAssist settings:
  - display_style:
      label: Choose how the config sections are displayed.
        Must reboot to apply
      options:
        - AllOpen: 0
        - AllClosed: 1
        - Accordion : 2
        - AccordionToggleClosed : 3
      default: AccordionToggleClosed  -
)~";

ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);  // Config assist class
ConfigAssistHelper confHelper(conf);              // Define a ConfigAssist helper

// Setup led
String s = (conf("led_buildin") == "") ? conf["led_buildin"] =  LED_BUILTIN : "";

// Print memory info
void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_I("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_I("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
  #endif
  LOG_I("%s > Wifi status: %i, ssid: %s, rssi %i\n", caller, (int)confHelper.getLedState(), WiFi.SSID().c_str(), WiFi.RSSI() );
}

// Handler function for Home page
void handleRoot() {
  String out("<h2>Hello from {name}</h2>");
  out += "<h4>Device time: " + conf.getLocalTime() +"</h4>";
  out += "<a href='/cfg'>Edit config</a>";
  conf["refresh_rate"] = conf("refresh_rate").toInt() + 1;
#if defined(ESP32)
    out.replace("{name}", "ESP32");
#else
    out.replace("{name}", "ESP8266!");
#endif
#if (CA_USE_TIMESYNC)
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
  LOG_I("Data changed: %s = %s \n", key.c_str(), conf(key).c_str());
  if(key == "display_style")
    conf.setDisplayType((ConfigAssistDisplayType)conf("display_style").toInt());

}

// Setup function
void setup(void) {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

#if defined(CA_USE_LITTLEFS)
  LOG_I("Starting with LITTLEFS.. \n");
#else
  LOG_I("Starting with SPIFFS.. \n");
#endif

  debugMemory("setup");

  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary

  // Register handlers for web server
  server.on("/", handleRoot);         // Add root handler
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });

  server.onNotFound(handleNotFound);  // Append not found handler

  debugMemory("Loaded config");

  // Will be called when portal is updating a key
  conf.setRemotUpdateCallback(onDataChanged);

  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);

  // Set the defined display type
  conf.setDisplayType((ConfigAssistDisplayType)conf("display_style").toInt());

  // Set Wi-Fi credentials from config and start connection process
  confHelper.connectToNetworkAsync(15000, 13, [](ConfigAssistHelper::WiFiResult result, const String& msg) {
      switch (result) {
          case ConfigAssistHelper::WiFiResult::SUCCESS:
              LOG_D("Connected to Wi-Fi! IP: %s\n", msg.c_str());
              confHelper.startMDNS();
              conf.setupConfigPortalHandlers(server);
              server.begin();
              LOG_D("HTTP server started\n");
              LOG_I("Device started. Visit http://%s\n", WiFi.localIP().toString().c_str());
              break;

          case ConfigAssistHelper::WiFiResult::INVALID_CREDENTIALS:
              LOG_D("Invalid credentials: %s\n", msg.c_str());
              // Append config assist handlers to web server,
              // setup a minimal ap to edit config
              conf.setupConfigPortal(server, true);
              break;

          case ConfigAssistHelper::WiFiResult::CONNECTION_TIMEOUT:
              LOG_D("Connection fail: %s\n", msg.c_str());
              conf.setupConfigPortal(server, true);
              break;

          case ConfigAssistHelper::WiFiResult::DISCONNECTION_ERROR:
              LOG_D("Disconnected: %s\n", msg.c_str());
              // Will reconnect automatically
              //WiFi.setAutoReconnect(true);
              confHelper.setReconnect(true);
              // confHelper.retryConnection(20000);
              break;

          default:
              LOG_D("Unknown result: %s\n", msg.c_str());
              break;
      }
  });
}

// App main loop
void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif

  // Run connection checks
  confHelper.loop();

  // Display info
  if (millis() - pingMillis >= 10000){
    // if debug is enabled in config display memory debug messages
    if(conf("debug").toInt()) debugMemory("Loop");
    pingMillis = millis();
  }

  // Allow the cpu to switch to other tasks
  delay(2);
}
