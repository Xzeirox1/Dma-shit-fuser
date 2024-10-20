#include <iostream>
#include "GameManager.h"
#include "Offsets.h"
#include "structs and stuff/structs.h"
#include "PlayerManager.h"
#include "Memory/Memory.h"

bool GameManager::GetGOMAddress()
{
	try
	{
		//std::cout << "[GameManager] Looking for GOM from base address 0x" << std::hex << baseAddy << "\n";
		uint64_t gomAddress = mem.Read<uint64_t>(baseAddy + Offsets::GOM);
		GOM = mem.Read<GameObjectManager>(gomAddress);
		//std::cout << "[GameManager] Found GOM ==> 0x" << std::hex << gomAddress << "\n";
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "[GameManager] Unable to find GOM: " << std::dec << e.what() << "\n";
		return false;
	};
}

bool GameManager::GetLGWAddress()
{
	try
	{
		//std::cout << "[GameManager] Looking for LGW address from GOM nodes\n";
		uint64_t activeNodes = mem.Read<uint64_t>(GOM.ActiveNodes);
		uint64_t lastActiveNode = mem.Read<uint64_t>(GOM.LastActiveNode);

		gameWorldAddress = GetObjectFromList(activeNodes, lastActiveNode, "GameWorld");

		if (gameWorldAddress == 0)
		{
			throw std::exception("Game world address is 0");
		}

		localGameWorldAddress = mem.ReadChain(gameWorldAddress, Offsets::LGW);

		//std::cout << "[GameManager] Got LGW at address ==> 0x" << std::hex << localGameWorldAddress << "\n";
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "[GameManager] Unable to find LGW: " << std::dec << e.what() << "\n";
		return false;
	}
}

bool GameManager::GetCameras()
{
	struct cameras
	{
		uint64_t list;
		uint64_t min;
		uint64_t count;
		uint64_t max;
	};

	FPSCameraAddy = 0;

	AllCamerasAddy = mem.Read<uint64_t>(baseAddy + Offsets::ALLCAMS);
	cameras allCams = mem.Read<cameras>(AllCamerasAddy);

	if (allCams.count == 0)
	{
		std::cout << "[GameManager] Failed to get cameras... Length was 0";
		return false;
	}

	//std::cout << "[GameManager] Found " << std::dec << allCams.count << " cameras...\n";

	for (int i = 0; i < allCams.count; i++)
	{
		uint64_t camera = mem.Read<uint64_t>(allCams.list + ((uint64_t) i * 0x8));
		if (camera != NULL)
		{
			uint64_t cameraObject = mem.Read<uint64_t>(camera + 0x30);
			uint64_t cameraNamePtr = mem.Read<uint64_t>(cameraObject + 0x60);

			if (!cameraNamePtr)
				return false; // Reached end of list

			char name[64];
			for (int j = 0; j < sizeof(name); j++)
			{
				name[j] = mem.Read<char>(cameraNamePtr + j);
				if (name[j] == NULL)
					break;
			}

			if (strcmp(name, "FPS Camera") == 0)
			{
				FPSCameraAddy = cameraObject;
				//std::cout << "[GameManager] Got FPS camera at address 0x" << std::hex << cameraObject << "\n";
				return true;
			}
		}
	}

	// we have reached the end of the loop without finding the FPS camera... Return false
	return false;
}

bool GameManager::GetRegisteredPlayersAddress()
{
	uint64_t registeredPlayers = mem.Read<uint64_t>(gameManager.localGameWorldAddress + Offsets::LocalGameWorld::RegisteredPlayers);

	if (registeredPlayers == 0x0)
	{
		std::cout << "[GameManager] failed to find registered players\n";
		return false;
	}

	std::cout << std::hex << "[GameManager] found registered players list at 0x" << registeredPlayers << "\n";
	playerManager.registeredPlayers = registeredPlayers;
	return true;
}

bool GameManager::GetExfilPoints()
{
	exfilPoints.clear();

	uint64_t exfilManagerAddress = mem.Read<uint64_t>(gameManager.localGameWorldAddress + Offsets::LocalGameWorld::ExfilManager);
	
	if (exfilManagerAddress == 0) return false;

	uint64_t exfiltrationPointArray = mem.Read<uint64_t>(exfilManagerAddress + Offsets::LocalGameWorld::Exfil::ExfilPointArray);
	if (exfiltrationPointArray == 0) return false;

	int numExfilPoints = mem.Read<int>(exfiltrationPointArray + 0x18);
	if (numExfilPoints == 0) return false;

	std::cout << "num exfil points: " << std::dec << numExfilPoints << "\n";

	for (int i = 0; i < numExfilPoints; i++)
	{
		uint64_t exfilPointAddy = mem.Read<uint64_t>(exfiltrationPointArray + 0x20 + (i * 0x8));

		ExfilPoint newPoint = ExfilPoint(exfilPointAddy);

		exfilPoints.push_back(newPoint);
	}

	return true;
}

bool GameManager::CheckInRaid()
{
	// Init
	std::cout << "[GameManager] Checking raid status with GOM, LGW, and Cameras...\n";
	if (!GetGOMAddress()) return false;
	if (!GetLGWAddress()) return false;
	if (!GetCameras()) return false;
	if (!FindViewMatrix()) return false;
	if (!GetRegisteredPlayersAddress()) return false;
	if (!GetExfilPoints()) return false;
	std::cout << "[GameManager] All attributes found! We are in raid!\n";
	return true;
}

uint64_t GameManager::GetObjectFromList(uint64_t listPtr, uint64_t lastObjectPtr, const std::string& objectName)
{
	BaseObject activeObject = mem.Read<BaseObject>(listPtr);
	BaseObject lastObject = mem.Read<BaseObject>(lastObjectPtr);

	if (activeObject.object != 0x0)
	{
		while (activeObject.object != 0 && activeObject.object != lastObject.object)
		{
			uint64_t classNamePtr = mem.Read<uint64_t>(activeObject.object + 0x60);
			UnityString256 nameStruct = mem.Read<UnityString256>(classNamePtr);
			std::string name(nameStruct.name, strnlen(nameStruct.name, sizeof(nameStruct.name)));

				
			if (name == objectName)
				return activeObject.object;

			activeObject = mem.Read<BaseObject>(activeObject.nextObjectLink);
		}
	}
	if (lastObject.object != 0x0)
	{
		uint64_t classNamePtr = mem.Read<uint64_t>(lastObject.object + 0x60);
		UnityString256 nameStruct = mem.Read<UnityString256>(classNamePtr);
		std::string name(nameStruct.name, strnlen(nameStruct.name, sizeof(nameStruct.name)));

		if (name == objectName)
			return lastObject.object;
	}

	return 0;
}

bool GameManager::FindViewMatrix()
{
	try
	{
		std::cout << "[GameManager] Looking for view matrix\n";
		uint64_t base = mem.ReadChain(FPSCameraAddy, Offsets::viewMatrix);
		base += 0xDC;
		matrixFpsAddress = base;
		std::cout << "[GameManager] Got view matrix address at: 0x" << std::hex << base << "\n";

		return (base != 0x0);
	}
	catch (std::exception& e)
	{
		std::cout << "Failed to find view matrix: " << e.what() << "\n";
		return false;
	}
}

Matrix previousMatrixFPS;

// Declare a variable to store the previous matrix
void GameManager::ReadViewMatrix()
{
	Matrix tempMatrixFPS = mem.Read<Matrix>(matrixFpsAddress);

	if (IsMatrixDifferenceAboveThreshold(tempMatrixFPS, previousMatrixFPS, 0.5f))
	{
		MatrixFPS = SmoothMatrix(previousMatrixFPS, tempMatrixFPS, 0.3f);
		previousMatrixFPS = MatrixFPS;
	}
	else
	{
		MatrixFPS = previousMatrixFPS;
	}
}

bool GameManager::IsMatrixDifferenceAboveThreshold(const Matrix& matrix1, const Matrix& matrix2, float threshold)
{
	return (std::abs(matrix1._11 - matrix2._11) > threshold ||
		std::abs(matrix1._12 - matrix2._12) > threshold ||
		std::abs(matrix1._13 - matrix2._13) > threshold ||
		std::abs(matrix1._14 - matrix2._14) > threshold ||
		std::abs(matrix1._21 - matrix2._21) > threshold ||
		std::abs(matrix1._22 - matrix2._22) > threshold ||
		std::abs(matrix1._23 - matrix2._23) > threshold ||
		std::abs(matrix1._24 - matrix2._24) > threshold ||
		std::abs(matrix1._31 - matrix2._31) > threshold ||
		std::abs(matrix1._32 - matrix2._32) > threshold ||
		std::abs(matrix1._33 - matrix2._33) > threshold ||
		std::abs(matrix1._34 - matrix2._34) > threshold ||
		std::abs(matrix1._41 - matrix2._41) > threshold ||
		std::abs(matrix1._42 - matrix2._42) > threshold ||
		std::abs(matrix1._43 - matrix2._43) > threshold ||
		std::abs(matrix1._44 - matrix2._44) > threshold);
}

Matrix GameManager::SmoothMatrix(const Matrix& previousMatrix, const Matrix& currentMatrix, float smoothingFactor)
{
	Matrix smoothedMatrix;
	smoothedMatrix._11 = smoothingFactor * previousMatrix._11 + (1.0f - smoothingFactor) * currentMatrix._11;
	smoothedMatrix._12 = smoothingFactor * previousMatrix._12 + (1.0f - smoothingFactor) * currentMatrix._12;
	smoothedMatrix._13 = smoothingFactor * previousMatrix._13 + (1.0f - smoothingFactor) * currentMatrix._13;
	smoothedMatrix._14 = smoothingFactor * previousMatrix._14 + (1.0f - smoothingFactor) * currentMatrix._14;
	smoothedMatrix._21 = smoothingFactor * previousMatrix._21 + (1.0f - smoothingFactor) * currentMatrix._21;
	smoothedMatrix._22 = smoothingFactor * previousMatrix._22 + (1.0f - smoothingFactor) * currentMatrix._22;
	smoothedMatrix._23 = smoothingFactor * previousMatrix._23 + (1.0f - smoothingFactor) * currentMatrix._23;
	smoothedMatrix._24 = smoothingFactor * previousMatrix._24 + (1.0f - smoothingFactor) * currentMatrix._24;
	smoothedMatrix._31 = smoothingFactor * previousMatrix._31 + (1.0f - smoothingFactor) * currentMatrix._31;
	smoothedMatrix._32 = smoothingFactor * previousMatrix._32 + (1.0f - smoothingFactor) * currentMatrix._32;
	smoothedMatrix._33 = smoothingFactor * previousMatrix._33 + (1.0f - smoothingFactor) * currentMatrix._33;
	smoothedMatrix._34 = smoothingFactor * previousMatrix._34 + (1.0f - smoothingFactor) * currentMatrix._34;
	smoothedMatrix._41 = smoothingFactor * previousMatrix._41 + (1.0f - smoothingFactor) * currentMatrix._41;
	smoothedMatrix._42 = smoothingFactor * previousMatrix._42 + (1.0f - smoothingFactor) * currentMatrix._42;
	smoothedMatrix._43 = smoothingFactor * previousMatrix._43 + (1.0f - smoothingFactor) * currentMatrix._43;
	smoothedMatrix._44 = smoothingFactor * previousMatrix._44 + (1.0f - smoothingFactor) * currentMatrix._44;
	return smoothedMatrix;
}

Vector2 GameManager::WorldToScreen(const Vector3 position)
{
	Matrix m = MatrixTranspose(MatrixFPS);
	Vector3 translationVector = Vector3(m._41, m._42, m._43);
	Vector3 up = Vector3(m._21, m._22, m._23);
	Vector3 right = Vector3(m._11, m._12, m._13);

	float w = translationVector.Dot(position) + m._44;

	if (w < 0.098f)
		return Vector2(0.f, 0.f);

	float y = up.Dot(position) + m._24;
	float x = right.Dot(position) + m._14;

	int sizeRight = 1920;
	int sizeBottom = 1080;
	return Vector2((sizeRight / 2) * (1.f + x / w), (sizeBottom / 2) * (1.f - y / w));
}

Matrix GameManager::MatrixTranspose(const Matrix pM)
{
	Matrix pOut = {};
	pOut._11 = pM._11;
	pOut._12 = pM._21;
	pOut._13 = pM._31;
	pOut._14 = pM._41;

	pOut._21 = pM._12;
	pOut._22 = pM._22;
	pOut._23 = pM._32;
	pOut._24 = pM._42;

	pOut._31 = pM._13;
	pOut._32 = pM._23;
	pOut._33 = pM._33;
	pOut._34 = pM._43;

	pOut._41 = pM._14;
	pOut._42 = pM._24;
	pOut._43 = pM._34;
	pOut._44 = pM._44;

	return pOut;
}