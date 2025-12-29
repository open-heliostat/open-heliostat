#ifndef GEOMHELPERS
#define GEOMHELPERS
#include <math.h>
const double pi = 3.14159265359;
template<typename T>
T degToRad(T deg){
    return deg*pi/180.;
}
template<typename T>
T radToDeg(T rad) {
    return rad*180./pi;
}

struct vec2;

struct vec3 {
    double x;
    double y;
    double z;
    template<typename T>
    void operator=(T const& obj) {
        x = obj;
        y = obj;
        z = obj;
    }
    template<typename T>
    vec3 operator*(T const& obj) {
        vec3 res;
        res.x = x * obj;
        res.y = y * obj;
        res.z = z * obj;
        return res;
    }
    template<typename T>
    vec3 operator/(T const& obj) {
        vec3 res;
        res.x = x / obj;
        res.y = y / obj;
        res.z = z / obj;
        return res;
    }
    vec3 operator+(vec3 const& obj) {
        vec3 res;
        res.x = x + obj.x;
        res.y = y + obj.y;
        res.z = z + obj.z;
        return res;
    }
    vec3 operator-(vec3 const& obj) {
        vec3 res;
        res.x = x - obj.x;
        res.y = y - obj.y;
        res.z = z - obj.z;
        return res;
    }
    vec3 operator-() {
        return vec3{-x, -y, -z};
    }
    double sqLength() {
        return x*x+y*y+z*z;
    }
    double length() {
        return sqrt(sqLength());
    }
    double dot(vec3 v) {
        // ||a|| ||b|| cos(θ)
        return x * v.x + y * v.y + z * v.z;
    }
    double getAngle(double x, double y) {
        return atan2(y, x);
    }
    void rotZ(double angle) {
        double _x = x * cos(angle) - y * sin(angle);
        y = x * sin(angle) + y * cos(angle);
        x = _x;
    }
    void rotX(double angle) {
        double _z = z * cos(angle) - y * sin(angle);
        y = z * sin(angle) + y * cos(angle);
        z = _z;
    }
    void rotY(double angle) {
        double _x = x * cos(angle) - z * sin(angle);
        z = x * sin(angle) + z * cos(angle);
        x = _x;
    }
    double dotDist(vec3 v) {
        return acos(v.normalize().dot(this->normalize()));
    }
    vec3 cross(vec3 v) {
        // ||a|| ||b|| sin(θ) V
        return vec3 {y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x};
    }
    vec3 normalize() {
        double len = length();
        if (len == 0.0) return vec3{0.0, 0.0, 0.0};
        return *this / len;
    }
    vec3 setUpDirection(vec3 up) {
        double az = getAngle(up.x, up.y);
        up.rotZ(-az);
        double el = getAngle(up.z, up.x);
        // up.rotY(el);
        vec3 result = *this;
        result.rotZ(-az);
        result.rotY(el);
        return result;
    }
    vec2 toSpherical();
};

struct vec2 {
    double x;
    double y;
    template<typename T>
    vec2 operator*(T const& obj) {
        vec2 res;
        res.x = x * obj;
        res.y = y * obj;
        return res;
    }
    template<typename T>
    vec2 operator/(T const& obj) {
        vec2 res;
        res.x = x / obj;
        res.y = y / obj;
        return res;
    }
    vec2 operator+(vec2 const& obj) {
        vec2 res;
        res.x = x + obj.x;
        res.y = y + obj.y;
        return res;
    }
    vec2 operator-(vec2 const& obj) {
        vec2 res;
        res.x = x - obj.x;
        res.y = y - obj.y;
        return res;
    }
    vec2 toDeg() {
        return radToDeg(*this);
    }
    vec2 toRad() {
        return degToRad(*this);
    }
    vec3 toCartesian() {
        return vec3{
            sin(x) * cos(y),
            sin(x) * sin(y),
            cos(x)
        };
    }
    double angularDistance(vec2 v) {
        double deltaElevation = v.x - x;
        double deltaAzimuth = v.y - y;
        // Haversine formula
        double a =  sin(deltaElevation / 2.0) * sin(deltaElevation / 2.0) +
                    cos(x) * cos(v.x) *
                    sin(deltaAzimuth / 2.0) * sin(deltaAzimuth / 2.0);

        double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
        return c;
        // return acos(sin(x)*sin(v.x) + cos(y)*cos(v.y)*cos(x-v.x));
    }
};

struct ObjectDirection : vec2 {
    bool isCycling = false;
    double cycleFreq = 0.2;
    double cycleAmp = 0.1;
    ObjectDirection(double azimuth, double elevation) {
        setElevation(elevation);
        setAzimuth(azimuth);
    }
    ObjectDirection(vec2 vec) {
        x = vec.x;
        y = vec.y;
    }
    ObjectDirection() {}
    ObjectDirection cycle(uint32_t now) {
        double phase = now * cycleFreq / 1000.;
        vec2 offset = {sin(phase) * cycleAmp, cos(phase) * cycleAmp};
        return ObjectDirection(*this + offset);
    }
    ObjectDirection bisect(ObjectDirection &obj) {
        vec3 res = this->toCartesian() + obj.toCartesian();
        // ESP_LOGI("Geometry", "%f %f %f", this->toCartesian().x, this->toCartesian().y, this->toCartesian().z);
        // ESP_LOGI("Geometry", "%f %f %f", obj.toCartesian().x, obj.toCartesian().y, obj.toCartesian().z);
        // ESP_LOGI("Geometry", "%f %f %f", res.x, res.y, res.z);
        return ObjectDirection(res.toSpherical());
    }
    void setElevation(double elevation) {
        // x = degToRad(180.-elevation);
        x = degToRad(elevation);
    }
    double getElevation() {
        // return 180.-radToDeg(x);
        return radToDeg(x);
    }
    void setAzimuth(double azimuth) {
        y = degToRad(azimuth);
    }
    double getAzimuth() {
        return radToDeg(y);
    }
};

vec3 toCartesian(vec2 spherical);
vec2 toSpherical(vec3 cartesian);

struct SphericalCoordinate
{
    double azimuth;
    double elevation;

};
#endif