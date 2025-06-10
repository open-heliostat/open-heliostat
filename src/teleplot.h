#ifndef TELEPLOT_H
#define TELEPLOT_H

#include <WiFiUdp.h>

#define TELEPLOT_HOST "192.168.4.2"  // Your computer's IP
#define TELEPLOT_PORT 47269            // Default Teleplot port

extern WiFiUDP teleplotUDP;

// Format: >name:value:timestamp
#define TELEPLOT_SEND(name, value) \
    do { \
        String msg = (String(name) + ":" + String(value)); \
        teleplotUDP.beginPacket(TELEPLOT_HOST, TELEPLOT_PORT); \
        teleplotUDP.print(msg); \
        teleplotUDP.endPacket(); \
    } while(0)

#endif
