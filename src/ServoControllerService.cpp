#include <ServoControllerService.h>

JsonRouter<Servo_Driver> ServoControllerJsonRouter::router = JsonRouter<Servo_Driver>(
{
    {"offset", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.setEncoderOffset(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"invert", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<bool>()) {
            controller.encoder.invert = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"enabled", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<bool>()) {
            controller.enabled = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"target", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.setAngle(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"tolerance", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.tolerance = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"P", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.P = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"I", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.I = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"D", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.D = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"S", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.S = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"limits", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<JsonObject>()) {
            auto obj = content.as<JsonObject>();
            if (obj["enabled"].is<bool>()) {
                controller.hasLimits = obj["enabled"].as<bool>();
            }
            if (obj["begin"].is<double>()) {
                controller.limitA = obj["begin"].as<double>();
            }
            if (obj["end"].is<double>()) {
                controller.limitB = obj["end"].as<double>();
            }
            return true;
        }
        return false;
    }},
    {"motor", [](JsonVariant content, Servo_Driver &controller) {
        return MotorDriverJsonRouter::router.parse(content, controller.motor);
    }},
    {"plot", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<bool>()) {
            controller.plot = content.as<bool>();
            return true;
        }
        else return false;
    }},

},
{
    {"position", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.getAngle());
    }},
    {"target", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.targetAngle);
    }},
    {"tolerance", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.tolerance);
    }},
    {"offset", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.encoderOffset);
    }},
    {"enabled", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.enabled);
    }},
    {"invert", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.encoder.invert);
    }},
    {"encoderError", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.encoder.error);
    }},
    {"P", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.P);
    }},
    {"I", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.I);
    }},
    {"D", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.D);
    }},
    {"S", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.S);
    }},
    {"curGain", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.curGain);
    }},
    {"derivative", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.derivative);
    }},
    {"integral", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.integral);
    }},
    {"limits", [](Servo_Driver &controller, const JsonVariant target) {
        target["enabled"] = controller.hasLimits;
        target["begin"] = controller.limitA;
        target["end"] = controller.limitB;
    }},
    {"motor", [](Servo_Driver &controller, const JsonVariant target) {
        MotorDriverJsonRouter::router.serialize(controller.motor, target);
    }},
    {"plot", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.plot);
    }},
});

void ServoControllerService::begin() {
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}
