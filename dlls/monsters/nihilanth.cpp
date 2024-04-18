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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
//#include "explode.h"// XDM: sphere
#include "gamerules.h"

#define N_SCALE		15
#define N_SPHERES	20
#define N_MAX_FRIENDS	3

class CNihilanth : public CBaseMonster
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void) { return CLASS_ALIEN_MILITARY; }
//	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual bool GibMonster(void);

	virtual void SetObjectCollisionBox(void)
	{
		pev->absmin = pev->origin + Vector( -16 * N_SCALE, -16 * N_SCALE, -48 * N_SCALE );
		pev->absmax = pev->origin + Vector( 16 * N_SCALE, 16 * N_SCALE, 28 * N_SCALE );
	}

	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual bool IsPushable(void) { return false; }// XDM: for explosions

	void EXPORT StartupThink(void);
	void EXPORT HuntThink(void);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink(void);
	void EXPORT CommandUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void FloatSequence(void);
	void NextActivity(void);

	void Flight(void);

	BOOL AbsorbSphere(void);
	BOOL EmitSphere(void);
	void TargetSphere( USE_TYPE useType, float value );
	CBaseEntity *RandomTargetname( const char *szName );
	void ShootBalls(void);
	void MakeFriend(const Vector &vecStart);
	void DestroyFriends(void);

	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);

	virtual void PainSound(void);
	virtual void DeathSound(void);

	static const char *pAttackSounds[];	// vocalization: play sometimes when he launches an attack
	static const char *pBallSounds[];	// the sound of the lightening ball launch
	static const char *pShootSounds[];	// grunting vocalization: play sometimes when he launches an attack
	static const char *pRechargeSounds[];	// vocalization: play when he recharges
	static const char *pLaughSounds[];	// vocalization: play sometimes when hit and still has lots of health
	static const char *pPainSounds[];	// vocalization: play sometimes when hit and has much less health and no more chargers
	static const char *pDeathSounds[];	// vocalization: play as he dies


	float m_flForce;

	float m_flNextPainSound;

	Vector m_velocity;
	Vector m_avelocity;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	float  m_flMinZ;
	float  m_flMaxZ;

	//Vector m_vecGoal;

	float m_flLastSeen;
	float m_flPrevSeen;

	int m_irritation;

	int m_iLevel;
	int m_iTeleport;

	EHANDLE m_hRecharger;

	EHANDLE m_hSphere[N_SPHERES];
	int	m_iActiveSpheres;

	float m_flAdj;

	CSprite *m_pBall;

	char m_szRechargerTarget[64];
	char m_szDrawUse[64];
	char m_szTeleportUse[64];
	char m_szTeleportTouch[64];
	char m_szDeadUse[64];
	char m_szDeadTouch[64];

	float m_flShootEnd;
	float m_flShootTime;

	EHANDLE m_hFriend[N_MAX_FRIENDS];

	unsigned short sprEShockStart;// XDM
	unsigned short sprWarpGlow1;
	unsigned short sprWarpGlow2;
};

LINK_ENTITY_TO_CLASS( monster_nihilanth, CNihilanth );

TYPEDESCRIPTION	CNihilanth::m_SaveData[] =
{
	DEFINE_FIELD( CNihilanth, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CNihilanth, m_flNextPainSound, FIELD_TIME ),
	DEFINE_FIELD( CNihilanth, m_velocity, FIELD_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_avelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_flMinZ, FIELD_FLOAT ),
	DEFINE_FIELD( CNihilanth, m_flMaxZ, FIELD_FLOAT ),
	//DEFINE_FIELD( CNihilanth, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CNihilanth, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CNihilanth, m_flPrevSeen, FIELD_TIME ),
	DEFINE_FIELD( CNihilanth, m_irritation, FIELD_INTEGER ),
	DEFINE_FIELD( CNihilanth, m_iLevel, FIELD_INTEGER ),
	DEFINE_FIELD( CNihilanth, m_iTeleport, FIELD_INTEGER ),
	DEFINE_FIELD( CNihilanth, m_hRecharger, FIELD_EHANDLE ),
	DEFINE_ARRAY( CNihilanth, m_hSphere, FIELD_EHANDLE, N_SPHERES ),
	DEFINE_FIELD( CNihilanth, m_iActiveSpheres, FIELD_INTEGER ),
	DEFINE_FIELD( CNihilanth, m_flAdj, FIELD_FLOAT ),
	DEFINE_FIELD( CNihilanth, m_pBall, FIELD_CLASSPTR ),
	DEFINE_ARRAY( CNihilanth, m_szRechargerTarget, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( CNihilanth, m_szDrawUse, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( CNihilanth, m_szTeleportUse, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( CNihilanth, m_szTeleportTouch, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( CNihilanth, m_szDeadUse, FIELD_CHARACTER, 64 ),
	DEFINE_ARRAY( CNihilanth, m_szDeadTouch, FIELD_CHARACTER, 64 ),
	DEFINE_FIELD( CNihilanth, m_flShootEnd, FIELD_TIME ),
	DEFINE_FIELD( CNihilanth, m_flShootTime, FIELD_TIME ),
	DEFINE_ARRAY( CNihilanth, m_hFriend, FIELD_EHANDLE, N_MAX_FRIENDS),
};

IMPLEMENT_SAVERESTORE( CNihilanth, CBaseMonster );

class CNihilanthHVR : public CBaseMonster
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void Spawn(void);
	virtual void Precache(void);
	// XDM
	virtual	bool IsMonster(void) const { return false; }
	virtual bool IsProjectile(void) const { return true; }
	virtual bool IsPushable(void) { return false; }

	void CircleInit( CBaseEntity *pTarget );
	void AbsorbInit(void);
	void TeleportInit( CNihilanth *pOwner, CBaseEntity *pEnemy, CBaseEntity *pTarget, CBaseEntity *pTouch );
	void GreenBallInit(void);
	void ZapInit( CBaseEntity *pEnemy );
	bool CircleTarget(const Vector &vecTarget);

	void EXPORT HoverThink(void);
	void EXPORT DissipateThink(void);

	void EXPORT ZapThink(void);
	void EXPORT TeleportThink(void);
	void EXPORT TeleportTouch(CBaseEntity *pOther);

	void EXPORT RemoveTouch(CBaseEntity *pOther);
	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT ZapTouch(CBaseEntity *pOther);

	void MovetoTarget(const Vector &vecTarget);
	void Crawl(void);
	void Zap(void);
	void Teleport(void);

	float m_flIdealVel;
	Vector m_vecIdeal;
	CNihilanth *m_pNihilanth;
	EHANDLE m_hTouch;
};

LINK_ENTITY_TO_CLASS( nihilanth_energy_ball, CNihilanthHVR );


TYPEDESCRIPTION	CNihilanthHVR::m_SaveData[] =
{
	DEFINE_FIELD( CNihilanthHVR, m_flIdealVel, FIELD_FLOAT ),
	DEFINE_FIELD( CNihilanthHVR, m_vecIdeal, FIELD_VECTOR ),
	DEFINE_FIELD( CNihilanthHVR, m_pNihilanth, FIELD_CLASSPTR ),
	DEFINE_FIELD( CNihilanthHVR, m_hTouch, FIELD_EHANDLE ),
//	DEFINE_FIELD( CNihilanthHVR, m_nFrames, FIELD_INTEGER ),
};


IMPLEMENT_SAVERESTORE( CNihilanthHVR, CBaseMonster );


//=========================================================
// Nihilanth, final Boss monster
//=========================================================

const char *CNihilanth::pAttackSounds[] =
{
	"X/x_attack1.wav",
	"X/x_attack2.wav",
	"X/x_attack3.wav",
};

const char *CNihilanth::pBallSounds[] =
{
	"X/x_ballattack1.wav",
};

const char *CNihilanth::pShootSounds[] =
{
	"X/x_shoot1.wav",
};

const char *CNihilanth::pRechargeSounds[] =
{
	"X/x_recharge1.wav",
	"X/x_recharge2.wav",
	"X/x_recharge3.wav",
};

const char *CNihilanth::pLaughSounds[] =
{
	"X/x_laugh1.wav",
	"X/x_laugh2.wav",
};

const char *CNihilanth::pPainSounds[] =
{
	"X/x_pain1.wav",
	"X/x_pain2.wav",
};

const char *CNihilanth::pDeathSounds[] =
{
	"X/x_die1.wav",
};


void CNihilanth :: Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.nihilanthHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = 30;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_YELLOW;// XDM
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.nihilanthScore;
	if (m_iGibCount == 0)
		m_iGibCount = 160;// XDM3038a

	CBaseMonster::Spawn();// XDM3038b: Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	//SET_MODEL(ENT(pev), STRING(pev->model));// XDM3037
	// UTIL_SetSize(this, Vector( -300, -300, 0), Vector(300, 300, 512));
	UTIL_SetSize(this, Vector( -32, -32, 0), Vector(32, 32, 64));
	UTIL_SetOrigin(this, pev->origin);

	pev->flags			|= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->view_ofs.Set( 0, 0, 300 );
	pev->sequence = 0;
	m_flFieldOfView = -1; // 360 degrees
	ResetSequenceInfo( );

	InitBoneControllers();

	SetThink(&CNihilanth::StartupThink);
	SetNextThink(0.1f);

	m_vecDesired.Set(1, 0, 0);
	m_posDesired.Set(pev->origin.x, pev->origin.y, 512.0f);

	m_iLevel = 1;
	m_iTeleport = 1;

	if (m_szRechargerTarget[0] == '\0')	strcpy( m_szRechargerTarget, "n_recharger" );
	if (m_szDrawUse[0] == '\0')			strcpy( m_szDrawUse, "n_draw" );
	if (m_szTeleportUse[0] == '\0')		strcpy( m_szTeleportUse, "n_leaving" );
	if (m_szTeleportTouch[0] == '\0')	strcpy( m_szTeleportTouch, "n_teleport" );
	if (m_szDeadUse[0] == '\0')			strcpy( m_szDeadUse, "n_dead" );
	if (m_szDeadTouch[0] == '\0')		strcpy( m_szDeadTouch, "n_ending" );

	// near death
	/*
	m_iTeleport = 10;
	m_iLevel = 10;
	m_irritation = 2;
	pev->health = 100;
	*/
	SetBits(pev->spawnflags, SF_NORESPAWN);// XDM3038b: force no respawn for this one
}

void CNihilanth::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/nihilanth.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "X");

	CBaseMonster::Precache();// XDM3038a

	PRECACHE_MODEL("sprites/eshock.spr");
	sprEShockStart = PRECACHE_MODEL("sprites/eshock_start.spr");
	sprWarpGlow1 = PRECACHE_MODEL("sprites/warp_glow1.spr");// XDM
	sprWarpGlow2 = PRECACHE_MODEL("sprites/warp_glow2.spr");
	UTIL_PrecacheOther( "nihilanth_energy_ball" );
	UTIL_PrecacheOther( "monster_alien_controller" );
	UTIL_PrecacheOther( "monster_alien_slave" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBallSounds );
	PRECACHE_SOUND_ARRAY( pShootSounds );
	PRECACHE_SOUND_ARRAY( pRechargeSounds );
	PRECACHE_SOUND_ARRAY( pLaughSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND("debris/beamstart7.wav");
}

void CNihilanth :: PainSound(void)
{
	if (m_flNextPainSound > gpGlobals->time)
		return;

	m_flNextPainSound = gpGlobals->time + RANDOM_FLOAT( 2, 5 );

	if (pev->health > gSkillData.nihilanthHealth / 2)
	{
		EMIT_SOUND( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pLaughSounds ), GetSoundVolume(), 0.2 );
	}
	else if (m_irritation >= 2)
	{
		EMIT_SOUND( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), GetSoundVolume(), 0.2 );
	}
}

void CNihilanth :: DeathSound(void)
{
	EMIT_SOUND( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDeathSounds ), GetSoundVolume(), 0.1 );
}


void CNihilanth::NullThink(void)
{
	StudioFrameAdvance( );
	SetNextThink(0.5);
}


void CNihilanth::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink(&CNihilanth::HuntThink);
	SetNextThink(0.1);
	SetUse(&CNihilanth::CommandUse);
}


void CNihilanth::StartupThink(void)
{
	m_irritation = 0;
	m_flAdj = 512;

	CBaseEntity *pEntity;

	pEntity = UTIL_FindEntityByTargetname( NULL, "n_min");
	if (pEntity)
		m_flMinZ = pEntity->pev->origin.z;
	else
		m_flMinZ = -4096;

	pEntity = UTIL_FindEntityByTargetname( NULL, "n_max");
	if (pEntity)
		m_flMaxZ = pEntity->pev->origin.z;
	else
		m_flMaxZ = 4096;

	m_hRecharger = this;
	for (int i = 0; i < N_SPHERES; i++)
	{
		EmitSphere( );
	}
	m_hRecharger = NULL;

	SetThink(&CNihilanth::HuntThink);
	SetUse(&CNihilanth::CommandUse);
	SetNextThink(0.1);
}

/*void CNihilanth::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	CBaseMonster::Killed(pInflictor, pAttacker, iGib);
}*/

void CNihilanth :: DyingThink(void)
{
	SetNextThink(0.1);
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (pev->deadflag == DEAD_NO)
	{
/*		CSprite *m_pShock = CSprite::SpriteCreate("sprites/eshock_start.spr", pev->origin, TRUE);
		if (m_pShock)// XDM
		{
			m_pShock->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation);
			m_pShock->SetAttachment(edict(), 1);
			m_pShock->SetScale(3.0);
			m_pShock->TurnOn();
			m_pShock->AnimateAndDie(24);
		}*/
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_SPRITE);
			WRITE_COORD(pev->origin.x);// pos
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z+256);
			WRITE_SHORT(sprEShockStart);
			WRITE_BYTE(40);// size * 10
			WRITE_BYTE(255);
		MESSAGE_END();

		DeathSound( );
		pev->deadflag = DEAD_DYING;

		m_posDesired.z = m_flMaxZ;

		ParticlesCustom(pev->origin, 400, 2.0, 128, 192);// XDM3038
		if (g_pGameRules->FAllowEffects())
		{
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_BEAMDISK);// XDM3038
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z + 1024);
			WRITE_SHORT(g_iModelIndexShockWave);
			WRITE_BYTE(0);	// startframe
			WRITE_BYTE(20);	// framerate
			WRITE_BYTE(30);	// life
			WRITE_BYTE(1);	// width
			WRITE_BYTE(16);	// noise
			WRITE_BYTE(207);// color
			WRITE_BYTE(127);
			WRITE_BYTE(255);
			WRITE_BYTE(207);// brightness
			WRITE_BYTE(0);	// speed
		MESSAGE_END();
		}
	}

	if (pev->deadflag == DEAD_DYING)
	{
		Flight( );

		if (fabs( pev->origin.z - m_flMaxZ ) < 16)
		{
			pev->velocity.Clear();
			FireTargets( m_szDeadUse, this, this, USE_ON, 1.0 );
			pev->deadflag = DEAD_DEAD;
			if (m_iGibModelIndex != 0)// XDM3038c
			{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_BREAKMODEL);
				WRITE_COORD(pev->origin.x);// pos
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z - 64);
				WRITE_COORD(256);// size
				WRITE_COORD(256);
				WRITE_COORD(400);
				WRITE_COORD(0);// velocity
				WRITE_COORD(0);
				WRITE_COORD(160);
				WRITE_BYTE(30);// randomization
				WRITE_SHORT(m_iGibModelIndex);// XDM3038c g_iModelIndexAGibs);//model id#
				WRITE_BYTE(m_iGibCount);// # of shards
				WRITE_BYTE(200);// duration
				WRITE_BYTE(BREAK_FLESH | BREAK_SMOKE);// flags: unfortunately, we cannot make them additive
			MESSAGE_END();
			}
		}
	}

	if (m_fSequenceFinished)
	{
		pev->avelocity.y += RANDOM_FLOAT( -100, 100 );
		if (pev->avelocity.y < -100)
			pev->avelocity.y = -100;
		else if (pev->avelocity.y > 100)
			pev->avelocity.y = 100;

		pev->sequence = LookupSequence( "die1" );
	}

	if (m_pBall)
	{
		if (m_pBall->pev->renderamt > 0)
		{
			m_pBall->pev->renderamt = max( 0, m_pBall->pev->renderamt - 2);
		}
		else
		{
			UTIL_Remove( m_pBall );
			m_pBall = NULL;
		}
	}

	Vector vecDir, vecSrc, vecAngles;

	UTIL_MakeAimVectors( pev->angles );
	int iAttachment = RANDOM_LONG( 1, 4 );

	do {
		vecDir.Set( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ));
	} while (DotProduct( vecDir, vecDir) > 1.0);

	switch (RANDOM_LONG(1, 4))
	{
	case 1: // head
		vecDir.z = fabs( vecDir.z ) * 0.5;
		vecDir = vecDir + 2 * gpGlobals->v_up;
		break;
	case 2: // eyes
		if (DotProduct( vecDir, gpGlobals->v_forward ) < 0)
			vecDir = vecDir * -1;

		vecDir = vecDir + 2 * gpGlobals->v_forward;
		break;
	case 3: // left hand
		if (DotProduct( vecDir, gpGlobals->v_right ) > 0)
			vecDir = vecDir * -1;
		vecDir = vecDir - 2 * gpGlobals->v_right;
		break;
	case 4: // right hand
		if (DotProduct( vecDir, gpGlobals->v_right ) < 0)
			vecDir = vecDir * -1;
		vecDir = vecDir + 2 * gpGlobals->v_right;
		break;
	}

	GetAttachment( iAttachment - 1, vecSrc, vecAngles );

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 4096, ignore_monsters, ENT(pev), &tr );

	MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
		WRITE_BYTE( TE_BEAMENTPOINT );
		WRITE_SHORT( entindex() + 0x1000 * iAttachment );
		WRITE_COORD( tr.vecEndPos.x);
		WRITE_COORD( tr.vecEndPos.y);
		WRITE_COORD( tr.vecEndPos.z);
		WRITE_SHORT( g_iModelIndexLaser );
		WRITE_BYTE( 0 ); // frame start
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 5 ); // life
		WRITE_BYTE( 100 );  // width
		WRITE_BYTE( 120 );   // noise
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 192 );		// g
		WRITE_BYTE( 255);		// b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();

	GetAttachment( 0, vecSrc, vecAngles );
	CNihilanthHVR *pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
	if (pEntity)
	{
		pEntity->pev->owner = edict();// XDM3037: old compatibility hack
		pEntity->pev->velocity.Set(RANDOM_FLOAT( -0.7, 0.7 ), RANDOM_FLOAT( -0.7, 0.7 ), 1.0f);
		pEntity->pev->velocity *= 600.0f;
		pEntity->GreenBallInit( );
	}
	return;
}



void CNihilanth::CrashTouch(CBaseEntity *pOther)
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP)
	{
		SetTouchNull();
		SetNextThink(0);
	}
}



bool CNihilanth :: GibMonster(void)
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "debris/beamstart7.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
	return false;
}



void CNihilanth :: FloatSequence(void)
{
	if (m_irritation >= 2)
	{
		pev->sequence = LookupSequence( "float_open" );
	}
	else if (m_avelocity.y > 30)
	{
		pev->sequence = LookupSequence( "walk_r" );
	}
	else if (m_avelocity.y < -30)
	{
		pev->sequence = LookupSequence( "walk_l" );
	}
	else if (m_velocity.z > 30)
	{
		pev->sequence = LookupSequence( "walk_u" );
	}
	else if (m_velocity.z < -30)
	{
		pev->sequence = LookupSequence( "walk_d" );
	}
	else
	{
		pev->sequence = LookupSequence( "float" );
	}
}


void CNihilanth :: ShootBalls(void)
{
	if (m_flShootEnd > gpGlobals->time)
	{
		Vector vecHand, vecAngle;
		while (m_flShootTime < m_flShootEnd && m_flShootTime < gpGlobals->time)
		{
			if (m_hEnemy != NULL)
			{
				Vector vecSrc, vecDir;
				CNihilanthHVR *pEntity;
				GetAttachment( 2, vecHand, vecAngle );
				vecSrc = vecHand + pev->velocity * (m_flShootTime - gpGlobals->time);
				// vecDir = (m_posTarget - vecSrc).Normalize( );
				vecDir = (m_posTarget - pev->origin).Normalize( );
				vecSrc = vecSrc + vecDir * (gpGlobals->time - m_flShootTime);
				pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
				if (pEntity)
				{
					pEntity->pev->owner = edict();// XDM3037: old compatibility hack
					pEntity->pev->velocity = vecDir * 200.0;
					pEntity->ZapInit( m_hEnemy );
				}
				GetAttachment( 3, vecHand, vecAngle );
				vecSrc = vecHand + pev->velocity * (m_flShootTime - gpGlobals->time);
				// vecDir = (m_posTarget - vecSrc).Normalize( );
				vecDir = (m_posTarget - pev->origin).Normalize( );
				vecSrc += vecDir * (gpGlobals->time - m_flShootTime);
				pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
				if (pEntity)
				{
					pEntity->pev->owner = edict();// XDM3037: old compatibility hack
					pEntity->pev->velocity = vecDir * 200.0;
					pEntity->ZapInit( m_hEnemy );
				}
			}
			m_flShootTime += 0.2;
		}
	}
}

void CNihilanth::MakeFriend(const Vector &vecStart)
{
	for (size_t i = 0; i < N_MAX_FRIENDS; i++)
	{
		if (m_hFriend[i] != NULL && !m_hFriend[i]->IsAlive())
		{
			if (pev->rendermode == kRenderNormal) // don't do it if they are already fading
				m_hFriend[i]->MyMonsterPointer()->FadeMonster( );
			m_hFriend[i] = NULL;
		}

		if (m_hFriend[i].Get() == NULL)
		{
			int iSprite = 0;
			if (RANDOM_LONG(0, 1) == 0)
			{
				iSprite = sprWarpGlow1;
				int iNode = WorldGraph.FindNearestNode ( vecStart, bits_NODE_AIR );
				if (iNode != NO_NODE)
				{
					CNode &node = WorldGraph.Node( iNode );
					TraceResult tr;
					// WARNING! This causes ShouldCollide(x, NULL) call and a possible crash!
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 32), node.m_vecOrigin + Vector(0, 0, 32), dont_ignore_monsters, large_hull, edict(), &tr);// XDM3038c: pentIgnore was NULL
					if (tr.fStartSolid == 0)
						m_hFriend[i] = Create("monster_alien_controller", node.m_vecOrigin, pev->angles, g_vecZero, edict(), SF_MONSTER_FADECORPSE|SF_NOREFERENCE|SF_NORESPAWN);// XDM3038
				}
			}
			else
			{
				iSprite = sprWarpGlow2;
				int iNode = WorldGraph.FindNearestNode ( vecStart, bits_NODE_LAND | bits_NODE_WATER );
				if (iNode != NO_NODE)
				{
					CNode &node = WorldGraph.Node( iNode );
					TraceResult tr;
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 36), node.m_vecOrigin + Vector(0, 0, 36), dont_ignore_monsters, human_hull, edict(), &tr);// XDM3038c: pentIgnore was NULL
					if (tr.fStartSolid == 0)
						m_hFriend[i] = Create("monster_alien_slave", node.m_vecOrigin, pev->angles, g_vecZero, edict(), SF_MONSTER_FADECORPSE|SF_NOREFERENCE|SF_NORESPAWN);// XDM3038
				}
			}
			if (m_hFriend[i] != NULL)
			{
				EMIT_SOUND( m_hFriend[i]->edict(), CHAN_WEAPON, "debris/beamstart7.wav", VOL_NORM, ATTN_NORM );
				Vector vecSpot(m_hFriend[i]->pev->origin);// XDM
				ParticlesCustom(vecSpot, 320, 1.0, 176, 160);
				MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSpot);
					WRITE_BYTE(TE_GLOWSPRITE);
					WRITE_COORD(vecSpot.x);
					WRITE_COORD(vecSpot.y);
					WRITE_COORD(vecSpot.z);
					WRITE_SHORT(iSprite);
					WRITE_BYTE(5);		// life in 0.1
					WRITE_BYTE(10);		// scale in 0.1
					WRITE_BYTE(200);	// fade time
				MESSAGE_END();
			}
			return;
		}
	}
}


void CNihilanth::DestroyFriends(void)
{
	for (size_t i = 0; i < N_MAX_FRIENDS; i++)
	{
		if (m_hFriend[i].Get())
		{
			Vector vecSpot = m_hFriend[i]->pev->origin;// XDM
			EMIT_SOUND(m_hFriend[i]->edict(), CHAN_WEAPON, "debris/beamstart7.wav", VOL_NORM, ATTN_NORM);
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSpot);
				WRITE_BYTE(TE_GLOWSPRITE);
				WRITE_COORD(vecSpot.x);
				WRITE_COORD(vecSpot.y);
				WRITE_COORD(vecSpot.z);
				switch (RANDOM_LONG(0,1))
				{
				case 0: WRITE_SHORT(sprWarpGlow1); break;
				case 1: WRITE_SHORT(sprWarpGlow2); break;
				}
				WRITE_BYTE(5);		// life in 0.1
				WRITE_BYTE(10);		// scale in 0.1
				WRITE_BYTE(200);	// fade time
			MESSAGE_END();
			m_hFriend[i]->Killed(this, this, GIB_DISINTEGRATE);
			m_hFriend[i] = NULL;
			if (g_pGameRules->FAllowEffects())
				ParticlesCustom(vecSpot, 320, 1.0, 176, 160);
		}
	}
}

void CNihilanth :: NextActivity( )
{
	UTIL_MakeAimVectors( pev->angles );

	if (m_irritation >= 2)
	{
		if (m_pBall == NULL)
		{
			m_pBall = CSprite::SpriteCreate( "sprites/eshock.spr", pev->origin, TRUE );
			if (m_pBall)
			{
				m_pBall->SetTransparency( kRenderTransAdd, 0, 255, 160, 255, kRenderFxNoDissipation );
				m_pBall->SetAttachment( edict(), 1 );
				m_pBall->SetScale( 3.0 );
				m_pBall->pev->framerate = 10.0;
				m_pBall->TurnOn( );
			}
		}

		if (m_pBall)
		{
			MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 );		// entity, attachment
				WRITE_COORD( pev->origin.x );		// origin
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( 256 );	// radius
				WRITE_BYTE( 0 );	// R
				WRITE_BYTE( 200 );	// G
				WRITE_BYTE( 160 );	// B
				WRITE_BYTE( 200 );	// life * 10
				WRITE_COORD( 0 ); // decay
			MESSAGE_END();
		}
	}

	if ((pev->health < gSkillData.nihilanthHealth / 2 || m_iActiveSpheres < N_SPHERES / 2) && m_hRecharger.Get() == NULL && m_iLevel <= 9)
	{
		char szName[64];

		CBaseEntity *pEnt = NULL;
		CBaseEntity *pRecharger = NULL;
		float flDist = 8192;

		sprintf(szName, "%s%d", m_szRechargerTarget, m_iLevel );

		while ((pEnt = UTIL_FindEntityByTargetname( pEnt, szName )) != NULL)
		{
			vec_t flLocal = (pEnt->pev->origin - pev->origin).Length();
			if (flLocal < flDist)
			{
				flDist = flLocal;
				pRecharger = pEnt;
			}
		}

		if (pRecharger)
		{
			m_hRecharger = pRecharger;
			m_posDesired.Set( pev->origin.x, pev->origin.y, pRecharger->pev->origin.z );
			m_vecDesired = (pRecharger->pev->origin - m_posDesired).Normalize( );
			m_vecDesired.z = 0;
			m_vecDesired.NormalizeSelf();
		}
		else
		{
			m_hRecharger = NULL;
			ALERT( at_aiconsole, "nihilanth can't find %s\n", szName );
			m_iLevel++;
			if (m_iLevel > 9)
				m_irritation = 2;
		}
	}

	vec_t flDist = (m_posDesired - pev->origin).Length();
	vec_t flDot = DotProduct( m_vecDesired, gpGlobals->v_forward );

	if (m_hRecharger != NULL)
	{
		// at we at power up yet?
		if (flDist < 128.0)
		{
			int iseq = LookupSequence( "recharge" );
			if (iseq != pev->sequence)
			{
				char szText[64];

				sprintf( szText, "%s%d", m_szDrawUse, m_iLevel );
				FireTargets( szText, this, this, USE_ON, 1.0 );

				ALERT( at_console, "fireing %s\n", szText );
			}
			pev->sequence = LookupSequence( "recharge" );
		}
		else
		{
			FloatSequence( );
		}
		return;
	}

	if (m_hEnemy != NULL && !m_hEnemy->IsAlive())
	{
		m_hEnemy = NULL;
	}

	if (m_flLastSeen + 15 < gpGlobals->time)
	{
		m_hEnemy = NULL;
	}

	if (m_hEnemy.Get() == NULL)
	{
		Look( 4096 );
		m_hEnemy = BestVisibleEnemy( );
	}

	if (m_hEnemy != NULL && m_irritation != 0)
	{
		if (m_flLastSeen + 5 > gpGlobals->time && flDist < 256 && flDot > 0)
		{
			if (m_irritation >= 2 && pev->health < gSkillData.nihilanthHealth / 2.0)
			{
				pev->sequence = LookupSequence( "attack1_open" );
			}
			else
			{
				if (RANDOM_LONG(0, 1 ) == 0)
				{
					pev->sequence = LookupSequence( "attack1" ); // zap
				}
				else
				{
					char szText[64];

					sprintf( szText, "%s%d", m_szTeleportTouch, m_iTeleport );
					CBaseEntity *pTouch = UTIL_FindEntityByTargetname( NULL, szText );

					sprintf( szText, "%s%d", m_szTeleportUse, m_iTeleport );
					CBaseEntity *pTrigger = UTIL_FindEntityByTargetname( NULL, szText );

					if (pTrigger != NULL || pTouch != NULL)
					{
						pev->sequence = LookupSequence( "attack2" ); // teleport
					}
					else
					{
						m_iTeleport++;
						pev->sequence = LookupSequence( "attack1" ); // zap
					}
				}
			}
			return;
		}
	}

	FloatSequence( );
}

void CNihilanth :: HuntThink(void)
{
	SetNextThink(0.1);
	DispatchAnimEvents( );
	StudioFrameAdvance( );

	ShootBalls( );

	// if dead, force cancelation of current animation
	if (pev->health <= 0)
	{
		DestroyFriends();// XDM3038
		SetThink(&CNihilanth::DyingThink);
		m_fSequenceFinished = TRUE;
		return;
	}

	// ALERT( at_console, "health %.0f\n", pev->health );

	// if damaged, try to abosorb some spheres
	if (pev->health < gSkillData.nihilanthHealth && AbsorbSphere( ))
	{
		pev->health += gSkillData.nihilanthHealth / N_SPHERES;
	}

	// get new sequence
	if (m_fSequenceFinished)
	{
		// if (!m_fSequenceLoops)
		pev->frame = 0;
		NextActivity( );
		ResetSequenceInfo( );
		pev->framerate = 2.0 - 1.0 * (pev->health / gSkillData.nihilanthHealth);
	}

	// look for current enemy
	if (m_hEnemy.Get() != NULL && m_hRecharger.Get() == NULL)
	{
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->pev->origin;
			m_vecTarget = (m_posTarget - pev->origin).Normalize();
			m_vecDesired = m_vecTarget;
			m_posDesired.Set( pev->origin.x, pev->origin.y, m_posTarget.z + m_flAdj );
		}
		else
		{
			m_flAdj = min( m_flAdj + 10, 1000 );
		}
	}

	// don't go too high
	if (m_posDesired.z > m_flMaxZ)
		m_posDesired.z = m_flMaxZ;

	// don't go too low
	if (m_posDesired.z < m_flMinZ)
		m_posDesired.z = m_flMinZ;

	Flight( );
}



void CNihilanth :: Flight(void)
{
	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + m_avelocity );
	// Vector vecEst1 = pev->origin + m_velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );

	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if (flSide < 0)
	{
		if (m_avelocity.y < 180)
		{
			m_avelocity.y += 6; // 9 * (3.0/2.0);
		}
	}
	else
	{
		if (m_avelocity.y > -180)
		{
			m_avelocity.y -= 6; // 9 * (3.0/2.0);
		}
	}
	m_avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	Vector vecEst = pev->origin + m_velocity * 2.0 + gpGlobals->v_up * m_flForce * 20;

	// add immediate force
	UTIL_MakeAimVectors( pev->angles );
	m_velocity.x += gpGlobals->v_up.x * m_flForce;
	m_velocity.y += gpGlobals->v_up.y * m_flForce;
	m_velocity.z += gpGlobals->v_up.z * m_flForce;


	float flSpeed = m_velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0.0f ), Vector( m_velocity.x, m_velocity.y, 0.0f ) );
	if (flDir < 0)
		flSpeed = -flSpeed;

//	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// sideways drag
	m_velocity.x *= (1.0 - fabs( gpGlobals->v_right.x ) * 0.05);
	m_velocity.y *= (1.0 - fabs( gpGlobals->v_right.y ) * 0.05);
	m_velocity.z *= (1.0 - fabs( gpGlobals->v_right.z ) * 0.05);

	// general drag
	m_velocity *= 0.995;

	// apply power to stay correct height
	if (m_flForce < 100 && vecEst.z < m_posDesired.z)
	{
		m_flForce += 10;
	}
	else if (m_flForce > -100 && vecEst.z > m_posDesired.z)
	{
		if (vecEst.z > m_posDesired.z)
			m_flForce -= 10;
	}

	UTIL_SetOrigin(this, pev->origin + m_velocity * 0.1 );
	pev->angles = pev->angles + m_avelocity * 0.1;

	// ALERT( at_console, "%5.0f %5.0f : %4.0f : %3.0f : %2.0f\n", m_posDesired.z, pev->origin.z, m_velocity.z, m_avelocity.y, m_flForce );
}


BOOL CNihilanth :: AbsorbSphere(void)
{
	for (size_t i = 0; i < N_SPHERES; i++)
	{
		if (m_hSphere[i] != NULL)
		{
			CNihilanthHVR *pSphere = (CNihilanthHVR *)((CBaseEntity *)m_hSphere[i]);
			if (pSphere)
				pSphere->AbsorbInit( );

			m_hSphere[i] = NULL;
			m_iActiveSpheres--;
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CNihilanth :: EmitSphere(void)
{
	m_iActiveSpheres = 0;
	int empty = 0;

	for (size_t i = 0; i < N_SPHERES; i++)
	{
		if (m_hSphere[i] != NULL)
		{
			m_iActiveSpheres++;
		}
		else
		{
			empty = i;
		}
	}

	if (m_iActiveSpheres >= N_SPHERES)
		return FALSE;

	Vector vecSrc(m_hRecharger->pev->origin);
	CNihilanthHVR *pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
	{
		pEntity->pev->owner = edict();// XDM3037: old compatibility hack
		pEntity->pev->velocity = pev->origin - vecSrc;
		pEntity->CircleInit( this );
	}
	m_hSphere[empty] = pEntity;
	return TRUE;
}


void CNihilanth :: 	TargetSphere( USE_TYPE useType, float value )
{
	CBaseMonster *pSphere = NULL;
	int i = 0;// XDM3035: ANSI
	for (i = 0; i < N_SPHERES; ++i)
	{
		if (m_hSphere[i] != NULL)
		{
			pSphere = m_hSphere[i]->MyMonsterPointer();
			if (pSphere->m_hEnemy.Get() == NULL)
				break;
		}
	}
	if (i == N_SPHERES || pSphere == NULL)
	{
		return;
	}

	Vector vecSrc, vecAngles;
	GetAttachment( 2, vecSrc, vecAngles );
	UTIL_SetOrigin(pSphere, vecSrc );
	pSphere->Use( this, this, useType, value );
	pSphere->pev->velocity = m_vecDesired * RANDOM_FLOAT( 50, 100 ) + Vector( RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ) );
}



void CNihilanth :: HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case 1:	// shoot
		break;
	case 2:	// zen
		if (m_hEnemy != NULL)
		{
			if (RANDOM_LONG(0,4) == 0)
				EMIT_SOUND( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), GetSoundVolume(), 0.2 );

			EMIT_SOUND( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pBallSounds ), GetSoundVolume(), 0.2 );

			MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x3000 );		// entity, attachment
				WRITE_COORD( pev->origin.x );		// origin
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( 256 );	// radius
				WRITE_BYTE( 128 );	// R
				WRITE_BYTE( 128 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 10 );	// life * 10
				WRITE_COORD( 128 ); // decay
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x4000 );		// entity, attachment
				WRITE_COORD( pev->origin.x );		// origin
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( 256 );	// radius
				WRITE_BYTE( 128 );	// R
				WRITE_BYTE( 128 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 10 );	// life * 10
				WRITE_COORD( 128 ); // decay
			MESSAGE_END();

			m_flShootTime = gpGlobals->time;
			m_flShootEnd = gpGlobals->time + 1.0;
		}
		break;
	case 3:	// prayer
		if (m_hEnemy != NULL)
		{
			char szText[32];

			sprintf( szText, "%s%d", m_szTeleportTouch, m_iTeleport );
			CBaseEntity *pTouch = UTIL_FindEntityByTargetname( NULL, szText );

			sprintf( szText, "%s%d", m_szTeleportUse, m_iTeleport );
			CBaseEntity *pTrigger = UTIL_FindEntityByTargetname( NULL, szText );

			if (pTrigger != NULL || pTouch != NULL)
			{
				EMIT_SOUND( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), GetSoundVolume(), 0.2 );

				Vector vecSrc, vecAngles;
				GetAttachment( 2, vecSrc, vecAngles );
				CNihilanthHVR *pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
				if (pEntity)
				{
					pEntity->pev->velocity = pev->origin - vecSrc;
					pEntity->pev->owner = edict();// XDM3038: old compatibility hack
					pEntity->TeleportInit( this, m_hEnemy, pTrigger, pTouch );
				}
			}
			else
			{
				m_iTeleport++; // unexpected failure

				EMIT_SOUND( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pBallSounds ), GetSoundVolume(), 0.2 );

				ALERT( at_aiconsole, "nihilanth can't target %s\n", szText );

				MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
					WRITE_BYTE( TE_ELIGHT );
					WRITE_SHORT( entindex( ) + 0x3000 );		// entity, attachment
					WRITE_COORD( pev->origin.x );		// origin
					WRITE_COORD( pev->origin.y );
					WRITE_COORD( pev->origin.z );
					WRITE_COORD( 256 );	// radius
					WRITE_BYTE( 128 );	// R
					WRITE_BYTE( 128 );	// G
					WRITE_BYTE( 255 );	// B
					WRITE_BYTE( 10 );	// life * 10
					WRITE_COORD( 128 ); // decay
				MESSAGE_END();

				MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
					WRITE_BYTE( TE_ELIGHT );
					WRITE_SHORT( entindex( ) + 0x4000 );		// entity, attachment
					WRITE_COORD( pev->origin.x );		// origin
					WRITE_COORD( pev->origin.y );
					WRITE_COORD( pev->origin.z );
					WRITE_COORD( 256 );	// radius
					WRITE_BYTE( 128 );	// R
					WRITE_BYTE( 128 );	// G
					WRITE_BYTE( 255 );	// B
					WRITE_BYTE( 10 );	// life * 10
					WRITE_COORD( 128 ); // decay
				MESSAGE_END();

				m_flShootTime = gpGlobals->time;
				m_flShootEnd = gpGlobals->time + 1.0;
			}
		}
		break;
	case 4:	// get a sphere
		{
			if (m_hRecharger != NULL)
			{
				if (!EmitSphere( ))
				{
					m_hRecharger = NULL;
				}
			}
		}
		break;
	case 5:	// start up sphere machine
		{
			EMIT_SOUND( edict(), CHAN_VOICE , RANDOM_SOUND_ARRAY( pRechargeSounds ), GetSoundVolume(), 0.2 );
		}
		break;
	case 6:
		if (m_hEnemy != NULL)
		{
			Vector vecSrc, vecAngles;
			GetAttachment( 2, vecSrc, vecAngles );
			CNihilanthHVR *pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
			if (pEntity)
			{
				pEntity->pev->velocity = pev->origin - vecSrc;
				pEntity->ZapInit( m_hEnemy );
			}
		}
		break;
	case 7:
		/*
		Vector vecSrc, vecAngles;
		GetAttachment( 0, vecSrc, vecAngles );
		CNihilanthHVR *pEntity = (CNihilanthHVR *)Create( "nihilanth_energy_ball", vecSrc, pev->angles, edict() );
		pEntity->pev->velocity.Set( RANDOM_FLOAT( -0.7, 0.7 ), RANDOM_FLOAT( -0.7, 0.7 ), 1.0 ) * 600.0;
		pEntity->GreenBallInit( );
		*/
		break;
	}
}



void CNihilanth::CommandUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	switch (useType)
	{
	case USE_OFF:
		{
		CBaseEntity *pTouch = UTIL_FindEntityByTargetname( NULL, m_szDeadTouch );
		if ( pTouch && m_hEnemy != NULL )
			pTouch->Touch( m_hEnemy );
		}
		break;
	case USE_ON:
		if (m_irritation == 0)
		{
			m_irritation = 1;
		}
		break;
	case USE_SET:
		break;
	case USE_TOGGLE:
		break;
	}
}

int CNihilanth::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pInflictor && pInflictor->pev->owner == edict())
		return 0;

	if (flDamage >= pev->health)
	{
		pev->health = 1;
		if (m_irritation != 3)
			return 0;
	}
	PainSound();
	pev->health -= flDamage;
	return 0;
}

void CNihilanth::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (m_irritation == 3)
		m_irritation = 2;

	if (m_irritation == 2 && ptr->iHitgroup == 2 && flDamage > 2)
		m_irritation = 3;

	if (m_irritation != 3 && flDamage > 4.0f)
	{
		Vector vecBlood = (ptr->vecEndPos - pev->origin).Normalize();
		UTIL_BloodStream(ptr->vecEndPos, vecBlood, BloodColor(), flDamage + (100 - 100 * (pev->health / gSkillData.nihilanthHealth)));
	}

	// SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage * 5.0);// a little surface blood.
	// CBaseMonster::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);// XDM3038c ?
	AddMultiDamage( pAttacker, this, flDamage, bitsDamageType );
}



CBaseEntity *CNihilanth::RandomTargetname( const char *szName )
{
	int total = 0;
	CBaseEntity *pEntity = NULL;
	CBaseEntity *pNewEntity = NULL;
	while ((pNewEntity = UTIL_FindEntityByTargetname(pNewEntity, szName)) != NULL)
	{
		total++;
		if (RANDOM_LONG(0,total-1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}




//=========================================================
// Controller bouncy ball attack
//=========================================================
void CNihilanthHVR :: Spawn(void)
{
	Precache();
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 3.0;
}

void CNihilanthHVR :: Precache(void)
{
	//PRECACHE_MODEL("sprites/flare6.spr");
	PRECACHE_MODEL("sprites/nhth1.spr");
	PRECACHE_MODEL("sprites/exit1.spr");
	//PRECACHE_MODEL("sprites/animglow01.spr");
	//PRECACHE_MODEL("sprites/xspark4.spr");
	PRECACHE_MODEL("sprites/blueflare1.spr");
	PRECACHE_SOUND("x/x_ballhit.wav");
	PRECACHE_SOUND("x/x_ballzap.wav");
	PRECACHE_SOUND("x/x_teleattack1.wav");
}

// blue healing spheres
void CNihilanthHVR :: CircleInit( CBaseEntity *pTarget )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	SET_MODEL(edict(), "sprites/blueflare1.spr");
	pev->rendercolor.Set(31, 191, 255);
	pev->scale = 2.0f;
	m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM
	pev->renderamt = 255;

	UTIL_SetSize(this, g_vecZero, g_vecZero);
	UTIL_SetOrigin(this, pev->origin);

	SetThink(&CNihilanthHVR::HoverThink);
	SetTouch(&CNihilanthHVR::BounceTouch);
	SetNextThink(0.1);

	m_hTargetEnt = pTarget;
}

/*CBaseEntity *CNihilanthHVR::RandomClassname(const char *szName)
{
	int total = 0;

	CBaseEntity *pEntity = NULL;
	CBaseEntity *pNewEntity = NULL;
	while ((pNewEntity = UTIL_FindEntityByClassname(pNewEntity, szName)) != NULL)
	{
		total++;
		if (RANDOM_LONG(0,total-1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}*/

void CNihilanthHVR::HoverThink(void)
{
	SetNextThink(0.1);
	pev->frame = ((int)pev->frame + 1) % m_nFrames;

	if (m_hTargetEnt != NULL)
		CircleTarget(m_hTargetEnt->pev->origin + Vector(0, 0, 16 * N_SCALE));
	else
		Destroy();
}

void CNihilanthHVR :: AbsorbInit( void  )
{
	SetThink(&CNihilanthHVR::DissipateThink);
	pev->renderamt = 255;
	pev->renderfx = kRenderFxPulseFastWide;

	if (m_hTargetEnt.Get())
	{
//	MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM3038
		WRITE_BYTE( TE_BEAMENTS );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( m_hTargetEnt->entindex() + 0x1000 );
		WRITE_SHORT( g_iModelIndexLaser );
		WRITE_BYTE( 0 );	// framestart
		WRITE_BYTE( 20 );	// framerate
		WRITE_BYTE( 50 );	// life
		WRITE_BYTE( 80 );	// width
		WRITE_BYTE( 80 );	// noise
		WRITE_BYTE( 0 );	// r
		WRITE_BYTE( 207 );	// g
		WRITE_BYTE( 160 );	// b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 30 );	// speed
	MESSAGE_END();
	}
	if (g_pGameRules->FAllowEffects())
	{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM3038
		WRITE_BYTE(TE_ELIGHT);
		WRITE_SHORT(entindex());// entity, attachment
		WRITE_COORD(pev->origin.x);// origin
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(pev->renderamt);	// radius
		WRITE_BYTE(95);// R
		WRITE_BYTE(191);// G
		WRITE_BYTE(255);// B
		WRITE_BYTE(40);// life * 10 -- estimated decay time 5s
		WRITE_COORD(200);// decay
	MESSAGE_END();
	}
}

void CNihilanthHVR::DissipateThink(void)
{
	SetNextThink(0.1);

	if (pev->scale > 5.0)
	{
		if (g_pGameRules->FAllowEffects())
			UTIL_Sparks(pev->origin);// XDM3038

		Destroy();
		return;
	}
	pev->renderamt -= 2;
	pev->scale += 0.1;

	if (m_hTargetEnt != NULL)
	{
		CircleTarget( m_hTargetEnt->pev->origin + Vector( 0, 0, 4096 ) );
	}
	else
	{
		Destroy();
		return;
	}
}

bool CNihilanthHVR::CircleTarget(const Vector &vecTarget)
{
	Vector vecDest(vecTarget);
	Vector vecEst = pev->origin + pev->velocity * 0.5;
	Vector vecSrc(pev->origin);
	vecDest.z = 0;
	vecEst.z = 0;
	vecSrc.z = 0;
	vec_t d1 = (vecDest - vecSrc).Length() - 24 * N_SCALE;
	vec_t d2 = (vecDest - vecEst).Length() - 24 * N_SCALE;
	bool fClose = false;

	if (m_vecIdeal.IsZero())
	{
		m_vecIdeal = pev->velocity;
	}

	if (d1 < 0 && d2 <= d1)
	{
		// ALERT( at_console, "too close\n");
		m_vecIdeal = m_vecIdeal - (vecDest - vecSrc).Normalize() * 50;
	}
	else if (d1 > 0 && d2 >= d1)
	{
		// ALERT( at_console, "too far\n");
		m_vecIdeal = m_vecIdeal + (vecDest - vecSrc).Normalize() * 50;
	}
	pev->avelocity.z = d1 * 20;

	if (d1 < 32)
	{
		fClose = true;
	}

	m_vecIdeal += Vector( RANDOM_FLOAT( -2, 2 ), RANDOM_FLOAT( -2, 2 ), RANDOM_FLOAT( -2, 2 ));
	m_vecIdeal = Vector( m_vecIdeal.x, m_vecIdeal.y, 0.0f ).Normalize( ) * 200
		/* + Vector( -m_vecIdeal.y, m_vecIdeal.x, 0 ).Normalize( ) * 32 */
		+ Vector( 0.0f, 0.0f, m_vecIdeal.z );
	// m_vecIdeal = m_vecIdeal + Vector( -m_vecIdeal.y, m_vecIdeal.x, 0 ).Normalize( ) * 2;

	// move up/down
	d1 = vecTarget.z - pev->origin.z;
	if (d1 > 0 && m_vecIdeal.z < 200)
		m_vecIdeal.z += 20;
	else if (d1 < 0 && m_vecIdeal.z > -200)
		m_vecIdeal.z -= 20;

	pev->velocity = m_vecIdeal;

	// ALERT( at_console, "%.0f %.0f %.0f\n", m_vecIdeal.x, m_vecIdeal.y, m_vecIdeal.z );
	return fClose;
}


// shock balls
void CNihilanthHVR :: ZapInit( CBaseEntity *pEnemy )
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL(edict(), "sprites/nhth1.spr");
	m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->scale = 2.0;
	pev->velocity = (pEnemy->pev->origin - pev->origin).Normalize() * 200;
	m_hEnemy = pEnemy;
	SetThink(&CNihilanthHVR::ZapThink);
	SetTouch(&CNihilanthHVR::ZapTouch);
	SetNextThink(0.1);
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "x/x_ballzap.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}

void CNihilanthHVR :: ZapThink( void  )
{
	SetNextThink(0.05);

	// check world boundaries
	if (m_hEnemy.Get() == NULL || !IsInWorld())// ||  pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		SetTouchNull();
		UTIL_Remove( this );
		return;
	}

	if (pev->velocity.Length() < 2000)
		pev->velocity *= 1.2;

	// MovetoTarget( m_hEnemy->Center( ) );

	if ((m_hEnemy->Center() - pev->origin).Length() < 256)
	{
		TraceResult tr;
		UTIL_TraceLine( pev->origin, m_hEnemy->Center(), dont_ignore_monsters, edict(), &tr );

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity != NULL && pEntity->pev->takedamage)
		{
			ClearMultiDamage();
			CBaseEntity *pAttacker = GetDamageAttacker();
			pEntity->TraceAttack(pAttacker, gSkillData.nihilanthZap, pev->velocity, &tr, DMG_SHOCK);// XDM3038c: attacker
			ApplyMultiDamage(this, pAttacker);// XDM3038c: attacker
		}

		//MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM3038
			WRITE_BYTE( TE_BEAMENTPOINT );
			WRITE_SHORT( entindex() );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_SHORT( g_iModelIndexLaser );
			WRITE_BYTE( 0 );		// frame start
			WRITE_BYTE( 20 );		// framerate
			WRITE_BYTE( 3 );		// life
			WRITE_BYTE( 20 );		// width
			WRITE_BYTE( 20 );		// noise
			WRITE_BYTE( 63 );		// r
			WRITE_BYTE( 191 );		// g
			WRITE_BYTE( 255 );		// b
			WRITE_BYTE( 255 );		// brightness
			WRITE_BYTE( 10 );		// speed
		MESSAGE_END();

		UTIL_EmitAmbientSound( edict(), tr.vecEndPos, "x/x_ballhit.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG( 100, 120 ));

		SetTouchNull();
		UTIL_Remove( this );
		SetNextThink(0.2);
		return;
	}
	pev->frame = (int)(pev->frame + 1) % m_nFrames;// XDM

	if (g_pGameRules->FAllowEffects())
	{
	//MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM3038
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex( ) );		// entity, attachment
		WRITE_COORD( pev->origin.x );		// origin
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 128 );	// radius
		WRITE_BYTE( 127 );	// R
		WRITE_BYTE( 127 );	// G
		WRITE_BYTE( 255 );	// B
		WRITE_BYTE( 10 );	// life * 10
		WRITE_COORD( 128 ); // decay
	MESSAGE_END();
	}
	// Crawl( );
}

void CNihilanthHVR::ZapTouch(CBaseEntity *pOther)
{
	UTIL_EmitAmbientSound( edict(), pev->origin, "x/x_ballhit.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG( 90, 95 ) );
	if (g_pGameRules->FAllowEffects())
	{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_BYTE(20);		// radius
		WRITE_BYTE(100);	// r
		WRITE_BYTE(128);	// g
		WRITE_BYTE(255);	// b
		WRITE_BYTE(10);		// life
		WRITE_BYTE(100);	// decay
	MESSAGE_END();
	}
	StreakSplash(pev->origin, UTIL_RandomBloodVector(), 7, 32, 80, 240);// "r_efx.h"
	RadiusDamage(pev->origin, this, GetDamageAttacker(), 50, 128, CLASS_NONE, DMG_SHOCK);// XDM3038c: attacker
	pev->velocity.Clear();// XDM3038a: ???
	/*
	for (int i = 0; i < 10; i++)
	{
		Crawl( );
	}
	*/
	SetTouchNull();
	SetNextThink(0.1);
	SetThink(&CBaseEntity::SUB_Remove);
// XDM3038: unsafe	UTIL_Remove( this );
//	SetNextThink(0.2);
}

// slow teleporter that flies towards player
void CNihilanthHVR :: TeleportInit( CNihilanth *pOwner, CBaseEntity *pEnemy, CBaseEntity *pTarget, CBaseEntity *pTouch )
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->velocity.z *= 0.2;

	SET_MODEL(edict(), "sprites/exit1.spr");
	m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM

	m_hOwner = pOwner;// XDM3038
	m_pNihilanth = pOwner;
	m_hEnemy = pEnemy;
	m_hTargetEnt = pTarget;
	m_hTouch = pTouch;

	SetThink(&CNihilanthHVR::TeleportThink);
	SetTouch(&CNihilanthHVR::TeleportTouch);
	SetNextThink(0.1);

	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "x/x_teleattack1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);// XDM3038
}

// many balls flying out when Nihilanth dies
void CNihilanthHVR :: GreenBallInit( )
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->scale = 1.0;
	SET_MODEL(edict(), "sprites/exit1.spr");
	m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM
	SetTouch(&CNihilanthHVR::RemoveTouch);
}

void CNihilanthHVR :: TeleportThink( void  )
{
	SetNextThink(0.1);

	// check world boundaries
	if (m_hEnemy.Get() == NULL || !m_hEnemy->IsAlive())// XDM3037 || pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav" );
		UTIL_Remove( this );
		return;
	}

	if ((m_hEnemy->Center() - pev->origin).Length() < 128)
	{
		STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav" );

		if (m_hTargetEnt != NULL)
			m_hTargetEnt->Use( m_hEnemy, m_hEnemy, USE_ON, 1.0 );

		if (m_hTouch != NULL && m_hEnemy != NULL)
			m_hTouch->Touch( m_hEnemy );

		Destroy();// XDM3037: moved to after Use()
		return;
	}
	else
	{
		MovetoTarget( m_hEnemy->Center( ) );
	}

//	MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM3038
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex( ) );		// entity, attachment
		WRITE_COORD( pev->origin.x );		// origin
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 256 );	// radius
		WRITE_BYTE( 0 );	// R
		WRITE_BYTE( 255 );	// G
		WRITE_BYTE( 0 );	// B
		WRITE_BYTE( 10 );	// life * 10
		WRITE_COORD( 256 ); // decay
	MESSAGE_END();
	pev->frame = (int)(pev->frame + 1) % m_nFrames;// XDM
}

void CNihilanthHVR::TeleportTouch(CBaseEntity *pOther)
{
	SetTouchNull();
	STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav" );
	CBaseEntity *pEnemy = m_hEnemy;
	if (pOther == pEnemy)
	{
		if (m_hTargetEnt != NULL)
			m_hTargetEnt->Use( pEnemy, pEnemy, USE_ON, 1.0 );

		if (m_hTouch != NULL && pEnemy != NULL )
			m_hTouch->Touch( pEnemy );
	}
	else
	{
		m_pNihilanth->MakeFriend( pev->origin );
	}
	UTIL_EmitAmbientSound(edict(), pev->origin, "x/x_ballhit.wav", VOL_NORM, 0.5, 0, PITCH_LOW);// XDM3038a
	StreakSplash(pev->origin, UTIL_RandomBloodVector(), 2, 32, 80, 300);// "r_efx.h" // XDM3038a

	if (g_pGameRules->FAllowEffects())
		ParticlesCustom(pev->origin, 200, 2.0, BLOOD_COLOR_GREEN, 160);// XDM3038a

	SetNextThink(0.01);
	SetThink(&CBaseEntity::SUB_Remove);
// XDM3038: unsafe	UTIL_Remove( this );
}

void CNihilanthHVR::MovetoTarget(const Vector &vecTarget)
{
	if (m_vecIdeal.IsZero())
		m_vecIdeal = pev->velocity;

	// accelerate
	vec_t flSpeed = m_vecIdeal.Length();
	if (flSpeed > 300)
	{
		m_vecIdeal.NormalizeSelf();
		m_vecIdeal *= 300;
	}
	m_vecIdeal += (vecTarget - pev->origin).Normalize() * 300;
	pev->velocity = m_vecIdeal;
}

void CNihilanthHVR :: Crawl(void)
{
	Vector vecAim(RandomVector().Normalize());
	Vector vecPnt(pev->origin + pev->velocity * 0.2 + vecAim * 128);
	//MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);// XDM3038
		WRITE_BYTE( TE_BEAMENTPOINT );
		WRITE_SHORT( entindex() );
		WRITE_COORD( vecPnt.x);
		WRITE_COORD( vecPnt.y);
		WRITE_COORD( vecPnt.z);
		WRITE_SHORT( g_iModelIndexLaser );
		WRITE_BYTE( 0 );	// frame start
		WRITE_BYTE( 10 );	// framerate
		WRITE_BYTE( 3 );	// life
		WRITE_BYTE( 20 );	// width
		WRITE_BYTE( 80 );	// noise
		WRITE_BYTE( 64 );	// r, g, b
		WRITE_BYTE( 128 );	// r, g, b
		WRITE_BYTE( 255);	// r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );	// speed
	MESSAGE_END();
}

void CNihilanthHVR::RemoveTouch(CBaseEntity *pOther)
{
//?	STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav" );
	SetTouchNull();
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0.01);
// XDM3038: unsafe	UTIL_Remove( this );
}

void CNihilanthHVR::BounceTouch(CBaseEntity *pOther)
{
	Vector vecDir(m_vecIdeal);
	vecDir.NormalizeSelf();
	TraceResult tr;
	UTIL_GetGlobalTrace(&tr);
	float n = -DotProduct(tr.vecPlaneNormal, vecDir);
	vecDir += tr.vecPlaneNormal * (n * 2.0);
	m_vecIdeal = vecDir * m_vecIdeal.Length();
}
