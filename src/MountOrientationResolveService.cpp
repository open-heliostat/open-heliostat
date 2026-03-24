#include <MountOrientationResolveService.h>

#include <closedloopcontroller.h>
#include <math.h>
#include <orientation_contract.h>
#include <servocontroller.h>

namespace {
constexpr double kResolveServoSpeedDegPerSec = 20.0;
constexpr double kResolveStepperSpeedDegPerSec = 16.0;
constexpr double kReachToleranceDeg = 1.0;

double modDeg(double a) {
    return a - 360.0 * floor(a / 360.0);
}

void computeRange(double minDeg, double maxDeg, double &outMin, double &outMax) {
    outMin = minDeg;
    outMax = maxDeg;
    if (outMax < outMin) {
        double tmp = outMin;
        outMin = outMax;
        outMax = tmp;
    }
}

void computeLimitsRange(const AbstractController &controller, double &outMin, double &outMax) {
    double limitA = modDeg(controller.limitA);
    double limitB = modDeg(controller.limitB);
    double interval = modDeg(limitB - limitA);
    double middle = (limitA + limitB) * 0.5;
    if (controller.limitB < controller.limitA) {
        middle = modDeg(middle + 180.0);
    }
    outMin = modDeg(middle - interval * 0.5);
    outMax = modDeg(middle + interval * 0.5);
    if (interval < 1e-3) {
        outMin = modDeg(limitA);
        outMax = modDeg(limitB);
    }
}

double interpolateDeg(double minDeg, double maxDeg, double fraction) {
    double span = maxDeg - minDeg;
    if (span < 0.0) {
        span += 360.0;
    }
    return modDeg(minDeg + span * fraction);
}

double angularErrorDeg(AbstractController &controller, double targetDeg)
{
    return fabs(controller.angularDistance(targetDeg, controller.getAngle()));
}
}

JsonRouter<MountOrientationResolveState> MountOrientationResolveRouter::router = JsonRouter<MountOrientationResolveState>(
{
    {"running", [](JsonVariant content, MountOrientationResolveState &state) {
        if (!content.is<bool>() || !state.service) {
            return false;
        }
        if (content.as<bool>()) {
            state.service->startRun();
        } else {
            state.service->stopRun();
        }
        return true;
    }},
    {"useLimits", [](JsonVariant content, MountOrientationResolveState &state) {
        if (content.is<bool>()) {
            state.useLimits = content.as<bool>();
            return true;
        }
        return false;
    }},
    {"settleMs", [](JsonVariant content, MountOrientationResolveState &state) {
        if (content.is<uint16_t>()) {
            state.settleMs = max<uint16_t>(content.as<uint16_t>(), 200);
            return true;
        }
        return false;
    }},
    {"samplesPerPose", [](JsonVariant content, MountOrientationResolveState &state) {
        if (content.is<uint8_t>()) {
            state.samplesPerPose = max<uint8_t>(content.as<uint8_t>(), 1);
            return true;
        }
        return false;
    }},
    {"reset", [](JsonVariant content, MountOrientationResolveState &state) {
        if (content.is<bool>() && content.as<bool>() && state.service) {
            state.service->reset();
            return true;
        }
        return false;
    }},
    {"apply", [](JsonVariant content, MountOrientationResolveState &state) {
        if (content.is<bool>() && content.as<bool>() && state.service) {
            return state.service->applyResult();
        }
        return false;
    }}
},
{
    {"running", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.running); }},
    {"useLimits", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.useLimits); }},
    {"hasResult", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.hasResult); }},
    {"observabilityOk", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.observabilityOk); }},
    {"rms", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.rms); }},
    {"completedPoses", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.completedPoses); }},
    {"totalPoses", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.totalPoses); }},
    {"settleMs", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.settleMs); }},
    {"samplesPerPose", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.samplesPerPose); }},
    {"status", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.status); }},
    {"failureReason", [](MountOrientationResolveState &state, const JsonVariant target) { target.set(state.failureReason); }},
    {"result", [](MountOrientationResolveState &state, const JsonVariant target) {
        JsonObject obj = target.to<JsonObject>();
        obj["tiltDeg"] = state.tiltDeg;
        obj["tiltAzimuthDeg"] = state.tiltAzimuthDeg;
        obj["gravityX"] = state.gravityX;
        obj["gravityY"] = state.gravityY;
        obj["gravityZ"] = state.gravityZ;
        obj["sensorRollDeg"] = state.sensorRollDeg;
        obj["sensorPitchDeg"] = state.sensorPitchDeg;
    }}
});

void MountOrientationResolveRouter::read(MountOrientationResolveState &state, JsonObject &root)
{
    router.serialize(state, root);
}

StateUpdateResult MountOrientationResolveRouter::update(JsonObject &root, MountOrientationResolveState &state, const String &originId)
{
    (void)originId;
    return router.parse(root, state) ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}

MountOrientationResolveService::MountOrientationResolveService(HeliostatService &heliostatService,
                                                               HeliostatController &controller,
                                                               ADXL345 &accelerometer) :
    _heliostatService(heliostatService),
    _controller(controller),
    _accelerometer(accelerometer)
{
    _state.service = this;
}

void MountOrientationResolveService::readState(JsonObject &root)
{
    MountOrientationResolveRouter::read(_state, root);
}

StateUpdateResult MountOrientationResolveService::updateState(JsonObject &root, const String &originId)
{
    return MountOrientationResolveRouter::update(root, _state, originId);
}

void MountOrientationResolveService::buildPoseTargets()
{
    double azMin = 0.0;
    double azMax = 360.0;
    double elMin = 10.0;
    double elMax = 80.0;

    if (_state.useLimits && _controller.azimuthController.hasLimits) {
        computeLimitsRange(_controller.azimuthController, azMin, azMax);
    }

    if (_state.useLimits && _controller.elevationController.hasLimits) {
        computeLimitsRange(_controller.elevationController, elMin, elMax);
    } else {
        computeRange(elMin, elMax, elMin, elMax);
    }

    const double azFractions[MOUNT_ORIENTATION_MAX_POSES] = {
        0.00, 0.17, 0.33, 0.50, 0.67, 0.83,
        0.83, 0.67, 0.50, 0.33, 0.17, 0.00,
    };
    const double elFractions[MOUNT_ORIENTATION_MAX_POSES] = {
        0.25, 0.25, 0.25, 0.25, 0.25, 0.25,
        0.70, 0.70, 0.70, 0.70, 0.70, 0.70,
    };

    for (int i = 0; i < MOUNT_ORIENTATION_MAX_POSES; i++) {
        _poseTargets[i].azimuth = interpolateDeg(azMin, azMax, azFractions[i]);
        _poseTargets[i].elevation = elMin + (elMax - elMin) * elFractions[i];
    }
}

void MountOrientationResolveService::startRun()
{
    restoreControllerEnabled();
    _wasControllerEnabled = _controller.enabled;
    _wasAzimuthEnabled = _controller.azimuthController.enabled;
    _wasElevationEnabled = _controller.elevationController.enabled;
    _savedAzimuthStepperSpeed = 0;
    _savedElevationStepperSpeed = 0;
    if (_controller.azimuthController.getType() == 1) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.azimuthController);
        _savedAzimuthStepperSpeed = ctrl->stepper.getMaxSpeed();
    }
    if (_controller.elevationController.getType() == 1) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.elevationController);
        _savedElevationStepperSpeed = ctrl->stepper.getMaxSpeed();
    }
    _controller.enabled = false;
    _controller.azimuthController.enabled = true;
    _controller.elevationController.enabled = true;
    _activePoseIndex = 0;
    _posePending = false;
    _poseSettling = false;
    _state.running = true;
    _state.hasResult = false;
    _state.observabilityOk = false;
    _state.rms = 0.0f;
    _state.sensorRollDeg = 0.0f;
    _state.sensorPitchDeg = 0.0f;
    _state.completedPoses = 0;
    _state.totalPoses = MOUNT_ORIENTATION_MAX_POSES;
    _state.status = "moving";
    _state.failureReason = "";
    buildPoseTargets();
    callUpdateHandlers("orientation-resolve");
}

void MountOrientationResolveService::restoreControllerEnabled()
{
    _controller.enabled = _wasControllerEnabled;
    _controller.azimuthController.enabled = _wasAzimuthEnabled;
    _controller.elevationController.enabled = _wasElevationEnabled;
    if (_controller.azimuthController.getType() == 1 && _savedAzimuthStepperSpeed > 0) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.azimuthController);
        ctrl->stepper.enable();
        ctrl->stepper.setMaxSpeed(_savedAzimuthStepperSpeed);
    }
    if (_controller.elevationController.getType() == 1 && _savedElevationStepperSpeed > 0) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.elevationController);
        ctrl->stepper.enable();
        ctrl->stepper.setMaxSpeed(_savedElevationStepperSpeed);
    }
}

void MountOrientationResolveService::enableAxisActuation()
{
    _controller.azimuthController.enabled = true;
    _controller.elevationController.enabled = true;

    if (_controller.azimuthController.getType() == 1) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.azimuthController);
        ctrl->stepper.enable();
        ctrl->stepper.setMaxSpeed(kResolveStepperSpeedDegPerSec);
    }
    if (_controller.elevationController.getType() == 1) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.elevationController);
        ctrl->stepper.enable();
        ctrl->stepper.setMaxSpeed(kResolveStepperSpeedDegPerSec);
    }
}

void MountOrientationResolveService::stopAxisActuation()
{
    if (_controller.azimuthController.getType() == 2) {
        auto *ctrl = static_cast<Servo_Driver *>(&_controller.azimuthController);
        ctrl->motor.setSpeed(0.0);
    } else if (_controller.azimuthController.getType() == 1) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.azimuthController);
        ctrl->stepper.stop();
        ctrl->stepper.disable();
    }

    if (_controller.elevationController.getType() == 2) {
        auto *ctrl = static_cast<Servo_Driver *>(&_controller.elevationController);
        ctrl->motor.setSpeed(0.0);
    } else if (_controller.elevationController.getType() == 1) {
        auto *ctrl = static_cast<ClosedLoopController *>(&_controller.elevationController);
        ctrl->stepper.stop();
        ctrl->stepper.disable();
    }

    _controller.azimuthController.enabled = false;
    _controller.elevationController.enabled = false;
}

void MountOrientationResolveService::commandPoseTarget(const SphericalCoordinate &target)
{
    enableAxisActuation();

    if (_controller.azimuthController.getType() == 2) {
        auto *ctrl = static_cast<Servo_Driver *>(&_controller.azimuthController);
        ctrl->setAngleWithMaxSpeed(target.azimuth, kResolveServoSpeedDegPerSec);
    } else {
        _controller.azimuthController.setAngle(target.azimuth);
    }

    if (_controller.elevationController.getType() == 2) {
        auto *ctrl = static_cast<Servo_Driver *>(&_controller.elevationController);
        ctrl->setAngleWithMaxSpeed(target.elevation, kResolveServoSpeedDegPerSec);
    } else {
        _controller.elevationController.setAngle(target.elevation);
    }
}

bool MountOrientationResolveService::hasReachedPoseTarget(const SphericalCoordinate &target)
{
    double azTolerance = max(_controller.azimuthController.tolerance, kReachToleranceDeg);
    double elTolerance = max(_controller.elevationController.tolerance, kReachToleranceDeg);
    return angularErrorDeg(_controller.azimuthController, target.azimuth) <= azTolerance
        && angularErrorDeg(_controller.elevationController, target.elevation) <= elTolerance;
}

void MountOrientationResolveService::stopRun(const String &status)
{
    _state.running = false;
    _posePending = false;
    _poseSettling = false;
    _state.status = status;
    stopAxisActuation();
    restoreControllerEnabled();
    callUpdateHandlers("orientation-resolve");
}

void MountOrientationResolveService::reset()
{
    stopRun("idle");
    _state.hasResult = false;
    _state.observabilityOk = false;
    _state.rms = 0.0f;
    _state.tiltDeg = 0.0f;
    _state.tiltAzimuthDeg = 0.0f;
    _state.gravityX = 0.0f;
    _state.gravityY = 0.0f;
    _state.gravityZ = 1.0f;
    _state.sensorRollDeg = 0.0f;
    _state.sensorPitchDeg = 0.0f;
    _state.completedPoses = 0;
    _state.failureReason = "";
    callUpdateHandlers("orientation-resolve");
}

bool MountOrientationResolveService::captureCurrentPoseObservation(MountOrientationObservation &observation)
{
    if (_controller.azimuthController.encoder.error || _controller.elevationController.encoder.error) {
        return false;
    }

    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    uint8_t collected = 0;
    uint8_t targetSamples = max<uint8_t>(_state.samplesPerPose, 1);

    for (uint8_t i = 0; i < targetSamples; i++) {
        float sx = 0.0f;
        float sy = 0.0f;
        float sz = 0.0f;
        if (!_accelerometer.readAcceleration(sx, sy, sz)) {
            return false;
        }
        ax += sx;
        ay += sy;
        az += sz;
        collected++;
        delay(5);
    }

    if (collected == 0) {
        return false;
    }

    auto pos = _controller.getPosition();
    observation.azDeg = static_cast<float>(pos.azimuth);
    observation.elDeg = static_cast<float>(pos.elevation);
    observation.ax = ax / collected;
    observation.ay = ay / collected;
    observation.azz = az / collected;
    return true;
}

void MountOrientationResolveService::failRun(const String &reason)
{
    _state.failureReason = reason;
    _state.status = "failed";
    _state.running = false;
    _posePending = false;
    restoreControllerEnabled();
    callUpdateHandlers("orientation-resolve");
}

void MountOrientationResolveService::finishSolve()
{
    _state.observabilityOk = checkMountOrientationObservability(_observations, _state.completedPoses);
    if (!_state.observabilityOk) {
        failRun("Insufficient azimuth/elevation coverage");
        return;
    }

    MountOrientationEstimate estimate;
    if (!estimateMountOrientation(_observations, _state.completedPoses, &estimate)) {
        failRun("Failed to estimate mount orientation");
        return;
    }

    _state.tiltDeg = estimate.tiltDeg;
    _state.tiltAzimuthDeg = estimate.tiltAzimuthDeg;
    _state.gravityX = estimate.gravityX;
    _state.gravityY = estimate.gravityY;
    _state.gravityZ = estimate.gravityZ;
    _state.sensorRollDeg = estimate.sensorRollDeg;
    _state.sensorPitchDeg = estimate.sensorPitchDeg;
    _state.rms = estimate.rms;
    _state.hasResult = true;
    _state.running = false;
    _state.status = "solved";
    stopAxisActuation();
    restoreControllerEnabled();
    callUpdateHandlers("orientation-resolve");
}

bool MountOrientationResolveService::applyResult()
{
    if (!_state.hasResult) {
        return false;
    }

    _controller.tiltDeg = OrientationContract::clampRange(_state.tiltDeg,
                                                          OrientationContract::kMinTiltDeg,
                                                          OrientationContract::kMaxTiltDeg);
    _controller.tiltAzimuthDeg = OrientationContract::clampRange(_state.tiltAzimuthDeg,
                                                                 OrientationContract::kMinTiltAzimuthDeg,
                                                                 OrientationContract::kMaxTiltAzimuthDeg);
    _state.status = "applied";
    _heliostatService.callUpdateHandlers("orientation-resolve");
    callUpdateHandlers("orientation-resolve");
    return true;
}

void MountOrientationResolveService::loop()
{
    if (!_state.running) {
        return;
    }

    if (_activePoseIndex >= MOUNT_ORIENTATION_MAX_POSES) {
        finishSolve();
        return;
    }

    uint32_t now = millis();
    if (!_posePending) {
        commandPoseTarget(_poseTargets[_activePoseIndex]);
        _poseStartMs = now;
        _posePending = true;
        _poseSettling = false;
        _state.status = "moving";
        callUpdateHandlers("orientation-resolve");
        return;
    }

    if (!_poseSettling) {
        if (!hasReachedPoseTarget(_poseTargets[_activePoseIndex])) {
            return;
        }
        stopAxisActuation();
        _poseStartMs = now;
        _poseSettling = true;
        _state.status = "settling";
        callUpdateHandlers("orientation-resolve");
        return;
    }

    if (now - _poseStartMs < _state.settleMs) {
        return;
    }

    _state.status = "sampling";
    MountOrientationObservation observation = {};
    if (!captureCurrentPoseObservation(observation)) {
        failRun("Failed to capture stable accelerometer sample");
        return;
    }

    _observations[_activePoseIndex] = observation;
    _activePoseIndex++;
    _state.completedPoses = _activePoseIndex;
    _posePending = false;
    callUpdateHandlers("orientation-resolve");

    if (_activePoseIndex >= MOUNT_ORIENTATION_MAX_POSES) {
        finishSolve();
    }
}