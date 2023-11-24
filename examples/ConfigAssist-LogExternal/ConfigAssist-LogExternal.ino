#define LOGGER_LOG_MODE  3                  // Set default logging mode using external function
#define LOGGER_LOG_FILENAME "/logE"
#define LOGGER_LOG_LEVEL 5                  // Errors & Warnings & Info & Debug & Verbose
void _log_printf(const char *format, ...);  // Provide custom print log function

#include <ConfigAssist.h>                   // Config assist class

bool logToFile = true;
static File logFile;

#define MAX_LOG_FMT 128
static char fmtBuf[MAX_LOG_FMT];
static char outBuf[512];
static va_list arglist;

// Custom log print function 
void _log_printf(const char *format, ...){
  strncpy(fmtBuf, format, MAX_LOG_FMT);
  fmtBuf[MAX_LOG_FMT - 1] = 0;
  va_start(arglist, format);  
  vsnprintf(outBuf, MAX_LOG_FMT, fmtBuf, arglist);
  va_end(arglist);
  Serial.print(outBuf);
  if (logToFile){
    if(!logFile) logFile = STORAGE.open(LOGGER_LOG_FILENAME, "a+");
    if(!logFile){
      Serial.printf("Failed to open log file: %s\n", LOGGER_LOG_FILENAME);
      logToFile = false;
      return;
    }
    logFile.print(outBuf);
    logFile.flush();
  }
}

// Print the log generated to serial port
void serialPrintLog(){
  Serial.printf("Display log: %s\n", LOGGER_LOG_FILENAME);  
  File f = STORAGE.open(LOGGER_LOG_FILENAME, "r");
  // Read from the file until there's nothing else in it:
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
  
  // STORAGE.remove(LOGGER_LOG_FILENAME); //Uncomment to reset the log file

  // Display the log file
  serialPrintLog();
  
  LOG_I("* * * * Starting  * * * * * \n");

  LOG_E("F:This is an ERROR message \n");
  LOG_W("F:This is a WARNING message \n");
  LOG_I("F:This is an INFO message \n");
  LOG_D("F:This is a DEBUG message \n");
  LOG_V("F:This is a VERBOSE message \n");

  // Create a config class with an ini filename and json disabled
  ConfigAssist info("/info.ini", NULL);
  
  //info.deleteConfig(); //Uncomment to remove ini file and re-built
    
  if(!info.valid()){ //Add boot counter 
    info.put("bootCnt", 0, true);    
  }else{ // Ini is valid, increase counter and display the value
    info.put("bootCnt", info["bootCnt"].toInt() + 1, true);
    LOG_I("Info file: bootCnt:  %lu\n", info["bootCnt"].toInt());
  }
  // ave keys & values into ini file
  info.saveConfigFile();
  
  // On the fly Stop loggin to file 
  logToFile = false;
  logFile.close();

  LOG_E("S:This is an ERROR message \n");
  LOG_W("S:This is a WARNING message \n");
  LOG_I("S:This is an INFO message \n");
  LOG_D("S:This is a DEBUG message \n");
  LOG_V("S:This is a VERBOSE message \n");

  LOG_D("End of setup()..\n");  
  
}

void loop() {
  // put your main code here, to run repeatedly:

}

