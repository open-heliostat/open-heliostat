#ifndef StatelessService_h
#define StatelessService_h

#include <map>
#include <ArduinoJson.h>
#include <EventSocket.h>
#include <FS.h>

template <typename T>
using JsonEventTrigger = const std::function<bool (const JsonVariant& content, T &state)>;

template <typename T>
using JsonEventTriggerMap = std::map<const String, JsonEventTrigger<T>>;

template <class T>
class JsonEventRouter
{
public:
    JsonEventRouter(JsonEventTriggerMap<T> eventMap) :
        eventMap(eventMap) {}
    bool parse(JsonVariant obj, T &state) 
    {
        // String str;
        // serializeJson(obj, str);
        // ESP_LOGI("Event Router", "%s", str.c_str());
        bool success = false;
        for (auto const& e : eventMap) {
            if (obj[e.first].template is<JsonVariant>() && e.second(obj[e.first], state)) success = true;
        }
        // ESP_LOGI("Event Router", "%d", success);
        return success;
    }
private:
    const JsonEventTriggerMap<T> eventMap;
};

template <typename T>
using StatelessReader = const std::function<void (T &state, const JsonVariant& target)>;

template <typename T>
using JsonStatelessReaderMap = std::map<const String, StatelessReader<T>>;

using EventEmitter = std::function<void (JsonObject event)>;

template <class T>
class JsonStatelessReader
{
public:
    JsonStatelessReader(JsonStatelessReaderMap<T> readerMap, EventEmitter emitter = [](JsonObject event) {}) :
        readerMap(readerMap),
        emitter(emitter) {}
    void serialize(T &state, JsonVariant target) 
    {
        serializeWithoutPropagation(state, target);
        emitter(target.as<JsonObject>());
    }
    void serializeWithoutPropagation(T &state, JsonVariant target) 
    {
        if (target.as<JsonObject>().size() > 0) {
            for (auto const& e : readerMap) if (target[e.first].template is<JsonVariant>()) e.second(state, target[e.first].template as<JsonVariant>());
        }
        else if (target.is<JsonVariant>()) {
            JsonObject obj = target.to<JsonObject>();
            for (auto const& e : readerMap) e.second(state, obj[e.first].template to<JsonVariant>());
        }
    }
private:
    const JsonStatelessReaderMap<T> readerMap;
    const EventEmitter emitter;
};

template <class T>
class JsonRouter
{
public:
    JsonRouter(JsonEventTriggerMap<T> eventRouterMap, JsonStatelessReaderMap<T> stateReaderMap = {}, EventEmitter emitter = [](JsonObject event) {}) :
        eventRouter(eventRouterMap), stateReader(stateReaderMap, emitter) {}
    bool route(JsonVariant content, T &state)
    {
        // String str;
        // serializeJson(content, str);
        // ESP_LOGI("Router", "%s", str.c_str());
        if (eventRouter.parse(content, state)) return true;
        else stateReader.serialize(state, content);
        return false;
    }
    bool parse(JsonVariant obj, T &state) 
    {
        return eventRouter.parse(obj, state);
    }
    void serialize(T &state, JsonVariant target) 
    {
        stateReader.serialize(state, target);
    }
    void serializeWithoutPropagation(T &state, JsonVariant target) 
    {
        stateReader.serializeWithoutPropagation(state, target);
    }
private:
    JsonEventRouter<T> eventRouter;
    JsonStatelessReader<T> stateReader;
};

class JsonFilePersistence
{
public:
    JsonFilePersistence(const char *filePath, FS *fs) : filePath(filePath), fs(fs) {}
    JsonDocument readFromFS()
    {
        File settingsFile = fs->open(filePath, "r");
        JsonDocument jsonDocument;
        DeserializationError error = deserializeJson(jsonDocument, settingsFile);
        return jsonDocument;
    }
    bool writeToFS(JsonObject jsonObject)
    {
        mkdirs();
        File settingsFile = fs->open(filePath, "w");
        if (!settingsFile) return false;
        String str;
        serializeJson(jsonObject, str);
        ESP_LOGI("Writing file :", "\n%s", str.c_str());
        serializeJson(jsonObject, settingsFile);
        settingsFile.close();
        return true;
    }
private:
    void mkdirs()
    {
        String path(filePath);
        int index = 0;
        while ((index = path.indexOf('/', index + 1)) != -1)
        {
            String segment = path.substring(0, index);
            if (!fs->exists(segment))
            {
                fs->mkdir(segment);
            }
        }
    }
    const char *filePath;
    FS *fs;
};

// class StatelessService
// {
// public:
//     void begin() 
//     {
//         socket->registerEvent(eventName);
//         socket->onEvent(eventName, [&](JsonObject &root, int originID) { route(root); });
//         ESP_LOGI("Stateless Service", "Registered Json Event : %s", eventName);
//     }
//     bool route(JsonVariant contents) 
//     {
//         return router.route(contents);
//     }
//     void emitEvent(JsonObject &json) 
//     {
//         socket->emitEvent(eventName, json);
//     }
// private:
//     const char* eventName = nullptr;
//     EventSocket *socket = nullptr;
//     JsonRouter router = JsonRouter({});
// };

#endif