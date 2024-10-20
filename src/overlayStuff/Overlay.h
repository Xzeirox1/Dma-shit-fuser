#pragma once
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <chrono>

#include "../structs and stuff/Player.h"


class Overlay
{
public:
	bool CreateOverlay();
	void OverlayLoop();
	void DestroyOverlay();

	bool running = false;

	bool renderEsp = true;

	bool inRaid = false;

	// Related to the overlay
	void ToggleEsp();
	void HandleCheatOverlay();
	void RenderEsp();
	void RenderExfils();

	void CheckRaidStart();


	// Functions supporting the overlay
	void UpdatePlayerList();
	void UpdatePlayerPositions();
	void ReadViewMatrix();

	int tmpCount = 0;

	// These are the timers for the methods to update the data while in raid
	std::chrono::steady_clock::time_point loopIntervalLastUpdatedTime;
	std::vector<std::chrono::milliseconds> loopTimes;
	std::vector<std::chrono::milliseconds> loopIntervals;
	std::vector<void(Overlay::*)()> loopFunctions;

	Overlay()
	{
		loopIntervalLastUpdatedTime = std::chrono::steady_clock::now();

		loopIntervals = {
			std::chrono::milliseconds(20),
			std::chrono::milliseconds(5000),
			std::chrono::milliseconds(5000),
			std::chrono::milliseconds(30),
		};

		loopTimes = {
			std::chrono::milliseconds(0),
			std::chrono::milliseconds(0),
			std::chrono::milliseconds(0),
			std::chrono::milliseconds(0),
		};


		loopFunctions = {
			&Overlay::ReadViewMatrix,
			&Overlay::CheckRaidStart,
			&Overlay::UpdatePlayerList,
			&Overlay::UpdatePlayerPositions,
		};
	}
private:

	// Overlay
	WNDCLASSEXA wc;
	HWND Hwnd;
	char ClassName[16] = "NULL";
	char TitleName[16] = "Overlay";

	Player* localPlayer = nullptr;

	void ManageTimedUpdateFunctions();
};

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;

extern bool IsKeyDown(int VK);

inline Overlay ov;