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
// Zombie
//=========================================================
// UNDONE: Don't flinch every time you get hit
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"globals.h"
#include	"game.h"
#include	"gamerules.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
enum zombie_animation_events_e
{
	ZOMBIE_AE_ATTACK_RIGHT = 1,
	ZOMBIE_AE_ATTACK_LEFT,
	ZOMBIE_AE_ATTACK_BOTH
};

#define ZOMBIE_ATTACK_DIST			72
#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual int IgnoreConditions(void);
	virtual void PainSound(void);
	virtual void AlertSound(void);
	virtual void IdleSound(void);
	virtual void AttackSound(void);
	// No range attacks
	virtual BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	virtual BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual const char *GetDropItemName(void) { return "item_flare"; }// XDM3038

	float m_flNextFlinch;
	float m_flNextPainTime;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie );

const char *CZombie::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombie::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombie::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombie::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombie::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombie::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CZombie::Classify (void)
{
	return m_iClass?m_iClass:CLASS_HUMAN_MONSTER;// XDM3038a
}

//=========================================================
// Spawn
//=========================================================
void CZombie::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.zombieHealth;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_YELLOW;// XDM: now green and yellow colors are different
	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;// XDM: medium one
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.zombieScore;
	if (m_flDamage1 == 0)// XDM3038c
		m_flDamage1 = gSkillData.zombieDmgSlash;

	CBaseMonster::Spawn();// XDM3038b: Precache();

	//SET_MODEL(ENT(pev), STRING(pev->model));// XDM3037
	UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->view_ofs		= VEC_VIEW_OFFSET;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	m_flNextPainTime	= gpGlobals->time;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/zombie.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "zombie");

	CBaseMonster::Precache();// XDM3038a

	// XDM3038
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);

	CBaseMonster::Precache();// XDM3038
}	

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed(void)
{
	pev->yaw_speed = 90 + 10*gSkillData.iSkillLevel;// XDM3035c
}

int CZombie::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	// Take 30% damage from bullets
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir;
		if (pInflictor)
			vecDir = Center() - pInflictor->Center();// XDM3034
		else
			vecDir = g_vecAttackDir;

		vecDir.NormalizeSelf();
		pev->velocity += vecDir * DamageForce(flDamage);
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();

	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

void CZombie::PainSound(void)
{
	if (m_flNextPainTime <= gpGlobals->time)
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);// XDM3038c
		m_flNextPainTime = gpGlobals->time + RANDOM_FLOAT(4,6);
	}
}

void CZombie::AlertSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);// XDM3038c
}

void CZombie::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);// XDM3038c
}

void CZombie::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);// XDM3038c
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		case ZOMBIE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack(ZOMBIE_ATTACK_DIST, m_flDamage1, DMG_SLASH);
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					if (pEvent->event == ZOMBIE_AE_ATTACK_RIGHT)
					{
						pHurt->pev->punchangle.z = -18;
						pHurt->pev->velocity -= gpGlobals->v_right * 100;
					}
					else
					{
						pHurt->pev->punchangle.z = 18;
						pHurt->pev->velocity += gpGlobals->v_right * 100;
					}
					pHurt->PunchPitchAxis(-5);
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack(ZOMBIE_ATTACK_DIST, m_flDamage1*2.0, DMG_SLASH);
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->PunchPitchAxis(-5);
					pHurt->pev->velocity += gpGlobals->v_forward * m_flDamage1*8;
				}
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

int CZombie::IgnoreConditions (void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))// XDM3038c: fix?
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
	
}








#define	GONOME_MELEE_DIST			80 
#define	GONOME_JUMP_DIST			400
#define	GONOME_JUMP_INTERVAL		8
#define	GONOME_SPIT_DIST			1000
#define GONOME_SPIT_HEIGHT_DIFF		256

enum gonome_animation_events_e
{
	GONOME_AE_ATTACK_LEFT = 1,
	GONOME_AE_ATTACK_RIGHT,
	GONOME_AE_TAKE_GUTS,
	GONOME_AE_THROW_GUTS,
	GONOME_AE_JUMP_ATTACK
};

class CGonome : public CZombie
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void AlertSound(void);
	virtual void AttackSound(void);
	virtual void DeathSound(void);
	virtual void IdleSound(void);
	virtual void PainSound(void);
	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	void EXPORT LeapTouch(CBaseEntity *pOther);
	virtual float FallDamage(const float &flFallVelocity);

	CUSTOM_SCHEDULES;
	float m_flNextThrowGuts;
	float m_flNextJump;
	float m_painSoundTime;

	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pEatSounds[];
	static const char *pIdleSounds[];
	static const char *pPainSounds[];
};

LINK_ENTITY_TO_CLASS(monster_gonome, CGonome);

const char *CGonome::pAlertSounds[] = 
{
	"gonome/gonome_alert1.wav",
	"gonome/gonome_alert2.wav",
};

const char *CGonome::pAttackSounds[] = 
{
	"gonome/gonome_melee1.wav",
	"gonome/gonome_melee2.wav",
};

const char *CGonome::pDeathSounds[] = 
{
	"gonome/gonome_death1.wav",
	"gonome/gonome_death2.wav",
	"gonome/gonome_death3.wav",
};

const char *CGonome::pEatSounds[] = 
{
	"gonome/gonome_eat.wav",
};

const char *CGonome::pIdleSounds[] = 
{
	"gonome/gonome_idle1.wav",
	"gonome/gonome_idle2.wav",
	"gonome/gonome_idle3.wav",
};

const char *CGonome::pPainSounds[] = 
{
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
};

enum zombie_tasks_e
{
	TASK_EAT_AND_HEAL = LAST_COMMON_TASK + 1,
	TASK_JUMP_ATTACK,
};

Task_t	tlGonomeVictoryEat[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SIGNAL1			},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_RUN_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_IDLE				},
};

Schedule_t	slGonomeVictoryEat[] =
{
	{ 
		tlGonomeVictoryEat,
		ARRAYSIZE ( tlGonomeVictoryEat ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"VictoryEat"
	},
};

Task_t tlGonomeRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t slGonomeRangeAttack1[] =
{
	{ tlGonomeRangeAttack1, ARRAYSIZE(tlGonomeRangeAttack1), 0, 0, "Guts" },
};

Task_t tlGonomeJump[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_MELEE_ATTACK2 },
	{ TASK_JUMP_ATTACK,			(float)3.5		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t slGonomeJump[] =
{
	{ tlGonomeJump,	ARRAYSIZE(tlGonomeJump),	0,	0,	"Jump" },
};

DEFINE_CUSTOM_SCHEDULES(CGonome)
{
	slGonomeRangeAttack1,
	slGonomeVictoryEat,
	slGonomeJump,
};

IMPLEMENT_CUSTOM_SCHEDULES(CGonome, CZombie);


void CGonome::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.gonomeHealth;

	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_YELLOW;

	m_iGibCount = 16;
	CZombie::Spawn();
	m_flNextJump = m_flNextThrowGuts = gpGlobals->time;
}

void CGonome::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/gonome.mdl");

	CZombie::Precache();
	UTIL_PrecacheOther("gonomeguts");
}	

float CGonome::FallDamage(const float &flFallVelocity)
{
	if (flFallVelocity > 800)
		return flFallVelocity*0.01f;

	return 0.0f;
}

BOOL CGonome::CheckMeleeAttack1(float flDot, float flDist)
{
	if ( flDist <= GONOME_MELEE_DIST && flDot >= 0.7 )
		return TRUE;

	return FALSE;
}

BOOL CGonome::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= GONOME_JUMP_DIST && flDot >= 0.7 && (gpGlobals->time >= m_flNextJump))
		return TRUE;

	return FALSE;
}  

BOOL CGonome::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > GONOME_MELEE_DIST && flDist <= GONOME_SPIT_DIST && flDot >= 0.5 && (gpGlobals->time >= m_flNextThrowGuts))
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > GONOME_SPIT_HEIGHT_DIFF)
				return FALSE;
		}

		if (IsMoving())
			m_flNextThrowGuts = gpGlobals->time + 6.0;
		else
			m_flNextThrowGuts = gpGlobals->time + 0.75;

		return TRUE;
	}
	return FALSE;
}

int CGonome::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		flDamage *= 0.3;

	PainSound();
	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

void CGonome::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_JUMP_ATTACK:
		{
			if (m_fSequenceFinished)
			{
				TaskComplete();
				SetTouchNull();
				if (g_pGameRules->FAllowEffects())
					UTIL_ScreenShake(pev->origin, 4.0, 5.0, 1.0, 200);

				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		CZombie::RunTask(pTask);
		break;
	}
}

void CGonome::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	/* no changes to base class case TASK_MELEE_ATTACK1:
		{
			CBaseMonster::StartTask(pTask);
		}
		break;
	case TASK_MELEE_ATTACK2:
		{
			CBaseMonster::StartTask(pTask);
		}
		break;
	case TASK_GET_PATH_TO_ENEMY:
		{
			if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
				TaskComplete();
			else
				TaskFail();
		}
		break;*/
	case TASK_JUMP_ATTACK:
		{
			SetTouch(&CGonome::LeapTouch);
			m_flNextJump = gpGlobals->time + GONOME_JUMP_INTERVAL;
		}
		break;

	case TASK_EAT_AND_HEAL:
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pEatSounds[RANDOM_LONG(0,ARRAYSIZE(pEatSounds)-1)], GetSoundVolume(), ATTN_NORM, 0, 95 + RANDOM_LONG(0,9));
			if (pev->health < pev->max_health)
			{
				pev->health += RANDOM_LONG(5,15);
				if (pev->health > pev->max_health)
					pev->health = pev->max_health;
			}
			TaskComplete();
		}
		break;

	default:
		{
			CZombie::StartTask(pTask);
			break;
		}
	}
}

Schedule_t *CGonome::GetScheduleOfType(int Type)
{
	switch (Type)
	{
		case SCHED_RANGE_ATTACK1:
			return slGonomeRangeAttack1;
		break;

		case SCHED_VICTORY_DANCE:
			return slGonomeVictoryEat;
		break;// FIX: ghoul mistake

		case SCHED_MELEE_ATTACK2:
			return slGonomeJump;
		break;
	}
	return CZombie::GetScheduleOfType(Type);
}

void CGonome::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case GONOME_AE_TAKE_GUTS:
		{
			Vector vecGutsPos, aAng;
			GetAttachment(0, vecGutsPos, aAng);
			PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, vecGutsPos, pev->angles,  0.0, 0.0, MONFX_GONOMEGUTS, 2, 0, 0);
		}
		break;

		case GONOME_AE_THROW_GUTS:
		{
			Vector	vecSpitOffset, vecSpitDir, aAng, vecAngles;
			UTIL_MakeVectors(pev->angles);
			GetAttachment(0, vecSpitOffset, aAng);

			if (m_hEnemy)
				vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();
			else
				vecSpitDir = gpGlobals->v_forward;

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
			//CGonomeGuts::Shoot(vecSpitOffset, vecSpitDir, GetDamageAttacker(), this);
		}
		break;

		case GONOME_AE_ATTACK_LEFT:
		case GONOME_AE_ATTACK_RIGHT:
		{
			AttackSound();
			CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_MELEE_DIST, gSkillData.gonomeDmgBite, DMG_SLASH);
			if (pHurt)
			{
				pHurt->pev->velocity -= gpGlobals->v_forward * 70.0f;
				pHurt->pev->velocity += gpGlobals->v_up * 60.0f;
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
			}
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
		}
		break;

		case GONOME_AE_JUMP_ATTACK:
		{
			AttackSound();
			ClearBits(pev->flags, FL_ONGROUND);
			UTIL_SetOrigin(this, pev->origin + Vector(0, 0, 1));
			UTIL_MakeVectors(pev->angles);
			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				vec_t height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if (height < 32)
					height = 32;
				float speed = sqrt(2 * gravity * height);
				float time = speed / gravity;

				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir /= time;
				vecJumpDir.z = speed;

				vec_t distance = vecJumpDir.Length();
				if (distance > 650)
					vecJumpDir = vecJumpDir * (650.0f / distance);
			}
			else
			{
				vecJumpDir.Set( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z );
				vecJumpDir *= 300.0f;
			}
			pev->velocity = vecJumpDir;
		}
		break;

		default:
			CZombie::HandleAnimEvent(pEvent);
	}
}

void CGonome::LeapTouch(CBaseEntity *pOther)
{
	if (pOther->pev->takedamage == DAMAGE_NO)
		return;

	if (pOther->Classify() == Classify())
		return;

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
		pOther->TakeDamage(this, this, gSkillData.gonomeDmgJump, DMG_SLASH | DMG_ALWAYSGIB);
		pOther->pev->velocity += gpGlobals->v_forward * 300;// FIXME
		UTIL_BloodDrips(pev->origin, pev->origin-pOther->pev->origin, pOther->BloodColor(), gSkillData.gonomeDmgBite);
	}
	SetTouchNull();
}

void CGonome::AlertSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);
}

void CGonome::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

void CGonome::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

void CGonome::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

void CGonome::PainSound(void)
{
	if (m_flNextPainTime <= gpGlobals->time)
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
		m_flNextPainTime = gpGlobals->time + RANDOM_FLOAT(4,6);
	}
}
