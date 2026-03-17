#include <geometry.h>

namespace {
double normalizeAzimuthDeg(double azimuthDeg)
{
    double normalized = fmod(azimuthDeg, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized;
}
}

vec2 vec3::toSpherical() {
    double theta = atan2(sqrt(x*x + y*y), z);
    double phi = atan2(y, x);
    return vec2{theta, phi};
}

vec3 toCartesian(vec2 spherical) {
    spherical = {90. - spherical.x, spherical.y}; // Convert from elevation/azimuth to theta/phi
    spherical = degToRad(spherical);
    double theta = spherical.x;
    double phi = spherical.y;
    return vec3{
        sin(theta) * cos(phi),
        sin(theta) * sin(phi),
        cos(theta)
    };
}

vec2 toSpherical(vec3 cartesian) {
    double r = sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);       // Radius
    double phi = atan2(cartesian.y, cartesian.x);
    double theta = acos(cartesian.z / r);
    vec2 spherical = radToDeg(vec2{theta, phi});
    spherical.x = 90. - spherical.x; // Convert from theta/phi to elevation/azimuth
    // double phi = atan2(sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y), cartesian.z); // Elevation angle
    return spherical;
}

vec3 applyMountOrientationTransform(vec3 input, double tiltDeg, double tiltAzimuthDeg)
{
    if (fabs(tiltDeg) < 1e-12) {
        return input;
    }

    double azimuthRad = degToRad(normalizeAzimuthDeg(tiltAzimuthDeg));
    double tiltRad = degToRad(tiltDeg);

    vec3 transformed = input;
    transformed.rotZ(-azimuthRad);
    transformed.rotY(-tiltRad);
    transformed.rotZ(azimuthRad);
    return transformed;
}