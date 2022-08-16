
#ifndef STATUS_WEB_SERVER
#define STATUS_WEB_SERVER

#include "WiFi.h"
#include "WebServer.h"
#include "ESPManager.h"
#include "BLEManager.h"

class StatusWebServer {
    private:
    static const unsigned long TIMEOUT = 5000; // ms 
    static const unsigned short PORT = 80;

    unsigned long currentTime = 0;
    unsigned long previousTime = 0;
    static String index;

    static WebServer* server;

    static void routing();
    static void homeRoute();
    static void notFoundRoute();

    public:
    StatusWebServer();
    void begin();
    void listen();
    void handleClient();
};
#endif