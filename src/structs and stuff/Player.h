#pragma once
#include <cstdint>
#include <string>
#include <SimpleMath.h>
#include "structs.h"

enum PlayerType
{
	LocalPlayer,
	ClientPlayer,
	ObservedPlayerView,
	UNKNOWNERROR
};

enum boneListIndexes
{
	Pelvis, Head, LForearm, LPalm, RForearm, RPalm, LThigh, LFoot, RThigh, RFoot
};

class Player
{
private:

	uint64_t playerBoneMatrixPtr;

	void SetupBonePtrs();

public:
	Player(uint64_t baseAddress);
	uint64_t baseAddress;
	PlayerType playerType;

	bool isLocalPlayer = false;
	bool isScav = false;
	bool isAi = false;

	// Bone related stuff
	std::vector<allPlayerBones> boneList = {
		allPlayerBones::HumanPelvis, allPlayerBones::HumanHead, allPlayerBones::HumanLForearm2,
		allPlayerBones::HumanLPalm, allPlayerBones::HumanRForearm2, allPlayerBones::HumanRPalm,
		allPlayerBones::HumanLThigh2, allPlayerBones::HumanLFoot,
		allPlayerBones::HumanRThigh2, allPlayerBones::HumanRFoot,
	};

	std::vector<uint64_t> bonePtrs = std::vector<uint64_t>(boneList.size());

	std::vector<TransformAccessReadOnly> boneTransforms = std::vector<TransformAccessReadOnly>(boneList.size());
	std::vector<TransformData> boneTransformsData = std::vector<TransformData>(boneList.size());
	std::vector<DirectX::SimpleMath::Vector3> bonePositions = std::vector<DirectX::SimpleMath::Vector3>(boneList.size());

	std::vector<PVOID> pMatriciesBuffers = std::vector<PVOID>(boneList.size());
	std::vector<PVOID> pIndicesBuffers = std::vector<PVOID>(boneList.size());

	void UpdateBonePositions();

	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Vector3 GetTransformPosition(int boneIndex);
};