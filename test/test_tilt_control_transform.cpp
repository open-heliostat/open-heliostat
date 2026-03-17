#include <unity.h>
#include <cmath>

#include "geometry.h"
#include "heliostat.h"

namespace {
constexpr double kAxisTolerance = 1e-12;
constexpr double kSphericalTolerance = 1e-6;

class StubController : public AbstractController {
public:
    explicit StubController(Encoder &encoder) : AbstractController(encoder) {
        enabled = true;
        targetAngle = 0.0;
    }

    void run() override {}
    void init() override {}

    void setAngle(double angle) override {
        currentAngle = angle;
        setTarget(angle);
    }

    double getAngle() override {
        return currentAngle;
    }

private:
    double currentAngle = 0.0;
};

void assertVectorComponentSigns(const vec3 &north, const vec3 &east, const vec3 &south, const vec3 &west)
{
    TEST_ASSERT_GREATER_THAN(0.0, north.x);
    TEST_ASSERT_GREATER_THAN(0.0, east.y);
    TEST_ASSERT_LESS_THAN(0.0, south.x);
    TEST_ASSERT_LESS_THAN(0.0, west.y);
}

void assertCardinalCrossAxisNearZero(const vec3 &north, const vec3 &east, const vec3 &south, const vec3 &west)
{
    TEST_ASSERT_TRUE(std::fabs(north.y) < kAxisTolerance);
    TEST_ASSERT_TRUE(std::fabs(east.x) < kAxisTolerance);
    TEST_ASSERT_TRUE(std::fabs(south.y) < kAxisTolerance);
    TEST_ASSERT_TRUE(std::fabs(west.x) < kAxisTolerance);
}

double angularDistanceDeg(double a, double b)
{
    double d = fmod(a - b + 540.0, 360.0) - 180.0;
    return fabs(d);
}

SphericalCoordinate legacyReflect(SphericalCoordinate source, SphericalCoordinate target)
{
    vec3 bisector = (toCartesian({source.elevation, source.azimuth}) + toCartesian({target.elevation, target.azimuth})) / 2.0;
    vec2 result = toSpherical(bisector);
    return {result.y, result.x};
}

SphericalCoordinate transformedReflect(SphericalCoordinate source, SphericalCoordinate target, double tiltDeg, double tiltAzimuthDeg)
{
    vec3 sourceVec = applyMountOrientationTransform(toCartesian({source.elevation, source.azimuth}), tiltDeg, tiltAzimuthDeg);
    vec3 targetVec = applyMountOrientationTransform(toCartesian({target.elevation, target.azimuth}), tiltDeg, tiltAzimuthDeg);
    vec3 bisector = (sourceVec + targetVec) / 2.0;
    vec2 result = toSpherical(bisector);
    return {result.y, result.x};
}
}

void test_cardinal_downslope_azimuth_produces_directional_shift()
{
    vec3 input{0.0, 0.0, 1.0};

    vec3 north = applyMountOrientationTransform(input, 30.0, 0.0);
    vec3 east = applyMountOrientationTransform(input, 30.0, 90.0);
    vec3 south = applyMountOrientationTransform(input, 30.0, 180.0);
    vec3 west = applyMountOrientationTransform(input, 30.0, 270.0);

    assertVectorComponentSigns(north, east, south, west);
    assertCardinalCrossAxisNearZero(north, east, south, west);
}

void test_reflect_changes_when_non_zero_orientation_is_applied()
{
    Encoder azEncoder;
    Encoder elEncoder;
    StubController azimuthController(azEncoder);
    StubController elevationController(elEncoder);
    SerialGPS gps;
    HeliostatController heliostat(azimuthController, elevationController, gps);

    SphericalCoordinate source{130.0, 42.0};
    SphericalCoordinate target{215.0, 17.0};

    heliostat.tiltDeg = 30.0;
    heliostat.tiltAzimuthDeg = 90.0;

    SphericalCoordinate reflected = heliostat.reflect(source, target);
    SphericalCoordinate baseline = legacyReflect(source, target);

    bool changed = angularDistanceDeg(reflected.azimuth, baseline.azimuth) > kSphericalTolerance ||
                   fabs(reflected.elevation - baseline.elevation) > kSphericalTolerance;
    TEST_ASSERT_TRUE(changed);
}

void test_reflect_zero_tilt_matches_legacy_math()
{
    Encoder azEncoder;
    Encoder elEncoder;
    StubController azimuthController(azEncoder);
    StubController elevationController(elEncoder);
    SerialGPS gps;
    HeliostatController heliostat(azimuthController, elevationController, gps);

    SphericalCoordinate source{130.0, 42.0};
    SphericalCoordinate target{215.0, 17.0};

    heliostat.tiltDeg = 0.0;
    heliostat.tiltAzimuthDeg = 270.0;

    SphericalCoordinate reflected = heliostat.reflect(source, target);
    SphericalCoordinate baseline = legacyReflect(source, target);

    TEST_ASSERT_TRUE(angularDistanceDeg(reflected.azimuth, baseline.azimuth) < kSphericalTolerance);
    TEST_ASSERT_DOUBLE_WITHIN(kSphericalTolerance, baseline.elevation, reflected.elevation);
}

void test_reflect_consumes_live_tilt_state_between_calls()
{
    Encoder azEncoder;
    Encoder elEncoder;
    StubController azimuthController(azEncoder);
    StubController elevationController(elEncoder);
    SerialGPS gps;
    HeliostatController heliostat(azimuthController, elevationController, gps);

    SphericalCoordinate source{130.0, 42.0};
    SphericalCoordinate target{215.0, 17.0};

    heliostat.tiltDeg = 0.0;
    heliostat.tiltAzimuthDeg = 0.0;
    SphericalCoordinate first = heliostat.reflect(source, target);

    heliostat.tiltDeg = 25.0;
    heliostat.tiltAzimuthDeg = 180.0;
    SphericalCoordinate second = heliostat.reflect(source, target);

    SphericalCoordinate expected = transformedReflect(source, target, heliostat.tiltDeg, heliostat.tiltAzimuthDeg);

    bool changed = angularDistanceDeg(first.azimuth, second.azimuth) > kSphericalTolerance ||
                   fabs(first.elevation - second.elevation) > kSphericalTolerance;
    TEST_ASSERT_TRUE(changed);
    TEST_ASSERT_TRUE(angularDistanceDeg(second.azimuth, expected.azimuth) < kSphericalTolerance);
    TEST_ASSERT_DOUBLE_WITHIN(kSphericalTolerance, expected.elevation, second.elevation);
}
