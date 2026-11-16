#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <vector>

static std::vector<String> bruceChatQueue;
static const size_t MAX_BRUCE_CHAT_QUEUE = 25;

inline void setupBruceChatRoutes(AsyncWebServer &server) {
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SD.exists("/portals/chat/style.css")) {
            request->send(SD, "/portals/chat/style.css", "text/css");
        } else {
            request->send(404, "text/plain", "style.css not found");
        }
    });

    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SD.exists("/portals/chat/app.js")) {
            request->send(SD, "/portals/chat/app.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "app.js not found");
        }
    });

    server.on("/db.json", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SD.exists("/portals/chat/db.json")) {
            request->send(SD, "/portals/chat/db.json", "application/json");
        } else {
            request->send(200, "application/json", "{"accounts":{"admin":{"nick":"admin","password":"1234","color":"#d32f2f","role":"admin"}},"banned":[]}");
        }
    });

    server.on("/messages", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "[";
        for (size_t i = 0; i < bruceChatQueue.size(); i++) {
            json += bruceChatQueue[i];
            if (i + 1 < bruceChatQueue.size()) json += ",";
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    server.on("/chat_post", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("data", true)) {
            String rawData = request->getParam("data", true)->value();
            DynamicJsonDocument doc(4096);
            if (!deserializeJson(doc, rawData)) {
                String type = doc["type"] | "";
                if (type == "db_sync" && doc.containsKey("db")) {
                    File f = SD.open("/portals/chat/db.json", FILE_WRITE);
                    if (f) {
                        serializeJson(doc["db"], f);
                        f.close();
                    }
                } else {
                    if (bruceChatQueue.size() >= MAX_BRUCE_CHAT_QUEUE) {
                        bruceChatQueue.erase(bruceChatQueue.begin());
                    }
                    bruceChatQueue.push_back(rawData);
                }
            }
        }
        request->send(200, "text/plain", "OK");
    });
}
