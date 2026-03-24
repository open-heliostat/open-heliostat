#ifndef MOUNT_ORIENTATION_RESOLVE_H
#define MOUNT_ORIENTATION_RESOLVE_H

#include <Arduino.h>

struct MountOrientationObservation {
    float azDeg;
    float elDeg;
    float ax;
    float ay;
    float azz;
};

struct MountOrientationEstimate {
    float gravityX = 0.0f;
    float gravityY = 0.0f;
    float gravityZ = 1.0f;
    float tiltDeg = 0.0f;
    float tiltAzimuthDeg = 0.0f;
    float rms = 0.0f;
    float sensorRollDeg = 0.0f;
    float sensorPitchDeg = 0.0f;
    uint32_t usedCount = 0;
};

bool checkMountOrientationObservability(const MountOrientationObservation *observations, int count);
bool estimateMountOrientation(const MountOrientationObservation *observations, int count, MountOrientationEstimate *outEstimate);

#endif