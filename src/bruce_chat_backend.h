#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <vector>

static std::vector<String> bruceChatEventsQueue;
static const size_t BRUCE_MAX_CHAT_EVENTS = 25;

static const char* BRUCE_DEFAULT_DB_JSON = R"json({"accounts":{"admin":{"nick":"admin","password":"1234","color":"#d32f2f","role":"admin"}},"banned":[]})json";

inline void setupBruceChatRoutes(AsyncWebServer &server) {
    // 1. Раздача стилей и скрипта чата с SD-карты
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

    // 2. Чтение общей базы данных аккаунтов, ролей и банов с SD-карты
    server.on("/db.json", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SD.exists("/portals/chat/db.json")) {
            request->send(SD, "/portals/chat/db.json", "application/json");
        } else {
            request->send(200, "application/json", BRUCE_DEFAULT_DB_JSON);
        }
    });

    // 3. Отдача очереди новых событий/сообщений клиентам (Polling)
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

    // 4. Прием событий от клиентов и сохранение изменений БД на SD-карту
    server.on("/chat_post", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("data", true)) {
            String rawData = request->getParam("data", true)->value();

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, rawData);
            if (!err) {
                String type = doc["type"] | "";

                if (type == "db_sync" && doc["db"].is<JsonObject>()) {
                    if (!SD.exists("/portals")) SD.mkdir("/portals");
                    if (!SD.exists("/portals/chat")) SD.mkdir("/portals/chat");

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
