#include "ESPNowService.h"

JsonRouter<ESPNowState> ESPNowJsonRouter::router = JsonRouter<ESPNowState>(
{
    {"sendMessage", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<JsonObject>()) {
            JsonObject obj = value.as<JsonObject>();
            if (obj["message"].is<String>() && obj["address"].is<String>()) {
                String message = obj["message"].as<String>();
                String address = obj["address"].as<String>();
                // Parse MAC address string to uint8_t array
                uint8_t macAddr[6];
                sscanf(address.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &macAddr[0], &macAddr[1], &macAddr[2],
                       &macAddr[3], &macAddr[4], &macAddr[5]);
                
                if (ESPNow::sendMessage(message, macAddr)) {
                    obj["error"] = "OK";
                } else {
                    obj["error"] = "FAILED";
                }
            }
        }
        return false;
    }},
    {"broadcast", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<String>()) {
            String message = value.as<String>();
            ESPNow::broadcast(message);
            return true;
        }
        else if (value.is<JsonObject>()) {
            JsonObject obj = value.as<JsonObject>();
            if (obj["message"].is<String>()) {
                String message = obj["message"].as<String>();
                if (obj["numRetries"].is<int>()) {
                    int numRetries = obj["numRetries"].as<int>();
                    ESPNow::broadcast(message, numRetries);
                }
                else ESPNow::broadcast(message);
                return true;
            }
        }
        return false;
    }},
    {"reply", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<String>()) {
            String message = value.as<String>();
            ESPNow::reply(message);
            return true;
        }
        return false;
    }},
    {"ping", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<String>()) {
            String address = value.as<String>();
            uint8_t macAddr[6];
            sscanf(address.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &macAddr[0], &macAddr[1], &macAddr[2],
                   &macAddr[3], &macAddr[4], &macAddr[5]);
            ESPNow::ping(macAddr);
            return true;
        }
        return false;
    }},
    {"enabled", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<bool>()) {
            controller.enabled = value.as<bool>();
            return true;
        }
        return false;
    }},
    {"channel", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<int>()) {
            controller.channel = value.as<int>();
            return true;
        }
        return false;
    }},
    {"retryDelay", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<int>()) {
            ESPNow::retryDelay = value.as<int>();
            return true;
        }
        return false;
    }},
    {"autoPing", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<bool>()) {
            ESPNow::autoPing = value.as<bool>();
            return true;
        }
        return false;
    }},
    {"messageHistorySize", [](JsonVariant value, ESPNowState& controller) {
        if (value.is<int>()) {
            ESPNow::messageHistorySize = value.as<int>();
            return true;
        }
        return false;
    }},
    },
{
    {"enabled", [](ESPNowState& controller, JsonVariant content) {
        content.set(controller.enabled);
    }},
    {"channel", [](ESPNowState& controller, JsonVariant content) {
        content.set(controller.channel);
    }},
    {"retryDelay", [](ESPNowState& controller, JsonVariant content) {
        content.set(ESPNow::retryDelay);
    }},
    {"autoPing", [](ESPNowState& controller, JsonVariant content) {
        content.set(ESPNow::autoPing);
    }},
    {"macAddress", [](ESPNowState& controller, JsonVariant content) {
        content.set(ESPNow::getMacAddress());
    }},
    {"numReceived", [](ESPNowState& controller, JsonVariant content) {
        content.set(ESPNow::numReceived);
    }},
    {"messageHistorySize", [](ESPNowState& controller, JsonVariant content) {
        content.set(ESPNow::messageHistorySize);
    }},
    {"messageHistory", [](ESPNowState& controller, JsonVariant content) {
        JsonArray messages = content.to<JsonArray>();
        // create array from message history vector
        int size = ESPNow::messageHistory.size();
        if (size > 0) {
            String msgs[size];
            for (int i = 0; i < size; i++) {
                msgs[i] = ESPNow::messageHistory[i];
            }
            copyArray(msgs, size, messages);
        }
    }},
    {"peers", [](ESPNowState& controller, JsonVariant content) {
        JsonArray peers = content.to<JsonArray>();
        for (const auto& peer : ESPNow::peerList) {
            JsonObject peerObj = peers.add<JsonObject>();
            char macStr[18];
            ESPNow::formatMacAddress(peer.peerInfo.peer_addr, macStr, 18);
            peerObj["mac"] = macStr;
            peerObj["numPings"] = peer.numPings;
            peerObj["numSent"] = peer.numSent;
            peerObj["numReceived"] = peer.numReceived;
            peerObj["numLost"] = peer.numLost;
            peerObj["pingMeanTime"] = peer.pingMeanTime;
        }
    }}
});

void ESPNowService::begin()
{
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
    ESPNow::setup();
}

void ESPNowService::loop()
{
    ESPNow::update(millis());
}