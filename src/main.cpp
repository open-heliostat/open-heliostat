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
#include <MovementSequencerService.h>
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

    gpsneo.init();
    gpsSettingsService.begin();
    gpsStateService.begin();

    azSequencerService.begin();
    elSequencerService.begin();
    heliostatService.begin();

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
    azSequencerService.loop();
    elSequencerService.loop();
    heliostatService.loop();
    unsigned long now = millis();
    if (now - lastTick > 1000) {
        lastTick = now;
        gpsStateService.loop();
        // if (encoder1.hasNewData()) Serial.println(encoder1.angle);
        // if (encoder2.hasNewData()) Serial.println(encoder2.angle);
    }
}
