# ConfigAssist

A lightweight library allowing quick configuration of **esp32/esp8266** devices. **Define** application variables using a json dictionary and **edit** them with a **responsive** configuration portal. Variables are updated instantly using **async get requests** and saved automatically on a ini file in local storage.

## Description

**ConfigAssist** will help to automate definition of variables used in a typical **esp32/esp8266** application. It will automatically generate a web portal with edit html elements for each variable based on **json definition** text, allowing quick editing is value from a html page. 

Application variables like **Wifi ssid**, **Wifi password**, **Host Name** can be quick edited there and will be ready for use in your application. Every time a variable is changed in the web page, it will automatically updated by ConfigAssist using an **async** JavaScript request. As the user leaves or closes the page at end the data are auto saved to the ini file.

## Features

* Automate **variables** definition in a typical ``ESP32/ESP8266`` project using a json definition description.
* Configuration **portal** with various **html controls** for editing these variables from a ``web browser``.
* Automatically generate an **ini** file in internal storage and auto save.
* On the fly **update** values in ini file using **ajax** requests.
* **Backup & Restore** device configurations.
* **Wi-Fi scan** support for auto fill nearby Wi-Fi station connections.
* Save **WiFi credentials** to **nvs** to retain ST connections.
* **Validate** Wi-Fi station connections when connecting from AP.
* Auto **synchronize** ESP32/ESP8266 internal **clock** with browser clock.
* Support on the fly **firmware upgrades** (OTA).
* Support automatic firmware **upgrades** over the **internet**.

<p align="center">
  <img src="docs/config.png">
</p>


## Features description

**ConfigAssist** can perform a **Wifi Scan** on setup and attach a **drop down list** on the field **st_ssid** with nearby available wifi **access points**. The list will be sorted by **signal strength**, with the strongest signals to be placed first and will be refreshed every 15 seconds. Users can choose a valid nearby **ssid** from the wifi list. 

**Station** Wifi **connections** can be **validated** during setup with ``Test connection`` link available on each **st_ssid** field. The device will be switched to **WIFI_AP_STA** and **ConfigAssist** will try to test the Station connection with **Wifi ssid**, **Wifi password** entered without disconnecting from the Access Point. If the connection is successful the Station ip address and signal strength will be displayed.

**WiFi credentials** can also be saved to **nvs** to retain ST connections. On **factory defaults** the nvs will be not cleared and the ST connection will be still available.
You can use the command ``http://ip/cfg?_CLEAR=1`` to clear nvs.
Uncomment **CA_USE_PERSIST_CON** in ``ConfigAssist.h`` to use this feature.

**ConfigAssist** can also check and synchronize the internal **clock** of ESP device with the browser time if needed. So even if no internet connection (AP mode) and no **npt** server is available the device will get the correct time. If **CA_TIMEZONE_KEY** string exists in variables it will be used to set the device time zone string. If not it will use browser offset.

**ConfigAssist** can add web based **OTA** updates to your ESP32/ESP8266 projects. With the button **Upgrade** you can upload a firmware file (*.bin) from you pc and perform a **firmware upgrade** to the device.

**ConfigAssist** can also pefrom **Firmware upgrades** over **internet**. It will compare the currnet firmware version of the device with a remote firmware stored in a **web site** location. If there is a new firmware it will automatically download it and pefrom the upgrade.

Check the <a href="/examples/ConfigAssist-FirmwareCheck/README.txt">FirmwareCheck</a>  example for more details.

These features can be disabled to save memory by commenting the lines **CA_USE_WIFISCAN**, **CA_USE_TESTWIFI**, **CA_USE_TIMESYNC**, **CA_USE_OTAUPLOAD**, and **CA_USE_FIMRMCHECK** in ``configAssist.h``.

Device's configuration ``(*.ini files)`` can be downloaded with the **Backup** button and can be restored later with the **Restore** button.

You can use the command ``http://ip/cfg?_RST=1`` to manually clear the ini file and load defaults.

## Configuration variables

Variables **descriptions** and **default values** are based on a **text description** in json format that acts as a template defining the **type**, **label** and extra **info** of each variable. 

All config variables must be defined there describing the **variable type**, **default value** and the **label** that will be displayed to the user. It can also include and some **attributes** in case of special variables like **datalist** for list box, min, max, step for **input number** etc.
 ```
   "name": "st_ssid",
  "label": "Name for WLAN (Ssid to connect)",
"default": ""
   ```
A simple html page will be generated by **ConfigAssist** allowing quick editing these variables from a web Browser with a connection to the device. While editing the variables they will be **instantly** available to the application and will be automatically saved. 

Application variables can be used in code with operator **[]** i.e. ```conf['variable']```. The configuration data will be stored in the **SPIFFS** as an **ini file** <em>(Plain Text)</em> and will be automatically loaded on each reboot.

## How it works

On first run when no data (**ini file**) is present in local storage, **ConfigAssist** will start an **Access Point** and load the default json dictionary with variable definitions that will be edited. It will then generate an html page with html controls allowing data to be edited from the connected devices.

On each **change** in the html page data will be **updated** and will be available immediately to the application. The data will be saved automatically on local storage when the user finishes editing. If the configuration file is valid during next boot, **json dictionary** will not be loaded reducing memory consumption and speeding up the whole process. Note that if data is not changed **ConfigAssist** will not re-save the file.

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
+ If you keyword name ends with ``_pass`` (or ``_pass`` and a number i.e``_pass1``) a **password field** will be used. See **CA_PASSWD_KEY** definition. 
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
const char* VARIABLES_DEF_JSON PROGMEM = R"~(
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


+ Define your static instance with **defaults**
  - `ConfigAssist conf;`        

+ if you want to use a different external **ini file name**  
  - `ConfigAssist conf(INI_FILE);`

+ if you want to use a different external **ini file name** and **json description**
  - `ConfigAssist conf(INI_FILE, VARIABLES_DEF_JSON);  // ConfigAssist with custom name & dictionary`

+ if you want to use a different external **ini file name** and **json description** disabled
  - `ConfigAssist conf(INI_FILE, NULL);  // ConfigAssist with custom ini name & dictionary disabled`
 
## WIFI Access point handlers

**ConfigAssist** must be initialized with a pointer to a web server to automatically handle AP form requests.
Setup will add web handlers /cfg, /scan, to the server and if apEnable = true will enable Access Point.
```
conf.setup(server, /*Start AP*/ true);
```
You can add /cfg handler to your application after connecting the device to the internet.
Editing config will be enabled for station users.

```
//ConfigAssist will register handlers to the web server
//After connecting to the internet AP will not be started
conf.setup(server);
```

## Setup function
```
void setup()
  // Must have storage to read from
  STORAGE.begin(true);
  
  //Will try to load ini file. On fail load json descr.
  if( conf["st_ssid"]=="" ){  //Ssid empty  
    //Start Access point server and edit config
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
Just call the class constructor with a filename to be saved and null to disable json.
```
ConfigAssist info("/info.ini", NULL);
```
and add the parameters to be stored with
```
info.put("var1", "test1", true);
info.saveConfigFile();
```
## Logging to a file
**ConfigAssist** can redirect serial print functions to a file in a spiffs and a **Debug log** can be generated.
In your application you use **LOG_E**, **LOG_W**, **LOG_I**, **LOG_D** macros instead of **Serial.prinf**
to print your messages. **ConfigAssist** can record these messages with **timestamps** to a file.

+ define **ConfigAssist**  log mode
Define the log level (Error = 1 .. Verbose = 5)
  ```
  #define LOGGER_LOG_LEVEL 5 // Errors & Warnings & Info & Debug & Verbose
  ```
if you want to enable serial print to a log file use..
  ```
  //Enable configAssist logging to file
  #define LOGGER_LOG_MODE  2 // Log to file

  //Define the log filename
  #define LOGGER_LOG_FILENAME "/log1"
  ```
 You can also use your own log_printf function by setting log mode to 3
   ```
 #define LOGGER_LOG_MODE  3        // External log_printf function
  ```

 Check ConfigAssist-LotExternal.ino in ``examples/`` folder.

## Compile
Download library files and place them on ./libraries directory under ArduinoProjects
Then include the **configAssist.h** in your application and compile..

+ compile for arduino-esp3 or arduino-esp8266.
+ In order to compile you must install **ArduinoJson** library.
+ To use Persistent ST connections On ESP8266 devices you must install **Preferences** library to provide ESP32-compatible Preferences API using LittleFS
+ if your variables exceed **CA_MAX_PARAMS** increase this value in class header.

You can remove old **ini** config file by calling `conf.deleteConfig();` in your setup function.
See **ConfigAssist-ESP32-ESP8266.ino** line:210

###### If you get compilation errors on arduino-esp32 you need to update your arduino-esp32 library in the IDE using Boards Manager

## Other examples
You can see an advanced example of **ConfigAssist** usage on <a target="_blank" title="PlantStatus LilyGO TTGO Higrow" href="https://github.com/gemi254/PlantStatus-LilyGO-TTGO-Higrow">**PlantStatus**</a> a plant monitoring and logging application
