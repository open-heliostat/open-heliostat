#ifndef EncoderService_h
#define EncoderService_h

#include <EventEndpoint.h>

#include <encoder.h>

#define ENCODER_STATE_EVENT "encoder"

class EncoderState
{
public:
    double angle;

    static void read(EncoderState &state, JsonObject &root) {
        root["angle"] = state.angle;
    }

    static StateUpdateResult update(JsonObject &root, EncoderState &state, const String &originId) {
        (void)originId;
        bool changed = false;
        if (root["angle"].is<double>() & state.angle != root["angle"]) {
            state.angle = root["angle"];
            changed = true;
        }
        if (changed) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }

    static void readState(Encoder *encoder, JsonObject &root) {
        root["angle"] = encoder->angle;
    }
};

class EncoderStateService : public StatefulService<EncoderState>
{
public:
    EncoderStateService( EventSocket *socket,
                         Encoder *encoder);
    void begin();
    void updateState();
    void loop();

private:
    EventEndpoint<EncoderState> _eventEndpoint;

    void onConfigUpdated();
    Encoder *_encoder;
};
#endif