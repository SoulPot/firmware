
#include "WiFi.h"

class StatusWebServer {
    private:
    static const unsigned long TIMEOUT = 2000; // ms 

    unsigned long currentTime = 0;
    unsigned long previousTime = 0; 
    String index;

    WiFiServer server;
    WiFiClient client;

    void NotFoundRoute();

    public:
    StatusWebServer();
    void listen();
    void handleClient();
};