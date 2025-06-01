#include "sun.h"

void setupSolarTracker() {
    SolarPosition::setTimeProvider(now);
}

SphericalCoordinate computeSolarPosition(double latitude, double longitude) {
    SolarPosition currentPosition(latitude, longitude);
    SolarPosition_t currentSolarPosition = currentPosition.getSolarPosition();
    // ESP_LOGI("Sun", "%f %f", latitude, longitude);
    // ESP_LOGI("Sun", "%f %f", currentSolarPosition.azimuth, currentSolarPosition.elevation);
    return {currentSolarPosition.azimuth, currentSolarPosition.elevation};
}