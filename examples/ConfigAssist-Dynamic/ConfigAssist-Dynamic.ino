#include <ConfigAssist.h>  // Config assist class definition

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
  LOG_I("Config exists: %i\n",config.confExists());
  //Start storage and builds config
  //config.deleteConfig(); // Uncomment to remove ini file and re-built

  // Dict is disabled, Ini file not found
  if(!config.confExists()){
    String textDict="YAML DICT: \n";
    for(int i=0; i<10; ++i){
      textDict += "  - var_"+ String(i) +":\n";
      textDict += "      default: "+ String(i) +"\n";
    }
    LOG_I("Generated yaml:\n %s\n",textDict.c_str());

    // Build ini file from textDict
    config.setDictStr(textDict.c_str(), true);
    LOG_I("Config valid: %i\n",config.valid());
    // Save keys & values into ini file
    config.saveConfigFile();
    config.dump();
  }else{ // Ini file is valid, display the values
    config.dump();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
