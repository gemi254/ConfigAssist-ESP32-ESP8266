#if defined(ESP32)
  #include "SPIFFS.h"
  #include <WebServer.h>
#else
  #include <LittleFS.h>
  #include <ESP8266WebServer.h>  
#endif

#include <configAssist.h>  // Config assist class

//Set default configAssist logLevel on run time.
#define DEF_LOG_LEVEL '4' //Errors & Warnings & Info & Debug

// Print the log generated to serial port
void serialPrintLog(){
  Serial.print("Display log..\n");  
  File f = STORAGE.open(LOG_FILENAME); 
  // read from the file until there's nothing else in it:
  while (f.available()) 
  {
      Serial.write(f.read()); 
  }                               
  // close the file:              
  f.close();
  Serial.print("\nDisplay log..Done\n");
}

void setup() {
  //Set configAssist default log level  
  setLogPrintLevel(DEF_LOG_LEVEL);
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  Serial.print("Starting..\n");

  #if defined(ESP32)  
    if(!STORAGE.begin(true)) Serial.println("ESP32 storage init failed!"); 
  #else
    if(!STORAGE.begin()) Serial.println("ESP8266 storage init failed!"); 
  #endif
  //Uncomment to reset the log file
  //STORAGE.remove(LOG_FILENAME); 

  //Display the log file
  serialPrintLog();

  //Enable configAssist logging to file  
  setLogPrintToFile(true);
  
  LOG_INF("* * * * Starting  * * * * * \n");
  LOG_ERR("This is an ERROR message \n");
  LOG_WRN("This is a WARNING message \n");
  LOG_INF("This is an INFO message \n");
  LOG_DBG("This is a DEBUG message \n");

  // Create a config class with an ini filename for storage 
  ConfigAssist info("/info.ini");
  
  //info.deleteConfig(); //Uncomment to remove ini file and re-built
    
  if(!info.valid()){ //Add boot counter 
    info.put("bootCnt", 0, true);    
  }else{ //Ini is valid, increase counter and display the value
    info.put("bootCnt", info["bootCnt"].toInt() + 1, true);
    LOG_INF("Info file: bootCnt:  %i\n", info["bootCnt"].toInt());
  }
  //Save keys & values into ini file
  info.saveConfigFile();
  LOG_DBG("End of setup()..\n");
  //Close the log file  
  setLogPrintToFile(false);
}

void loop() {
  // put your main code here, to run repeatedly:

}



