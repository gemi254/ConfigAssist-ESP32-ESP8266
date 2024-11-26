#include <ConfigAssist.h>  // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class

#if defined(ESP32)
  #include "firmCheckESP32PMem.h"
  WebServer server(80);
#else
  #include "firmCheckESP8266PMem.h"
  ESP8266WebServer  server(80);
#endif

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22
#endif
#define APP_VER "1.1" // Application version
#define INI_FILE "/FirmwareCheck.ini" // Define SPIFFS storage file

// Config class
ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);
  // Define a ConfigAssist helper
ConfigAssistHelper confHelper(conf);


String hostName;                      // Default Host name

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

// Setup function
void setup(void) {

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting.. ver: %s\n", APP_VER);
  LOG_I("WiFi mode: %i\n", (int)WiFi.getMode());

  if(false) conf.deleteConfig(); // Set to true ro remove old ini file and re-built it fron dictionary

  // Register handlers for web server
  server.on("/", handleRoot);
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });
  server.onNotFound(handleNotFound);  // Append not found handler

  // Connect to any available network
  bool bConn = confHelper.connectToNetwork(15000, LED_BUILTIN);

  // Append config assist handlers to web server, setup ap on no connection
  conf.setup(server, !bConn);
  if(!bConn){
    LOG_E("Connect failed.\n");
  }else{
    confHelper.startMDNS();
  }

  // Start web server
  server.begin();
  LOG_I("HTTP server started\n");
}

// App main loop
void loop(void) {
  server.handleClient();
  confHelper.loop();
  // Allow the cpu to switch to other tasks
  delay(2);
}
