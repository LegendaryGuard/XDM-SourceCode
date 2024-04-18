//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "maprules.h"
#include "gamerules.h"
#include "coop_gamerules.h"
#include "globals.h"
#include "game.h"
#include "monsters.h"
#include "triggers.h"

#define landmark euser4

// IMPORTANT:
// CoOp heavily depends on proper mapping!
// For example: if there's an isolated unreachable monster somewhere, it will still be registered as a game target, which will prevent players drom finishing the map.
// Bad changelevels will also mess the game up, transitions must be placed well.
// Bad trigger_autosaves will make players stuck or die (example: in elevators, doors, trains).
// It is assumed that all players are active and there are NO spectator-only clients (like in round rules), so it is not necessary to distinguish players by their state
// This is the most sophisticated co-operative game rules ever writthen for Half-Life engine to this day.
// NOTE: on save/restore: always reserve slots for MAX_PLAYERS, number of actual joined clients may vary.

//-----------------------------------------------------------------------------
// Purpose: How many points have exactly this entity as landmark?
// Note   : Normally it should be moved to a separate CPP file, but we need pev->landmark
// Input  : *pEntLandmark - 
// Output : uint32 - count
//-----------------------------------------------------------------------------
uint32 CountPlayersHaveLandmark(edict_t *pEntLandmark)
{
	uint32 count = 0;
	for (CLIENT_INDEX ci = 1; ci <= gpGlobals->maxClients; ++ci)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(ci);
		if (pPlayer)// don't check if this player is active (not spectating)
		{
			if (pPlayer->pev->landmark == pEntLandmark)
				++count;
		}
	}
	return count;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
// Warning: It is called when creating rules, not when transfering them
//-----------------------------------------------------------------------------
CGameRulesCoOp::CGameRulesCoOp() : CGameRulesMultiplay()
{
	m_iPersistBetweenMaps = GR_PERSIST|GR_PERSIST_KEEP_EXTRASCORE;// XDM3035c: collect data between levels
	m_iRegisteredTargets = 0;
	m_iChangeLevelTriggers = 0;
	m_iLastCheckedNumActivePlayers = 0;
	m_iLastCheckedNumFinishedPlayers = 0;
	m_iLastCheckedNumTransferablePlayers = 0;
	m_iGameLogicFlags = 0;
	memset(m_szNextMap, 0, MAX_MAPNAME);
	memset(m_szLastLandMark, 0, MAX_MAPNAME);// XDM3038a
	m_hEntLandmark = NULL;
	m_hFirstPlayer = NULL;
	m_vLastLandMark.Clear();
	memset(m_szTransitionInventoryPlayerIDs, 0, MAX_PLAYERS*sizeof(m_szTransitionInventoryPlayerIDs[0]));
	//m_pSaveData = SV_SaveInit(0);

	//cvar_t m_cvEndOfLevelSpectate = {"mp_coop_eol_spectate",	"1",			FCVAR_SERVER | FCVAR_EXTDLL};
	//CVAR_REGISTER(&m_cvEndOfLevelSpectate);
	/*too early! if (mp_monstersrespawn.value > 0.0f)
		m_iGameMode = COOP_MODE_MONSTERFRAGS;
	else if (m_iChangeLevelTriggers > 0)
		m_iGameMode = COOP_MODE_LEVEL;
	else
		m_iGameMode = COOP_MODE_SWEEP;*/
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameRulesCoOp::~CGameRulesCoOp()
{
// will be called after this.	CGameRulesMultiplay::~CGameRulesMultiplay();
	m_iChangeLevelTriggers = 0;
	//memset(m_szNextMap, 0, MAX_MAPNAME);
	m_ActivatedCheckPoints.clear();// XDM3038
	m_RegisteredTargets.clear();
	m_iRegisteredTargets = 0;
	m_hEntLandmark = NULL;
	m_hFirstPlayer = NULL;
	//SV_SaveFree(m_pSaveData);
	//m_pSaveData = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Server is activated, all entities spawned
// Input  : *pEdictList - 
//			edictCount - 
//			clientMax - 
//-----------------------------------------------------------------------------
void CGameRulesCoOp::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
//	DBG_GR_PRINT("CGameRulesCoOp::ServerActivate()\n");
	// good place to choose mode based on specific entities
	if (m_iChangeLevelTriggers > 0 && mp_coop_usemaptransition.value > 0.0f)// XDM3038a: check first!
	{
		m_iGameMode = COOP_MODE_LEVEL;
		//m_hEntLandmark = UTIL_FindEntityByTargetname(NULL, m_szLastLandMark, NULL);// XDM3038c: too late for dormant checking
		// We do this in MakeDormant() instead
		/*int entindex = 1;
		edict_t *pEdict = NULL;
		do
		{
			pEdict = INDEXENT(entindex);
			++entindex;
			if (pEdict)
			{
				if (UTIL_IsValidEntity(pEdict))
				{
					CBaseEntity *pEntity = NULL;
					pEntity = CBaseEntity::Instance(pEdict);
					if (pEntity)
					{
						if (pEntity->IsDormant())
						{
							conprintf(1, "CoOp: Found dormant entity %s (%s)\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->classname));
						}
					}
				}
			}
		} while (entindex < gpGlobals->maxEntities);*/
	}
	else if (mp_monstersrespawn.value > 0.0f)
		m_iGameMode = COOP_MODE_MONSTERFRAGS;
	else
		m_iGameMode = COOP_MODE_SWEEP;

	CGameRulesMultiplay::ServerActivate(pEdictList, edictCount, clientMax);// XDM3037a
}

//-----------------------------------------------------------------------------
// Purpose: First thing called after constructor. Initialize all data, cvars, etc.
//-----------------------------------------------------------------------------
void CGameRulesCoOp::Initialize(void)
{
	m_iChangeLevelTriggers = 0;
	memset(m_szNextMap, 0, MAX_MAPNAME);
	m_iLastCheckedNumActivePlayers = 0;
	m_iLastCheckedNumFinishedPlayers = 0;
	m_iLastCheckedNumTransferablePlayers = 0;
	m_iGameLogicFlags = 0;// XDM3038c
	m_ActivatedCheckPoints.clear();// XDM3038
	m_RegisteredTargets.clear();
	m_iRegisteredTargets = 0;
	m_hEntLandmark = NULL;
	m_hFirstPlayer = NULL;
	CGameRulesMultiplay::Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Runs every server frame, should handle any timer tasks, periodic events, etc.
//-----------------------------------------------------------------------------
void CGameRulesCoOp::StartFrame(void)
{
	// frag/time limit/etc
	CGameRulesMultiplay::StartFrame();

	if (!IsGameOver())
	{
		// Iterate from end to start using reverse iterator
		/* STL is a pile of shit as it always was		for (m_TargetRevIterator = m_RegisteredTargets.rbegin(); m_TargetRevIterator != m_RegisteredTargets.rend(); ++m_TargetRevIterator)
		{
			if ((*m_TargetRevIterator).Get() == NULL)// EHANDLE invalidated
				m_RegisteredTargets.erase(m_TargetRevIterator);
		}
		for (m_ActivatedCheckPointsRevIterator = m_ActivatedCheckPoints.rbegin(); m_ActivatedCheckPointsRevIterator != m_ActivatedCheckPoints.rend(); ++m_ActivatedCheckPointsRevIterator)
		{
			if ((*m_ActivatedCheckPointsRevIterator).Get() == NULL)// EHANDLE invalidated
				m_ActivatedCheckPoints.erase(m_ActivatedCheckPointsRevIterator);
		}*/
		/* last hopes were ruined by this crappy piece of crappy crap!		size_t invalidated_items = 0;
		for (m_TargetIterator = m_RegisteredTargets.begin(); m_TargetIterator != m_RegisteredTargets.end(); ++m_TargetIterator)
		{
			if ((*m_TargetIterator).Get() == NULL)// EHANDLE invalidated
			{
				m_TargetIterator = m_RegisteredTargets.erase(m_TargetIterator);
				++invalidated_items;
			}
		}
		if (invalidated_items > 0)// this should be non-normal situation
			conprintf(2, "CGameRulesCoOp::StartFrame(): invalidated %u game targets\n", invalidated_items);

		for (m_ActivatedCheckPointsIterator = m_ActivatedCheckPoints.begin(); m_ActivatedCheckPointsIterator != m_ActivatedCheckPoints.end(); ++m_ActivatedCheckPointsIterator)
		{
			if ((*m_ActivatedCheckPointsIterator).Get() == NULL)// EHANDLE invalidated
			{
				m_ActivatedCheckPointsIterator = m_ActivatedCheckPoints.erase(m_ActivatedCheckPointsIterator);
				invalidated_items++;
			}
		}
		if (invalidated_items > 0)// this should be non-normal situation
			conprintf(2, "CGameRulesCoOp::StartFrame(): invalidated %u check points\n", invalidated_items);
		*/
		bool finish = false;
		if (m_iGameMode == COOP_MODE_LEVEL)
		{
			/* checked when touched	if (CheckPlayersTouchedTriggers(true))// checks if all players touched trigger
			 {
				 finish = true;
				 conprintf(1, "CGameRulesCoOp: mode %d: all players touched triggers\n", m_iGameMode);
			 }*/
		}
		else if (m_iGameMode == COOP_MODE_SWEEP)
		{
			if (m_iRegisteredTargets > 0 && m_RegisteredTargets.size() <= 0)
			{
				finish = true;
				conprintf(1, "CGameRulesCoOp: mode %d: all registered targets eliminated\n", m_iGameMode);
			}
		}

		if (finish)
		{
			SERVER_PRINT("GAME: ended by enemies limit\n");
			IntermissionStart(g_pGameRules->GetBestPlayer(TEAM_NONE), m_pLastVictim);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override because CoOp game must not be interrupted in COOP_MODE_LEVEL
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::CheckLimits(void)
{
	DBG_GR_PRINT("CGameRulesCoOp::CheckLimits()\n");
	if (m_iGameMode == COOP_MODE_LEVEL)
	{
		return false;// XDM3038: TESTME
	}
	else if (m_iGameMode == COOP_MODE_SWEEP)
	{
		m_iRemainingScore = m_RegisteredTargets.size();// XDM3038a: 20150716 TESTME
		// don't return here!
	}
	return CGameRulesMultiplay::CheckLimits();// this will look into GetScoreLimit() which is overridden and dangerous!
}

//-----------------------------------------------------------------------------
// Purpose: Check if it is ok to end the game by force (time limit)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::CheckEndConditions(void)
{
	DBG_GR_PRINT("CGameRulesCoOp::CheckEndConditions()\n");
	if (m_iGameMode == COOP_MODE_LEVEL)
	{
		if (m_iChangeLevelTriggers > 0 && m_szNextMap[0])// no IS_MAP_VALID(m_szNextMap)) // we have next map
			return true;// even if some players did not reach the end of the level, time limit can end the game
		else
			return false;// we don't have any map to jump to
	}
	else
		return CGameRulesMultiplay::CheckEndConditions();
}

//-----------------------------------------------------------------------------
// Purpose: A player touched checkpoint. Called from CBasePlayer
// Input  : *pPlayer - 
//			*pCheckPoint - 
//-----------------------------------------------------------------------------
void CGameRulesCoOp::OnPlayerCheckPoint(CBasePlayer *pPlayer, CBaseEntity *pCheckPoint)// XDM3038
{
	if (pPlayer == NULL)
		return;

	DBG_GR_PRINT("CGameRulesMultiplay::OnPlayerCheckPoint(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pCheckPoint?pCheckPoint->entindex():0, pCheckPoint?STRING(pCheckPoint->pev->classname):"");

	if (pCheckPoint == NULL)
		return;

	int checkpointindex = pCheckPoint->entindex();
	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#CHECKPOINT\n", FStringNull(pCheckPoint->pev->targetname)?UTIL_dtos1(checkpointindex):STRING(pCheckPoint->pev->targetname));
	//bool bDangerousCheckpoint = false;
	if (pPlayer->IsOnTrain())
	{
		//bDangerousCheckpoint = true;
		conprintf(0, "Warning: player %d %s passed checkpoint on a train!\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	}
	else if (!FNullEnt(pPlayer->pev->groundentity) && FBitSet(pPlayer->pev->flags, FL_ONGROUND))
	{
		//bDangerousCheckpoint = true;
		CBaseEntity *pGroundEntity = CBaseEntity::Instance(pPlayer->pev->groundentity);
		if (pGroundEntity->IsMovingBSP() || !pGroundEntity->pev->velocity.IsZero())
			conprintf(0, "Warning: player %d %s passed checkpoint on a moving entity (%s[%d])!\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), STRING(pGroundEntity->pev->classname), pGroundEntity->entindex());
	}
	pPlayer->m_vecLastSpawnOrigin = pPlayer->pev->origin;// XDM3038: remember PLAYER's current position because trigger_autosave may be located somewhere else
	pPlayer->m_vecLastSpawnAngles = pPlayer->pev->angles;
	pPlayer->m_iLastSpawnFlags = pPlayer->pev->flags;// remember ducking, because it is VERY important!

	//CBaseEntity *pSearch = NULL;
	int foundindex = 0;
	for (m_ActivatedCheckPointsIterator = m_ActivatedCheckPoints.begin(); m_ActivatedCheckPointsIterator != m_ActivatedCheckPoints.end(); ++m_ActivatedCheckPointsIterator)
	{
		//pSearch = *m_ActivatedCheckPointsIterator;
		foundindex = *m_ActivatedCheckPointsIterator;
		//if (pSearch == pCheckPoint)
		if (foundindex == checkpointindex)
			break;
	}

	if (foundindex != checkpointindex)//if (pSearch != pCheckPoint)// not found
	{
		conprintf(0, "CGameRulesCoOp: adding new checkpoint: %s[%d] \"%s\".\n", STRING(pCheckPoint->pev->classname), checkpointindex, STRING(pCheckPoint->pev->targetname));
		//m_ActivatedCheckPoints.push_back(pCheckPoint);
		m_ActivatedCheckPoints.push_back(checkpointindex);
		m_vecForceSpawnSpot = pPlayer->pev->origin;
	}
	if (g_pGameRules->FAllowEffects())// XDM
	{
		MESSAGE_BEGIN(MSG_PVS, gmsgTeleport, pPlayer->pev->origin);
			WRITE_BYTE(0);// this is departure position
			WRITE_SHORT(pPlayer->entindex());
			WRITE_COORD(pPlayer->pev->origin.x);// pos
			WRITE_COORD(pPlayer->pev->origin.y);
			WRITE_COORD(pPlayer->pev->origin.z);
		MESSAGE_END();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Entity is being removed
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CGameRulesCoOp::OnEntityRemoved(CBaseEntity *pEntity)
{
	// A registered monster was removed
	if (m_iGameMode == COOP_MODE_SWEEP && m_iRegisteredTargets > 0)
	{
		if (UnregisterTarget(pEntity))
			return;
	}
	// WTF!! Somebody destroyed a checkpoint!!!
	int ei = pEntity->entindex();
	for (m_ActivatedCheckPointsIterator = m_ActivatedCheckPoints.begin(); m_ActivatedCheckPointsIterator != m_ActivatedCheckPoints.end(); ++m_ActivatedCheckPointsIterator)
	{
		if ((*m_ActivatedCheckPointsIterator) == ei)//if ((*m_ActivatedCheckPointsIterator) == pEntity)
		{
			m_ActivatedCheckPointsIterator = m_ActivatedCheckPoints.erase(m_ActivatedCheckPointsIterator);
			m_iRemainingScore = m_ActivatedCheckPoints.size();
			//conprintf(2, "CGameRulesCoOp: remaining check points: %u of %u\n", m_iRemainingScore, m_iRegisteredTargets);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: How many points awarded to anyone that kills this entity
// Warning: CONCEPT: relationship SHOULD NOT be checked here! Just amount of points
// Input  : *pAttacker - player/monster
//			*pKilled - player/monster
// Output : int
//-----------------------------------------------------------------------------
int CGameRulesCoOp::IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled)
{
	/*if (pKilled && pKilled->IsMonster())
		return pKilled->GetScoreAward();// XDM3038c //return SCORE_AWARD_NORMAL;

	// this returns "1"
	return CGameRulesMultiplay::IPointsForKill(pAttacker, pKilled);// for players*/
	if (pKilled)
	{
		if (pKilled->IsMonster() || pKilled->IsPlayer())
			return pKilled->GetScoreAward();// XDM3038c: those values are taken from skill cfg
	}
	return 0;//return CGameRulesMultiplay::IPointsForKill(pAttacker, pKilled);// just slow
}

//-----------------------------------------------------------------------------
// Purpose: A monster got killed, run logic
// Warning: Not guaranteed to be called for every monster!!!
// Input  : *pVictim - a monster that was killed
//			*pKiller - a player, monster or whatever it can be
//			*pInflictor - the actual entity that did the damage (weapon, projectile, etc.)
//-----------------------------------------------------------------------------
void CGameRulesCoOp::MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	ASSERT(pVictim != NULL);
	if (pVictim == NULL)
		return;
	DBG_GR_PRINT("CGameRulesCoOp::MonsterKilled(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->classname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");

	CBasePlayer *pPlayerKiller = NULL;
	CBaseMonster *pMonsterKiller = NULL;
	int iKiller = 0;
	int iVictim = pVictim->entindex();
	bool allykill = false;

	if (pKiller)
	{
		iKiller = pKiller->entindex();
		if (pKiller->IsPlayer())
			pPlayerKiller = (CBasePlayer *)pKiller;
		// both can and MUST be true! (see below)	else
			pMonsterKiller = pKiller->MyMonsterPointer();

		// pVictim was killed by the same entity twice!
		if (pVictim->m_hLastKiller == pKiller)// is not NULL already so don't need to check
		{
			//no	if (pVictim->m_iLastScoreAward == 0)// pVictim did not kill anyone after respawning
			{
			MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);// let everyone laugh?
				WRITE_BYTE(GAME_EVENT_LOSECOMBO);
				WRITE_SHORT(iKiller);//hard to use this for monsters because of indexes and lack of information on client side
				WRITE_SHORT(iVictim);
			MESSAGE_END();
			}
			// multiple?	pVictim->m_hLastKiller = NULL;
		}
		else
			pVictim->m_hLastKiller = pKiller;// remember new killer

		if (pMonsterKiller)// || pPlayerKiller public CBaseMonster)// must be valid for monsters AND players
			pMonsterKiller->m_hLastVictim = pVictim;

		if (pVictim->pev == pKiller->pev)// killed self
		{
			AddScore(pKiller, -SCORE_AWARD_NORMAL);
		}
		else if (pPlayerKiller && !pPlayerKiller->IsObserver())// && pKiller->IsPlayer()) // XDM3035: don't award spectators!
		{
			// pKiller's killer is now a victim. Check BEFORE awarding!
			if (pPlayerKiller->m_hLastKiller == pVictim)
			{
				if (pPlayerKiller->m_iLastScoreAward == 0)// this is the FIRST victim after respawn
				{
					MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ALL:MSG_BROADCAST), gmsgGREvent);// let everyone know
						WRITE_BYTE(GAME_EVENT_REVENGE);
						WRITE_SHORT(iKiller);
						WRITE_SHORT(iVictim);
					MESSAGE_END();
					pPlayerKiller->m_Stats[STAT_REVENGE_COUNT]++;// XDM3037
					if (mp_revengemode.value > 0.0f)
						AddScore(pKiller, SCORE_AWARD_NORMAL);// extra score for killing your killer!
				}
				pPlayerKiller->m_hLastKiller = NULL;// TODO TESTME!!!!! only once! (critical for mp_revengemode)
			}

			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			int relation = g_pGameRules->PlayerRelationship(pPlayerKiller, pVictim);
			if (relation == GR_NOTTEAMMATE || relation == GR_ENEMY)
			{
				AddScore(pKiller, IPointsForKill(pKiller, pVictim));// +
			}
			else if (relation == GR_TEAMMATE || relation == GR_ALLY)
			{
				allykill = true;
				AddScore(pKiller, -IPointsForKill(pKiller, pVictim));// -
			}

			if (mp_firetargets_player.value > 0)// TODO: mp_firetargets_monster?
				FireTargets("game_monsterkill", pKiller, pKiller, USE_TOGGLE, 0);

			if (pVictim->m_LastHitGroup == HITGROUP_HEAD)// XDM3037: headshot
			{
				pPlayerKiller->m_Stats[STAT_HEADSHOT_COUNT]++;
				MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgGREvent, NULL, pPlayerKiller->edict());
					WRITE_BYTE(GAME_EVENT_HEADSHOT);
					WRITE_SHORT(iKiller);
					WRITE_SHORT(iVictim);
				MESSAGE_END();
			}
		}
		else  if (pKiller->IsMonster())// monster kills another monster
		{
			AddScore(pKiller, IPointsForKill(pKiller, pVictim));// award the killer monster
		}
		else// killed by the world
		{
			pVictim->m_hLastKiller = NULL;
			// nobody cares if monster is killed by the world		AddScore(pVictim, -SCORE_AWARD_NORMAL);
		}
	}
	//AddScore(pKiller, IPointsForKill(pKiller, pVictim));
	CGameRulesMultiplay::MonsterKilled(pVictim, pKiller, pInflictor);// <- stats are there

	UnregisterTarget(pVictim);// XDM3038a: NOT GUARANTEED TO BE CALLED!
	/* old	if (m_iGameMode == COOP_MODE_SWEEP && m_iRegisteredTargets > 0)
	{
		for (m_TargetIterator = m_RegisteredTargets.begin(); m_TargetIterator != m_RegisteredTargets.end(); ++m_TargetIterator)
		{
			//if ((*m_TargetIterator) == pVictim)
			if ((*m_TargetIterator) == iVictim)
			{
				m_TargetIterator = m_RegisteredTargets.erase(m_TargetIterator);
		//^		if (m_iGameMode == COOP_MODE_SWEEP)
				{
					m_iRemainingScore = m_RegisteredTargets.size();
					//conprintf(2, "CGameRulesCoOp: remaining monsters: %u of %u\n", m_iRemainingScore, m_iRegisteredTargets);
				}
				break;
			}
		}
	}*/
	//if (pSearch != pVictim) {}// not found

	// update the scores
	// killed scores
	/* monsters aren't shown on score board
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_???(ENTINDEX(pVictim->edict()));
		WRITE_SHORT((int)pVictim->pev->frags);
		WRITE_SHORT(pVictim->m_iDeaths);
	MESSAGE_END();*/

	// killers score, if it's a player
	if (pPlayerKiller)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
			WRITE_BYTE(ENTINDEX(pPlayerKiller->edict()));
			WRITE_SHORT((int)pPlayerKiller->pev->frags);
			WRITE_SHORT(pPlayerKiller->m_iDeaths);
		MESSAGE_END();
		// let the killer paint another decal as soon as he'd like.
		pPlayerKiller->m_flNextDecalTime = gpGlobals->time;
	}

	// XDM3038: moved to the end
	DeathNotice(pVictim, pKiller, pInflictor);

	if (mp_firetargets_player.value > 0)// TODO: mp_firetargets_monster?
		FireTargets("game_monsterdie", pVictim, pVictim, USE_TOGGLE, 0);

	CheckLimits();
}

//-----------------------------------------------------------------------------
// Purpose: Entity restrictions may apply here, register monsters
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::IsAllowedToSpawn(CBaseEntity *pEntity)
{
	if (!CGameRulesMultiplay::IsAllowedToSpawn(pEntity))// XDM3038a: don't register anything that won't be added to game
		return false;

	if (!pEntity->IsProjectile())
	{
		if (pEntity->IsMonster())
		{
			// XDM3038a: 20150716 if (mp_monstersrespawn.value > 0.0f)
				// No! What about monstermaked monsters?? ClearBits(pEntity->pev->spawnflags, SF_NORESPAWN);
			if (mp_monstersrespawn.value <= 0.0f)
				SetBits(pEntity->pev->spawnflags, SF_NORESPAWN);

			// TODO: UNDONE: CONSIDER REVISIT: use of IRelationship is ureliable because it is dynamic
			if (g_iRelationshipTable[pEntity->Classify()][CLASS_PLAYER] > R_NO)// dislike or hate players
			{
				if (!FBitSet(pEntity->pev->flags, FL_GODMODE) && !FBitSet(pEntity->pev->spawnflags, SF_MONSTER_INVULNERABLE))// don't register those who we cannot kill
				{
					//m_RegisteredTargets.push_back(EHANDLE(pEntity));// STL is horrible// TODO: is temporary ok?
					m_RegisteredTargets.push_back(pEntity->entindex());
					conprintf(2, "CGameRulesCoOp: registered hostile monster: %s[%d] \"%s\" (#%u)\n", STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->targetname), m_iRegisteredTargets);
					++m_iRegisteredTargets;
				}
			}
			SetBits(m_iGameLogicFlags, COOP_MAP_HAS_MONSTERS);
		}
		else if (pEntity->IsTrigger())
		{
			if (FClassnameIs(pEntity->pev, "trigger_changelevel"))
			{
				if (pEntity->pev->solid != SOLID_NOT)// is valid
				{
					SERVER_PRINTF("* Registered %s[%d] \"%s\" (#%u)\n", STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->targetname), m_iChangeLevelTriggers);
					++m_iChangeLevelTriggers;

					if (m_szLastMap[0] && _strnicmp(m_szLastMap, ((CChangeLevel *)pEntity)->m_szMapName, MAX_MAPNAME) == 0)// trigger points to a previous map
					{
						pEntity->pev->rendercolor.Set(0,63,255);
						pEntity->pev->teleport_time = GetStartTime() + 90.0f;// become active later
						SetBits(m_iGameLogicFlags, COOP_MAP_HAS_TRANSITION_PREVIOUS);
					}
					else
					{
						pEntity->pev->rendercolor.Set(0,255,0);
						pEntity->pev->teleport_time = GetStartTime() + 60.0f;// become active sooner
						// triggers don't have it if (pEntity->pev->watertype ==), the only way to find out is to find corresponding spawn spots and look into their pev->watertype which is tedious for now
						SetBits(m_iGameLogicFlags, COOP_MAP_HAS_TRANSITION_NEXT);
					}
				}
			}
			else if (FClassnameIs(pEntity->pev, "trigger_endsection"))
			{
				SetBits(m_iGameLogicFlags, COOP_MAP_HAS_END);
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Should this entity become dormant after level transiiton
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FShouldMakeDormant(CBaseEntity *pEntity)
{
	if (GetGameMode() == COOP_MODE_LEVEL)
	{
		//vEndOldOffset = pEntity->pev->origin - m_vLastLandMark;
		//vEndNewOffset = pEntity->pev->neworigin - m_hEntLandmark->pev->origin;
		//vEndOldOffset == vEndNewOffset
		//pEntity->pev->neworigin = vOffset + m_hEntLandmark->pev->origin;
		// m_hEntLandmark is NULL Vector vNewOrigin = (pEntity->pev->origin - m_vLastLandMark) + m_hEntLandmark->pev->origin;
		if (POINT_CONTENTS(pEntity->pev->origin) != CONTENTS_SOLID)//if (POINT_CONTENTS(vNewOrigin) != CONTENTS_SOLID)
		{
			//pEntity->pev->origin = vNewOrigin;
			return false;
		}
	}
	return CGameRulesMultiplay::FShouldMakeDormant(pEntity);
}

//-----------------------------------------------------------------------------
// Purpose: Should this entity be shown on the minimap?
// Input  : *pEntity - 
//			*pPlayer - map owner
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool CGameRulesCoOp::FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer)
{
	if (CGameRulesMultiplay::FShowEntityOnMap(pEntity, pPlayer))
		return true;

	if (pEntity->IsMonster())
		return true;

	return false;
}*/

//-----------------------------------------------------------------------------
// Purpose: Are players allowed to switch to spectator mode in game? (cheat!)
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FAllowSpectatorChange(CBasePlayer *pPlayer)
{
	if (m_iGameMode == COOP_MODE_LEVEL && (mp_coop_eol_spectate.value > 0.0f) && pPlayer->pev->landmark == m_hEntLandmark.Get() && !pPlayer->IsObserver())// this player finished the level, AND is not trying to join the game again
		return true;
	else
		return CGameRulesMultiplay::FAllowSpectatorChange(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Always transfer my instance to the next map if playing campaign
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FPersistBetweenMaps(void)
{
	if (m_iGameMode == COOP_MODE_LEVEL)
	{
		if (g_pWorld)
			if (FBitSet(g_pWorld->pev->spawnflags, SF_WORLD_NEWUNIT))// XDM3038c: reset all data when it comes to new campaign. g_pWorld is the NEW map.
				return false;

		return true;
	}
	return CGameRulesMultiplay::FPersistBetweenMaps();
}

//-----------------------------------------------------------------------------
// Purpose: Has this player reached the goal? (used only by COOP_MODE_LEVEL)
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FPlayerFinishedLevel(CBasePlayer *pPlayer)
{
	if (m_iGameMode == COOP_MODE_LEVEL)
	{
		if (pPlayer->pev->landmark != NULL)// WARNING! landmark may be changed AFTER the player touched the trigger
		{
			if (pPlayer->pev->landmark == m_hEntLandmark.Get())// check if this is the right landmark (there may be many)
			{
				return true;
			}
			else
			{
				DBG_GR_PRINT("CGameRulesCoOp::FPlayerFinishedLevel(%d %s): player with wrong landmark.\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player take damage from this attacker?
// Input  : *pPlayer - 
//			*pAttacker - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
	if (pAttacker && pAttacker->IsPlayer())
	{
		if ((mp_friendlyfire.value <= 0.0f) && (pAttacker != pPlayer))
			return false;
	}
	return CGameRulesMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

//-----------------------------------------------------------------------------
// Purpose: Should this entity be shown on the minimap?
// Input  : *pEntity - 
//			*pPlayer - map owner
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool CGameRulesCoOp::FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer)
{
	if (m_iGameMode == COOP_MODE_LEVEL)
	{
		if (pEntity->IsTrigger())
			Already IsGoal	if (FClassnameIs(pEntity->pev, "trigger_changelevel"));
				return true;
	}
	else if (m_iGameMode == COOP_MODE_SWEEP)
	{
		if (pEntity in m_RegisteredTargets)
			return true;
	}
	return CGameRulesMultiplay::FShowEntityOnMap(pEntity, pPlayer);
}*/

//-----------------------------------------------------------------------------
// Purpose: Player is spawning, add default items to his inventory
// Input  : *pPlayer - 
// Output : size_t - number of items added
//-----------------------------------------------------------------------------
size_t CGameRulesCoOp::PlayerAddDefault(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesCoOp::PlayerAddDefault(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	size_t count = CGameRulesMultiplay::PlayerAddDefault(pPlayer);

	if (FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_INVENTORY))// XDM3038b
		count += RestorePlayerData(pPlayer);

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: Relationship determines score assignment and other important stuff
// Warning: DO NOT call ->IRelationship from here!!!
// Input  : *pPlayer - subject
//			*pTarget - object
// Output : int GR_NEUTRAL
//-----------------------------------------------------------------------------
int CGameRulesCoOp::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	if (pTarget->IsPlayer())
	{
		return GR_ALLY;
	}
	else if (pTarget->IsMonster())
	{
		// RECURSION! if (pKilled->MyMonsterPointer()->IRelationship(pAttacker) > R_NO)// may change dynamically, which is bad...
		//int r = g_iRelationshipTable[pKilled->Classify()][pAttacker->Classify()];// XDM3035b: use base relationship, not dynamic
		//int r = g_iRelationshipTable[pAttacker->Classify()][pKilled->Classify()];// works better for barnacles
		int r = g_iRelationshipTable[pPlayer->Classify()][pTarget->Classify()];// because "bad monsters" were registered as targets
		if (r >= R_DL)// enemy
			return GR_ENEMY;
		else if (r == R_FR)// fear
			return GR_NOTTEAMMATE;// we should give frags for killing those who we fear, right?
		else if (r == R_AL)// ally
			return GR_ALLY;

		return GR_NEUTRAL;
	}
	else
		return CGameRulesMultiplay::PlayerRelationship(pPlayer, pTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Is this spawn spot valid for specified player?
// Input  : *pPlayer - 
//			pSpot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::ValidateSpawnPoint(CBaseEntity *pPlayer, CBaseEntity *pSpot)// XDM3038a
{
	DBG_GR_PRINT("CGameRulesCoOp::ValidateSpawnPoint(%d %s, %d %s)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"", pSpot?pSpot->entindex():0, pSpot?STRING(pSpot->pev->classname):"");
	if (pSpot == NULL)
		return false;
	if (m_szLastLandMark[0] != 0)// level was changed
	{
		if (FStringNull(pSpot->pev->message))// do we allow players to spawn at spots without landmark name specified?
		{
			if (mp_coop_allowneutralspots.value <= 0)
			{
				ASSERT(pSpot->pev->watertype == PLAYERSTART_TYPE_RANDOM);// it may be designer's fault
				return false;
			}
		}
		else if (strncmp(STRING(pSpot->pev->message), m_szLastLandMark, MAX_MAPNAME) != 0)
			return false;
	}
	else if (pSpot->pev->watertype != PLAYERSTART_TYPE_RANDOM && pSpot->pev->watertype != PLAYERSTART_TYPE_MAPSTART)// fresh map start (there was no level change)
	{
		if (!FBitSet(pSpot->pev->spawnflags, SF_PLAYERSTART_ALLOWSTART))// map designer allowed game to start here
			return false;// there's no previous map, spawn only at the start or random points
	}
	return CGameRulesMultiplay::ValidateSpawnPoint(pPlayer, pSpot);
}

//-----------------------------------------------------------------------------
// Purpose: Force player to start at specified position
// Input  : *pPlayer - 
// Output : vecSpot - is set only when needed
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FForcePlayerSpawnSpot(CBasePlayer *pPlayer, Vector &vecSpot)// XDM3038
{
	CBaseEntity *pLastSpot = GetLastCheckPoint();// Not pPlayer->!!!!!!!!
	if (m_iGameMode == COOP_MODE_LEVEL && pLastSpot && (mp_coop_commonspawnspot.value > 0))
	{
		vecSpot = m_vecForceSpawnSpot;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Special case if we're using autosave/checkpoints
// Input  : *pPlayer - 
//			bSpectator - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CGameRulesCoOp::PlayerUseSpawnSpot(CBasePlayer *pPlayer, bool bSpectator)
{
	DBG_GR_PRINT("CGameRulesCoOp::PlayerUseSpawnSpot(%d %s, %d)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), bSpectator?1:0);
	CBaseEntity *pPlayerLastSpot = pPlayer->GetLastCheckPoint();
	Vector vecSpawn(0,0,0);
	bool bForceSpawnSpot = FForcePlayerSpawnSpot(pPlayer, vecSpawn);
	if (m_iGameMode == COOP_MODE_LEVEL && (pPlayerLastSpot || bForceSpawnSpot))
	{
		if (bForceSpawnSpot == false)// if we don't use same spot for all players
			vecSpawn = pPlayer->m_vecLastSpawnOrigin;// use personal

		if (SpawnPointCheckObstacles(pPlayer, vecSpawn, true, mp_spawnkill.value > 0.0f) == false)
		{
			conprintf(1, "CGameRulesCoOp::PlayerUseSpawnSpot(%d %s, %d): player blocked at start spot!\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), bSpectator?1:0);
			return NULL;// player will fail to start and become a spectator
		}
		else
		{
			pPlayer->pev->oldorigin = pPlayer->pev->origin;
			pPlayer->pev->origin = vecSpawn;// XDM3038
			pPlayer->pev->origin.z += 1.0f;
			pPlayer->pev->velocity.Clear();
			pPlayer->pev->v_angle.Clear();
			pPlayer->pev->punchangle.Clear();
			pPlayer->pev->angles = pPlayer->m_vecLastSpawnAngles;
			pPlayer->pev->fixangle = TRUE;
			pPlayer->pev->flags |= (pPlayer->m_iLastSpawnFlags & SPAWNSPOT_SAVE_PLAYERFLAGS);// restore ONLY NECESSARY flags!
			UTIL_SetOrigin(pPlayer, pPlayer->pev->origin);//why not?
		}
		if (bForceSpawnSpot)
			return GetLastCheckPoint();
		else
			return pPlayerLastSpot;
	}
	else
		return CGameRulesMultiplay::PlayerUseSpawnSpot(pPlayer, bSpectator);
}

//-----------------------------------------------------------------------------
// Purpose: Score limit for this game type
// Output : uint32
//-----------------------------------------------------------------------------
uint32 CGameRulesCoOp::GetScoreLimit(void)
{
//	DBG_GR_PRINT("CGameRulesCoOp::GetScoreLimit()\n");
	if (m_iGameMode == COOP_MODE_SWEEP)// && m_iRegisteredTargets > 0)
		return 0;// XDM3038c: NO! We can get 25 score for one monster! m_iRegisteredTargets;
	else if (m_iGameMode == COOP_MODE_LEVEL)
		return CountPlayers();// XDM3038: new. m_iLastCheckedNumActivePlayers;// those who haven't reached the goal yet
	else
		return CGameRulesMultiplay::GetScoreLimit();
}

//-----------------------------------------------------------------------------
// Purpose: How much score remaining until end of game
// Output : uint32
//-----------------------------------------------------------------------------
uint32 CGameRulesCoOp::GetScoreRemaining(void)
{
//	DBG_GR_PRINT("CGameRulesCoOp::GetScoreRemaining()\n");
	if (m_iGameMode == COOP_MODE_LEVEL)
		return m_iLastCheckedNumActivePlayers;// XDM3038
	else
		return CGameRulesMultiplay::GetScoreRemaining();// returns m_iRemainingScore which holds right values for COOP_MODE_SWEEP and COOP_MODE_MONSTERFRAGS
}

//-----------------------------------------------------------------------------
// Purpose: Real number of active players on server
// Warning: Overloaded to also count some important numbers!
// Output : uint32
//-----------------------------------------------------------------------------
uint32 CGameRulesCoOp::CountPlayers(void)
{
//	DBG_GR_PRINT("CGameRulesCoOp::CountPlayers()\n");
	uint32 iNumConnectedPlayers = 0;
	m_iLastCheckedNumActivePlayers = 0;
	m_iLastCheckedNumFinishedPlayers = 0;
	m_iLastCheckedNumTransferablePlayers = 0;
	for (CLIENT_INDEX ci = 1; ci <= gpGlobals->maxClients; ++ci)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(ci);
		if (pPlayer)
		{
			++iNumConnectedPlayers;
			// Check before spectators! Because spectator may be someone who finished this map!
			if (FPlayerFinishedLevel(pPlayer))
			{
				++m_iLastCheckedNumFinishedPlayers;
				if (pPlayer->IsObserver() || mp_coop_checktransition.value <= 0.0f || InTransitionVolume(pPlayer, m_hEntLandmark.Get()?STRING(m_hEntLandmark->pev->targetname):NULL))
					++m_iLastCheckedNumTransferablePlayers;// some players may have finished the map, but are not in transition volume in which case we must deny map change
#if defined (_DEBUG)
				else
					conprintf(0, "CGameRulesCoOp::CountPlayers(): player %d \"%s\" is not in transition volume!\n", ci, STRING(pPlayer->pev->netname));
#endif
			}
			else if (FPlayerIsActive(pPlayer))// there may be some spectators who WERE NOT playing and HAVE NOT FINISHED the level
				++m_iLastCheckedNumActivePlayers;// so this must be an active player
		}
	}
	return iNumConnectedPlayers;
}

//-----------------------------------------------------------------------------
// Purpose: Get player with the best score (or first got to the end of level)
// Input  : teamIndex - may be TEAM_NONE
// Output : CBasePlayer *
//-----------------------------------------------------------------------------
CBasePlayer *CGameRulesCoOp::GetBestPlayer(TEAM_ID teamIndex)
{
//	DBG_GR_PRINT("CGameRulesCoOp::GetBestPlayer(TEAM_ID %hd)\n", teamIndex);
	if (m_iGameMode == COOP_MODE_LEVEL && (mp_coop_eol_firstwin.value > 0.0f))
	{
		if (m_hFirstPlayer.Get() && m_hFirstPlayer->IsPlayer())
		{
			DBG_GR_PRINT("CGameRulesCoOp::GetBestPlayer(%d): first %d %s\n", teamIndex, m_hFirstPlayer->entindex(), STRING(m_hFirstPlayer->pev->netname));
			return (CBasePlayer *)(CBaseEntity *)m_hFirstPlayer;
		}
	}
	return CGameRulesMultiplay::GetBestPlayer(teamIndex);
}

//-----------------------------------------------------------------------------
// Purpose: trigger_changelevel was touched
//			We don't allow the trigger to change level itself, instead we just remember sufficient parameters
//			and start intermission when ready (the map will be changed normally after that).
// Warning: May get called multiple times or even every frame.
// Input  : *pActivator - 
//			*szNextMap - 
//			*pEntLandmark - optional
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::FAllowLevelChange(CBasePlayer *pActivator, const char *szNextMap, edict_t *pEntLandmark)
{
	DBG_GR_PRINT("CGameRulesCoOp::FAllowLevelChange(%d, %s, %s)\n", pActivator?pActivator->entindex():0, szNextMap, pEntLandmark?STRING(pEntLandmark->v.targetname):"");
	if (m_iGameMode != COOP_MODE_LEVEL)
		return false;
	if (IsGameOver())
		return CGameRulesMultiplay::FAllowLevelChange(pActivator, szNextMap, pEntLandmark);// XDM3038c

	//bool bTryingGoBackWhileHavingNextMap = false;
	// don't allow last player to change the next map and fail the match (others may be spectating by now)
	if (m_szNextMap[0] == 0 || strcmp(m_szNextMap, szNextMap) != 0)// && m_iLastCheckedNumActivePlayers > 1))// XDM3038: switch everytime ANY player touches new trigger
	{
		if (FBitSet(m_iGameLogicFlags, COOP_MAP_HAS_TRANSITION_NEXT|COOP_MAP_HAS_END) && FStrEq(szNextMap, m_szLastMap) && (CountPlayersHaveLandmark(pEntLandmark) < m_iLastCheckedNumActivePlayers))// this trigger points to the previous map
		{
			if (pActivator) ClientPrint(pActivator->pev, HUD_PRINTCENTER, "#COOP_BADNEXTMAP_BK", m_szNextMap);// tell this player that he cannot change level back alone
			DBG_GR_PRINT("CGameRulesCoOp::FAllowLevelChange(): m_szNextMap is not set\n");
		}
		else
		{
			strcpy(m_szNextMap, szNextMap);
			SERVER_PRINTF("GAME: next map set to \"%s\"\n", m_szNextMap);
			ClientPrint(NULL, HUD_PRINTCENTER, "#COOP_SETNEXTMAP", m_szNextMap, pActivator?STRING(pActivator->pev->netname):NULL);
			m_hEntLandmark.Set(pEntLandmark);// must be set before spectating

			// Remember the first player who finished the map
			if (m_hFirstPlayer.Get() == NULL)
				m_hFirstPlayer = pActivator;
		}
	}

	// Change player's landmark and play visual effects
	if (pEntLandmark && pActivator && pActivator->pev->landmark != pEntLandmark)
	{
		UTIL_EmitAmbientSound(pEntLandmark, pEntLandmark->v.origin, "game/dom_touch.wav", VOL_NORM, ATTN_NORM, 0, 110);// pActivator->edict()?
		if (pActivator->pev->landmark == NULL)// touched a trigger for the first time
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgGREvent);
				WRITE_BYTE(GAME_EVENT_PLAYER_FINISH);
				WRITE_SHORT(pActivator->entindex());
				WRITE_SHORT(pActivator->pev->team);
			MESSAGE_END();
			SetBits(pActivator->m_iGameFlags, EGF_REACHEDGOAL);// XDM3038c
			pActivator->m_Stats[STAT_ENDLEVEL_COUNT]++;// XDM3038c: once per map
		}
		if (g_pGameRules->FAllowEffects())
		{
			MESSAGE_BEGIN(MSG_PVS, gmsgTeleport, pActivator->pev->origin);
				WRITE_BYTE(MSG_TELEPORT_FL_SOUND);// this is departure position
				WRITE_SHORT(pActivator->entindex());
				WRITE_COORD(pActivator->pev->origin.x);// pos
				WRITE_COORD(pActivator->pev->origin.y);
				WRITE_COORD(pActivator->pev->origin.z);
			MESSAGE_END();
		}
		pActivator->pev->landmark = pEntLandmark;
		if (mp_coop_eol_spectate.value > 0.0f && g_pGameRules->FAllowSpectatorChange(pActivator))// why check "allow"?
		{
			if (FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_INVENTORY))// XDM3038c: save now, while inventory is full
				SavePlayerData(pActivator);

			pActivator->ObserverStart(pActivator->pev->origin, pActivator->pev->angles, OBS_ROAMING, NULL);
		}
		if (sv_lognotice.value > 0.0f)
			UTIL_LogPrintf("\"%s<%i><%s>\" had landmark change to \"%s\"\n", STRING(pActivator->pev->netname), GETPLAYERUSERID(pActivator->edict()), GETPLAYERAUTHID(pActivator->edict()), pEntLandmark?STRING(pEntLandmark->v.targetname):"(none)");
	}

	// Now let's see if this was the last one
	if (CheckPlayersTouchedTriggers(true))
	{
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)// move everyone to landmark before changing level
		{
			CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
			if (pPlayer)
				pPlayer->pev->landmark = m_hEntLandmark.Get();
		}
		SERVER_PRINTF("GAME: ended by transition activation: %s %s (%s)\n", szNextMap, m_hEntLandmark.Get()?STRING(m_hEntLandmark->pev->targetname):"(no landmark)", (m_szLastMap[0] && _stricmp(m_szLastMap, szNextMap) == 0)?"previous":"next");
		IntermissionStart(g_pGameRules->GetBestPlayer(TEAM_NONE), m_pLastVictim);
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: EndOfGame. Show score board, camera shows the winner
// Input  : *pWinner -
//			*pInFrameEntity - some secondary entity to show in focus
//-----------------------------------------------------------------------------
void CGameRulesCoOp::IntermissionStart(CBasePlayer *pWinner, CBaseEntity *pInFrameEntity)
{
	if (!IsGameOver())
	{
		CGameRulesMultiplay::IntermissionStart(pWinner, pInFrameEntity);
		if (g_pWorld)
		{
			if (g_pWorld->m_iNumSecrets > 0)
				ClientPrint(NULL, HUD_PRINTTALK, "#SECRETSFOUND\n", UTIL_dtos1(gpGlobals->found_secrets), UTIL_dtos2(g_pWorld->m_iNumSecrets));
		}
		if (m_iGameMode == COOP_MODE_LEVEL)// && IS_MAP_VALID(m_szNextMap))
		{
			if (m_szNextMap[0])
				ClientPrint(NULL, HUD_PRINTTALK, "#MP_NEXTMAP\n", m_szNextMap);// XDM3038b: announce next map
			else if (FBitSet(m_iGameLogicFlags, COOP_MAP_HAS_TRANSITION_NEXT) && !FBitSet(m_iGameLogicFlags, COOP_MAP_HAS_END))
				ClientPrint(NULL, HUD_PRINTTALK, "#MP_NEXTMAPFAIL\n");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server is changing to a new level
// In CoOp all players touch trigger_changelevel to start intermission and THEN
// this function gets called.
//-----------------------------------------------------------------------------
void CGameRulesCoOp::ChangeLevel(void)
{
	DBG_GR_PRINT("CGameRulesCoOp::ChangeLevel()\n");
	if (m_iGameMode == COOP_MODE_LEVEL && m_iChangeLevelTriggers > 0 && IsGameOver() && IS_MAP_VALID(m_szNextMap))
	{
		m_pIntermissionEntity1 = NULL;
		m_pIntermissionEntity2 = NULL;
		strcpy(m_szLastMap, STRING(gpGlobals->mapname));// remember current map
		if (m_hEntLandmark.Get())
		{
			strncpy(m_szLastLandMark, STRING(m_hEntLandmark->pev->targetname), MAX_MAPNAME);// XDM3038a
			m_vLastLandMark = m_hEntLandmark->pev->origin;
		}
		else
		{
			memset(m_szLastLandMark, 0, MAX_MAPNAME);
			m_vLastLandMark.Clear();
		}
		if (mp_coop_keepscore.value > 0)// XDM3038a
			SetBits(m_iPersistBetweenMaps, GR_PERSIST_KEEP_SCORE|GR_PERSIST_KEEP_LOSES);

		if (mp_coop_keepinventory.value > 0)// XDM3038b: apply policy
			SetBits(m_iPersistBetweenMaps, GR_PERSIST_KEEP_INVENTORY);

		if (FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_INVENTORY))// XDM3038b: this bit may be added somewhere else
			SavePlayersData();

		SetBits(m_iPersistBetweenMaps, GR_PERSIST_KEEP_STATS);// always keep stats when playing sequential maps

		conprintf(1, "CHANGE LEVEL: %s (LM %s) (CoOp)\n", m_szNextMap, m_szLastLandMark);
		CHANGE_LEVEL(m_szNextMap, NULL);//m_hEntLandmark.Get()?STRINGV(m_hEntLandmark.Get()->v.targetname):NULL);
		// TODO: implement local save/restore mechanism (FPersistBetweenMaps allows to)
		// NOTE: global entities are somehow already saved!
	}
	else
	{
		CGameRulesMultiplay::ChangeLevel();
		//memset(m_szLastMap, 0, MAX_MAPNAME);// XDM3038a: probably better to not to touch this
	}
}

//-----------------------------------------------------------------------------
// Purpose: Can the server change levels? (used by triggers)
// Check if all players touched the same one trigger!
// Input  : bCheckTransitionVolume
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::CheckPlayersTouchedTriggers(bool bCheckTransitionVolume)
{
	DBG_GR_PRINT("CGameRulesCoOp::CheckPlayersTouchedTriggers(%d)\n", bCheckTransitionVolume?1:0);
	if (m_iChangeLevelTriggers > 0 && m_hEntLandmark.Get() != NULL)
	{
		uint32 iTotalPlayers = CountPlayers();
		DBG_GR_PRINT("CGameRulesCoOp::CheckPlayersTouchedTriggers(%d): iTotalPlayers = %u, m_iLastCheckedNumActivePlayers = %u, m_iLastCheckedNumFinishedPlayers = %u\n", bCheckTransitionVolume?1:0, iTotalPlayers, m_iLastCheckedNumActivePlayers, m_iLastCheckedNumFinishedPlayers);
		if (iTotalPlayers == 0 || m_iLastCheckedNumFinishedPlayers == 0)// during level change
			return false;

		if (m_iLastCheckedNumTransferablePlayers == iTotalPlayers)// XDM3038: not only finished players, but also in transition
		{
			ASSERT(m_iLastCheckedNumActivePlayers == 0);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Last activated check point
// Output : CBaseEntity * - last touched trigger_autosave
//-----------------------------------------------------------------------------
CBaseEntity *CGameRulesCoOp::GetLastCheckPoint(void)
{
	//std::vector<CBaseEntity *>::size_type sz = m_ActivatedCheckPoints.size();
	std::vector<int>::size_type sz = m_ActivatedCheckPoints.size();
	if (sz > 0)
		return UTIL_EntityByIndex(m_ActivatedCheckPoints.back());// XDM3038a: proper way
		//return m_ActivatedCheckPoints.back();// XDM3038a: proper way

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Remove monster from our registry of targets
// Warning: May be called multiple times for one entity!
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesCoOp::UnregisterTarget(CBaseEntity *pEntity)
{
	DBG_GR_PRINT("CGameRulesMultiplay::UnregisterTarget(%d %s)\n", pEntity?pEntity->entindex():0, pEntity?STRING(pEntity->pev->classname):"");
	if (pEntity == NULL)
		return false;

	int ei = pEntity->entindex();
	for (m_TargetIterator = m_RegisteredTargets.begin(); m_TargetIterator != m_RegisteredTargets.end(); ++m_TargetIterator)
	{
		if ((*m_TargetIterator) == ei)//if ((*m_TargetIterator) == pEntity)
		{
			m_TargetIterator = m_RegisteredTargets.erase(m_TargetIterator);
			if (m_iGameMode == COOP_MODE_SWEEP)// XDM3038a: 20150716
			{
				m_iRemainingScore = m_RegisteredTargets.size();
				//conprintf(2, "CGameRulesCoOp: remaining monsters: %u of %u\n", m_iRemainingScore, m_iRegisteredTargets);
			}
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Save inventory for all players
// Warning: Right now it is a HACK
// Warning: Observers have their inventories saved before level change
//-----------------------------------------------------------------------------
void CGameRulesCoOp::SavePlayersData(void)
{
	DBG_GR_PRINT("CGameRulesCoOp::SavePlayersData()\n");
	//SV_SaveGameState(m_pSaveData);
	for (CLIENT_INDEX ci = 1; ci <= gpGlobals->maxClients; ++ci)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(ci);
		if (pPlayer)
		{
			if (!pPlayer->IsObserver())// observers are saved earlier because they lose inventory
				SavePlayerData(pPlayer);
		}
		else
			m_szTransitionInventoryPlayerIDs[ci-1] = 0;// mark slot as empty
	}
}

//-----------------------------------------------------------------------------
// Purpose: Save inventory for a player
// Warning: Right now it is a HACK
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesCoOp::SavePlayerData(CBasePlayer *pPlayer)
{
	//DBG_GR_PRINT("CGameRulesCoOp::SavePlayerData(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	conprintf(2, "CGameRulesCoOp::SavePlayerData(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	//SV_SaveGameState(m_pSaveData);
	CLIENT_INDEX ci = pPlayer->entindex();
	int iClientUserID = GETPLAYERUSERID(pPlayer->edict());
	ASSERT(iClientUserID != 0);
	if (iClientUserID == 0)
	{
		conprintf(0, "SavePlayerData(%d %s) ERROR: unable to get player's User ID!\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
		if (IS_DEDICATED_SERVER())// safety is more important here
			return;

		conprintf(0, "Using index instead!\n");
		iClientUserID = ci;//return;
	}
	m_szTransitionInventoryPlayerIDs[ci-1] = iClientUserID;
	size_t i;
	for (i = 0; i<PLAYER_INVENTORY_SIZE; ++i)
	{
		CBasePlayerItem *pItem = pPlayer->GetInventoryItem(i);
		if (pItem)
			strncpy(m_szTransitionInventoryItems[ci-1][i], STRING(pItem->pev->classname), MAX_ENTITY_STRING_LENGTH);
		else
			m_szTransitionInventoryItems[ci-1][i][0] = 0;
	}
	for (i = 0; i<MAX_AMMO_SLOTS; ++i)
		m_szTransitionInventoryAmmo[ci-1][i] = pPlayer->m_rgAmmo[i];

	m_szTransitionInventoryArmor[ci-1] = pPlayer->pev->armorvalue;
	m_szTransitionInventoryLJM[ci-1] = (pPlayer->m_fLongJump == TRUE);
}

//-----------------------------------------------------------------------------
// Purpose: Restore inventory for a player
// Warning: Right now it is a HACK
// Note   : Clear saved values after restore
// Input  : *pPlayer - 
// Output : size_t - number of items added
//-----------------------------------------------------------------------------
size_t CGameRulesCoOp::RestorePlayerData(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesCoOp::RestorePlayerData(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	int iClientUserID = GETPLAYERUSERID(pPlayer->edict());
	ASSERT(iClientUserID != 0);
	if (iClientUserID == 0)
	{
		conprintf(0, "RestorePlayerData(%d %s) ERROR: unable to get player's User ID!\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
		if (IS_DEDICATED_SERVER())// safety is more important here
			return 0;

		conprintf(0, "Using index instead!\n");
		iClientUserID = pPlayer->entindex();//return 0;
	}
	CBaseEntity *pAddedItem;
	size_t si, i, count = 0;
	g_SilentItemPickup = true;
	for (si = 0; si<MAX_PLAYERS; ++si)// search for this player's save data
	{
		if (m_szTransitionInventoryPlayerIDs[si] == iClientUserID)
		{
			//SV_RestoreEntity(m_pSaveData, pPlayer->entindex(), pPlayer->edict());
			for (i = 0; i<PLAYER_INVENTORY_SIZE; ++i)
			{
				if (m_szTransitionInventoryItems[si][i][0] != 0)
				{
					pAddedItem = pPlayer->GiveNamedItem(m_szTransitionInventoryItems[si][i]);
					if (pAddedItem)
					{
						ClearBits(pAddedItem->pev->spawnflags, SF_NOREFERENCE|SF_ITEM_NOFALL);// make it droppable again
						++count;
					}
					m_szTransitionInventoryItems[si][i][0] = 0;// restore only once!
				}
			}
			for (i = 0; i<MAX_AMMO_SLOTS; ++i)
			{
				pPlayer->GiveAmmo(m_szTransitionInventoryAmmo[si][i], i);
				m_szTransitionInventoryAmmo[si][i] = 0;
			}
			pPlayer->pev->armorvalue = m_szTransitionInventoryArmor[si];
			m_szTransitionInventoryArmor[si] = 0;
			pPlayer->m_fLongJump = m_szTransitionInventoryLJM[si];
			m_szTransitionInventoryLJM[si] = 0;
		}
	}
	//if (si == MAX_PLAYERS)
	//	conprintf(0, "CGameRulesCoOp::RestorePlayersInventory(): inventory not found for player %d (uid %d)\n", ci, iClientUserID);

	g_SilentItemPickup = false;
	return count;
}
