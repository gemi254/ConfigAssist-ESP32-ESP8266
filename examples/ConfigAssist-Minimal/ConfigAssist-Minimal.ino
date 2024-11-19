#include <ConfigAssist.h>  // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class
#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif
// Create a config class with an ini filename for storage
// Default wifi connect credentials yaml
ConfigAssist conf("/minimal.ini", true);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");

  //conf.deleteConfig(); // Uncomment to remove ini file and re-built

  // Define a ConfigAssist helper
  ConfigAssistHelper confHelper(conf);

   // Connect to any available network
  bool bConn = confHelper.connectToNetwork(15000);
  if(!bConn) LOG_E("Connect failed.\n");

  // Start AP to fix wifi credentials
  conf.setup(server, !bConn);

  server.on("/", []() {              // Append dump handler
    conf.init();
    conf.dump(&server);
  });

  // Start web server
  server.begin();
  conf.dump();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}
