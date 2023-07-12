#include "memory.h"
#include "vector.h"
#include <thread>
#include <iostream>

namespace offset
{
	//client
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDEA98C;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4DFFF7C;

	//engine
	constexpr ::std::ptrdiff_t dwClientState = 0x59F19C;
	constexpr ::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;

	//entity
	constexpr ::std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
	constexpr ::std::ptrdiff_t m_bDormant = 0xED;
	constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr ::std::ptrdiff_t m_iHealth = 0x100;
	constexpr ::std::ptrdiff_t m_vecOrigin = 0x138;
	constexpr ::std::ptrdiff_t m_vecViewOffset = 0x108;
	constexpr ::std::ptrdiff_t m_aimPunchAngle = 0x303C;
	constexpr ::std::ptrdiff_t m_bSpottedByMask = 0x980;

	//username
	constexpr ::std::ptrdiff_t m_iAccountID = 0x2FD8;
	
}

constexpr Vector3 CalcAngle(
	const Vector3& localPos,
	const Vector3& enemyPos,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPos - localPos).ToAngle() - viewAngles);
}

int main()
{
	//memory class initialization
	const auto memory = Memory{ "csgo.exe" };

	//addresses for modules
	const auto client = memory.GetModuleAddress("client.dll");
	const auto engine = memory.GetModuleAddress("engine.dll");

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		//key for aimbot
		if (!GetAsyncKeyState(VK_RBUTTON))
			continue;

		//get local player
		const auto& localPlayer = memory.Read<std::uintptr_t>(client + offset::dwLocalPlayer);
		const auto& localTeam= memory.Read<std::int32_t>(localPlayer+ offset::m_iTeamNum);

		//eye position = origin + view offset
		const auto localEyePos = memory.Read<Vector3>(localPlayer + offset::m_vecOrigin) +
			memory.Read<Vector3>(localPlayer + offset::m_vecViewOffset);

		const auto& clientState = memory.Read<std::uintptr_t>(engine + offset::dwClientState);

		const auto& viewAngles = memory.Read<Vector3>(clientState + offset::dwClientState_ViewAngles);

		const auto& aimPunch = memory.Read<Vector3>(localPlayer + offset::m_aimPunchAngle) * 2;

		//fov of aimbot

		auto botFOV = 5.f;
		auto botAngle = Vector3{};

		for (auto i = 0; i <= 32; ++i)
		{
			const auto& player = memory.Read<std::uintptr_t>(client + offset::dwEntityList + i * 0x10);

			//filter teammates
			if (memory.Read<std::int32_t>(player + offset::m_iTeamNum) == localTeam)
				continue;

			//filter dormant entities
			if (memory.Read<bool>(player + offset::m_bDormant))
				continue;

			//filter dead players
			if (!memory.Read<std::uint32_t>(player + offset::m_iHealth))
				continue;

			//filter non-spotted players
			if (!memory.Read<bool>(player + offset::m_bSpottedByMask))
				continue;

			//get bone matrix
			const auto boneMatrix = memory.Read<std::uintptr_t>(player + offset::m_dwBoneMatrix);

			//get head pos in 3D space
			const auto playerHeadPos = Vector3(
				memory.Read<float>(boneMatrix + 0x30 * 8 + 0x0C),
				memory.Read<float>(boneMatrix + 0x30 * 8 + 0x1C),
				memory.Read<float>(boneMatrix + 0x30 * 8 + 0x2C)
			);
			
			//get proper angle and fov
			const auto angle = CalcAngle(localEyePos, playerHeadPos, viewAngles + aimPunch);

			const auto fov = std::hypot(angle.x, angle.y);

			if (fov < botFOV)
			{
				botFOV = fov;
				botAngle = angle;
			}
		}
		//if valid target
		if (!botAngle.IsZero())
		{
			memory.Write<Vector3>(clientState + offset::dwClientState_ViewAngles, viewAngles + botAngle / 5.f);
		}
	}
	return 0;
}