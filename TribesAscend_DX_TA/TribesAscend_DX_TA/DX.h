#pragma once
//#include <iostream>
#include "Hook.h"

// DirectX includes
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#include <d3d9.h>
#include <d3dx9.h>


using namespace Hooks;

namespace DX {
	/*
		Hook Objects
	*/
	extern LPDIRECT3DDEVICE9 mainDevice;

	//LPDIRECT3DDEVICE9
	extern VMTHook* beginSceneVMTHook;
	extern JumpHook* endSceneJumpHook;

	extern VMTHook* endSceneVMTHook;
	extern VMTHook* resetVMTHook;

	extern VMTHook* drawIndexedPrimitiveVMTHook;
	extern VMTHook* drawPrimitiveVMTHook;
	extern VMTHook* drawPrimitiveUPVMTHook;

	extern VMTHook* setTextureVMTHook;

	extern FullJumpHook* presentFullJumpHook;

	/*
		Typedefs for callbacks
	*/
	//LPDIRECT3DDEVICE9
	typedef HRESULT(__stdcall* beginScene)(LPDIRECT3DDEVICE9);
	typedef HRESULT(__stdcall* endScene)(LPDIRECT3DDEVICE9);
	typedef HRESULT(__stdcall* reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
	typedef HRESULT(__stdcall* drawIndexedPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
	typedef HRESULT(__stdcall* drawPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, UINT, UINT);
	typedef HRESULT(__stdcall* drawIndexedPrimitiveUP)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, UINT, UINT, UINT, CONST void*, D3DFORMAT, CONST void*, UINT);
	typedef HRESULT(__stdcall* setTexture)(LPDIRECT3DDEVICE9, DWORD, IDirect3DBaseTexture9*);

	/*
		Function prototypes
	*/

	//Device
	bool hook(void);
	bool createDevice(void);

	HRESULT __stdcall beginSceneHook(LPDIRECT3DDEVICE9 device);
	HRESULT __stdcall endSceneGetDevice(LPDIRECT3DDEVICE9 device);

	HRESULT __stdcall endSceneHook(LPDIRECT3DDEVICE9 device);
	HRESULT __stdcall resetHook(LPDIRECT3DDEVICE9 device, D3DPRESENT_PARAMETERS* pPresentationParameters);
	
	HRESULT __stdcall drawIndexedPrimitiveHook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE primType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
	HRESULT __stdcall drawPrimitiveHook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
	HRESULT __stdcall drawPrimitiveUPHook(LPDIRECT3DDEVICE9 device, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);

	HRESULT __stdcall setTextureHook(LPDIRECT3DDEVICE9 device, DWORD Stage, IDirect3DBaseTexture9* pTexture);


	//Global
	void initHooks(void);
	void getResolution(void);
	//Functions
	void toggleDraw(void);
}

namespace DX {
		void ImGui_setup(void);
		void ImGui_render(void);

		void printStartMessage(void);
}