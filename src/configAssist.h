#if !defined(_CONFIG_ASSIST_H)
#define  _CONFIG_ASSIST_H
#define CA_CLASS_VERSION "2.6.9c"        // Class version
#define CA_MAX_PARAMS 50                 // Maximum parameters to handle
#define CA_DEF_CONF_FILE "/config.ini"   // Default Ini file to save configuration
#define CA_INI_FILE_DELIM '~'            // Ini file pairs seperator
#define CA_FILENAME_IDENTIFIER "_F"      // Keys ending with this is a text box with the filename of a text area
#define CA_DONT_ALLOW_SPACES false       // Allow spaces in var names ?
#define CA_SSID_KEY     "_ssid"          // The key part that defines a ssid field
#define CA_PASSWD_KEY   "_pass"          // The key part that defines a password field

#define CA_HOSTNAME_KEY "host_name"      // The key that defines host name
#define CA_TIMEZONE_KEY "time_zone"      // The key that defines time zone for setting time

#define CA_USE_WIFISCAN                 // Comment to disable wifi scan
#define CA_USE_TESTWIFI                 // Comment to disable test wifi st connection
#define CA_USE_TIMESYNC                 // Comment to disable sync esp with browser if out of sync
#define CA_USE_OTAUPLOAD                // Comment to disable ota and reduce memory

// Define Platform libs
#if defined(ESP32)
  #define WEB_SERVER WebServer
  #define STORAGE SPIFFS // one of: SPIFFS LittleFS SD_MMC 
#else
  #define WEB_SERVER ESP8266WebServer
  #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC 
#endif

#include "espLogger.h"

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
    // Initialize with defaults
    ConfigAssist();
    // Initialize with custom ini file, default dict
    ConfigAssist(String ini_file);
    // Initialize with custom Ini file, and custom json dict
    ConfigAssist(String ini_file, const char * jStr);
    ~ConfigAssist();
  public:  
    // Load configs after storage is started
    void init();
    // Set ini file at run time
    void setIniFile(String ini_file);
    // Set json at run time.. Must called before _init || _jsonLoaded
    void setJsonDict(const char * jStr);
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
    String get(String key);    
    // Update the value of key = val (int)
    bool put(String key, int val, bool force = false);
    // Update the value of key = val (string)
    bool put(String key, String val, bool force = false);    
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
    // Return next key and val from configs on each call in key order
    bool getNextKeyVal(confPairs &c);
    // Get the configuration in json format
    String getJsonConfig();
    // Display config items
    void dump();    
    // Load json description file. On update=true update only additional pair info    
    int loadJsonDict(String jStr, bool update=false);     
    // Load config pairs from an ini file
    bool loadConfigFile(String filename="");
    // Delete configuration files
    bool deleteConfig(String filename="");
    // Save configs vectors in an ini file
    bool saveConfigFile(String filename="");
    // Get system local time
    String getLocalTime();
    // Compare browser with system time and correct if needed
    void checkTime(uint32_t timeUtc, int timeOffs);
    // Respond a HTTP request for /scan results
    void handleWifiScanRequest();
    // Send html upload page to client
    void sendHtmlUploadPage();
#ifdef CA_USE_OTAUPLOAD
    // Send html ota upload page to client
    void sendHtmlOtaUploadPage();
#endif
    // Upload a file to SPIFFS
    void handleFileUpload();
    String testWiFiSTConnection(String no);    
    // Download a file in browser window
    void handleDownloadFile(String fileName);
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
#ifdef CA_USE_TIMESYNC 
    String getTimeSyncScript();
#endif    
    // Get html custom message page
    String getMessageHtml();
    
  private:
    // Is string numeric
    bool isNumeric(String s);
    // Name ends with key + number?
    bool endsWith(String name, String key, String& no);
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
    int getKeyPos(String key);
    // Get seperation location of given key
    int getSepKeyPos(String key);
    // Extract a config tokens from keyValPair and load it into configs vector
    void loadVectItem(String keyValPair);    
    // Build json on Wifi scan complete     
#ifdef CA_USE_WIFISCAN
    static void scanComplete(int networksFound);
    // Send wifi scan results to client
    static void checkScanRes();
    // Start async wifi scan
    static void startScanWifi();
#endif    
  private: 
    enum input_types { TEXT_BOX=1, TEXT_AREA=2, CHECK_BOX=3, OPTION_BOX=4, RANGE_BOX=5, COMBO_BOX=6};
    std::vector<confPairs> _configs;
    std::vector<confSeperators> _seperators;
    bool _init;
    bool _iniLoaded;
    bool _jsonLoaded;
    bool _dirty;
    const char * _jStr;
    String _confFile;
    static WEB_SERVER *_server;
    static String _jWifi;
    bool _apEnabled;
};

#endif // _CONFIG_ASSIST_H