#ifndef BRUCE_CHAT_BACKEND_H
#define BRUCE_CHAT_BACKEND_H

#include <Arduino.h>
#include <SD.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>

static std::vector<String> bruceChatEventsQueue;
static const size_t BRUCE_MAX_CHAT_EVENTS = 25;

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
            request->send(200, "application/json", "{\"accounts\":{\"admin\":{\"nick\":\"admin\",\"password\":\"1234\",\"color\":\"#d32f2f\",\"role\":\"admin\"}},\"banned\":[]}");
        }
    });

    server.on("/messages", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "[";
        for (size_t i = 0; i < bruceChatEventsQueue.size(); i++) {
            json += bruceChatEventsQueue[i];
            if (i + 1 < bruceChatEventsQueue.size()) {
                json += ",";
            }
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("data", true)) {
            String rawData = request->getParam("data", true)->value();

            DynamicJsonDocument doc(4096);
            DeserializationError err = deserializeJson(doc, rawData);
            if (!err) {
                String type = doc["type"] | "";

                if (type == "db_sync" && doc.containsKey("db")) {
                    if (!SD.exists("/portals/chat")) {
                        SD.mkdir("/portals/chat");
                    }
                    File f = SD.open("/portals/chat/db.json", FILE_WRITE);
                    if (f) {
                        serializeJson(doc["db"], f);
                        f.close();
                    }
                } else {
                    if (bruceChatEventsQueue.size() >= BRUCE_MAX_CHAT_EVENTS) {
                        bruceChatEventsQueue.erase(bruceChatEventsQueue.begin());
                    }
                    bruceChatEventsQueue.push_back(rawData);
                }
            }
        }
        request->send(200, "text/plain", "OK");
    });
}

#endif // BRUCE_CHAT_BACKEND_H
