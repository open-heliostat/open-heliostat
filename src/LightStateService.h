#ifndef LightStateService_h
#define LightStateService_h

/**
 *   ESP32 SvelteKit
 *
 *   A simple, secure and extensible framework for IoT projects for ESP32 platforms
 *   with responsive Sveltekit front-end built with TailwindCSS and DaisyUI.
 *   https://github.com/theelims/ESP32-sveltekit
 *
 *   Copyright (C) 2018 - 2023 rjwats
 *   Copyright (C) 2023 - 2024 theelims
 *
 *   All Rights Reserved. This software may be modified and distributed under
 *   the terms of the LGPL v3 license. See the LICENSE file for details.
 **/

#include <LightMqttSettingsService.h>

#include <EventSocket.h>
#include <HttpEndpoint.h>
#include <MqttEndpoint.h>
#include <EventEndpoint.h>
#include <WebSocketServer.h>
#include <FeaturesService.h>

#define DEFAULT_LED_STATE false
#define OFF_STATE "OFF"
#define ON_STATE "ON"

#define LIGHT_SETTINGS_ENDPOINT_PATH "/rest/lightState"
#define LIGHT_SETTINGS_SOCKET_PATH "/ws/lightState"
#define LIGHT_SETTINGS_EVENT "led"

class LightState
{
public:
    bool ledOn;
    float red;
    float green;
    float blue;

    static void read(LightState &settings, JsonObject &root)
    {
        root["led_on"] = settings.ledOn;
        root["red"] = settings.red;
        root["green"] = settings.green;
        root["blue"] = settings.blue;
    }

    static StateUpdateResult update(JsonObject &root, LightState &lightState, const String &originId)
    {
        (void)originId;
        boolean newState = root["led_on"] | DEFAULT_LED_STATE;
        float red = root["red"] | 0.;
        float green = root["green"] | 0.;
        float blue = root["blue"] | 0.;
        if (lightState.ledOn != newState || lightState.red != red || lightState.green != green || lightState.blue != blue)
        {
            lightState.ledOn = newState;
            lightState.red = red;
            lightState.green = green;
            lightState.blue = blue;
            return StateUpdateResult::CHANGED;
        }
        return StateUpdateResult::UNCHANGED;
    }

    static void homeAssistRead(LightState &settings, JsonObject &root)
    {
        root["state"] = settings.ledOn ? ON_STATE : OFF_STATE;
    }

    static StateUpdateResult homeAssistUpdate(JsonObject &root, LightState &lightState, const String &originId)
    {
        (void)originId;
        String state = root["state"];
        // parse new led state
        boolean newState = false;
        if (state.equals(ON_STATE))
        {
            newState = true;
        }
        else if (!state.equals(OFF_STATE))
        {
            return StateUpdateResult::ERROR;
        }
        // change the new state, if required
        if (lightState.ledOn != newState)
        {
            lightState.ledOn = newState;
            return StateUpdateResult::CHANGED;
        }
        return StateUpdateResult::UNCHANGED;
    }
};

class LightStateService : public StatefulService<LightState>
{
public:
    LightStateService(PsychicHttpServer *server,
                      EventSocket *socket,
                      SecurityManager *securityManager,
                      PsychicMqttClient *mqttClient,
                      LightMqttSettingsService *lightMqttSettingsService,
                      FeaturesService *featuresService);

    void begin();
    void updateState(LightState lightState);

private:
    HttpEndpoint<LightState> _httpEndpoint;
    EventEndpoint<LightState> _eventEndpoint;
    MqttEndpoint<LightState> _mqttEndpoint;
    WebSocketServer<LightState> _webSocketServer;
    PsychicMqttClient *_mqttClient;
    LightMqttSettingsService *_lightMqttSettingsService;
    FeaturesService *_featuresService;

    void registerConfig();
    void onConfigUpdated();
};

#endif
