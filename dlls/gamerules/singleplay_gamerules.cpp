//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "skill.h"
#include "items.h"
#include "game.h"
#include "pm_shared.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameRulesSinglePlay::CGameRulesSinglePlay(void) : CGameRules()
{
	//m_bPersistBetweenMaps = true;?
}

//-----------------------------------------------------------------------------
// Purpose: Runs every server frame, should handle any timer tasks, periodic events, etc.
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::StartFrame(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize HUD (client data) for a client
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::InitHUD(CBasePlayer *pPlayer)
{
	UpdateGameMode(NULL);// XDM3038: send to "everyone" so the bot DLL will not ignore this update
}

//-----------------------------------------------------------------------------
// Purpose: A network client is connecting
// Input  : *pEntity -
//			*pszName -
//			*pszAddress -
//			] -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	pEntity->v.team = TEAM_NONE;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when player picks up a new weapon
// Input  : *pPlayer - 
//			*pWeapon - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
	if (!pWeapon->CanDeploy())// maybe && pPlayer->CanDeployItem(pWeapon)? or not...
		return false;// that weapon can't deploy anyway.

	if (pPlayer->m_pActiveItem == NULL)
		return true;// player doesn't have an active item!

	if (!pPlayer->m_pActiveItem->CanHolster())
		return false;

	//if (pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight())
	//	return true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : float
//-----------------------------------------------------------------------------
float CGameRulesSinglePlay::GetPlayerFallDamage(CBasePlayer *pPlayer)
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	// XDM3038c: WTF?! pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return (pPlayer->m_flFallVelocity - PLAYER_MAX_SAFE_FALL_SPEED) * DAMAGE_FOR_FALL_SPEED;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::PlayerSpawn(CBasePlayer *pPlayer)
{
	// >>CBasePlayer::Spawn	PlayerUseSpawnSpot(pPlayer, false);// XDM

	CBaseEntity	*pWeaponEntity = NULL;
	while ((pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip")) != NULL)
		pWeaponEntity->Touch(pPlayer);

	FireTargets("game_playerspawn", pPlayer, pPlayer, USE_TOGGLE, 0);// XDM3038c: someone might be using this in SP
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::PlayerThink(CBasePlayer *pPlayer)
{
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038 Are players allowed to shoot while climbing?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FAllowShootingOnLadder(CBasePlayer *pPlayer)
{
	if (gSkillData.iSkillLevel >= SKILL_HARD)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: how many points awarded to anyone that kills this entity
// Input  : *pAttacker - 
//			*pKilled - 
// Output : int
//-----------------------------------------------------------------------------
int CGameRulesSinglePlay::IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled)
{
	if (pKilled && pKilled->IsMonster())
		return SCORE_AWARD_NORMAL;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A player got killed
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
}

//-----------------------------------------------------------------------------
// Purpose: A monster got killed // XDM3035
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	if (pKiller)
		pKiller->pev->frags += IPointsForKill(pKiller, (CBasePlayer *)pVictim);
}

//-----------------------------------------------------------------------------
// Purpose: Work out what killed the player, and send a message to all clients about it
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CGameRulesSinglePlay::DeathNotice(CBaseEntity *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
}

//-----------------------------------------------------------------------------
// Purpose: Player picked up a weapon
// Input  : *pPlayer - 
//			*pWeapon - 
//-----------------------------------------------------------------------------
/*void CGameRulesSinglePlay::PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
}*/

//-----------------------------------------------------------------------------
// Purpose: what is the time in the future at which this weapon may spawn?
// Input  : *pWeapon - 
// Output : float
//-----------------------------------------------------------------------------
float CGameRulesSinglePlay::GetWeaponRespawnDelay(const CBasePlayerItem *pWeapon)
{
	return -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Weapon tries to respawn, calculate desired time for it
// Input  : *pWeapon -
// Output : float - the DELAY after which it can try to spawn again (0 == now, -1 == fail)
//-----------------------------------------------------------------------------
float CGameRulesSinglePlay::OnWeaponTryRespawn(CBasePlayerItem *pWeapon)
{
	return -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
// Input  : *pWeapon - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CGameRulesSinglePlay::GetWeaponRespawnSpot(CBasePlayerItem *pWeapon)
{
	return pWeapon->pev->origin;
}

//-----------------------------------------------------------------------------
// Purpose: Any conditions inhibiting the respawning of this weapon?
// Input  : *pWeapon - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FWeaponShouldRespawn(const CBasePlayerItem *pWeapon)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::CanHaveItem(CBasePlayer *pPlayer, CBaseEntity *pItem)
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player drop specified item?
// Warning: Assumes the item is in player's inventory!
// Input  : *pPlayer - player instance
//			*pItem - existing item instance
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::CanDropPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	if (pPlayer == NULL || pItem == NULL)
		return false;

	if (FBitSet(pItem->iFlags(), ITEM_FLAG_CANNOTDROP))// XDM3038c: disallow dropping in single, but allow in MP and CoOp
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pItem - 
//-----------------------------------------------------------------------------
/*void CGameRulesSinglePlay::PlayerGotItem(CBasePlayer *pPlayer, CBaseEntity *pItem)
{
}*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FItemShouldRespawn(const CBaseEntity *pItem)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: After which delay should this Item respawn?
// Input  : *pItem -
// Output : float - delay in seconds
//-----------------------------------------------------------------------------
float CGameRulesSinglePlay::GetItemRespawnDelay(const CBaseEntity *pItem)
{
	return -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Some game variations may choose to randomize spawn locations
// Input  : *pItem - 
// Output : Vector
//-----------------------------------------------------------------------------
const Vector &CGameRulesSinglePlay::GetItemRespawnSpot(CBaseEntity *pItem)
{
	if (pItem)
		return pItem->pev->origin;

	return g_vecZero;
}

//-----------------------------------------------------------------------------
// Purpose: Entity restrictions may apply here
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::IsAllowedToSpawn(CBaseEntity *pEntity)
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*szName - 
//			iCount - 
//-----------------------------------------------------------------------------
//void CGameRulesSinglePlay::PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, const int &iCount)
//{
//}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAmmo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FAmmoShouldRespawn(const CBasePlayerAmmo *pAmmo)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Ammo respawn time
// Input  : *pAmmo - 
// Output : float
//-----------------------------------------------------------------------------
float CGameRulesSinglePlay::GetAmmoRespawnDelay(const CBasePlayerAmmo *pAmmo)
{
	return -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Charger respawn time // used by airtank
// Input  : *pAmmo - 
// Output : float
//-----------------------------------------------------------------------------
float CGameRulesSinglePlay::GetChargerRechargeDelay(void)
{
	return 10.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Which weapons should be packed and dropped
// Input  : *pPlayer - 
// Output : int GR_NONE
//-----------------------------------------------------------------------------
short CGameRulesSinglePlay::DeadPlayerWeapons(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_GUN_NO;
}

//-----------------------------------------------------------------------------
// Purpose: Which ammo should be packed and dropped
// Input  : *pPlayer - 
// Output : int GR_NONE
//-----------------------------------------------------------------------------
short CGameRulesSinglePlay::DeadPlayerAmmo(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_AMMO_NO;
}

//-----------------------------------------------------------------------------
// Purpose: Determines relationship between given player and entity
// Warning: DO NOT CALL IRelationship() from here!
// Input  : *pPlayer - 
//			*pTarget - 
// Output : int - GR_NOTTEAMMATE
//-----------------------------------------------------------------------------
int CGameRulesSinglePlay::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	return GR_NOTTEAMMATE;// why would a single player in half life need this? 
}

//-----------------------------------------------------------------------------
// Purpose: Are effects allowed on this server?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FAllowEffects(void)// XDM
{
	return (sv_effects.value > 0.0);
}

//-----------------------------------------------------------------------------
// Purpose: Should server always precache all possible ammo types?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FPrecacheAllAmmo(void)
{
	return (sv_precacheammo.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Should server always precache all possible items?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FPrecacheAllItems(void)
{
	return (sv_precacheitems.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Should server always precache all possible weapons?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::FPrecacheAllWeapons(void)
{
	return (sv_precacheweapons.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Get player with the best score
// Input  : teamIndex - may be TEAM_NONE
// Output : CBasePlayer *
//-----------------------------------------------------------------------------
CBasePlayer *CGameRulesSinglePlay::GetBestPlayer(TEAM_ID teamIndex)
{
	return UTIL_ClientByIndex(1);// just don't want to put "1" here
}

//-----------------------------------------------------------------------------
// Purpose: client entered a console command
// Input  : *pPlayer - client
//			*pcmd - command line, use CMD_ARGC() and CMD_ARGV()
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesSinglePlay::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	if (DeveloperCommand(pPlayer, pcmd))
	{
	}
	else if (FStrEq(pcmd, "VModEnable"))
	{
	}
	else return false;

	return true;
}
