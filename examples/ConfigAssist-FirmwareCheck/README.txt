This is an Example hosting firmware updates of an esp application on a remote web server (like github.com) and 
perform automatic check and upgrade when the version is changed.
  
The firmware version of the device running ConfigAssist is compared with the latest version stored in the remote web server.
If there is a change an `Upgrade firmware` button will be displayed in order to perform an automatic firware upgrade to 
the newest version.

How to use this feature..
Bellow is the steps needed to use firmware check and upgrade using ConfigAssist. This example 
is using github.com to host the new firmware.

!. Create a repo in github.com with your project.
    * For example `https://github.com/gemi254/ControlAssist-ESP32-ESP8266`

2. Create a sub folder to store your firmware information and the binary file.
    * For example `https://github.com/gemi254/ControlAssist-ESP32-ESP8266/firmware`

3. Create e file named "latest.json" inside your firmware folder with firmware information in json format.
    * Inside latest.json put the latest version number, a short description, and a link to the firmware file.
    * For example `https://github.com/gemi254/ControlAssist-ESP32-ESP8266/firmware/latest.json`
        contents:
        {
        "ver" : "1.0.1",
        "url" : "https://raw.githubusercontent.com/gemi254/ConfigAssist-ESP32-ESP8266/main/examples/ConfigAssist-FirmwareCheck/firmware/FirmwareCheck1.0.1.bin",
        "descr": "A new firmware v1.0.1 with bug fixes and improvements."
        }

4. Set the default value of variable ``firmware_url``.
    * In you project at your VARIABLES_DEF_YAML (ConfigAssist-FirmwareCheck.ino line 55).

6. In you project, define your current firmware version. 
    *  char FIRMWARE_VERSION[] = "1.0.0";

7. Compile and rename the firmware file.
    * firmware.bin -> FirmwareCheck1.1.1.bin

8. Upload the firmware to github at url defined in latest.json

9. By pressing `Firmware` button in config page, configAssist will compare current version with the version stored in github.com
If newer version exists it will show a button `Upgrade Firmware` that will automatically perform a firmware upgrade with 
the latest firmware stored in github.com
