#include <ConfigAssist.h>  // Config assist class
#include <ConfigAssistHelper.h>  // Config assist helper class
#if defined(ESP32)
  WebServer server(80);
#else
  ESP8266WebServer  server(80);
#endif
// Create a config class with an ini filename for storage and disabled json 
ConfigAssist conf("/minimal.ini");

void setup() {
  // put your setup code here, to run once:
    
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 storage init failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 storage init failed!"); 
  #endif
  LOG_I("Starting..\n");
  
  //conf.deleteConfig(); // Uncomment to remove ini file and re-built

  // Define a ConfigAssist helper
  ConfigAssistHelper confHelper(conf);
  
   // Connect to any available network  
  bool bConn = confHelper.connectToNetwork(15000);

  // Check connection
  conf.setup(server, !bConn); 
  if(!bConn) LOG_E("Connect failed.\n");

  server.on("/", []() {              // Append dump handler
    conf.init();
    conf.dump(&server);
  });

  // Start web server
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient(); 
}
