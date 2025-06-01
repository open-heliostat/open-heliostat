#include "RemoteService.h"

JsonRouter<RemotesController> RemoteJsonRouter::router = JsonRouter<RemotesController>(
{
    {"addRemote", [](JsonVariant content, RemotesController& controller) {
        if (content.is<JsonObject>()) {
            JsonObject remote = content.as<JsonObject>();
            if (remote["ip"].is<String>() || remote["rxId"].is<uint32_t>()) {
                controller.addRemote(
                    remote["hostname"].as<String>(),
                    remote["ip"].as<String>(),
                    remote["rxId"].as<uint32_t>()
                );
                return true;
            }
        }
        return false;
    }},
    {"removeRemote", [](JsonVariant content, RemotesController& controller) {
        if (content.is<JsonObject>()) {
            JsonObject remote = content.as<JsonObject>();
            if (remote["ip"].is<String>()) {
                controller.removeRemote(remote["ip"].as<String>());
                return true;
            } else if (remote["rxId"].is<String>()) {
                controller.removeRemote(remote["rxId"].as<uint32_t>());
                return true;
            } else if (remote["index"].is<int>()) {
                controller.removeRemote(remote["index"].as<int>());
                return true;
            }
        }
        return false;
    }},
    {"clearRemotes", [](JsonVariant content, RemotesController& controller) {
        controller.clearRemotes();
        return true;
    }},
    {"remotes", [](JsonVariant content, RemotesController& controller) {
        if (content.is<JsonArray>()) {
            JsonArray remotes = content.as<JsonArray>();
            controller.clearRemotes();
            for (JsonVariant remote : remotes) {
                if (remote.is<JsonObject>()) {
                    JsonObject obj = remote.as<JsonObject>();
                    Remote rem;
                    if (obj["hostname"].is<String>()) rem.hostname = obj["hostname"].as<String>();
                    if (obj["ip"].is<String>()) rem.ip = obj["ip"].as<String>();
                    if (obj["rxId"].is<uint32_t>()) rem.rxId = obj["rxId"].as<uint32_t>();
                    if (obj["macAddress"].is<String>()) rem.macAddress = obj["macAddress"].as<String>();
                    controller.addRemote(rem);
                }
            }
            return true;
        }
        return false;
    }},
    {"settings", [](JsonVariant content, RemotesController& controller) {
        if (content.is<JsonObject>()) {
            JsonObject settings = content.as<JsonObject>();
            if (settings["isMaster"].is<bool>()) {
                controller.isMaster = settings["isMaster"].as<bool>();
                return true;
            }
        }
        return false;
    }},
    {"scanMDNS", [](JsonVariant content, RemotesController& controller) {
        controller.scanMDNS();
        return true;
    }},
},
{
    {"remotes", [](RemotesController& controller, JsonVariant content) {
        JsonArray remotes = content.to<JsonArray>();
        for (const auto& remote : controller.remotes) {
            JsonObject obj = remotes.add<JsonObject>();
            obj["hostname"] = remote.hostname;
            obj["ip"] = remote.ip;
            obj["rxId"] = remote.rxId;
            obj["macAddress"] = remote.macAddress;
        }
    }},
    {"settings", [](RemotesController& controller, JsonVariant content) {
        JsonObject settings = content.to<JsonObject>();
        settings["isMaster"] = controller.isMaster;
    }}
});

void RemoteService::begin()
{
    _httpRouterEndpoint.begin();
    _fsPersistence.readFromFS();
}

void RemoteService::loop()
{
    // No continuous operations needed
}