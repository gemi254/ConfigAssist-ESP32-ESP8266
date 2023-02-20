#define CLASS_VERSION "1.7"          // Class version
#define MAX_PARAMS 50                // Maximum parameters to handle
#define DEF_CONF_FILE "/config.ini"  // Default Ini file to save configuration
#define INI_FILE_DELIM '~'           // Ini file pairs seperator
#define FILENAME_IDENTIFIER "_#"     // Keys ending is a hidden filename and not to be saved
#define DONT_ALLOW_SPACES false      // Allow spaces in var names ?
#define PASSWD_KEY   "_pass"         // The key part that defines a password field
#define HOSTNAME_KEY "host_name"     // The key that defines host name

// Define Platform libs
#if defined(ESP32)
  #define WEB_SERVER WebServer
  #define STORAGE SPIFFS // one of: SPIFFS LittleFS SD_MMC 
#else
  #define WEB_SERVER ESP8266WebServer
  #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC 
#endif

// LOG shortcuts
//#define DEBUG_CONFIG_ASSIST    //Uncomment to serial print DBG messages
#define logPrint Serial.printf
//void logPrint(const char *format, ...);
void logPrintNull(const char *format, ...) {}

#ifdef ESP32
  #define LOG_NO_COLOR
  #define LOG_COLOR_DBG
  #define DBG_FORMAT(format, type) LOG_COLOR_DBG "[%s %s @ %s:%u] " format LOG_NO_COLOR "", esp_log_system_timestamp(), type, pathToFileName(__FILE__), __LINE__
  #define LOG_ERR(format, ...) logPrint(DBG_FORMAT(format,"ERR"), ##__VA_ARGS__)
  #define LOG_WRN(format, ...) logPrint(DBG_FORMAT(format,"WRN"), ##__VA_ARGS__)
  #define LOG_INF(format, ...) logPrint(DBG_FORMAT(format,"INF"), ##__VA_ARGS__)  
  #if defined(DEBUG_CONFIG_ASSIST)
    #define LOG_DBG(format, ...) logPrint(DBG_FORMAT(format,"DBG"), ##__VA_ARGS__)
  #else    
    #define LOG_DBG(format, ...) logPrintNull(DBG_FORMAT(format,"DBG"), ##__VA_ARGS__)
  #endif
#else
  #define LOG_ERR logPrint
  #define LOG_WRN logPrint
  #define LOG_INF logPrint
  #if defined(DEBUG_CONFIG_ASSIST)
    #define LOG_DBG logPrint
  #else
    #define LOG_DBG logPrintNull    
  #endif
#endif

//Structure for config elements
struct confPairs {
    String name;
    String value;
    String label;
    String attribs;
    int readNo;
    byte type;
};

//Seperators of config elements
struct confSeperators {
    String name;
    String value;
};

//Memory static valiables (html pages)
#include "configAssistPMem.h"

// ConfigAssist class
class ConfigAssist{ 
  public:
    ConfigAssist() {_dict = false; _valid=false; _confFile=""; }
    ~ConfigAssist() {}
  public:  
    // Load configs after storage started
    void init(const char * jStr, String ini_file = DEF_CONF_FILE) { 
      _jStr = jStr;
      _confFile = ini_file;
      loadConfigFile(ini_file); 
      //On fail load defaults from dict
      if(!_valid) loadJsonDict(_jStr);
    }

    // if not Use dictionary load default minimal config
    void init() { init(NULL); }

    // Is config loaded valid ?
    bool valid(){ return _valid;}
    bool exists(String variable){ return getKeyPos(variable) >= 0; }

    // Start an AP with a web server and render config values loaded from json dictionary
    // for quick connection to wifi
    void setup(WEB_SERVER &server, std::function<void(void)> handler) {
      String hostName = getHostName();
      LOG_INF("ConfigAssist starting AP\n");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(hostName.c_str(),"",1);
      LOG_INF("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());      
      if (MDNS.begin(hostName.c_str()))  LOG_INF("AP MDNS responder Started\n");      
      server.begin();
      server.on("/",handler);
      server.on("/cfg",handler);
      LOG_INF("AP HTTP server started");
    } 
    // Get a temponary hostname
    static String getMacID(){
      String mac = WiFi.macAddress();
      mac.replace(":","");
      return mac.substring(6);
    }

    // Get a temponary hostname
    String getHostName(){
      String hostName = get(HOSTNAME_KEY);
      if(hostName=="") hostName = "ESP_ASSIST_" + getMacID();
      else hostName.replace("{mac}", getMacID());
      return hostName;
    }

    // Implement operator [] i.e. val = config['key']    
    String operator [] (String k) { return get(k); }
     
    // Return the value of a given key, Empty on not found
    String get(String variable) {
      int keyPos = getKeyPos(variable);
      if (keyPos >= 0) {
        return _configs[keyPos].value;        
      }
      return String(""); 
    }
    
    // Update the value of thisKey = value
    bool put(String thisKey, String value, bool force=false) {      
      LOG_DBG("Put %s=%s\n", thisKey.c_str(), value.c_str()); 
      int keyPos = getKeyPos(thisKey);      
      if (keyPos >= 0) {
        _configs[keyPos].value = value;
        return true;
      }
      if(force) add(thisKey, value);
      else LOG_ERR("Put failed on Key: %s=%s\n", thisKey.c_str(), value.c_str());
      return false;      
    }
    
    // Add vectors by key (name in confPairs)
    void add(String key, String val){
      confPairs d = {key, val, "", "", 0, 1 };
      //LOG_DBG("Adding  key %s=%s\n", key.c_str(), val.c_str()); 
      _configs.push_back(d);
    }

    // Add vectors pairs
    void add(confPairs &c){
       //LOG_DBG("Adding  key[%i]: %s=%s\n", readNo, key.c_str(), val.c_str()); 
      _configs.push_back({c}) ;
    }

    // Add seperator by key
    void addSeperator(String key, String val){
       //LOG_DBG("Adding sep key: %s=%s\n", key.c_str(), val.c_str()); 
      _seperators.push_back({key, val}) ;      
    }

    // Sort vectors by key (name in confPairs)
    void sort(){
      std::sort(_configs.begin(), _configs.end(), [] (
        const confPairs &a, const confPairs &b) {
        return a.name < b.name;}
      );
    }
    
    // Sort seperator vectors by key (name in confSeperators)
    void sortSeperators(){
      std::sort(_seperators.begin(), _seperators.end(), [] (
        const confSeperators &a, const confSeperators &b) {
        return a.name < b.name;}
      );
    }
    
    // Sort vectors by readNo in confPairs
    void sortReadOrder(){
      std::sort(_configs.begin(), _configs.end(), [] (
        const confPairs &a, const confPairs &b) {
        return a.readNo < b.readNo;}
      );
    }
    
    // Return next key and value from configs on each call in key order
    bool getNextKeyVal(confPairs &c) {
      static uint8_t row = 0;
      if (row++ < _configs.size()) { 
          c = _configs[row - 1];
          return true;
      }
      // end of vector reached, reset
      row = 0;
      return false;
    }

    // Get the configuration in json format
    String getJsonConfig(){
      confPairs c;
      String j = "{";
      while (getNextKeyVal(c)){
          j += "\"" + c.name + "\": \"" + c.value + "\",";          
      }
      if(j.length()>1) j.remove(j.length() - 1);
      j+="}";
      return j;
    }

    // Display config items
    void dump(){
      confPairs c;
      while (getNextKeyVal(c)){
          LOG_INF("[%u]: %s = %s; %s; %i\n", c.readNo, c.name.c_str(), c.value.c_str(), c.label.c_str(), c.type );
      }
    }
    
    // Load json description file. 
    // On update=true update only additional pair info
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
        confPairs c = {"", "", "", "", 0, 1};        
        if (obj.containsKey("name")){
          String k = obj["name"];
          String l = obj["label"];
          c.name = k;
          c.label = l;
          c.attribs ="";
          c.readNo = i;
          if (obj.containsKey("options")){       //Options list
            String d = obj["default"];
            String o = obj["options"];
            c.value = d;
            c.attribs = o;
            c.type = OPTION_BOX;
          }else if (obj.containsKey("datalist")){ //Combo box
            String d = obj["default"];
            String l = obj["datalist"];
            c.value = d;
            c.attribs = l;
            c.type = COMBO_BOX;            
           }else if (obj.containsKey("range")){  //Input range
            String d = obj["default"];
            String r = obj["range"];
            c.value = d;
            c.attribs = r;
            c.type = RANGE_BOX;
          }else if (obj.containsKey("file")){   //Text area
            String f = obj["file"];
            c.value = f;            
            if(obj.containsKey("default")){
              String d = obj["default"];
              c.attribs = d;
            }
            c.type = TEXT_AREA;
          }else if (obj.containsKey("checked")){ //Check box
            String ck = obj["checked"];
            c.value = ck;
            c.type = CHECK_BOX;
          }else if (obj.containsKey("default")){ //Edit box
            String d = obj["default"];
            c.value = d;
            c.type = TEXT_BOX;
          }else{
            LOG_ERR("Undefined value on param : %i.", i);
          }  

          if (c.type) { //Valid
            if(update){
              int keyPos = getKeyPos(c.name);
              if (keyPos >= 0) {
                //Update all other fields but not value,key        
                _configs[keyPos].readNo = i;
                _configs[keyPos].label = c.label;
                _configs[keyPos].type = c.type;
                _configs[keyPos].attribs = c.attribs;
                LOG_DBG("Json upd key[%i]=%s, label: %s, type:%i, read: %i\n", keyPos, c.name.c_str(), c.label.c_str(),c.type, i);
              }else{
                LOG_ERR("Undefined json Key: %s\n", c.name.c_str());
              }
            }else{
              LOG_DBG("Json add: %s, val %s, read: %i\n", c.name.c_str(), c.value.c_str(), i);                         
              add(c);
            }
            i++;
          }

        }else if (obj.containsKey("seperator")){
          String sepNo = "sep_" + String(i);
          String val =  obj["seperator"];
          addSeperator(sepNo, val);          
        }else{
          LOG_ERR("Undefined keys name/default on param : %i.", i);
        }
      }
      //sort vector for binarry search
      sort();
      sortSeperators();
      _dict = true;
      if(!update) _valid = true;
      LOG_INF("Loaded json dict\n");
      return i;
    }
     
    // Load config pairs from an ini file
    bool loadConfigFile(String filename="") {
      if(filename=="") filename = _confFile;
      File file = STORAGE.open(filename,"r");
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
          LOG_DBG("Load: %s\n" , configLineStr.c_str());          
          loadVectItem(configLineStr);
        } 
        sort();
      }
      file.close();
      LOG_INF("Loaded config: %s\n",filename.c_str());
      _valid = true;
      return true;
    }

    // Delete configuration file
    bool deleteConfig(String filename=""){
      if(filename=="") filename = _confFile;
      LOG_INF("Remove config: %s\n",filename.c_str());
      return STORAGE.remove(filename);
    }

    // Save configs vectors in an ini file
    bool saveConfigFile(String filename="") {
      if(filename=="") filename = _confFile;
      File file = STORAGE.open(filename, "w+");
      if (!file){
        LOG_ERR("Failed to save config file\n");
        return false;
      }
      //Save config file with updated content
      for (auto& row: _configs) {
        if(row.name==HOSTNAME_KEY){
          row.value.replace("{mac}", getMacID());
        }
        char configLine[512];
        sprintf(configLine, "%s%c%s\n", row.name.c_str(), INI_FILE_DELIM, row.value.c_str());   
        file.write((uint8_t*)configLine, strlen(configLine));
        LOG_DBG("Saved: %s = %s\n", row.name.c_str(), row.value.c_str());
      }        
      LOG_INF("File saved: %s\n", filename.c_str());
      file.close();      
      return true;      
    }

    // Respond a HTTP request for the form use the CONF_FILE
    // to save. Save, Reboot ESP, Reset to defaults, cancel edits
    void handleFormRequest(WEB_SERVER * server){
      //Save config form
      if (server->args() > 0) {
        server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        
        //Discard edits and load json defaults;
        if (server->hasArg(F("RST"))) {
          deleteConfig();
          _configs.clear();
          loadJsonDict(_jStr);
          //saveConfigFile(CONF_FILE);
          LOG_INF("_valid: %i\n", _valid);
          if(!_valid){
            server->send(200, "text/html", "Failed to load config.");
          }else{
            server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/cfg\">");
          }
          server->client().flush(); 
          return;
        }      
        //Reload and loose changes
        if (server->hasArg(F("CANCEL"))) {
          server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/\">");
          return;
        }    

        //Reset all check boxes.Only If checked will be posted
        for(uint8_t k=0; k < _configs.size(); ++k) {
          if(_configs[k].type==CHECK_BOX) _configs[k].value = "0";
        }
        //Update configs from form post vals
        for(uint8_t i=0; i<server->args(); ++i ){
          String key(server->argName(i));
          String val(server->arg(i));
          if(key=="apName" || key =="SAVE" || key=="RST" || key=="RBT" || key=="plain") continue;
          if(key.endsWith(FILENAME_IDENTIFIER)) continue;
          //Check if if text box with file name
          String fileNameKey = server->argName(i) + FILENAME_IDENTIFIER;
          if(server->hasArg(fileNameKey)){
              String fileName = server->arg(fileNameKey);
              saveText(fileName, val);
              val = fileName;
          }
          LOG_DBG("Form upd: %s = %s\n", key.c_str(), val.c_str());
          put(key, val);
        }

        //Save config file 
        if (server->hasArg(F("SAVE"))) {
            String out(CONFIGASSIST_HTML_MESSAGE);
            if(saveConfigFile()) out.replace("{msg}", "Config file saved");
            else out.replace("{msg}", "<font color='red'>Failed to save config</font>");            
            out.replace("{title}", "Save");
            out.replace("{url}", "/cfg");
            out.replace("{refresh}", "3000");            
            server->send(200, "text/html", out);
            server->client().flush(); 
            delay(500);
        }
        
        if (server->hasArg(F("RBT"))) {
            //saveConfigFile(CONF_FILE);
            String out(CONFIGASSIST_HTML_MESSAGE);
            out.replace("{title}", "Reboot device");
            out.replace("{url}", "/cfg");
            out.replace("{refresh}", "10000");
            out.replace("{msg}", "Device will restart in a few seconds");      
            server->send(200, "text/html", out);
            server->client().flush(); 
            delay(1000);
            ESP.restart();
        }
        return;
      }
      LOG_DBG("Generate form _valid: %i, _dict: %i\n", _valid, _dict);
      //Load dictionary if no pressent
      if(!_dict) loadJsonDict(_jStr, true);
      //Send config form data
      server->setContentLength(CONTENT_LENGTH_UNKNOWN);
      String out(CONFIGASSIST_HTML_START);
      out.replace("{host_name}", getHostName());
      server->sendContent(out);
      server->sendContent(CONFIGASSIST_HTML_CSS);
      server->sendContent(CONFIGASSIST_HTML_CSS_CTRLS);      
      server->sendContent(CONFIGASSIST_HTML_SCRIPT);
      out = String(CONFIGASSIST_HTML_BODY);
      out.replace("{host_name}", getHostName());
      server->sendContent(out);
      //Render html keys
      sortReadOrder();      
      while(getEditHtmlChunk(out)){
        server->sendContent(out);        
      }
      sort();
      out = String(CONFIGASSIST_HTML_END);
      out.replace("{appVer}", CLASS_VERSION);
      server->sendContent(out);
    }
    
    //Get edit page html table (no form)
    String getEditHtml(){
      sortReadOrder();      
      String ret = "";
      String out = "";
      while(getEditHtmlChunk(out)){
        ret += out;
      }
      sort();
      return ret;
    }

  private:
    // Load a file into a string
    bool loadText(String fPath, String &txt){
      File file = STORAGE.open(fPath, "r");
      if (!file || !file.size()) {
        LOG_ERR("Failed to load: %s, sz: %u B\n", fPath.c_str(), file.size());
        return false;
      }
      //Load text from file
      txt = "";
      while (file.available()) {
        txt += file.readString();
      } 
      LOG_DBG("Loaded: %s, sz: %u B\n" , fPath.c_str(), txt.length() );          
      file.close();
      return true;
    }
    
    // Write a string to a file
    bool saveText(String fPath, String txt){
      STORAGE.remove(fPath);
      File file = STORAGE.open(fPath, "w+");
      if(!file){
        LOG_ERR("Failed to save: %s, sz: %u B\n", fPath.c_str(), txt.length());
        return false;
      } 
      file.print(txt);
      LOG_DBG("Saved: %s, sz: %u B\n" , fPath.c_str(), txt.length() ); 
      file.close();
      return true;
    }

    // Render keys,values to html lines
    bool getEditHtmlChunk(String &out){      
      confPairs c;
      out="";
      if(!getNextKeyVal(c)) return false;
      out = String(CONFIGASSIST_HTML_LINE);
      String elm;
      if(c.type == TEXT_BOX){
        elm = String(CONFIGASSIST_HTML_TEXT_BOX);
        if(c.name.indexOf(PASSWD_KEY)>=0)
          elm.replace("<input ", "<input type=\"password\" ");
      }else if(c.type == TEXT_AREA){
        String file = String(CONFIGASSIST_HTML_TEXT_AREA_FNAME);        
        file.replace("{key}", c.name + FILENAME_IDENTIFIER);
        file.replace("{val}", c.value);
        elm = String(CONFIGASSIST_HTML_TEXT_AREA);
        String txt="";
        //Replace loaded text
        if(loadText(c.value, txt)){ 
          elm.replace("{val}", txt); 
        }else{ //Text not yet saved, load default
          elm.replace("{val}", c.attribs); 
        }
        elm = file + "\n" + elm;
      }else if(c.type == CHECK_BOX){
        elm = String(CONFIGASSIST_HTML_CHECK_BOX);        
        if(c.value=="True" || c.value=="on" || c.value=="1"){
          elm.replace("{chk}"," checked");
        }else
          elm.replace("{chk}","");
      }else if(c.type == OPTION_BOX){
        elm = String(CONFIGASSIST_HTML_SELECT_BOX);
        String optList = getOptionsListHtml(c.value, c.attribs);
        elm.replace("{opt}", optList);
      }else if(c.type == COMBO_BOX){
        elm = String(CONFIGASSIST_HTML_DATA_LIST);
        String optList = getOptionsListHtml(c.value, c.attribs, true);            
        elm.replace("{opt}", optList);
      }else if(c.type == RANGE_BOX){
        elm = getRangeHtml(c.value, c.attribs);
      }
      out.replace("{elm}",elm); 
      out.replace("{key}",c.name);
      out.replace("{lbl}",c.label);
      out.replace("{val}",c.value);
      String sKey = "sep_" + String(c.readNo);
      int sepKeyPos = getSepKeyPos(sKey);
      if(sepKeyPos>=0){
        //Add a seperator
        String outSep = String(CONFIGASSIST_HTML_SEP);
        String sVal = _seperators[sepKeyPos].value;
        outSep.replace("{val}", sVal);
         if(sepKeyPos > 0)
          out = String(CONFIGASSIST_HTML_SEP_CLOSE) + outSep + out;
        else 
          out = outSep + out;
        LOG_DBG("SEP key[%i]: %s = %s\n", sepKeyPos, sKey.c_str(), sVal.c_str());
      }  
      LOG_DBG("HTML key[%i]: %s = %s, type: %i, lbl: %s, attr: %s\n", c.readNo, c.name.c_str(), c.value.c_str(), c.type, c.label.c_str(), c.attribs.c_str() );
      return true;
    }

    // Render range 
    String getRangeHtml(String defVal, String attribs){
      char *token = NULL;
      char seps[] = ",";
      token = strtok(&attribs[0], seps);
      int i=0;
      String ret(CONFIGASSIST_HTML_INPUT_RANGE);
      while( token != NULL ){
        String optVal(token);
        optVal.replace("'","");
        optVal.trim();
        if(i==0){
          ret.replace("{min}", optVal);
        }else if(i==1){
          ret.replace("{max}", optVal);
        }else if(i==2){          
          if(optVal=="") optVal="1";
          ret.replace("{step}", optVal);
        }
        token = strtok( NULL, seps );
        i++;
      }
      ret.replace("{val}",defVal);
      return ret;
    }

    //Render options list
    String getOptionsListHtml(String defVal, String attribs, bool isDataList=false){
      String ret="";
      char *token = NULL;
      char seps[] = ",";
      //Look for line feeds as seperators
      //LOG_DBG("defVal: %s attribs: %s \n",defVal.c_str(), attribs.c_str());
      if(attribs.indexOf("\n")>=0){          
          seps[0] = '\n';
      }      
      token = strtok(&attribs[0], seps);
      while( token != NULL ){
        String opt;
        if(isDataList)
          opt = String(CONFIGASSIST_HTML_SELECT_DATALIST_OPTION);
        else
          opt = String(CONFIGASSIST_HTML_SELECT_OPTION);
        String optVal(token);
        if(optVal=="") continue;
        optVal.replace("'","");
        optVal.trim();
        opt.replace("{optVal}", optVal);
        if(optVal == defVal){
          opt.replace("{sel}", " selected");
        }else{
          opt.replace("{sel}", "");
        }
        ret += opt;        
        token = strtok( NULL, seps );
      }
      return ret;
    }

    // Get location of given key to retrieve other elements
    int getKeyPos(String thisKey) {
      if (_configs.empty()) return -1;
      auto lower = std::lower_bound(_configs.begin(), _configs.end(), thisKey, [](
          const confPairs &a, const String &b) { 
          return a.name < b;}
      );
      int keyPos = std::distance(_configs.begin(), lower); 
      if (thisKey == _configs[keyPos].name) return keyPos;
      else LOG_WRN("Key %s not found\n", thisKey.c_str()); 
      return -1; // not found
    }

    // Get seperation location of given key
    int getSepKeyPos(String thisKey) {
      if (_seperators.empty()) return -1;
      auto lower = std::lower_bound(_seperators.begin(), _seperators.end(), thisKey, [](
          const confSeperators &a, const String &b) { 
          //Serial.printf("Comp conf sep: %s %s \n", a.name.c_str(), b.c_str());    
          return a.name < b;}
      );
      int keyPos = std::distance(_seperators.begin(), lower); 
      if (thisKey == _seperators[keyPos].name) return keyPos;
      //LOG_INF("Sep key %s not found\n", thisKey.c_str()); 
      return -1; // not found
    }

    // Extract a config tokens from keyValPair and load it into configs vector
    void loadVectItem(String keyValPair) {  
      if (keyValPair.length()) {
        std::string token[2];
        int i = 0;
        std::istringstream pair(keyValPair.c_str());
        while (std::getline(pair, token[i++], INI_FILE_DELIM));
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
    enum input_types { TEXT_BOX=1, TEXT_AREA=2, CHECK_BOX=3, OPTION_BOX=4, RANGE_BOX=5, COMBO_BOX=6};
    std::vector<confPairs> _configs;
    std::vector<confSeperators> _seperators;
    bool _valid;
    bool _dict;
    const char * _jStr;
    String _confFile;
};