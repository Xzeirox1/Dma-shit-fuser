#pragma once
#include <vector>
#include "structs and stuff/Player.h"


class PlayerManager
{
public:
	bool UpdatePlayerList();
	bool UpdatePlayerPositions();
	Player* FindLocalPlayer();

	std::vector<Player>currentPlayerList;

	uint64_t registeredPlayers { 0 };
};

inline PlayerManager playerManager;