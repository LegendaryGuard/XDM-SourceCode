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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"gamerules.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
enum barney_animation_events_e
{
	BARNEY_AE_DRAW = 2,
	BARNEY_AE_SHOOT,
	BARNEY_AE_HOLSTER
};

enum barney_bodies_e
{
	BARNEY_BODY_GUNHOLSTERED = 0,
	BARNEY_BODY_GUNDRAWN,
	BARNEY_BODY_GUNGONE
};

class CBarney : public CTalkMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int ISoundMask(void);
	virtual void AlertSound(void);
	virtual int Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void RunTask(Task_t *pTask);
	//virtual void StartTask(Task_t *pTask);
	virtual int	ObjectCaps(void) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	void BarneyFirePistol(void);
	virtual void DeclineFollowing(void);
	// Override these to set behavior
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual Schedule_t *GetSchedule (void);
	virtual MONSTERSTATE GetIdealState(void);

	virtual void DeathSound(void);
	virtual void PainSound(void);
	virtual void TalkInit(void);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	//virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual bool HasHumanGibs(void) { return (BloodColor() == BLOOD_COLOR_RED); }// XDM3035c: zombie mod support :)

	virtual const char *GetDropItemName(void) { return "weapon_9mmhandgun"; }// XDM3038
	virtual CBaseEntity *DropItem(const char *pszItemName, const Vector &vecPos, const Vector &vecAng);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	static TYPEDESCRIPTION m_SaveData[];
	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;
	// UNDONE: What is this for?  It isn't used?
	float m_flPlayerDamage;// how much pain has the player inflicted on me?
	int m_iShell;
	CUSTOM_SCHEDULES;
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};

const char *CBarney::pPainSounds[] = 
{
	"barney/ba_pain1.wav",
	"barney/ba_pain2.wav",
	"barney/ba_pain3.wav",
};

const char *CBarney::pDeathSounds[] = 
{
	"barney/ba_die1.wav",
	"barney/ba_die2.wav",
	"barney/ba_die3.wav",
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney );

TYPEDESCRIPTION	CBarney::m_SaveData[] =
{
	DEFINE_FIELD( CBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBarney, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE ( tlBaFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] =
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE ( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE ( tlBaFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t	tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBaStand[] =
{
	{
		tlIdleBaStand,
		ARRAYSIZE ( tlIdleBaStand ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBarney )
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};


IMPLEMENT_CUSTOM_SCHEDULES( CBarney, CTalkMonster );

/*void CBarney :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );
}*/

void CBarney :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CBarney :: ISoundMask ( void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CBarney :: Classify (void)
{
	if (!HasMemory(bits_MEMORY_KILLED))// XDM3037: IsAlive() will return true right away, and we need to add score
		return m_iClass?m_iClass:CLASS_PLAYER_ALLY;
	else
		return CLASS_GIB;// XDM3035b: TESTME
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney :: AlertSound(void)
{
	if (m_hEnemy != NULL && FOkToSpeak())
		PlaySentence("ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney :: SetYawSpeed (void)
{
	int ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
	case ACT_TURN_LEFT:// XDM3038c
	case ACT_TURN_RIGHT:
		ys = 100;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBarney :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney :: BarneyFirePistol (void)
{
	Vector vecShootOrigin;
	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	Vector angDir;// = UTIL_VecToAngles(vecShootDir);
	VectorAngles(vecShootDir, angDir);
#if defined (NOSQB)
	SetBlending(0, -angDir.x);
#else
	SetBlending(0, angDir.x);
#endif
	pev->effects |= EF_MUZZLEFLASH;
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, NULL, 1024, BULLET_9MM, 0, this, this/*m_hActivator*/, 0);

	if (g_pGameRules->FAllowEffects())
	{
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,80) + gpGlobals->v_up * RANDOM_FLOAT(75,180) + gpGlobals->v_forward * RANDOM_FLOAT(-32, 32);
		EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, SHELL_BRASS, TE_BOUNCE_SHELL);
	}

	int pitchShift = RANDOM_LONG(0, 20);
	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", VOL_NORM, ATTN_NORM, 0, 100 + pitchShift);// XDM: sound precached in CWeaponGlock::Precache()

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarney :: HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;

	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		pev->body = BARNEY_BODY_GUNDRAWN;
		m_fGunDrawn = TRUE;
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		pev->body = BARNEY_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.barneyHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = 20;// XDM
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.barneyScore;
	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;// XDM: medium one

	CTalkMonster::Spawn();// XDM3038b: Precache();

	//SET_MODEL(ENT(pev), STRING(pev->model));// XDM3037
	//UTIL_SetOrigin(this, pev->origin);// XDM3038b
	UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid			= SOLID_BBOX;//???SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->view_ofs.Set(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->body			= BARNEY_BODY_GUNHOLSTERED; // gun in holster
	m_fGunDrawn			= FALSE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	MonsterInit();
	SetUse(&CTalkMonster::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney :: Precache()
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/barney.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "barney");
	if (FStringNull(m_iszSentencePrefix))// XDM3038a
		m_iszSentencePrefix = MAKE_STRING("BA");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	// XDM3038
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);

	CTalkMonster::Precache();
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	// XDM3038c TalkInit();
	//UTIL_PrecacheWeapon("weapon_9mmhandgun");// XDM3038: done in base class
}

// Init talk data
void CBarney :: TalkInit()
{
	CTalkMonster::TalkInit();
	// speach group names (group names are in sentences.txt)
	m_szGrp[TLK_ANSWER] =		DEFAULT_TLK_ANSWER;
	m_szGrp[TLK_QUESTION] =		DEFAULT_TLK_QUESTION;
	m_szGrp[TLK_IDLE] =			DEFAULT_TLK_IDLE;
	m_szGrp[TLK_STARE] =		DEFAULT_TLK_STARE;
	m_szGrp[TLK_USE] =			"OK";
	m_szGrp[TLK_UNUSE] =		"WAIT";
	m_szGrp[TLK_STOP] =			DEFAULT_TLK_STOP;
	m_szGrp[TLK_NOSHOOT] =		"SCARED";
	m_szGrp[TLK_HELLO] =		DEFAULT_TLK_HELLO;
	m_szGrp[TLK_PLHURT1] =		DEFAULT_TLK_PLHURT1;
	m_szGrp[TLK_PLHURT2] =		DEFAULT_TLK_PLHURT2;
	m_szGrp[TLK_PLHURT3] =		DEFAULT_TLK_PLHURT3;
	m_szGrp[TLK_PHELLO] =		DEFAULT_TLK_PHELLO;
	m_szGrp[TLK_PIDLE] =		DEFAULT_TLK_PIDLE;
	m_szGrp[TLK_PQUESTION] =	DEFAULT_TLK_PQUESTION;
	m_szGrp[TLK_SMELL] =		DEFAULT_TLK_SMELL;
	m_szGrp[TLK_WOUND] =		DEFAULT_TLK_WOUND;
	m_szGrp[TLK_MORTAL] =		DEFAULT_TLK_MORTAL;
	m_szGrp[TLK_DECLINE] =		"POK";// XDM3038a
	m_szGrp[TLK_HEAR] =			DEFAULT_TLK_HEAR;
	m_szGrp[TLK_KILL] =			DEFAULT_TLK_KILL;
	m_szGrp[TLK_SHOT] =			"SHOT";
	m_szGrp[TLK_FEAR] =			NULL;
	m_szGrp[TLK_FEARPLAYER] =	"MAD";
	m_szGrp[TLK_PLDEAD] =		DEFAULT_TLK_PLDEAD;// really, there are no suitable sounds!
	// get voice for head - just one barney voice for now
	// XDM3038c m_voicePitch = 100;
}

int CBarney::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && pAttacker && (pAttacker->IsPlayer()) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy.Get() == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pAttacker->pev->origin, pAttacker->pev->v_angle, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence(m_szGrp[TLK_FEARPLAYER], 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence(m_szGrp[TLK_SHOT], 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence(m_szGrp[TLK_SHOT], 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CBarney :: PainSound (void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);// XDM3038c
}

//=========================================================
// DeathSound
//=========================================================
void CBarney :: DeathSound (void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);// XDM3038c
}

void CBarney::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case HITGROUP_ARMOR:
		if (FBitSet(bitsDamageType, DMGM_RICOCHET) && !FBitSet(bitsDamageType, DMG_IGNOREARMOR))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				// not here! UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}
	CTalkMonster::TraceAttack( pAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

// XDM3038
CBaseEntity *CBarney::DropItem(const char *pszItemName, const Vector &vecPos, const Vector &vecAng)
{
	CBaseEntity *pItem = CBaseMonster::DropItem(pszItemName, vecPos, vecAng);
	if (pItem)
		pev->body = BARNEY_BODY_GUNGONE;
	return pItem;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t* CBarney :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;
	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarney :: GetSchedule (void)
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound = PBestSound();
		//ASSERT( pSound != NULL );
		if (pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			else if (pSound->m_iType & (bits_SOUND_COMBAT | bits_SOUND_WORLD | bits_SOUND_CARCASS | bits_SOUND_MEAT | bits_SOUND_GARBAGE | bits_SOUND_DEATH))// XDM3038a
				return GetScheduleOfType(SCHED_HEAR_SOUND);
		}
	}

	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence(m_szGrp[TLK_KILL], 4, VOL_NORM, ATTN_NORM);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if (m_hEnemy.Get() == NULL && IsFollowing())
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CBarney :: GetIdealState (void)
{
	return CTalkMonster::GetIdealState();
}

void CBarney::DeclineFollowing(void)
{
	PlaySentence(m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM);
}

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadBarney : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);// XDM3037
	virtual int	Classify(void) { return CLASS_GIB; }// XDM3035b
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual bool IsAlive(void) const { return false; }// XDM3038c
	virtual void KeyValue(KeyValueData *pkvd);
	virtual const char *GetDropItemName(void) { return "weapon_9mmhandgun";}// XDM3038

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadBarney::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney :: Spawn( )
{
	// Corpses have less health
	if (pev->health <= 0)
		pev->health = gSkillData.barneyHealth * 0.6;// XDM

	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_RED;

	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;// XDM: medium one

	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));// XDM3037
	pev->effects			= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead barney with bad pose\n" );
	}
	MonsterInitDead();
}

void CDeadBarney::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/barney.mdl");

	CBaseMonster::Precache();// XDM3038c: !!! need to precache drop item too!
}
