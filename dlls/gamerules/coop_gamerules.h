//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef COOP_GAMERULES_H
#define COOP_GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

// don't have time to use something better
#include <vector>

// these pev->flags will be saved and reapplied to the player when he respawns at that spot (check point)
#define SPAWNSPOT_SAVE_PLAYERFLAGS		(FL_SWIM|FL_ONGROUND|FL_PARTIALGROUND|FL_WATERJUMP|FL_DUCKING|FL_FLOAT)

#define COOP_MAP_HAS_MONSTERS				(1 << 0)
#define COOP_MAP_HAS_TRANSITION_NEXT		(1 << 1)
#define COOP_MAP_HAS_TRANSITION_PREVIOUS	(1 << 2)
// cannot determine for now #define COOP_MAP_HAS_TRANSITION_JUNCTION	(1 << 3)
#define COOP_MAP_HAS_END					(1 << 4)

class CGameRulesCoOp : public CGameRulesMultiplay
{
public:
	CGameRulesCoOp();
	virtual ~CGameRulesCoOp();

	virtual short GetGameType(void) { return GT_COOP; }// XDM3035
	virtual const char *GetGameDescription(void) {return "Co-Operative";}
	virtual bool IsCoOp(void) { return true; }

	//virtual void InitHUD(CBasePlayer *pPlayer);
	virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	virtual void Initialize(void);
	virtual void StartFrame(void);
	virtual void ChangeLevel(void);
	virtual bool CheckLimits(void);// XDM3038
	virtual bool CheckEndConditions(void);// XDM3038

	virtual void OnPlayerCheckPoint(CBasePlayer *pPlayer, CBaseEntity *pCheckPoint);// XDM3038
	virtual void OnEntityRemoved(CBaseEntity *pEntity);// XDM3038

	virtual int IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled);
	//virtual void PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);
	virtual void MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);

	virtual bool FAllowLevelChange(CBasePlayer *pActivator, const char *szNextMap, edict_t *pEntLandmark);
	virtual bool FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);

	virtual bool IsAllowedToSpawn(CBaseEntity *pEntity);
	virtual bool FShouldMakeDormant(CBaseEntity *pEntity);// XDM3038c
	//virtual bool FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer);// XDM3038
	virtual bool FAllowMonsters(void) { return true; }
	virtual bool FAllowSpectatorChange(CBasePlayer *pPlayer);
	virtual bool FPersistBetweenMaps(void);
	virtual bool FPlayerFinishedLevel(CBasePlayer *pPlayer);

	virtual size_t PlayerAddDefault(CBasePlayer *pPlayer);// XDM3038b
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);

	virtual bool ValidateSpawnPoint(CBaseEntity *pPlayer, CBaseEntity *pSpot);// XDM3038a
	virtual bool FForcePlayerSpawnSpot(CBasePlayer *pPlayer, Vector &vecSpot);// XDM3038
	CBaseEntity *PlayerUseSpawnSpot(CBasePlayer *pPlayer, bool bSpectator);
	virtual CBasePlayer *GetBestPlayer(TEAM_ID teamIndex);
	virtual uint32 CountPlayers(void);
	virtual uint32 GetScoreLimit(void);
	virtual uint32 GetScoreRemaining(void);

	bool CheckPlayersTouchedTriggers(bool bCheckTransitionVolume);
	CBaseEntity *GetLastCheckPoint(void);
	bool UnregisterTarget(CBaseEntity *pEntity);

protected:
	virtual void IntermissionStart(CBasePlayer *pWinner, CBaseEntity *pInFrameEntity);

	void SavePlayersData(void);
	void SavePlayerData(CBasePlayer *pPlayer);
	size_t RestorePlayerData(CBasePlayer *pPlayer);

	uint32		m_iChangeLevelTriggers;
	uint32		m_iRegisteredTargets;
	uint32		m_iLastCheckedNumFinishedPlayers;// players that are reached the goal; these MAY or may NOT be spectating
	uint32		m_iLastCheckedNumTransferablePlayers;// players that are in transition space
	uint32		m_iGameLogicFlags;//
	char		m_szNextMap[MAX_MAPNAME];
	char		m_szLastLandMark[MAX_MAPNAME];// XDM3038a: used to search for a good spawn spot
	EHANDLE		m_hEntLandmark;
	EHANDLE		m_hFirstPlayer;// player who reached end of level first
	Vector		m_vLastLandMark;// XDM3038c: where the last landmark was located (previous map coordinates)

	std::vector<int> m_ActivatedCheckPoints;
	std::vector<int>::iterator m_ActivatedCheckPointsIterator;
	//std::vector<EHANDLE> m_ActivatedCheckPoints;
	//std::vector<EHANDLE>::iterator m_ActivatedCheckPointsIterator;
	//std::vector<EHANDLE>::reverse_iterator m_ActivatedCheckPointsRevIterator;

	std::vector<int> m_RegisteredTargets;
	std::vector<int>::iterator m_TargetIterator;
	//std::vector<EHANDLE> m_RegisteredTargets;
	//std::vector<EHANDLE>::iterator m_TargetIterator;
	//std::vector<EHANDLE>::reverse_iterator m_TargetRevIterator;

	int			m_szTransitionInventoryPlayerIDs[MAX_PLAYERS];// XDM3038b: GETPLAYERUSERID()
	char		m_szTransitionInventoryItems[MAX_PLAYERS][PLAYER_INVENTORY_SIZE][MAX_ENTITY_STRING_LENGTH];// XDM3038b
	int			m_szTransitionInventoryAmmo[MAX_PLAYERS][MAX_AMMO_SLOTS];// XDM3038b
	int			m_szTransitionInventoryArmor[MAX_PLAYERS];// XDM3038c
	BOOL		m_szTransitionInventoryLJM[MAX_PLAYERS];// XDM3038c

	// UNDONE: SAVERESTOREDATA		*m_pSaveData;
};

#endif // COOP_GAMERULES_H
