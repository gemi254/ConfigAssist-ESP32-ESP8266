//#define LOGGER_LOG_LEVEL 5 // Errors & Warnings & Info & Debug & Verbose
#include <configAssist.h>  // Config assist class definition

// Create a config class with an ini filename for storage 
ConfigAssist config("/info1.ini");

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
  
  //config.deleteConfig(); // Uncomment to remove ini file and re-built   
  
  // Json is disabled, Ini file not found
  if(!config.valid()){
    // Build a json description
    String dynJson="[\n";
    for(int i=0; i<10; ++i){
      dynJson += "{";
      dynJson += "\"name\" : \"var_" + String(i) + "\", ";
      dynJson += "\"default\" : \"val_" + String(i)+ "\"";
      if(i==9) dynJson += "}\n";
      else dynJson += "},\n";
    }
    dynJson+="]";
    LOG_I("Generated Json: %s\n",dynJson.c_str());
    // Build ini file from json
    config.setJsonDict(dynJson.c_str(),true);
    LOG_I("Config valid: %i\n",config.valid());
    config.dump();
    // Save keys & values into ini file
    config.saveConfigFile();
  }else{ // Ini file is valid, display the values
    LOG_I("Config valid: %i\n",config.valid());
    config.dump();
  }  
}

void loop() {
  // put your main code here, to run repeatedly:
}
