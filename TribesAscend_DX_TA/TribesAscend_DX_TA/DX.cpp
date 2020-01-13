#include <windows.h>
#pragma comment(lib,"winmm.lib")

#include <string>
#include <iostream>

#include <ctime>
#include "DX.h"
//#include "Memory.h"

#define USE_QUERY_TIMER
//#define USE_TIMEGETTIME_TIMER

#define USE_IMGUI
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

/*
//#define USE_UEUTILITIES
#ifdef USE_UEUTILITIES
#include "Keys.h"
using namespace UE_Utilities;
#endif
*/

#define TEXTURE_STAGES 8

#define DEV
#define RDEV
#define CONSOLE
//#define DANGER
#define NOTREADY

#define PRINT(x) std::cout << x << std::endl
#define P PRINT
//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) std::cout << x << std::endl
#define DP DEBUG_PRINT
#endif

#define LOG

using namespace std;

#ifdef RDEV
bool b_developer = false;
#endif

namespace DX {
	IDirect3DBaseTexture9* setTextureTexture[TEXTURE_STAGES] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	int i_textureStages = 0, i_textureStagesPrev = 0;

	LPDIRECT3DTEXTURE9 transparentTexture = NULL;
	bool b_isSniping = false;
	bool b_isSnipingFlag = false;
	bool b_draw = true;

	bool b_showMenu = true;

	bool b_disableZoomBars = false;
	bool b_minimalLODTextures_DIP = false;
	bool b_minimalLODTextures_ST = false;
	//bool b_minimalLODTexturesStages_DIP[TEXTURE_STAGES] = { true, true, true, true, true, true, true, true };
	bool b_minimalLODTexturesStages_DIP[TEXTURE_STAGES] = { true, true, true, true, false, true, true, true };
	int i_LODTexturesStages_DIP[TEXTURE_STAGES] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int i_LODTexturesStages_ST[TEXTURE_STAGES] = { -1, -1, -1, -1, -1, -1, -1, -1 };

	//bool b_minimalLODTexturesStages_DIP[TEXTURE_STAGES] = { true, true, false, true, false, true, true, true };
	//bool b_minimalLODTexturesStages_ST[TEXTURE_STAGES] = { true, true, false, false, false, true, true, true };
	bool b_minimalLODTexturesStages_ST[TEXTURE_STAGES] = { true, true, false, false, false, true, true, true };

	bool b_textureReset = false;
	int i_defaultLOD = 0;
	int i_LODstart = 0;
	int i_LODend = 100000;
	bool b_preserveChainBullets = true;
	bool b_keepWeaponsAndModels = false;

	bool b_resetLOD = false;
	bool b_resetLODflag = false;

	int frameCount = 0;

	bool b_limitFPS = false;
	int i_fpsLimit = 200;
	double d_fpsDelay = 1000.0 / i_fpsLimit;
	unsigned long long i_previousFrameTime = 0;
	LARGE_INTEGER li_previousFrameTime;
	DWORD i_currentTIme = 0;
	DWORD i_prevTime = 0;
}

namespace DX {
	/*
	void toggleDraw(void) {
		b_draw = !b_draw;
	}
	*/
}

namespace DX {
	HWND hWnd;
	WNDPROC oWndProc;
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LPDIRECT3DDEVICE9 mainDevice = NULL;

	JumpHook* endSceneJumpHook = NULL;

	VMTHook* beginSceneVMTHook = NULL;
	VMTHook* endSceneVMTHook = NULL;
	VMTHook* resetVMTHook = NULL;

	VMTHook* drawIndexedPrimitiveVMTHook = NULL;
	VMTHook* drawPrimitiveVMTHook = NULL;
	VMTHook* drawIndexedPrimitiveUPVMTHook = NULL;

	VMTHook* setTextureVMTHook = NULL;

	bool hook(void) {

#ifdef CONSOLE
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		printStartMessage();
#endif

		bool res = createDevice();
		while (!res) {
			res = createDevice();
			if (!res)
				Sleep(100);
		}

		//QueryPerformanceCounter(&li_previousFrameTime);
		li_previousFrameTime.QuadPart = 0;
		timeBeginPeriod(0);

		return res;
	}

	bool createDevice(void) {
		LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (!pD3D)
			return false;

		hWnd = FindWindow(NULL, L"Tribes: Ascend (32-bit, DX9)");

		if (hWnd == NULL) {
			return false;
		}

		//#ifdef USE_IMGUI
		oWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWL_WNDPROC, (LONG_PTR)WndProc);
		//#endif

		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = hWnd;
		LPDIRECT3DDEVICE9 device;
		HRESULT res = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
		if (FAILED(res))
			return false;

		DWORD endSceneAddress = VMTHook::getFunctionInstruction(device, 42);
		endSceneJumpHook = new JumpHook(endSceneAddress, (DWORD)endSceneGetDevice, pre);
		pD3D->Release();
		device->Release();
		return true;
	}

	HRESULT __stdcall endSceneGetDevice(LPDIRECT3DDEVICE9 device) {
		mainDevice = device;

		endSceneJumpHook->unhook();

		initHooks();

#ifdef USE_IMGUI
		ImGui_setup();
#endif

		D3DXCreateTextureFromFile(device, L"transparent.png", &transparentTexture);

		HRESULT res = ((endScene)endSceneJumpHook->getHookAddress())(device);
		return res;
	}

	HRESULT __stdcall beginSceneHook(LPDIRECT3DDEVICE9 device) {
		b_isSnipingFlag = false;
		//b_isSniping = false;

		if (b_resetLODflag) {
			b_resetLOD = true;
			b_resetLODflag = false;
		}

		HRESULT res = ((beginScene)beginSceneVMTHook->getOriginalFunction())(device);
		return res;
	}

	HRESULT __stdcall endSceneHook(LPDIRECT3DDEVICE9 device) {
		/*
		#ifdef USE_UEUTILITIES
				keyManager.checkKeyStates();
		#endif
		*/
		if (b_resetLOD) {
			frameCount = (frameCount + 1) % 3;
			if (frameCount == 0) {
				b_resetLOD = false;
			}
		}

		if (b_isSnipingFlag) {
			b_isSniping = true;
		}
		else {
			b_isSniping = false;
		}

		//b_minimalLODTextures_DIP = false;

#ifdef USE_IMGUI
		ImGui_render();
#endif

		// So calling font->DrawText for some reason resets
		// the VMT of the device, so we need to rehook endScene
		// Because the VMT is not reset by us, then hooked -> true
		// so we need to disable the hooked check, or set hooked
		// to false after font->DrawText
		endSceneVMTHook->hook();
		drawIndexedPrimitiveVMTHook->hook();
		drawPrimitiveVMTHook->hook();
		drawIndexedPrimitiveUPVMTHook->hook();
		resetVMTHook->hook();
		setTextureVMTHook->hook();
		HRESULT res = ((endScene)endSceneVMTHook->getOriginalFunction())(device);

		static unsigned long long currentTime = GetTickCount64();
		static LARGE_INTEGER QPF;
		QueryPerformanceFrequency(&QPF);

		double timeElapsedTick = 0;


#ifdef USE_QUERY_TIMER
		if (b_limitFPS) {
			static LARGE_INTEGER QPC;
			QueryPerformanceCounter(&QPC);
			unsigned long long deltaCounter = QPC.QuadPart - li_previousFrameTime.QuadPart;
			timeElapsedTick = deltaCounter / (QPF.QuadPart * 1.0);// *1000;
			while (timeElapsedTick < (d_fpsDelay/1000)) {
				//Sleep(1);
				QueryPerformanceCounter(&QPC);
				deltaCounter = QPC.QuadPart - li_previousFrameTime.QuadPart;
				timeElapsedTick = deltaCounter / (QPF.QuadPart * 1.0);
				//timeElapsedTick = ((QPC.QuadPart - li_previousFrameTime.QuadPart) / (QPF.QuadPart *1.0)) *1000;
			}
			li_previousFrameTime = QPC;
		}
#endif


#ifdef USE_TIMEGETTIME_TIMER
		if (b_limitFPS) {
			i_currentTIme = timeGetTime();
			DWORD timeElapsed = i_currentTIme - i_prevTime;
			while (timeElapsed < d_fpsDelay) {
				i_currentTIme = timeGetTime();
				timeElapsed = i_currentTIme - i_prevTime;
			}
			i_prevTime = i_currentTIme;
		}
#endif



		return res;
	}

	HRESULT __stdcall resetHook(LPDIRECT3DDEVICE9 device, D3DPRESENT_PARAMETERS* pPresentationParameters) {

		//b_minimalLODTextures_DIP = false;
		//b_minimalLODTextures_ST = false;

#ifdef USE_IMGUI
		ImGui_ImplDX9_InvalidateDeviceObjects();
#endif

		HRESULT res = ((reset)resetVMTHook->getOriginalFunction())(device, pPresentationParameters);

#ifdef USE_IMGUI
		ImGui_ImplDX9_CreateDeviceObjects();
#endif

		return res;
	}

	HRESULT __stdcall drawIndexedPrimitiveHook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE primType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {

#ifdef DEBUG
		cout << "drawIndexedPrimitiveHook" << endl;
#endif

		//IDirect3DBaseTexture9* pTexture;
		//bool b = device->GetTexture(0, &pTexture) == D3D_OK;


		if (!b_draw) {
			return D3D_OK;
		}
		//11944->Phase
		//9658->BXT
		if (NumVertices == 11944 || NumVertices == 9658) {
			b_isSnipingFlag = true;
		}

		if (b_resetLOD) {
			for (int i = 0; i <= TEXTURE_STAGES; i++) {
				IDirect3DBaseTexture9* pTexture;
				bool b = device->GetTexture(i, &pTexture) == D3D_OK;
				if (!b || !pTexture) {
					continue;
				}
				pTexture->SetLOD(0);
			}
			HRESULT res = ((drawIndexedPrimitive)drawIndexedPrimitiveVMTHook->getOriginalFunction())(device, primType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
			return res;
		}

		//i_textureStages /*TEXTURE_STAGES*/
		if (b_minimalLODTextures_DIP && NumVertices >= i_LODstart && NumVertices <= i_LODend) {
			for (int i = 0; i < i_textureStages; i++) {
				if (!b_minimalLODTexturesStages_DIP[i])
					continue;

				if (b_preserveChainBullets) {
					int stage = i;
					if (stage == 4) {
						// 3292 is turret with shield pack?
						if (NumVertices < 205 || (NumVertices >= 3292 && NumVertices <= 3294))
							continue;
					}

					if (stage == 2) {
						if (NumVertices <= 70) {
							// 50 is chain/disc
							// 70 is eagle
							continue;
						}
					}

					if (i == 2 || i == 3 || i == 4) {
						if (NumVertices < 150)
							continue;
					}
				}

				IDirect3DBaseTexture9* pTexture;
				bool b = true;
				if (b_minimalLODTextures_DIP && false) {
					b = device->GetTexture(i, &pTexture) == D3D_OK; //DIP
				}
				else {
					pTexture = setTextureTexture[i]; //DIP+ST
				}

				if (!b || !pTexture) {
					continue;
				}

				int lod = pTexture->GetLOD();
				int level = pTexture->GetLevelCount();
				if (i_LODTexturesStages_DIP[i] == -1 || i_LODTexturesStages_DIP[i] >= level) {
					if (lod != level - 1) {
						pTexture->SetLOD(level - 1);
					}
					else {

					}
				}
				else {
					if (lod != i_LODTexturesStages_DIP[i] && i_LODTexturesStages_DIP[i] < level - 1)
						pTexture->SetLOD(i_LODTexturesStages_DIP[i]);
				}

			}
		}
		else {
			if (b_minimalLODTextures_DIP) {
				//return D3D_OK;
			}
		}

		HRESULT res = ((drawIndexedPrimitive)drawIndexedPrimitiveVMTHook->getOriginalFunction())(device, primType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		return res;
	}

	HRESULT __stdcall drawPrimitiveHook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
		if (!b_draw) {
			return D3D_OK;
		}

		if (b_disableZoomBars && PrimitiveCount == 2 && PrimitiveType == 5 && StartVertex == 0 /*&& !b_isSniping*/) {
			bool stateSuccess = device->SetTexture(1, transparentTexture) == D3D_OK;

		}
		HRESULT res = ((drawPrimitive)drawPrimitiveVMTHook->getOriginalFunction())(mainDevice, PrimitiveType, StartVertex, PrimitiveCount);
		return res;
	}

	HRESULT __stdcall drawIndexedPrimitiveUPHook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
		if (!b_draw) {
			return D3D_OK;
		}

		// bxt = 512
		// phase = 822

		if (NumVertices == 512 || (NumVertices >= 1991 && NumVertices <= 1991)) {
			b_isSnipingFlag = true;
		}
		HRESULT res = ((drawIndexedPrimitiveUP)drawIndexedPrimitiveUPVMTHook->getOriginalFunction())(device, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
		return res;
	}

	HRESULT __stdcall setTextureHook(LPDIRECT3DDEVICE9 device, DWORD Stage, IDirect3DBaseTexture9* pTexture) {
		HRESULT res;

#ifdef DEBUG
		cout << "setTextureHook: Stage = " << Stage << endl;
#endif

		if (Stage <= i_textureStagesPrev && Stage < TEXTURE_STAGES) {
			i_textureStages = i_textureStagesPrev;
		}

		if (Stage < TEXTURE_STAGES) {
			i_textureStagesPrev = Stage;
		}

		if (Stage < TEXTURE_STAGES)
			setTextureTexture[Stage] = pTexture;

		if (b_minimalLODTextures_ST) {
			if (pTexture) {
				if (b_preserveChainBullets) {
					if (!b_minimalLODTexturesStages_ST[Stage]) {
						res = ((setTexture)(setTextureVMTHook->getOriginalFunction()))(device, Stage, pTexture);
						return res;
					}
				}

				int lod = pTexture->GetLOD();
				int level = pTexture->GetLevelCount();

				if (frameCount == 0 && (Stage == 1 || Stage == 2 || Stage == 3 || Stage == 4) && false) {
					pTexture->SetLOD(0);
				}
				else {
					if (i_LODTexturesStages_ST[Stage] == -1) {
						if (lod != level - 1) {
							pTexture->SetLOD(level - 1);
						}
						else {

						}
					}
					else {
						pTexture->SetLOD(i_LODTexturesStages_ST[Stage]);
					}
				}
			}
		}


		res = ((setTexture)(setTextureVMTHook->getOriginalFunction()))(device, Stage, pTexture);
		return res;
	}


	void initHooks(void) {
		beginSceneVMTHook = new VMTHook(mainDevice, 41, (DWORD)beginSceneHook, pre);
		endSceneVMTHook = new VMTHook(mainDevice, 42, (DWORD)endSceneHook, pre);
		resetVMTHook = new VMTHook(mainDevice, 16, (DWORD)resetHook, pre);
		drawIndexedPrimitiveVMTHook = new VMTHook(mainDevice, 82, (DWORD)drawIndexedPrimitiveHook, pre);
		drawPrimitiveVMTHook = new VMTHook(mainDevice, 81, (DWORD)drawPrimitiveHook, pre);
		drawIndexedPrimitiveUPVMTHook = new VMTHook(mainDevice, 84, (DWORD)drawIndexedPrimitiveUPHook, pre);
		setTextureVMTHook = new VMTHook(mainDevice, 65, (DWORD)setTextureHook, pre);
	}
}

#ifdef USE_IMGUI
namespace DX {
	void ImGui_setup(void) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.WantCaptureMouse = true;
		io.MouseDrawCursor = true;
		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX9_Init(mainDevice);

	}

	void ImGui_render(void) {
		if (b_showMenu) {
			ImGui_ImplDX9_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			/*
			static bool b = true;
			if (true) {
				::ImGui::ShowDemoWindow(&b);
			}
			*/

			{
				ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

				static float f = 0.0f;
				static int counter = 0;

				ImGui::Begin("TribesAscend_DX_TA V1.0.1");

				ImGui::BeginTabBar("TabBar", tab_bar_flags);
				{
					static char str[40] = "";
					if (ImGui::BeginTabItem("Visuals")) {
#ifdef RDEV
						ImGui::Checkbox("Developer mode", &b_developer);
						ImGui::Separator();
#endif
						ImGui::Checkbox("Disable zoom bars", &b_disableZoomBars);
						ImGui::Separator();

						if (ImGui::Checkbox("FPS limiting", &b_limitFPS)) {
							//QueryPerformanceCounter(&li_previousFrameTime);
						}

						if (ImGui::SliderInt("FPS", &i_fpsLimit, 30, 1000)) {
							d_fpsDelay = 1000.0 / i_fpsLimit;
							//QueryPerformanceCounter(&li_previousFrameTime);
						}

						if (ImGui::CollapsingHeader("LOD changer (notexturestreaming)")) {
#ifdef DANGER
							if (ImGui::CollapsingHeader("[DANGEROUS] DrawIndexedPrimitive method")) {
								if (ImGui::Checkbox("Enable [WARNING - Causes freeze on alt+tab]##DIP", &b_minimalLODTextures_DIP)) {
									if (!b_minimalLODTextures_DIP) {
										b_resetLODflag = true;
									}
								}
#ifdef RDEV
								if (b_developer) {
									static bool customizeStages = false;
									if (ImGui::Checkbox("Edit stages##DIP", &customizeStages)) {
										if (!customizeStages) {
											for (int i = 0; i < TEXTURE_STAGES; i++) {
												i_LODTexturesStages_DIP[i] = -1;
											}
										}
									}
									if (customizeStages) {
										bool stageChanged = false;
										for (int i = 0; i < TEXTURE_STAGES; i++) {
											string s = to_string(i);
											strcpy(str, "Stage ");
											strcat(str, s.c_str());
											strcat(str, "##DIP");
											stageChanged = stageChanged || (ImGui::Checkbox(str, &b_minimalLODTexturesStages_DIP[i]) && b_minimalLODTextures_DIP);
											ImGui::SameLine();
											strcpy(str, "LOD value##DIP");
											strcat(str, s.c_str());
											ImGui::SliderInt(str, &i_LODTexturesStages_DIP[i], -1, 20);
										}
										if (stageChanged) {
											b_resetLODflag = true;
										}
									}
								}
#endif
							}
#endif
							if (ImGui::CollapsingHeader("DIP+ST method")) {
								if (ImGui::Checkbox("Enable##DIPST", &b_minimalLODTextures_DIP)) {
									if (!b_minimalLODTextures_DIP) {
										b_resetLODflag = true;
									}
								}
#ifdef RDEV
								if (b_developer) {
									static bool customizeStages = false;
									if (ImGui::Checkbox("Edit stages##DIPST", &customizeStages)) {
										if (!customizeStages) {
											for (int i = 0; i < TEXTURE_STAGES; i++) {
												i_LODTexturesStages_DIP[i] = -1;
											}
										}
									}
									if (customizeStages) {
										bool stageChanged = false;
										for (int i = 0; i < TEXTURE_STAGES; i++) {
											string s = to_string(i);
											strcpy(str, "Stage ");
											strcat(str, s.c_str());
											strcat(str, "##DIP");
											stageChanged = stageChanged || (ImGui::Checkbox(str, &b_minimalLODTexturesStages_DIP[i]) && b_minimalLODTextures_DIP);
											ImGui::SameLine();
											strcpy(str, "LOD value##DIPST");
											strcat(str, s.c_str());
											ImGui::SliderInt(str, &i_LODTexturesStages_DIP[i], -1, 20);
										}
										if (stageChanged) {
											b_resetLODflag = true;
										}
									}
								}
#endif
							}

#ifdef USE_SETTEXTURE_LODMETHOD
							if (ImGui::CollapsingHeader("SetTexture method")) {
								if (ImGui::Checkbox("Enable##ST", &b_minimalLODTextures_ST)) {
									if (!b_minimalLODTextures_ST) {
										b_resetLODflag = true;
									}
								}
#ifdef RDEV
								if (b_developer) {
									static bool customizeStages = false;
									if (ImGui::Checkbox("Edit stages##ST", &customizeStages)) {
										if (!customizeStages) {
											for (int i = 0; i < TEXTURE_STAGES; i++) {
												i_LODTexturesStages_ST[i] = -1;
											}
										}
									}
									if (customizeStages) {
										bool stageChanged = false;
										for (int i = 0; i < TEXTURE_STAGES; i++) {
											string s = to_string(i);
											strcpy(str, "Stage ");
											strcat(str, s.c_str());
											strcat(str, "##ST");
											stageChanged = stageChanged || (ImGui::Checkbox(str, &b_minimalLODTexturesStages_ST[i]) && b_minimalLODTextures_ST);
											ImGui::SameLine();
											strcpy(str, "LOD value##ST");
											strcat(str, s.c_str());
											ImGui::SliderInt(str, &i_LODTexturesStages_ST[i], -1, 20);
										}
										if (stageChanged) {
											b_resetLODflag = true;
										}
									}

								}
#endif

								/*
								if (ImGui::Checkbox("Apply to guns and player models", &b_keepWeaponsAndModels)) {
									if (!b_keepWeaponsAndModels) {
										b_minimalLODTexturesStages_ST[1] = false;
									}
									else {
										b_minimalLODTexturesStages_ST[1] = true;
									}
								}
								*/
							}

							//ImGui::Separator();
							//ImGui::Checkbox("Preserve chain bullets (not squares)", &b_preserveChainBullets);
#endif
							ImGui::Separator();
							if (ImGui::Button("Reset LODs")) {
								b_resetLODflag = true;
								b_minimalLODTextures_ST = false;
								b_minimalLODTextures_DIP = false;
							}
#ifdef DEV
							ImGui::Separator();
							ImGui::SliderInt("NumVert Lower bound", &i_LODstart, 0, 5000);
							ImGui::SliderInt("NumVert Upper bound", &i_LODend, 0, 20000);
#endif
#ifdef DEV
							/*
							ImGui::Separator();
							for (int i = 0; i < TEXTURE_STAGES; i++) {
								string s = to_string(i);
								strcpy(str, "Stage ");
								strcat(str, s.c_str());
								ImGui::Checkbox(str, &b_minimalLODTexturesStages_ST[i]);
							}
							*/
#endif
						}

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Themes")) {
						static int style_idx = 1;
						if (ImGui::Combo("Theme", &style_idx, "Classic\0Dark\0Light\0")) {
							switch (style_idx) {
							case 0:
								ImGui::StyleColorsClassic();
								break;
							case 1:
								ImGui::StyleColorsDark();
								break;
							case 2:
								ImGui::StyleColorsLight();
								break;
							}
						}
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				ImGui::End();
			}

			ImGui::EndFrame();

			/*
			mainDevice->SetRenderState(D3DRS_ZENABLE, false);
			mainDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			mainDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
			*/

			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		}
	}


}
#endif

namespace DX {


	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		//if (::ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam));
#ifdef USE_IMGUI
		ImGuiIO& io = ::ImGui::GetIO();

		if (msg == WM_KEYUP) {
			if (wParam == VK_F2) {
				b_showMenu = !b_showMenu;
			}
			//if (wParam == VK_MENU) {
			//	b_minimalLODTextures_DIP = false;
			//	b_minimalLODTextures_ST = false;
			//}
		}

		if (b_showMenu) {
			ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
			//return true;
		}
#endif
		/*
		if (b_showMenu && (io.WantCaptureMouse && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MOUSEWHEEL || msg == WM_MOUSEMOVE))) {
			return TRUE;
		}
		*/

		return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
	}

	void printStartMessage(void) {
		P("Tribes Ascend DirectX Modding V1.0.1");
		P("--------------------------------------");
		P("* Calling device->GetTexture in DrawIndexedPrimitive causes the Tribes process to hang in the xaudio2_7.dll module when the device is resetting (ALT+TAB)."
			" This can also occur in the setTexture hook function.");
		P("In the event Tribes hangs, kill this console window.");
	}
}