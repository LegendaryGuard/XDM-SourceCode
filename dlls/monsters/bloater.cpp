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
// Bloater
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"// XDM


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define BLOATER_AE_ATTACK_MELEE1	( 1 )
#define BLOATER_AE_ATTACK_MELEE2	( 2 )
#define BLOATER_AE_ATTACK_RANGE1	( 3 )
#define BLOATER_AE_ATTACK_RANGE2	( 4 )

class CBloater : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);

	virtual void PainSound(void);
	virtual void AlertSound(void);
	virtual void IdleSound(void);
	virtual void AttackSound(void);
	virtual void DeathSound(void);

	// No range attacks
	virtual BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	virtual BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
//	int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_bloater, CBloater );

const char *CBloater::pIdleSounds[] = 
{
	"bloater/bl_idle1.wav",
	"bloater/bl_idle2.wav",
};
const char *CBloater::pAlertSounds[] = 
{
	"bloater/bl_alert1.wav",
};
const char *CBloater::pPainSounds[] = 
{
	"bloater/bl_pain1.wav",
	"bloater/bl_pain2.wav",
};
const char *CBloater::pAttackSounds[] = 
{
	"bloater/bl_attack1.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBloater :: Classify (void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;// XDM
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBloater :: SetYawSpeed (void)
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}
/*
int CBloater::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	PainSound();
	return CBaseMonster::TakeDamage( pInflictor, pAttacker, flDamage, bitsDamageType );
}
*/

void CBloater :: IdleSound (void)
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch() );
}

void CBloater :: AlertSound (void)
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch() );
}

void CBloater :: PainSound (void)
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch() );
}

void CBloater :: AttackSound(void)
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch() );
}

void CBloater :: DeathSound (void)
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bloater/bl_die.wav", GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch());
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBloater :: HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case BLOATER_AE_ATTACK_MELEE1:
		case BLOATER_AE_ATTACK_MELEE2:
		{
		}
		break;

		case BLOATER_AE_ATTACK_RANGE1:
		case BLOATER_AE_ATTACK_RANGE2:
		{
			// do stuff for this event.
			AttackSound();
			Vector vecAng, vecOrg;
			GetAttachment(atoi(pEvent->options), vecOrg, vecAng);
			CAGrenade::ShootTimed(vecOrg, vecAng, gpGlobals->v_forward * 800, this, this, 3.0, 0.0f, 0);
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBloater :: Spawn()
{
	if (pev->health <= 0)
		pev->health = gSkillData.controllerHealth * 0.8;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_YELLOW;
	//if (m_iScoreAward == 0)
	//	m_iScoreAward = gSkillData.bloaterScore;
	if (m_iGibCount == 0)
		m_iGibCount = 10;// XDM: medium one

	CBaseMonster::Spawn();// XDM3038b: Precache();
	//SET_MODEL(ENT(pev), STRING(pev->model));// XDM3038
	UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->spawnflags		|= FL_FLY;
	pev->view_ofs		= Vector(0, 0, -2);// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBloater :: Precache()
{
	if (FStringNull(pev->model))// XDM3038
		pev->model = MAKE_STRING("models/floater.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "bloater");

	CBaseMonster::Precache();// XDM3038a
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND("bloater/bl_die.wav");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// Chase enemy schedule
Task_t tlBloaterChaseEnemy[] = 
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)128		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},

};

Schedule_t slBloaterChaseEnemy[] =
{
	{ 
		tlBloaterChaseEnemy,
		ARRAYSIZE ( tlBloaterChaseEnemy ),
		bits_COND_NEW_ENEMY			|
		bits_COND_TASK_FAILED,
		0,
		"BloaterChaseEnemy"
	},
};



Task_t	tlBloaterStrafe[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_GET_PATH_TO_ENEMY,		(float)128					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slBloaterStrafe[] =
{
	{ 
		tlBloaterStrafe,
		ARRAYSIZE ( tlBloaterStrafe ), 
		bits_COND_NEW_ENEMY,
		0,
		"BloaterStrafe"
	},
};


Task_t	tlBloaterTakeCover[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slBloaterTakeCover[] =
{
	{ 
		tlBloaterTakeCover,
		ARRAYSIZE ( tlBloaterTakeCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"BloaterTakeCover"
	},
};


Task_t	tlBloaterFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slBloaterFail[] =
{
	{
		tlBloaterFail,
		ARRAYSIZE ( tlBloaterFail ),
		0,
		0,
		"BloaterFail"
	},
};



DEFINE_CUSTOM_SCHEDULES( CBloater )
{
	slBloaterChaseEnemy,
	slBloaterStrafe,
	slBloaterTakeCover,
	slBloaterFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBloater, CBaseMonster );
