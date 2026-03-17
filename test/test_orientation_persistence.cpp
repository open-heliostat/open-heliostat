#include <unity.h>
#include <ArduinoJson.h>

#include "orientation_contract.h"

void test_save_map_includes_mount_orientation_fields()
{
    JsonDocument doc;
    JsonObject saveMap = doc.to<JsonObject>();

    OrientationContract::addMountOrientationSaveMap(saveMap);

    TEST_ASSERT_TRUE(saveMap["mountOrientation"]["tiltDeg"].as<bool>());
    TEST_ASSERT_TRUE(saveMap["mountOrientation"]["tiltAzimuthDeg"].as<bool>());
}

void test_legacy_defaults_remain_level_when_payload_is_empty()
{
    double tiltDeg = 0.0;
    double tiltAzimuthDeg = 0.0;

    JsonDocument doc;
    JsonObject patch = doc.to<JsonObject>();

    bool ok = OrientationContract::applyMountOrientationPatch(patch, tiltDeg, tiltAzimuthDeg);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.0, tiltDeg);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.0, tiltAzimuthDeg);
}
