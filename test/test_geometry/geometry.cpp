#include "geometry_test.h"

vec3 toCartesian(vec2 spherical) {
    spherical = degToRad(spherical);
    double theta = spherical.x;
    double phi = spherical.y;
    return vec3(
        sin(theta) * cos(phi),
        sin(theta) * sin(phi),
        cos(theta)
    );
}

vec2 toSpherical(vec3 cartesian) {
    double r = sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);       // Radius
    double phi = atan2(cartesian.y, cartesian.x);
    double theta = acos(cartesian.z / r);
    // double phi = atan2(sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y), cartesian.z); // Elevation angle
    return radToDeg(vec2(theta, phi));
}