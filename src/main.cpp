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

#include <ESP32SvelteKit.h>
#include <PsychicHttpServer.h>
#include <GPSService.h>
#include <HeliostatService.h>
#include <ArtNetService.h>
#include <MovementSequencerService.h>
#include <ESPNowService.h>
#include <pins.h>

#define SERIAL_BAUD_RATE 115200

PsychicHttpServer server;

ESP32SvelteKit esp32sveltekit(&server, 200);
Motor_Driver motor1 = {MOT1A, MOT1B, MOT1C, 0};
Motor_Driver motor2 = {MOT2A, MOT2B, MOT2C, 2};

Encoder encoder1 = Encoder(SDA1, SCL1);
Encoder encoder2 = Encoder(SDA2, SCL2, Wire1);

Servo_Driver servo1 = {motor1, encoder1};
Servo_Driver servo2 = {motor2, encoder2};

MovementSequencer azSequencer = MovementSequencer(servo1);
MovementSequencer elSequencer = MovementSequencer(servo2);

SerialGPS gpsneo = SerialGPS(Serial1, GPSRX, GPSTX);

HeliostatController heliostatController = {servo1, servo2, gpsneo};

MovementSequencerService azSequencerService = MovementSequencerService(
    &server,
    esp32sveltekit.getSocket(),
    esp32sveltekit.getFS(),
    esp32sveltekit.getSecurityManager(),
    azSequencer,
    "/rest/heliostat/azimuth/sequence",
    "/config/heliostat-az-sequence.json",
    "heliostat-az-sequence");

MovementSequencerService elSequencerService = MovementSequencerService(
    &server,
    esp32sveltekit.getSocket(),
    esp32sveltekit.getFS(),
    esp32sveltekit.getSecurityManager(),
    elSequencer,
    "/rest/heliostat/elevation/sequence",
    "/config/heliostat-el-sequence.json",
    "heliostat-el-sequence");

HeliostatService heliostatService = HeliostatService(
    &server,
    esp32sveltekit.getSocket(),
    esp32sveltekit.getFS(),
    esp32sveltekit.getSecurityManager(),
    heliostatController);

ArtNetService artNetService = ArtNetService(
    &esp32sveltekit,
    &heliostatController);

GPSSettingsService gpsSettingsService = GPSSettingsService(
    &server,
    esp32sveltekit.getFS(),
    esp32sveltekit.getSecurityManager(),
    &gpsneo);

GPSStateService gpsStateService =  GPSStateService(
    esp32sveltekit.getSocket(),
    &gpsSettingsService,
    &gpsneo,
    esp32sveltekit.getFeatureService());

ESPNowState espNowState;

ESPNowService espNowService = ESPNowService(
    &server,
    &esp32sveltekit,
    espNowState);

WiFiUDP teleplotUDP;

void setup()
{
    // start serial and filesystem
    Serial.begin(SERIAL_BAUD_RATE);

    // increase httpd stack for HttpJsonRouter
    server.config.stack_size = 8192;
    server.config.max_resp_headers = 12;
    server.config.max_open_sockets = 11;
    server.config.lru_purge_enable = true;

    // start ESP32-SvelteKit
    esp32sveltekit.begin();

    gpsSettingsService.begin();
    gpsneo.init();
    gpsStateService.begin();

    azSequencerService.begin();
    elSequencerService.begin();
    heliostatService.begin();
    artNetService.begin();
    espNowService.begin();

    esp32sveltekit.getFeatureService()->addFeature("motors", true);

    pinMode(MOTEN, OUTPUT); // ENABLE MOTOR DRIVER
    digitalWrite(MOTEN, HIGH);
    
    // closedLoopControllerService.begin();
}

unsigned long lastTick = 0;

void loop()
{
    // Delete Arduino loop task, as it is not needed in this example
    // vTaskDelete(NULL);
    static unsigned long lastLoopMs = 0;
    static unsigned long lastLoopWarnMs = 0;
    const unsigned long loopWarnMs = 50;   // warn if main loop iteration gap exceeds this
    const unsigned long loopWarnCooldownMs = 500;

    unsigned long loopStartUs = micros();
    unsigned long nowMs = millis();
    if (lastLoopMs != 0) {
        unsigned long gap = nowMs - lastLoopMs;
        if (gap > loopWarnMs && (nowMs - lastLoopWarnMs) > loopWarnCooldownMs) {
            Serial.printf("Main loop gap: %lu ms\n", gap);
            lastLoopWarnMs = nowMs;
        }
    }
    lastLoopMs = nowMs;

    // Profiling helper without C++14 generic lambdas to stay compatible with the build flags.
    auto prof = [&](const char *name, void (*fn)()) {
        unsigned long t0 = micros();
        fn();
        unsigned long dt = micros() - t0;
        if (dt > 20000) { // 20 ms section warning
            Serial.printf("Main section slow: %s %lu us\n", name, dt);
        }
    };

    prof("azSequencerService", +[](){ azSequencerService.loop(); });
    prof("elSequencerService", +[](){ elSequencerService.loop(); });
    prof("heliostatService",   +[](){ heliostatService.loop(); });
    prof("artNetService",      +[](){ artNetService.loop(); });
    prof("espNowService",      +[](){ espNowService.loop(); });

    unsigned long now = millis();
    if (now - lastTick > 1000) {
        lastTick = now;
        // gpsStateService moved to dedicated FreeRTOS task to avoid blocking control loop.
        // if (encoder1.hasNewData()) Serial.println(encoder1.angle);
        // if (encoder2.hasNewData()) Serial.println(encoder2.angle);
    }

    unsigned long loopDurUs = micros() - loopStartUs;
    if (loopDurUs > 50000 && (millis() - lastLoopWarnMs) > loopWarnCooldownMs) { // 50 ms total
        Serial.printf("Main loop total slow: %lu us\n", loopDurUs);
        lastLoopWarnMs = millis();
    }
}
