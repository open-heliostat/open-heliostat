#include <SourceTargetSequencerService.h>

JsonRouter<SourceTargetSequencer> SourceTargetSequencerJsonRouter::router = JsonRouter<SourceTargetSequencer>(
    {
        {"loop", [](JsonVariant content, SourceTargetSequencer &sequencer) {
             if (content.is<bool>()) {
                 sequencer.setLooping(content.as<bool>());
                 return true;
             }
             return false;
         }},
        {"frames", [](JsonVariant content, SourceTargetSequencer &sequencer) {
             if (!content.is<JsonArray>()) return false;
             SourceTargetSequencer::Frame buffer[SourceTargetSequencer::kMaxFrames];
             size_t count = 0;
             for (JsonVariant v : content.as<JsonArray>()) {
                 if (!v.is<JsonObject>()) continue;
                 JsonObject obj = v.as<JsonObject>();
                 if (!(obj["tMs"].is<unsigned long>() || obj["tMs"].is<int>() || obj["tMs"].is<double>())) continue;
                 auto hasNum = [&](const char *key) {
                     return obj[key].is<double>() || obj[key].is<int>();
                 };
                 if (!hasNum("sourceAz") || !hasNum("sourceEl") || !hasNum("targetAz") || !hasNum("targetEl")) continue;
                 buffer[count].tMs = static_cast<uint32_t>(obj["tMs"].as<unsigned long>());
                 buffer[count].sourceAz = obj["sourceAz"].as<double>();
                 buffer[count].sourceEl = obj["sourceEl"].as<double>();
                 buffer[count].targetAz = obj["targetAz"].as<double>();
                 buffer[count].targetEl = obj["targetEl"].as<double>();
                 if (++count >= SourceTargetSequencer::kMaxFrames) break;
             }
             if (count == 0) {
                 sequencer.clear();
                 return true;
             }
             return sequencer.load(buffer, count, sequencer.isLooping());
         }},
        {"start", [](JsonVariant content, SourceTargetSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.start();
                 return true;
             }
             return false;
         }},
        {"stop", [](JsonVariant content, SourceTargetSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.stop();
                 return true;
             }
             return false;
         }},
        {"clear", [](JsonVariant content, SourceTargetSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.clear();
                 return true;
             }
             return false;
         }},
    },
    {
        {"running", [](SourceTargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.isRunning());
         }},
        {"loop", [](SourceTargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.isLooping());
         }},
        {"duration", [](SourceTargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.totalDuration());
         }},
        {"frames", [](SourceTargetSequencer &sequencer, JsonVariant target) {
             JsonArray arr = target.to<JsonArray>();
             SourceTargetSequencer::Frame frame;
             for (size_t i = 0; i < sequencer.size(); ++i) {
                 if (sequencer.frameAt(i, frame)) {
                     JsonObject obj = arr.add<JsonObject>();
                     obj["tMs"] = frame.tMs;
                     obj["sourceAz"] = frame.sourceAz;
                     obj["sourceEl"] = frame.sourceEl;
                     obj["targetAz"] = frame.targetAz;
                     obj["targetEl"] = frame.targetEl;
                 }
             }
         }},
    });

void SourceTargetSequencerJsonRouter::read(SourceTargetSequencer &state, JsonObject &root)
{
    router.serialize(state, root);
}

void SourceTargetSequencerJsonRouter::readForSave(SourceTargetSequencer &state, JsonObject &root)
{
    getSaveMap(root);
    router.serialize(state, root);
    JsonDocument ref = getSaveMap();
    JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
}

StateUpdateResult SourceTargetSequencerJsonRouter::update(JsonObject &root, SourceTargetSequencer &state, const String &originId)
{
    (void)originId;
    if (router.parse(root, state) && JsonSaveManager::needsToSave(root, getSaveMap())) return StateUpdateResult::CHANGED;
    else return StateUpdateResult::UNCHANGED;
}

void SourceTargetSequencerJsonRouter::getSaveMap(JsonObject &root)
{
    root["frames"] = true;
    root["loop"] = true;
}

JsonDocument SourceTargetSequencerJsonRouter::getSaveMap()
{
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    getSaveMap(obj);
    return doc;
}

SourceTargetSequencerService::SourceTargetSequencerService(PsychicHttpServer *server,
                                                   EventSocket *socket,
                                                   FS *fs,
                                                   SecurityManager *securityManager,
                                                   SourceTargetSequencer &sequencer,
                                                   const char *restPath,
                                                   const char *settingsPath,
                                                   const char *eventName) :
    StatefulService(sequencer),
    _httpRouterEndpoint(_router.read, _router.update, this, server, restPath, securityManager),
    _fsPersistence(_router.readForSave, _router.update, this, fs, settingsPath),
    _eventEndpoint(_router.read, _router.update, this, socket, eventName) {}

void SourceTargetSequencerService::begin()
{
    _eventEndpoint.begin();
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}

void SourceTargetSequencerService::loop()
{
    if (_state.isRunning()) {
        _state.update();
    }
}
