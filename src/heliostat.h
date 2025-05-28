#ifndef HELIOSTAT_CLASS_H
#define HELIOSTAT_CLASS_H

#include <closedloopcontroller.h>
#include <sun.h>
#include <gpsneo.h>

using DirectionsMap = std::map<String, SphericalCoordinate>;

class HeliostatController
{
public:
    HeliostatController(AbstractController &azimuthController, AbstractController &elevationController, SerialGPS &gps) : 
        azimuthController(azimuthController), elevationController(elevationController), gps(gps) {}

    SphericalCoordinate getTarget() 
    {
        return SphericalCoordinate{azimuthController.getTarget(), elevationController.getTarget()};
    }

    SphericalCoordinate getPosition() 
    {
        return SphericalCoordinate{azimuthController.getAngle(), elevationController.getAngle()};
    }

    void setPosition(double azimuth, double elevation)
    {
        azimuthController.setAngle(azimuth);
        elevationController.setAngle(elevation);
    }

    void setPosition(SphericalCoordinate target)
    {
        azimuthController.setAngle(target.azimuth);
        elevationController.setAngle(target.elevation);
    }

    SphericalCoordinate reflect(SphericalCoordinate source, SphericalCoordinate target) 
    {
        vec3 bisector = toCartesian({source.azimuth, source.elevation}) + toCartesian({target.azimuth, target.elevation});
        vec2 result = toSpherical(bisector);
        // ESP_LOGI("Reflector", "%f %f", result.x, result.y);
        return {result.x, result.y};
    }

    void reflectCurrentSource() {
        auto directionsMap = getDirectionsMap();
        if (directionsMap.find(currentSource) != directionsMap.end() && directionsMap.find(currentTarget) != directionsMap.end()) {
            auto reflection = reflect(directionsMap[currentSource], directionsMap[currentTarget]);
            setPosition(reflection);
            // ESP_LOGI("Reflector", "%f %f", reflection.azimuth, reflection.elevation);
        }
    }

    void run() 
    {
        unsigned long now = millis();
        if (enabled & now - lastCommand > 1000) {
            reflectCurrentSource();
            lastCommand = now;
        }
        azimuthController.run();
        elevationController.run();
    }

    void init() 
    {
        setupSolarTracker();
        azimuthController.init();
        elevationController.init();
    }

    bool isTimeSet() 
    {
        // return sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
        // return timeStatus() == timeSet;
        time_t now = time(nullptr);
        struct tm *timeinfo = localtime(&now);
        return (timeinfo->tm_year > 120); // tm_year is years since 1900, so 120 means year 2020
        return year() > 2020;
    }

    bool enabled = true;

    String currentSource = "Sun";
    String currentTarget = "Default Target";

    double latitude;
    double longitude;

    void getLocationFromGPS() {
        if (gps.numSats > 3) {
            latitude = gps.coords.latitude;
            longitude = gps.coords.longitude;
        }
    }

    SphericalCoordinate getSolarPosition() 
    {
        return computeSolarPosition(latitude, longitude);
    }

    DirectionsMap getDirectionsMap() 
    {
        DirectionsMap map = {};
        if (isTimeSet()) map.insert({"Sun", getSolarPosition()});
        map.insert(targetsMap.begin(), targetsMap.end());
        return map;
    }

    void addTarget(String name) 
    {
        targetsMap.insert({name, {120., 45.}});
    }
    
    bool deleteTarget(String name) 
    {
        return targetsMap.erase(name) > 0;
    }
    
    bool renameTarget(String oldName, String newName) 
    {
        if (targetsMap.find(oldName) != targetsMap.end()) {
            targetsMap.insert({newName, targetsMap[oldName]});
            deleteTarget(oldName);
            return true;
        }
        else return false;
    }

    bool setTarget(String name, double azimuth, double elevation) 
    {
        if (targetsMap.find(name) != targetsMap.end()) {
            targetsMap[name].azimuth = azimuth;
            targetsMap[name].elevation = elevation;
            return true;
        }
        else return false;
    }

    DirectionsMap targetsMap = 
    {
        // {"Default Target", {120., 15.}}
    };

    AbstractController &azimuthController;
    AbstractController &elevationController;

    unsigned long lastCommand = 0;
    SerialGPS &gps;
};
#endif