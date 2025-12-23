#ifndef SUNTRACKER
#define SUNTRACKER
#include <SolarPosition.h>
#include "TimeLib.h"
#include "geometry.h"
SphericalCoordinate computeSolarPosition(double latitude, double longitude);
void setupSolarTracker();
#endif