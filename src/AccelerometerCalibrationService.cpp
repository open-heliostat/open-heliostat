#include <AccelerometerCalibrationService.h>
#include <math.h>

namespace {
constexpr float kDeg2Rad = 0.017453292519943295f;
constexpr float kRad2Deg = 57.29577951308232f;
constexpr float kEpsilon = 1e-6f;
}

static double modDeg(double a) {
    return a - 360.0 * floor(a / 360.0);
}

static void computeRange(double minDeg, double maxDeg, double &outMin, double &outMax) {
    outMin = minDeg;
    outMax = maxDeg;
    if (outMax < outMin) {
        double tmp = outMin;
        outMin = outMax;
        outMax = tmp;
    }
}

static void computeLimitsRange(const AbstractController &controller, double &outMin, double &outMax) {
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

JsonRouter<AccelCalibState> AccelCalibRouter::router = JsonRouter<AccelCalibState>(
{
    {"running", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>()) {
            state.running = content.as<bool>();
            return true;
        }
        return false;
    }},
    {"sampleIntervalMs", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<uint16_t>()) {
            state.sampleIntervalMs = content.as<uint16_t>();
            return true;
        }
        return false;
    }},
    {"settleMs", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<uint16_t>()) {
            state.settleMs = content.as<uint16_t>();
            return true;
        }
        return false;
    }},
    {"autoMove", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>()) {
            state.autoMove = content.as<bool>();
            return true;
        }
        return false;
    }},
    {"useLimits", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>()) {
            state.useLimits = content.as<bool>();
            return true;
        }
        return false;
    }},
    {"azSteps", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<uint8_t>()) {
            state.azSteps = max<uint8_t>(content.as<uint8_t>(), 2);
            return true;
        }
        return false;
    }},
    {"elSteps", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<uint8_t>()) {
            state.elSteps = max<uint8_t>(content.as<uint8_t>(), 2);
            return true;
        }
        return false;
    }},
    {"azMinDeg", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<float>()) {
            state.azMinDeg = content.as<float>();
            return true;
        }
        return false;
    }},
    {"azMaxDeg", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<float>()) {
            state.azMaxDeg = content.as<float>();
            return true;
        }
        return false;
    }},
    {"elMinDeg", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<float>()) {
            state.elMinDeg = content.as<float>();
            return true;
        }
        return false;
    }},
    {"elMaxDeg", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<float>()) {
            state.elMaxDeg = content.as<float>();
            return true;
        }
        return false;
    }},
    {"reset", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>() && content.as<bool>()) {
            if (state.service) state.service->resetSamples();
            return true;
        }
        return false;
    }},
    {"capture", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>() && content.as<bool>()) {
            if (state.service) state.service->captureSample();
            return true;
        }
        return false;
    }},
    {"solve", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>() && content.as<bool>()) {
            if (state.service) state.service->solveCalibration();
            return true;
        }
        return false;
    }},
    {"apply", [](JsonVariant content, AccelCalibState &state) {
        if (content.is<bool>() && content.as<bool>()) {
            if (state.service) state.service->applyOffsets(true);
            return true;
        }
        return false;
    }},
},
{
    {"running", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.running);
    }},
    {"hasCalib", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.hasCalib);
    }},
    {"rms", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.rms);
    }},
    {"sampleCount", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.sampleCount);
    }},
    {"sampleIntervalMs", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.sampleIntervalMs);
    }},
    {"settleMs", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.settleMs);
    }},
    {"autoMove", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.autoMove);
    }},
    {"useLimits", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.useLimits);
    }},
    {"azSteps", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.azSteps);
    }},
    {"elSteps", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.elSteps);
    }},
    {"azMinDeg", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.azMinDeg);
    }},
    {"azMaxDeg", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.azMaxDeg);
    }},
    {"elMinDeg", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.elMinDeg);
    }},
    {"elMaxDeg", [](AccelCalibState &state, const JsonVariant target) {
        target.set(state.elMaxDeg);
    }},
    {"params", [](AccelCalibState &state, const JsonVariant target) {
        JsonArray arr = target.to<JsonArray>();
        for (int i = 0; i < ACCEL_ORIENT_PARAM_COUNT; i++) {
            arr.add(state.params[i]);
        }
    }},
    {"paramsDeg", [](AccelCalibState &state, const JsonVariant target) {
        JsonObject obj = target.to<JsonObject>();
        obj["azOffsetDeg"] = state.params[0] * kRad2Deg;
        obj["elOffsetDeg"] = state.params[1] * kRad2Deg;
        obj["rollDeg"] = state.params[2] * kRad2Deg;
        obj["pitchDeg"] = state.params[3] * kRad2Deg;
    }},
});

AccelCalibService::AccelCalibService(PsychicHttpServer *server,
                                     FS *fs,
                                     SecurityManager *securityManager,
                                     HeliostatController &controller,
                                     ADXL345 &accelerometer,
                                     int sdaPin,
                                     int sclPin,
                                     uint32_t i2cClockHz) :
    _httpEndpoint(AccelCalibRouter::read,
                  AccelCalibRouter::update,
                  this,
                  server,
                  "/rest/accelcalib",
                  securityManager),
    _fsPersistence(_router.readForSave,
                   AccelCalibRouter::update,
                   this,
                   fs,
                   "/config/accel_calib.json"),
    _controller(controller),
    _accelerometer(accelerometer),
    _sdaPin(sdaPin),
    _sclPin(sclPin),
    _i2cClockHz(i2cClockHz)
{
    _state.service = this;
}

void AccelCalibService::begin() {
    _httpEndpoint.begin();
    _fsPersistence.readFromFS();
    _accelerometer.begin(_sdaPin, _sclPin, _i2cClockHz);
}

void AccelCalibService::loop() {
    if (!_state.running) {
        _posePending = false;
        _prevRunning = false;
        return;
    }

    if (!_prevRunning) {
        _sweepIndex = 0;
        _posePending = false;
        _prevRunning = true;
    }

    uint32_t now = millis();

    if (_state.autoMove) {
        if (!_posePending) {
            double azMin = 0.0;
            double azMax = 360.0;
            double elMin = 0.0;
            double elMax = 90.0;

            if (_state.useLimits && _controller.azimuthController.hasLimits) {
                computeLimitsRange(_controller.azimuthController, azMin, azMax);
            } else {
                computeRange(_state.azMinDeg, _state.azMaxDeg, azMin, azMax);
                if (fabs(azMax - azMin) < kEpsilon) {
                    azMax = azMin + 10.0;
                }
            }

            if (_state.useLimits && _controller.elevationController.hasLimits) {
                computeLimitsRange(_controller.elevationController, elMin, elMax);
            } else {
                computeRange(_state.elMinDeg, _state.elMaxDeg, elMin, elMax);
                if (fabs(elMax - elMin) < kEpsilon) {
                    elMax = elMin + 10.0;
                }
            }

            uint8_t azSteps = max<uint8_t>(_state.azSteps, 2);
            uint8_t elSteps = max<uint8_t>(_state.elSteps, 2);

            uint32_t totalSteps = static_cast<uint32_t>(azSteps) * static_cast<uint32_t>(elSteps);
            uint32_t index = _sweepIndex % max<uint32_t>(totalSteps, 1);

            uint32_t elIndex = index / azSteps;
            uint32_t azIndex = index % azSteps;
            if (elIndex % 2 == 1) {
                azIndex = (azSteps - 1) - azIndex;
            }

            double azSpan = azMax - azMin;
            double elSpan = elMax - elMin;

            if (azSpan < 0) azSpan += 360.0;
            if (elSpan < 0) elSpan += 360.0;

            double azMaxAdj = azMax;
            if (!_state.useLimits && azSteps > 1) {
                azMaxAdj = azMax - (azSpan / azSteps);
            }

            double azTarget = azSteps <= 1 ? azMin : (azMin + (azMaxAdj - azMin) * (double)azIndex / (double)(azSteps - 1));
            double elTarget = elSteps <= 1 ? elMin : (elMin + (elMax - elMin) * (double)elIndex / (double)(elSteps - 1));

            _controller.setPosition(azTarget, elTarget);
            _poseStartMs = now;
            _posePending = true;
            _sweepIndex++;
        } else if (now - _poseStartMs >= _state.settleMs) {
            captureSample();
            _lastSampleMs = now;
            _posePending = false;
        }
        return;
    }

    if (now - _lastSampleMs >= _state.sampleIntervalMs) {
        captureSample();
        _lastSampleMs = now;
    }
}

bool AccelCalibService::captureSample() {
    if (_sampleCount >= ACCEL_CALIB_MAX_SAMPLES) return false;

    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    if (!_accelerometer.readAcceleration(ax, ay, az)) return false;

    auto pos = _controller.getPosition();
    Sample s;
    s.az = static_cast<float>(pos.azimuth) * kDeg2Rad;
    s.el = static_cast<float>(pos.elevation) * kDeg2Rad;
    s.ax = ax;
    s.ay = ay;
    s.azz = az;

    _samples[_sampleCount] = s;
    _sampleCount++;
    _state.sampleCount = _sampleCount;
    return true;
}

void AccelCalibService::resetSamples() {
    _sampleCount = 0;
    _state.sampleCount = 0;
    _state.rms = 0.0f;
    _state.hasCalib = false;
    callUpdateHandlers("accelcalib");
}

bool AccelCalibService::solveCalibration() {
    if (_sampleCount < 10) {
        return false;
    }

    float rms = 0.0f;
    bool ok = calibrateOrientation(_samples, static_cast<int>(_sampleCount), _state.params, &rms);
    if (!ok) return false;

    _state.rms = rms;
    _state.hasCalib = true;
    callUpdateHandlers("accelcalib");
    return true;
}

void AccelCalibService::applyOffsets(bool addToExisting) {
    if (!_state.hasCalib) return;

    float azOffsetDeg = _state.params[0] * kRad2Deg;
    float elOffsetDeg = _state.params[1] * kRad2Deg;

    double azBase = addToExisting ? _controller.azimuthController.encoderOffset : 0.0;
    double elBase = addToExisting ? _controller.elevationController.encoderOffset : 0.0;

    _controller.azimuthController.setEncoderOffset(azBase + azOffsetDeg);
    _controller.elevationController.setEncoderOffset(elBase + elOffsetDeg);
}
