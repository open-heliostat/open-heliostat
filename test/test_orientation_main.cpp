#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_patch_updates_only_provided_key();
void test_values_are_clamped_to_locked_bounds();
void test_invalid_combined_payload_is_rejected_atomically();
void test_save_map_includes_mount_orientation_fields();
void test_legacy_defaults_remain_level_when_payload_is_empty();
void test_mount_orientation_is_serialized_with_locked_field_names();
void test_non_object_mount_orientation_payload_is_rejected();

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_patch_updates_only_provided_key);
    RUN_TEST(test_values_are_clamped_to_locked_bounds);
    RUN_TEST(test_invalid_combined_payload_is_rejected_atomically);
    RUN_TEST(test_save_map_includes_mount_orientation_fields);
    RUN_TEST(test_legacy_defaults_remain_level_when_payload_is_empty);
    RUN_TEST(test_mount_orientation_is_serialized_with_locked_field_names);
    RUN_TEST(test_non_object_mount_orientation_payload_is_rejected);
    return UNITY_END();
}
