#ifndef SourceTargetSequencerService_h
#define SourceTargetSequencerService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <lib/HttpStateRouterEndpoint.h>
#include <lib/JsonStateRouter.h>
#include <SourceTargetSequencer.h>

using JsonStateRouting::JsonRouter;
using JsonStateRouting::JsonSaveManager;

class SourceTargetSequencerJsonRouter
{
public:
    static bool route(JsonVariant content, SourceTargetSequencer &sequencer)
    {
        return router.route(content, sequencer);
    }
    static void read(SourceTargetSequencer &state, JsonObject &root);
    static void readForSave(SourceTargetSequencer &state, JsonObject &root);
    static StateUpdateResult update(JsonObject &root, SourceTargetSequencer &state, const String &originId);
    static void getSaveMap(JsonObject &root);
    static JsonDocument getSaveMap();
    static JsonRouter<SourceTargetSequencer> router;
};

class SourceTargetSequencerService : public StatefulService<SourceTargetSequencer&>
{
public:
    SourceTargetSequencerService(PsychicHttpServer *server,
                             EventSocket *socket,
                             FS *fs,
                             SecurityManager *securityManager,
                             SourceTargetSequencer &sequencer,
                             const char *restPath,
                             const char *settingsPath,
                             const char *eventName);
    void begin();
    void loop();

private:
    HttpStateRouterEndpoint<SourceTargetSequencer&> _httpRouterEndpoint;
    FSPersistence<SourceTargetSequencer&> _fsPersistence;
    EventEndpoint<SourceTargetSequencer&> _eventEndpoint;
    SourceTargetSequencerJsonRouter _router;
};

#endif
