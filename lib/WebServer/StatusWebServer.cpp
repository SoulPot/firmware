#include "SerialLogger.h"
#include "StatusWebServer.h"

StatusWebServer::StatusWebServer() {
    Logger.Debug("Hello from status web server...");
    this->index = "<!DOCTYPE html>";
    this->index += "<html lang='fr'>";
    this->index += "<head>";
    this->index += "    <title>Serveur ESP32</title>";
    this->index += "    <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'/>";
    this->index += "    <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
    this->index += "</head>";

    this->index += "<body>";
    this->index += "<div> Hello from WebServer from Analyzer</div>";

    this->index += "</body>";
    this->index += "</html>";
    this->server = WiFiServer();
    this->currentTime = millis();
    this->server.begin();
}

void StatusWebServer::listen() {
    String header;
    String currentLine = "";
    this->client = this->server.available();
    if (this->client) {
        this->currentTime = millis();
        this->previousTime = currentTime;
        if (this->client.connected()) {
            Logger.Info("New client connected");
            String line = this->client.readString();
            String route = line.substring(line.indexOf('/'), line.length() - 1);
            route = route.substring(0, route.indexOf(' '));
            Logger.Debug(route);
        }
        while(this->client.connected() && this->currentTime - this->previousTime <= StatusWebServer::TIMEOUT) {
            this->currentTime = millis();
            if(this->client.available()) {
                char c = client.read();
                header += c;
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");

                        client.println("<body><h1>ESP32 Web Server</h1>");
                        client.println("</body></html>");
                        client.println(); // html ends with a blank line
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                    Logger.Debug(currentLine);
                } 
            }
        }
        Logger.Debug(header);

        if (this->currentTime - this->previousTime > StatusWebServer::TIMEOUT) {
            Logger.Error("Timeout while connecting to client");
        }
    }

    if (this->client.connected()) {
        this->client.stop();
        Logger.Info("client disconnected");
    }
}

void StatusWebServer::handleClient() {
}

void StatusWebServer::NotFoundRoute() {
}