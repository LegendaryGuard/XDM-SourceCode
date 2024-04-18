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
// Gargantua
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "customentity.h"
#include "weapons.h"
#include "effects.h"
#include "soundent.h"
#include "decals.h"
#include "explode.h"
#include "gamerules.h"// XDM: allow effects
#include "game.h"

//=========================================================
// Gargantua Monster
//=========================================================
const float GARG_ATTACKDIST = 80.0;

// Garg animation events
enum garg_anim_events_e
{
	GARG_AE_SLASH_LEFT = 1,
	GARG_AE_BEAM_ATTACK_RIGHT,// No longer used
	GARG_AE_LEFT_FOOT,
	GARG_AE_RIGHT_FOOT,
	GARG_AE_STOMP,
	GARG_AE_BREATHE
};

// Gargantua is immune to any damage but this
#define GARG_DAMAGE					(DMG_ENERGYBEAM|DMG_CRUSH|DMG_BLAST|DMG_RADIATION)
#define GARG_EYE_SPRITE_NAME		"sprites/gargeye1.spr"
#define GARG_BEAM_SPRITE_NAME		"sprites/xbeam3.spr"
//#define GARG_BEAM_SPRITE2			"sprites/xbeam3.spr"
#define GARG_STOMP_SPRITE_NAME		"sprites/gargstomp.spr"
#define GARG_STOMP_BUZZ_SOUND		"garg/gar_buzz.wav"
#define GARG_GIB_MODEL				"models/metalplategibs.mdl"
#define GARG_FLAME_LENGTH			300// XDM
#define ATTN_GARG					0.75f// XDM3038a
#define STOMP_SPRITE_COUNT			10

//int gStompSprite = 0, gGargGibModel = 0;


class CSmoker : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Think(void);
};

LINK_ENTITY_TO_CLASS( env_smoker, CSmoker );

void CSmoker::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	SetNextThink(0);
	pev->solid = SOLID_NOT;
	UTIL_SetSize(this, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->angles = g_vecZero;
}

void CSmoker::Think(void)
{
	MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, pev->origin );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -pev->dmg, pev->dmg ));
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -pev->dmg, pev->dmg ));
		WRITE_COORD( pev->origin.z);
		WRITE_SHORT( g_iModelIndexSmoke );
		WRITE_BYTE((short)(pev->scale * 10));// XDM
		WRITE_BYTE(RANDOM_LONG(8,14)); // framerate
	MESSAGE_END();
	pev->health--;
	if (pev->health > 0)
	{
		SetNextThink(RANDOM_FLOAT(0.1, 0.2));
		CBaseEntity::Think();// XDM3037: allow SetThink()
	}
	else
		Destroy();
}


#define SPIRAL_INTERVAL		0.1 //025

// Spiral Effect
class CSpiral : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Think(void);
	virtual int ObjectCaps(void) { return FCAP_DONT_SAVE; }
	static CSpiral *CreateSpiral( const Vector &origin, float height, float radius, float duration, CBaseEntity *pOwner );
};

LINK_ENTITY_TO_CLASS( streak_spiral, CSpiral );

void CSpiral::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	SetNextThink(0);
	pev->solid = SOLID_NOT;
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	pev->effects |= EF_NODRAW;
	pev->angles = g_vecZero;
}

CSpiral *CSpiral::CreateSpiral(const Vector &origin, float height, float radius, float duration, CBaseEntity *pOwner)
{
	if (duration <= 0)
		return NULL;
	if (NUMBER_OF_ENTITIES() > (gpGlobals->maxEntities - ENTITY_INTOLERANCE))// XDM3035a: preserve server stability
		return NULL;

	CSpiral *pSpiral = GetClassPtr((CSpiral *)NULL, "streak_spiral");// XDM
	if (pSpiral)
	{
		if (pOwner)// XDM3034: multiplayer
			pSpiral->pev->owner = pOwner->edict();

		pSpiral->Spawn();
		pSpiral->pev->dmgtime = pSpiral->pev->nextthink;
		pSpiral->pev->origin = origin;
		pSpiral->pev->scale = radius;
		pSpiral->pev->dmg = height;
		pSpiral->pev->speed = duration;
		pSpiral->pev->health = 0;
		pSpiral->pev->angles = g_vecZero;
	}
	return pSpiral;
}

void CSpiral::Think(void)
{
	float time = gpGlobals->time - pev->dmgtime;
	while (time > SPIRAL_INTERVAL)
	{
		Vector position(pev->origin);
		Vector direction(0,0,1);

		float fraction = 1.0f / pev->speed;
		float radius = (pev->scale * pev->health) * fraction;

		position.z += (pev->health * pev->dmg) * fraction;
		pev->angles.y = (pev->health * 360 * 8) * fraction;
		UTIL_MakeVectors( pev->angles );
		position = position + gpGlobals->v_forward * radius;
		direction = (direction + gpGlobals->v_forward).Normalize();

		StreakSplash(position, direction, RANDOM_LONG(8,11), 20, RANDOM_LONG(50,150), 400);
		// Jeez, how many counters should this take ? :)
		pev->dmgtime += SPIRAL_INTERVAL;
		pev->health += SPIRAL_INTERVAL;
		time -= SPIRAL_INTERVAL;
	}
	SetNextThink(0);
	CBaseEntity::Think();// XDM3037: allow SetThink()

	if (pev->health >= pev->speed)
		Destroy();
}


//#define	STOMP_INTERVAL		0.025
#define	STOMP_LIFE			8.0f

class CStomp : public CBaseEntity
{
public:
	virtual void Spawn(void);
	//virtual void Precache(void);
	void EXPORT StompThink(void);
	static CStomp *StompCreate(int modelindex, const Vector &origin, const Vector &end, float speed, CBaseEntity *pOwner);
};

LINK_ENTITY_TO_CLASS( garg_stomp, CStomp );

CStomp *CStomp::StompCreate(int modelindex, const Vector &origin, const Vector &end, float speed, CBaseEntity *pOwner)
{
	CStomp *pStomp = GetClassPtr((CStomp *)NULL, "garg_stomp");
	if (pStomp)
	{
		pStomp->pev->modelindex = modelindex;
		pStomp->pev->origin = origin;
		pStomp->pev->movedir = (end - origin);
		pStomp->pev->health = pStomp->pev->movedir.NormalizeSelf();
		pStomp->pev->speed = speed;
		if (pOwner)// XDM3034: multiplayer
		{
			pStomp->pev->owner = pOwner->edict();
			pStomp->m_hOwner = pOwner;// XDM3037a
		}
		pStomp->Spawn();
	}
	return pStomp;
}

void CStomp::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	pev->frame = 0;
	pev->framerate = 30;
	pev->rendermode = kRenderTransAdd;// XDM
	pev->renderamt = 255;// XDM
	pev->scale = 0.8;
	pev->dmgtime = gpGlobals->time + STOMP_LIFE;// XDM3038b
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, GARG_STOMP_BUZZ_SOUND, VOL_NORM, ATTN_NORM, 0, PITCH_NORM * 0.5);
	PartSystem(pev->origin, -pev->movedir, VECTOR_CONE_45DEGREES, pev->modelindex, pev->rendermode, PARTSYSTEM_TYPE_FLAMECONE, g_pGameRules->FAllowEffects()?80:64, 512, 32768/* HACK!! RENDERSYSTEM_FLAG_SCALE*/, entindex());
	SetThink(&CStomp::StompThink);
	SetNextThink(0);// now
}

// XDM3038a
/*void CStomp::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING(GARG_STOMP_SPRITE_NAME);

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}*/

//-----------------------------------------------------------------------------
// Purpose: Garg's "stomp" flies through walls and entities, dealing damage
// WARNING: Must not be just Think() overload! XDM3038a
//-----------------------------------------------------------------------------
void CStomp::StompThink(void)
{
	// Do damage for this frame
	Vector vecStart(pev->origin);
	vecStart.z += 8.0f;//???
	pev->origin += pev->movedir * pev->speed * gpGlobals->frametime;
	TraceResult tr;
	UTIL_TraceHull(vecStart, pev->origin, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit && tr.pHit != pev->owner)
	{
		CBaseEntity *pOwner = NULL;
		if (pev->owner)
			pOwner = CBaseEntity::Instance(pev->owner);

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity)
			pEntity->TakeDamage(this, pOwner, gSkillData.gargantuaDmgStomp, DMG_SONIC);
	}

	UTIL_SetOrigin(this, pev->origin);
	// Accelerate the effect
	pev->speed += gpGlobals->frametime * pev->framerate;
	pev->framerate += gpGlobals->frametime * 320.0f;
	SetNextThink(0.01);
	// traffic	pev->scale += gpGlobals->frametime*0.5f;

	// health has the "life" of this effect
	//pev->health -= STOMP_INTERVAL * pev->speed;

	//if (pev->health <= 0)// Life has run out
	if (pev->dmgtime <= gpGlobals->time || tr.fAllSolid)// XDM3038b: ok, so it wasn't designed to fly through walls
	{
		pev->velocity.Clear();
		pev->speed = 0;
		PartSystem(pev->origin, -pev->movedir, g_vecZero, pev->modelindex, kRenderNormal, PARTSYSTEM_TYPE_REMOVEANY, 0, 0, 0, entindex());
		STOP_SOUND(ENT(pev), CHAN_BODY, GARG_STOMP_BUZZ_SOUND);
		SetThink(&CBaseEntity::SUB_Remove);// XDM3035: delay a liiiittle bit so this entindex won't be occupied anytime soon
		SetNextThink(0.1);
	}
}




class CGargantua : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void);
	virtual void SetObjectCollisionBox(void)
	{
		pev->absmin = pev->origin + Vector(-80, -80, 0);
		pev->absmax = pev->origin + Vector(80, 80, 214);
	}
	virtual bool IsPushable(void) { return false; }// XDM3037a
	virtual void SetYawSpeed(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);		// Swipe
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);		// Flames
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);		// Stomp attack
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);
	virtual void PrescheduleThink(void);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual void AlertSound(void);// XDM3038b
	virtual void IdleSound(void);// XDM3038b
	virtual void PainSound(void);// XDM3037a
	virtual void OnFreePrivateData(void);// XDM3037a

	void DeathEffect(void);
	void EyeOff(void);
	void EyeOn(int level);
	void EyeUpdate(void);
	void Leap(void);
	void StompAttack(void);
	void FlameCreate(void);
	void FlameUpdate(void);
	void FlameControls( float angleX, float angleY );
	void FlameDestroy(void);
	inline bool FlameIsOn(void) { return (m_pFlame[0] != NULL || m_pFlame[1] != NULL || m_pFlame[2] != NULL || m_pFlame[3] != NULL); }

	void FlameDamage( Vector vecStart, Vector vecEnd, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int iClassIgnore, int bitsDamageType );

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

protected:
	static const char *pAttackHitSounds[];
	static const char *pBeamAttackSounds[];
	static const char *pAttackMissSounds[];
	static const char *pFootSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pStompSounds[];
	static const char *pBreatheSounds[];

	CBaseEntity* GargantuaCheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	CSprite		*m_pEyeGlow;		// Glow around the eyes
	CBeam		*m_pFlame[4];		// Flame beams

	int			m_eyeBrightness;	// Brightness target
	float		m_seeTime;			// Time to attack (when I see the enemy, I set this)
	float		m_flameTime;		// Time of next flame attack
	float		m_painSoundTime;	// Time of next pain sound
	float		m_streakTime;		// streak timer (don't send too many)
	float		m_flameX;			// Flame thrower aim
	float		m_flameY;
	string_t	m_iszSpriteBeam;// XDM3038b: allow some customization
	string_t	m_iszSpriteEye;
	string_t	m_iszSpriteStomp;
	int			m_iStompSprite;// XDM3038a: index
};

LINK_ENTITY_TO_CLASS( monster_gargantua, CGargantua );

TYPEDESCRIPTION	CGargantua::m_SaveData[] =
{
	DEFINE_FIELD( CGargantua, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CGargantua, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CGargantua, m_seeTime, FIELD_TIME ),
	DEFINE_FIELD( CGargantua, m_flameTime, FIELD_TIME ),
	DEFINE_FIELD( CGargantua, m_streakTime, FIELD_TIME ),
	DEFINE_FIELD( CGargantua, m_painSoundTime, FIELD_TIME ),
	DEFINE_ARRAY( CGargantua, m_pFlame, FIELD_CLASSPTR, 4 ),
	DEFINE_FIELD( CGargantua, m_flameX, FIELD_FLOAT ),
	DEFINE_FIELD( CGargantua, m_flameY, FIELD_FLOAT ),
	DEFINE_FIELD( CGargantua, m_iszSpriteBeam, FIELD_STRING ),// XDM3038b
	DEFINE_FIELD( CGargantua, m_iszSpriteEye, FIELD_STRING ),
	DEFINE_FIELD( CGargantua, m_iszSpriteStomp, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CGargantua, CBaseMonster );

const char *CGargantua::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGargantua::pBeamAttackSounds[] =
{
	"garg/gar_flameoff1.wav",
	"garg/gar_flameon1.wav",
	"garg/gar_flamerun1.wav",
};


const char *CGargantua::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};
/*
const char *CGargantua::pRicSounds[] =
{
#if 0
	"weapons/ric1.wav",
	"weapons/ric2.wav",
	"weapons/ric3.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
#else
	"debris/metal4.wav",
	"debris/metal6.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
#endif
};
*/
const char *CGargantua::pFootSounds[] =
{
	"garg/gar_step1.wav",
	"garg/gar_step2.wav",
};

const char *CGargantua::pIdleSounds[] =
{
	"garg/gar_idle1.wav",
	"garg/gar_idle2.wav",
	"garg/gar_idle3.wav",
	"garg/gar_idle4.wav",
	"garg/gar_idle5.wav",
};

const char *CGargantua::pAttackSounds[] =
{
	"garg/gar_attack1.wav",
	"garg/gar_attack2.wav",
	"garg/gar_attack3.wav",
};

const char *CGargantua::pAlertSounds[] =
{
	"garg/gar_alert1.wav",
	"garg/gar_alert2.wav",
	"garg/gar_alert3.wav",
};

const char *CGargantua::pPainSounds[] =
{
	"garg/gar_pain1.wav",
	"garg/gar_pain2.wav",
	"garg/gar_pain3.wav",
};

const char *CGargantua::pStompSounds[] =
{
	"garg/gar_stomp1.wav",
};

const char *CGargantua::pBreatheSounds[] =
{
	"garg/gar_breathe1.wav",
	"garg/gar_breathe2.wav",
	"garg/gar_breathe3.wav",
};
//=========================================================
// AI Schedules Specific to this monster
//=========================================================
#if 0
enum gargantua_schedules_e
{
	SCHED_ = LAST_COMMON_SCHEDULE + 1,
};
#endif

enum gargantua_tasks_e
{
	TASK_SOUND_ATTACK = LAST_COMMON_TASK + 1,
	TASK_FLAME_SWEEP,
};

Task_t	tlGargFlame[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOUND_ATTACK,		(float)0		},
	// { TASK_PLAY_SEQUENCE,		(float)ACT_SIGNAL1	},
	{ TASK_SET_ACTIVITY,		(float)ACT_MELEE_ATTACK2 },
	{ TASK_FLAME_SWEEP,			(float)4.5		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slGargFlame[] =
{
	{
		tlGargFlame,
		ARRAYSIZE ( tlGargFlame ),
		0,
		0,
		"GargFlame"
	},
};


// primary melee attack
Task_t	tlGargSwipe[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slGargSwipe[] =
{
	{
		tlGargSwipe,
		ARRAYSIZE ( tlGargSwipe ),
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"GargSwipe"
	},
};


DEFINE_CUSTOM_SCHEDULES( CGargantua )
{
	slGargFlame,
	slGargSwipe,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGargantua, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CGargantua :: Classify (void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_MILITARY;// XDM
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGargantua :: SetYawSpeed (void)
{
	int ys;
	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_WALK:
	case ACT_RUN:
		ys = 60;
		break;

	default:
		ys = 60;
		break;
	}
	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CGargantua :: Spawn()
{
	if (pev->health <= 0)
		pev->health = gSkillData.gargantuaHealth;
	if (pev->armorvalue == 0)
		pev->armorvalue = 100;// XDM
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_GREEN;
	if (m_iGibCount == 0)
		m_iGibCount = 56;// XDM: big one
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.gargantuaScore;

	CBaseMonster::Spawn();// XDM3038b: Precache( );
	UTIL_SetSize(this, Vector(-32, -32, 0), Vector(32, 32, 200));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	//pev->view_ofs		.Set( 0, 0, 96 );// taken from mdl file
	m_flFieldOfView		= -0.2;// width of forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_pEyeGlow = CSprite::SpriteCreate(STRING(m_iszSpriteEye), pev->origin, FALSE);
	if (m_pEyeGlow)
	{
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
		m_pEyeGlow->SetAttachment( edict(), 1 );
		m_pEyeGlow->SetScale(0.5);
	}
	EyeOff();
	m_seeTime = gpGlobals->time + 5;
	m_flameTime = gpGlobals->time + 2;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGargantua :: Precache()
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/garg.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "garg");

	CBaseMonster::Precache();// XDM3038a

	if (FStringNull(m_iszSpriteEye))// XDM3038b
		m_iszSpriteEye = MAKE_STRING(GARG_EYE_SPRITE_NAME);
	if (FStringNull(m_iszSpriteBeam))
		m_iszSpriteBeam = MAKE_STRING(GARG_BEAM_SPRITE_NAME);
	if (FStringNull(m_iszSpriteStomp))// resource overload!
		m_iszSpriteStomp = MAKE_STRING(GARG_STOMP_SPRITE_NAME);

	PRECACHE_MODEL(STRINGV(m_iszSpriteEye));
	PRECACHE_MODEL(STRINGV(m_iszSpriteBeam));
	m_iStompSprite = PRECACHE_MODEL(STRINGV(m_iszSpriteStomp));// XDM3038b
	m_iGibModelIndex = PRECACHE_MODEL(GARG_GIB_MODEL);// XDM3038b
	PRECACHE_SOUND( GARG_STOMP_BUZZ_SOUND );

	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pBeamAttackSounds);
	PRECACHE_SOUND_ARRAY(pFootSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pStompSounds);
	PRECACHE_SOUND_ARRAY(pBreatheSounds);
}

void CGargantua::EyeOn( int level )
{
	m_eyeBrightness = level;
}

void CGargantua::EyeOff(void)
{
	m_eyeBrightness = 0;
}

void CGargantua::EyeUpdate(void)
{
	if ( m_pEyeGlow )
	{
		m_pEyeGlow->pev->renderamt = UTIL_Approach( m_eyeBrightness, m_pEyeGlow->pev->renderamt, 26 );
		if (m_pEyeGlow->pev->renderamt == 0)
			m_pEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pEyeGlow->pev->effects &= ~EF_NODRAW;

		UTIL_SetOrigin(m_pEyeGlow, pev->origin);
	}
}

//#include "game.h"

void CGargantua::StompAttack(void)
{
	TraceResult trace;
	UTIL_MakeVectors( pev->angles );
	Vector vecStart	= pev->origin + Vector(0,0,60) + 35 * gpGlobals->v_forward;
	Vector vecAim = ShootAtEnemy( vecStart );
	Vector vecEnd = (vecAim * 1024) + vecStart;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &trace );
	CStomp::StompCreate(m_iStompSprite, vecStart, trace.vecEndPos, 10, this);
	UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000 );
	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pStompSounds[ RANDOM_LONG(0,ARRAYSIZE(pStompSounds)-1) ], VOL_NORM, ATTN_GARG, 0, PITCH_NORM + RANDOM_LONG(-10,10) );

	if (g_pGameRules->FAllowEffects())// XDM3035
	{
		UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,20), ignore_monsters, edict(), &trace );
		if ( trace.flFraction < 1.0 )
			UTIL_DecalTrace( &trace, DECAL_GARGSTOMP1 );
	}
}

void CGargantua :: FlameCreate(void)
{
	short			i;
	Vector		posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );

	for ( i = 0; i < 4; i++ )
	{
		if ( i < 2 )
			m_pFlame[i] = CBeam::BeamCreate(STRING(m_iszSpriteBeam), 240);
		else
			m_pFlame[i] = CBeam::BeamCreate(STRING(m_iszSpriteBeam), 140);

		if ( m_pFlame[i] )
		{
			int attach = i%2;
			// attachment is 0 based in GetAttachment
			GetAttachment( attach+1, posGun, angleGun );

			Vector vecEnd = (gpGlobals->v_forward * GARG_FLAME_LENGTH) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->PointEntInit( trace.vecEndPos, entindex() );
			if ( i < 2 )
				m_pFlame[i]->SetColor( 0, 0, 255 );
			else
				m_pFlame[i]->SetColor( 0, 255, 0 );

			m_pFlame[i]->SetBrightness( 190 );
			m_pFlame[i]->SetFlags( BEAM_FSHADEIN );
			m_pFlame[i]->SetScrollRate( 20 );
			// attachment is 1 based in SetEndAttachment
			m_pFlame[i]->SetEndAttachment( attach + 2 );
			CSoundEnt::InsertSound( bits_SOUND_COMBAT, posGun, 384, 0.3 );
		}
	}
	if (g_pGameRules->FAllowEffects())// XDM
		DynamicLight(EyePosition(), 20, 0, 160,255, 10, 200);

	EMIT_SOUND_DYN(edict(), CHAN_BODY, pBeamAttackSounds[ 1 ], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pBeamAttackSounds[ 2 ], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}

void CGargantua :: FlameControls( float angleX, float angleY )
{
	NormalizeAngle180(&angleY);// XDM3037a

	if (angleY < -45)
		angleY = -45;
	else if (angleY > 45)
		angleY = 45;

	m_flameX = UTIL_ApproachAngle( angleX, m_flameX, 4 );
	m_flameY = UTIL_ApproachAngle( angleY, m_flameY, 8 );
	SetBoneController( 0, m_flameY );
	SetBoneController( 1, m_flameX );
}

void CGargantua :: FlameUpdate(void)
{
	static float	offset[2] = { 60, -60 };
	short			i;
	Vector			vecStart, angleGun;
	TraceResult		trace;
	bool			streaks = false;

	for ( i = 0; i < 2; i++ )
	{
		if ( m_pFlame[i] )
		{
			Vector vecAim = pev->angles;
			vecAim.x += m_flameX;
			vecAim.y += m_flameY;

			UTIL_MakeVectors( vecAim );

			GetAttachment( i+1, vecStart, angleGun );
			Vector vecEnd = vecStart + (gpGlobals->v_forward * GARG_FLAME_LENGTH); //  - offset[i] * gpGlobals->v_right;

			UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->SetStartPos( trace.vecEndPos );
			m_pFlame[i+2]->SetStartPos( (vecStart * 0.6) + (trace.vecEndPos * 0.4) );

			if ( trace.flFraction != 1.0 && gpGlobals->time >= m_streakTime )
			{
				if (g_pGameRules->FAllowEffects())
				{
					StreakSplash( trace.vecEndPos, trace.vecPlaneNormal, RANDOM_INT2(3,7), 10, 40, 400 );// XDM: now color is 3 or 7 (blue) "r_efx.h"
					streaks = true;
					UTIL_DecalTrace(&trace, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));
				}
			}
			FlameDamage( vecStart, trace.vecEndPos, this, this, gSkillData.gargantuaDmgFire, CLASS_ALIEN_MONSTER, DMG_BURN );
			if (g_pGameRules == NULL || g_pGameRules->FAllowEffects() && !g_pGameRules->IsMultiplayer())// XDM3037a
			{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecStart);
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 * (i + 2) );		// entity, attachment
				WRITE_COORD( vecStart.x );		// origin
				WRITE_COORD( vecStart.y );
				WRITE_COORD( vecStart.z );
				WRITE_COORD( RANDOM_FLOAT( 32, 48 ) );	// radius
				WRITE_BYTE( 0 );	// R
				WRITE_BYTE( 200 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 2 );	// life * 10
				WRITE_COORD( 0 ); // decay
			MESSAGE_END();
			}
		}
	}
	//if (g_pGameRules->FAllowEffects())// XDM
	//	DynamicLight(EyePosition(), 20, 0, 160,255, 10, 255);

	if ( streaks )
		m_streakTime = gpGlobals->time + 0.1f;// XDM3037a
}

void CGargantua :: FlameDamage( Vector vecStart, Vector vecEnd, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage;
	Vector		vecSpot, vecSrc;
	Vector vecMid = (vecStart + vecEnd) * 0.5f;
	Vector vecAim = (vecEnd - vecStart).Normalize();
	vec_t searchRadius = (vecStart - vecMid).Length();
	vec_t dist;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecMid, searchRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			vecSpot = pEntity->BodyTarget( vecMid );
			dist = DotProduct( vecAim, vecSpot - vecMid );
			if (dist > searchRadius)
				dist = searchRadius;
			else if (dist < -searchRadius)
				dist = searchRadius;

			vecSrc = vecAim; vecSrc *= dist; vecSrc += vecMid;//vecSrc = vecMid + dist * vecAim;

			UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				// decrease damage for an ent that's farther from the flame.
				dist = ( vecSrc - tr.vecEndPos ).Length();

				if (dist > 64)
				{
					flAdjustedDamage = flDamage - (dist - 64) * 0.4;
					if (flAdjustedDamage <= 0)
						continue;
				}
				else
					flAdjustedDamage = flDamage;

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage();
					pEntity->TraceAttack(pInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
					ApplyMultiDamage(pInflictor, pAttacker);
				}
				else
					pEntity->TakeDamage(pInflictor, pAttacker, flAdjustedDamage, bitsDamageType);
			}
		}
	}
}

void CGargantua :: FlameDestroy(void)
{
	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pBeamAttackSounds[ 0 ], VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
	for (short i = 0; i < 4; i++ )
	{
		if ( m_pFlame[i] )
		{
			UTIL_Remove( m_pFlame[i] );
			m_pFlame[i] = NULL;
		}
	}
}

void CGargantua :: PrescheduleThink(void)
{
	if ( !HasConditions( bits_COND_SEE_ENEMY ) )
	{
		m_seeTime = gpGlobals->time + 5;
		EyeOff();
	}
	else
		EyeOn( 200 );

	EyeUpdate();
}

void CGargantua::AlertSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);// XDM3038c
	//EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolume(), ATTN_GARG, 0, GetSoundPitch());
}

void CGargantua::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);// XDM3038c
	//EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolume(), ATTN_GARG, 0, GetSoundPitch());
}

// XDM3037a
void CGargantua::PainSound(void)
{
	if (m_painSoundTime < gpGlobals->time)
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);// XDM3038c
		//EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolume(), ATTN_GARG, 0, GetSoundPitch());
		m_painSoundTime = gpGlobals->time + RANDOM_FLOAT(2.5, 4);
	}
}

void CGargantua::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (IsAlive())
	{
		// UNDONE: Hit group specific damage?
		if (FBitSet(bitsDamageType, GARG_DAMAGE))
		{
			/*if (m_painSoundTime < gpGlobals->time)
			{
				PainSound();// XDM3037a
				m_painSoundTime = gpGlobals->time + RANDOM_FLOAT( 2.5, 4 );
			}*/
		}
		else
		{
			if (!FBitSet(bitsDamageType, DMG_IGNOREARMOR|DMG_NOHITPOINT))
			{
				if (pev->dmgtime <= gpGlobals->time)// || (RANDOM_LONG(0,100) < 20))
				{
					UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(0.5,1.5));
					pev->dmgtime = gpGlobals->time + 0.2;
				}
			}
			flDamage = 0;
		}
	}
	CBaseMonster::TraceAttack( pAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CGargantua::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
//	ALERT( at_aiconsole, "CGargantua::TakeDamage\n");
	if ( IsAlive() )
	{
		if (!FBitSet(bitsDamageType, GARG_DAMAGE))
			flDamage *= 0.01;
		if (FBitSet(bitsDamageType, DMG_BLAST))
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}
	return CBaseMonster::TakeDamage( pInflictor, pAttacker, flDamage, bitsDamageType );
}

void CGargantua::DeathEffect(void)
{
	pev->solid = SOLID_NOT;// XDM3038c
	pev->takedamage = DAMAGE_NO;// XDM3038c
	UTIL_MakeVectors(pev->angles);
	Vector deathPos(gpGlobals->v_forward); deathPos *= 100.0f; deathPos += pev->origin;
	if (sv_overdrive.value > 0.0f)
		pev->renderfx = kRenderFxExplode;// XDM3038c

	// Create a spiral of streaks
	CSpiral::CreateSpiral( deathPos, (pev->absmax.z - pev->absmin.z) * 0.6, 125, 1.75, this );

	Vector position(pev->origin);
	position.z += 32;
	short i;
	for ( i = 0; i < 7; i+=2 )
	{
		ExplosionCreate(position + RandomVector(64,64,0), g_vecZero, this, this, 60 + (i*20), SF_ENVEXPLOSION_NODAMAGE, (float)i * 0.3f);
		//XDM	SpawnExplosion( position, 70, (i * 0.3), 60 + (i*20) );
		position.z += 15;
	}

	if (NUMBER_OF_ENTITIES() > (gpGlobals->maxEntities - ENTITY_INTOLERANCE))// XDM3035a: preserve server stability
		return;// don't do any additional, less important effects

	CBaseEntity *pSmoker = Create("env_smoker", pev->origin, g_vecZero, NULL);// TODO: UNDONE: replace with RenderSystem
	if (pSmoker)
	{
		pSmoker->pev->health = 1;	// 1 smoke balls
		pSmoker->pev->scale = 4.6;	// 4.6X normal size
		pSmoker->pev->dmg = 0;		// 0 radial distribution
		pSmoker->SetNextThink(2.5);	// Start in 2.5 seconds
	}
}

void CGargantua::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
//	EyeOff();
	FlameDestroy();// XDM3038c
	if (m_pEyeGlow)
	{
		UTIL_Remove(m_pEyeGlow);
		m_pEyeGlow = NULL;
	}
	CBaseMonster::Killed(pInflictor, pAttacker, GIB_NEVER);
}

//=========================================================
// CheckMeleeAttack1
// Garg swipe attack
//
//=========================================================
BOOL CGargantua::CheckMeleeAttack1( float flDot, float flDist )
{
//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if (flDot >= 0.7)
	{
		if (flDist <= GARG_ATTACKDIST)
			return TRUE;
	}
	return FALSE;
}

// Flame thrower madness!
BOOL CGargantua::CheckMeleeAttack2( float flDot, float flDist )
{
//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if ( gpGlobals->time > m_flameTime )
	{
		if (flDot >= 0.8 && flDist > GARG_ATTACKDIST)
		{
			if ( flDist <= GARG_FLAME_LENGTH )
				return TRUE;
		}
	}
	return FALSE;
}


//=========================================================
// CheckRangeAttack1
// flDot is the cos of the angle of the cone within which
// the attack can occur.
//=========================================================
//
// Stomp attack
//
//=========================================================
BOOL CGargantua::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->time > m_seeTime )
	{
		if (flDot >= 0.7 && flDist > GARG_ATTACKDIST)
		{
			return TRUE;
		}
	}
	return FALSE;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGargantua::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GARG_AE_SLASH_LEFT:
		{
			// HACKHACK!!!
			CBaseEntity *pHurt = GargantuaCheckTraceHullAttack( GARG_ATTACKDIST + 10.0, gSkillData.gargantuaDmgSlash, DMG_SLASH );
			if (pHurt)
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->PunchPitchAxis(30.0f); // pitch
					pHurt->pev->punchangle.y = -30;	// yaw
					pHurt->pev->punchangle.z = 30;	// roll
					//UTIL_MakeVectors(pev->angles);	// called by CheckTraceHullAttack
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 50 + RANDOM_LONG(0,15) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 50 + RANDOM_LONG(0,15) );

			// WTF?	Vector forward;
			//ANGLE_VECTORS( pev->angles, forward, NULL, NULL );
		}
		break;

	case GARG_AE_RIGHT_FOOT:
	case GARG_AE_LEFT_FOOT:
		{
			if (g_pGameRules->FAllowEffects())
				UTIL_ScreenShake( pev->origin, 4.0, 3.0, 1.0, 750 );

			EMIT_SOUND_DYN ( edict(), CHAN_BODY, pFootSounds[ RANDOM_LONG(0,ARRAYSIZE(pFootSounds)-1) ], GetSoundVolume(), ATTN_GARG, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		}
		break;

	case GARG_AE_STOMP:
		{
			StompAttack();
			m_seeTime = gpGlobals->time + 12;
		}
		break;

	case GARG_AE_BREATHE:
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pBreatheSounds);// XDM3038c
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.

// Overridden for Gargantua because his swing starts lower as
// a percentage of his height (otherwise he swings over the
// players head)
//=========================================================
CBaseEntity* CGargantua::GargantuaCheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += 64;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * flDist * 0.3);
	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if (tr.pHit && iDamage > 0)// XDM3038c
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if (pEntity)
			pEntity->TakeDamage( this, this, iDamage, iDmgType );

		return pEntity;
	}
	return NULL;
}


Schedule_t *CGargantua::GetScheduleOfType( int Type )
{
	// HACKHACK - turn off the flames if they are on and garg goes scripted / dead
	if (FlameIsOn())
		FlameDestroy();

	switch (Type)
	{
		case SCHED_MELEE_ATTACK2:
			return slGargFlame;
		case SCHED_MELEE_ATTACK1:
			return slGargSwipe;
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


void CGargantua::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FLAME_SWEEP:
		{
			FlameCreate();
			m_flWaitFinished = gpGlobals->time + pTask->flData;
			m_flameTime = gpGlobals->time + 6;
			m_flameX = 0;
			m_flameY = 0;
		}
		break;

	case TASK_SOUND_ATTACK:
		{
			if ( RANDOM_LONG(0,100) < 30 )
				EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], GetSoundVolume(), ATTN_GARG, 0, GetSoundPitch());

			TaskComplete();
		}
		break;

	case TASK_DIE:
		{
			m_flWaitFinished = gpGlobals->time + 1.6;
			DeathEffect();
		}
		// FALL THROUGH
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CGargantua::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		if ( gpGlobals->time > m_flWaitFinished )
		{
			pev->solid = SOLID_NOT;
			pev->takedamage = DAMAGE_NO;
			//pev->renderamt = 0;
			pev->renderfx = kRenderFxExplode;
			StopAnimation();
			MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, pev->origin );
				WRITE_BYTE(TE_BREAKMODEL);
				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				// size
				WRITE_COORD( 64 );
				WRITE_COORD( 64 );
				WRITE_COORD( 120 );
				// velocity
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				// randomization
				WRITE_BYTE( 200 );
				WRITE_SHORT( m_iGibModelIndex );	//model id#
				WRITE_BYTE(m_iGibCount);// # of shards
				WRITE_BYTE( 30 );// 3.0 seconds
				WRITE_BYTE( BREAK_FLESH | BREAK_SMOKE );// XDM
			MESSAGE_END();
			FlameDestroy();// XDM3038c
			if (m_pEyeGlow)
			{
				UTIL_Remove(m_pEyeGlow);
				m_pEyeGlow = NULL;
			}
			SetNextThink(0.15);
			SetThink(&CBaseEntity::SUB_Remove);// RunPostDeath may override this, so it's ok to set
			if (ShouldRespawn())
				SetBits(pev->effects, EF_NODRAW);// XDM3038c: garg always explodes, so make it disappear until it respawns

			RunPostDeath();
		}
		else
			CBaseMonster::RunTask(pTask);

		break;

	case TASK_FLAME_SWEEP:
		if ( gpGlobals->time > m_flWaitFinished )
		{
			FlameDestroy();
			TaskComplete();
			FlameControls( 0, 0 );
			SetBoneController( 0, 0 );
			SetBoneController( 1, 0 );
		}
		else
		{
			bool cancel = false;
			Vector angles = g_vecZero;
			FlameUpdate();
			CBaseEntity *pEnemy = m_hEnemy;
			if ( pEnemy )
			{
				Vector org(pev->origin);
				org.z += 64;
				Vector dir = pEnemy->BodyTarget(org) - org;
				VectorAngles(dir, angles);//angles = UTIL_VecToAngles( dir );
#if !defined (NOSQB)
				angles.x = -angles.x;
#endif
				angles.y -= pev->angles.y;
				if ( dir.Length() > 400 )
					cancel = true;
			}
			if ( fabs(angles.y) > 60 )
				cancel = true;

			if ( cancel )
			{
				m_flWaitFinished -= 0.5;
				m_flameTime -= 0.5;
			}
			// FlameControls( angles.x + 2 * sin(gpGlobals->time*8), angles.y + 28 * sin(gpGlobals->time*8.5) );
			FlameControls( angles.x, angles.y );
		}
		break;

	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

void CGargantua::OnFreePrivateData(void)// XDM3037a
{
	for (short i = 0; i < 4; ++i)
	{
		if (m_pFlame[i])
		{
			//m_pFlame[i]->SetBrightness(0);
			m_pFlame[i]->pev->effects = EF_NODRAW;
			//m_pFlame[i]->SetStartEntity(0);
			//m_pFlame[i]->SetEndEntity(0);
			SetBits(m_pFlame[i]->pev->flags, FL_KILLME);
			// UNSAFE! pointer may be bad! UTIL_Remove(m_pFlame[i]);
			m_pFlame[i] = NULL;
		}
	}
	if (m_pEyeGlow)
	{
		// UNSAFE! pointer may be bad! UTIL_Remove(m_pEyeGlow);
		SetBits(m_pEyeGlow->pev->flags, FL_KILLME);
		m_pEyeGlow = NULL;
	}
	CBaseMonster::OnFreePrivateData();// XDM3038c
}
