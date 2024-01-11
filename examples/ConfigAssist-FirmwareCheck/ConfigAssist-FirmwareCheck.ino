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

#define APP_NAME "FirmwareCheck"      // Define application name
#define INI_FILE "/FirmwareCheck.ini" // Define SPIFFS storage file

// Config class
ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);

// Store readonly firmware version variable
//bool b1 = conf.put("firmware_ver", "1.0.0", true);
// Setup internal led variable
//bool b2 = (conf["led_buildin"] == "") ? conf.put("led_buildin", LED_BUILTIN, true) : false;

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
// Setup function
void setup(void) {
  
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  
  LOG_I("Starting.. ver: %s\n", conf["firmware_ver"].c_str());

  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary
 
  // Register handlers for web server    
  server.on("/", handleRoot);  
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });
  server.onNotFound(handleNotFound);  // Append not found handler

  // Define a ConfigAssist helper
  ConfigAssistHelper confHelper(conf);

  // Connect to any available network  
  bool bConn = confHelper.connectToNetwork(15000, "led_buildin");
 
  // Append config assist handlers to web server, setup ap on no connection
  conf.setup(server, !bConn); 
  if(!bConn) LOG_E("Connect failed.\n");
 
    
  if (MDNS.begin(conf["host_name"].c_str())) {
    LOG_I("MDNS responder started\n");
  }

  // Start web server
  server.begin();
  LOG_I("HTTP server started\n");
  //conf.dump();
}

// App main loop 
void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif  

  // Allow the cpu to switch to other tasks  
  delay(2);
}
