#include "hooks.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"

#include "minhook/minhook.h"

#include "loader_gui.h"

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include <mutex>

#include "utils.h"

class i_surface
{
public:
	void unlock_cursor()
	{
		using fn = void(__thiscall*)(i_surface*);
		return (*(fn**)this)[61](this);
	}
};

using end_scene_fn = long(__stdcall*)(IDirect3DDevice9*);
using reset_fn = long(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using lock_cursor_fn = void(__thiscall*)(i_surface*); //62

lock_cursor_fn lock_cursor_original = nullptr;
end_scene_fn end_scene_original = nullptr;
reset_fn reset_original = nullptr;
WNDPROC wndproc_original = nullptr;

i_surface* surface = nullptr;

void lock_cursor_hook(i_surface* self)
{
	if (loader_gui::is_gui_open)
		self->unlock_cursor();
	else
		lock_cursor_original(self);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT STDMETHODCALLTYPE hooked_wndproc(HWND window, UINT message_type, WPARAM w_param, LPARAM l_param)
{
	if (message_type == WM_KEYDOWN)
		if (w_param == VK_HOME)
			loader_gui::is_gui_open = !loader_gui::is_gui_open;
	
	if (ImGui_ImplWin32_WndProcHandler(window, message_type, w_param, l_param) && loader_gui::is_gui_open)
		return true;

	return CallWindowProc(wndproc_original, window, message_type, w_param, l_param);
}

long __stdcall end_scene_hook(IDirect3DDevice9* device)
{
	static std::once_flag f;
	std::call_once(f, [&]()
	{
		ImGui::CreateContext();
		auto* const game_hwnd = FindWindowW(L"Valve001", nullptr);
		ImGui_ImplWin32_Init(game_hwnd);
		ImGui_ImplDX9_Init(device);
		ImGui::GetIO().IniFilename = nullptr;
	});
	
	IDirect3DStateBlock9* pixel_state = NULL;
	IDirect3DVertexDeclaration9* vertex_declaration;
	IDirect3DVertexShader9* vertex_shader;
	device->CreateStateBlock(D3DSBT_PIXELSTATE, &pixel_state);
	device->GetVertexDeclaration(&vertex_declaration);
	device->GetVertexShader(&vertex_shader);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);

	DWORD state1;
	DWORD state2;
	DWORD state3;
	device->GetRenderState(D3DRS_COLORWRITEENABLE, &state1);
	device->GetRenderState(D3DRS_COLORWRITEENABLE, &state2);
	device->GetRenderState(D3DRS_SRGBWRITEENABLE, &state3);

	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	loader_gui::draw();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	device->SetRenderState(D3DRS_COLORWRITEENABLE, state1);
	device->SetRenderState(D3DRS_COLORWRITEENABLE, state2);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, state3);

	pixel_state->Apply();
	pixel_state->Release();
	device->SetVertexDeclaration(vertex_declaration);
	device->SetVertexShader(vertex_shader);

	return end_scene_original(device);
}

long __stdcall reset_hook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_parameters)
{
	const auto ret = reset_original(device, present_parameters);
	ImGui_ImplDX9_InvalidateDeviceObjects();

	if (ret > 0)
		ImGui_ImplDX9_CreateDeviceObjects();

	return ret;
}

IDirect3DDevice9* get_device()
{
	auto* const game_hwnd = FindWindowW(L"Valve001", nullptr);
	auto* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d9) return nullptr;

	D3DPRESENT_PARAMETERS d3dpp {};
	d3dpp.hDeviceWindow = game_hwnd;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed = TRUE;

	IDirect3DDevice9* device9 = nullptr;
	if (FAILED(d3d9->CreateDevice(0, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device9)))
	{
		d3d9->Release();
		return nullptr;
	}

	return device9;
}

typedef void* (*abstract_interface)(const char* name, int* return_code);
i_surface* get_surface()
{
	auto create_interface = (abstract_interface)GetProcAddress(GetModuleHandleA("vguimatsurface.dll"), "CreateInterface");
	return (i_surface*)create_interface("VGUI_Surface030", 0);
}

inline unsigned int get_virtual(void* _class, const unsigned int index) { return static_cast<unsigned int>((*static_cast<int**>(_class))[index]); }

void hooks::init_hooks()
{
	MH_Initialize();

	auto* const game_hwnd = FindWindowW(L"Valve001", nullptr);
	wndproc_original = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
		game_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hooked_wndproc)));

	auto* device = get_device();
	void** device_vtable = *reinterpret_cast<void***>(device);
	if (device)
		device->Release(), device = nullptr;
	
	MH_CreateHook(device_vtable[42], (void*)end_scene_hook, (void**)&end_scene_original);
	MH_CreateHook(device_vtable[16], (void*)reset_hook, (void**)&reset_original);

	surface = get_surface();
	void** vfn = *reinterpret_cast<void***>(surface);//get_virtual(surface, 62)  ;
	if (surface)
		surface = nullptr;
	MH_CreateHook(vfn[62], (void*)lock_cursor_hook, (void**)&lock_cursor_original);
	
	MH_EnableHook(MH_ALL_HOOKS);
}

void hooks::shutdown_hooks()
{
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}
