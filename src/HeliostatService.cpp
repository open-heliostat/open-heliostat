#include <HeliostatService.h>

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
        obj["latitude"] = controller.latitude;
        obj["longitude"] = controller.longitude;
        obj["isTimeSet"] = controller.isTimeSet();
        obj["azimuth"] = controller.getSolarPosition().azimuth;
        obj["elevation"] = controller.getSolarPosition().elevation;
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
        ESP_LOGI("Read Map", "%s", dir.first);
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
            ESP_LOGI("Update Map", "%s", kv.key().c_str());
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