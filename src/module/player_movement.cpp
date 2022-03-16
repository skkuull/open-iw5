#include <std_include.hpp>
#include "game/game.hpp"

#include "utils/hook.hpp"

#include "player_movement.hpp"

const game::native::dvar_t* player_movement::player_sustainAmmo;
const game::native::dvar_t* player_movement::jump_ladderPushVel;
const game::native::dvar_t* player_movement::jump_height;
const game::native::dvar_t* player_movement::pm_bounces;
const game::native::dvar_t* player_movement::pm_playerEjection;
const game::native::dvar_t* player_movement::pm_playerCollision;
const game::native::dvar_t* player_movement::pm_rocketJump;
const game::native::dvar_t* player_movement::pm_elevators;

void player_movement::pm_weapon_use_ammo(game::native::playerState_s* ps, const game::native::Weapon weapon,
	bool is_alternate, int amount, game::native::PlayerHandIndex hand)
{
	if (!player_movement::player_sustainAmmo->current.enabled)
	{
		game::native::PM_WeaponUseAmmo(ps, weapon, is_alternate, amount, hand);
	}
}

static DWORD bounce_addr = SELECT_VALUE(0x43D91F, 0x424D58, 0x0);
static DWORD dont_bounce_addr = SELECT_VALUE(0x43D933, 0x424D6C, 0x0);
__declspec(naked) void player_movement::pm_step_slide_move_stub()
{
	__asm
	{
		push eax
		mov eax, player_movement::pm_bounces
		cmp byte ptr [eax + 0xC], 1
		pop eax

		// Bounce if enabled is true
		je bounce

		// Code hook skipped
		cmp dword ptr [esp + 0x24], 0
		jnz dont_bounce

	bounce:
		jmp bounce_addr

	dont_bounce:
		jmp dont_bounce_addr
	}
}

int player_movement::stuck_in_client_stub(game::native::gentity_s* self)
{
	if (player_movement::pm_playerEjection->current.enabled)
	{
		return reinterpret_cast<int(*)(game::native::gentity_s*)>(0x4F8930)(self);
	}

	return 0;
}

void player_movement::cm_transformed_capsule_trace_stub(game::native::trace_t* results, const float* start,
	const float* end, const game::native::Bounds* bounds, const game::native::Bounds* capsule, int contents,
	const float* origin, const float* angles)
{
	if (player_movement::pm_playerCollision->current.enabled)
	{
		game::native::CM_TransformedCapsuleTrace(results, start, end,
			bounds, capsule, contents, origin, angles);
	}
}

game::native::gentity_s* player_movement::weapon_rocket_launcher_fire_stub(game::native::gentity_s* ent,
	const game::native::Weapon weapon, float spread, game::native::weaponParms* wp, const float* gun_vel,
	game::native::missileFireParms* fire_parms, game::native::missileFireParms* magic_bullet)
{
	auto* result = game::native::Weapon_RocketLauncher_Fire(ent, weapon, spread, wp,
		gun_vel, fire_parms, magic_bullet);

	if (ent->client != nullptr && player_movement::pm_rocketJump->current.enabled)
	{
		ent->client->ps.velocity[0] += (0 - wp->forward[0]) * 64.0f;
		ent->client->ps.velocity[1] += (0 - wp->forward[1]) * 64.0f;
		ent->client->ps.velocity[2] += (0 - wp->forward[2]) * 64.0f;
	}

	return result;
}

void player_movement::pm_player_trace_stub(game::native::pmove_t* pm, game::native::trace_t* results,
	const float* start, const float* end, const game::native::Bounds* bounds, int pass_entity_num, int content_mask)
{
	game::native::PM_playerTrace(pm, results, start, end, bounds, pass_entity_num, content_mask);

	if (player_movement::pm_elevators->current.enabled)
	{
		results->startsolid = false;
	}
}

void player_movement::pm_trace_stub(const game::native::pmove_t* pm, game::native::trace_t* results,
	const float* start, const float* end, const game::native::Bounds* bounds, int pass_entity_num, int content_mask)
{
	game::native::PM_trace(pm, results, start, end, bounds, pass_entity_num, content_mask);

	if (player_movement::pm_elevators->current.enabled)
	{
		results->allsolid = false;
	}
}

static DWORD push_off_ladder_addr = SELECT_VALUE(0x63EA4C, 0x41686C, 0x0);
__declspec(naked) void player_movement::jump_push_off_ladder_stub()
{
	__asm
	{
		push eax
		mov eax, player_movement::jump_ladderPushVel
		fld dword ptr [eax + 0xC]
		pop eax

		jmp push_off_ladder_addr
	}
}

static DWORD jump_start_addr = 0x41696F;
void player_movement::jump_start_stub()
{
	__asm
	{
		push eax
		mov eax, player_movement::jump_height
		fld dword ptr [eax + 0xC]
		pop eax

		jmp jump_start_addr
	}
}

const game::native::dvar_t* player_movement::dvar_register_player_sustain_ammo(const char* dvar_name,
		bool value, unsigned __int16 /*flags*/, const char* description)
{
	player_movement::player_sustainAmmo = game::native::Dvar_RegisterBool(dvar_name,
		value, game::native::DVAR_CODINFO, description);

	return player_movement::player_sustainAmmo;
}

const game::native::dvar_t* player_movement::dvar_register_jump_ladder_push_vel(const char* dvar_name,
	float value, float min, float max, unsigned __int16 /*flags*/, const char* description)
{
	player_movement::jump_ladderPushVel = game::native::Dvar_RegisterFloat(dvar_name,
		value, min, max, game::native::DVAR_CODINFO, description);

	return player_movement::jump_ladderPushVel;
}

const game::native::dvar_t* player_movement::dvar_register_jump_height(const char* dvar_name,
	float value, float min, float max, unsigned __int16 /*flags*/, const char* description)
{
	player_movement::jump_height = game::native::Dvar_RegisterFloat(dvar_name,
		value, min, max, game::native::DVAR_CODINFO, description);

	return player_movement::jump_height;
}

void player_movement::patch_mp()
{
	player_movement::pm_playerEjection = game::native::Dvar_RegisterBool("pm_playerEjection",
		true, game::native::DVAR_CODINFO, "Push intersecting players away from each other");
	player_movement::pm_rocketJump = game::native::Dvar_RegisterBool("pm_rocketJump",
		false, game::native::DVAR_CODINFO, "CoD4 rocket jumps");

	utils::hook(0x418D9C, &player_movement::dvar_register_player_sustain_ammo, HOOK_CALL).install()->quick();
	utils::hook(0x4160A7, &player_movement::dvar_register_jump_ladder_push_vel, HOOK_CALL).install()->quick();
	utils::hook(0x41602B, &player_movement::dvar_register_jump_height, HOOK_CALL).install()->quick();

	utils::hook(0x42B5DA, &player_movement::pm_weapon_use_ammo, HOOK_CALL).install()->quick();
	utils::hook(0x42B2BD, &player_movement::pm_weapon_use_ammo, HOOK_CALL).install()->quick();
	utils::hook(0x42AE95, &player_movement::pm_weapon_use_ammo, HOOK_CALL).install()->quick();

	utils::hook(0x424D51, &player_movement::pm_step_slide_move_stub, HOOK_JUMP).install()->quick();

	utils::hook(0x4F9EFB, &player_movement::stuck_in_client_stub, HOOK_CALL).install()->quick(); // ClientEndFrame
	utils::hook(0x57CF45, &player_movement::cm_transformed_capsule_trace_stub, HOOK_CALL).install()->quick(); // SV_ClipMoveToEntity
	utils::hook(0x482C1B, &player_movement::cm_transformed_capsule_trace_stub, HOOK_CALL).install()->quick(); // CG_ClipMoveToEntity

	utils::hook(0x530CCB, &player_movement::weapon_rocket_launcher_fire_stub, HOOK_CALL).install()->quick(); // FireWeapon

	utils::hook(0x422861, &player_movement::pm_player_trace_stub, HOOK_CALL).install()->quick(); // PM_JitterPoint
	utils::hook(0x4228B5, &player_movement::pm_player_trace_stub, HOOK_CALL).install()->quick(); // PM_JitterPoint

	utils::hook(0x41F995, &player_movement::pm_trace_stub, HOOK_CALL).install()->quick(); // PM_CheckDuck
	utils::hook(0x41F8D8, &player_movement::pm_trace_stub, HOOK_CALL).install()->quick(); // PM_CheckDuck
	utils::hook(0x41F941, &player_movement::pm_trace_stub, HOOK_CALL).install()->quick(); // PM_CheckDuck

	utils::hook(0x416866, &player_movement::jump_push_off_ladder_stub, HOOK_JUMP).install()->quick(); // Jump_Check
	utils::hook::nop(0x41686B, 1); // Nop skipped opcode

	utils::hook(0x416969, &player_movement::jump_start_stub, HOOK_JUMP).install()->quick(); // Jump_Check
	utils::hook::nop(0x41696E, 1); // Nop skipped opcode
}

void player_movement::patch_sp()
{
	player_movement::player_sustainAmmo = game::native::Dvar_RegisterBool("player_sustainAmmo",
		false, game::native::DVAR_CODINFO, "Firing weapon will not decrease clip ammo");
	player_movement::jump_ladderPushVel = game::native::Dvar_RegisterFloat("jump_ladderPushVel",
		128.0f, 0.0f, 1024.0f, game::native::DVAR_CODINFO, "The velocity of a jump off of a ladder");

	utils::hook(0x648C3A, &player_movement::pm_weapon_use_ammo, HOOK_CALL).install()->quick();
	utils::hook(0x64891D, &player_movement::pm_weapon_use_ammo, HOOK_CALL).install()->quick();
	utils::hook(0x6484E2, &player_movement::pm_weapon_use_ammo, HOOK_CALL).install()->quick();

	utils::hook(0x43D918, &player_movement::pm_step_slide_move_stub, HOOK_JUMP).install()->quick();

	utils::hook(0x41F9A6, &player_movement::cm_transformed_capsule_trace_stub, HOOK_CALL).install()->quick(); // SV_ClipMoveToEntity
	utils::hook(0x57B14F, &player_movement::cm_transformed_capsule_trace_stub, HOOK_CALL).install()->quick(); // CG_ClipMoveToEntity

	utils::hook(0x643F84, &player_movement::pm_player_trace_stub, HOOK_CALL).install()->quick(); // PM_JitterPoint
	utils::hook(0x643FDB, &player_movement::pm_player_trace_stub, HOOK_CALL).install()->quick(); // PM_JitterPoint

	utils::hook(0x64181A, &player_movement::pm_trace_stub, HOOK_CALL).install()->quick(); // PM_CheckDuck
	utils::hook(0x641701, &player_movement::pm_trace_stub, HOOK_CALL).install()->quick(); // PM_CheckDuck
	utils::hook(0x6417A9, &player_movement::pm_trace_stub, HOOK_CALL).install()->quick(); // PM_CheckDuck

	utils::hook(0x63EA46, player_movement::jump_push_off_ladder_stub, HOOK_JUMP).install()->quick(); // Jump_Check
	utils::hook::nop(0x63EA4B, 1); // Nop skipped opcode
}

void player_movement::post_load()
{
	// Un-cheat missileMacross. It seems it retained its functionality
	utils::hook::set<BYTE>(SELECT_VALUE(0x44DFED, 0x50DDDD, 0x48C16F), 0x0);

	if (game::is_dedi())
	{
		return;
	}

	player_movement::pm_bounces = game::native::Dvar_RegisterBool("pm_bounces", false,
		game::native::dvar_flags::DVAR_CODINFO, "CoD4 Bounces");
	player_movement::pm_playerCollision = game::native::Dvar_RegisterBool("pm_playerCollision",
		true, game::native::DVAR_CODINFO, "Push intersecting players away from each other");
	player_movement::pm_elevators = game::native::Dvar_RegisterBool("pm_elevators",
		false, game::native::DVAR_CODINFO, "CoD4 Elevators");

	if (game::is_mp()) this->patch_mp();
	else if (game::is_sp()) this->patch_sp();
}

REGISTER_MODULE(player_movement);
