#include <unity.h>

#include "MountOrientationResolve.h"
#include "geometry.h"

namespace {
vec3 sensorMountRotate(const vec3 &input, double rollRad, double pitchRad)
{
    double cr = cos(rollRad);
    double sr = sin(rollRad);
    double cp = cos(pitchRad);
    double sp = sin(pitchRad);

    return vec3{
        cp * input.x + sp * sr * input.y + sp * cr * input.z,
        cr * input.y - sr * input.z,
        -sp * input.x + cp * sr * input.y + cp * cr * input.z,
    };
}

MountOrientationObservation makeObservation(double azDeg,
                                            double elDeg,
                                            double tiltDeg,
                                            double tiltAzimuthDeg,
                                            double sensorRollDeg = 0.0,
                                            double sensorPitchDeg = 0.0)
{
    vec3 gravityMount = applyMountOrientationTransform({0.0, 0.0, 1.0}, tiltDeg, tiltAzimuthDeg);
    gravityMount.rotY(degToRad(elDeg));
    gravityMount.rotZ(degToRad(azDeg));
    vec3 measured = sensorMountRotate(gravityMount, degToRad(sensorRollDeg), degToRad(sensorPitchDeg));
    return {
        static_cast<float>(azDeg),
        static_cast<float>(elDeg),
        static_cast<float>(measured.x),
        static_cast<float>(measured.y),
        static_cast<float>(measured.z),
    };
}
}

void test_mount_orientation_estimate_recovers_level_mount()
{
    MountOrientationObservation observations[8] = {
        makeObservation(0.0, 20.0, 0.0, 0.0),
        makeObservation(60.0, 20.0, 0.0, 0.0),
        makeObservation(120.0, 20.0, 0.0, 0.0),
        makeObservation(180.0, 20.0, 0.0, 0.0),
        makeObservation(240.0, 60.0, 0.0, 0.0),
        makeObservation(300.0, 60.0, 0.0, 0.0),
        makeObservation(30.0, 60.0, 0.0, 0.0),
        makeObservation(150.0, 60.0, 0.0, 0.0),
    };
    MountOrientationEstimate estimate;

    bool ok = estimateMountOrientation(observations, 8, &estimate);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 0.0f, estimate.tiltDeg);
}

void test_mount_orientation_estimate_recovers_known_tilt()
{
    MountOrientationObservation observations[8] = {
        makeObservation(0.0, 15.0, 15.0, 45.0),
        makeObservation(45.0, 15.0, 15.0, 45.0),
        makeObservation(90.0, 15.0, 15.0, 45.0),
        makeObservation(135.0, 15.0, 15.0, 45.0),
        makeObservation(180.0, 65.0, 15.0, 45.0),
        makeObservation(225.0, 65.0, 15.0, 45.0),
        makeObservation(270.0, 65.0, 15.0, 45.0),
        makeObservation(315.0, 65.0, 15.0, 45.0),
    };
    MountOrientationEstimate estimate;

    bool ok = estimateMountOrientation(observations, 8, &estimate);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.4f, 15.0f, estimate.tiltDeg);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 45.0f, estimate.tiltAzimuthDeg);
}

void test_mount_orientation_estimate_recovers_level_mount_with_sensor_mount_offset()
{
    MountOrientationObservation observations[8] = {
        makeObservation(0.0, 20.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(60.0, 20.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(120.0, 20.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(180.0, 20.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(240.0, 60.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(300.0, 60.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(30.0, 60.0, 0.0, 0.0, 0.0, 90.0),
        makeObservation(150.0, 60.0, 0.0, 0.0, 0.0, 90.0),
    };
    MountOrientationEstimate estimate;

    bool ok = estimateMountOrientation(observations, 8, &estimate);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.4f, 0.0f, estimate.tiltDeg);
}

void test_mount_orientation_observability_rejects_narrow_motion()
{
    MountOrientationObservation observations[8] = {
        makeObservation(10.0, 20.0, 5.0, 10.0),
        makeObservation(20.0, 20.0, 5.0, 10.0),
        makeObservation(30.0, 22.0, 5.0, 10.0),
        makeObservation(35.0, 22.0, 5.0, 10.0),
        makeObservation(40.0, 24.0, 5.0, 10.0),
        makeObservation(42.0, 24.0, 5.0, 10.0),
        makeObservation(45.0, 26.0, 5.0, 10.0),
        makeObservation(48.0, 26.0, 5.0, 10.0),
    };

    TEST_ASSERT_FALSE(checkMountOrientationObservability(observations, 8));
}