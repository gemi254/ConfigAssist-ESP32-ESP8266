# ConfigAssist
A litewave library allowing easy editing of application variables and configuration using
a json dictionary for **esp32/esp8266** devices.

![](docs/config.png)

## Description
It allows editing application variables by a web form generated by **configAssist**. 
A json description text, with name, default, label must be defined with all the variables that the 
application will use. 
A simple html page  with an edit form will automatically generated allowing editing these variables from the web 
Browser. The variables then can be used with operator **[]** like  ```conf['variable']```.
The configuration data will be stored in the **SPIFFS** as an **ini file** (Text) and will be 
loaded on each reboot.

## How it works
On first run when no data (ini file) is found, ConfigAssist will start an **AccessPoint** and load the
default dictionary with variables to be edited. A web form will be generated in order to 
initialize application variables using the **web page** from the remote host connected to AccesPoint.
Data will be saved on local storage and will be available on next reboot. 
ConfigAssist uses c++ vectors to dynamically store variables and binary search for speeding the process.

## How to use variables
ConfigAssist consists of single file "configAssist.h" that must be included in your application 
The application variables can be used directly by accessing the **class** itself by operator **[]**
i.e.

+ `String ssid = conf["st_ssid"];`
+ `int pinNo = conf["st_ssid"].toInt();`
+ `digitalWrite(conf["led_pin"].toInt(), 0)`;
+ `float float_value = atof(conf["float_value"].c_str());`

## Parameters definition with JSON
In your application sketch file you must define a json dictionary that includes all the information needed 
for the html form to be generated. See example below. Each variable will be displayed on config page with the order 
Defined in the json file.

for example ..
```
const char* appConfigDict_json PROGMEM = R"~(
[
  {
      "name": "st_ssid",
     "label": "Name for WLAN (Ssid to connect)",
   "default": ""
  },
  {
      "name": "st_pass",
     "label": "Password for WLAN",
   "default": ""
  },
  {
      "name": "host_name",
     "label": "Host name to use for MDNS and AP",
   "default": "ConfigAssist"
  }  
]
```

## Project definitions in your main app

+ include the **configAssist**  class
  - `#include "configAssist.h"  // Setup assistant class`

+ Define your static instance
  - `Config conf;        //configAssist class`

+ in your setup function you must init the class
## Access point handler
Define a web server handler function for the **configAssist**. This function will be passed to 
conf.setup in order for configAssist to handle form requests
```
// Handler function for AP config form
static void handleAssistRoot() { 
  conf.handleFormRequest(&server); 
}
```
## Setup function
```
void setup()
  // Must have storage to read from
  STORAGE.begin(true);
  
 //Initialize config with application's json dictionary
 conf.init(appConfigDict_json);  

  //Failed to load config or ssid empty
  if(!conf.valid() || conf["st_ssid"]=="" ){ 
    //Start Access point server and edit config
    //Will reboot for saved data to be loaded
    conf.setup(server, handleAssistRoot);
    return;
  }
  ...
  ```

## Compile
Download the files **configAssist.h** and **configAssistPMem.h** and put it in the same directory
as your **sketch foler**. Then include the **configAssist,h** in your application and compile..

+ compile for arduino-esp3 or arduino-esp8266.
+ In order to compile you must install **ArduinoJson** library.
+ if your variables exceed **MAX_PARAMS** increase this value in class header.

###### If you get compilation errors on arduino-esp32 you need to update your arduino-esp32 library in the IDE using Boards Manager
