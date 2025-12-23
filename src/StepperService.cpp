#include <StepperService.h>


JsonRouter<TMC5160Controller> TMC5160ControllerJsonRouter::router = JsonRouter<TMC5160Controller>(
{
    {"config", [](JsonVariant content, TMC5160Controller &controller) {
        return configRouter.parse(content, controller);
    }},
    {"control", [](JsonVariant content, TMC5160Controller &controller) {
        return controlRouter.parse(content, controller);
    }},
},
{
    {"control", [](TMC5160Controller &controller, const JsonVariant target) {
        target["speed"] = controller.getSpeed();
        target["accel"] = controller.getAcceleration();
        target["move"] = 0.;
    }},
    {"diag", [](TMC5160Controller &controller, const JsonVariant target) {
        target["isEnabled"] = controller.isEnabled();
        target["status"] = controller.getStatus();
        target["version"] = controller.driver.version();
    }},
    {"config", [](TMC5160Controller &controller, const JsonVariant target) {
        target["enabled"] = controller.isEnabled();
        target["maxSpeed"] = controller.maxSpeed;
        target["maxAccel"] = controller.maxAccel;
        target["invertDirection"] = controller.driver.shaft();
        target["driverCurrent"] = controller.driver.rms_current();
        target["stepsPerRot"] = controller.stepsPerRotation;
    }}
});

JsonEventRouter<TMC5160Controller> TMC5160ControllerJsonRouter::controlRouter = JsonEventRouter<TMC5160Controller>({
    {"enabled", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<bool>()) {
            if (content.as<bool>() == true) controller.enable();
            else controller.disable();
            return true;
        }
        else return false;
    }},
    {"accel", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<double>()) {
            controller.setAcceleration(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"speed", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<double>()) {
            controller.setSpeed(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"move", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<double>()) {
            controller.setMaxSpeed();
            controller.moveR(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"stop", [](JsonVariant content, TMC5160Controller &controller) {
        controller.setSpeed(0);
        return true;
    }},
});

JsonEventRouter<TMC5160Controller> TMC5160ControllerJsonRouter::configRouter = JsonEventRouter<TMC5160Controller>({
    {"maxSpeed", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<double>()) {
            controller.maxSpeed = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"maxAccel", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<double>()) {
            controller.maxAccel = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"driverCurrent", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<double>()) {
            controller.driver.rms_current(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"enabled", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<bool>()) {
            if (content.as<bool>() == true) controller.enable();
            else controller.disable();
            return true;
        }
        else return false;
    }},
    {"invertDirection", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<bool>()) {
            controller.driver.shaft(content.as<bool>());
            return true;
        }
        else return false;
    }},
    {"stepsPerRot", [](JsonVariant content, TMC5160Controller &controller) {
        if (content.is<int>()) {
            controller.stepsPerRotation = content.as<int>();
            return true;
        }
        else return false;
    }},
});

void StepperService::begin() {
    _httpRouterEndpoint.begin();
    _eventEndpoint.begin();
    _fsPersistence.readFromFS();
}