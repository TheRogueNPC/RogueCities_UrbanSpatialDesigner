#pragma once

#include <cstdint>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace LInput {

class MouseState {
public:
    void Update();

    [[nodiscard]] int X() const { return x_; }
    [[nodiscard]] int Y() const { return y_; }

    [[nodiscard]] bool LeftButtonDown() const { return left_down_; }
    [[nodiscard]] bool RightButtonDown() const { return right_down_; }
    [[nodiscard]] bool MiddleButtonDown() const { return middle_down_; }

private:
    int x_{ 0 };
    int y_{ 0 };
    bool left_down_{ false };
    bool right_down_{ false };
    bool middle_down_{ false };
};

class KeyboardState {
public:
    void Update();

    // `key` is a virtual-key code on Windows (VK_*) or ASCII for A-Z/0-9.
    [[nodiscard]] bool KeyDown(int key) const;

    [[nodiscard]] bool CtrlDown() const { return ctrl_down_; }
    [[nodiscard]] bool ShiftDown() const { return shift_down_; }
    [[nodiscard]] bool AltDown() const { return alt_down_; }

private:
    bool ctrl_down_{ false };
    bool shift_down_{ false };
    bool alt_down_{ false };
};

class Input {
public:
    MouseState Mouse;
    KeyboardState Keyboard;

    void Init();
    void Update();
};

// =========================
// Inline implementations
// =========================

inline void MouseState::Update() {
#if defined(_WIN32)
    HWND hwnd = GetForegroundWindow();
    POINT p{};
    if (GetCursorPos(&p)) {
        if (hwnd != nullptr) {
            ScreenToClient(hwnd, &p);
        }
        x_ = static_cast<int>(p.x);
        y_ = static_cast<int>(p.y);
    }

    left_down_ = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    right_down_ = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    middle_down_ = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
#else
    x_ = 0;
    y_ = 0;
    left_down_ = false;
    right_down_ = false;
    middle_down_ = false;
#endif
}

inline void KeyboardState::Update() {
#if defined(_WIN32)
    ctrl_down_ = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    shift_down_ = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    alt_down_ = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
#else
    ctrl_down_ = false;
    shift_down_ = false;
    alt_down_ = false;
#endif
}

inline bool KeyboardState::KeyDown(int key) const {
#if defined(_WIN32)
    return (GetAsyncKeyState(key) & 0x8000) != 0;
#else
    (void)key;
    return false;
#endif
}

inline void Input::Init() {
    Update();
}

inline void Input::Update() {
    Mouse.Update();
    Keyboard.Update();
}

} // namespace LInput

