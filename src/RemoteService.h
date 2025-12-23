#ifndef RemoteService_h
#define RemoteService_h

#include <ESP32SvelteKit.h>
#include <StatelessService.h>
#include <HttpRouterEndpoint.h>
#include <FSPersistence.h>
#include "remotes.h"

class RemoteJsonRouter 
{
public:
    static bool route(JsonVariant content, RemotesController& controller)
    {
        return router.route(content, controller);
    }

    static void read(RemotesController& state, JsonObject& root)
    {
        router.serialize(state, root);
    }

    static void readForSave(RemotesController& state, JsonObject& root)
    {
        getSaveMap(root);
        router.serialize(state, root);
        JsonDocument ref = root;
        Serial.println(ref.as<String>());
        ref = getSaveMap();
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }

    static StateUpdateResult update(JsonObject& root, RemotesController& state, const String &originId)
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
        root["remotes"] = true;
        root["settings"] = true;
    }

    static JsonRouter<RemotesController> router;
};

class RemoteService : public StatefulService<RemotesController&>
{
public:
    RemoteService(PsychicHttpServer* server,
                 ESP32SvelteKit* sveltekit,
                 RemotesController& controller) :
        _httpRouterEndpoint(_router.read, 
                           _router.update, 
                           this, 
                           server, 
                           "/rest/remote", 
                           sveltekit->getSecurityManager()),
        _fsPersistence(_router.readForSave,
                      _router.update,
                      this,
                      sveltekit->getFS(),
                      "/config/remotes.json"),
        StatefulService(controller) {}

    void begin();
    void loop();

private:
    HttpRouterEndpoint<RemotesController&> _httpRouterEndpoint;
    FSPersistence<RemotesController&> _fsPersistence;
    RemoteJsonRouter _router;
};

#endif