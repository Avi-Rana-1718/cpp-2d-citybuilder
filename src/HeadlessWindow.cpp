// Native window for --headless / --bench runs.
//
// Lives in its own translation unit so <windows.h> macros (min, max, ...)
// never leak into the header-only game code.
//
// The window is shown (so OpenGL renders normally and screenshots work)
// but placed far off-screen, never activated, and kept off the taskbar,
// so a benchmark does not steal focus from whatever the user is doing.

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

void* createHeadlessWindowHandle(int width, int height) {
    static const wchar_t* kClassName = L"ChimeHeadlessWindow";
    static bool registered = false;
    HINSTANCE instance = GetModuleHandleW(nullptr);

    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = instance;
        wc.lpszClassName = kClassName;
        if (!RegisterClassW(&wc)) return nullptr;
        registered = true;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,   // no taskbar button, never takes focus
        kClassName, L"Chime (headless)",
        WS_POPUP,
        -10000, -10000, width, height,         // far outside every monitor
        nullptr, nullptr, instance, nullptr);
    if (hwnd == nullptr) return nullptr;

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    return hwnd;
}
#else
void* createHeadlessWindowHandle(int, int) {
    return nullptr; // caller falls back to a normal window, then hides it
}
#endif
