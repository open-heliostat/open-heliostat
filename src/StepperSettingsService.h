#ifndef StepperSettingsService_h
#define StepperSettingsService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <tmcdriver.h>
#include <vector>

#define STEPPER_SETTINGS_FILE "/config/stepperSettings.json"
#define STEPPER_SETTINGS_EVENT "steppersettings"

class StepperSettings
{
public:
    String name;
    bool enableOnStart;
    bool invertDirection;
    int32_t maxSpeed;
    uint32_t maxAcceleration;
    uint16_t current;

    static void read(StepperSettings &settings, JsonObject &root)
    {
        root["name"] = settings.name;
        root["enableOnStart"] = settings.enableOnStart;
        root["invertDirection"] = settings.invertDirection;
        root["maxSpeed"] = settings.maxSpeed;
        root["maxAcceleration"] = settings.maxAcceleration;
        root["current"] = settings.current;
    }

    static StateUpdateResult update(JsonObject &root, StepperSettings &settings, const String &originId)
    {   
        (void)originId;
        settings.name = root["name"] | "Stepper";
        settings.enableOnStart = root["enableOnStart"] | false;
        settings.invertDirection = root["invertDirection"] | false;
        settings.maxSpeed = root["maxSpeed"].as<double>();
        settings.maxAcceleration = root["maxAcceleration"].as<double>();
        settings.current = root["current"].as<double>();
        Serial.println(root["current"].as<String>());
        return StateUpdateResult::CHANGED;
    }

    static void readState(TMC5160Controller &stepper, JsonObject &root) {
        root["invertDirection"] = stepper.driver.shaft();
        root["maxSpeed"] = stepper.getMaxSpeed();
        root["maxAcceleration"] = stepper.getAcceleration();
        root["current"] = stepper.driver.rms_current();
    }
};

class MultiStepperSettings
{
public:
    std::vector<StepperSettings> settings;
    static void read(MultiStepperSettings &settings, JsonObject &root)
    {
        JsonArray jsonArray = root["steppers"].to<JsonArray>();
        for (StepperSettings stepper : settings.settings) {
            JsonObject obj = jsonArray.add<JsonObject>();
            stepper.read(stepper, obj);
        }
    }
    static StateUpdateResult update(JsonObject &root, MultiStepperSettings &settings, const String &originId)
    {
        (void)originId;
        JsonArray jsonArray = root["steppers"].as<JsonArray>();
        bool hasChanged = false;
        for (int i = 0; i < min(jsonArray.size(), settings.settings.size()); i++) {
            JsonObject obj = jsonArray[i];
            if (StepperSettings::update(obj, settings.settings[i], originId) == StateUpdateResult::CHANGED) hasChanged = true;
        }
        return hasChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
    }
};

class StepperSettingsService : public StatefulService<MultiStepperSettings>
{
public:
    StepperSettingsService(EventSocket *socket, 
                           FS *fs,
                           std::vector<TMC5160Controller*>& steppers);
    void begin();
    void updateState();
    int32_t getMaxSpeed();
    int32_t getMaxAccel();
    MultiStepperSettings getState();

private:
    EventEndpoint<MultiStepperSettings> _eventEndpoint;
    FSPersistence<MultiStepperSettings> _fsPersistence;
    std::vector<TMC5160Controller*>& _steppers;

    void onConfigUpdated();
};

#endif
