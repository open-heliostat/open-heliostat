#ifndef ServoControllerService_h
#define ServoControllerService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <lib/JsonStateRouter.h>
#include <lib/HttpStateRouterEndpoint.h>
#include <servocontroller.h>
#include <DCMotorService.h>

using JsonStateRouting::JsonRouter;
using JsonStateRouting::JsonSaveManager;

#define SERVO_CONTROLLER_STATE_EVENT "servo"
#define SERVO_CONTROLLER_SETTINGS_EVENT "servosettings"
#define SERVO_SETTINGS_FILE "/config/servoSettings.json"

class ServoControllerJsonRouter
{
public:
    static bool route(JsonVariant content, Servo_Driver &controller)
    {
        return router.route(content, controller);
    }
    static void read(Servo_Driver &state, JsonObject &root) 
    {
        router.serialize(state, root);
    }
    static void readForSave(Servo_Driver &state, JsonObject &root) 
    {
        getSaveMap(root);
        router.serialize(state, root);
        JsonDocument ref = getSaveMap();
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }
    static StateUpdateResult update(JsonObject &root, Servo_Driver &state, const String &originId)
    { 
        (void)originId;
        if (router.parse(root, state) && JsonSaveManager::needsToSave(root, getSaveMap())) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }
    static const void getSaveMap(JsonObject &root) 
    {
        root["enabled"] = true;
        root["invert"] = true;
        root["offset"] = true;
        root["tolerance"] = true;
        root["P"] = true;
        root["I"] = true;
        root["D"] = true;
        root["S"] = true;
        root["maxSpeed"] = true;
        root["ramp"] = true;
        root["limits"]["enabled"] = true;
        root["limits"]["begin"] = true;
        root["limits"]["end"] = true;
        root["motor"] = MotorDriverJsonRouter::getSaveMap();
    }
    static const JsonDocument getSaveMap() 
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getSaveMap(obj);
        return doc;
    }
    static JsonRouter<Servo_Driver> router;
};

class ServoControllerService : public StatefulService<Servo_Driver&>
{
public:
    ServoControllerService(PsychicHttpServer *server,
                          EventSocket *socket,
                          FS *fs,
                          SecurityManager *securityManager,
                          Servo_Driver &controller) :
                            _httpRouterEndpoint(_router.read, _router.update, this, server, "/rest/servo", securityManager),
                            _fsPersistence(_router.readForSave, _router.update, this, fs, "/config/servo.json"),
                            StatefulService(controller)
                            {}
    void begin();

private:
    HttpStateRouterEndpoint<Servo_Driver&> _httpRouterEndpoint;
    FSPersistence<Servo_Driver&> _fsPersistence;
    ServoControllerJsonRouter _router;
};
#endif
