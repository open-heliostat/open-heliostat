#ifndef TargetSequencerService_h
#define TargetSequencerService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <lib/HttpStateRouterEndpoint.h>
#include <lib/JsonStateRouter.h>
#include <TargetSequencer.h>

using JsonStateRouting::JsonRouter;
using JsonStateRouting::JsonSaveManager;

class TargetSequencerJsonRouter
{
public:
    static bool route(JsonVariant content, TargetSequencer &sequencer)
    {
        return router.route(content, sequencer);
    }
    static void read(TargetSequencer &state, JsonObject &root);
    static void readForSave(TargetSequencer &state, JsonObject &root);
    static StateUpdateResult update(JsonObject &root, TargetSequencer &state, const String &originId);
    static void getSaveMap(JsonObject &root);
    static JsonDocument getSaveMap();
    static JsonRouter<TargetSequencer> router;
};

class TargetSequencerService : public StatefulService<TargetSequencer&>
{
public:
    TargetSequencerService(PsychicHttpServer *server,
                             EventSocket *socket,
                             FS *fs,
                             SecurityManager *securityManager,
                             TargetSequencer &sequencer,
                             const char *restPath,
                             const char *settingsPath,
                             const char *eventName);
    void begin();
    void loop();

private:
    HttpStateRouterEndpoint<TargetSequencer&> _httpRouterEndpoint;
    FSPersistence<TargetSequencer&> _fsPersistence;
    EventEndpoint<TargetSequencer&> _eventEndpoint;
    TargetSequencerJsonRouter _router;
};

#endif
