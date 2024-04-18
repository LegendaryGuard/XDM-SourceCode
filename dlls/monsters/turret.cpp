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
/*

===== turret.cpp ========================================================

*/

//
// TODO:
//		Take advantage of new monster fields like m_hEnemy and get rid of that OFFSET() stuff
//		Revisit enemy validation stuff, maybe it's not necessary with the newest monster code
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "gamerules.h"
#include "effects.h"
#include "pm_materials.h"
#include "soundent.h"
#include "lightp.h"
#include "hornet.h"
#include "flamecloud.h"
#include "plasmaball.h"
#include "globals.h"
#include "game.h"
#include "crossbowbolt.h"
#include "razordisk.h"
#include "teleporter.h"


#define TURRET_GLOW_SPRITE "sprites/tu_glow.spr"

#define TURRET_SHOTS			2
#define TURRET_RANGE			(100 * 12)
#define TURRET_SPREAD			g_vecZero
#define TURRET_TURNRATE			30// angles per 0.1 second
#define TURRET_MAXWAIT			15// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN			5// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME	0.8

typedef enum turret_anim_e
{
	TURRET_ANIM_NONE = 0,
	TURRET_ANIM_FIRE,
	TURRET_ANIM_SPIN,
	TURRET_ANIM_DEPLOY,
	TURRET_ANIM_RETIRE,
	TURRET_ANIM_DIE,
} TURRET_ANIM;

// XDM3038c
enum turret_spinstate_e
{
	TURRET_SPIN_NO = 0,
	TURRET_SPIN_YES,
	TURRET_SPIN_UP,
	TURRET_SPIN_DOWN
};

// XDM3038c
enum turret_firemode_e
{
	TURRET_FIRE_BULLETS = 0,
	TURRET_FIRE_LIGHTP,
	TURRET_FIRE_HORNETS,
	TURRET_FIRE_FLAME,
	TURRET_FIRE_PLASMA,
	TURRET_FIRE_AGRENADES,
	TURRET_FIRE_LGRENADES,
	TURRET_FIRE_LGRENADES_C,
	TURRET_FIRE_BOLTS,
	TURRET_FIRE_BOLTS_E,
	TURRET_FIRE_RAZORDISKS,
	TURRET_FIRE_RAZORDISKS_E,
	TURRET_FIRE_ROCKETS,
	TURRET_FIRE_TELEPORTERS1,
	TURRET_FIRE_TELEPORTERS2
};

class CBaseTurret : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void KeyValue(KeyValueData *pkvd);
	//virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM3035a
	virtual int Classify(void);
	virtual bool GibMonster(void) { return false; }
	virtual bool ShouldGibMonster(int iGib) const { return false; }// XDM3035a
	virtual bool HasHumanGibs(void) { return false; }// XDM3035c
	virtual bool IsPushable(void) { return false; }// XDM
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	//virtual void OnFreePrivateData(void);
	void EXPORT TurretUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	// Think functions
	void EXPORT ActiveThink(void);
	void EXPORT SearchThink(void);
	void EXPORT AutoSearchThink(void);
	void EXPORT TurretDeath(void);
	//void EXPORT SpinUpThink(void);// XDM3038c
	//void EXPORT SpinDownThink(void);// XDM3038c
	virtual void SpinDown(void);
	virtual void SpinUp(void);
	void EXPORT Deploy(void);
	void EXPORT Retire(void);
	void EXPORT Initialize(void);
	virtual void Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy);
	virtual void Ping(void);
	virtual void EyeOn(void);
	virtual void EyeOff(void);
	void EyeDestroy(void);
	virtual void AlertSound(void);// XDM
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void SetEyeColor(int r, int g, int b);// XDM
	virtual void SetEyeBrightness(int a);// XDM
	// other functions
	void SetTurretAnim(TURRET_ANIM anim);
	int MoveTurret(void);

	static TYPEDESCRIPTION m_SaveData[];

	float m_flMaxSpin;// Max time to spin the barrel w/o a target
	int m_iSpin;
	CSprite *m_pEyeGlow;
	int	m_eyeBrightness;
	int	m_iDeployHeight;
	int	m_iRetractHeight;
	int m_iMinPitch;
	int m_iBaseTurnRate;// angles per second
	float m_fTurnRate;// actual turn rate
	int m_iOrientation;// 0 = floor, 1 = Ceiling
	int	m_iOn;
	int m_fBeserk;			// Sometimes this bitch will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
					// enters its range
	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
// XDM	int m_iSearchSpeed;		// Not Used!
	// movement
	float	m_flStartYaw;
	Vector	m_vecCurAngles;
	Vector	m_vecGoalAngles;
	float	m_flPingTime;	// Time until the next ping, used when searching
	float	m_flSpinUpTime;	// Amount of time until the barrel should spin down when searching
	float	m_flFireRate;		// How many rounds/second
	int		m_iFireMode;
	int		m_iBulletType;// XDM3038c: Bullet
};

TYPEDESCRIPTION	CBaseTurret::m_SaveData[] =
{
	DEFINE_FIELD( CBaseTurret, m_flMaxSpin, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iSpin, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseTurret, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iDeployHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iRetractHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iMinPitch, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iBaseTurnRate, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fTurnRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iOrientation, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fBeserk, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iAutoStart, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flMaxWait, FIELD_FLOAT ),
//	DEFINE_FIELD( CBaseTurret, m_iSearchSpeed, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_vecCurAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_vecGoalAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_flPingTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flSpinUpTime, FIELD_TIME ),
//	DEFINE_FIELD( CBaseTurret, m_flInSpinProc, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseTurret, m_flFireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iFireMode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iBulletType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBaseTurret, CBaseMonster );

//
// ID as a machine
//
int	CBaseTurret::Classify(void)
{
	if (m_iOn || m_iAutoStart)
	{
		/* not needed as we now check Owner in IRelationship	if(m_hOwner.Get() && m_hOwner->IsPlayer())// XDM3037: used by player
			return CLASS_PLAYER_ALLY;
		else*/
			return m_iClass?m_iClass:CLASS_MACHINE;
	}

	return CLASS_NONE;
}

void CBaseTurret::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "maxsleep"))
	{
		m_flMaxWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "orientation"))
	{
		m_iOrientation = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "searchspeed"))
	{
		//m_iSearchSpeed = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "turnrate"))
	{
		m_iBaseTurnRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "style") ||
			 FStrEq(pkvd->szKeyName, "height") ||
			 FStrEq(pkvd->szKeyName, "value1") ||
			 FStrEq(pkvd->szKeyName, "value2") ||
			 FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firemode"))// XDM
	{
		m_iFireMode = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bullettype"))// XDM
	{
		m_iBulletType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firerate"))// XDM
	{
		m_flFireRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}	
	else
		CBaseMonster::KeyValue(pkvd);
}

void CBaseTurret::Spawn(void)
{
	m_bloodColor = DONT_BLEED;
	m_iGibCount = 0;// XDM3038a
	CBaseMonster::Spawn();// XDM3038b: Precache();
	//SET_MODEL(edict(), STRING(pev->model));// XDM3038a
	pev->movetype		= MOVETYPE_FLY;
	pev->sequence		= 0;
	pev->frame			= 0;
	pev->solid			= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;
	SetBits(pev->flags, FL_IMMUNE_SLIME);
	MonsterInit();// Important to call this in right order!
	SetUse(&CBaseTurret::TurretUse);

	if (FBitSet(pev->spawnflags, SF_MONSTER_TURRET_AUTOACTIVATE) && !FBitSet(pev->spawnflags, SF_MONSTER_TURRET_STARTINACTIVE))
		m_iAutoStart = TRUE;

	ResetSequenceInfo();
	SetBoneController(0, 0);
	SetBoneController(1, 0);
	m_flFieldOfView = VIEW_FIELD_FULL;
	// m_flSightRange = TURRET_RANGE;
	m_MonsterState = MONSTERSTATE_NONE;
	SetNextThink(1.0);// overrides value set in MonsterInit()
}

void CBaseTurret::Precache(void)
{
	if (m_iFireMode == TURRET_FIRE_BULLETS)
	{
		if (m_iBulletType == BULLET_NONE)// XDM3038c
			conprintf(0, "Design error: %s[%d] \"%s\" with no bullet type!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
			//m_iBulletType = BULLET_9MM;
	}
	if (m_flFireRate <= 0.0f)
	{
		switch (m_iFireMode)
		{
		default: m_flFireRate = 2; break;
		case TURRET_FIRE_BULLETS:		m_flFireRate = 1/MP5_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_LIGHTP:		m_flFireRate = 1/CHEMGUN_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_HORNETS:		m_flFireRate = 1/HGUN_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_FLAME:			m_flFireRate = 1/FLAMETHROWER_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_PLASMA:		m_flFireRate = 1/PLASMA_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_AGRENADES:		m_flFireRate = 1/ALAUNCHER_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_LGRENADES:		m_flFireRate = 1/GLAUNCHER_ATTACK_INTERVAL2; break;
		case TURRET_FIRE_LGRENADES_C:	m_flFireRate = 1/GLAUNCHER_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_BOLTS:			m_flFireRate = 1/CROSSBOW_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_BOLTS_E:		m_flFireRate = 1/CROSSBOW_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_RAZORDISKS:	m_flFireRate = 1/DLAUNCHER_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_RAZORDISKS_E:	m_flFireRate = 1/DLAUNCHER_ATTACK_INTERVAL2; break;
		case TURRET_FIRE_ROCKETS:		m_flFireRate = 1/RPG_ATTACK_INTERVAL1; break;
		case TURRET_FIRE_TELEPORTERS1:	m_flFireRate = 1/DISPLACER_CHARGE_TIME1; break;
		case TURRET_FIRE_TELEPORTERS2:	m_flFireRate = 1/DISPLACER_CHARGE_TIME2; break;
		}
		conprintf(2, "%s[%d] \"%s\" using default fire rate: %g\n", STRING(pev->classname), entindex(), STRING(pev->targetname), m_flFireRate);
	}
	CBaseMonster::Precache();// XDM3038a

	if (FStringNull(pev->noise))// XDM3038a
		pev->noise = MAKE_STRING("turret/tu_fire1.wav");
	if (FStringNull(pev->noise1))// XDM3038c
		pev->noise1 = MAKE_STRING("turret/tu_deploy.wav");

	PRECACHE_SOUND(STRINGV(pev->noise));
	PRECACHE_SOUND(STRINGV(pev->noise1));// activation sound
	PRECACHE_SOUND("turret/tu_fire1.wav");
	PRECACHE_SOUND("turret/tu_ping.wav");
	PRECACHE_SOUND("turret/tu_active2.wav");
	PRECACHE_SOUND("turret/tu_die.wav");
	PRECACHE_SOUND("turret/tu_die2.wav");
	PRECACHE_SOUND("turret/tu_die3.wav");
	PRECACHE_SOUND("turret/tu_retract.wav");
	PRECACHE_SOUND("turret/tu_spinup.wav");
	PRECACHE_SOUND("turret/tu_spindown.wav");
	PRECACHE_SOUND("turret/tu_search.wav");
	PRECACHE_SOUND("turret/tu_alert.wav");
}

// XDM3038c: rarely crashes
/*void CBaseTurret::OnFreePrivateData(void)
{
	EyeDestroy();
	CBaseMonster::OnFreePrivateData();
}*/

void CBaseTurret::Initialize(void)
{
	m_iOn = 0;
	m_fBeserk = 0;
	m_iSpin = 0;
	SetBoneController(0, 0);
	SetBoneController(1, 0);
	if (m_iBaseTurnRate == 0) m_iBaseTurnRate = TURRET_TURNRATE;
	if (m_flMaxWait == 0) m_flMaxWait = TURRET_MAXWAIT;
	m_flStartYaw = pev->angles.y;
	if (m_iOrientation == 1)
	{
		pev->idealpitch = 180;
		pev->angles.x = 180;
		pev->view_ofs.z = -pev->view_ofs.z;
		pev->effects |= EF_INVLIGHT;
		pev->angles.y += 180;
		if (pev->angles.y > 360)
			pev->angles.y -= 360;
	}

	m_vecGoalAngles.x = 0;

	if (m_iAutoStart)
	{
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::AutoSearchThink);
		SetNextThink(0.1);
	}
	else
		SetThink(&CBaseEntity::SUB_DoNothing);
}

void CBaseTurret::TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!IsAlive())// XDM3037
		return;
	if (pev->deadflag == DEAD_DYING)
		return;

	if (pActivator->IsPlayer()/* && (pActivator->pev->weapons & (1<<WEAPON_SUIT))*/)// XDM: players can reactivate turrets/* with HEV computer*/
	{
		// XDM3037: we do check for pCaller because player may indirectly activate a turret (a trap), which affects gameplay
		//short gt = g_pGameRules->GetGameType();
		//if (/*(gt != GT_COOP && gt != GT_SINGLE) || */pActivator == pCaller)
		if (!(ObjectCaps() & FCAP_IMPULSE_USE) || pActivator == pCaller)// XDM3037: this is NOT a sentry or it IS, but used directly
		{
			//pev->impulse = 1;
			m_hActivator = pActivator;// remember the activator?
			m_hOwner = pActivator;// XDM3037: remember the activator
			pev->team = pActivator->pev->team;// XDM3037: assign team
			UTIL_Sparks(GetGunPosition());// XDM3037
			EMIT_SOUND(edict(), CHAN_BODY, "turret/tu_alert.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
		}
	}

	if (!ShouldToggle(useType, m_iOn == 1))
		return;

	if (m_iOn)
	{
		m_hEnemy = NULL;
		SetNextThink(0.1);
		m_iAutoStart = FALSE;// switching off a turret disables autostart
		//!!!! this should spin down first! !BUGBUG
		SetThink(&CBaseTurret::Retire);
	}
	else
	{
		SetNextThink(0.1); // turn on delay
		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if (FBitSet(pev->spawnflags, SF_MONSTER_TURRET_AUTOACTIVATE))
			m_iAutoStart = TRUE;

		SetThink(&CBaseTurret::Deploy);
	}
}

// XDM3038c
void CBaseTurret::Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy)
{
	Vector vecAngles;
	VectorAngles(vecDirToEnemy, vecAngles);
	switch (m_iFireMode)
	{
		default:
		case TURRET_FIRE_BULLETS:		FireBullets(1, vecSrc, vecDirToEnemy, TURRET_SPREAD, NULL, TURRET_RANGE, m_iBulletType, 0.0f, this, GetDamageAttacker(), 0); break;
		case TURRET_FIRE_LIGHTP:		CLightProjectile::CreateLP(vecSrc, vecAngles, vecDirToEnemy, GetDamageAttacker(), this, 0.0f, 0); break;
		case TURRET_FIRE_HORNETS:		CHornet::CreateHornet(vecSrc, vecAngles, vecDirToEnemy*800, GetDamageAttacker(), this, 0.0f, false); break;
		case TURRET_FIRE_FLAME:			CFlameCloud::CreateFlame(vecSrc, vecDirToEnemy*500, g_iModelIndexFlameFire, 0.2, 0.02, FLAMECLOUD_VELMULT, 0.0f, 255, 10, 0, GetDamageAttacker(), this); break;
		case TURRET_FIRE_PLASMA:		CPlasmaBall::CreatePB(vecSrc, vecAngles, vecDirToEnemy, PLASMABALL_SPEED, 0.0f, GetDamageAttacker(), this, 0); break;
		case TURRET_FIRE_AGRENADES:		CAGrenade::ShootTimed(vecSrc, vecAngles, vecDirToEnemy*ALAUNCHER_GRENADE_VELOCITY, GetDamageAttacker(), this, ALAUNCHER_GRENADE_TIME/2, 0.0f, 0); break;
		case TURRET_FIRE_LGRENADES:		CLGrenade::CreateGrenade(vecSrc, vecAngles, vecDirToEnemy*GLAUNCHER_GRENADE_VELOCITY, GetDamageAttacker(), this, 0.0f, GLAUNCHER_GRENADE_TIME, true); break;
		case TURRET_FIRE_LGRENADES_C:	CLGrenade::CreateGrenade(vecSrc, vecAngles, vecDirToEnemy*GLAUNCHER_GRENADE_VELOCITY, GetDamageAttacker(), this, 0.0f, GLAUNCHER_GRENADE_TIME, false); break;
		case TURRET_FIRE_BOLTS:			CCrossbowBolt::BoltCreate(vecSrc, vecAngles, vecDirToEnemy, GetDamageAttacker(), this, 0.0f, true); break;
		case TURRET_FIRE_BOLTS_E:		CCrossbowBolt::BoltCreate(vecSrc, vecAngles, vecDirToEnemy, GetDamageAttacker(), this, 0.0f, false); break;
		case TURRET_FIRE_RAZORDISKS:	CRazorDisk::DiskCreate(vecSrc, vecAngles, vecDirToEnemy, GetDamageAttacker(), this, 0.0f, 0); break;
		case TURRET_FIRE_RAZORDISKS_E:	CRazorDisk::DiskCreate(vecSrc, vecAngles, vecDirToEnemy, GetDamageAttacker(), this, 0.0f, 1); break;
		case TURRET_FIRE_ROCKETS:		CRpgRocket::CreateRpgRocket(vecSrc, vecAngles, vecDirToEnemy, GetDamageAttacker(), this, NULL, 0.0f); break;
		case TURRET_FIRE_TELEPORTERS1:	CTeleporter::CreateTeleporter(vecSrc, vecDirToEnemy*TELEPORTER_FLY_SPEED1, GetDamageAttacker(), this, m_hEnemy, 0.0f, TELEPORTER_LIFE, 0); break;
		case TURRET_FIRE_TELEPORTERS2:	CTeleporter::CreateTeleporter(vecSrc, vecDirToEnemy*TELEPORTER_FLY_SPEED1, GetDamageAttacker(), this, m_hEnemy, 0.0f, TELEPORTER_LIFE, 1); break;
	}
	if (!FStringNull(pev->noise))
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, STRING(pev->noise), VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, QUIET_GUN_VOLUME, 0.25);
	pev->effects |= EF_MUZZLEFLASH;
}

// Called almost every frame
void CBaseTurret::Ping(void)
{
	// make the pinging noise every second while searching
	if (m_flPingTime == 0)
	{
		m_flPingTime = gpGlobals->time + 1.0f;
	}
	else if (m_flPingTime <= gpGlobals->time)
	{
		m_flPingTime = gpGlobals->time + 1;
		EMIT_SOUND_DYN(edict(), CHAN_ITEM, "turret/tu_ping.wav", VOL_NORM, ATTN_NORM, 0, GetSoundPitch());// TURRET_MACHINE_VOLUME?
		SetEyeColor(255,0,0);// XDM: after activation (Deploy()) the glow is still yellow. This line fixes it.
		EyeOn();
	}
	else if (m_eyeBrightness > 0)
	{
		EyeOff();
	}
}

void CBaseTurret::EyeOn(void)
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness != 255)
			m_eyeBrightness = 255;

		m_pEyeGlow->SetBrightness( m_eyeBrightness );
	}
}

// This is not really an OFF function, it is a gradual FADE function!
void CBaseTurret::EyeOff(void)
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness > 0)
		{
			m_eyeBrightness = max( 0, m_eyeBrightness - 30 );
			m_pEyeGlow->SetBrightness( m_eyeBrightness );
		}
	}
}

void CBaseTurret::EyeDestroy(void)
{
	if (m_pEyeGlow)
	{
		UTIL_Remove(m_pEyeGlow);
		m_pEyeGlow = NULL;
	}
}

void CBaseTurret::SetEyeColor(int r, int g, int b)
{
	if (m_pEyeGlow)
		m_pEyeGlow->SetColor(r,g,b);
}

void CBaseTurret::SetEyeBrightness(int a)
{
	if (m_pEyeGlow)
		m_pEyeGlow->SetBrightness(a);
}

void CBaseTurret::Deploy(void)
{
	SetNextThink(0.1);
	StudioFrameAdvance();

	if (pev->sequence != TURRET_ANIM_DEPLOY)
	{
		if(m_pEyeGlow)// XDM: turn on the glow, startup color: yellow
		{
			EyeOn();
			SetEyeColor(255,255,0);
		}
		m_iOn = 1;
		SetTurretAnim(TURRET_ANIM_DEPLOY);
		AlertSound();
		SUB_UseTargets(this, USE_ON, 0);
	}

	if (m_fSequenceFinished)
	{
		ClearBits(pev->flags, FL_NOTARGET);// attract enemies/barnacles
		pev->maxs.z = m_iDeployHeight;
		pev->mins.z = -m_iDeployHeight;
		UTIL_SetSize(this, pev->mins, pev->maxs);
		m_vecCurAngles.x = 0;

		if (m_iOrientation == 1)
			m_vecCurAngles.y = UTIL_AngleMod(pev->angles.y + 180);
		else
			m_vecCurAngles.y = UTIL_AngleMod(pev->angles.y);

		EyeOff();// XDM: activation completed
		SetTurretAnim(TURRET_ANIM_SPIN);
		pev->framerate = 0;
		SetThink(&CBaseTurret::SearchThink);
	}

	m_flLastSight = gpGlobals->time + m_flMaxWait;
}

void CBaseTurret::Retire(void)
{
	// make the turret level
	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;
	SetNextThink(0.1);
	StudioFrameAdvance();

	EyeOff();
	SetEyeColor(63,255,0);// XDM: suspend color: green!
	//EyeOff();// XDM: Moved down

	if (!MoveTurret())
	{
		if (m_iSpin == TURRET_SPIN_YES)
		{
			SpinDown();//SpinDownCall();
		}
		else if (pev->sequence != TURRET_ANIM_RETIRE)
		{
			SetTurretAnim(TURRET_ANIM_RETIRE);
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_retract.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, 120);
			SUB_UseTargets( this, USE_OFF, 0 );
		}
		else if (m_fSequenceFinished)
		{
			SetBits(pev->flags, FL_NOTARGET);// don't attract enemies/barnacles
			SetEyeBrightness(0);// XDM
			m_iOn = 0;
			m_flLastSight = 0;
			SetTurretAnim(TURRET_ANIM_NONE);
			pev->maxs.z = m_iRetractHeight;
			pev->mins.z = -m_iRetractHeight;
			UTIL_SetSize(this, pev->mins, pev->maxs);
			if (m_iAutoStart)
			{
				SetThink(&CBaseTurret::AutoSearchThink);
				SetNextThink(0.1f);
			}
			else
				SetThink(&CBaseEntity::SUB_DoNothing);
		}
	}
	else
		SetTurretAnim(TURRET_ANIM_SPIN);
}

void CBaseTurret::SetTurretAnim(TURRET_ANIM anim)
{
	if (pev->sequence != anim)
	{
		/* XDM3038c: this block is useless!
		switch (anim)
		{
		case TURRET_ANIM_FIRE:
		case TURRET_ANIM_SPIN:
			if (pev->sequence != TURRET_ANIM_FIRE && pev->sequence != TURRET_ANIM_SPIN)
			{
				pev->frame = 0;
			}
			break;
		default:
			pev->frame = 0;
			break;
		}*/

		pev->sequence = anim;
		ResetSequenceInfo();

		switch (anim)
		{
		case TURRET_ANIM_RETIRE:
			pev->frame			= 255;
			pev->framerate		= -1.0;
			break;
		case TURRET_ANIM_DIE:
			pev->framerate		= 1.0;
			break;
		}
		//ALERT(at_console, "Turret anim #%d\n", anim);
	}
}

// Shooting think, stop rotating around and aim
void CBaseTurret::ActiveThink(void)
{
	if (POINT_CONTENTS(EyePosition()) == CONTENTS_SOLID)// XDM3038c: HACK for turrets falling through the ground
	{
		conprintf(1, "Error: %s in CONTENTS_SOLID!\n", STRING(pev->classname));
		Killed(g_pWorld, g_pWorld, GIB_REMOVE);// don't just remove because we may have some events to fire
		return;
	}

	SetNextThink(0.1);
	StudioFrameAdvance();

	if ((!m_iOn) || (m_hEnemy.Get() == NULL) || g_pGameRules && g_pGameRules->IsGameOver())
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::SearchThink);
		return;
	}

	// if it's dead, look for something new
	if (!m_hEnemy->IsAlive())
	{
		if (m_flLastSight <= 0.0f)// XDM3038c :-/
		{
			m_flLastSight = gpGlobals->time + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(&CBaseTurret::SearchThink);
				return;
			}
		}
	}

	Vector vecMid(pev->origin + pev->view_ofs);// source point
	Vector vecTarget(m_hEnemy->BodyTarget(vecMid));// destination point
	// Look for our current enemy
	bool fEnemyVisible = FVisible(m_hEnemy);// XDM3038c: UNDONE FBoxVisible(m_hEnemy, vecTarget, 0.0f);
	Vector vecDirToEnemy(vecTarget); vecDirToEnemy -= vecMid;// calculate dir and dist to enemy
	vec_t flDistToEnemy = vecDirToEnemy.NormalizeSelf();
	Vector vecAngToEnemy(0,0,0);
	VectorAngles(vecDirToEnemy, vecAngToEnemy);
	// Current enemy is not visible
	if (!fEnemyVisible || (flDistToEnemy > TURRET_RANGE))
	{
		if (m_flLastSight <= 0.0f)// XDM3038c
			m_flLastSight = gpGlobals->time + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(&CBaseTurret::SearchThink);
				return;
			}
		}
		fEnemyVisible = false;
	}
	else
		m_vecLastSight = vecTarget;

	//UTIL_DebugBeam(vecMid, vecMid+vecDirToEnemy*160, 5, 255,255,0);
	UTIL_MakeAimVectors(m_vecCurAngles);
	//UTIL_DebugBeam(vecMid, vecMid+gpGlobals->v_forward*128, 4.0f, 255,95,0);

	//ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", m_vecCurAngles.x, m_vecCurAngles.y, gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );
	//Vector vecLOS(vecDirToEnemy);? //(vecMid - m_vecLastSight).Normalize();
	Vector vecLOS(m_vecLastSight); vecLOS -= vecMid; vecLOS.NormalizeSelf();
	// Is the Gun looking at the target
	bool fAttack;
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.866) // 30 degree slop
		fAttack = false;
	else
		fAttack = true;
	// fire the gun

	//UTIL_DebugBeam(vecMid, vecMid+vecLOS*160, 5, 255,0,0);

	// XDM
	if (/*!m_flInSpinProc && */m_iSpin == TURRET_SPIN_YES && ((fAttack) || (m_fBeserk)))
	{
		if (m_flNextAttack <= gpGlobals->time)// XDM3038c: unlike normal monsters, this one shoots from code, not by model animation event
		{
			Vector vecSrc, vecAng;
			GetAttachment(0, vecSrc, vecAng);
			SetTurretAnim(TURRET_ANIM_FIRE);
			Shoot(vecSrc, gpGlobals->v_forward);
			//UTIL_ShowLine(vecSrc, vecSrc+gpGlobals->v_forward*20, 2.0f, 255, 240, 0);
			m_flNextAttack = gpGlobals->time + 1/m_flFireRate;
		}
	}
	else
		SetTurretAnim(TURRET_ANIM_SPIN);

	// move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0,9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0,360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0,90) - 90 * m_iOrientation;
			TakeDamage(this, this, 1, DMG_NOHITPOINT|DMG_IGNOREARMOR); // don't beserk forever
			return;
		}
	}
	else if (fEnemyVisible)
	{
#if !defined (NOSQB)
		vecAngToEnemy.x = -vecAngToEnemy.x;
#endif
		NormalizeAngle360(&vecAngToEnemy.y);// XDM3038
		//ALERT(at_console, "[%.2f]", vec.x);
		NormalizeAngle180(&vecAngToEnemy.x);// XDM3038

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]
		if (m_iOrientation == 0)
		{
			if (vecAngToEnemy.x > 90)
				vecAngToEnemy.x = 90;
			else if (vecAngToEnemy.x < m_iMinPitch)
				vecAngToEnemy.x = m_iMinPitch;
		}
		else
		{
			if (vecAngToEnemy.x < -90)
				vecAngToEnemy.x = -90;
			else if (vecAngToEnemy.x > -m_iMinPitch)
				vecAngToEnemy.x = -m_iMinPitch;
		}
		// ALERT(at_console, "->[%.2f]\n", vec.x);
		m_vecGoalAngles.y = vecAngToEnemy.y;
		m_vecGoalAngles.x = vecAngToEnemy.x;
	}
	if (m_iSpin != TURRET_SPIN_YES)
		SpinUp();//SpinUpCall();

	MoveTurret();
}

//
// This search function will sit with the turret deployed and look for a new target.
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//
void CBaseTurret::SearchThink(void)
{
	// ensure rethink
	SetTurretAnim(TURRET_ANIM_SPIN);
	StudioFrameAdvance();
	SetNextThink(0.1);

	if (m_flSpinUpTime == 0 && m_flMaxSpin)
		m_flSpinUpTime = gpGlobals->time + m_flMaxSpin;

	Ping();

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive())
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target
	if (m_hEnemy.Get() == NULL)
	{
		Look(TURRET_RANGE);
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, spin up the barrel and start to attack
	if (m_hEnemy != NULL)
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink(&CBaseTurret::ActiveThink);
	}
	else
	{
		// Are we out of time, do we need to retract?
 		if (gpGlobals->time > m_flLastSight)
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			m_flSpinUpTime = 0;
			SetThink(&CBaseTurret::Retire);
		}
		// should we stop the spin?
		else if ((m_flSpinUpTime != 0) && (gpGlobals->time > m_flSpinUpTime))
		{
			SpinDown();//SpinDownCall();
		}

		// generic hunt for new victims
		m_vecGoalAngles.y = (m_vecGoalAngles.y + 0.1 * m_fTurnRate);
		if (m_vecGoalAngles.y >= 360)
			m_vecGoalAngles.y -= 360;

		MoveTurret();
	}
}

//
// This think function will deploy the turret when something comes into range. This is for
// automatically activated turrets.
//
void CBaseTurret::AutoSearchThink(void)
{
	// ensure rethink
	StudioFrameAdvance();
	SetNextThink(0.3);

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() || m_hEnemy == m_hOwner)// XDM3037: don't attack the guy that activated me
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target
	if (m_hEnemy.Get() == NULL)
	{
		Look(TURRET_RANGE);
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != NULL)
	{
		SetThink(&CBaseTurret::Deploy);
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_alert.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, GetSoundPitch());
	}
}

int CBaseTurret::MoveTurret(void)
{
	int state = 0;
	// any x movement?
	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1;

		m_vecCurAngles.x += 0.1f * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

#if defined (NOSQB)
		if (m_iOrientation == 0)
			SetBoneController(1, m_vecCurAngles.x);
		else
			SetBoneController(1, -m_vecCurAngles.x);
#else
		if (m_iOrientation == 0)
			SetBoneController(1, -m_vecCurAngles.x);
		else
			SetBoneController(1, m_vecCurAngles.x);
#endif
		state = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1;
		float flAngleDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);

		if (flAngleDist > 180)
		{
			flAngleDist = 360 - flAngleDist;
			flDir = -flDir;
		}
		if (flAngleDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10)
				m_fTurnRate += m_iBaseTurnRate;
		}
		else if (m_fTurnRate > 45)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1f * m_fTurnRate * flDir;

		NormalizeAngle360(&m_vecCurAngles.y);// XDM3038

		if (flAngleDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;

		//ALERT(at_console, "%.2f -> %.2f\n", m_vecCurAngles.y, y);
		if (m_iOrientation == 0)
			SetBoneController(0, m_vecCurAngles.y - pev->angles.y);
		else
			SetBoneController(0, pev->angles.y - 180 - m_vecCurAngles.y);

		state = 1;
	}

	if (!state)
		m_fTurnRate = m_iBaseTurnRate;

	//ALERT(at_console, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x, m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y);

	if (m_iSpin == TURRET_SPIN_UP)
	{
		if (pev->framerate < 1.0f)
		{
			pev->framerate += 0.075f;
			if (pev->framerate >= 1.0f)// after the barrel is spun up, turn on the hum
			{
				pev->framerate = 1.0f;
				SetNextThink(0.1);
				EMIT_SOUND_DYN(edict(), CHAN_STATIC, "turret/tu_active2.wav", TURRET_MACHINE_VOLUME, ATTN_STATIC, 0, GetSoundPitch());
				m_iSpin = TURRET_SPIN_YES;
			}
		}
		// if it were a separate think function MoveTurret();
	}
	else if (m_iSpin == TURRET_SPIN_DOWN)
	{
		if (pev->framerate > 0.0f)
		{
			pev->framerate -= 0.04;
			if (pev->framerate <= 0.001f)
			{
				pev->framerate = 0;
				m_iSpin = TURRET_SPIN_NO;
			}
		}
	}
	return state;
}

// THINK FUNCTION
// Works, but it steals control and the turret stops rotating for a while
/*void CBaseTurret::SpinUpThink(void)
{
	StudioFrameAdvance();
	SetNextThink(0.1);

	if (m_iSpin != TURRET_SPIN_YES)
	{
		//m_flInSpinProc = TRUE;// XDM
		SetTurretAnim(TURRET_ANIM_SPIN);
		// for the first pass, spin up the the barrel
		if (m_iSpin != TURRET_SPIN_UP)//(!m_iStartSpin)
		{
			SetNextThink(1.0); // spinup delay
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_spinup.wav", TURRET_MACHINE_VOLUME, ATTN_STATIC, 0, GetSoundPitch());
			m_iSpin = TURRET_SPIN_UP;//m_iStartSpin = 1;
			if (pev->framerate >= 1.0f)// only restart if was not half-way here
				pev->framerate = 0.1f;
		}
		else if (pev->framerate < 1.0f)
		{
			pev->framerate += 0.075f;
			if (pev->framerate >= 1.0f)// after the barrel is spun up, turn on the hum
			{
				pev->framerate = 1.0f;
				SetNextThink(0.1);// retarget delay
				EMIT_SOUND_DYN(edict(), CHAN_STATIC, "turret/tu_active2.wav", TURRET_MACHINE_VOLUME, ATTN_STATIC, 0, GetSoundPitch());
				//m_iStartSpin = 0;
				m_iSpin = TURRET_SPIN_YES;
			}
		}
		// if it were a separate think function MoveTurret();
	}
	else//was separate if (m_iSpin == TURRET_SPIN_YES)
	{
		//m_flInSpinProc = FALSE;// XDM
		SetThink(&CBaseTurret::SearchThink);
	}
}

// THINK FUNCTION
// Works, but it steals control and the turret stops rotating for a while
void CBaseTurret::SpinDownThink(void)
{
	StudioFrameAdvance();
	SetNextThink(0.1);

	if (m_iSpin != TURRET_SPIN_NO)
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
		if (m_iSpin != TURRET_SPIN_DOWN)
		{
			SetNextThink(1.0);
			STOP_SOUND(edict(), CHAN_STATIC, "turret/tu_active2.wav");
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_spindown.wav", TURRET_MACHINE_VOLUME, ATTN_STATIC, 0, GetSoundPitch());
			m_iSpin = TURRET_SPIN_DOWN;
			// can it be slower? pev->framerate = 1.0f;
		}
		else if (pev->framerate > 0.0f)
		{
			pev->framerate -= 0.04;
			if (pev->framerate <= 0.001f)
			{
				pev->framerate = 0;
				m_iSpin = TURRET_SPIN_NO;
			}
		}
		// if it were a separate think function MoveTurret();
	}
	else
	{
		//?m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::SearchThink);
		//^ SetNextThink(0.1);
	}
}

void CBaseTurret::SpinDown(void)
{
	SetThink(&CBaseTurret::SpinDownThink);
	SetNextThink(0);
}

void CBaseTurret::SpinUp(void)
{
	SetThink(&CBaseTurret::SpinUpThink);
	SetNextThink(0);
}*/

void CBaseTurret::SpinDown(void)
{
	if (m_iSpin != TURRET_SPIN_DOWN && m_iSpin != TURRET_SPIN_NO)
	{
		STOP_SOUND(edict(), CHAN_STATIC, "turret/tu_active2.wav");
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_spindown.wav", TURRET_MACHINE_VOLUME, ATTN_STATIC, 0, GetSoundPitch());
		m_iSpin = TURRET_SPIN_DOWN;
		// can it be slower? pev->framerate = 1.0f;
		SetTurretAnim(TURRET_ANIM_SPIN);
		SetNextThink(1.0);
	}
}

void CBaseTurret::SpinUp(void)
{
	if (m_iSpin != TURRET_SPIN_UP && m_iSpin != TURRET_SPIN_YES)
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
		SetNextThink(1.0);
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_spinup.wav", TURRET_MACHINE_VOLUME, ATTN_STATIC, 0, GetSoundPitch());
		m_iSpin = TURRET_SPIN_UP;
		if (pev->framerate >= 1.0f)
			pev->framerate = 0.1f;
	}
}

void CBaseTurret::TurretDeath(void)
{
	if (m_fSequenceFinished && !MoveTurret())
	{
		pev->deadflag = DEAD_DEAD;// XDM3037
		pev->frame = 255;// XDM3037: HACK! To run TASK_DIE
		MaintainSchedule();//RunTask(DIE);
		SetThinkNull();
		pev->framerate = 0;
		DontThink();// XDM3038a
		EyeDestroy();// XDM3038a
	}
	else
	{
		StudioFrameAdvance();
		SetNextThink(0.1);
		//EyeOff();// XDM3037

		if (RANDOM_LONG(0, 8) == 0)
		{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, EyePosition());
				WRITE_BYTE(TE_SMOKE);
				WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
				WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
				WRITE_COORD(pev->origin.z + pev->view_ofs.z*2.0f);
				WRITE_SHORT(g_iModelIndexSmoke);
				WRITE_BYTE(RANDOM_LONG(20,25)); // scale * 10
				WRITE_BYTE(10 - m_iOrientation * 5); // framerate
			MESSAGE_END();
		}

		if (RANDOM_LONG(0, 5) == 0)
		{
			Vector vecSrc(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x), RANDOM_FLOAT(pev->absmin.y, pev->absmax.y), 0.0f);
			if (m_iOrientation == 0)
				vecSrc += Vector(0.0f, 0.0f, RANDOM_FLOAT(pev->origin.z, pev->absmax.z));
			else
				vecSrc += Vector(0.0f, 0.0f, RANDOM_FLOAT(pev->absmin.z, pev->origin.z));

			UTIL_Sparks(vecSrc);
		}
	}
}

/* XDM3038c void CBaseTurret::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == HITGROUP_ARMOR)
	{
		// hit armor
		if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,10) < 1))
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			pev->dmgtime = gpGlobals->time;
		}
		flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}

	if (pev->takedamage == DAMAGE_NO)
		return;

	AddMultiDamage( pAttacker, this, flDamage, bitsDamageType );
}*/

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET
int CBaseTurret::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (!m_iOn)
		flDamage *= 0.01f;

	// XDM3035a: we MUST call this in order to use Killed() and MonsterKilled()
	int iRet = CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
	if (iRet && !HasMemory(bits_MEMORY_KILLED))
	{
		if (pev->health > 0)
		{
			if (m_iOn)
			{
				m_fBeserk = 1;
				SetThink(&CBaseTurret::SearchThink);
			}
		}
	}
	return iRet;
}

void CBaseTurret::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	if (HasMemory(bits_MEMORY_KILLED))// XDM3037: TESTME
	{
		CBaseMonster::Killed(pInflictor, pAttacker, iGib);
		return;
	}

	CBaseMonster::Killed(pInflictor, pAttacker, iGib);// this sets bits_MEMORY_KILLED
	//SetThinkNull();
	SetTouchNull();
	SetUseNull();
	// XDM3038a	EyeDestroy();// XDM3037a!!! Eye will be recreated while respawning
	STOP_SOUND(edict(), CHAN_STATIC, "turret/tu_active2.wav");
	if (m_pEyeGlow)
	{
		if (g_pGameRules->FAllowEffects())
		{
			m_pEyeGlow->pev->renderfx = kRenderFxFlickerFast;
			m_pEyeGlow->Expand(0, 480);// this will remove the glow
			m_pEyeGlow = NULL;
		}
		else
			EyeDestroy();
	}
	if (m_iOrientation == 0)
		m_vecGoalAngles.x = -15;
	else
		m_vecGoalAngles.x = -90;

	//CBaseMonster::Killed(pInflictor, pAttacker, iGib);
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;
	//?pev->deadflag = DEAD_DEAD;
	//SetTurretAnim(TURRET_ANIM_DIE);
	//pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->angles.y = UTIL_AngleMod(pev->angles.y + RANDOM_LONG(0, 2) * 120);
	pev->framerate = 1.0f;
	m_IdealActivity = ACT_DIESIMPLE;
	DeathSound();
	ChangeSchedule(GetScheduleOfType(SCHED_DIE));
	MaintainSchedule();//RunTask();
	//ClearBits(pev->flags, FL_MONSTER); // why are they set in the first place???
	SUB_UseTargets(this, USE_ON, 0); // wake up others
	SetThink(&CBaseTurret::TurretDeath);// XDM3038c: WARNING! This interrupts SUB_Respawn if called after RunPostDeath()!!!
	RunPostDeath();// XDM3037: hack to respawn. Normally should be called at the end of animaiton, but somehow it won't.
	SetNextThink(0.1);
}

void CBaseTurret::AlertSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, STRING(pev->noise1), TURRET_MACHINE_VOLUME, ATTN_NORM, 0, GetSoundPitch());
}

void CBaseTurret::PainSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, gSoundsMetal[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)], VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
}

void CBaseTurret::DeathSound(void)
{
	if (m_iOn)// XDM: play different sounds according to sate
	{
		if (RANDOM_LONG(0,1) == 0)
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_die2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, GetSoundPitch());
		else
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_die3.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, GetSoundPitch());
	}
	else
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "turret/tu_die.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, GetSoundPitch());
}




class CTurret : public CBaseTurret
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	// Think functions
	//virtual void EXPORT SpinUpCall(void);
	//virtual void EXPORT SpinDownCall(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	// other functions
	virtual void Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy);

private:
	int m_iStartSpin;
};

TYPEDESCRIPTION	CTurret::m_SaveData[] =
{
	DEFINE_FIELD( CTurret, m_iStartSpin, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTurret, CBaseTurret );

LINK_ENTITY_TO_CLASS( monster_turret, CTurret );

void CTurret::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.turretHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = gSkillData.turretHealth*0.5;// XDM
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.turretScore;

	//Precache();
	//SET_MODEL(edict(), STRING(pev->model));
	m_HackedGunPos.Set(0.0f, 0.0f, 12.75f);
	m_flMaxSpin = TURRET_MAXSPIN;
	CBaseTurret::Spawn();// <- Precache()
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	if (m_voicePitch == 0)
		m_voicePitch = PITCH_NORM;// XDM3038a

	if (pev->view_ofs.IsZero())
		pev->view_ofs.z = 12.75;// default

	UTIL_SetSize(this, Vector(-32, -32, -m_iRetractHeight), Vector(32, 32, m_iRetractHeight));
	m_pEyeGlow = CSprite::SpriteCreate(TURRET_GLOW_SPRITE, pev->origin, FALSE);
	if (m_pEyeGlow)
	{
		m_pEyeGlow->SetTransparency(kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation);
		m_pEyeGlow->SetAttachment(edict(), 2);
	}
	m_eyeBrightness = 0;
	SetThink(&CBaseTurret::Initialize);
	SetNextThink(0.3);
}

void CTurret::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/turret.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "turret");
	if (m_iBulletType == BULLET_NONE)// XDM3038c
		m_iBulletType = BULLET_12MM;

	PRECACHE_MODEL(TURRET_GLOW_SPRITE);
	CBaseTurret::Precache();
}

// Only big turret has spinup animaitons
void CTurret::Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy)
{
	if (!m_iSpin)// XDM: don't fire when the barrel is not spinning
	{
		SpinUp();//SetThink(&CTurret::SpinUpCall); SetNextThink(0);
		return;// XDM3035b
	}
	CBaseTurret::Shoot(vecSrc, vecDirToEnemy);// XDM3038c
	//EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "turret/tu_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
}





class CMiniTurret : public CBaseTurret
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SpinDown(void);
	virtual void SpinUp(void);
};

LINK_ENTITY_TO_CLASS( monster_miniturret, CMiniTurret );

void CMiniTurret::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.miniturretHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = gSkillData.miniturretHealth*0.5;// XDM
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.miniturretScore;

	m_HackedGunPos.Set(0, 0, 12.75);
	m_flMaxSpin = 0;
	CBaseTurret::Spawn();// <-Precache()
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	if (m_voicePitch == 0)
		m_voicePitch = 105;// XDM3038a

	if (pev->view_ofs.IsZero())
		pev->view_ofs.z = 12.75;// default

	UTIL_SetSize(this, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));
	SetThink(&CBaseTurret::Initialize);
	SetNextThink(0.3);
}

void CMiniTurret::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/miniturret.mdl");
	if (FStringNull(pev->noise))// XDM3038a
		pev->noise = MAKE_STRING("weapons/pl_gun2.wav");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "turret");
	if (m_iBulletType == BULLET_NONE)// XDM3038c
		m_iBulletType = BULLET_9MM;

	CBaseTurret::Precache();
}

void CMiniTurret::SpinDown(void)
{
	m_iSpin = TURRET_SPIN_NO;
	SetNextThink(0);
}

void CMiniTurret::SpinUp(void)
{
	m_iSpin = TURRET_SPIN_YES;
	SetNextThink(0);
}





// BUGBUG: triggered monster_sentry is non-solid!!!

//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CSentry : public CBaseTurret
{
public:
	virtual int	ObjectCaps(void);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM3035a
	virtual void AlignToFloor(void) {};// XDM3035b: do nothing
	virtual bool IsPushable(void) { return true; }
	virtual void SpinDown(void);
	virtual void SpinUp(void);

	void EXPORT SentryTouch(CBaseEntity *pOther);
	void EXPORT SentryDeath(void);
};

LINK_ENTITY_TO_CLASS( monster_sentry, CSentry );

// XDM3037
int	CSentry::ObjectCaps(void)
{
	if (IsAlive())
		return CBaseTurret::ObjectCaps() | FCAP_IMPULSE_USE;
	else
		return CBaseTurret::ObjectCaps();
}

void CSentry::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.sentryHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = gSkillData.sentryHealth/2;// XDM
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.sentryScore;

	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;
	CBaseTurret::Spawn();
	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch	= -60;
	if (m_voicePitch == 0)
		m_voicePitch = 110;// XDM3038a

	if (pev->view_ofs.IsZero())
		pev->view_ofs.z = 48;// default

	m_HackedGunPos.Set(0,0,pev->view_ofs.z);
	ClearBits(pev->flags, FL_NOTARGET);// attract enemies/barnacles (unlike wall-mounted turrets)
	pev->movetype = MOVETYPE_TOSS;// XDM3038
	pev->solid = SOLID_BBOX;// XDM3038
	pev->origin.z += 2.0f;// XDM3038c: HACK
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(this, Vector(-16, -16, 0), Vector(16, 16, 32));// XDM
	DROP_TO_FLOOR(edict());// does not help

	SetTouch(&CSentry::SentryTouch);
	SetThink(&CBaseTurret::Initialize);
	SetNextThink(0.3);
}

void CSentry::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/sentry.mdl");
	if (FStringNull(pev->noise))// XDM3038a
		pev->noise = MAKE_STRING("weapons/pl_gun2.wav");
	if (FStringNull(pev->noise1))// XDM3038c
		pev->noise1 = MAKE_STRING("turret/tu_deploy_short.wav");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "turret");
	if (m_iBulletType == BULLET_NONE)// XDM3038c
		m_iBulletType = BULLET_9MM;

	//CBaseMonster::Precache();// XDM3038a: skip CBaseTurret and its sounds? Too much of a hassle
	CBaseTurret::Precache();
}

int CSentry::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	// XDM3035c: don't call CBaseTurret::TakeDamage() because it nullifies damage in disabled state
	int iRet = CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);

	if (iRet && !HasMemory(bits_MEMORY_KILLED) && pev->health > 0)
	{
		if (m_iOn)
		{
			m_fBeserk = 1;
			SetThink(&CBaseTurret::SearchThink);
			SetNextThink(0.01);
		}
		else
		{
			SetThink(&CBaseTurret::Deploy);
			SetUseNull();
			SetNextThink(0.01);
		}
	}
	return 1;
}

void CSentry::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)// XDM3035a
{
	/*CBaseMonster::Killed(pInflictor, pAttacker, iGib);

	if (HasMemory(bits_MEMORY_KILLED))// XDM3037: TESTME
		return;

	SetThinkNull();
	SetTouchNull();
	SetBoneController(0, 0);
	SetBoneController(1, 0);
	//CBaseMonster::Killed(pInflictor, pAttacker, iGib);
	pev->takedamage = DAMAGE_NO;
	//pev->deadflag = DEAD_DYING;
	pev->deadflag = DEAD_DEAD;
	DeathSound();
	//SetTurretAnim(TURRET_ANIM_DIE);
	UTIL_SetSize(this, Vector(-2, -2, 0), Vector(2, 2, 8));// XDM
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles.y = UTIL_AngleMod(pev->angles.y + RANDOM_LONG(0, 2) * 120);
	pev->framerate = 1.0f;
	m_IdealActivity = ACT_DIESIMPLE;
	ChangeSchedule(GetScheduleOfType(SCHED_DIE));
	MaintainSchedule();//RunTask();

	//CBaseTurret::Killed(pInflictor, pAttacker, iGib);
	SetThink(&CSentry::SentryDeath);
	SetNextThink(0.1);*/

	// XDM3037: TESTME
	bool bWasDead = HasMemory(bits_MEMORY_KILLED);
	CBaseTurret::Killed(pInflictor, pAttacker, iGib);

	if (bWasDead)
		return;

	pev->movetype = MOVETYPE_TOSS;
	if (FBitSet(pev->flags, FL_ONGROUND))// XDM3038c: HACK
	{
		ClearBits(pev->flags, FL_ONGROUND);
		pev->origin.z += 1.0f;
		UTIL_SetOrigin(this, pev->origin);
	}
	RunPostDeath();// XDM3037: hack to respawn. Normally should be called at the end of animaiton, but somehow it won't.
	SetThink(&CSentry::SentryDeath);
	SetNextThink(0.1);
}

void CSentry::SentryTouch(CBaseEntity *pOther)
{
	if (pOther && (pOther->IsPlayer() || pOther->IsMonster()) && pOther != m_hOwner)// pev->flags & FL_MONSTER)))
		if (m_pfnUse && m_iOn == 0)
			(this->*m_pfnUse)(pOther, this, USE_ON, 1.0f);
		//TakeDamage(pOther, pOther, 0, 0);
}

void CSentry::SentryDeath(void)
{
	if (m_fSequenceFinished)
	{
		SetBits(pev->flags, FL_NOTARGET);// don't attract enemies/barnacles?
		pev->solid = SOLID_NOT;// XDM3038c
		pev->deadflag = DEAD_DEAD;// XDM3037
		pev->frame = 255;// XDM3037: HACK! To run TASK_DIE
		MaintainSchedule();//RunTask(DIE);
		//RunPostDeath();// XDM3037: HACK!
		pev->framerate = 0;
		if (POINT_CONTENTS(EyePosition()) == CONTENTS_SOLID)// XDM3038c: HACK for turrets falling through the ground
		{
			conprintf(1, "Error: %s[%d] in CONTENTS_SOLID! Removing!\n", STRING(pev->classname), entindex());
			pev->health = 0;
			Destroy();
			// already dead! Killed(g_pWorld, g_pWorld, GIB_REMOVE);// don't just remove because we may have some events to fire
			//return;
		}
		else
		{
			SetThinkNull();
			DontThink();// XDM3038a
		}
	}
	else
	{
		StudioFrameAdvance();// for m_fSequenceFinished
		SetNextThink(0.1);
		Vector vecSrc, vecAng;
		GetAttachment(1, vecSrc, vecAng);

		if (RANDOM_LONG(0,8) == 0)
		{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSrc);
				WRITE_BYTE(TE_SMOKE);
				WRITE_COORD(vecSrc.x + RANDOM_FLOAT(-16, 16));
				WRITE_COORD(vecSrc.y + RANDOM_FLOAT(-16, 16));
				WRITE_COORD(vecSrc.z - 32);
				WRITE_SHORT(g_iModelIndexSmoke);
				WRITE_BYTE(RANDOM_LONG(10,15)); // scale * 10
				WRITE_BYTE(RANDOM_LONG(8,10)); // framerate
			MESSAGE_END();
		}
		if (RANDOM_LONG(0,6) == 0)
			UTIL_Sparks(vecSrc);
	}
}

void CSentry::SpinDown(void)
{
	m_iSpin = TURRET_SPIN_NO;
	SetNextThink(0);
}

void CSentry::SpinUp(void)
{
	m_iSpin = TURRET_SPIN_YES;
	SetNextThink(0);
}
