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
// bullsquid - big, spotty tentacle-mouthed meanie.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"gamerules.h"
#include	"weapons.h"

#define		SQUID_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

//=========================================================
// monster-specific schedule types
//=========================================================
enum bullsquid_schedules_e
{
	SCHED_SQUID_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_SQUID_SMELLFOOD,
	SCHED_SQUID_SEECRAB,
	SCHED_SQUID_EAT,
	SCHED_SQUID_SNIFF_AND_EAT,
	SCHED_SQUID_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum bullsquid_tasks_e
{
	TASK_SQUID_HOPTURN = LAST_COMMON_TASK + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
enum bullsquid_animation_events_e
{
	BSQUID_AE_SPIT = 1,
	BSQUID_AE_BITE,
	BSQUID_AE_BLINK,
	BSQUID_AE_TAILWHIP,
	BSQUID_AE_HOP,
	BSQUID_AE_THROW
};

class CBullsquid : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int ISoundMask(void);
	virtual int Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void IdleSound(void);
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void AlertSound(void);
	virtual void AttackSound(void);
	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual void RunAI(void);
	virtual bool FValidateHintType(short sHint);
	virtual Schedule_t *GetSchedule(void);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int IRelationship(CBaseEntity *pTarget);
	virtual int IgnoreConditions(void);
	virtual MONSTERSTATE GetIdealState(void);
	virtual int	Save(CSave &save);
	virtual int Restore(CRestore &restore);
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 
	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
	int m_iSquidSpitSprite;
	static const char *pAttackSounds[];
	static const char *pAttackGrowlSounds[];
	static const char *pAttackHitSounds[];
	static const char *pIdleSounds[];
	//static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static short sAIHints[];// XDM3038c
};
LINK_ENTITY_TO_CLASS( monster_bullchicken, CBullsquid );

TYPEDESCRIPTION	CBullsquid::m_SaveData[] = 
{
	DEFINE_FIELD( CBullsquid, m_fCanThreatDisplay, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBullsquid, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CBullsquid, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBullsquid, CBaseMonster );

const char *CBullsquid::pAttackSounds[] = 
{
	//"bullchicken/bc_attack1.wav",
	"bullchicken/bc_attack2.wav",
	"bullchicken/bc_attack3.wav",
};

const char *CBullsquid::pAttackGrowlSounds[] = 
{
	"bullchicken/bc_attackgrowl.wav",
	"bullchicken/bc_attackgrowl2.wav",
	"bullchicken/bc_attackgrowl3.wav",
};

const char *CBullsquid::pAttackHitSounds[] = 
{
	//"bullchicken/bc_bite1.wav",
	"bullchicken/bc_bite2.wav",
	"bullchicken/bc_bite3.wav",
};

const char *CBullsquid::pIdleSounds[] = 
{
	"bullchicken/bc_idle1.wav",
	"bullchicken/bc_idle2.wav",
	"bullchicken/bc_idle3.wav",
	"bullchicken/bc_idle4.wav",
	"bullchicken/bc_idle5.wav",
};

const char *CBullsquid::pPainSounds[] = 
{
	"bullchicken/bc_pain1.wav",
	"bullchicken/bc_pain2.wav",
	"bullchicken/bc_pain3.wav",
	"bullchicken/bc_pain4.wav",
};

const char *CBullsquid::pDeathSounds[] = 
{
	"bullchicken/bc_die1.wav",
	"bullchicken/bc_die2.wav",
	"bullchicken/bc_die3.wav",
};

// XDM3038c
short CBullsquid::sAIHints[] =
{
	HINT_WORLD_HUMAN_BLOOD,
	HINT_WORLD_ALIEN_BLOOD
};


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBullsquid::Classify(void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_PREDATOR;// XDM
}

//=========================================================
// Spawn
//=========================================================
void CBullsquid::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.bullsquidHealth;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_YELLOW;// XDM
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.bullsquidScore;
	if (m_iGibCount == 0)
		m_iGibCount = 16;// XDM: medium one
	if (m_flDamage1 == 0)// XDM3038c
		m_flDamage1 = gSkillData.bullsquidDmgBite;
	if (m_flDamage2 == 0)// XDM3038c
		m_flDamage2 = gSkillData.bullsquidDmgWhip;

	CBaseMonster::Spawn();// XDM3038b: Precache();
	UTIL_SetSize(this, Vector(-32, -32, 0), Vector(32, 32, 48));// XDM3038c: changed height from 64 to 48
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime	= gpGlobals->time;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBullsquid::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/bullsquid.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "bullchicken");

	CBaseMonster::Precache();// XDM3038a
	// XDM3034	UTIL_PrecacheOther("a_grenade");
	m_iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
	// XDM3038c: arrays and macros
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackGrowlSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	//PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CBullsquid::IgnoreConditions (void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	if ( m_hEnemy != NULL )
	{
		if ( FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) )
		{
			// (Unless after a tasty headcrab)
			iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
		}
	}


	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CBullsquid::IRelationship(CBaseEntity *pTarget)
{
	if ((gpGlobals->time - m_flLastHurtTime) < 5 && FClassnameIs(pTarget->pev, "monster_headcrab"))
	{
		// if squid has been hurt in the last 5 seconds, and is getting relationship for a headcrab, 
		// tell squid to disregard crab. 
		return R_NO;
	}

	return CBaseMonster :: IRelationship ( pTarget );
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CBullsquid::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pAttacker == this)// XDM3034: a_grenade hurts it's owner! OR: use entindex for comparison?
		return 0;

	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if ( m_hEnemy != NULL && IsMoving() && pAttacker == m_hEnemy && gpGlobals->time - m_flLastHurtTime > 3 )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();
		
		if ( flDist > SQUID_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[ m_iRouteIndex ].vecLocation ).Length2D();// reusing flDist. 

			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist * 0.5, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}

	if (pAttacker && pAttacker->Classify() == CLASS_ALIEN_PREY)// XDM3034
	{
		// don't forget about headcrabs if it was a headcrab that hurt the squid.
		m_flLastHurtTime = gpGlobals->time;
	}
	return CBaseMonster :: TakeDamage ( pInflictor, pAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBullsquid :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CBullsquid :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy->pev->health <= m_flDamage2 && flDist <= 85 && flDot >= 0.7 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CBullsquid :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 85 && flDot >= 0.7 && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )		// The player & bullsquid can be as much as their bboxes 
	{										// apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}
	return FALSE;
}

//=========================================================
//  FValidateHintType 
//=========================================================
bool CBullsquid :: FValidateHintType ( short sHint )
{
	for (size_t i = 0; i < ARRAYSIZE(sAIHints); ++i)
	{
		if (sAIHints[i] == sHint)
			return true;
	}
	return CBaseMonster::FValidateHintType(sHint);// XDM3038c
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBullsquid :: ISoundMask (void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

#define SQUID_ATTN_IDLE	(float)1.5

void CBullsquid::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pIdleSounds, SQUID_ATTN_IDLE, GetSoundPitch());
}

void CBullsquid::PainSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pPainSounds, ATTN_NORM, GetSoundPitch()+RANDOM_LONG(-15,15));
}

void CBullsquid::AlertSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0,1)], GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch()+RANDOM_LONG(40,60));
}

void CBullsquid::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

void CBullsquid::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pAttackSounds, ATTN_NORM, GetSoundPitch()+RANDOM_LONG(-15,15));
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBullsquid :: SetYawSpeed (void)
{
	int ys = 0;
	switch ( m_Activity )
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 80;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}
	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBullsquid :: HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
		case BSQUID_AE_SPIT:
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			//Vector aAng;
			//TODO: need a model	GetAttachment(0, vecSpitOffset, aAng);
			vecSpitOffset = ( gpGlobals->v_right * 8.0f + gpGlobals->v_forward * 37.0f + gpGlobals->v_up * 23.0f );
			vecSpitOffset = ( pev->origin + vecSpitOffset );

			if (m_hEnemy)// XDM3034
				vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();
			else
				vecSpitDir = gpGlobals->v_forward;

			vecSpitDir.x += RANDOM_FLOAT(-0.01, 0.01);
			vecSpitDir.y += RANDOM_FLOAT(-0.01, 0.01);
			vecSpitDir.z += RANDOM_FLOAT(0, 0.01);// XDM3035: a_grenades have gravity.

			// do stuff for this event.
			AttackSound();

			if (g_pGameRules->FAllowEffects())
			{
			// spew the spittle temporary ents.
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSpitOffset);
				WRITE_BYTE(TE_SPRITE_SPRAY);
				WRITE_COORD(vecSpitOffset.x);	// pos
				WRITE_COORD(vecSpitOffset.y);	
				WRITE_COORD(vecSpitOffset.z);	
				WRITE_COORD(vecSpitDir.x);	// dir
				WRITE_COORD(vecSpitDir.y);	
				WRITE_COORD(vecSpitDir.z);	
				WRITE_SHORT(m_iSquidSpitSprite);	// model
				WRITE_BYTE(12);			// count
				WRITE_BYTE(210);			// speed
				WRITE_BYTE(25);			// noise ( client will divide by 100 )
			MESSAGE_END();
			}
			// XDM3034
			CAGrenade *pSpit = CAGrenade::ShootTimed(vecSpitOffset, pev->angles, vecSpitDir * 1100.0f, this, this, 6.0f, 0.0f, (int)sv_overdrive.value);
			if (pSpit)
			{
				pSpit->pev->gravity = 0.01f;
				pSpit->pev->movetype = MOVETYPE_FLY;
			}
			//CSquidSpit::Shoot( pev, vecSpitOffset, vecSpitDir * 900 );
		}
		break;

		case BSQUID_AE_BITE:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack(64, m_flDamage1, DMG_SLASH );
			if ( pHurt )
			{
				pHurt->pev->velocity -= gpGlobals->v_forward * 100.0f;
				pHurt->pev->velocity += gpGlobals->v_up * 100.0f;
			}
		}
		break;

		case BSQUID_AE_TAILWHIP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack(64, m_flDamage2, DMG_CLUB | DMG_ALWAYSGIB );
			if ( pHurt ) 
			{
				pHurt->pev->punchangle.z = -20.0f;
				pHurt->PunchPitchAxis(-20.0f);
				pHurt->pev->velocity += gpGlobals->v_right * 200.0f;
				pHurt->pev->velocity += gpGlobals->v_up * 100.0f;
			}
		}
		break;

		case BSQUID_AE_BLINK:
		{
			// close eye. 
			pev->skin = 1;
		}
		break;

		case BSQUID_AE_HOP:
		{
			float flGravity = g_psv_gravity->value;
			// throw the squid up into the air on this frame.
			if (FBitSet(pev->flags, FL_ONGROUND))
				ClearBits(pev->flags, FL_ONGROUND);

			// jump into air for 0.8 (24/30) seconds
			//pev->velocity.z += (0.875 * flGravity) * 0.5;
			pev->velocity.z += (0.625 * flGravity) * 0.5;
		}
		break;

		case BSQUID_AE_THROW:
			{
				// squid throws its prey IF the prey is a client. 
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, 0, 0 );
				if ( pHurt )
				{
					// croonchy bite sound
					EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch()+RANDOM_LONG(-10,10));
					//pHurt->PunchPitchAxis(RANDOM_FLOAT(-20,5));
					//pHurt->pev->punchangle.z = RANDOM_LONG(0,49) - 25;
					//pHurt->pev->punchangle.y = RANDOM_LONG(0,89) - 45;

					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->pev->origin, 25.0, 1.5, 0.7, 2 );

					if ( pHurt->IsPlayer() )
					{
						UTIL_MakeVectors( pev->angles );
						pHurt->pev->velocity += gpGlobals->v_forward * 300 + gpGlobals->v_up * 300;
					}
				}
			}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CBullsquid :: RunAI (void)
{
	// first, do base class stuff
	CBaseMonster :: RunAI();

	if (pev->skin != 0)
		pev->skin = 0;// close eye if it was open.

	if ( RANDOM_LONG(0,39) == 0 )
		pev->skin = 1;

	if ( m_hEnemy != NULL && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( (pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST )
		{
			pev->framerate = 1.25;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSquidRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSquidRangeAttack1[] =
{
	{ 
		tlSquidRangeAttack1,
		ARRAYSIZE ( tlSquidRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Squid Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlSquidChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slSquidChaseEnemy[] =
{
	{ 
		tlSquidChaseEnemy1,
		ARRAYSIZE ( tlSquidChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"Squid Chase Enemy"
	},
};

Task_t tlSquidHurtHop[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0		},
	{ TASK_SQUID_HOPTURN,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},// in case squid didn't turn all the way in the air.
};

Schedule_t slSquidHurtHop[] =
{
	{
		tlSquidHurtHop,
		ARRAYSIZE ( tlSquidHurtHop ),
		0,
		0,
		"SquidHurtHop"
	}
};

Task_t tlSquidSeeCrab[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXCITED	},
	{ TASK_FACE_ENEMY,			(float)0			},
};

Schedule_t slSquidSeeCrab[] =
{
	{
		tlSquidSeeCrab,
		ARRAYSIZE ( tlSquidSeeCrab ),
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"SquidSeeCrab"
	}
};

// squid walks to something tasty and eats it.
Task_t tlSquidEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSquidEat[] =
{
	{
		tlSquidEat,
		ARRAYSIZE( tlSquidEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SquidEat"
	}
};

// This is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind the squid.
// This schedule plays a sniff animation before going to the source of food.
Task_t tlSquidSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSquidSniffAndEat[] =
{
	{
		tlSquidSniffAndEat,
		ARRAYSIZE( tlSquidSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SquidSniffAndEat"
	}
};

// squid does this to stinky things. 
Task_t tlSquidWallow[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the squid can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_INSPECT_FLOOR},
	{ TASK_EAT,						(float)50				},// keeps squid from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSquidWallow[] =
{
	{
		tlSquidWallow,
		ARRAYSIZE( tlSquidWallow ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,
		"SquidWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES( CBullsquid ) 
{
	slSquidRangeAttack1,
	slSquidChaseEnemy,
	slSquidHurtHop,
	slSquidSeeCrab,
	slSquidEat,
	slSquidSniffAndEat,
	slSquidWallow
};

IMPLEMENT_CUSTOM_SCHEDULES( CBullsquid, CBaseMonster );

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CBullsquid :: GetSchedule(void)
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
				return GetScheduleOfType ( SCHED_SQUID_HURTHOP );

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound *pSound = PBestScent();
				if ( pSound && (!FInViewCone (pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )// scent is behind or occluded
					return GetScheduleOfType( SCHED_SQUID_SNIFF_AND_EAT );

				return GetScheduleOfType( SCHED_SQUID_EAT );// food is right out in the open. Just go get it.
			}

			if ( HasConditions(bits_COND_SMELL) )
			{
				// there's something stinky. 
				CSound *pSound = PBestScent();
				if ( pSound )
					return GetScheduleOfType( SCHED_SQUID_WALLOW);
			}

			break;
		}
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( m_fCanThreatDisplay && IRelationship( m_hEnemy ) == R_HT )
				{
					// this means squid sees a headcrab!
					m_fCanThreatDisplay = FALSE;// only do the headcrab dance once per lifetime.
					return GetScheduleOfType ( SCHED_SQUID_SEECRAB );
				}
				else
					return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound *pSound = PBestScent();
				if ( pSound && (!FInViewCone (pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )// scent is behind or occluded
					return GetScheduleOfType( SCHED_SQUID_SNIFF_AND_EAT );

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_SQUID_EAT );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
			else if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_CHASE_ENEMY );

			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CBullsquid :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slSquidRangeAttack1[ 0 ];
		break;
	case SCHED_SQUID_HURTHOP:
		return &slSquidHurtHop[ 0 ];
		break;
	case SCHED_SQUID_SEECRAB:
		return &slSquidSeeCrab[ 0 ];
		break;
	case SCHED_SQUID_EAT:
		return &slSquidEat[ 0 ];
		break;
	case SCHED_SQUID_SNIFF_AND_EAT:
		return &slSquidSniffAndEat[ 0 ];
		break;
	case SCHED_SQUID_WALLOW:
		return &slSquidWallow[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		return &slSquidChaseEnemy[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBullsquid :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK1:// XDM3037
		{
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, pAttackGrowlSounds[0], GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch()+RANDOM_LONG(-10,10));
			CBaseMonster::StartTask(pTask);
			break;
		}
	case TASK_MELEE_ATTACK2:
		{
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, pAttackGrowlSounds[RANDOM_LONG(1,2)], GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch()+RANDOM_LONG(-10,10));
			CBaseMonster::StartTask(pTask);
			break;
		}
	case TASK_SQUID_HOPTURN:
		{
			SetActivity ( ACT_HOP );
			MakeIdealYaw ( m_vecEnemyLKP );
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if ( BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				//SPAM	ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CBullsquid :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SQUID_HOPTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );
			if ( m_fSequenceFinished )
				m_iTaskStatus = TASKSTATUS_COMPLETE;

			break;
		}
	default:
		{
			CBaseMonster::RunTask(pTask);
			break;
		}
	}
}

//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CBullsquid :: GetIdealState (void)
{
	int	iConditions = IScheduleFlags();
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
		{
			if ( m_hEnemy != NULL && ( iConditions & bits_COND_LIGHT_DAMAGE || iConditions & bits_COND_HEAVY_DAMAGE ) && FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) )
			{
				// if the squid has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
				m_hEnemy = NULL;
				m_IdealMonsterState = MONSTERSTATE_ALERT;
			}
			break;
		}
	}
	m_IdealMonsterState = CBaseMonster :: GetIdealState();
	return m_IdealMonsterState;
}
