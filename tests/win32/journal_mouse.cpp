#include "../../src/journal_mouse_win32.hpp"
#include <cstdio>
#include <cstdlib>

static void Check(bool condition, const char* description) {
    if (condition) return;
    std::fprintf(stderr, "%s (Win32 error %lu)\n", description, GetLastError());
    std::exit(1);
}

static std::optional<RAWINPUTDEVICE> GetMouseRegistration() {
    UINT count = 0;
    Check(GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == 0,
          "Count registered devices");
    std::vector<RAWINPUTDEVICE> devices(count);
    if (count) {
        Check(GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == count,
              "Read registered devices");
    }
    for (const auto& device : devices) {
        if (device.usUsagePage == 1 && device.usUsage == 2) return device;
    }
    return std::nullopt;
}

static void RegisterMouse(HWND window, DWORD flags) {
    const RAWINPUTDEVICE registration{1, 2, flags, window};
    Check(RegisterRawInputDevices(&registration, 1, sizeof(registration)) != FALSE,
          "Change raw mouse recipient");
}

int main() {
    HWND previous = CreateWindowExW(0, L"STATIC", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                    GetModuleHandleW(nullptr), nullptr);
    Check(previous != nullptr, "Create initial raw input recipient");
    RegisterMouse(previous, RIDEV_INPUTSINK);

    OQIMouse::RawMouseWindow journal;
    Check(journal.Open(previous), "Open journal mouse window");
    Check(journal.OwnsRegistration(), "Journal receives raw mouse");
    auto active = GetMouseRegistration();
    Check(active && active->hwndTarget != previous && active->hwndTarget != nullptr && active->dwFlags == 0,
          "Input targets a separate window in the foreground");
    journal.Close();
    auto restored = GetMouseRegistration();
    Check(restored && restored->hwndTarget == previous && restored->dwFlags == RIDEV_INPUTSINK,
          "Closing journal restores the existing recipient and flags");

    Check(journal.Open(previous), "Reopen journal mouse window");
    RegisterMouse(previous, 0);
    const auto intervening = GetMouseRegistration();
    Check(intervening && intervening->hwndTarget == previous, "Read the intervening registration");
    Check(!journal.OwnsRegistration(), "Detect an intervening recipient");
    journal.Close();
    restored = GetMouseRegistration();
    Check(restored && restored->hwndTarget == previous && restored->dwFlags == intervening->dwFlags,
          "Keep the intervening recipient and flags");

    RegisterMouse(nullptr, RIDEV_REMOVE);
    Check(journal.Open(previous), "Open journal when no mouse is registered");
    journal.Close();
    Check(!GetMouseRegistration(), "Closing restores absence of a mouse registration");
    DestroyWindow(previous);
}
