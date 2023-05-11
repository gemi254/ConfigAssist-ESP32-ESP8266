#if !defined(_CONFIG_ASSIST_H)
#define  _CONFIG_ASSIST_H

#define CLASS_VERSION "2.6.1"        // Class version
#define MAX_PARAMS 50                // Maximum parameters to handle
#define DEF_CONF_FILE "/config.ini"  // Default Ini file to save configuration
#define INI_FILE_DELIM '~'           // Ini file pairs seperator
#define FILENAME_IDENTIFIER "_#"     // Keys ending is a hidden filename and not to be saved
#define DONT_ALLOW_SPACES false      // Allow spaces in var names ?
#define PASSWD_KEY   "_pass"         // The key part that defines a password field
#define HOSTNAME_KEY "host_name"     // The key that defines host name
#define TIMEZONE_KEY "time_zone"     // The key that defines time zone for setting time

#define USE_WIFISCAN true            // Set to false to disable wifi scan
#define USE_TIMESYNC true            // Set to false to disable sync esp with browser if out of sync

// Define Platform libs
#if defined(ESP32)
  #define WEB_SERVER WebServer
  #define STORAGE SPIFFS // one of: SPIFFS LittleFS SD_MMC 
#else
  #define WEB_SERVER ESP8266WebServer
  #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC 
#endif

#define IS_BOOL_TRUE(x) (x=="On" || x=="on" || x=="True" || x=="true" || x=="1")

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

// ConfigAssist class
class ConfigAssist{ 
  public:
    ConfigAssist();
    ConfigAssist(String ini_file);    
    ~ConfigAssist();
  public:  
    // Load configs after storage started
    // Simple ini file, on the fly no dict
    void init(String ini_file);
    // Editable Ini file, with json dict
    void init(String ini_file, const char * jStr);
    // Dictionary only, default ini file
    void initJsonDict(const char * jStr);    
    // Is config loaded valid ?
    bool valid();
    bool exists(String variable);
    // Start an AP with a web server and render config values loaded from json dictionary
    void setup(WEB_SERVER &server, bool apEnable = false);
    // Get a temponary hostname
    static String getMacID();
    // Get a temponary hostname
    String getHostName();    
    // Implement operator [] i.e. val = config['key']    
    String operator [] (String k);
    // Return the value of a given key, Empty on not found
    String get(String variable);    
    // Update the value of thisKey = value (int)
    bool put(String thisKey, int value, bool force=false);
    // Update the value of thisKey = value (string)
    bool put(String thisKey, String value, bool force=false);    
    // Add vectors by key (name in confPairs)
    void add(String key, String val);
    // Add vectors pairs
    void add(confPairs &c);
    // Add seperator by key
    void addSeperator(String key, String val);
    // Sort vectors by key (name in confPairs)
    void sort();
    // Sort seperator vectors by key (name in confSeperators)
    void sortSeperators();
    // Sort vectors by readNo in confPairs
    void sortReadOrder();    
    // Return next key and value from configs on each call in key order
    bool getNextKeyVal(confPairs &c);
    // Get the configuration in json format
    String getJsonConfig();
    // Display config items
    void dump();    
    // Load json description file. On update=true update only additional pair info    
    int loadJsonDict(String jStr, bool update=false);     
    // Load config pairs from an ini file
    bool loadConfigFile(String filename="");
    // Delete configuration file
    bool deleteConfig(String filename="");
    // Save configs vectors in an ini file
    bool saveConfigFile(String filename="");
    // Get system local time
    String getLocalTime();    
    // Compare browser with system time and correct if needed
    void checkTime(uint32_t timeUtc, int timeOffs);
    // Respond a HTTP request for /scan results
    void handleWifiScanRequest();
    // Respond a not found HTTP request
    void handleNotFound();
    // Respond a HTTP request for the form use the CONF_FILE
    // to save. Save, Reboot ESP, Reset to defaults, cancel edits
    void handleFormRequest(WEB_SERVER * server);
    // Send edit html to client
    void sendHtmlEditPage(WEB_SERVER * server);    
    // Get edit page html table (no form)
    String getEditHtml();
    // Get page css
    String getCSS();
    // Get browser time synchronization java script
    String getTimeSyncScript();
    // Get html custom message page
    String getMessageHtml();
    
  private:
    // Is string numeric
    bool isNumeric(String s);    
    // Decode given string, replace encoded characters
    String urlDecode(String inVal);
    // Load a file into a string
    bool loadText(String fPath, String &txt);    
    // Write a string to a file
    bool saveText(String fPath, String txt);
    // Render keys,values to html lines
    bool getEditHtmlChunk(String &out);
    // Render range 
    String getRangeHtml(String defVal, String attribs);
    // Render options list
    String getOptionsListHtml(String defVal, String attribs, bool isDataList=false);
    // Get location of given key to retrieve other elements
    int getKeyPos(String thisKey);
    // Get seperation location of given key
    int getSepKeyPos(String thisKey);
    // Extract a config tokens from keyValPair and load it into configs vector
    void loadVectItem(String keyValPair);    
    // Build json on Wifi scan complete     
    static void scanComplete(int networksFound);
    // Send wifi scan results to client
    static void checkScanRes();
    // Start async wifi scan
    static void startScanWifi();
  private: 
    enum input_types { TEXT_BOX=1, TEXT_AREA=2, CHECK_BOX=3, OPTION_BOX=4, RANGE_BOX=5, COMBO_BOX=6};
    std::vector<confPairs> _configs;
    std::vector<confSeperators> _seperators;
    bool _iniValid;
    bool _jsonLoaded;
    bool _dirty;
    const char * _jStr;
    String _confFile;
    static WEB_SERVER *_server;
    static String _jWifi;
};


// LOG shortcuts
#ifndef LOG_LEVEL 
  #define LOG_LEVEL '3' //Set to 0 for stop printng messages, 1 to print error only
#endif  

#ifdef ESP32
  #define DBG_FORMAT(format, type) "[%s %s @ %s:%u] " format "", esp_log_system_timestamp(), type, pathToFileName(__FILE__), __LINE__
#else
  #define DBG_FORMAT(format, type) "[%s %s %u] " format "",type, __FILE__, __LINE__ 
#endif

void logPrint(const char *level, const char *format, ...);

#define LOG_ERR(format, ...) logPrint("1", DBG_FORMAT(format,"ERR"), ##__VA_ARGS__)
#define LOG_WRN(format, ...) logPrint("2", DBG_FORMAT(format,"WRN"), ##__VA_ARGS__)
#define LOG_INF(format, ...) logPrint("3", DBG_FORMAT(format,"INF"), ##__VA_ARGS__)  
#define LOG_DBG(format, ...) logPrint("4", DBG_FORMAT(format,"DBG"), ##__VA_ARGS__)

#endif // _CONFIG_ASSIST_H