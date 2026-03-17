#include <unity.h>
#include <cmath>

#include "geometry.h"

void test_cardinal_downslope_azimuth_produces_directional_shift()
{
    vec3 input{0.0, 0.0, 1.0};

    vec3 north = applyMountOrientationTransform(input, 30.0, 0.0);
    vec3 east = applyMountOrientationTransform(input, 30.0, 90.0);
    vec3 south = applyMountOrientationTransform(input, 30.0, 180.0);
    vec3 west = applyMountOrientationTransform(input, 30.0, 270.0);

    TEST_ASSERT_GREATER_THAN(0.0, north.x);
    TEST_ASSERT_GREATER_THAN(0.0, east.y);
    TEST_ASSERT_LESS_THAN(0.0, south.x);
    TEST_ASSERT_LESS_THAN(0.0, west.y);

    TEST_ASSERT_TRUE(std::fabs(north.y) < 1e-12);
    TEST_ASSERT_TRUE(std::fabs(east.x) < 1e-12);
    TEST_ASSERT_TRUE(std::fabs(south.y) < 1e-12);
    TEST_ASSERT_TRUE(std::fabs(west.x) < 1e-12);
}
