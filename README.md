# ConfigAssist

A lightweight library allowing quick configuration of **esp32/esp8266** devices. Setup application variables using a **responsive** configuration portal and a json definition dictionary. Variables are updated instantly using **async get requests** and saved automatically on a ini file in local storage.

<p align="center">
  <img src="docs/config.png">
</p>

## Description

**ConfigAssist** will help to automate definition of variables used in a typical **esp32/esp8266** application. It will automatically generate a web portal with html controls based on **json  definition** text, allowing quick editing from an html page. 

Application variables like **Wifi ssid**, **Wifi password**, **Host Name** can be edited there for use in your application. Every time a variable is changed in the web page, it will automatically updated by ConfigAssist using an **async** JavaScript get request. As the user leaves or closes the page at end the data are saved to the ini file.

**ConfigAssist** can also perform a **Wifi Scan** on setup and attach a **drop down list** on the field **st_ssid** with nearby available wifi **access points**. The list will be sorted by **signal strength** with the strongest wifi signal to be placed first and will be refreshed every 15 seconds. Users can choose a valid nearby **ssid** from the list.

**ConfigAssist** can also check and synchronize the internal **clock** of ESP device with the browser time if needed. So even if no internet connection (AP mode) and no **npt** server is available the device will have the correct time. If **TIMEZONE_KEY** string exists in variables it will be used to set the device time zone string. If not it will use browser offset.

These features can be disabled by setting **USE_WIFISCAN** and **USE_TIMESYNC** to false.

Variables **descriptions** and **default values** are based on a **text description** in json format that acts as a template defining the **type**, **label** and extra **info** of each variable. 

All config variables must be defined there describing the **variable type**, **default value** and the **label** that will be displayed to the user. It can also include and some **attributes** in case of special variables like **datalist** for list box, min, max, step for **input number** etc.
 ```
   "name": "st_ssid",
  "label": "Name for WLAN (Ssid to connect)",
"default": ""
   ```
A simple html page will be generated by **ConfigAssist** allowing editing these variables from a web Browser with a connection to the device. While editing the variables they will be **instantly** available to the application and will be automatically saved. 

Application variables can be used with operator **[]** i.e. ```conf['variable']```. The configuration data will be stored in the **SPIFFS** as an **ini file** <em>(Plain Text)</em> and will be automatically loaded on each reboot.

## How it works
On first run when no data (**ini file**) is present in local storage, **ConfigAssist** will start an **Access Point** and load the default json dictionary with variable definitions to be edited.  **ConfigAssist** will generate an html page with html controls allowing data to be edited from connected devices.

On each **change** in the html page data will be **updated** and will be available immediately to the application. Also the data will be saved automatically on local storage when the user finishes editing. If the configuration file is valid during next boot, **json dictionary** will not be loaded reducing memory consumption and speeding up the whole process. Note that if data is not changed **ConfigAssist** will not overwrite the file.

ConfigAssist uses **c++ vectors** to dynamically allocate and store variables and **binary search** for speeding the access process.

## How to use variables
**ConfigAssist** consists of single file "configAssist.h" that must be included in your application 
The application variables can be used directly by accessing the **class** itself by operator **[]**
i.e.

+ `String ssid = conf["st_ssid"];`
+ `bool debug = conf["debug"].toInt();`
+ `int pinNo = conf["st_ssid"].toInt();`
+ `digitalWrite(conf["led_pin"].toInt(), 0)`;
+ `float float_value = atof(conf["float_value"].c_str());`

## Variables definition with JSON dictionary

In your application sketch file you must define a json dictionary that includes all the information needed for the html edit form to be generated. Each variable will be displayed on edit page with the order defined in the json file.  See example below...


+ If you use keywords `name, default` an **edit box** will be generated to edit the variable. You can add `attribs` keywords to specify min, max, step for a numeric field.
+ If you keyword name contains ``_pass`` a **password field** will be used. See **PASSWD_KEY** definition. 
+ If you use keyword `checked` instead of `default` a Boolean value will be used that will be edited by a **check box**
+ You can combine keywords `default` with `options` in order to use a select list that will be edited by a **drop list**. 
  - The `options` field must contain a comma separated list of values and can be enclosed by single quotes.
+ You can combine keywords `default` with `range` in order to use a value that will be edited by a **input range**. 
  - The `range` field must contain a comma separated list of `min, max, step` and can be enclosed by single quotes.
+ You can combine keywords `default` with `datalist` in order to use a value that will be edited by a **combo box**. 
  - The `datalist` field must contain a comma or line feed separated list of default values for drop down list.
+ You can combine keywords `default` with `file` in order to use a small text be edited by a **text area**. 
  - The `file` field must contain a valid file path that the text will be saved to. The `default` keyword can also be used to define a default value.
  
A **separator title** can also be used to group configuration values under a specific title.
<p align="center">
  <img src="docs/config_colapsed.png">
</p>


```
const char* appConfigDict_json PROGMEM = R"~(
[{
 "seperator": "Wifi settings"
  },{
      "name": "st_ssid",
     "label": "Name for WLAN (Ssid to connect)",
   "default": ""
  },{
      "name": "st_pass",
     "label": "Password for WLAN",
   "default": ""
  },{
 "seperator": "Application settings"
  },{
      "name": "host_name",
     "label": "Host name to use for MDNS and AP",
   "default": "ConfigAssist"
 },{
      "name": "debug",
     "label": "Check to enable debug",
   "checked": "False"
 },{
      "name": "sensor_type",
     "label": "Enter the sensor type",
   "options": "'BMP280', 'DHT12', 'DHT21', 'DHT22'",
   "default": "DHT22"
 },{
      "name": "led_pin",
     "label": "Enter the pin that the led is connected",
   "default": "4",
   "attribs": "min='2' max='16' step='1'"
  },{ 
      "name": "refresh_rate",
     "label": "Enter the sensor update refresh rate",
     "range": "10, 50, 1",
   "default": "30"
 },{
      "name": "time_zone",
     "label": "Needs to be a valid time zone string",
   "default": "EET-2EEST,M3.5.0/3,M10.5.0/4",    
  "datalist": "
'Etc/GMT,GMT0'
'Etc/GMT-0,GMT0'
'Etc/GMT-1,<+01>-1'
'Etc/GMT-2,<+02>-2'"
}.{
   "name": "calibration_data",
  "label": "Enter data for 2 Point calibration.</br>Data will be saved to /calibration.ini",
   "file": "/calibration.ini",
"default": "X1=222, Y1=1.22
X2=900, Y2=3.24"
},{
```

## Project definitions in your main app

+ include the **configAssist**  class
  - `#include <configAssist.h>  //ConfigAssist class`

+ Define your static instance
  - `ConfigAssist conf;         //ConfigAssist class`

+ in your setup function you must init the config class with a pointer to the dictionary
  - `conf.initJsonDict(appConfigDict_json);` or `conf.init("/info.ini");` for default minimal settings

+ if you want to use a different external **ini file name**
  - `conf.init(ini_file_name, appConfigDict_json);`
  
## WIFI Access point handlers

**ConfigAssist** must be initialized with a pointer to a web server to automatically handle AP form requests.
Setup will add web handlers /cfg, /scan, to the server and if apEnable = true will enable Access Point.
```
conf.setup(server, /*Start AP*/ true);
```
You can add /cfg handler to your application after connecting the device to the internet.
Editing config will be enabled for station users.

```
//ConfigAssist will register handlers to the webserver
//After connecting to the internet AP will not be started
conf.setup(server);
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
    //Data will be availble instantly 
    conf.setup(server, true);
    return;
  }
  
  ...
  
  //Check connection
  if(WiFi.status() == WL_CONNECTED ){
    Serial.printf("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", conf["st_ssid"].c_str(), WiFi.localIP().toString().c_str()); 
  }else{
    //Fall back to Access point for editing config
    Serial.println("Connect failed.");
    conf.setup(server, true);
    return;
  }
  ```
  
**ConfigAssist** can also used to quick generate and store ini files.
Just call the class constructor with a filename to be saved.
```
ConfigAssist info("/info.ini");
```
and add the parameters to be stored with
```
info.put("var1", "test1", true);
info.saveConfigFile();
```
## Logging to a file
**ConfigAssist** can redirect serial print functions to a file in a spiffs and a **Debug log** can be generated.
In you application you use **LOG_ERR**, **LOG_WRN**, **LOG_INF**, **LOG_DBG** macros instead of **Serial.prinf**
to print your messages. **ConfigAssist** can record these messages with **timestamps** to a file.

if you want to enable serial print to a log file..
+ define **ConfigAssist**  external variables
  ```
  extern bool ca_logToFile;
  extern byte ca_logLevel;  
  ```
+ Then enable record to file in your **setup** function use..
  ```
  //Enable configAssist logging to file
  ca_logToFile = true;    
  //Set configAssist default log level
  ca_logLevel = DEF_LOG_LEVEL;
  ```
  
## Compile
Donwload library files and place them on ./libraries directory under ArduinoProjects
Then include the **configAssist,h** in your application and compile..

+ compile for arduino-esp3 or arduino-esp8266.
+ In order to compile you must install **ArduinoJson** library.
+ if your variables exceed **MAX_PARAMS** increase this value in class header.

###### If you get compilation errors on arduino-esp32 you need to update your arduino-esp32 library in the IDE using Boards Manager

## Other examples
You can see an advanced example of **ConfigAssist** usage on <a target="_blank" title="PlantStatus LilyGO TTGO Higrow" href="https://github.com/gemi254/PlantStatus-LilyGO-TTGO-Higrow">**PlantStatus**</a> a plant monitoring and logging application
