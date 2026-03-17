#include <unity.h>
#include <ArduinoJson.h>

#include "orientation_contract.h"

void test_patch_updates_only_provided_key()
{
    double tiltDeg = 5.0;
    double tiltAzimuthDeg = 20.0;

    JsonDocument doc;
    JsonObject patch = doc.to<JsonObject>();
    patch["tiltDeg"] = 30.0;

    bool ok = OrientationContract::applyMountOrientationPatch(patch, tiltDeg, tiltAzimuthDeg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 30.0, tiltDeg);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 20.0, tiltAzimuthDeg);
}

void test_values_are_clamped_to_locked_bounds()
{
    double tiltDeg = 0.0;
    double tiltAzimuthDeg = 0.0;

    JsonDocument doc;
    JsonObject patch = doc.to<JsonObject>();
    patch["tiltDeg"] = 120.0;
    patch["tiltAzimuthDeg"] = -15.0;

    bool ok = OrientationContract::applyMountOrientationPatch(patch, tiltDeg, tiltAzimuthDeg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 90.0, tiltDeg);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.0, tiltAzimuthDeg);
}

void test_invalid_combined_payload_is_rejected_atomically()
{
    double tiltDeg = 12.0;
    double tiltAzimuthDeg = 210.0;

    JsonDocument doc;
    JsonObject patch = doc.to<JsonObject>();
    patch["tiltDeg"] = 45.0;
    patch["tiltAzimuthDeg"] = "bad-type";

    bool ok = OrientationContract::applyMountOrientationPatch(patch, tiltDeg, tiltAzimuthDeg);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 12.0, tiltDeg);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 210.0, tiltAzimuthDeg);
}
