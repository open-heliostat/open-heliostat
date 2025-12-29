#ifndef GPSNEO
#define GPSNEO
#include <Arduino.h>
#include "WiFi.h"
#include <WiFiUdp.h>

#include <TinyGPS++.h>
#include "TimeLib.h"
#include <time.h>
#include <sys/time.h>
#include <cstdlib>
#include <cstring>

struct GeoCoords {
    double longitude;
    double latitude;
    double altitude;
};

class SerialGPS {
private:
    const uint8_t TX;
    const uint8_t RX;
    HardwareSerial &serial;
    TinyGPSPlus gps;

public:
    GeoCoords coords;
    int fixQuality;
    int numSats = 0;
    uint32_t lastUpdate = 0;
    uint32_t sinceLastUpdate = 0;
    char timeStr[16];
    char dateStr[16];
    bool hasSerial = false;

    SerialGPS(HardwareSerial &serial_ = Serial1, uint8_t RX_ = 25, uint8_t TX_ = 33) : serial(serial_), RX(RX_), TX(TX_) {}
    void init() {
        serial.begin(9600, SERIAL_8N1, RX, TX);
    }
    bool update() {
        unsigned long now = millis();
        bool updated = false;
        if (serial.available() > 0) {
            while (serial.available() > 0) {
                int s = serial.read();
                gps.encode(s);
                // Serial.print(char(s));
            }
            if (gps.satellites.isUpdated()) {
                numSats = gps.satellites.value();
            }
            if (gps.time.isValid()) {
                sprintf(timeStr, "%i:%i:%i", gps.time.hour(), gps.time.minute(), gps.time.second());
                sprintf(dateStr, "%i/%i/%i", gps.date.day(), gps.date.month(), gps.date.year());
                
                // Update TimeLib time
                setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
                
                // Also update system time so NTP and GPS time stay in sync (GPS time is UTC)
                struct tm timeinfo = {0};
                timeinfo.tm_year = gps.date.year() - 1900;  // tm_year is years since 1900
                timeinfo.tm_mon = gps.date.month() - 1;     // tm_mon is 0-11
                timeinfo.tm_mday = gps.date.day();
                timeinfo.tm_hour = gps.time.hour();
                timeinfo.tm_min = gps.time.minute();
                timeinfo.tm_sec = gps.time.second();
                timeinfo.tm_isdst = 0;
                // Convert as UTC regardless of configured TZ
#if defined(__USE_MISC) || defined(__GNU_SOURCE) || defined(_BSD_SOURCE)
                time_t gpsTime = timegm(&timeinfo);
#else
                char previousTz[64] = {0};
                const char *tz = getenv("TZ");
                if (tz) {
                    strncpy(previousTz, tz, sizeof(previousTz) - 1);
                }
                setenv("TZ", "UTC0", 1);
                tzset();
                time_t gpsTime = mktime(&timeinfo);
                if (previousTz[0] != '\0') {
                    setenv("TZ", previousTz, 1);
                } else {
                    unsetenv("TZ");
                }
                tzset();
#endif

                struct timeval tv = {.tv_sec = gpsTime, .tv_usec = 0};
                settimeofday(&tv, nullptr);
                
                // updated = true;
            }
            if (gps.location.isUpdated()) {
                coords.latitude = gps.location.lat();
                coords.longitude = gps.location.lng();
                coords.altitude = gps.altitude.meters();
                fixQuality = gps.location.FixQuality();
                updated = true;
            }
            if (!hasSerial) {
                updated = true;
                hasSerial = true;
            }
            lastUpdate = now;
        }
        else if (hasSerial && now - lastUpdate > 10000) {
            hasSerial = false;
            updated = true;
        }
        return updated;
    }
};

// #define NMEAGPS_DERIVED_TYPES
// #define NMEAGPS_PARSE_PROPRIETARY
// #define NMEAGPS_PARSE_MFR_ID
// #include <ublox/ubxGPS.h>
// #define DEBUG_PORT Serial
// #include <NeoGPS_cfg.h>
// #include <ublox/ubxGPS.h>
// #include <Streamers.h>
// class MyGPS : public ubloxGPS
// {
// public:

//     enum
//       {
//         GETTING_STATUS, 
//         GETTING_LEAP_SECONDS, 
//         GETTING_UTC, 
//         RUNNING
//       }
//         state NEOGPS_BF(8);

//     MyGPS( Stream *device ) : ubloxGPS( device )
//     {
//       state = GETTING_STATUS;
//     }

//     //--------------------------

//     void get_status()
//     {
//       static bool acquiring = false;

//       if (fix().status == gps_fix::STATUS_NONE) {
//         static uint32_t dotPrint;
//         bool            requestNavStatus = false;

//         if (!acquiring) {
//           acquiring = true;
//           dotPrint = millis();
//           DEBUG_PORT.print( F("Acquiring...") );
//           requestNavStatus = true;

//         } else if (millis() - dotPrint > 1000UL) {
//           dotPrint = millis();
//           DEBUG_PORT << '.';

//           static uint8_t requestPeriod;
//           if ((++requestPeriod & 0x07) == 0)
//             requestNavStatus = true;
//         }

//         if (requestNavStatus)
//           // Turn on the UBX status message
//           enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_STATUS );

//       } else {
//         if (acquiring)
//           DEBUG_PORT << '\n';
//         DEBUG_PORT << F("Acquired status: ") << (uint8_t) fix().status << '\n';

//         #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE) & \
//             defined(UBLOX_PARSE_TIMEGPS)

//           if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS ))
//             DEBUG_PORT.println( F("enable TIMEGPS failed!") );

//           state = GETTING_LEAP_SECONDS;
//         #else
//           start_running();
//           state = RUNNING;
//         #endif
//       }
//     } // get_status

//     //--------------------------

//     void get_leap_seconds()
//     {
//       #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE) & \
//           defined(UBLOX_PARSE_TIMEGPS)

//         if (GPSTime::leap_seconds != 0) {
//           DEBUG_PORT << F("Acquired leap seconds: ") << GPSTime::leap_seconds << '\n';

//           if (!disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEGPS ))
//             DEBUG_PORT.println( F("disable TIMEGPS failed!") );

//           #if defined(UBLOX_PARSE_TIMEUTC)
//             if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
//               DEBUG_PORT.println( F("enable TIMEUTC failed!") );
//             state = GETTING_UTC;
//           #else
//             start_running();
//           #endif
//         }
//       #endif

//     } // get_leap_seconds

//     //--------------------------

//     void get_utc()
//     {
//       #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE) & \
//           defined(UBLOX_PARSE_TIMEUTC)

//         lock();
//           bool            safe = is_safe();
//           NeoGPS::clock_t sow  = GPSTime::start_of_week();
//         //   NeoGPS::time_t  utc  = fix().dateTime;
//         unlock();

//         if (safe && (sow != 0)) {
//         //   DEBUG_PORT << F("Acquired UTC: ") << utc << '\n';
//           DEBUG_PORT << F("Acquired Start-of-Week: ") << sow << '\n';

//           start_running();
//         }
//       #endif

//     } // get_utc

//     //--------------------------

//     void start_running()
//     {
//       bool enabled_msg_with_time = false;

//       #if defined(UBLOX_PARSE_POSLLH)
//         if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_POSLLH ))
//           DEBUG_PORT.println( F("enable POSLLH failed!") );

//         enabled_msg_with_time = true;
//       #endif

//       #if defined(UBLOX_PARSE_PVT)
//         if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_PVT ))
//           DEBUG_PORT.println( F("enable PVT failed!") );

//         enabled_msg_with_time = true;
//       #endif

//       #if defined(UBLOX_PARSE_VELNED)
//         if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_VELNED ))
//           DEBUG_PORT.println( F("enable VELNED failed!") );

//         enabled_msg_with_time = true;
//       #endif

//       #if defined(UBLOX_PARSE_DOP)
//         if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_DOP ))
//           DEBUG_PORT.println( F("enable DOP failed!") );
//         else
//           DEBUG_PORT.println( F("enabled DOP.") );

//         enabled_msg_with_time = true;
//       #endif

//       #if defined(UBLOX_PARSE_SVINFO)
//         if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_SVINFO ))
//           DEBUG_PORT.println( F("enable SVINFO failed!") );
        
//         enabled_msg_with_time = true;
//       #endif

//       #if defined(UBLOX_PARSE_TIMEUTC)

//         #if defined(GPS_FIX_TIME) & defined(GPS_FIX_DATE)
//           if (enabled_msg_with_time &&
//               !disable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
//             DEBUG_PORT.println( F("disable TIMEUTC failed!") );

//         #elif defined(GPS_FIX_TIME) | defined(GPS_FIX_DATE)
//           // If both aren't defined, we can't convert TOW to UTC,
//           // so ask for the separate UTC message.
//           if (!enable_msg( ublox::UBX_NAV, ublox::UBX_NAV_TIMEUTC ))
//             DEBUG_PORT.println( F("enable TIMEUTC failed!") );
//         #endif

//       #endif

//       state = RUNNING;
//       trace_header( DEBUG_PORT );

//     } // start_running

//     //--------------------------

//     bool running()
//     {
//       switch (state) {
//         case GETTING_STATUS      : get_status      (); break;
//         case GETTING_LEAP_SECONDS: get_leap_seconds(); break;
//         case GETTING_UTC         : get_utc         (); break;
//       }

//       return (state == RUNNING);

//     } // running

// } NEOGPS_PACKED;

// TinyGPSPlus gps;
// char timeStr[16];
// char dateStr[16];

// extern WiFiUDP udp;

// #define NAV5_LEN 44
// #define NAV5X_LEN 48

// const byte NMEA_LEN = 16;
// const byte FREQ_LEN = 14;
// const byte BAUD_LEN = 28;

// const byte GPGGA = 0;
// const byte GPGLL = 1;
// const byte GPGLV = 2;
// const byte GPGSA = 3;
// const byte GPRMC = 4;
// const byte GPVTG = 5;

// const byte NMEA_ID_POS  = 7;
// const byte DDC_POS      = 8;
// const byte SERIAL_1_POS = 9;
// const byte SERIAL_2_POS = 10;
// const byte USB_POS      = 11;
// const byte SPI_POS      = 12;

// const byte BAUD_0 = 14;
// const byte BAUD_1 = 15;
// const byte BAUD_2 = 16;
// const byte BAUD_3 = 17;

// const byte MEAS_RATE_1 = 6;
// const byte MEAS_RATE_2 = 7;
// const byte NAV_RATE_1  = 8;
// const byte NAV_RATE_2  = 9;

// const char CFG_RATE[FREQ_LEN] = {
// 	0xB5, // sync char 1
// 	0x62, // sync char 2
// 	0x06, // class
// 	0x08, // id
// 	0x06, // length LSB
// 	0x00, // length MSB
// 	0x64, // payload measRate (ms) 1
// 	0x00, // payload measRate (ms) 2
// 	0x00, // payload navRate (cycles) 1
// 	0x00, // payload navRate (cycles) 2
// 	0x01, // payload timeRef 1
// 	0x00, // payload timeRef 2
// 	0x00, // CK_A
// 	0x00  // CK_B
// };

// char CFG_NAV5[NAV5_LEN] = {
// 	0xB5, // sync char 1
// 	0x62, // sync char 2
// 	0x06, // class
// 	0x24, // id
// 	0x24, // length LSB
// 	0x00, // length MSB
// 	0b000001, // Bitmask
//     0x00, // Bitmask
//     2, // Dynamic platform model : Stationary
//     3, // Fixmode: Auto
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Fixed Altitude
//     0x00, // Min elev
//     0x00, // DR Limit
//     0x00, // pDop
//     0x00, // pDop
//     0x00, // tDop
//     0x00, // tDop
//     0x00, // pAcc
//     0x00, // pAcc
//     0x00, // tAcc
//     0x00, // tAcc
//     0x00, // Static Hold
//     0x00, // dpgs timeout
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
// 	0x00, // CK_A
// 	0x00  // CK_B
// };

// char CFG_NAV5X[NAV5X_LEN] = {
// 	0xB5, // sync char 1
// 	0x62, // sync char 2
// 	0x06, // class
// 	0x23, // id
// 	0x24, // length LSB
// 	0x00, // length MSB
// 	0b000000, // Bitmask
//     0b011000, // Bitmask ppp aop
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // MinSV
//     0x00, // MaxSV
//     0x00, // minCNO
//     0x00, // Reserved
//     0x00, // iniFix3D
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // WknRollover
//     0x00, // WknRollover
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x01, // Use PPP YES
//     0x01, // Use AOP YES
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // AOP Max Error
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
//     0x00, // Reserved
// 	0x00, // CK_A
// 	0x00  // CK_B
// };

// // https://wiki.openstreetmap.org/wiki/U-blox_raw_format#U-BLOX6
// char RAMPATCH1[24] = {
//     0xb5, 0x62, 0x09, 0x01, 0x10, 0x00, 0xdc, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0xcc, 0x21, 0x00, 0x00, 0x00, 0x02, 0x10, 0x27, 0x0e
// };
// char RAMPATCH2[24] = {
//     0xb5, 0x62, 0x09, 0x01, 0x10, 0x00, 0x08, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xcc, 0x21, 0x00, 0x00, 0x00, 0x02, 0x11, 0x42, 0x4d
// };
// char RAMPATCH7[24] = {
//     0xb5, 0x62, 0x09, 0x01, 0x10, 0x00, 0xc8, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x69, 0x21, 0x00, 0x00, 0x00, 0x02, 0x10, 0x2b, 0x22
// };
// char RAMPATCH8[24] = {
//     0xb5, 0x62, 0x09, 0x01, 0x10, 0x00, 0x0c, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x69, 0x21, 0x00, 0x00, 0x00, 0x02, 0x11, 0x5f, 0xf0
// };
// char POLLRMXRAW[11] = {
//     0xb5, 0x62, 0x06, 0x01, 0x03, 0x00, 0x02, 0x10, 0x01, 0x1d, 0x66
// };

// void insertChecksum(char packet[], const byte len)
// {
// 	uint8_t ck_a = 0;
// 	uint8_t ck_b = 0;

// 	// exclude the first and last two bytes in packet
// 	for (byte i = 2; i < (len - 2); i++)
// 	{
// 		ck_a += packet[i];
// 		ck_b += ck_a;
// 	}

// 	packet[len - 2] = ck_a;
// 	packet[len - 1] = ck_b;
// }

// void changeGPSFreq(uint16_t hertz)
// {
// 	uint16_t normHerz = hertz / (1000 / ((CFG_RATE[MEAS_RATE_2] << 8) | CFG_RATE[MEAS_RATE_1]));
// 	char configPacket[FREQ_LEN];
// 	memcpy(configPacket, CFG_RATE, FREQ_LEN);

// 	configPacket[NAV_RATE_1] = (char)(normHerz & 0xFF);
// 	configPacket[NAV_RATE_2] = (char)((normHerz >> 8) & 0xFF);

// 	insertChecksum(configPacket, FREQ_LEN);
// 	Serial1.write(configPacket, FREQ_LEN);
// }


// void setConfigGPS()
// {
//     Serial1.begin(9600, SERIAL_8N1, 25, 33);
// 	insertChecksum(CFG_NAV5, NAV5_LEN);
// 	Serial1.write(CFG_NAV5, NAV5_LEN);
// 	insertChecksum(CFG_NAV5X, NAV5X_LEN);
// 	Serial1.write(CFG_NAV5X, NAV5X_LEN);
//     changeGPSFreq(2);
//     // insertChecksum(RAMPATCH7, 24);
//     Serial1.write(RAMPATCH7, 24);
//     Serial1.write(POLLRMXRAW, 11);
// }

// bool printData = false;
// String lines = "";

// void updateGPS() {
// 	while (Serial1.available() > 0) {
//         char r = Serial1.read();
//         gps.encode(r);
//         // Serial.print(r);
//         if (printData) lines += r;
//         // if (r == 0x05 && Serial1.available() > 0) {
//         //     char r1 = Serial1.read();
//         //     gps.encode(r1);
//         //     if (r1 == 0x01) Serial.println("ACK GOOD");
//         //     else if (r1 == 0x00) Serial.println("ACK BAD");
//         //     else Serial.println("ACK WTF???");
//         // }
//     }
//     if (gps.location.isUpdated()){
//         static double latitudeAverage;
//         static double longitudeAverage;
//         static double altitudeAverage;
//         static long numUpdates = 0;
//         double newLatitude = gps.location.lat();
//         double newLongitude = gps.location.lng();
//         double newAltitude = gps.altitude.meters();
//         if (true || numUpdates == 0) {
//             latitudeAverage = newLatitude;
//             longitudeAverage = newLongitude;
//             altitudeAverage = newAltitude;
//         }
//         else {
//             latitudeAverage = (latitudeAverage*numUpdates + newLatitude) / (numUpdates+1);
//             longitudeAverage = (longitudeAverage*numUpdates + newLongitude) / (numUpdates+1);
//             altitudeAverage = (altitudeAverage*numUpdates + newAltitude) / (numUpdates+1);
//         }
//         numUpdates++;

//         // latitude = latitudeAverage;
//         // longitude = longitudeAverage;
//         // altitude = altitudeAverage;
//         setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
//         // computeSolarPosition();
//         sprintf(timeStr, "%i:%i:%i", hour(), minute(), second());
//         sprintf(timeStr, "%i:%i:%i", hour(), minute(), second());
//         sprintf(dateStr, "%i/%i/%i", day(), month(), year());
//         // Serial.println(timeStr);
//         // sprintf(dateStr, "%s %s %i %i", dayStr(day()), monthStr(month()), day(), year());
//         // Serial.print(">Latitude:"); 
//         // Serial.println(latitudeAverage*1000., 6);
//         // Serial.print(">Longitude:"); 
//         // Serial.println(longitudeAverage*1000., 6);
//         // Serial.print(">Altitude:"); 
//         // Serial.println(altitudeAverage);
//         // Serial.print(">Satellites:"); 
//         // Serial.println(gps.satellites.value());
//         //   Serial.print(">Date:"); 
//         //   Serial.println(gps.date.value());
//         //   Serial.print(">Time:"); 
//         //   Serial.println(gps.time.value());
//         //   Serial.print(">Age:"); 
//         //   Serial.println(gps.location.age());
//         // Serial.print(">Position:"); 
//         // Serial.print(latitudeAverage*1000., 6);
//         // Serial.print(":"); 
//         // Serial.print(longitudeAverage*1000., 6);
//         // Serial.println("|xy"); 
//         if (false && WiFi.status() == WL_CONNECTED) {
//             udp.beginPacket("teleplot.fr", 60524);
//             udp.printf("Latitude:"); 
//             udp.println(latitudeAverage*1000., 6);
//             udp.printf("Longitude:"); 
//             udp.println(longitudeAverage*1000., 6);
//             udp.printf("Altitude:"); 
//             udp.println(altitudeAverage);
//             udp.printf("Satellites:"); 
//             udp.println(gps.satellites.value());
//             udp.print("Position:"); 
//             udp.print(latitudeAverage*1000., 6);
//             udp.print(":"); 
//             udp.print(longitudeAverage*1000., 6);
//             udp.println("|xy"); 
//             udp.endPacket();
//         }
//         Serial1.write(POLLRMXRAW, 11);
//     }
// }
#endif