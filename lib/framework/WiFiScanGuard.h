#ifndef WIFI_SCAN_GUARD_H
#define WIFI_SCAN_GUARD_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Acquire a global mutex protecting WiFi scan/scanDelete lifecycles.
// Returns true when the lock is taken, false otherwise.
bool wifiScanLock(TickType_t timeoutTicks = 0);

// Release the global WiFi scan mutex.
void wifiScanUnlock();

#endif // WIFI_SCAN_GUARD_H
