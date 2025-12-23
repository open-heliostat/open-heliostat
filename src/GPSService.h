#ifndef GPSService_h
#define GPSService_h

#include <HttpEndpoint.h>
#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <FeaturesService.h>

#include <gpsneo.h>

#define GPS_STATE_EVENT "gps"
#define GPS_SETTINGS_ENDPOINT "/rest/gpsSettings"
#define GPS_SETTINGS_FILE "/config/gpsSettings.json"

class GPSSettings
{
public:
    bool enabled;

    static void read(GPSSettings &settings, JsonObject &root) {
        root["enabled"] = settings.enabled;
    }

    static StateUpdateResult update(JsonObject &root, GPSSettings &settings, const String &originId) {
        (void)originId;
        if (root["enabled"].is<bool>() & settings.enabled != root["enabled"]) {
            settings.enabled = root["enabled"];
            return StateUpdateResult::CHANGED;
        }
        else return StateUpdateResult::UNCHANGED;
    }
};

class GPSSettingsService : public StatefulService<GPSSettings>
{
public:
    GPSSettingsService( PsychicHttpServer *server, 
                        FS *fs,
                        SecurityManager *securityManager,
                        SerialGPS *gps);
    void begin();
    bool isEnabled();

private:
    HttpEndpoint<GPSSettings> _httpEndpoint;
    FSPersistence<GPSSettings> _fsPersistence;
    SerialGPS *_GPS;

    void onConfigUpdated();
};

class GPSState
{
public:
    double latitude;
    double longitude;
    double altitude;
    int numSats;
    int fixQuality;
    char timeStr[16];
    char dateStr[16];
    uint32_t sinceLastUpdate;
    bool hasSerial;

    static void read(GPSState &state, JsonObject &root) {
        root["latitude"] = state.latitude;
        root["longitude"] = state.longitude;
        root["altitude"] = state.altitude;
        root["numSats"] = state.numSats;
        root["fixQuality"] = state.fixQuality;
        root["timeStr"] = state.timeStr;
        root["dateStr"] = state.dateStr;
        root["sinceLastUpdate"] = state.sinceLastUpdate;
        root["hasSerial"] = state.hasSerial;
    }

    static StateUpdateResult update(JsonObject &root, GPSState &state, const String &originId) {
        (void)originId;
        bool changed = false;
        if (root["latitude"].is<double>() & state.latitude != root["latitude"]) {
            state.latitude = root["latitude"];
            changed = true;
        }
        if (root["longitude"].is<double>() & state.longitude != root["longitude"]) {
            state.longitude = root["longitude"];
            changed = true;
        }
        if (root["altitude"].is<double>() & state.altitude != root["altitude"]) {
            state.altitude = root["altitude"];
            changed = true;
        }
        if (root["numSats"].is<int>() & state.numSats != root["numSats"]) {
            state.numSats = root["numSats"];
            changed = true;
        }
        if (root["fixQuality"].is<int>() & state.fixQuality != root["fixQuality"]) {
            state.fixQuality = root["fixQuality"];
            changed = true;
        }
        if (root["timeStr"].is<const char*>() & strcmp(state.timeStr, root["timeStr"]) != 0) {
            strcpy(state.timeStr, root["timeStr"]);
            changed = true;
        }
        if (root["dateStr"].is<const char*>() & strcmp(state.dateStr, root["dateStr"]) != 0) {
            strcpy(state.dateStr, root["dateStr"]);
            changed = true;
        }
        if (root["sinceLastUpdate"].is<int>() & state.sinceLastUpdate != root["sinceLastUpdate"]) {
            state.sinceLastUpdate = root["sinceLastUpdate"];
            changed = true;
        }
        if (root["hasSerial"].is<bool>() & state.hasSerial != root["hasSerial"]) {
            state.hasSerial = root["hasSerial"];
            changed = true;
        }
        if (changed) return StateUpdateResult::CHANGED;
        else return StateUpdateResult::UNCHANGED;
    }

    static void readState(SerialGPS *gps, JsonObject &root) {
        root["latitude"] = gps->coords.latitude;
        root["longitude"] = gps->coords.longitude;
        root["altitude"] = gps->coords.altitude;
        root["numSats"] = gps->numSats;
        root["fixQuality"] = gps->fixQuality;
        root["timeStr"] = gps->timeStr;
        root["dateStr"] = gps->dateStr;
        root["sinceLastUpdate"] = gps->sinceLastUpdate;
        root["hasSerial"] = gps->hasSerial;
    }
};

class GPSStateService : public StatefulService<GPSState>
{
public:
    GPSStateService( EventSocket *socket,
                     GPSSettingsService *gpsSettingsService,
                     SerialGPS *gps,
                     FeaturesService *featuresService);
    void begin();
    void updateState();
    void loop();

private:
    EventEndpoint<GPSState> _eventEndpoint;
    GPSSettingsService *_gpsSettingsService;
    SerialGPS *_GPS;
    FeaturesService *_featuresService;

    void onConfigUpdated();
};
#endif