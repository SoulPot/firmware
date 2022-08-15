#include "SerialLogger.h"
#include "StatusWebServer.h"

String StatusWebServer::index;
WebServer* StatusWebServer::server;

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
    this->index += "<div>SP Firmware status page</div>";
    this->index += ESPManager::getHtmlState();

    this->index += "</body>";
    this->index += "</html>";
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
    StatusWebServer::server->handleClient();
}

void StatusWebServer::notFoundRoute() {
    StatusWebServer::server->send(404, "text/plain", "Page not found.");
}

void StatusWebServer::homeRoute() {
    StatusWebServer::server->send(200, "text/html", StatusWebServer::index);
}