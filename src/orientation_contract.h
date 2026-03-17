#ifndef ORIENTATION_CONTRACT_H
#define ORIENTATION_CONTRACT_H

#include <ArduinoJson.h>
#include <algorithm>

namespace OrientationContract {

constexpr double kMinTiltDeg = -90.0;
constexpr double kMaxTiltDeg = 90.0;
constexpr double kMinTiltAzimuthDeg = 0.0;
constexpr double kMaxTiltAzimuthDeg = 360.0;

inline double clampRange(double value, double minValue, double maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

inline bool applyMountOrientationPatch(JsonVariant content, double &tiltDeg, double &tiltAzimuthDeg)
{
    if (!content.is<JsonObject>()) {
        return false;
    }

    JsonObject obj = content.as<JsonObject>();
    const bool hasTiltDeg = obj["tiltDeg"].is<JsonVariant>();
    const bool hasTiltAzimuthDeg = obj["tiltAzimuthDeg"].is<JsonVariant>();

    double nextTiltDeg = tiltDeg;
    double nextTiltAzimuthDeg = tiltAzimuthDeg;

    if (hasTiltDeg) {
        if (!obj["tiltDeg"].is<double>()) {
            return false;
        }
        nextTiltDeg = clampRange(obj["tiltDeg"].as<double>(), kMinTiltDeg, kMaxTiltDeg);
    }

    if (hasTiltAzimuthDeg) {
        if (!obj["tiltAzimuthDeg"].is<double>()) {
            return false;
        }
        nextTiltAzimuthDeg = clampRange(obj["tiltAzimuthDeg"].as<double>(), kMinTiltAzimuthDeg, kMaxTiltAzimuthDeg);
    }

    tiltDeg = nextTiltDeg;
    tiltAzimuthDeg = nextTiltAzimuthDeg;
    return true;
}

inline void writeMountOrientation(JsonObject obj, double tiltDeg, double tiltAzimuthDeg)
{
    obj["tiltDeg"] = tiltDeg;
    obj["tiltAzimuthDeg"] = tiltAzimuthDeg;
}

inline void addMountOrientationSaveMap(JsonObject root)
{
    root["mountOrientation"]["tiltDeg"] = true;
    root["mountOrientation"]["tiltAzimuthDeg"] = true;
}

} // namespace OrientationContract

#endif
