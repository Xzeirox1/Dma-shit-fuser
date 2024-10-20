#include <imgui.h>
#include "Overlay.h"
#include "../PlayerManager.h"
#include "../GameManager.h"
#include <chrono>


void Overlay::HandleCheatOverlay()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(1920, 1080));

	ImGui::Begin("##ESP", (bool*) NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);

	this->ManageTimedUpdateFunctions();

	if (inRaid)
	{
		ImGui::GetBackgroundDrawList()->AddText(ImVec2(0.f, 0.f), ImColor(0, 255, 0), "IN RAID");

		RenderEsp();
		RenderExfils();
	}
	else
	{
		ImGui::GetBackgroundDrawList()->AddText(ImVec2(0.f, 0.f), ImColor(255, 0, 0), "NOT IN RAID");
	}


	ImGui::End();
}

void Overlay::RenderEsp()
{
	for (Player& curPlayer : playerManager.currentPlayerList)
	{
		if (curPlayer.isLocalPlayer)
		{
			continue; // We dont want to draw the bones on our local player
		}

		// calculate the screen positions for each of the player's bones
		std::vector<DirectX::SimpleMath::Vector2> boneScreenPositions = std::vector<DirectX::SimpleMath::Vector2>(curPlayer.bonePositions.size());

		bool offScreenFlag = false;
		for (int i = 0; i < curPlayer.bonePositions.size(); i++)
		{
			Vector2 screenPos = gameManager.WorldToScreen(curPlayer.bonePositions[i]);

			// if bone is offscreen there was a read error and we dont 
			if (screenPos.x == 0 && screenPos.y == 0)
			{
				offScreenFlag = true;
				break;
			}

			boneScreenPositions[i] = screenPos;
		}

		if (offScreenFlag) continue; // skip to next player


		ImColor color;

		// is a player
		if (!curPlayer.isScav)
		{
			color = ImColor(0.f, 1.f, 0.f, 1.f);
		}
		// is a player scav
		else if (curPlayer.isScav && !curPlayer.isAi)
		{
			color = ImColor(1.f, 0.f, 0.f, 1.f);
		}
		// is just a scav
		else if (curPlayer.isScav && curPlayer.isAi)
		{
			color = ImColor(0.f, 0.f, 1.f, 1.f);

		}

		float gameWidth = 1920;
		float gameHeight = 1080;

		// Now we draw the skeleton ESP

		// Head to Pelvis
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::Head].x, boneScreenPositions[boneListIndexes::Head].y),
			ImVec2(boneScreenPositions[boneListIndexes::Pelvis].x, boneScreenPositions[boneListIndexes::Pelvis].y),
			color, 3.f
		);

		// Head to LArm
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::Head].x, boneScreenPositions[boneListIndexes::Head].y),
			ImVec2(boneScreenPositions[boneListIndexes::LForearm].x, boneScreenPositions[boneListIndexes::LForearm].y),
			color, 3.f
		);

		// LArm to LPalm
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::LForearm].x, boneScreenPositions[boneListIndexes::LForearm].y),
			ImVec2(boneScreenPositions[boneListIndexes::LPalm].x, boneScreenPositions[boneListIndexes::LPalm].y),
			color, 3.f
		);

		// Head to RArm
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::Head].x, boneScreenPositions[boneListIndexes::Head].y),
			ImVec2(boneScreenPositions[boneListIndexes::RForearm].x, boneScreenPositions[boneListIndexes::RForearm].y),
			color, 3.f
		);

		// RArm to RPalm
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::RForearm].x, boneScreenPositions[boneListIndexes::RForearm].y),
			ImVec2(boneScreenPositions[boneListIndexes::RPalm].x, boneScreenPositions[boneListIndexes::RPalm].y),
			color, 3.f
		);

		// Pelvis to LThigh
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::Pelvis].x, boneScreenPositions[boneListIndexes::Pelvis].y),
			ImVec2(boneScreenPositions[boneListIndexes::LThigh].x, boneScreenPositions[boneListIndexes::LThigh].y),
			color, 3.f
		);

		// LThigh to LFoot
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::LThigh].x, boneScreenPositions[boneListIndexes::LThigh].y),
			ImVec2(boneScreenPositions[boneListIndexes::LFoot].x, boneScreenPositions[boneListIndexes::LFoot].y),
			color, 3.f
		);

		// Pelvis to RThigh
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::Pelvis].x, boneScreenPositions[boneListIndexes::Pelvis].y),
			ImVec2(boneScreenPositions[boneListIndexes::RThigh].x, boneScreenPositions[boneListIndexes::RThigh].y),
			color, 3.f
		);

		// RThigh to RFoot
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(boneScreenPositions[boneListIndexes::RThigh].x, boneScreenPositions[boneListIndexes::RThigh].y),
			ImVec2(boneScreenPositions[boneListIndexes::RFoot].x, boneScreenPositions[boneListIndexes::RFoot].y),
			color, 3.f
		);

		// now display the distance if we have a reference to the local player's position
		if (localPlayer != nullptr && localPlayer->position != Vector3(0.f, 0.f, 0.f))
		{
			int dist = Vector3::Distance(localPlayer->position, curPlayer.bonePositions[0]);
			std::string toWrite = "Dist: " + std::to_string(dist);
			ImGui::GetWindowDrawList()->AddText(ImVec2((float) boneScreenPositions[0].x, (float) boneScreenPositions[0].y), color, toWrite.c_str());
		}
	}
}

void Overlay::RenderExfils()
{
	if (localPlayer ==  nullptr) return;
	for (ExfilPoint& exfilPoint : gameManager.exfilPoints)
	{
		/*int dist = Vector3::Distance(localPlayer->position, position);
		std::string toWrite = "Extract - Dist: " + std::to_string(dist);*/
		
		Vector2 screenPos = gameManager.WorldToScreen(exfilPoint.position);

		std::string stringToWrite = exfilPoint.exfilName + " : " + std::to_string(exfilPoint.status);

		ImColor color = ImColor(0.f, 1.f, 0.f, 0.5f);

		if (exfilPoint.status != 4)
		{
			color = ImColor(1.f, 0.f, 0.f, 0.5f);
		}

		if (screenPos != Vector2(0, 0))
			ImGui::GetBackgroundDrawList()->AddText(ImVec2(screenPos.x, screenPos.y), color, stringToWrite.c_str());
	}
}