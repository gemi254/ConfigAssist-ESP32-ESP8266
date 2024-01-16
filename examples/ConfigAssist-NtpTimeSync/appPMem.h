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
      default: NtpTimeSync  
  - led_buildin:
      label: Enter the pin that the build in led is connected. 
        Leave blank for auto.
      attribs: "min='4' max='23' step='1'"
      default: 4

Time settings:
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

  - ntp_server1: 
      label: Time server to sync time1
      default: "europe.pool.ntp.org"
  - ntp_server2: 
      label: Time server to sync time2
      default: "time.windows.com"
  - ntp_server3: 
      label: Time server to sync time3
      default: "pool.ntp.org"
)~"; 
