#include <iostream>
#include "PlayerManager.h"
#include "GameManager.h"
#include "Offsets.h"
#include "Memory/Memory.h"
#include "structs and stuff/structs.h"

/*
In offline everyone is LocalPlayer
In Online only you are ClientPlayer
Everyone else is ObservedPlayerView

ObservedPlayerView -> Enemies, scavs and so on in ONLINE Raid
ClientPlayer -> our player in online raid, obtainable from EFT.GameWorld->MainPlayer
LocalPlayer -> everyone in offline raids, contains EFT.Player fields.
*/

bool PlayerManager::UpdatePlayerList()
{
	// registeredPlayers will be read and set by the game manager when we are checking raid status
	// we dont need to re-read it anytime we update the player list
	if (registeredPlayers == 0x0)
		return false;

	std::cout << "[PlayerManager] Reading registered players ==> 0x" << std::hex << registeredPlayers << "\n";

	uint64_t registeredPlayersListPtr = mem.Read<uint64_t>(registeredPlayers + offsetof(List, listBase));
	List playerList = mem.Read<List>(registeredPlayersListPtr);

	if (playerList.itemCount <= 0)
	{
		std::cout << "[PlayerManager] Failed to get players list... are we in raid?\n";
		return false;
	}

	std::vector<uint64_t> newPlayerList;
	for (int i = 0; i < playerList.itemCount; i++)
	{
		uint64_t playerAddress = mem.Read<uint64_t>(registeredPlayersListPtr + (offsetof(ListInternal, firstEntry) + (i * 8)));

		if (!playerAddress)
			break;

		newPlayerList.push_back(playerAddress);
	}

	int oldSize = currentPlayerList.size();

	// Remove players from currentPlayerList that are not in newPlayerList
	currentPlayerList.erase(
		std::remove_if(currentPlayerList.begin(), currentPlayerList.end(),
			[&](const Player& p) {
				return std::find(newPlayerList.begin(), newPlayerList.end(), p.baseAddress) == newPlayerList.end();
			}),
		currentPlayerList.end());

	// Add new players from newPlayerList to currentPlayerList if they are not already present
	for (uint64_t newPlayerAddy : newPlayerList)
	{
		if (std::find_if(currentPlayerList.begin(), currentPlayerList.end(),
			[&](const Player& p) { return p.baseAddress == newPlayerAddy; }) == currentPlayerList.end())
		{
			Player p = Player(newPlayerAddy);
			currentPlayerList.push_back(p);
		}
	}

	// ^^^
	// the code above reduces the redunancy of deleting and re-creating all Player instances every time
	// we update the registered players list
	// so we dont need to re-read information of players that are still alive because their information
	// will be the exact same as it was before. I.E. bone ptrs chain && side && isAi etc...

	int newSize = currentPlayerList.size();

	std::cout << std::dec << "[PlayerManager] playerList size went from " << oldSize << " to " << newSize << "\n";
	return true;
}

bool PlayerManager::UpdatePlayerPositions()
{
	if (currentPlayerList.size() == 0) return false;

	auto handle = mem.CreateScatterHandle();

	// first we read all the bone transforms
	for (Player& player : currentPlayerList)
	{
		// now we loop through the player bones
		for (int i = 0; i < player.bonePtrs.size(); i++)
		{
			TransformAccessReadOnly* transformAccessReadOnly = &player.boneTransforms[i];

			void* buffer = static_cast<void*>(transformAccessReadOnly);

			uint64_t addy = player.bonePtrs[i] + 0x38;

			mem.AddScatterReadRequest(handle, addy, buffer, sizeof(TransformAccessReadOnly));
		}
	}

	mem.ExecuteReadScatter(handle);

	// then we read all the bone transform's data
	for (Player& player : currentPlayerList)
	{
		// now we loop through all the player bones
		for (int i = 0; i < player.bonePtrs.size(); i++)
		{
			TransformData* transformData = &player.boneTransformsData[i];

			void* buffer = static_cast<void*>(transformData);

			if (player.boneTransforms[i].pTransformData == 0)
			{
				// we did not read a valid bone transform.
				// this happens when the player leaves a raid but we have not updated the 
				// player list yet
				std::cout << "Failed to read bone transform data. Exiting...\n";
				mem.CloseScatterHandle(handle);
				return false;
			}

			uint64_t addy = player.boneTransforms[i].pTransformData + 0x18;

			mem.AddScatterReadRequest(handle, addy, buffer, sizeof(TransformData));
		}
	}

	mem.ExecuteReadScatter(handle);

	// and now finally we can read the information inside the bone transform
	for (Player& player : currentPlayerList)
	{
		for (int i = 0; i < player.bonePtrs.size(); i++)
		{
			TransformAccessReadOnly pTransformAccessReadOnly = player.boneTransforms[i];
			TransformData transformData = player.boneTransformsData[i];

			if (transformData.pTransformArray == 0 || transformData.pTransformIndices == 0)
			{
				std::cout << "Failed to read information inside bone transform. Exiting...\n";
				mem.CloseScatterHandle(handle);
				return false;
			}

			SIZE_T sizeMatriciesBuf;
			SIZE_T sizeIndicesBuf;

			try
			{
				sizeMatriciesBuf = sizeof(Matrix34) * pTransformAccessReadOnly.index + sizeof(Matrix34);
				sizeIndicesBuf = sizeof(int) * pTransformAccessReadOnly.index + sizeof(int);
			}
			catch (...)
			{
				std::cout << "Bone transform index was not properly read inside the transform data. Exiting...\n";
				mem.CloseScatterHandle(handle);
				return false;
			}

			player.pMatriciesBuffers[i] = malloc(sizeMatriciesBuf);
			player.pIndicesBuffers[i] = malloc(sizeIndicesBuf);

			if (player.pMatriciesBuffers[i] && player.pIndicesBuffers[i])
			{
				mem.AddScatterReadRequest(handle, transformData.pTransformArray, player.pMatriciesBuffers[i], sizeMatriciesBuf);
				mem.AddScatterReadRequest(handle, transformData.pTransformIndices, player.pIndicesBuffers[i], sizeIndicesBuf);
			}
		}
	}

	mem.ExecuteReadScatter(handle);

	mem.CloseScatterHandle(handle);

	//now that we have actually updated the data, update the player position
	for (Player& player : currentPlayerList)
	{
		player.UpdateBonePositions();
	}

	return true;
}

Player* PlayerManager::FindLocalPlayer()
{
	// If we have no players, we are no longer in raid
	if (currentPlayerList.size() == 0)
		return nullptr;

	// find the local player
	for (int i = 0; i < currentPlayerList.size(); i++)
	{
		// in offline raids the first LocalPlayer is always OUR player
		// so this will break after the very first index
		if (currentPlayerList[i].playerType == LocalPlayer)
		{
			std::cout << "Found our LocalPlayer from offline raid\n";
			currentPlayerList[i].isLocalPlayer = true;
			return &currentPlayerList[i];
		}
		// in online raids the client player is always the local player but is not always the first index
		else if (currentPlayerList[i].playerType == ClientPlayer)
		{
			std::cout << "Found ClientPlayer from online raid\n";
			currentPlayerList[i].isLocalPlayer = true;
			return &currentPlayerList[i];
		}
	}
	//for (Player& player : currentPlayerList)
	//{
	//	// in offline raids the first LocalPlayer is always OUR player
	//	// so this will break after the very first index
	//	if (player.playerType == LocalPlayer)
	//	{
	//		std::cout << "Found our LocalPlayer from offline raid\n";
	//		player.isLocalPlayer = true;
	//		return player;
	//	}
	//	// in online raids the client player is always the local player but is not always the first index
	//	else if (player.playerType == ClientPlayer)
	//	{
	//		std::cout << "Found ClientPlayer from online raid\n";
	//		player.isLocalPlayer = true;
	//		return player;
	//	}
	//}

	return nullptr;
}