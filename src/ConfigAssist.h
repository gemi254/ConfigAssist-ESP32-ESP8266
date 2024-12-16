#if !defined(_CONFIG_ASSIST_H)
#define  _CONFIG_ASSIST_H

#include <vector>

#include "dYaml.h"
    using namespace dyml;

#if !defined(STORAGE)
  #if defined(ESP8266) || defined(CA_USE_LITTLEFS)
    #include <LittleFS.h>
    #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC
  #else
    #include "SPIFFS.h"
    #define STORAGE SPIFFS // SPIFFS
  #endif
#endif

#if defined(ESP32)
  #include <WebServer.h>
  #include <ESPmDNS.h>
#else
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
#endif

#if !defined(LOGGER_LOG_LEVEL)
  #define LOGGER_LOG_LEVEL 3             // Set log level for this module
#endif

#define CA_CLASS_VERSION "2.8.7"         // Class version
#define CA_MAX_PARAMS 50                 // Maximum parameters to handle
#define CA_DEF_CONF_FILE "/config.ini"   // Default Ini file to save configuration
#define CA_INI_FILE_DELIM '~'            // Ini file pairs seperator
#define CA_FILENAME_IDENTIFIER "_F"      // Keys ending with this is a text box with the filename of a text area
#define CA_DONT_ALLOW_SPACES false       // Allow spaces in var names ?
#define CA_ST_SSID_KEY     "st_ssid"     // Vars ending with this key or key + Num defines a ssid field
#define CA_ST_PASSWD_KEY   "st_pass"     // The key part that defines a password field
#define CA_ST_STATICIP_KEY "st_ip"       // The key part that defines a static ip field
#define CA_AP_SSID_KEY     "ap_ssid"     // Vars ending with this key or key + Num defines a ssid field
#define CA_AP_PASSWD_KEY   "ap_pass"     // The key part that defines a password field
#define CA_AP_STATICIP_KEY "ap_ip"       // The key part that defines a static ip field
#define CA_NTPSYNC_KEY     "ntp_server"  // Vars ending with this key or key + Num define a ntp server

#define CA_HOSTNAME_KEY "host_name"      // The key that defines host name
#define CA_TIMEZONE_KEY "time_zone"      // The key that defines time zone for setting time
#define CA_FIRMWVER_KEY "firmware_ver"   // The key that defines the firmware version
#define CA_FIRMWURL_KEY "firmware_url"   // The key that defines the url with firmware information
#if !defined(CA_USE_WIFISCAN)
#define CA_USE_WIFISCAN     1            // Set to 0 to disable wifi scan
#endif
#if !defined(CA_USE_TESTWIFI)
#define CA_USE_TESTWIFI     1            // Set to 0 to disable test wifi st connection
#endif
#if !defined(CA_USE_TIMESYNC)
#define CA_USE_TIMESYNC     1            // Set to 0 to disable sync esp with browser if out of sync
#endif
#if !defined(CA_USE_OTAUPLOAD)
#define CA_USE_OTAUPLOAD    1            // Set to 0 to disable ota and reduce memory
#endif
#if !defined(CA_USE_FIMRMCHECK)
#define CA_USE_FIMRMCHECK   1            // Set to 0 to disable firmware check and upgrade from url
#endif
#if !defined(CA_USE_PERSIST_CON)
#define CA_USE_PERSIST_CON  0            // Set to 1 to enable saving wifi credentials to nvs
#endif


// Define Platform libs
#if defined(ESP32)
  #define WEB_SERVER WebServer
#else
  #define WEB_SERVER ESP8266WebServer
  #define STORAGE LittleFS // one of: SPIFFS LittleFS SD_MMC
#endif

#include "espLogger.h"

#define IS_BOOL_TRUE(x) (x=="On" || x=="on" || x=="True" || x=="true" || x=="1")

#if (CA_USE_PERSIST_CON)
  #define CA_PREFERENCES_NS "ConfigAssist" // Name space for pererences
  #include <Preferences.h>
#endif

//Structure for config elements
struct confPairs {
    String name;
    String value;
    String label;
    String attribs;
    String text;
    int readNo;
    byte type;
};

// Define display types for the portal
enum class ConfigAssistDisplayType:uint8_t  {
  // All cards open
  AllOpen = 0,
  // All cards closed
  AllClosed,
  // Open clicked and close other cards
  Accordion,
  // Open/Close clicked and close other cards
  AccordionToggleClosed,
};

//Seperators of config elements
struct confSeperators {
    String name;
    String value;
};

// Positions of keys inside array
struct confNdx {
    String key;
    int ndx;
};

// Call back function type
typedef std::function<void(String key)> ConfigAssistChangeCbf;

// ConfigAssist class
class ConfigAssist{
  public:
    // Initialize with defaults
    ConfigAssist();
    // Initialize with custom ini file, load default dict ?
    ConfigAssist(const String& ini_file, bool defaultYaml = false);
    // Initialize with custom Ini file, and custom yaml dict
    ConfigAssist(const String& ini_file, const char * dictStr);
    ~ConfigAssist();
  public:
    // Load configs after storage is started
    void init();
    // Set the portal dislay type. See ConfigAssistDisplayType
    void setDisplayType(ConfigAssistDisplayType display) { _displayType = display; }
    // Start storage if not init
    void startStorage();
    // Set ini file at run time
    void setIniFile(const String& ini_file);
    // Set yaml at run time.. Must called before _init
    void setDictStr(const char * dictStr, bool load = false);
    // Add a script to main page
    void setSubScript(const char * initScript){ _subScript = initScript; }
    // Is config loaded valid ?
    bool valid();
    // Is configuration file exists>
    bool confExists();
    // Is key exists in configuration
    bool exists(String key);
    // Start an AP with a web server and render config values loaded from yaml dictionary
    void setup(WEB_SERVER& server, bool apEnable = false);
    void setupConfigPortal(WEB_SERVER& server, bool apEnable = false);
    // Setup an access point, settings from config or defaults
    bool setupAP(bool startMDNS = false);
    // Setup web servers handlers to handle config portal
    void setupConfigPortalHandlers(WEB_SERVER& server);
    // Add a global callback function to handle changes on form updates
    void setRemotUpdateCallback(ConfigAssistChangeCbf ev);
    // AP started ?
    bool isAPEnabled();
    // Get a temponary hostname
    static String getMacID();
    // Get a temponary hostname
    String getHostName();
    // Get static IP settings from ipString (IP address, subnet mask, and gateway)
    bool getIPFromString(String ipStr, IPAddress &ip, IPAddress &mask, IPAddress &gw);
    // Implement Access operator () i.e. val = config('key')
    String operator () (const String &key);
    // Implement Modify operator [] i.e. config['key'] = val
    String& operator[] (const String& key);
    // Return the value of a given key, Empty on not found
    String get(const String &key);
    // Update the value of key = val (int)
    bool put(const String &key, int val, bool force = false);
    // Update the value of key = val (string)
    bool put(const String &key, String val, bool force = false);
    // Clear all elements
    void clear();
  private:
    // Add vectors by key (name in confPairs)
    int add(const String &key, const String &val, bool force = false);
    // Add unique name vectors pairs
    int add(confPairs &c);
    // Add seperator by key
    void addSeperator(const String &key, const String &val);
    // Rebuild keys indexes wher sorting by readNo
    void rebuildKeysNdx();
    // Sort keys asc by name
    void sortKeysNdx();
    // Sort keys in yaml definition read order
    void sortReadOrder();
    // Sort seperator vectors by key (name in confSeperators)
    void sortSeperators();
  public:
    // Return next key and val from configs on each call in key order
    bool getNextKeyVal(confPairs &c, bool reset = false);
    // Display config items in web server, yaml like
    void dumpYaml(WEB_SERVER *server = NULL);
    // Display config items in web server, or on log on NULL
    void dump(WEB_SERVER *server = NULL);
    // Load a description file. On updateProps = true update only additional pair info
    String loadDict(const char *dictStr, bool updateProps = false);
    // Build config file from dict
    void buildConfigFile();
    // Load config pairs from an ini file
    bool loadConfigFile(String filename = "");
    void loadConfig();
    // Delete configuration files
    bool deleteConfig(String filename = "");
    // Save configs vectors in an ini file
    bool saveConfigFile(String filename = "");
    // Get system local time
    String getLocalTime();
    // Compare browser with system time and correct if needed
    void checkTime(uint32_t timeUtc, int timeOffs);
    // Respond a HTTP request for /scan results
    void handleWifiScanRequest();
    // Send html upload page to client
    void sendHtmlUploadPage();
#if (CA_USE_OTAUPLOAD)
    // Send html ota upload page to client
    void sendHtmlOtaUploadPage();
#endif
#if (CA_USE_FIMRMCHECK)
    // Send html firmware check page to client
    void sendHtmlFirmwareCheckPage();
#endif
    // Upload a file to SPIFFS
    void handleFileUpload();
    // Test the specified connection no
    String testWiFiSTConnection(String no);
    // Download a file in browser window
    void handleDownloadFile(const String &fileName);
    // Respond a not found HTTP request
    void handleNotFound();
    // Respond a HTTP request for the form use the CONF_FILE
    // to save. Save, Reboot ESP, Reset to defaults, cancel edits
    void handleFormRequest(WEB_SERVER* server);
    // Send edit html to client
    void sendHtmlEditPage(WEB_SERVER* server);
    // Get edit page html table (no form)
    //String getEditHtml();
    // Get page css
    String getCSS();
    // Get browser time synchronization java script
#if (CA_USE_TIMESYNC)
    String getTimeSyncScript();
#endif
    // Get html custom message page
    String getMessageHtml();
    // Name ends with key + number?
    bool endsWith(const String &name, const String &key, String& no);
  private:
    // Split a String with delimeter, index -> itemNo
    String splitString(const String &s, char delim, int index);
    // Get child nodes as string
    inline String getChilds(dyml::Directyaml::Node &node);
    // Get all child nodes as tree string, on option key add key & val
    String getChildsTree(dyml::Directyaml::Node node, bool keyVal = false);
    // Is string numeric
    bool isNumeric(String s);
    // Decode given string, replace encoded characters
    String urlDecode(const String& text);
    // Load a file into a string
    bool loadText(const String &fPath, String& txt);
    // Write a string to a file
    bool saveText(const String &fPath, String& txt);
    // Stream text files to web server (long files)
    bool streamText(const String &fPath, WEB_SERVER &server);

#if (CA_USE_PERSIST_CON)
    // Clear nvs
    bool clearPrefs();
    // Save a key from nvs
    bool savePref(const String &key, const String &val);
    // Load a key from nvs
    bool loadPref(const String &key, String &val);
#endif
    // Implement seperators mode
    void modifySeperator(int sepNo, String &outSep);
    // Render keys, values to html lines
    bool streamHtmlElementChunk(WEB_SERVER &server);
    // Render range
    String getRangeHtml(String defVal, String attribs);
    // Render options list
    String getOptionsListHtml(String defVal, String attribs, bool isDataList = false);
    // Get location of given key to retrieve other elements
    int getKeyPos(String key);
    // Get seperation location of given key
    int getSepKeyPos(String key);
    // Extract a config tokens from keyValPair and load it into configs vector
    void loadVectItem(String keyValPair);
 #if (CA_USE_WIFISCAN)
    // Build json on Wifi scan complete
    static void scanComplete(int networksFound);
    // Send wifi scan results to client
    static void checkScanRes();
    // Start async wifi scan
    static void startScanWifi();
#endif
  private:
    enum input_types { TEXT_BOX=1, TEXT_AREA=2, CHECK_BOX=3, OPTION_BOX=4, RANGE_BOX=5, COMBO_BOX=6};
    std::vector<confPairs>        _configs;
    std::vector<confNdx>          _keysNdx;
    std::vector<confSeperators>   _seperators;
    int                           _readNo;
    int                           _forcedNo ;
    uint8_t                       _row;
    bool                          _init;
    bool                          _iniLoaded;
    bool                          _dictLoaded;
    bool                          _dirty;
    const char *                  _dictStr;
    const char *                  _subScript;
    String                        _confFile;
    static WEB_SERVER *           _server;
    static String                 _jWifi;
    bool                          _apEnabled;
    ConfigAssistDisplayType       _displayType;
    ConfigAssistChangeCbf         _changeCbf;
    static bool                   _storageStarted;
#if (CA_USE_PERSIST_CON)
    static Preferences _prefs;
#endif
};
#endif // _CONFIG_ASSIST_H
