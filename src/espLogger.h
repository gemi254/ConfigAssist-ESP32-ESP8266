#ifndef __ESPLOGGER_H__
#define __ESPLOGGER_H__

#define LOGGER_LOG_MODE_SERIAL     (1)  //Log to serial port
#define LOGGER_LOG_MODE_FILE       (2)  //Log to a spiffs file
#define LOGGER_LOG_MODE_EXTERNAL   (3)  //Provide the _log_printf

#ifndef LOGGER_LOG_MODE
  #define LOGGER_LOG_MODE     LOGGER_LOG_MODE_SERIAL
#endif

#define _LOG_LEVEL_NONE      (0)
#define _LOG_LEVEL_ERROR     (1)
#define _LOG_LEVEL_WARN      (2)
#define _LOG_LEVEL_INFO      (3)
#define _LOG_LEVEL_DEBUG     (4)
#define _LOG_LEVEL_VERBOSE   (5)

#ifndef LOGGER_LOG_LEVEL
    #define LOGGER_LOG_LEVEL   _LOG_LEVEL_VERBOSE
#endif

#ifndef LOGGER_LOG_FILENAME
    #define LOGGER_LOG_FILENAME "/log"
#endif

#if LOGGER_LOG_MODE == LOGGER_LOG_MODE_SERIAL
    #define _DEBUG_PORT Serial
#elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_FILE
    static File log_file;
    #define _DEBUG_PORT log_file 
    #define _CHK_LOG_FILE if(!log_file) log_file = STORAGE.open(LOGGER_LOG_FILENAME, "a+")
    #define LOGGER_CLOSE_LOG() log_file.close();
#elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_EXTERNAL
    #ifndef _log_printf
    #endif 
#else 
    #define _DEBUG_PORT Serial    
#endif

#if LOGGER_LOG_MODE != LOGGER_LOG_MODE_EXTERNAL
#define _log_printf(...) _DEBUG_PORT.printf(__VA_ARGS__)
#endif 

#ifndef _LOG_FORMAT
    #ifdef ESP32
        #define _LOG_FORMAT(type, format) "[%s %s @ %s:%u] " format "", esp_log_system_timestamp(), #type, pathToFileName(__FILE__), __LINE__
    #else
        #define _LOG_FORMAT(type, format) "[ %s @ %s:%u] " format "", #type, __FILE__, __LINE__
    #endif
#endif

#if LOGGER_LOG_LEVEL >= _LOG_LEVEL_VERBOSE
    #if LOGGER_LOG_MODE == LOGGER_LOG_MODE_FILE  
        #define LOG_V(format, ...) { _CHK_LOG_FILE;  _log_printf(_LOG_FORMAT(V, format), ##__VA_ARGS__); }
    #elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_EXTERNAL
        #define LOG_V(format, ...) { _log_printf(_LOG_FORMAT(V, format), ##__VA_ARGS__); }
    #else
        #define LOG_V(format, ...) _log_printf(_LOG_FORMAT(V, format), ##__VA_ARGS__)
    #endif
#else
    #define LOG_V(format, ...)
#endif

#if LOGGER_LOG_LEVEL >= _LOG_LEVEL_DEBUG
    #if LOGGER_LOG_MODE == LOGGER_LOG_MODE_FILE  
        #define LOG_D(format, ...) { _CHK_LOG_FILE;  _log_printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
    #elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_EXTERNAL
        #define LOG_D(format, ...) { _log_printf(_LOG_FORMAT(D, format), ##__VA_ARGS__); }
    #else
       #define LOG_D(format, ...) _log_printf(_LOG_FORMAT(D, format), ##__VA_ARGS__)
    #endif
#else
    #define LOG_D(format, ...)
#endif

#if LOGGER_LOG_LEVEL >= _LOG_LEVEL_INFO
    #if LOGGER_LOG_MODE == LOGGER_LOG_MODE_FILE   
        #define LOG_I(format, ...) { _CHK_LOG_FILE;  _log_printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
    #elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_EXTERNAL
        #define LOG_I(format, ...) { _log_printf(_LOG_FORMAT(I, format), ##__VA_ARGS__); }
    #else
        #define LOG_I(format, ...) _log_printf(_LOG_FORMAT(I, format), ##__VA_ARGS__)
    #endif
#else
    #define LOG_I(format, ...)
#endif

#if LOGGER_LOG_LEVEL >= _LOG_LEVEL_WARN
    #if LOGGER_LOG_MODE == LOGGER_LOG_MODE_FILE  
        #define LOG_W(format, ...) { _CHK_LOG_FILE;  _log_printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
    #elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_EXTERNAL
        #define LOG_W(format, ...) { _log_printf(_LOG_FORMAT(W, format), ##__VA_ARGS__); }
    #else
        #define LOG_W(format, ...) _log_printf(_LOG_FORMAT(W, format), ##__VA_ARGS__)
    #endif
#else
    #define LOG_W(format, ...)
#endif

#if LOGGER_LOG_LEVEL >= _LOG_LEVEL_ERROR
    #if LOGGER_LOG_MODE == LOGGER_LOG_MODE_FILE
        #define LOG_E(format, ...) { _CHK_LOG_FILE;  _log_printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
    #elif LOGGER_LOG_MODE == LOGGER_LOG_MODE_EXTERNAL
        #define LOG_E(format, ...) { _log_printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
    #else
        #define LOG_E(format, ...) _log_printf(_LOG_FORMAT(E, format), ##__VA_ARGS__)
    #endif
#else
    #define LOG_E(format, ...)
#endif

#define LOG_N(format, ...)

#endif //__ESPLOGGER_H__