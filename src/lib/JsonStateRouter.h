#ifndef JsonStateRouter_h
#define JsonStateRouter_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <StatefulService.h>

#include <functional>
#include <list>
#include <utility>

/**
 *   ESP32 SvelteKit
 *
 *   A lightweight toolkit for routing JSON events and serializing state
 *   updates into a stateless service layer.
 */

namespace JsonStateRouting
{
    template <typename T>
    using JsonEventTrigger = const std::function<bool (const JsonVariant& content, T &state)>;

    template <typename T>
    using TriggerPair = std::pair<const String, JsonEventTrigger<T>>;

    template <typename T>
    using JsonEventTriggerMap = std::list<TriggerPair<T>>;

    template <typename T>
    using StatelessReader = const std::function<void (T &state, const JsonVariant& target)>;

    template <typename T>
    using ReaderPair = std::pair<const String, StatelessReader<T>>;

    template <typename T>
    using JsonStatelessReaderMap = std::list<ReaderPair<T>>;

    using EventEmitter = std::function<void (JsonObject event)>;

    template <class T>
    class JsonEventRouter
    {
    public:
        JsonEventRouter(JsonEventTriggerMap<T> eventMap) :
            _eventMap(eventMap) {}

        bool parse(JsonVariant obj, T &state)
        {
            bool success = false;
            for (const auto &entry : _eventMap)
            {
                if (!obj[entry.first].template is<JsonVariant>())
                {
                    continue;
                }
                if (entry.second(obj[entry.first], state))
                {
                    success = true;
                }
            }
            return success;
        }

    private:
        const JsonEventTriggerMap<T> _eventMap;
    };

    template <class T>
    class JsonStatelessReader
    {
    public:
        JsonStatelessReader(JsonStatelessReaderMap<T> readerMap, EventEmitter emitter = [](JsonObject event) {}) :
            _readerMap(readerMap),
            _emitter(emitter) {}

        void serialize(T &state, JsonVariant target)
        {
            serializeWithoutPropagation(state, target);
            _emitter(target.as<JsonObject>());
        }

        void serializeWithoutPropagation(T &state, JsonVariant target)
        {
            if (target.as<JsonObject>().size() > 0)
            {
                for (const auto &entry : _readerMap)
                {
                    if (target[entry.first].template is<JsonVariant>())
                    {
                        entry.second(state, target[entry.first].template as<JsonVariant>());
                    }
                }
            }
            else if (target.is<JsonVariant>())
            {
                JsonObject obj = target.to<JsonObject>();
                for (const auto &entry : _readerMap)
                {
                    entry.second(state, obj[entry.first].template to<JsonVariant>());
                }
            }
        }

    private:
        const JsonStatelessReaderMap<T> _readerMap;
        const EventEmitter _emitter;
    };

    template <class T>
    class JsonStateRouter
    {
    public:
        JsonStateRouter(JsonEventTriggerMap<T> eventRouterMap,
                        JsonStatelessReaderMap<T> stateReaderMap = {},
                        EventEmitter emitter = [](JsonObject event) {}) :
            _eventRouter(eventRouterMap),
            _stateReader(stateReaderMap, emitter)
        {
        }

        bool route(JsonVariant content, T &state)
        {
            if (_eventRouter.parse(content, state))
            {
                return true;
            }
            _stateReader.serialize(state, content);
            return false;
        }

        bool parse(JsonVariant obj, T &state)
        {
            return _eventRouter.parse(obj, state);
        }

        void serialize(T &state, JsonVariant target)
        {
            _stateReader.serialize(state, target);
        }

        void serializeWithoutPropagation(T &state, JsonVariant target)
        {
            _stateReader.serializeWithoutPropagation(state, target);
        }

    private:
        JsonEventRouter<T> _eventRouter;
        JsonStatelessReader<T> _stateReader;
    };

    template <typename T>
    using JsonRouter = JsonStateRouter<T>;

    class JsonSaveManager
    {
    public:
        static bool needsToSave(JsonObject state, JsonDocument saveMap)
        {
            return checkFieldsRecursively(saveMap.as<JsonObject>(), state);
        }

        static bool checkFieldsRecursively(JsonObject ref, JsonObject obj)
        {
            for (JsonPair kv : ref)
            {
                if (kv.value() == true && obj[kv.key()].is<JsonVariant>())
                {
                    return true;
                }
                else if (kv.value().is<JsonObject>() && obj[kv.key()].is<JsonObject>())
                {
                    if (checkFieldsRecursively(kv.value(), obj[kv.key()]))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        static void filterFieldsRecursively(JsonObject ref, JsonObject obj)
        {
            for (JsonPair kv : obj)
            {
                if (ref[kv.key()].is<bool>() && ref[kv.key()].as<bool>())
                {
                    continue;
                }
                if (ref[kv.key()].is<JsonObject>() && kv.value().is<JsonObject>())
                {
                    filterFieldsRecursively(ref[kv.key()], kv.value());
                    continue;
                }
                obj.remove(kv.key());
            }
        }
    };

    class JsonFilePersistence
    {
    public:
        JsonFilePersistence(const char *filePath, FS *fs) :
            _filePath(filePath),
            _fs(fs) {}

        JsonDocument readFromFS()
        {
            JsonDocument doc;
            if (!_fs)
            {
                return doc;
            }

            File settingsFile = _fs->open(_filePath, "r");
            if (!settingsFile)
            {
                return doc;
            }

            deserializeJson(doc, settingsFile);
            settingsFile.close();
            return doc;
        }

        bool writeToFS(JsonObject jsonObject)
        {
            if (!_fs)
            {
                return false;
            }
            mkdirs();
            File settingsFile = _fs->open(_filePath, "w");
            if (!settingsFile)
            {
                return false;
            }
            serializeJson(jsonObject, settingsFile);
            settingsFile.close();
            return true;
        }

    private:
        void mkdirs()
        {
            if (!_fs)
            {
                return;
            }
            String path(_filePath);
            int index = 0;
            while ((index = path.indexOf('/', index + 1)) != -1)
            {
                String segment = path.substring(0, index);
                if (!_fs->exists(segment))
                {
                    _fs->mkdir(segment);
                }
            }
        }

        const char *_filePath;
        FS *_fs;
    };

    template <class T>
    class StaticJsonStateRouter
    {
    public:
        static bool route(JsonVariant content, T &controller)
        {
            return router.route(content, controller);
        }

        static void read(T &state, JsonObject &root)
        {
            router.serialize(state, root);
        }

        static StateUpdateResult update(JsonObject &root, T &state, const String &originId)
        {
            (void)originId;
            if (router.parse(root, state))
            {
                return StateUpdateResult::CHANGED;
            }
            return StateUpdateResult::UNCHANGED;
        }

        static JsonStateRouter<T> router;
    };

    template <class T>
    using StaticJsonRouter = StaticJsonStateRouter<T>;
}

#endif
