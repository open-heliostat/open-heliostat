#ifndef StepperControlService_h
#define StepperControlService_h

#include <StepperSettingsService.h>

#include <EventEndpoint.h>
#include <FeaturesService.h>

#include <tmcdriver.h>

#define STEPPER_CONTROL_EVENT "steppercontrol"

class StepperControl
{
public:
    bool isEnabled;
    bool direction;
    float move;
    float newMove;
    float speed;
    float acceleration;
    uint32_t status;
    uint8_t version;
    // StepperSettings settings;

    static void read(StepperControl &settings, JsonObject &root)
    {
        root["isEnabled"] = settings.isEnabled;
        root["direction"] = settings.direction;
        root["move"] = settings.move;
        root["speed"] = settings.speed;
        root["acceleration"] = settings.acceleration;
        root["status"] = settings.status;
        root["version"] = settings.version;
        // Serial.print("Read : ");
        // Serial.println(settings.status);
    }

    static StateUpdateResult update(JsonObject &root, StepperControl &settings, const String &originId)
    {
        (void)originId;
        bool hasChanged = false;
        if (root["isEnabled"].is<bool>() && settings.isEnabled != root["isEnabled"]) {
            settings.isEnabled = root["isEnabled"];
            hasChanged = true;
        }
        if (root["direction"].is<bool>() && settings.direction != root["direction"]) {
            settings.direction = root["direction"];
            hasChanged = true;
        }
        if (root["move"].is<float>() && abs(settings.move - float(root["move"])) > 0.01) {
            settings.newMove = settings.move - float(root["move"]);
            settings.move = root["move"];
            hasChanged = true;
        }
        if (root["speed"].is<float>() && abs(settings.speed - float(root["speed"])) > 0.01) {
            settings.speed = root["speed"];
            hasChanged = true;
        }
        if (root["acceleration"].is<float>() &&  abs(settings.acceleration - float(root["acceleration"])) > 0.01) {
            settings.acceleration = root["acceleration"];
            hasChanged = true;
        }
        if (root["status"].is<uint32_t>() && settings.status != root["status"]) {
            settings.status = root["status"];
            hasChanged = true;
        }
        if (root["version"].is<uint8_t>() && settings.version != root["version"]) {
            settings.version = root["version"];
            hasChanged = true;
        }
        // Serial.print("Update : ");
        // Serial.println(float(root["speed"]));
        if (hasChanged) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }

    static void readState(TMC5160Controller *stepper, JsonObject &root) {
        root["isEnabled"] = stepper->enabled;
        if (abs(stepper->getSpeed()) > 0.001) root["direction"] = stepper->getSpeed() >= 0;
        // root["move"] = stepper->move();
        root["speed"] = abs(stepper->getSpeed());
        // Serial.print("Read State : ");
        // Serial.println(float(root["speed"]));
        root["acceleration"] = stepper->getAcceleration();
        root["status"] = stepper->getStatus();
        root["version"] = stepper->driver.version();
        // Serial.println(stepper->getStatus());
    }
};

class StepperControllers
{
public:
    std::vector<StepperControl> steppers;
    static void read(StepperControllers &steppers, JsonObject &root)
    {
        JsonArray jsonArray = root["steppers"].to<JsonArray>();
        for (StepperControl stepper : steppers.steppers) {
            JsonObject obj = jsonArray.add<JsonObject>();
            stepper.read(stepper, obj);
        }
    }
    static StateUpdateResult update(JsonObject &root, StepperControllers &steppers, const String &originId)
    {
        (void)originId;
        JsonArray jsonArray = root["steppers"].as<JsonArray>();
        bool hasChanged = false;
        for (int i = 0; i < min(jsonArray.size(), steppers.steppers.size()); i++) {
            JsonObject obj = jsonArray[i];
            if (StepperControl::update(obj, steppers.steppers[i], originId) == StateUpdateResult::CHANGED) hasChanged = true;
        }
        return hasChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
    }
    static void readState(std::vector<TMC5160Controller*> &steppers, JsonObject &root) {
        JsonArray jsonArray = root["steppers"].to<JsonArray>();
        for (TMC5160Controller *stepper : steppers) {
            JsonObject obj = jsonArray.add<JsonObject>();
            StepperControl::readState(stepper, obj);
        }
    }
};

class StepperControlService : public StatefulService<StepperControllers>
{
public:
    StepperControlService(EventSocket *socket,
                          StepperSettingsService *stepperSettingsService,
                          std::vector<TMC5160Controller*>& steppers,
                          FeaturesService *featuresService);
    void begin();
    void loop();

private:
    EventEndpoint<StepperControllers> _eventEndpoint;
    StepperSettingsService *_stepperSettingsService;
    std::vector<TMC5160Controller*>& _steppers;
    FeaturesService *_featuresService;

    unsigned long lastUpdate = 0;

    void registerConfig();
    void configureArtNet();
    void onConfigUpdated(const String &originId);
    void updateState();
};

#endif
