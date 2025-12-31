#include <TargetSequencerService.h>

JsonRouter<TargetSequencer> TargetSequencerJsonRouter::router = JsonRouter<TargetSequencer>(
    {
        {"loop", [](JsonVariant content, TargetSequencer &sequencer) {
             if (content.is<bool>()) {
                 sequencer.setLooping(content.as<bool>());
                 return true;
             }
             return false;
         }},
        {"target", [](JsonVariant content, TargetSequencer &sequencer) {
             if (content.is<String>()) {
                 sequencer.setTargetName(content.as<String>());
                 return true;
             }
             return false;
         }},
        {"start", [](JsonVariant content, TargetSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.start();
                 return true;
             }
             return false;
         }},
        {"stop", [](JsonVariant content, TargetSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.stop();
                 return true;
             }
             return false;
         }},
        {"clear", [](JsonVariant content, TargetSequencer &sequencer) {
             if (content.is<bool>() && content.as<bool>()) {
                 sequencer.clear();
                 return true;
             }
             return false;
         }},
    },
    {
        {"running", [](TargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.isRunning());
         }},
        {"loop", [](TargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.isLooping());
         }},
        {"duration", [](TargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.totalDuration());
         }},
        {"target", [](TargetSequencer &sequencer, JsonVariant target) {
             target.set(sequencer.target());
         }},
        {"frames", [](TargetSequencer &sequencer, JsonVariant target) {
             JsonObject obj = target.to<JsonObject>();
             JsonArray azArr = obj["az"].to<JsonArray>();
             JsonArray elArr = obj["el"].to<JsonArray>();
             TargetSequencer::Frame frame;
             for (size_t i = 0; i < sequencer.sizeAz(); ++i) {
                 if (sequencer.frameAzAt(i, frame)) {
                     JsonObject o = azArr.add<JsonObject>();
                     o["tMs"] = frame.tMs;
                     o["value"] = frame.value;
                 }
             }
             for (size_t i = 0; i < sequencer.sizeEl(); ++i) {
                 if (sequencer.frameElAt(i, frame)) {
                     JsonObject o = elArr.add<JsonObject>();
                     o["tMs"] = frame.tMs;
                     o["value"] = frame.value;
                 }
             }
         }},
    });

void TargetSequencerJsonRouter::read(TargetSequencer &state, JsonObject &root)
{
    router.serialize(state, root);
}

void TargetSequencerJsonRouter::readForSave(TargetSequencer &state, JsonObject &root)
{
    getSaveMap(root);
    router.serialize(state, root);
    JsonDocument ref = getSaveMap();
    JsonSaveManager::filterFieldsRecursively(ref.as<JsonObject>(), root);
}

StateUpdateResult TargetSequencerJsonRouter::update(JsonObject &root, TargetSequencer &state, const String &originId)
{
    (void)originId;

    TargetSequencer::Frame azBuf[TargetSequencer::kMaxFrames];
    TargetSequencer::Frame elBuf[TargetSequencer::kMaxFrames];
    size_t azCount = state.sizeAz();
    size_t elCount = state.sizeEl();
    for (size_t i = 0; i < azCount; ++i) state.frameAzAt(i, azBuf[i]);
    for (size_t i = 0; i < elCount; ++i) state.frameElAt(i, elBuf[i]);

    auto parseFrames = [](JsonVariant arrVar, TargetSequencer::Frame *out, size_t &cnt) {
        cnt = 0;
        if (!arrVar.is<JsonArray>()) return;
        for (JsonVariant v : arrVar.as<JsonArray>()) {
            if (!v.is<JsonObject>()) continue;
            JsonObject o = v.as<JsonObject>();
            if (!(o["tMs"].is<unsigned long>() || o["tMs"].is<int>() || o["tMs"].is<double>())) continue;
            if (!(o["value"].is<double>() || o["value"].is<int>())) continue;
            out[cnt].tMs = static_cast<uint32_t>(o["tMs"].as<unsigned long>());
            out[cnt].value = o["value"].as<double>();
            if (++cnt >= TargetSequencer::kMaxFrames) break;
        }
    };

    bool hasFrames = false;
    if (root.containsKey("frames")) {
        JsonObject framesObj = root["frames"].as<JsonObject>();
        parseFrames(framesObj["az"], azBuf, azCount);
        parseFrames(framesObj["el"], elBuf, elCount);
        hasFrames = true;
    } else {
        if (root.containsKey("framesAz")) {
            parseFrames(root["framesAz"], azBuf, azCount);
            hasFrames = true;
        }
        if (root.containsKey("framesEl")) {
            parseFrames(root["framesEl"], elBuf, elCount);
            hasFrames = true;
        }
    }

    bool changed = router.parse(root, state);

    if (hasFrames) {
        state.load(azBuf, azCount, elBuf, elCount, state.isLooping());
        changed = true;
    }

    if (JsonSaveManager::needsToSave(root, getSaveMap())) changed = true;
    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}

void TargetSequencerJsonRouter::getSaveMap(JsonObject &root)
{
    root["frames"] = true;
    root["loop"] = true;
    root["target"] = true;
}

JsonDocument TargetSequencerJsonRouter::getSaveMap()
{
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    getSaveMap(obj);
    return doc;
}

TargetSequencerService::TargetSequencerService(PsychicHttpServer *server,
                                                   EventSocket *socket,
                                                   FS *fs,
                                                   SecurityManager *securityManager,
                                                   TargetSequencer &sequencer,
                                                   const char *restPath,
                                                   const char *settingsPath,
                                                   const char *eventName) :
    StatefulService(sequencer),
    _httpRouterEndpoint(_router.read, _router.update, this, server, restPath, securityManager),
    _fsPersistence(_router.readForSave, _router.update, this, fs, settingsPath),
    _eventEndpoint(_router.read, _router.update, this, socket, eventName) {}

void TargetSequencerService::begin()
{
    _eventEndpoint.begin();
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}

void TargetSequencerService::loop()
{
    if (_state.isRunning()) {
        _state.update();
    }
}
