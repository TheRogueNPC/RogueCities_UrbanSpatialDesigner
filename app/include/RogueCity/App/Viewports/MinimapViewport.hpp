/**
 * @class MinimapViewport
 * @brief Provides a 2D orthographic top-down minimap view for the city generation process.
 *
 * The MinimapViewport acts as an overview for the entire city-making process, offering contextual interactivity and navigation aid.
 * It can optionally sync its XY position with the primary viewport, allowing for coordinated navigation.
 * The minimap displays a simplified rendering of the city output and supports user-configurable size and camera position.
 *
 * @note The minimap is designed to be as useful as the editor itself, with potential for independent or synced camera control.
 *
 * @enum Size
 * Defines the available minimap sizes: Small, Medium, Large.
 *
 * @method MinimapViewport()
 * Constructor.
 *
 * @method ~MinimapViewport()
 * Destructor.
 *
 * @method void initialize()
 * Initializes the minimap viewport.
 *
 * @method void update(float delta_time)
 * Updates the minimap viewport state.
 *
 * @method void render()
 * Renders the minimap viewport.
 *
 * @method void set_city_output(const Generators::CityGenerator::CityOutput* output)
 * Sets the city output to display in the minimap (simplified rendering).
 *
 * @method void set_camera_position(const Core::Vec2& xy)
 * Sets the camera position for the minimap (can be synced or independent).
 *
 * @method Core::Vec2 get_camera_xy() const
 * Gets the current camera XY position.
 *
 * @method void set_size(Size size)
 * Sets the minimap size.
 *
 * @method Size get_size() const
 * Gets the current minimap size.
 *
 * @private
 * @var city_output_
 * Pointer to the city output data for rendering.
 *
 * @var camera_xy_
 * Current camera XY position.
 *
 * @var size_
 * Current minimap size.
 *
 * @var texture_size_
 * Render texture dimensions for the minimap.
 */
 
#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Pipeline/CityGenerator.hpp"
#include <imgui.h>

namespace RogueCity::App {

/// Minimap viewport: 2D orthographic top-down view acts as an overveiw for the entire city making proccess. needs to have contextual interactivity and is just as useful as the editor itself, Syncs XY with primary viewport (optional), acts as navigation aid
class MinimapViewport {
public:
    enum class Size { Small, Medium, Large };

    MinimapViewport();
    ~MinimapViewport();

    void initialize();
    void update(float delta_time);
    void render();

    /// Set city output to display (simplified rendering)
    void set_city_output(const Generators::CityGenerator::CityOutput* output);

    /// Camera position (synced or independent)
    ///todo allow for a toggle to set if synced or independant 
    void set_camera_position(const Core::Vec2& xy);
    [[nodiscard]] Core::Vec2 get_camera_xy() const;

    /// Configuration needs to allow for user control
    void set_size(Size size);
    [[nodiscard]] Size get_size() const;

private:
    const Generators::CityGenerator::CityOutput* city_output_{ nullptr };
    Core::Vec2 camera_xy_{ 0.0, 0.0 };
    Size size_{ Size::Medium };
    ImVec2 texture_size_{ 512, 512 };  // Render texture dimensions
};

} // namespace RogueCity::App
