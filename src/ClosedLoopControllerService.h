#ifndef ClosedLoopControllerService_h
#define ClosedLoopControllerService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <StatelessService.h>
#include <HttpRouterEndpoint.h>

#include <StepperService.h>

#include <closedloopcontroller.h>

#define CL_CONTROLLER_STATE_EVENT "controller"
#define CL_CONTROLLER_SETTINGS_EVENT "controllersettings"
#define CL_SETTINGS_FILE "/config/controllerSettings.json"

class ClosedLoopControllerJsonRouter
{
public:
    static bool route(JsonVariant content, ClosedLoopController &controller)
    {
        return router.route(content, controller);
    }
    static void read(ClosedLoopController &state, JsonObject &root) 
    {
        router.serialize(state, root);
    }
    static void readForSave(ClosedLoopController &state, JsonObject &root) 
    {
        getSaveMap(root);
        router.serialize(state, root);
        JsonDocument ref = getSaveMap();
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }
    static StateUpdateResult update(JsonObject &root, ClosedLoopController &state)
    { 
        if (router.parse(root, state) && JsonSaveManager::needsToSave(root, getSaveMap())) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }
    static const void getSaveMap(JsonObject &root) 
    {
        root["calibration"]["enabled"] = true;
        root["calibration"]["offsets"] = true;
        root["limits"]["enabled"] = true;
        root["limits"]["begin"] = true;
        root["limits"]["end"] = true;
        root["enabled"] = true;
        root["invert"] = true;
        root["offset"] = true;
        root["stepper"] = TMC5160ControllerJsonRouter::getSaveMap();
    }
    static const JsonDocument getSaveMap() 
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getSaveMap(obj);
        return doc;
    }
    static JsonRouter<ClosedLoopController> router;
    static JsonEventRouter<ClosedLoopController> calibrationRouter;
    static JsonEventRouter<ClosedLoopController> limitsRouter;
};

class ClosedLoopControllerService : public StatefulService<ClosedLoopController&>
{
public:
    ClosedLoopControllerService(PsychicHttpServer *server,
                                EventSocket *socket,
                                FS *fs,
                                SecurityManager *securityManager,
                                ClosedLoopController &controller) :
                                    _httpRouterEndpoint(_router.read, _router.update, this, server, "/rest/controllers", securityManager),
                                    _fsPersistence(_router.readForSave, _router.update, this, fs, "/config/controllers.json"),
                                    // _eventEndpoint(_router.read, _router.update, this, socket, "controllers"),
                                    StatefulService(controller)
                                    {}
    void begin();

private:
    // EventEndpoint<ClosedLoopController&> _eventEndpoint;
    HttpRouterEndpoint<ClosedLoopController&> _httpRouterEndpoint;
    FSPersistence<ClosedLoopController&> _fsPersistence;
    ClosedLoopControllerJsonRouter _router;
};
#endif