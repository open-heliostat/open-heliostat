#ifndef ACCELEROMETER_CALIBRATION_SERVICE_H
#define ACCELEROMETER_CALIBRATION_SERVICE_H

#include <Arduino.h>
#include <lib/JsonStateRouter.h>
#include <lib/HttpStateRouterEndpoint.h>
#include <FSPersistence.h>

#include <heliostat.h>
#include <AccelerometerCalibration.h>
#include <adxl345.h>

using JsonStateRouting::JsonRouter;
using JsonStateRouting::JsonSaveManager;

constexpr int ACCEL_CALIB_MAX_SAMPLES = 512;

class AccelCalibService;

struct AccelCalibState {
    bool running = false;
    bool hasCalib = false;
    bool autoMove = true;
    bool useLimits = true;
    float params[ACCEL_ORIENT_PARAM_COUNT] = {0.0f};
    float rms = 0.0f;
    uint32_t sampleCount = 0;
    uint16_t sampleIntervalMs = 200;
    uint16_t settleMs = 700;
    uint8_t azSteps = 12;
    uint8_t elSteps = 6;
    float azMinDeg = 0.0f;
    float azMaxDeg = 360.0f;
    float elMinDeg = 0.0f;
    float elMaxDeg = 90.0f;
    AccelCalibService *service = nullptr;
};

class AccelCalibRouter
{
public:
    static bool route(JsonVariant content, AccelCalibState &state)
    {
        return router.route(content, state);
    }
    static void read(AccelCalibState &state, JsonObject &root)
    {
        router.serialize(state, root);
    }
    static void readForSave(AccelCalibState &state, JsonObject &root)
    {
        getSaveMap(root);
        router.serialize(state, root);
        JsonDocument ref = getSaveMap();
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }
    static StateUpdateResult update(JsonObject &root, AccelCalibState &state, const String &originId)
    {
        (void)originId;
        bool changed = router.parse(root, state);
        if (changed && JsonSaveManager::needsToSave(root, getSaveMap())) {
            return StateUpdateResult::CHANGED;
        }
        return changed ? StateUpdateResult::UNCHANGED : StateUpdateResult::UNCHANGED;
    }
    static const JsonDocument getSaveMap()
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getSaveMap(obj);
        return doc;
    }
    static const void getSaveMap(JsonObject &root)
    {
        root["hasCalib"] = true;
        root["params"] = true;
    }
    static JsonRouter<AccelCalibState> router;
};

class AccelCalibService : public StatefulService<AccelCalibState>
{
public:
    AccelCalibService(PsychicHttpServer *server,
                      FS *fs,
                      SecurityManager *securityManager,
                      HeliostatController &controller,
                      ADXL345 &accelerometer,
                      int sdaPin = -1,
                      int sclPin = -1,
                      uint32_t i2cClockHz = 400000);

    void begin();
    void loop();

    bool captureSample();
    void resetSamples();
    bool solveCalibration();
    void applyOffsets(bool addToExisting = true);

private:
    HttpStateRouterEndpoint<AccelCalibState> _httpEndpoint;
    FSPersistence<AccelCalibState> _fsPersistence;
    AccelCalibRouter _router;

    HeliostatController &_controller;
    ADXL345 &_accelerometer;

    Sample _samples[ACCEL_CALIB_MAX_SAMPLES];
    uint32_t _sampleCount = 0;
    uint32_t _lastSampleMs = 0;
    uint32_t _poseStartMs = 0;
    bool _posePending = false;
    bool _prevRunning = false;
    uint32_t _sweepIndex = 0;

    int _sdaPin;
    int _sclPin;
    uint32_t _i2cClockHz;
};

#endif
