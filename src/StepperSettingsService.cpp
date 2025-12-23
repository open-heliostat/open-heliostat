#include <StepperSettingsService.h>

StepperSettingsService::StepperSettingsService(EventSocket *socket,
                                               FS *fs,
                                               std::vector<TMC5160Controller*>& steppers) :                  
                                                        _eventEndpoint(MultiStepperSettings::read,
                                                                        MultiStepperSettings::update,
                                                                        this,
                                                                        socket,
                                                                        STEPPER_SETTINGS_EVENT),
                                                        _fsPersistence(MultiStepperSettings::read, MultiStepperSettings::update, this, fs, STEPPER_SETTINGS_FILE),
                                                        _steppers(steppers)
{

    for (TMC5160Controller *s : steppers) {
        StepperSettings settings = StepperSettings();
        _state.settings.push_back(settings);
    }

    // configure settings service update handler to update LED state
    addUpdateHandler([&](const String &originId)
                     { onConfigUpdated(); },
                     false);
}

void StepperSettingsService::begin()
{
    _eventEndpoint.begin();
    _fsPersistence.readFromFS();
    // _stepper->init();
    // onConfigUpdated();
    for (int i = 0; i < _steppers.size(); i++) {
        if (_state.settings[i].enableOnStart) _steppers[i]->enable();
        else _steppers[i]->disable();
    }
    updateState();
}

// int32_t StepperSettingsService::getMaxSpeed() {
//     return _state.maxSpeed;
// }

// int32_t StepperSettingsService::getMaxAccel() {
//     return _state.maxAcceleration;
// }

MultiStepperSettings StepperSettingsService::getState() {
    return _state;
}

void StepperSettingsService::onConfigUpdated()
{
    for (int i = 0; i < _steppers.size(); i++) {        
        auto settings = _state.settings[i];
        auto stepper = *_steppers[i];
        stepper.maxSpeed = settings.maxSpeed;
        stepper.maxAccel = settings.maxAcceleration;
        stepper.driver.rms_current(settings.current);
        stepper.driver.shaft(settings.invertDirection);
    }
}

void StepperSettingsService::updateState() 
{
    for (int i = 0; i < _steppers.size(); i++) {           
        auto settings = _state.settings[i];
        auto stepper = *_steppers[i];  
        JsonDocument json;
        JsonObject jsonObject = json.to<JsonObject>();
        settings.readState(stepper, jsonObject);
        update(jsonObject, _state.update, "timer");
    }
}