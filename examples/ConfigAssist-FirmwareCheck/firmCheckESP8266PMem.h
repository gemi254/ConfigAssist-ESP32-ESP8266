const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi settings:
  - st_ssid:
      label: Name for WLAN
      default:
  - st_pass:
      label: Password for WLAN
      default:
  - host_name: 
      label: >-
        Host name to use for MDNS and AP<br>{mac} will be replaced with device's mac id
      default: configAssist_{mac}

Application settings:
  - app_name:
      label: Name your application
      default: FirmwareCheck  
  - firmware_url:
      label: Firmware upgrade url with version and info
      default: >-
        https://raw.githubusercontent.com/gemi254/ConfigAssist-ESP32-ESP8266/main/examples/ConfigAssist-FirmwareCheck/firmware/ESP8266/lastest.json
  - time_zone:
      label: Needs to be a valid time zone string
      default: EET-2EEST,M3.5.0/3,M10.5.0/4 
      datalist: 
        - Etc/GMT,GMT0
        - Etc/GMT-0,GMT0
        - Etc/GMT-1,<+01>-1
        - Etc/GMT-2,<+02>-2
        - Etc/GMT-3,<+03>-3
        - Etc/GMT-4,<+04>-4
        - Etc/GMT-5,<+05>-5
        - Etc/GMT-6,<+06>-6
        - Etc/GMT-7,<+07>-7
        - Etc/GMT-8,<+08>-8
        - Etc/GMT-9,<+09>-9
        - Etc/GMT-10,<+10>-10
        - Etc/GMT-11,<+11>-11
        - Etc/GMT-12,<+12>-12
        - Etc/GMT-13,<+13>-13
        - Etc/GMT-14,<+14>-14
        - Etc/GMT0,GMT0
        - Etc/GMT+0,GMT0
        - Etc/GMT+1,<-01>1
        - Etc/GMT+2,<-02>2
        - Etc/GMT+3,<-03>3
        - Etc/GMT+4,<-04>4
        - Etc/GMT+5,<-05>5
        - Etc/GMT+6,<-06>6
        - Etc/GMT+7,<-07>7
        - Etc/GMT+8,<-08>8
        - Etc/GMT+9,<-09>9
        - Etc/GMT+10,<-10>10
        - Etc/GMT+11,<-11>11
        - Etc/GMT+12,<-12>12
)~"; 