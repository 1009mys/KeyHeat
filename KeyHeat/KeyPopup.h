#pragma once

#include <Windows.h>
#include <string>
#include <random>
#include <algorithm>
#include "SettingsWindow.h"

constexpr wchar_t kPopupClassName[] = L"KeyHeatPopupWindow";
constexpr COLORREF kPopupTransparentColor = RGB(1, 2, 3);

struct KeyPopupData
{
	std::wstring text;
	ULONGLONG startTick;
	DWORD durationMs;
	HFONT font;
	COLORREF color;
};

struct PopupMonitorContext
{
	DWORD targetIndex;
	DWORD currentIndex;
	RECT workArea;
	bool found;
};

BOOL CALLBACK FindMonitorByIndexProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam);

RECT GetMonitorWorkAreaByIndex(DWORD monitorIndex);

LRESULT CALLBACK KeyPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool EnsurePopupClassRegistered(HINSTANCE instance);

HFONT CreatePopupFont(HWND owner, const std::wstring& fontName, DWORD fontSize);

void ShowKeyPopup(HINSTANCE instance, const std::wstring& text, const AppearanceSettings& appearance);
