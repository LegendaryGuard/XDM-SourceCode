//=========================================================
// Ghoul. pit drone monster
//=========================================================
// This code is a total shit. It is not part of XHL. It should be rewritten.
// Ghoul, you are a %s moron: why the %s did you rename sounds, classname!?
// Where are talk sounds? Where is save/restore data!? Do you even know what pev->weapons is?
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "game.h"
#include "globals.h"
#include "projectile.h"
#include "crossbowbolt.h"// STUB

#define	PITDRONE_MELEE_DIST		75 
#define	PITDRONE_SPIT_DIST		1280 

enum
{
	SCHED_PITDRONE_HURTHOP = LAST_COMMON_SCHEDULE + 1,
};

enum 
{
	TASK_PITDRONE_HOPTURN = LAST_COMMON_TASK + 1,
	TASK_EAT_AND_HEAL
};

// animation events
enum
{
	BPITDRONE_AE_SPIT = 1,
	BPITDRONE_AE_BITE,
	BPITDRONE_AE_HOP,
};

class CPitDrone : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void IdleSound(void);
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void AlertSound(void);
	virtual void AttackSound(void);
	virtual void SpitSound(void);
	virtual void AttackHitSound(void);
	virtual void AttackMissSound(void);
	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual Schedule_t *GetSchedule(void);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int	Save(CSave &save); 
	virtual int Restore(CRestore &restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastHurtTime;
	float m_flNextSpitTime;
	float m_fPainSoundTime;

	static const char *pDeathSounds[];
	static const char *pAttackSounds[];
	static const char *pSpitSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS(monster_pitdrone, CPitDrone);

TYPEDESCRIPTION	CPitDrone::m_SaveData[] = 
{
	DEFINE_FIELD(CPitDrone, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CPitDrone, m_flNextSpitTime, FIELD_TIME),
	DEFINE_FIELD(CPitDrone, m_fPainSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CPitDrone, CBaseMonster);

const char *CPitDrone::pAlertSounds[] = 
{
	"pitdrone/pit_drone_alert1.wav",
	"pitdrone/pit_drone_alert2.wav",
	"pitdrone/pit_drone_alert3.wav",
};

const char *CPitDrone::pAttackSounds[] = 
{
	"pitdrone/pit_drone_melee_attack1.wav",
	"pitdrone/pit_drone_melee_attack2.wav",
};

const char *CPitDrone::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CPitDrone::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CPitDrone::pSpitSounds[] = 
{
	"pitdrone/pit_drone_attack_spike1.wav",
	"pitdrone/pit_drone_attack_spike2.wav",
};

const char *CPitDrone::pIdleSounds[] = 
{
	"pitdrone/pit_drone_idle1.wav",
	"pitdrone/pit_drone_idle2.wav",
	"pitdrone/pit_drone_idle3.wav",
};

const char *CPitDrone::pDeathSounds[] = 
{
	"pitdrone/pit_drone_die1.wav",
	"pitdrone/pit_drone_die2.wav",
	"pitdrone/pit_drone_die3.wav",
};

const char *CPitDrone::pPainSounds[] = 
{
	"pitdrone/pit_drone_pain1.wav",
	"pitdrone/pit_drone_pain2.wav",
	"pitdrone/pit_drone_pain3.wav",
	"pitdrone/pit_drone_pain4.wav",
};

// UNDONE: pit_drone_communicate


void CPitDrone::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.pitdroneHealth;
	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_YELLOW;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.pitdroneScore;
	if (m_iGibCount == 0)
		m_iGibCount = 16;

	CBaseMonster::Spawn();
	//SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(this, Vector(-32, -32, 0), Vector(32, 32, 48));
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects	 = 0;
	pev->weapons	 = 6;
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextSpitTime = gpGlobals->time;
	SetBodygroup(1, pev->weapons);
	MonsterInit();
}

void CPitDrone::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/pit_drone.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "pitdrone");

	CBaseMonster::Precache();

	//UTIL_PrecacheOther("pitdronespike");
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pSpitSounds);
	PRECACHE_SOUND("pitdrone/pit_drone_eat.wav");
}

int CPitDrone::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pAttacker == this)
		return 0;

	float flDist;
	Vector vecApex;
	if (m_hEnemy != NULL && IsMoving() && pAttacker == m_hEnemy && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();
		if (flDist > PITDRONE_SPIT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();
			if (FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}
	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

BOOL CPitDrone::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= PITDRONE_SPIT_DIST)
		return FALSE;

	if (flDist > PITDRONE_MELEE_DIST && flDist <= PITDRONE_SPIT_DIST && flDot >= 0.5 && (gpGlobals->time >= m_flNextSpitTime) && pev->weapons > 0)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256.0f)
				return FALSE;
		}

		if (IsMoving())
			m_flNextSpitTime = gpGlobals->time + 5;
		else
			m_flNextSpitTime = gpGlobals->time + 0.5;

		return TRUE;
	}
	return FALSE;
}

BOOL CPitDrone::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= PITDRONE_MELEE_DIST && flDot >= 0.7)
		return TRUE;

	return FALSE;
}

int	CPitDrone::Classify(void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

void CPitDrone::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

void CPitDrone::PainSound(void)
{
	if (IsAlive() && (m_fPainSoundTime < gpGlobals->time))
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
		m_fPainSoundTime = gpGlobals->time + RANDOM_FLOAT(2.5, 4);
	}
}

void CPitDrone::AlertSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);
}

void CPitDrone::SpitSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pSpitSounds);
}

void CPitDrone::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

void CPitDrone::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

void CPitDrone::AttackHitSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
}

void CPitDrone::AttackMissSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);
}

void CPitDrone::SetYawSpeed(void)
{
/*	int ys;
	ys = 0;

	switch ( m_Activity )
	{
	case ACT_WALK:			ys = 90;	break;
	case ACT_RUN:			ys = 90;	break;
	case ACT_IDLE:			ys = 90;	break;
	case ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}*/
	pev->yaw_speed = 90;//ys
}

void CPitDrone::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case BPITDRONE_AE_SPIT:
		{
			Vector vecSpitOffset, vecSpitDir, vecAngles;
			UTIL_MakeVectors(pev->angles);
			GetAttachment(0, vecSpitOffset, vecAngles);

			if (m_hEnemy)
				vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();
			else
				vecSpitDir = gpGlobals->v_forward;

			SpitSound();
			pev->weapons--;
			SetBodygroup(1, pev->weapons);
			CCrossbowBolt::BoltCreate(vecSpitOffset, vecAngles, vecSpitDir, this, this, gSkillData.pitdroneDmgSpit, true);// STUB
			//CPitdroneSpike::Shoot(vecSpitOffset, vecSpitDir, GetDamageAttacker(), this);
		}
		break;

		case BPITDRONE_AE_BITE:
		{
			AttackSound();
			CBaseEntity *pHurt = CheckTraceHullAttack(PITDRONE_MELEE_DIST, gSkillData.pitdroneDmgBite, DMG_SLASH);
			if (pHurt)
			{
				pHurt->pev->velocity += gpGlobals->v_forward * -70.0f + gpGlobals->v_up * 60.0f;
				AttackHitSound();
			}
			else
				AttackMissSound();
		}
		break;

		case BPITDRONE_AE_HOP:
		{
			float flGravity = g_psv_gravity->value;
			if (FBitSet(pev->flags, FL_ONGROUND))
				ClearBits(pev->flags, FL_ONGROUND);

			pev->velocity.z += (0.625 * flGravity) * 0.5;
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent(pEvent);
	}
}

Task_t	tlPitdroneRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slPitdroneRangeAttack1[] =
{
	{ 
		tlPitdroneRangeAttack1, ARRAYSIZE(tlPitdroneRangeAttack1), 0, 0, "Spit"
	},
};

Task_t tlPitdroneHurtHop[] =
{
	{ TASK_STOP_MOVING,			(float)0	},
	{ TASK_SOUND_WAKE,			(float)0	},
	{ TASK_PITDRONE_HOPTURN,	(float)0	},
	{ TASK_FACE_ENEMY,			(float)0	},
};

Schedule_t slPitdroneHurtHop[] =
{
	{
		tlPitdroneHurtHop, ARRAYSIZE ( tlPitdroneHurtHop ),	0,	0,	"PitdroneHurtHop"
	}
};

Task_t	tlPitdroneVictoryEat[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
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
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_EAT_AND_HEAL,					(float)0.2					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_IDLE				},
};

Schedule_t	slPitdroneVictoryEat[] =
{
	{
		tlPitdroneVictoryEat,
		ARRAYSIZE ( tlPitdroneVictoryEat ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"VictoryEat"
	},
};

DEFINE_CUSTOM_SCHEDULES(CPitDrone)
{
	slPitdroneRangeAttack1,
	slPitdroneHurtHop,
	slPitdroneVictoryEat
};

IMPLEMENT_CUSTOM_SCHEDULES(CPitDrone, CBaseMonster);

Schedule_t *CPitDrone::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
		{
			if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
				return GetScheduleOfType(SCHED_PITDRONE_HURTHOP);
		}
		break;
	case MONSTERSTATE_COMBAT:
		{
			if (HasConditions(bits_COND_ENEMY_DEAD))
				return CBaseMonster::GetSchedule();
			else if (HasConditions(bits_COND_NEW_ENEMY))
				return GetScheduleOfType(SCHED_WAKE_ANGRY);
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
				return GetScheduleOfType(SCHED_MELEE_ATTACK1);
			else
				return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
		break;
	//default:
	//		return CBaseMonster::GetSchedule();
	//	break;
	}
	return CBaseMonster::GetSchedule();
}

Schedule_t *CPitDrone::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slPitdroneRangeAttack1[0];
		break;

	case SCHED_PITDRONE_HURTHOP:
		return &slPitdroneHurtHop[0];
		break;

	case SCHED_VICTORY_DANCE:
			return &slPitdroneVictoryEat[0];
		break;
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

void CPitDrone::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
		{
			CBaseMonster::StartTask(pTask);
			break;
		}

	case TASK_PITDRONE_HOPTURN:
		{
			SetActivity(ACT_HOP);
			MakeIdealYaw(m_vecEnemyLKP);
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			else
				TaskFail();
		}
		break;
	case TASK_EAT_AND_HEAL:
		{
			if (pev->health < pev->max_health)
				pev->health += RANDOM_LONG(5, 15);

			if (pev->health > pev->max_health)
				pev->health = pev->max_health;

			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_eat.wav", GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch());
			TaskComplete();
		}
		break;
	default:
		{
			CBaseMonster::StartTask(pTask);
			break;
		}
	}
}

void CPitDrone::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_PITDRONE_HOPTURN:
		{
			MakeIdealYaw(m_vecEnemyLKP);
			ChangeYaw(pev->yaw_speed);
			if (m_fSequenceFinished)
				m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;
	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}
