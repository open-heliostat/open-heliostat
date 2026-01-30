#pragma once

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <PsychicHttp.h>
#include <SecurityManager.h>
#include <HeliostatService.h>

#define POWER_MANAGEMENT_SERVICE_PATH "/rest/powerManagement"

class PowerManagementService
{
public:
    PowerManagementService(PsychicHttpServer *server, SecurityManager *securityManager, HeliostatController *controller);

    void begin();
    void loop();

    // Power saving modes
    void enablePowerSaving(bool enable);
    void setTrackingInterval(unsigned long intervalMs);
    void setSleepBetweenMoves(bool enable);
    void setMotorPowerTimeout(unsigned long timeout);
    void setWiFiPowerSave(bool enable);
    
    // Tracking control
    bool shouldAllowTracking();
    bool areMotorsAtTarget();

private:
    PsychicHttpServer *_server;
    SecurityManager *_securityManager;
    HeliostatController *_controller;

    // Power management state
    bool _powerSavingEnabled = true;
    unsigned long _trackingInterval = 30000; // 30 seconds between tracking updates
    bool _sleepBetweenMoves = true;
    unsigned long _motorPowerTimeout = 300000; // 5 minutes
    unsigned long _lastTrackingUpdate = 0;
    unsigned long _lastMotorActivity = 0;
    bool _motorsEnabled = true;
    bool _wifiPowerSave = false;
    bool _isMoving = false;

    // Helper functions
    void updateMotorPower();
    void enableMotors(bool enable);
    bool isDaytime();
    void enterLightSleep(unsigned long durationMs);

    // HTTP endpoints
    esp_err_t getPowerStatus(PsychicRequest *request);
    esp_err_t setPowerSettings(PsychicRequest *request, JsonVariant &json);
};
