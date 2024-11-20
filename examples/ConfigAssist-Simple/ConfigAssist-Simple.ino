#include <ConfigAssist.h>  // Config assist class
bool reset = false;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();

  LOG_I("Starting..\n");

  // Create a config class with an ini filename for storage
  // and disabled yaml dictionary (NULL)
  // If ini file exists load it
  ConfigAssist info("/info.ini");

  if(reset){
    info.deleteConfig(); // Remove ini file and re-built
    info.clear();        // Clear loaded keys
  }
  // Ini file not exists
  if(!info.confExists()) {
    // Add a key even if not exists.
    // It will be not editable, but it will be saved to an ini file
    LOG_I("Config file not exists\n");
    info["string"] = "This is a string";
    info["numString"] = "12345";
    info["integer"] = 1;
    info["float"] = 1.0;

  }else{ // Ini file is valid, display the values
    LOG_I("Config file already exists\n");
    info["integer"] = info("integer").toInt() + 1;
    info["float"] = info("float").toFloat() + .5;
  }
  // Save keys & values into an ini file
  info.saveConfigFile();
  info.dump();
}

void loop() {
  // put your main code here, to run repeatedly:
}
