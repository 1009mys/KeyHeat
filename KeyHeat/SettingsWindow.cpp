#include "SettingsWindow.h"

#include <Windows.h>
#include <commdlg.h>
#include <string>



std::wstring ReadRegistryString(const wchar_t* valueName, const wchar_t* fallback)
{
	wchar_t buffer[LF_FACESIZE]{};
	DWORD size = sizeof(buffer);
	LONG result = RegGetValueW(
		HKEY_CURRENT_USER,
		kRegistryPath,
		valueName,
		RRF_RT_REG_SZ,
		nullptr,
		buffer,
		&size);

	if (result != ERROR_SUCCESS)
	{
		return fallback;
	}

	return buffer;
}

DWORD ReadRegistryDword(const wchar_t* valueName, DWORD fallback)
{
	DWORD value = fallback;
	DWORD size = sizeof(value);
	LONG result = RegGetValueW(
		HKEY_CURRENT_USER,
		kRegistryPath,
		valueName,
		RRF_RT_REG_DWORD,
		nullptr,
		&value,
		&size);

	if (result != ERROR_SUCCESS)
	{
		return fallback;
	}

	return value;
}

AppearanceSettings ReadAppearanceFromRegistry()
{
	AppearanceSettings settings{};
	settings.fontName = ReadRegistryString(kRegistryFontName, L"Segoe UI");
	settings.fontSize = ReadRegistryDword(kRegistryFontSize, 16);
	settings.textColor = ReadRegistryDword(kRegistryTextColor, 0x00000000);
	settings.monitorIndex = ReadRegistryDword(kRegistryMonitorIndex, 0);
	return settings;
}

void WriteAppearanceToRegistry(const AppearanceSettings& settings)
{
	HKEY key = nullptr;
	DWORD disposition = 0;
	LONG result = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		kRegistryPath,
		0,
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_SET_VALUE,
		nullptr,
		&key,
		&disposition);

	if (result == ERROR_SUCCESS)
	{
		RegSetValueExW(
			key,
			kRegistryFontName,
			0,
			REG_SZ,
			reinterpret_cast<const BYTE*>(settings.fontName.c_str()),
			static_cast<DWORD>((settings.fontName.size() + 1) * sizeof(wchar_t)));

		RegSetValueExW(
			key,
			kRegistryFontSize,
			0,
			REG_DWORD,
			reinterpret_cast<const BYTE*>(&settings.fontSize),
			sizeof(settings.fontSize));

		RegSetValueExW(
			key,
			kRegistryTextColor,
			0,
			REG_DWORD,
			reinterpret_cast<const BYTE*>(&settings.textColor),
			sizeof(settings.textColor));

		RegSetValueExW(
			key,
			kRegistryMonitorIndex,
			0,
			REG_DWORD,
			reinterpret_cast<const BYTE*>(&settings.monitorIndex),
			sizeof(settings.monitorIndex));

		RegCloseKey(key);
	}
}

bool ReadHookEnabledFromRegistry()
{
	DWORD enabled = 0;
	DWORD size = sizeof(enabled);
	LONG result = RegGetValueW(
		HKEY_CURRENT_USER,
		kRegistryPath,
		kRegistryValueName,
		RRF_RT_REG_DWORD,
		nullptr,
		&enabled,
		&size);

	if (result != ERROR_SUCCESS)
	{
		return false;
	}

	return enabled != 0;
}

void WriteHookEnabledToRegistry(bool enabled)
{
	HKEY key = nullptr;
	DWORD disposition = 0;
	LONG result = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		kRegistryPath,
		0,
		nullptr,
		REG_OPTION_NON_VOLATILE,
		KEY_SET_VALUE,
		nullptr,
		&key,
		&disposition);

	if (result == ERROR_SUCCESS)
	{
		DWORD value = enabled ? 1 : 0;
		RegSetValueExW(
			key,
			kRegistryValueName,
			0,
			REG_DWORD,
			reinterpret_cast<const BYTE*>(&value),
			sizeof(value));
		RegCloseKey(key);
	}
}

void UpdateSettingsWindowState(HWND hwnd, bool enabled)
{
	SetWindowTextW(GetDlgItem(hwnd, kStatusLabelId), enabled ? L"Status: Hook Running" : L"Status: Hook Stopped");
	SetWindowTextW(GetDlgItem(hwnd, kToggleButtonId), enabled ? L"Stop Hook" : L"Start Hook");
}

void PopulateAppearanceControls(HWND hwnd, const AppearanceSettings& settings)
{
	HWND fontCombo = GetDlgItem(hwnd, kFontNameComboId);
	LRESULT index = SendMessageW(fontCombo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(settings.fontName.c_str()));
	if (index != CB_ERR)
	{
		SendMessageW(fontCombo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
	}
	else
	{
		SetWindowTextW(fontCombo, settings.fontName.c_str());
	}

	wchar_t sizeText[16]{};
	wsprintfW(sizeText, L"%lu", settings.fontSize);
	SetWindowTextW(GetDlgItem(hwnd, kFontSizeEditId), sizeText);

	wchar_t colorText[16]{};
	wsprintfW(colorText, L"%06lX", settings.textColor & 0x00FFFFFF);
	SetWindowTextW(GetDlgItem(hwnd, kTextColorEditId), colorText);

	HWND monitorCombo = GetDlgItem(hwnd, kMonitorComboId);
	if (monitorCombo != nullptr)
	{
		LRESULT count = SendMessageW(monitorCombo, CB_GETCOUNT, 0, 0);
		for (LRESULT i = 0; i < count; ++i)
		{
			LRESULT itemData = SendMessageW(monitorCombo, CB_GETITEMDATA, static_cast<WPARAM>(i), 0);
			if (itemData == static_cast<LRESULT>(settings.monitorIndex))
			{
				SendMessageW(monitorCombo, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
				break;
			}
		}
	}
}

BOOL CALLBACK EnumMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam)
{
	auto* context = reinterpret_cast<MonitorEnumContext*>(lParam);
	if (context == nullptr || context->combo == nullptr)
	{
		return TRUE;
	}

	MONITORINFO monitorInfo{};
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (!GetMonitorInfoW(monitor, &monitorInfo))
	{
		return TRUE;
	}

	int width = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
	int height = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

	wchar_t label[128]{};
	wsprintfW(label, L"Monitor %d%s (%dx%d)", context->index + 1, (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) ? L" [Primary]" : L"", width, height);
	LRESULT itemIndex = SendMessageW(context->combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
	if (itemIndex != CB_ERR && itemIndex != CB_ERRSPACE)
	{
		SendMessageW(context->combo, CB_SETITEMDATA, static_cast<WPARAM>(itemIndex), static_cast<LPARAM>(context->index));
	}

	++context->index;
	return TRUE;
}

void PopulateMonitorList(HWND hwnd)
{
	HWND combo = GetDlgItem(hwnd, kMonitorComboId);
	if (combo == nullptr)
	{
		return;
	}

	SendMessageW(combo, CB_RESETCONTENT, 0, 0);
	MonitorEnumContext context{};
	context.combo = combo;
	context.index = 0;
	EnumDisplayMonitors(nullptr, nullptr, EnumMonitorProc, reinterpret_cast<LPARAM>(&context));

	if (SendMessageW(combo, CB_GETCOUNT, 0, 0) > 0)
	{
		SendMessageW(combo, CB_SETCURSEL, 0, 0);
	}
}

int CALLBACK EnumFontProc(const LOGFONTW* fontInfo, const TEXTMETRICW*, DWORD, LPARAM lParam)
{
	HWND combo = reinterpret_cast<HWND>(lParam);
	LRESULT exists = SendMessageW(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(fontInfo->lfFaceName));
	if (exists == CB_ERR)
	{
		SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontInfo->lfFaceName));
	}

	return 1;
}

void PopulateInstalledFonts(HWND hwnd)
{
	HWND combo = GetDlgItem(hwnd, kFontNameComboId);
	if (combo == nullptr)
	{
		return;
	}

	HDC hdc = GetDC(hwnd);
	if (hdc == nullptr)
	{
		return;
	}

	LOGFONTW logFont{};
	logFont.lfCharSet = DEFAULT_CHARSET;
	EnumFontFamiliesExW(hdc, &logFont, reinterpret_cast<FONTENUMPROCW>(EnumFontProc), reinterpret_cast<LPARAM>(combo), 0);
	ReleaseDC(hwnd, hdc);
}

bool ShowColorPalette(HWND hwnd, DWORD currentColor, DWORD* selectedColor)
{
	if (selectedColor == nullptr)
	{
		return false;
	}

	static COLORREF customColors[16]{};
	CHOOSECOLORW chooseColor{};
	chooseColor.lStructSize = sizeof(chooseColor);
	chooseColor.hwndOwner = hwnd;
	chooseColor.rgbResult = currentColor & 0x00FFFFFF;
	chooseColor.lpCustColors = customColors;
	chooseColor.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (!ChooseColorW(&chooseColor))
	{
		return false;
	}

	*selectedColor = chooseColor.rgbResult & 0x00FFFFFF;
	return true;
}

std::wstring ReadControlText(HWND hwnd, int controlId)
{
	wchar_t buffer[LF_FACESIZE]{};
	GetWindowTextW(GetDlgItem(hwnd, controlId), buffer, LF_FACESIZE);
	return buffer;
}

AppearanceSettings ReadAppearanceFromControls(HWND hwnd, const AppearanceSettings& current)
{
	AppearanceSettings updated = current;

	updated.fontName = ReadControlText(hwnd, kFontNameComboId);
	if (updated.fontName.empty())
	{
		updated.fontName = L"Segoe UI";
	}

	const std::wstring sizeText = ReadControlText(hwnd, kFontSizeEditId);
	unsigned long parsedSize = wcstoul(sizeText.c_str(), nullptr, 10);
	if (parsedSize > 0)
	{
		updated.fontSize = static_cast<DWORD>(parsedSize);
	}

	const std::wstring colorText = ReadControlText(hwnd, kTextColorEditId);
	unsigned long parsedColor = wcstoul(colorText.c_str(), nullptr, 16);
	updated.textColor = static_cast<DWORD>(parsedColor & 0x00FFFFFF);

	HWND monitorCombo = GetDlgItem(hwnd, kMonitorComboId);
	if (monitorCombo != nullptr)
	{
		LRESULT selected = SendMessageW(monitorCombo, CB_GETCURSEL, 0, 0);
		if (selected != CB_ERR)
		{
			LRESULT itemData = SendMessageW(monitorCombo, CB_GETITEMDATA, static_cast<WPARAM>(selected), 0);
			if (itemData != CB_ERR)
			{
				updated.monitorIndex = static_cast<DWORD>(itemData);
			}
		}
	}

	return updated;
}

LRESULT CALLBACK SettingsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
		return TRUE;
	}

	auto* context = reinterpret_cast<SettingsWindowContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_CREATE:
		CreateWindowExW(
			0,
			L"STATIC",
			L"Status: Hook Stopped",
			WS_CHILD | WS_VISIBLE,
			16,
			16,
			220,
			24,
			hwnd,
			reinterpret_cast<HMENU>(kStatusLabelId),
			GetModuleHandleW(nullptr),
			nullptr);

		CreateWindowExW(
			0,
			L"BUTTON",
			L"Start Hook",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			16,
			50,
			220,
			28,
			hwnd,
			reinterpret_cast<HMENU>(kToggleButtonId),
			GetModuleHandleW(nullptr),
			nullptr);

		CreateWindowExW(0, L"STATIC", L"Font Name", WS_CHILD | WS_VISIBLE, 16, 90, 90, 20, hwnd, reinterpret_cast<HMENU>(kFontNameLabelId), GetModuleHandleW(nullptr), nullptr);
		CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL | CBS_SORT, 110, 88, 130, 300, hwnd, reinterpret_cast<HMENU>(kFontNameComboId), GetModuleHandleW(nullptr), nullptr);

		CreateWindowExW(0, L"STATIC", L"Font Size", WS_CHILD | WS_VISIBLE, 16, 120, 90, 20, hwnd, reinterpret_cast<HMENU>(kFontSizeLabelId), GetModuleHandleW(nullptr), nullptr);
		CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 110, 118, 130, 22, hwnd, reinterpret_cast<HMENU>(kFontSizeEditId), GetModuleHandleW(nullptr), nullptr);

		CreateWindowExW(0, L"STATIC", L"Text Color", WS_CHILD | WS_VISIBLE, 16, 150, 90, 20, hwnd, reinterpret_cast<HMENU>(kTextColorLabelId), GetModuleHandleW(nullptr), nullptr);
		CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 110, 148, 70, 22, hwnd, reinterpret_cast<HMENU>(kTextColorEditId), GetModuleHandleW(nullptr), nullptr);
		CreateWindowExW(0, L"BUTTON", L"Pick...", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 186, 148, 54, 22, hwnd, reinterpret_cast<HMENU>(kPickColorButtonId), GetModuleHandleW(nullptr), nullptr);

		CreateWindowExW(0, L"STATIC", L"Monitor", WS_CHILD | WS_VISIBLE, 16, 180, 90, 20, hwnd, reinterpret_cast<HMENU>(kMonitorLabelId), GetModuleHandleW(nullptr), nullptr);
		CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST, 110, 178, 130, 200, hwnd, reinterpret_cast<HMENU>(kMonitorComboId), GetModuleHandleW(nullptr), nullptr);

		CreateWindowExW(
			0,
			L"BUTTON",
			L"Save Appearance",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,
			16,
			210,
			224,
			28,
			hwnd,
			reinterpret_cast<HMENU>(kSaveAppearanceButtonId),
			GetModuleHandleW(nullptr),
			nullptr);

		if (context != nullptr && context->hookEnabled != nullptr && context->appearanceSettings != nullptr)
		{
			PopulateInstalledFonts(hwnd);
			PopulateMonitorList(hwnd);
			UpdateSettingsWindowState(hwnd, *context->hookEnabled);
			PopulateAppearanceControls(hwnd, *context->appearanceSettings);
		}
		return 0;

	case WM_COMMAND:
		if (LOWORD(wParam) == kToggleButtonId && context != nullptr && context->hookEnabled != nullptr)
		{
			if (*context->hookEnabled)
			{
				context->stopHook();
				*context->hookEnabled = false;
			}
			else
			{
				if (context->startHook())
				{
					*context->hookEnabled = true;
				}
			}

			WriteHookEnabledToRegistry(*context->hookEnabled);
			UpdateSettingsWindowState(hwnd, *context->hookEnabled);
		}

		if (LOWORD(wParam) == kSaveAppearanceButtonId && context != nullptr)
		{
			if (context->appearanceSettings != nullptr)
			{
				*context->appearanceSettings = ReadAppearanceFromControls(hwnd, *context->appearanceSettings);
				WriteAppearanceToRegistry(*context->appearanceSettings);
			}
		}

		if (LOWORD(wParam) == kPickColorButtonId && context != nullptr)
		{
			if (context->appearanceSettings != nullptr)
			{
				DWORD pickedColor = context->appearanceSettings->textColor;
				if (ShowColorPalette(hwnd, context->appearanceSettings->textColor, &pickedColor))
				{
					context->appearanceSettings->textColor = pickedColor;
					PopulateAppearanceControls(hwnd, *context->appearanceSettings);
				}
			}
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND CreateSettingsWindow(HINSTANCE instance, SettingsWindowContext* context)
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = SettingsWindowProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = kSettingsClassName;

	if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		return nullptr;
	}

	return CreateWindowExW(
		0,
		kSettingsClassName,
		L"KeyHeat Settings",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		280,
		300,
		nullptr,
		nullptr,
		instance,
		context);
}
