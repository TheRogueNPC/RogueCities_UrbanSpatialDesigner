# Tech Stack

Project: RogueCities_UrbanSpatialDesigner

- **Language:** C++ (modern standard, core libraries + templates)
- **Build system:** CMake
- **Toolchains / IDEs:** MSVC / Visual Studio (solution files present), CMake builds (Ninja / MSBuild)
- **Rendering / UI:** Dear ImGui (ImGui + gl3w / platform backends)
- **Math & Utilities:** GLM, magic_enum, nlohmann_json
- **Scripting / Embedding:** Lua (sol2)
- **Third-party libs (in `3rdparty/`):** gl3w, glm, ImDesignManager, imvue, LInput, lua, magic_enum, nlohmann_json, sol2, tabulate, ymery-cpp, nanosvg, libcss (vendor list)
- **Packaging / deps:** vcpkg (repo includes vcpkg folder and integration points)
- **AI / Tooling:** Python FastAPI toolserver (tools/toolserver.py), PowerShell & batch bridge scripts for AI bridge (tools/*.ps1, .bat), AI client code under `AI/client`.
- **Tests:** CTest-compatible test executables (targets like `test_generators`, `test_core`, etc.)
- **CI / Pipeline:** Azure Pipelines (`azure-pipelines.yml` present)
- **OS / Platform:** Windows-first development (solution and .ilk files), cross-platform CMake support present.

---

Folder tree (top-level snapshot)

```
azure-pipelines.yml
build_and_run_gui.ps1
CMakeLists.txt
imgui.ini
ReadMe.md
RogueCities_UrbanSatialDesigner.slnx
RogueCityVisualizerGui.lnk
StartupBuild.bat
3rdparty/
	gl3w/
	glm/
	ImDesignManager/
	imvue/
	LInput/
	lua/
	magic_enum/
	nlohmann_json/
	sol2/
	tabulate/
	ymery-cpp/
AI/
	ai_config.json
	CMakeLists.txt
	client/
	config/
	docs/
	integration/
	protocol/
	runtime/
	tools/
app/
	CMakeLists.txt
	include/
	src/
	bin/
	imgui.ini
	RogueCityVisualizerGui.ilk
	RogueCityVisualizerHeadless.ilk
	test_ai.ilk
	test_core.ilk
	test_editor_hfsm.ilk
	test_generators.ilk
	test_ui_toolchain.ilk
	test_viewport.ilk
	AI/
build/
	ALL_BUILD.vcxproj
	ALL_BUILD.vcxproj.filters
	cmake_install.cmake
	CMakeCache.txt
	INSTALL.vcxproj
	INSTALL.vcxproj.filters
	RogueCityMVP.sln
	test_ai.vcxproj
	test_ai.vcxproj.filters
	test_core.vcxproj
	test_core.vcxproj.filters
	test_editor_hfsm.vcxproj
	test_editor_hfsm.vcxproj.filters
	test_generators.vcxproj
	test_generators.vcxproj.filters
	test_ui_toolchain.vcxproj
	test_ui_toolchain.vcxproj.filters
	ZERO_CHECK.vcxproj
	ZERO_CHECK.vcxproj.filters
	CMakeFiles/
	core/
	generators/
	...
build_gui/
	build.ninja
	cmake_install.cmake
	CMakeCache.txt
	app/
	CMakeFiles/
	core/
	generators/
	visualizer/
build_imgui_latest/
	ALL_BUILD.vcxproj
	ALL_BUILD.vcxproj.filters
	cmake_install.cmake
	CMakeCache.txt
	INSTALL.vcxproj
	INSTALL.vcxproj.filters
	RogueCityMVP.sln
	test_core.vcxproj
	test_core.vcxproj.filters
	test_editor_hfsm.vcxproj
	test_editor_hfsm.vcxproj.filters
	test_generators.vcxproj
	test_generators.vcxproj.filters
	ZERO_CHECK.vcxproj
	ZERO_CHECK.vcxproj.filters
	CMakeFiles/
	core/
	generators/
	...
core/
	...
docs/
examples/
Executibles/
generators/
mcp-server/
tests/
tools/
vcpkg/
visualizer/

```

Notes / next steps

- Add or update this file if dependencies change. Use this as a quick reference for onboarding and CI.
