#include <configAssist.h>  // Config assist class definition

// Create a config class with an ini filename for storage 
ConfigAssist config("/info1.ini");

void setup() {
  // put your setup code here, to run once:
    
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");
  
  //config.deleteConfig(); // Uncomment to remove ini file and re-built   
  
  // Dict is disabled, Ini file not found
  if(!config.valid()){
    String textDict="YAML DICT: \n";
    for(int i=0; i<10; ++i){
      textDict += "  - var_"+ String(i) +":\n";
      textDict += "     default: "+ String(i) +"\n";
    }
    LOG_I("Generated yaml: %s\n",textDict.c_str());
   
    // Build ini file from textDict
    config.setDictStr(textDict.c_str(),true);
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
