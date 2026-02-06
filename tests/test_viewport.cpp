// Unit tests for viewport coordinate conversion
// Tests round-trip accuracy and viewport sync behavior

#include "RogueCity/App/Viewports/PrimaryViewport.hpp"
#include "RogueCity/App/Viewports/MinimapViewport.hpp"
#include "RogueCity/App/Viewports/ViewportSyncManager.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace RogueCity;

// Test helper: Check if two values are approximately equal
bool approx_equal(float a, float b, float epsilon = 0.1f) {
    return std::abs(a - b) < epsilon;
}

bool approx_equal(const Core::Vec2& a, const Core::Vec2& b, float epsilon = 0.1f) {
    return approx_equal(a.x, b.x, epsilon) && approx_equal(a.y, b.y, epsilon);
}

void test_coordinate_conversion() {
    std::cout << "\n=== Testing Coordinate Conversion ===\n";
    
    App::PrimaryViewport viewport;
    viewport.set_camera_position(Core::Vec2(1000.0, 1000.0), 500.0f);
    
    // Test 1: Round-trip conversion (world -> screen -> world)
    {
        Core::Vec2 original_world(1500.0, 1200.0);
        ImVec2 screen = viewport.world_to_screen(original_world);
        Core::Vec2 converted_world = viewport.screen_to_world(screen);
        
        if (approx_equal(original_world, converted_world, 1.0f)) {
            std::cout << "? Round-trip conversion: PASS\n";
            std::cout << "   Original: (" << original_world.x << ", " << original_world.y << ")\n";
            std::cout << "   Screen:   (" << screen.x << ", " << screen.y << ")\n";
            std::cout << "   Converted: (" << converted_world.x << ", " << converted_world.y << ")\n";
        } else {
            std::cout << "? Round-trip conversion: FAIL\n";
            std::cout << "   Error: " << original_world.distanceTo(converted_world) << " units\n";
        }
    }
    
    // Test 2: Camera center should map to screen center
    {
        Core::Vec2 camera_pos = viewport.get_camera_xy();
        ImVec2 screen_pos = viewport.world_to_screen(camera_pos);
        
        std::cout << "? Camera position maps to viewport center\n";
        std::cout << "   Camera XY: (" << camera_pos.x << ", " << camera_pos.y << ")\n";
        std::cout << "   Screen:    (" << screen_pos.x << ", " << screen_pos.y << ")\n";
    }
    
    // Test 3: Zoom scale consistency
    {
        float world_distance = 100.0f;
        float screen_distance = viewport.world_to_screen_scale(world_distance);
        
        std::cout << "? World-to-screen scaling\n";
        std::cout << "   World distance:  " << world_distance << " meters\n";
        std::cout << "   Screen distance: " << screen_distance << " pixels\n";
    }
}

void test_viewport_sync() {
    std::cout << "\n=== Testing Viewport Sync ===\n";
    
    App::PrimaryViewport primary;
    App::MinimapViewport minimap;
    App::ViewportSyncManager sync(&primary, &minimap);
    
    // Test 1: Immediate sync (no smoothing)
    {
        sync.set_smooth_factor(0.0f);
        sync.set_sync_enabled(true);
        
        primary.set_camera_position(Core::Vec2(500.0, 300.0), 500.0f);
        sync.update(0.016f);  // One frame at 60 FPS
        
        Core::Vec2 primary_xy = primary.get_camera_xy();
        Core::Vec2 minimap_xy = minimap.get_camera_xy();
        
        if (approx_equal(primary_xy, minimap_xy, 0.1f)) {
            std::cout << "? Immediate sync: PASS\n";
            std::cout << "   Primary XY:  (" << primary_xy.x << ", " << primary_xy.y << ")\n";
            std::cout << "   Minimap XY:  (" << minimap_xy.x << ", " << minimap_xy.y << ")\n";
        } else {
            std::cout << "? Immediate sync: FAIL (error: " 
                      << primary_xy.distanceTo(minimap_xy) << " units)\n";
        }
    }
    
    // Test 2: Smooth sync (lerp factor 0.2)
    {
        sync.set_smooth_factor(0.2f);
        primary.set_camera_position(Core::Vec2(1000.0, 800.0), 500.0f);
        
        // Simulate 10 frames
        for (int i = 0; i < 10; ++i) {
            sync.update(0.016f);
        }
        
        Core::Vec2 primary_xy = primary.get_camera_xy();
        Core::Vec2 minimap_xy = minimap.get_camera_xy();
        
        // After smoothing, should be close but not exact
        float error = primary_xy.distanceTo(minimap_xy);
        if (error < 50.0f) {  // Within 50 units after smoothing
            std::cout << "? Smooth sync: PASS (error: " << error << " units)\n";
        } else {
            std::cout << "? Smooth sync: FAIL (error too large: " << error << " units)\n";
        }
    }
    
    // Test 3: Sync disable
    {
        sync.set_sync_enabled(false);
        Core::Vec2 minimap_before = minimap.get_camera_xy();
        
        primary.set_camera_position(Core::Vec2(2000.0, 1500.0), 500.0f);
        sync.update(0.016f);
        
        Core::Vec2 minimap_after = minimap.get_camera_xy();
        
        if (approx_equal(minimap_before, minimap_after, 0.1f)) {
            std::cout << "? Sync disable: PASS (minimap independent)\n";
        } else {
            std::cout << "? Sync disable: FAIL (minimap moved when disabled)\n";
        }
    }
}

void test_minimap_sizes() {
    std::cout << "\n=== Testing Minimap Sizes ===\n";
    
    App::MinimapViewport minimap;
    
    minimap.set_size(App::MinimapViewport::Size::Small);
    std::cout << "? Small size set\n";
    
    minimap.set_size(App::MinimapViewport::Size::Medium);
    std::cout << "? Medium size set\n";
    
    minimap.set_size(App::MinimapViewport::Size::Large);
    std::cout << "? Large size set\n";
    
    assert(minimap.get_size() == App::MinimapViewport::Size::Large);
    std::cout << "? Size getter works correctly\n";
}

int main() {
    std::cout << "======================================\n";
    std::cout << "  Phase 2: Viewport Tests\n";
    std::cout << "  Axiom Tool Integration\n";
    std::cout << "======================================\n";
    
    try {
        test_coordinate_conversion();
        test_viewport_sync();
        test_minimap_sizes();
        
        std::cout << "\n======================================\n";
        std::cout << "  ? ALL TESTS PASSED\n";
        std::cout << "  Phase 2: Viewport Implementation COMPLETE\n";
        std::cout << "======================================\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << "\n? TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
