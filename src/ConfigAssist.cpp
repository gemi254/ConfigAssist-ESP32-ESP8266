#include "configAssist.h"     // Config assist class
#include "configAssistPMem.h" // Memory static valiables (html pages)

#if defined(ESP32)
  #ifdef CA_USE_OTAUPLOAD
      #include "Update.h"
  #endif  
#endif

// Class static members
WEB_SERVER *ConfigAssist::_server = NULL;
String ConfigAssist::_jWifi="[{}]";

#ifdef CA_USE_PERSIST_CON
 Preferences ConfigAssist::_prefs;
#endif

// Standard defaults constructor
ConfigAssist::ConfigAssist() {
  _init = false;
  _jsonLoaded = false; _iniLoaded=false;  
  _dirty = false; _apEnabled=false;
  _confFile = String(CA_DEF_CONF_FILE);
  _jStr = CA_DEFAULT_DICT_JSON;
}

// Standard constructor with ini file
ConfigAssist::ConfigAssist(String ini_file) {
  _init = false;
  _jsonLoaded = false; _iniLoaded=false;  
  _dirty = false; _apEnabled=false;
  
  if (ini_file != "") _confFile = ini_file;
  else _confFile = CA_DEF_CONF_FILE; 

  _jStr = CA_DEFAULT_DICT_JSON;
}

// Standard constructor with ini file and json description
ConfigAssist::ConfigAssist(String ini_file, const char * jStr){
  _init = false;
  _jsonLoaded = false; _iniLoaded=false;  
  _dirty = false; _apEnabled=false;
  
  if (ini_file != "") _confFile = ini_file;
  else _confFile = CA_DEF_CONF_FILE; 
  
  _jStr = jStr;  
}

// delete 
ConfigAssist::~ConfigAssist() {}

// Set ini file at run time
void ConfigAssist::setIniFile(String ini_file){
  if (ini_file != "") _confFile = ini_file;
}

// Set json at run time.. Must called before _init || _jsonLoaded
void ConfigAssist::setJsonDict(const char * jStr, bool load){
  if(_jsonLoaded){
    LOG_E("Configuration already initialized.\n");
    return;
  }
  
  if(jStr==NULL) return;
  _jStr = jStr;
  if(load) loadJsonDict(_jStr);  
}

// Initialize with defaults
void ConfigAssist::init() {
  if(_init) return;
  _init = true;
  loadConfigFile(_confFile);
  //Failed to load ini file
  if(!_iniLoaded){
    _dirty = true;
    loadJsonDict(_jStr);
  }
  
  LOG_V("ConfigAssist::init done ini:%i json:%i\n",_iniLoaded, _jsonLoaded);
}

// Is configuration valid
bool ConfigAssist::valid(){ 
  if(!_init) init();
  return (_iniLoaded || _jsonLoaded);
}

bool ConfigAssist::exists(String key){ return getKeyPos(key) >= 0; }            
    
// Start an AP with a web server and render config values loaded from json dictionary
void ConfigAssist::setup(WEB_SERVER &server, bool apEnable ){
  String hostName = getHostName();      
  _server = &server;
  if(apEnable){
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(hostName.c_str(),"",1);
    LOG_I("Wifi AP SSID: %s started, use 'http://%s' to connect\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());      
    if (MDNS.begin(hostName.c_str()))  LOG_V("AP MDNS responder Started\n");  
    server.begin();
    _apEnabled = true;
  }
#ifdef CA_USE_WIFISCAN 
  startScanWifi();      
  server.on("/scan", [this] { this->handleWifiScanRequest(); } );
#endif  
  server.on("/cfg",[this] { this->handleFormRequest(this->_server);  } );
  server.on("/upl", [this] { this->sendHtmlUploadPage(); } );
  server.on("/fupl", HTTP_POST,[this](){  },  [this](){this->handleFileUpload(); });
#ifdef CA_USE_OTAUPLOAD  
  server.on("/ota", [this] { this->sendHtmlOtaUploadPage(); } );
#endif
#ifdef CA_USE_FIMRMCHECK
  if(get(CA_FIRMWURL_KEY) != "")
    server.on("/fwc", [this] { this->sendHtmlFirmwareCheckPage(); } );
#endif
  server.onNotFound([this] { this->handleNotFound(); } );
  LOG_V("ConfigAssist setup AP & handlers done.\n");      
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
  String hostName = get(CA_HOSTNAME_KEY);
  if(hostName=="") hostName = "configAssist_" + getMacID();
  else hostName.replace("{mac}", getMacID());
  return hostName;
}

// Return the value of a given key, Empty on not found
String ConfigAssist::get(String key) {
  int keyPos = getKeyPos(key);
  if (keyPos >= 0) {
    return _configs[keyPos].value;        
  }
  return String(""); 
}
 
// Update the value of key = value (string)
bool ConfigAssist::put(String key, String val, bool force) {
  int keyPos = getKeyPos(key);      
  if (keyPos >= 0) {
    // Save 0,1 on booleans
    if(_configs[keyPos].type == CHECK_BOX){
      val = String(IS_BOOL_TRUE(val));
    }
    LOG_N("Check : %s, %s\n", _configs[keyPos].value.c_str(), value.c_str());
    if(_configs[keyPos].value != val){
      LOG_D("Put %s=%s\n", key.c_str(), val.c_str()); 
      _configs[keyPos].value = val;
      _dirty = true;
    }
    return true;
  }else if(force) {
      add(key, val, force);
      _dirty = true; 
      return true;
  }else{
    LOG_E("Put failed, key: %s, val: %s\n", key.c_str(), val.c_str());
  } 
  LOG_D("Put key: %s. val: %s\n", key.c_str(), val.c_str()); 
  return false;    
}

// Update the value of key = value (int), force to add if not exists
bool ConfigAssist::put(String key, int val, bool force) {
    return  put(key, String(val), force); 
}

// Add vectors by key (name in confPairs)
void ConfigAssist::add(String key, String val, bool force){
  static int readNo = 0;
  static int forcedNo = -1;
  confPairs c;
  if(force){ // Forced variables will not be editable
    c = {key, val, "", "", forcedNo , TEXT_BOX };
    forcedNo --;
    add(c);
  }else{
    c = {key, val, "", "", readNo , TEXT_BOX };
    readNo ++;
    add(c);
  }
}

// Add unique name vectors pairs
void ConfigAssist::add(confPairs &c){
  int keyPos = getKeyPos(c.name);
  if(keyPos>=0){
    LOG_E("Add Key: %s already exists\n", c.name.c_str());
    return;
  }
  _configs.push_back({c});
  _keysNdx.push_back( {c.name, _configs.size() - 1} );
  sortKeysNdx();
  LOG_V("Add key no: %i, key: %s, val: %s\n", c.readNo, c.name.c_str(), c.value.c_str()); 
}

// Add seperator by key
void ConfigAssist::addSeperator(String key, String val){
  LOG_V("Adding sep key: %s=%s\n", key.c_str(), val.c_str()); 
  _seperators.push_back({key, val}) ;      
}

// Sort vectors by key (name in logKeysNdx)
void ConfigAssist::sortKeysNdx(){
  std::sort(_keysNdx.begin(), _keysNdx.end(), [] (
    const keysNdx &a, const keysNdx &b) {
    return a.key < b.key;}
  );
}

// Sort seperator vectors by key (name in confSeperators)
void ConfigAssist::sortSeperators(){
  std::sort(_seperators.begin(), _seperators.end(), [] (
    const confSeperators &a, const confSeperators &b) {
    return a.name < b.name;}
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
void ConfigAssist::dump(WEB_SERVER *server){
  confPairs c;
  char outBuff[256];
  int len = 0;
  strcpy(outBuff, "Dump editable keys: \n");
  if(server){
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->sendContent(String(outBuff));    
  }else{
    LOG_I("%s", outBuff);
  }
  while (getNextKeyVal(c)){
      if(c.readNo >= 0){
        len =  sprintf(outBuff, "No: %02i. key: %s, val: %s, lbl: %s, type: %i\n", c.readNo, c.name.c_str(), c.value.c_str(), c.label.c_str(), c.type );
        if(server) server->sendContent(outBuff, len);
        else LOG_I("%s", outBuff);
      }
  }

  strcpy(outBuff, "Dump read only keys: \n");
  if(server) server->sendContent("\n" + String(outBuff));    
  else LOG_I("%s", outBuff);  
  while (getNextKeyVal(c)){
      if(c.readNo < 0){
        len =  sprintf(outBuff, "No: %03i, key: %s, val: %s, lbl: %s, type: %i\n", c.readNo, c.name.c_str(), c.value.c_str(), c.label.c_str(), c.type );
        if(server) server->sendContent(outBuff, len);
        else LOG_I("%s", outBuff);
      }
  }

  strcpy(outBuff, "Dump indexes: \n");
  if(server) server->sendContent("\n" + String(outBuff));    
  else LOG_I("%s", outBuff);  
  size_t i = 0;  
  while( i < _keysNdx.size() ){
      int len =  sprintf(outBuff, "No: %02i, ndx: %i, key: %s\n", i, _keysNdx[i].ndx, _keysNdx[i].key.c_str());      
      if(server) server->sendContent(outBuff, len);
      else LOG_I("%s", outBuff);
      i++;
  }   
  
  strcpy(outBuff, "Dump sperators: \n");
  if(server) server->sendContent("\n" + String(outBuff));    
  else LOG_I("%s", outBuff);  
  i = 0;
  while( i < _seperators.size() ){
      int len =  sprintf(outBuff, "No: %02i, key: %s, val: %s\n", i, _seperators[i].name.c_str(), _seperators[i].value.c_str() );
      if(server) server->sendContent(outBuff, len);
      else LOG_I("%s", outBuff);
      i++;
  }
}

// Load json description file. On updateInfo = true update only additional pair info    
int ConfigAssist::loadJsonDict(const char *jStr, bool updateInfo) { 
  if(jStr==NULL) return -1; 
  DeserializationError error;
  const int capacity = JSON_ARRAY_SIZE(CA_MAX_PARAMS)+ CA_MAX_PARAMS*JSON_OBJECT_SIZE(8);
  DynamicJsonDocument doc(capacity);
  error = deserializeJson(doc, jStr);
  
  if (error) { 
    LOG_E("Deserialize Json failed: %s\n", error.c_str());
    _jsonLoaded = false;   
    return -1;    
  }
  LOG_V("Load json dict\n");
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
      if (obj.containsKey("options")){        //Options list
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
      }else if (obj.containsKey("range")){   //Input range
        String d = obj["default"];
        String r = obj["range"];
        c.value = d;
        c.attribs = r;
        c.type = RANGE_BOX;
      }else if (obj.containsKey("file")){    //Text area
        String f = obj["file"];
        c.value = f;            
        if(obj.containsKey("default")){
          String d = obj["default"];
          c.attribs = d;
        }
        c.type = TEXT_AREA;
      }else if (obj.containsKey("checked")){  //Check box
        String ck = obj["checked"];
        c.value = ck;
        c.type = CHECK_BOX;
      }else if (obj.containsKey("default")){  //Edit box
        String d = obj["default"];
        c.value = d;
        if(obj.containsKey("attribs")){
          String a = obj["attribs"];
          c.attribs = a;
        } 
        c.type = TEXT_BOX;
      }else{
        LOG_E("Undefined value on param no: %i\n", i);
      }  

      if (c.type) { //Valid
        if(c.name==CA_HOSTNAME_KEY) c.value.replace("{mac}", getMacID());
#ifdef CA_USE_PERSIST_CON      
        String no;
        if(endsWith(c.name,CA_SSID_KEY, no)){ 
          loadPref(c.name, c.value); 
        }else if(endsWith(c.name,CA_PASSWD_KEY, no)){
          loadPref(c.name, c.value);
        }
#endif         
        if(updateInfo){
          int keyPos = getKeyPos(c.name);
          //Add a Json key if not exists in ini file
          if (keyPos < 0) {
            LOG_V("Json key: %s not found in ini file.\n", c.name.c_str());
            add(c.name, c.value);
            keyPos = getKeyPos(c.name);
          }           
          if (keyPos >= 0) { //Update all other fields but not value,key        
            _configs[keyPos].readNo = i;
            _configs[keyPos].label = c.label;
            _configs[keyPos].type = c.type;
            _configs[keyPos].attribs = c.attribs;
            LOG_D("Json upd pos: %i, key: %s, type:%i, read: %i\n", keyPos, c.name.c_str(), c.type, i);
          }else{
            LOG_E("Undefined json Key: %s\n", c.name.c_str());
          }
        }else{
          LOG_D("Json add: %s, val %s, read: %i\n", c.name.c_str(), c.value.c_str(), i);
          add(c);
        }
        i++;
      }

    }else if (obj.containsKey("seperator")){
      String sepNo = "sep_" + String(i);
      String val =  obj["seperator"];
      addSeperator(sepNo, val);          
    }else{
      LOG_E("Undefined key name/default on param : %i.", i);
    }
  }
  //Sort seperators vectors for binarry search
  sortSeperators();
  _jsonLoaded = true;
  LOG_D("Loaded json dict, keys: %i\n", i);
  return i;
}
     
// Load config pairs from an ini file
bool ConfigAssist::loadConfigFile(String filename) {
  if(filename=="") filename = _confFile;
  LOG_D("Loading file: %s\n",filename.c_str());
  File file = STORAGE.open(filename, "r");
  if (!file){
    LOG_E("File: %s not exists!\n", filename.c_str());
    _iniLoaded = false;       
    _iniLoaded = false;       
    return false;  
    _iniLoaded = false;
    return false;  
  }else if(!file.size()) {
    LOG_E("Failed to load: %s, size: %u\n", filename.c_str(), file.size());
    //if (!file.size()) STORAGE.remove(CONFIG_FILE_PATH); 
    _iniLoaded = false;    
  }

  _configs.reserve(CA_MAX_PARAMS);
  if(file && file.size()){
    // extract each config line from file
    while (file.available()) {
      String configLineStr = file.readStringUntil('\n');
      LOG_D("Load: %s\n" , configLineStr.c_str());          
      loadVectItem(configLineStr);
    } 
    LOG_I("Loaded config: %s, keyCnt: %i\n",filename.c_str(), _configs.size());
    _iniLoaded = true;
    file.close();
  }

  return _iniLoaded;
}

// Delete configuration file
bool ConfigAssist::deleteConfig(String filename){
  if(filename=="") filename = _confFile;
  LOG_I("Removing config: %s\n",filename.c_str());
  return STORAGE.remove(filename);
}

// Save configs vectors in an ini file
bool ConfigAssist::saveConfigFile(String filename) {
  if(filename=="") filename = _confFile;
  if(!_dirty){
    LOG_V("Alread saved: %s\n", filename.c_str() );
    return true;
  } 

  File file = STORAGE.open(filename, "w+");
  if (!file){
    LOG_E("Failed to save config file\n");
    _dirty = true;
  }

  //Save config file with updated content
  size_t szOut=0;
  for (auto& row: _configs) {
    if(row.name==CA_HOSTNAME_KEY){
      row.value.replace("{mac}", getMacID());
    }

#ifdef CA_USE_PERSIST_CON        
    String no;
    if(endsWith(row.name, CA_SSID_KEY, no)){
      savePref(row.name, row.value);
      row.value = "";
    }else if(endsWith(row.name, CA_PASSWD_KEY, no)){
      savePref(row.name, row.value);
      row.value = "";
    }
#endif 

    if(file){
      char configLine[512];
      sprintf(configLine, "%s%c%s\n", row.name.c_str(), CA_INI_FILE_DELIM, row.value.c_str());   
      szOut+=file.write((uint8_t*)configLine, strlen(configLine));
      LOG_D("Saved: %s = %s, type: %i\n", row.name.c_str(), row.value.c_str(), row.type);
    }
  }
  
  if(szOut>0){
    file.close();
    LOG_I("File saved: %s, keys: %i, sz: %u B\n", filename.c_str(), _configs.size(), szOut);  
    _dirty = false;
  }
  
  if(file) file.close();
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
  LOG_D("Remote utc: %d, local: %llu\n", timeUtc, tvLocal.tv_sec);
  LOG_D("Time diff: %ld\n",diff);
  if( abs(diff) > 5L ){ //5 Secs
    LOG_D("LocalTime: %s\n", getLocalTime().c_str());          
    struct timeval tvRemote;
    tvRemote.tv_usec = 0;
    tvRemote.tv_sec = timeUtc;
    settimeofday(&tvRemote, NULL);
    String tmz="";
    if(exists(CA_TIMEZONE_KEY)) tmz = get(CA_TIMEZONE_KEY);
    if(tmz==""){
      String tmz = "GMT";
      if(timeOffs >= 0) tmz += "+" + String(timeOffs);
      else tmz += String(timeOffs);
    } 
    setenv("TZ", tmz.c_str(), 1);
    tzset();
    LOG_I("LocalTime after sync: %s, tmz=%s\n", getLocalTime().c_str(), tmz.c_str());
    
  }      
}

#ifdef CA_USE_WIFISCAN
// Respond a HTTP request for /scan results
void ConfigAssist::handleWifiScanRequest(){
  checkScanRes();   _server->sendContent(_jWifi); 
}
#endif

// Test Station connections
String ConfigAssist::testWiFiSTConnection(String no){
  String msg = "";
  String ssid = get("st_ssid"+no);
  String pass = get("st_pass"+no);
  if(ssid != "" && WiFi.status() != WL_CONNECTED){
    //Connect to Wifi station with ssid from conf file
    uint32_t startAttemptTime = millis();
    if(WiFi.getMode()!=WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
    LOG_D("Wifi Station testing, no: %s, ssid: %s\n", (no != "") ? no.c_str():"0", ssid.c_str());
    LOG_N("Wifi Station testing, no: %s, ssid: %s, pass: %s\n", (no != "") ? no.c_str():"0", ssid.c_str(), pass.c_str());
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000)  {
      Serial.print(".");
      delay(500);
      Serial.flush();
    }  
    Serial.println();    
  }
  msg = "{";
  if(WiFi.status() != WL_CONNECTED){
    msg += "\"status\": \"Error\", ";
    msg += "\"ssid\": \"" + ssid + "\", ";
    msg += "\"code\": \"" + String(WiFi.status()) + "\"";
  }else if(ssid != WiFi.SSID()){ //Connected and test other ssid
    msg += "\"status\": \"Failed\", ";
    msg += "\"ssid\": \"" + ssid + "\", ";
    msg += "\"code\": \"" + String("Already in a ST connection") + "\"";  
  }else{
    msg += "\"status\": \"Success\", ";
    msg += "\"ip\": \"" + WiFi.localIP().toString()+ "\", ";
    msg += "\"ssid\": \"" + WiFi.SSID() + "\", ";
    msg += "\"rssi\": \"" + String(WiFi.RSSI()) + " db\"";
  }
  
  msg += "}"; 
  LOG_D("Wifi Station testing: %s\n", msg.c_str() );
  return msg;
}

// Download a file in browser window
void ConfigAssist::handleDownloadFile(String fileName){
  File f = STORAGE.open(fileName.c_str(), "r");
  if (!f) {
    f.close();
    const char* resp_str = "File does not exist or cannot be opened";
    LOG_E("%s: %s", resp_str, fileName.c_str());
    _server->send(200, "text/html", resp_str);
    _server->client().flush(); 
    return;  
  }
  uint32_t config_len = f.size();
  // download file as attachment, required file name in inFileName
  LOG_I("Download file: %s, size: %zu B\n", fileName.c_str(), f.size());
  _server->sendHeader("Content-Type", "text/text");
  _server->setContentLength(config_len);
  int n = fileName.lastIndexOf( '/' );
  //String downloadName = getHostName() + "_" + fileName.substring( n + 1 );
  String downloadName = fileName.substring( n + 1 );
  _server->sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
  _server->sendHeader("Content-Length", String(f.size()));
  _server->sendHeader("Connection", "close");
  size_t sz = _server->streamFile(f, "application/octet-stream");
  if (sz != f.size()) {
    LOG_E("File: %s, Sent %zu, expected: %zu!\n", fileName.c_str(), sz, f.size()); 
  } 
  f.close();
}

// Respond a not found HTTP request
void ConfigAssist::handleNotFound(){
  _server->send ( 200, "text/html", "<meta http-equiv=\"refresh\" content=\"0;url=/cfg\">");
}

// Send html upload page to client
void ConfigAssist::sendHtmlUploadPage(){
  String out(CONFIGASSIST_HTML_START);
  out.replace("{title}", "Upload to spiffs");
  out += CONFIGASSIST_HTML_UPLOAD;
  _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  //out.replace("{host_name}", getHostName());
  _server->sendContent(out);
}
#ifdef CA_USE_OTAUPLOAD

// Send html OTA upload page to client
void ConfigAssist::sendHtmlOtaUploadPage(){
  String out(CONFIGASSIST_HTML_START);
  out.replace("{title}", "Upload a new firmware");
  out += CONFIGASSIST_HTML_OTAUPLOAD;
  _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server->sendContent(out);
}
#endif //CA_USE_OTAUPLOAD

#ifdef CA_USE_FIMRMCHECK
// Send html OTA upload page to client
void ConfigAssist::sendHtmlFirmwareCheckPage(){
  String out(CONFIGASSIST_HTML_START);
  out.replace("{title}", "Check firmware");  
  _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server->sendContent(out);
  out = "";
  out += CONFIGASSIST_HTML_FIRMW_CHECK;
  out.replace("{FIRMWARE_VERSION}", get(CA_FIRMWVER_KEY));
  out.replace("{FIRMWARE_URL}", get(CA_FIRMWURL_KEY));  
  _server->sendContent(out);
}
#endif //CA_USE_FIMRMCHECK

// Respond a HTTP request for upload a file
void ConfigAssist::handleFileUpload(){
  static File tmpFile;
  static String filename,tmpFilename;
  HTTPUpload& uploadfile = _server->upload(); 
  bool isOta = _server->hasArg("ota");  
  if(uploadfile.status == UPLOAD_FILE_START){
#ifdef CA_USE_OTAUPLOAD
    if(isOta){
      uint32_t otaSize = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      LOG_I("Firmware update initiated: %s\r\n", uploadfile.filename.c_str());
      if (!Update.begin(otaSize)) { //start with max available size
				LOG_E("OTA Error: %x\n", Update.getError());
        Update.printError(Serial);
			}
      return;
    }
#endif //CA_USE_OTAUPLOAD
    filename = uploadfile.filename;
    if(filename.length()==0) return;
    if(!filename.startsWith("/")) filename = "/"+filename;
    tmpFilename = filename + ".tmp";
    LOG_I("Upload started, name: %s\n", tmpFilename.c_str());
    //Remove the previous tmp version
    if(STORAGE.exists(tmpFilename)) STORAGE.remove(tmpFilename);                         
    tmpFile = STORAGE.open(tmpFilename, "w+");    
  }else if (uploadfile.status == UPLOAD_FILE_WRITE){
#ifdef CA_USE_OTAUPLOAD
    if(isOta){ // flashing firmware to ESP
			if (Update.write(uploadfile.buf, uploadfile.currentSize) != uploadfile.currentSize) {
				LOG_E("OTA Error: %x\n", Update.getError());
        Update.printError(Serial);        
			}
			// Store the next milestone to output
			uint16_t chunk_size  = 51200;
			static uint32_t next = 51200;

			// Check if we need to output a milestone (100k 200k 300k)
			if (uploadfile.totalSize >= next) {
				Serial.printf("%dk ", next / 1024);
				next += chunk_size;
			}
      return;
    }
#endif //CA_USE_OTAUPLOAD
    //Write the received bytes to the file
    if(tmpFile) tmpFile.write(uploadfile.buf, uploadfile.currentSize);
  }else if (uploadfile.status == UPLOAD_FILE_END){
    String msg = "";
    String out(CONFIGASSIST_HTML_START);
    out += CONFIGASSIST_HTML_MESSAGE;
    out.replace("{url}", "/cfg");
    out.replace("{refresh}", "9000");
#ifdef CA_USE_OTAUPLOAD
    if(isOta){ // flashing firmware to ESP
      if (Update.end(true)) { //true to set the size to the current progress
        msg = "Firmware update successful file: <font style='color: blue;'>" + uploadfile.filename + "</font>, size: " + uploadfile.totalSize + " B";
        LOG_I("\n%s\n", msg.c_str());
        out.replace("{reboot}", "true");
        msg += "<br>Device now will reboot..";
			} else {
				msg = "Firmware update ERROR, file: "+ uploadfile.filename + ", size: " + uploadfile.totalSize + " B" + ", error:" + Update.getError();
        LOG_E("\n%s\n", msg.c_str());
        Update.printError(Serial);
        out.replace("{reboot}", "false");
			}
      out.replace("{title}", "Firmware upgrade");
      out.replace("{msg}", msg.c_str());
      this->_server->send(200,"text/html",out);
      return;
    }
#endif //CA_USE_OTAUPLOAD
    if(tmpFile){ // If the file was successfully created    
      tmpFile.close();  
      if(STORAGE.exists(filename)) STORAGE.remove(filename);
      //Uploaded a temp file, rename it on success
      LOG_D("Rename temp name: %s to: %s\n", tmpFilename.c_str(), filename.c_str() );
      STORAGE.rename(tmpFilename, filename);
      msg = "Upload success, file: "+ uploadfile.filename + ", size: " + uploadfile.totalSize + " B";
      LOG_I("%s\n", msg.c_str());
      out.replace("{title}", "Restore config");
      out.replace("{reboot}", "true");
      msg += "<br>Device now will reboot..";
      out.replace("{msg}", msg.c_str());
      this->_server->send(200,"text/html",out);
      delay(500);
    }else{
      msg = "Upload Failed!, file: "+ uploadfile.filename + ", size: " + uploadfile.totalSize + ", status: " + uploadfile.status;
      LOG_E("%s\n", msg.c_str());
      out.replace("{title}", "Restore config");
      out.replace("{reboot}", "false");
      out.replace("{msg}", msg.c_str());
      this->_server->send(200,"text/html",out);      
    }
  }
}

// Respond a HTTP request for the form use the CONF_FILE
// to save. Save, Reboot ESP, Reset to defaults, cancel edits
void ConfigAssist::handleFormRequest(WEB_SERVER * server){
  if(server == NULL ) server = _server;
  String reply = "";

  //Save config form
  if (server->args() > 0) {
    //Download a file 
    if (server->hasArg(F("_DWN"))) {
      if(server->hasArg(F("_F"))){
        LOG_I("Download file:%s\n",server->arg(F("_F")).c_str());      
        handleDownloadFile(server->arg(F("_F")));
      }else{ //Download default config
        LOG_I("Download def:%s\n",_confFile.c_str());      
        handleDownloadFile(_confFile);
      }
      return;
    }
    if( server->hasArg(F("_UPL")) || server->hasArg(F("_UPG")) || server->hasArg(F("_FWC")) ){
      return;
    }
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);        
    //Discard edits and load json defaults;
    if (server->hasArg(F("_RST"))) {
      deleteConfig();
      _configs.clear();
      loadJsonDict(_jStr);
      saveConfigFile();
      if(_dirty){
        server->send(200, "text/html", "Failed to save config.");
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
#ifdef CA_USE_PERSIST_CON
    //Clear preferences
    if (server->hasArg(F("_CLEAR"))) {
        if(clearPrefs()) reply = "Cleared preferences.";
        else reply = "ERROR: Failed to clear preferences.";
        server->send(200, "text/html", reply); 
        server->client().flush();
        return;
    } 
#endif //CA_USE_PERSIST_CON   
    //Test wifi?    
    if (server->hasArg(F("_TEST_WIFI"))) {
      LOG_D("Testing WIFI ST connection..\n");
      String no = server->arg(F("_TEST_WIFI"));
      String msg = testWiFiSTConnection(no);
      server->send(200, "text/json", msg.c_str());
      server->client().flush(); 
      LOG_D("Testing WIFI ST connection..Done\n");
      if(_apEnabled) WiFi.disconnect();
      return;
    }

    //Reboot esp?    
    if (server->hasArg(F("_RBT_CONFIRM"))) {
      LOG_D("Restarting..\n");
      server->send(200, "text/html", "OK");
      server->client().flush(); 
      delay(1000);
      ESP.restart();
      return;
    }
    
    //Update configs from form post vals
    for(uint8_t i=0; i<server->args(); ++i ){
      String key(server->argName(i));
      String val(server->arg(i));
      key = urlDecode(key);
      val = urlDecode(val);
      if(key=="apName" || key =="_SAVE" || key=="_RST" || key=="_RBT" || key=="_UPG" || key=="_FWC" || key=="plain" || key=="_TS") continue;
      //Ignore text box save filenames
      if(key.endsWith(CA_FILENAME_IDENTIFIER)) continue;
      if(key=="clockOffs" && server->hasArg("clockUTC")) continue;
      if(key=="clockUTC"){
          // Synchronize to browser clock if out of sync
          String offs("0");
          if(server->hasArg("clockOffs")){
            offs = String(server->arg("clockOffs"));
          }
          checkTime(val.toInt(), offs.toInt());
          server->send(200, "text/html", "OK");
          server->client().flush(); 
          return;
      }
      //Check if it is a text box with file name
      String fileNameKey = server->argName(i) + CA_FILENAME_IDENTIFIER;
      if(server->hasArg(fileNameKey)){
          String fileName = server->arg(fileNameKey);
          saveText(fileName, val);
          val = fileName;
      }
      LOG_D("Form upd: %s = %s\n", key.c_str(), val.c_str());
      if(!put(key, val)) reply = "ERROR: " + key;
      else reply = "OK";
    }
    
    //Reboot esp
    if (server->hasArg(F("_RBT"))) {
        saveConfigFile();
        String out(CONFIGASSIST_HTML_START);
        out += CONFIGASSIST_HTML_MESSAGE;
        String timestamp = server->arg("_TS");
        bool reboot = true;
        if(timestamp != ""){
          struct timeval tvLocal;
          gettimeofday(&tvLocal, NULL);        
          //Is time in sync?
          if (tvLocal.tv_sec > 10000){            
            long timeUtc = timestamp.toInt();
            long diff = tvLocal.tv_sec - timeUtc;
            if( diff > 5L ) reboot = false;
            LOG_D("Check device reboot: %i, timeDif: %lu\n", reboot, diff );
          }else{
            LOG_D("Check device reboot: Time is not synchronized.\n");
          }          
        }
        if(!reboot){
          out.replace("{reboot}", "false");
          out.replace("{msg}", "Already restarted, Redirecting to home");
          out.replace("{refresh}", "3000");
        }else{
          out.replace("{reboot}", "true");          
          out.replace("{msg}", "Configuration saved.<br>Device will be restarted in a few seconds");
          out.replace("{refresh}", "10000");
        }
        out.replace("{title}", "Reboot device");
        out.replace("{url}", "/cfg");
        out.replace("{refresh}", "10000");
        server->send(200, "text/html", out);
        server->client().flush(); 
        return;
    }        
    //Save config file 
    if (server->hasArg(F("_SAVE"))) {
        if(saveConfigFile()) reply = "Config saved.";
        else reply = "ERROR: Failed to save config.";
        server->send(200, "text/html", reply); 
        server->client().flush();
        return;
    }  
  }
  //Generate html page and send it to client
  if(server->args() < 1) sendHtmlEditPage(server);
  else server->send(200, "text/html", reply); 
}
    
// Send edit html to client
void ConfigAssist::sendHtmlEditPage(WEB_SERVER * server){
  //Load dictionary if no pressent and update info only
  if(!_jsonLoaded) loadJsonDict(_jStr, true);
  LOG_D("Generate form, iniValid: %i, jsonLoaded: %i, dirty: %i\n", _iniLoaded, _jsonLoaded, _dirty);      
  //Send config form data
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  String out(CONFIGASSIST_HTML_START);
  out.replace("{title}", "Configuration for " + getHostName());
  server->sendContent(out);
  server->sendContent(CONFIGASSIST_HTML_CSS);
  server->sendContent(CONFIGASSIST_HTML_CSS_CTRLS);      
  String script(CONFIGASSIST_HTML_SCRIPT);
  script.replace("{CA_FILENAME_IDENTIFIER}",CA_FILENAME_IDENTIFIER);
  script.replace("{CA_PASSWD_KEY}",CA_PASSWD_KEY);
  
#ifdef CA_USE_TESTWIFI 
    out = String("<script>") + CONFIGASSIST_HTML_SCRIPT_TEST_ST_CONNECTION + String("</script>");
    server->sendContent(out);
#endif

  String subScript = "";
#ifdef CA_USE_TIMESYNC 
  subScript += CONFIGASSIST_HTML_SCRIPT_TIME_SYNC;  
#endif
#ifdef CA_USE_WIFISCAN 
  subScript += CONFIGASSIST_HTML_SCRIPT_WIFI_SCAN;
#endif  
  //if(CA_USE_TESTWIFI) subScript += CONFIGASSIST_HTML_SCRIPT_TEST_ST_CONNECTION;

  script.replace("/*{SUB_SCRIPT}*/", subScript);
  server->sendContent(script);
  out = String(CONFIGASSIST_HTML_BODY);
  out.replace("{host_name}", getHostName());
  server->sendContent(out);
  //Render html keys
  //sortReadOrder();      
  while(getEditHtmlChunk(out)){
    server->sendContent(out);        
  }
  //sort();
  out = String(CONFIGASSIST_HTML_END);
  
#ifdef CA_USE_OTAUPLOAD   
  out.replace("<!--extraButtons-->", String(HTML_UPGRADE_BUTTON) +"\n<!--extraButtons-->");
#endif
#ifdef CA_USE_FIMRMCHECK
  if(get(CA_FIRMWURL_KEY) != "")
    out.replace("<!--extraButtons-->", String(HTML_FIRMWCHECK_BUTTON) +"\n<!--extraButtons-->");
#endif
  out.replace("{appVer}", CA_CLASS_VERSION);
  server->sendContent(out);
}
    
//Get edit page html table (no form)
String ConfigAssist::getEditHtml(){
  String ret = "";
  String out = "";
  while(getEditHtmlChunk(out)){
    ret += out;
  }
  return ret;
}

// Get page css
String ConfigAssist::getCSS(){
  return String(CONFIGASSIST_HTML_CSS);
}
#ifdef CA_USE_TIMESYNC

// Get browser time synchronization java script
String ConfigAssist::getTimeSyncScript(){
  return String(CONFIGASSIST_HTML_SCRIPT_TIME_SYNC);
}
#endif //CA_USE_TIMESYNC 

// Get html custom message page
String ConfigAssist::getMessageHtml(){
  return String(CONFIGASSIST_HTML_START) + String(CONFIGASSIST_HTML_MESSAGE);
}

// Is string numeric
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
      LOG_N("%c\n", s.charAt(i));
      return false;
    }
  }
  return true;
}

// name ends with key + number?
bool ConfigAssist::endsWith(String name, String key, String &no) {
  int l = name.length();
  while( --l >= 0 )
    if( !isDigit(name.charAt(l)) ) break;
  
  String sKey = name.substring(0, l + 1);
  no = name.substring(l + 1, name.length());

  if(sKey.endsWith(key)){
    return true;
  }
  LOG_N("Not endsWith name: %s, key: %s, sKey: %s no: %s\n", name.c_str(), key.c_str(), sKey.c_str(), no.c_str());
  return false;  
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
    LOG_E("Failed to load: %s, sz: %u B\n", fPath.c_str(), file.size());
    return false;
  }
  //Load text from file
  txt = "";
  while (file.available()) {
    txt += file.readString();
  } 
  LOG_D("Loaded: %s, sz: %u B\n" , fPath.c_str(), txt.length() );          
  file.close();
  return true;
}

#ifdef CA_USE_PERSIST_CON
// Clear nvs
bool ConfigAssist::clearPrefs(){
  if (!_prefs.begin(CA_PREFERENCES_NS, false)) {  
    LOG_E("Failed to begin prefs\n");
    return false;
  }
  _prefs.clear();
  _prefs.end();
  LOG_I("Cleared prefs.\n");
  return true;
}

// Save a key to nvs
bool ConfigAssist::savePref(String key, String val){
  if (!_prefs.begin(CA_PREFERENCES_NS, false)) {  
    LOG_E("Failed to begin prefs\n");
    return false;
  }
  _prefs.putString(key.c_str(), val.c_str());
  _prefs.end();
  LOG_D("Saved prefs key: %s = %s\n", key.c_str(), val.c_str());
  return true;
}

// Load a key from nvs
bool ConfigAssist::loadPref(String key, String &val){
  if (!_prefs.begin(CA_PREFERENCES_NS, false)) {  
    LOG_E("Failed to begin prefs\n");
    return false;
  }
  String pVal = _prefs.getString(key.c_str(),"");
  if(pVal != "" ){ //Pref exist
    val = pVal;
  }else if(val!=""){  //Pref not exist but val is not empty
    //Move to nvs
    LOG_I("Moving key: %s = %s to nvs\n", key.c_str(), val.c_str());
    _prefs.putString(key.c_str(), val.c_str());
  }
  LOG_V("Loaded prefs key: %s = %s\n", key.c_str(), val.c_str());
  _prefs.end();
  return true;
}
#endif    

// Write a string to a file
bool ConfigAssist::saveText(String fPath, String txt){
  STORAGE.remove(fPath);
  File file = STORAGE.open(fPath, "w+");
  if(!file){
    LOG_E("Failed to save: %s, sz: %u B\n", fPath.c_str(), txt.length());
    return false;
  } 
  file.print(txt);
  LOG_D("Saved: %s, sz: %u B\n" , fPath.c_str(), txt.length() ); 
  file.close();
  return true;
}
    
// Render keys,values to html lines
bool ConfigAssist::getEditHtmlChunk(String &out){      
  confPairs c;
  out="";
  if(!getNextKeyVal(c)) return false;
  LOG_V("getEditHtmlChunk %i, %s\n", c.readNo, c.name.c_str());
  // Values added manually not editable
  if(c.readNo<0) return true;
  out = String(CONFIGASSIST_HTML_LINE);
  String elm;
  String no;
  if(c.type == TEXT_BOX){
    elm = String(CONFIGASSIST_HTML_TEXT_BOX);    
    if(endsWith(c.name, CA_PASSWD_KEY, no)) { //Password field
      elm.replace("<input ", "<input type=\"password\" " +c.attribs);
      //Generate pass view checkbox
      String chk1(CONFIGASSIST_HTML_CHECK_VIEW_PASS);
      chk1.replace("{key}","_PASS_VIEW" + String(no));
      chk1.replace("{label}","View");
      chk1 += "";
      chk1.replace("{chk}","");
      c.label = chk1 + c.label;

      //Don't show passwords
      String ast = "";
      for(unsigned int k=0; k < c.value.length(); ++k){
        ast+='*';
      }
      c.value = ast;
    }else if(isNumeric(c.value))
      elm.replace("<input ", "<input type=\"number\" " +c.attribs);
#ifdef CA_USE_TESTWIFI 
    else if(endsWith(c.name, CA_SSID_KEY, no)) {
      out.replace("<td class=\"card-lbl\">", "<td class=\"card-lbl\" id=\"st_ssid" + no + "-lbl\">");
      c.label +="&nbsp;&nbsp;<a href=\"\" title=\"Test ST connection\" onClick=\"testWifi(" + no + "); return false;\" id=\"_TEST_WIFI" + no + "\">Test connection</a>";
    }
#endif //CA_USE_TESTWIFI 
    else 
      elm.replace("<input ", "<input " +c.attribs);
  }else if(c.type == TEXT_AREA){
    String file = String(CONFIGASSIST_HTML_TEXT_AREA_FNAME);        
    file.replace("{key}", c.name + CA_FILENAME_IDENTIFIER);
    file.replace("{val}", c.value);
    elm = String(CONFIGASSIST_HTML_TEXT_AREA);
    String txt="";
    //Replace loaded text
    if(loadText(c.value, txt)){ 
      elm.replace("{val}", txt); 
    }else{ //Text not yet saved, load default text
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
    if(sepKeyPos > 0) //Close the previous seperator
      out = String(CONFIGASSIST_HTML_SEP_CLOSE) + outSep + out;
    else 
      out = outSep + out;
    LOG_V("SEP key[%i]: %s = %s\n", sepKeyPos, sKey.c_str(), sVal.c_str());
  }else if(c.readNo==0 && _seperators.size() < 1 ){ //No start seperator found
    LOG_W("No seperator found: %s, using default.\n", sKey.c_str());
    //addSeperator("0","General settings");
    String outSep = String(CONFIGASSIST_HTML_SEP);
    outSep.replace("{val}", "General settings");
    out = outSep + out;
  }  
  LOG_V("HTML key[%i]: %s = %s, type: %i, attr: %s\n", c.readNo, c.name.c_str(), c.value.c_str(), c.type, c.attribs.c_str() );
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
  LOG_N("defVal: %s attribs: %s \n",defVal.c_str(), attribs.c_str());
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
int ConfigAssist::getKeyPos(String key) {
  if(!_init) init();
  if (_keysNdx.empty()) return -1;
  auto lower = std::lower_bound(_keysNdx.begin(), _keysNdx.end(), key, [](
      const keysNdx &a, const String &b) { 
      return a.key < b;}
  );
  int keyPos = std::distance(_keysNdx.begin(), lower); 
  //LOG_I("getKeyPos  ndx: %i\n", _keysNdx[keyPos].ndx);
  //if (key == _keysNdx[keyPos].key) return _keysNdx[keyPos].ndx;
  if (_keysNdx[keyPos].ndx < _configs.size() && key == _configs[ _keysNdx[keyPos].ndx ].name) 
    return _keysNdx[keyPos].ndx;
  else 
    LOG_V("Get pos, key %s not found.\n", key.c_str()); 
  return -1; // Not found
}

// Get seperation location of given key
int ConfigAssist::getSepKeyPos(String key) {
  if (_seperators.empty()) return -1;
  auto lower = std::lower_bound(_seperators.begin(), _seperators.end(), key, [](
      const confSeperators &a, const String &b) { 
      return a.name < b;}
  );
  int keyPos = std::distance(_seperators.begin(), lower); 
  if (key == _seperators[keyPos].name) return keyPos;
  LOG_N("Sep key %s not found.\n", key.c_str()); 
  return -1; // not found
}

// Extract a config tokens from keyValPair and load it into configs vector
void ConfigAssist::loadVectItem(String keyValPair) {  
  if (!keyValPair.length()) return;

  std::string token[2];
  int i = 0;
  std::istringstream pair(keyValPair.c_str());
  String key,val;
  while (std::getline(pair, token[i++], CA_INI_FILE_DELIM));
  if (i != 3){ 
    if (i == 2) { //Empty param
      key = token[0].c_str(); 
      val = "";
    }else{
      LOG_E("Unable to parse '%s', len %u, items: %i\n", keyValPair.c_str(), keyValPair.length(), i);
    }
  }else{
    key = token[0].c_str(); 
    val = token[1].c_str();
#if CA_DONT_ALLOW_SPACES
    val.replace(" ","");
#endif
  }
  val.replace("\r","");
  val.replace("\n","");

#ifdef CA_USE_PERSIST_CON      
  String no;
  if(endsWith(key,CA_SSID_KEY, no)){ 
    loadPref(key, val);        
  }else if(endsWith(key,CA_PASSWD_KEY, no)){
    loadPref(key, val);
  }
#endif

  //No label added for memory issues
  add(key, val, true);
  if (_configs.size() > CA_MAX_PARAMS) 
    LOG_W("Config file entries: %u exceed max: %u\n", _configs.size(), CA_MAX_PARAMS);
}

#ifdef CA_USE_WIFISCAN     
// Build json on Wifi scan complete     
void ConfigAssist::scanComplete(int networksFound) {
  LOG_I("%d network(s) found\n", networksFound);      
  if( networksFound <= 0 ) return;
  
  _jWifi = "[";
  for (int i = 0; i < networksFound; ++i){
      if(i) _jWifi += ",\n";
      _jWifi += "{";
      _jWifi += "\"rssi\":"+String(WiFi.RSSI(i));
      _jWifi += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      _jWifi += "}";
      LOG_D("%i,%s\n", WiFi.RSSI(i), WiFi.SSID(i).c_str());
    }
  _jWifi += "]";
  LOG_V("Scan complete \n");    
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
    LOG_D("Scan in progress..\n");
  }else if(n==-2){
    LOG_D("Starting async scan..\n");          
    WiFi.scanNetworks(/*async*/true,/*show_hidden*/true);
  }else{
    LOG_D("Scan complete status: %i\n", n);
  }
}  
#endif //CA_USE_WIFISCAN