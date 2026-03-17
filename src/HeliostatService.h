#ifndef HeliostatService_h
#define HeliostatService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <lib/HttpStateRouterEndpoint.h>
#include <lib/JsonStateRouter.h>
#include <ClosedLoopControllerService.h>
#include <ServoControllerService.h>

#include <heliostat.h>

#include <esp_debug_helpers.h>

using JsonStateRouting::JsonRouter;
using JsonStateRouting::JsonSaveManager;

class HeliostatControllerJsonRouter
{
public:
    static bool route(JsonVariant content, HeliostatController &controller)
    {
        return router.route(content, controller);
    }
    static void read(HeliostatController &state, JsonObject &root)
    {
        router.serialize(state, root);
    }
    static void readForSave(HeliostatController &state, JsonObject &root)
    {
        getSaveMap(root, state);
        router.serialize(state, root);
        JsonDocument ref = root;
        // Serial.println(ref.as<String>());
        ref = getSaveMap(state);
        JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
    }
    static StateUpdateResult update(JsonObject &root, HeliostatController &state, const String &originId)
    {
        (void)originId;
        if (!router.parse(root, state)) {
            return StateUpdateResult::ERROR;
        }
        if (JsonSaveManager::needsToSave(root, getSaveMap(state))) {
            return StateUpdateResult::CHANGED;
        }
        return StateUpdateResult::UNCHANGED;
    }
    static const void getSaveMap(JsonObject &root, HeliostatController &state)
    {
        if (state.azimuthController.getType() == 2) {
            root["azimuth"] = ServoControllerJsonRouter::getSaveMap();
        }
        else if (state.azimuthController.getType() == 1) {
            root["azimuth"] = ClosedLoopControllerJsonRouter::getSaveMap();
        }
        if (state.elevationController.getType() == 2) {
            root["elevation"] = ServoControllerJsonRouter::getSaveMap();
        }
        else if (state.elevationController.getType() == 1) {
            root["elevation"] = ClosedLoopControllerJsonRouter::getSaveMap();
        }
        root["currentTarget"] = true;
        root["currentSource"] = true;
        root["add"] = true;
        root["remove"] = true;
        root["rename"] = true;
        root["set"] = true;
        root["sourcesMap"] = true;
        root["sunTracker"]["latitude"] = true;
        root["sunTracker"]["longitude"] = true;
    }
    static const JsonDocument getSaveMap(HeliostatController &state)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        getSaveMap(obj, state);
        return doc;
    }
    static bool removeFromMap(String target, DirectionsMap &map);
    static bool updateDirectionsMap(JsonVariant content, DirectionsMap &map);
    static void readDirectionsMap(DirectionsMap map, JsonObject &object);
    static JsonRouter<HeliostatController> router;
};

class HeliostatService : public StatefulService<HeliostatController&>
{
public:
    HeliostatService(PsychicHttpServer *server,
                        EventSocket *socket,
                        FS *fs,
                        SecurityManager *securityManager,
                        HeliostatController &controller) :
                            _httpRouterEndpoint(_router.read, _router.update, this, server, "/rest/heliostat", securityManager),
                            _eventEndpoint(_router.read, _router.update, this, socket, "heliostat-service"),
                            _fsPersistence(_router.readForSave, _router.update, this, fs, "/config/heliostat.json"),
                            StatefulService(controller) {}
    void begin();
    void loop();

private:
    EventEndpoint<HeliostatController&> _eventEndpoint;
    HttpStateRouterEndpoint<HeliostatController&> _httpRouterEndpoint;
    FSPersistence<HeliostatController&> _fsPersistence;
    HeliostatControllerJsonRouter _router;
};

// class HeliostatControllerState
// {
// public:
//     String currentSource;
//     String currentTarget;
//     DirectionsMap sourcesMap = {};
//     DirectionsMap targetsMap = {};

//     static void read(HeliostatControllerState &state, JsonObject &root) {
//         root["currentSource"] = state.currentSource;
//         root["currentTarget"] = state.currentTarget;
//         JsonObject sourcesMapJson = root["sourcesMap"].to<JsonObject>();
//         readDirectionsMap(state.sourcesMap, sourcesMapJson);
//         JsonObject targetsMapJson = root["targetsMap"].to<JsonObject>();
//         readDirectionsMap(state.targetsMap, targetsMapJson);
//     }

//     static void readDirectionsMap(DirectionsMap&map, JsonObject &object) 
//     {
//         for (auto &dir : map) { 
//             JsonObject obj = object[dir.first];
//             obj["elevation"] = dir.second.elevation;
//             obj["azimuth"] = dir.second.azimuth;
//         }
//     }

//     static StateUpdateResult update(JsonObject &root, HeliostatControllerState &state) {
//         state.currentSource = root["currentSource"] | "";
//         state.currentTarget = root["currentTarget"] | "";
//         for (auto source : root["sourcesMap"].as<JsonObject>()) {
//             String key = String(source.key().c_str());
//             if (state.sourcesMap.find(key) != state.sourcesMap.end()) {
//                 state.sourcesMap[key].azimuth = source.value()["azimuth"] | state.sourcesMap[key].azimuth;
//                 state.sourcesMap[key].elevation = source.value()["elevation"] | state.sourcesMap[key].elevation;
//             }
//             else {
//                 state.sourcesMap[key] = {source.value()["azimuth"], source.value()["elevation"]};
//             }
//         }
//         for (auto target : root["targetsMap"].as<JsonObject>()) {
//             String key = String(target.key().c_str());
//             if (state.targetsMap.find(key) != state.targetsMap.end()) {
//                 state.targetsMap[key].azimuth = target.value()["azimuth"] | state.targetsMap[key].azimuth;
//                 state.targetsMap[key].elevation = target.value()["elevation"] | state.targetsMap[key].elevation;
//             }
//             else {
//                 state.targetsMap[key] = {target.value()["azimuth"], target.value()["elevation"]};
//             }
//         }
//         return StateUpdateResult::CHANGED;
//     }

//     static void readController(HeliostatController &controller, JsonObject &root) {
//         root["currentSource"] = controller.currentSource;
//         root["currentTarget"] = controller.currentTarget;
//         JsonObject sourcesMapJson = root["sourcesMap"].to<JsonObject>();
//         readDirectionsMap(controller.sourcesMap, sourcesMapJson);
//         JsonObject targetsMapJson = root["targetsMap"].to<JsonObject>();
//         readDirectionsMap(controller.targetsMap, targetsMapJson);
//     }

//     static void updateController(HeliostatControllerState &state, HeliostatController &controller) {
//         controller.currentSource = state.currentSource;
//         controller.currentTarget = state.currentTarget;
//         controller.sourcesMap = state.sourcesMap;
//         controller.targetsMap = state.targetsMap;
//     }
// };

// class HeliostatControllerStateService : public StatefulService<HeliostatControllerState>
// {
// public:
//     HeliostatControllerStateService(EventSocket *socket, FS *fs, HeliostatController &controller) :
//         _eventEndpoint(HeliostatControllerState::read, HeliostatControllerState::update, this, socket, "heliostat-control"),
//         _fsPersistence(HeliostatControllerState::read, HeliostatControllerState::update, this, fs, "/config/heliostat-control.json"),
//         controller(controller)
//     {
//         addUpdateHandler([&](const String &originId) { updateController(originId); }, false);
//     }
//     void begin()
//     {
//         _eventEndpoint.begin();
//         _fsPersistence.readFromFS();
//         updateState();
//         updateController("begin");
//     }
//     void updateState()
//     {
//         JsonDocument json;
//         JsonObject jsonObject = json.to<JsonObject>();
//         _state.readController(controller, jsonObject);
//         update(jsonObject, _state.update, "stateUpdate");
//     }

// private:
//     EventEndpoint<HeliostatControllerState> _eventEndpoint;
//     FSPersistence<HeliostatControllerState> _fsPersistence;
//     HeliostatController &controller;

//     void updateController(const String &originId)
//     {
//         if (originId != "stateUpdate") {
//             HeliostatControllerState::updateController(_state, controller);
//         }
//     }
// };


// class HeliostatStatelessService
// {
// public:
//     HeliostatStatelessService(EventSocket *socket, FS *fs, PsychicHttpServer *server ,HeliostatController &controller) :
//         socket(socket),
//         file(filePath, fs),
//         server(server),
//         controller(controller) {}
//     void begin()
//     {
//         // socket->registerEvent(eventName);
//         // socket->onEvent(eventName, [&](JsonObject &root, int originID) {
//         //     String str;
//         //     serializeJson(root, str);
//         //     ESP_LOGI("Heliostat Service", "%s", str.c_str());
//         //     if (router.route(root, controller)) saveStateIfNeeded(root);
//         // });
//         // socket->onSubscribe(eventName, [&](const String &originID) {
//         //     JsonDocument doc;
//         //     JsonObject obj = doc.to<JsonObject>();
//         //     router.serialize(controller, obj);
//         // });
//         // ESP_LOGI("Heliostat Service", "Registered Json Event : %s", eventName);
//         JsonDocument savedState = file.readFromFS();
//         if (savedState.is<JsonObject>()) {
//             router.parse(savedState.as<JsonObject>(), controller);
//             String str;
//             serializeJson(savedState, str);
//             ESP_LOGI("Loading File", "\n%s", str.c_str());
//         }
//         else {
//             saveState();
//         }
//         server->on(restPath, HTTP_GET, [&](PsychicRequest *request){
//             PsychicJsonResponse response = PsychicJsonResponse(request, false);
//             ESP_LOGV("HTTP GET", "Path : %s, Body: %s", request->path().c_str(), request->body().c_str());
//             JsonObject jsonObject = response.getRoot();
//             String path(request->path());
//             if (path.startsWith("/rest/heliostat")) path = path.substring(15);
//             JsonObject obj = resolvePath(path, jsonObject);
//             JsonDocument requestBody;
//             if (deserializeJson(requestBody, request->body()) == DeserializationError::Ok) {
//                 obj.set(requestBody.as<JsonObject>());
//                 ESP_LOGV("HTTP GET", "Json %s", requestBody.as<String>().c_str());
//             }
//             router.serialize(controller, response.getRoot());
//             response.getRoot() = obj;
//             ESP_LOGV("HTTP GET", "Response %s", response.getRoot().as<String>().c_str());
//             return response.send();
//         });
//         server->on(restPath, HTTP_POST, [&](PsychicRequest *request, JsonVariant &json){
//             ESP_LOGV("HTTP POST", "Path : %s, Json : %s", request->path().c_str(), json.as<String>().c_str());
//             JsonDocument doc;
//             JsonObject jsonObject = doc.to<JsonObject>();
//             String path(request->path());
//             if (path.startsWith("/rest/heliostat")) path = path.substring(15);
//             JsonObject obj = resolvePath(path, jsonObject);
//             obj.set(json.as<JsonObject>());
//             if (router.parse(doc.as<JsonObject>(), controller)) {
//                 saveStateIfNeeded(doc.as<JsonObject>());
//                 PsychicJsonResponse response = PsychicJsonResponse(request, false);
//                 router.serialize(controller, doc.as<JsonObject>());
//                 response.getRoot() = obj;
//                 ESP_LOGV("HTTP POST", "Response %s", response.getRoot().as<String>().c_str());
//                 return response.send();
//             }
//             else return request->reply(400);
//         });
//     }
//     JsonObject resolvePath(String path, JsonObject obj) {
//         int index = 0;
//         while ((index = path.indexOf('/')) != -1)
//         {
//             String segment = path.substring(0, index);
//             ESP_LOGV("HTTP SUBPATH", "%s", segment.c_str());
//             if (index > 0) obj = obj[segment].to<JsonObject>();
//             path = path.substring(index+1);
//         }
//         if (path.length()>0) obj = obj[path].to<JsonObject>();
//         return obj;
//     }
//     void saveStateIfNeeded(JsonObject state)
//     {
//         if (needsToSave(state)) saveState();
//     }
//     void saveState() {
//         JsonDocument doc = getSaveMap();
//         router.serializeWithoutPropagation(controller, doc.as<JsonObject>());
//         JsonDocument ref = getSaveMap();
//         String str;
//         serializeJson(doc, str);
//         ESP_LOGV("Pre Filter", "\n%s", str.c_str());
//         deserializeJson(doc, str, DeserializationOption::Filter(ref));
//         ESP_LOGV("Post Filter", "\n%s", doc.as<String>().c_str());
//         file.writeToFS(doc.as<JsonObject>());
//     }
//     bool needsToSave(JsonObject &state) {
//         JsonDocument saveMap = getSaveMap();
//         return checkFields(saveMap.as<JsonObject>(), state);
//     }
//     bool checkFields(JsonObject ref, JsonObject obj) {
//         for (JsonPair kv : ref) {
//             if (kv.value() == true && obj[kv.key()].is<JsonVariant>()) return true;
//             else if (kv.value().is<JsonObject>() && obj[kv.key()].is<JsonObject>() && checkFields(kv.value(), obj[kv.key()])) return true;
//         }
//         return false;
//     }
//     JsonDocument getSaveMap() {
//         JsonDocument doc;
//         doc["elevation"] = closedLoopControllerRouter.getSaveMap();
//         doc["azimuth"] = closedLoopControllerRouter.getSaveMap();
//         return doc;
//     }
//     void emitEvent(JsonObject &json)
//     {
//         String jsonStr;
//         serializeJson(json, jsonStr);
//         ESP_LOGI("Heliostat Event", "%s", jsonStr.c_str());
//         // esp_backtrace_print(10);
//         socket->emitEvent(eventName, json);
//     }

// private:
//     const char* eventName = "heliostat-service";
//     const char* restPath = "/rest/heliostat/?*";
//     const char* filePath = "/config/heliostat.json";
//     EventSocket *socket;
//     PsychicHttpServer *server;
//     JsonFilePersistence file;
//     HeliostatController &controller;
//     ClosedLoopControllerJsonRouter closedLoopControllerRouter;
//     JsonRouter<HeliostatController> router = JsonRouter<HeliostatController>(
//     {
//         {"azimuth", [&](JsonVariant content, HeliostatController &controller) {
//             return closedLoopControllerRouter.router.parse(content, controller.azimuthController);
//         }},
//         {"elevation", [&](JsonVariant content, HeliostatController &controller) {
//             return closedLoopControllerRouter.router.parse(content, controller.elevationController);
//         }}
//     },
//     {
//         {"azimuth", [&](HeliostatController &controller, JsonVariant content) {
//             closedLoopControllerRouter.router.serialize(controller.azimuthController, content);
//         }},
//         {"elevation", [&](HeliostatController &controller, JsonVariant content) {
//             closedLoopControllerRouter.router.serialize(controller.elevationController, content);
//         }}
//     }
//     //, [&](JsonObject event) {emitEvent(event);}
//     );
// };

#endif

