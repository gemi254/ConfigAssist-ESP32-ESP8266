#ifndef CONFIGASSISTHELPER_H
#define CONFIGASSISTHELPER_H

// Include necessary libraries depending on the platform
#include "ConfigAssist.h"
#if defined(ESP32)
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include <time.h>
#include <functional>

// ConfigAssistHelper class declaration
class ConfigAssistHelper {
public:
    // Enum for Wi-Fi connection result
    enum class WiFiResult {
        SUCCESS,              // Successful connection
        CONNECTION_TIMEOUT,   // Connection timeout error
        DISCONNECTION_ERROR,  // Disconnection error
        NTP_SYNC_FAILED,      // NTP synchronization failed
        INVALID_CREDENTIALS   // Invalid Wi-Fi credentials
    };

    // Enum to track the LED state
    enum class LEDState {
        OFF,           // LED is off
        CONNECTING,    // LED is blinking while connecting
        CONNECTED,     // LED is on after successful connection
        DISCONNECTED,  // LED is on after disconnection
        TIMEOUT        // LED shows timeout indication
    };

    // Define callback type for Wi-Fi connection result
    using WiFiResultCallback = std::function<void(WiFiResult, const String&)>;

    // Constructor that initializes the class with a ConfigAssist instance
    ConfigAssistHelper(ConfigAssist& conf);
    // Destructor
    ~ConfigAssistHelper();

    // Set the time zone for the system
    void setEnvTimeZone(const char* tz);
    // Set the time zone from the configuration file
    void setEnvTimeZone();
    // Synchronize time asynchronously with a timeout
    void syncTimeAsync(uint32_t syncTimeout = 20000, bool force = false);
    // Synchronize time synchronously with a timeout
    void syncTime(uint32_t syncTimeout = 20000, bool force = false, bool async = false);
    // Check if the time is synchronized
    bool isTimeSync();
    // Wait for the time synchronization with a timeout
    void waitForTimeSync(uint32_t syncTimeout = 20000);
    // Validate the Wi-Fi configuration in the configuration file
    bool validateWiFiConfig();
    // Set a callback function to handle Wi-Fi result
    void setWiFiResultCallback(WiFiResultCallback callback);
    // Set the pin number for the LED indicator
    void setLedPin(uint8_t pin);
    // Set the connection timeout for Wi-Fi
    void setConnectionTimeout(uint32_t connectTimeout);
    // Set whether to reconnect to Wi-Fi automatically
    void setReconnect(bool reconnect);
    // Get the current LED state
    LEDState getLedState();
    // Run the main loop to manage Wi-Fi connection and LED states
    void loop();
    // Connect to the Wi-Fi network if connectTimeout == 0 default timeout from config or 15000
    bool connectToNetwork(uint32_t connectTimeout = 0, const uint8_t ledPin = 0, const bool async = false);
    // Connect to the network asynchronously
    void connectToNetworkAsync(uint32_t connectTimeout = 0, const uint8_t ledPin = 0, WiFiResultCallback callback = nullptr);
    // Start mDNS (Multicast DNS) for the device
    bool startMDNS();

private:
    // Reset the system clock
    void resetClock();
    // Restore the system clock after a time synchronization attempt
    void restoreClock(time_t elapsed);
    // Get SSID, password, and IP from the configuration
    bool getStSSID(String& ssid, String& pass, String& ip);
    // Print a character to the serial monitor
    void serialPrint(char ch = '.');
    // Print the current status to the serial monitor
    void printStatus(bool end = false);
    // Update the LED state based on the current connection status
    void updateLED();
    // Begin Wi-Fi connection with SSID, password, and IP
    void beginWiFiConnection(const String& ssid, const String& password, const String& ip);
    // Attempt to connect to Wi-Fi
    bool connectWiFi(bool async);
    // Wait for a result from the connection process
    void waitForResult();
    // Wait for the connection to complete with a timeout
    void waitForConnection(uint32_t connectTimeout = 0);
    // Check if the device is still connected to Wi-Fi
    // Will try to reconnect or swich to another connection
    void checkConnection();

    ConfigAssist&   _conf;                // Reference to the ConfigAssist instance
    confPairs       _curSSID;
    uint8_t         _ledPin;              // Pin for the LED
    LEDState        _ledState;            // Current LED state
    bool            _reconnect;           // Flag to indicate whether to reconnect automatically
    bool            _waitForResult;       // Flag to wait for the result of the Wi-Fi connection
    time_t          _timeOld;             // Previous time value
    uint32_t        _connectTimeout;      // Timeout for Wi-Fi connection
    uint32_t        _startAttemptTime;    // Time when the connection attempt started
    uint32_t        _checkConnectionTime; // Time to check connection status
    WiFiResultCallback _resultCallback;   // Callback for Wi-Fi connection result
};

#endif // CONFIGASSISTHELPER_H
