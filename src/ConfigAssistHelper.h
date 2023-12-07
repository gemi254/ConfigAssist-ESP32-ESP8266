// Helper class allowing eazy connection to WiFi and set static ip using
// key values contained in a config class;
class ConfigAssistHelper 
{    
    public:
        ConfigAssistHelper(ConfigAssist &conf) { _conf = &conf; };
        ~ConfigAssistHelper() {}
    public:
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
                ledPin = (*_conf)[ledKey].toInt();
                pinMode(ledPin, OUTPUT);
            }

            while(_conf->getNextKeyVal(c)){
                String no;
                if( _conf->endsWith(c.name, CA_SSID_KEY, no ) ){
                    // Find a ssid, pass pair in config
                    String st_ssidKey = c.name;
                    String st_ssid = (*_conf)[st_ssidKey];
                    if(st_ssid == "") continue;
                    String st_passKey = st_ssidKey;
                    st_passKey.replace(CA_SSID_KEY, CA_PASSWD_KEY);
                    LOG_D("Found ssid key: %s, val: %s\n", st_ssidKey.c_str(), st_ssid.c_str());
                    String st_pass = (*_conf)[st_passKey];                    
                    LOG_D("Found pass key: %s, val: %s\n", st_passKey.c_str(), st_pass.c_str());

                    //Set static ip if defined
                    String st_ipKey = st_ssidKey;
                    st_ipKey.replace(CA_SSID_KEY, CA_STATICIP_KEY);
                    String st_ip = (*_conf)[st_ipKey];
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
            _conf->getNextKeyVal(c, true);
            // Turn off led
            if(ledPin >=0 ) digitalWrite(ledPin, 1);

            if (WiFi.status() == WL_CONNECTED)  return true;
            else return false;
        }
    private:        
        ConfigAssist *_conf;

};