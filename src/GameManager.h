#pragma once
#include "SimpleMath.h"
#include "structs and stuff/structs.h"
#include "structs and stuff/ExfilPoint.h"
using namespace DirectX::SimpleMath;


class GameManager
{
public:
	/*
	Game world
	*/
	bool GetGOMAddress();
	bool GetLGWAddress();
	bool GetCameras();
	bool GetRegisteredPlayersAddress();
	bool GetExfilPoints();

	bool CheckInRaid();

	/*
	Cameras & Matrix
	*/
	uint64_t matrixFpsAddress { 0 };
	Matrix MatrixFPS = {};
	bool FindViewMatrix();
	void ReadViewMatrix();
	Vector2 WorldToScreen(const Vector3 position);
	Matrix MatrixTranspose(const Matrix pM);

	bool IsMatrixDifferenceAboveThreshold(const Matrix& matrix1, const Matrix& matrix2, float threshold);
	Matrix SmoothMatrix(const Matrix& previousMatrix, const Matrix& currentMatrix, float smoothingFactor);

	/*
	Other
	*/
	std::vector<ExfilPoint> exfilPoints;
	uint64_t GetObjectFromList(uint64_t listPtr, uint64_t lastObjectPtr, const std::string& objectName);
	uint64_t baseAddy { 0 };
	GameObjectManager GOM;
	uint64_t gameWorldAddress { 0 };
	uint64_t localGameWorldAddress { 0 };

	uint64_t AllCamerasAddy { 0 };
	uint64_t FPSCameraAddy { 0 };

};

inline GameManager gameManager;