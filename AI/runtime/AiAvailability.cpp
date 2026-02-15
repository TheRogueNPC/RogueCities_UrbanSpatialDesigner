// FILE: AiAvailability.cpp
// PURPOSE: Runtime availability check for AI bridge features

#include "AiAvailability.h"
#include "AiBridgeRuntime.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace RogueCity::AI {

bool AiAvailability::s_checked = false;
bool AiAvailability::s_available = false;
const char* AiAvailability::s_reason = nullptr;

bool AiAvailability::IsAvailable() {
    if (!s_checked) {
        s_available = CheckAvailability();
        s_checked = true;
    }
    return s_available;
}

const char* AiAvailability::GetUnavailableReason() {
    IsAvailable(); // Ensure check happened
    return s_reason ? s_reason : "";
}

void AiAvailability::Reset() {
    s_checked = false;
    s_available = false;
    s_reason = nullptr;
}

bool AiAvailability::CheckAvailability() {
    #ifdef _WIN32
    // Check for winhttp.dll (required for health checks)
    HMODULE winhttp = LoadLibraryExA("winhttp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!winhttp) {
        s_reason = "Missing winhttp.dll (required for HTTP health checks)";
        return false;
    }
    FreeLibrary(winhttp);
    #endif
    
    // Try to instantiate AiBridgeRuntime (tests config loading)
    try {
        auto& runtime = AiBridgeRuntime::Instance();
        (void)runtime; // Avoid unused warning
        
        // If we get here, initialization succeeded
        return true;
    }
    catch (const std::exception&) {
        s_reason = "AI bridge initialization failed";
        return false;
    }
    catch (...) {
        s_reason = "AI bridge initialization failed (unknown error)";
        return false;
    }
}

} // namespace RogueCity::AI
