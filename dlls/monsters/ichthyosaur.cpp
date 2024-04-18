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
// icthyosaur - evin, satan fish monster
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "flyingmonster.h"
#include "soundent.h"
#include "animation.h"
#include "effects.h"
#include "globals.h"

#define SEARCH_RETRY	16
#define ICHTHYOSAUR_SPEED 150

enum ichthyosaur_animation_events_e
{
	ICHTHYOSAUR_AE_SHAKE_RIGHT = 1,
	ICHTHYOSAUR_AE_SHAKE_LEFT
};

enum ichthyosaur_skins_e
{
	EYE_MAD = 0,
	EYE_BASE,
	EYE_CLOSED,
	EYE_BACK,
	EYE_LOOK
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

// UNDONE: Save/restore here
class CIchthyosaur : public CFlyingMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);

	virtual int	Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	virtual Schedule_t *GetSchedule(void);
	virtual Schedule_t *GetScheduleOfType(int Type);

	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);

	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);

	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);

	virtual float ChangeYaw(float yawSpeed);
	virtual Activity GetStoppedActivity(void);

	virtual void Move(float flInterval);
	virtual void MoveExecute(CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval);
	virtual void MonsterThink(void);
	virtual void Stop(void);

	virtual void IdleSound(void);
	virtual void AlertSound(void);
	virtual void AttackSound(void);
	virtual void DeathSound(void);
	virtual void PainSound(void);
	void BiteSound(void);

	void EXPORT CombatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BiteTouch(CBaseEntity *pOther);
	void Swim(void);
	Vector DoProbe(const Vector &Probe);

	float VectorToPitch( const Vector &vec);
	float FlPitchDiff(void);
	float ChangePitch( int speed );

	Vector m_SaveVelocity;
	float m_idealDist;
	float m_flBlink;

	float m_flEnemyTouched;
	BOOL  m_bOnAttack;

	float m_flMaxSpeed;
	float m_flMinSpeed;
	float m_flMaxDist;

//	CBeam *m_pBeam;

	float m_flNextAlert;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pDeathSounds[];
	static const char *pPainSounds[];

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_ichthyosaur, CIchthyosaur );

TYPEDESCRIPTION	CIchthyosaur::m_SaveData[] =
{
	DEFINE_FIELD( CIchthyosaur, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CIchthyosaur, m_idealDist, FIELD_FLOAT ),
	DEFINE_FIELD( CIchthyosaur, m_flBlink, FIELD_FLOAT ),
	DEFINE_FIELD( CIchthyosaur, m_flEnemyTouched, FIELD_FLOAT ),
	DEFINE_FIELD( CIchthyosaur, m_bOnAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CIchthyosaur, m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CIchthyosaur, m_flMinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CIchthyosaur, m_flMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CIchthyosaur, m_flNextAlert, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CIchthyosaur, CFlyingMonster );


const char *CIchthyosaur::pIdleSounds[] =
{
	"ichy/ichy_idle1.wav",
	"ichy/ichy_idle2.wav",
	"ichy/ichy_idle3.wav",
	"ichy/ichy_idle4.wav",
};

const char *CIchthyosaur::pAlertSounds[] =
{
	"ichy/ichy_alert2.wav",
	"ichy/ichy_alert3.wav",
};

const char *CIchthyosaur::pAttackSounds[] =
{
	"ichy/ichy_attack1.wav",
	"ichy/ichy_attack2.wav",
};

const char *CIchthyosaur::pBiteSounds[] =
{
	"ichy/ichy_bite1.wav",
	"ichy/ichy_bite2.wav",
};

const char *CIchthyosaur::pPainSounds[] =
{
	"ichy/ichy_pain2.wav",
	"ichy/ichy_pain3.wav",
	"ichy/ichy_pain5.wav",
};

const char *CIchthyosaur::pDeathSounds[] =
{
	"ichy/ichy_die2.wav",
	"ichy/ichy_die4.wav",
};

#define EMIT_ICKY_SOUND( chan, array ) \
	EMIT_SOUND_DYN ( ENT(pev), chan , array [ RANDOM_LONG(0,ARRAYSIZE( array )-1) ], GetSoundVolume(), 0.6, 0, RANDOM_LONG(95,105) );


void CIchthyosaur :: IdleSound(void)
{
	EMIT_ICKY_SOUND( CHAN_VOICE, pIdleSounds );
}

void CIchthyosaur :: AlertSound(void)
{
	EMIT_ICKY_SOUND( CHAN_VOICE, pAlertSounds );
}

void CIchthyosaur :: AttackSound(void)
{
	EMIT_ICKY_SOUND( CHAN_VOICE, pAttackSounds );
}

void CIchthyosaur :: BiteSound(void)
{
	EMIT_ICKY_SOUND( CHAN_WEAPON, pBiteSounds );
}

void CIchthyosaur :: DeathSound(void)
{
	EMIT_ICKY_SOUND( CHAN_VOICE, pDeathSounds );
}

void CIchthyosaur :: PainSound(void)
{
	EMIT_ICKY_SOUND( CHAN_VOICE, pPainSounds );
}

//=========================================================
// monster-specific tasks and states
//=========================================================
enum ichthyosaur_tasks_e
{
	TASK_ICHTHYOSAUR_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_ICHTHYOSAUR_SWIM,
	TASK_ICHTHYOSAUR_FLOAT,
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t	tlSwimAround[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_ICHTHYOSAUR_SWIM,		0.0 },
};

static Schedule_t	slSwimAround[] =
{
	{
		tlSwimAround,
		ARRAYSIZE(tlSwimAround),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY		|
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT,
		"SwimAround"
	},
};

static Task_t	tlSwimAgitated[] =
{
	{ TASK_STOP_MOVING,				(float) 0 },
	{ TASK_SET_ACTIVITY,			(float)ACT_RUN },
	{ TASK_WAIT,					(float)2.0 },
};

static Schedule_t	slSwimAgitated[] =
{
	{
		tlSwimAgitated,
		ARRAYSIZE(tlSwimAgitated),
		0,
		0,
		"SwimAgitated"
	},
};


static Task_t	tlCircleEnemy[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_ICHTHYOSAUR_CIRCLE_ENEMY, 0.0 },
};

static Schedule_t	slCircleEnemy[] =
{
	{
		tlCircleEnemy,
		ARRAYSIZE(tlCircleEnemy),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"CircleEnemy"
	},
};


Task_t tlTwitchDie[] =
{
	{ TASK_STOP_MOVING,			0		 },
	{ TASK_SOUND_DIE,			(float)0 },
	{ TASK_DIE,					(float)0 },
	{ TASK_ICHTHYOSAUR_FLOAT,	(float)0 },
};

Schedule_t slTwitchDie[] =
{
	{
		tlTwitchDie,
		ARRAYSIZE( tlTwitchDie ),
		0,
		0,
		"Die"
	},
};


DEFINE_CUSTOM_SCHEDULES(CIchthyosaur)
{
    slSwimAround,
	slSwimAgitated,
	slCircleEnemy,
	slTwitchDie,
};
IMPLEMENT_CUSTOM_SCHEDULES(CIchthyosaur, CFlyingMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CIchthyosaur :: Classify (void)
{
	// XDM3035c
	return m_iClass?m_iClass:CLASS_ALIEN_PREDATOR;// eat some crabs! CLASS_ALIEN_MONSTER;
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CIchthyosaur :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDot >= 0.7 && m_flEnemyTouched > gpGlobals->time - 0.2 )
		return TRUE;

	return FALSE;
}

void CIchthyosaur::BiteTouch(CBaseEntity *pOther)
{
	// bite if we hit who we want to eat
	if ( pOther == m_hEnemy )
	{
		m_flEnemyTouched = gpGlobals->time;
		m_bOnAttack = TRUE;
	}
}

void CIchthyosaur::CombatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_bOnAttack == TRUE ) )
		return;

	if (m_bOnAttack)
	{
		m_bOnAttack = 0;
	}
	else
	{
		m_bOnAttack = 1;
	}
}

//=========================================================
// CheckRangeAttack1  - swim in for a chomp
//
//=========================================================
BOOL CIchthyosaur :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > -0.7 && (m_bOnAttack || ( flDist <= 192 && m_idealDist <= 192)))
		return TRUE;

	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CIchthyosaur :: SetYawSpeed (void)
{
	pev->yaw_speed = 100;
}



//=========================================================
// Killed - overrides CFlyingMonster.
//
void CIchthyosaur::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	CBaseMonster::Killed( pAttacker, pInflictor, iGib );
	pev->velocity.Clear();
}
/*
void CIchthyosaur::BecomeDead(void)
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health.
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.
}
*/

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CIchthyosaur :: HandleAnimEvent(MonsterEvent_t *pEvent)
{
	int bDidAttack = FALSE;
	switch (pEvent->event)
	{
	case ICHTHYOSAUR_AE_SHAKE_RIGHT:
	case ICHTHYOSAUR_AE_SHAKE_LEFT:
		{
			if (m_hEnemy.Get() != NULL && FVisible(m_hEnemy))
			{
				CBaseEntity *pHurt = m_hEnemy;
				if (!pHurt)
					break;
				if (m_flEnemyTouched < gpGlobals->time - 0.2 && (m_hEnemy->BodyTarget( pev->origin ) - pev->origin).Length() > (32+16+32))
					break;

				UTIL_MakeAimVectors ( pev->angles );
				if (DotProduct(ShootAtEnemy(pev->origin), gpGlobals->v_forward) > 0.707)
				{
					m_bOnAttack = TRUE;
					pHurt->pev->punchangle.z = -18.0f;
					pHurt->PunchPitchAxis(-5.0f);
					pHurt->pev->velocity -= gpGlobals->v_right * 300;
					if (pHurt->IsPlayer())
					{
						pHurt->pev->angles.x += RANDOM_FLOAT( -35, 35 );
						pHurt->pev->angles.y += RANDOM_FLOAT( -90, 90 );
						pHurt->pev->angles.z = 0;
						pHurt->pev->fixangle = TRUE;
					}
					pHurt->TakeDamage( this, this, m_flDamage1, DMG_SLASH );
				}
			}
			BiteSound();
			bDidAttack = TRUE;
		}
		break;
	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}

	if (bDidAttack)
	{
		//Vector vecSrc = pev->origin + gpGlobals->v_forward * 32;
		//UTIL_Bubbles( vecSrc - Vector( 8, 8, 8 ), vecSrc + Vector( 8, 8, 8 ), 16 );
		FX_BubblesBox(pev->origin + gpGlobals->v_forward * 32, Vector(8,8,8), 16);// XDM3037
	}
}

//=========================================================
// Spawn
//=========================================================
void CIchthyosaur :: Spawn()
{
	if (pev->health <= 0)
		pev->health = gSkillData.ichthyosaurHealth;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_GREEN;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.ichthyosaurScore;
	if (m_iGibCount == 0)
		m_iGibCount = 18;// XDM: medium one
	if (m_flDamage1 == 0)// XDM3038c
		m_flDamage1 = gSkillData.ichthyosaurDmgShake;

	CFlyingMonster::Spawn();// XDM3038b: Precache( );

	UTIL_SetSize(this, Vector(-32, -32, -32), Vector(32, 32, 32));
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->view_ofs.Set( 0, 0, 16 );
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_RANGE_ATTACK1 | bits_CAP_SWIM;
	SetBits(pev->flags, FL_SWIM);
	SetFlyingSpeed( ICHTHYOSAUR_SPEED );
	SetFlyingMomentum( 2.5 );	// Set momentum constant

	MonsterInit();

	SetTouch(&CIchthyosaur::BiteTouch);
	SetUse(&CIchthyosaur::CombatUse);

	m_idealDist = 384;
	m_flMinSpeed = 80;
	m_flMaxSpeed = 300;
	m_flMaxDist = 384;

	Vector Forward;
	AngleVectors(pev->angles, Forward, NULL, NULL);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CIchthyosaur :: Precache()
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/icky.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "ichy");

	CFlyingMonster::Precache();// XDM3038a

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CIchthyosaur::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = 80;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_ALERT:
		m_flightSpeed = 150;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = 400;
		// eat them
		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		// chase them down and eat them
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_CHASE_ENEMY );
		}
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		{
			m_bOnAttack = TRUE;
		}
		if ( pev->health < pev->max_health - 20 )
		{
			m_bOnAttack = TRUE;
		}

		return GetScheduleOfType( SCHED_STANDOFF );
	}
	return CFlyingMonster :: GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CIchthyosaur :: GetScheduleOfType ( int Type )
{
	// ALERT( at_console, "GetScheduleOfType( %d ) %d\n", Type, m_bOnAttack );
	switch	( Type )
	{
	case SCHED_IDLE_WALK:
		return slSwimAround;
	case SCHED_STANDOFF:
		return slCircleEnemy;
	case SCHED_FAIL:
		return slSwimAgitated;
	case SCHED_DIE:
		return slTwitchDie;
	case SCHED_CHASE_ENEMY:
		AttackSound( );
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}



//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CIchthyosaur::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ICHTHYOSAUR_CIRCLE_ENEMY:
		break;
	case TASK_ICHTHYOSAUR_SWIM:
		break;
	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		else
		{
			m_bOnAttack = TRUE;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_ICHTHYOSAUR_FLOAT:
		pev->skin = EYE_BASE;
		SetSequenceByName( "bellyup" );
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CIchthyosaur :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ICHTHYOSAUR_CIRCLE_ENEMY:
		if (m_hEnemy.Get() == NULL)
		{
			TaskComplete( );
		}
		else if (FVisible( m_hEnemy ))
		{
			Vector vecFrom = m_hEnemy->EyePosition( );
			Vector vecDelta = (pev->origin - vecFrom).Normalize( );
			Vector vecSwim = CrossProduct( vecDelta, Vector( 0, 0, 1 ) ).Normalize( );

			if (DotProduct( vecSwim, m_SaveVelocity ) < 0)
				vecSwim = vecSwim * -1.0;

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecSwim * 32;

			// ALERT( at_console, "vecPos %.0f %.0f %.0f\n", vecPos.x, vecPos.y, vecPos.z );

			TraceResult tr;
			UTIL_TraceHull( vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr );

			if (tr.flFraction > 0.5)
				vecPos = tr.vecEndPos;

			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * (vecPos - pev->origin).Normalize() * m_flightSpeed;

			// ALERT( at_console, "m_SaveVelocity %.2f %.2f %.2f\n", m_SaveVelocity.x, m_SaveVelocity.y, m_SaveVelocity.z );

			if (HasConditions( bits_COND_ENEMY_FACING_ME ) && m_hEnemy->FVisible( this ))
			{
				m_flNextAlert -= 0.1;

				if (m_idealDist < m_flMaxDist)
					m_idealDist += 4;

				if (m_flightSpeed > m_flMinSpeed)
					m_flightSpeed -= 2;
				else if (m_flightSpeed < m_flMinSpeed)
					m_flightSpeed += 2;

				if (m_flMinSpeed < m_flMaxSpeed)
					m_flMinSpeed += 0.5;
			}
			else
			{
				m_flNextAlert += 0.1;

				if (m_idealDist > 128)
					m_idealDist -= 4;

				if (m_flightSpeed < m_flMaxSpeed)
					m_flightSpeed += 4;
			}
			// ALERT( at_console, "%.0f\n", m_idealDist );
		}
		else
		{
			m_flNextAlert = gpGlobals->time + 0.2;
		}

		if (m_flNextAlert < gpGlobals->time)
		{
			// ALERT( at_console, "AlertSound()\n");
			AlertSound( );
			m_flNextAlert = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
		}

		break;
	case TASK_ICHTHYOSAUR_SWIM:
		if (m_fSequenceFinished)
		{
			TaskComplete( );
		}
		break;
	case TASK_DIE:
		if ( m_fSequenceFinished )
		{
			pev->deadflag = DEAD_DEAD;
			TaskComplete();
			BecomeDead();// XDM3038c
		}
		break;

	case TASK_ICHTHYOSAUR_FLOAT:
		pev->angles.x = UTIL_ApproachAngle( 0, pev->angles.x, 20 );
		pev->velocity *= 0.8;
		if (pev->waterlevel > 1 && pev->velocity.z < 64)
		{
			pev->velocity.z += 8;
		}
		else
		{
			pev->velocity.z -= 8;
		}
		// ALERT( at_console, "%f\n", pev->velocity.z );
		break;

	default:
		CFlyingMonster :: RunTask ( pTask );
		break;
	}
}



float CIchthyosaur::VectorToPitch( const Vector &vec )
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
#if defined (NOSQB)
		pitch = RAD2DEG(-atan2(vec.z, sqrt(vec.x*vec.x+vec.y*vec.y)));// FIXED: SQB
#else
		pitch = (int) (atan2(vec.z, sqrt(vec.x*vec.x+vec.y*vec.y)) * 180 / M_PI);
#endif
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

//=========================================================
void CIchthyosaur::Move(float flInterval)
{
	CFlyingMonster::Move( flInterval );
}

float CIchthyosaur::FlPitchDiff(void)
{
	float flCurrentPitch = UTIL_AngleMod( pev->angles.z );
	if (flCurrentPitch == pev->idealpitch)
		return 0;

	float flPitchDiff = pev->idealpitch - flCurrentPitch;
	if ( pev->idealpitch > flCurrentPitch )
	{
		if (flPitchDiff >= 180)
			flPitchDiff -= 360;
	}
	else
	{
		if (flPitchDiff <= -180)
			flPitchDiff += 360;
	}
	return flPitchDiff;
}

float CIchthyosaur :: ChangePitch( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlPitchDiff();
		float target = 0;
		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}
		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * 0.1 );
	}
	return 0;
}

float CIchthyosaur::ChangeYaw(float yawSpeed)
{
	if (pev->movetype == MOVETYPE_FLY)
	{
		float diff = FlYawDiff();
		float target = 0;
		if (m_IdealActivity != GetStoppedActivity())
		{
			if (diff < -20)
				target = 20;
			else if (diff > 20)
				target = -20;
		}
		pev->angles.z = UTIL_Approach(target, pev->angles.z, 220.0 * 0.1);
	}
	return CFlyingMonster::ChangeYaw( yawSpeed );
}

Activity CIchthyosaur:: GetStoppedActivity(void)
{
	if (pev->movetype != MOVETYPE_FLY)// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;

	return ACT_WALK;
}

void CIchthyosaur::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}

void CIchthyosaur::MonsterThink (void)
{
	CFlyingMonster::MonsterThink( );

	if (pev->deadflag == DEAD_NO)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			Swim( );

			// blink the eye
			if (m_flBlink < gpGlobals->time)
			{
				pev->skin = EYE_CLOSED;
				if (m_flBlink + 0.2 < gpGlobals->time)
				{
					m_flBlink = gpGlobals->time + RANDOM_FLOAT( 3, 4 );
					if (m_bOnAttack)
						pev->skin = EYE_MAD;
					else
						pev->skin = EYE_BASE;
				}
			}
		}
		if ((pev->waterlevel < 1) && (pev->dmgtime < gpGlobals->time))// XDM: live only in water!
		{
			TakeDamage(g_pWorld, g_pWorld, 4, DMG_GENERIC|DMG_DROWN);
			pev->dmgtime = gpGlobals->time + 1;
		}
	}
}

void CIchthyosaur :: Stop(void)
{
	if (!m_bOnAttack)
		m_flightSpeed = 80.0;
}

void CIchthyosaur::Swim( )
{
//	int retValue = 0;

	Vector Angles;
	Vector Forward, Right, Up;
	Vector start(pev->origin);

	if (FBitSet( pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

#if defined (NOSQB)
		Angles.Set(pev->angles.x, pev->angles.y, pev->angles.z);
#else
		Angles.Set( -pev->angles.x, pev->angles.y, pev->angles.z );
#endif
		AngleVectors(Angles, Forward, Right, Up);
		pev->velocity = Forward * 200 + Up * 200;
		return;
	}

	if (m_bOnAttack && m_flightSpeed < m_flMaxSpeed)
	{
		m_flightSpeed += 40;
	}
	if (m_flightSpeed < 180)
	{
		if (m_IdealActivity == ACT_RUN)
			SetActivity( ACT_WALK );
		if (m_IdealActivity == ACT_WALK)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		if (m_IdealActivity == ACT_WALK)
			SetActivity( ACT_RUN );
		if (m_IdealActivity == ACT_RUN)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}

/*
	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
		m_pBeam->PointEntInit( pev->origin + m_SaveVelocity, entindex( ) );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetColor( 255, 180, 96 );
		m_pBeam->SetBrightness( 192 );
	}
*/
#define PROBE_LENGTH 150
	VectorAngles(m_SaveVelocity, Angles);//Angles = UTIL_VecToAngles( m_SaveVelocity );
#if !defined (NOSQB)
	Angles.x = -Angles.x;
#endif
	AngleVectors(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector/2).Normalize();

	Angles.Set( -pev->angles.x, pev->angles.y, pev->angles.z );
	AngleVectors(Angles, Forward, Right, Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	vec_t flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5f);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 80.0f;

	// ALERT( at_console, "%.0f %.0f\n", m_flightSpeed, pev->velocity.Length() );


	// ALERT( at_console, "Steer %f %f %f\n", SteeringVector.x, SteeringVector.y, SteeringVector.z );

/*
	m_pBeam->SetStartPos( pev->origin + pev->velocity );
	m_pBeam->RelinkBeam( );
*/

	// ALERT( at_console, "speed %f\n", m_flightSpeed );

	VectorAngles(m_SaveVelocity, Angles);//Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x -= 360;

	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1 );
	if (pev->angles.x < -80) pev->angles.x = -80;
	if (pev->angles.x >  80) pev->angles.x =  80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, pev->angles.y );

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	else if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))// XDM3038c: +else
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	else if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))// XDM3038c: +else
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
			turn = -speed;
		else
			turn = speed;
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	// ALERT( at_console, "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	else if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))// XDM3038c: +else
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	else if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))// XDM3038c: +else
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed/2 * 0.1;
	//float flTempRoll = pev->angles.z;// test
	if (fabs(turn) < speed)
	{
		//flTempRoll += turn;
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			//flTempRoll -= speed;
			pev->angles.z -= speed;
		}
		else
		{
			//flTempRoll += speed;
			pev->angles.z += speed;
		}
	}
//?	pev->angles.z = UTIL_Approach(flTempRoll, pev->angles.z, 5);

	if (pev->angles.z < -20) pev->angles.z = -20;
	else if (pev->angles.z > 20) pev->angles.z =  20;
/*
#if defined (NOSQB)
	AngleVectors(Angles, Forward, Right, Up);
#else
	ANGLE_VECTORS( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up);
#endif*/
	// UTIL_MoveToOrigin ( ENT(pev), pev->origin + Forward * speed, speed, MOVE_STRAFE );
}

Vector CIchthyosaur::DoProbe(const Vector &Probe)
{
	float frac;
	BOOL bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);
	Vector WallNormal(0,0,-1); // WATER normal is Straight Down for fish.
	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if ( tr.fAllSolid || tr.flFraction < 0.99 )
	{
		if (tr.flFraction < 0.0) tr.flFraction = 0.0;
		else if (tr.flFraction > 1.0) tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = TRUE;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy.Get() == NULL || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;
		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1-frac) * (DotProduct(WallNormal.Normalize(), m_SaveVelocity.Normalize()));
		if (SteeringForce < 0.0)
			SteeringForce = -SteeringForce;

		SteeringVector.SetLength(SteeringForce);
		return SteeringVector;
	}
	return g_vecZero;
}
