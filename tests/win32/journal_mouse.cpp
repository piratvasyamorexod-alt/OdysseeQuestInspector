#include "../../src/journal_mouse_win32.hpp"
#include <cstdio>
#include <cstdlib>

static void Check(bool condition, const char* description) {
    if (condition) return;
    std::fprintf(stderr, "%s (Win32 error %lu)\n", description, GetLastError());
    std::exit(1);
}

static void Pump() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

static void SendMouse(DWORD flags, LONG x = 0, LONG y = 0, DWORD data = 0) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = x;
    input.mi.dy = y;
    input.mi.mouseData = data;
    input.mi.dwFlags = flags;
    Check(SendInput(1, &input, sizeof(input)) == 1, "Send Windows mouse input");
}

static void WaitForPackets(const OQIMouse::MouseEvents& mouse, unsigned expected) {
    const auto deadline = GetTickCount64() + 2000;
    do {
        Pump();
        if (mouse.Count() >= expected) return;
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 20, QS_ALLINPUT);
    } while (GetTickCount64() < deadline);
    Check(false, "Receive Windows mouse events while pumping the same thread as the game");
}

int main() {
    auto instance = GetModuleHandleW(nullptr);
    HWND game = CreateWindowExW(WS_EX_TOPMOST, L"STATIC", L"Journal mouse test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                40, 40, 500, 350, nullptr, nullptr, instance, nullptr);
    Check(game != nullptr, "Create an interactive game window");
    SetForegroundWindow(game);
    Pump();
    Check(OQIMouse::HasFocus(game), "Give the test window foreground focus");

    RECT client{};
    Check(OQIMouse::ClientBounds(game, client), "Read the client rectangle in desktop coordinates");
    RECT original{};
    Check(GetClipCursor(&original) != FALSE, "Read the existing cursor clip");
    const POINT start{client.left + (client.right - client.left) / 2,
                      client.top + (client.bottom - client.top) / 2};
    OQIMouse::JournalPointer pointer;
    pointer.Enter(client, true);
    POINT current{};
    Check(GetCursorPos(&current) && current.x == start.x && current.y == start.y,
          "Center the cursor in the focused client area");

    OQIMouse::MouseEvents mouse;
    Check(mouse.Start(game), "Listen to Windows mouse input");
    SendMessageW(game, WM_LBUTTONDOWN, MK_LBUTTON, 0);
    SendMessageW(game, WM_MOUSEWHEEL, MAKEWPARAM(0, static_cast<WORD>(-WHEEL_DELTA)), 0);
    const auto direct = mouse.Take();
    Check(direct.pressed && direct.down && direct.wheel == -1,
          "Receive a click and scroll delivered to the game window");
    SendMessageW(game, WM_LBUTTONUP, 0, 0);
    Check(!mouse.Take().down, "Release the button in the window-message path");
    const auto before = mouse.Count();
    SendMouse(MOUSEEVENTF_MOVE, 15, 20);
    WaitForPackets(mouse, before + 1);
    const auto motion = mouse.Take();
    Check(motion.position && motion.position->x > start.x && motion.position->y > start.y,
          "Receive cursor movement in desktop coordinates");
    const auto beforeClick = mouse.Count();
    SendMouse(MOUSEEVENTF_LEFTDOWN);
    SendMouse(MOUSEEVENTF_LEFTUP);
    WaitForPackets(mouse, beforeClick + 2);
    Check(mouse.Take().pressed, "Receive a fast click between two game frames");
    Check(!mouse.Take().pressed, "Consume the click once");

    const auto beforeWheel = mouse.Count();
    SendMouse(MOUSEEVENTF_WHEEL, 0, 0, WHEEL_DELTA / 2);
    WaitForPackets(mouse, beforeWheel + 1);
    Check(mouse.Take().wheel == 0, "Keep a partial wheel notch");
    SendMouse(MOUSEEVENTF_WHEEL, 0, 0, WHEEL_DELTA / 2);
    WaitForPackets(mouse, beforeWheel + 2);
    Check(mouse.Take().wheel == 1, "Deliver one complete wheel notch");

    HWND other = CreateWindowExW(WS_EX_TOPMOST, L"STATIC", L"Focus test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 80, 80, 300, 220, nullptr, nullptr, instance, nullptr);
    Check(other != nullptr, "Create the other window for the focus test");
    SetForegroundWindow(other);
    Pump();
    Check(OQIMouse::HasFocus(other) && !OQIMouse::HasFocus(game), "Switch focus away from the game");
    const auto beforeOutside = mouse.Count();
    SendMouse(MOUSEEVENTF_WHEEL, 0, 0, WHEEL_DELTA);
    Pump();
    Check(mouse.Count() == beforeOutside && mouse.Take().wheel == 0, "Ignore mouse input in another window");

    SetForegroundWindow(game);
    Pump();
    Check(OQIMouse::HasFocus(game) && mouse.TakeFocusLoss(), "Remember focus loss between frames");
    Check(!mouse.TakeFocusLoss(), "Consume focus loss once");
    const auto afterFocus = mouse.Count();
    SendMouse(MOUSEEVENTF_LEFTDOWN);
    WaitForPackets(mouse, afterFocus + 1);
    const auto clicked = mouse.Take();
    Check(clicked.pressed && clicked.down, "Accept a held click after focus returns");
    SendMouse(MOUSEEVENTF_LEFTUP);
    WaitForPackets(mouse, afterFocus + 2);
    Check(!mouse.Take().down, "Release the button after focus returns");

    mouse.Stop();
    pointer.Leave();
    RECT restored{};
    Check(GetClipCursor(&restored) && EqualRect(&restored, &original), "Restore the prior cursor clip");
    DestroyWindow(other);
    DestroyWindow(game);
}
