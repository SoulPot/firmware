#include "SerialLogger.h"
#include "StatusWebServer.h"

String StatusWebServer::index;
WebServer* StatusWebServer::server;

StatusWebServer::StatusWebServer() {
    Logger.Debug("Hello from status web server...");
    this->server = new WebServer(StatusWebServer::PORT);
    StatusWebServer::routing();
    this->currentTime = millis();
}

void StatusWebServer::routing() {
    StatusWebServer::server->onNotFound(StatusWebServer::notFoundRoute);
    StatusWebServer::server->on("/", StatusWebServer::homeRoute);
}

void StatusWebServer::begin() {
    StatusWebServer::server->begin();
}

void StatusWebServer::listen() {
    // refresh test
    StatusWebServer::server->handleClient();
}

void StatusWebServer::notFoundRoute() {
    StatusWebServer::server->send(404, "text/plain", "Page not found.");
}

void StatusWebServer::homeRoute() {
    StatusWebServer::index = "<!DOCTYPE html>";
    StatusWebServer::index += "<html lang='fr'>";
    StatusWebServer::index += "<head>";
    StatusWebServer::index += "    <title>Serveur ESP32</title>";
    StatusWebServer::index += "    <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'/>";
    StatusWebServer::index += "    <link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
    StatusWebServer::index += "</head>";
    StatusWebServer::index += "<body>";
    StatusWebServer::index += "<div>SP Firmware status page</div>";

    StatusWebServer::index += ESPManager::getHtmlState();

    StatusWebServer::index += "</body>";
    StatusWebServer::index += "</html>";
    StatusWebServer::server->send(200, "text/html", StatusWebServer::index);
}