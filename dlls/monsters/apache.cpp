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
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#include "effects.h"


#define SF_APACHE_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_APACHE_NOWRECKAGE		0x08
#define HITGROUP_APACHE_BLADES		6

class CApache : public CBaseMonster
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual bool GibMonster(void);
	virtual void SetObjectCollisionBox(void)
	{
		pev->absmin = pev->origin + Vector(-300, -300, -172);
		pev->absmax = pev->origin + Vector( 300,  300,  8);
	}
	void EXPORT HuntThink(void);
	void EXPORT FlyTouch(CBaseEntity *pOther);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink(void);
	void ShowDamage(void);
	void Flight(void);
	void FireRocket(void);
	BOOL FireGun(void);
	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;
	Vector m_vecTarget;
	Vector m_posTarget;
	Vector m_vecDesired;
	Vector m_posDesired;
	Vector m_vecGoal;
	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;
	int m_iSoundState; // don't save this
	int m_iExplode;
	int m_iBodyGibs;
	float m_flGoalSpeed;
	int m_iDoSmokePuff;
	CBeam *m_pBeam;
	BOOL m_flAirBlow;
};
LINK_ENTITY_TO_CLASS( monster_apache, CApache );

TYPEDESCRIPTION	CApache::m_SaveData[] =
{
	DEFINE_FIELD( CApache, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_flPrevSeen, FIELD_TIME ),
//	DEFINE_FIELD( CApache, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
//	DEFINE_FIELD( CApache, m_iExplode, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CApache, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_iDoSmokePuff, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CApache, CBaseMonster );

int	CApache::Classify (void)
{
	return m_iClass?m_iClass:CLASS_HUMAN_MILITARY;// XDM
}

void CApache::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.apacheHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = 50 * gSkillData.iSkillLevel;// XDM
	if (m_iGibCount == 0)
		m_iGibCount = 128;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.apacheScore;

	m_bloodColor = DONT_BLEED;
	CBaseMonster::Spawn();// XDM3038b: Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL(edict(), STRING(pev->model));// XDM3037
	UTIL_SetSize(this, Vector( -60, -200, -150 ), Vector( 60, 200, 4 ) );// XDM
	UTIL_SetOrigin(this, pev->origin);
	m_flFieldOfView	= -0.707; // 270 degrees
	pev->flags |= FL_MONSTER;
	pev->takedamage	= DAMAGE_AIM;
	pev->max_health = pev->health;// XDM3038a
	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0, 0xFF);
	InitBoneControllers();
	m_flAirBlow = FALSE;
	m_iRockets = 10;

	if (FBitSet(pev->spawnflags, SF_APACHE_WAITFORTRIGGER))
	{
		SetUse(&CApache::StartupUse);
	}
	else
	{
		SetThink(&CApache::HuntThink);
		SetTouch(&CApache::FlyTouch);
		SetNextThink(1.0);
	}
}

void CApache::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/apache.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "apache");

	CBaseMonster::Precache();// XDM3038a

	m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/apachegibs.mdl" );
	PRECACHE_SOUND("apache/ap_rotor2.wav");
	PRECACHE_SOUND("apache/ap_whine1.wav");
	PRECACHE_SOUND("apache/ap_fire.wav");
	PRECACHE_SOUND("apache/ap_hit.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");

	PRECACHE_SOUND("debris/water_crash1.wav");// XDM
	PRECACHE_SOUND("debris/water_crash2.wav");
	//PRECACHE_MODEL("sprites/lgtning.spr");

	UTIL_PrecacheOther( "hvr_rocket" );
}

void CApache::NullThink(void)
{
	StudioFrameAdvance();
	SetNextThink(0.5);
}

void CApache::StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CApache::HuntThink);
	SetTouch(&CApache::FlyTouch);
	SetNextThink(0.1);
	SetUseNull();
}

void CApache::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;
	STOP_SOUND(edict(), CHAN_STATIC, "apache/ap_rotor2.wav");
	UTIL_SetSize(this, Vector(-32, -32, -64), Vector(32, 32, 0));
	pev->body = 1;// XDM
	SetThink(&CApache::DyingThink);
	SetTouch(&CApache::CrashTouch);
	SetNextThink(0.1);
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;

	if (FBitSet(pev->spawnflags, SF_APACHE_NOWRECKAGE))
		m_flNextRocket = gpGlobals->time + 4.0;
	else
		m_flNextRocket = gpGlobals->time + 15.0;
}

void CApache::DyingThink(void)
{
	STOP_SOUND( ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav" );
	StudioFrameAdvance();
	SetNextThink(0.1);
	pev->avelocity *= 1.05;

	// still falling?
	if (!m_flAirBlow && m_flNextRocket > gpGlobals->time && pev->waterlevel <= 0)// XDM: pev->waterlevel <= 0
	{
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, pev->origin );
			WRITE_BYTE(TE_EXPLOSION);// This just makes a dynamic light now
			WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-150, 150));
			WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-150, 150));
			WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-150, -50));
			WRITE_SHORT(g_iModelIndexFireball);
			WRITE_BYTE(RANDOM_LONG(0,29) + 30);// scale * 10
			WRITE_BYTE(24); // framerate
			WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();
		// lots of smoke
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, pev->origin );
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD(pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD(pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT(g_iModelIndexSmoke);
			WRITE_BYTE(100);// scale * 10
			WRITE_BYTE(10);// framerate
		MESSAGE_END();
		if (g_pGameRules->FAllowEffects())
		{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSpot);
			WRITE_BYTE(TE_BREAKMODEL);
			WRITE_COORD(vecSpot.x);// position
			WRITE_COORD(vecSpot.y);
			WRITE_COORD(vecSpot.z);
			WRITE_COORD(400);// size
			WRITE_COORD(400);
			WRITE_COORD(132);
			WRITE_COORD(pev->velocity.x);// velocity
			WRITE_COORD(pev->velocity.y);
			WRITE_COORD(pev->velocity.z);
			WRITE_BYTE(50);// randomization
			WRITE_SHORT(m_iBodyGibs);//model id#
			WRITE_BYTE(RANDOM_LONG(4,8));// # of shards
			WRITE_BYTE(30);// 3.0 seconds
			WRITE_BYTE(BREAK_METAL);// flags
		MESSAGE_END();
		}
		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		SetNextThink(0.2);
		return;
	}
	else
	{
		if (pev->waterlevel >= 1)// XDM
		{
			if (RANDOM_LONG(0,1) == 0)
				EMIT_SOUND(ENT(pev), CHAN_STATIC, "debris/water_crash1.wav", VOL_NORM, 0.2);
			else
				EMIT_SOUND(ENT(pev), CHAN_STATIC, "debris/water_crash2.wav", VOL_NORM, 0.2);

			if (g_pGameRules->FAllowEffects())
			{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_BEAMDISK);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z - 144);//pev->mins.z);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z + 2000); // reach damage radius over .2 seconds
				WRITE_SHORT(g_iModelIndexShockWave);
				WRITE_BYTE(0);	// startframe
				WRITE_BYTE(10);	// framerate
				WRITE_BYTE(10);	// life
				WRITE_BYTE(1);	// width
				WRITE_BYTE(16);	// noise
				WRITE_BYTE(200);
				WRITE_BYTE(200);
				WRITE_BYTE(200);
				WRITE_BYTE(32);// brightness
				WRITE_BYTE(0);	// speed
			MESSAGE_END();
			}
			//StreakSplash(pev->origin, UTIL_RandomBloodVector(), 7, 64, 80, 480);// "r_efx.h"
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", VOL_NORM, 0.3);
			// blast circle
			MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, pev->origin );
				WRITE_BYTE(TE_BEAMCYLINDER);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z + 2000); // reach damage radius over .2 seconds
				WRITE_SHORT(g_iModelIndexShockWave);
				WRITE_BYTE(0);	// startframe
				WRITE_BYTE(0);	// framerate
				WRITE_BYTE(4);	// life
				WRITE_BYTE(32);	// width
				WRITE_BYTE(0);	// noise
				WRITE_BYTE(255);	// r, g, b
				WRITE_BYTE(255);	// r, g, b
				WRITE_BYTE(192);	// r, g, b
				WRITE_BYTE(128);	// brightness
				WRITE_BYTE(0);	// speed
			MESSAGE_END();
		}
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		// fireball
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 256 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 120 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();
		// big smoke
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512 );
			WRITE_SHORT( g_iModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 5 ); // framerate
		MESSAGE_END();
		RadiusDamage(pev->origin, this, this, pev->max_health, pev->max_health*DAMAGE_TO_RADIUS_DEFAULT, CLASS_NONE, DMG_BLAST | DMG_BURN);// XDM3038a: use g_pWorld as attacker?
		UTIL_ScreenShake(pev->origin, 3, 5, 2, pev->max_health*DAMAGE_TO_RADIUS_DEFAULT);
		// gibs
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE(TE_BREAKMODEL);
			WRITE_COORD(vecSpot.x);// position
			WRITE_COORD(vecSpot.y);
			WRITE_COORD(vecSpot.z + 64);
			WRITE_COORD(400);// size
			WRITE_COORD(400);
			WRITE_COORD(128);
			WRITE_COORD(0);// velocity
			WRITE_COORD(0);
			WRITE_COORD(200);
			WRITE_BYTE(30);// randomization
			WRITE_SHORT(m_iBodyGibs);//model id#
			WRITE_BYTE(m_iGibCount);// # of shards
			WRITE_BYTE(200);// 10.0 seconds
			if (pev->waterlevel >= 1)// XDM
				WRITE_BYTE(BREAK_METAL);
			else
				WRITE_BYTE(BREAK_METAL | BREAK_SMOKE);

		MESSAGE_END();
		SetThinkNull();
		CBaseMonster::Killed(g_pWorld, m_hTBDAttacker, GIB_REMOVE);// XDM3038a: this will call game rules logic
		//SetThink(&CBaseEntity::SUB_Remove);
		//SetNextThink(0.1);
	}
}

void CApache::FlyTouch(CBaseEntity *pOther)
{
	// bounce if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP)
	{
		TraceResult tr;
		UTIL_GetGlobalTrace(&tr);
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_hit.wav", VOL_NORM, 0.3, 0, 110 );
		// UNDONE, do a real bounce
		pev->velocity += tr.vecPlaneNormal * (pev->velocity.Length() + 200.0f);
	}
}

void CApache::CrashTouch(CBaseEntity *pOther)
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP)
	{
		SetTouchNull();
		m_flNextRocket = gpGlobals->time;
		SetNextThink(0);
	}
}

bool CApache::GibMonster(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "apache/ap_hit.wav", VOL_NORM, ATTN_NORM, 0, 200);
	return false;
}

void CApache::HuntThink(void)
{
	StudioFrameAdvance();
	SetNextThink(0.1);

	ShowDamage();

	if ( m_pGoalEnt == NULL && !FStringNull(pev->target) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if (m_pGoalEnt)
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if (m_hEnemy == NULL)
	{
		Look( 4092 );
		m_hEnemy = BestVisibleEnemy();
	}

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 5;

	if (m_hEnemy != NULL)
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center();
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	vec_t flLength = (pev->origin - m_posDesired).Length();

	if (m_pGoalEnt)
	{
		// ALERT( at_console, "%.0f\n", flLength );
		if (flLength < 128)
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if (m_pGoalEnt)
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = (pev->origin - m_posDesired).Length();
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		// float flLength2 = (m_posTarget - pev->origin).Length() * (1.5 - DotProduct((m_posTarget - pev->origin).Normalize(), pev->velocity.Normalize() ));
		// if (flLength2 < flLength)
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize( )) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize();
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize();
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight();

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		if (FireGun( ))
		{
			// slow down if we're fireing
			if (m_flGoalSpeed > 400)
				m_flGoalSpeed = 400;
		}

		// don't fire rockets and gun on easy mode
		if (gSkillData.iSkillLevel == SKILL_EASY)
			m_flNextRocket = gpGlobals->time + 10.0;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize();
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	if ((m_iRockets % 2) == 1)
	{
		FireRocket();
		if (m_iRockets <= 0)// empty
		{
			m_flNextRocket = gpGlobals->time + 10;// reload rockets
			m_iRockets = 10;
		}
		else
			m_flNextRocket = gpGlobals->time + ((gSkillData.iSkillLevel == SKILL_EASY)?0.75:0.5);
	}
	else if (pev->angles.x < 0 && DotProduct(pev->velocity, gpGlobals->v_forward) > -100 && m_flNextRocket < gpGlobals->time)
	{
		if (m_flLastSeen + 60 > gpGlobals->time)
		{
			if (m_hEnemy != NULL)
			{
				// make sure it's a good shot
				if (DotProduct( m_vecTarget, vecEst) > .965)
				{
					TraceResult tr;
					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, ignore_monsters, edict(), &tr );
					if ((tr.vecEndPos - m_posTarget).Length() < 512)
						FireRocket();
				}
			}
			else
			{
				TraceResult tr;
				UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, dont_ignore_monsters, edict(), &tr );
				// just fire when close
				if ((tr.vecEndPos - m_posTarget).Length() < 512)
					FireRocket();
			}
		}
	}
}

void CApache::Flight(void)
{
	// tilt model 5 degrees
	Vector vecAdj( 5.0f, 0.0f, 0.0f );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2 + vecAdj);
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );

	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );
	if (flSide < 0)
	{
		if (pev->avelocity.y < 60)
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
	}
	else
	{
		if (pev->avelocity.y > -60)
			pev->avelocity.y -= 8; // 9 * (3.0/2.0);
	}
	pev->avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity + vecAdj);
	Vector vecEst = pev->origin + pev->velocity * 2.0 + gpGlobals->v_up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj);
	pev->velocity += gpGlobals->v_up * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4; // 32ft/sec

	vec_t flSpeed = pev->velocity.Length();
	vec_t flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0.0f ), Vector( pev->velocity.x, pev->velocity.y, 0.0f ) );
	if (flDir < 0)
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );
	// fly sideways
	if (flSlip > 0)
	{
		if (pev->angles.z > -30 && pev->avelocity.z > -15)
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{

		if (pev->angles.z < 30 && pev->avelocity.z < 15)
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}

	// sideways drag
	pev->velocity.x *= (1.0f - fabs(gpGlobals->v_right.x) * 0.05f);
	pev->velocity.y *= (1.0f - fabs(gpGlobals->v_right.y) * 0.05f);
	pev->velocity.z *= (1.0f - fabs(gpGlobals->v_right.z) * 0.05f);

	// general drag
	pev->velocity *= 0.995f;

	// apply power to stay correct height
	if (m_flForce < 80 && vecEst.z < m_posDesired.z)
	{
		m_flForce += 12;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > m_posDesired.z)
			m_flForce -= 8;
	}

#if defined (NOSQB)
	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed && pev->angles.x + pev->avelocity.x < 40)
		pev->avelocity.x += 12.0;
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x > -20)
		pev->avelocity.x -= 12.0;
	else if (pev->angles.x + pev->avelocity.x < 0)
		pev->avelocity.x += 4.0;
	else if (pev->angles.x + pev->avelocity.x > 0)
		pev->avelocity.x -= 4.0;
#else
	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x  < 20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0;
	}
	else if (pev->angles.x + pev->avelocity.x > 0)
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0;
	}
	else if (pev->angles.x + pev->avelocity.x < 0)
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0;
	}
#endif

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce );
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce );

	// make rotor, engine sounds
	if (m_iSoundState == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav", VOL_NORM, 0.3, 0, 110 );
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );
		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		edict_t *pPlayerEdict = FIND_CLIENT_IN_PVS(edict());// XDM3035c: wtf is this anyway?
		if (!FNullEnt(pPlayerEdict))
		{
		CBaseEntity *pPlayer = CBaseEntity::Instance(pPlayerEdict);
		// UNDONE: this needs to send different sounds to every player for multiplayer.
		if (pPlayer && pPlayer->IsPlayer())
		{
			float pitch = DotProduct(pev->velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize());
			pitch = (int)(100 + pitch / 50.0);
			if (pitch > 250)
				pitch = 250;
			else if (pitch < 50)
				pitch = 50;
			else if (pitch == 100)// WTF is this!?
				pitch = 101;

			float flVol = (m_flForce / 100.0) + .1;
			if (flVol > 1.0)
				flVol = 1.0;

			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav", VOL_NORM, 0.3, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
		}
		}
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
		// ALERT( at_console, "%.0f %.2f\n", pitch, flVol );
	}
}

void CApache::FireRocket(void)
{
	static float side = 1.0f;
	static int count;

	if (m_iRockets <= 0)
		return;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + 1.5f * (gpGlobals->v_forward * 21.0f + gpGlobals->v_right * 70.0f * side + gpGlobals->v_up * -79.0f);

	switch (m_iRockets % 5)
	{
	case 0:	vecSrc += gpGlobals->v_right * 10; break;// XDM3037a: optimization
	case 1: vecSrc -= gpGlobals->v_right * 10; break;
	case 2: vecSrc += gpGlobals->v_up * 10; break;
	case 3: vecSrc -= gpGlobals->v_up * 10; break;
	case 4: break;
	}

	MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSrc );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_SHORT( g_iModelIndexSmoke );
		WRITE_BYTE( 20 ); // scale * 10
		WRITE_BYTE( 12 ); // framerate
	MESSAGE_END();

	CBaseEntity *pRocket = Create("hvr_rocket", vecSrc, pev->angles, pev->velocity + gpGlobals->v_forward * 100, edict(), SF_NORESPAWN);
	if (pRocket)
	{
		pRocket->pev->owner = edict();// XDM3037: old compatibility hack
		m_iRockets--;
	}
	side = -side;
}

BOOL CApache::FireGun(void)
{
	UTIL_MakeAimVectors(pev->angles);

	Vector posGun, angGun;
	GetAttachment(1, posGun, angGun);

	Vector vecTarget((m_posTarget - posGun).Normalize());
	Vector vecOut;
	vecOut.x = DotProduct(gpGlobals->v_forward, vecTarget);
	vecOut.y = -DotProduct(gpGlobals->v_right, vecTarget);
	vecOut.z = DotProduct(gpGlobals->v_up, vecTarget);

	Vector angles;// = UTIL_VecToAngles (vecOut);
	VectorAngles(vecOut, angles);

#if !defined (NOSQB)
	angles.x = -angles.x;
#endif
	NormalizeAngle180(&angles.x);// XDM3035c
	NormalizeAngle180(&angles.y);

	if (angles.x > m_angGun.x)
		m_angGun.x = min(angles.x, m_angGun.x + 12);
	if (angles.x < m_angGun.x)
		m_angGun.x = max(angles.x, m_angGun.x - 12);
	if (angles.y > m_angGun.y)
		m_angGun.y = min(angles.y, m_angGun.y + 12);
	if (angles.y < m_angGun.y)
		m_angGun.y = max(angles.y, m_angGun.y - 12);

	m_angGun.y = SetBoneController(0, m_angGun.y);
	m_angGun.x = SetBoneController(1, m_angGun.x);

	Vector posBarrel, angBarrel;
	GetAttachment(0, posBarrel, angBarrel);
	//Vector vecGun = (posBarrel - posGun).Normalize();
	Vector vecGun(posBarrel);
	vecGun -= posGun;
	vecGun.NormalizeSelf();

	if (DotProduct(vecGun, vecTarget) > 0.98)
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "apache/ap_fire.wav", VOL_NORM, ATTN_NORM);
		FireBullets(1, posGun, vecGun, VECTOR_CONE_4DEGREES, NULL, 8192, BULLET_12MM, 0, this, this/*m_hActivator*/, 0);
		return TRUE;
	}
	else
	{
		if (m_pBeam)
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
	}
	return FALSE;
}

void CApache::ShowDamage(void)
{
	if (m_iDoSmokePuff > 0 || RANDOM_LONG(0,99) > pev->health)
	{
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 32 );
			WRITE_SHORT( g_iModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG(0,9) + 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	if (m_iDoSmokePuff > 0)
		m_iDoSmokePuff--;
}

int CApache::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pInflictor && pInflictor->m_hOwner == this)//pInflictor->pev->owner == edict())
		return 0;

	if (flDamage > 4.0f)
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "apache/ap_hit.wav", VOL_NORM, ATTN_NORM);

	if (bitsDamageType & DMG_BLAST)
		flDamage *= 1.25f;

	if (flDamage > (gSkillData.apacheHealth * 0.6 + pev->health))// XDM
		m_flAirBlow = TRUE;

	m_hTBDAttacker = pAttacker;// XDM3038a: HACK!
	// ALERT( at_console, "%.0f\n", flDamage );
	return CBaseEntity::TakeDamage(  pInflictor, pAttacker, flDamage, bitsDamageType );
}

void CApache::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );
	// ignore blades
	if (ptr->iHitgroup == HITGROUP_APACHE_BLADES && FBitSet(bitsDamageType, DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB))
		return;

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == HITGROUP_HEAD || ptr->iHitgroup == HITGROUP_CHEST)
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		//AddMultiDamage( pAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else
	{
		// do half damage in the body
		flDamage *= 0.5f;
		//AddMultiDamage( pAttacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet(ptr->vecEndPos, 2.0);
	}
	CBaseMonster::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);// XDM3038c
}


class CApacheHVR : public CGrenade
{
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsPushable(void) {return FALSE;}// XDM
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	void EXPORT IgniteThink(void);
	void EXPORT AccelerateThink(void);
	void EXPORT BlowTouch(CBaseEntity *pOther);

	static TYPEDESCRIPTION m_SaveData[];
	int m_iTrail;
	Vector m_vecForward;
};
LINK_ENTITY_TO_CLASS( hvr_rocket, CApacheHVR );

TYPEDESCRIPTION	CApacheHVR::m_SaveData[] =
{
//	DEFINE_FIELD( CApacheHVR, m_iTrail, FIELD_INTEGER ),// Dont' save, precache
	DEFINE_FIELD( CApacheHVR, m_vecForward, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CApacheHVR, CGrenade );

void CApacheHVR::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL(edict(), STRING(pev->model));// XDM3038a
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	UTIL_SetOrigin(this, pev->origin);
	SetThink(&CApacheHVR::IgniteThink);
	SetTouch(&CApacheHVR::BlowTouch);
	UTIL_MakeAimVectors(pev->angles);
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;
	SetNextThink(0.1);
	//pev->dmg = 150;
	pev->dmg = gSkillData.DmgRPG;// XDM
}

void CApacheHVR::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/HVR.mdl");

	CBaseAnimating::Precache();
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocket1.wav");
}

void CApacheHVR::IgniteThink(void)
{
	// pev->movetype = MOVETYPE_TOSS;
	// pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;
	// make rocket sound
	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/rocket1.wav", VOL_NORM, 0.5);// XDM3035c: changed CHAN_VOICE to CHAN_BODY
	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BEAMFOLLOW);
		WRITE_SHORT(entindex());// entity
		WRITE_SHORT(m_iTrail);// model
		WRITE_BYTE(15); // life
		WRITE_BYTE(8); // width
		WRITE_BYTE(255);// r, g, b
		WRITE_BYTE(255);// r, g, b
		WRITE_BYTE(255);// r, g, b
		WRITE_BYTE(255); // brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	// set to accelerate
	SetThink(&CApacheHVR::AccelerateThink);
	SetNextThink(0.1);
}

void CApacheHVR::AccelerateThink(void)
{
	// check world boundaries
	if (!IsInWorld())// XDM3035c
	{
		STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/rocket1.wav");// XDM
		Destroy();
		return;
	}
	// accelerate
	vec_t flSpeed = pev->velocity.Length();
	if (flSpeed < 1800)
	{
		pev->velocity += m_vecForward * 200.0f;
	}
	// re-aim
	VectorAngles(pev->velocity, pev->angles);//pev->angles = UTIL_VecToAngles(pev->velocity);
	SetNextThink(0.1);
}

// XDM
void CApacheHVR::BlowTouch(CBaseEntity *pOther)
{
	STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/rocket1.wav");// XDM
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0,0,8), ignore_monsters, ENT(pev), & tr);
	if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * 4.0f);

	Explode(pev->origin, DMG_BLAST, g_iModelIndexFireball, 20, g_iModelIndexWExplosion, 20, 2.0f);
}
