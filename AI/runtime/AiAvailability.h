#pragma once

namespace RogueCity::AI {

/// Checks if AI bridge runtime is available at runtime
/// Returns false if DLLs are missing or initialization failed
class AiAvailability {
public:
    /// Check if AI features are available (cached after first call)
    static bool IsAvailable();
    
    /// Get reason why AI is unavailable (empty if available)
    static const char* GetUnavailableReason();
    
    /// Force re-check (for testing)
    static void Reset();
    
private:
    static bool CheckAvailability();
    
    static bool s_checked;
    static bool s_available;
    static const char* s_reason;
};

} // namespace RogueCity::AI
