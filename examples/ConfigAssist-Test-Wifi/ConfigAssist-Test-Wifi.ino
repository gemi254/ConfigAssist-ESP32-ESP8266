#include <ConfigAssist.h>        // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class

#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

#define INI_FILE "/TestWifi.ini" // Define SPIFFS storage file

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22
#endif
const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi connection settings:
  - st_ssid1:
      label: Name for WLAN
  - st_pass1:
      label: Password for WLAN
  - st_ip1:
      label: Static ip setup (ip mask gateway) (192.168.1.100 255.255.255.0 192.168.1.1)
  - st_ssid2:
      label: Name for WLAN
  - st_pass2:
      label: Password for WLAN
  - st_ip2:
      label: Static ip setup (ip mask gateway) (192.168.1.100 255.255.255.0 192.168.1.1)

Wifi Access Point settings:
  - ap_ssid:
      label: Access point ssid. (Leave blank for defaule)
  - ap_pass:
      label: Password for Access point, leave blank for none.
  - ap_ip:
      label: Static ip setup (ip mask gateway) (192.168.4.1 255.255.255.0 192.168.4.1)

Application settings:
  - host_name:
      label: >-
        Host name to use for MDNS and AP<br>{mac} will be replaced with device's mac id
      default: configAssist_{mac}
  - app_name:
      label: Name your application
      default: TestWifi
  - led_buildin:
      label: Enter the pin that the build in led is connected.
        Leave blank for auto.
      attribs: "min='2' max='23' step='1'"
  - debug:
      label: Debug application
      checked: false
)~";

ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);
// Define a ConfigAssist helper class to manage wifi connections
ConfigAssistHelper confHelper(conf);

String hostName;
unsigned long pingMillis = millis();

// Print memory info
void debugMemory(const char* caller) {
  #if defined(ESP32)
    LOG_D("%s > Free: heap %u, block: %u, pSRAM %u\n", caller, ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  #else
    LOG_D("%s > Free: heap %u\n", caller, ESP.getFreeHeap());
    LOG_D("%s > WiFi: %i, state: %i\n", caller, WiFi.isConnected(), (int)confHelper.getLedState() );
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
  #if (CA_USE_TESTWIFI)
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

void onWiFiResult(ConfigAssistHelper::WiFiResult res, const String& message) {
    if (res == ConfigAssistHelper::WiFiResult::SUCCESS) {
        LOG_D("Connected to Wi-Fi! IP: %s\n", message.c_str());
        confHelper.startMDNS();
        conf.setupConfigPortalHandlers(server);
        server.begin();
        LOG_D("HTTP server started\n");
        LOG_I("Device started. Visit http://%s\n", WiFi.localIP().toString().c_str());

    }else if (res == ConfigAssistHelper::WiFiResult::DISCONNECTION_ERROR) {
        LOG_E("Disconnect error: %s\n", message.c_str());
        confHelper.setReconnect(true);

    }else if (res == ConfigAssistHelper::WiFiResult::CONNECTION_TIMEOUT) {
        LOG_E("Connect timeout: %s\n", message.c_str());
        conf.setupConfigPortal(server, true);

    }else if (res == ConfigAssistHelper::WiFiResult::INVALID_CREDENTIALS) {
        LOG_W("Invalid credentials : %s\n", message.c_str());
        conf.setupConfigPortal(server, true);
    }
}

//Setup function
void setup(void) {
  // Setup internal led variable if not set
  if (conf("led_buildin") == "")  conf["led_buildin"] = LED_BUILTIN;
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");
  debugMemory("setup");

   //conf.deleteConfig();  //Uncomment to remove ini file and re-built it fron yaml

  // Setup web server
  server.on("/", handleRoot);
  server.on("/d", []() {    // Append dump handler
    conf.dump(&server);
  });

  server.onNotFound(handleNotFound);

  // Start connecting to any available network,
  // On wifi event onWiFiResult will be called
  // Starting mdns, webserver services will be
  // called when the connection has been made
  confHelper.connectToNetworkAsync(15000, LED_BUILTIN, onWiFiResult);

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

  // Run connection checker
  confHelper.loop();

  delay(2);
}
