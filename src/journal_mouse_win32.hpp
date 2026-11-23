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
    };

private:
    static inline MouseEvents* active = nullptr;
    HWND window = nullptr;
    HHOOK hook = nullptr;
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
            if (message == WM_LBUTTONDOWN) mouse.pending.pressed = true;
            if (message == WM_MOUSEWHEEL) mouse.wheelDelta += static_cast<short>(HIWORD(input.mouseData));
        }
        return CallNextHookEx(nullptr, code, message, data);
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
        return true;
    }

    void Stop() {
        if (active == this) active = nullptr;
        if (hook) UnhookWindowsHookEx(std::exchange(hook, nullptr));
        Discard();
    }

    void Discard() {
        pending = {};
        wheelDelta = 0;
    }

    bool IsListening() const { return hook != nullptr; }
    unsigned Count() const { return count; }

    State Take() {
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
