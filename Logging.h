#pragma once

// define a function REMOTE_LOG_WRITE(char *) to use remote logging
extern void LOG_CALLBACK(char *);
static void LOG_REMOVE_NEWLINE(char *msg) {
    int i = strlen(msg);
    if (i > 0 && msg[i-1] == '\n')   // remove trailing \n
        msg[i-1] = 0;
};

#ifdef LOG_REMOTE
    static void __FORMAT_LOG(const char* m, ...) { 
        char *logmsg;
        va_list args;
        va_start(args, m);
        vasprintf(&logmsg, m, args);
        LOG_CALLBACK(logmsg);
        free(logmsg);
        va_end(args);
    };
#else
    #define __FORMAT_LOG(...)   Serial.printf(__VA_ARGS__)
#endif

#if not defined (LOG_LEVEL) || LOG_LEVEL == 0
    #define ERROR(...)
    #define INFO(...)
    #define DEBUG(...)
    #define DEBUG_BIN(...)
#elif LOG_LEVEL == 1
    #define ERROR(...)    __FORMAT_LOG(__VA_ARGS__)
    #define INFO(...)
    #define DEBUG(...)
    #define DEBUG_BIN(...)
#elif LOG_LEVEL == 2
    #define ERROR(...)    __FORMAT_LOG(__VA_ARGS__)
    #define INFO(...)     __FORMAT_LOG(__VA_ARGS__)
    #define DEBUG(...)
    #define DEBUG_BIN(...)
#elif LOG_LEVEL == 3
    #define ERROR(...)    __FORMAT_LOG(__VA_ARGS__)
    #define INFO(...)     __FORMAT_LOG(__VA_ARGS__)
    #define DEBUG(...)    __FORMAT_LOG(__VA_ARGS__)
    static void DEBUG_BIN(const char* m, uint8_t b[], int l) {
            Serial.print(m);
            for (int i=0; i<l;i++)
                Serial.printf("%02X", b[i]);
            Serial.println();
    };
#endif


