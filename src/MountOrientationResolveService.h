#ifndef MOUNT_ORIENTATION_RESOLVE_SERVICE_H
#define MOUNT_ORIENTATION_RESOLVE_SERVICE_H

#include <Arduino.h>
#include <StatefulService.h>
#include <lib/JsonStateRouter.h>

#include <HeliostatService.h>
#include <MountOrientationResolve.h>
#include <adxl345.h>
#include <heliostat.h>

using JsonStateRouting::JsonRouter;

constexpr int MOUNT_ORIENTATION_MAX_POSES = 12;

class MountOrientationResolveService;

struct MountOrientationResolveState {
    bool running = false;
    bool useLimits = true;
    bool hasResult = false;
    bool observabilityOk = false;
    float rms = 0.0f;
    float tiltDeg = 0.0f;
    float tiltAzimuthDeg = 0.0f;
    float gravityX = 0.0f;
    float gravityY = 0.0f;
    float gravityZ = 1.0f;
    float sensorRollDeg = 0.0f;
    float sensorPitchDeg = 0.0f;
    uint8_t completedPoses = 0;
    uint8_t totalPoses = MOUNT_ORIENTATION_MAX_POSES;
    uint16_t settleMs = 900;
    uint8_t samplesPerPose = 10;
    String status = "idle";
    String failureReason = "";
    MountOrientationResolveService *service = nullptr;
};

class MountOrientationResolveRouter
{
public:
    static void read(MountOrientationResolveState &state, JsonObject &root);
    static StateUpdateResult update(JsonObject &root, MountOrientationResolveState &state, const String &originId);
    static JsonRouter<MountOrientationResolveState> router;
};

class MountOrientationResolveService : public StatefulService<MountOrientationResolveState>
{
public:
    MountOrientationResolveService(HeliostatService &heliostatService,
                                   HeliostatController &controller,
                                   ADXL345 &accelerometer);

    void loop();
    void startRun();
    void stopRun(const String &status = "cancelled");
    void reset();
    bool applyResult();
    void readState(JsonObject &root);
    StateUpdateResult updateState(JsonObject &root, const String &originId);

private:
    HeliostatService &_heliostatService;
    HeliostatController &_controller;
    ADXL345 &_accelerometer;

    bool _posePending = false;
    bool _poseSettling = false;
    bool _wasControllerEnabled = false;
    bool _wasAzimuthEnabled = false;
    bool _wasElevationEnabled = false;
    uint32_t _savedAzimuthStepperSpeed = 0;
    uint32_t _savedElevationStepperSpeed = 0;
    uint32_t _poseStartMs = 0;
    uint8_t _activePoseIndex = 0;
    SphericalCoordinate _poseTargets[MOUNT_ORIENTATION_MAX_POSES] = {};
    MountOrientationObservation _observations[MOUNT_ORIENTATION_MAX_POSES] = {};

    void buildPoseTargets();
    bool captureCurrentPoseObservation(MountOrientationObservation &observation);
    void finishSolve();
    void failRun(const String &reason);
    void restoreControllerEnabled();
    void enableAxisActuation();
    void stopAxisActuation();
    void commandPoseTarget(const SphericalCoordinate &target);
    bool hasReachedPoseTarget(const SphericalCoordinate &target);
};

#endif