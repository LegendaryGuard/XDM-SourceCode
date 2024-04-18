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
//=========================================================
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"
#include "nodes.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define HC_AE_JUMPATTACK	2
#define HC_AE_BITEATTACK	3// XDM3038c

Task_t	tlHCRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.5		},
};

Schedule_t	slHCRangeAttack1[] =
{
	{
		tlHCRangeAttack1,
		ARRAYSIZE ( tlHCRangeAttack1 ),
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"
	},
};

Task_t	tlHCRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHCRangeAttack1Fast[] =
{
	{
		tlHCRangeAttack1Fast,
		ARRAYSIZE ( tlHCRangeAttack1Fast ),
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"
	},
};

LINK_ENTITY_TO_CLASS( monster_headcrab, CHeadCrab );

DEFINE_CUSTOM_SCHEDULES(CHeadCrab)
{
	slHCRangeAttack1,
	slHCRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHeadCrab, CBaseMonster);

const char *CHeadCrab::pIdleSounds[] =
{
	"headcrab/hc_idle1.wav",
	"headcrab/hc_idle2.wav",
	"headcrab/hc_idle3.wav",
};
const char *CHeadCrab::pAlertSounds[] =
{
	"headcrab/hc_alert1.wav",
};
const char *CHeadCrab::pPainSounds[] =
{
	"headcrab/hc_pain1.wav",
	"headcrab/hc_pain2.wav",
	"headcrab/hc_pain3.wav",
};
const char *CHeadCrab::pAttackSounds[] =
{
	"headcrab/hc_attack1.wav",
	"headcrab/hc_attack2.wav",
	"headcrab/hc_attack3.wav",
};

const char *CHeadCrab::pDeathSounds[] =
{
	"headcrab/hc_die1.wav",
	"headcrab/hc_die2.wav",
};

const char *CHeadCrab::pAttackHitSounds[] =
{
	"headcrab/hc_headbite.wav",
};

// XDM3038c
short CHeadCrab::sAIHints[] =
{
	HINT_WORLD_HEAT_SOURCE,
	HINT_WORLD_HUMAN_BLOOD
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHeadCrab::Classify(void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_PREY;// XDM
}

// XDM3038a: special hack to ignore tripmines (try c2a5d)
/*int CHeadCrab::IRelationship(CBaseEntity *pTarget)
{
	if (pTarget->IsProjectile())
	{
		if (pTarget->pev->solid == SOLID_NOT)// cannot bite anyway
			return R_NO;

		if (pTarget->pev->movetype == MOVETYPE_NONE)
			return R_NO;
	}
	return CBaseMonster::IRelationship(pTarget);
}*/

//=========================================================
// Spawn
//=========================================================
void CHeadCrab::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.headcrabHealth;
	if (m_flDamage1 == 0)// XDM3037
		m_flDamage1 = gSkillData.headcrabDmgBite;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_YELLOW;// XDM: now green and yellow are different colors
	if (m_iGibCount == 0)
		m_iGibCount = 4;// XDM: small one
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.headcrabScore;

	CBaseMonster::Spawn();// XDM3038b: Precache();
	UTIL_SetSize(this, Vector(-12, -12, 0), Vector(12, 12, 24));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->view_ofs.Set(0, 0, 20);// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHeadCrab::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037: Precache() may be called directly, 4ex. by UTIL_PrecacheOther()
		pev->model = MAKE_STRING("models/headcrab.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "headcrab");

	CBaseMonster::Precache();// XDM3038a
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
}

//=========================================================
// Center - returns the real center of the headcrab.  The
// bounding box is much larger than the actual creature so
// this is needed for targeting
//=========================================================
Vector CHeadCrab::Center(void)
{
	return Vector(pev->origin.x, pev->origin.y, pev->origin.z + 6);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHeadCrab::SetYawSpeed(void)
{
	int ys;
	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 30;
		break;
	case ACT_RUN:
		ys = 40;
		break;
	case ACT_WALK:
		ys = 30;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 60;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 30;
		break;
	default:
		ys = 30;
		break;
	}
	pev->yaw_speed = ys * gSkillData.iSkillLevel;// XDM3035
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it is in the air
//=========================================================
void CHeadCrab::LeapTouch(CBaseEntity *pOther)
{
	if (pOther->pev->takedamage == DAMAGE_NO)
		return;

	if (pOther->Classify() == Classify())
		return;

	// Don't hit if back on ground
	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		AttackHitSound();// XDM3038c
		pOther->TakeDamage(this, this, m_flDamage1, DMG_SLASH);
		if (g_pGameRules->FAllowEffects())
			UTIL_BloodDrips(pev->origin, pev->origin-pOther->pev->origin, pOther->BloodColor(), m_flDamage1);// XDM3037
	}
	SetTouchNull();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHeadCrab::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case HC_AE_JUMPATTACK:
		{
			ClearBits(pev->flags, FL_ONGROUND);
			pev->origin.z += 1.0f;
			UTIL_SetOrigin(this, pev->origin);// take him off ground so engine doesn't instantly reset onground
			UTIL_MakeVectors(pev->angles);
			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				vec_t height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if (height < 16)
					height = 16;
				vec_t speed = sqrt(2 * gravity * height);
				//float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				//vecJumpDir *= 1.0 / time;
				vecJumpDir *= gravity/speed;
				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;
				// Don't jump too far/fast
				vec_t distance = vecJumpDir.Length();
				if (distance > 650.0f)
					vecJumpDir *= (650.0f / distance);
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir.Set(gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z);
				vecJumpDir *= 350.0f;
			}
			AttackSound();
			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(2.0f, 2.25f);
		}
		break;

		case HC_AE_BITEATTACK:// XDM3038c
		{
			// this is bad! CBaseEntity *pOther = CheckTraceHullAttack(24, m_flDamage1, DMG_SLASH);
			UTIL_MakeVectors(pev->angles);
			Vector vecStart(pev->origin);
			Vector vecEnd(vecStart);
			vecEnd -= gpGlobals->v_up * 20.0f;
			TraceResult tr;
			UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, edict(), &tr);
			if (!FNullEnt(tr.pHit))// && tr.pHit == pev->groundentity)
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if (pEntity && pEntity != g_pWorld)// && pEntity == m_hEnemy)
				{
					pEntity->TakeDamage(this, GetDamageAttacker(), m_flDamage1, DMG_SLASH);
					if (g_pGameRules->FAllowEffects())
						UTIL_BloodDrips(pev->origin, pev->origin-pEntity->pev->origin, pEntity->BloodColor(), m_flDamage1);

					AttackHitSound();
				}
			}
			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(0.5f, 1.0f);
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent(pEvent);
			break;
	}
}

//=========================================================
// PrescheduleThink
//=========================================================
void CHeadCrab::PrescheduleThink(void)
{
	// make the crab coo a little bit in combat state
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT(0, 5) < 0.1f)
		IdleSound();
}

//=========================================================
// StartTask
//=========================================================
void CHeadCrab::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		{
			AttackSound();// XDM3038c
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch(&CHeadCrab::LeapTouch);
		}
		break;
	case TASK_RANGE_ATTACK2:
		{
			AttackSound();// XDM3038c
			m_IdealActivity = ACT_RANGE_ATTACK2;
			SetTouch(&CHeadCrab::LeapTouch);
		}
		break;
	default: CBaseMonster::StartTask(pTask); break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHeadCrab::RunTask(Task_t *pTask)
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouchNull();
				m_IdealActivity = ACT_IDLE;
				m_flNextAttack = gpGlobals->time + GetTaskDelay(pTask->iTask);// XDM3038c
			}
		}
		break;
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK2_NOTURN:
		{
			// XDM3038c: COMPATIBILITY HACK: because standard animaiton does not include event, force send it here
			MonsterEvent_t e;
			e.event = HC_AE_BITEATTACK;
			e.options = NULL;
			HandleAnimEvent(&e);
			//conprintf(1, "CHeadCrab::RunTask(%d)\n", pTask->iTask);
			CBaseMonster::RunTask(pTask);
		}
		break;
	default:
		{
			CBaseMonster::RunTask(pTask);
		}
		break;
	}
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CHeadCrab::CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65 )
		return TRUE;

	return FALSE;
}

BOOL CHeadCrab::CheckMeleeAttack2(float flDot, float flDist)
{
	if (pev->groundentity == m_hEnemy.Get())
		return TRUE;

	return FALSE;
}

// XDM3038c: experimental
float CHeadCrab::GetTaskDelay(int iTask)
{
	switch (iTask)
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			return RANDOM_FLOAT(0.25f, 1.0f);
		}
		break;
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK2_NOTURN:
		{
			return RANDOM_FLOAT(0.25f, 0.5f);
		}
		break;
	}
	return CBaseMonster::GetTaskDelay(iTask);
}

int CHeadCrab::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType & DMG_ACID)// Don't take any acid damage -- BigMomma's mortar is acid
	{
		if (pAttacker && pAttacker->Classify() != CLASS_ALIEN_PREDATOR)// XDM3034: bullsquids should be able to hurt headcrabs
			flDamage = 0;// don't return, react
	}
	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

void CHeadCrab::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pIdleSounds, ATTN_IDLE, GetSoundPitch());
}

void CHeadCrab::AlertSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pAlertSounds, ATTN_STATIC, GetSoundPitch());
}

// XDM3038c
void CHeadCrab::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pAttackSounds, ATTN_STATIC, GetSoundPitch());
}

void CHeadCrab::PainSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pPainSounds, ATTN_STATIC, GetSoundPitch());
}

void CHeadCrab::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pDeathSounds, ATTN_IDLE, GetSoundPitch());
}

void CHeadCrab::AttackHitSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_WEAPON, pAttackHitSounds, ATTN_STATIC, GetSoundPitch());
}

Schedule_t *CHeadCrab::GetScheduleOfType(int Type)
{
	switch (Type)
	{
		case SCHED_RANGE_ATTACK1: return &slHCRangeAttack1[0]; break;
		case SCHED_RANGE_ATTACK2: return &slHCRangeAttack1Fast[0]; break;// XDM3035c: wtf?
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

//-----------------------------------------------------------------------------
// Purpose: XDM: don't let the headcrab bite a barnacle!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHeadCrab::FBecomeProne(void)
{
	pev->basevelocity.Clear();
	SetTouchNull();
	SetActivity(ACT_MELEE_ATTACK2);// cool sequence!
	ClearConditions(bits_COND_SEE_DISLIKE);
	return CBaseMonster::FBecomeProne();
}

//-----------------------------------------------------------------------------
// Purpose: Override
// Output : float - final fall damage value
//-----------------------------------------------------------------------------
float CHeadCrab::FallDamage(const float &flFallVelocity)
{
	if (flFallVelocity > 800)
		return flFallVelocity*0.01;

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Override
// Input  : sHint - AI hint found in path node
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHeadCrab::FValidateHintType(short sHint)
{
	for (size_t i = 0; i < ARRAYSIZE(sAIHints); ++i)
	{
		if (sAIHints[i] == sHint)
			return true;
	}
	return CBaseMonster::FValidateHintType(sHint);// XDM3038c
}








LINK_ENTITY_TO_CLASS(monster_babycrab, CBabyCrab);

void CBabyCrab::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.headcrabHealth*0.25f;	// less health than full grown

	if (m_flDamage1 == 0)// XDM3037
		m_flDamage1 = gSkillData.headcrabDmgBite*0.3f;

	if (m_voicePitch == 0)// XDM3038b: before MonsterInit()!
		m_voicePitch = PITCH_NORM + RANDOM_LONG(40,50);

	if (m_iGibCount == 0)
		m_iGibCount = 1;// XDM: very small one

	CHeadCrab::Spawn();// Precache() is there
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 191;
	UTIL_SetSize(this, Vector(-12, -12, 0), Vector(12, 12, 24));
}

void CBabyCrab::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/baby_headcrab.mdl");// XDM3037

	CHeadCrab::Precache();
}

void CBabyCrab::SetYawSpeed(void)
{
	pev->yaw_speed = 120;
}

BOOL CBabyCrab::CheckRangeAttack1(float flDot, float flDist)
{
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (pev->groundentity && (pev->groundentity->v.flags & (FL_CLIENT|FL_MONSTER)))
			return TRUE;

		// A little less accurate, but jump from closer
		if (flDist <= 180 && flDot >= 0.55)
			return TRUE;
	}
	return FALSE;
}

Schedule_t *CBabyCrab::GetScheduleOfType(int Type)
{
	switch (Type)
	{
		case SCHED_FAIL:// If you fail, try to jump!
		{
			if (m_hEnemy.Get())
				return slHCRangeAttack1Fast;
		}
		break;
		case SCHED_RANGE_ATTACK1:
		{
			return slHCRangeAttack1Fast;
		}
		break;
	}
	return CHeadCrab::GetScheduleOfType(Type);
}
