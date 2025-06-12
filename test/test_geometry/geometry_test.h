#ifndef GEOMHELPERS_TEST
#define GEOMHELPERS_TEST
#include <math.h>

const double pi = 3.14159265359;

struct vec2 {
    double x;
    double y;
    vec2() : x(0), y(0) {}
    vec2(double _x, double _y) : x(_x), y(_y) {}
};

struct vec3 {
    double x;
    double y;
    double z;
    vec3() : x(0), y(0), z(0) {}
    vec3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

inline vec2 degToRad(vec2 deg) {
    return vec2(deg.x * pi / 180.0, deg.y * pi / 180.0);
}

inline vec2 radToDeg(vec2 rad) {
    return vec2(rad.x * 180.0 / pi, rad.y * 180.0 / pi);
}

vec3 toCartesian(vec2 spherical);
vec2 toSpherical(vec3 cartesian);

#endif
