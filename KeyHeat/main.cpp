#include <Windows.h>
#include <iostream>
#include "SettingsWindow.h"
#include "KeyPopup.h"

LRESULT CALLBACK KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam);

std::wstring resolveKeyName(const KBDLLHOOKSTRUCT& info)
{
    LONG keyInfo = static_cast<LONG>(info.scanCode << 16);
    if ((info.flags & LLKHF_EXTENDED) != 0)
    {
        keyInfo |= 1 << 24;
    }

    wchar_t keyName[128]{};
    int copied = GetKeyNameTextW(keyInfo, keyName, 128);
    if (copied > 0)
	{
        return std::wstring(keyName, copied);
	}

    return L"Unknown Key";
}

HHOOK g_keyboardHook = nullptr;
bool g_isHookEnabled = false;
AppearanceSettings g_appearanceSettings{};

bool StartKeyboardHook()
{
    if (g_keyboardHook != nullptr)
    {
        return true;
    }

    g_keyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        KeyboardHookProc,
        GetModuleHandleW(nullptr),
        0);

    return g_keyboardHook != nullptr;
}

void StopKeyboardHook()
{
    if (g_keyboardHook != nullptr)
    {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
}

LRESULT CALLBACK KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        const auto* info =
            reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYDOWN ||
            wParam == WM_SYSKEYDOWN)
        {
            ShowKeyPopup(GetModuleHandleW(nullptr), resolveKeyName(*info), g_appearanceSettings);
        }
    }

    // Always pass to next hook.
    return CallNextHookEx(
        g_keyboardHook,
        nCode,
        wParam,
        lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();
#endif

    g_isHookEnabled = ReadHookEnabledFromRegistry();
    g_appearanceSettings = ReadAppearanceFromRegistry();
    if (g_isHookEnabled && !StartKeyboardHook())
    {
        std::cerr
            << "SetWindowsHookEx failed: "
            << GetLastError()
            << '\n';
        g_isHookEnabled = false;
        WriteHookEnabledToRegistry(false);
    }

    SettingsWindowContext context{};
    context.hookEnabled = &g_isHookEnabled;
    context.startHook = StartKeyboardHook;
    context.stopHook = StopKeyboardHook;
    context.appearanceSettings = &g_appearanceSettings;

    HWND settingsWindow = CreateSettingsWindow(GetModuleHandleW(nullptr), &context);
    if (!settingsWindow)
    {
        std::cerr << "Failed to create settings window.\n";
        StopKeyboardHook();
        return 1;
    }

    ShowWindow(settingsWindow, SW_SHOW);
    UpdateWindow(settingsWindow);

    // Message loop is required for hook callback.
    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    StopKeyboardHook();

    return 0;
}