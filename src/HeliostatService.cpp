#include <HeliostatService.h>
#include "TimeLib.h"
#include <time.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// Build a UTC epoch without depending on the currently configured timezone
time_t makeUtcTime(struct tm tmValue) {
#if defined(__USE_MISC) || defined(__GNU_SOURCE) || defined(_BSD_SOURCE)
    // timegm is available in newlib used by ESP32; it interprets tm as UTC
    return timegm(&tmValue);
#else
    char previousTz[64] = {0};
    const char *tz = getenv("TZ");
    if (tz) {
        strncpy(previousTz, tz, sizeof(previousTz) - 1);
    }

    setenv("TZ", "UTC0", 1);
    tzset();
    time_t utcEpoch = mktime(&tmValue);

    if (previousTz[0] != '\0') {
        setenv("TZ", previousTz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return utcEpoch;
#endif
}

long currentTzOffsetSeconds(time_t now) {
    struct tm localTm;
    struct tm gmTm;
    localtime_r(&now, &localTm);
    gmtime_r(&now, &gmTm);

    time_t localAsEpoch = mktime(&localTm);
    time_t gmAsEpoch = mktime(&gmTm);
    return static_cast<long>(difftime(localAsEpoch, gmAsEpoch));
}

void formatIso8601(char *buffer, size_t len, const struct tm &tmValue, const char *suffix) {
    snprintf(buffer, len, "%04d-%02d-%02dT%02d:%02d:%02d%s",
             tmValue.tm_year + 1900,
             tmValue.tm_mon + 1,
             tmValue.tm_mday,
             tmValue.tm_hour,
             tmValue.tm_min,
             tmValue.tm_sec,
             suffix);
}

}

JsonRouter<HeliostatController> HeliostatControllerJsonRouter::router = JsonRouter<HeliostatController>(
{
    {"azimuth", [&](JsonVariant content, HeliostatController &controller) {
        if (controller.azimuthController.getType() == 1) {
            auto* azimuthCtrl = static_cast<ClosedLoopController*>(&controller.azimuthController);
            return ClosedLoopControllerJsonRouter::router.parse(content, *azimuthCtrl);
        }
        else if (controller.azimuthController.getType() == 2) {
            auto* servoCtrl = static_cast<Servo_Driver*>(&controller.azimuthController);
            return ServoControllerJsonRouter::router.parse(content, *servoCtrl);
        }
        return false;
    }},
    {"elevation", [&](JsonVariant content, HeliostatController &controller) {
        if (controller.elevationController.getType() == 1) {
            auto* elevationCtrl = static_cast<ClosedLoopController*>(&controller.elevationController);
            return ClosedLoopControllerJsonRouter::router.parse(content, *elevationCtrl);
        }
        else if (controller.elevationController.getType() == 2) {
            auto* servoCtrl = static_cast<Servo_Driver*>(&controller.elevationController);
            return ServoControllerJsonRouter::router.parse(content, *servoCtrl);
        }
        return false;
    }},
    {"sourcesMap", [&](JsonVariant content, HeliostatController &controller) {
        return updateDirectionsMap(content.as<JsonObject>(), controller.targetsMap);
    }},
    {"currentTarget", [&](JsonVariant content, HeliostatController &controller) {
        if (content.is<String>()) {
            controller.currentTarget = content.as<String>();
            return true;
        }
        return false;
    }},
    {"currentSource", [&](JsonVariant content, HeliostatController &controller) {
        if (content.is<String>()) {
            controller.currentSource = content.as<String>();
            return true;
        }
        return false;
    }},
    {"add", [&](JsonVariant content, HeliostatController &controller) {
        JsonObject obj = content.as<JsonObject>();
        controller.targetsMap.insert({obj["name"] | "New target", {obj["azimuth"] | 180., obj["elevation"] | 30.}});
        return true;
    }},
    {"remove", [&](JsonVariant content, HeliostatController &controller) {
        return controller.deleteTarget(content.as<String>());
    }},
    {"rename", [&](JsonVariant content, HeliostatController &controller) {
        return controller.renameTarget(content["oldName"].as<String>(), content["newName"].as<String>());
    }},
    {"set", [&](JsonVariant content, HeliostatController &controller) {
        return controller.setTarget(content["name"].as<String>(), content["azimuth"].as<double>(), content["elevation"].as<double>());
    }},
    {"sunTracker", [&](JsonVariant content, HeliostatController &controller) {
        if (content.is<JsonObject>()) {
            JsonObject obj = content.as<JsonObject>();
            if (obj["latitude"].is<double>()) controller.latitude = obj["latitude"].as<double>();
            if (obj["longitude"].is<double>()) controller.longitude = obj["longitude"].as<double>();
            if (obj["getFromGPS"].is<JsonVariant>()) controller.getLocationFromGPS();
            if (obj["timeIso"].is<const char*>()) {
                const char *iso = obj["timeIso"].as<const char*>();
                struct tm tmValue = {0};
                if (strptime(iso, "%Y-%m-%dT%H:%M:%S", &tmValue) != nullptr) {
                    tmValue.tm_isdst = 0;
                    time_t manualTime = makeUtcTime(tmValue);

                    // Update TimeLib
                    setTime(tmValue.tm_hour, tmValue.tm_min, tmValue.tm_sec,
                            tmValue.tm_mday, tmValue.tm_mon + 1, tmValue.tm_year + 1900);

                    struct timeval tv = {.tv_sec = manualTime, .tv_usec = 0};
                    settimeofday(&tv, nullptr);
                }
            }
            if (obj["time"].is<JsonObject>()) {
                JsonObject timeObj = obj["time"];
                int year = timeObj["year"].as<int>() | 0;
                int month = timeObj["month"].as<int>() | 1;
                int day = timeObj["day"].as<int>() | 1;
                int hour = timeObj["hour"].as<int>() | 0;
                int minute = timeObj["minute"].as<int>() | 0;
                int second = timeObj["second"].as<int>() | 0;
                
                // Update TimeLib time (expects day, month, year ordering)
                setTime(hour, minute, second, day, month, year);

                // Treat incoming values as UTC so local TZ/DST does not skew the date
                struct tm timeinfo = {0};
                timeinfo.tm_year = year - 1900;  // tm_year is years since 1900
                timeinfo.tm_mon = month - 1;     // tm_mon is 0-11
                timeinfo.tm_mday = day;
                timeinfo.tm_hour = hour;
                timeinfo.tm_min = minute;
                timeinfo.tm_sec = second;
                timeinfo.tm_isdst = 0;
                time_t manualTime = makeUtcTime(timeinfo);
                struct timeval tv = {.tv_sec = manualTime, .tv_usec = 0};
                settimeofday(&tv, nullptr);
            }
            
            return true;
        }
        return false;
    }},
    {"longitude", [&](JsonVariant content, HeliostatController &controller) {
        if (content.is<double>()) {
            controller.longitude = content.as<double>();
            return true;
        }
        return false;
    }},
},
{
    {"sourcesMap", [&](HeliostatController &controller, JsonVariant content)  {
        JsonObject obj = content.to<JsonObject>();
        readDirectionsMap(controller.targetsMap, obj);
    }},
    {"currentTarget", [&](HeliostatController &controller, JsonVariant content)  {
        content.set(controller.currentTarget);
    }},
    {"currentSource", [&](HeliostatController &controller, JsonVariant content)  {
        content.set(controller.currentSource);
    }},
    {"sunTracker", [&](HeliostatController &controller, JsonVariant content) {
        JsonObject obj = content.to<JsonObject>();
        time_t now = time(nullptr);
        long tzOffsetSeconds = currentTzOffsetSeconds(now);
        int tzOffsetMinutes = static_cast<int>(tzOffsetSeconds / 60);

        obj["latitude"] = controller.latitude;
        obj["longitude"] = controller.longitude;
        obj["isTimeSet"] = controller.isTimeSet();
        obj["azimuth"] = controller.getSolarPosition().azimuth;
        obj["elevation"] = controller.getSolarPosition().elevation;
        obj["timestamp"] = static_cast<long>(now);

        const char *tz = getenv("TZ");
        obj["tz"] = tz ? tz : "UTC";
        obj["offsetMinutes"] = tzOffsetMinutes;

        char utcIso[26];
        char localIso[32];
        char offsetSuffix[7];

        struct tm utcTm;
        struct tm localTm;
        gmtime_r(&now, &utcTm);
        localtime_r(&now, &localTm);

        formatIso8601(utcIso, sizeof(utcIso), utcTm, "Z");

        char sign = tzOffsetSeconds >= 0 ? '+' : '-';
        int absMinutes = tzOffsetMinutes >= 0 ? tzOffsetMinutes : -tzOffsetMinutes;
        snprintf(offsetSuffix, sizeof(offsetSuffix), "%c%02d:%02d", sign, absMinutes / 60, absMinutes % 60);
        formatIso8601(localIso, sizeof(localIso), localTm, offsetSuffix);

        obj["utcIso"] = utcIso;
        obj["localIso"] = localIso;
    }},
    {"azimuth", [&](HeliostatController &controller, JsonVariant content) {
        if (content.is<JsonObject>()) {
            if (controller.azimuthController.getType() == 1) {
                auto* azimuthCtrl = static_cast<ClosedLoopController*>(&controller.azimuthController);
                ClosedLoopControllerJsonRouter::router.serialize(*azimuthCtrl, content);
            }
            else if (controller.azimuthController.getType() == 2) {
                auto* servoCtrl = static_cast<Servo_Driver*>(&controller.azimuthController);
                ServoControllerJsonRouter::router.serialize(*servoCtrl, content);
            }
        }
    }},
    {"elevation", [&](HeliostatController &controller, JsonVariant content) {
        if (content.is<JsonObject>()) {
            if (controller.elevationController.getType() == 1) {
                auto* elevationCtrl = static_cast<ClosedLoopController*>(&controller.elevationController);
                ClosedLoopControllerJsonRouter::router.serialize(*elevationCtrl, content);
            }
            else if (controller.elevationController.getType() == 2) {
                auto* servoCtrl = static_cast<Servo_Driver*>(&controller.elevationController);
                ServoControllerJsonRouter::router.serialize(*servoCtrl, content);
            }
        }
    }},
});


void HeliostatControllerJsonRouter::readDirectionsMap(DirectionsMap map, JsonObject &object) 
{
    for (auto &dir : map) { 
        JsonObject obj = object[dir.first].to<JsonObject>();
        obj["elevation"] = dir.second.elevation;
        obj["azimuth"] = dir.second.azimuth;
        // ESP_LOGI("Read Map", "%s", dir.first);
    }
}

bool HeliostatControllerJsonRouter::updateDirectionsMap(JsonVariant content, DirectionsMap &map) 
{
    bool updated = false;
    if (content.is<JsonObject>()) {
        for (auto kv : content.as<JsonObject>()) {
            JsonObject obj = kv.value().as<JsonObject>();
            if (map.find(String(kv.key().c_str())) != map.end()) {
                SphericalCoordinate &target = map[String(kv.key().c_str())];
                target.azimuth = obj["azimuth"] | target.azimuth;
                target.elevation = obj["elevation"] | target.elevation;
                ESP_LOGI("Update Map", "%s", kv.key().c_str());
            }
            else map.insert({kv.key().c_str(), {obj["azimuth"] | 120., obj["elevation"] | 45.}});
            // ESP_LOGI("Update Map", "%s", kv.key().c_str());
            updated = true;
        }
    }
    return updated;
}

bool HeliostatControllerJsonRoutenameFromMap(String target, DirectionsMap &map) 
{
    if (map.count(target) > 0) {
        map.erase(target);
        return true;
    }
    return false;
}

void HeliostatService::begin() 
{
    // _stateService.begin();
    _eventEndpoint.begin();
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
    _state.init();
}
void HeliostatService::loop() 
{
    _state.run();
    // _stateService.updateState();
}