#define VERSION 1.0.3

#include <iostream>
#include <map>
#include <string>

//#include <ctime>
#include "CustomTextureManager.h"
#include "DX.h"
#include "Files.h"
//#include "Memory.h"

#define USE_QUERY_TIMER

//#define USE_TIMEGETTIME_TIMER
#ifdef USE_TIMEGETTIME_TIMER
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#endif

#define USE_IMGUI
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                              UINT msg,
                                              WPARAM wParam,
                                              LPARAM lParam);
#endif

#define TEXTURE_STAGES 8

//#define DEV
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
bool b_developer = true;
#endif

//#define USE_DXCANVAS
#ifdef USE_DXCANVAS
#include "DXCanvas.h"
#endif

//#define USE_PRESENT_MID_HOOK

namespace DX {
class TreeNode {
  vector<TreeNode*> children;
  void* data = NULL;

 public:
  void addChild(TreeNode* node) { children.push_back(node); }
  void setData(void* data) { this->data = data; }
} o_tree_root;
}  // namespace DX

namespace DX {

JumpHook* endSceneJumpHook = NULL;

VMTHook* beginSceneVMTHook = NULL;
VMTHook* endSceneVMTHook = NULL;
VMTHook* resetVMTHook = NULL;

VMTHook* drawIndexedPrimitiveVMTHook = NULL;
VMTHook* drawPrimitiveVMTHook = NULL;
VMTHook* drawIndexedPrimitiveUPVMTHook = NULL;

VMTHook* setTextureVMTHook = NULL;

FullJumpHook* presentFullJumpHook = NULL;

}  // namespace DX

namespace DX {
IDirect3DBaseTexture9* setTextureTexture[TEXTURE_STAGES] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
int i_textureStages = 0, i_textureStagesPrev = 0;

LPDIRECT3DTEXTURE9 transparentTexture = NULL;
bool b_isSniping = false;
bool b_isSnipingFlag = false;
bool b_draw = true;

bool b_showMenu = false;

bool b_disableZoomBars = false;
bool b_minimalLODTextures_DIP = false;
bool b_minimalLODTextures_ST = false;
// bool b_minimalLODTexturesStages_DIP[TEXTURE_STAGES] = { true, true, true,
// true, true, true, true, true };
bool b_minimalLODTexturesStages_DIP[TEXTURE_STAGES] = {true, true, true, true,
                                                       true, true, true, true};
int i_LODTexturesStages_DIP[TEXTURE_STAGES] = {-1, -1, -1, -1, -1, -1, -1, -1};
int i_LODTexturesStages_ST[TEXTURE_STAGES] = {-1, -1, -1, -1, -1, -1, -1, -1};

// bool b_minimalLODTexturesStages_DIP[TEXTURE_STAGES] = { true, true, false,
// true, false, true, true, true }; bool
// b_minimalLODTexturesStages_ST[TEXTURE_STAGES] = { true, true, false, false,
// false, true, true, true };
bool b_minimalLODTexturesStages_ST[TEXTURE_STAGES] = {true,  true, false, false,
                                                      false, true, true,  true};

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

int i_resx = 0;
int i_resy = 0;

// ------------

map<string, LPDIRECT3DTEXTURE9> m_loaded_textures_map;
map<string, string> m_file_name_to_absolute_path;
vector<string> v_loaded_textures_names;

int i_test_DIP_lower = 1e4;
int i_test_DIP_upper = 1e4;

}  // namespace DX

namespace DX {
// void updateCanvas(void);
/*
void toggleDraw(void) {
        b_draw = !b_draw;
}
*/

bool b_use_custom_textures = false;

LPDIRECT3DTEXTURE9 loadTexture(string path) {
  LPDIRECT3DTEXTURE9 texture = NULL;
  if (D3DXCreateTextureFromFileA(mainDevice, path.c_str(), &texture) ==
      D3D_OK) {
    return texture;
  }
  return NULL;
}

void loadTextures(void) {
  m_loaded_textures_map.clear();
  v_loaded_textures_names.clear();

  v_loaded_textures_names.push_back(string("<Default>"));
  v_loaded_textures_names.push_back(string("<None>"));

  using namespace UsefulSnippets;
  vector<Files::FileObject> v_texture_files =
      Files::getFiles("textures", ".png");
  vector<Files::FileObject> v_texture_files_jpg =
      Files::getFiles("textures", ".jpg");

  v_texture_files.insert(v_texture_files.end(), v_texture_files_jpg.begin(),
                         v_texture_files_jpg.end());

  for (vector<Files::FileObject>::iterator i = v_texture_files.begin();
       i != v_texture_files.end(); i++) {
    string s_file_absolute_path = i->getAbsolutePath();
    LPDIRECT3DTEXTURE9 texture = loadTexture(s_file_absolute_path);
    if (texture) {
      m_loaded_textures_map[i->getFileName()] = texture;
      m_file_name_to_absolute_path[i->getFileName()] = s_file_absolute_path;
      v_loaded_textures_names.push_back(i->getFileName());
    }
  }

  for (map<string, LPDIRECT3DTEXTURE9>::iterator i =
           m_loaded_textures_map.begin();
       i != m_loaded_textures_map.end(); i++) {
    cout << "Loaded texuture: " << i->first << " -> " << i->second << endl;
  }
  for (int j = 100; j < 1e4 + 1; j++) {
    for (int i = 0; i < TEXTURE_STAGES; i++) {
      o_custom_texture_staged_manager.addCustomTexture(j, i,
                                                       transparentTexture);
    }
    //cout << "Added " << j << " to manager." << endl;
  }
}

}  // namespace DX

namespace DX {
HWND hWnd;
WNDPROC oWndProc;
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LPDIRECT3DDEVICE9 mainDevice = NULL;

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

  // QueryPerformanceCounter(&li_previousFrameTime);
  li_previousFrameTime.QuadPart = 0;
#ifdef USE_TIMEGETTIME_TIMER
  timeBeginPeriod(0);
#endif
  return res;
}

bool createDevice(void) {
  LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
  if (!pD3D)
    return false;

  hWnd = FindWindow(NULL, "Tribes: Ascend (32-bit, DX9)");

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
  HRESULT res =
      pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                         D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
  if (FAILED(res))
    return false;

  DWORD endSceneAddress = VMTHook::getFunctionInstruction(device, 42);
  endSceneJumpHook =
      new JumpHook(endSceneAddress, (DWORD)endSceneGetDevice, pre);
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

  D3DXCreateTextureFromFile(device, "grid.png", &transparentTexture);
  loadTextures();

  getResolution();
#ifdef USE_DXCANVAS
  DX::Objects::f_defaultFont.create();
#endif
  HRESULT res = ((endScene)endSceneJumpHook->getHookAddress())(device);
  return res;
}

HRESULT __stdcall beginSceneHook(LPDIRECT3DDEVICE9 device) {
  b_isSnipingFlag = false;
  // b_isSniping = false;

  if (b_resetLODflag) {
    b_resetLOD = true;
    b_resetLODflag = false;
  }

  HRESULT res = ((beginScene)beginSceneVMTHook->getOriginalFunction())(device);
  return res;
}

HRESULT __stdcall endSceneHook(LPDIRECT3DDEVICE9 device) {
  if (b_resetLOD) {
    frameCount = (frameCount + 1) % 3;
    if (frameCount == 0) {
      b_resetLOD = false;
    }
  }

  if (b_isSnipingFlag) {
    b_isSniping = true;
  } else {
    b_isSniping = false;
  }

  // b_minimalLODTextures_DIP = false;

#ifdef USE_IMGUI
#ifndef USE_PRESENT_MID_HOOK
  ImGui_render();
#endif
#endif

  static unsigned long long currentTime = GetTickCount64();
  static LARGE_INTEGER QPF;
  QueryPerformanceFrequency(&QPF);

  double timeElapsedTick = 0;

#ifdef USE_QUERY_TIMER
  if (b_limitFPS) {
    static LARGE_INTEGER QPC;
    QueryPerformanceCounter(&QPC);
    unsigned long long deltaCounter =
        QPC.QuadPart - li_previousFrameTime.QuadPart;
    timeElapsedTick = deltaCounter / (QPF.QuadPart * 1.0);  // *1000;
    while (timeElapsedTick < (d_fpsDelay / 1000)) {
      // Sleep(1);
      QueryPerformanceCounter(&QPC);
      deltaCounter = QPC.QuadPart - li_previousFrameTime.QuadPart;
      timeElapsedTick = deltaCounter / (QPF.QuadPart * 1.0);
      // timeElapsedTick = ((QPC.QuadPart - li_previousFrameTime.QuadPart) /
      // (QPF.QuadPart *1.0)) *1000;
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

#ifdef USE_DXCANVAS
  if (DXCanvas.isReady()) {
    DXCanvas.begin();
    DXCanvas.setFont(&DX::Objects::f_defaultFont);
    DXCanvas.setPos(0, 0);
    DXCanvas.setColor(255, 255, 255, 255);
    DXCanvas.drawText(L"TribesAscend_DX_TA_V1.0.1", 0, 0);
    // DXCanvas.setType(Drawable::Type9);
    DXCanvas.end();
  }
#ifndef USE_PRESENT_MID_HOOK
  updateCanvas();
#endif
#endif

  // So calling font->DrawText for some reason resets
  // the VMT of the device, so we need to rehook endScene
  // Because the VMT is not reset by us, then hooked -> true
  // so we need to disable the hooked check, or set hooked
  // to false after font->DrawText
  beginSceneVMTHook->hook();
  endSceneVMTHook->hook();
  drawIndexedPrimitiveVMTHook->hook();
  drawPrimitiveVMTHook->hook();
  drawIndexedPrimitiveUPVMTHook->hook();
  resetVMTHook->hook();
  setTextureVMTHook->hook();
  HRESULT res = ((endScene)endSceneVMTHook->getOriginalFunction())(device);

  return res;
}

HRESULT __stdcall resetHook(LPDIRECT3DDEVICE9 device,
                            D3DPRESENT_PARAMETERS* pPresentationParameters) {
  // b_minimalLODTextures_DIP = false;
  // b_minimalLODTextures_ST = false;

#ifdef USE_IMGUI
  ImGui_ImplDX9_InvalidateDeviceObjects();
#endif

#ifdef USE_DXCANVAS
  if (DX::Objects::f_defaultFont.getFont()) {
    DX::Objects::f_defaultFont.getFont()->OnLostDevice();
  }
#endif

  HRESULT res = ((reset)resetVMTHook->getOriginalFunction())(
      device, pPresentationParameters);
#ifdef USE_DXCANVAS
  if (DX::Objects::f_defaultFont.getFont()) {
    DX::Objects::f_defaultFont.getFont()->OnResetDevice();
    DX::Objects::f_defaultFont.getFont()->Release();
    DX::Objects::f_defaultFont.created = false;
    DX::Objects::f_defaultFont.font = NULL;
    DX::Objects::f_defaultFont.create();
    getResolution();
    DXCanvas.initialise();
    DXCanvas.clear();
  }
#endif

#ifdef USE_IMGUI
  ImGui_ImplDX9_CreateDeviceObjects();
#endif

  return res;
}

HRESULT __stdcall drawIndexedPrimitiveHook(LPDIRECT3DDEVICE9 device,
                                           D3DPRIMITIVETYPE primType,
                                           INT BaseVertexIndex,
                                           UINT MinVertexIndex,
                                           UINT NumVertices,
                                           UINT startIndex,
                                           UINT primCount) {
#ifdef DEBUG
  cout << "drawIndexedPrimitiveHook" << endl;
#endif

  // IDirect3DBaseTexture9* pTexture;
  // bool b = device->GetTexture(0, &pTexture) == D3D_OK;

  if (!b_draw) {
    return D3D_OK;
  }
  // 11944->Phase
  // 9658->BXT
  if (NumVertices == 11944 || NumVertices == 9658) {
    b_isSnipingFlag = true;
  }

  if (b_resetLOD && false) {
    for (int i = 0; i <= TEXTURE_STAGES; i++) {
      IDirect3DBaseTexture9* pTexture;
      bool b = device->GetTexture(i, &pTexture) == D3D_OK;
      if (!b || !pTexture) {
        continue;
      }
      pTexture->SetLOD(0);
    }
    HRESULT res = ((drawIndexedPrimitive)
                       drawIndexedPrimitiveVMTHook->getOriginalFunction())(
        device, primType, BaseVertexIndex, MinVertexIndex, NumVertices,
        startIndex, primCount);
    return res;
  }

  // i_textureStages /*TEXTURE_STAGES*/
  if (b_minimalLODTextures_DIP && NumVertices >= i_LODstart &&
      NumVertices <= i_LODend) {
    for (int i = 0; i < /*i_textureStages*/ TEXTURE_STAGES; i++) {
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
        b = device->GetTexture(i, &pTexture) == D3D_OK;  // DIP
      } else {
        pTexture = setTextureTexture[i];  // DIP+ST
      }

      if (!b || !pTexture) {
        continue;
      }

      int lod = pTexture->GetLOD();
      int level = pTexture->GetLevelCount();
      if (i_LODTexturesStages_DIP[i] == -1 ||
          i_LODTexturesStages_DIP[i] >= level) {
        if (lod != level - 1) {
          pTexture->SetLOD(level - 1);
        } else {
        }
      } else {
        if (lod != i_LODTexturesStages_DIP[i] &&
            i_LODTexturesStages_DIP[i] < level - 1)
          pTexture->SetLOD(i_LODTexturesStages_DIP[i]);
      }
    }
  } else {
    if (b_minimalLODTextures_DIP) {
      // return D3D_OK;
    }
  }

  if (i_test_DIP_lower <= NumVertices && i_test_DIP_upper >= NumVertices) {
    for (int i = 0; i < /*i_textureStages*/ TEXTURE_STAGES; i++) {
      device->SetTexture(i, transparentTexture);
    }
  }

  if (b_use_custom_textures) {
    CustomTextureStaged* o_custom_texture_staged =
        o_custom_texture_staged_manager.tick(NumVertices);
    if (o_custom_texture_staged) {
      unsigned int** uipp_texture_custom_stages =
          o_custom_texture_staged->getStagesTextures();
      if (uipp_texture_custom_stages) {
        // cout << "Custom texture applied. primCount = " << primCount << endl;
        for (int i = 0; i < /*i_textureStages*/ TEXTURE_STAGES; i++) {
          if (!o_custom_texture_staged->usingStage(i))
            continue;
          unsigned int* texture = uipp_texture_custom_stages[i];
          if (true || texture)
            device->SetTexture(i, (LPDIRECT3DTEXTURE9)texture);
        }
      }
    }
  }
  HRESULT res = ((drawIndexedPrimitive)
                     drawIndexedPrimitiveVMTHook->getOriginalFunction())(
      device, primType, BaseVertexIndex, MinVertexIndex, NumVertices,
      startIndex, primCount);
  return res;
}

HRESULT __stdcall drawPrimitiveHook(LPDIRECT3DDEVICE9 device,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    UINT StartVertex,
                                    UINT PrimitiveCount) {
  if (!b_draw) {
    return D3D_OK;
  }

  if (b_disableZoomBars && PrimitiveCount == 2 && PrimitiveType == 5 &&
      StartVertex == 0 /*&& !b_isSniping*/) {
    bool stateSuccess = device->SetTexture(1, transparentTexture) == D3D_OK;
  }
  HRESULT res = ((drawPrimitive)drawPrimitiveVMTHook->getOriginalFunction())(
      mainDevice, PrimitiveType, StartVertex, PrimitiveCount);
  return res;
}

HRESULT __stdcall drawIndexedPrimitiveUPHook(LPDIRECT3DDEVICE9 device,
                                             D3DPRIMITIVETYPE PrimitiveType,
                                             UINT MinVertexIndex,
                                             UINT NumVertices,
                                             UINT PrimitiveCount,
                                             CONST void* pIndexData,
                                             D3DFORMAT IndexDataFormat,
                                             CONST void* pVertexStreamZeroData,
                                             UINT VertexStreamZeroStride) {
  if (!b_draw) {
    return D3D_OK;
  }

  // bxt = 512
  // phase = 822

  if (NumVertices == 512 || (NumVertices >= 1991 && NumVertices <= 1991)) {
    b_isSnipingFlag = true;
  }
  HRESULT res = ((drawIndexedPrimitiveUP)
                     drawIndexedPrimitiveUPVMTHook->getOriginalFunction())(
      device, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount,
      pIndexData, IndexDataFormat, pVertexStreamZeroData,
      VertexStreamZeroStride);
  return res;
}

HRESULT __stdcall setTextureHook(LPDIRECT3DDEVICE9 device,
                                 DWORD Stage,
                                 IDirect3DBaseTexture9* pTexture) {
  HRESULT res;

#ifdef DEBUG
  cout << "setTextureHook: Stage = " << Stage << endl;
#endif

  if (b_resetLOD) {
    if (pTexture)
      pTexture->SetLOD(0);
    i_textureStages = 0;
    i_textureStagesPrev = 0;
    res = ((setTexture)(setTextureVMTHook->getOriginalFunction()))(
        device, Stage, pTexture);
    return res;
  }

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
          res = ((setTexture)(setTextureVMTHook->getOriginalFunction()))(
              device, Stage, pTexture);
          return res;
        }
      }

      int lod = pTexture->GetLOD();
      int level = pTexture->GetLevelCount();

      if (frameCount == 0 &&
          (Stage == 1 || Stage == 2 || Stage == 3 || Stage == 4) && false) {
        pTexture->SetLOD(0);
      } else {
        if (i_LODTexturesStages_ST[Stage] == -1) {
          if (lod != level - 1) {
            pTexture->SetLOD(level - 1);
          } else {
          }
        } else {
          pTexture->SetLOD(i_LODTexturesStages_ST[Stage]);
        }
      }
    }
  }

  res = ((setTexture)(setTextureVMTHook->getOriginalFunction()))(device, Stage,
                                                                 pTexture);
  return res;
}

void getResolution(void) {
  D3DDEVICE_CREATION_PARAMETERS cparams;
  RECT rect;

  mainDevice->GetCreationParameters(&cparams);
  GetClientRect(cparams.hFocusWindow, &rect);

  i_resx = rect.right - rect.left;
  i_resy = rect.bottom - rect.top;

  // hud_pos = { i_resx / 2 , 4 * i_resy / 5 };
}

#ifdef USE_PRESENT_MID_HOOK
void __stdcall presentHook(LPDIRECT3DDEVICE9 device,
                           const RECT* pSourceRect,
                           const RECT* pDestRect,
                           HWND hDestWindowOverride,
                           const RGNDATA* pDirtyRegion) {
#ifdef USE_DXCANVAS
  updateCanvas();
#endif
  ImGui_render();
  endSceneVMTHook->hook();
}
#endif

void initHooks(void) {
  beginSceneVMTHook = new VMTHook(mainDevice, 41, (DWORD)beginSceneHook, pre);
  endSceneVMTHook = new VMTHook(mainDevice, 42, (DWORD)endSceneHook, pre);
  resetVMTHook = new VMTHook(mainDevice, 16, (DWORD)resetHook, pre);
  drawIndexedPrimitiveVMTHook =
      new VMTHook(mainDevice, 82, (DWORD)drawIndexedPrimitiveHook, pre);
  drawPrimitiveVMTHook =
      new VMTHook(mainDevice, 81, (DWORD)drawPrimitiveHook, pre);
  drawIndexedPrimitiveUPVMTHook =
      new VMTHook(mainDevice, 84, (DWORD)drawIndexedPrimitiveUPHook, pre);
  setTextureVMTHook = new VMTHook(mainDevice, 65, (DWORD)setTextureHook, pre);
#ifdef USE_PRESENT_MID_HOOK
  DWORD addr = JumpHook::firstNonJMPInstruction(
      VMTHook::getFunctionInstruction(mainDevice, 17));
  presentFullJumpHook =
      new FullJumpHook(addr, 0x26, 0x2d, (DWORD)presentHook, 5);
#endif
}
}  // namespace DX

#ifdef USE_IMGUI
namespace DX {
void ImGui_setup(void) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
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

    // static bool b = true;
    // ImGui::ShowDemoWindow(&b);

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

      ImGui::Begin("TribesAscend_DX_TA##V1.0.3");

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
            // QueryPerformanceCounter(&li_previousFrameTime);
          }

          if (ImGui::SliderInt("FPS", &i_fpsLimit, 30, 1000)) {
            d_fpsDelay = 1000.0 / i_fpsLimit;
            // QueryPerformanceCounter(&li_previousFrameTime);
          }

          if (ImGui::CollapsingHeader("LOD changer (notexturestreaming)")) {
#ifdef DANGER
            if (ImGui::CollapsingHeader(
                    "[DANGEROUS] DrawIndexedPrimitive method")) {
              if (ImGui::Checkbox(
                      "Enable [WARNING - Causes freeze on alt+tab]##DIP",
                      &b_minimalLODTextures_DIP)) {
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
                    stageChanged =
                        stageChanged ||
                        (ImGui::Checkbox(str,
                                         &b_minimalLODTexturesStages_DIP[i]) &&
                         b_minimalLODTextures_DIP);
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
            if (/*ImGui::CollapsingHeader("DIP+ST method")*/ true) {
              if (ImGui::Checkbox("Enable##DIPST", &b_minimalLODTextures_DIP)) {
                if (!b_minimalLODTextures_DIP) {
                  b_resetLODflag = true;
                }
              }
#ifdef RDEV
              if (b_developer) {
                static bool customizeStages = false;
                if (/*ImGui::Checkbox("Edit stages##DIPST", &customizeStages)*/
                    false) {
                  if (!customizeStages) {
                    for (int i = 0; i < TEXTURE_STAGES; i++) {
                      i_LODTexturesStages_DIP[i] = -1;
                    }
                  }
                }
                if (customizeStages || true) {
                  bool stageChanged = false;
                  for (int i = 0; i < TEXTURE_STAGES; i++) {
                    string s = to_string(i);
                    strcpy(str, "Stage ");
                    strcat(str, s.c_str());
                    strcat(str, "##DIP");
                    stageChanged =
                        stageChanged ||
                        (ImGui::Checkbox(str,
                                         &b_minimalLODTexturesStages_DIP[i]) &&
                         b_minimalLODTextures_DIP);
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
                    stageChanged =
                        stageChanged ||
                        (ImGui::Checkbox(str,
                                         &b_minimalLODTexturesStages_ST[i]) &&
                         b_minimalLODTextures_ST);
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
              if (ImGui::Checkbox("Apply to guns and player models",
              &b_keepWeaponsAndModels)) { if (!b_keepWeaponsAndModels) {
                              b_minimalLODTexturesStages_ST[1] = false;
                      }
                      else {
                              b_minimalLODTexturesStages_ST[1] = true;
                      }
              }
              */
            }

            // ImGui::Separator();
            // ImGui::Checkbox("Preserve chain bullets (not squares)",
            // &b_preserveChainBullets);
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

        if (ImGui::BeginTabItem("Skinning/Texture replacement")) {
          ImGui::Checkbox("Use custom textures", &b_use_custom_textures);
          static int i_texture_replacement_prim_count = 100;
          static int i_texture_replacement_stage = 0;
          ImGui::Text("DrawIndexed Primitive Count");
          ImGui::SliderInt("##dippc", &i_texture_replacement_prim_count, 100,
                           1E5);

          ImGui::Text("DrawIndexed Stage");
          ImGui::SliderInt("##dips", &i_texture_replacement_stage, 0,
                           TEXTURE_STAGES - 1);

          static int i_texture_combo_selected_index = 0;
          if (ImGui::BeginCombo(
                  "Textures",
                  v_loaded_textures_names[i_texture_combo_selected_index]
                      .c_str(),
                  0)) {
            for (int n = 0; n < v_loaded_textures_names.size(); n++) {
              bool is_selected = (i_texture_combo_selected_index == n);
              if (ImGui::Selectable(v_loaded_textures_names[n].c_str(),
                                    is_selected)) {
                i_texture_combo_selected_index = n;

                if (n <= 1) {
                  if (n == 0) {  // Default
                    // o_custom_texture_staged_manager.removeCustomTexture(
                    //    i_texture_replacement_prim_count,
                    //    i_texture_replacement_stage);
                    CustomTextureStaged* op_custom_texture_staged =
                        o_custom_texture_staged_manager.addCustomTexture(
                            i_texture_replacement_prim_count,
                            i_texture_replacement_stage, NULL);
                    op_custom_texture_staged->disableStage(
                        i_texture_replacement_stage);
                  } else if (n == 1) {
                    CustomTextureStaged* op_custom_texture_staged =
                        o_custom_texture_staged_manager.addCustomTexture(
                            i_texture_replacement_prim_count,
                            i_texture_replacement_stage, NULL);
                  }
                } else {
                  map<string, LPDIRECT3DTEXTURE9>::iterator iter_texture =
                      m_loaded_textures_map.find(v_loaded_textures_names[n]);

                  // map<string, string>::iterator iter_absolute_path =
                  //    m_file_name_to_absolute_path.find(v_loaded_textures_names[n]);

                  if (iter_texture != m_loaded_textures_map.end()) {
                    o_custom_texture_staged_manager.addCustomTexture(
                        i_texture_replacement_prim_count,
                        i_texture_replacement_stage, iter_texture->second,
                        v_loaded_textures_names[n]);
                    cout << "Applying texture (" << iter_texture->first << ", "
                         << iter_texture->second << ") to "
                         << i_texture_replacement_prim_count << ":"
                         << i_texture_replacement_stage << endl;

                  } else {
                    cout
                        << "Could not apply the texture as the texture was not "
                           "found."
                        << endl;
                  }
                }
              }

              if (is_selected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          if (ImGui::Button("Apply to all stages")) {
            map<string, LPDIRECT3DTEXTURE9>::iterator iter_texture =
                m_loaded_textures_map.find(
                    v_loaded_textures_names[i_texture_combo_selected_index]);

            if (iter_texture != m_loaded_textures_map.end()) {
              for (int i = 0; i < TEXTURE_STAGES; i++) {
                o_custom_texture_staged_manager.addCustomTexture(
                    i_texture_replacement_prim_count, i, iter_texture->second);
                cout << "Applying texture (" << iter_texture->first << ", "
                     << iter_texture->second << ") to "
                     << i_texture_replacement_prim_count << ":" << i << endl;
              }

            } else {
              cout << "Could not apply the texture as the texture was not "
                      "found."
                   << endl;
            }
          }

          ImGui::Separator();
          ImGui::SliderInt("NumVert Lower bound", &i_test_DIP_lower, 0, 1e4);
          ImGui::SliderInt("NumVert Upper bound", &i_test_DIP_upper, 0, 1e4);

          ImGui::Separator();

          if (ImGui::Button("Save config")) {
            o_custom_texture_staged_manager.save(string("custom_texture.txt"));
          }

          ImGui::EndTabItem();
        }

        /*
if (ImGui::BeginTabItem("Skinning/Texture replacement##1")) {
  static bool disable_mouse_wheel = false;
  static bool disable_menu = false;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_HorizontalScrollbar |
      (disable_mouse_wheel ? ImGuiWindowFlags_NoScrollWithMouse : 0);
  ImGui::BeginChild(
      "Child1",
      ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 260), false,
      window_flags);
  for (int i = 0; i < 100; i++) {
    ImGui::Text("%04d: scrollable region", i);
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // Child 2: rounded border

  ImGuiWindowFlags window_flags_ =
      (disable_mouse_wheel ? ImGuiWindowFlags_NoScrollWithMouse : 0) |
      (disable_menu ? 0 : ImGuiWindowFlags_MenuBar);
  ImGui::BeginChild("Child2", ImVec2(0, 260), true, window_flags_);
  ImGui::Columns(1);
  for (int i = 0; i < 100; i++) {
    char buf[32];
    sprintf(buf, "%03d", i);
    ImGui::Button(buf, ImVec2(-1.0f, 0.0f));
  }
  ImGui::EndChild();

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
*/

        if (ImGui::BeginTabItem("Skinning/Texture replacement##2")) {
          if (ImGui::TreeNode("Trees")) {
            if (ImGui::TreeNode("Basic trees")) {
              for (int i = 0; i < 5; i++)
                if (ImGui::TreeNode((void*)(intptr_t)i, "Child %d", i)) {
                  ImGui::Text("blah blah");
                  ImGui::SameLine();
                  if (ImGui::SmallButton("button")) {
                  };
                  ImGui::TreePop();
                }
              ImGui::TreePop();
            }
            ImGui::TreePop();
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

}  // namespace DX
#endif

namespace DX {
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // if (::ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam));
#ifdef USE_IMGUI
  ImGuiIO& io = ::ImGui::GetIO();

  if (msg == WM_KEYUP) {
    if (wParam == VK_INSERT) {
      b_showMenu = !b_showMenu;
    }

    if (wParam == VK_F3) {
      b_use_custom_textures = !b_use_custom_textures;
    }
    // if (wParam == VK_MENU) {
    //	b_minimalLODTextures_DIP = false;
    //	b_minimalLODTextures_ST = false;
    //}
  }

  if (b_showMenu) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    return true;
  }
#endif
  /*
  if (b_showMenu && (io.WantCaptureMouse && (msg == WM_LBUTTONDOWN || msg ==
  WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg ==
  WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MOUSEWHEEL || msg ==
  WM_MOUSEMOVE))) { return TRUE;
  }
  */

  return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

void printStartMessage(void) {
  P("Tribes Ascend DirectX Modding V1.0.3");
  P("--------------------------------------");
  // P("* Calling device->GetTexture in DrawIndexedPrimitive causes the Tribes
  // process to hang in the xaudio2_7.dll module when the device is resetting
  // (ALT+TAB)." " This can also occur in the setTexture hook function.");
  P("In the event Tribes hangs, kill this console window.");
}
}  // namespace DX