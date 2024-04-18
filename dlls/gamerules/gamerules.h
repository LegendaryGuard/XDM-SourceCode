//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef GAMERULES_H
#define GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

//#define _DEBUG_GAMERULES	1

// This works even in release builds
#if defined (_DEBUG_GAMERULES)
#define DBG_GR_PRINT	DBG_PrintF
#else
#define DBG_GR_PRINT
#endif

#include "gamedefs.h"// XDM

class CBaseMonster;
class CBasePlayer;
class CBasePlayerAmmo;
class CBasePlayerItem;
class CItem;

// XDM3037: TODO?
/*interface IGameRules
{
}*/

// XDM3036: TODO?
/*class CBaseGameGoalEntity// : public CBaseEntity
{
public:
	virtual bool IsGameGoal(void) const { return true; }// TODO: mygamerules == currentgamerules
};*/


//-----------------------------------------------------------------------------
// Base class of all game rules
//
// Redefine virtual functions with extreme caution!
//-----------------------------------------------------------------------------
class CGameRules
{
public:
	CGameRules();
	virtual ~CGameRules();

	virtual short GetGameType(void);// XDM3035x
	virtual short GetGameMode(void) { return m_iGameMode; }// XDM3036
	virtual short GetGameState(void) { return m_iGameState; }// XDM3038c: read-only!
	virtual short GetCombatMode(void) { return GAME_COMBATMODE_NORMAL; }// XDM3038a
	virtual const char *GetGameDescription(void) { return GAMETITLE_DEFAULT_STRING; }// game name that gets seen in the server browser
	virtual void GameSetState(short iGameState);// XDM3037a
	float GetStartTime(void) const { return m_fStartTime; }// when the game was started (read-only)

	// Rules properties
	virtual bool IsMultiplayer(void) { return false; }// is this a multiplayer game?
	virtual bool IsTeamplay(void) { return false; }// is this game being played with team rules?
	virtual bool IsCoOp(void) { return false; }// players fight against NPCs
	virtual bool IsRoundBased(void) { return false; }// multiple rounds played on each map
	virtual bool IsGameOver(void);
	virtual bool FStartImmediately(void) { return true; }// GAME_STATE_ACTIVE is set in ServerActivate()

	// Global server routines
	virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);// XDM3037a
	virtual void Initialize(void);// XDM: need this because g_pGameRules cannot be used from constructor
	virtual void RefreshSkillData(void);// fill skill data struct with proper values
	virtual void StartFrame(void) {}// GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual bool CheckLimits(void) { return false; }// has the game reached one of its limits?
	virtual bool CheckEndConditions(void) { return false; }// check if it is ok to end the game
	virtual bool ValidateSpawnPoint(CBaseEntity *pPlayer, CBaseEntity *pSpot);// XDM3038a
	virtual int SpawnSpotUsePolicy(void) { return SPAWNSPOT_UNDEFINED; }// XDM3038a
	virtual void EndMultiplayerGame(void) {}
	virtual void ChangeLevel(void) {}
	//virtual void OnChangeLevel(CBasePlayer *pActivator, const char *szNextMap, edict_t *pEntLandmark) {}// XDM3035: trigger

	// Policies
	virtual bool FUseExtraScore(void) { return false; }// XDM3037a
	virtual bool PlayTextureSounds(void) { return true; }
	virtual bool PlayFootstepSounds(CBasePlayer *pPlayer, float fvol) { return true; }
	virtual bool FAllowMonsters(void) { return true; }//are monsters allowed
	virtual bool FAllowEffects(void) { return true; }// XDM: are effects allowed
	virtual bool FAllowMapMusic(void) { return true; }// XDM: dynamic map music
	virtual bool FPersistBetweenMaps(void) { return (m_iPersistBetweenMaps != 0); }
	virtual bool FAllowFlashlight(void) { return true; }// Are players allowed to switch on their flashlight?
	virtual bool FAllowSpectators(void) { return false; }// XDM3038c
	virtual bool IsAllowedToSpawn(CBaseEntity *pEntity) { return true; }// can this item spawn (eg monsters don't spawn in deathmatch).
	virtual bool FShouldMakeDormant(CBaseEntity *pEntity) { return true; }// XDM3038c: should this entity become dormant after level transiiton
	virtual bool FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer);// XDM3038
	virtual bool FAllowLevelChange(CBasePlayer *pActivator, const char *szNextMap, edict_t *pEntLandmark) { return false; }
	virtual void OnEntityRemoved(CBaseEntity *pEntity) {}// XDM3038
	virtual short DeadPlayerWeapons(CBasePlayer *pPlayer) { return GR_PLR_DROP_GUN_ACTIVE; }// what happens to a dead player's weapons
	virtual short DeadPlayerAmmo(CBasePlayer *pPlayer) { return GR_PLR_DROP_AMMO_ACTIVE; }// what happens to a dead player's ammo
	virtual bool FPrecacheAllAmmo(void) { return false; }// XDM3038c
	virtual bool FPrecacheAllItems(void) { return false; }// XDM3038c
	virtual bool FPrecacheAllWeapons(void) { return false; }// XDM3038c

	// Client connection/disconnection
	virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] ) { return true; }// a client just connected to the server (player hasn't spawned yet)
	virtual void ClientDisconnected(CBasePlayer *pPlayer) {}// a client just disconnected from the server
	virtual void ClientRemoveFromGame(CBasePlayer *pPlayer) {}// XDM3038c: remove client from scoreboard, disable interaciton, teams, etc. Used by spectators mostly.
	virtual bool ClientCommand(CBasePlayer *pPlayer, const char *pcmd);
	virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer);// the player has changed userinfo; can change it now
	virtual void InitHUD(CBasePlayer *pPlayer) {}// the client dll is ready for updating
	virtual void UpdateGameMode(edict_t *pePlayer);// the client needs to be informed of the current game mode

	// Client routines
	virtual bool PlayerSpawnStart(CBasePlayer *pPlayer) { return true; }// XDM3038a: called by CBasePlayer::Spawn() at the very beginning
	virtual void PlayerSpawn(CBasePlayer *pPlayer) {}// called by CBasePlayer::Spawn() just before releasing player into the game
	virtual void PlayerThink(CBasePlayer *pPlayer) {}// called by CBasePlayer::PreThink() every frame, before physics are run and after keys are accepted
	virtual bool PlayerStartObserver(CBasePlayer *pPlayer) { return false; }// XDM3037a: allow/deny
	virtual bool PlayerStopObserver(CBasePlayer *pPlayer) { return false; }// XDM3037a: allow/deny
	virtual CBaseEntity *PlayerUseSpawnSpot(CBasePlayer *pPlayer, bool bSpectator);// place this player on their spawnspot and face them the proper direction
	virtual size_t PlayerAddDefault(CBasePlayer *pPlayer) { return 0; }
	virtual void OnPlayerCheckPoint(CBasePlayer *pPlayer, CBaseEntity *pCheckPoint) {}// XDM3038
	virtual void OnPlayerFoundSecret(CBasePlayer *pPlayer, CBaseEntity *pSecret) {}// XDM3038c
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget) { return GR_NEUTRAL; }// what is the player's relationship with this entity? DO NOT CALL IRelationship() from here!

	// Client policies
	virtual bool FPlayerIsActive(CBasePlayer *pPlayer) { return true; }// XDM3038
	virtual bool FPlayerCanRespawn(const CBasePlayer *pPlayer) { return false; }// is this player allowed to respawn now?
	virtual bool FForceRespawnPlayer(CBasePlayer *pPlayer) { return false; }
	virtual bool FPlayerStartAsObserver(CBasePlayer *pPlayer) { return false; }// XDM3037a: should player become observer right from the very beginning
	virtual bool FAllowSpectatorChange(CBasePlayer *pPlayer) { return false; }// XDM
	virtual bool FAllowShootingOnLadder(CBasePlayer *pPlayer) { return true; }// XDM3038
	virtual bool FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker) { return true; }// can this player take damage from this attacker?
	virtual bool FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon) { return false; }// should the player switch to this weapon?
	virtual float GetPlayerRespawnDelay(const CBasePlayer *pPlayer) { return 0.0f; }// XDM3038c
	virtual float GetPlayerFallDamage(CBasePlayer *pPlayer) { return 0.0f; }// this client just hit the ground after a fall. How much damage?
	virtual CBasePlayerItem *GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon);// updated: get next-best weapon, but don't select it
	virtual int ShouldShowStartMenu(CBasePlayer *pPlayer) { return 0; }// XDM3038a
	virtual bool FShouldAutoAim(CBasePlayer *pPlayer, CBaseEntity *pTarget);
	virtual bool FForcePlayerSpawnSpot(CBasePlayer *pPlayer, Vector &vecSpot) { return false; }// XDM3038
	virtual int GetPlayerMaxHealth(void) { return MAX_PLAYER_HEALTH; }// XDM3037: now game rules decide it
	virtual int GetPlayerMaxArmor(void) { return MAX_NORMAL_BATTERY; }// XDM3038: now game rules decide it
	// Weapon retrieval
	virtual bool CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem);// The player is touching an CBasePlayerItem, do I give it to him?
	virtual bool CanDropPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem);// XDM3037a
	virtual bool CanHaveItem(CBasePlayer *pPlayer, CBaseEntity *pItem) { return true; }// is this player allowed to take this CItem?
	virtual bool CanPickUpOwnedThing(CBasePlayer *pPlayer, CBaseEntity *pEntity) { return true; }// XDM3038c
	//virtual bool CanPickUp(CBasePlayer *pPlayer, CBaseEntity *pEntity);// XDM3038c

	virtual bool FMonsterCanRespawn(const CBaseMonster *pMonster) { return false; }// XDM3038c
	virtual float GetMonsterRespawnDelay(const CBaseMonster *pMonster) { return 0.0f; }// XDM3038c

	// Client scoring
	virtual int IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled) { return 0; }// how many points do I award whoever kills this player?
	virtual void PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor) {}// called each time a player dies
	virtual void MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor) {}// called each time a monster dies
	virtual void DeathNotice(CBaseEntity *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor) {}// call this from within a GameRules class to report an obituary.

	// Item spawn/respawn control
	virtual bool FItemShouldRespawn(const CBaseEntity *pItem) { return 0; }// Should this item respawn?
	virtual float GetItemRespawnDelay(const CBaseEntity *pItem) { return 0.0f; }// when may this item respawn?
	virtual const Vector &GetItemRespawnSpot(CBaseEntity *pItem) { return g_vecZero; }// where in the world should this item respawn?

	// Weapon spawn/respawn control
	virtual bool FWeaponShouldRespawn(const CBasePlayerItem *pWeapon) { return false; }// should this weapon respawn?
	virtual float GetWeaponRespawnDelay(const CBasePlayerItem *pWeapon) { return 0.0f; }
	virtual float OnWeaponTryRespawn(CBasePlayerItem *pWeapon) { return 0.0f; } // can i respawn now, and if not, when should i try again?
	virtual Vector GetWeaponRespawnSpot(CBasePlayerItem *pWeapon) { return g_vecZero; }// where in the world should this weapon respawn?
	virtual float GetWeaponWorldScale(void);

	// Ammo spawn/respawn control
	virtual bool FAmmoShouldRespawn(const CBasePlayerAmmo *pAmmo) { return false; }// should this ammo item respawn?
	virtual float GetAmmoRespawnDelay(const CBasePlayerAmmo *pAmmo) { return 0.0f; }// when should this ammo item respawn?

	// Healthcharger respawn control
	virtual float GetChargerRechargeDelay(void) { return 0.0f; }// how long until a depleted HealthCharger recharges itself?

	// Teamplay stuff
	virtual TEAM_ID GetTeamIndex(const char *pTeamName) { return TEAM_NONE; }
	virtual const char *GetTeamName(TEAM_ID teamIndex) { return ""; }
	virtual bool IsValidTeam(const char *pTeamName) { return true; }
	virtual bool IsValidTeam(TEAM_ID teamIndex) { return true; }
	virtual bool IsRealTeam(TEAM_ID teamIndex) { return false; }
	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib) {}
	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, TEAM_ID teamindex, bool bKill, bool bGib) {}
	virtual bool AddScoreToTeam(TEAM_ID teamIndex, int score) { return false; }
	virtual bool AddScore(CBaseEntity *pWinner, int score);
	virtual uint32 GetScoreLimit(void) { return 0; }
	virtual uint32 GetScoreRemaining(void) { return 0; }
	virtual uint32 GetTimeLimit(void) { return 0; }// XDM3038: seconds
	virtual short NumPlayersInTeam(TEAM_ID teamIndex) { return 0; }
	virtual short MaxTeams(void) { return 1; }// maximum possible number for this game type
	virtual short GetNumberOfTeams(void) { return 0; }// number of currently active teams
	virtual uint32 GetRoundsLimit(void) { return 0; }
	virtual uint32 GetRoundsPlayed(void) { return 0; }
	virtual uint32 CountPlayers(void);
	virtual bool ServerIsFull(void) { return true; }
	virtual bool CheckPlayersReady(void) { return true; }// have all the players press their ready buttons?
	virtual int GetTeamScore(TEAM_ID teamIndex) { return 0; }// XDM3037a
	virtual CBasePlayer *GetBestPlayer(TEAM_ID teamIndex) { return NULL; }
	virtual TEAM_ID GetBestTeam(void) { return TEAM_NONE; }
	virtual CBaseEntity *GetTeamBaseEntity(TEAM_ID teamIndex) { return NULL; }
	virtual void SetTeamBaseEntity(TEAM_ID teamIndex, CBaseEntity *pEntity) {}

	virtual CBaseEntity	*GetIntermissionActor1(void) { return NULL; }
	virtual CBaseEntity	*GetIntermissionActor2(void) { return NULL; }
	virtual float GetIntermissionStartTime(void) { return 0; }
	virtual float GetIntermissionEndTime(void) { return 0; }

	// Debug
	virtual void DumpInfo(void);

	Vector	m_vecForceSpawnSpot;
protected:
	float	m_fStartTime;// absolute game start time on current map
	char	m_szLastMap[MAX_MAPNAME];// XDM3038a: all game rules need this
	short	m_iGameMode;// XDM3035a: additional game mode
	short	m_iGameState;// XDM3037a
	short	m_iPersistBetweenMaps;// XDM3038a: now flags
	bool	m_bGameOver;
};


//-----------------------------------------------------------------------------
// Singleplayer rules
//-----------------------------------------------------------------------------
class CGameRulesSinglePlay : public CGameRules
{
public:
	CGameRulesSinglePlay(void);
	virtual short GetGameType(void) { return GT_SINGLE; }// XDM3035
	virtual void StartFrame(void);

	//virtual bool FStartImmediately(void) { return true; }// XDM3038
	virtual bool FAllowFlashlight(void) { return true; }
	virtual bool FAllowSpectators(void) { return false; }// XDM3038c
	virtual bool FAllowLevelChange(CBasePlayer *pActivator, const char *szNextMap, edict_t *pEntLandmark) { return true; }
	virtual bool FAllowEffects(void);
	virtual bool FAllowMapMusic(void) { return true; }
	virtual bool FPrecacheAllAmmo(void);// XDM3038c
	virtual bool FPrecacheAllItems(void);// XDM3038c
	virtual bool FPrecacheAllWeapons(void);// XDM3038c

	virtual bool IsMultiplayer(void) { return false; }
	//virtual bool IsTeamplay(void) { return false; }
	virtual const char *GetGameDescription(void) { return "Single"; }

	virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	virtual bool ClientCommand(CBasePlayer *pPlayer, const char *pcmd);// XDM
	virtual void InitHUD(CBasePlayer *pPlayer);		// the client dll is ready for updating
	virtual bool CheckLimits(void) { return false; }

	virtual bool IsAllowedToSpawn(CBaseEntity *pEntity);
	virtual bool FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual float GetPlayerFallDamage(CBasePlayer *pPlayer);

	virtual void PlayerSpawn(CBasePlayer *pPlayer);
	virtual void PlayerThink(CBasePlayer *pPlayer);

	virtual bool FPlayerIsActive(CBasePlayer *pPlayer) { return true; }// XDM3038
	virtual bool FPlayerCanRespawn(const CBasePlayer *pPlayer) { return false; }
	virtual bool FAllowShootingOnLadder(CBasePlayer *pPlayer);// XDM3038
	virtual float GetPlayerRespawnDelay(const CBasePlayer *pPlayer) { return SINGLEPLAYER_RESTART_DELAY; }// XDM3038a: when may this player respawn?

	virtual int IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled);
	virtual void PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);
	virtual void MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);// XDM3035
	virtual void DeathNotice(CBaseEntity *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);

	virtual bool FWeaponShouldRespawn(const CBasePlayerItem *pWeapon);
	virtual float GetWeaponRespawnDelay(const CBasePlayerItem *pWeapon);
	virtual float OnWeaponTryRespawn(CBasePlayerItem *pWeapon);
	virtual Vector GetWeaponRespawnSpot(CBasePlayerItem *pWeapon);

	virtual bool CanHaveItem(CBasePlayer *pPlayer, CBaseEntity *pItem);
	virtual bool CanDropPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem);// XDM3038c

	virtual bool FItemShouldRespawn(const CBaseEntity *pItem);
	virtual float GetItemRespawnDelay(const CBaseEntity *pItem);
	virtual const Vector &GetItemRespawnSpot(CBaseEntity *pItem);

	virtual bool FAmmoShouldRespawn(const CBasePlayerAmmo *pAmmo);
	virtual float GetAmmoRespawnDelay(const CBasePlayerAmmo *pAmmo);

	virtual float GetChargerRechargeDelay(void);

	virtual short DeadPlayerWeapons(CBasePlayer *pPlayer);
	virtual short DeadPlayerAmmo(CBasePlayer *pPlayer);

	virtual CBasePlayer *GetBestPlayer(TEAM_ID teamIndex);
	virtual TEAM_ID GetBestTeam(void) { return TEAM_NONE; }

	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
};




#define MP_ADD_DEFAULT_WEAPON	"weapon_crowbar"

//-----------------------------------------------------------------------------
// Frag-based "free for all" multiplayer game
//-----------------------------------------------------------------------------
class CGameRulesMultiplay : public CGameRules
{
public:
	CGameRulesMultiplay();
	virtual ~CGameRulesMultiplay();

	virtual short GetGameType(void) { return GT_DEATHMATCH; }// XDM3035
	virtual short GetCombatMode(void);// XDM3038a

	virtual void Initialize(void);// XDM
	virtual void StartFrame(void);

	virtual bool IsAllowedToSpawn(CBaseEntity *pEntity);
	virtual bool FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer);// XDM3038
	virtual bool FAllowFlashlight(void);
	virtual bool FAllowSpectators(void);// XDM3038c
	virtual bool FAllowMonsters(void);
	virtual bool FAllowEffects(void);
	virtual bool FAllowMapMusic(void);
	virtual bool FPrecacheAllAmmo(void);// XDM3038c
	virtual bool FPrecacheAllItems(void);// XDM3038c
	virtual bool FPrecacheAllWeapons(void);// XDM3038c
	virtual bool FAllowLevelChange(CBasePlayer *pActivator, const char *szNextMap, edict_t *pEntLandmark) { return false; }
	virtual bool FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	//virtual bool FStartImmediately(void) { return true; }// XDM3038
	virtual void OnPlayerFoundSecret(CBasePlayer *pPlayer, CBaseEntity *pSecret);// XDM3038c

	virtual bool IsMultiplayer(void) { return true; }
	virtual const char *GetGameDescription(void) { return "Deathmatch"; }
	virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);// XDM3038

	virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	virtual void ClientDisconnected(CBasePlayer *pPlayer);
	virtual void ClientRemoveFromGame(CBasePlayer *pPlayer);// XDM3038c
	virtual bool ClientCommand(CBasePlayer *pPlayer, const char *pcmd);
	virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer);// XDM3035
	virtual void InitHUD(CBasePlayer *pPlayer);// the client dll is ready for updating
	virtual int ShouldShowStartMenu(CBasePlayer *pPlayer);// XDM3038a

	//virtual void UpdateGameMode(CBasePlayer *pPlayer); // the client needs to be informed of the current game mode
	virtual bool CheckLimits(void);
	virtual bool CheckEndConditions(void);// XDM3038

	virtual float GetPlayerFallDamage(CBasePlayer *pPlayer);
	virtual bool FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);

	virtual bool PlayerSpawnStart(CBasePlayer *pPlayer);// XDM3038a: called by CBasePlayer::Spawn() at the very beginning
	virtual void PlayerSpawn(CBasePlayer *pPlayer);
	virtual void PlayerThink(CBasePlayer *pPlayer);
	virtual bool PlayerStartObserver(CBasePlayer *pPlayer);// XDM3037a
	virtual bool PlayerStopObserver(CBasePlayer *pPlayer);// XDM3037a

	virtual bool FPlayerIsActive(CBasePlayer *pPlayer);// XDM3038
	virtual bool FPlayerCanRespawn(const CBasePlayer *pPlayer);
	virtual bool FForceRespawnPlayer(CBasePlayer *pPlayer);
	virtual bool FPlayerStartAsObserver(CBasePlayer *pPlayer);// XDM3038
	virtual bool FAllowSpectatorChange(CBasePlayer *pPlayer);
	virtual bool FAllowShootingOnLadder(CBasePlayer *pPlayer);// XDM3038
	virtual float GetPlayerRespawnDelay(const CBasePlayer *pPlayer);// XDM3038a: when may this player respawn?

	virtual size_t PlayerAddDefault(CBasePlayer *pPlayer);
	virtual int SpawnSpotUsePolicy(void);// XDM3038a
	virtual int GetPlayerMaxHealth(void);// XDM3037
	virtual int GetPlayerMaxArmor(void);// XDM3038

	virtual int IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled);
	virtual void PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);
	virtual void MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);// XDM3035
	virtual void DeathNotice(CBaseEntity *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);

	virtual bool CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem);
	virtual bool CanDropPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem);// XDM3037a
	virtual bool CanHaveItem(CBasePlayer *pPlayer, CBaseEntity *pItem);
	virtual bool CanPickUpOwnedThing(CBasePlayer *pPlayer, CBaseEntity *pEntity);// XDM3038c
	//virtual bool CanPickUp(CBasePlayer *pPlayer, CBaseEntity *pEntity);// XDM3038c

	virtual bool FMonsterCanRespawn(const CBaseMonster *pMonster);// XDM3038c
	virtual float GetMonsterRespawnDelay(const CBaseMonster *pMonster);// XDM3038c

	virtual bool FWeaponShouldRespawn(const CBasePlayerItem *pWeapon);
	virtual float GetWeaponRespawnDelay(const CBasePlayerItem *pWeapon);
	virtual float OnWeaponTryRespawn(CBasePlayerItem *pWeapon);
	virtual Vector GetWeaponRespawnSpot(CBasePlayerItem *pWeapon);
	virtual float GetWeaponWorldScale(void);

	virtual bool FItemShouldRespawn(const CBaseEntity *pItem);
	virtual float GetItemRespawnDelay(const CBaseEntity *pItem);
	virtual const Vector &GetItemRespawnSpot(CBaseEntity *pItem);

	virtual bool FAmmoShouldRespawn(const CBasePlayerAmmo *pAmmo);
	virtual float GetAmmoRespawnDelay(const CBasePlayerAmmo *pAmmo);

	virtual float GetChargerRechargeDelay(void);

	virtual short DeadPlayerWeapons(CBasePlayer *pPlayer);
	virtual short DeadPlayerAmmo(CBasePlayer *pPlayer);

	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
	virtual bool AddScore(CBaseEntity *pWinner, int score);
	virtual uint32 GetScoreLimit(void);
	virtual uint32 GetScoreRemaining(void);
	virtual uint32 GetTimeLimit(void);// XDM3038: seconds

	virtual bool PlayTextureSounds(void);
	virtual bool PlayFootstepSounds(CBasePlayer *pPlayer, float fvol);

	virtual bool CheckPlayersReady(void);

	virtual CBasePlayer *GetBestPlayer(TEAM_ID teamIndex);
	virtual TEAM_ID GetBestTeam(void) { return TEAM_NONE; }

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame(void);

	virtual bool ServerIsFull(void);
	virtual void ChangeLevel(void);
	virtual void DumpInfo(void);

	// Read-only data
	virtual CBaseEntity	*GetIntermissionActor1(void) { return m_pIntermissionEntity1; }
	virtual CBaseEntity	*GetIntermissionActor2(void) { return m_pIntermissionEntity2; }
	virtual float GetIntermissionStartTime(void) { return m_flIntermissionStartTime; }
	virtual float GetIntermissionEndTime(void) { return m_flIntermissionEndTime; }

protected:
	virtual void IntermissionStart(CBasePlayer *pWinner, CBaseEntity *pInFrameEntity);
	virtual void SendLeftUpdates(void);// XDM3036: former parameters are useless

	void SendMOTDToClient(CBasePlayer *pClient);

	CBaseEntity	*m_pLastVictim;
	CBaseEntity	*m_pIntermissionEntity1;// primary, can be NULL
	CBaseEntity	*m_pIntermissionEntity2;// in-frame entity, can be NULL

	float		m_flIntermissionEndTime;
	float		m_flIntermissionStartTime;// XDM ?

	uint32		m_iRemainingScore;
	float		m_fRemainingTime;

	uint32		m_iRemainingScoreSent;
	uint32		m_iRemainingTimeSent;// float is bad for comparison

	uint32		m_iLastCheckedNumActivePlayers;// players that are PLAYING, not spectating

	CLIENT_INDEX m_iFirstScoredPlayer;// index of the first player to score (for GAME_EVENT_FIRST_SCORE)
	CLIENT_INDEX m_iLeader;// index of current best player

	uint32		m_bReadyButtonsHit;// XDM3038a: padding
	uint32		m_bSendStats;// XDM3038c
	//uint32		m_iSendFlags;// XDM3038c: send something over netrowk
};


gametype_prefix_t *FindGamePrefix(int game_type);
int DetectGameTypeByPrefix(const char *pString);
int DetermineGameType(void);
CGameRules *InstallGameRules(int game_type);
bool IsActiveTeam(const TEAM_ID &team_id);
bool IsActivePlayer(CBaseEntity *pPlayerEntity);
bool IsActivePlayer(const CLIENT_INDEX &idx);
bool IsValidPlayerIndex(const CLIENT_INDEX &idx);
bool WasActivePlayer(CBaseEntity *pPlayerEntity);// XDM3038c

bool DeveloperCommand(CBasePlayer *pPlayer, const char *pcmd);// XDM: special version of ClientCommand() for development/administrative purposes


extern DLL_GLOBAL CGameRules *g_pGameRules;


#endif // GAMERULES_H
