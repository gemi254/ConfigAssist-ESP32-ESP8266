#include "ConfigAssistHelper.h"
#if defined(ESP32)
  #include <WebServer.h>
  #include <ESPmDNS.h>
#else
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
#endif

// Constructor: Initializes ConfigAssistHelper with a reference to ConfigAssist
ConfigAssistHelper::ConfigAssistHelper(ConfigAssist& conf):
    _conf(conf),
    _ledPin(0),
    _ledState(LEDState::OFF),
    _reconnect(false),
    _waitForResult(false),
    _timeOld(0),
    _connectTimeout(10000)
{
    _resultCallback = nullptr;
}
// Destructor
ConfigAssistHelper::~ConfigAssistHelper() {}

// Set the environment time zone
void ConfigAssistHelper::setEnvTimeZone(const char* tz) {
  // If the time zone is not defined, set it to "GMT0"
  if (tz[0] == '\0') {
      LOG_E("Tz undefined, set `GMT0`.\n");
      setenv("TZ", "GMT0", 1);
      return;
  }
  // Set the time zone only if it's different from the current one
  if(getenv("TZ") && strcmp(getenv("TZ"), tz) == 0) return;
  LOG_D("Set tz: %s\n", tz);
  setenv("TZ", tz, 1);
  tzset();
}

// Set the time zone using the configuration file's time zone key
void ConfigAssistHelper::setEnvTimeZone() {
  setEnvTimeZone(_conf(CA_TIMEZONE_KEY).c_str());
}

// Synchronize time asynchronously with a specified timeout
void ConfigAssistHelper::syncTimeAsync(uint32_t syncTimeout, bool force) {
  syncTime(syncTimeout, force, true);
}

// Synchronize time with optional forced sync and asynchronous option
void ConfigAssistHelper::syncTime(uint32_t syncTimeout, bool force, bool async) {
  // If not connected to Wi-Fi, return early
  if (WiFi.status() != WL_CONNECTED) {
    LOG_E("Sync time fail. Not conn\n");
    return;
  }

  // Get the time zone from the config, or default to GMT0
  String tzString = _conf(CA_TIMEZONE_KEY);
  if (tzString == "") {
    LOG_E("Time zone key '%s' not in config!\n", CA_TIMEZONE_KEY);
    tzString = "GMT0";
  }

  // Set the time zone based on the configuration or the default
  setEnvTimeZone(tzString.c_str());

  // Retrieve the NTP servers from the configuration
  String ntpServers[3] = {"", "", ""};
  confPairs c;
  int i = 0;
  _conf.getNextKeyVal(c, true);
  while (_conf.getNextKeyVal(c)) {
    String no;
    if (_conf.endsWith(c.name, CA_NTPSYNC_KEY, no)) {
      ntpServers[i] = _conf(c.name);
      if (++i >= 3) break;
  }
  }

  // If no NTP servers are found, return
  if(i == 0) {
    LOG_E("No Ntp servers in config\n");
    return;
  }

  // Reset the clock if forced sync is enabled
  if(force) {
    _timeOld = time(nullptr);
    resetClock();
  }

  // Synchronize time using the NTP servers
  LOG_V("Sync time, NTP servers: %s, %s, %s\n", ntpServers[0].c_str(), ntpServers[1].c_str(), ntpServers[2].c_str());
  configTzTime(tzString.c_str(), ntpServers[0].c_str(), ntpServers[1].c_str(), ntpServers[2].c_str());

  // Start a timer for the synchronization process
  time_t start = millis();
  if(!async) waitForTimeSync(syncTimeout);

  // If the time was forced to sync, restore the clock if necessary
  if (force && !async && !isTimeSync()) {
    time_t elapsed = (millis() - start);
    restoreClock(elapsed);
  }

  // Log the duration of the time synchronization
  LOG_D("Time sync ended ms: %lu\n", (unsigned long)(millis() - start));
}

// Check if the system time is synchronized
bool ConfigAssistHelper::isTimeSync() {
  return time(nullptr) > 1000000000l;
}

// Wait for time synchronization with a timeout
void ConfigAssistHelper::waitForTimeSync(const uint32_t timeout) {
  LOG_D("Waiting for time sync..\n");
  uint32_t startAttemptTime = millis();
  while (!isTimeSync() && millis() - startAttemptTime < timeout) {
    printStatus();
    delay(100);
  }
  printStatus(true);
  #if LOGGER_LOG_LEVEL > 3
  time_t tnow = time(nullptr);
  LOG_D("Wait sync: %i, time: %s", isTimeSync(), ctime(&tnow));
  #endif
}

// Validate the Wi-Fi configuration in the config file
bool ConfigAssistHelper::validateWiFiConfig() {
  confPairs c;
  _conf.getNextKeyVal(c, true);

  String ssid, password, ip;
  while(getStSSID(ssid, password, ip)) {
      if (!ssid.isEmpty()) break;
  }
  _conf.getNextKeyVal(c, true);

  return ssid != "";
}

// Set a callback function for handling Wi-Fi result
void ConfigAssistHelper::setWiFiResultCallback(WiFiResultCallback callback) {
  _resultCallback = callback;
}

// Set the LED pin for indicating connection status
void ConfigAssistHelper::setLedPin(uint8_t pin) {
  _ledPin = pin;
  if(_ledPin > 0) pinMode(_ledPin, OUTPUT);
}

// Set the timeout for Wi-Fi connection
void ConfigAssistHelper::setConnectionTimeout(uint32_t timeout) {
  _connectTimeout = timeout;
}

// Enable or disable automatic reconnection to Wi-Fi
void ConfigAssistHelper::setReconnect(bool reconnect) {
  _reconnect = reconnect;
}

// Get the current LED state (e.g., OFF, CONNECTING, CONNECTED)
ConfigAssistHelper::LEDState ConfigAssistHelper::getLedState() {
  return _ledState;
}

// Reset clock to 0
void ConfigAssistHelper::resetClock(){
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  // Reset clock
  settimeofday(&tv, nullptr);
}
// Restore clock to _timeOld + elapsed ms
void ConfigAssistHelper::restoreClock(time_t elapsed){
  struct timeval tv;
  tv.tv_sec = _timeOld + (elapsed) / 1000;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  LOG_W("Restoring old clock time.\n");
}
// Find wifi credentials from config.
// Return true if found
bool ConfigAssistHelper::getStSSID(String& ssid, String& pass, String& ip) {
  // Finds st_ssid1, Second call st_ssid2 etc.
  while (_conf.getNextKeyVal(_curSSID)) {
    String no;
    if (_conf.endsWith(_curSSID.name, CA_ST_SSID_KEY, no)) {
      if (_curSSID.value == "") continue;
      ssid = _curSSID.value;
      String key;
      key  = CA_ST_PASSWD_KEY + no;
      pass = _conf(key);
      key  = CA_ST_STATICIP_KEY + no;
      ip = _conf(key);
      return true;
    }
  }
  // Reset configs
  _conf.getNextKeyVal(_curSSID, true);
  ssid = "";
  pass = "";
  return false;
}

void ConfigAssistHelper::serialPrint(char ch){
  #if defined(LOGGER_LOG_LEVEL)
    #if LOGGER_LOG_LEVEL > 2
        Serial.print(ch);
    #endif
  #else
    Serial.print(ch);
  #endif
}

// Flash led and print dot on serial to indicate connection
void ConfigAssistHelper::printStatus(bool end) {
  static uint32_t lastPrintTime = 0;
  static int col = 0;

  if (end) {
    if (col > 0) serialPrint('\n');
    col = 0;
    lastPrintTime = 0;
    return;
  }

  updateLED();

  if (millis() - lastPrintTime >= 1000) {
    serialPrint('.');
    lastPrintTime = millis();
    if (++col >= 60) {
      col = 0;
      Serial.println();
    }
  }
}
// Main loop to manage Wi-Fi connection and LED state updates
void ConfigAssistHelper::loop() {
  if (_waitForResult){
    waitForResult();
  } else {
    updateLED();
    if (_ledState == LEDState::CONNECTED || _ledState == LEDState::DISCONNECTED)
      checkConnection();
  }

  #if defined(ESP8266)
  if(MDNS.isRunning()) MDNS.update(); // Handle MDNS updates for ESP8266
  #endif
}

// Flush led according to state
void ConfigAssistHelper::updateLED() {
  if (_ledPin == 0) return;
  //Used to avoid redundant operations when the LED state remains unchanged.
  //static LEDState ledState = LEDState::OFF;
  static bool pinStateOld = 0;

  uint32_t currentTime = millis();
  bool pinState = HIGH;

  switch (_ledState) {
    case LEDState::OFF:
        pinState = HIGH;
        break;
    case LEDState::CONNECTING: // off    on
        pinState = (currentTime % 1000 < 500) ? LOW : HIGH;
        break;
    case LEDState::CONNECTED:
        pinState = HIGH;
        break;
    case LEDState::DISCONNECTED:
        pinState = (currentTime % 2000 < 1000) ? LOW : HIGH;
        break;
    case LEDState::TIMEOUT:
        pinState = (currentTime % 500 < 250) ? LOW : HIGH;
        break;
  }

  if(pinState == pinStateOld) return;
  //LOG_D("Led pin:%i, state: %i, pin: %i\n", _ledPin, (int)_ledState, pinState );
  digitalWrite(_ledPin, pinState);
  pinStateOld = pinState;
}

// Wait for a connection result
// Called from loop to wait a connection
void ConfigAssistHelper::waitForResult(){
    if (!WiFi.isConnected() && millis() - _startAttemptTime < _connectTimeout) {
        // Optionally, add visual feedback or retries
        printStatus();
    } else { // Connect or timeout
        printStatus(true);
        if (WiFi.isConnected()) {
            LOG_I("Wifi connected. ip: %s\n", WiFi.localIP().toString().c_str());
            _ledState = LEDState::CONNECTED;
            _waitForResult = false;
            if (_resultCallback) _resultCallback(WiFiResult::SUCCESS, WiFi.localIP().toString());
        } else { // Disconected
            LOG_E("Conn timeout result\n");
            // Next connection if any
            if(!connectWiFi(true)){
                _waitForResult = false ;
                _ledState = LEDState::TIMEOUT;
                if (_resultCallback) _resultCallback(WiFiResult::CONNECTION_TIMEOUT, "Timeout result.");
            }
        }
    }
}
// Wait until connection or timout. FLash led and print dots
void ConfigAssistHelper::waitForConnection(uint32_t connectTimeout) {
    uint32_t startAttemptTime = millis();

    while (!WiFi.isConnected() && millis() - startAttemptTime < connectTimeout) {
        printStatus();
        delay(100);
    }
    printStatus(true);
}
// Check WiFi connection status and send event
// If using AP (Access Point) mode or AP_STA mode, setAutoReconnect()
// might not work as intended.
void ConfigAssistHelper::checkConnection()
{
  if ( millis() - _checkConnectionTime > 5000) {
    if(!WiFi.isConnected() && _ledState != LEDState::DISCONNECTED) {
      //WiFi.reconnect();
      _ledState = LEDState::DISCONNECTED;
      if (_resultCallback) _resultCallback(WiFiResult::DISCONNECTION_ERROR, "Disconnection.");
      LOG_D("Reconnecting Wifi...\n");

      #if defined(ESP8266)
      if( _reconnect && wifi_station_disconnect()) {
          wifi_station_connect();
      }
      #else
      if(_reconnect) WiFi.setAutoReconnect(true);
      #endif

      if( _conf("conn_failover").toInt()){
          // Try next connection
          connectWiFi(true);
      }

    }else if(WiFi.isConnected() && _ledState != LEDState::CONNECTED) { // Was not connected and reconn
      _ledState = LEDState::CONNECTED;
      if (_resultCallback) _resultCallback(WiFiResult::SUCCESS, WiFi.localIP().toString());
    }

    LOG_V("Checking connection Wifimode: %i, ledstate: %i\n", (int)WiFi.getMode(), (int)_ledState);
    _checkConnectionTime = millis();
  }
}
// Connect to the Wi-Fi network and manage connection timeout and LED state
bool ConfigAssistHelper::connectToNetwork(uint32_t connectTimeout, const uint8_t ledPin, const bool async) {
  if(WiFi.isConnected()) return true; // Return if already connected
  if (!validateWiFiConfig()) {
    // If Wi-Fi configuration is invalid, invoke the callback with an error
    if (_resultCallback) {
        _resultCallback(WiFiResult::INVALID_CREDENTIALS, "Invalid Wi-Fi configuration.");
    } else {
        LOG_E("Invalid Wi-Fi configuration.\n");
    }
    return false;
  }
  // Set LED pin for indicating connection status
  setLedPin(ledPin);
  _connectTimeout = connectTimeout;
  return connectWiFi(async); // Attempt to connect to Wi-Fi
}

// Connect to the network asynchronously and use the callback for results
void ConfigAssistHelper::connectToNetworkAsync(uint32_t connectTimeout, const uint8_t ledPin, WiFiResultCallback callback) {
  if(callback) setWiFiResultCallback(callback);
  if(connectToNetwork(connectTimeout, ledPin, true))
    _waitForResult = true; // Set flag to wait for connection result
}

// Helper method for handling Wi-Fi connection
void ConfigAssistHelper::beginWiFiConnection(const String& ssid, const String& password, const String& ip) {
  LOG_I("Connecting to WiFi: %s\n", ssid.c_str());
  LOG_D("Wifi pass: %s\n", password.c_str() );
  if (ip != "") {
    IPAddress ipAddr, mask, gw;
    if (_conf.getIPFromString(ip, ipAddr, mask, gw)) {
        WiFi.config(ipAddr, gw, mask);
        LOG_D("Wifi ip: %s\n", ip.c_str() );
    }
  }
  WiFi.disconnect();
  if(WiFi.getMode() == WIFI_AP) WiFi.mode(WIFI_AP_STA);
  else WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
}

// Connect to next wifi ssid
// Returns WiFi.isConnected(), on async always true
bool ConfigAssistHelper::connectWiFi(bool async){
  if(WiFi.isConnected()) return true;
  String st_ssid, st_pass, st_ip;
  // Get next not empty ssid
  if (getStSSID(st_ssid, st_pass, st_ip)) {
    // Begin wifi connection
    beginWiFiConnection(st_ssid, st_pass, st_ip);
    _startAttemptTime = millis();
    _ledState = LEDState::CONNECTING;
    if (async) {
      // Unblock loop
      _waitForResult = true;
      // Check for connection on loop
      return true;
    } else { // Block mode, wait or timeout
      // Block loop
      _waitForResult = false;
      // Wait connection or timeout
      waitForConnection(_connectTimeout);
      if(WiFi.isConnected()){
          LOG_I("Wifi conn ip: %s\n", WiFi.localIP().toString().c_str());
          _ledState = LEDState::CONNECTED;
          if (_resultCallback) _resultCallback(WiFiResult::SUCCESS, WiFi.localIP().toString());
      }else{
          LOG_E("Conn timeout async: %i\n", async);
          // Next connection if any
          if(!connectWiFi(async)){
              _ledState = LEDState::TIMEOUT;
              _waitForResult = false;
              if (_resultCallback) _resultCallback(WiFiResult::CONNECTION_TIMEOUT, "Timeout connecting.");
          }
      }
      updateLED();
      return WiFi.isConnected();
    }
  }else{
    LOG_D("No more SSIDS, failover: %s\n",  _conf("conn_failover").c_str() );
    if( _conf("conn_failover").toInt()){
        LOG_D("Starting over..\n");
        // Reset configs
        _conf.getNextKeyVal(_curSSID, true);
        // Try from the beggining
        return connectWiFi(async);
    }
  }
  return false;
}

// Start mDNS (Multicast DNS) for the device to enable local network discovery
bool ConfigAssistHelper::startMDNS() {
  if(MDNS.hostname(0) != "") {
    LOG_E("MDNS already running: %s\n", MDNS.hostname(0).c_str());
    return false;
  }

  String host = _conf(CA_HOSTNAME_KEY);
  if(host == "") host = _conf.getHostName();

  // Start mDNS with the given hostname
  bool ret = MDNS.begin(host.c_str());
  if(ret) {
    LOG_D("MDNS started, host: %s\n", host.c_str());
  } else {
    LOG_E("MDNS failed to start, host: %s\n", host.c_str());
  }
  return ret;
}
