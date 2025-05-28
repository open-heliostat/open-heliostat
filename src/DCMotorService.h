#ifndef DCMotorService_h
#define DCMotorService_h

#include <EventEndpoint.h>
#include <HttpRouterEndpoint.h>
#include <FSPersistence.h>
#include <StatelessService.h>

#include <dcmotor.h>

class MotorDriverJsonRouter
{
public:
    static bool route(JsonVariant content, Motor_Driver &controller)
    {
        return router.route(content, controller);
    }
    static void read(Motor_Driver &state, JsonObject &root) 
    {
        router.serialize(state, root);
    }
    static void readForSave(Motor_Driver &state, JsonObject &root) 
    {
        getSaveMap(root);
        router.serialize(state, root);
        JsonDocument ref = getSaveMap();
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }
    static StateUpdateResult update(JsonObject &root, Motor_Driver &state)
    { 
        if (router.parse(root, state) && JsonSaveManager::needsToSave(root, getSaveMap())) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }
    static const void getSaveMap(JsonObject &root) 
    {
        root["config"]["minVal"] = true;
        root["config"]["shape"] = true;
        root["config"]["enable"] = true;
        root["config"]["invert"] = true;
    }
    static const JsonDocument getSaveMap() 
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getSaveMap(obj);
        return doc;
    }
    static JsonRouter<Motor_Driver> router;
    static JsonEventRouter<Motor_Driver> controlRouter;
    static JsonEventRouter<Motor_Driver> configRouter;
};

class DCMotorService : public StatefulService<Motor_Driver&>
{
public:
    DCMotorService(PsychicHttpServer *server,
                    FS *fs,
                    SecurityManager *securityManager,
                    Motor_Driver &controller) :
                        _httpRouterEndpoint(_router.read, _router.update, this, server, "/rest/motor", securityManager),
                        _fsPersistence(_router.readForSave, _router.update, this, fs, "/config/motor.json"),
                        StatefulService(controller)
                        {}
    void begin();

private:
    HttpRouterEndpoint<Motor_Driver&> _httpRouterEndpoint;
    FSPersistence<Motor_Driver&> _fsPersistence;
    MotorDriverJsonRouter _router;
};

#endif
