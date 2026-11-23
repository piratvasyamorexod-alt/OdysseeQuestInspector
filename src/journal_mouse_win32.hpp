#pragma once

#include <Windows.h>
#include <commctrl.h>
#include <future>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#pragma comment(lib, "comctl32.lib")

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
    static inline thread_local MouseEvents* activeListener = nullptr;
    static inline MouseEvents* activeWindow = nullptr;
    HWND window = nullptr;
    HHOOK windowHook = nullptr;
    std::thread listener;
    DWORD listenerId = 0;
    mutable std::mutex inputMutex;
    bool lostFocus = false;
    bool lowLevelClick = false;
    bool lowLevelWheel = false;
    bool leftDown = false;
    State pending;
    int wheelDelta = 0;
    unsigned count = 0;

    static LRESULT CALLBACK Observe(int code, WPARAM message, LPARAM data) {
        if (code == HC_ACTION && activeListener && HasFocus(activeListener->window)) {
            auto& mouse = *activeListener;
            const auto& input = *reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
            std::lock_guard lock(mouse.inputMutex);
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
        if (code == HC_ACTION && activeWindow) {
            const auto& message = *reinterpret_cast<const CWPSTRUCT*>(data);
            auto& mouse = *activeWindow;
            if (message.hwnd == mouse.window) {
                if ((message.message == WM_ACTIVATEAPP && !message.wParam)
                    || (message.message == WM_ACTIVATE && LOWORD(message.wParam) == WA_INACTIVE)) {
                    std::lock_guard lock(mouse.inputMutex);
                    mouse.lostFocus = true;
                    mouse.ClearPending();
                } else if ((message.message == WM_LBUTTONDOWN || message.message == WM_LBUTTONUP
                            || message.message == WM_MOUSEWHEEL)
                           && HasFocus(mouse.window)) {
                    // Some input wrappers pass buttons directly to the game window.
                    // Use those messages until the low-level hook sees each kind.
                    std::lock_guard lock(mouse.inputMutex);
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

    void Listen(std::promise<bool> ready, HMODULE module) {
        MSG message{};
        // Create the thread's queue before Start returns. The game thread can now
        // pace its frames without delaying Windows' low-level mouse callback.
        PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        listenerId = GetCurrentThreadId();
        activeListener = this;
        const auto hook = SetWindowsHookExW(WH_MOUSE_LL, Observe, module, 0);
        ready.set_value(hook != nullptr);
        if (!hook) return;
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            DispatchMessageW(&message);
        }
        UnhookWindowsHookEx(hook);
    }

    void ClearPending() {
        pending = {};
        wheelDelta = 0;
        lowLevelClick = lowLevelWheel = false;
        leftDown = false;
    }

public:
    MouseEvents() = default;
    MouseEvents(const MouseEvents&) = delete;
    MouseEvents& operator=(const MouseEvents&) = delete;
    ~MouseEvents() { Stop(); }

    bool Start(HWND game) {
        if (!game || activeWindow || listener.joinable()) return false;
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&Observe), &module)) return false;
        window = game;
        std::promise<bool> ready;
        auto installed = ready.get_future();
        try {
            listener = std::thread(&MouseEvents::Listen, this, std::move(ready), module);
        } catch (const std::system_error&) {
            return false;
        }
        if (!installed.get()) {
            listener.join();
            return false;
        }
        activeWindow = this;
        if (GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId()) {
            windowHook = SetWindowsHookExW(WH_CALLWNDPROC, ObserveWindow, nullptr, GetCurrentThreadId());
        }
        return true;
    }

    void Stop() {
        if (activeWindow == this) activeWindow = nullptr;
        if (windowHook) UnhookWindowsHookEx(std::exchange(windowHook, nullptr));
        if (listener.joinable()) {
            PostThreadMessageW(listenerId, WM_QUIT, 0, 0);
            // A later hook in the chain may belong to this thread.
            const HANDLE worker = listener.native_handle();
            while (MsgWaitForMultipleObjects(1, &worker, FALSE, INFINITE, QS_SENDMESSAGE) == WAIT_OBJECT_0 + 1) {
                MSG message{};
                PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
            }
            listener.join();
        }
        Discard();
    }

    void Discard() {
        std::lock_guard lock(inputMutex);
        ClearPending();
    }

    bool IsListening() const { return listener.joinable(); }
    unsigned Count() const {
        std::lock_guard lock(inputMutex);
        return count;
    }
    bool TakeFocusLoss() {
        std::lock_guard lock(inputMutex);
        return std::exchange(lostFocus, false);
    }

    State Take() {
        std::lock_guard lock(inputMutex);
        pending.down = leftDown;
        pending.wheel = wheelDelta / WHEEL_DELTA;
        wheelDelta -= pending.wheel * WHEEL_DELTA;
        return std::exchange(pending, State{});
    }
};

class JournalPointer {
    HWND cursorWindow = nullptr;
    HCURSOR arrow = nullptr;
    int showCalls = 0;
    std::optional<RECT> originalClip, ownedClip;

    static bool SameRect(const RECT& a, const RECT& b) {
        return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
    }

    static LRESULT CALLBACK KeepSystemCursor(HWND window, UINT message, WPARAM wp, LPARAM lp,
                                              UINT_PTR id, DWORD_PTR context) {
        auto& pointer = *reinterpret_cast<JournalPointer*>(context);
        if (message == WM_SETCURSOR && LOWORD(lp) == HTCLIENT && HasFocus(window)) {
            SetCursor(pointer.arrow);
            return TRUE;
        }
        if (message == WM_NCDESTROY) {
            RemoveWindowSubclass(window, KeepSystemCursor, id);
            pointer.cursorWindow = nullptr;
        }
        return DefSubclassProc(window, message, wp, lp);
    }

public:
    JournalPointer() = default;
    JournalPointer(const JournalPointer&) = delete;
    JournalPointer& operator=(const JournalPointer&) = delete;
    ~JournalPointer() { Leave(); }

    void Enter(HWND window, const RECT& client, bool center) {
        RECT previous{};
        if (GetClipCursor(&previous) && !SameRect(client, previous) && ClipCursor(&client)) {
            originalClip = previous;
            ownedClip = client;
        }
        arrow = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        if (arrow && GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId()
            && SetWindowSubclass(window, KeepSystemCursor, 0, reinterpret_cast<DWORD_PTR>(this))) {
            cursorWindow = window;
        }
        do {
            ++showCalls;
        } while (ShowCursor(TRUE) < 0);
        if (arrow) SetCursor(arrow);
        if (center) SetCursorPos(client.left + (client.right - client.left) / 2,
                                client.top + (client.bottom - client.top) / 2);
    }

    void Leave() {
        if (cursorWindow) RemoveWindowSubclass(std::exchange(cursorWindow, nullptr), KeepSystemCursor, 0);
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
