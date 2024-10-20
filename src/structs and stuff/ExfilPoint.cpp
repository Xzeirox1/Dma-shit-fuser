#include "ExfilPoint.h"
#include "Memory/Memory.h"
#include "structs.h"
#include "../Offsets.h"

ExfilPoint::ExfilPoint(uint64_t baseAddress)
{
	this->baseAddress = baseAddress;
	std::cout << "[exfilpoint] new point at 0x" << std::hex << baseAddress << "\n";
	this->transformInternal = mem.ReadChain(baseAddress, Offsets::LocalGameWorld::Exfil::transformInternalChain);

	this->position = UnityHelper::GetTransformPosition(transformInternal);

	this->status = mem.Read<int>(baseAddress + Offsets::LocalGameWorld::Exfil::ExfilPoint::status);
	std::cout << "[exfilpoint] status: " << std::dec << this->status << "\n";

	this->exitTriggerSettingsAddress = mem.Read<uint64_t>(baseAddress + 0x58);

	ReadExfilName();
}

void ExfilPoint::ReadExfilName()
{
	uint64_t nameAddress = mem.Read<uint64_t>(exitTriggerSettingsAddress + 0x10);

	if (nameAddress != 0)
	{
		int count = mem.Read<int>(nameAddress + 0x10);
		std::vector<char> buffer;
		buffer.reserve(count);

		for (int i = 0; i < count; i++)
		{
			char c = mem.Read<char>(nameAddress + 0x14 + (i * 0x2));
			buffer.push_back(c);
		}

		std::string name(buffer.begin(), buffer.end());
		std::replace(name.begin(), name.end(), '_', ' ');

		this->exfilName = name;
		std::cout << "[exfilpoint] name: " << name << "\n";
	}
	else
	{
		this->exfilName = "Unknown Extract Name";
	}
}
