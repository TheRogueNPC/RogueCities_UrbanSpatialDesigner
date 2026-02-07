# ==================================================
# RogueCity AI Protocol Layer
# ==================================================
# AI-driven UI control protocol for editor automation
# Lives in AI/ directory per Ai-Control Implementation.md

project(RogueCityAI VERSION 0.1.0 LANGUAGES CXX)

message(STATUS "Configuring AI Protocol Layer...")

# === SOURCE FILES ===
set(AI_SOURCES
    protocol/UiAgentProtocol.cpp
    tools/HttpClient.cpp
    integration/AiAssist.cpp
)

set(AI_HEADERS
    protocol/UiAgentProtocol.h
    tools/HttpClient.h
    integration/AiAssist.h
)

# === LIBRARY TARGET ===
add_library(RogueCityAI STATIC ${AI_SOURCES} ${AI_HEADERS})

target_include_directories(RogueCityAI 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/3rdparty/nlohmann_json/include
)

target_compile_features(RogueCityAI PUBLIC cxx_std_20)

# === DEPENDENCIES ===
# Core library (for Editor state)
target_link_libraries(RogueCityAI PUBLIC RogueCityCore)

# nlohmann_json (header-only)
# We've downloaded it to 3rdparty/nlohmann_json/include

# ImGui (for UI controls) - check if available
# RogueCityImGui is created in app/CMakeLists.txt
if(NOT TARGET RogueCityImGui)
    # Create ImGui target if it doesn't exist
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rdparty/imvue/imgui")
    if(EXISTS "${IMGUI_DIR}/imgui.h")
        add_library(RogueCityImGui STATIC
            "${IMGUI_DIR}/imgui.cpp"
            "${IMGUI_DIR}/imgui_draw.cpp"
            "${IMGUI_DIR}/imgui_widgets.cpp"
            "${IMGUI_DIR}/imgui_tables.cpp"
            "${IMGUI_DIR}/imgui_demo.cpp"
        )
        target_include_directories(RogueCityImGui PUBLIC "${IMGUI_DIR}")
        target_compile_features(RogueCityImGui PUBLIC cxx_std_20)
    endif()
endif()

if(TARGET RogueCityImGui)
    target_link_libraries(RogueCityAI PUBLIC RogueCityImGui)
    message(STATUS "AI Protocol Layer: ImGui found, UI controls enabled")
else()
    message(STATUS "AI Protocol Layer: ImGui not found, UI controls disabled")
endif()

message(STATUS "AI Protocol Layer configured successfully")
