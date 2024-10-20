#pragma once
#include <cstdint>
#include <SimpleMath.h>
#include <string>

class ExfilPoint
{
private:
	uint64_t baseAddress;
	uint64_t transformInternal { 0 };
	uint64_t exitTriggerSettingsAddress { 0 };

	void ReadExfilName();
public:
	
	ExfilPoint(uint64_t baseAddress);
	DirectX::SimpleMath::Vector3 position;

	int status { 0 };

	std::string exfilName;
};