#include <StepperControlService.h>

StepperControlService::StepperControlService(EventSocket *socket,
                                             StepperSettingsService *stepperSettingsService,
                                             std::vector<TMC5160Controller*>& steppers,
                                             FeaturesService *featuresService) :
                                                    _eventEndpoint(StepperControllers::read,
                                                                    StepperControllers::update,
                                                                    this,
                                                                    socket,
                                                                    STEPPER_CONTROL_EVENT),
                                                    _stepperSettingsService(stepperSettingsService),
                                                    _steppers(steppers),
                                                    _featuresService(featuresService)
{
    // configure update handler for when the stepper settings change
    // _stepperSettingsService->addUpdateHandler([&](const String &originId)
    //                                             { onConfigUpdated(originId); },
    //                                             false);

    for (TMC5160Controller *s : steppers) {
        StepperControl state = StepperControl();
        _state.steppers.push_back(state);
    }

    addUpdateHandler([&](const String &originId)
                     { onConfigUpdated(originId); },
                     false);
    
    _featuresService->addFeature("stepper", true);
}

void StepperControlService::begin()
{
    _eventEndpoint.begin();
    updateState();
    onConfigUpdated("begin");
}

void StepperControlService::loop() {
    unsigned long now = millis();
    if (now - lastUpdate > 500) {
        updateState();
        lastUpdate = now;
    }
}

void StepperControlService::onConfigUpdated(const String &originId)
{
    if (originId != "driver") {
        for (int i = 0; i < _steppers.size(); i++) {
            TMC5160Controller *stepper = _steppers[i];
            StepperControl state = _state.steppers[i];
            if (state.isEnabled & !stepper->enabled) stepper->enable();
            else if(!state.isEnabled & stepper->enabled) stepper->disable();
            stepper->setAcceleration(state.acceleration);
            stepper->setSpeed(state.direction ? state.speed : -state.speed);
            if (abs(state.newMove)>0.001) {
                stepper->move(state.newMove*360.);
                state.newMove = 0;
            }
        }
    }
}

void StepperControlService::updateState() {
    JsonDocument json;
    JsonObject jsonObject = json.to<JsonObject>();
    _state.readState(_steppers, jsonObject);
    update(jsonObject, _state.update, "driver");
}