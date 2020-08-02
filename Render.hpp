#pragma once

namespace Render
{
	void InitializeBrush(uint64_t windowDC, int32_t brushwidth, uint64_t color);
	NTSTATUS HookSubmitCommand();
	void RenderText(uint64_t windowDC, LPCWSTR text, int x, int y, uint32_t color);
	void RenderLine(uint64_t windowDC, int x, int y, int x1, int y1);
	void RenderRectangle(uint64_t windowDC, int left, int top, int right, int bottom);
	void RenderRectangle(uint64_t windowDC, int left, int top, int right, int bottom, int rounding);
	void Ellipse(uint64_t windowDC, int left, int top, int right, int bottom);
	bool ResolveWin32Functions();
	std::uintptr_t getStockObject(const std::int32_t objectIndex);
}

namespace win32 {
	typedef  enum
	{
		PS_COSMETIC = 0x00000000,
		PS_ENDCAP_ROUND = 0x00000000,
		PS_JOIN_ROUND = 0x00000000,
		PS_SOLID = 0x00000000,
		PS_DASH = 0x00000001,
		PS_DOT = 0x00000002,
		PS_DASHDOT = 0x00000003,
		PS_DASHDOTDOT = 0x00000004,
		PS_NULL = 0x00000005,
		PS_INSIDEFRAME = 0x00000006,
		PS_USERSTYLE = 0x00000007,
		PS_ALTERNATE = 0x00000008,
		PS_ENDCAP_SQUARE = 0x00000100,
		PS_ENDCAP_FLAT = 0x00000200,
		PS_JOIN_BEVEL = 0x00001000,
		PS_JOIN_MITER = 0x00002000,
		PS_GEOMETRIC = 0x00010000
	} PenStyle;

	typedef struct {
		LONG lfHeight;
		LONG lfWidth;
		LONG lfEscapement;
		LONG lfOrientation;
		LONG lfWeight;
		BYTE lfItalic;
		BYTE lfUnderline;
		BYTE lfStrikeOut;
		BYTE lfCharSet;
		BYTE lfOutPrecision;
		BYTE lfClipPrecision;
		BYTE lfQuality;
		BYTE lfPitchAndFamily;
		WCHAR lfFaceName[32];
	} LOGFONTW;

	extern std::uint64_t(*ntUserGetForegroundWindow)();
	extern std::int32_t(*ntUserQueryWindow)(std::uint64_t handle, std::int32_t index);
	extern std::uint64_t(*ntUserGetDCEx)(std::uint64_t, std::uint64_t, std::uint64_t);
	extern std::uint64_t(*ntUserReleaseDC)(std::uint64_t dc);
	extern std::uint64_t(*ntGdiDdDDISubmitCommand)(std::uint64_t);
	extern std::uint64_t(*greCreateFontIndirectW)(LOGFONTW* font, std::uint64_t);
	extern std::uint64_t(*ntGdiSelectFont)(std::uint64_t dc, std::uint64_t font);
	extern std::uint64_t(*ntGdiCreateSolidBrush)(std::uint64_t color, std::uint64_t brush);
	extern std::uint64_t(*ntGdiCreatePen)(std::uint32_t penStyle, std::int32_t width, std::uint64_t color, std::uint64_t brush);
	extern void(*ntGdiSelectBrush)(std::uint64_t dc, std::uint64_t brush);
	extern void(*ntGdiSelectPen)(std::uint64_t dc, std::uint64_t pen);
	extern std::uint32_t(*greSetTextColor)(std::uint64_t dc, std::uint32_t color /*RGB*/);
	extern std::uint32_t(*greSetBkColor)(std::uint64_t dc, std::uint32_t color /*RGB*/);
	extern std::uint32_t(*greSetBkMode)(std::uint64_t dc, std::uint32_t mode);
	extern void(*greExtTextOutWInternal)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t, std::uint64_t,
		LPCWSTR text, std::uint32_t textSize, std::uint64_t, std::uint64_t, std::uint64_t);
	extern bool(*ntGdiMoveTo)(std::uint64_t dc, std::uint64_t left, std::uint64_t top, std::uint64_t* oldPosition);
	extern bool(*greLineTo)(std::uint64_t dc, std::uint64_t left, std::uint64_t top);
	extern std::uint32_t(*greRectangle)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t right, std::uint64_t bottom);
	extern std::uint32_t(*ntGdiRoundRect)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t right,
		std::uint64_t bottom, std::uint32_t width, std::uint32_t height);
	extern std::uint32_t(*ntGdiEllipse)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t right, std::uint64_t bottom);
}

