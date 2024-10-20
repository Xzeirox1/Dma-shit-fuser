#include "structs.h"

namespace UnityHelper
{
	DirectX::SimpleMath::Vector3 GetTransformPosition(uint64_t transformInternalAddress)
	{
		__m128 result {};

		const __m128 mulVec0 = { -2.000, 2.000, -2.000, 0.000 };
		const __m128 mulVec1 = { 2.000, -2.000, -2.000, 0.000 };
		const __m128 mulVec2 = { -2.000, -2.000, 2.000, 0.000 };

		TransformAccessReadOnly pTransformAccessReadOnly = mem.Read<TransformAccessReadOnly>(transformInternalAddress + 0x38);
		TransformData transformData = mem.Read<TransformData>(pTransformAccessReadOnly.pTransformData + 0x18);

		SIZE_T sizeMatriciesBuf = sizeof(Matrix34) * pTransformAccessReadOnly.index + sizeof(Matrix34);
		SIZE_T sizeIndicesBuf = sizeof(int) * pTransformAccessReadOnly.index + sizeof(int);

		// Allocate memory for storing large amounts of data (matricies and indicies)
		PVOID pMatriciesBuf = malloc(sizeMatriciesBuf);
		PVOID pIndicesBuf = malloc(sizeIndicesBuf);

		if (pMatriciesBuf && pIndicesBuf)
		{
			// Read Matricies array into the buffer
			mem.Read(transformData.pTransformArray, pMatriciesBuf, sizeMatriciesBuf);
			//Driver.ReadMem(gTargetProcessID, transformData.pTransformArray, pMatriciesBuf, sizeMatriciesBuf);
			// Read Indices array into the buffer
			mem.Read(transformData.pTransformIndices, pIndicesBuf, sizeIndicesBuf);
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
					std::cout << "Invalid transformIndex: " << transformIndex << std::endl;
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
					std::cout << "detected infinite bone transform loop due to 0 index... breaking\n";
					break;
				}
			}

			free(pMatriciesBuf);
			free(pIndicesBuf);
		}

		return DirectX::SimpleMath::Vector3(result.m128_f32[0], result.m128_f32[1], result.m128_f32[2]);
	}
}