#include <unity.h>
#include <ArduinoJson.h>

#include "orientation_contract.h"

void test_mount_orientation_is_serialized_with_locked_field_names()
{
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    JsonObject mountOrientation = root["mountOrientation"].to<JsonObject>();

    OrientationContract::writeMountOrientation(mountOrientation, 18.5, 133.25);

    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 18.5, root["mountOrientation"]["tiltDeg"].as<double>());
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 133.25, root["mountOrientation"]["tiltAzimuthDeg"].as<double>());
}

void test_non_object_mount_orientation_payload_is_rejected()
{
    double tiltDeg = 1.0;
    double tiltAzimuthDeg = 2.0;

    JsonDocument doc;
    doc.set("not-an-object");

    bool ok = OrientationContract::applyMountOrientationPatch(doc.as<JsonVariant>(), tiltDeg, tiltAzimuthDeg);

    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.0, tiltDeg);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 2.0, tiltAzimuthDeg);
}
