// Helper class allowing easy connection to WiFi, set static ip using and synchronize time 
// using the key values contained in a config class;

class ConfigAssistHelper 
{    
    public:
        ConfigAssistHelper(ConfigAssist &conf): _conf(conf) { }
        ~ConfigAssistHelper() {}
    public:
        void setEnvTimeZone(){
            setEnvTimeZone(_conf[CA_TIMEZONE_KEY].c_str());
        }
        void setEnvTimeZone(const char *tz){
            LOG_D("Set environment tz: %s\n", tz);
            setenv("TZ", tz, 1);
            tzset();
        }
        // Setup ntp time synch
        void syncTime(uint32_t syncTimout = 20000){
            if (WiFi.status() != WL_CONNECTED)  return;
            if(_conf[CA_TIMEZONE_KEY]==""){
                LOG_E("No time zone found in config!\n");
                return;
            }

            String ntpServers[3] = {"", "", ""};
            confPairs c;
            int i = 0;
            while(_conf.getNextKeyVal(c)){
                String no;
                if( _conf.endsWith(c.name, CA_NTPSYNC_KEY, no ) ){                    
                    ntpServers[i] = _conf[c.name];
                    if(i++ > 3) break;
                }
            }

            // Reset
            _conf.getNextKeyVal(c, true);
            
            setEnvTimeZone();
    
            configTzTime(_conf[CA_TIMEZONE_KEY].c_str(), ntpServers[0].c_str(), ntpServers[1].c_str(), ntpServers[2].c_str());
            LOG_D("syncTime tz: %s, npt1: %s, ntp2:, %s ntp3: %s\n", _conf[CA_TIMEZONE_KEY].c_str(), ntpServers[0].c_str(), ntpServers[1].c_str(), ntpServers[2].c_str());
            
            // Wait until time is in sync or timeout
            waitTimeSync( syncTimout );            
        }

        // Is time sycnhronized ?
        bool isTimeSync(){ return time(nullptr) > 1000000000l; }
        
        // Wait for ntp time synchronization
        void waitTimeSync(const uint32_t timeout = 20000 ){
            LOG_I("Synchronizing time..\n");                        
            uint32_t startAttemptTime = millis();
            while (!isTimeSync() && millis() - startAttemptTime < timeout) {
                Serial.print(".");
                delay(500);
            }
            Serial.println();
            
            // Show time
            time_t tnow = time(nullptr);
            LOG_I("Synchronized : %i, time: %s", isTimeSync(), ctime(&tnow) );
        }
        
        // Set static ip from space seperated string
        bool setStaticIP(String st_ip){
            if(st_ip.length() <= 0) return false;

            IPAddress ip, mask, gw;

            int ndx = st_ip.indexOf(' ');
            String s = st_ip.substring(0, ndx);
            s.trim();
            if(!ip.fromString(s)){
                LOG_E("Error parsing static ip: %s\n",s.c_str());
                return false;
            }

            st_ip = st_ip.substring(ndx + 1, st_ip.length() );
            ndx = st_ip.indexOf(' ');
            s = st_ip.substring(0, ndx);
            s.trim();
            if(!mask.fromString(s)){
                LOG_E("Error parsing static ip mask: %s\n",s.c_str());
                return false;
            }

            st_ip = st_ip.substring(ndx + 1, st_ip.length() );
            s = st_ip;
            s.trim();
            if(!gw.fromString(s)){
                LOG_E("Error parsing static ip gw: %s\n",s.c_str());
                return false;
            }
            LOG_I("Wifi ST setting static ip: %s, mask: %s  gw: %s \n", ip.toString().c_str(), mask.toString().c_str(), gw.toString().c_str());
            WiFi.config(ip, gw, mask);
            return true;  
        }

        // Try multiple connections and connect wifi 
        bool connectToNetwork(uint32_t CONNECT_TIMEOUT = 10000, const char *ledKey = NULL){
            LOG_V("Connect with timeout: %zu\n", CONNECT_TIMEOUT);
            confPairs c;
            int ledPin = -1;
            // Setup led
            if(ledKey){
                ledPin = _conf[ledKey].toInt();
                pinMode(ledPin, OUTPUT);
            }

            while(_conf.getNextKeyVal(c)){
                String no;
                if( _conf.endsWith(c.name, CA_SSID_KEY, no ) ){
                    // Find a ssid, pass pair in config
                    String st_ssidKey = c.name;
                    String st_ssid = c.value;
                    if(st_ssid == "") continue;
                    String st_passKey = st_ssidKey;
                    st_passKey.replace(CA_SSID_KEY, CA_PASSWD_KEY);
                    LOG_V("Found ssid key: %s, val: %s\n", st_ssidKey.c_str(), st_ssid.c_str());
                    String st_pass = _conf[st_passKey];                    
                    LOG_V("Found pass key: %s, val: %s\n", st_passKey.c_str(), st_pass.c_str());

                    //Set static ip if defined
                    String st_ipKey = st_ssidKey;
                    st_ipKey.replace(CA_SSID_KEY, CA_STATICIP_KEY);
                    String st_ip = _conf[st_ipKey];
                    if(st_ip!="") setStaticIP(st_ip);
                        
                    LOG_I("Wifi ST connecting to: %s, %s \n",st_ssid.c_str(), st_pass.c_str());
                    WiFi.begin(st_ssid.c_str(), st_pass.c_str());
                    int col = 0;
                    uint32_t startAttemptTime = millis();
                    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < CONNECT_TIMEOUT) {
                        if(ledPin >=0 ) digitalWrite(ledPin, 0);
                        Serial.printf(".");
                        if (++col >= 60){
                            col = 0;
                            Serial.printf("\n");
                        }                        
                        Serial.flush();
                        delay(10);
                        if(ledPin >=0 ) digitalWrite(ledPin, 1);
                        delay(490);
                    }
                    Serial.printf("\n");
                    if (WiFi.status() != WL_CONNECTED){
                        LOG_E("Wifi connect failed.\n");
                        WiFi.disconnect();
                    }else{
                        LOG_I("Wifi AP SSID: %s connected, use 'http://%s' to connect\n", st_ssid.c_str(), WiFi.localIP().toString().c_str());
                        break;
                    }
                }                
            }
            //Close key vals
            _conf.getNextKeyVal(c, true);
            // Turn off led
            if(ledPin >=0 ) digitalWrite(ledPin, 1);

            if (WiFi.status() == WL_CONNECTED)  return true;
            else return false;
        }
    private:        
        ConfigAssist& _conf;
};