#include "Overlay.h"
#include "../GameManager.h"
#include "../PlayerManager.h"

void Overlay::ManageTimedUpdateFunctions()
{
	// Before you say anything about this spaghetti code...
	// I know it would be cleaner to do this with multiple threads and a mutex
	// 
	// But for some reason when I tested both ways (with threads vs like this) the performance with DMA reads seemed better when everything is handeled in a single thread...
	// So thats why I do this
	// 
	// I may have done something wrong my first try with threads but...
	// If it aint broke dont fix it ¯\_(''/)_/¯
	// 
	// 
	auto currentTime = std::chrono::steady_clock::now();
	auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - loopIntervalLastUpdatedTime);

	for (auto& loopTime : loopTimes)
	{
		loopTime += elapsedTime;
	}

	for (int i = 0; i < loopTimes.size(); ++i)
	{
		if (loopTimes[i] >= loopIntervals[i])
		{
			// see if we have passed the ms delay between each function...
			// if we have then run it and reset the timer
			(this->*loopFunctions[i])();
			loopTimes[i] = std::chrono::milliseconds(0);
		}
	}

	// Now update the timer
	loopIntervalLastUpdatedTime = std::chrono::steady_clock::now();
}


void Overlay::CheckRaidStart()
{
	// this flag will be switched whenever an invalid player list or player position read is detected.
	// only then should we check if we are in raid
	if (inRaid) return;
	this->inRaid = gameManager.CheckInRaid();
}

void Overlay::UpdatePlayerList()
{
	if (!inRaid) return;
	this->inRaid = playerManager.UpdatePlayerList();
	this->localPlayer = playerManager.FindLocalPlayer();
}

void Overlay::UpdatePlayerPositions()
{
	if (!inRaid) return;
	this->inRaid = playerManager.UpdatePlayerPositions();
}

void Overlay::ReadViewMatrix()
{
	if (!inRaid) return;
	gameManager.ReadViewMatrix();
}

void Overlay::ToggleEsp()
{
	this->renderEsp = !this->renderEsp;
}