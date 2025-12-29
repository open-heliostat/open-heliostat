#include <GPSService.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

GPSStateService::GPSStateService(EventSocket *socket,
                                 GPSSettingsService *gpsSettingsService,
                                 SerialGPS *gps,
                                 FeaturesService *featuresService) :
                                                    _eventEndpoint(GPSState::read,
                                                                    GPSState::update,
                                                                    this,
                                                                    socket,
                                                                    GPS_STATE_EVENT),
                                                    _gpsSettingsService(gpsSettingsService),
                                                    _GPS(gps),
                                                    _featuresService(featuresService)
{
    _featuresService->addFeature("gps", true);
}

void GPSStateService::begin()
{
    _eventEndpoint.begin();
    updateState();
    startTask(1000);
}

void GPSStateService::loop() {
    if (_gpsSettingsService->isEnabled() && _GPS->update()) updateState();
}

void GPSStateService::taskThunk(void *param) {
    auto *svc = static_cast<GPSStateService *>(param);
    for (;;) {
        svc->loop();
        vTaskDelay(pdMS_TO_TICKS(svc->_taskPeriodMs));
    }
}

void GPSStateService::startTask(uint32_t periodMs) {
    _taskPeriodMs = periodMs;
    if (_taskHandle == nullptr) {
        xTaskCreatePinnedToCore(taskThunk, "gpsTask", 4096, this, 1, &_taskHandle, 0);
    }
}

void GPSStateService::updateState() {
    JsonDocument json;
    JsonObject jsonObject = json.to<JsonObject>();
    _state.readState(_GPS, jsonObject);
    update(jsonObject, _state.update, "driver");
}

GPSSettingsService::GPSSettingsService(PsychicHttpServer *server,
                                        FS *fs,
                                        SecurityManager *securityManager,
                                        SerialGPS *gps) :                  
                                                _httpEndpoint(GPSSettings::read,
                                                                GPSSettings::update,
                                                                this,
                                                                server,
                                                                GPS_SETTINGS_ENDPOINT,
                                                                securityManager,
                                                                AuthenticationPredicates::IS_AUTHENTICATED),
                                                _fsPersistence(GPSSettings::read, GPSSettings::update, this, fs, GPS_SETTINGS_FILE),
                                                _GPS(gps)
{
    // configure settings service update handler to update LED state
    addUpdateHandler([&](const String &originId)
                     { onConfigUpdated(); },
                     false);
}

void GPSSettingsService::begin()
{
    _httpEndpoint.begin();
    _fsPersistence.readFromFS();
    onConfigUpdated();
}

bool GPSSettingsService::isEnabled() {
    return _state.enabled;
}

void GPSSettingsService::onConfigUpdated()
{
}