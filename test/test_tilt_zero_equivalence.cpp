#include <unity.h>

#include "geometry.h"

namespace {
constexpr double kVectorTolerance = 1e-12;

void assertVectorNear(const vec3 &actual, const vec3 &expected)
{
    TEST_ASSERT_DOUBLE_WITHIN(kVectorTolerance, expected.x, actual.x);
    TEST_ASSERT_DOUBLE_WITHIN(kVectorTolerance, expected.y, actual.y);
    TEST_ASSERT_DOUBLE_WITHIN(kVectorTolerance, expected.z, actual.z);
}
}

void test_zero_tilt_returns_original_vector()
{
    vec3 input{0.25, -0.4, 0.88};
    vec3 transformed = applyMountOrientationTransform(input, 0.0, 45.0);

    assertVectorNear(transformed, input);
}

void test_tilt_azimuth_wrap_360_matches_zero()
{
    vec3 input{0.3, 0.5, 0.2};

    vec3 atZero = applyMountOrientationTransform(input, 20.0, 0.0);
    vec3 atWrap = applyMountOrientationTransform(input, 20.0, 360.0);

    assertVectorNear(atWrap, atZero);
}
