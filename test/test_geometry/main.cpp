#include <iostream>
#include "geometry_test.h"

void test_conversion() {
    // Test case 1: Convert from spherical to cartesian and back
    vec2 spherical(1., 45.);  // 45 degrees theta, 45 degrees phi
    std::cout << "Original spherical coordinates: (" << spherical.x << ", " << spherical.y << ")\n";
    
    vec3 cartesian = toCartesian(spherical);
    std::cout << "Cartesian coordinates: (" << cartesian.x << ", " << cartesian.y << ", " << cartesian.z << ")\n";
    
    vec2 spherical_back = toSpherical(cartesian);
    std::cout << "Back to spherical: (" << spherical_back.x << ", " << spherical_back.y << ")\n";
    
    // Test case 2: Special case - pointing straight up
    vec2 up(0.0, 0.0);
    std::cout << "\nTesting vertical vector:\n";
    vec3 up_cart = toCartesian(up);
    std::cout << "Vertical cartesian: (" << up_cart.x << ", " << up_cart.y << ", " << up_cart.z << ")\n";
}

int main() {
    std::cout << "Running geometry tests...\n\n";
    test_conversion();
    return 0;
}
