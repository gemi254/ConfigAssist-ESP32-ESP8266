#define APP_VERSION "1.0"            // Version
#define MAX_PARAMS 50                // Maximum parameters to handle
#define DELIM '~'                    // Ini file pairs seperator
#define CONF_FILE "/config.ini"      // Ini file to save configuration
#define DONT_ALLOW_SPACES false       // Allow spaces in var names ?
#define HOSTNAME_KEY "host_name"     // The key that defines host name

//Structure for config elements
struct confPairs {
    String name;
    String value;
    String label;
    int readNo;
};

// Define Platform libs
#if defined(ESP32)
  #define WEB_SERVER WebServer
  #define STORAGE SPIFFS // one of: SPIFFS LittleFS SD_MMC 
#else
  #define WEB_SERVER ESP8266WebServer
  #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC 
#endif

// LOG shortcuts
#ifdef ESP32
  #define LOG_NO_COLOR
  #define LOG_COLOR_DBG
  #define DBG_FORMAT(format) LOG_COLOR_DBG "[%s DEBUG @ %s:%u] " format LOG_NO_COLOR "", esp_log_system_timestamp(), pathToFileName(__FILE__), __LINE__
  #define LOG_ERR(format, ...) Serial.printf(DBG_FORMAT(format), ##__VA_ARGS__)
  #define LOG_DBG(format, ...) Serial.printf(DBG_FORMAT(format), ##__VA_ARGS__)
  #define LOG_WRN(format, ...) Serial.printf(DBG_FORMAT(format), ##__VA_ARGS__)
  #define LOG_INF(format, ...) Serial.printf(DBG_FORMAT(format), ##__VA_ARGS__)
#else
  //#define LOG_DBG(format, ...) Serial.printf(format, ##__VA_ARGS__)
  #define LOG_ERR Serial.printf
  #define LOG_DBG Serial.printf
  #define LOG_WRN Serial.printf
  #define LOG_INF Serial.printf
#endif

//Memory static valiables (html pages)
#include "configAssistPMem.h"

// ConfigAssist class
class ConfigAssist{ 
  public:
    ConfigAssist() {_dict = false; _valid=false; _hostName="";}
    ~ConfigAssist() {}
  
  public:  
    // Load configs after storage started
    void init(const char * jStr,String ini_file=CONF_FILE) { 
      _jStr = jStr;
      loadConfigFile(ini_file); 
      //On fail load defaults from dict
      if(!_valid) loadJsonDict(_jStr);
      //Set hostname
      if(_valid) _hostName = get(HOSTNAME_KEY);
      if(_hostName=="") _hostName = getDefaultHostName("ESP");
    }
    
    //if not Use dictionary laod default minimal config
    //For quick connection to wifi
    void init() { 
      init(NULL);
    }

    // Is config loaded valid ?
    bool valid(){ return _valid;}

    // Start an AP with web server and edit config values loaded from json dictionary
    void setup(WEB_SERVER &server, std::function<void(void)> handler) {
      LOG_INF("Config starting AP..\n");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(_hostName.c_str(),"",1);
      LOG_INF("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());      
      if (MDNS.begin(_hostName.c_str()))  LOG_INF("AP MDNS responder Started\n");      
      server.begin();
      server.on("/",handler);
      server.on("/config",handler);
      LOG_INF("AP HTTP server started");
    } 

    // Get a temponary hostname
    static String getDefaultHostName(String appName){
      String mac = WiFi.macAddress();
      mac.replace(":","");
      mac = mac.substring(6);
      return String(appName) + "_" + mac;  
    }

    // Implement operator [] i.e. val = config['key']    
    String operator [] (String k) { return get(k); }
        
    // Return next key and value from configs on each call in key order
    bool getNextKeyVal(String &keyName, String &keyVal, String &label, uint8_t &readNo) {
      static uint8_t row = 0;
      if (row++ < _configs.size()) {          
          keyName = _configs[row - 1].name.c_str();
          keyVal = _configs[row - 1].value.c_str(); 
          label = _configs[row - 1].label.c_str(); 
          readNo = _configs[row - 1].readNo;
          return true;
      }
      // end of vector reached, reset
      row = 0;
      return false;
    }
    
    // return the value of a given key, Empty on not found
    String get(String variable) {
      int keyPos = getKeyPos(variable);
      if (keyPos >= 0) {
        return _configs[keyPos].value;        
      }
      return String(""); 
    }
    
    // Update the value of thisKey = value
    bool put(String thisKey, String value) {      
      LOG_INF("Put key %s, val %s\n", thisKey.c_str(), value.c_str()); 
      int keyPos = getKeyPos(thisKey);
      if (keyPos >= 0) {
        _configs[keyPos].value = value;
        return true;
      }
      LOG_ERR("Put failed on Key: %s=%s\n", thisKey.c_str(), value.c_str());
      return false;      
    }

    // Display config items
    void dump(){
      String var, val, lbl; uint8_t readNo;
      while (getNextKeyVal(var, val, lbl, readNo)){
          LOG_INF("[%u]: %s = %s; %s\n",readNo, var.c_str(), val.c_str(), lbl.c_str() );
      }
    }

    // Add vectors by key (name in confPairs)
    void add(String key, String val, int readNo=0, String lbl=""){
      //LOG_INF("Adding key %s, val %s\n", key.c_str(), val.c_str()); 
      _configs.push_back({key, val, lbl, readNo}) ;      
    }

    // Sort vectors by key (name in confPairs)
    void sort(){
      std::sort(_configs.begin(), _configs.end(), [] (
        const confPairs &a, const confPairs &b) {
        return a.name < b.name;}
      );
    }

    // Sort vectors by key (readNo in confPairs)
    void sortReadOrder(){
      std::sort(_configs.begin(), _configs.end(), [] (
        const confPairs &a, const confPairs &b) {
        return a.readNo < b.readNo;}
      );
    }

    // Load json description file. 
    // if update=true update only additional pair info
    int loadJsonDict(String jStr, bool update=false) { 
      if(jStr==NULL) return loadJsonDict(appDefConfigDict_json, update);      
      DeserializationError error;
      const int capacity = JSON_ARRAY_SIZE(MAX_PARAMS)+ MAX_PARAMS*JSON_OBJECT_SIZE(8);
      DynamicJsonDocument doc(capacity);
      error = deserializeJson(doc, jStr.c_str());      
      if (error) { 
        LOG_ERR("Deserialize Json failed: %s\n", error.c_str());
        _dict = false;   
        return false;    
      }
      //Parse json description
      JsonArray array = doc.as<JsonArray>();
      int i=0;
      for (JsonObject  obj  : array) {        
        if (obj.containsKey("name") && obj.containsKey("default") ) {
          String key = obj["name"];
          String val = obj["default"];
          String lbl = obj["label"];
          if(!update){
            //LOG_INF("Json add: %s, val %s\n", key.c_str(), val.c_str());            
            add(key, val, i, lbl);
          }else{
            int keyPos = getKeyPos(key);
            if (keyPos >= 0) {
              //LOG_INF("Json upd: %i, %s, val %s\n", keyPos, key.c_str(), val.c_str());            
              // update other fields but not value        
              _configs[keyPos].readNo = i;
              _configs[keyPos].label = lbl;                
            }else{
              LOG_ERR("Not exists in json Key: %s\n", key.c_str());
            }
          }
          i++;             
        }else{
          LOG_ERR("Undefined keys name/default on param : %i.", i);
        }
      }
      //sort vector for binarry search
      sort();
      _dict = true;
      if(!update) _valid = true;
      LOG_INF("Loaded json dict\n");
      return i;
    }
     
    // Load config pairs from an ini file
    bool loadConfigFile(String filename) {
      #if defined(ESP32)
        File file = STORAGE.open(filename);
      #else
        File file = STORAGE.open(filename,"r");
      #endif
      if (!file || !file.size()) {
        LOG_ERR("Failed to load: %s, sz: %u\n", filename.c_str(), file.size());
        //if (!file.size()) STORAGE.remove(CONFIG_FILE_PATH); 
        _valid = false;       
        return false;
      }else{
        _configs.reserve(MAX_PARAMS);
        // extract each config line from file
        while (file.available()) {
          String configLineStr = file.readStringUntil('\n');
          //LOG_INF("Load: %s\n" , configLineStr.c_str());
          //if (!configLineStr.length()) continue;
          loadVectItem(configLineStr);
        } 
        sort();
      }
      LOG_INF("Loaded config: %s\n",filename.c_str());
      _valid = true;
      return true;
    }

    // Delete configuration file
    bool deleteConfig(String filename){
      LOG_INF("Remove config: %s\n",filename.c_str());
      return STORAGE.remove(filename);
    }

    // Save configs vectors in an ini file
    bool saveConfigFile(String filename) {
      File file = STORAGE.open(filename, "w+");
      char configLine[512];
      if (!file){
        LOG_ERR("Failed to save config file\n");
        return false;
      }else{
        for (const auto& row: _configs) {
          // recreate config file with updated content
          //LOG_INF("Save: %s=%s\n", row.name.c_str(), row.value.c_str());
          if (!strcmp(row.name.c_str() + strlen(row.name.c_str()) - 5, "_Pass")) {
              // replace passwords with asterisks
              sprintf(configLine, "%s%c%s\n", row.name.c_str(), DELIM, "************");   
          }else {
              sprintf(configLine, "%s%c%s\n", row.name.c_str(), DELIM, row.value.c_str());   
          }
          file.write((uint8_t*)configLine, strlen(configLine));
        }        
        LOG_INF("File saved: %s\n", filename.c_str());
        file.close();
        
        return true;
      }
    }

    // Respond a HTTP request for the form use the CONF_FILE
    // to save. Save, Reboot ESP, Reset to defaults, cancel edits
    void handleFormRequest(WEB_SERVER * server){
      //LOG_INF("Handle req: %i\n",server->args());
      //Save config form
      if (server->args() > 0) {
        server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        
        //Discard and load json defaults;
        if (server->hasArg(F("RST"))) {
          deleteConfig(CONF_FILE);
          _configs.clear();
          loadJsonDict(_jStr);
          //dump();
          //saveConfigFile(CONF_FILE);
          LOG_INF("_valid: %i\n", _valid);
          if(!_valid){
            server->send(200, "text/html", "Failed to load config.");
          }else{
            server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/config\">");
          }
          server->client().flush(); 
          return;
        }      
        //Reload and loose changes
        if (server->hasArg(F("CANCEL"))) {
          server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/\">");
          return;
        }    
        //Update configs from form post vals
        for(uint8_t  i=0; i<server->args(); ++i ){
          String key(server->argName(i));
          String val(server->arg(i));
          if(key=="apName" || key =="SAVE" || key=="RST" || key=="RBT" || key=="plain") continue;
          put(key, val);
          String msg = " " + server->argName(i) + ": " + server->arg(i);
          LOG_INF("Form upd: %s\n", msg.c_str());
        }
        //Save config file 
        if (server->hasArg(F("SAVE"))) {
            String out(HTML_PAGE_MESSAGE);
            if(saveConfigFile(CONF_FILE)) out.replace("{msg}", "Config file saved");
            else out.replace("{msg}", "<font color='red'>Failed to save config</font>");            
            out.replace("{title}", "Save");
            out.replace("{url}", "/config");
            out.replace("{refresh}", "3000");            
            server->send(200, "text/html", out);
            server->client().flush(); 
            delay(1000);
        }
        
        if (server->hasArg(F("RBT"))) {
            //saveConfigFile(CONF_FILE);
            String out(HTML_PAGE_MESSAGE);
            out.replace("{title}", "Reboot device");
            out.replace("{url}", "/config");
            out.replace("{refresh}", "10000");
            out.replace("{msg}", "Device will restart in a few seconds");      
            server->send(200, "text/html", out);
            server->client().flush(); 
            delay(1000);
            ESP.restart();
        }
        return;
      }
      LOG_INF("Config form _valid: %i, _dict: %i\n", _valid, _dict);
      //Load dictionary if no pressent
      if(!_dict) loadJsonDict(_jStr, true);
      //Send config form data
      server->setContentLength(CONTENT_LENGTH_UNKNOWN);
      String out(HTML_PAGE_START);
      out.replace("{appName}", _hostName);
      server->send(200, "text/html", out); 
      sortReadOrder();
      //Render html keys
      String var, val, lbl; uint8_t readNo;
      while (getNextKeyVal(var, val, lbl, readNo)){
          out = String(HTML_PAGE_INPUT_LINE);
          out.replace("{key}",var);
          out.replace("{lbl}",lbl);
          out.replace("{val}",val);
          server->sendContent(out);
          //LOG_INF("HTML key: %s = %s [%i] %s\n",var.c_str(), val.c_str(), readNo, lbl.c_str() );
      }
      sort();
      server->sendContent(HTML_PAGE_END);
    }
  private:
    // Get location of given key to retrieve other elements
    int getKeyPos(String thisKey) {
      if (_configs.empty()) return -1;
      auto lower = std::lower_bound(_configs.begin(), _configs.end(), thisKey, [](
          const confPairs &a, const String &b) { 
          //Serial.printf("Comp: %s %s \n", a.name.c_str(), b.c_str());    
          return a.name < b;}
      );
      int keyPos = std::distance(_configs.begin(), lower); 
      if (thisKey == _configs[keyPos].name) return keyPos;
      else LOG_INF("Key %s not found\n", thisKey.c_str()); 
      return -1; // not found
    }

    // Extract a config tokens from input and load into configs vector
    void loadVectItem(String keyValPair) {  
      if (keyValPair.length()) {
        std::string token[2];
        int i = 0;
        std::istringstream ss(keyValPair.c_str());
        while (std::getline(ss, token[i++], DELIM));
        if (i != 3) 
          if (i == 2) { //Empty param
            String key(token[0].c_str()); 
            add(key, "");
          }else{
            LOG_ERR("Unable to parse '%s', len %u, items: %i\n", keyValPair.c_str(), keyValPair.length(), i);
          }
        else {
          String key(token[0].c_str()); 
          String val(token[1].c_str());
          val.replace("\r","");
          val.replace("\n","");
          #if DONT_ALLOW_SPACES
          val.replace(" ","");
          #endif                    
          //No label used for memory issues
          add(key, val);
        }
      }
      if (_configs.size() > MAX_PARAMS) 
        LOG_WRN("Config file entries: %u exceed max: %u\n", _configs.size(), MAX_PARAMS);
    }
     
  private: 
    std::vector<confPairs> _configs;
    bool _valid;
    bool _dict;
    String _hostName;
    const char * _jStr;    
};
