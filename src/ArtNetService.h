#ifndef ArtNetService_h
#define ArtNetService_h

#include <Arduino.h>
#include <WiFiUdp.h>
#include <EventEndpoint.h>
#include <FSPersistence.h>
#include <HttpEndpoint.h>
#include <FeaturesService.h>
#include <ESP32SvelteKit.h>

#include <heliostat.h>

#define ARTNET_REST_PATH "/rest/artnet"
#define ARTNET_CONFIG_FILE "/config/artnet.json"
#define ARTNET_EVENT "artnet-service"
#define ARTNET_PORT 6454

struct ArtNetChannelConfig
{
	uint16_t coarseChannel = 0; // 1-512, 0 disables mapping
	uint16_t fineChannel = 0;   // optional, 0 to ignore
	double minValue = 0.0;
	double maxValue = 360.0;

	ArtNetChannelConfig() = default;
	ArtNetChannelConfig(uint16_t coarse, uint16_t fine, double minV, double maxV) :
		coarseChannel(coarse), fineChannel(fine), minValue(minV), maxValue(maxV) {}

	static void read(ArtNetChannelConfig &cfg, JsonObject &root);
	static StateUpdateResult update(JsonObject obj, ArtNetChannelConfig &cfg, const String &originId);
};

struct ArtNetStatus
{
	bool listening = false;
	uint32_t lastPacketMs = 0;
	uint16_t lastUniverse = 0;
	uint8_t lastSequence = 0;
	double lastAzimuth = 0.0;
	double lastElevation = 0.0;
};

struct ArtNetSettings
{
	bool enabled = false;
	bool takeoverHeliostat = true;         // when true, disable automatic heliostat reflection while Art-Net drives targets
	uint16_t universe = 0;                 // Art-Net universe to listen to
	uint16_t enableChannel = 0;            // optional channel to gate control; 0 disables gating
	uint8_t enableThreshold = 10;          // DMX value that turns control on when enableChannel is set
	ArtNetChannelConfig azimuthMapping = ArtNetChannelConfig(1, 2, 0.0, 360.0);
	ArtNetChannelConfig elevationMapping = ArtNetChannelConfig(3, 4, 0.0, 90.0);
	ArtNetStatus status;

	static void read(ArtNetSettings &settings, JsonObject &root);
	static StateUpdateResult update(JsonObject &root, ArtNetSettings &settings, const String &originId);
};

class ArtNetService : public StatefulService<ArtNetSettings>
{
public:
	ArtNetService(ESP32SvelteKit *kit,
				  HeliostatController *heliostat);
	void begin();
	void loop();

private:
	WiFiUDP _udp;
	HeliostatController *_heliostat;
	FeaturesService *_featuresService;
	bool _hasStoredHeliostatEnabled = false;
	bool _storedHeliostatEnabled = false;

	HttpEndpoint<ArtNetSettings> _httpEndpoint;
	EventEndpoint<ArtNetSettings> _eventEndpoint;
	FSPersistence<ArtNetSettings> _fsPersistence;

	void onConfigUpdated(const String &originId);
	void configureSocket();
	void handlePacket(size_t packetSize);
	void applyDmxFrame(const uint8_t *dmx, size_t length, uint8_t sequence, uint16_t universe);
	bool frameIsEnabled(const uint8_t *dmx, size_t length) const;
	bool readChannelValue(const ArtNetChannelConfig &cfg, const uint8_t *dmx, size_t length, uint16_t &outValue) const;
	double mapToRange(const ArtNetChannelConfig &cfg, uint16_t value) const;
	void sendPollReply(const IPAddress &remoteIp, uint16_t remotePort);
};

#endif // ArtNetService_h
