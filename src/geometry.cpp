#include <geometry.h>
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