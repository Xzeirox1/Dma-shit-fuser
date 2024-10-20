#include "Player.h"
#include <iostream>
#include "structs.h"
#include "Memory/Memory.h"
#include <vector>

#include "../Offsets.h"


Player::Player(uint64_t baseAddress)
{
	this->baseAddress = baseAddress;

	uint64_t classNamePtr = mem.ReadChain(baseAddress, Offsets::Unity::className);
	UnityString256 nameStruct = mem.Read<UnityString256>(classNamePtr);
	std::string name(nameStruct.name, strnlen(nameStruct.name, sizeof(nameStruct.name)));
	//std::memcpy(className, nameStruct.name, sizeof(className));

	
	// LocalPlayer & ClientPlayer
	if (name == "LocalPlayer" || name == "ClientPlayer")
	{
		//std::cout << "Found LocalPlayer!\n";
		playerType = LocalPlayer;

		playerBoneMatrixPtr = mem.ReadChain(this->baseAddress, Offsets::Player::boneMatrix);
	}
	// ObservedPlayerView
	else if (name == "ObservedPlayerView")
	{
		//std::cout << "Found ObservedPlayerView!\n";
		playerType = ObservedPlayerView;
		playerBoneMatrixPtr = mem.ReadChain(this->baseAddress, Offsets::ObservedPlayerView::boneMatrix);

		INT32 playerSide = mem.Read<INT32>(this->baseAddress + Offsets::ObservedPlayerView::side);

		if (playerSide == 4)
		{
			this->isScav = true;

			this->isAi = mem.Read<bool>(this->baseAddress + Offsets::ObservedPlayerView::isAI);
		}
	}
	else
	{
		std::cout << "Found Unknown type: " << name << "!\n";
		playerType = UNKNOWNERROR;
	}

	// do this at the start. we dont need to re-read the bone ptr 
	// ... addresses every time we update bone positions
	SetupBonePtrs();
}

void Player::SetupBonePtrs()
{
	int index = 0;
	for (auto& curBone : boneList)
	{
		bonePtrs[index] = mem.ReadChain(playerBoneMatrixPtr, { (0x20 + (static_cast<uint64_t>(curBone) * 0x8)), 0x10 });
		index++;
	}
}

// LocalPlayer && ClientPlayer
// A8 (playerBody) ] 28 (skeleton root joint) ] 28 (values) ] 10
// ObservedPlayer
// 0x60 ] 0x28 ] 0x28 ] 0x10
void Player::UpdateBonePositions()
{
	for (int i = 0; i < this->bonePtrs.size(); i++)
	{
		this->bonePositions[i] = GetTransformPosition(i);
	}

	// Set our root position to the first bone
	this->position = bonePositions[0];
}

DirectX::SimpleMath::Vector3 Player::GetTransformPosition(int boneIndex)
{
	__m128 result {};

	const __m128 mulVec0 = { -2.000, 2.000, -2.000, 0.000 };
	const __m128 mulVec1 = { 2.000, -2.000, -2.000, 0.000 };
	const __m128 mulVec2 = { -2.000, -2.000, 2.000, 0.000 };

	// move these variables out of the get transfomr position function and into some scatter reads.
	//TransformAccessReadOnly pTransformAccessReadOnly = mem.Read<TransformAccessReadOnly>(pTransform + 0x38);
	//TransformData transformData = mem.Read<TransformData>(pTransformAccessReadOnly.pTransformData + 0x18);

	TransformAccessReadOnly pTransformAccessReadOnly = boneTransforms[boneIndex];
	TransformData transformData = boneTransformsData[boneIndex];

	SIZE_T sizeMatriciesBuf = sizeof(Matrix34) * pTransformAccessReadOnly.index + sizeof(Matrix34);

	/*SIZE_T sizeMatriciesBuf = sizeof(Matrix34) * pTransformAccessReadOnly.index + sizeof(Matrix34);
	SIZE_T sizeIndicesBuf = sizeof(int) * pTransformAccessReadOnly.index + sizeof(int);*/

	// Allocate memory for storing large amounts of data (matricies and indicies)
	/*PVOID pMatriciesBuf = malloc(sizeMatriciesBuf);
	PVOID pIndicesBuf = malloc(sizeIndicesBuf);*/

	PVOID pMatriciesBuf = this->pMatriciesBuffers[boneIndex];
	PVOID pIndicesBuf = this->pIndicesBuffers[boneIndex];

	if (pMatriciesBuf && pIndicesBuf)
	{
		// Read Matricies array into the buffer
		//mem.Read(transformData.pTransformArray, pMatriciesBuf, sizeMatriciesBuf);
		//Driver.ReadMem(gTargetProcessID, transformData.pTransformArray, pMatriciesBuf, sizeMatriciesBuf);
		// Read Indices array into the buffer
		//mem.Read(transformData.pTransformIndices, pIndicesBuf, sizeIndicesBuf);
		//Driver.ReadMem(gTargetProcessID, transformData.pTransformIndices, pIndicesBuf, sizeIndicesBuf);

		result = *(__m128*)((ULONGLONG) pMatriciesBuf + 0x30 * pTransformAccessReadOnly.index);
		int transformIndex = *(int*) ((ULONGLONG) pIndicesBuf + 0x4 * pTransformAccessReadOnly.index);

		while (transformIndex >= 0)
		{

			Matrix34 matrix34;

			// if we read bad memory when the player leaves a game... check and make sure we aernt accessing 
			// an invalid transform index
			if (transformIndex >= 0 && transformIndex < (sizeMatriciesBuf / sizeof(Matrix34)))
			{
				matrix34 = *(Matrix34*) ((ULONGLONG) pMatriciesBuf + 0x30 * transformIndex);
			}
			else
			{
				// Handle the case when transformIndex is out of bounds
				//std::cout << "Invalid transformIndex: " << transformIndex << std::endl;
				free(pMatriciesBuf);
				free(pIndicesBuf);
				return DirectX::SimpleMath::Vector3(0, 0, 0);
			}

			__m128 xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x00));	// xxxx
			__m128 yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x55));	// yyyy
			__m128 zwxy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x8E));	// zwxy
			__m128 wzyw = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0xDB));	// wzyw
			__m128 zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0xAA));	// zzzz
			__m128 yxwy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x71));	// yxwy
			__m128 tmp7 = _mm_mul_ps(*(__m128*)(&matrix34.vec2), result);

			result = _mm_add_ps(
				_mm_add_ps(
					_mm_add_ps(
						_mm_mul_ps(
							_mm_sub_ps(
								_mm_mul_ps(_mm_mul_ps(xxxx, mulVec1), zwxy),
								_mm_mul_ps(_mm_mul_ps(yyyy, mulVec2), wzyw)),
							_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0xAA))),
						_mm_mul_ps(
							_mm_sub_ps(
								_mm_mul_ps(_mm_mul_ps(zzzz, mulVec2), wzyw),
								_mm_mul_ps(_mm_mul_ps(xxxx, mulVec0), yxwy)),
							_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0x55)))),
					_mm_add_ps(
						_mm_mul_ps(
							_mm_sub_ps(
								_mm_mul_ps(_mm_mul_ps(yyyy, mulVec0), yxwy),
								_mm_mul_ps(_mm_mul_ps(zzzz, mulVec1), zwxy)),
							_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0x00))),
						tmp7)), *(__m128*)(&matrix34.vec0));

			int oldTransformIndex = transformIndex;
			transformIndex = *(int*) ((ULONGLONG) pIndicesBuf + 0x4 * transformIndex);


			// EDIT: this shouldnt happen anymore now that we added checks in the PlayerManager::UpdatePlayerPositions function...
			// but im just going to leave it here because it doesnt hurt to have it
	
			// if we try to read memory of a bone position that no-longer exists
			// like when the player exits a raid: the transform index will equal 0.
			// so if the next tranform index is the same as the old one and they both equal 0
			// we know to break out of our and clear the memory.
			// this is to prevent an infinite while loop
			if (oldTransformIndex == transformIndex && transformIndex == 0)
			{
				//std::cout << "detected infinite bone transform loop due to 0 index... breaking\n";
				break;
			}
		}

		free(pMatriciesBuf);
		free(pIndicesBuf);
	}

	return DirectX::SimpleMath::Vector3(result.m128_f32[0], result.m128_f32[1], result.m128_f32[2]);
}