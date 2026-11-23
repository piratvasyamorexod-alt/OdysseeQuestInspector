#pragma once

#include <Windows.h>
#include <optional>
#include <utility>

namespace OQIMouse {

inline HWND WindowRoot(HWND window) {
    return window ? GetAncestor(window, GA_ROOT) : nullptr;
}

inline bool HasFocus(HWND game) {
    return game && WindowRoot(GetForegroundWindow()) == WindowRoot(game);
}

inline bool ClientBounds(HWND window, RECT& bounds) {
    RECT client{};
    if (!GetClientRect(window, &client)) return false;
    POINT topLeft{client.left, client.top}, bottomRight{client.right, client.bottom};
    if (!ClientToScreen(window, &topLeft) || !ClientToScreen(window, &bottomRight)) return false;
    bounds = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
    return bounds.right > bounds.left && bounds.bottom > bounds.top;
}

class MouseEvents {
public:
    struct State {
        std::optional<POINT> position;
        bool pressed = false;
        int wheel = 0;
        bool down = false;
    };

private:
    static inline MouseEvents* active = nullptr;
    HWND window = nullptr;
    HHOOK hook = nullptr;
    HHOOK windowHook = nullptr;
    bool lostFocus = false;
    bool lowLevelClick = false;
    bool lowLevelWheel = false;
    bool leftDown = false;
    State pending;
    int wheelDelta = 0;
    unsigned count = 0;

    static LRESULT CALLBACK Observe(int code, WPARAM message, LPARAM data) {
        if (code == HC_ACTION && active && HasFocus(active->window)) {
            auto& mouse = *active;
            const auto& input = *reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
            switch (message) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MOUSEWHEEL:
                mouse.pending.position = input.pt;
                ++mouse.count;
                break;
            }
            if (message == WM_LBUTTONDOWN) {
                mouse.lowLevelClick = mouse.leftDown = mouse.pending.pressed = true;
            }
            if (message == WM_LBUTTONUP) mouse.leftDown = false;
            if (message == WM_MOUSEWHEEL) {
                mouse.lowLevelWheel = true;
                mouse.wheelDelta += static_cast<short>(HIWORD(input.mouseData));
            }
        }
        return CallNextHookEx(nullptr, code, message, data);
    }

    static LRESULT CALLBACK ObserveWindow(int code, WPARAM sent, LPARAM data) {
        if (code == HC_ACTION && active) {
            const auto& message = *reinterpret_cast<const CWPSTRUCT*>(data);
            auto& mouse = *active;
            if (message.hwnd == mouse.window) {
                if ((message.message == WM_ACTIVATEAPP && !message.wParam)
                    || (message.message == WM_ACTIVATE && LOWORD(message.wParam) == WA_INACTIVE)) {
                    mouse.lostFocus = true;
                    mouse.Discard();
                } else if ((message.message == WM_LBUTTONDOWN || message.message == WM_LBUTTONUP
                            || message.message == WM_MOUSEWHEEL)
                           && HasFocus(mouse.window)) {
                    // Some input wrappers pass buttons directly to the game window.
                    // Use those messages until the low-level hook sees each kind.
                    if (!mouse.lowLevelClick && message.message == WM_LBUTTONDOWN) {
                        mouse.leftDown = mouse.pending.pressed = true;
                    }
                    if (!mouse.lowLevelClick && message.message == WM_LBUTTONUP) mouse.leftDown = false;
                    if (!mouse.lowLevelWheel && message.message == WM_MOUSEWHEEL) {
                        mouse.wheelDelta += GET_WHEEL_DELTA_WPARAM(message.wParam);
                    }
                }
            }
        }
        return CallNextHookEx(nullptr, code, sent, data);
    }

public:
    MouseEvents() = default;
    MouseEvents(const MouseEvents&) = delete;
    MouseEvents& operator=(const MouseEvents&) = delete;
    ~MouseEvents() { Stop(); }

    bool Start(HWND game) {
        if (!game || active) return false;
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&Observe), &module)) return false;
        hook = SetWindowsHookExW(WH_MOUSE_LL, Observe, module, 0);
        if (!hook) return false;
        window = game;
        active = this;
        if (GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId()) {
            windowHook = SetWindowsHookExW(WH_CALLWNDPROC, ObserveWindow, nullptr, GetCurrentThreadId());
        }
        return true;
    }

    void Stop() {
        if (active == this) active = nullptr;
        if (windowHook) UnhookWindowsHookEx(std::exchange(windowHook, nullptr));
        if (hook) UnhookWindowsHookEx(std::exchange(hook, nullptr));
        Discard();
    }

    void Discard() {
        pending = {};
        wheelDelta = 0;
        lowLevelClick = lowLevelWheel = false;
        leftDown = false;
    }

    bool IsListening() const { return hook != nullptr; }
    unsigned Count() const { return count; }
    bool TakeFocusLoss() { return std::exchange(lostFocus, false); }

    State Take() {
        pending.down = leftDown;
        pending.wheel = wheelDelta / WHEEL_DELTA;
        wheelDelta -= pending.wheel * WHEEL_DELTA;
        return std::exchange(pending, State{});
    }
};

class JournalPointer {
    int showCalls = 0;
    std::optional<RECT> originalClip, ownedClip;

    static bool SameRect(const RECT& a, const RECT& b) {
        return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
    }

public:
    JournalPointer() = default;
    JournalPointer(const JournalPointer&) = delete;
    JournalPointer& operator=(const JournalPointer&) = delete;
    ~JournalPointer() { Leave(); }

    void Enter(const RECT& client, bool center) {
        RECT previous{};
        if (GetClipCursor(&previous) && !SameRect(client, previous) && ClipCursor(&client)) {
            originalClip = previous;
            ownedClip = client;
        }
        do {
            ++showCalls;
        } while (ShowCursor(TRUE) < 0);
        if (const auto cursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512))) SetCursor(cursor);
        if (center) SetCursorPos(client.left + (client.right - client.left) / 2,
                                client.top + (client.bottom - client.top) / 2);
    }

    void Leave() {
        RECT current{};
        if (originalClip && ownedClip && GetClipCursor(&current) && SameRect(*ownedClip, current)) {
            ClipCursor(&*originalClip);
        }
        originalClip.reset();
        ownedClip.reset();
        while (showCalls > 0) {
            ShowCursor(FALSE);
            --showCalls;
        }
    }
};

} // namespace OQIMouse
