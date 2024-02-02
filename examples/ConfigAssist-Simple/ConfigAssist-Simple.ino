#include <ConfigAssist.h>  // Config assist class

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
  
  // Create a config class with an ini filename for storage
  // and disabled json (NULL)
  ConfigAssist info("/info.ini", NULL);
  
  //info.deleteConfig(); // Uncomment to remove ini file and re-built
  
  // Ini file not exists
  if(!info.valid()) {
    // Add a key even if not exists. 
    // It will be not editable, but it will be saved to an ini file
    info.put("var1", "test1", true);
    info.put("var2", 1234, true);
    // Save keys & values into ini file
    info.saveConfigFile();
  }else{ // Ini file is valid, display the values
    LOG_I("Info file: var1: '%s', var2: '%s'\n", info["var1"].c_str(), info["var2"].c_str() );      
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
