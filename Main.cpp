#include "Include.hpp"


//pls no leak :'-(

std::uintptr_t originalSimpleCallFunction {};

std::uint32_t rectangleCount {};
RectangleRequest drawRectRequest[1000];

typedef HBITMAP(*CreateCompatibleBitmap_t)(_In_ HDC hdc, _In_ int cx, _In_ int cy);
typedef HDC(*CreateCompatibleDC_t)(_In_ HDC hdc);
typedef HBITMAP(*SelectBitmap_t)(_In_ HDC hdc, _In_ HBITMAP hbm);
typedef BOOL(*BitBit_t)(_In_ HDC hdcDst, _In_ INT x, _In_ INT y, _In_ INT cx, _In_ INT cy, _In_opt_ HDC hdcSrc, _In_ INT xSrc, _In_ INT ySrc, _In_ DWORD rop4, _In_ DWORD crBackColor, _In_ FLONG fl);
typedef BOOL(NTAPI* DeleteObject_t)(HGDIOBJ hobj);
typedef BOOL(APIENTRY* DeleteObjectApp_t)(_In_ HANDLE 	hobj);

CreateCompatibleBitmap_t NtGdiCreateCompatibleBitmap = 0;
CreateCompatibleDC_t GreCreateCompatibleDC = 0;
SelectBitmap_t GreSelectBitmap = 0;
DeleteObject_t GreDeleteObject = 0;
BitBit_t	NtGdiBitBlt = 0;
DeleteObjectApp_t NtGdiDeleteObjectApp = 0;

PEPROCESS NTAPI SubmitCommandHook(const std::uint64_t parameter1/*VOID*/) {
	const auto window = win32::ntUserGetForegroundWindow();
	const auto processId = win32::ntUserQueryWindow(window, 0);
	
	if(!window || !processId)
		return PsGetCurrentProcess();

	PEPROCESS process{};

	if (!NT_SUCCESS(PsLookupProcessByProcessId(reinterpret_cast<HANDLE>(processId), &process)))
		return PsGetCurrentProcess();

	if (!strstr("csgo.exe", reinterpret_cast<char*>(process) + 0x450))
		return PsGetCurrentProcess();

	//0x0 for getting the virtual screen (other hwnd somehow flicker)
	const auto windowDC = win32::ntUserGetDCEx(0x0, 0, 1);

	if (!windowDC)
		return PsGetCurrentProcess();

	//setting up backbuffer
	HBITMAP bitmap = NtGdiCreateCompatibleBitmap((HDC)windowDC, 500, 300);
	HDC windowDCMem = GreCreateCompatibleDC((HDC)windowDC);
	HBITMAP oldBitmap = GreSelectBitmap(windowDCMem, bitmap);

	//rendering
	Render::InitializeBrush(windowDC, 2, 0);

	Render::RenderText(windowDC, L"LMFAO", 50, 100, 0);
	Render::RenderRectangle(windowDC, 50, 50, 150, 150);

	//copy content to the regular context
	NtGdiBitBlt((HDC)windowDC, 0, 0, 500, 300, windowDCMem, 0, 0, 0xCC0020, 0, 0);

	GreSelectBitmap(windowDCMem, oldBitmap);
	NtGdiDeleteObjectApp(bitmap);
	GreDeleteObject(bitmap);
	win32::ntUserReleaseDC(windowDC);
	win32::ntUserReleaseDC((std::uint64_t)windowDCMem);

	return PsGetCurrentProcess();
}


VOID
Unload(
	IN PDRIVER_OBJECT DriverObject
) {
	//nigga
}

NTSTATUS DriverEntry(const PDRIVER_OBJECT driverObject, const PUNICODE_STRING registryPath) {

	driverObject->DriverUnload = Unload;

	PEPROCESS explorerProcess{};

	if (Nt::findProcessByName("explorer.exe", &explorerProcess)) {
		return STATUS_NOT_FOUND;
	}

	KAPC_STATE apcState{};
	KeStackAttachProcess(explorerProcess, &apcState);

	//------------------------------>

	std::uintptr_t win32kfull{};
	std::size_t win32kfullSize{};

	win32kfull = Nt::get_krnl_module_base("win32kfull.sys", win32kfullSize);

	if (!win32kfull) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kfull base!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	//get backbuffer dependencies
	DbgPrintEx(0, 0, "[DRIVER] win32kfull: 0x%p		win32kfullsize: 0x%p		\n", win32kfull, win32kfullSize);

	NtGdiCreateCompatibleBitmap = (CreateCompatibleBitmap_t)Nt::get_krnl_module_export("win32kfull.sys", "NtGdiCreateCompatibleBitmap");
	if (!NtGdiCreateCompatibleBitmap) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kfull:NtGdiCreateCompatibleBitmap!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	GreCreateCompatibleDC = (CreateCompatibleDC_t)Nt::get_krnl_module_export("win32kbase.sys", "GreCreateCompatibleDC");
	if (!GreCreateCompatibleDC) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kfull:GreCreateCompatibleDC!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	GreSelectBitmap = (SelectBitmap_t)Nt::get_krnl_module_export("win32kbase.sys", "GreSelectBitmap");
	if (!GreSelectBitmap) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kfull:GreSelectBitmap!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	GreDeleteObject = (DeleteObject_t)Nt::get_krnl_module_export("win32kbase.sys", "GreDeleteObject");
	if (!GreDeleteObject) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kbase:GreDeleteObject!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	NtGdiBitBlt = (BitBit_t)Nt::get_krnl_module_export("win32kfull.sys", "NtGdiBitBlt");
	if (!NtGdiBitBlt) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kfull:NtGdiBitBlt!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	NtGdiDeleteObjectApp = (DeleteObjectApp_t)Nt::get_krnl_module_export("win32kbase.sys", "NtGdiDeleteObjectApp");
	if (!NtGdiDeleteObjectApp) {
		DbgPrintEx(0, 0, "[DRIVER] Failed to get win32kfull:NtGdiDeleteObjectApp!");
		KeUnstackDetachProcess(&apcState);
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrintEx(0, 0, "[DRIVER] NtGdiCreateCompatibleBitmap: 0x%p\n", NtGdiCreateCompatibleBitmap);
	DbgPrintEx(0, 0, "[DRIVER] NtGdiCreateCompatibleDC: 0x%p\n", GreCreateCompatibleDC);
	DbgPrintEx(0, 0, "[DRIVER] GreSelectBitmap: 0x%p\n", GreSelectBitmap);
	DbgPrintEx(0, 0, "[DRIVER] GreDeleteObject: 0x%p\n", GreDeleteObject);
	DbgPrintEx(0, 0, "[DRIVER] NtGdiBitBlt: 0x%p\n", NtGdiBitBlt);
	DbgPrintEx(0, 0, "[DRIVER] NtGdiDeleteObjectApp: 0x%p\n", NtGdiDeleteObjectApp);

	//KeUnstackDetachProcess(&apcState);
	//return STATUS_SUCCESS;
	//------------------------------>

	if (!Render::ResolveWin32Functions())
		return STATUS_NOT_FOUND;

	auto status = Render::HookSubmitCommand();

	KeUnstackDetachProcess(&apcState);

	return status;
}