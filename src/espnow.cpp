#include "espnow.h"
#include <ESPmDNS.h>
#include <ArduinoJson.h>

namespace ESPNow
{
    // Forward declaration of callback
    void staticReceiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
    const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t lastAddress[6];
    int maxSendRetries = 10;
    wifi_interface_t interface = WIFI_IF_AP;
    ESPNowState state = {};
    std::function<void(String)> messageCallback;
    std::vector<String> messageHistory = {};
    std::vector<ESPNowPeer> peerList;
    uint32_t pingInterval = 1000;
    uint32_t lastPingTimestamp = 0;
    uint32_t numReceived = 0;
    uint32_t retryDelay = 0;
    int messageHistorySize = 10;
    int pingID = 0;
    bool autoPing = false;

    // LabelClass *printLabel;

    // void (*remoteCb)(String name, RemoteESP &remote);

    void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
    // Formats MAC Address
    {
        snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    }

    void printAddress(const uint8_t *macAddr) {
        char macStr[18];
        formatMacAddress(macAddr, macStr, 18);
        Serial.print("Mac address is: ");
        Serial.println(macStr);
    }

    bool isPeer(const uint8_t *macAddr) {
        auto it = std::find_if(peerList.begin(), peerList.end(), [&macAddr](const ESPNowPeer &peer) {
            return memcmp(peer.peerInfo.peer_addr, macAddr, 6) == 0;
        });
        return it != peerList.end();
    }

    void addPeer(const uint8_t *macAddr) {
        esp_now_peer_info_t peerInfo = {};
        peerInfo.encrypt = false;
        memcpy(&peerInfo.peer_addr, macAddr, 6);
        peerInfo.channel = 0;
        peerInfo.ifidx = interface;
        peerList.push_back({peerInfo, 0, 0, 0, 0, 0, 0.0});
    }

    void addPeer(const String address) {
        uint8_t macAddr[6];
        sscanf(address.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &macAddr[0], &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]);
        addPeer(macAddr);
    }

    ESPNowPeer *getPeer(const uint8_t *macAddr) {
        auto it = std::find_if(peerList.begin(), peerList.end(), [&macAddr](const ESPNowPeer &peer) {
            return memcmp(peer.peerInfo.peer_addr, macAddr, 6) == 0;
        });
        if (it != peerList.end()) {
            return &(*it);
        }
        return nullptr;
    }

    void deletePeer(const uint8_t *macAddr) {
        auto it = std::remove_if(peerList.begin(), peerList.end(), [&macAddr](const ESPNowPeer &peer) {
            return memcmp(peer.peerInfo.peer_addr, macAddr, 6) == 0;
        });
        if (it != peerList.end()) {
            peerList.erase(it, peerList.end());
        }
    }

    bool sendMessage(const String &message, const uint8_t *macAddr)
    {
        esp_now_peer_info_t peerInfo = {};
        peerInfo.encrypt = false;
        memcpy(&peerInfo.peer_addr, macAddr, 6);
        if (!esp_now_is_peer_exist(macAddr))
        {
            esp_now_add_peer(&peerInfo);
        }
        esp_now_get_peer(macAddr, &peerInfo);
        peerInfo.ifidx = interface;
        esp_now_mod_peer(&peerInfo);
        // esp_wifi_config_espnow_rate(interface, );
        // Send message
        esp_err_t result = esp_now_send(macAddr, (const uint8_t *)message.c_str(), message.length());
        // delay(1);

        // Print results to serial monitor
        if (result == ESP_OK)
        {
            // Serial.println("Send message success");
            ESPNowPeer *peer = getPeer(macAddr);
            if (peer)
            {
                peer->numSent++;
            }
            return true;
        }
        else if (result == ESP_ERR_ESPNOW_NOT_INIT)
        {
            ESP_LOGI("ESP-NOW", "not Init.");
        }
        else if (result == ESP_ERR_ESPNOW_ARG)
        {
            Serial.println("Invalid Argument");
        }
        else if (result == ESP_ERR_ESPNOW_INTERNAL)
        {
            Serial.println("Internal Error");
        }
        else if (result == ESP_ERR_ESPNOW_NO_MEM)
        {
            Serial.println("ESP_ERR_ESPNOW_NO_MEM");
        }
        else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
        {
            Serial.println("Peer not found.");
        }
        else
        {
            Serial.println("Unknown error");
        }
        return false;
    }

    // void sendAll(const String &message) {
    //     for (auto & r : remoteESPMap) {
    //         if (r.first == "Broadcast") continue;
    //         r.second.sendMessage(message);
    //     }
    // }

    // void sendMessage(const String &message, String name) {
    //     if (name == "all") sendAll(message);
    //     else {
    //         auto it = remoteESPMap.find(name);
    //         if (it != remoteESPMap.end()) {
    //             it->second.sendMessage(message);
    //         }
    //         else Serial.println("Peer Not Found");
    //     }
    // }

    void broadcast(const String &message)
    // Emulates a broadcast
    {
        // Broadcast a message to every device in range
        sendMessage(message, broadcastAddress);
        for (auto &peer : peerList) {
            peer.numSent++;
        }
    }

    void broadcast(const String &message, int numRetries) {
        for (int i = 0; i < numRetries; i++) {
            broadcast(message);
            delay(retryDelay);
        }
    }

    void reply(const String &message)
    {
        sendMessage(message, lastAddress);
    }

    void ping(const uint8_t *macAddr)
    {
        if (!isPeer(macAddr)) {
            addPeer(macAddr);
            ESPNowPeer *peer = getPeer(macAddr);
            peer->pingTimestamp = millis();
            peer->numPings = 0;
        }
        else {
            ESPNowPeer *peer = getPeer(macAddr);
            peer->pingTimestamp = millis();
            peer->numPings++;
        }
        sendMessage("ping", macAddr);
    }

    // void setupPrintLabel(LabelClass *label)
    // {
    //     printLabel = label;
    // }

    void staticReceiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
    // Called when data is received
    {
        uint32_t now = millis();
        // bool isRegistered = remoteMap.count(std::string((const char*)macAddr)) != 0;
        esp_now_peer_info_t peerInfo;
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        memcpy(&peerInfo.peer_addr, macAddr, 6);
        memcpy(lastAddress, macAddr, 6);
        // Serial.println(isRegistered);
        // Only allow a maximum of 250 characters in the message + a null terminating byte
        char buffer[ESP_NOW_MAX_DATA_LEN + 1];
        int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
        strncpy(buffer, (const char *)data, msgLen);
        // Make sure we are null terminated
        buffer[msgLen] = 0;

        // char address[ESP_NOW_MAX_DATA_LEN + 1] = "";
        // char message[ESP_NOW_MAX_DATA_LEN + 1] = "";
        String stringMsg = String(buffer);

        uint8_t address[6];
        memcpy(&address, macAddr, 6);

        ESPNowPeer *peer = getPeer(macAddr);

        if (stringMsg == "ping")
        {
            sendMessage("pong", address);
        }
        else if (stringMsg == "pong")
        {
            if (peer)
            {
                peer->pingMeanTime = (peer->pingMeanTime * peer->numPings + (now - peer->pingTimestamp)) / (peer->numPings + 1);
                // ESP_LOGI("ESP-NOW", "Ping time: %d ms, Mean time: %f ms", now - peer->pingTimestamp, peer->pingMeanTime);
            }
        }
        
        // if (stringMsg == "Creatures?") sendMessage("Yes, master. My name is " + name, macAddr);
        // else if (stringMsg.substring(0, 24) == "Yes, master. My name is ") {
        //     // remoteESPs.push_back(peerInfo);
        //     // sendMessage("hello", peerInfo.peer_addr);
        //     String name = stringMsg.substring(24);
        //     RemoteESP newCreature = {peerInfo, name};
        //     auto it = remoteESPMap.emplace(name, newCreature);
        //     if (it.second && remoteCb) remoteCb(name, it.first->second);
        // }
        // else JsonSeq::readCommand(stringMsg);
        ESP_LOGI("ESP-NOW", "Received message: %s", stringMsg.c_str());
        auto& instance = ESPNowInstance::getInstance();
        if (instance.messageCallback) {
            // ESP_LOGI("ESP-NOW", "Invoking message callback");
            instance.messageCallback(stringMsg);
        }

        char macStr[18];
        formatMacAddress(macAddr, macStr, 18);

        // check if string only contains alphanumeric characters
        bool isAlphanumeric = true;
        for (int i = 0; i < stringMsg.length(); i++) {
            if (!isalnum(stringMsg[i]) && stringMsg[i] != ' ' && stringMsg[i] != ':' && stringMsg[i] != '{' && stringMsg[i] != '}') {
                isAlphanumeric = false;
                break;
            }
        }

        // check if string is valid JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, stringMsg);
        
        if (error && !isAlphanumeric) return;

        if (messageHistory.size() < messageHistorySize) {
            messageHistory.push_back(macStr + String(" : ") + stringMsg);
        }
        else
        {
            messageHistory.erase(messageHistory.begin());
            messageHistory.push_back(macStr + String(" : ") + stringMsg);
        }

        if (peer)
        {
            peer->numReceived++;
        }

        numReceived++;

        // const int numLines = 5;
        // static int lineIdx = 0;
        // static String lineBuffer[numLines];
        // char macStr[18];
        // formatMacAddress(macAddr, macStr, 18);
        // lineBuffer[lineIdx] = String(macStr) + String(" : ") + String(buffer);
        // lineIdx = (lineIdx + 1) % numLines;
        // String lines = "";
        // for (int i = 0; i < numLines; i++) {
        //     int index = (i + lineIdx) % numLines;
        //     if (i < numLines - 1) lines += lineBuffer[index] + " <br> ";
        //     else lines += lineBuffer[index];
        // }
        // Serial.println(lineBuffer[lineIdx]);
        // Serial.println(lines.c_str());

        // if (printLabel != NULL)
        // {   
        //     printLabel->update(lines);
        // }
    }

    void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
    // Called when data is sent
    {
        if (status != ESP_NOW_SEND_SUCCESS)
        {
            ESPNowPeer *peer = getPeer(macAddr);
            if (peer)
            {
                peer->numLost++;
                ESP_LOGI("ESP-NOW", "Lost packet to %s", macAddr);
            }
            // esp_now_peer_info_t peerInfo;
            // peerInfo.channel = 0;
            // peerInfo.encrypt = false;
            // memcpy(&peerInfo.peer_addr, macAddr, 6);
            // for (auto & r : remoteESPMap) {
            //     if (r.first == "Broadcast") continue;
            //     if (std::equal(peerInfo.peer_addr, peerInfo.peer_addr+6, r.second.peerInfo.peer_addr)) {
            //         r.second.RECV_OK = false;
            //         // Serial.println(r.second.name + " lost a packet.");
            //     }
            // }
            // char macStr[18];
            // formatMacAddress(macAddr, macStr, 18);
            // Serial.print("Last Packet Sent to: ");
            // Serial.print(macStr);
            // Serial.print(", Status: ");
            // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
        }
    }

    void setMessageCallback(std::function<void(String)> callback)
    {
        ESPNowInstance::getInstance().messageCallback = callback;
    }


    // void setRetryNum(int retryNum) {
    //     maxSendRetries = retryNum;
    //     for (auto & r : remoteESPMap) {
    //         r.second.maxRetries = maxSendRetries;
    //     }
    // }

    // void setRetryDelay(int retryDel) {
    //     retryDelay = retryDel;
    //     for (auto & r : remoteESPMap) {
    //         r.second.retryDelay = retryDelay;
    //     }
    // }

    // void loadPeers() {
    //     preferences.begin("espnowpeers");
    //     int numPeers = preferences.getInt("numPeers", 0);
    //     int i = 0;
    //     for (int i = 0; i < numPeers; i++) {
    //         esp_now_peer_info peerInfo;
    //         memcpy(&peerInfo.peer_addr, preferences.getString(("addr"+String(i)).c_str(),"000000").c_str(), 6);
    //         String name = preferences.getString(("name"+String(i)).c_str(), "");
    //         RemoteESP newCreature = {peerInfo, name};
    //         auto it = remoteESPMap.emplace(name, newCreature);
    //         if (it.second && remoteCb) remoteCb(name, it.first->second);
    //     }
    //     preferences.end();
    // }
    // void savePeers() {
    //     preferences.begin("espnowpeers");
    //     int numPeers = remoteESPMap.size();
    //     preferences.putInt("numPeers", numPeers);
    //     int i = 0;
    //     for (auto const& r : remoteESPMap) {
    //         preferences.putString(("name"+String(i)).c_str(), r.first);
    //         preferences.putString(("addr"+String(i)).c_str(), String((const char*)r.second.peerInfo.peer_addr));
    //         i++;
    //     }
    //     preferences.end();
    // }


    void getMacAddress(uint8_t *macAddr) {
        esp_wifi_get_mac(interface, macAddr);
    }

    String getMacAddress() {
        uint8_t macAddr[6];
        esp_wifi_get_mac(interface, macAddr);
        char macStr[18];
        formatMacAddress(macAddr, macStr, 18);
        return String(macStr);
    }

    void setup()
    {   
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        int channel = state.channel;
        WiFi.mode(WIFI_AP_STA);
        // esp_wifi_set_protocol(WIFI_IF_STA, uint8_t(WIFI_PROTOCOL_LR));
        // esp_wifi_set_protocol(WIFI_IF_STA, uint8_t(7));
        // preferences.begin("wireless");
        if (WiFi.status() == WL_CONNECTED) {
            // channel = WiFi.channel();
            // preferences.putInt("lastChannel", channel);
            interface = WIFI_IF_AP;
            // esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_LR );
            esp_wifi_set_protocol( WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N );
            esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N );
        }
        else {
            // channel = preferences.getInt("lastChannel", channel);
            interface = WIFI_IF_STA;
            esp_wifi_set_protocol( WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N );
            esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N );
            // esp_wifi_set_protocol( WIFI_IF_STA, WIFI_PROTOCOL_LR );
        }
        // preferences.end();
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);
        delay(10);
        // Initialize ESP-NOW
        if (esp_now_init() == ESP_OK)
        {
            ESP_LOGI("ESP-NOW", "Init Success");
            esp_now_register_recv_cb(staticReceiveCallback);
            esp_now_register_send_cb(sentCallback);
            // ping(broadcastAddress);
            // sendMessage("Creatures?", broadcastAddress);
            // esp_now_peer_info broadcastPeer;
            // memcpy(&broadcastPeer.peer_addr, broadcastAddress, 6);
            // remoteESPMap.emplace("Broadcast", broadcastPeer);
            // loadPeers();
            // WiFi.setSleep(0);
            esp_wifi_set_ps(WIFI_PS_NONE);
            esp_wifi_config_espnow_rate(WIFI_IF_AP, wifi_phy_rate_t(0));
            esp_wifi_config_espnow_rate(WIFI_IF_STA, wifi_phy_rate_t(0));
        }
        else
        {
            ESP_LOGI("ESP-NOW", "Init Failed");
        }
        MDNS.addServiceTxt("http", "tcp", "Mac Address", getMacAddress().c_str());
    }

    void update(unsigned long now)
    {
        if (autoPing && now - pingInterval > lastPingTimestamp) {
            lastPingTimestamp = now;
            if (peerList.size() > 0) {
                if (pingID >= peerList.size()) {
                    pingID = 0;
                }
                ping(peerList[pingID].peerInfo.peer_addr);
                pingID++;
            }
        }   
    }
}