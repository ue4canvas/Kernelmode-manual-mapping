
#include "Hvpp.h"

//#include "Detour.h"

#include "Debug.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mutex>
#include <algorithm>

#include "ImRender.h"
#ifndef NO_IMGUI
#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "imgui\dx9\imgui_impl_dx9.h"
#include "imgui\win32\imgui_impl_win32.h"
#endif

#ifdef _DEBUG
#include "CConsole.h"
#endif

using namespace std;

HMODULE g_module = nullptr;
HWND g_hwnd = nullptr;
WNDPROC g_wndproc = nullptr;
#ifndef NO_IMGUI
bool g_menu_opened = false;
#endif
bool g_orbwalker = true;
bool g_range = true;
bool g_2range_objmanager = false;
bool g_champ_info = true;
bool g_turret_range = true;
bool g_auto_evade = true;
bool g_zoom_hack = false;
bool g_spell_prediction = true;

bool g_bInit = false;

DWORD g_BaseAddr;
#ifndef NO_IMGUI
IDirect3DDevice9* myDevice;
#endif

typedef HRESULT(WINAPI* Prototype_Present)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
HRESULT WINAPI Hooked_Present(LPDIRECT3DDEVICE9 Device, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion);
Hvpp presentHvpp;

LRESULT WINAPI WndProc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

void __cdecl spoofedDrawCircle(Vector* position, float range, int* color, int a4, float a5, int a6, float alpha);

HRESULT WINAPI Hooked_Present(LPDIRECT3DDEVICE9 Device, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion)
{
	if (g_bInit == false) {
		g_BaseAddr = (DWORD)GetModuleHandle(NULL);

#ifdef _DEBUG
		//CConsole console;
#endif

		g_hwnd = FindWindowA(nullptr, nullptr);
		g_wndproc = WNDPROC(SetWindowLongA(g_hwnd, GWL_WNDPROC, LONG_PTR(WndProc)));

#ifndef NO_IMGUI
		Original_Reset = (Prototype_Reset)DetourFunc((PBYTE)GetDeviceAddress(16), (PBYTE)Hooked_Reset, 5);

		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX9_Shutdown();

		ImGui::DestroyContext(ImGui::GetCurrentContext());
#endif
	}

#ifndef NO_IMGUI
	myDevice = Device;
	DO_ONCE([&]()
		{
			render.init(Device);
		});

	ImGui::CreateContext();
	render.begin_draw();

	if (ImGui_ImplWin32_Init(g_hwnd))
	{
		if (ImGui_ImplDX9_Init(Device))
		{
			if (g_menu_opened)
			{
			}
		}
	}
#endif

#ifndef NO_IMGUI
	render.end_draw();
#endif

	exit:
	return ((Prototype_Present)(presentHvpp.GetOriginalAddress()))(Device, pSrcRect, pDestRect, hDestWindow, pDirtyRegion);
}

#ifndef NO_IMGUI
typedef HRESULT(WINAPI* Prototype_Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
Prototype_Reset Original_Reset;

HRESULT WINAPI Hooked_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	HRESULT result = Original_Reset(pDevice, pPresentationParameters);

	if (result >= 0)
		ImGui_ImplDX9_CreateDeviceObjects();

	return result;
}
#endif

#ifndef NO_IMGUI
LRESULT ImGui_ImplDX9_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto& io = ImGui::GetIO();

	switch (msg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		io.MouseDown[0] = true;
		return true;
	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		return true;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		io.MouseDown[1] = true;
		return true;
	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		return true;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		io.MouseDown[2] = true;
		return true;
	case WM_MBUTTONUP:
		io.MouseDown[2] = false;
		return true;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON1) == MK_XBUTTON1)
			io.MouseDown[3] = true;
		else if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON2) == MK_XBUTTON2)
			io.MouseDown[4] = true;
		return true;
	case WM_XBUTTONUP:
		if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON1) == MK_XBUTTON1)
			io.MouseDown[3] = false;
		else if ((GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON2) == MK_XBUTTON2)
			io.MouseDown[4] = false;
		return true;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return true;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		return true;
	case WM_KEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		return true;
	case WM_KEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		return true;
	case WM_CHAR:
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacter((unsigned short)wParam);
		return true;
	}

	return 0;
}
#endif

LRESULT WINAPI WndProc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	switch (u_msg)
	{
#ifndef NO_IMGUI
	case WM_KEYDOWN: {
		switch (w_param) {
		case VK_END: {
			g_menu_opened = !g_menu_opened;
			break;
		}
		default:
			break;
		};
		break;
	}
#endif
	case WM_MOUSEWHEEL: {
		//check if game menu opened
		break;
	}
	default:
		break;
	}

#ifndef NO_IMGUI
	if (g_menu_opened && ImGui_ImplDX9_WndProcHandler(hwnd, u_msg, w_param, l_param)) {
		return true;
	}
#endif

	return CallWindowProcA(g_wndproc, hwnd, u_msg, w_param, l_param);
}

void DllMainAttach(HMODULE hModule) {
	g_module = hModule;
	debug::init();

	if (!ColdHook_Service::ServiceGlobalInit(nullptr)) {
		debug::flog("Could not initialize ColdHook global service\n");
		return;
	}
	presentHvpp.Init((PVOID)GetD3D9VTableFunction(17), (PVOID)Hooked_Present);
	presentHvpp.Hook();
	presentHvpp.Hide();
}

void DllMainDetach() {
	presentHvpp.GlobalUnhide();
	ColdHook_Service::ServiceGlobalShutDown(true, nullptr);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID)
{
	if (hModule != nullptr)
		DisableThreadLibraryCalls(hModule);

	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DllMainAttach(hModule);
		return TRUE;
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		DllMainDetach();
		return TRUE;
	}

	return FALSE;
}