#include "Include.hpp"

PEPROCESS NTAPI SubmitCommandHook(const std::uint64_t parameter1/*VOID*/);

std::uintptr_t originalDxgkImport{};

namespace win32 {
	std::uint64_t(*ntUserGetForegroundWindow)() {};
	std::int32_t(*ntUserQueryWindow)(std::uint64_t handle, std::int32_t index) {};
	std::uint64_t(*ntUserGetDCEx)(std::uint64_t, std::uint64_t, std::uint64_t) {};
	std::uint64_t(*ntUserReleaseDC)(std::uint64_t dc) {};
	std::uint64_t(*ntGdiDdDDISubmitCommand)(std::uint64_t) {};
	std::uint64_t(*greCreateFontIndirectW)(LOGFONTW* font, std::uint64_t) {};
	std::uint64_t(*ntGdiSelectFont)(std::uint64_t dc, std::uint64_t font) {};
	std::uint64_t(*ntGdiCreateSolidBrush)(std::uint64_t color, std::uint64_t brush) {};
	std::uint64_t(*ntGdiCreatePen)(std::uint32_t penStyle, std::int32_t width, std::uint64_t color, std::uint64_t brush) {};
	void(*ntGdiSelectBrush)(std::uint64_t dc, std::uint64_t brush) {};
	void(*ntGdiSelectPen)(std::uint64_t dc, std::uint64_t pen) {};
	std::uint32_t(*greSetTextColor)(std::uint64_t dc, std::uint32_t color /*RGB*/) {};
	std::uint32_t(*greSetBkColor)(std::uint64_t dc, std::uint32_t color /*RGB*/) {};
	std::uint32_t(*greSetBkMode)(std::uint64_t dc, std::uint32_t mode) {};
	void(*greExtTextOutWInternal)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t, std::uint64_t,
		LPCWSTR text, std::uint32_t textSize, std::uint64_t, std::uint64_t, std::uint64_t) {};
	bool(*ntGdiMoveTo)(std::uint64_t dc, std::uint64_t left, std::uint64_t top, std::uint64_t* oldPosition) {};
	bool(*greLineTo)(std::uint64_t dc, std::uint64_t left, std::uint64_t top) {};
	std::uint32_t(*greRectangle)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t right, std::uint64_t bottom) {};
	std::uint32_t(*ntGdiRoundRect)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t right,
		std::uint64_t bottom, std::uint32_t width, std::uint32_t height) {};
	std::uint32_t(*ntGdiEllipse)(std::uint64_t dc, std::uint32_t left, std::uint32_t top, std::uint64_t right, std::uint64_t bottom) {};

	std::uint64_t(*ntGdiCreateCompatibleBitmap)(std::uint64_t dc, std::uint32_t cx, std::uint32_t cy) {};
	std::uint64_t(*ntGdiCreateCompatibleDC)(std::uint64_t dc) {};

}

namespace Render
{
	void InitializeBrush(uint64_t windowDC, int32_t brushwidth, uint64_t color)
	{
		win32::LOGFONTW logFont{};
		logFont.lfHeight = 10;
		logFont.lfWeight = 160 /* FW_NORMAL */;
		logFont.lfCharSet = 1 /* DEFAULT_CHARSET */;
		memcpy(&logFont.lfFaceName, L"Arial", sizeof(L"Arial"));
		const auto font = win32::greCreateFontIndirectW(&logFont, 0);
		if (font)
			win32::ntGdiSelectFont(windowDC, font);

		win32::greSetBkMode(windowDC, 1);

		win32::ntGdiSelectBrush(windowDC, Render::getStockObject(5));

		const auto pen = win32::ntGdiCreatePen(win32::PS_SOLID, brushwidth, color, 0);
		if (pen)
			win32::ntGdiSelectPen(windowDC, pen);
	}

	void RenderText(uint64_t windowDC, LPCWSTR text, int x, int y, uint32_t color)
	{
		win32::greSetBkColor(windowDC, 0x985F30);

		//Set the actual text color
		win32::greSetTextColor(windowDC, color);
		
		win32::greExtTextOutWInternal(windowDC, x, y, 0, 0, text, wcslen(text), 0, 0, 0);
	}

	void RenderLine(uint64_t windowDC, int x, int y, int x1, int y1)
	{
		win32::ntGdiMoveTo(windowDC, x, y, nullptr);
		win32::greLineTo(windowDC, x1, y1);
	}

	void RenderRectangle(uint64_t windowDC, int left, int top, int right, int bottom)
	{
		win32::greRectangle(windowDC, left, top, right, bottom);
	}

	void RenderRectangle(uint64_t windowDC, int left, int top, int right, int bottom, int rounding)
	{
		win32::ntGdiRoundRect(windowDC, 800, 800, 900, 900, rounding, rounding);
	}

	void Ellipse(uint64_t windowDC, int left, int top, int right, int bottom)
	{
		win32::ntGdiEllipse(windowDC, left, top, right, bottom);
	}

	std::uintptr_t getStockObject(const std::int32_t objectIndex) {
		const auto peb = PsGetProcessPeb(PsGetCurrentProcess());
		if (!peb)
			return {};

		// GdiSharedMemory is same as GdiSharedHandleTable
		const auto gdiSharedHandleTable = *reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(peb) + 0xF8 /*offset inside the PEB*/);
		if (!gdiSharedHandleTable)
			return {};

		const auto brushArray = reinterpret_cast<std::uintptr_t*>(gdiSharedHandleTable + 0x1800B0 /* brush handle table offset (function for reference: GetStockObject) */);
		if (!brushArray)
			return {};

		const auto hollowBrush = brushArray[objectIndex];

		return hollowBrush;
	}

	NTSTATUS HookSubmitCommand() {
		std::uintptr_t dxgkrnl{};
		std::size_t dxgkrnlSize{};
		if (Nt::findKernelModuleByName("dxgkrnl.sys", &dxgkrnl, &dxgkrnlSize))
			return STATUS_FAILED_DRIVER_ENTRY;

		std::uintptr_t dxgkSubmitCommand{};
		if (Nt::findModuleExportByName(dxgkrnl, "DxgkSubmitCommand", &dxgkSubmitCommand))
			return STATUS_FAILED_DRIVER_ENTRY;

		auto importCall = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(dxgkSubmitCommand),
			0x50, "\x48\x83\xEC\xFF\x48\x8B\xF1", "xxx?xxx");
		if (!importCall)
			return STATUS_FAILED_DRIVER_ENTRY;

		importCall += 0x7;

		// mov rax, address
		// call rax
		std::uint8_t shellcode[]{ 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD0 };

		const auto hookFunction = &SubmitCommandHook;
		memcpy(shellcode + 0x2, &hookFunction, sizeof(hookFunction));

		// Remap the memory and make it writable because we will overwrite it and it's located in the text section
		const auto mdl = IoAllocateMdl(reinterpret_cast<void*>(importCall), sizeof(shellcode),
			false, false, nullptr);
		if (!mdl)
			return STATUS_FAILED_DRIVER_ENTRY;

		MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
		const auto ntGdiDdDDISubmitCommandRemapped = reinterpret_cast<std::uintptr_t*>(MmMapLockedPagesSpecifyCache(mdl, KernelMode,
			MmNonCached, nullptr, false, HighPagePriority));
		if (!ntGdiDdDDISubmitCommandRemapped)
			return STATUS_FAILED_DRIVER_ENTRY;

		originalDxgkImport = *ntGdiDdDDISubmitCommandRemapped;

		DbgPrintEx(0, 0, "\n0x%p\n", ntGdiDdDDISubmitCommandRemapped);

		memcpy(ntGdiDdDDISubmitCommandRemapped, shellcode, sizeof(shellcode));

		MmUnlockPages(mdl);
		IoFreeMdl(mdl);
		
		return STATUS_SUCCESS;
	}

	bool ResolveWin32Functions() {
		{ // Get addresses of functions in the win32kfull driver
			std::uintptr_t win32kfull{};
			std::size_t win32kfullSize{};
			if (Nt::findKernelModuleByName("win32kfull.sys", &win32kfull, &win32kfullSize))
				return false;

			// For most of the stuff you can just call the Nt functions which are exported
			if (Nt::findModuleExportByName(win32kfull, "NtUserGetForegroundWindow", reinterpret_cast<std::uintptr_t*>(&win32::ntUserGetForegroundWindow)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtUserQueryWindow", reinterpret_cast<std::uintptr_t*>(&win32::ntUserQueryWindow)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtUserGetDCEx", reinterpret_cast<std::uintptr_t*>(&win32::ntUserGetDCEx)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiSelectFont", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiSelectFont)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiCreateSolidBrush", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiCreateSolidBrush)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiSelectBrush", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiSelectBrush)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiCreatePen", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiCreatePen)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiSelectPen", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiSelectPen)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiMoveTo", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiMoveTo)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiRoundRect", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiRoundRect)))
				return false;

			if (Nt::findModuleExportByName(win32kfull, "NtGdiEllipse", reinterpret_cast<std::uintptr_t*>(&win32::ntGdiEllipse)))
				return false;

			// Some functions are not exported directly or some Nt functions contain user mode address validity checks which we need to skip so we search for the internal GDI functions
			std::uintptr_t win32kfullText{};
			std::size_t win32kfullTextSize{};
			if (Nt::findModuleSection(win32kfull, ".text", &win32kfullText, &win32kfullTextSize))
				return false;

			{
				std::uintptr_t ntGdiRectangle{};
				if (Nt::findModuleExportByName(win32kfull, "NtGdiRectangle", reinterpret_cast<std::uintptr_t*>(&ntGdiRectangle)))
					return false;

				// Get the address of the GreRectangle function
				auto greRectangleAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(ntGdiRectangle),
					0x100, "\x8B\xD6\x48\x8B\xCB\xE8\xFF\xFF\xFF\xFF\x8B\xD8", "xxxxxx????xx");
				if (!greRectangleAddress)
					return false;

				greRectangleAddress += 6;
				win32::greRectangle = reinterpret_cast<decltype(win32::greRectangle)>(greRectangleAddress +
					*reinterpret_cast<std::int32_t*>(greRectangleAddress) + sizeof(std::int32_t));
			}

			{ // Get the address of the GreCreateFontIndirectW function
				auto greCreateFontIndirectWAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(win32kfullText),
					win32kfullTextSize, "\xE8\xFF\xFF\xFF\xFF\x49\x89\x47\x30", "x????xxxx");
				if (!greCreateFontIndirectWAddress)
					return false;

				greCreateFontIndirectWAddress += 1;
				win32::greCreateFontIndirectW = reinterpret_cast<decltype(win32::greCreateFontIndirectW)>(greCreateFontIndirectWAddress +
					*reinterpret_cast<std::int32_t*>(greCreateFontIndirectWAddress) + sizeof(std::int32_t));
			}

			{ // Get the address of the GreSetTextColor function
				auto greSetTextColorAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(win32kfullText),
					win32kfullTextSize, "\x48\x8B\xCF\xE8\xFF\xFF\xFF\xFF\x44\x8B\xF8\x45\x33\xC9", "xxxx????xxxxxx");
				if (!greSetTextColorAddress)
					return false;

				greSetTextColorAddress += 4;
				win32::greSetTextColor = reinterpret_cast<decltype(win32::greSetTextColor)>(greSetTextColorAddress +
					*reinterpret_cast<std::int32_t*>(greSetTextColorAddress) + sizeof(std::int32_t));
			}

			{ // Get the address of the GreSetBkMode function
				auto greSetBkModeAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(win32kfullText),
					win32kfullTextSize, "\xBA\xFF\xFF\xFF\xFF\x48\x8B\xCB\xE8\xFF\xFF\xFF\xFF\xBA\xFF\xFF\xFF\x00", "x????xxxx????xxxxx");
				if (!greSetBkModeAddress)
					return false;

				greSetBkModeAddress += 9;
				win32::greSetBkMode = reinterpret_cast<decltype(win32::greSetBkMode)>(greSetBkModeAddress +
					*reinterpret_cast<std::int32_t*>(greSetBkModeAddress) + sizeof(std::int32_t));
			}

			{ // Get the address of the GreSetBkColor function
				auto greSetBkColorAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(win32kfullText),
					win32kfullTextSize, "\xE8\xFF\xFF\xFF\xFF\xBA\xFF\xFF\xFF\xFF\x48\x8B\xCB\x44\x8B\xF0\xE8", "x????x????xxxxxxx");
				if (!greSetBkColorAddress)
					return false;

				greSetBkColorAddress += 17;
				win32::greSetBkColor = reinterpret_cast<decltype(win32::greSetBkColor)>(greSetBkColorAddress +
					*reinterpret_cast<std::int32_t*>(greSetBkColorAddress) + sizeof(std::int32_t));
			}

			{ // Get the address of the GreExtTextOutWInternal function
				std::uintptr_t ntGdiExtTextOutW{};
				if (Nt::findModuleExportByName(win32kfull, "NtGdiExtTextOutW", reinterpret_cast<std::uintptr_t*>(&ntGdiExtTextOutW)))
					return false;

				auto greExtTextOutWInternalAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(ntGdiExtTextOutW),
					0x1000, "\x8B\x54\x24\x78\xE8\xFF\xFF\xFF\xFF\x8B\xF0", "xxxxx????xx");
				if (!greExtTextOutWInternalAddress)
					return false;

				greExtTextOutWInternalAddress += 5;
				win32::greExtTextOutWInternal = reinterpret_cast<decltype(win32::greExtTextOutWInternal)>(greExtTextOutWInternalAddress +
					*reinterpret_cast<std::int32_t*>(greExtTextOutWInternalAddress) + sizeof(std::int32_t));
			}

			{ // Get the address of the NtGdiLineTo function
				std::uintptr_t ntGdiLineTo{};
				if (Nt::findModuleExportByName(win32kfull, "NtGdiLineTo", reinterpret_cast<std::uintptr_t*>(&ntGdiLineTo)))
					return false;

				auto ntGdiLineToAddress = Scanner::scanPattern(reinterpret_cast<std::uint8_t*>(ntGdiLineTo),
					0x100, "\x45\x8B\xC6\x8B\xD6\x48\x8B\xCB\xE8", "xxxxxxxxx");
				if (!ntGdiLineToAddress)
					return false;

				ntGdiLineToAddress += 9;
				win32::greLineTo = reinterpret_cast<decltype(win32::greLineTo)>(ntGdiLineToAddress +
					*reinterpret_cast<std::int32_t*>(ntGdiLineToAddress) + sizeof(std::int32_t));
			}
		}

		{ // Get addresses of functions in the win32kbase driver
			std::uintptr_t win32kbase{};
			std::size_t win32kbaseSize{};
			if (Nt::findKernelModuleByName("win32kbase.sys", &win32kbase, &win32kbaseSize))
				return false;

			if (Nt::findModuleExportByName(win32kbase, "NtUserReleaseDC", reinterpret_cast<std::uintptr_t*>(&win32::ntUserReleaseDC)))
				return false;
		}

		return true;
	}
}
