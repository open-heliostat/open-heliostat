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
    {"ramp", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<double>()) {
            controller.outputRamp = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"autotune", [](JsonVariant content, Servo_Driver &controller) {
        if (content.is<JsonObject>()) {
            auto obj = content.as<JsonObject>();
            if (obj["start"].is<bool>() && obj["start"].as<bool>()) {
                double amp = obj["amp"].is<double>() ? obj["amp"].as<double>() : controller.autoTuneAmp;
                double band = obj["band"].is<double>() ? obj["band"].as<double>() : controller.autoTuneBand;
                controller.beginAutoTune(amp, band);
                return true;
            }
            if (obj["cancel"].is<bool>() && obj["cancel"].as<bool>()) {
                controller.cancelAutoTune();
                return true;
            }
        }
        return false;
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
        target.set(controller.getTarget());
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
    {"ramp", [](Servo_Driver &controller, const JsonVariant target) {
        target.set(controller.outputRamp);
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
    {"autotune", [](Servo_Driver &controller, const JsonVariant target) {
        target["active"] = controller.isAutoTuneActive();
        target["done"] = controller.isAutoTuneDone();
        target["amp"] = controller.autoTuneAmp;
        target["band"] = controller.autoTuneBand;
        target["Ku"] = controller.tunedKu;
        target["Tu"] = controller.tunedTu;
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
