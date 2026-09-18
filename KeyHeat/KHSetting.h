#pragma once

#include <Windows.h>
#include <string>


struct AppearanceSettings
{
	std::wstring fontName;
	DWORD fontSize;
	DWORD textColor;
	DWORD monitorIndex;
};

class KHSetting
{
private:
	inline static constexpr wchar_t RegistryPath[] = L"Software\\KeyHeat";
	inline static constexpr wchar_t RegistryValueName[] = L"HookEnabled";
	inline static constexpr wchar_t RegistryFontName[] = L"FontName";
	inline static constexpr wchar_t RegistryFontSize[] = L"FontSize";
	inline static constexpr wchar_t RegistryTextColor[] = L"TextColor";
	inline static constexpr wchar_t RegistryMonitorIndex[] = L"MonitorIndex";

	static std::wstring ReadRegiString(const wchar_t* valueName, const wchar_t* fallback);
	static DWORD ReadRegiDword(const wchar_t* valueName, DWORD fallback);
	static void WriteRegiString(const wchar_t* valueName, const std::wstring& value);
	static void WriteRegiDword(const wchar_t* valueName, DWORD value);

	inline static const AppearanceSettings DefaultAppearanceSettings{
		L"Segoe UI",
		16,
		RGB(255, 255, 255),
		0
	};

public:
	static bool ReadHookEnabled();
	static void WriteHookEnabled(bool enabled);
	static AppearanceSettings ReadAppearance();
	static void WriteAppearance(const AppearanceSettings& settings);
};
