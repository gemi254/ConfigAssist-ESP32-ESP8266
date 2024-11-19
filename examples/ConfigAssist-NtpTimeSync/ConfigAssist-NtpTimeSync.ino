#include <ConfigAssist.h>  // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class

#include "appPMem.h"

#ifndef LED_BUILTIN
  #define LED_BUILTIN 22
#endif
#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif

#define INI_FILE "/NtpTimeSync.ini" // Define SPIFFS storage file

// Config class
ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);

// Define a ConfigAssist helper class
ConfigAssistHelper confHelper(conf);

// Setup internal led variable if not set
bool b = (conf("led_buildin") == "") ? conf["led_buildin"] =  LED_BUILTIN : false;

time_t tnow;
unsigned long pingMillis = millis();  // Ping

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
  // Send a web browser sync java script to client
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

  LOG_I("Starting.. \n");

  //conf.deleteConfig(); // Uncomment to remove old ini file and re-built it fron dictionary

  //Active seperator open/close, All others closed
  conf.setDisplayType(ConfigAssistDisplayType::AccordionToggleClosed);

  // Register handlers for web server
  server.on("/", handleRoot);
  server.on("/d", []() {              // Append dump handler
    conf.dump(&server);
  });
  server.onNotFound(handleNotFound);  // Append not found handler

  // Connect to any available network
  bool bConn = confHelper.connectToNetwork(15000, conf("led_buildin").toInt());

  // Append config assist handlers to web server, setup ap on no connection
  conf.setup(server, !bConn);
  if(!bConn) LOG_E("Connect failed.\n");

  if (MDNS.begin(conf("host_name").c_str())) {
    LOG_I("MDNS responder started\n");
  }

  // Start web server
  server.begin();
  LOG_I("HTTP server started\n");

  // Setup time synchronization
  // Wait max 10 sec
  LOG_I("Start time sync..\n");
  confHelper.syncTime(20000);
  tnow = time(nullptr);
}

int cntSoft = 0;
int cntHard = 0;

// App main loop
void loop(void) {
  server.handleClient();
  #if not defined(ESP32)
    MDNS.update();
  #endif

  // Display info
  if (millis() - pingMillis >= 5000){
    tnow = time(nullptr);
    LOG_I("Time in sync: %i clock: %s", confHelper.isTimeSync(), ctime(&tnow));
    cntSoft++;
    cntHard++;
    pingMillis = millis();
  }

  if(cntSoft>2){
    // Force time synchronization,
    // Clock will not wait if already time is set
    // and time will be automatically sync in background.
    confHelper.syncTime(20000, false);
    tnow = time(nullptr);
    cntSoft = 0;
    LOG_I("Soft resync time: %i clock: %s", confHelper.isTimeSync(), ctime(&tnow));
  }

  if(cntHard>4){
    // Force time synchronization,
    // Clock will be reseted and wait for max 20 sec to sync
    // If fail clock will be restored
    confHelper.syncTime(20000, true);
    tnow = time(nullptr);
    LOG_I("Hard resynced time: %i clock: %s", confHelper.isTimeSync(), ctime(&tnow));
    cntHard = 0;
  }

  // Allow the cpu to switch to other tasks
  delay(2);
}
