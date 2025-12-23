#include <EncoderService.h>

EncoderStateService::EncoderStateService(EventSocket *socket,
                                Encoder *encoder) :
                                                    _eventEndpoint(EncoderState::read,
                                                                    EncoderState::update,
                                                                    this,
                                                                    socket,
                                                                    ENCODER_STATE_EVENT),
                                                    _encoder(encoder)
{
}

void EncoderStateService::begin()
{
    _eventEndpoint.begin();
    updateState();
}

void EncoderStateService::loop() {
    if (_encoder->update()) updateState();
}

void EncoderStateService::updateState() {
    JsonDocument json;
    JsonObject jsonObject = json.to<JsonObject>();
    _state.readState(_encoder, jsonObject);
    update(jsonObject, _state.update, "encoder");
}