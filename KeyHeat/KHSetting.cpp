#include "KHSetting.h"

#include <cwchar>

std::wstring KHSetting::ReadRegiString(const wchar_t* valueName, const wchar_t* fallback)
{
	if (valueName == nullptr)
	{
		return fallback != nullptr ? fallback : L"";
	}

	DWORD byteCount = 0;
	LONG result = RegGetValueW(
		HKEY_CURRENT_USER,
		RegistryPath,
		valueName,
		RRF_RT_REG_SZ,
		nullptr,
		nullptr,
		&byteCount);

	if (result != ERROR_SUCCESS || byteCount < sizeof(wchar_t))
	{
		return fallback != nullptr ? fallback : L"";
	}

	std::wstring value(byteCount / sizeof(wchar_t), L'\0');
	result = RegGetValueW(
		HKEY_CURRENT_USER,
		RegistryPath,
		valueName,
		RRF_RT_REG_SZ,
		nullptr,
		value.data(),
		&byteCount);

	if (result != ERROR_SUCCESS)
	{
		return fallback != nullptr ? fallback : L"";
	}

	value.resize(wcsnlen(value.c_str(), value.size()));
	if (value.empty())
	{
		return fallback != nullptr ? fallback : L"";
	}

	return value;
}

DWORD KHSetting::ReadRegiDword(const wchar_t* valueName, DWORD fallback)
{
	if (valueName == nullptr)
	{
		return fallback;
	}

	DWORD value = fallback;
	DWORD byteCount = sizeof(value);
	LONG result = RegGetValueW(
		HKEY_CURRENT_USER,
		RegistryPath,
		valueName,
		RRF_RT_REG_DWORD,
		nullptr,
		&value,
		&byteCount);

	if (result != ERROR_SUCCESS)
	{
		return fallback;
	}

	return value;
}

void KHSetting::WriteRegiString(const wchar_t* valueName, const std::wstring& value)
{
	if (valueName == nullptr)
	{
		return;
	}

	HKEY key = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, RegistryPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
	{
		return;
	}

	const DWORD byteCount = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
	RegSetValueExW(key, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), byteCount);
	RegCloseKey(key);
}

void KHSetting::WriteRegiDword(const wchar_t* valueName, DWORD value)
{
	if (valueName == nullptr)
	{
		return;
	}

	HKEY key = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, RegistryPath, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
	{
		return;
	}

	RegSetValueExW(key, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
	RegCloseKey(key);
}

bool KHSetting::ReadHookEnabled()
{
	return ReadRegiDword(RegistryValueName, 0) != 0;
}

void KHSetting::WriteHookEnabled(bool enabled)
{
	WriteRegiDword(RegistryValueName, enabled ? 1u : 0u);
}

AppearanceSettings KHSetting::ReadAppearance()
{
	AppearanceSettings settings{};
	settings.fontName = ReadRegiString(RegistryFontName, DefaultAppearanceSettings.fontName.c_str());
	settings.fontSize = ReadRegiDword(RegistryFontSize, DefaultAppearanceSettings.fontSize);
	settings.textColor = ReadRegiDword(RegistryTextColor, DefaultAppearanceSettings.textColor);
	settings.monitorIndex = ReadRegiDword(RegistryMonitorIndex, DefaultAppearanceSettings.monitorIndex);
	return settings;
}

void KHSetting::WriteAppearance(const AppearanceSettings& settings)
{
	WriteRegiString(RegistryFontName, settings.fontName.empty() ? DefaultAppearanceSettings.fontName : settings.fontName);
	WriteRegiDword(RegistryFontSize, settings.fontSize == 0 ? DefaultAppearanceSettings.fontSize : settings.fontSize);
	WriteRegiDword(RegistryTextColor, settings.textColor & 0x00FFFFFF);
	WriteRegiDword(RegistryMonitorIndex, settings.monitorIndex);
}
