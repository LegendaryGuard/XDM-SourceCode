/**
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
/*

===== monsters.cpp ========================================================

  Monster-related utility code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "nodes.h"
#include "animation.h"
#include "saverestore.h"
#include "weapons.h"
#include "scripted.h"
#include "squadmonster.h"
#include "decals.h"
#include "sound.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"// XDM
#include "globals.h"
#include "pm_materials.h"
#include "player.h"

static const char *g_szMonsterStateNames[] = { "None", "Idle", "Combat", "Alert", "Hunt", "Prone", "Scripted", "Dead", "" };

#define MONSTER_CUT_CORNER_DIST 8// 8 means the monster's bounding box is contained without the box of the node in WC

// Global Savedata for monster
// UNDONE: Save schedule data?  Can this be done?  We may
// lose our enemy pointer or other data (goal ent, target, etc)
// that make the current schedule invalid, perhaps it's best
// to just pick a new one when we start up again.
TYPEDESCRIPTION	CBaseMonster::m_SaveData[] =
{
	DEFINE_FIELD( CBaseMonster, m_hEnemy, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseMonster, m_hTargetEnt, FIELD_EHANDLE ),
	DEFINE_ARRAY( CBaseMonster, m_hOldEnemy, FIELD_EHANDLE, MAX_OLD_ENEMIES ),
	DEFINE_ARRAY( CBaseMonster, m_vecOldEnemy, FIELD_POSITION_VECTOR, MAX_OLD_ENEMIES ),
// XDM
	DEFINE_FIELD( CBaseMonster, m_iClass, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseMonster, m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flWaitFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_flMoveWaitFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_IdealActivity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_LastHitGroup, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_MonsterState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_IdealMonsterState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iTaskStatus, FIELD_INTEGER ),

	//Schedule_t			*m_pSchedule;
	DEFINE_FIELD( CBaseMonster, m_iScheduleIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afConditions, FIELD_INTEGER ),

	//WayPoint_t			m_Route[ ROUTE_SIZE ];
//	DEFINE_FIELD( CBaseMonster, m_movementGoal, FIELD_INTEGER ),
//	DEFINE_FIELD( CBaseMonster, m_iRouteIndex, FIELD_INTEGER ),
//	DEFINE_FIELD( CBaseMonster, m_moveWaitTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_vecMoveGoal, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_movementActivity, FIELD_INTEGER ),
	//		int					m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
//	DEFINE_FIELD( CBaseMonster, m_afSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_vecLastPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_iHintNode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afMemory, FIELD_INTEGER ),
	//DEFINE_FIELD( CBaseMonster, m_iMaxHealth, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_vecEnemyLKP, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_cAmmoLoaded, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_afCapability, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_bitsDamageType, FIELD_INTEGER ),
	DEFINE_ARRAY( CBaseMonster, m_rgbTimeBasedDamage, FIELD_CHARACTER, CDMG_TIMEBASED ),
	DEFINE_FIELD( CBaseMonster, m_lastDamageAmount, FIELD_INTEGER ),// XDM3035b
	DEFINE_FIELD( CBaseMonster, m_bloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_failSchedule, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_flHungryTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_flDistTooFar, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flDistLook, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_iTriggerCondition, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iszTriggerTarget, FIELD_STRING ),
	DEFINE_FIELD( CBaseMonster, m_HackedGunPos, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseMonster, m_scriptState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_pCine, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseMonster, m_vFrozenViewAngles, FIELD_VECTOR ),// XDM: used by fgrenade
	DEFINE_FIELD( CBaseMonster, m_flUnfreezeTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_fFrozen, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_fFreezeEffect, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseMonster, m_iGibCount, FIELD_UINT32 ),// XDM3034
	DEFINE_FIELD( CBaseMonster, m_iszGibModel, FIELD_STRING ),// XDM3034
	DEFINE_ARRAY( CBaseMonster, m_szSoundDir, FIELD_CHARACTER, MAX_ENTITY_STRING_LENGTH),// XDM3038c: don't want to use string_t because it may overflow some day
	DEFINE_FIELD( CBaseMonster, m_flFallVelocity, FIELD_FLOAT ),// XDM3035c
	DEFINE_FIELD( CBaseMonster, m_flDamage1, FIELD_FLOAT ),// XDM3038c
	DEFINE_FIELD( CBaseMonster, m_flDamage2, FIELD_FLOAT ),// XDM3038c
	DEFINE_FIELD( CBaseMonster, m_iScoreAward, FIELD_INTEGER ),// XDM3038c
	DEFINE_FIELD( CBaseMonster, m_iGameFlags, FIELD_INTEGER ),// XDM3038c
};

//IMPLEMENT_SAVERESTORE( CBaseMonster, CBaseToggle );
int CBaseMonster::Save(CSave &save)
{
	if (!CBaseToggle::Save(save))
		return 0;

	return save.WriteFields("CBaseMonster", this, m_SaveData, ARRAYSIZE(m_SaveData));
}

int CBaseMonster::Restore(CRestore &restore)
{
	if (!CBaseToggle::Restore(restore))
		return 0;
	int status = restore.ReadFields("CBaseMonster", this, m_SaveData, ARRAYSIZE(m_SaveData));

	// We don't save/restore routes yet
	RouteClear();

	// We don't save/restore schedules yet
	m_pSchedule = NULL;
	m_iTaskStatus = TASKSTATUS_NEW;

	// Reset animation
	m_Activity = ACT_RESET;

	// If we don't have an enemy, clear conditions like see enemy, etc.
	if (m_hEnemy.Get() == NULL)
		m_afConditions = 0;

	return status;
}

//=========================================================
// KeyValue
//
// !!! netname entvar field is used in squadmonster for groupname!!!
//=========================================================
void CBaseMonster::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "TriggerTarget"))
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerCondition"))
	{
		m_iTriggerCondition = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "class") || FStrEq(pkvd->szKeyName, "m_iClass"))// XDM, stupid SHL compatibility
	{
		m_iClass = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "voicepitch"))
	{
		m_voicePitch = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "enemy"))// XDM3038c
	{
		if (g_ServerActive)// this is only valid during the game, not level design. Don't bother.
		{
			CBaseEntity *pFound = UTIL_FindEntityByTargetname(NULL, pkvd->szValue);
			if (pFound)
			{
				m_hEnemy = pFound;
				SetConditions(bits_COND_NEW_ENEMY);
			}
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "GibCount"))// XDM3038c
	{
		m_iGibCount = strtoul(pkvd->szValue, NULL, 10);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bloodcolor"))// XDM3038a
	{
		m_bloodColor = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Damage1"))// XDM3038c
	{
		m_flDamage1 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Damage2"))// XDM3038c
	{
		m_flDamage2 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ScoreAward"))// XDM3038c
	{
		m_iScoreAward = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "SoundDir"))// XDM3038a
	{
		strcpy(m_szSoundDir, pkvd->szValue);// TODO: SECURITY: strlen
		m_szSoundDir[MAX_ENTITY_STRING_LENGTH-1]='\0';
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Precache common resources for monsters
// WARNING: Requires m_iGibCount, m_bloodColor set!
// Note   : m_iszGibModel is used here
//-----------------------------------------------------------------------------
void CBaseMonster::Precache(void)
{
	DBG_MON_PRINTF("%s[%d]::Precache()\n", STRING(pev->classname), entindex());
	CBaseToggle::Precache();

	if (m_szSoundDir[0] == '\0')
		conprintf(0, "%s[%d]::Precache() error: m_szSoundDir is empty!\n", STRING(pev->classname), entindex());

	// XDM3038a: precache gibs for monsters
	// XDM3038c: WARNING: if Spawn() was not called (e.g. UTIL_PrecacheOther()), this IS 0! if (m_iGibCount > 0)
	//{
		if (FStringNull(m_iszGibModel))
		{
			if (HasHumanGibs())
				m_iszGibModel = MAKE_STRING(g_szDefaultGibsHuman);
			else if (HasAlienGibs())
				m_iszGibModel = MAKE_STRING(g_szDefaultGibsAlien);
			//else
			//	nothing
		}
		if (!FStringNull(m_iszGibModel))
			m_iGibModelIndex = PRECACHE_MODEL(STRINGV(m_iszGibModel));
	//}
	PRECACHE_SOUND("common/npc_step1.wav");// NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	if (m_iScoreAward <= 0)// XDM3038c
		m_iScoreAward = SCORE_AWARD_NORMAL;

	const char *pDropItemName = GetDropItemName();
	if (pDropItemName && *pDropItemName)
		UTIL_PrecacheOther(pDropItemName);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: 
//-----------------------------------------------------------------------------
void CBaseMonster::Spawn(void)
{
	DBG_MON_PRINTF("%s[%d]::Spawn()\n", STRING(pev->classname), entindex());
	CBaseToggle::Spawn();
	if (m_fFrozen)
		FrozenEnd();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: does not create a copy
//-----------------------------------------------------------------------------
CBaseEntity *CBaseMonster::StartRespawn(void)
{
	DBG_MON_PRINTF("%s[%d]::StartRespawn()\n", STRING(pev->classname), entindex());
	// cannot because the body must stay SetBits(pev->effects, EF_NODRAW);
	ScheduleRespawn(g_pGameRules?g_pGameRules->GetMonsterRespawnDelay(this):10.0f);
	return this;
}

//=========================================================
// MonsterInit - after a monster is spawned, it needs to
// be dropped into the world, checked for mobility problems,
// and put on the proper path, if any. This function does
// all of those things after the monster spawns. Any
// initialization that should take place for all monsters
// goes here.
//=========================================================
void CBaseMonster::MonsterInit(void)
{
	DBG_MON_PRINTF("%s[%d]::MonsterInit()\n", STRING(pev->classname), entindex());
	if (!g_pGameRules->FAllowMonsters())
	{
		pev->flags |= FL_KILLME;// Post this because some monster code modifies class data after calling this function
		pev->spawnflags |= SF_NORESPAWN;	
		SetThinkNull();
		return;
	}

	// UNDONE: Bad place for this, but we don't have Spawn()!
	if (FBitSet(m_iGameFlags, EGF_DIED))
		SetBits(m_iGameFlags, EGF_RESPAWNED);// XDM3038c

	SetBits(m_iGameFlags, EGF_SPAWNED);// XDM3038c

	// XDM3038: sanity check against bad mappers
	ValidateBodyGroups(true);

	// Set fields common to all monsters
	//pev->effects		= 0;
	pev->takedamage		= DAMAGE_AIM;
	pev->ideal_yaw		= pev->angles.y;
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_NO;
	pev->flags			|= FL_MONSTER;

	if (FBitSet(pev->spawnflags, SF_MONSTER_HITMONSTERCLIP))
		pev->flags		|= FL_MONSTERCLIP;

	m_IdealMonsterState	= MONSTERSTATE_IDLE;// Assume monster will be idle, until proven otherwise
	m_IdealActivity		= ACT_IDLE;

	ClearSchedule();
	RouteClear();
	InitBoneControllers(); // FIX: should be done in Spawn
	m_iHintNode			= NO_NODE;
	m_afMemory			= MEMORY_CLEAR;
	m_hEnemy			= NULL;
	m_flDistTooFar		= g_psv_zmax->value;// XDM3038b
	m_flDistLook		= 2048.0f;// XDM3038b
	m_flBurnTime		= 0.0f;
	//m_flRespawnTime		= 0.0f;
	m_flFallVelocity	= -pev->velocity.z;

	//if (m_iGibCount == 0)// XDM
	// XDM3038c	m_iGibCount = 1;
	if (sv_overdrive.value > 0.0f)
		m_iGibCount *= pev->scale;// XDM3038c

	if (m_voicePitch == 0)// XDM
		m_voicePitch = PITCH_NORM;

	// DON'T reset these!
	//m_hLastKiller = NULL;
	//m_hLastVictim = NULL;

	//UTIL_SetOrigin(this, pev->origin);// Because this function is usually called from Spawn(), this thing is done in CBaseEntity::Spawn()

	// set eye position
	SetEyePosition();
	SetUse(&CBaseMonster::MonsterUse);
	SetThink(&CBaseMonster::MonsterInitThink);
	SetNextThink(0.1f);
}

// Call after animation/pose is set up
void CBaseMonster::MonsterInitDead(void)
{
	DBG_MON_PRINTF("%s[%d]::MonsterInitDead()\n", STRING(pev->classname), entindex());
	InitBoneControllers();

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_TOSS;// so he'll fall to ground
	pev->takedamage		= DAMAGE_YES;// XDM3037: take damage, no aiming
	m_iClass = CLASS_GIB;// XDM3038: affects game logic and frags: barney won't attack player who is shooting a dead scientist

	SetBits(pev->flags, FL_FLOAT);// XDM: and float in water
	SetBits(pev->spawnflags, SF_NORESPAWN);// XDM3037: reliably don't respawn

	pev->frame = 0;
	ResetSequenceInfo();
	pev->framerate = 0;

	// Copy health
	pev->max_health		= pev->health;
	pev->deadflag		= DEAD_DEAD;

	UTIL_SetSize(this, g_vecZero, g_vecZero);
	UTIL_SetOrigin(this, pev->origin);

	// Setup health counters, etc.
	BecomeDead();
	Remember(bits_MEMORY_KILLED);// XDM3038a: don't get "killed" by any little shot
	SetThink(&CBaseMonster::CorpseFallThink);
	SetNextThink(0.5f);
}

//=========================================================
// Makes a monster full for a little while.
//=========================================================
void CBaseMonster::Eat(float flFullDuration)
{
	DBG_MON_PRINTF("%s[%d]::Eat(%g)\n", STRING(pev->classname), entindex(), flFullDuration);
	TakeHealth(flFullDuration*0.5f, DMG_GENERIC);// XDM3038c
	m_flHungryTime = gpGlobals->time + flFullDuration;
}

//=========================================================
// Returns true if a monster is hungry.
//=========================================================
bool CBaseMonster::FShouldEat(void) const
{
	if (m_flHungryTime > gpGlobals->time)
		return false;

	return true;
}

//=========================================================
// Called by Barnacle victims when the barnacle pulls their head into its mouth
//=========================================================
void CBaseMonster::BarnacleVictimBitten(CBaseEntity *pBarnacle)
{
	DBG_MON_PRINTF("%s[%d]::BarnacleVictimBitten(%s[%d])\n", STRING(pev->classname), entindex(), pBarnacle?STRING(pBarnacle->pev->classname):"", pBarnacle?pBarnacle->entindex():0);
	ClearConditions(bits_COND_HEAR_SOUND | bits_COND_SMELL_FOOD | bits_COND_SMELL);
	//SetConditions(bits_COND_LIGHT_DAMAGE);
	Schedule_t *pNewSchedule = GetScheduleOfType(SCHED_BARNACLE_VICTIM_CHOMP);
	if (pNewSchedule)
		ChangeSchedule(pNewSchedule);
}

//=========================================================
// Called by barnacle victims when the host barnacle is killed.
//=========================================================
void CBaseMonster::BarnacleVictimReleased(void)
{
	DBG_MON_PRINTF("%s[%d]::BarnacleVictimReleased()\n", STRING(pev->classname), entindex());
	pev->velocity.Clear();
	pev->movetype = MOVETYPE_STEP;
	ClearBits(pev->flags, FL_ONGROUND);
	SetConditions(bits_COND_LIGHT_DAMAGE);
	m_IdealMonsterState = MONSTERSTATE_IDLE;
}

//=========================================================
// Monsters dig through the active sound list for any sounds that may interest them. (smells, too!)
//=========================================================
void CBaseMonster::Listen(void)
{
	float	hearingSensitivity;
	CSound	*pCurrentSound;

	m_iAudibleList = SOUNDLIST_EMPTY;
	ClearConditions(bits_COND_HEAR_SOUND | bits_COND_SMELL | bits_COND_SMELL_FOOD);
	m_afSoundTypes = 0;

	int iMySounds = ISoundMask();

	if ( m_pSchedule )
	{
		//!!!WATCH THIS SPOT IF YOU ARE HAVING SOUND RELATED BUGS!
		// Make sure your schedule AND personal sound masks agree!
		iMySounds &= m_pSchedule->iSoundMask;
	}

	int iSound = CSoundEnt::ActiveList();

	// UNDONE: Clear these here?
	ClearConditions( bits_COND_HEAR_SOUND | bits_COND_SMELL_FOOD | bits_COND_SMELL );
	hearingSensitivity = HearingSensitivity( );

	while ( iSound != SOUNDLIST_EMPTY )
	{
		pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );

		if ( pCurrentSound &&
			( pCurrentSound->m_iType & iMySounds )	&&
			( pCurrentSound->m_vecOrigin - EarPosition() ).Length() <= pCurrentSound->m_iVolume * hearingSensitivity )

		//if ( ( g_pSoundEnt->m_SoundPool[ iSound ].m_iType & iMySounds ) && ( g_pSoundEnt->m_SoundPool[ iSound ].m_vecOrigin - EarPosition()).Length () <= g_pSoundEnt->m_SoundPool[ iSound ].m_iVolume * hearingSensitivity )
		{
 			// the monster cares about this sound, and it's close enough to hear.
			//g_pSoundEnt->m_SoundPool[ iSound ].m_iNextAudible = m_iAudibleList;
			pCurrentSound->m_iNextAudible = m_iAudibleList;

			if ( pCurrentSound->FIsSound() )
			{
				// this is an audible sound.
				SetConditions( bits_COND_HEAR_SOUND );
			}
			else
			{
				// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
				//if ( g_pSoundEnt->m_SoundPool[ iSound ].m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				if ( pCurrentSound->m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				{
					// the detected scent is a food item, so set both conditions.
					// !!!BUGBUG - maybe a virtual function to determine whether or not the scent is food?
					SetConditions( bits_COND_SMELL_FOOD );
					SetConditions( bits_COND_SMELL );
				}
				else
				{
					// just a normal scent.
					SetConditions( bits_COND_SMELL );
				}
			}
			//m_afSoundTypes |= g_pSoundEnt->m_SoundPool[ iSound ].m_iType;
			m_afSoundTypes |= pCurrentSound->m_iType;
			m_iAudibleList = iSound;
		}

		//iSound = g_pSoundEnt->m_SoundPool[ iSound ].m_iNext;
		if (pCurrentSound)
			iSound = pCurrentSound->m_iNext;
		else
			iSound = SOUNDLIST_EMPTY;// XDM3037a
	}
}

//=========================================================
// FLSoundVolume - subtracts the volume of the given sound
// from the distance the sound source is from the caller,
// and returns that value, which is considered to be the 'local'
// volume of the sound.
//=========================================================
float CBaseMonster::FLSoundVolume(CSound *pSound)
{
	return ( pSound->m_iVolume - ( ( pSound->m_vecOrigin - pev->origin ).Length() ) );
}

//=========================================================
// FValidateHintType - tells use whether or not the monster cares
// about the type of Hint Node given
//=========================================================
bool CBaseMonster::FValidateHintType(short sHint)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns an integer that describes the relationship between this monster and target.
// Warning: May be called before Spawn()!
// XDM3037: when monster is owned by a player, detect enemies properly
// Input  : *pTarget - 
// Output : int R_NO
//-----------------------------------------------------------------------------
int CBaseMonster::IRelationship(CBaseEntity *pTarget)
{
	if (m_hOwner)// XDM3037
	{
		if (m_hOwner == pTarget)
			return R_AL;
		if (m_hOwner == pTarget->m_hOwner)// target is owned by the same owner
			return R_AL;

		// Do not use PlayerRelationship() because it may someday call this and cause a recursion
		if (g_pGameRules && g_pGameRules->IsTeamplay() && g_pGameRules->IsRealTeam(pev->team) && g_pGameRules->IsRealTeam(pTarget->pev->team))// target entity does not belong to any real team
		{
			if (pev->team == pTarget->pev->team)// actual teams are valid and equal
				return R_AL;
			else
				return R_DL;
		}

		if (m_hOwner->IsPlayer() || m_hOwner->IsMonster())
			return m_hOwner->IRelationship(pTarget);// XDM3038c: copy our owner's relation
	}
	return CBaseToggle::IRelationship(pTarget);
}

// XDM3038c
Vector CBaseMonster::EyePosition(void)
{
	if (!IsAlive())
		return pev->origin;

	return CBaseToggle::EyePosition();
}

//=========================================================
// Look - Base class monster function to find enemies or
// food by sight. iDistance is distance ( in units ) that the
// monster can see.
//
// Sets the sight bits of the m_afConditions mask to indicate
// which types of entities were sighted.
// Function also sets the Looker's m_pLink
// to the head of a link list that contains all visible ents.
// (linked via each ent's m_pLink field)
//
//=========================================================
void CBaseMonster::Look(int iDistance)
{
	// DON'T let visibility information from last frame sit around!
	ClearConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT);

	m_pLink = NULL;

	if (FBitSet(pev->spawnflags, SF_MONSTER_PRISONER))
		return;// XDM3038a: compact code

	int	iSighted = 0;
	// See no evil if prisoner is set
	CBaseEntity	*pSightEnt = NULL;// the current visible entity that we're dealing with
	//CBaseEntity *pList[100];
	//Vector delta(iDistance, iDistance, iDistance);
	//Vector checkmins(pev->origin - delta);// XDM3037
	//Vector checkmaxs(pev->origin + delta);
	//edict_t *pEdict = INDEXENT(1);
	int r = R_NO;
	// Find only monsters/clients in box, NOT limited to PVS
	//size_t count = UTIL_EntitiesInBox(pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT|FL_MONSTER );
	//for ( size_t i = 0; i < count; ++i )// XDM3037: old concept is obsolete
	//for (int i = 1; i < gpGlobals->maxEntities; ++i, pEdict++)
	//while ((pSightEnt = UTIL_FindEntityInBox(pSightEnt, checkmins, checkmaxs)) != NULL)
	while ((pSightEnt = UTIL_FindEntityInSphere(pSightEnt, Center(), iDistance)) != NULL)// XDM3038c: avoid rare crash with bad edict during map loading
	{
		//if (!UTIL_IsValidEntity(pEdict))
		//	continue;
		//if (BoundsIntersect(pEdict->v.absmin, pEdict->v.absmax, checkmins, checkmaxs) == false)// XDM3037
		//		continue;
		//pSightEnt = CBaseEntity::Instance(pEdict);//pSightEnt = pList[i];

		if (!pSightEnt || !(pSightEnt->IsMonster() || pSightEnt->IsPlayer() || pSightEnt->IsProjectile()))// XDM3037
			continue;

		// !!!temporarily only considering other monsters and clients, don't see prisoners
		if (pSightEnt != this && !FBitSet(pSightEnt->pev->spawnflags, SF_MONSTER_PRISONER) && pSightEnt->IsAlive())
		{
			// the looker will want to consider this entity
			// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
			r = IRelationship(pSightEnt);
			if (r != R_NO && !FBitSet(pSightEnt->pev->flags, FL_NOTARGET) && FInViewCone(pSightEnt) && FVisible(pSightEnt))
			{
				/* HL does not return proper light levels
				if (Classify() == CLASS_HUMAN_MILITARY || Classify() == CLASS_ALIEN_PREDATOR)
				{
					Illumination()?
					if (pSightEnt->pev->light_level < 32)// XDM: too dark to see
						continue;
				}*/

				if (pSightEnt->IsPlayer())
				{
					/* XDM3037: done in IRelationship()	if (g_pGameRules->IsTeamplay())// XDM: team turrets, etc.
					{
						if (pev->team > TEAM_NONE && pev->team == pSightEnt->pev->team)
							continue;
					}*/

					if (FBitSet(pev->spawnflags, SF_MONSTER_WAIT_TILL_SEEN))
					{
						CBaseMonster *pClient = pSightEnt->MyMonsterPointer();
						// don't link this client in the list if the monster is wait till seen and the player isn't facing the monster
						if (pClient && !pClient->FInViewCone(this))// XDM3038a: horrible typo
							continue;// we're not in the player's view cone.
						else// player sees us, become normal now.
							ClearBits(pev->spawnflags, SF_MONSTER_WAIT_TILL_SEEN);
					}
					// if we see a client, remember that (mostly for scripted AI)
					iSighted |= bits_COND_SEE_CLIENT;
				}
				else if (pSightEnt->IsMonster())
				{
					if (m_hOwner.Get() && pSightEnt->m_hOwner.Get())
						if (m_hOwner->IRelationship(pSightEnt->m_hOwner) == R_AL)// XDM3038c: these monster's owners are friends
							continue;
				}

				pSightEnt->m_pLink = m_pLink;
				m_pLink = pSightEnt;

				if (m_hEnemy.Get() && pSightEnt == m_hEnemy)// XDM3037a: only if enemy does exist!
				{
					// we know this ent is visible, so if it also happens to be our enemy, store that now.
					iSighted |= bits_COND_SEE_ENEMY;
				}

				// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when we see monsters other than the Enemy.
				switch (r)
				{
				case R_NM:
					iSighted |= bits_COND_SEE_NEMESIS;
					break;
				case R_HT:
					iSighted |= bits_COND_SEE_HATE;
					break;
				case R_DL:
					iSighted |= bits_COND_SEE_DISLIKE;
					break;
				case R_FR:
					iSighted |= bits_COND_SEE_FEAR;
					break;
				case R_AL:
					break;
				default:
					conprintf(2, "%s[%d] can't assess %s\n", STRING(pev->classname), entindex(), STRING(pSightEnt->pev->classname));
					break;
				}
			}
		}
	}
	SetConditions(iSighted);
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBaseMonster::ISoundMask(void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER;
}

//=========================================================
// PBestSound - returns a pointer to the sound the monster
// should react to. Right now responds only to nearest sound.
//=========================================================
CSound *CBaseMonster::PBestSound(void)
{
	if (m_iAudibleList == SOUNDLIST_EMPTY)
	{
#if defined (_DEBUG)
		ALERT ( at_aiconsole, "ERROR! monster %s[%d] has no audible sounds!\n", STRING(pev->classname), entindex());
#endif
		return NULL;
	}

	int iThisSound = m_iAudibleList;
	int	iBestSound = -1;
	float flBestDist = 8192;// so first nearby sound will become best so far.
	float flDist;
	CSound *pSound;
	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iThisSound );
		if ( pSound && pSound->FIsSound() )
		{
			flDist = ( pSound->m_vecOrigin - EarPosition()).Length();
			if ( flDist < flBestDist )
			{
				iBestSound = iThisSound;
				flBestDist = flDist;
			}
			iThisSound = pSound->m_iNextAudible;
		}
		else
			break;
	}
	if (iBestSound >= 0)
	{
		pSound = CSoundEnt::SoundPointerForIndex( iBestSound );
		return pSound;
	}
#if defined (_DEBUG)
	conprintf(2, "NULL Return from PBestSound()\n");
#endif
	return NULL;
}

//=========================================================
// PBestScent - returns a pointer to the scent the monster
// should react to. Right now responds only to nearest scent
//=========================================================
CSound *CBaseMonster::PBestScent(void)
{
	if (m_iAudibleList == SOUNDLIST_EMPTY)
	{
#if _DEBUG
		ALERT ( at_aiconsole, "ERROR! PBestScent() has empty soundlist!\n" );
#endif
		return NULL;
	}

	int iThisScent = m_iAudibleList;// smells are in the sound list.
	int	iBestScent = -1;
	float flBestDist = 8192;// so first nearby smell will become best so far.
	float flDist;
	CSound *pSound;
	while ( iThisScent != SOUNDLIST_EMPTY )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iThisScent );
		if ( pSound && pSound->FIsScent() )
		{
			flDist = ( pSound->m_vecOrigin - pev->origin ).Length();
			if ( flDist < flBestDist )
			{
				iBestScent = iThisScent;
				flBestDist = flDist;
			}
			iThisScent = pSound->m_iNextAudible;
		}
		else
			break;
	}
	if ( iBestScent >= 0 )
	{
		pSound = CSoundEnt::SoundPointerForIndex( iBestScent );
		return pSound;
	}
#if _DEBUG
	conprintf(2, "NULL Return from PBestScent()\n");
#endif
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035c: Gives a possibility for some monsters to reduce falling damage they take
// Output : float - final fall damage value
//-----------------------------------------------------------------------------
float CBaseMonster::FallDamage(const float &flFallVelocity)
{
	// right now  damage = mysize/playersize * playerdamageformula
	return flFallVelocity*0.01*DAMAGE_FOR_FALL_SPEED * (pev->maxs - pev->mins).Volume()/((VEC_HUMAN_HULL_MAX-VEC_HUMAN_HULL_MIN).Volume());
}

void CBaseMonster::CallMonsterThink(void)
{
	DBG_PRINT_ENT_THINK(CallMonsterThink);
	//ALERT(at_console, "%s[%d]::CallMonsterThink() (%s)\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
	this->MonsterThink();
}

//=========================================================
// Monster Think - calls out to core AI functions and handles this
// monster's specific animation events
//=========================================================
void CBaseMonster::MonsterThink(void)
{
	DBG_PRINT_ENT_THINK(MonsterThink);
	//ALERT(at_console, "%s[%d]::MonsterThink() (%s)\n", STRING(pev->classname), entindex(), STRING(pev->targetname));

	if (g_pGameRules->IsGameOver())// XDM3035a
		return;

	if (pev->waterlevel >= WATERLEVEL_HEAD)// XDM
	{
		// Ichthyosaur has CLASS_ALIEN_BIOWEAPON!!!
		if (IsHuman() && pev->air_finished < gpGlobals->time)// drown!
		{
			TakeDamage(g_pWorld, g_pWorld, 2, DMG_DROWN | DMG_NEVERGIB);// XDM: All humans must die underwater!!!
			pev->air_finished = gpGlobals->time + 1.0;
		}
	}
	else
	{
		if (pev->movetype == MOVETYPE_STEP)// || MOVETYPE_TOSS// Some monsters prefer to fly
		{
			if (FBitSet(pev->flags, FL_ONGROUND))// touched ground
			{
				if (m_flFallVelocity > 0.0f)// was falling
				{
					float flFallDamage =  FallDamage(m_flFallVelocity);
					if (flFallDamage > 0.0f)
					{
						if (flFallDamage > pev->health)// note: play on item channel because we play footstep landing on body channel
						{
							EMIT_SOUND(edict(), CHAN_ITEM, "common/bodysplat.wav", VOL_NORM, ATTN_NORM);
							UTIL_DecalPoints(pev->origin, pev->origin-Vector(0.0f,0.0f,m_flFallVelocity), edict(), (m_bloodColor == BLOOD_COLOR_RED?DECAL_BLOODSMEARR1:DECAL_BLOODSMEARY1));// the biggest spot
							pev->punchangle.x = 0.0f;// reset
							pev->avelocity.Clear();
						}
						else if (flFallDamage > pev->health/2)
						{
							UTIL_DecalPoints(pev->origin, pev->origin-Vector(0.0f,0.0f,m_flFallVelocity), edict(), UTIL_BloodDecalIndex(m_bloodColor));// XDM3035a
						}
						TakeDamage(g_pWorld, g_pWorld, flFallDamage, DMG_FALL | DMG_NEVERGIB);
					}
					m_flFallVelocity = 0.0f;
				}
			}
			else
				m_flFallVelocity = -pev->velocity.z;
		}
	}

	/*if (m_hEnemy != NULL && m_hEnemy->IsPlayer() && HasConditions(bits_COND_SEE_ENEMY))
	{
		g_musicEventInvoker = edict();
		MP3_PlayState(MST_COMBAT);// XDM
	}*/
	if (!IsPlayer())// XDM: player already did it
		FrozenThink();

	SetNextThink(0.1f);// keep monster thinking.

//	ALERT(at_console, "MonsterThink() 1\n");
	RunAI();

	float flInterval = StudioFrameAdvance(); // animate
// start or end a fidget
// This needs a better home -- switching animations over time should be encapsulated on a per-activity basis
// perhaps MaintainActivity() or a ShiftAnimationOverTime() or something.
	if ( m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished )
	{
		int iSequence;
		// animation does loop, which means we're playing subtle idle. Might need to fidget.
		if (m_fSequenceLoops)
			iSequence = LookupActivity ( m_Activity );
		else
			iSequence = LookupActivityHeaviest ( m_Activity );
		// animation that just ended doesn't loop! That means we just finished a fidget
		// and should return to our heaviest weighted idle (the subtle one)

		if (iSequence != ACTIVITY_NOT_AVAILABLE)
		{
			pev->sequence = iSequence;	// Set to new anim (if it's there)
			ResetSequenceInfo();
		}
	}

//	ALERT(at_console, "MonsterThink() 2\n");
	DispatchAnimEvents( flInterval );

	if ( !MovementIsComplete() )
	{
		Move( flInterval );
	}
#if _DEBUG
	else
	{
		if ( !TaskIsRunning() && !TaskIsComplete() )
			ALERT( at_error, "Schedule stalled!!\n" );
	}
#endif
}

//=========================================================
// CBaseMonster - USE - will make a monster angry at whomever
// activated it.
//=========================================================
void CBaseMonster::MonsterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(MonsterUse);
	m_IdealMonsterState = MONSTERSTATE_ALERT;
}

//=========================================================
// CBaseMonster - DEAD USE - will gib - XDM: undone: take items
//=========================================================
/*void CBaseMonster::CorpseUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	GibMonster();
}*/

//=========================================================
// Ignore conditions - before a set of conditions is allowed
// to interrupt a monster's schedule, this function removes
// conditions that we have flagged to interrupt the current
// schedule, but may not want to interrupt the schedule every
// time. (Pain, for instance)
//=========================================================
int CBaseMonster::IgnoreConditions(void)
{
	int iIgnoreConditions = 0;

	if ( !FShouldEat() )
	{
		// not hungry? Ignore food smell.
		iIgnoreConditions |= bits_COND_SMELL_FOOD;
	}

	if ( m_MonsterState == MONSTERSTATE_SCRIPT && m_pCine )
		iIgnoreConditions |= m_pCine->IgnoreConditions();

	return iIgnoreConditions;
}

//=========================================================
// 	RouteClear - zeroes out the monster's route array and goal
//=========================================================
void CBaseMonster::RouteClear(void)
{
	RouteNew();
	m_movementGoal = MOVEGOAL_NONE;
	m_movementActivity = ACT_IDLE;
	Forget( bits_MEMORY_MOVE_FAILED );
}

//=========================================================
// Route New - clears out a route to be changed, but keeps
// goal intact.
//=========================================================
void CBaseMonster::RouteNew(void)
{
	m_Route[0].iType		= 0;
	m_iRouteIndex			= 0;
}

//=========================================================
// FRouteClear - returns TRUE is the Route is cleared out
// ( invalid )
//=========================================================
bool CBaseMonster::FRouteClear(void)
{
	if ( m_Route[ m_iRouteIndex ].iType == 0 || m_movementGoal == MOVEGOAL_NONE )
		return true;

	return false;
}

//=========================================================
// FRefreshRoute - after calculating a path to the monster's
// target, this function copies as many waypoints as possible
// from that path to the monster's Route array
//=========================================================
bool CBaseMonster::FRefreshRoute (void)
{
	CBaseEntity	*pPathCorner;
	int			i;
	bool		returnCode = false;

	RouteNew();

	switch (m_movementGoal)
	{
		case MOVEGOAL_PATHCORNER:
			{
				// monster is on a path_corner loop
				pPathCorner = m_pGoalEnt;
				i = 0;

				while ( pPathCorner && i < ROUTE_SIZE )
				{
					m_Route[ i ].iType = bits_MF_TO_PATHCORNER;
					m_Route[ i ].vecLocation = pPathCorner->pev->origin;

					pPathCorner = pPathCorner->GetNextTarget();

					// Last path_corner in list?
					if ( !pPathCorner )
						m_Route[i].iType |= bits_MF_IS_GOAL;

					++i;
				}
			}
			returnCode = true;
			break;

		case MOVEGOAL_ENEMY:
			returnCode = BuildRoute( m_vecEnemyLKP, bits_MF_TO_ENEMY, m_hEnemy );
			break;

		case MOVEGOAL_LOCATION:
			returnCode = BuildRoute( m_vecMoveGoal, bits_MF_TO_LOCATION, NULL );
			break;

		case MOVEGOAL_TARGETENT:
			if (m_hTargetEnt != NULL)
			{
				returnCode = BuildRoute( m_hTargetEnt->pev->origin, bits_MF_TO_TARGETENT, m_hTargetEnt );
			}
			break;

		case MOVEGOAL_NODE:
			returnCode = FGetNodeRoute( m_vecMoveGoal );
//			if ( returnCode )
//				RouteSimplify( NULL );
			break;
	}

	return returnCode;
}

BOOL CBaseMonster::MoveToEnemy( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	m_movementGoal = MOVEGOAL_ENEMY;
	return FRefreshRoute();
}

BOOL CBaseMonster::MoveToLocation( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	m_movementGoal = MOVEGOAL_LOCATION;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

BOOL CBaseMonster::MoveToTarget( Activity movementAct, float waitTime )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	m_movementGoal = MOVEGOAL_TARGETENT;
	return FRefreshRoute();
}

BOOL CBaseMonster::MoveToNode( Activity movementAct, float waitTime, const Vector &goal )
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;
	m_movementGoal = MOVEGOAL_NODE;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

#if defined (_DEBUG)
void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b )
{
	if (m_Route[m_iRouteIndex].iType == 0)
	{
		ALERT(at_aiconsole, "Can't draw route.\n");
		return;
	}

	//UTIL_ParticleEffect ( m_Route[ m_iRouteIndex ].vecLocation, g_vecZero, 255, 25 );
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(m_Route[m_iRouteIndex].vecLocation.x);
		WRITE_COORD(m_Route[m_iRouteIndex].vecLocation.y);
		WRITE_COORD(m_Route[m_iRouteIndex].vecLocation.z);
		WRITE_SHORT(g_iModelIndexLaser);
		WRITE_BYTE(0);// frame start
		WRITE_BYTE(10);// framerate
		WRITE_BYTE(100);// life
		WRITE_BYTE(16);// width
		WRITE_BYTE(0);// noise
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
		WRITE_BYTE(255);// brightness
		WRITE_BYTE(10);// speed
	MESSAGE_END();

	for (int i = m_iRouteIndex ; i < ROUTE_SIZE - 1; ++i)
	{
		if ((m_Route[i].iType & bits_MF_IS_GOAL) || (m_Route[i+1].iType == 0))
			break;

		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(TE_BEAMPOINTS);
			WRITE_COORD(m_Route[i].vecLocation.x);
			WRITE_COORD(m_Route[i].vecLocation.y);
			WRITE_COORD(m_Route[i].vecLocation.z);
			WRITE_COORD(m_Route[i+1].vecLocation.x);
			WRITE_COORD(m_Route[i+1].vecLocation.y);
			WRITE_COORD(m_Route[i+1].vecLocation.z);
			WRITE_SHORT(g_iModelIndexLaser);
			WRITE_BYTE(0);// frame start
			WRITE_BYTE(10);// framerate
			WRITE_BYTE(10*(i+1));// life
			WRITE_BYTE(8);// width
			WRITE_BYTE(0);// noise
			WRITE_BYTE(r);
			WRITE_BYTE(g);
			WRITE_BYTE(b);
			WRITE_BYTE(255);// brightness
			WRITE_BYTE(10);// speed
		MESSAGE_END();
		//UTIL_ParticleEffect ( m_Route[ i ].vecLocation, g_vecZero, 255, 25 );
	}
}
#endif

int ShouldSimplify( int routeType )
{
	routeType &= ~bits_MF_IS_GOAL;

	if ( (routeType == bits_MF_TO_PATHCORNER) || (routeType & bits_MF_DONT_SIMPLIFY) )
		return FALSE;
	return TRUE;
}

//=========================================================
// RouteSimplify
//
// Attempts to make the route more direct by cutting out
// unnecessary nodes & cutting corners.
//
//=========================================================
void CBaseMonster::RouteSimplify( CBaseEntity *pTargetEnt )
{
	// BUGBUG: this doesn't work 100% yet
	int			i, count, outCount;
	Vector		vecStart;
	WayPoint_t	outRoute[ ROUTE_SIZE * 2 ];	// Any points except the ends can turn into 2 points in the simplified route

	count = 0;

	for ( i = m_iRouteIndex; i < ROUTE_SIZE; ++i)
	{
		if ( !m_Route[i].iType )
			break;
		else
			count++;
		if ( m_Route[i].iType & bits_MF_IS_GOAL )
			break;
	}
	// Can't simplify a direct route!
	if ( count < 2 )
	{
//		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
		return;
	}

	outCount = 0;
	vecStart = pev->origin;
	for ( i = 0; i < count-1; ++i)
	{
		// Don't eliminate path_corners
		if ( !ShouldSimplify( m_Route[m_iRouteIndex+i].iType ) )
		{
			outRoute[outCount] = m_Route[ m_iRouteIndex + i ];
			outCount++;
		}
		else if ( CheckLocalMove ( vecStart, m_Route[m_iRouteIndex+i+1].vecLocation, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			// Skip vert
			continue;
		}
		else
		{
			Vector vecTest, vecSplit;

			// Halfway between this and next
			vecTest = (m_Route[m_iRouteIndex+i+1].vecLocation + m_Route[m_iRouteIndex+i].vecLocation) * 0.5;

			// Halfway between this and previous
			vecSplit = (m_Route[m_iRouteIndex+i].vecLocation + vecStart) * 0.5f;

			int iType = (m_Route[m_iRouteIndex+i].iType | bits_MF_TO_DETOUR) & ~bits_MF_NOT_TO_MASK;
			if ( CheckLocalMove ( vecStart, vecTest, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecTest;
			}
			else if ( CheckLocalMove ( vecSplit, vecTest, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecSplit;
				outRoute[outCount+1].iType = iType;
				outRoute[outCount+1].vecLocation = vecTest;
				++outCount; // Adding an extra point
			}
			else
			{
				outRoute[outCount] = m_Route[ m_iRouteIndex + i ];
			}
		}
		// Get last point
		vecStart = outRoute[ outCount ].vecLocation;
		outCount++;
	}
	ASSERT( i < count );
	outRoute[outCount] = m_Route[ m_iRouteIndex + i ];
	++outCount;

	// Terminate
	outRoute[outCount].iType = 0;
	ASSERT( outCount < (ROUTE_SIZE*2) );

// Copy the simplified route, disable for testing
	m_iRouteIndex = 0;
	for ( i = 0; i < ROUTE_SIZE && i < outCount; ++i)
	{
		m_Route[i] = outRoute[i];
	}

	// Terminate route
	if ( i < ROUTE_SIZE )
		m_Route[i].iType = 0;

// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT( "simplify" ) != 0 )
		DrawRoute( pev, outRoute, 0, 255, 0, 0 );
//	else
		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 255, 0 );
#endif
}

//=========================================================
// FBecomeProne - tries to send a monster into PRONE state.
// right now only used when a barnacle snatches someone, so
// may have some special case stuff for that.
//=========================================================
bool CBaseMonster::FBecomeProne(void)
{
	DBG_MON_PRINTF("%s[%d]::FBecomeProne()\n", STRING(pev->classname), entindex());
	if (CBaseToggle::FBecomeProne())// XDM3038c
	{
		//if (FBitSet(pev->flags, FL_ONGROUND))
		//	ClearBits(pev->flags, FL_ONGROUND);

		m_IdealMonsterState = MONSTERSTATE_PRONE;
		return true;
	}
	return false;
}

void CBaseMonster::Stop(void)
{
	DBG_MON_PRINTF("%s[%d]::Stop()\n", STRING(pev->classname), entindex());
	m_IdealActivity = GetStoppedActivity();
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBaseMonster::CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CBaseMonster::CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( flDist > 64 && flDist <= 512 && flDot >= 0.5 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CBaseMonster::CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if (flDist <= 64 && flDot >= 0.7 && m_hEnemy.IsValid() && FBitSet(m_hEnemy->pev->flags, FL_ONGROUND))
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CBaseMonster::CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckAttacks - sets all of the bits for attacks that the
// monster is capable of carrying out on the passed entity.
//=========================================================
void CBaseMonster::CheckAttacks(CBaseEntity *pTarget, const vec_t &flDist)
{
	// Clear all attack conditions
	ClearConditions(bits_COND_CAN_ATTACK);

	if (!CanAttack())// XDM3038c
		return;

	Vector2D vec2LOS;
	UTIL_MakeVectors(pev->angles);
	vec2LOS = (pTarget->pev->origin - pev->origin).Make2D();
	vec2LOS.NormalizeSelf();
	vec_t flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());
	// we know the enemy is in front now. We'll find which attacks the monster is capable of by
	// checking for corresponding Activities in the model file, then do the simple checks to validate
	// those attack types.
	if ( m_afCapability & bits_CAP_RANGE_ATTACK1 )
	{
		if ( CheckRangeAttack1 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK1 );
	}
	if ( m_afCapability & bits_CAP_RANGE_ATTACK2 )
	{
		if ( CheckRangeAttack2 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_RANGE_ATTACK2 );
	}
	if ( m_afCapability & bits_CAP_MELEE_ATTACK1 )
	{
		if ( CheckMeleeAttack1 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
	}
	if ( m_afCapability & bits_CAP_MELEE_ATTACK2 )
	{
		if ( CheckMeleeAttack2 ( flDot, flDist ) )
			SetConditions( bits_COND_CAN_MELEE_ATTACK2 );
	}
}

//=========================================================
// CanCheckAttacks - prequalifies a monster to do more fine
// checking of potential attacks.
//=========================================================
bool CBaseMonster::FCanCheckAttacks (void)
{
	if (m_fFrozen)// XDM3035
		return false;

	if ( HasConditions(bits_COND_SEE_ENEMY) && !HasConditions( bits_COND_ENEMY_TOOFAR ) )
		return true;

	return false;
}

//=========================================================
// CheckEnemy - part of the Condition collection process,
// gets and stores data and conditions pertaining to a monster's
// enemy. Returns TRUE if Enemy LKP was updated.
//=========================================================
int CBaseMonster::CheckEnemy ( CBaseEntity *pEnemy )
{
//	ALERT(at_console, "CBaseMonster(%s)::CheckEnemy(%s)\n", STRING(pev->classname), STRING(pEnemy->pev->classname));

	vec_t	flDistToEnemy;
	int		iUpdatedLKP;// set this to TRUE if you update the EnemyLKP in this function.

	iUpdatedLKP = FALSE;
	ClearConditions ( bits_COND_ENEMY_FACING_ME );

	if ( !FVisible( pEnemy ) )
	{
//		ASSERT(!HasConditions(bits_COND_SEE_ENEMY));
		ClearConditions(bits_COND_SEE_ENEMY);// XDM3035b
		SetConditions( bits_COND_ENEMY_OCCLUDED );
	}
	else
		ClearConditions( bits_COND_ENEMY_OCCLUDED );

	if ( !pEnemy->IsAlive() )
	{
		SetConditions ( bits_COND_ENEMY_DEAD );
		ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED );
		return FALSE;
	}

	Vector vecEnemyPos(pEnemy->pev->origin);
	// distance to enemy's origin
	flDistToEnemy = ( vecEnemyPos - pev->origin ).Length();
	vecEnemyPos.z += pEnemy->pev->size.z * 0.5;
	// distance to enemy's head
	vec_t flDistToEnemy2 = (vecEnemyPos - pev->origin).Length();
	if (flDistToEnemy2 < flDistToEnemy)
		flDistToEnemy = flDistToEnemy2;
	else
	{
		// distance to enemy's feet
		vecEnemyPos.z -= pEnemy->pev->size.z;
		flDistToEnemy2 = (vecEnemyPos - pev->origin).Length();
		if (flDistToEnemy2 < flDistToEnemy)
			flDistToEnemy = flDistToEnemy2;
	}

	if ( HasConditions( bits_COND_SEE_ENEMY ) )
	{
		iUpdatedLKP = TRUE;
		m_vecEnemyLKP = pEnemy->pev->origin;

		CBaseMonster *pEnemyMonster = pEnemy->MyMonsterPointer();
		if (pEnemyMonster)
		{
			if (pEnemyMonster->FInViewCone(this))
				SetConditions( bits_COND_ENEMY_FACING_ME );
			else
				ClearConditions( bits_COND_ENEMY_FACING_ME );
		}

		if (!pEnemy->pev->velocity.IsZero())
		{
			// trail the enemy a bit
			m_vecEnemyLKP = m_vecEnemyLKP - pEnemy->pev->velocity * RANDOM_FLOAT( -0.05, 0 );
		}
		else
		{
			// UNDONE: use pev->oldorigin?
		}
	}
	else if ( !HasConditions(bits_COND_ENEMY_OCCLUDED|bits_COND_SEE_ENEMY) && ( flDistToEnemy <= 256 ) )
	{
		// if the enemy is not occluded, and unseen, that means it is behind or beside the monster.
		// if the enemy is near enough the monster, we go ahead and let the monster know where the
		// enemy is.
		iUpdatedLKP = TRUE;
		m_vecEnemyLKP = pEnemy->pev->origin;
	}

	if ( flDistToEnemy >= m_flDistTooFar )
	{
		// enemy is very far away from monster
		SetConditions( bits_COND_ENEMY_TOOFAR );
	}
	else
		ClearConditions( bits_COND_ENEMY_TOOFAR );

	if ( FCanCheckAttacks() )
	{
		CheckAttacks ( m_hEnemy, flDistToEnemy );
	}

	if ( m_movementGoal == MOVEGOAL_ENEMY )
	{
		for ( int i = m_iRouteIndex; i < ROUTE_SIZE; ++i)
		{
			if ( m_Route[ i ].iType == (bits_MF_IS_GOAL|bits_MF_TO_ENEMY) )
			{
				// UNDONE: Should we allow monsters to override this distance (80?)
				if ( (m_Route[ i ].vecLocation - m_vecEnemyLKP).Length() > 80 )
				{
					// Refresh
					FRefreshRoute();
					return iUpdatedLKP;
				}
			}
		}
	}
	return iUpdatedLKP;
}

//=========================================================
// PushEnemy - remember the last few enemies, always remember the player
//=========================================================
void CBaseMonster::PushEnemy( CBaseEntity *pEnemy, Vector &vecLastKnownPos )
{
	if (pEnemy == NULL)
		return;

	short i;
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for (i = 0; i < MAX_OLD_ENEMIES; ++i)
	{
		if (m_hOldEnemy[i] == pEnemy)
			return;
		if (m_hOldEnemy[i].Get() == NULL) // someone died, reuse their slot
			break;
	}
	if (i >= MAX_OLD_ENEMIES)
		return;

	m_hOldEnemy[i] = pEnemy;
	m_vecOldEnemy[i] = vecLastKnownPos;
}

//=========================================================
// PopEnemy - try remembering the last few enemies
//=========================================================
BOOL CBaseMonster::PopEnemy(void)
{
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for (short i = MAX_OLD_ENEMIES - 1; i >= 0; --i)
	{
		if (m_hOldEnemy[i] != NULL)
		{
			if (m_hOldEnemy[i]->IsAlive()) // cheat and know when they die
			{
				m_hEnemy = m_hOldEnemy[i];
				m_vecEnemyLKP = m_vecOldEnemy[i];
				// ALERT( at_console, "remembering\n");
				return TRUE;
			}
			else
				m_hOldEnemy[i] = NULL;
		}
	}
	return FALSE;
}

//=========================================================
// SetActivity
//=========================================================
void CBaseMonster::SetActivity ( Activity NewActivity )
{
	int	iSequence = LookupActivity ( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if ( !(m_Activity == ACT_WALK || m_Activity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
				pev->frame = 0;
		}
		pev->sequence = iSequence;// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_aiconsole, "%s[%d] \"%s\" has no sequence for act: %d (%s) \n", STRING(pev->classname), entindex(), STRING(pev->targetname), NewActivity, activity_map[ActivityMapFind(NewActivity)].name);
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;
}

//=========================================================
// SetSequenceByName
//=========================================================
void CBaseMonster::SetSequenceByName ( char *szSequence )
{
	int	iSequence = LookupSequence ( szSequence );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_aiconsole, "%s has no sequence named:%f\n", STRING(pev->classname), szSequence );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

#define	LOCAL_STEP_SIZE	16
//=========================================================
// CheckLocalMove - returns TRUE if the caller can walk a
// straight line from its current origin to the given
// location. If so, don't use the node graph!
//
// if a valid pointer to a int is passed, the function
// will fill that int with the distance that the check
// reached before hitting something. THIS ONLY HAPPENS
// IF THE LOCAL MOVE CHECK FAILS!
//
// !!!PERFORMANCE - should we try to load balance this?
// DON"T USE SETORIGIN!
//=========================================================
int CBaseMonster::CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	Vector	vecStartPos;// record monster's position before trying the move
	float	flYaw;
	vec_t	flDist;
	float	flStep, stepSize;
	int		iReturn;

	vecStartPos = pev->origin;

	flYaw = VecToYaw(vecEnd - vecStart);// build a yaw that points to the goal.
	flDist = ( vecEnd - vecStart ).Length2D();// get the distance.
	iReturn = LOCALMOVE_VALID;// assume everything will be ok.

	// move the monster to the start of the local move that's to be checked.
	UTIL_SetOrigin(this, vecStart);// !!!BUGBUG - won't this fire triggers? - nope, SetOrigin doesn't fire

	if (!FBitSet(pev->flags, FL_FLY|FL_SWIM))
		DROP_TO_FLOOR(edict());//make sure monster is on the floor!

	//pev->origin.z = vecStartPos.z;//!!!HACKHACK

//	pev->origin = vecStart;

/*
	if ( flDist > 1024 )
	{
		// !!!PERFORMANCE - this operation may be too CPU intensive to try checks this large.
		// We don't lose much here, because a distance this great is very likely
		// to have something in the way.

		// since we've actually moved the monster during the check, undo the move.
		pev->origin = vecStartPos;
		return FALSE;
	}
*/
	// this loop takes single steps to the goal.
	for ( flStep = 0 ; flStep < flDist ; flStep += LOCAL_STEP_SIZE )
	{
		stepSize = LOCAL_STEP_SIZE;

		if ( (flStep + LOCAL_STEP_SIZE) >= (flDist-1) )
			stepSize = (flDist - flStep) - 1;

		//UTIL_ParticleEffect ( pev->origin, g_vecZero, 255, 25 );

		if (!WALK_MOVE(edict(), flYaw, stepSize, WALKMOVE_CHECKONLY))
		{// can't take the next step, fail!
			if ( pflDist != NULL )
			{
				*pflDist = flStep;
			}
			if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			{
				// if this step hits target ent, the move is legal.
				iReturn = LOCALMOVE_VALID;
				break;
			}
			else
			{
				// If we're going toward an entity, and we're almost getting there, it's OK.
//				if ( pTarget && fabs( flDist - iStep ) < LOCAL_STEP_SIZE )
//					fReturn = TRUE;
//				else
				iReturn = LOCALMOVE_INVALID;
				break;
			}

		}
	}

	if (iReturn == LOCALMOVE_VALID && !FBitSet(pev->flags, FL_FLY|FL_SWIM) && (!pTarget || FBitSet(pTarget->pev->flags, FL_ONGROUND)))
	{
		// The monster can move to a spot UNDER the target, but not to it. Don't try to triangulate, go directly to the node graph.
		// UNDONE: Magic # 64 -- this used to be pev->size.z but that won't work for small creatures like the headcrab
		if ( fabs(vecEnd.z - pev->origin.z) > 64 )
			iReturn = LOCALMOVE_INVALID_DONT_TRIANGULATE;
	}
	/*
	// uncommenting this block will draw a line representing the nearest legal move.
	WRITE_BYTE(MSG_BROADCAST, svc_temp_entity);
	WRITE_BYTE(MSG_BROADCAST, TE_SHOWLINE);
	WRITE_COORD(MSG_BROADCAST, pev->origin.x);
	WRITE_COORD(MSG_BROADCAST, pev->origin.y);
	WRITE_COORD(MSG_BROADCAST, pev->origin.z);
	WRITE_COORD(MSG_BROADCAST, vecStart.x);
	WRITE_COORD(MSG_BROADCAST, vecStart.y);
	WRITE_COORD(MSG_BROADCAST, vecStart.z);
	*/

	// since we've actually moved the monster during the check, undo the move.
	UTIL_SetOrigin(this, vecStartPos);
	return iReturn;
}


float CBaseMonster::OpenDoorAndWait( CBaseEntity *pDoor )
{
	float flTravelTime = 0;

	//ALERT(at_aiconsole, "A door. ");
	if (pDoor && !pDoor->IsLockedByMaster(this))
	{
		//ALERT(at_aiconsole, "unlocked! ");
		pDoor->Use(this, this, USE_ON, 0.0);
		//ALERT(at_aiconsole, "pevDoor->nextthink = %d ms\n", (int)(1000*pevDoor->nextthink));
		//ALERT(at_aiconsole, "pevDoor->ltime = %d ms\n", (int)(1000*pevDoor->ltime));
		//ALERT(at_aiconsole, "pev-> nextthink = %d ms\n", (int)(1000*pev->nextthink));
		//ALERT(at_aiconsole, "pev->ltime = %d ms\n", (int)(1000*pev->ltime));

		flTravelTime = pDoor->pev->nextthink - pDoor->pev->ltime;

		//ALERT(at_aiconsole, "Waiting %d ms\n", (int)(1000*flTravelTime));
		if ( pDoor->pev->targetname )
		{
			edict_t *pentTarget = NULL;
			for (;;)
			{
				pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING(pDoor->pev->targetname));

				if ( VARS( pentTarget ) != pDoor->pev )
				{
					if (FNullEnt(pentTarget))
						break;

					if ( FClassnameIs ( pentTarget, STRING(pDoor->pev->classname) ) )
					{
						CBaseEntity *pDoor2 = Instance(pentTarget);
						if (pDoor2)
							pDoor2->Use(this, this, USE_ON, 0.0);
					}
				}
			}
		}
	}

	return gpGlobals->time + flTravelTime;
}

//=========================================================
// AdvanceRoute - poorly named function that advances the
// m_iRouteIndex. If it goes beyond ROUTE_SIZE, the route
// is refreshed.
//=========================================================
void CBaseMonster::AdvanceRoute ( float distance )
{
	if ( m_iRouteIndex == ROUTE_SIZE - 1 )
	{
		// time to refresh the route.
		if ( !FRefreshRoute() )
			ALERT ( at_aiconsole, "Can't Refresh Route!!\n" );
	}
	else
	{
		if ( ! (m_Route[ m_iRouteIndex ].iType & bits_MF_IS_GOAL) )
		{
			// If we've just passed a path_corner, advance m_pGoalEnt
			if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_PATHCORNER )
				m_pGoalEnt = m_pGoalEnt->GetNextTarget();

			// IF both waypoints are nodes, then check for a link for a door and operate it.
			//
			if (  (m_Route[m_iRouteIndex].iType   & bits_MF_TO_NODE) == bits_MF_TO_NODE
			   && (m_Route[m_iRouteIndex+1].iType & bits_MF_TO_NODE) == bits_MF_TO_NODE
			   && WorldGraph.IsActive())// XDM3037a
			{
				//ALERT(at_aiconsole, "SVD: Two nodes. ");

				int iSrcNode  = WorldGraph.FindNearestNode(m_Route[m_iRouteIndex].vecLocation, this );
				int iDestNode = WorldGraph.FindNearestNode(m_Route[m_iRouteIndex+1].vecLocation, this );
				int iLink;
				WorldGraph.HashSearch(iSrcNode, iDestNode, iLink);
				if ( iLink >= 0 && WorldGraph.m_pLinkPool[iLink].m_pLinkEnt != NULL )
				{
					//ALERT(at_aiconsole, "A link. ");
					if ( WorldGraph.HandleLinkEnt ( iSrcNode, WorldGraph.m_pLinkPool[iLink].m_pLinkEnt, m_afCapability, CGraph::NODEGRAPH_DYNAMIC ) )
					{
						//ALERT(at_aiconsole, "usable.");
						entvars_t *pevDoor = WorldGraph.m_pLinkPool[iLink].m_pLinkEnt;
						if (pevDoor)
						{
							m_flMoveWaitFinished = OpenDoorAndWait(CBaseEntity::Instance(ENT(pevDoor)));
							//ALERT( at_aiconsole, "Wating for door %.2f\n", m_flMoveWaitFinished-gpGlobals->time );
						}
					}
				}
				//ALERT(at_aiconsole, "\n");
			}
			m_iRouteIndex++;
		}
		else	// At goal!!!
		{
			if ( distance < m_flGroundSpeed * 0.2 /* FIX */ )
				MovementComplete();
		}
	}
}

int CBaseMonster::RouteClassify( int iMoveFlag )
{
	int movementGoal = MOVEGOAL_NONE;
	if ( iMoveFlag & bits_MF_TO_TARGETENT )
		movementGoal = MOVEGOAL_TARGETENT;
	else if ( iMoveFlag & bits_MF_TO_ENEMY )
		movementGoal = MOVEGOAL_ENEMY;
	else if ( iMoveFlag & bits_MF_TO_PATHCORNER )
		movementGoal = MOVEGOAL_PATHCORNER;
	else if ( iMoveFlag & bits_MF_TO_NODE )
		movementGoal = MOVEGOAL_NODE;
	else if ( iMoveFlag & bits_MF_TO_LOCATION )
		movementGoal = MOVEGOAL_LOCATION;

	return movementGoal;
}

//=========================================================
// BuildRoute
//=========================================================
bool CBaseMonster::BuildRoute ( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget )
{
	float	flDist;
	Vector	vecApex;
	int		iLocalMove;

	RouteNew();
	m_movementGoal = RouteClassify( iMoveFlag );

// so we don't end up with no moveflags
	m_Route[ 0 ].vecLocation = vecGoal;
	m_Route[ 0 ].iType = iMoveFlag | bits_MF_IS_GOAL;

// check simple local move
	iLocalMove = CheckLocalMove( pev->origin, vecGoal, pTarget, &flDist );

	if ( iLocalMove == LOCALMOVE_VALID )
	{
		// monster can walk straight there!
		return TRUE;
	}
// try to triangulate around any obstacles.
	else if ( iLocalMove != LOCALMOVE_INVALID_DONT_TRIANGULATE && FTriangulate( pev->origin, vecGoal, flDist, pTarget, &vecApex ) )
	{
		// there is a slightly more complicated path that allows the monster to reach vecGoal
		m_Route[ 0 ].vecLocation = vecApex;
		m_Route[ 0 ].iType = (iMoveFlag | bits_MF_TO_DETOUR);
		m_Route[ 1 ].vecLocation = vecGoal;
		m_Route[ 1 ].iType = iMoveFlag | bits_MF_IS_GOAL;

			/*
			WRITE_BYTE(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(MSG_BROADCAST, TE_SHOWLINE);
			WRITE_COORD(MSG_BROADCAST, vecApex.x );
			WRITE_COORD(MSG_BROADCAST, vecApex.y );
			WRITE_COORD(MSG_BROADCAST, vecApex.z );
			WRITE_COORD(MSG_BROADCAST, vecApex.x );
			WRITE_COORD(MSG_BROADCAST, vecApex.y );
			WRITE_COORD(MSG_BROADCAST, vecApex.z + 128 );
			*/

		RouteSimplify( pTarget );
		return true;
	}

// last ditch, try nodes
	if ( FGetNodeRoute( vecGoal ) )
	{
		//ALERT ( at_console, "Can get there on nodes\n" );
		m_vecMoveGoal = vecGoal;
		RouteSimplify( pTarget );
		return true;
	}
	// b0rk
	return false;
}

//=========================================================
// InsertWaypoint - Rebuilds the existing route so that the
// supplied vector and moveflags are the first waypoint in
// the route, and fills the rest of the route with as much
// of the pre-existing route as possible
//=========================================================
void CBaseMonster::InsertWaypoint(const Vector &vecLocation, const int &afMoveFlags)
{
	int			i, type;
	// we have to save some Index and Type information from the real
	// path_corner or node waypoint that the monster was trying to reach. This makes sure that data necessary
	// to refresh the original path exists even in the new waypoints that don't correspond directy to a path_corner
	// or node.
	type = afMoveFlags | (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK);

	for ( i = ROUTE_SIZE-1; i > 0; i-- )
		m_Route[i] = m_Route[i-1];

	m_Route[ m_iRouteIndex ].vecLocation = vecLocation;
	m_Route[ m_iRouteIndex ].iType = type;
}

//=========================================================
// FTriangulate - tries to overcome local obstacles by
// triangulating a path around them.
//
// iApexDist is how far the obstruction that we are trying
// to triangulate around is from the monster.
//=========================================================
BOOL CBaseMonster::FTriangulate ( const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex )
{
	Vector		vecDir;
	Vector		vecForward;
	Vector		vecLeft;// the spot we'll try to triangulate to on the left
	Vector		vecRight;// the spot we'll try to triangulate to on the right
	Vector		vecTop;// the spot we'll try to triangulate to on the top
	Vector		vecBottom;// the spot we'll try to triangulate to on the bottom
	Vector		vecFarSide;// the spot that we'll move to after hitting the triangulated point, before moving on to our normal goal.
	int			i;
	float		sizeX, sizeZ;

	// If the hull width is less than 24, use 24 because CheckLocalMove uses a min of
	// 24.
	sizeX = pev->size.x;
	if (sizeX < 24.0)
		sizeX = 24.0;
	else if (sizeX > 48.0)
		sizeX = 48.0;

	sizeZ = pev->size.z;
	//if (sizeZ < 24.0)
	//	sizeZ = 24.0;

	vecForward = (vecEnd - vecStart);
	vecForward.NormalizeSelf();
	Vector vecDirUp(0,0,1);
	vecDir = CrossProduct(vecForward, vecDirUp);

	// start checking right about where the object is, picking two equidistant starting points, one on
	// the left, one on the right. As we progress through the loop, we'll push these away from the obstacle,
	// hoping to find a way around on either side. pev->size.x is added to the ApexDist in order to help select
	// an apex point that insures that the monster is sufficiently past the obstacle before trying to turn back
	// onto its original course.

	vecLeft = pev->origin + ( vecForward * ( flDist + sizeX ) ) - vecDir * ( sizeX * 3 );
	vecRight = pev->origin + ( vecForward * ( flDist + sizeX ) ) + vecDir * ( sizeX * 3 );
	if (pev->movetype == MOVETYPE_FLY)
	{
		vecTop = pev->origin + (vecForward * flDist) + (vecDirUp * sizeZ * 3);
		vecBottom = pev->origin + (vecForward * flDist) - (vecDirUp *  sizeZ * 3);
	}

	vecFarSide = m_Route[ m_iRouteIndex ].vecLocation;

	vecDir *= sizeX * 2.0f;
	if (pev->movetype == MOVETYPE_FLY)
		vecDirUp *= sizeZ * 2.0f;

	for ( i = 0 ; i < 8; ++i )
	{
// Debug, Draw the triangulation
#if 0
		MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( vecRight.x );
			WRITE_COORD( vecRight.y );
			WRITE_COORD( vecRight.z );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
			WRITE_BYTE( TE_SHOWLINE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( vecLeft.x );
			WRITE_COORD( vecLeft.y );
			WRITE_COORD( vecLeft.z );
		MESSAGE_END();
#endif

#if 0
		if (pev->movetype == MOVETYPE_FLY)
		{
			MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( vecTop.x );
				WRITE_COORD( vecTop.y );
				WRITE_COORD( vecTop.z );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
				WRITE_BYTE( TE_SHOWLINE );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( vecBottom.x );
				WRITE_COORD( vecBottom.y );
				WRITE_COORD( vecBottom.z );
			MESSAGE_END();
		}
#endif

		if ( CheckLocalMove( pev->origin, vecRight, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			if ( CheckLocalMove ( vecRight, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if ( pApex )
					*pApex = vecRight;

				return TRUE;
			}
		}
		if ( CheckLocalMove( pev->origin, vecLeft, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			if ( CheckLocalMove ( vecLeft, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if ( pApex )
					*pApex = vecLeft;

				return TRUE;
			}
		}

		if (pev->movetype == MOVETYPE_FLY)
		{
			if ( CheckLocalMove( pev->origin, vecTop, pTargetEnt, NULL ) == LOCALMOVE_VALID)
			{
				if ( CheckLocalMove ( vecTop, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
				{
					if ( pApex )
						*pApex = vecTop;
						//ALERT(at_aiconsole, "triangulate over\n");

					return TRUE;
				}
			}
#if 1
			if ( CheckLocalMove( pev->origin, vecBottom, pTargetEnt, NULL ) == LOCALMOVE_VALID )
			{
				if ( CheckLocalMove ( vecBottom, vecFarSide, pTargetEnt, NULL ) == LOCALMOVE_VALID )
				{
					if ( pApex )
						*pApex = vecBottom;
						//ALERT(at_aiconsole, "triangulate under\n");

					return TRUE;
				}
			}
#endif
		}

		vecRight += vecDir;
		vecLeft -= vecDir;
		if (pev->movetype == MOVETYPE_FLY)
		{
			vecTop += vecDirUp;
			vecBottom -= vecDirUp;
		}
	}
	//ALERT(at_aiconsole, "%s[%d] failed to triangulate!\n", STRING(pev->classname), entindex());
	return FALSE;
}

//=========================================================
// Move - take a single step towards the next ROUTE location
//=========================================================
#define DIST_TO_CHECK	200

void CBaseMonster::Move ( float flInterval )
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if ( m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute() )
		{
			ALERT( at_aiconsole, "Tried to move with no route!\n" );
			TaskFail();
			return;
		}
	}

	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 200, 0 );
#endif

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	// local move to waypoint.
	vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
	MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
	ChangeYaw ( pev->yaw_speed );

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if (flWaypointDist < DIST_TO_CHECK)
		flCheckDist = flWaypointDist;
	else
		flCheckDist = DIST_TO_CHECK;

	if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
	{
		// Can't move, stop
		Stop();
		// Blocking entity is in global trace_ent
		CBaseEntity *pBlocker = NULL;
		if (!FNullEnt(gpGlobals->trace_ent))// XDM3035c
		{
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if (pBlocker)
				DispatchBlocked( edict(), pBlocker->edict() );
		}
		if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
		{
			// Can we still move toward our target?
			if ( flDist < m_flGroundSpeed )
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step
		}
		else
		{
			// try to triangulate around whatever is in the way.
			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
				RouteSimplify( pTargetEnt );
			}
			else
			{
				//ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
				Stop();
				// Only do this once until your route is cleared
				if ( m_moveWaitTime > 0 && !(m_afMemory & bits_MEMORY_MOVE_FAILED) )
				{
					FRefreshRoute();
					if ( FRouteClear() )
					{
						TaskFail();
					}
					else
					{
						// Don't get stuck
						if ( (gpGlobals->time - m_flMoveWaitFinished) < 0.2 )
							Remember( bits_MEMORY_MOVE_FAILED );

						m_flMoveWaitFinished = gpGlobals->time + 0.1;
					}
				}
				else
				{
					TaskFail();
					ALERT(at_aiconsole, "%s[%d] \"%s\" failed to move (%d)!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), HasMemory(bits_MEMORY_MOVE_FAILED)?1:0);
					//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
				}
				return;
			}
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if (ShouldAdvanceRoute(flWaypointDist))///!!!BUGBUG- magic number
		AdvanceRoute(flWaypointDist);

	// Might be waiting for a door
	if (m_flMoveWaitFinished > gpGlobals->time)
	{
		Stop();
		return;
	}

	// UNDONE: this is a hack to quit moving farther than it has looked ahead.
	if (flCheckDist < m_flGroundSpeed * flInterval)
	{
		flInterval = flCheckDist / m_flGroundSpeed;
		// ALERT( at_console, "%.02f\n", flInterval );
	}
	MoveExecute(pTargetEnt, vecDir, flInterval);

	if (MovementIsComplete())
	{
		Stop();
		RouteClear();
	}
}

BOOL CBaseMonster::ShouldAdvanceRoute(float flWaypointDist)
{
	if (flWaypointDist <= MONSTER_CUT_CORNER_DIST)
	{
		// ALERT( at_console, "cut %f\n", flWaypointDist );
		return TRUE;
	}
	return FALSE;
}

void CBaseMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	//float flYaw = VecToYaw ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin );// build a yaw that points to the goal.
	//WALK_MOVE( edict(), flYaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	//if (m_IdealActivity != m_movementActivity)
		m_IdealActivity = m_movementActivity;

	float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	float flStep;
	while (flTotal > 0.001)
	{
		// don't walk more than 16 units or stairs stop working
		flStep = min(16.0, flTotal);
		UTIL_MoveToOrigin(edict(), m_Route[m_iRouteIndex].vecLocation, flStep, MOVE_NORMAL);
		flTotal -= flStep;
	}
	// ALERT( at_console, "dist %f\n", m_flGroundSpeed * pev->framerate * flInterval );
}

//=========================================================
// MonsterInitThink - Calls StartMonster. Startmonster is virtual, but this function cannot be
//=========================================================
void CBaseMonster::MonsterInitThink(void)
{
	StartMonster();
}

// SHL
void CBaseMonster::StartPatrol(CBaseEntity *path)
{
	m_pGoalEnt = path;
	if (m_pGoalEnt)
	{
		// Monster will start turning towards his destination
		//MakeIdealYaw ( m_pGoalEnt->pev->origin );

		// set the monster up to walk a path corner path.
		// !!!BUGBUG - this is a minor bit of a hack.
		// JAYJAY
		m_movementGoal = MOVEGOAL_PATHCORNER;

		if (pev->movetype == MOVETYPE_FLY)
			m_movementActivity = ACT_FLY;
		else
			m_movementActivity = ACT_WALK;

		if (!FRefreshRoute())
			ALERT(at_aiconsole, "StartPatrol(): %s Can't create route!\n", STRING(pev->targetname));

		SetMonsterState(MONSTERSTATE_IDLE);
		ChangeSchedule(GetScheduleOfType(SCHED_IDLE_WALK));
	}
	else
		ALERT(at_error, "StartPatrol(): %s couldn't find target \"%s\"\n", STRING(pev->classname), STRING(pev->target));
}

//=========================================================
// StartMonster - final bit of initization before a monster
// is turned over to the AI.
//=========================================================
void CBaseMonster::StartMonster(void)
{
	// update capabilities
	if ( LookupActivity ( ACT_RANGE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_RANGE_ATTACK1;

	if ( LookupActivity ( ACT_RANGE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_RANGE_ATTACK2;

	if ( LookupActivity ( ACT_MELEE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_MELEE_ATTACK1;

	if ( LookupActivity ( ACT_MELEE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_MELEE_ATTACK2;

	// Raise monster off the floor one unit, then drop to floor
	if ( pev->movetype != MOVETYPE_FLY && !FBitSet( pev->spawnflags, SF_MONSTER_FALL_TO_GROUND ) )
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR(edict());
		// Try to move the monster to make sure it's not stuck in a brush.
		if (!WALK_MOVE(edict(), 0, 0, WALKMOVE_NORMAL))
		{
			ALERT(at_console, "Design error: %s \"%s\" stuck in wall\n", STRING(pev->classname), STRING(pev->targetname));
			if (g_pCvarDeveloper && g_pCvarDeveloper->value >= 2.0f)
				SetBits(pev->effects, EF_BRIGHTFIELD);
		}
	}
	else
		ClearBits(pev->flags, FL_ONGROUND);

	if (!FStringNull(pev->target))// this monster has a target
		StartPatrol(UTIL_FindEntityByTargetname(NULL, STRING(pev->target)));

	//SetMonsterState ( m_IdealMonsterState );
	//SetActivity ( m_IdealActivity );

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink(&CBaseMonster::CallMonsterThink);
	pev->nextthink += RANDOM_FLOAT(0.1, 0.4); // spread think times.

	// XDM3038: HACK? flags. BUGBUG: makes named monsters spawned by maker wait before acting
	if (!FStringNull(pev->targetname) && FBitSet(pev->spawnflags, SF_MONSTER_WAIT_TILL_SEEN|SF_MONSTER_WAIT_UNTIL_PROVOKED|SF_MONSTER_WAIT_FOR_SCRIPT))// wait until triggered
	{
		SetMonsterState( MONSTERSTATE_IDLE );
		// UNDONE: Some scripted sequence monsters don't have an idle?
		SetActivity( ACT_IDLE );
		ChangeSchedule( GetScheduleOfType( SCHED_WAIT_TRIGGER ) );
	}
}

void CBaseMonster::MovementComplete(void)
{
	switch (m_iTaskStatus)
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUNNING:
		m_iTaskStatus = TASKSTATUS_RUNNING_TASK;
		break;

	case TASKSTATUS_RUNNING_MOVEMENT:
		TaskComplete();
		break;

	case TASKSTATUS_RUNNING_TASK:
		ALERT(at_aiconsole, "ERROR: Movement completed twice!\n");
		break;

	case TASKSTATUS_COMPLETE:
		break;
	}
	m_movementGoal = MOVEGOAL_NONE;
}

int CBaseMonster::TaskIsRunning(void)
{
	if (m_iTaskStatus != TASKSTATUS_COMPLETE &&
		m_iTaskStatus != TASKSTATUS_RUNNING_MOVEMENT)
		return 1;

	return 0;
}

//=========================================================
// FindCover - tries to find a nearby node that will hide
// the caller from its enemy.
//
// If supplied, search will return a node at least as far
// away as MinDist, but no farther than MaxDist.
// if MaxDist isn't supplied, it defaults to a reasonable
// value
//=========================================================
// UNDONE: Should this find the nearest node?
//float CGraph::PathLength( int iStart, int iDest, int iHull, int afCapMask )
bool CBaseMonster::FindCover(const Vector &vecThreat, const Vector &vecViewOffset, float flMinDist, float flMaxDist)
{
	if (!WorldGraph.IsActive())// XDM3037a
		return false;

	int i;
	int iMyHullIndex;
	int iMyNode;
	int iThreatNode;
	vec_t flDist;
	Vector	vecLookersOffset;
	TraceResult tr;

	if (flMaxDist == 0.0f)
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if ( flMinDist > 0.5f * flMaxDist)
	{
#if _DEBUG
		ALERT ( at_aiconsole, "FindCover MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5f * flMaxDist;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
	iThreatNode = WorldGraph.FindNearestNode ( vecThreat, this );
	iMyHullIndex = WorldGraph.HullIndex( this );

	if ( iMyNode == NO_NODE )
	{
//SPAM	ALERT ( at_aiconsole, "FindCover() - %s has no nearest node!\n", STRING(pev->classname));
		return FALSE;
	}
	if ( iThreatNode == NO_NODE )
	{
		// ALERT ( at_aiconsole, "FindCover() - Threat has no nearest node!\n" );
		iThreatNode = iMyNode;
		// return FALSE;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for ( i = 0 ; i < WorldGraph.m_cNodes ; ++i)
	{
		int nodeNumber = (i + WorldGraph.m_iLastCoverSearch) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// could use an optimization here!!
		flDist = ( pev->origin - node.m_vecOrigin ).Length();

		// DON'T do the trace check on a node that is farther away than a node that we've already found to
		// provide cover! Also make sure the node is within the mins/maxs of the search.
		if ( flDist >= flMinDist && flDist < flMaxDist )
		{
			UTIL_TraceLine ( node.m_vecOrigin + vecViewOffset, vecLookersOffset, ignore_monsters, ignore_glass,  edict(), &tr );

			// if this node will block the threat's line of sight to me...
			if ( tr.flFraction != 1.0 )
			{
				// ..and is also closer to me than the threat, or the same distance from myself and the threat the node is good.
				if ( ( iMyNode == iThreatNode ) || WorldGraph.PathLength( iMyNode, nodeNumber, iMyHullIndex, m_afCapability ) <= WorldGraph.PathLength( iThreatNode, nodeNumber, iMyHullIndex, m_afCapability ) )
				{
					if ( FValidateCover ( node.m_vecOrigin ) && MoveToLocation( ACT_RUN, 0, node.m_vecOrigin ) )
					{
						/*
						MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
							WRITE_BYTE( TE_SHOWLINE);

							WRITE_COORD( node.m_vecOrigin.x );
							WRITE_COORD( node.m_vecOrigin.y );
							WRITE_COORD( node.m_vecOrigin.z );

							WRITE_COORD( vecLookersOffset.x );
							WRITE_COORD( vecLookersOffset.y );
							WRITE_COORD( vecLookersOffset.z );
						MESSAGE_END();
						*/
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

//=========================================================
// BuildNearestRoute - tries to build a route as close to the target
// as possible, even if there isn't a path to the final point.
//
// If supplied, search will return a node at least as far
// away as MinDist from vecThreat, but no farther than MaxDist.
// if MaxDist isn't supplied, it defaults to a reasonable
// value
//=========================================================
bool CBaseMonster::BuildNearestRoute(const Vector &vecThreat, const Vector &vecViewOffset, float flMinDist, float flMaxDist)
{
	if (!WorldGraph.IsActive())// XDM3037a
		return false;

	int i;
	int iMyHullIndex;
	int iMyNode;
	vec_t flDist;
	Vector	vecLookersOffset;
	TraceResult tr;

	if (flMaxDist == 0.0f)
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if ( flMinDist > 0.5f * flMaxDist)
	{
#if _DEBUG
		ALERT ( at_aiconsole, "BuildNearestRoute MinDist (%.0f) too close to MaxDist (%.0f)\n", flMinDist, flMaxDist );
#endif
		flMinDist = 0.5f * flMaxDist;
	}

	iMyNode = WorldGraph.FindNearestNode( pev->origin, this );
	iMyHullIndex = WorldGraph.HullIndex( this );

	if ( iMyNode == NO_NODE )
	{
//SPAM	ALERT ( at_aiconsole, "BuildNearestRoute() - %s has no nearest node!\n", STRING(pev->classname));
		return FALSE;
	}

	vecLookersOffset = vecThreat + vecViewOffset;// calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for ( i = 0 ; i < WorldGraph.m_cNodes ; ++i)
	{
		int nodeNumber = (i + WorldGraph.m_iLastCoverSearch) % WorldGraph.m_cNodes;

		CNode &node = WorldGraph.Node( nodeNumber );
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// can I get there?
		if (WorldGraph.NextNodeInRoute( iMyNode, nodeNumber, iMyHullIndex, 0 ) != iMyNode)
		{
			flDist = ( vecThreat - node.m_vecOrigin ).Length();

			// is it close?
			if ( flDist > flMinDist && flDist < flMaxDist)
			{
				// can I see where I want to be from there?
				UTIL_TraceLine( node.m_vecOrigin + pev->view_ofs, vecLookersOffset, ignore_monsters, edict(), &tr );

				if (tr.flFraction == 1.0)
				{
					// try to actually get there
					if ( BuildRoute ( node.m_vecOrigin, bits_MF_TO_LOCATION, NULL ) )
					{
						flMaxDist = flDist;
						m_vecMoveGoal = node.m_vecOrigin;
						return true; // UNDONE: keep looking for something closer!
					}
				}
			}
		}
	}
	return false;
}

//=========================================================
// BestVisibleEnemy - this functions searches the link
// list whose head is the caller's m_pLink field, and returns
// a pointer to the enemy entity in that list that is nearest the
// caller.
//
// !!!UNDONE - currently, this only returns the closest enemy.
// we'll want to consider distance, relationship, attack types, back turned, etc.
//=========================================================
CBaseEntity *CBaseMonster::BestVisibleEnemy(void)
{
	CBaseEntity	*pReturn = NULL;
	CBaseEntity	*pNextEnt = m_pLink;
	vec_t		fDist;
	vec_t		fNearest = VISIBLE_ENEMY_MAX_DIST;// so first visible entity will become the closest.
	int			iBestRelationship = R_NO;

	while (pNextEnt != NULL)
	{
		if (pNextEnt->IsAlive())
		{
			if (IRelationship(pNextEnt) > iBestRelationship)
			{
				// this entity is disliked MORE than the entity that we
				// currently think is the best visible enemy. No need to do
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship(pNextEnt);
				fNearest = (pNextEnt->pev->origin - pev->origin).Length();
				pReturn = pNextEnt;
			}
			else if (IRelationship(pNextEnt) == iBestRelationship)
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				fDist = (pNextEnt->pev->origin - pev->origin).Length();
				if (fDist <= fNearest)
				{
					fNearest = fDist;
					iBestRelationship = IRelationship(pNextEnt);
					pReturn = pNextEnt;
				}
			}
		}
		pNextEnt = pNextEnt->m_pLink;
	}
	return pReturn;
}


//=========================================================
// MakeIdealYaw - gets a yaw value for the caller that would
// face the supplied vector. Value is stuffed into the monster's ideal_yaw
//=========================================================
void CBaseMonster::MakeIdealYaw(const Vector &vecTarget)
{
	Vector	vecProjection;
	// strafing monster needs to face 90 degrees away from its goal
	if ( m_movementActivity == ACT_STRAFE_LEFT )
	{
		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;
		pev->ideal_yaw = VecToYaw(vecProjection - pev->origin);
	}
	else if ( m_movementActivity == ACT_STRAFE_RIGHT )
	{
		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;
		pev->ideal_yaw = VecToYaw(vecProjection - pev->origin);
	}
	else
		pev->ideal_yaw = VecToYaw(vecTarget - pev->origin);
}

//=========================================================
// FlYawDiff - returns the difference ( in degrees ) between
// monster's current yaw and ideal_yaw
//
// Positive result is left turn, negative is right turn
//=========================================================
float CBaseMonster::FlYawDiff (void)
{
	float flCurrentYaw = UTIL_AngleMod(pev->angles.y);// XDM3036? FOUND THE ERROR! pev->angles.y is somehow out of bounds!?
	if (flCurrentYaw == pev->ideal_yaw)
		return 0;

	return AngleDiff(pev->ideal_yaw, flCurrentYaw);
}

//=========================================================
// Changeyaw - turns a monster towards its ideal_yaw
//=========================================================
float CBaseMonster::ChangeYaw(float yawSpeed)
{
	float move, speed;
	float current = UTIL_AngleMod(pev->angles.y);
	float ideal = pev->ideal_yaw;
	if (current != ideal)
	{
		speed = /*(float)*/yawSpeed * gpGlobals->frametime * 10.0f;
		move = ideal - current;

		if (ideal > current)
		{
			if (move >= 180)
				move -= 360;
		}
		else
		{
			if (move <= -180)
				move += 360;
		}
		//NormalizeAngle(&move);

		if (move > 0)
		{// turning to the monster's left
			if (move > speed)
				move = speed;
		}
		else
		{// turning to the monster's right
			if (move < -speed)
				move = -speed;
		}

		pev->angles.y = UTIL_AngleMod(current + move);

		// turn head in desired direction only if they have a turnable head
		if (m_afCapability & bits_CAP_TURN_HEAD)
		{
			float yaw = pev->ideal_yaw - pev->angles.y;
			NormalizeAngle180(&yaw);
			// yaw *= 0.8;
			SetBoneController(0, yaw);
		}
	}
	else
		move = 0;

	return move;
}

//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
vec_t CBaseMonster::VecToYaw(const Vector &vecDir)
{
	//if (vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0)
	if (vecDir.IsZero())
		return pev->angles.y;

	return ::VecToYaw(vecDir);// XDM3038a: !!!
}

//=========================================================
// SetEyePosition
//
// queries the monster's model for $eyeposition and copies
// that vector to the monster's view_ofs
//
//=========================================================
void CBaseMonster::SetEyePosition(void)
{
	GetEyePosition(GET_MODEL_PTR(edict()), pev->view_ofs);

/*#if defined (_DEBUG)
	if (pev->view_ofs.IsZero())
		ALERT(at_aiconsole, "SetEyePosition(): %s %s has no view_ofs!\n", STRING(pev->classname), STRING(pev->targetname));
#endif*/
}

// Model animation events
void CBaseMonster::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case SCRIPT_EVENT_DEAD:
		if ( m_MonsterState == MONSTERSTATE_SCRIPT )
		{
			pev->deadflag = DEAD_DYING;
			// Kill me now! (and fade out when CineCleanup() is called)
#if _DEBUG
			ALERT( at_aiconsole, "Death event: %s\n", STRING(pev->classname) );
#endif
			pev->health = 0;
		}
#if _DEBUG
		else
			ALERT( at_aiconsole, "INVALID death event:%s\n", STRING(pev->classname) );
#endif
		break;
	case SCRIPT_EVENT_NOT_DEAD:
		if ( m_MonsterState == MONSTERSTATE_SCRIPT )
		{
			pev->deadflag = DEAD_NO;
			// This is for life/death sequences where the player can determine whether a character is dead or alive after the script
			pev->health = pev->max_health;
		}
		break;

	case SCRIPT_EVENT_SOUND:			// Play a named wave file
		EMIT_SOUND_DYN(edict(), CHAN_BODY, pEvent->options, GetSoundVolume(), ATTN_IDLE, 0, m_voicePitch);
		break;

	case SCRIPT_EVENT_SOUND_VOICE:
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, pEvent->options, GetSoundVolume(), ATTN_IDLE, 0, m_voicePitch);
		break;

	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 33% of the time
		if (RANDOM_LONG(0,2) == 0)
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:			// Play a named sentence group
		SENTENCEG_PlayRndSz(edict(), pEvent->options, GetSoundVolume(), ATTN_IDLE, 0, m_voicePitch);
		break;

	case SCRIPT_EVENT_FIREEVENT:		// Fire a trigger
		FireTargets(pEvent->options, this, this, USE_TOGGLE, 0);
		break;

	case SCRIPT_EVENT_NOINTERRUPT:		// Can't be interrupted from now on
		if (m_pCine)
			m_pCine->AllowInterrupt( FALSE );
		break;

	case SCRIPT_EVENT_CANINTERRUPT:		// OK to interrupt now
		if (m_pCine)
			m_pCine->AllowInterrupt( TRUE );
		break;

#if 0
	case SCRIPT_EVENT_INAIR:			// Don't DROP_TO_FLOOR()
	case SCRIPT_EVENT_ENDANIMATION:		// Set ending animation sequence to
		break;
#endif

	case MONSTER_EVENT_BODYDROP_HEAVY:
		if (pev->flags & FL_ONGROUND)
		{
			EMIT_SOUND_DYN(edict(), CHAN_BODY, gSoundsDropBody[RANDOM_LONG(0,NUM_BODYDROP_SOUNDS-1)], RANDOM_FLOAT(0.8, 1.0), ATTN_NORM, 0, PITCH_LOW);
		}
		break;

	case MONSTER_EVENT_BODYDROP_LIGHT:
		if (pev->flags & FL_ONGROUND)
		{
			EMIT_SOUND_DYN(edict(), CHAN_BODY, gSoundsDropBody[RANDOM_LONG(0,NUM_BODYDROP_SOUNDS-1)], RANDOM_FLOAT(0.6, 0.8), ATTN_NORM, 0, PITCH_NORM);
		}
		break;

	case MONSTER_EVENT_SWISHSOUND:
		{
			// NO MONSTER may use this anim event unless that monster's precache precaches this sound!!!
			EMIT_SOUND(edict(), CHAN_BODY, "zombie/claw_miss2.wav", VOL_NORM, ATTN_NORM);
			break;
		}

	default:
		ALERT( at_aiconsole, "%s[%d] %d: Unhandled animation event: %d\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pEvent->event);
		break;

	}
}

//=========================================================
// FGetNodeRoute - tries to build an entire node path from
// the callers origin to the passed vector. If this is
// possible, ROUTE_SIZE waypoints will be copied into the
// callers m_Route. TRUE is returned if the operation
// succeeds (path is valid) or FALSE if failed (no path
// exists )
//=========================================================
bool CBaseMonster::FGetNodeRoute (const Vector &vecDest)
{
	if (!WorldGraph.IsActive())// XDM3037a
	{
//		ALERT(at_aiconsole, "CBaseMonster()::FGetNodeRoute() WorldGraph.!IsActive()\n");
		return false;
	}
	int iPath[ MAX_PATH_SIZE ];
	int iSrcNode, iDestNode;
	int iResult;
	int i;
	int iNumToCopy;

	iSrcNode = WorldGraph.FindNearestNode ( pev->origin, this );
	iDestNode = WorldGraph.FindNearestNode ( vecDest, this );

	if ( iSrcNode == -1 )
	{
		// no node nearest self
//		ALERT ( at_aiconsole, "FGetNodeRoute: No valid node near self!\n" );
		return false;
	}
	else if ( iDestNode == -1 )
	{
		// no node nearest target
//		ALERT ( at_aiconsole, "FGetNodeRoute: No valid node near target!\n" );
		return false;
	}

	// valid src and dest nodes were found, so it's safe to proceed with
	// find shortest path
	int iNodeHull = WorldGraph.HullIndex( this ); // make this a monster virtual function
	iResult = WorldGraph.FindShortestPath ( iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability );

	if ( !iResult )
	{
#if 1
		ALERT ( at_aiconsole, "No Path from %d to %d!\n", iSrcNode, iDestNode );
		return false;
#else
		BOOL bRoutingSave = WorldGraph.m_fRoutingComplete;
		WorldGraph.m_fRoutingComplete = FALSE;
		iResult = WorldGraph.FindShortestPath(iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability);
		WorldGraph.m_fRoutingComplete = bRoutingSave;
		if ( !iResult )
		{
			ALERT(at_aiconsole, "CBaseMonster(%d)::FGetNodeRoute() No Path from %d to %d!\n", entindex(), iSrcNode, iDestNode );
			return false;
		}
		else
		{
			ALERT(at_aiconsole, "CBaseMonster(%d)::FGetNodeRoute() Routing is inconsistent!", entindex());
		}
#endif
	}

	// there's a valid path within iPath now, so now we will fill the route array
	// up with as many of the waypoints as it will hold.

	// don't copy ROUTE_SIZE entries if the path returned is shorter
	// than ROUTE_SIZE!!!
	if ( iResult < ROUTE_SIZE )
	{
		iNumToCopy = iResult;
	}
	else
	{
		iNumToCopy = ROUTE_SIZE;
	}

	for ( i = 0 ; i < iNumToCopy; ++i)
	{
		m_Route[ i ].vecLocation = WorldGraph.m_pNodes[ iPath[ i ] ].m_vecOrigin;
		m_Route[ i ].iType = bits_MF_TO_NODE;
	}

	if ( iNumToCopy < ROUTE_SIZE )
	{
		m_Route[ iNumToCopy ].vecLocation = vecDest;
		m_Route[ iNumToCopy ].iType |= bits_MF_IS_GOAL;
	}
	return true;
}

//=========================================================
// FindHintNode
//=========================================================
int CBaseMonster::FindHintNode(void)
{
	if (!WorldGraph.IsActive())// XDM3037a
		return false;

	int i, nodeNumber;
	Vector vStart(pev->origin); vStart += pev->view_ofs;
	TraceResult tr;
	if (WorldGraph.m_iLastActiveIdleSearch >= WorldGraph.m_cNodes)
		WorldGraph.m_iLastActiveIdleSearch = 0;

	for ( i = 0; i < WorldGraph.m_cNodes ; ++i)
	{
		nodeNumber = (i + WorldGraph.m_iLastActiveIdleSearch) % WorldGraph.m_cNodes;
		CNode &node = WorldGraph.Node( nodeNumber );
		if ( node.m_sHintType )
		{
			// this node has a hint. Take it if it is visible, the monster likes it, and the monster has an animation to match the hint's activity.
			if (FValidateHintType(node.m_sHintType))
			{
				if ( !node.m_sHintActivity || LookupActivity ( node.m_sHintActivity ) != ACTIVITY_NOT_AVAILABLE )
				{
					UTIL_TraceLine(vStart, node.m_vecOrigin + pev->view_ofs, ignore_monsters, edict(), &tr );
					if (tr.flFraction == 1.0)
					{
						WorldGraph.m_iLastActiveIdleSearch = nodeNumber + 1; // next monster that searches for hint nodes will start where we left off.
						return nodeNumber;// take it!
					}
				}
			}
			else
				ALERT(at_aiconsole, "%s[%d]: Couldn't validate hint type %hd\n", STRING(pev->classname), entindex(), node.m_sHintType);
		}
	}
	WorldGraph.m_iLastActiveIdleSearch = 0;// start at the top of the list for the next search.
	return NO_NODE;
}

//=========================================================
// FCheckAITrigger - checks the monster's AI Trigger Conditions,
// if there is a condition, then checks to see if condition is
// met. If yes, the monster's TriggerTarget is fired.
//
// Returns TRUE if the target is fired.
//=========================================================
bool CBaseMonster::FCheckAITrigger (void)
{
	if (m_iTriggerCondition == AITRIGGER_NONE)
	{
		// no conditions, so this trigger is never fired.
		return false;
	}

	BOOL fFireTarget = FALSE;
	switch ( m_iTriggerCondition )
	{
	case AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER:
		if ( m_hEnemy != NULL && m_hEnemy->IsPlayer() && HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_SEEPLAYER_UNCONDITIONAL:
		if ( HasConditions ( bits_COND_SEE_CLIENT ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_SEEPLAYER_NOT_IN_COMBAT:
		if ( HasConditions ( bits_COND_SEE_CLIENT ) &&
			 m_MonsterState != MONSTERSTATE_COMBAT	&&
			 m_MonsterState != MONSTERSTATE_PRONE	&&
			 m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_TAKEDAMAGE:
		if ( m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_DEATH:
		if ( pev->deadflag != DEAD_NO )
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HALFHEALTH:
		if ( IsAlive() && pev->health <= ( pev->max_health / 2 ) )
		{
			fFireTarget = TRUE;
		}
		break;

// !!!UNDONE - no persistant game state that allows us to track these two.
/*
	case AITRIGGER_SQUADMEMBERDIE:
		break;
	case AITRIGGER_SQUADLEADERDIE:
		break;
*/
	case AITRIGGER_HEARWORLD:
		if ((m_afConditions & bits_COND_HEAR_SOUND) && (m_afSoundTypes & bits_SOUND_WORLD))
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HEARPLAYER:
		if ((m_afConditions & bits_COND_HEAR_SOUND) && (m_afSoundTypes & bits_SOUND_PLAYER))
		{
			fFireTarget = TRUE;
		}
		break;
	case AITRIGGER_HEARCOMBAT:
		if ((m_afConditions & bits_COND_HEAR_SOUND) && (m_afSoundTypes & bits_SOUND_COMBAT))
		{
			fFireTarget = TRUE;
		}
		break;
	}

	if (fFireTarget)
	{
		/*if (bPlayMusicEvent)// XDM
		{
			g_musicEventTrack = m_iszMusicEventTrack;
			MP3_PlayState(MST_EVENT);
			m_bPlayMusicEvent = FALSE;
		}*/
		// fire the target, then set the trigger conditions to NONE so we don't fire again
		ALERT(at_aiconsole, "%s (%s)::FCheckAITrigger(%d): firing target: %s\n", STRING(pev->classname), STRING(pev->targetname), m_iTriggerCondition, STRING(m_iszTriggerTarget));
		FireTargets( STRING( m_iszTriggerTarget ), this, this, USE_TOGGLE, 0 );
		m_iTriggerCondition = AITRIGGER_NONE;
		return true;
	}

	return false;
}


//=========================================================
// CanPlaySequence - determines whether or not the monster
// can play the scripted sequence or AI sequence that is
// trying to possess it. If DisregardState is set, the monster
// will be sucked into the script no matter what state it is
// in. ONLY Scripted AI ents should allow this.
//=========================================================
int CBaseMonster::CanPlaySequence( BOOL fDisregardMonsterState, int interruptLevel )
{
	if ( m_pCine || !IsAlive() || m_MonsterState == MONSTERSTATE_PRONE )
	{
		// monster is already running a scripted sequence or dead!
		return FALSE;
	}

	if ( fDisregardMonsterState )
	{
		// ok to go, no matter what the monster state. (scripted AI)
		return TRUE;
	}

	if ( m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE )
	{
		// ok to go, but only in these states
		return TRUE;
	}

	if ( m_MonsterState == MONSTERSTATE_ALERT && interruptLevel >= SS_INTERRUPT_BY_NAME )
		return TRUE;

	// unknown situation
	return FALSE;
}

//=========================================================
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//=========================================================
#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks

BOOL CBaseMonster::FindLateralCover(const Vector &vecThreat, const Vector &vecViewOffset)
{
	DBG_MON_PRINTF("%s[%d]::FindLateralCover()\n", STRING(pev->classname), entindex());
	TraceResult	tr;
	Vector	vecBestOnLeft;
	Vector	vecBestOnRight;
	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	int		i;

	UTIL_MakeVectors ( pev->angles );
	vecStepRight = gpGlobals->v_right * COVER_DELTA;
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = pev->origin;

	for ( i = 0 ; i < COVER_CHECKS ; ++i)
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine( vecThreat + vecViewOffset, vecLeftTest + pev->view_ofs, ignore_monsters, ignore_glass, edict()/*pentIgnore*/, &tr);

		if (tr.flFraction != 1.0)
		{
			if ( FValidateCover ( vecLeftTest ) && CheckLocalMove( pev->origin, vecLeftTest, NULL, NULL ) == LOCALMOVE_VALID )
			{
				if ( MoveToLocation( ACT_RUN, 0, vecLeftTest ) )
					return TRUE;
			}
		}

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine(vecThreat + vecViewOffset, vecRightTest + pev->view_ofs, ignore_monsters, ignore_glass, edict()/*pentIgnore*/, &tr);

		if ( tr.flFraction != 1.0 )
		{
			if (  FValidateCover ( vecRightTest ) && CheckLocalMove( pev->origin, vecRightTest, NULL, NULL ) == LOCALMOVE_VALID )
			{
				if ( MoveToLocation( ACT_RUN, 0, vecRightTest ) )
					return TRUE;
			}
		}
	}
	return FALSE;
}


// XDM: SHL compatibility
Vector CBaseMonster::ShootAtEnemy(const Vector &shootOrigin)
{
	//DBG_MON_PRINTF("%s[%d]::ShootAtEnemy()\n", STRING(pev->classname), entindex());
	if (m_pCine != NULL && m_hTargetEnt != NULL)// && (m_pCine->m_fTurnType == 1))
	{
		//Vector vecDest = ( m_hTargetEnt->pev->absmin + m_hTargetEnt->pev->absmax ) / 2;
		return (m_hTargetEnt->BodyTarget(shootOrigin) - shootOrigin).Normalize();
	}
	else if (m_hEnemy)
	{
		return ((m_hEnemy->BodyTarget(shootOrigin) - m_hEnemy->pev->origin) + m_vecEnemyLKP - shootOrigin).Normalize();
	}
	return gpGlobals->v_forward;
}

//=========================================================
// FacingIdeal - tells us if a monster is facing its ideal
// yaw. Created this function because many spots in the
// code were checking the yawdiff against this magic
// number. Nicer to have it in one place if we're gonna
// be stuck with it.
//=========================================================
bool CBaseMonster::FacingIdeal(void)
{
	//ASSERT(pev->angles.y > -180 && pev->angles.y < 180);
	if (fabs(FlYawDiff()) <= 0.006)//!!!BUGBUG - no magic numbers!!!
		return true;

	return false;
}

//=========================================================
// FCanActiveIdle
//=========================================================
bool CBaseMonster::FCanActiveIdle(void)
{
	//if ( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE && !IsMoving() )
	//	return TRUE;

	return false;
}

void CBaseMonster::PlaySentence(const char *pszSentence, float duration, float volume, float attenuation)
{
	DBG_MON_PRINTF("%s[%d]::PlaySentence(\"%s\", duration %g)\n", STRING(pev->classname), entindex(), pszSentence, duration);
	if (pszSentence && IsAlive())
	{
		if (pszSentence[0] == '!')
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, PITCH_NORM);
		else
			SENTENCEG_PlayRndSz(edict(), pszSentence, volume, attenuation, 0, PITCH_NORM);
	}
}

void CBaseMonster::PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener)
{
	DBG_MON_PRINTF("%s[%d]::PlayScriptedSentence(\"%s\", duration %g)\n", STRING(pev->classname), entindex(), pszSentence, duration);
	PlaySentence(pszSentence, duration, volume, attenuation);
}

void CBaseMonster::SentenceStop(void)
{
	DBG_MON_PRINTF("%s[%d]::SentenceStop()\n", STRING(pev->classname), entindex());
	EMIT_SOUND(edict(), CHAN_VOICE, "common/null.wav", VOL_NORM, ATTN_IDLE);
}

// XDM3035a: UNDONE: use this on normal monsters?
void CBaseMonster::CorpseFallThink(void)
{
	if (pev->flags & FL_ONGROUND)
	{
		//if (pev->flFallVelocity > 0.0f)// must be valid once!
		{
		SetThinkNull();

		SetSequenceBox();
		UTIL_SetOrigin(this, pev->origin);// link into world.

		/* use this code somewhere else
		AlignToFloor();// XDM3035a: TESTME
		CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 100+m_iGibCount, 10);// XDM3035a

		if (m_iGibCount >= 10)// XDM3035a
			EMIT_SOUND_DYN(edict(), CHAN_BODY, gSoundsDropBody[RANDOM_LONG(0,NUM_BODYDROP_SOUNDS-1)], RANDOM_FLOAT(0.8, 1.0), ATTN_NORM, 0, PITCH_LOW);
		else
			EMIT_SOUND_DYN(edict(), CHAN_BODY, gSoundsDropBody[RANDOM_LONG(0,NUM_BODYDROP_SOUNDS-1)], RANDOM_FLOAT(0.6, 0.8), ATTN_NORM, 0, PITCH_NORM);

		pev->flFallVelocity = 0.0f;*/
		}
	}
	else
	{
		if (pev->waterlevel == WATERLEVEL_HEAD)// XDM
		{
			pev->movetype = MOVETYPE_FLY;
			pev->velocity *= 0.9f;
			pev->avelocity *= 0.9f;
			pev->velocity.z += 8;
		}
		else if (pev->waterlevel == WATERLEVEL_FEET)
		{
			pev->velocity.z -= 8;
			//pev->movetype = MOVETYPE_BOUNCE;
		}
		SetNextThink(0.1f);
	}
}

//=========================================================
// BBoxIsFlat - check to see if the monster's bounding box
// is lying flat on a surface (traces from all four corners
// are same length.)
//=========================================================
BOOL CBaseMonster::BBoxFlat(void)
{
	TraceResult	tr;
	Vector		vecPoint;
	float		flXSize, flYSize;
	float		flLength;
	float		flLength2;

	flXSize = pev->size.x / 2;
	flYSize = pev->size.y / 2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	vecPoint.z = pev->origin.z;

	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength = (vecPoint - tr.vecEndPos).Length();

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y - flYSize;

	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if ( flLength2 > flLength )
		return FALSE;

	flLength = flLength2;

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if ( flLength2 > flLength )
		return FALSE;

	flLength = flLength2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y - flYSize;
	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if ( flLength2 > flLength )
		return FALSE;

	flLength = flLength2;
	return TRUE;
}

//=========================================================
// Get Enemy - tries to find the best suitable enemy for the monster.
//=========================================================
BOOL CBaseMonster::GetEnemy(void)
{
	if (HasConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS))
	{
		CBaseEntity *pNewEnemy = BestVisibleEnemy();
		if (pNewEnemy != m_hEnemy && pNewEnemy != NULL)
		{
			// DO NOT mess with the monster's m_hEnemy pointer unless the schedule the monster is currently running will be interrupted
			// by COND_NEW_ENEMY. This will eliminate the problem of monsters getting a new enemy while they are in a schedule that doesn't care,
			// and then not realizing it by the time they get to a schedule that does. I don't feel this is a good permanent fix.
			if (m_pSchedule)
			{
				if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
				{
					PushEnemy( m_hEnemy, m_vecEnemyLKP );
					SetConditions(bits_COND_NEW_ENEMY);
					m_hEnemy = pNewEnemy;
					m_vecEnemyLKP = pNewEnemy->pev->origin;
				}
				// if the new enemy has an owner, take that one as well
				if (pNewEnemy->m_hOwner.Get())// XDM3037: pev->owner != NULL)
				{
					CBaseEntity *pOwner = pNewEnemy->m_hOwner->MyMonsterPointer();// GetMonsterPointer( pNewEnemy->pev->owner );
					if (pOwner && pOwner->IsMonster() && IRelationship(pOwner) >= R_DL)//(pOwner->pev->flags & FL_MONSTER) && IRelationship( pOwner ) != R_NO )
						PushEnemy( pOwner, m_vecEnemyLKP );
				}
			}
		}
	}

	// remember old enemies
	if (m_hEnemy.Get() == NULL && PopEnemy())
	{
		if (m_pSchedule)
		{
			if (m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY)
				SetConditions(bits_COND_NEW_ENEMY);
		}
	}

	if (m_hEnemy.Get())// monster has an enemy
		return TRUE;

	return FALSE;// monster has no enemy
}

//-----------------------------------------------------------------------------
// Purpose: Dead monster drops named item
//-----------------------------------------------------------------------------
CBaseEntity *CBaseMonster::DropItem(const char *pszItemName, const Vector &vecPos, const Vector &vecAng)
{
	DBG_MON_PRINTF("%s[%d]::DropItem(\"%s\")\n", STRING(pev->classname), entindex(), pszItemName);
	if (pszItemName == NULL)
	{
		ALERT(at_console, "CBaseMonster(%d)::DropItem(NULL)!\n", entindex());
		return NULL;
	}

	CBaseEntity *pItem = Create(pszItemName, vecPos, vecAng, pev->velocity, NULL, SF_NOREFERENCE|SF_NORESPAWN);// XDM3038a: SF_NOREFERENCE and NULL owner! Otherwise this item won't be removed!
	if (pItem)
	{
		pItem->pev->velocity = pev->velocity;// XDM3038
		pItem->pev->velocity *= 0.75f;
		pItem->pev->avelocity.Set(0.0f, RANDOM_FLOAT(0,100), 0.0f);
		if (pItem->IsPlayerItem())
			pItem->pev->impulse = ITEM_STATE_DROPPED;

		return pItem;
	}
	else
	{
		ALERT(at_console, "CBaseMonster(%d)::DropItem(%s) - failed!\n", entindex(), pszItemName);
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035c: special
// Input  : targetname -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::HasTarget(string_t targetname)
{
	if (FStrEq(STRING(targetname), STRING(m_iszTriggerTarget)))
		return true;

	return CBaseToggle::HasTarget(targetname);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMonster::FrozenStart(float freezetime)
{
	DBG_MON_PRINTF("%s[%d]::FrozenStart(%g)\n", STRING(pev->classname), entindex(), freezetime);
	if (!IsAlive())
		return;

	if (!IsPlayer())
	{
		m_movementGoal = MOVEGOAL_NONE;
		SetConditions(bits_COND_HEAVY_DAMAGE);
		m_movementGoal = MOVEGOAL_NONE;// temporary?
		FRefreshRoute();// XDM3035
	}
	if (m_fFrozen)// already frozen? add time!
	{
		m_flUnfreezeTime += freezetime;
		m_flNextAttack += freezetime;
	}
	else
	{
		m_flUnfreezeTime = gpGlobals->time + freezetime;
		m_flNextAttack = gpGlobals->time + freezetime;
	}
	m_flBurnTime = 0.0f;
	ClearBits(m_bitsDamageType, DMGM_FIRE|DMG_IGNITE);// XDM3038a
	m_hTBDAttacker = NULL;// XDM3037
	m_fFrozen = TRUE;
	m_fFreezeEffect = TRUE;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor.Set(63,143,255);
	pev->renderamt = RANDOM_LONG(0,80);
	//ChangeSchedule(slFail);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Note: unfortunately, we cannot restore initial/previous rener parameters
//-----------------------------------------------------------------------------
void CBaseMonster::FrozenEnd(void)
{
	DBG_MON_PRINTF("%s[%d]::FrozenEnd()\n", STRING(pev->classname), entindex());
	m_fFrozen = FALSE;
	m_fFreezeEffect = FALSE;
	pev->renderfx = kRenderFxNone;
	pev->rendercolor.Set(255,255,255);
	m_flUnfreezeTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMonster::FrozenThink(void)
{
	if (!m_fFrozen)
		return;

	if (m_flUnfreezeTime > 0.0f)
	{
		if (m_flUnfreezeTime <= gpGlobals->time)
		{
			FrozenEnd();
		}
		else
		{
			pev->renderamt = (m_flUnfreezeTime - gpGlobals->time)*64.0f/100.0f;// XDM3035: UNDONE: since we don't store freeze start/interval assume 100 seconds as maximum
			//pev->renderamt = (1.0f + sinf(gpGlobals->time))*64.0f + 10.0f;
			/*if (pev->renderamt >= 80)
			{
				pev->renderamt --;
				m_fFreezeEffect = FALSE;
			}
			else if (pev->renderamt <= 4)
			{
				pev->renderamt ++;
				m_fFreezeEffect = TRUE;
			}
			else
			{
				if (m_fFreezeEffect)
					pev->renderamt ++;
				else
					pev->renderamt --;
			}*/
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::CanAttack(void)
{
	if (m_fFrozen)// XDM3035c
	{
		if (BloodColor() != DONT_BLEED)// TESTME!
			return false;
	}

	if (m_flNextAttack > gpGlobals->time)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: hack
// Input  : posSrc - ???
// Output : position vector to aim at
//-----------------------------------------------------------------------------
Vector CBaseMonster::BodyTarget(const Vector &posSrc)
{
	if (gSkillData.iSkillLevel == SKILL_EASY)
		return Center();// mosters will try to shoot at center
	else
		return Center() * 0.75 + EyePosition() * 0.25;// mosters will try to aim for the head (if they can find one :) )
}

//-----------------------------------------------------------------------------
// Purpose: Pretend we got the gun position
// Output : position vector
//-----------------------------------------------------------------------------
Vector CBaseMonster::GetGunPosition(void)
{
	UTIL_MakeVectors(pev->angles);
	// Vector vecSrc = pev->origin + gpGlobals->v_forward * 10;
	//vecSrc.z = pevShooter->absmin.z + pevShooter->size.z * 0.7;
	//vecSrc.z = pev->origin.z + (pev->view_ofs.z - 4);
	Vector vecSrc = gpGlobals->v_forward * m_HackedGunPos.y
					+ gpGlobals->v_right * m_HackedGunPos.x
					+ gpGlobals->v_up * m_HackedGunPos.z;

	vecSrc += pev->origin;
	return vecSrc;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDamage - 
//			bitsDamageType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::IsHeavyDamage(float flDamage, int bitsDamageType) const
{
	if (bitsDamageType & DMG_DROWNRECOVER)
		return false;

	//if (flDamage <= 0)
	//	return false;

	if ((flDamage / pev->max_health) >= HEAVY_DAMAGE_PERCENT)// XDM3038
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::IsHuman(void)// const :(
{
	switch (Classify())
	{
	case CLASS_PLAYER:
	case CLASS_HUMAN_PASSIVE:
	case CLASS_HUMAN_MILITARY:
	case CLASS_PLAYER_ALLY:
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Does this monster gave standard human gibs? (TODO: obsolete: use gib model)
// Note   : HasHumanGibs is not always !HasAlienGibs
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::HasHumanGibs(void)
{
	if (IsHuman())// XDM3038a
		return true;

	if (BloodColor() == BLOOD_COLOR_RED)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Does this monster gave standard alien gibs? (TODO: obsolete: use gib model)
// Note   : HasAlienGibs is not always !HasHumanGibs
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::HasAlienGibs(void)
{
	if (BloodColor() == BLOOD_COLOR_RED)// XDM3038: faster
		return false;
	if (BloodColor() == BLOOD_COLOR_YELLOW)// XDM3038: faster
		return true;

	int myClass = Classify();
	if (myClass == CLASS_ALIEN_MILITARY ||
		myClass == CLASS_ALIEN_MONSTER ||
		myClass == CLASS_ALIEN_PASSIVE ||
		myClass == CLASS_INSECT ||
		myClass == CLASS_ALIEN_PREDATOR ||
		myClass == CLASS_ALIEN_PREY)
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: for everybody to hear
//-----------------------------------------------------------------------------
void CBaseMonster::DeathSound(void)
{
//STFU?	EMIT_SOUND(edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
}

//-----------------------------------------------------------------------------
// Purpose: Create some gore and get rid of a monster's model
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::GibMonster(void)
{
	DBG_PRINT_ENT_THINK(GibMonster);
	TraceResult	tr;
	bool gibbed = false;

	// XDM: big bloody decal!
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0.0f,0.0f,pev->size.z), ignore_monsters, edict(), &tr);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);

	//XDM	conprintf(0, ">>> sz = %f\n", (pev->size.x + pev->size.y + pev->size.z) * 0.3);
	//BADBAD!	int m_iGibCount = limit(pev->max_health * 0.3, 1, 64);

	//if (m_fFrozen)// lame
	//	EMIT_SOUND_DYN(edict(), CHAN_VOICE, gBreakSoundsGlass[RANDOM_LONG(0,NUM_BREAK_SOUNDS-1)], VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96, 104));
	//else
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "common/bodysplat.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96, 104));// XDM3035: play through CHAN_VOICE to stop all talking

	if (g_pGameRules->FAllowEffects())
		UTIL_BloodDrips(Center(), UTIL_RandomBloodVector(), BloodColor(), 160*(1+sv_overdrive.value));// XDM3035a: more fun

	// XDM3038c: draw decals disregarding gib models
	if (HasHumanGibs() || m_bloodColor == BLOOD_COLOR_RED)
	{
		if (m_iGibCount > 4)
			UTIL_DecalTrace(&tr, DECAL_BLOODSMEARR1);// the biggest spot
		else
			UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BLOODSMEARR2, DECAL_BLOODSMEARR3));
	}
	else if (HasAlienGibs() || m_bloodColor == BLOOD_COLOR_YELLOW)
	{
		if (m_iGibCount > 4)
			UTIL_DecalTrace(&tr, DECAL_BLOODSMEARY1);// the biggest spot
		else
			UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BLOODSMEARY2, DECAL_BLOODSMEARY3));
	}
	else if (m_bloodColor == BLOOD_COLOR_GREEN)
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_GBLOOD1, DECAL_GBLOOD3));
	else if (m_bloodColor == BLOOD_COLOR_BLUE)
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BBLOOD1, DECAL_BBLOOD3));

	// only humans throw skulls !!!UNDONE - eventually monsters will have their own sets of gibs
	if (HasHumanGibs())
	{
		if (IsHuman() && !IsPlayer())// XDM3035c: players have their own special case
			CGib::SpawnHeadGib(this);

		if (m_fFrozen)// XDM3035
			CGib::SpawnModelGibs(this, pev->origin, pev->mins, pev->maxs, (g_vecAttackDir*4.0f + RandomVector()), 10, MODEL_INDEX("models/glassgibs.mdl"), m_iGibCount, GIB_LIFE_DEFAULT, matGlass, BLOOD_COLOR_RED, BREAK_GLASS);
		else
			CGib::SpawnRandomGibs(this, m_iGibCount, true, (g_vecAttackDir*4.0f + RandomVector()));	// throw some human gibs.

		gibbed = true;
	}
	else if (HasAlienGibs())
	{
		if (m_fFrozen)// XDM3035
			CGib::SpawnModelGibs(this, pev->origin, pev->mins, pev->maxs, (g_vecAttackDir*4.0f + RandomVector()), 10, MODEL_INDEX("models/glassgibs.mdl"), m_iGibCount, GIB_LIFE_DEFAULT, matGlass, BLOOD_COLOR_RED, BREAK_GLASS);
		else
			CGib::SpawnRandomGibs(this, m_iGibCount, false, (g_vecAttackDir*4.0f + RandomVector()));// Throw alien gibs

		gibbed = true;
	}
	else if (m_iGibModelIndex != 0)//!FStringNull(m_iszGibModel))
	{
		CGib::SpawnModelGibs(this, pev->origin, pev->mins, pev->maxs, g_vecAttackDir, 10, m_iGibModelIndex, m_iGibCount, GIB_LIFE_DEFAULT, (BloodColor() == DONT_BLEED)?matNone:matFlesh, BloodColor(), Classify()==CLASS_MACHINE?BREAK_METAL:0);
		gibbed = true;
	}

	if (!IsPlayer())// don't remove players!
	{
		if (gibbed)
		{
			SetThink(&CBaseEntity::SUB_Remove);
			SetNextThink(0);
		}
		else
			FadeMonster();
	}
	return gibbed;
}

//-----------------------------------------------------------------------------
// Purpose: Just fade out
//-----------------------------------------------------------------------------
void CBaseMonster::FadeMonster(void)
{
	DBG_PRINT_ENT_THINK(FadeMonster);
	StopAnimation();
	pev->movetype = MOVETYPE_NONE;
	pev->velocity.Clear();
	pev->avelocity.Clear();
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

//-----------------------------------------------------------------------------
// Purpose: Determines the best type of death animation to play.
// Output : Activity
//-----------------------------------------------------------------------------
Activity CBaseMonster::GetDeathActivity(void)
{
	if (pev->deadflag != DEAD_NO)
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	Vector vecSrc(Center());
	bool fTriedDirection = false;
	Activity deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	UTIL_MakeVectors(pev->angles);
	vec_t flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir);
	TraceResult	tr;

	switch (m_LastHitGroup)
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;
	case HITGROUP_CHEST:
		{
			if (pev->health < (-0.2f*pev->max_health)/*-20*/)// XDM3034
				deathActivity = ACT_DIEVIOLENT;
			else
				deathActivity = ACT_DIE_CHESTSHOT;
		}
		break;
	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	default:
	//case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if (flDot > 0.3)// shot from behind, fall forward
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if (flDot <= -0.3)
		{
			if (pev->health < (-0.15f*pev->max_health)/*-15*/ && (m_LastHitGroup == HITGROUP_GENERIC || m_LastHitGroup == HITGROUP_ARMOR))// XDM3034: ACT_DIEVIOLENT not for arms or legs
				deathActivity = ACT_DIEVIOLENT;
			else
				deathActivity = ACT_DIEBACKWARD;// don't use: ACT_DIE_BACKSHOT?
		}
		break;
	}

	// can we perform the prescribed death?
	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		// no! did we fail to perform a directional death?
		if (fTriedDirection)
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if (flDot > 0.3)
				deathActivity = ACT_DIEFORWARD;
			else if (flDot <= -0.3)
				deathActivity = ACT_DIEBACKWARD;
		}
	}

	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}
	else if (deathActivity == ACT_DIEFORWARD)
	{
		// make sure there's room to fall forward
		UTIL_TraceHull(vecSrc, vecSrc + gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);
		if (tr.flFraction != 1.0)
			deathActivity = ACT_DIESIMPLE;
	}
	else if (deathActivity == ACT_DIEBACKWARD)
	{
		// make sure there's room to fall backward
		UTIL_TraceHull(vecSrc, vecSrc - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);
		if (tr.flFraction != 1.0)
			deathActivity = ACT_DIESIMPLE;
	}
	return deathActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Determines the best type of flinch animation to play.
// Output : Activity
//-----------------------------------------------------------------------------
Activity CBaseMonster::GetSmallFlinchActivity(void)
{
	Activity flinchActivity;
	switch (m_LastHitGroup)
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}
	// do we have a sequence for the ideal activity?
	if (LookupActivity(flinchActivity) == ACTIVITY_NOT_AVAILABLE)
		flinchActivity = ACT_SMALL_FLINCH;

	return flinchActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMonster::BecomeDead(void)
{
	DBG_PRINT_ENT_THINK(BecomeDead);
	if (pev->takedamage == DAMAGE_AIM)// XDM3037: TESTME
		pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	SetBits(pev->flags, FL_FLOAT);// XDM3038c
	// give the corpse half of the monster's original maximum health.
	//pev->health = pev->max_health / 2;
	//pev->max_health = 5; // XDM3035: HELL NO! max_health now becomes a counter for how many blood decals the corpse can place.

	if (pev->movetype == MOVETYPE_WALK
		|| pev->movetype == MOVETYPE_STEP
		|| pev->movetype == MOVETYPE_TOSS)
	{
		//AlignToFloor();// XDM3038c: TODO: overload it because monsters require different approach because lying "dead" posiiton is parallel to the ground
		// make the corpse fly away from the attack vector
		pev->movetype = MOVETYPE_TOSS;
		//pev->flags &= ~FL_ONGROUND;
		//pev->origin.z += 2;
		//pev->velocity = g_vecAttackDir;
		//pev->velocity *= RANDOM_FLOAT(300, 400);

		//nope, this fails the animation and other stuff	SetThink(&CBaseMonster::CorpseFallThink);// XDM3035a
	}

	//if (ShouldRespawn())// XDM3035
	//No! This disables AI and animations!	StartRespawn();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038b: now this has oncrete meaning: a corpse cannot stay, IF IT WAS NOT GIBBED.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::ShouldFadeOnDeath(void) const
{
	// if flagged to fade out or I have an owner (I came from a monster spawner)
	if (FBitSet(pev->spawnflags, SF_MONSTER_FADECORPSE))// XDM3038b: nope || m_hOwner.Get())// XDM3037: !FNullEnt( pev->owner ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Decide if this particular monster should be gibbed
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::ShouldGibMonster(int iGib) const
{
	// NO	if (ShouldFadeOnDeath())// XDM3038b
	// NO		return false;

	if (IsMultiplayer() && mp_instagib.value > 0.0f)// XDM
		return true;

	if (iGib == GIB_ALWAYS || (iGib == GIB_NORMAL && pev->health < (-pev->max_health*GIB_HEALTH_PERCENT_GIB)))
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: To not to call GibMonster directly
//-----------------------------------------------------------------------------
void CBaseMonster::CallGibMonster(void)
{
	DBG_PRINT_ENT_THINK(CallGibMonster);
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;// do something with the body. while monster blows up

	//EMIT_SOUND(edict(), CHAN_BODY, "common/null.wav", VOL_NORM, ATTN_NORM);// XDM
	EMIT_SOUND(edict(), CHAN_VOICE, "common/null.wav", VOL_NORM, ATTN_NORM);// XDM3038
	pev->effects = EF_NOINTERP|EF_NODRAW; // make the model invisible.
	GibMonster();

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if (pev->health < -99)
		pev->health = 0;

	if (ShouldFadeOnDeath())// && !fade)// this monster should've faded but is somehow gibbed
		Destroy();// XDM3038c: UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: in which cases this monster should respawn
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMonster::ShouldRespawn(void) const
{
	if (g_pGameRules)
	{
		if (!IsMonster())
			return false;
		if (IsProjectile())
			return false;
		if (IsPlayer())
			return false;
		if (FBitSet(pev->spawnflags, SF_NORESPAWN))
			return false;
		if (g_pGameRules->IsGameOver())// XDM3035c
			return false;

		//int c = Classify();// BAD! HACK!
		//if (c != CLASS_NONE && c != CLASS_PLAYER)// && c <= CLASS_PLAYER_ALLY XDM3035b: corpses has CLASS_GIB!
		{
			if (g_pGameRules->FMonsterCanRespawn(this))
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: After this monster is really dead, init respawn, fade, etc.
// Warning: Must be safe to call twice!
// Note   : It is the only legit place to call StartRespawn() from other than the usual remove function because monster's body may never be removed.
//-----------------------------------------------------------------------------
void CBaseMonster::RunPostDeath(void)
{
	DBG_MON_PRINTF("%s[%d] \"%s\": RunPostDeath()\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
	// check? m_fSequenceFinished, pev->framerate
	if (ShouldRespawn())
		StartRespawn();
	else if (!FBitSet(pev->effects, EF_NODRAW))// a permanent removal // XDM3038c: nodraw == no physical corpse
	{
		if (ShouldFadeOnDeath())// this monster was probably created by a monstermaker... fade the corpse out.
			SUB_StartFadeOut();
		else// body is gonna be around for a while, so have it stink for a bit.
			CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 384, 30);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called if health is <= 0. Every time monster takes damage.
// Input  : *pInflictor - 
//			*pAttacker - 
//			iGib - GIB_NORMAL
//-----------------------------------------------------------------------------
void CBaseMonster::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	//DBG_PRINT_ENT_THINK(Killed);
	//DBG_MON_PRINTF("CBaseMonster %s[%d] \"%s\"::Killed(%d %s, %d %s, %d)\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"", pAttacker?pAttacker->entindex():0, pAttacker?STRING(pAttacker->pev->classname):"", iGib);
	SetBits(m_iGameFlags, EGF_DIED);// XDM3038c
	if (!HasMemory(bits_MEMORY_KILLED))// XDM3037: was alive
	{
		if (pev->deadflag != DEAD_DEAD && g_pGameRules)// XDM3037: just gibbed already dead monster
			g_pGameRules->MonsterKilled(this, pAttacker, pInflictor);

		// clear the deceased's sound channels.(may have been firing or reloading when killed)
		EMIT_SOUND(edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);

		m_IdealMonsterState = MONSTERSTATE_DEAD;

		// Make sure this condition is fired too (TakeDamage breaks out before this happens on death)
		SetConditions(bits_COND_LIGHT_DAMAGE);
		Remember(bits_MEMORY_KILLED);

		CSoundEnt::InsertSound(bits_SOUND_DEATH, pev->origin, (iGib == GIB_ALWAYS)?320:256, (iGib == GIB_ALWAYS)?2:1);// XDM3037

		// tell owner (if any) that we're dead. This is mostly for MonsterMaker functionality.
		if (m_hOwner)// XDM3037 //if (!FNullEnt(pev->owner))
		{
			//CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
			//if (pOwner)
				m_hOwner->DeathNotice(this);
		}
	}

	// XDM3038: drop something
	if (!FBitSet(m_iGameFlags, EGF_DROPPEDITEMS) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_ITEMS))//HasMemory(bits_MEMORY_DROPPED_ITEMS))
	{
		SetBits(m_iGameFlags, EGF_DROPPEDITEMS);// XDM3038c
		//Remember(bits_MEMORY_DROPPED_ITEMS);// XDM3038a
		const char *pDropItemName = GetDropItemName();
		if (pDropItemName && *pDropItemName)
		{
			Vector vecGunPos, vecGunAngles;
			if (pev->deadflag == DEAD_DEAD)// monster_%s_dead, who is initially dead, can drop stuff too, make sure it does not fall through the floor
			{
				vecGunAngles = pev->angles;
				vecGunPos = pev->origin;
				if (FBitSet(pev->flags, FL_ONGROUND))
					vecGunPos.z += 2.0f;
			}
			else
				GetAttachment(0, vecGunPos, vecGunAngles);

			DropItem(pDropItemName, vecGunPos, vecGunAngles);
		}
	}

	// Decide what to do with the body. Body? What body?
	if (iGib == GIB_DISINTEGRATE)
	{
		if (pev->deadflag != DEAD_DEAD)// XDM3037
		{
			pev->deadflag = DEAD_DEAD;
			FCheckAITrigger();// XDM3035c: may need to fire some conditional triggers
		}
		Disintegrate();
		//return;
	}
	else if (iGib == GIB_FADE)// XDM3037
	{
		if (pev->deadflag != DEAD_DEAD)
		{
			pev->deadflag = DEAD_DEAD;
			FCheckAITrigger();
		}
		if (!IsPlayer())
		{
			FadeMonster();
			//return;
		}
	}
	else if (iGib == GIB_REMOVE)// XDM3037
	{
		if (pev->deadflag != DEAD_DEAD)
		{
			pev->deadflag = DEAD_DEAD;
			FCheckAITrigger();
		}
		//pev->effects |= EF_NODRAW;
		if (!IsPlayer())
		{
			Destroy();// XDM3038c: UTIL_Remove(this);
			//return;
		}
	}
	else if (ShouldGibMonster(iGib))
	{
		if (pAttacker && pAttacker->IsPlayer())
			((CBasePlayer *)pAttacker)->m_Stats[STAT_GIB_COUNT]++;// XDM3038c

		CallGibMonster();
		//return;
	}
	else if (FBitSet(pev->flags, FL_MONSTER) && pev->deadflag != DEAD_DEAD)// why check FL_MONSTER?
	{
		SetTouchNull();
		BecomeDead();
		//pev->deadflag = DEAD_DYING;// XDM3035 no! :(
		// this will interrupt death animation!		if (ShouldFadeOnDeath())
		//			SUB_StartFadeOut();
	}
	// DO NOT PLACE CODE HERE
}

//-----------------------------------------------------------------------------
// Purpose: Give some health to this entity
// Input  : &flHealth - 
//			&bitsDamageType - 
// Output : float
//-----------------------------------------------------------------------------
float CBaseMonster::TakeHealth(const float &flHealth, const int &bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)
		return 0;

	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any time-based damage
	m_bitsDamageType &= ~(bitsDamageType & ~DMGM_TIMEBASED);
	return CBaseEntity::TakeHealth(flHealth, bitsDamageType);
}

//-----------------------------------------------------------------------------
// Purpose: This should be the only function that ever reduces health.
// Input  : *pInflictor - a weapon, projectile, trigger, etc.
//			*pAttacker - player, monster, or anything else owning the gun
//			flDamage - amount of damage
//			bitsDamageType - indicates the type of damage sustained, ie: DMG_SHOCK
// Output : int - 0 means no damage was taken
//-----------------------------------------------------------------------------
int CBaseMonster::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	DBG_PRINT_ENT_TAKEDAMAGE;
	if (g_pGameRules->IsGameOver() && !IsPlayer())// XDM: players have g_pGameRules->FPlayerCanTakeDamage
		return 0;
	if (IsRemoving())
	{
		//DBG_FORCEBREAK
		return 0;
	}
	bool bDoDamage;
	if (pev->takedamage == DAMAGE_NO)
		bDoDamage = false;// return 0;// XDM3038a
	else if (FBitSet(pev->flags, FL_GODMODE) || (IsMonster() && FBitSet(pev->spawnflags, SF_MONSTER_INVULNERABLE)))// XDM3038a: now for everyone
		bDoDamage = false;
	else if (IsPlayer())
		bDoDamage = g_pGameRules->FPlayerCanTakeDamage((CBasePlayer *)this, pAttacker);// XDM3038a
	else
		bDoDamage = true;

	float flTake = flDamage;
	if (bDoDamage)
	{
		if (FBitSet(bitsDamageType, DMG_NOHITPOINT))
			m_LastHitGroup = HITGROUP_GENERIC;// XDM3037

		//if (bitsDamageType & DMG_BURN)// XDM
		//	m_flBurnDamage = flDamage;

		//conprintf(0, "CBaseMonster(%s[%d])::TakeDamage(%g), pev->deadflag %d, health %g of %g\n", STRING(pev->classname), entindex(), flDamage, pev->deadflag, pev->health, pev->max_health);

	//	if (pev->deadflag != DEAD_NO)// !IsAlive()) // XDM3035: DEAD_DYING too!
	// XDM3037		return DeadTakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);

		/*if (FBitSet(bitsDamageType, DMG_BURN))// XDM3037: show effect before armor cuts the damage value
		{
			if (flDamage >= 2.0f && g_pGameRules->FAllowEffects() && sv_flame_effect.value > 1)
				PLAYBACK_EVENT_FULL(/*FEV_RELIABLE* /FEV_UPDATE, edict(), g_usFlame, 0.0f, pev->origin.As3f(), pev->velocity.As3f()/*tr.vecPlaneNormal* /, 2.0f, 0.0f, g_iModelIndexFlameFire2, g_iModelIndexSmoke, RANDOM_LONG(0,1), 0);
		}*/

		//Vector vecDir;
		//flTake = flDamage;

		// XDM3037: armor now works same for everyone
		if (pev->armorvalue > 0 && !FBitSet(bitsDamageType, DMG_FALL | DMG_DROWN))// armor doesn't protect against fall or drown damage!
		{
			if (FBitSet(bitsDamageType, DMG_IGNOREARMOR))// XDM3038c: DMG_IGNOREARMOR now also drains suit's power (but still no protection)
				pev->armorvalue -= min(flTake, pev->armorvalue);
			else
			{
				float flArmorTake;// XDM3038
				if (FBitSet(bitsDamageType, DMG_BULLET))
					flArmorTake = flTake * ARMOR_TAKE_BULLET;
				else if (FBitSet(bitsDamageType, DMG_CLUB))// XDM3037: added 'else' so specifying multiple damage types won't result in dramatic damage reduction
					flArmorTake = flTake * ARMOR_TAKE_CLUB;
				else
					flArmorTake = flTake * ARMOR_TAKE_GENERIC;// * 1.0?!

				// Does this use more armor than we have?
				pev->armorvalue -= flArmorTake;
				flTake -= flArmorTake;

				if (pev->armorvalue < 0.0f)// negative means there was no enough armor
				{
					flTake -= pev->armorvalue;// add unreflected damage back to health damage
					pev->armorvalue = 0.0f;
				}
				//conprintf(1, "CBaseMonster::TakeDamage() flArmorTake = %g\n", flArmorTake);
			}
		}
	}

	//conprintf(1, "CBaseMonster::TakeDamage(initial %g, take %g)\n", flDamage, flTake);

	// set damage type sustained
	m_bitsDamageType |= bitsDamageType;

	// Calculate vector of the incoming attack.
	if (g_vecAttackDir.IsZero())// XDM3038a: HACK. This is like (pInflictor->Center() - this->Center()).Normalize();
	{
		if (pInflictor){// an actual missile was involved.
			g_vecAttackDir = Center(); g_vecAttackDir -= pInflictor->Center(); g_vecAttackDir.NormalizeSelf();// XDM3038c: reversed
		}
		else if (pAttacker){
			g_vecAttackDir = Center(); g_vecAttackDir -= pAttacker->Center(); g_vecAttackDir.NormalizeSelf();// XDM3038c: reversed
		}
	}

	if (FBitSet(bitsDamageType, DMG_IGNITE))// XDM3037: WARNING! only react to DMG_IGNITE MODIFIER to prevent recursion!! See CBaseEntity::CheckEnvironment()
	{
		if (pev->waterlevel < WATERLEVEL_WAIST)// XDM3035a: may be standing in water
		{
			if (m_flBurnTime == 0.0f)// XDM3037: was not burning, start.
			{
				// cl EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/flame_burn.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95, 105));
				m_flBurnTime = gpGlobals->time;
				m_hTBDAttacker = pAttacker;// XDM3037
			}
			// burntime = damage * burnrate
			m_flBurnTime += flDamage/TD_SLOWBURN_DAMAGE;// XDM3037: continue burning for N seconds
		}
		if (m_fFrozen)
			FrozenEnd();

		if (!IsPlayer())
		{
			ClearConditions(bits_COND_SEE_ENEMY);
			SetConditions(bits_COND_HEAVY_DAMAGE);
		}
		//ChangeSchedule(slTakeCoverFromOrigin);
	}

	if (FBitSet(bitsDamageType, DMGM_DISARM))// XDM3038a: 20150713
		m_flNextAttack += clamp(0.01f*flDamage, 0, 5);

	if (bDoDamage)// XDM3038a
	{
		if (pInflictor)// XDM: why only players?
			pev->dmg_inflictor = pInflictor->edict();
		else
			pev->dmg_inflictor = NULL;

		// add to the damage total for clients, which will be sent as a single message at the end of the frame
		// TODO: remove after combining shotgun blasts?
		if (IsPlayer())
			pev->dmg_take += flTake;
	}

	// if this is a player, move him around! XDM3035: not just player
	// WARNING! RadiusDamage() already added some velocity!!!
	if (FBitSet(bitsDamageType, DMGM_PUSH) && IsPushable() && !g_vecAttackDir.IsZero() && pInflictor && (pInflictor->pev->solid != SOLID_TRIGGER))
	{
		if (pev->movetype != MOVETYPE_NONE && pev->movetype != MOVETYPE_NOCLIP && pev->movetype != MOVETYPE_FOLLOW)
		{
			float flForce = DamageForce(flDamage);
			pev->velocity += g_vecAttackDir * flForce;// XDM3038c: reversed. Warning: should be normalized!
			if (FBitSet(pev->flags, FL_ONGROUND) && pev->solid != SOLID_TRIGGER && pev->solid != SOLID_BSP && pev->movetype != MOVETYPE_PUSHSTEP)
			{
				pev->origin.z += 1.0f;// hack
				ClearBits(pev->flags, FL_ONGROUND);// hack
			}
		}
	}

	// check for godmode or invincibility AFTER pushing
	if (!bDoDamage)//if (FBitSet(pev->flags, FL_GODMODE) || (IsMonster() && FBitSet(pev->spawnflags, SF_MONSTER_INVULNERABLE)))// XDM3038a: now for everyone
		flTake = 0;// XDM: don't return. Monsters must react to the damage.

	// do the damage
	pev->health -= flTake;
	if (IsPlayer() && pev->health < 1 && pev->health > 0)// XDM3037: HACK: fix 0.5 health bug
		pev->health = 0;

	// HACKHACK Don't kill monsters in a script. Let them break their scripts first!
	if (m_MonsterState == MONSTERSTATE_SCRIPT)
	{
		SetConditions(bits_COND_LIGHT_DAMAGE);
		return 0;
	}

	if (pAttacker)// XDM3038c: not sure if it will work
	{
		if (pAttacker->IsMonster() || pAttacker->IsPlayer())
			SetConditions(bits_COND_PROVOKED);
	}

	// death variants. warning: multiple modifiers may be specified at once!
	if (pev->health <= 0)
	{
		if (FBitSet(bitsDamageType, DMG_DISINTEGRATING))// XDM3035
		{
			Killed(pInflictor, pAttacker, GIB_DISINTEGRATE);
		}
		else if (FBitSet(bitsDamageType, DMG_VAPOURIZING))// XDM3034
		{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_SMOKE);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z);
				WRITE_SHORT(g_iModelIndexSmoke);
				WRITE_BYTE(40); // scale * 10
				WRITE_BYTE(16); // framerate
			MESSAGE_END(); // this is done in client event
			Killed(pInflictor, pAttacker, GIB_REMOVE);
		}
		else if (FBitSet(bitsDamageType, DMG_ALWAYSGIB) || (IsMultiplayer() && mp_instagib.value > 0.0))// XDM: instagib mode
		{
			Killed(pInflictor, pAttacker, GIB_ALWAYS);
		}
		else if (FBitSet(bitsDamageType, DMG_NEVERGIB))
		{
			Killed(pInflictor, pAttacker, GIB_NEVER);
		}
		else
		{
			int iGib = GIB_NEVER;// XDM3037a: if a bullet hits REALLY hard... :)
			if (FBitSet(bitsDamageType, DMGM_GIB_CORPSE) && (pev->health < (-GIB_HEALTH_PERCENT_GIB*pev->max_health)))
				iGib = GIB_ALWAYS;
			else if (/* XDM3038 !HasMemory(bits_MEMORY_KILLED) && */FBitSet(bitsDamageType, DMGM_BREAK) && (flTake >= (GIB_HEALTH_PERCENT_BREAK*pev->max_health)))// compare flTake, not pev->health - we need critical damage in a single strike, not accumulated!
				iGib = GIB_NORMAL;// only if alive

			Killed(pInflictor, pAttacker, iGib);// XDM3037a: acid or gas should never gib bodies
		}
	}
	else
	{
		if (pev->deadflag == DEAD_NO)// no pain sound during death animation.
		{
			if (IsHuman() && FBitSet(bitsDamageType, DMG_DROWN))// XDM: this is called once per second
			{
				switch (RANDOM_LONG(0,3))
				{
					case 0:	EMIT_SOUND(edict(), CHAN_VOICE, "player/pl_swim1.wav", 0.8, ATTN_STATIC); break;
					case 1:	EMIT_SOUND(edict(), CHAN_VOICE, "player/pl_swim2.wav", 0.8, ATTN_STATIC); break;
					case 2:	EMIT_SOUND(edict(), CHAN_VOICE, "player/pl_swim3.wav", 0.8, ATTN_STATIC); break;
					case 3:	EMIT_SOUND(edict(), CHAN_VOICE, "player/pl_swim4.wav", 0.8, ATTN_STATIC); break;
				}
				if (pev->waterlevel >= WATERLEVEL_WAIST)
				{
					//vecDir = EyePosition();// tmp
					FX_BubblesBox(EyePosition(), Vector(2,2,2), 20);//UTIL_Bubbles(vecDir - Vector(2,2,2), vecDir + Vector(2,2,2), 20);
				}
			}
			if (flTake > 0)// XDM3037a
				PainSound();// "Ouch!"
		}

		//MESSAGE_BEGIN(MSG_PVS, gmsgDamageFx, pev->origin);// XDM3035c

		// react to the damage (get mad)
		if (pAttacker && IsMonster() && IsAlive() && !FBitSet(pAttacker->pev->flags, FL_NOTARGET))// XDM3037
		{
			if (pAttacker->IsMonster() || pAttacker->IsPlayer())// only if the attack was a monster or client!
			{
				if (pAttacker != m_hOwner && pAttacker != m_hEnemy && IRelationship(pAttacker) != R_AL)// XDM3038c: get angry at the attacker
				{
					PushEnemy(m_hEnemy, m_vecEnemyLKP);// remember current enemy
					SetConditions(bits_COND_NEW_ENEMY);
					m_hEnemy = pAttacker;
					m_vecEnemyLKP = pAttacker->pev->origin;
				}
				else if (pInflictor)// enemy's last known position is somewhere down the vector that the attack came from.
				{
					if (m_hEnemy.Get() == NULL || pInflictor == m_hEnemy || !HasConditions(bits_COND_SEE_ENEMY))
						m_vecEnemyLKP = pInflictor->pev->origin;
				}
				else
					m_vecEnemyLKP = pev->origin - (g_vecAttackDir * 64.0f);// XDM3038c: reversed

				MakeIdealYaw(m_vecEnemyLKP);

				if (IsHeavyDamage(flDamage, bitsDamageType))// XDM
					SetConditions(bits_COND_HEAVY_DAMAGE);
				else
					SetConditions(bits_COND_LIGHT_DAMAGE);
			}
		}
	}
	if (!IsRemoving() && FBitSet(bitsDamageType, DMGM_VISIBLE) && !FBitSet(pev->effects, EF_NODRAW) && g_pGameRules->FAllowEffects() && (flTake > 2.0f))
	{
		MESSAGE_BEGIN(MSG_PVS, gmsgDamageFx, pev->origin);// XDM3035c
			WRITE_SHORT(entindex());
			WRITE_LONG(bitsDamageType);
			WRITE_COORD(flDamage);
		MESSAGE_END();
	}
	return (int)flTake;// XDM3037: was 1;
}

//-----------------------------------------------------------------------------
// Purpose: Trace-based damage detection. DOES NOT DECREASE HEALTH, just logic
// Input  : *pAttacker - player, monster, or anything else owning the gun
//			flDamage - amount of damage
//			&vecDir - direction at which the attacker was aiming
//			*ptr - trace that hit this entity
//			bitsDamageType - indicates the type of damage sustained, ie: DMG_SHOCK
//-----------------------------------------------------------------------------
void CBaseMonster::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
/*	TODO:
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
  Right now punchangle works fine.
*/
	//conprintf(0, "%s[%d]::TraceAttack(%g, tr.hg %d, bits %d)\n", STRING(pev->classname), entindex(), flDamage, ptr->iHitgroup, bitsDamageType);
	if (pev->takedamage == DAMAGE_NO)
		return;

	if (FBitSet(bitsDamageType, DMG_NOHITPOINT))// XDM3037: this affects GetDeathActivity() too
		m_LastHitGroup = HITGROUP_GENERIC;// TODO: save HITGROUP_ARMOR for TakeDamage() so armorvalue algorithm can take it into consideration
	else
		m_LastHitGroup = ptr->iHitgroup;

	if (!IsPlayer())// XDM3038c: OOP: player does his own calculations
	{
		switch (ptr->iHitgroup)
		{
		//case HITGROUP_GENERIC:
		//	break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.monHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.monChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.monStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.monArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.monLeg;
			break;
		case HITGROUP_ARMOR:
			{
				if (!FBitSet(bitsDamageType, DMG_IGNOREARMOR))
				{
					if (pev->armorvalue > 0)
						flDamage *= 0.5;// XDM3037: HACK. Anyway, it's just for monsters
				}
			}
			break;
		}
	}
	//conprintf(0, "appdmg %f\n", flDamage);

	if (ptr->iHitgroup == HITGROUP_ARMOR && FBitSet(bitsDamageType, DMGM_RICOCHET) && !FBitSet(bitsDamageType, DMG_IGNOREARMOR|DMG_NOHITPOINT))// XDM3035
	{
		if (g_pGameRules->FAllowEffects())
			UTIL_Ricochet(ptr->vecEndPos, min(0.5f,flDamage));
	}
	else if (BloodColor() != DONT_BLEED && !FBitSet(bitsDamageType, DMG_DONT_BLEED) && FBitSet(bitsDamageType, DMGM_BLEED))// XDM3038c: mask
	{
		UTIL_BloodDrips(ptr->vecEndPos, -vecDir, BloodColor(), flDamage*(1+sv_overdrive.value));// a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		//	if (ptr->iHitgroup == HITGROUP_HEAD && (bitsDamageType & (DMG_BULLET|DMG_CLUB)) && (pev->health <= 0))// XDM3038: TODO?
		//ugly		UTIL_BloodStream(ptr->vecEndPos, -vecDir, BloodColor(), cCount + (int)(flDamage*0.5f));
	}
	CBaseToggle::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);//AddMultiDamage(pAttacker, this, flDamage, bitsDamageType);
}

//-----------------------------------------------------------------------------
// CheckTraceHullAttack - expects a length to trace, amount of damage to do, and damage type.
// Returns a pointer to the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.
//-----------------------------------------------------------------------------
CBaseEntity *CBaseMonster::CheckTraceHullAttack(const float &flDist, const float &fDamage, const int &iDmgType)
{
	TraceResult tr;
	if (IsPlayer())
		UTIL_MakeVectors(pev->angles);
	else
		UTIL_MakeAimVectors(pev->angles);

	Vector vecStart(pev->origin);
	vecStart.z += pev->size.z * 0.5f;
	Vector vecEnd(vecStart);
	vecEnd += gpGlobals->v_forward * flDist;
	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, edict(), &tr);
	if (tr.pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (fDamage > 0.0f)
			pEntity->TakeDamage(this, GetDamageAttacker(), fDamage, iDmgType);// XDM3038c: added GetDamageAttacker()

		return pEntity;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall.
//-----------------------------------------------------------------------------
bool CBaseMonster::FInViewCone(CBaseEntity *pEntity)
{
	return FInViewCone(pEntity->EyePosition());//pev->origin);// XDM3038c
}

//-----------------------------------------------------------------------------
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall.
//-----------------------------------------------------------------------------
bool CBaseMonster::FInViewCone(const Vector &origin)
{
	UTIL_MakeVectors(pev->angles);
	Vector2D vec2LOS = (origin - pev->origin).Make2D();
	vec2LOS.NormalizeSelf();
	vec_t flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());
	if (flDot > m_flFieldOfView)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBaseMonster::ReportState(int printlevel)
{
	CBaseToggle::ReportState(printlevel);

	if ((int)m_MonsterState < ARRAYSIZE(g_szMonsterStateNames))
		conprintf(printlevel, "MonsterState: %d %s", m_MonsterState, g_szMonsterStateNames[m_MonsterState]);

	int i = ActivityMapFind(m_Activity);
	if (activity_map[i].type == (int)m_Activity)
		conprintf(printlevel, ", Activity: %d %s", m_Activity, activity_map[i].name);

	if (m_pSchedule)
	{
		conprintf(printlevel, ", Schedule: %s", m_pSchedule->pName?m_pSchedule->pName:"Unknown");
		Task_t *pTask = GetTask();
		if (pTask)
			conprintf(printlevel, ", Task: %d (#%d)", pTask->iTask, m_iScheduleIndex);
	}
	else
		conprintf(printlevel, ", No Schedule");

	conprintf(printlevel, ", Enemy: %s[%d]", m_hEnemy.Get()?STRING(m_hEnemy->pev->classname):"", m_hEnemy.Get()?m_hEnemy->entindex():0);

	if (IsMoving())
	{
		conprintf(printlevel, ", Moving");
		if (m_flMoveWaitFinished > gpGlobals->time)
			conprintf(printlevel, ": Stopped for %.2f.", m_flMoveWaitFinished - gpGlobals->time);
		else if (m_IdealActivity == GetStoppedActivity())
			conprintf(printlevel, ": In stopped anim.");
	}
	conprintf(printlevel, ", Yaw speed: %g", pev->yaw_speed);

	if (FBitSet(pev->spawnflags, SF_MONSTER_PRISONER))
		conprintf(printlevel, ", prisoner");
	if (FBitSet(pev->spawnflags, SF_MONSTER_PREDISASTER))
		conprintf(printlevel, ", pre-disaster");

	conprintf(printlevel, "\n");

#if defined (_DEBUG)
	if (g_pCvarDeveloper && g_pCvarDeveloper->value >= 2.0)
		DrawRoute(pev, m_Route, m_iRouteIndex, 0, 95, 255);
#endif
}



/* HACK
Uncomment if needed

#define SF_MONSTERTARGET_OFF 1

class CMonsterTarget : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Classify(void) { return (int)pev->frags; }
	virtual STATE GetState(void) { return (pev->health > 0)?STATE_ON:STATE_OFF; }
};

LINK_ENTITY_TO_CLASS( monster_target, CMonsterTarget );

void CMonsterTarget::Spawn(void)
{
	if (FBitSet(pev->spawnflags, SF_MONSTERTARGET_OFF))
		pev->health = 0;
	else
		pev->health = 1; // Don't ignore me, I'm not dead. I'm quite well really. I think I'll go for a walk...

	SetBits(pev->flags, FL_MONSTER);
}

void CMonsterTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, GetState() != STATE_OFF))// XDM3037: cast from STATE to bool
	{
		if (pev->health)
			pev->health = 0;
		else
			pev->health = 1;
	}
}*/
