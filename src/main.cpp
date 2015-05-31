#include "sdk.h"
#include "vthook.h"
#include "netvars.h"
#include "xorstr.h"

IBaseClientDLL* clientdll;
IClientEntityList* entitylist;
IVEngineClient* engine;
IEngineTrace* enginetrace;
IVModelInfoClient* modelinfo;
IVModelRender* modelrender;
IVRenderView* render;
IViewRender* view;
IFileSystem* filesystem;

CInput* g_pInput;
CViewRender* g_pViewRender;
CGlobalVars* g_pGlobals;
DWORD g_dwGameResources = NULL;
ClientModeShared* g_pClientMode;
IMatSystemSurface* g_pMatSurface;
CGlowObjectManager* g_pGlowObjectManager = NULL;

CVMTHookManager* pEngineHook;
CVMTHookManager* pPanelHook;
CVMTHookManager* pSurfaceHook;
CVMTHookManager* pClientHook;
CVMTHookManager* pClientModeHook;
CVMTHookManager* pModelRenderHook;
CVMTHookManager* pInputHook;

tPaintTraverse oPaintTraverse;

CreateInterfaceFn CvarFactory = NULL;
CreateInterfaceFn EngineFactory = NULL;
CreateInterfaceFn ClientFactory = NULL;
CreateInterfaceFn VGUIFactory = NULL;
CreateInterfaceFn VGUI2Factory = NULL;
CreateInterfaceFn MatSysFactory = NULL;
CreateInterfaceFn StudioRenderFactory = NULL;
CreateInterfaceFn InputSystemFactory = NULL;
CreateInterfaceFn FileSysFactory = NULL;

bool name;
int screenWidth, screenHeight;

void DrawString(int x, int y, Color clr, vgui::HFont font, const char *pszText)
{
	if (pszText == NULL)
		return;
	wchar_t szString[1024] = { '\0' };
	wsprintfW(szString, L"%S", pszText);
	g_pMatSurface->DrawSetTextPos(x, y);
	g_pMatSurface->DrawSetTextFont(font);
	g_pMatSurface->DrawSetTextColor(clr);
	g_pMatSurface->DrawPrintText(szString, wcslen(szString));
}

bool WorldToScreen(Vector &vOrigin, Vector &vScreen)
{
	const VMatrix& worldToScreen = engine->WorldToScreenMatrix();

	float w = worldToScreen[3][0] * vOrigin[0] + worldToScreen[3][1] * vOrigin[1] + worldToScreen[3][2] * vOrigin[2] + worldToScreen[3][3];
	vScreen.z = 0;
	if (w > 0.01)
	{
		float inverseWidth = 1 / w;
		vScreen.x = (screenWidth / 2) + (0.5 * ((worldToScreen[0][0] * vOrigin[0] + worldToScreen[0][1] * vOrigin[1] + worldToScreen[0][2] * vOrigin[2] + worldToScreen[0][3]) * inverseWidth) * screenWidth + 0.5);
		vScreen.y = (screenHeight / 2) - (0.5 * ((worldToScreen[1][0] * vOrigin[0] + worldToScreen[1][1] * vOrigin[1] + worldToScreen[1][2] * vOrigin[2] + worldToScreen[1][3]) * inverseWidth) * screenHeight + 0.5);
		return true;
	}
	return false;
}

void __fastcall hkPaintTraverse(void* ecx, void* edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce)
{
	oPaintTraverse(ecx, vguiPanel, forceRepaint, allowForce);

	static unsigned int MatSystemTopPanel;
	static vgui::HFont s_HFontPlayer;

	if (!MatSystemTopPanel)
	{
		const char* szName = ipanel()->GetName(vguiPanel);
		g_pCVar->ConsoleColorPrintf(Color::Blue(), "panel: %s\n", szName);

		if (_V_stricmp(szName, "MatSystemTopPanel") == 0) // MatSystemTopPanel
		{
			MatSystemTopPanel = vguiPanel;

			engine->GetScreenSize(screenWidth, screenHeight);
			s_HFontPlayer = g_pMatSurface->CreateFont();
			g_pMatSurface->SetFontGlyphSet(s_HFontPlayer, "Tahoma", 14, 150, 0, 0, FONTFLAG_OUTLINE);
		}
	}
	

	if (MatSystemTopPanel == vguiPanel)
	{
		if (GetAsyncKeyState(VK_F9) & 1)
		{
			name = !name;
			if (name) Beep(0x367, 200);
			else Beep(0x255, 200);
		}

		if (engine->IsInGame() && engine->IsConnected() && !engine->IsTakingScreenshot())
		{

			if (GetAsyncKeyState(VK_F11) & 1)
			{
				g_pCVar->ConsoleColorPrintf(Color::Red(), "Hello World!");
			}

			C_BaseEntity *pLocalEntity = (C_BaseEntity*)entitylist->GetClientEntity(engine->GetLocalPlayer());
			if (!pLocalEntity)
				return;


			for (int i = 0; i < entitylist->GetHighestEntityIndex(); i++)
			{
				C_BaseEntity* pBaseEntity = (C_BaseEntity*)entitylist->GetClientEntity(i);
				if (!pBaseEntity)
					continue;
				if (pBaseEntity->health() < 1)
					continue;
				if (pBaseEntity == pLocalEntity)
					continue;
				if (pLocalEntity->team() == pBaseEntity->team())
					continue;

				if (name)
				{
					Vector out;
					if (WorldToScreen(pBaseEntity->GetAbsOrigin(), out))
					{
						if (name)
						{
							player_info_t info;
							engine->GetPlayerInfo(i, &info);
							DrawString(out.x - 5, out.y, Color::Red(), s_HFontPlayer, info.name);
						}
					}
				}
			}
		}
	}
}

//--------------------------------------Hook-Thread-------------------------------------------
DWORD WINAPI Init(LPVOID lpArguments)
{
	offys.dwClientBase = (DWORD)GetModuleHandle(/*client.dll*/XorStr<0x2B, 11, 0x384A3344>("\x48\x40\x44\x4B\x41\x44\x1F\x56\x5F\x58" + 0x384A3344).s);
	offys.dwEngineBase = (DWORD)GetModuleHandle(/*engine.dll*/XorStr<0x07, 11, 0x2B15D8E1>("\x62\x66\x6E\x63\x65\x69\x23\x6A\x63\x7C" + 0x2B15D8E1).s);
	offys.dwFileSysBase = (DWORD)GetModuleHandle(/*filesystem_stdio.dll*/XorStr<0x9E, 21, 0x6A2B3D96>("\xF8\xF6\xCC\xC4\xD1\xDA\xD7\xD1\xC3\xCA\xF7\xDA\xDE\xCF\xC5\xC2\x80\xCB\xDC\xDD" + 0x6A2B3D96).s);
	offys.dwMatSysBase = (DWORD)GetModuleHandle(/*materialsystem.dll*/XorStr<0xC0,19,0x30C978B4>("\xAD\xA0\xB6\xA6\xB6\xAC\xA7\xAB\xBB\xB0\xB9\xBF\xA9\xA0\xE0\xAB\xBC\xBD"+0x30C978B4).s);

	EngineFactory = (CreateInterfaceFn)GetProcAddress((HMODULE)offys.dwEngineBase, CREATEINTERFACE_PROCNAME);
	ClientFactory = (CreateInterfaceFn)GetProcAddress((HMODULE)offys.dwClientBase, CREATEINTERFACE_PROCNAME);
	FileSysFactory = (CreateInterfaceFn)GetProcAddress((HMODULE)offys.dwFileSysBase, CREATEINTERFACE_PROCNAME);
	VGUIFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandle(/*vguimatsurface.dll*/XorStr<0xDB, 19, 0xAD9314E4>("\xAD\xBB\xA8\xB7\xB2\x81\x95\x91\x96\x96\x83\x87\x84\x8D\xC7\x8E\x87\x80" + 0xAD9314E4).s), CREATEINTERFACE_PROCNAME);
	VGUI2Factory = (CreateInterfaceFn)GetProcAddress(GetModuleHandle(/*vgui2.dll*/XorStr<0x95, 10, 0x16ABE336>("\xE3\xF1\xE2\xF1\xAB\xB4\xFF\xF0\xF1" + 0x16ABE336).s), CREATEINTERFACE_PROCNAME);
	MatSysFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandle(/*materialsystem.dll*/XorStr<0x72, 19, 0xBC96F5ED>("\x1F\x12\x00\x10\x04\x1E\x19\x15\x09\x02\x0F\x09\x1B\x12\xAE\xE5\xEE\xEF" + 0xBC96F5ED).s), CREATEINTERFACE_PROCNAME);
	StudioRenderFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandle(/*studiorender.dll*/XorStr<0xE9, 17, 0x6F9CA1E1>("\x9A\x9E\x9E\x88\x84\x81\x9D\x95\x9F\x96\x96\x86\xDB\x92\x9B\x94" + 0x6F9CA1E1).s), CREATEINTERFACE_PROCNAME);
	InputSystemFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandle(/*inputsystem.dll*/XorStr<0x9D,16,0x796EA5D5>("\xF4\xF0\xEF\xD5\xD5\xD1\xDA\xD7\xD1\xC3\xCA\x86\xCD\xC6\xC7"+0x796EA5D5).s), CREATEINTERFACE_PROCNAME);
	CvarFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandle(/*vstdlib.dll*/XorStr<0x50,12,0xBDFEE038>("\x26\x22\x26\x37\x38\x3C\x34\x79\x3C\x35\x36"+0xBDFEE038).s), CREATEINTERFACE_PROCNAME);

	engine = (IVEngineClient*)EngineFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	clientdll = (IBaseClientDLL*)ClientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);
	enginetrace = (IEngineTrace*)EngineFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);
	modelinfo = (IVModelInfoClient*)EngineFactory(VMODELINFO_CLIENT_INTERFACE_VERSION, NULL);
	entitylist = (IClientEntityList*)ClientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);
	g_pVGuiSurface = (vgui::ISurface*)VGUIFactory(VGUI_SURFACE_INTERFACE_VERSION, NULL);
	g_pMatSurface = (IMatSystemSurface*)g_pVGuiSurface->QueryInterface(MAT_SYSTEM_SURFACE_INTERFACE_VERSION);
	g_pVGuiPanel = (vgui::IPanel*)VGUI2Factory(VGUI_PANEL_INTERFACE_VERSION, NULL);
	g_pVGuiInput = (vgui::IInput*)VGUI2Factory(VGUI_INPUT_INTERFACE_VERSION, NULL);
	g_pVGui = (vgui::IVGui*)VGUI2Factory(VGUI_IVGUI_INTERFACE_VERSION, NULL);
	g_pVGuiSchemeManager = (vgui::ISchemeManager*)VGUI2Factory(VGUI_SCHEME_INTERFACE_VERSION, NULL);
	g_pVGuiSystem = (vgui::ISystem*)VGUI2Factory(VGUI_SYSTEM_INTERFACE_VERSION, NULL);
	g_pInputSystem = (IInputSystem*)InputSystemFactory(INPUTSYSTEM_INTERFACE_VERSION, NULL);
	filesystem = (IFileSystem*)FileSysFactory(BASEFILESYSTEM_INTERFACE_VERSION, NULL);
	g_pCVar = (ICvar*)CvarFactory(CVAR_INTERFACE_VERSION, NULL);

	materials = (IMaterialSystem*)MatSysFactory(MATERIAL_SYSTEM_INTERFACE_VERSION, NULL);
	studiorender = g_pStudioRender = (IStudioRender*)StudioRenderFactory(STUDIO_RENDER_INTERFACE_VERSION, NULL);
	modelrender = (IVModelRender*)EngineFactory(VENGINE_HUDMODEL_INTERFACE_VERSION, NULL);
	render = (IVRenderView*)EngineFactory(VENGINE_RENDERVIEW_INTERFACE_VERSION, NULL);


	g_pCVar->ConsoleColorPrintf(Color::Purple(), "-------------------------------\n"
												 " dude719 CSGO SDK Example 2015\n"
												 "-------------------------------\n");

	pPanelHook = new CVMTHookManager((PDWORD*)g_pVGuiPanel);
	oPaintTraverse = (tPaintTraverse)pPanelHook->dwHookMethod((DWORD)hkPaintTraverse, 41);

	g_pNetVars = new CNetVars();
	offys.dwEyePosOffset = (DWORD)g_pNetVars->GetOffset("DT_BasePlayer", "m_vecViewOffset[0]");
	offys.dwVecPunchAngles = (DWORD)g_pNetVars->GetOffset("DT_BasePlayer", "m_aimPunchAngle");
	offys.dwLocal = (DWORD)g_pNetVars->GetOffset("DT_BasePlayer", "m_Local");
	offys.dwhActiveWeapon = (DWORD)g_pNetVars->GetOffset("DT_BaseCombatCharacter", "m_hActiveWeapon");
	offys.dwbIsScoped = (DWORD)g_pNetVars->GetOffset("DT_CSPlayer", "m_bIsScoped");
	offys.dwPlayerState = (DWORD)g_pNetVars->GetOffset("DT_BasePlayer", "pl");
	offys.dwEyeAngles = (DWORD)g_pNetVars->GetOffset("DT_CSPlayer", "m_angEyeAngles");
	offys.dwNextPrimaryAttack = (DWORD)g_pNetVars->GetOffset("DT_BaseCombatWeapon", "m_flNextPrimaryAttack");
	offys.dwTickBase = (DWORD)g_pNetVars->GetOffset("DT_BasePlayer", "m_nTickBase");

	return 0;
}

//	Main Function
DWORD WINAPI DllMain(HMODULE hDll, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Init, NULL, 0, NULL);
		return 1;
	}
	else if (dwReasonForCall == DLL_PROCESS_DETACH)
	{
		pPanelHook->UnHook();
		return 1;
	}
	return 0;
}
