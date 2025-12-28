#include <ArtNetService.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <WiFi.h>
#include <esp_log.h>

static constexpr const char kArtNetMagic[8] = {'A', 'r', 't', '-', 'N', 'e', 't', 0x00};

// --- ArtNetChannelConfig helpers -------------------------------------------------
void ArtNetChannelConfig::read(ArtNetChannelConfig &cfg, JsonObject &root)
{
    root["coarse"] = cfg.coarseChannel;
    root["fine"] = cfg.fineChannel;
    root["min"] = cfg.minValue;
    root["max"] = cfg.maxValue;
}

StateUpdateResult ArtNetChannelConfig::update(JsonObject root, ArtNetChannelConfig &cfg, const String &originId)
{
    (void)originId;
    bool changed = false;

    if (root["coarse"].is<uint16_t>())
    {
        uint16_t coarse = root["coarse"].as<uint16_t>();
        if (coarse <= 512)
        {
            cfg.coarseChannel = coarse;
            changed = true;
        }
    }
    if (root["fine"].is<uint16_t>())
    {
        uint16_t fine = root["fine"].as<uint16_t>();
        if (fine <= 512)
        {
            cfg.fineChannel = fine;
            changed = true;
        }
    }
    if (root["min"].is<double>())
    {
        cfg.minValue = root["min"].as<double>();
        changed = true;
    }
    if (root["max"].is<double>())
    {
        cfg.maxValue = root["max"].as<double>();
        changed = true;
    }

    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}

// --- ArtNetSettings helpers ------------------------------------------------------
void ArtNetSettings::read(ArtNetSettings &settings, JsonObject &root)
{
    root["enabled"] = settings.enabled;
    root["takeoverHeliostat"] = settings.takeoverHeliostat;
    root["universe"] = settings.universe;
    root["enableChannel"] = settings.enableChannel;
    root["enableThreshold"] = settings.enableThreshold;

    JsonObject az = root["azimuth"].to<JsonObject>();
    ArtNetChannelConfig::read(settings.azimuthMapping, az);

    JsonObject el = root["elevation"].to<JsonObject>();
    ArtNetChannelConfig::read(settings.elevationMapping, el);

    JsonObject status = root["status"].to<JsonObject>();
    status["listening"] = settings.status.listening;
    status["lastPacketMs"] = settings.status.lastPacketMs;
    status["lastPacketAgeMs"] = settings.status.lastPacketMs == 0 ? 0 : (millis() - settings.status.lastPacketMs);
    status["lastUniverse"] = settings.status.lastUniverse;
    status["lastSequence"] = settings.status.lastSequence;
    status["lastAzimuth"] = settings.status.lastAzimuth;
    status["lastElevation"] = settings.status.lastElevation;
}

StateUpdateResult ArtNetSettings::update(JsonObject &root, ArtNetSettings &settings, const String &originId)
{
    (void)originId;
    bool changed = false;

    if (root["enabled"].is<bool>())
    {
        settings.enabled = root["enabled"].as<bool>();
        changed = true;
    }
    if (root["takeoverHeliostat"].is<bool>())
    {
        settings.takeoverHeliostat = root["takeoverHeliostat"].as<bool>();
        changed = true;
    }
    if (root["universe"].is<uint16_t>())
    {
        settings.universe = root["universe"].as<uint16_t>();
        changed = true;
    }
    if (root["enableChannel"].is<uint16_t>())
    {
        uint16_t channel = root["enableChannel"].as<uint16_t>();
        if (channel <= 512)
        {
            settings.enableChannel = channel;
            changed = true;
        }
    }
    if (root["enableThreshold"].is<uint8_t>())
    {
        settings.enableThreshold = root["enableThreshold"].as<uint8_t>();
        changed = true;
    }

    if (root["azimuth"].is<JsonObject>())
    {
        JsonObject az = root["azimuth"].as<JsonObject>();
        if (ArtNetChannelConfig::update(az, settings.azimuthMapping, originId) == StateUpdateResult::CHANGED)
        {
            changed = true;
        }
    }
    if (root["elevation"].is<JsonObject>())
    {
        JsonObject el = root["elevation"].as<JsonObject>();
        if (ArtNetChannelConfig::update(el, settings.elevationMapping, originId) == StateUpdateResult::CHANGED)
        {
            changed = true;
        }
    }

    if (settings.azimuthMapping.maxValue < settings.azimuthMapping.minValue)
    {
        std::swap(settings.azimuthMapping.maxValue, settings.azimuthMapping.minValue);
    }
    if (settings.elevationMapping.maxValue < settings.elevationMapping.minValue)
    {
        std::swap(settings.elevationMapping.maxValue, settings.elevationMapping.minValue);
    }

    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}

// --- ArtNetService ---------------------------------------------------------------
ArtNetService::ArtNetService(ESP32SvelteKit *kit,
                             HeliostatController *heliostat) : _heliostat(heliostat),
                                                              _featuresService(kit ? kit->getFeatureService() : nullptr),
                                                              _httpEndpoint(ArtNetSettings::read,
                                                                            ArtNetSettings::update,
                                                                            this,
                                                                            kit ? kit->getServer() : nullptr,
                                                                            ARTNET_REST_PATH,
                                                                            kit ? kit->getSecurityManager() : nullptr,
                                                                            AuthenticationPredicates::IS_ADMIN),
                                                              _eventEndpoint(ArtNetSettings::read,
                                                                              ArtNetSettings::update,
                                                                              this,
                                                                              kit ? kit->getSocket() : nullptr,
                                                                              ARTNET_EVENT),
                                                              _fsPersistence(ArtNetSettings::read,
                                                                             ArtNetSettings::update,
                                                                             this,
                                                                             kit ? kit->getFS() : nullptr,
                                                                             ARTNET_CONFIG_FILE)
{
    addUpdateHandler([&](const String &originId)
                     { onConfigUpdated(originId); },
                     false);
}

void ArtNetService::begin()
{
    _httpEndpoint.begin();
    _eventEndpoint.begin();
    _fsPersistence.readFromFS();
    if (_featuresService)
    {
        _featuresService->addFeature("artnet", true);
    }
    onConfigUpdated("begin");
}

void ArtNetService::loop()
{
    if (!_state.enabled)
    {
        return;
    }

    // Pull packets as fast as they arrive while keeping loop non-blocking
    int packetSize = _udp.parsePacket();
    while (packetSize > 0)
    {
        handlePacket(static_cast<size_t>(packetSize));
        packetSize = _udp.parsePacket();
    }
}

void ArtNetService::onConfigUpdated(const String &originId)
{
    // Only reconfigure for user-originated changes, not driver runtime updates
    if (originId == "driver")
    {
        return;
    }

    if (!_state.enabled && _heliostat && _hasStoredHeliostatEnabled)
    {
        _heliostat->enabled = _storedHeliostatEnabled;
        _hasStoredHeliostatEnabled = false;
    }
    configureSocket();
}

void ArtNetService::configureSocket()
{
    if (_state.enabled)
    {
        if (!_state.status.listening)
        {
            if (_udp.begin(ARTNET_PORT))
            {
                updateWithoutPropagation([&](ArtNetSettings &settings)
                                         {
                                             settings.status.listening = true;
                                             return StateUpdateResult::UNCHANGED;
                                         },
                                         "driver");
                ESP_LOGI("ArtNet", "Listening on UDP %d", ARTNET_PORT);
            }
            else
            {
                ESP_LOGE("ArtNet", "Failed to bind UDP port %d", ARTNET_PORT);
            }
        }
    }
    else
    {
        _udp.stop();
        updateWithoutPropagation([&](ArtNetSettings &settings)
                                 {
                                     settings.status.listening = false;
                                     return StateUpdateResult::UNCHANGED;
                                 },
                                 "driver");
    }
}

void ArtNetService::handlePacket(size_t packetSize)
{
    if (packetSize < 18)
    {
        _udp.flush();
        return;
    }

    uint8_t buffer[550];
    const size_t readLen = min(packetSize, sizeof(buffer));
    int received = _udp.read(buffer, readLen);
    if (received < 18)
    {
        return;
    }

    if (memcmp(buffer, kArtNetMagic, sizeof(kArtNetMagic)) != 0)
    {
        return;
    }

    uint16_t opCode = static_cast<uint16_t>(buffer[8]) | (static_cast<uint16_t>(buffer[9]) << 8);
    if (opCode == 0x2000) // ArtPoll
    {
        sendPollReply(_udp.remoteIP(), _udp.remotePort());
        return;
    }
    if (opCode != 0x5000)
    {
        return; // not ArtDMX
    }

    uint8_t sequence = buffer[12];
    uint16_t universe = static_cast<uint16_t>(buffer[14]) | (static_cast<uint16_t>(buffer[15]) << 8);

    uint16_t length = static_cast<uint16_t>(buffer[16] << 8 | buffer[17]);
    length = min<uint16_t>(length, received > 18 ? static_cast<uint16_t>(received - 18) : 0);

    if (universe != _state.universe)
    {
        return;
    }

    const uint8_t *dmx = buffer + 18;
    applyDmxFrame(dmx, length, sequence, universe);
}

void ArtNetService::applyDmxFrame(const uint8_t *dmx, size_t length, uint8_t sequence, uint16_t universe)
{
    if (!frameIsEnabled(dmx, length))
    {
        return;
    }

    uint16_t azValue = 0;
    uint16_t elValue = 0;
    bool hasAzimuth = readChannelValue(_state.azimuthMapping, dmx, length, azValue);
    bool hasElevation = readChannelValue(_state.elevationMapping, dmx, length, elValue);

    if (!hasAzimuth && !hasElevation)
    {
        return;
    }

    // Map to degrees and apply to heliostat
    SphericalCoordinate target = _heliostat ? _heliostat->getTarget() : SphericalCoordinate{0.0, 0.0};
    if (hasAzimuth)
    {
        target.azimuth = mapToRange(_state.azimuthMapping, azValue);
    }
    if (hasElevation)
    {
        target.elevation = mapToRange(_state.elevationMapping, elValue);
    }

    if (_heliostat)
    {
        if (_state.takeoverHeliostat)
        {
            if (!_hasStoredHeliostatEnabled)
            {
                _storedHeliostatEnabled = _heliostat->enabled;
                _hasStoredHeliostatEnabled = true;
            }
            _heliostat->enabled = false; // stop automatic sun tracking while Art-Net commands are active
        }
        _heliostat->azimuthController.enabled = true;
        _heliostat->elevationController.enabled = true;
        _heliostat->setPosition(target);
    }

    const uint32_t now = millis();
    updateWithoutPropagation([&](ArtNetSettings &settings)
                             {
                                 settings.status.lastPacketMs = now;
                                 settings.status.lastSequence = sequence;
                                 settings.status.lastUniverse = universe;
                                 if (hasAzimuth)
                                 {
                                     settings.status.lastAzimuth = target.azimuth;
                                 }
                                 if (hasElevation)
                                 {
                                     settings.status.lastElevation = target.elevation;
                                 }
                                 return StateUpdateResult::UNCHANGED;
                             },
                             "driver");
}

bool ArtNetService::frameIsEnabled(const uint8_t *dmx, size_t length) const
{
    if (_state.enableChannel == 0)
    {
        return true;
    }
    if (_state.enableChannel == 0 || _state.enableChannel > length)
    {
        return false;
    }
    uint8_t value = dmx[_state.enableChannel - 1];
    return value >= _state.enableThreshold;
}

bool ArtNetService::readChannelValue(const ArtNetChannelConfig &cfg, const uint8_t *dmx, size_t length, uint16_t &outValue) const
{
    if (cfg.coarseChannel == 0 || cfg.coarseChannel > length)
    {
        return false;
    }

    uint16_t coarse = dmx[cfg.coarseChannel - 1];
    uint16_t combined = coarse << 8; // scale 8-bit coarse to 16-bit space

    if (cfg.fineChannel > 0 && cfg.fineChannel <= length)
    {
        combined = (coarse << 8) | dmx[cfg.fineChannel - 1];
    }

    outValue = combined;
    return true;
}

double ArtNetService::mapToRange(const ArtNetChannelConfig &cfg, uint16_t value) const
{
    const double span = cfg.maxValue - cfg.minValue;
    if (fabs(span) < 1e-6)
    {
        return cfg.minValue;
    }
    double norm = static_cast<double>(value) / 65535.0;
    norm = std::max(0.0, std::min(1.0, norm));
    return cfg.minValue + norm * span;
}

void ArtNetService::sendPollReply(const IPAddress &remoteIp, uint16_t remotePort)
{
    // ArtPollReply is 239 bytes; we fill the key fields and zero the rest.
    uint8_t reply[239];
    memset(reply, 0, sizeof(reply));

    // ID
    memcpy(reply, kArtNetMagic, sizeof(kArtNetMagic));

    // OpCode (little endian 0x2100)
    reply[8] = 0x00;
    reply[9] = 0x21;

    IPAddress ip = WiFi.localIP();
    reply[10] = ip[0];
    reply[11] = ip[1];
    reply[12] = ip[2];
    reply[13] = ip[3];

    // Port hi/lo
    reply[14] = 0x19;
    reply[15] = 0x36;

    // Version hi/lo
    reply[16] = 0x00;
    reply[17] = 0x01;

    // Net/SubSwitch
    reply[18] = 0x00;
    reply[19] = 0x00;

    // OEM
    reply[20] = 0x00;
    reply[21] = 0x00;

    // Ubea version
    reply[22] = 0x00;

    // Status1 - indicate DHCP capable & port 15 bit clear, minimal
    reply[23] = 0x00;

    // ESTA manufacturer (dummy)
    reply[24] = 0x00;
    reply[25] = 0x00;

    const char *shortName = "OpenHelio";
    const char *longName = "Open Heliostat ArtNet";
    strncpy(reinterpret_cast<char *>(&reply[26]), shortName, 18);
    strncpy(reinterpret_cast<char *>(&reply[44]), longName, 64);

    const char *nodeReport = "#0001 [OK]";
    strncpy(reinterpret_cast<char *>(&reply[108]), nodeReport, 64);

    // Ports
    reply[172] = 0x00;        // NumPortsHi
    reply[173] = 0x01;        // NumPortsLo
    reply[174] = 0x80;        // PortTypes[0] - DMX output capable
    reply[182] = 0x80;        // GoodOutput[0] - data is being transmitted
    reply[190] = _state.universe & 0xFF; // SwOut[0] universe LSB

    // Style: 0x00 = Node
    reply[200] = 0x00;

    uint8_t mac[6];
    if (WiFi.macAddress(mac))
    {
        memcpy(&reply[201], mac, 6);
    }

    reply[207] = ip[0];
    reply[208] = ip[1];
    reply[209] = ip[2];
    reply[210] = ip[3];
    reply[211] = 1; // BindIndex

    // Status2
    reply[212] = 0x00;

    // Send reply back to requester
    _udp.beginPacket(remoteIp, remotePort);
    _udp.write(reply, sizeof(reply));
    _udp.endPacket();
}
