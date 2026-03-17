#include <unity.h>
#include <cmath>

#include "geometry.h"

namespace {
constexpr double kAxisTolerance = 1e-12;

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
