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
      default: ConfigAssist_{mac}

Application settings:
  - app_name:
      label: Name your application
      default: VarAttribues  
  - led_buildin:
      label: Pin that the build in led is connected. Leave blank for auto.
        Usually pin 4 on esp32, 2 on esp8266
      attribs: "min='2' max='23' step='1'"
      default:
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
      attribs: onClick="enableDisableBatchUpload()"  
  - batch_size:
      label: "Measurements size"
      default: 5
      attribs: >-
        onChange="const ctrl = document.getElementById('update_interval');
        const ui = ((ctrl.value * this.value) / 1000)/60 + ' mins';
        this.parentElement.parentElement.childNodes[5].innerHTML = 'Upload every&#8282; ' +  ui;"
  - update_interval:
      label: "Delay between measurements (ms)"
      default: 30000
      attribs: >-
        onChange="
          const ctrl = document.getElementById('batch_size');
          const ui = ((ctrl.value * this.value) / 1000)/60 + ' mins';
          ctrl.parentElement.parentElement.childNodes[5].innerHTML = 'Upload every&#8282; ' +  ui;" 
)~"; 

PROGMEM const char INIT_SCRIPT[] = R"=====(
function enableDisableBatchUpload(){
  if($('#batch_upload').checked){
    document.getElementById('batch_size').disabled = false;
    document.getElementById('update_interval').disabled = false;
  }else{
    document.getElementById('batch_size').disabled = true;
    document.getElementById('update_interval').disabled = true;
  }
}
document.addEventListener('DOMContentLoaded', function (event) {
  enableDisableBatchUpload();
});  
)=====";