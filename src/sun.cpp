#include "sun.h"
#include <time.h>

// Custom time provider that uses system time (updated by NTP) instead of TimeLib time
// This ensures that solar position calculations work correctly when time is set via:
// 1. NTP synchronization (which updates system time via configTzTime)
// 2. GPS (which should update both TimeLib and system time)  
// 3. Manual time setting (which should update both TimeLib and system time)
time_t systemTimeProvider() {
    return time(nullptr);
}

void setupSolarTracker() {
    // Use system time provider instead of TimeLib's now() function
    // This ensures NTP-synchronized time is used for solar calculations
    SolarPosition::setTimeProvider(systemTimeProvider);
}

SphericalCoordinate computeSolarPosition(double latitude, double longitude) {
    SolarPosition currentPosition(latitude, longitude);
    SolarPosition_t currentSolarPosition = currentPosition.getSolarPosition();
    // ESP_LOGI("Sun", "%f %f", latitude, longitude);
    // ESP_LOGI("Sun", "%f %f", currentSolarPosition.azimuth, currentSolarPosition.elevation);
    return {currentSolarPosition.azimuth, currentSolarPosition.elevation};
}