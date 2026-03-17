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
void test_zero_tilt_returns_original_vector();
void test_tilt_azimuth_wrap_360_matches_zero();
void test_cardinal_downslope_azimuth_produces_directional_shift();
void test_reflect_changes_when_non_zero_orientation_is_applied();
void test_reflect_zero_tilt_matches_legacy_math();
void test_reflect_consumes_live_tilt_state_between_calls();

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
    RUN_TEST(test_zero_tilt_returns_original_vector);
    RUN_TEST(test_tilt_azimuth_wrap_360_matches_zero);
    RUN_TEST(test_cardinal_downslope_azimuth_produces_directional_shift);
    RUN_TEST(test_reflect_changes_when_non_zero_orientation_is_applied);
    RUN_TEST(test_reflect_zero_tilt_matches_legacy_math);
    RUN_TEST(test_reflect_consumes_live_tilt_state_between_calls);
    return UNITY_END();
}
