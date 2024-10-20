#pragma once

namespace Offsets
{
	constexpr uint64_t GOM = 0x17FFD28;
	const std::vector<uint64_t> LGW = {0x30, 0x18, 0x28};
	constexpr uint64_t ALLCAMS = 0x179F500;

	const std::vector<uint64_t> viewMatrix = { 0x30, 0x18 };

	namespace LocalGameWorld
	{
		constexpr uint64_t RegisteredPlayers = 0x0;
		constexpr uint64_t ExfilManager = 0x0;

		namespace Exfil
		{
			constexpr uint64_t ExfilPointArray = 0x20;
			const std::vector<uint64_t> transformInternalChain = { 0x10, 0x30, 0x30, 0x8, 0x28, 0x10 };

			namespace ExfilPoint
			{
				constexpr uint64_t status = 0xA8;
			}
		}
	}

	namespace Player
	{
		const std::vector<uint64_t> boneMatrix = { 0xA8,0x28,0x28,0x10 };
	}

	namespace ObservedPlayerView
	{
		const std::vector<uint64_t> boneMatrix = { 0x60,0x28,0x28,0x10 };
		constexpr uint64_t side = 0xF0;
		constexpr uint64_t isAI = 0x109;
	}


	namespace Unity
	{
		const std::vector<uint64_t> className = { 0x0, 0x0, 0x48 };
	}
}
