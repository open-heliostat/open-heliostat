#ifndef MovementSequencerService_h
#define MovementSequencerService_h

#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <lib/HttpStateRouterEndpoint.h>
#include <lib/JsonStateRouter.h>
#include <movementsequencer.h>

using JsonStateRouting::JsonRouter;
using JsonStateRouting::JsonSaveManager;

class MovementSequencerJsonRouter
{
public:
    static bool route(JsonVariant content, MovementSequencer &sequencer)
    {
        return router.route(content, sequencer);
    }
    static void read(MovementSequencer &state, JsonObject &root);
    static void readForSave(MovementSequencer &state, JsonObject &root);
    static StateUpdateResult update(JsonObject &root, MovementSequencer &state, const String &originId);
    static void getSaveMap(JsonObject &root);
    static JsonDocument getSaveMap();
    static JsonRouter<MovementSequencer> router;
};

class MovementSequencerService : public StatefulService<MovementSequencer&>
{
public:
    MovementSequencerService(PsychicHttpServer *server,
                             EventSocket *socket,
                             FS *fs,
                             SecurityManager *securityManager,
                             MovementSequencer &sequencer,
                             const char *restPath,
                             const char *settingsPath,
                             const char *eventName);
    void begin();
    void loop();

private:
    HttpStateRouterEndpoint<MovementSequencer&> _httpRouterEndpoint;
    FSPersistence<MovementSequencer&> _fsPersistence;
    EventEndpoint<MovementSequencer&> _eventEndpoint;
    MovementSequencerJsonRouter _router;
};

#endif
