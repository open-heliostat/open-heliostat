#include <MovementSequencerService.h>

JsonRouter<MovementSequencer> MovementSequencerJsonRouter::router = JsonRouter<MovementSequencer>(
    {
        {"loop", [](JsonVariant content, MovementSequencer &sequencer) {
             if (content.is<bool>()) {
                 sequencer.setLooping(content.as<bool>());
                 return true;
             }
             return false;
         }},
        {"frames", [](JsonVariant content, MovementSequencer &sequencer) {
             if (!content.is<JsonArray>()) return false;
             MovementSequencer::Keyframe buffer[MovementSequencer::kMaxFrames];
             size_t count = 0;
             for (JsonVariant v : content.as<JsonArray>()) {
                 if (!v.is<JsonObject>()) continue;
                 JsonObject obj = v.as<JsonObject>();
                 if (!(obj["tMs"].is<unsigned long>() || obj["tMs"].is<int>() || obj["tMs"].is<double>())) continue;
                 if (!(obj["value"].is<double>() || obj["value"].is<int>())) continue;
                 buffer[count].tMs = static_cast<uint32_t>(obj["tMs"].as<unsigned long>());
                 buffer[count].value = obj["value"].as<double>();
                 if (++count >= MovementSequencer::kMaxFrames) break;
             }
             if (count == 0) {
                 sequencer.clear();
                 return true;
             }
             return sequencer.load(buffer, count, sequencer.isLooping());
         }},
        {"start", [](JsonVariant content, MovementSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.start();
                 return true;
             }
             return false;
         }},
        {"stop", [](JsonVariant content, MovementSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.stop();
                 return true;
             }
             return false;
         }},
        {"clear", [](JsonVariant content, MovementSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.clear();
                 return true;
             }
             return false;
         }},
    },
    {
        {"running", [](MovementSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.isRunning());
         }},
        {"loop", [](MovementSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.isLooping());
         }},
        {"duration", [](MovementSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.totalDuration());
         }},
        {"frames", [](MovementSequencer &sequencer, JsonVariant target) {
             JsonArray arr = target.to<JsonArray>();
             MovementSequencer::Keyframe frame;
             for (size_t i = 0; i < sequencer.size(); ++i) {
                 if (sequencer.frameAt(i, frame)) {
                     JsonObject obj = arr.add<JsonObject>();
                     obj["tMs"] = frame.tMs;
                     obj["value"] = frame.value;
                 }
             }
         }},
    });

void MovementSequencerJsonRouter::read(MovementSequencer &state, JsonObject &root)
{
    router.serialize(state, root);
}

void MovementSequencerJsonRouter::readForSave(MovementSequencer &state, JsonObject &root)
{
    getSaveMap(root);
    router.serialize(state, root);
    JsonDocument ref = getSaveMap();
    JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
}

StateUpdateResult MovementSequencerJsonRouter::update(JsonObject &root, MovementSequencer &state, const String &originId)
{
    (void)originId;
    if (router.parse(root, state) && JsonSaveManager::needsToSave(root, getSaveMap())) return StateUpdateResult::CHANGED;
    else return StateUpdateResult::UNCHANGED;
}

void MovementSequencerJsonRouter::getSaveMap(JsonObject &root)
{
    root["frames"] = true;
    root["loop"] = true;
}

JsonDocument MovementSequencerJsonRouter::getSaveMap()
{
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    getSaveMap(obj);
    return doc;
}

MovementSequencerService::MovementSequencerService(PsychicHttpServer *server,
                                                   EventSocket *socket,
                                                   FS *fs,
                                                   SecurityManager *securityManager,
                                                   MovementSequencer &sequencer,
                                                   const char *restPath,
                                                   const char *settingsPath,
                                                   const char *eventName) :
    StatefulService(sequencer),
    _httpRouterEndpoint(_router.read, _router.update, this, server, restPath, securityManager),
    _fsPersistence(_router.readForSave, _router.update, this, fs, settingsPath),
    _eventEndpoint(_router.read, _router.update, this, socket, eventName) {}

void MovementSequencerService::begin()
{
    _eventEndpoint.begin();
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}

void MovementSequencerService::loop()
{
    if (_state.isRunning()) {
        _state.update();
    }
}
