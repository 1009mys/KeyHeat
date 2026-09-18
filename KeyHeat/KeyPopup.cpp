#include <Windows.h>
#include <string>
#include <random>
#include <algorithm>
#include "SettingsWindow.h"
#include "KeyPopup.h"


BOOL CALLBACK FindMonitorByIndexProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam)
{
	auto* context = reinterpret_cast<PopupMonitorContext*>(lParam);
	if (context == nullptr)
	{
		return FALSE;
	}

	if (context->currentIndex == context->targetIndex)
	{
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (GetMonitorInfoW(monitor, &monitorInfo))
		{
			context->workArea = monitorInfo.rcWork;
			context->found = true;
		}
		return FALSE;
	}

	++context->currentIndex;
	return TRUE;
}

RECT GetMonitorWorkAreaByIndex(DWORD monitorIndex)
{
	PopupMonitorContext context{};
	context.targetIndex = monitorIndex;
	context.currentIndex = 0;
	context.found = false;
	EnumDisplayMonitors(nullptr, nullptr, FindMonitorByIndexProc, reinterpret_cast<LPARAM>(&context));

	if (context.found)
	{
		return context.workArea;
	}

	RECT primaryWorkArea{};
	SystemParametersInfoW(SPI_GETWORKAREA, 0, &primaryWorkArea, 0);
	return primaryWorkArea;
}

LRESULT CALLBACK KeyPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
		return TRUE;
	}

	auto* data = reinterpret_cast<KeyPopupData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_CREATE:
		SetTimer(hwnd, 1, 30, nullptr);
		return 0;
	case WM_TIMER:
		if (data != nullptr)
		{
			ULONGLONG elapsed = GetTickCount64() - data->startTick;
			if (elapsed >= data->durationMs)
			{
				DestroyWindow(hwnd);
				return 0;
			}

			DWORD alphaValue = static_cast<DWORD>(255 - (elapsed * 255 / data->durationMs));
			SetLayeredWindowAttributes(hwnd, kPopupTransparentColor, static_cast<BYTE>(alphaValue), LWA_COLORKEY | LWA_ALPHA);
		}
		return 0;
	case WM_PAINT:
		if (data != nullptr)
		{
			PAINTSTRUCT ps{};
			HDC hdc = BeginPaint(hwnd, &ps);
			RECT rect{};
			GetClientRect(hwnd, &rect);
			HBRUSH brush = CreateSolidBrush(kPopupTransparentColor);
			FillRect(hdc, &rect, brush);
			DeleteObject(brush);

			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, data->color);
			HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, data->font));
			DrawTextW(hdc, data->text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			SelectObject(hdc, oldFont);
			EndPaint(hwnd, &ps);
		}
		return 0;
	case WM_DESTROY:
		if (data != nullptr)
		{
			if (data->font != nullptr)
			{
				DeleteObject(data->font);
			}
			delete data;
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		}
		return 0;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool EnsurePopupClassRegistered(HINSTANCE instance)
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = KeyPopupProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wc.lpszClassName = kPopupClassName;

	if (RegisterClassExW(&wc) != 0)
	{
		return true;
	}

	return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HFONT CreatePopupFont(HWND owner, const std::wstring& fontName, DWORD fontSize)
{
	HDC hdc = GetDC(owner);
	if (hdc == nullptr)
	{
		return nullptr;
	}

	int height = -MulDiv(static_cast<int>(fontSize), GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(owner, hdc);

	return CreateFontW(
		height,
		0,
		0,
		0,
		FW_BOLD,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		fontName.c_str());
}

void ShowKeyPopup(HINSTANCE instance, const std::wstring& text, const AppearanceSettings& appearance)
{
	if (text.empty())
	{
		return;
	}

	if (!EnsurePopupClassRegistered(instance))
	{
		return;
	}

	auto* data = new KeyPopupData{};
	data->text = text;
	data->startTick = GetTickCount64();
	data->durationMs = 1200;
	data->font = CreatePopupFont(nullptr, appearance.fontName, appearance.fontSize == 0 ? 16 : appearance.fontSize);
	data->color = appearance.textColor;

	if (data->font == nullptr)
	{
		delete data;
		return;
	}

	SIZE textSize{};
	HDC screenDc = GetDC(nullptr);
	HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(screenDc, data->font));
	GetTextExtentPoint32W(screenDc, data->text.c_str(), static_cast<int>(data->text.size()), &textSize);
	SelectObject(screenDc, oldFont);
	ReleaseDC(nullptr, screenDc);

	int width = textSize.cx + 24;
	int height = textSize.cy + 16;

	RECT workArea = GetMonitorWorkAreaByIndex(appearance.monitorIndex);
	int maxX = static_cast<int>((workArea.right - workArea.left) - width);
	int maxY = static_cast<int>((workArea.bottom - workArea.top) - height);
	if (maxX < 0)
	{
		maxX = 0;
	}
	if (maxY < 0)
	{
		maxY = 0;
	}

	static std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<int> xDist(0, maxX);
	std::uniform_int_distribution<int> yDist(0, maxY);

	int x = workArea.left + xDist(rng);
	int y = workArea.top + yDist(rng);

	HWND popup = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		kPopupClassName,
		L"",
		WS_POPUP,
		x,
		y,
		width,
		height,
		nullptr,
		nullptr,
		instance,
		data);

	if (popup == nullptr)
	{
		DeleteObject(data->font);
		delete data;
		return;
	}

	SetLayeredWindowAttributes(popup, kPopupTransparentColor, 255, LWA_COLORKEY | LWA_ALPHA);
	ShowWindow(popup, SW_SHOWNOACTIVATE);
	UpdateWindow(popup);
}
