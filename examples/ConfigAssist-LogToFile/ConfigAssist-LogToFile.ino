#define LOGGER_LOG_MODE  2          // Log to file
#define LOGGER_LOG_LEVEL 5          // Errors & Warnings & Info & Debug & Verbose
#define LOGGER_LOG_FILENAME "/log1"

#include <ConfigAssist.h>           // Config assist class

// Print the log generated to serial port
void serialPrintLog(){
  Serial.printf("Display log: %s\n", LOGGER_LOG_FILENAME);
  File f = STORAGE.open(LOGGER_LOG_FILENAME, "r"); 
  // read from the file until there's nothing else in it:
  while (f.available()) 
  {
      Serial.write(f.read()); 
  }                               
  // close the file:              
  f.close();
  Serial.print("\nDisplay log..Done\n");
}

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  Serial.print("Starting..\n");

  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 storage init failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 storage init failed!"); 
  #endif
  
  //STORAGE.remove(LOGGER_LOG_FILENAME);  //Uncomment to reset the log file
  
  //Display the log file
  serialPrintLog();
  
  LOG_I("* * * * Starting  * * * * * \n");

  LOG_E("This is an ERROR message \n");
  LOG_W("This is a WARNING message \n");
  LOG_I("This is an INFO message \n");
  LOG_D("This is a DEBUG message \n");
  LOG_V("This is a VERBOSE message \n");

  // Create a config class with an ini filename and json disabled
  ConfigAssist info("/info.ini", NULL);
  
  //info.deleteConfig(); //Uncomment to remove ini file and re-built
    
  if(!info.valid()){ //Add boot counter 
    info.put("bootCnt", 0, true);    
  }else{ //Ini is valid, increase counter and display the value
    info.put("bootCnt", info["bootCnt"].toInt() + 1, true);
    LOG_I("Info file: bootCnt:  %lu\n", info["bootCnt"].toInt());
  }
  //Save keys & values into ini file
  info.saveConfigFile();
  LOG_D("End of setup()..\n");  
  LOGGER_CLOSE_LOG()
}

void loop() {
  // put your main code here, to run repeatedly:

}



