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
//searchents classname monster_alien_controller 0 kill
//searchents classname monster_alien_slave 0 kill
//=========================================================
// Alien slave monster
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "schedule.h"
#include "weapons.h"
#include "sound.h"
#include "soundent.h"
#include "customentity.h"// XDM
#include "effects.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
enum aslave_animation_events_e
{
	ISLAVE_AE_CLAW = 1,
	ISLAVE_AE_CLAWRAKE,// UNUSED
	ISLAVE_AE_ZAP_POWERUP,
	ISLAVE_AE_ZAP_SHOOT,
	ISLAVE_AE_ZAP_DONE
};

#define ISLAVE_MAX_BEAMS		8

class CISlave : public CSquadMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int ISoundMask(void);
	virtual void KeyValue(KeyValueData *pkvd);// XDM
	virtual int IRelationship(CBaseEntity *pTarget);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack2(float flDot, float flDist);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void DeathSound(void);
	virtual void PainSound(void);
	virtual void AlertSound(void);
	virtual void IdleSound(void);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
    virtual void StartTask(Task_t *pTask);
	virtual Schedule_t *GetSchedule(void);
	virtual Schedule_t *GetScheduleOfType(int Type);

	CUSTOM_SCHEDULES;
	virtual int	Save(CSave &save); 
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CallForHelp(float flDist, EHANDLE hEnemy, Vector &vecLocation);
	void ClearBeams(void);
	void ArmBeam(int side);
	void WackBeam(int side, CBaseEntity *pEntity);
	void ZapBeam(int side);
	void BeamGlow(void);

	int m_iBravery;
	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];
	int m_iBeams;
	int m_iHealingPolicy;// XDM3038c: not it's a policy
	EHANDLE m_hDead;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};
LINK_ENTITY_TO_CLASS( monster_alien_slave, CISlave );
LINK_ENTITY_TO_CLASS( monster_vortigaunt, CISlave );

TYPEDESCRIPTION	CISlave::m_SaveData[] = 
{
	DEFINE_FIELD( CISlave, m_iBravery, FIELD_INTEGER ),
	DEFINE_ARRAY( CISlave, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS ),
	DEFINE_FIELD( CISlave, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CISlave, m_iHealingPolicy, FIELD_INTEGER ),// XDM3038c
	DEFINE_FIELD( CISlave, m_hDead, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CISlave, CSquadMonster );

const char *CISlave::pAttackHitSounds[] = 
{
	"aslave/slv_strike1.wav",
	"aslave/slv_strike2.wav",
};

const char *CISlave::pAttackMissSounds[] = 
{
	"aslave/slv_miss1.wav",
	"aslave/slv_miss2.wav",
};

const char *CISlave::pPainSounds[] = 
{
	"aslave/slv_pain1.wav",
	"aslave/slv_pain2.wav",
};

const char *CISlave::pDeathSounds[] = 
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};

int CISlave::IRelationship( CBaseEntity *pTarget )
{
	if ((pTarget->IsPlayer()))
		if (FBitSet(pev->spawnflags, SF_MONSTER_WAIT_UNTIL_PROVOKED) && !HasMemory(bits_MEMORY_PROVOKED))
			return R_NO;

	return CBaseMonster::IRelationship(pTarget);
}

// XDM
void CISlave::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "healingpolicy"))// editor does not like upprecase letters
	{
		m_iHealingPolicy = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Spawn
//=========================================================
void CISlave :: Spawn()
{
	if (pev->health <= 0)
		pev->health = gSkillData.slaveHealth;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_YELLOW;// XDM3038: yellow looks better
	if (m_iClass == 0)// XDM3038b
		m_iClass = CLASS_ALIEN_MILITARY;
	if (m_iGibCount == 0)
		m_iGibCount = 16;// XDM: medium one
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.slaveScore;
	if (m_flDamage1 == 0)// XDM3038c
		m_flDamage1 = gSkillData.slaveDmgZap;
	if (m_flDamage2 == 0)// XDM3038c
		m_flDamage2 = gSkillData.slaveDmgClaw;

	CSquadMonster::Spawn();// XDM3038b: Precache();
	//SET_MODEL(edict(), STRING(pev->model));// XDM3037
	UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;

	if (pev->view_ofs.IsZero())
		pev->view_ofs.Set(0, 0, 64);// position of the eyes relative to monster's origin.

	if (m_voicePitch == 0)
		m_voicePitch = RANDOM_LONG(85, 110);

	if (m_iHealingPolicy == POLICY_ALLOW)// XDM
		pev->skin = 1;
	else if (m_iHealingPolicy == POLICY_DENY)
		pev->skin = 0;
	else if (RANDOM_LONG(0,2) == 0)// POLICY_UNDEFINED XDM3035c: rare
		pev->skin = 1;

	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CISlave :: Precache()
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/islave.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "aslave");

	CSquadMonster::Precache();// XDM3038a

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("aslave/slv_shoot.wav");
	PRECACHE_SOUND("aslave/zap.wav");
	//PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	//PRECACHE_SOUND("weapons/cbar_miss1.wav");
	
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	/*m_iszGibModel = ALLOC_STRING("models/islave_gibs1.mdl");// XDM
	m_iGibModelIndex = PRECACHE_MODEL(STRINGV(m_iszGibModel));*/
}	

void CISlave::CallForHelp(float flDist, EHANDLE hEnemy, Vector &vecLocation)
{
	// skip ones not on my netname
	if (FStringNull(pev->netname))
		return;

	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ))) != NULL)
	{
		vec_t d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy(hEnemy, vecLocation);
			}
		}
	}
}

//=========================================================
// ALertSound - scream
//=========================================================
void CISlave::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		SENTENCEG_PlayRndSz(edict(), "SLV_ALERT", GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch());
		CallForHelp(512, m_hEnemy, m_vecEnemyLKP);
	}
}

//=========================================================
// IdleSound
//=========================================================
void CISlave :: IdleSound(void)
{
	if (RANDOM_LONG(0, 2) == 0)
		SENTENCEG_PlayRndSz(edict(), "SLV_IDLE", GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch());
}

//=========================================================
// PainSound
//=========================================================
void CISlave :: PainSound(void)
{
	if (RANDOM_LONG(0, 2) == 0)
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);// XDM3038c
}

//=========================================================
// DieSound
//=========================================================

void CISlave :: DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);// XDM3038c
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CISlave :: ISoundMask(void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

void CISlave::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	ClearBeams();
	CSquadMonster::Killed(pInflictor, pAttacker, iGib);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CISlave :: SetYawSpeed (void)
{
	int ys;
	switch ( m_Activity )
	{
	case ACT_WALK:		
		ys = 60;	
		break;
	case ACT_RUN:		
		ys = 70;
		break;
	case ACT_IDLE:		
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}
	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CISlave :: HandleAnimEvent(MonsterEvent_t *pEvent)
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch (pEvent->event)
	{
		case ISLAVE_AE_CLAW:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack(70, m_flDamage2, DMG_SLASH);
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->PunchPitchAxis(-5.0f);
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], VOL_NORM, ATTN_NORM, 0, GetSoundPitch());
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], VOL_NORM, ATTN_NORM, 0, GetSoundPitch());
			}
		}
		break;

		/* UNUSED case ISLAVE_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.slaveDmgClawrake, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->PunchPitchAxis(-5);
				}
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], VOL_NORM, ATTN_NORM, 0, GetSoundPitch());
			}
			else
			{
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], VOL_NORM, ATTN_NORM, 0, GetSoundPitch());
			}
		}
		break;*/

		case ISLAVE_AE_ZAP_POWERUP:
		{
			// speed up attack when on hard
			if (gSkillData.iSkillLevel == SKILL_HARD)
				pev->framerate = 1.5;

			UTIL_MakeAimVectors( pev->angles );

			if (m_iBeams == 0)
			{
				Vector vecSrc(gpGlobals->v_forward); vecSrc *= 8.0f; vecSrc += pev->origin;
				MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSrc );
					WRITE_BYTE(TE_DLIGHT);
					WRITE_COORD(vecSrc.x);	// X
					WRITE_COORD(vecSrc.y);	// Y
					WRITE_COORD(vecSrc.z);	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 0 );		// r
				if (pev->skin > 0)// XDM
				{
					WRITE_BYTE( 63 );		// g
					WRITE_BYTE( 255 );		// b
				}else{
					WRITE_BYTE( 255 );		// g
					WRITE_BYTE( 63 );		// b
				}
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END( );
			}
			if (m_hDead != NULL)
			{
				WackBeam( -1, m_hDead );
				WackBeam( 1, m_hDead );
			}
			else
			{
				ArmBeam( -1 );
				ArmBeam( 1 );
				BeamGlow( );
			}
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "debris/zap4.wav", VOL_NORM, ATTN_NORM, 0, 100 + m_iBeams * 10);
			// pev->skin = m_iBeams / 2; XDM
		}
		break;

		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams( );

			if (m_hDead != NULL)
			{
				Vector vecDest(m_hDead->pev->origin);
				vecDest.z += 38.0f;
				TraceResult trace;
				UTIL_TraceHull( vecDest, vecDest, dont_ignore_monsters, human_hull, m_hDead->edict(), &trace );
				if (!trace.fStartSolid)
				{
					CBaseMonster *pMonster = m_hDead->MyMonsterPointer();
					if (pMonster)
					{
						ClearBits(pMonster->pev->spawnflags, SF_MONSTER_WAIT_TILL_SEEN);
						int tmp = pMonster->pev->skin;
						pMonster->CBaseEntity::Spawn(TRUE);// XDM3035: looks like it works
						pMonster->pev->skin = tmp;
						pMonster->Forget(bits_MEMORY_KILLED);

						if (pMonster->ShouldFadeOnDeath())
							pMonster->pev->rendermode = kRenderNormal;

						WackBeam(-1, pMonster);
						WackBeam(1, pMonster);
						pMonster->AlignToFloor();// XDM3038a
					}
					m_hDead = NULL;
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "aslave/slv_shoot.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
					break;
				}
			}
			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( -1 );
			ZapBeam( 1 );

			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "aslave/slv_shoot.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			// STOP_SOUND( edict(), CHAN_WEAPON, "debris/zap4.wav" );
			ApplyMultiDamage(this, this);

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
		break;

		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams();
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CISlave :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (pev->skin > 0)// XDM
		return FALSE;

	if (m_flNextAttack > gpGlobals->time)
		return FALSE;

	return CSquadMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CISlave :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (pev->skin <= 0)// XDM
		return FALSE;

	if (m_flNextAttack > gpGlobals->time)
		return FALSE;

	m_hDead = NULL;
	m_iBravery = 0;

	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname(pEntity, STRING(pev->classname)/*"monster_alien_slave"*/)) != NULL)
	{
		if (FBitSet(pEntity->pev->effects, EF_NODRAW))// XDM3038b: ignore gibbed monsters that are probably waiting to respawn
			continue;

		TraceResult tr;
		UTIL_TraceLine(EyePosition(), pEntity->EyePosition(), ignore_monsters, edict(), &tr );
		if (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())
		{
			if (pEntity->pev->deadflag == DEAD_DEAD)
			{
				vec_t d = (pev->origin - pEntity->pev->origin).Length();
				if (d < flDist)
				{
					m_hDead = pEntity;
					flDist = d;
				}
				--m_iBravery;
			}
			else
			{
				++m_iBravery;
			}
		}
	}
	if (m_hDead != NULL)
		return TRUE;
	else
		return FALSE;
}

//=========================================================
// StartTask
//=========================================================
void CISlave :: StartTask ( Task_t *pTask )
{
	ClearBeams();
	CSquadMonster::StartTask(pTask);
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================
int CISlave::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	if (IsAlive())
	{
		if ((bitsDamageType & DMG_SLASH) && pAttacker)
		{
			//if (IRelationship(pAttacker) < R_DL)
			if (FStrEq(pev->classname, pAttacker->pev->classname))// XDM3038c: it was too unfair
				return 0;
		}
		m_afMemory |= bits_MEMORY_PROVOKED;
	}
	return CSquadMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}


void CISlave::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (pAttacker && (bitsDamageType & DMG_SHOCK) && FStrEq(pev->classname, pAttacker->pev->classname))// XDM3038c && !(bitsDamageType & (DMG_BULLET | DMG_ENERGYBEAM | DMG_IGNOREARMOR | DMG_DISINTEGRATING)))// XDM3035a
		return;

	CSquadMonster::TraceAttack( pAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSlaveAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSlaveAttack1[] =
{
	{ 
		tlSlaveAttack1,
		ARRAYSIZE ( tlSlaveAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 
		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};


DEFINE_CUSTOM_SCHEDULES( CISlave )
{
	slSlaveAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CISlave, CSquadMonster );

//=========================================================
//=========================================================
Schedule_t *CISlave :: GetSchedule(void)
{
//	ALERT(at_console, "CISlave(%d %s)::GetSchedule()\n", entindex(), STRING(pev->targetname));

	ClearBeams( );
/*
	if (pev->spawnflags)
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound = PBestSound();
		//ASSERT( pSound != NULL );
		if (pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			else if (pSound->m_iType & bits_SOUND_COMBAT)
				m_afMemory |= bits_MEMORY_PROVOKED;
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
// dead enemy
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster :: GetSchedule();
		}

		if (pev->health < 20 || m_iBravery < 0)
		{
			if (!HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
			{
				m_failSchedule = SCHED_CHASE_ENEMY;
				if (HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				if ( HasConditions ( bits_COND_SEE_ENEMY ) && HasConditions ( bits_COND_ENEMY_FACING_ME ) )
				{
					// ALERT( at_console, "exposed\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
		}
		break;
	}
	return CSquadMonster::GetSchedule( );
}

Schedule_t *CISlave :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_FAIL:
		if (HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
		{
			return CSquadMonster :: GetScheduleOfType( SCHED_MELEE_ATTACK1 ); ;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSlaveAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSlaveAttack1;
	}
	return CSquadMonster :: GetScheduleOfType( Type );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CISlave :: ArmBeam( int side )
{
	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	TraceResult tr;
	float flDist = 1.0;
	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (short i = 0; i < 3; ++i)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, edict(), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (m_pBeam[m_iBeams])
	{
		m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
		m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
		m_pBeam[m_iBeams]->SetColor(0, 255, 127);
		m_pBeam[m_iBeams]->SetBrightness(63);
		m_pBeam[m_iBeams]->SetNoise(80);
		m_pBeam[m_iBeams]->SetFlags(BEAM_FSHADEIN);// XDM
		++m_iBeams;
	}
	if (g_pGameRules->FAllowEffects())// XDM3035
		UTIL_DecalTrace(&tr, DECAL_GUNSHOT1 + RANDOM_LONG(0,4));// XDM
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlave :: BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness(b);
		}
	}
}

//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CISlave :: WackBeam( int side, CBaseEntity *pEntity )
{
	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(pEntity->Center(), entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(0, 63, 255);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_pBeam[m_iBeams]->SetFlags(BEAM_FSHADEIN);// XDM
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CISlave :: ZapBeam( int side )
{
	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	TraceResult tr;
	Vector vecSrc(pev->origin + gpGlobals->v_up * 36);
	Vector vecAim(ShootAtEnemy(vecSrc));
	float deflection = 0.01;
	vecAim += side * gpGlobals->v_right * RANDOM_FLOAT(0, deflection) + gpGlobals->v_up * RANDOM_FLOAT(-deflection, deflection);
	UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, edict(), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(0, 255, 63);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(20);
	m_iBeams++;

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage != DAMAGE_NO)
		pEntity->TraceAttack(this, m_flDamage1, vecAim, &tr, DMG_SHOCK);

	UTIL_EmitAmbientSound( edict(), tr.vecEndPos, "aslave/zap.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CISlave :: ClearBeams()
{
	for (int i = 0; i < ISLAVE_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	// pev->skin = 0; XDM

	STOP_SOUND( edict(), CHAN_WEAPON, "debris/zap4.wav" );
}
