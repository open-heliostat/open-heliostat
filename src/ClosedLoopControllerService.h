#ifndef ClosedLoopControllerService_h
#define ClosedLoopControllerService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <StatelessService.h>

#include <closedloopcontroller.h>

#define CL_CONTROLLER_STATE_EVENT "controller"
#define CL_CONTROLLER_SETTINGS_EVENT "controllersettings"
#define CL_SETTINGS_FILE "/config/controllerSettings.json"


class ClosedLoopControllerJsonReader
{
public:
    ClosedLoopControllerJsonReader(ClosedLoopController &controller) : 
        controller(controller) {}
    bool route(JsonVariant content)
    {
        return router.route(content);
    }
private:
    JsonRouter router = JsonRouter(
    {
        {"calibration", [&](JsonVariant content) {
            return calibrationRouter.route(content);
        }},
        {"target", [&](JsonVariant content) {
            if (content.is<double>()) {
                controller.setAngle(content.as<double>());
                return true;
            }
            else return false;
        }}
    },
    {
        {"position", [&](const JsonVariant& target) {
            target.set(controller.getAngle());
        }},
        {"target", [&](const JsonVariant target) {
            target.set(controller.targetAngle);
        }},
        {"calibration", [&](const JsonVariant target) {
            target["running"].set(controller.calibrationRunning);
            target["steps"].set(controller.calibrationSteps);
            if (target["offsets"].is<JsonVariant>()) {
                auto array = target["offsets"].to<JsonArray>();
                for (int i =0; i < controller.calibrationSteps; i++) {
                    array.add(controller.calibrationOffsets[i]);
                }
            }
        }},
    });
    JsonEventRouter calibrationRouter = JsonEventRouter({
        {"start", [&](JsonVariant content) {
            controller.startCalibration();
            return true;
        }},
        {"stop", [&](JsonVariant content) {
            controller.stopCalibration();
            return true;
        }},
    });
    ClosedLoopController &controller;
};

class ClosedLoopControllerState
{
public:
    double targetAngle;
    double curAngle;

    static void read(ClosedLoopControllerState &state, JsonObject &root) {
        root["targetAngle"] = state.targetAngle;
        root["curAngle"] = state.curAngle;
    }

    static StateUpdateResult update(JsonObject &root, ClosedLoopControllerState &state) {
        bool changed = false;
        if (root["targetAngle"].is<double>() & state.targetAngle != root["targetAngle"]) {
            state.targetAngle = root["targetAngle"];
            changed = true;
        }
        if (root["curAngle"].is<double>() & state.curAngle != root["curAngle"]) {
            state.curAngle = root["curAngle"];
            changed = true;
        }
        if (changed) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }

    static void readState(ClosedLoopController *controller, JsonObject &root) {
        root["targetAngle"] = controller->targetAngle;
        root["curAngle"] = (int)(controller->encoder.angle * 100 + 0.5) / 100.0;
    }
};

class ClosedLoopControllerStates
{
public:
    std::vector<ClosedLoopControllerState> controllers;
    static void read(ClosedLoopControllerStates &controllers, JsonObject &root)
    {
        JsonArray jsonArray = root["controllers"].to<JsonArray>();
        for (ClosedLoopControllerState controller : controllers.controllers) {
            JsonObject obj = jsonArray.add<JsonObject>();
            controller.read(controller, obj);
        }
    }
    static StateUpdateResult update(JsonObject &root, ClosedLoopControllerStates &controllers)
    {
        JsonArray jsonArray = root["controllers"].as<JsonArray>();
        bool hasChanged = false;
        for (int i = 0; i < min(jsonArray.size(), controllers.controllers.size()); i++) {
            JsonObject obj = jsonArray[i];
            if (controllers.controllers[i].update(obj, controllers.controllers[i]) == StateUpdateResult::CHANGED) hasChanged = true;
        }
        return hasChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
    }
    static void readState(std::vector<ClosedLoopController*> &controllers, JsonObject &root) {
        JsonArray jsonArray = root["controllers"].to<JsonArray>();
        for (ClosedLoopController *controller : controllers) {
            JsonObject obj = jsonArray.add<JsonObject>();
            ClosedLoopControllerState::readState(controller, obj);
        }
    }
};

class ClosedLoopControllerStateService : public StatefulService<ClosedLoopControllerStates>
{
public:
    ClosedLoopControllerStateService( EventSocket *socket,
                                      std::vector<ClosedLoopController*>& controllers);
    void begin();
    void updateState();
    void loop();

private:
    EventEndpoint<ClosedLoopControllerStates> _eventEndpoint;
    std::vector<ClosedLoopController*>& _controllers;

    void onConfigUpdated(const String &originId);
};

class ClosedLoopControllerSettings
{
public:
    bool enabled;
    bool hasLimits;
    double tolerance;
    double limitA;
    double limitB;
    String name;

    static void read(ClosedLoopControllerSettings &state, JsonObject &root) {
        root["tolerance"] = state.tolerance;
        root["enabled"] = state.enabled;
        root["limitA"] = state.limitA;
        root["limitB"] = state.limitB;
        root["name"] = state.name;
    }
    static StateUpdateResult update(JsonObject &root, ClosedLoopControllerSettings &state) {
        state.tolerance = root["tolerance"] | 0.2;
        state.enabled = root["enabled"] | false;
        state.hasLimits = root["hasLimits"] | false;
        state.limitA = root["limitA"] | 0.;
        state.limitB = root["limitB"] | 360.;
        state.name = root["name"] | "Controller";
        return StateUpdateResult::CHANGED;
    }

    static void readState(ClosedLoopController *controller, JsonObject &root) {
        root["tolerance"] = controller->tolerance;
        root["enabled"] = controller->enabled;
        root["hasLimits"] = controller->hasLimits;
        root["limitA"] = controller->limitA;
        root["limitB"] = controller->limitB;
    }
};

class MultiClosedLoopControllerSettings
{
public:
    std::vector<ClosedLoopControllerSettings> settings;
    static void read(MultiClosedLoopControllerSettings &settings, JsonObject &root)
    {
        JsonArray jsonArray = root["controllers"].to<JsonArray>();
        for (ClosedLoopControllerSettings controller : settings.settings) {
            JsonObject obj = jsonArray.add<JsonObject>();
            controller.read(controller, obj);
        }
    }
    static StateUpdateResult update(JsonObject &root, MultiClosedLoopControllerSettings &settings)
    {
        JsonArray jsonArray = root["controllers"].as<JsonArray>();
        bool hasChanged = false;
        for (int i = 0; i < min(jsonArray.size(), settings.settings.size()); i++) {
            JsonObject obj = jsonArray[i];
            if (settings.settings[i].update(obj, settings.settings[i]) == StateUpdateResult::CHANGED) hasChanged = true;
        }
        return hasChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
    }
};

class ClosedLoopControllerSettingsService : public StatefulService<MultiClosedLoopControllerSettings>
{
public:
    ClosedLoopControllerSettingsService(EventSocket *socket,
                                        FS *fs,
                                        std::vector<ClosedLoopController*>& controllers);
    void begin();
    void loop();

private:
    EventEndpoint<MultiClosedLoopControllerSettings> _eventEndpoint;
    FSPersistence<MultiClosedLoopControllerSettings> _fsPersistence;
    std::vector<ClosedLoopController*>& _controllers;
    ClosedLoopControllerStateService _closedLoopControllerStateService;

    void onConfigUpdated();
};
#endif