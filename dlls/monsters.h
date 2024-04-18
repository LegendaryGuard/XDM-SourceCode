/***
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
#ifndef MONSTERS_H
#define MONSTERS_H

#include "skill.h"
#include "basemonster.h"
#include "pm_shared.h"// XDM3035c: VEC_HULL_*
/*

===== monsters.h ========================================================

  Header file for monster-related utility code

*/

// CHECKLOCALMOVE result types
#define	LOCALMOVE_INVALID					0 // move is not possible
#define LOCALMOVE_INVALID_DONT_TRIANGULATE	1 // move is not possible, don't try to triangulate
#define LOCALMOVE_VALID						2 // move is possible

// Monster Spawnflags
#define	SF_MONSTER_WAIT_TILL_SEEN		(1 << 0)// spawnflag that makes monsters wait until player can see them before attacking.
#define	SF_MONSTER_GAG					(1 << 1)// no idle noises from this monster
#define SF_MONSTER_HITMONSTERCLIP		(1 << 2)
#define SF_MONSTER_DONT_DROP_ITEMS		(1 << 3)// XDM3038c: don't drop any items when killed
#define SF_MONSTER_PRISONER				(1 << 4)// monster won't attack anyone, no one will attacke him.
// SF_SQUADMONSTER_LEADER flag here		(1 << 5)// do not use this slot
#define SF_MONSTER_WAIT_UNTIL_PROVOKED	(1 << 6)// don't attack the player unless provoked
#define	SF_MONSTER_WAIT_FOR_SCRIPT		(1 << 7)// makes monsters wait to check for attacking until the script is done or they've been attacked
#define SF_MONSTER_PREDISASTER			(1 << 8)// this is a predisaster scientist or barney. Influences how they speak.
#define SF_MONSTER_FADECORPSE			(1 << 9)// fade out corpse after death
#define SF_MONSTER_INVULNERABLE			(1 << 10)// takes no damage
#define SF_MONSTER_FALL_TO_GROUND		(1 << 31)// 0x80000000

// specialty spawnflags
#define SF_MONSTER_TURRET_AUTOACTIVATE	(1 << 5)// 32
#define SF_MONSTER_TURRET_STARTINACTIVE	(1 << 6)// 64

// spawn flags 256 and above are already taken by the engine

// MoveToOrigin stuff
#define MOVE_START_TURN_DIST			64 // when this far away from moveGoal, start turning to face next goal
#define MOVE_STUCK_DIST					32 // if a monster can't step this far, it is stuck.

// MoveToOrigin stuff
#define MOVE_NORMAL						0// normal move in the direction monster is facing
#define MOVE_STRAFE						1// moves in direction specified, no matter which way monster is facing

Vector VecCheckToss(entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj = 1.0f);
Vector VecCheckThrow(entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0f);
Vector UTIL_VecEnemyPrediction(const Vector &vecSrc, const Vector &vecEnemyPos, const Vector &vecEnemyVel, float flProjectileSpeed);

// these bits represent the monster's memory
#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// monster knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
#define bits_MEMORY_PATH_FINISHED		( 1 << 3 )// Finished monster path (just used by big momma for now)
#define bits_MEMORY_ON_PATH				( 1 << 4 )// Moving on a path
#define bits_MEMORY_MOVE_FAILED			( 1 << 5 )// Movement has already failed
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
#define bits_MEMORY_KILLED				( 1 << 7 )// HACKHACK -- remember that I've already called my Killed()
//#define bits_MEMORY_DROPPED_ITEMS		( 1 << 8 )// superceded by game flags. items were dropped, prevent dropping multiple times
//#define bits_MEMORY_SEEN_PLAYER			( 1 << 9 )// XDM3038c: seen player, can activate AI, etc.
#define bits_MEMORY_CUSTOM4				( 1 << 28 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 29 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 30 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 31 )	// Monster-specific memory

// trigger conditions for scripted AI
// these MUST match the CHOICES interface in halflife.fgd for the base monster
enum aitriggers_e
{
	AITRIGGER_NONE = 0,
	AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER,
	AITRIGGER_TAKEDAMAGE,
	AITRIGGER_HALFHEALTH,
	AITRIGGER_DEATH,
	AITRIGGER_SQUADMEMBERDIE,
	AITRIGGER_SQUADLEADERDIE,
	AITRIGGER_HEARWORLD,
	AITRIGGER_HEARPLAYER,
	AITRIGGER_HEARCOMBAT,
	AITRIGGER_SEEPLAYER_UNCONDITIONAL,
	AITRIGGER_SEEPLAYER_NOT_IN_COMBAT,
};
/*
		0 : "No Trigger"
		1 : "See Player"
		2 : "Take Damage"
		3 : "50% Health Remaining"
		4 : "Death"
		5 : "Squad Member Dead"
		6 : "Squad Leader Dead"
		7 : "Hear World"
		8 : "Hear Player"
		9 : "Hear Combat"
*/

#define GIB_COUNT_HUMAN			10// XDM3038c: lowered to avoid overflows
#define GIB_LIFE_DEFAULT		4
#define SF_GIB_SOLID			(1 << 0)
#define SF_GIB_RANDOMBODY		(1 << 1)
#define SF_GIB_RANDOMSKIN		(1 << 2)
#define SF_GIB_DROPTOFLOOR		(1 << 3)

//
// A gib is a chunk of a body, or a piece of wood/metal/rocks/etc.
//
class CGib : public CBaseEntity// TODO: CBaseAnimating?
{
public:
	virtual void Spawn(void);// XDM: crash fix and world gibs
	virtual void Precache(void);
	virtual void KeyValue(KeyValueData *pkvd);
	//virtual void OnFreePrivateData(void);// XDM3035c
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);// XDM
	//virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);// XDM
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM
	virtual float FallDamage(const float &flFallVelocity);// XDM3035c
	virtual Vector Center(void) { return pev->origin; }// XDM: faster
	virtual bool IsPushable(void) { return true; }// XDM
	virtual int Classify(void) { return CLASS_GIB; }// XDM
	virtual int ObjectCaps(void) { return (CBaseEntity::ObjectCaps()/* & ~FCAP_ACROSS_TRANSITION*/) | FCAP_DONT_SAVE; }

	void SpawnGib(const char *szGibModel);
	void EXPORT BounceGibTouch(CBaseEntity *pOther);
	void EXPORT StickyGibTouch(CBaseEntity *pOther);
	void EXPORT WaitTillLand(void);
	void LimitVelocity(void);

	static CGib *SpawnHeadGib(CBaseEntity *pVictim);
	static void SpawnRandomGibs(CBaseEntity *pVictim, size_t cGibs, bool human, Vector velocity);
	static void SpawnStickyGibs(CBaseEntity *pVictim, const Vector &vecOrigin, size_t cGibs);
	static int SpawnModelGibs(CBaseEntity *pVictim, const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &velocity, int rndVel, int modelIndex, size_t count, float life, int material, int bloodcolor, int flags = 0);// XDM

	int		m_bloodColor;
	int		m_cBloodDecals;
	int		m_material;
	float	m_lifeTime;
};


#define CUSTOM_SCHEDULES \
		virtual Schedule_t *ScheduleFromName(const char *pName);\
		static Schedule_t *m_scheduleList[];

#define DEFINE_CUSTOM_SCHEDULES(derivedClass) \
	Schedule_t *derivedClass::m_scheduleList[] =

#define IMPLEMENT_CUSTOM_SCHEDULES(derivedClass, baseClass) \
		Schedule_t *derivedClass::ScheduleFromName(const char *pName) \
		{\
			Schedule_t *pSchedule = ScheduleInList(pName, m_scheduleList, ARRAYSIZE(m_scheduleList)); \
			if (pSchedule == NULL) \
				return baseClass::ScheduleFromName(pName); \
			return pSchedule;\
		}





//-----------------------------------------------------------------------------
// XDM: ALL CLASS DEFINITIONS ARE HERE!
//-----------------------------------------------------------------------------
class CHeadCrab : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void);
	virtual void RunTask(Task_t *pTask);
	virtual void StartTask(Task_t *pTask);
	virtual void SetYawSpeed(void);
	virtual Vector Center(void);
	virtual Vector BodyTarget(const Vector &posSrc) { return Center(); }
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);

	virtual void AlertSound(void);
	virtual void AttackSound(void);// XDM3038c
	virtual void AttackHitSound(void);// XDM3038c
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void IdleSound(void);

	virtual void PrescheduleThink(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);

	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }// There are no animation events for these!
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist) { return FALSE; }
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);// <- BUGBUG: there IS an animation with ACT_MELEE_ATTACK2, but no event to bite!!
	virtual float GetTaskDelay(int iTask);// XDM3038c

	virtual float GetSoundVolume(void) { return VOL_NORM; }
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual bool FBecomeProne(void);// XDM
	virtual float FallDamage(const float &flFallVelocity);// XDM3035c
	virtual bool FValidateHintType(short sHint);// XDM3038c

	void EXPORT LeapTouch(CBaseEntity *pOther);

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
	static short sAIHints[];// XDM3038c
};


class CBabyCrab : public CHeadCrab
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual float GetSoundVolume(void) { return 0.8f; }
};


class CRat : public CBabyCrab
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void SetYawSpeed(void);

	virtual void AlertSound(void);
	virtual void AttackSound(void);// XDM3038c
	virtual void AttackHitSound(void);// XDM3038c
	virtual void IdleSound(void);
	virtual void DeathSound(void);
	virtual void PainSound(void);

	virtual float GetSoundVolume(void) { return 0.8f; }
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
};


// XDM3038a: cbase extern int g_iRelationshipTable[NUM_RELATIONSHIP_ClASSES][NUM_RELATIONSHIP_ClASSES];

#endif	//MONSTERS_H
