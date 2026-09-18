#pragma once

#include <Windows.h>
#include <commdlg.h>
#include <string>
#include "KHSetting.h"

constexpr wchar_t kSettingsClassName[] = L"KeyHeatSettingsWindow";
constexpr int kToggleButtonId = 1001;
constexpr int kStatusLabelId = 1002;
constexpr int kFontNameLabelId = 1003;
constexpr int kFontNameComboId = 1004;
constexpr int kFontSizeLabelId = 1005;
constexpr int kFontSizeEditId = 1006;
constexpr int kTextColorLabelId = 1007;
constexpr int kTextColorEditId = 1008;
constexpr int kSaveAppearanceButtonId = 1009;
constexpr int kPickColorButtonId = 1010;
constexpr int kMonitorLabelId = 1011;
constexpr int kMonitorComboId = 1012;

struct SettingsWindowContext
{
	bool* hookEnabled;
	bool (*startHook)();
	void (*stopHook)();
	AppearanceSettings* appearanceSettings;
};

struct MonitorEnumContext
{
	HWND combo;
	int index;
};
// Updates controls in the settings window to reflect the current enabled state.
void UpdateSettingsWindowState(HWND hwnd, bool enabled);

// Fills the appearance-related controls with the provided settings values.
void PopulateAppearanceControls(HWND hwnd, const AppearanceSettings& settings);

// Enumerates monitors and appends each one to the monitor selection control.
BOOL CALLBACK EnumMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam);

// Populates the monitor selection control with available displays.
void PopulateMonitorList(HWND hwnd);

// Enumerates installed fonts and appends each matching font to the font control.
int CALLBACK EnumFontProc(const LOGFONTW* fontInfo, const TEXTMETRICW*, DWORD, LPARAM lParam);

// Populates the font selection control with installed fonts.
void PopulateInstalledFonts(HWND hwnd);

// Shows the color picker and stores the selected color when confirmed.
bool ShowColorPalette(HWND hwnd, DWORD currentColor, DWORD* selectedColor);

// Reads the current text value from a child control.
std::wstring ReadControlText(HWND hwnd, int controlId);

// Builds appearance settings from the current control values with fallback to current values.
AppearanceSettings ReadAppearanceFromControls(HWND hwnd, const AppearanceSettings& current);

// Handles messages for the settings window.
LRESULT CALLBACK SettingsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Creates and returns the main settings window.
HWND CreateSettingsWindow(HINSTANCE instance, SettingsWindowContext* context);
