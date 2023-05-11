#include <Arduino.h>
#include <vector>
#include <regex>
#include <ArduinoJson.h>
#include <LittleFS.h>
#if defined(ESP32)
  #include <WebServer.h>
  #include "SPIFFS.h"
  #include <ESPmDNS.h>
  #include <SD_MMC.h>
#else
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include "TZ.h"
#endif
#include <FS.h>

#include "configAssistPMem.h" //Memory static valiables (html pages)
#include "configAssist.h"

WEB_SERVER *ConfigAssist::_server = NULL;
String ConfigAssist::_jWifi="[{}]";


ConfigAssist::ConfigAssist() {
  _jsonLoaded = false; _iniValid=false; _confFile = DEF_CONF_FILE; 
}

ConfigAssist::ConfigAssist(String ini_file) {  
      _jStr = NULL;  _jsonLoaded = false; _iniValid = false; _dirty = false; 
      if (ini_file != "") _confFile = ini_file;
      else _confFile = DEF_CONF_FILE; 
      init( _confFile );
    }
    
ConfigAssist::~ConfigAssist() {}

void ConfigAssist::init(String ini_file) { 
  _jStr = NULL;
  if(ini_file!="") _confFile = ini_file;
  loadConfigFile(_confFile);
  //On fail load defaults from dict  = DEF_CONF_FILE
  if(!_iniValid){
    _dirty = true;
  }      
}

// Editable Ini file, with json dict
void ConfigAssist::init(String ini_file, const char * jStr) { 
  if(jStr) _jStr = jStr;
  else _jStr = appDefConfigDict_json;

  if(ini_file!="") _confFile = ini_file;
  loadConfigFile(_confFile); 
  
  //On fail load defaults from dict
  if(!_iniValid){
    loadJsonDict(_jStr);
    _dirty = true;
  }      
}

// Dictionary only, default ini file
void ConfigAssist::initJsonDict(const char * jStr) { 
  init(_confFile,  jStr);
}
bool ConfigAssist::valid(){ return _iniValid;}
bool ConfigAssist::exists(String variable){ return getKeyPos(variable) >= 0; }            
    
// Start an AP with a web server and render config values loaded from json dictionary
void ConfigAssist::setup(WEB_SERVER &server, bool apEnable ){
  String hostName = getHostName();      
  LOG_INF("ConfigAssist setup webserver\n");
  _server = &server;
  if(apEnable){
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_INF("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());      
    if (MDNS.begin(hostName.c_str()))  LOG_INF("AP MDNS responder Started\n");  
    server.begin();
  }
  if(USE_WIFISCAN)  startScanWifi();      
  server.on("/cfg",[this] { this->handleFormRequest(this->_server);  } );
  server.on("/scan", [this] { this->handleWifiScanRequest(); } );
  server.onNotFound([this] { this->handleNotFound(); } );
  LOG_DBG("ConfigAssist setup done. %x\n", this);      
}

// Implement operator [] i.e. val = config['key']    
String ConfigAssist::operator [] (String k) { return get(k); }     
    
// Get a temponary hostname
String ConfigAssist::getMacID(){
  String mac = WiFi.macAddress();
  mac.replace(":","");
  return mac.substring(6);
}

// Get a temponary hostname
String ConfigAssist::getHostName(){
  String hostName = get(HOSTNAME_KEY);
  if(hostName=="") hostName = "ESP_ASSIST_" + getMacID();
  else hostName.replace("{mac}", getMacID());
  return hostName;
}

// Return the value of a given key, Empty on not found
String ConfigAssist::get(String variable) {
  int keyPos = getKeyPos(variable);
  if (keyPos >= 0) {
    return _configs[keyPos].value;        
  }
  return String(""); 
}
    
// Update the value of thisKey = value (int)
bool ConfigAssist::put(String thisKey, int value, bool force) {
    return  put(thisKey, String(value), force); 
} 
    
// Update the value of thisKey = value (string)
bool ConfigAssist::put(String thisKey, String value, bool force) {      
  int keyPos = getKeyPos(thisKey);      
  if (keyPos >= 0) {
    //Save 0,1 on booleans
    if(_configs[keyPos].type == CHECK_BOX){
      value = String(IS_BOOL_TRUE(value));
    }
    //LOG_DBG("Check : %s, %s\n", _configs[keyPos].value.c_str(), value.c_str());
    if(_configs[keyPos].value != value){
      LOG_DBG("Put %s=%s\n", thisKey.c_str(), value.c_str()); 
      _configs[keyPos].value = value;
      _dirty = true;
    }
    return true;
  }else if(force) {
      add(thisKey, value);
      sort();
      _dirty = true; 
      return true;
  }else{
    LOG_ERR("Put failed on Key: %s=%s\n", thisKey.c_str(), value.c_str());
  } 
  return false;    
}
    
// Add vectors by key (name in confPairs)
void ConfigAssist::add(String key, String val){      
  confPairs d = {key, val, "", "", -1 , TEXT_BOX };
  //LOG_DBG("Adding key %s=%s\n", key.c_str(), val.c_str()); 
  _configs.push_back(d);
}

// Add vectors pairs
void ConfigAssist::add(confPairs &c){
    //LOG_DBG("Adding key[%i]: %s=%s\n", readNo, key.c_str(), val.c_str()); 
  _configs.push_back({c}) ;
}

// Add seperator by key
void ConfigAssist::addSeperator(String key, String val){
    //LOG_DBG("Adding sep key: %s=%s\n", key.c_str(), val.c_str()); 
  _seperators.push_back({key, val}) ;      
}

// Sort vectors by key (name in confPairs)
void ConfigAssist::sort(){
  std::sort(_configs.begin(), _configs.end(), [] (
    const confPairs &a, const confPairs &b) {
    return a.name < b.name;}
  );
}

// Sort seperator vectors by key (name in confSeperators)
void ConfigAssist::sortSeperators(){
  std::sort(_seperators.begin(), _seperators.end(), [] (
    const confSeperators &a, const confSeperators &b) {
    return a.name < b.name;}
  );
}

// Sort vectors by readNo in confPairs
void ConfigAssist::sortReadOrder(){
  std::sort(_configs.begin(), _configs.end(), [] (
    const confPairs &a, const confPairs &b) {
    return a.readNo < b.readNo;}
  );
}

// Return next key and value from configs on each call in key order
bool ConfigAssist::getNextKeyVal(confPairs &c) {
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
String ConfigAssist::getJsonConfig(){
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
void ConfigAssist::dump(){
  confPairs c;
  while (getNextKeyVal(c)){
      LOG_INF("[%u]: %s = %s; %s; %i\n", c.readNo, c.name.c_str(), c.value.c_str(), c.label.c_str(), c.type );
  }
}
    
// Load json description file. On update=true update only additional pair info    
int ConfigAssist::loadJsonDict(String jStr, bool update) { 
  if(jStr==NULL) return -1; 
  DeserializationError error;
  const int capacity = JSON_ARRAY_SIZE(MAX_PARAMS)+ MAX_PARAMS*JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);
  error = deserializeJson(doc, jStr.c_str());
  
  if (error) { 
    LOG_ERR("Deserialize Json failed: %s\n", error.c_str());
    _jsonLoaded = false;   
    return -1;    
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
        if(obj.containsKey("attribs")){
          String a = obj["attribs"];
          c.attribs = a;
        } 
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
  _jsonLoaded = true;
  LOG_INF("Loaded json dict\n");
  return i;
}
     
// Load config pairs from an ini file
bool ConfigAssist::loadConfigFile(String filename) {
  LOG_DBG("Loading config: %s\n",filename.c_str());
  if(filename=="") filename = _confFile;
  File file = STORAGE.open(filename,"r");
  if (!file || !file.size()) {
    LOG_ERR("Failed to load: %s, sz: %u\n", filename.c_str(), file.size());
    //if (!file.size()) STORAGE.remove(CONFIG_FILE_PATH); 
    _iniValid = false;       
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
  _iniValid = true;
  return true;
}

// Delete configuration file
bool ConfigAssist::deleteConfig(String filename){
  if(filename=="") filename = _confFile;
  LOG_INF("Remove config: %s\n",filename.c_str());
  return STORAGE.remove(filename);
}

// Save configs vectors in an ini file
bool ConfigAssist::saveConfigFile(String filename) {
  if(filename=="") filename = _confFile;
  if(!_dirty){
    LOG_INF("Alread saved: %s\n", filename.c_str() );
    return true;
  } 
  File file = STORAGE.open(filename, "w+");
  if (!file){
    LOG_ERR("Failed to save config file\n");
    return false;
  }
  //Save config file with updated content
  size_t szOut=0;
  for (auto& row: _configs) {
    if(row.name==HOSTNAME_KEY){
      row.value.replace("{mac}", getMacID());
    }
    char configLine[512];
    sprintf(configLine, "%s%c%s\n", row.name.c_str(), INI_FILE_DELIM, row.value.c_str());   
    szOut+=file.write((uint8_t*)configLine, strlen(configLine));
    LOG_DBG("Saved: %s = %s, type: %i\n", row.name.c_str(), row.value.c_str(), row.type);
  }
  LOG_INF("File saved: %s, sz: %u B\n", filename.c_str(), szOut);
  file.close();
  _dirty = false;
  return true;
}
// Get system local time
String ConfigAssist::getLocalTime() {      
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_t currEpoch = tv.tv_sec;
  char timeFormat[20];
  strftime(timeFormat, sizeof(timeFormat), "%d/%m/%Y %H:%M:%S", localtime(&currEpoch));
  return String(timeFormat);
}
    
// Compare browser with system time and correct if needed
void ConfigAssist::checkTime(uint32_t timeUtc, int timeOffs){
  struct timeval tvLocal;
  gettimeofday(&tvLocal, NULL);
  
  long diff = (long)timeUtc - tvLocal.tv_sec;
  LOG_DBG("Remote utc: %lu, local: %lu\n", timeUtc, tvLocal.tv_sec);
  LOG_DBG("Time diff: %u\n",diff);      
  if( abs(diff) > 5L ){ //5 Secs
    LOG_DBG("LocalTime: %s\n", getLocalTime().c_str());          
    struct timeval tvRemote;
    tvRemote.tv_usec = 0;
    tvRemote.tv_sec = timeUtc;
    settimeofday(&tvRemote, NULL);
    String tmz;
    if(getKeyPos(TIMEZONE_KEY) >= 0) tmz = get(TIMEZONE_KEY);
    if(tmz==""){
      String tmz = "GMT";
      if(timeOffs >= 0) tmz += "+" + String(timeOffs);
      else tmz += String(timeOffs);
    } 
    setenv("TZ", tmz.c_str(), 1);
    tzset();
    LOG_INF("LocalTime after sync: %s, tmz=%s\n", getLocalTime().c_str(), tmz.c_str());
    
  }      
}

// Respond a HTTP request for /scan results
void ConfigAssist::handleWifiScanRequest(){
  checkScanRes();  _server->sendContent(_jWifi); 
}

// Respond a not found HTTP request
void ConfigAssist::handleNotFound(){
  _server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/cfg\">");
}

// Respond a HTTP request for the form use the CONF_FILE
// to save. Save, Reboot ESP, Reset to defaults, cancel edits
void ConfigAssist::handleFormRequest(WEB_SERVER * server){
  if(server == NULL ) server = _server;
  //Save config form
  if (server->args() > 0) {
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);        
    //Discard edits and load json defaults;
    if (server->hasArg(F("_RST"))) {
      deleteConfig();
      _configs.clear();
      loadJsonDict(_jStr);
      //saveConfigFile(CONF_FILE);
      LOG_INF("_iniValid: %i\n", _iniValid);
      if(!_iniValid){
        server->send(200, "text/html", "Failed to load config.");
      }else{
        server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/cfg\">");
      }
      server->client().flush(); 
      return;
    }      
    //Reload and loose changes
    if (server->hasArg(F("_CANCEL"))) {
      server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/\">");
      return;
    }

    //Reboot esp?    
    if (server->hasArg(F("_RBT_CONFIRM"))) {
      LOG_DBG("Restarting..\n");
      server->send(200, "text/html", "OK");
      server->client().flush(); 
      delay(1000);
      ESP.restart();
      return;
    }
    
    //Update configs from form post vals
    String reply = "";
    for(uint8_t i=0; i<server->args(); ++i ){
      String key(server->argName(i));
      String val(server->arg(i));
      key = urlDecode(key);
      val = urlDecode(val);
      if(key=="apName" || key =="_SAVE" || key=="_RST" || key=="_RBT" || key=="plain") continue;
      if(key.endsWith(FILENAME_IDENTIFIER)) continue;
      if(key=="clockUTC"){
          // Synchronize to browser clock if out of sync
          String offs("0");
          if(server->hasArg("offs")){
            offs = String(server->arg("offs"));
          }
          checkTime(val.toInt(), offs.toInt());
          server->send(200, "text/html", "OK");
          return;
      }
      //Check if if text box with file name
      String fileNameKey = server->argName(i) + FILENAME_IDENTIFIER;
      if(server->hasArg(fileNameKey)){
          String fileName = server->arg(fileNameKey);
          saveText(fileName, val);
          val = fileName;
      }
      LOG_DBG("Form upd: %s = %s\n", key.c_str(), val.c_str());
      if(!put(key, val)) reply = "ERROR: " + key;
    }
    
    //Reboot esp
    if (server->hasArg(F("_RBT"))) {
        saveConfigFile();
        String out(CONFIGASSIST_HTML_MESSAGE);
        out.replace("{title}", "Reboot device");
        out.replace("{url}", "/cfg");
        out.replace("{refresh}", "10000");
        out.replace("{msg}", "Configuration saved.<br>Device will restart in a few seconds");      
        server->send(200, "text/html", out);
        server->client().flush(); 
        return;
    }        
    //Save config file 
    if (server->hasArg(F("_SAVE"))) {
        if(saveConfigFile()) reply = "Config saved.";
        else reply = "ERROR: Failed to save config.";
        delay(100);
    }
    if(reply.startsWith("ERROR:"))
      server->send(500, "text/html", reply);            
    else
      server->send(200, "text/html", reply);
    return;
  }
  //Generate html page and send it to client
  sendHtmlEditPage(server);
}
    
// Send edit html to client
void ConfigAssist::sendHtmlEditPage(WEB_SERVER * server){
  LOG_DBG("Generate form, iniValid: %i, jsonLoaded: %i, dirty: %i\n", _iniValid, _jsonLoaded, _dirty);      
  //Load dictionary if no pressent
  if(!_jsonLoaded) loadJsonDict(_jStr, true);
  //Send config form data
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  String out(CONFIGASSIST_HTML_START);
  out.replace("{host_name}", getHostName());
  server->sendContent(out);
  server->sendContent(CONFIGASSIST_HTML_CSS);
  server->sendContent(CONFIGASSIST_HTML_CSS_CTRLS);      
  String script(CONFIGASSIST_HTML_SCRIPT);
  String subScript = "";
  if(USE_TIMESYNC) subScript = CONFIGASSIST_HTML_SCRIPT_TIME_SYNC;  
  if(USE_WIFISCAN) subScript += CONFIGASSIST_HTML_SCRIPT_WIFI_SCAN;
  script.replace("/*{SUB_SCRIPT}*/", subScript);
  server->sendContent(script);
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
  //LOG_DBG("Generate form end\n");
}
    
//Get edit page html table (no form)
String ConfigAssist::getEditHtml(){
  sortReadOrder();      
  String ret = "";
  String out = "";
  while(getEditHtmlChunk(out)){
    ret += out;
  }
  sort();
  return ret;
}

bool ConfigAssist::isNumeric(String s){ //1.0, -.232, .233, -32.32
  unsigned int l = s.length();
  if(l==0) return false;
  bool dec=false, sign=false;
  for(unsigned int i = 0; i < l; ++i) {
    if (s.charAt(i) == '.'){
      if(dec) return false;
      else dec = true;
    }else if(s.charAt(i) == '+' || s.charAt(i) == '-' ){
      if(sign) return false;
      else sign = true;
    }else if (!isDigit(s.charAt(i))){
      //LOG_INF("%c\n", s.charAt(i));
      return false;
    }
  }
  return true;
}
    
// Decode given string, replace encoded characters
String ConfigAssist::urlDecode(String inVal) {
  std::string decodeVal(inVal.c_str()); 
  std::string replaceVal = decodeVal;
  std::smatch match; 
  while (regex_search(decodeVal, match, std::regex("(%)([0-9A-Fa-f]{2})"))) {
    std::string s(1, static_cast<char>(std::strtoul(match.str(2).c_str(),nullptr,16))); // hex to ascii 
    replaceVal = std::regex_replace(replaceVal, std::regex(match.str(0)), s);
    decodeVal = match.suffix().str();
  }
  return String(replaceVal.c_str());      
}

// Load a file into a string
bool ConfigAssist::loadText(String fPath, String &txt){
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
bool ConfigAssist::saveText(String fPath, String txt){
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
bool ConfigAssist::getEditHtmlChunk(String &out){      
  confPairs c;
  out="";
  if(!getNextKeyVal(c)) return false;
  //Values added manually not editable
  if(c.readNo<0) return true;
  out = String(CONFIGASSIST_HTML_LINE);
  String elm;
  if(c.type == TEXT_BOX){
    elm = String(CONFIGASSIST_HTML_TEXT_BOX);
    if(c.name.indexOf(PASSWD_KEY)>=0)
      elm.replace("<input ", "<input type=\"password\" " +c.attribs);
    else if(isNumeric(c.value))
      elm.replace("<input ", "<input type=\"number\" " +c.attribs);
    else 
      elm.replace("<input ", "<input " +c.attribs);
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
    if(IS_BOOL_TRUE(c.value)){
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
String ConfigAssist::getRangeHtml(String defVal, String attribs){
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
String ConfigAssist::getOptionsListHtml(String defVal, String attribs, bool isDataList){
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
int ConfigAssist::getKeyPos(String thisKey) {
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
int ConfigAssist::getSepKeyPos(String thisKey) {
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
void ConfigAssist::loadVectItem(String keyValPair) {  
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
    
// Build json on Wifi scan complete     
void ConfigAssist::scanComplete(int networksFound) {
  LOG_INF("%d network(s) found\n", networksFound);      
  if( networksFound <= 0 ) return;
  
  _jWifi = "[";
  for (int i = 0; i < networksFound; ++i){
      if(i) _jWifi += ",\n";
      _jWifi += "{";
      _jWifi += "\"rssi\":"+String(WiFi.RSSI(i));
      _jWifi += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      _jWifi += "}";
      LOG_DBG("%i,%s\n", WiFi.RSSI(i), WiFi.SSID(i).c_str());
    }
  _jWifi += "]";
  LOG_DBG("Scan complete \n");    
}

// Send wifi scan results to client
void ConfigAssist::checkScanRes(){
  int n = WiFi.scanComplete();
  if(n>0){
    scanComplete(n);
    WiFi.scanDelete();
    startScanWifi();
  }        
}

// Start async wifi scan
void ConfigAssist::startScanWifi(){
  int n = WiFi.scanComplete();
  if(n==-1){
    LOG_DBG("Scan in progress..\n");
  }else if(n==-2){
    LOG_DBG("Starting async scan..\n");          
    WiFi.scanNetworks(/*async*/true,/*show_hidden*/true);
  }else{
    LOG_DBG("Scan complete status: %i\n", n);
  }
}  
//Is Application providing logPrint function?
#ifndef CONFIG_ASSIST_LOG_PRINT_CUSTOM  
  #define MAX_OUT 200
  static va_list arglist;
  static char fmtBuf[MAX_OUT];
  static char outBuf[MAX_OUT];
  //Logging with level to serial
  void logPrint(const char *level, const char *format, ...){
    if(level[0] > LOG_LEVEL) return;
    strncpy(fmtBuf, format, MAX_OUT);
    fmtBuf[MAX_OUT - 1] = 0;
    va_start(arglist, format); 
    vsnprintf(outBuf, MAX_OUT, fmtBuf, arglist);
    va_end(arglist);
    //size_t msgLen = strlen(outBuf);
    Serial.print(outBuf);
  }
#endif
