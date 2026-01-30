#include "WiFiScanGuard.h"

static SemaphoreHandle_t wifiScanMutex()
{
    static StaticSemaphore_t buffer;
    static SemaphoreHandle_t mutex = xSemaphoreCreateMutexStatic(&buffer);
    return mutex;
}

bool wifiScanLock(TickType_t timeoutTicks)
{
    return xSemaphoreTake(wifiScanMutex(), timeoutTicks) == pdTRUE;
}

void wifiScanUnlock()
{
    xSemaphoreGive(wifiScanMutex());
}
