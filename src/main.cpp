#include "overlayStuff/Overlay.h"
#include "GameManager.h"
#include "PlayerManager.h"
#include <thread>
#include "Memory/Memory.h"

void clearConsole()
{
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}

void InitMemory()
{

	if (!mem.Init("escapefromtarkov.exe", true, true))
	{
		std::cout << "Failed to INIT DMA... Is the game running?\n";
		return;
	}

	std::cout << "DMA Initialized!!\n";

	uint64_t base = mem.GetBaseDaddy("UnityPlayer.dll");
	std::cout << "Base ::: 0x" << std::hex << base << "\n";
	gameManager.baseAddy = base;
}

void HandleShutdown()
{
	ov.DestroyOverlay();
}

int main()
{
	if (!ov.CreateOverlay())
		return 2;

	InitMemory();

	ov.OverlayLoop();

	VMMDLL_Close(mem.vHandle);

	return 0;
}

