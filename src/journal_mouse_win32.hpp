#pragma once

#include <Windows.h>
#include <optional>
#include <utility>
#include <vector>

namespace OQIMouse {

inline HWND WindowRoot(HWND window) {
    return window ? GetAncestor(window, GA_ROOT) : nullptr;
}

inline bool HasFocus(HWND game) {
    return game && WindowRoot(GetForegroundWindow()) == WindowRoot(game);
}

class RawMouseWindow {
public:
    struct Events {
        LONG x = 0, y = 0;
        bool down = false;
        bool pressed = false;
        int wheel = 0;
    };

private:
    static constexpr wchar_t className[] = L"OdysseeQuestInspector.RawMouse";
    static constexpr USHORT desktopPage = 1;
    static constexpr USHORT mouseUsage = 2;
    HWND window = nullptr;
    HWND game = nullptr;
    std::optional<RAWINPUTDEVICE> previousRegistration;
    bool registered = false;
    Events events;
    int wheelRemainder = 0;
    unsigned packetCount = 0;

    static bool GetRegistrations(std::vector<RAWINPUTDEVICE>& devices) {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) != 0) return false;
        devices.resize(count);
        return !count || GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == count;
    }

    void ReadPacket(LPARAM param) {
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(param), RID_INPUT, &input,
                            &size, sizeof(RAWINPUTHEADER)) == UINT(-1) || input.header.dwType != RIM_TYPEMOUSE) return;
        ++packetCount;
        const auto& mouse = input.data.mouse;
        if (!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
            events.x += mouse.lLastX;
            events.y += mouse.lLastY;
        }
        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) events.down = events.pressed = true;
        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) events.down = false;
        if (mouse.usButtonFlags & RI_MOUSE_WHEEL) wheelRemainder += static_cast<short>(mouse.usButtonData);
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lp);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        if (message == WM_INPUT) {
            auto* mouse = reinterpret_cast<RawMouseWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (mouse && HasFocus(mouse->game) && GET_RAWINPUT_CODE_WPARAM(wp) == RIM_INPUT) {
                mouse->ReadPacket(lp);
            }
        }
        return DefWindowProcW(window, message, wp, lp);
    }

    static HINSTANCE Instance() { return GetModuleHandleW(nullptr); }

public:
    RawMouseWindow() = default;
    RawMouseWindow(const RawMouseWindow&) = delete;
    RawMouseWindow& operator=(const RawMouseWindow&) = delete;
    ~RawMouseWindow() { Close(); }

    bool Open(HWND gameWindow) {
        game = gameWindow;
        if (!game) return false;
        previousRegistration.reset();
        std::vector<RAWINPUTDEVICE> devices;
        if (!GetRegistrations(devices)) return false;
        for (const auto& device : devices) {
            if (device.usUsagePage != desktopPage) continue;
            if (device.usUsage == 0 && (device.dwFlags & RIDEV_PAGEONLY)) return false;
            if (device.usUsage == mouseUsage) previousRegistration = device;
        }

        WNDCLASSW cls{};
        cls.lpfnWndProc = WindowProc;
        cls.hInstance = Instance();
        cls.lpszClassName = className;
        if (!RegisterClassW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        window = CreateWindowExW(0, className, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, cls.hInstance, this);
        if (!window) {
            UnregisterClassW(className, Instance());
            return false;
        }

        // Raw mouse registration is process-wide. Save the old target before
        // claiming it, then restore it before the game reacquires the mouse.
        const RAWINPUTDEVICE device{desktopPage, mouseUsage, 0, window};
        registered = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
        if (!registered) Close();
        return registered;
    }

    void Close() {
        if (!window) return;
        if (registered && OwnsRegistration()) {
            if (!previousRegistration || !RegisterRawInputDevices(&*previousRegistration, 1, sizeof(RAWINPUTDEVICE))) {
                const RAWINPUTDEVICE remove{desktopPage, mouseUsage, RIDEV_REMOVE, nullptr};
                RegisterRawInputDevices(&remove, 1, sizeof(remove));
            }
        }
        registered = false;
        DestroyWindow(std::exchange(window, nullptr));
        UnregisterClassW(className, Instance());
        events = {};
        wheelRemainder = 0;
    }

    bool IsOpen() const { return registered; }
    unsigned PacketsRead() const { return packetCount; }

    bool OwnsRegistration() const {
        std::vector<RAWINPUTDEVICE> devices;
        if (!window || !GetRegistrations(devices)) return false;
        for (const auto& device : devices) {
            if (device.usUsagePage == desktopPage && device.usUsage == mouseUsage) {
                return device.hwndTarget == window;
            }
        }
        return false;
    }

    Events Read(bool focused) {
        if (!registered) return {};
        MSG msg;
        while (PeekMessageW(&msg, window, WM_INPUT, WM_INPUT, PM_REMOVE)) DispatchMessageW(&msg);
        if (!focused) {
            events = {};
            wheelRemainder = 0;
            return {};
        }
        events.wheel = wheelRemainder / WHEEL_DELTA;
        wheelRemainder -= events.wheel * WHEEL_DELTA;
        return std::exchange(events, Events{0, 0, events.down});
    }
};

} // namespace OQIMouse
