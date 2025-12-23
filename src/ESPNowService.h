#ifndef ESPNowService_h
#define ESPNowService_h

#include <ESP32SvelteKit.h>
#include <StatelessService.h>
#include <HttpRouterEndpoint.h>
#include <FSPersistence.h>
#include "espnow.h"

class ESPNowJsonRouter
{
public:
    static bool route(JsonVariant content, ESPNowState& state)
    {
        return router.route(content, state);
    }

    static void read(ESPNowState& state, JsonObject& root)
    {
        router.serialize(state, root);
    }

    static void readForSave(ESPNowState& state, JsonObject& root)
    {
        getSaveMap(root);
        router.serialize(state, root);
        JsonDocument ref = root;
        Serial.println(ref.as<String>());
        ref = getSaveMap();
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }

    static StateUpdateResult update(JsonObject& root, ESPNowState& state, const String &originId)
    {
        (void)originId;
        if (router.parse(root, state) && JsonSaveManager::needsToSave(root, getSaveMap())) {
            return StateUpdateResult::CHANGED;
        }
        return StateUpdateResult::UNCHANGED;
    }

    static const JsonDocument getSaveMap()
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getSaveMap(obj);
        return doc;
    }

    static const void getSaveMap(JsonObject& root)
    {
        root["enabled"] = true;
        root["channel"] = true;
        root["retryDelay"] = true;
    }

    static JsonRouter<ESPNowState> router;
};

class ESPNowService : public StatefulService<ESPNowState>
{
public:
    ESPNowService(PsychicHttpServer* server,
                ESP32SvelteKit* sveltekit,
                ESPNowState& event) :
        _httpRouterEndpoint(ESPNowJsonRouter::read, 
                   ESPNowJsonRouter::update,
                           this, 
                           server, 
                           "/rest/espnow", 
                           sveltekit->getSecurityManager()),
        _fsPersistence(_router.readForSave,
                  ESPNowJsonRouter::update,
                      this,
                      sveltekit->getFS(),
                      "/config/espnow.json"),
        StatefulService(event) {}

    void begin();
    void loop();

private:
    HttpRouterEndpoint<ESPNowState> _httpRouterEndpoint;
    FSPersistence<ESPNowState> _fsPersistence;
    ESPNowJsonRouter _router;
};

#endif