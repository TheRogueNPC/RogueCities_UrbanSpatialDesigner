#include <iostream>
#include <string>
#include <sstream>

// This file is a UI toolchain bootstrap stub.
// It does NOT include ImGui; it's safe to use in console tests.
// Later, the logic here can be moved or wrapped for an ImGui Visualizer app.

#if defined(ROGUECITY_HAS_SOL2)
#include <sol/sol.hpp>
#endif

#if defined(ROGUECITY_HAS_TABULATE)
#include <tabulate/table.hpp>
#endif

namespace RogueCity::UI {

struct ToolchainStatus {
    bool lua_available{false};
    bool tabulate_available{false};

    std::string toString() const {
        std::ostringstream oss;
        oss << "[UI Toolchain Status]\n";
        oss << "  Lua/sol2:       " << (lua_available ? "OK" : "MISSING") << "\n";
        oss << "  tabulate:       " << (tabulate_available ? "OK" : "MISSING") << "\n";
        return oss.str();
    }
};

// This probe stays console-only (no ImGui) but will compile and run light checks
// for optional UI/tooling dependencies when CMake wires them in.
ToolchainStatus ProbeToolchain()
{
    ToolchainStatus status{};

#if defined(ROGUECITY_HAS_SOL2)
    status.lua_available = true;
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    lua.script("x = 123");
    (void)lua["x"].get<int>();
#endif

#if defined(ROGUECITY_HAS_TABULATE)
    status.tabulate_available = true;
    tabulate::Table t;
    t.add_row({"Key", "Value"});
    t.add_row({"ui_toolchain_demo", "OK"});
    (void)t.str();
#endif

    return status;
}

} // namespace RogueCity::UI

int main()
{
    using namespace RogueCity::UI;

    std::cout << "=== RogueCity UI Toolchain Probe ===" << std::endl;

    ToolchainStatus status = ProbeToolchain();
    std::cout << status.toString() << std::endl;

    if (!status.lua_available || !status.tabulate_available) {
        std::cout << "Note: External UI toolchain libraries are not wired yet. "
                     "This is expected before ImGui/Visualizer implementation." << std::endl;
    }

    std::cout << "UI toolchain probe complete." << std::endl;
    return 0;
}
