#if defined(ESP32)
  #include "SPIFFS.h"
  #include <WebServer.h>
#else
  #include <LittleFS.h>
  #include <ESP8266WebServer.h>  
#endif

#include <configAssist.h>  // Config assist class

void setup() {
  // put your setup code here, to run once:
    
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  LOG_INF("Starting..\n");

  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 Storage failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 Storage failed!"); 
  #endif
  
  // Create a config class with an ini filename for storage 
  ConfigAssist info("/info.ini");
  
  //info.deleteConfig(); //Uncomment to remove ini file and re-built
  
  //Add a key even if ini not exists. 
  //It will be not editable, but it will be saved to an ini file
  if(!info.valid()){
    info.put("var1", "test1", true);
    info.put("var2", 1234, true);
    //Save keys & values into ini file
    info.saveConfigFile();
  }else{ //Ini file is valid, display the values
    LOG_INF("Info file: var1:  %s, var2: %s\n", info["var1"].c_str(), info["var2"].c_str() );      
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
