const char* VARIABLES_DEF_YAML PROGMEM = R"~(
Wifi settings:
  - st_ssid:
      label: Name for WLAN
      default: mdk3
  - st_pass:
      label: Password for WLAN
      default: 2843028858
  - host_name: 
      label: >-
        Host name to use for MDNS and AP<br>{mac} will be replaced with device's mac id
      default: ConfigAssist_{mac}

Application settings:
  - app_name:
      label: Name your application
      default: VarAttribues  
  - led_buildin:
      label: Enter the pin that the build in led is connected. 
        Leave blank for auto.
        Usually pin 4 on esp32, pin 2 on esp8266
      attribs: "min='4' max='23' step='1'"
      default: 4
  - display_style:
      label: Choose how the config sections are displayed. 
        Must reboot to apply
      options: AllOpen: 0
        - AllClosed: 1 
        - Accordion : 2
        - AccordionToggleClosed : 3
      default: AccordionToggleClosed   

Batch upload settings:
  - batch_upload:
      label: "Enable batch upload measurements"
      checked: False
      attribs: >-
        onClick="
        if(this.checked){
          document.getElementById('batch_size').disabled = false;
          document.getElementById('update_interval').disabled = false;
        }else{
          document.getElementById('batch_size').disabled = true;
          document.getElementById('update_interval').disabled = true;
        }"  
  - batch_size:
      label: "Measurements size"
      default: 5
      attribs: >-
        onChange="const ctrl = document.getElementById('update_interval');
        const ui = ((ctrl.value * this.value) / 1000)/60 + ' mins';
        this.parentElement.parentElement.childNodes[5].innerHTML = 'Upload every&#8282; ' +  ui;
        "
  - update_interval:
      label: "Delay between measurements (ms)"
      default: 30000
      attribs: >-
        onChange="
          const ctrl = document.getElementById('batch_size');
          const ui = ((ctrl.value * this.value) / 1000)/60 + ' mins';
          ctrl.parentElement.parentElement.childNodes[5].innerHTML = 'Upload every&#8282; ' +  ui;
        " 
)~"; 
