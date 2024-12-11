// Helper class allowing easy connection to WiFi, set static ip using and synchronize time
// using the key values contained in the config class;
#include "ConfigAssist.h"
#if defined(ESP32)
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include <time.h>

class ConfigAssistHelper {
    public:
        enum class WiFiResult {
            SUCCESS,
            CONNECTION_TIMEOUT,
            DISCONNECTION_ERROR,
            NTP_SYNC_FAILED,
            INVALID_CREDENTIALS
        };

        enum class LEDState {
            OFF,
            CONNECTING,
            CONNECTED,
            DISCONNECTED,
            TIMEOUT
        };

        // Define callback types
        using WiFiResultCallback = std::function<void(WiFiResult, const String&)>;

        ConfigAssistHelper(ConfigAssist& conf):
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

        ~ConfigAssistHelper() {
            // Remove event handlers if any
            //_wifiDisconnectHandler.remove();
        }

        // Set time zone
        void setEnvTimeZone(const char* tz) {
            if (tz[0] == '\0') {
                LOG_E("Environment tz is not set, set default `GMT0`.\n");
                setenv("TZ", "GMT0", 1);
                return;
            }
            // Already set
            if(strcmp(getenv("TZ"), tz) == 0) return;
            LOG_D("Set environment tz: %s\n", tz);
            setenv("TZ", tz, 1);
            tzset();
        }

        // Set time zone from configAssist
        void setEnvTimeZone() {
            setEnvTimeZone(_conf(CA_TIMEZONE_KEY).c_str());
        }

        // Sync time, force to reset clock
        void syncTimeAsync(uint32_t syncTimeout = 20000, bool force = false) {
            syncTime(syncTimeout, force, true);
        }
        // Sync time, force to reset clock, async to not wait until sync
        // In asynchronous mode, if time synchronization fails, no restoration of the clock occurs.
        void syncTime(uint32_t syncTimeout = 20000, bool force = false, bool async = false) {
            if (WiFi.status() != WL_CONNECTED) {
                LOG_E("Not connected to Wi-Fi. Cannot synchronize time.\n");
                return;
            }

            String tzString = _conf(CA_TIMEZONE_KEY);
            if ( tzString == "") {
                LOG_E("Time zone key '%s' found in config!\n",CA_TIMEZONE_KEY);
                tzString = "GMT0";
            }

            setEnvTimeZone(tzString.c_str());

            String ntpServers[3] = {"", "", ""};
            confPairs c;
            int i = 0;
            // Reset
            _conf.getNextKeyVal(c,true);
            while (_conf.getNextKeyVal(c)) {
                String no;
                if (_conf.endsWith(c.name, CA_NTPSYNC_KEY, no)) {
                    ntpServers[i] = _conf(c.name);
                    if (++i >= 3) break;
                }
            }

            if(i == 0) {
                LOG_E("No Ntp servers found in config\n");
                return;
            }

            if(force){
                _timeOld = time(nullptr);
                resetClock();
            }

            LOG_V("Sync time, NTP servers: %s, %s, %s\n", ntpServers[0].c_str(), ntpServers[1].c_str(), ntpServers[2].c_str());
            configTzTime(tzString.c_str(), ntpServers[0].c_str(), ntpServers[1].c_str(), ntpServers[2].c_str());

            time_t start = millis();
            if(!async) waitForTimeSync(syncTimeout);

            // Clock reseted and wait for sync timeout
            if (force && !async && !isTimeSync()) {
                time_t elapsed = (millis() - start);
                restoreClock(elapsed);
            }

            LOG_D("Time sync ended ms: %lu\n",  (unsigned long)(millis() - start));
        }

        // Is clock in sync
        bool isTimeSync() {
            return time(nullptr) > 1000000000l;
        }

        // Wait for clock to be synced
        void waitForTimeSync(const uint32_t timeout = 20000) {
            LOG_D("Waiting for time sync..\n");
            uint32_t startAttemptTime = millis();
            while (!isTimeSync() && millis() - startAttemptTime < timeout) {
                Serial.print(".");
                delay(500);
            }
            Serial.println();

            time_t tnow = time(nullptr);
            LOG_D("Wait sync: %i, time: %s", isTimeSync(), ctime(&tnow));
        }

        // Check if ssid exists in config
        bool validateWiFiConfig() {
            //Reset
            confPairs c;
            _conf.getNextKeyVal(c, true);

            String ssid, password, ip;
            while(getStSSID(ssid, password, ip)) {
                if (!ssid.isEmpty()) break;
            }
            //Reset
            _conf.getNextKeyVal(c, true);

            return ssid != "";
        }

        // Set a wifi result callback
        void setWiFiResultCallback(WiFiResultCallback callback) {
            _resultCallback = callback;
        }

        // Set LED pin
        void setLedPin(uint8_t pin) {
            _ledPin = pin;
            if(_ledPin > 0) pinMode(_ledPin, OUTPUT);
        }

        // Set connection timeout
        void setConnectionTimeout(uint32_t timeout) {
            _connectTimeout = timeout;
        }
       // Set re connection if disconnected
        void setReconnect(bool recconect) {
            _reconnect = recconect;
        }
        // Get led state
        LEDState getLedState() { return _ledState; }

    private:
        // Reset clock to 0
        void resetClock(){
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            // Reset clock
            settimeofday(&tv, nullptr);
        }

        // Restore clock to _timeOld + elapsed ms
        void restoreClock(time_t elapsed){
            struct timeval tv;
            tv.tv_sec = _timeOld + (elapsed) / 1000;
            tv.tv_usec = 0;
            settimeofday(&tv, nullptr);
            LOG_W("Restoring old clock time.\n");
        }
        // Find wifi credentials from config.
        // Return true if found
        bool getStSSID(String& ssid, String& pass, String& ip) {
            confPairs c;
            // Finds st_ssid1, Second call st_ssid2 etc.
            while (_conf.getNextKeyVal(c)) {
                String no;
                if (_conf.endsWith(c.name, CA_ST_SSID_KEY, no)) {
                    if (c.value == "") continue;
                    ssid = c.value;
                    String key = c.name;
                    key.replace(CA_ST_SSID_KEY, CA_ST_PASSWD_KEY);
                    pass = _conf(key);
                    key.replace(CA_ST_PASSWD_KEY, CA_ST_STATICIP_KEY);
                    ip = _conf(key);
                    return true;
                }
            }
            // Reset configs
            _conf.getNextKeyVal(c,true);
            ssid = "";
            pass = "";
            return false;
        }

        // Flash led and print dot on serial to indicate connection
        void printStatus(bool end = false) {
            static uint32_t lastPrintTime = 0;
            static int col = 0;

            if (end) {
                if (col > 0) Serial.println();
                col = 0;
                lastPrintTime = 0;
                return;
            }

            updateLED();

            if (millis() - lastPrintTime >= 1000) {
                Serial.print(".");
                lastPrintTime = millis();
                if (++col >= 60) {
                    col = 0;
                    Serial.println();
                }
            }
        }

        // Flush led according to state
        void updateLED() {
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

        // Helper method for handling Wi-Fi connection
        void beginWiFiConnection(const String& ssid, const String& password, const String& ip) {
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
            if(WiFi.getMode() != WIFI_STA || WiFi.getMode() != WIFI_AP_STA)
                WiFi.mode(WIFI_AP_STA);
            WiFi.begin(ssid.c_str(), password.c_str());
        }

        // Connect to next wifi ssid
        bool connectWiFi(bool async){
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
                    // Wait connection or timeout
                    waitForConnection(_connectTimeout);
                    if(WiFi.isConnected()){
                        LOG_D("Wifi connected. ip: %s\n", WiFi.localIP().toString().c_str());
                        _ledState = LEDState::CONNECTED;
                        if (_resultCallback) _resultCallback(WiFiResult::SUCCESS, WiFi.localIP().toString());
                    }else{
                        LOG_E("Wifi connection timeout\n");
                        _ledState = LEDState::TIMEOUT;
                        if (_resultCallback) _resultCallback(WiFiResult::CONNECTION_TIMEOUT, "Timeout connecting.");
                        // Next connection if any
                        if(!connectWiFi(async))
                           _waitForResult = false;
                    }
                    updateLED();
                    return WiFi.isConnected();
                }
            }else{
                LOG_D("No more ssids to connect\n");
            }
            return false;
        }

        // Wait for a connection result
        void waitForResult(){
            if (!WiFi.isConnected() && millis() - _startAttemptTime < _connectTimeout) {
                // Optionally, add visual feedback or retries
                printStatus();
            } else { // Connect or timeout
                printStatus(true);
                if (WiFi.isConnected()) {
                if (_resultCallback) _resultCallback(WiFiResult::SUCCESS, WiFi.localIP().toString());
                _ledState = LEDState::CONNECTED;
                _waitForResult = false;
            } else { // Disconected
                if (_resultCallback) _resultCallback(WiFiResult::CONNECTION_TIMEOUT, "Timeout connecting.");
                _ledState = LEDState::TIMEOUT;
                // Next connection if any
                if(!connectWiFi(true))
                    _waitForResult = false ;
                }
            }
        }

        // Check WiFi connection status and send event
        //If using AP (Access Point) mode or AP_STA mode, setAutoReconnect()
        // might not work as intended.
        void checkConnection(){
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
                }else if(WiFi.isConnected() && _ledState != LEDState::CONNECTED) {
                    _ledState = LEDState::CONNECTED;
                    if (_resultCallback) _resultCallback(WiFiResult::SUCCESS, WiFi.localIP().toString());
                }
                LOG_V("Checking connection Wifimode: %i, ledstate: %i\n", (int)WiFi.getMode(), (int)_ledState);
                _checkConnectionTime = millis();
            }
        }

        // Wait until connection or timout. FLash led and print dots
        void waitForConnection(uint32_t connectTimeout = 10000) {
            uint32_t startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < connectTimeout) {
                printStatus();
                delay(100);
            }
            printStatus(true);
        }
    public:
        // Called from main loop to check for wifi connection
        // if _waitForResult => Wait until connection or timout
        // Check connection status
        void loop() {
            if (_waitForResult){
                // Will update led, print status
                waitForResult();
            }else{
                updateLED();
                if(_ledState == LEDState::CONNECTED ||
                   _ledState == LEDState::DISCONNECTED)
                    checkConnection();
            }
            #if defined(ESP8266)
            if(MDNS.isRunning()) MDNS.update(); // Handle MDNS
            #endif
        }

        // Connect to network, async = true will return and a connection event will be send
        bool connectToNetwork(uint32_t connectTimeout = 10000, const uint8_t ledPin = 0, const bool async = false) {
            if(WiFi.isConnected()) return true;
            if (!validateWiFiConfig()) {
                if (_resultCallback) {
                    _resultCallback(WiFiResult::INVALID_CREDENTIALS, "Invalid Wi-Fi configuration.");
                }else{
                    LOG_E("Invalid Wi-Fi configuration.\n");
                }
                return false;
            }
            setLedPin(ledPin);
            _connectTimeout = connectTimeout;
            bool bRet = connectWiFi(async);
            return bRet;
        }

        // Start wifi and call callback on Wifi Result
        void connectToNetworkAsync(uint32_t connectTimeout = 10000, const uint8_t ledPin = 0, WiFiResultCallback callback = nullptr) {
            if(callback) setWiFiResultCallback(callback);
            if(connectToNetwork(connectTimeout, ledPin, true))
                _waitForResult = true;

        }

        // Start MDNS server with CA_HOSTNAME_KEY or hostname
        bool startMDNS(){
            if(MDNS.hostname(0) != ""){
                LOG_E("MDNS already running: %s\n", MDNS.hostname(0).c_str());
                return false;
            }
            String host = _conf(CA_HOSTNAME_KEY);
            if(host == "") host = _conf.getHostName();
            // Oldest versions not include MDNS.begin(String)
            bool ret = MDNS.begin(host.c_str());
            if(ret) LOG_D("MDNS started, host: %s\n", host.c_str());
            else LOG_E("MDNS failed to start, host: %s\n", host.c_str());
            return ret;
        }
    private:
        ConfigAssist&       _conf;
        uint8_t             _ledPin;
        LEDState            _ledState;
        bool                _reconnect;
        bool                _waitForResult;
        time_t              _timeOld;
        uint32_t            _connectTimeout;
        uint32_t            _startAttemptTime;
        uint32_t            _checkConnectionTime;
        WiFiResultCallback  _resultCallback;
};



