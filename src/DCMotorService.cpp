#include <DCMotorService.h>

JsonRouter<Motor_Driver> MotorDriverJsonRouter::router = JsonRouter<Motor_Driver>(
{
    {"config", [](JsonVariant content, Motor_Driver &controller) {
        return configRouter.parse(content, controller);
    }},
    {"control", [](JsonVariant content, Motor_Driver &controller) {
        return controlRouter.parse(content, controller);
    }},
},
{
    {"control", [](Motor_Driver &controller, const JsonVariant target) {
        target["speed"] = controller.getSpeed();
        target["duty"] = controller.getDuty();
        target["direction"] = controller.getDirection();
    }},
    {"config", [](Motor_Driver &controller, const JsonVariant target) {
        target["minVal"] = controller.getMin();
        target["shape"] = controller.shape;
        target["enable"] = controller.isRunning;
        target["invert"] = controller.invert;
    }}
});

JsonEventRouter<Motor_Driver> MotorDriverJsonRouter::controlRouter = JsonEventRouter<Motor_Driver>({
    {"speed", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<double>()) {
            controller.setSpeed(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"duty", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<int>()) {
            controller.setDuty(content.as<int>());
            return true;
        }
        else return false;
    }},
    {"direction", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<bool>()) {
            controller.setDirection(content.as<bool>());
            return true;
        }
        else return false;
    }},
});

JsonEventRouter<Motor_Driver> MotorDriverJsonRouter::configRouter = JsonEventRouter<Motor_Driver>({
    {"minVal", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<double>()) {
            controller.setMin(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"shape", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<double>()) {
            controller.shape = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"enable", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<bool>()) {
            controller.isRunning = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"invert", [](JsonVariant content, Motor_Driver &controller) {
        if (content.is<bool>()) {
            controller.invert = content.as<bool>();
            return true;
        }
        else return false;
    }},
});

void DCMotorService::begin() {
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}
