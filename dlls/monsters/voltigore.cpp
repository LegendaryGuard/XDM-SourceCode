//=========================================================
// Ghoul. Voltigore monster from Opposing Force
//=========================================================
// This code is a total shit. It is not part of XHL. It should be rewritten.
// Ghoul, you are a %s moron: why the %s did you rename sounds, classname!?
// Where are talk sounds? Where is save/restore data!? Do you even know what pev->weapons is?
// Do you know how UTIL_MakeVectors() works and where you should use it!?!
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
//#include "schedule.h"
#include "nodes.h"
//#include "soundent.h"
#include "weapons.h"
#include "globals.h"
#include "lightp.h"

#define	VOLTIGORE_MELEE_DIST			115 
#define	VOLTIGORE_MONFX_DIST			4000 
#define VOLTIGORE_BABY_HEALTH_FACTOR	0.33
#define VOLTIGORE_BABY_DMG_FACTOR		0.25

enum voltigore_animation_events_e
{
	AE_SLASH_LEFT = 1,
	AE_SLASH_BOTH,
	AE_THROW_EBOLT,
	AE_SND_RANGE_ATT
};


class CVoltigore : public CBaseMonster
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
	virtual void AttackHitSound(void);
	virtual void AttackMissSound(void);
	virtual void StartTask(Task_t *pTask);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int	Save(CSave &save); 
	virtual int Restore(CRestore &restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextThrowEbolt;
	float m_fPainSoundTime;

	static const char *pDeathSounds[];
	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS(monster_alien_voltigore, CVoltigore);

TYPEDESCRIPTION	CVoltigore::m_SaveData[] = 
{
	DEFINE_FIELD(CVoltigore, m_flNextThrowEbolt, FIELD_TIME),
	DEFINE_FIELD(CVoltigore, m_fPainSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CVoltigore, CBaseMonster);

const char *CVoltigore::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CVoltigore::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CVoltigore::pAttackSounds[] = 
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char *CVoltigore::pIdleSounds[] = 
{
	"voltigore/voltigore_idle1.wav",
	"voltigore/voltigore_idle2.wav",
	"voltigore/voltigore_idle3.wav",
};

const char *CVoltigore::pDeathSounds[] = 
{
	"voltigore/voltigore_die1.wav",
	"voltigore/voltigore_die2.wav",
	"voltigore/voltigore_die3.wav",
};

const char *CVoltigore::pAlertSounds[] = 
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

const char *CVoltigore::pPainSounds[] = 
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};


enum voltigore_tasks_e
{
	TASK_EAT_AND_HEAL = LAST_COMMON_TASK + 1,
};

void CVoltigore::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.voltigoreHealth;
	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_YELLOW;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.voltigoreScore;
	if (m_iGibCount == 0)
		m_iGibCount = 16;

	CBaseMonster::Spawn();
	//SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(this, Vector(-80, -80, 0), Vector(80, 80, 90));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->view_ofs		= Vector(0,0, 85);
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextThrowEbolt	= gpGlobals->time;
	MonsterInit();
}

void CVoltigore::Precache()
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/voltigore.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "voltigore");

	CBaseMonster::Precache();

	PRECACHE_SOUND("voltigore/voltigore_attack_shock.wav");
	PRECACHE_SOUND("voltigore/voltigore_run_grunt1.wav");
	PRECACHE_SOUND("voltigore/voltigore_run_grunt2.wav");
	m_iGibModelIndex = PRECACHE_MODEL("models/voltigore_gibs.mdl");

	//UTIL_PrecacheOther("voltigorebolt");
	PRECACHE_SOUND("voltigore/voltigore_eat.wav");
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
}

int CVoltigore::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB | DMG_SHOCK))
		flDamage *= 0.75;

	PainSound();
	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

BOOL CVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= VOLTIGORE_MELEE_DIST && flDot >= 0.7)
		return TRUE;

	return FALSE;
}

BOOL CVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	if ((gpGlobals->time > m_flNextThrowEbolt) && flDot >= 0.7 && flDist > VOLTIGORE_MELEE_DIST && flDist <= VOLTIGORE_MONFX_DIST)
		return TRUE;

	return FALSE;
}

int	CVoltigore::Classify(void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

void CVoltigore::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

void CVoltigore::PainSound(void)
{
	if ((IsAlive()) && m_fPainSoundTime < gpGlobals->time)
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
		m_fPainSoundTime = gpGlobals->time + RANDOM_FLOAT(2.5, 4);
	}
}

void CVoltigore::AlertSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);
}

void CVoltigore::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

void CVoltigore::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

void CVoltigore::AttackHitSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
}

void CVoltigore::AttackMissSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);
}

void CVoltigore::SetYawSpeed(void)
{
/*	int ys;
	ys = 0;

	switch ( m_Activity )
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}*/
	pev->yaw_speed = 90;//ys;
}

void CVoltigore::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case AE_SLASH_LEFT:
		{
			AttackSound();
			CBaseEntity *pHurt = CheckTraceHullAttack(VOLTIGORE_MELEE_DIST, gSkillData.voltigoreDmgBite, DMG_SLASH);
			if (pHurt)
			{
				pHurt->pev->velocity += gpGlobals->v_forward * -100.0f + gpGlobals->v_up * 80.0f;
				AttackHitSound();
			}
			else
				AttackMissSound();
		}
		break;

	case AE_SLASH_BOTH:
		{
			AttackSound();
			CBaseEntity *pHurt = CheckTraceHullAttack(VOLTIGORE_MELEE_DIST, gSkillData.voltigoreDmgBite*2.0f, DMG_SLASH);
			if (pHurt)
			{
				if (pHurt->pev->flags & (FL_MONSTER|FL_CLIENT))
				{
					pHurt->PunchPitchAxis(10);
					pHurt->pev->punchangle.y = -10;
					pHurt->pev->punchangle.z = 15;
					pHurt->pev->velocity += gpGlobals->v_forward * -100.0f;
				}
				AttackHitSound();
			}
			else 
			AttackMissSound();
		}
		break;

	case AE_SND_RANGE_ATT:
		{
			if (pev->dmgtime <= gpGlobals->time)
			{
				PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, pev->origin, pev->angles, 0.0, 0.0, MONFX_VOLTIGORE_ANIM_EFFECT, 0, 0, 0);
				pev->dmgtime = gpGlobals->time + 2.0f;
			}
		}
		break;

	case AE_THROW_EBOLT:
		{
			Vector vecSrc, vecDir, vecAngles;
			UTIL_MakeVectors(pev->angles);
			GetAttachment(0, vecSrc, vecAngles);

			if (m_hEnemy)
				vecDir = (m_hEnemy->pev->origin - vecSrc).Normalize();
			else
				vecDir = gpGlobals->v_forward;
			
			//CVoltigoreBolt::Shoot(vecSrc, vecDir, GetDamageAttacker(), this);
			CLightProjectile::CreateLP(vecSrc, vecAngles, vecDir, this, this, gSkillData.voltigoreDmgBolt, 1);// STUB
			m_flNextThrowEbolt = gpGlobals->time + 5.0f;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

Task_t tlVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t slVoltigoreRangeAttack1[] =
{
	{
		tlVoltigoreRangeAttack1, ARRAYSIZE (tlVoltigoreRangeAttack1), 0, 0, "Range Attack1"
	},
};

Task_t tlVoltigoreVictoryEat[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_WAIT,							(float)0.2					},
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

Schedule_t slVoltigoreVictoryEat[] =
{
	{
		tlVoltigoreVictoryEat,
		ARRAYSIZE(tlVoltigoreVictoryEat),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"VictoryEat"
	},
};

DEFINE_CUSTOM_SCHEDULES(CVoltigore)
{
	slVoltigoreRangeAttack1,
	slVoltigoreVictoryEat,
};

IMPLEMENT_CUSTOM_SCHEDULES(CVoltigore, CBaseMonster);

Schedule_t *CVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
		case SCHED_RANGE_ATTACK1:
			return slVoltigoreRangeAttack1;
			break;

		case SCHED_VICTORY_DANCE:
			return slVoltigoreVictoryEat;
		break;
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

void CVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch (pTask->iTask)
	{
	/*case TASK_MELEE_ATTACK1:
		{
			CBaseMonster::StartTask(pTask);
		}
		break;*/
	case TASK_EAT_AND_HEAL:
		{
			if (pev->health < pev->max_health)
				pev->health += RANDOM_LONG(10, 20);

			if (pev->health > pev->max_health)
				pev->health = pev->max_health;

			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/voltigore_eat.wav", GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch());
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

void CVoltigore::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	switch (RANDOM_LONG(0,1))
	{
	case 0:
		{
			pev->takedamage = DAMAGE_NO;
			pev->solid = SOLID_NOT;
			SetThink(&CBaseEntity::SUB_Remove);
			Vector vecSrc = pev->origin + Vector(0,0,40);
			PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, vecSrc, pev->angles,  0.0, 0.0, MONFX_VOLTIGORE_ANIM_EFFECT, 1, 0, 0);
 			::RadiusDamage(vecSrc, this, this, gSkillData.voltigoreDmgBolt, gSkillData.voltigoreDmgBolt*3, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB | DMG_DONT_BLEED);
			//TODO: Move to client and draw with RenderSystem (R)
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_BREAKMODEL);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z);
				WRITE_COORD(96);
				WRITE_COORD(128);
				WRITE_COORD(192);
				WRITE_COORD(0);
				WRITE_COORD(0);
				WRITE_COORD(0);
				WRITE_BYTE(25);
				WRITE_SHORT(m_iGibModelIndex);
				WRITE_BYTE(m_iGibCount);
				WRITE_BYTE(50);
				WRITE_BYTE(BREAK_FLESH);
			MESSAGE_END();
			CBaseMonster::Killed(pInflictor, pAttacker, GIB_NEVER);
			break;
		}
		case 1:
		{
			pev->takedamage = DAMAGE_NO;
			pev->solid = SOLID_NOT;
			CBaseMonster::Killed(pInflictor, pAttacker, GIB_NORMAL);
			break;
		}
	}
}

//Baby voltigore

#define	BABY_VOLTIGORE_MELEE_DIST	40 
#define	BABY_VOLTIGORE_ZAP_DIST		115 

class CVoltigoreBaby : public CVoltigore
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	virtual BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	/*virtual ?!*/ CBaseEntity *BVCheckTraceHullAttack(float flDist, float fDamage, int iDmgType);
};
LINK_ENTITY_TO_CLASS( monster_voltigore_baby, CVoltigoreBaby );

// BAD CODE!!! NO BASE CLASS CALLS
void CVoltigoreBaby::Spawn()
{
	if (pev->health <= 0)
		pev->health = gSkillData.voltigoreHealth*VOLTIGORE_BABY_HEALTH_FACTOR;

	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_YELLOW;

	if (m_iGibCount == 0)
		m_iGibCount = 10;

	Precache();
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(this, Vector(-18, -8, 0), Vector(26, 24, 32));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->view_ofs		= Vector(0,0, 25);
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextThrowEbolt	= gpGlobals->time;
	MonsterInit();
}

void CVoltigoreBaby::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/baby_voltigore.mdl");

	CVoltigore::Precache();
}

void CVoltigoreBaby::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case AE_SLASH_LEFT:
		{
			AttackSound();
			CBaseEntity *pHurt = BVCheckTraceHullAttack(BABY_VOLTIGORE_MELEE_DIST, gSkillData.voltigoreDmgBite*VOLTIGORE_BABY_DMG_FACTOR, DMG_SLASH);
			if (pHurt)
			{
				pHurt->pev->punchangle.z = 15;
				pHurt->PunchPitchAxis(-7);
				AttackHitSound();
			}
			else
				AttackMissSound();
		}
		break;

	case AE_SLASH_BOTH:
		{
			AttackSound();
			CBaseEntity *pHurt = BVCheckTraceHullAttack(BABY_VOLTIGORE_MELEE_DIST, gSkillData.voltigoreDmgBite*(VOLTIGORE_BABY_DMG_FACTOR*2), DMG_SLASH );
			if (pHurt)
			{
				if (pHurt->pev->flags & (FL_MONSTER|FL_CLIENT))
				{
					pHurt->pev->punchangle.z = 25;
					pHurt->PunchPitchAxis(-15);
					pHurt->pev->velocity += gpGlobals->v_forward * -50;
				}
				AttackHitSound();
			}
			else 
			AttackMissSound();
		}
		break;

		case AE_SND_RANGE_ATT:
		{
			if (pev->dmgtime <= gpGlobals->time)
			{
				PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, pev->origin, pev->angles,  0.0, 0.0, MONFX_VOLTIGORE_ANIM_EFFECT, 0, 0, 0);
				m_flNextThrowEbolt = gpGlobals->time + 7.5;
				pev->dmgtime = gpGlobals->time + 2;
			}
		}
		break;

		case AE_THROW_EBOLT:
		{
			BVCheckTraceHullAttack(BABY_VOLTIGORE_ZAP_DIST, gSkillData.voltigoreDmgBolt*VOLTIGORE_BABY_DMG_FACTOR, DMG_SHOCK | DMG_DONT_BLEED);
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent(pEvent);
	}
}

int CVoltigoreBaby::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	PainSound();
	return CBaseMonster::TakeDamage( pInflictor, pAttacker, flDamage, bitsDamageType );
}

BOOL CVoltigoreBaby::CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= BABY_VOLTIGORE_MELEE_DIST && flDot >= 0.7 )
		return TRUE;

	return FALSE;
}

BOOL CVoltigoreBaby::CheckMeleeAttack2( float flDot, float flDist )
{
	if ( flDist <= BABY_VOLTIGORE_ZAP_DIST && flDot >= 0.7 && (gpGlobals->time > m_flNextThrowEbolt))
		return TRUE;

	return FALSE;
}

void CVoltigoreBaby::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	CBaseMonster::Killed(pInflictor, pAttacker, GIB_NORMAL);
}

CBaseEntity *CVoltigoreBaby::BVCheckTraceHullAttack(float flDist, float fDamage, int iDmgType)
{
	TraceResult tr;
	UTIL_MakeVectors(pev->angles);
	Vector vecStart(pev->origin);
	vecStart.z += 28;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) + (gpGlobals->v_up * flDist * 0.3);
	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if (fDamage > 0 )
			pEntity->TakeDamage(this, this, fDamage, iDmgType);

		return pEntity;
	}
	return NULL;
}
