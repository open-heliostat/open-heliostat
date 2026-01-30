#include "PowerManagementService.h"
#include <ArduinoJson.h>
#include <pins.h>

PowerManagementService::PowerManagementService(PsychicHttpServer *server, SecurityManager *securityManager, HeliostatController *controller)
    : _server(server), _securityManager(securityManager), _controller(controller)
{
    _lastMotorActivity = millis();
    _lastTrackingUpdate = millis();
    ESP_LOGI("PowerManagement", "PowerManagementService initialized");
}

void PowerManagementService::begin()
{
    ESP_LOGI("PowerManagement", "Registering HTTP endpoints...");
    
    _server->on(POWER_MANAGEMENT_SERVICE_PATH,
                HTTP_GET,
                _securityManager->wrapRequest(std::bind(&PowerManagementService::getPowerStatus, this, std::placeholders::_1),
                                              AuthenticationPredicates::IS_AUTHENTICATED));

    _server->on(POWER_MANAGEMENT_SERVICE_PATH,
                HTTP_POST,
                _securityManager->wrapCallback(
                    std::bind(&PowerManagementService::setPowerSettings, this, std::placeholders::_1, std::placeholders::_2),
                    AuthenticationPredicates::IS_ADMIN));

    ESP_LOGI("PowerManagement", "Registered endpoints: %s", POWER_MANAGEMENT_SERVICE_PATH);
}

void PowerManagementService::loop()
{
    if (!_powerSavingEnabled) return;

    unsigned long now = millis();
    
    // Check if motors have reached their target positions
    bool motorsAtTarget = areMotorsAtTarget();
    
    // Track when movement completes
    if (_isMoving && motorsAtTarget) {
        _isMoving = false;
        _lastMotorActivity = now;
        ESP_LOGI("PowerManagement", "Motors reached target, movement complete");
    }
    
    // Update motor power based on activity and time
    updateMotorPower();
    
    // Handle sleep between moves if enabled
    if (_sleepBetweenMoves && motorsAtTarget && _controller->enabled) {
        unsigned long timeSinceLastUpdate = now - _lastTrackingUpdate;
        unsigned long timeToSleep = 0;
        
        if (timeSinceLastUpdate < _trackingInterval) {
            timeToSleep = _trackingInterval - timeSinceLastUpdate;
            
            // Only sleep if we have significant time left (>1 second)
            if (timeToSleep > 1000) {
                ESP_LOGI("PowerManagement", "Entering light sleep for %lu ms", timeToSleep);
                enterLightSleep(timeToSleep);
            }
        }
    }
}

bool PowerManagementService::shouldAllowTracking()
{
    if (!_powerSavingEnabled) return true;
    
    unsigned long now = millis();
    unsigned long timeSinceLastUpdate = now - _lastTrackingUpdate;
    
    bool intervalReached = timeSinceLastUpdate >= _trackingInterval;
    bool motorsAtTarget = areMotorsAtTarget();
    
    if (intervalReached && motorsAtTarget) {
        _lastTrackingUpdate = now;
        _isMoving = true;
        ESP_LOGI("PowerManagement", "Allowing new tracking update");
        return true;
    }
    
    return false;
}

bool PowerManagementService::areMotorsAtTarget()
{
    if (!_controller->enabled) return true;
    
    // Calculate error for both motors
    double azimuthError = abs(_controller->azimuthController.angularDistance(
        _controller->azimuthController.getTarget(), 
        _controller->azimuthController.getAngle()));
    double elevationError = abs(_controller->elevationController.angularDistance(
        _controller->elevationController.getTarget(), 
        _controller->elevationController.getAngle()));
    
    // Motors are at target if both errors are within tolerance
    bool atTarget = (azimuthError < _controller->azimuthController.tolerance) && 
                   (elevationError < _controller->elevationController.tolerance);
    
    return atTarget;
}

void PowerManagementService::updateMotorPower()
{
    unsigned long now = millis();
    bool shouldEnableMotors = true;
    
    unsigned long timeSinceActivity = now - _lastMotorActivity;
    bool activityTimeout = !_controller->enabled && (timeSinceActivity > _motorPowerTimeout);
    bool nighttime = !isDaytime();

    // Check if motors should be powered down due to inactivity
    if (activityTimeout) {
        shouldEnableMotors = false;
    }

    // Check if it's nighttime (optional power saving)
    if (nighttime && !_controller->enabled) {
        shouldEnableMotors = false;
    }

    // Update motor power state
    if (shouldEnableMotors != _motorsEnabled) {
        enableMotors(shouldEnableMotors);
        _motorsEnabled = shouldEnableMotors;
    }
}

void PowerManagementService::enableMotors(bool enable)
{
    // Control the MOTEN pin
    digitalWrite(MOTEN, enable ? HIGH : LOW);
    
    ESP_LOGI("PowerManagement", "Motors %s", enable ? "enabled" : "disabled for power saving");
}

bool PowerManagementService::isDaytime()
{
    if (!_controller->isTimeSet()) {
        return true; // Default to daytime if time unknown
    }
    
    auto solarPos = _controller->getSolarPosition();
    bool daytime = solarPos.elevation > -10; // Sun is above -10 degrees (civil twilight)
    
    return daytime;
}

void PowerManagementService::enablePowerSaving(bool enable)
{
    _powerSavingEnabled = enable;
    if (!enable) {
        enableMotors(true); // Re-enable motors if power saving is disabled
        _motorsEnabled = true;
    }
}

void PowerManagementService::setTrackingInterval(unsigned long intervalMs)
{
    _trackingInterval = intervalMs;
    ESP_LOGI("PowerManagement", "Tracking interval set to %lu ms", intervalMs);
}

void PowerManagementService::setSleepBetweenMoves(bool enable)
{
    _sleepBetweenMoves = enable;
    ESP_LOGI("PowerManagement", "Sleep between moves %s", enable ? "enabled" : "disabled");
}

void PowerManagementService::setMotorPowerTimeout(unsigned long timeout)
{
    _motorPowerTimeout = timeout;
}

void PowerManagementService::enterLightSleep(unsigned long durationMs)
{
    // Configure light sleep to wake up after duration or on WiFi activity
    esp_sleep_enable_timer_wakeup(durationMs * 1000); // Convert to microseconds
    
    // Enter light sleep (keeps WiFi connection alive)
    esp_light_sleep_start();
    
    ESP_LOGI("PowerManagement", "Woke up from light sleep");
}

void PowerManagementService::setWiFiPowerSave(bool enable)
{
    _wifiPowerSave = enable;
    if (enable) {
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    } else {
        esp_wifi_set_ps(WIFI_PS_NONE);
    }
}

esp_err_t PowerManagementService::getPowerStatus(PsychicRequest *request)
{
    PsychicJsonResponse response = PsychicJsonResponse(request, false);
    JsonObject root = response.getRoot();

    unsigned long now = millis();
    unsigned long timeSinceActivity = now - _lastMotorActivity;
    unsigned long timeSinceLastUpdate = now - _lastTrackingUpdate;

    root["powerSavingEnabled"] = _powerSavingEnabled;
    root["trackingInterval"] = _trackingInterval;
    root["sleepBetweenMoves"] = _sleepBetweenMoves;
    root["motorPowerTimeout"] = _motorPowerTimeout;
    root["motorsEnabled"] = _motorsEnabled;
    root["wifiPowerSave"] = _wifiPowerSave;
    root["isDaytime"] = isDaytime();
    root["lastMotorActivity"] = _lastMotorActivity;
    root["timeSinceLastActivity"] = timeSinceActivity;
    root["lastTrackingUpdate"] = _lastTrackingUpdate;
    root["timeSinceLastUpdate"] = timeSinceLastUpdate;
    root["motorsAtTarget"] = areMotorsAtTarget();
    root["isMoving"] = _isMoving;

    return response.send();
}

esp_err_t PowerManagementService::setPowerSettings(PsychicRequest *request, JsonVariant &json)
{
    if (json.is<JsonObject>()) {
        JsonObject obj = json.as<JsonObject>();
        
        if (obj["powerSavingEnabled"].is<bool>()) {
            enablePowerSaving(obj["powerSavingEnabled"]);
        }
        
        if (obj["trackingInterval"].is<unsigned long>()) {
            setTrackingInterval(obj["trackingInterval"]);
        }
        
        if (obj["sleepBetweenMoves"].is<bool>()) {
            setSleepBetweenMoves(obj["sleepBetweenMoves"]);
        }
        
        if (obj["motorPowerTimeout"].is<unsigned long>()) {
            setMotorPowerTimeout(obj["motorPowerTimeout"]);
        }
        
        if (obj["wifiPowerSave"].is<bool>()) {
            setWiFiPowerSave(obj["wifiPowerSave"]);
        }
        
        return request->reply(200);
    }
    
    return request->reply(400);
}
