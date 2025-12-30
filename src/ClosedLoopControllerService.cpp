#include <ClosedLoopControllerService.h>

JsonRouter<ClosedLoopController> ClosedLoopControllerJsonRouter::router = JsonRouter<ClosedLoopController>(
{
    {"calibration", [](JsonVariant content, ClosedLoopController &controller) {
        return calibrationRouter.parse(content, controller);
    }},
    {"offset", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<double>()) {
            controller.setEncoderOffset(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"limits", [](JsonVariant content, ClosedLoopController &controller) {
        return limitsRouter.parse(content, controller);
    }},
    {"invert", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<bool>()) {
            controller.encoder.invert = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"enabled", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<bool>()) {
            controller.enabled = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"target", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<double>()) {
            controller.setAngle(content.as<double>());
            return true;
        }
        else return false;
    }},
    {"tolerance", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<double>()) {
            controller.tolerance = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"stepper", [](JsonVariant content, ClosedLoopController &controller) {
        return TMC5160ControllerJsonRouter::router.parse(content, controller.stepper);
    }},
},
{
    {"position", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.getAngle());
    }},
    {"target", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.getTarget());
    }},
    {"tolerance", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.tolerance);
    }},
    {"offset", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.encoderOffset);
    }},
    {"enabled", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.enabled);
    }},
    {"invert", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.encoder.invert);
    }},
    {"encoderError", [](ClosedLoopController &controller, const JsonVariant target) {
        target.set(controller.encoder.error);
    }},
    {"limits", [](ClosedLoopController &controller, const JsonVariant target) {
        target["enabled"] = controller.hasLimits;
        target["begin"] = controller.limitA;
        target["end"] = controller.limitB;
    }},
    {"calibration", [](ClosedLoopController &controller, const JsonVariant target) {
        target["running"].set(controller.calibrationRunning);
        target["enabled"].set(controller.hasCalibration);
        target["steps"].set(controller.calibrationSteps);
        target["speed"].set(controller.calibrationSpeed);
        target["decay"].set(controller.calibrationDecay);
        if (target["offsets"].is<JsonVariant>()) {
            auto array = target["offsets"].to<JsonArray>();
            copyArray(controller.calibrationOffsets, array);
        }
    }},
    {"stepper", [](ClosedLoopController &controller, const JsonVariant target) {
        if (target.is<JsonObject>()) TMC5160ControllerJsonRouter::router.serialize(controller.stepper, target);
    }},
});

JsonEventRouter<ClosedLoopController> ClosedLoopControllerJsonRouter::calibrationRouter = JsonEventRouter<ClosedLoopController>({
    {"start", [](JsonVariant content, ClosedLoopController &controller) {
        controller.startCalibration();
        return true;
    }},
    {"stop", [](JsonVariant content, ClosedLoopController &controller) {
        controller.stopCalibration();
        return true;
    }},
    {"reset", [](JsonVariant content, ClosedLoopController &controller) {
        controller.resetCalibration();
        return true;
    }},
    {"speed", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<int>()) {
            controller.setCalibrationSpeed(content.as<int>());
            return true;
        }
        else return false;
    }},
    {"decay", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<double>()) {
            controller.calibrationDecay = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"enabled", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<bool>()) {
            controller.hasCalibration = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"running", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<bool>()) {
            if (content.as<bool>()) controller.startCalibration();
            else controller.stopCalibration();
            return true;
        }
        else return false;
    }},
    {"offsets", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<JsonArray>()) {
            auto array = content.as<JsonArray>();
            if (array.size() == controller.calibrationSteps) {
                copyArray(array, controller.calibrationOffsets);
                return true;
            }
        }
        return false;
    }},
});

JsonEventRouter<ClosedLoopController> ClosedLoopControllerJsonRouter::limitsRouter = JsonEventRouter<ClosedLoopController>({
    {"enabled", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<bool>()) {
            controller.hasLimits = content.as<bool>();
            return true;
        }
        else return false;
    }},
    {"begin", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<double>()) {
            controller.limitA = content.as<double>();
            return true;
        }
        else return false;
    }},
    {"end", [](JsonVariant content, ClosedLoopController &controller) {
        if (content.is<double>()) {
            controller.limitB = content.as<double>();
            return true;
        }
        else return false;
    }}
});

void ClosedLoopControllerService::begin() {
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}