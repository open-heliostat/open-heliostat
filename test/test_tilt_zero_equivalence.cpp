#include <unity.h>

#include "geometry.h"

void test_zero_tilt_returns_original_vector()
{
    vec3 input{0.25, -0.4, 0.88};
    vec3 transformed = applyMountOrientationTransform(input, 0.0, 45.0);

    TEST_ASSERT_DOUBLE_WITHIN(1e-12, input.x, transformed.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, input.y, transformed.y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, input.z, transformed.z);
}

void test_tilt_azimuth_wrap_360_matches_zero()
{
    vec3 input{0.3, 0.5, 0.2};

    vec3 atZero = applyMountOrientationTransform(input, 20.0, 0.0);
    vec3 atWrap = applyMountOrientationTransform(input, 20.0, 360.0);

    TEST_ASSERT_DOUBLE_WITHIN(1e-12, atZero.x, atWrap.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, atZero.y, atWrap.y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, atZero.z, atWrap.z);
}
