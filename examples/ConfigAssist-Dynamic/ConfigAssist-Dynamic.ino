#include <ConfigAssist.h>  // Config assist class definition

// Create a config class with an ini filename for storage
//Start storage and load config
ConfigAssist config("/info1.ini");
bool reset = false;
void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");

  if(reset){
    config.deleteConfig(); // Remove ini file and re-built
    config.clear();        // Clear loaded keys
  }
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
