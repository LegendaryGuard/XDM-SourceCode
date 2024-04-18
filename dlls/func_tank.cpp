/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"
#include "player.h"
#include "soundent.h"// XDM
#include "lightp.h"
#include "hornet.h"
#include "flamecloud.h"
#include "plasmaball.h"
#include "skill.h"
#include "game.h"
#include "gamerules.h"

#define SF_TANK_ACTIVE			0x0001
#define SF_TANK_NOTSOLID			0x0002
//#define SF_TANK_HUMANS			0x0004
//#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_SOUNDON			0x8000

enum tankbullets_e
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3,
	TANK_BULLET_LIGHTP,// XDM3034
	TANK_BULLET_HORNETS,
	TANK_BULLET_FLAME,
	TANK_BULLET_PLASMA,
};

//	Custom damage
//	env_laser (duration is 0.5 rate of fire)
//	rockets
//	explosion?
class CFuncTank : public CBaseDelay// XDM3035: m_hActivator
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Think(void);
	virtual bool OnControls(entvars_t *pevTest);
	virtual bool IsBSPModel(void) const { return (*STRING(pev->model) == '*'); }//{ return true; }// XDM
	virtual int Classify(void);
	virtual int	ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }// XDM3038c: FIXED!
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	virtual void Fire(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
	//virtual void Fire2(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
	Vector UpdateTargetPosition(CBaseEntity *pTarget)
	{
		return pTarget->BodyTarget(pev->origin);
	}
	void StartRotSound(void);
	void StopRotSound(void);
	void TrackTarget(void);
	// Bmodels don't go across transitions
	void TankActivate(void);
	void TankDeactivate(void);
	bool CanFire(void);
	bool InRange(float range);
	inline bool IsActive(void) { return FBitSet(pev->spawnflags, SF_TANK_ACTIVE); }
	CBaseEntity *FindTarget(void);
	void TankTrace(const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr);
	Vector BarrelPosition(void)
	{
		Vector forward, right, up;
		AngleVectors(pev->angles, forward, right, up);
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}
	void AdjustAnglesForBarrel(Vector &angles, float distance);
	bool StartControl(CBasePlayer* pController);
	void StopControl(void);
	void ControllerPostFrame(void);

protected:
	CBasePlayer *m_pController;
	float		m_flNextAttack;
	Vector		m_vecControllerUsePos;
	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
								// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle
	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle
	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// How many rounds/second
	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track
	float		m_maxRange;		// Max range to aim/track
	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_spriteScale;	// Scale of any sprites we shoot
	string_t	m_iszSpriteSmoke;
	string_t	m_iszSpriteFlash;
	int			m_iSpriteSmoke;// XDM3038a: indexes
	int			m_iSpriteFlash;
	int			m_bulletType;	// Bullet type
	int			m_iBulletDamage; // 0 means use Bullet type's default damage
	Vector		m_sightOrigin;	// Last sight of target
	int			m_spread;		// firing spread
	unsigned short m_usFire;// XDM3038a
};

TYPEDESCRIPTION	CFuncTank::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTank, m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_persist, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteSmoke, FIELD_MODELNAME ),// XDM3038c: type
	DEFINE_FIELD( CFuncTank, m_iszSpriteFlash, FIELD_MODELNAME ),// XDM3038c: type
	DEFINE_FIELD( CFuncTank, m_bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_sightOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spread, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pController, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTank, m_vecControllerUsePos, FIELD_POSITION_VECTOR ),// XDM3038c: FIELD_POSITION_VECTOR
	DEFINE_FIELD( CFuncTank, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_iBulletDamage, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncTank, CBaseEntity );

static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};

void CFuncTank::Spawn(void)
{
	m_iSpriteSmoke = 0;
	m_iSpriteFlash = 0;
	Precache();

	if (FBitSet(pev->spawnflags, SF_TANK_NOTSOLID))// XDM
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}
	else if (*STRING(pev->model) == '*')// XDM3038c
	{
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSH;// required for SOLID_BSP // so it doesn't get pushed by anything
	}
	else
	{
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_NONE;
	}

	SET_MODEL(edict(), STRING(pev->model));

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if (IsActive())
		pev->nextthink = pev->ltime + 1.0;

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if (m_fireRate <= 0)
		m_fireRate = 1;
	if (m_maxRange <= 0)
		m_maxRange = 1024;
	if (m_spread > ARRAYSIZE(gTankSpread))
		m_spread = 0;

	pev->oldorigin = pev->origin;
}

void CFuncTank::Precache(void)
{
	if (!FStringNull(m_iszSpriteFlash))
		m_iSpriteFlash = PRECACHE_MODEL(STRINGV(m_iszSpriteFlash));

	if (!FStringNull(m_iszSpriteSmoke))
		m_iSpriteSmoke = PRECACHE_MODEL(STRINGV(m_iszSpriteSmoke));

	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRINGV(pev->noise));
	if (!FStringNull(pev->noise1))// XDM3038c
		PRECACHE_SOUND(STRINGV(pev->noise1));

	m_usFire = PRECACHE_EVENT(1, "events/fx/func_tank_fire.sc");
}

void CFuncTank::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "yawrate"))
	{
		m_yawRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "yawrange"))
	{
		m_yawRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "yawtolerance"))
	{
		m_yawTolerance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrange"))
	{
		m_pitchRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrate"))
	{
		m_pitchRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchtolerance"))
	{
		m_pitchTolerance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firerate"))
	{
		m_fireRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "barrel"))
	{
		m_barrelPos.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "barrely"))
	{
		m_barrelPos.y = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "barrelz"))
	{
		m_barrelPos.z = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritescale"))
	{
		m_spriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritesmoke"))
	{
		m_iszSpriteSmoke = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spriteflash"))
	{
		m_iszSpriteFlash = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rotatesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "persistence"))
	{
		m_persist = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bullet"))
	{
		m_bulletType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "bullet_damage" )) 
	{
		m_iBulletDamage = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firespread"))
	{
		m_spread = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "minRange"))
	{
		m_minRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maxRange"))
	{
		m_maxRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);// XDM3038c: FIXED!
}

// XDM3038a: the fun has begun
int CFuncTank::Classify(void)
{
	if (IsActive())
	{
		if (m_pController == NULL)
			return CLASS_MACHINE;// there's no m_iClass

		if (!m_pController->IsPlayer())
			return m_pController->Classify();
	}
	return CLASS_NONE;
}

////////////// START NEW STUFF //////////////
//==================================================================================
// TANK CONTROLLING
bool CFuncTank::OnControls(entvars_t *pevTest)
{
	if (!FBitSet(pev->spawnflags, SF_TANK_CANCONTROL))
		return false;

	// WTF?	Vector offset = pevTest->origin - pev->origin;
	if ((m_vecControllerUsePos - pevTest->origin).Length() < 30)
		return true;

	return false;
}

bool CFuncTank::StartControl(CBasePlayer *pController)
{
	if (m_pController != NULL)
		return false;

	// Team only or disabled?
	if (IsLockedByMaster(pController))// XDM3038a
		return false;

	pev->frame = 1;// XDM3038a
	m_hActivator = pController;
	//ALERT(at_console, "using TANK!\n");

	m_pController = pController;
	if (m_pController->m_pActiveItem)
		m_pController->m_pActiveItem->Holster();

	SetBits(m_pController->m_iHideHUD, HIDEHUD_WEAPONS);
	m_vecControllerUsePos = m_pController->pev->origin;
	pev->nextthink = pev->ltime + 0.1;
	return true;
}

void CFuncTank::StopControl(void)
{
	pev->frame = 0;// XDM3038a
	// TODO: bring back the controllers current weapon
	if (!m_pController)
		return;

	if (m_pController->m_pActiveItem)
		m_pController->m_pActiveItem->Deploy();

	//ALERT(at_console, "stopped using TANK\n");
	ClearBits(m_pController->m_iHideHUD, HIDEHUD_WEAPONS);
	DontThink();// XDM3038a
	m_pController = NULL;

	if (IsActive())
		pev->nextthink = pev->ltime + 1.0;
}

// Called each frame by the player's ItemPostFrame
void CFuncTank::ControllerPostFrame(void)
{
	//ASSERT(m_pController != NULL);
	if (m_pController == NULL)// XDM3035
		return;

	if (gpGlobals->time < m_flNextAttack)
		return;

	if (FBitSet(m_pController->pev->button, IN_ATTACK|IN_ATTACK))// XDM
	{
		Vector vecForward;
		AngleVectors(pev->angles, vecForward, NULL, NULL);
		m_fireLast = gpGlobals->time - (1/m_fireRate) - 0.01f;  // to make sure the gun doesn't fire too many bullets

		//pev->frame = 1;// XDM3038a
		//if (m_pController->pev->button & IN_ATTACK)
			Fire(BarrelPosition(), vecForward, m_pController);
		//else
		//	Fire2(BarrelPosition(), vecForward, m_pController);

		// HACKHACK -- make some noise (that the AI can hear)
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, LOUD_GUN_VOLUME, 1);// XDM

		if (m_pController && m_pController->IsPlayer())
			((CBasePlayer *)m_pController)->m_iWeaponVolume = LOUD_GUN_VOLUME;

		m_flNextAttack = gpGlobals->time + (1/m_fireRate);
	}
}
////////////// END NEW STUFF //////////////

void CFuncTank::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FBitSet(pev->spawnflags, SF_TANK_CANCONTROL) && pActivator && pActivator->IsPlayer() && !(m_pController == NULL && IsActive()))// player-controlled turret and NOT busy
	{   
		if (g_pGameRules && g_pGameRules->GetCombatMode() == GAME_COMBATMODE_NOSHOOTING)// XDM3038a
			return;

		if (value == 2 && useType == USE_SET)
		{
			ControllerPostFrame();
		}
		else if (!m_pController && useType != USE_OFF)
		{
			if (StartControl((CBasePlayer *)pActivator))// XDM3038a: if
				((CBasePlayer*)pActivator)->m_pTank = this;
		}
		else
			StopControl();
	}
	else
	{
		if (!ShouldToggle(useType, IsActive()))
			return;

		if (IsActive())
		{
			TankDeactivate();
			m_hActivator = NULL;// XDM3035
		}
		else
		{
			TankActivate();
			m_hActivator = pActivator;// XDM3035
		}
	}
}

// XDM3038a: NEWNEWNEW!
CBaseEntity *CFuncTank::FindTarget(void)
{
	int r = R_NO;
	int rBest = R_NO;
	vec_t fDistance;
	vec_t fBestDistance = m_maxRange;
	CBaseEntity *pEntity = NULL;
	CBaseEntity *pTarget = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, m_maxRange)) != NULL)
	{
		if (!UTIL_IsValidEntity(pEntity))
			continue;

		if (pEntity->IsProjectile() || !(pEntity->IsMonster() || pEntity->IsPlayer()))// XDM3037
			continue;

		if (FBitSet(pEntity->pev->flags, FL_NOTARGET) || pEntity == this || !pEntity->IsAlive())
			continue;

		fDistance = (pEntity->pev->origin - pev->origin).Length();
		if (fDistance > fBestDistance)
			continue;

		r = IRelationship(pEntity);
		if (r <= R_NO)// we don't attack these anyway
			continue;
		if (r < rBest)// we already found a better target
			continue;

		if (FVisible(pEntity))
		{
			pTarget = pEntity;
			rBest = r;
			fBestDistance = fDistance;
		}
	}
	return pTarget;
}

void CFuncTank::TankActivate(void)
{
	SetBits(pev->spawnflags, SF_TANK_ACTIVE);
	m_fireLast = 0;
	pev->nextthink = pev->ltime + 0.1;
}

void CFuncTank::TankDeactivate(void)
{
	ClearBits(pev->spawnflags, SF_TANK_ACTIVE);
	m_fireLast = 0;
	StopRotSound();
}

bool CFuncTank::CanFire(void)// XDM3035c
{
	if ((gpGlobals->time - m_lastSightTime) < m_persist)
	{
		if (m_flNextAttack <= gpGlobals->time)
			return true;
	}
	return false;
}

bool CFuncTank::InRange(float range)
{
	if (range < m_minRange)
		return false;
	if (m_maxRange > 0 && range > m_maxRange)
		return false;

	return true;
}

void CFuncTank::Think(void)
{
	pev->avelocity.Clear();
	TrackTarget();

	if (fabs(pev->avelocity.x) > 1 || fabs(pev->avelocity.y) > 1)
		StartRotSound();
	else
		StopRotSound();
}

// Automatic, non-player tanks (turrets)
void CFuncTank::TrackTarget(void)
{
	CBaseEntity *pTarget = NULL;// XDM3038a // edict_t *pTarget = NULL;// XDM3035c
	Vector angles, direction, targetPosition, barrelEnd;
	BOOL updateTime = FALSE, lineOfSight;
	TraceResult tr;
	// Get a position to aim for
	if (m_pController)
	{
		// Tanks attempt to COPY player's VIEW angles
		angles = m_pController->pev->v_angle;
#if !defined (NOSQB)
		angles[PITCH] = -angles[PITCH];
#endif
		pev->nextthink = pev->ltime + 0.05;
	}
	else
	{
		if (IsActive())
			pev->nextthink = pev->ltime + 0.1;
		else
		{
			m_fireLast = 0;// XDM3038a
			return;
		}

		if (!IsMultiplayer() && FNullEnt(FIND_CLIENT_IN_PVS(edict())))// XDM3038a: doesn't work in mp
		{
			if (IsActive())
				pev->nextthink = pev->ltime + 2;	// Wait 2 secs

			m_fireLast = 0;// XDM3038a
			return;
		}

		pTarget = FindTarget();
		if (!pTarget)
		{
			m_fireLast = 0;// XDM3038a
			return;
		}
		// Calculate angle needed to aim at target
		barrelEnd = BarrelPosition();
		targetPosition = pTarget->pev->origin + pTarget->pev->view_ofs;
		vec_t range = (targetPosition - barrelEnd).Length();

		if (!InRange(range))
		{
			m_fireLast = 0;// XDM3038a
			return;
		}
		UTIL_TraceLine( barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr );

		lineOfSight = FALSE;
		// No line of sight, don't track
		if (tr.flFraction == 1.0 || tr.pHit == pTarget->edict())
		{
			lineOfSight = TRUE;
			//CBaseEntity *pInstance = CBaseEntity::Instance(pTarget);
			if ( InRange( range ) && pTarget && pTarget->IsAlive() )
			{
				updateTime = TRUE;
				m_sightOrigin = UpdateTargetPosition( pTarget );
			}
		}

		// Track sight origin
		// !!! I'm not sure what i changed
		direction = m_sightOrigin - pev->origin;
		//direction = m_sightOrigin - barrelEnd;
		//angles = UTIL_VecToAngles( direction );
		VectorAngles(direction, angles);
		// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center) 
		AdjustAnglesForBarrel( angles, direction.Length() );
	}
#if !defined (NOSQB)
	angles.x = -angles.x;
#endif
	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance( angles.y, m_yawCenter );
	angles.x = m_pitchCenter + UTIL_AngleDistance( angles.x, m_pitchCenter );

	// Limit against range in y
	if ( angles.y > m_yawCenter + m_yawRange )
	{
		angles.y = m_yawCenter + m_yawRange;
		updateTime = FALSE;	// Don't update if you saw the player, but out of range
	}
	else if ( angles.y < (m_yawCenter - m_yawRange) )
	{
		angles.y = (m_yawCenter - m_yawRange);
		updateTime = FALSE; // Don't update if you saw the player, but out of range
	}

	if ( updateTime )
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
	pev->avelocity.y = distY * 10;
	if (pev->avelocity.y > m_yawRate)
		pev->avelocity.y = m_yawRate;
	else if (pev->avelocity.y < -m_yawRate)
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if (angles.x > m_pitchCenter + m_pitchRange)
		angles.x = m_pitchCenter + m_pitchRange;
	else if (angles.x < m_pitchCenter - m_pitchRange)
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance(angles.x, pev->angles.x);
	pev->avelocity.x = distX  * 10;

	if (pev->avelocity.x > m_pitchRate)
		pev->avelocity.x = m_pitchRate;
	else if (pev->avelocity.x < -m_pitchRate)
		pev->avelocity.x = -m_pitchRate;

	if (m_pController)
		return;

	if (CanFire() && ((fabs(distX) < m_pitchTolerance && fabs(distY) < m_yawTolerance) || FBitSet(pev->spawnflags, SF_TANK_LINEOFSIGHT)))
	{
		Vector forward;
		AngleVectors(pev->angles, forward, NULL, NULL);
		BOOL fire = FALSE;
		if (FBitSet(pev->spawnflags, SF_TANK_LINEOFSIGHT))
		{
			vec_t length = direction.Length();
			UTIL_TraceLine( barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr );
			if (pTarget && tr.pHit == pTarget->edict())// XDM3035c
				fire = TRUE;
		}
		else
			fire = TRUE;

		if (fire)
		{
			if (m_fireLast == 0.0f)
				m_fireLast = gpGlobals->time - (1/m_fireRate) - 0.01f;

			Fire(BarrelPosition(), forward, GetDamageAttacker());// XDM3035: m_hActivator
			m_flNextAttack = gpGlobals->time + (1/m_fireRate);
		}
		else
			m_fireLast = 0;
	}
	else
		m_fireLast = 0;
}

// If barrel is offset, add in additional rotation
void CFuncTank::AdjustAnglesForBarrel(Vector &angles, float distance)
{
	if ( m_barrelPos.y != 0 || m_barrelPos.z != 0 )
	{
		float r2, d2;
		distance -= m_barrelPos.z;
		d2 = distance * distance;
		if ( m_barrelPos.y )
		{
			r2 = m_barrelPos.y * m_barrelPos.y;
			angles.y += RAD2DEG(atan2(m_barrelPos.y, sqrt(d2 - r2)));
		}
		if ( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			angles.x += RAD2DEG(atan2(-m_barrelPos.z, sqrt(d2 - r2)));// possible SQB!
		}
	}
}

// Fire targets and spawn sprites
// Warning: pAttacker can be NULL!
void CFuncTank::Fire(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker)
{
	if ( m_fireLast != 0 )
	{
		PLAYBACK_EVENT_FULL(FEV_RELIABLE|FEV_UPDATE, edict(), m_usFire, 0.0, barrelEnd, pev->angles, m_spriteScale, RANDOM_FLOAT(15.0, 20.0), m_iSpriteFlash, m_iSpriteSmoke, 0, 0);
		if (!FStringNull(pev->noise1))// XDM3038c: custom fire sound
			EMIT_SOUND(edict(), CHAN_WEAPON, STRINGV(pev->noise1), VOL_NORM, ATTN_NORM);// another valveshit: we cannot really use sound indexes to transmit them using event!

		/* oldshit	if ( m_iszSpriteSmoke )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteSmoke), barrelEnd, TRUE );
			if (pSprite)
			{
				pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0, 20.0 ) );
				pSprite->SetTransparency( kRenderTransAlpha, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, 255, kRenderFxNone );
				pSprite->pev->velocity.z = RANDOM_FLOAT(40, 80);
				pSprite->SetScale( m_spriteScale );
			}
		}
		if ( m_iszSpriteFlash )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteFlash), barrelEnd, TRUE );
			if (pSprite)
			{
				pSprite->AnimateAndDie( 60 );
				pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
				pSprite->SetScale( m_spriteScale );
				// Hack Hack, make it stick around for at least 100 ms.
				pSprite->pev->nextthink += 0.1;
			}
		}*/
		SUB_UseTargets(this, USE_TOGGLE, 0);
		//CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, LOUD_GUN_VOLUME, 0.5);// XDM: why not here?

		if ((gpGlobals->time - m_fireLast) * m_fireRate > 0.0f)// XDM3035c
			m_fireLast = gpGlobals->time;
	}
}

/*void CFuncTank::Fire2( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
    Fire(barrelEnd, forward, pAttacker);
}*/

void CFuncTank::TankTrace(const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr)
{
	// get circular gaussian spread
	float x, y, z;
	do {
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);
	Vector vecDir(vecForward);
	vecDir += x * vecSpread.x * gpGlobals->v_right + y * vecSpread.y * gpGlobals->v_up;
	Vector vecEnd(vecStart);
	vecEnd += vecDir * 4096;
	UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, edict(), &tr);
}

void CFuncTank::StartRotSound(void)
{
	if (FStringNull(pev->noise) || FBitSet(pev->spawnflags, SF_TANK_SOUNDON))
		return;

	SetBits(pev->spawnflags, SF_TANK_SOUNDON);
	EMIT_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noise), 0.85, ATTN_NORM);
}

void CFuncTank::StopRotSound(void)
{
	if (FBitSet(pev->spawnflags, SF_TANK_SOUNDON))
		STOP_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noise));

	ClearBits(pev->spawnflags, SF_TANK_SOUNDON);
}


class CFuncTankGun : public CFuncTank
{
public:
	virtual void Fire( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker );
	//virtual void Fire2( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker );
};
LINK_ENTITY_TO_CLASS( func_tank, CFuncTankGun );

void CFuncTankGun::Fire( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	//??? BUGBUG fires many bullets first time
	if (m_fireLast != 0)// HACK: is needed to prevent bullets from accumulating into enormous values
	{
		// FireBullets needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors(pev->angles);
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if (bulletCount > 0)
		{
			if (gpGlobals->time - m_fireLast > 2/m_fireRate)// limiter: don't allow to accumulate over more than two delays
				bulletCount = 2;

			switch (m_bulletType)
			{
			case TANK_BULLET_9MM:
				FireBullets(bulletCount, barrelEnd, forward, gTankSpread[m_spread], NULL, 4096, BULLET_9MM, m_iBulletDamage, this, pAttacker);
				break;

			case TANK_BULLET_MP5:
				FireBullets(bulletCount, barrelEnd, forward, gTankSpread[m_spread], NULL, 4096, BULLET_MP5, m_iBulletDamage, this, pAttacker);
				break;

			case TANK_BULLET_12MM:
				FireBullets(bulletCount, barrelEnd, forward, gTankSpread[m_spread], NULL, 4096, BULLET_12MM, m_iBulletDamage, this, pAttacker);
				break;

			case TANK_BULLET_LIGHTP:// XDM3034
				CLightProjectile::CreateLP(barrelEnd, pev->angles, forward, pAttacker, this, m_iBulletDamage, 0);
				break;

			case TANK_BULLET_HORNETS:
				CHornet::CreateHornet(barrelEnd, pev->angles, forward*800, pAttacker, this, m_iBulletDamage, false);
				break;

			case TANK_BULLET_FLAME:
				CFlameCloud::CreateFlame(barrelEnd, forward*500, g_iModelIndexFlameFire, 0.2, 0.02, FLAMECLOUD_VELMULT, m_iBulletDamage, 255, 10, RANDOM_LONG(0,1), pAttacker, this);
				break;

			case TANK_BULLET_PLASMA:
				CPlasmaBall::CreatePB(barrelEnd, pev->angles, forward, PLASMABALL_SPEED, m_iBulletDamage, pAttacker, this, 0);
				break;

			default:
			case TANK_BULLET_NONE:
				break;
			}
			CFuncTank::Fire( barrelEnd, forward, pAttacker );

			if (m_pController && m_bulletType != TANK_BULLET_HORNETS && m_bulletType != TANK_BULLET_FLAME)// XDM3038b: m_pController can be NULL!
				UTIL_ScreenShakeOne(m_pController, Center(), RANDOM_FLOAT(2,3), 5.0, 0.5);// XDM3038a
		}
	}
	//else
	//	CFuncTank::Fire( barrelEnd, forward, pAttacker );
}

/*void CFuncTankGun::Fire2( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	Fire( barrelEnd, forward, pAttacker );
}*/


class CFuncTankLaser : public CFuncTank
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Activate(void);
	virtual void Think(void);
	virtual void Fire(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
	//virtual void Fire2(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	CLaser *GetLaser(void);
	static TYPEDESCRIPTION m_SaveData[];

private:
	CLaser *m_pLaser;
	float m_laserTime;
};

LINK_ENTITY_TO_CLASS( func_tanklaser, CFuncTankLaser );

TYPEDESCRIPTION	CFuncTankLaser::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTankLaser, m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankLaser, m_laserTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFuncTankLaser, CFuncTank );

void CFuncTankLaser::Activate(void)
{
	if (!GetLaser())
	{
		ALERT(at_error, "Laser tank %s with no env_laser!\n", STRING(pev->targetname));
		Destroy();
	}
	else
		m_pLaser->TurnOff();
}

void CFuncTankLaser::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "laserentity"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

CLaser *CFuncTankLaser::GetLaser(void)
{
	if ( m_pLaser )
		return m_pLaser;

	edict_t	*pentLaser;

	pentLaser = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->message) );
	while ( !FNullEnt( pentLaser ) )
	{
		// Found the landmark
		if ( FClassnameIs( pentLaser, "env_laser" ) )
		{
			m_pLaser = (CLaser *)CBaseEntity::Instance(pentLaser);
			break;
		}
		else
			pentLaser = FIND_ENTITY_BY_TARGETNAME( pentLaser, STRING(pev->message) );
	}

	return m_pLaser;
}

void CFuncTankLaser::Think(void)
{
	if ( m_pLaser && m_pLaser->IsOn() && (gpGlobals->time > m_laserTime) )// XDM3035: this code is obsolete, but needs to be tested to be removed completely
		m_pLaser->TurnOff();

	CFuncTank::Think();
}

void CFuncTankLaser::Fire( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	if (m_fireLast != 0 && GetLaser())
	{
		// TankTrace needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors(pev->angles);
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if (bulletCount > 0)
		{
			TraceResult tr;
			for (int i = 0; i < bulletCount; ++i)
			{
				m_pLaser->pev->origin = barrelEnd;
				TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );
				m_laserTime = gpGlobals->time;
				m_pLaser->TurnOn();
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0f;
				m_pLaser->m_hOwner = pAttacker;// XDM3037
				m_pLaser->FireAtPoint(tr);
				m_pLaser->SetThink(&CLaser::TurnOffThink);
				m_pLaser->SetNextThink(gpGlobals->frametime * 3.0f);// TESTME 0.2f;// XDM3035: TUNE
			}
			CFuncTank::Fire( barrelEnd, forward, pAttacker );
		}
	}
	else
	{
		CFuncTank::Fire( barrelEnd, forward, pAttacker );
	}
}
/*
void CFuncTankLaser::Fire2( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	Fire( barrelEnd, forward, pev );
}
*/
class CFuncTankRocket : public CFuncTank
{
public:
	virtual void Precache(void);
	virtual void Fire(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
//	virtual void Fire2(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
};
LINK_ENTITY_TO_CLASS( func_tankrocket, CFuncTankRocket );

void CFuncTankRocket::Precache(void)
{
	UTIL_PrecacheOther( "rpg_rocket" );
	CFuncTank::Precache();
}

void CFuncTankRocket::Fire( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	if ( m_fireLast != 0 )
	{
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if (bulletCount > 0)
		{
			for (int i = 0; i < bulletCount; ++i)
			{
				Vector vecAng(pev->angles);// XDM3038: stupid quake bug (SQB)
#if !defined (NOSQB)
				vecAng.x = -vecAng.x;
#endif
				CRpgRocket::CreateRpgRocket(barrelEnd, vecAng, forward, (pAttacker!=NULL)?pAttacker:this, this, NULL, m_iBulletDamage);// XDM3038c: <-bulletdamage
			}
			CFuncTank::Fire(barrelEnd, forward, pAttacker);// XDM3034: pAttacker was 'this'
			UTIL_ScreenShakeOne(m_pController, Center(), RANDOM_FLOAT(2,3), 4.0, 0.5);// XDM3038a
		}
	}
//	else
//		CFuncTank::Fire( barrelEnd, forward, pAttacker );// XDM3034: pAttacker was this
}
/*
void CFuncTankRocket::Fire2( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	Fire( barrelEnd, forward, pev );
}
*/
class CFuncTankMortar : public CFuncTank
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Fire(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
//	virtual void Fire2(const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker);
};
LINK_ENTITY_TO_CLASS( func_tankmortar, CFuncTankMortar );

void CFuncTankMortar::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

void CFuncTankMortar::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	if (pev->dmg <= 0)
		pev->dmg = gSkillData.DmgMortar;

	CFuncTank::Spawn();
}

void CFuncTankMortar::Fire( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	if ( m_fireLast != 0 )
	{
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		// Only create 1 explosion
		if ( bulletCount > 0 )
		{
			TraceResult tr;
			// TankTrace needs gpGlobals->v_up, etc.
			UTIL_MakeAimVectors(pev->angles);
			TankTrace(barrelEnd, forward, gTankSpread[m_spread], tr);
			ExplosionCreate(tr.vecEndPos, pev->angles, (pAttacker!=NULL)?pAttacker:this, this, pev->dmg, 0, 0.0f);// XDM
			CFuncTank::Fire(barrelEnd, forward, pAttacker);
		}
	}
//	else
//		CFuncTank::Fire( barrelEnd, forward, pAttacker );
}
/*
void CFuncTankMortar::Fire2( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	Fire( barrelEnd, forward, pev );
}
*/
//============================================================================	
// FUNC TANK CONTROLS
//============================================================================
class CFuncTankControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps(void);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Think(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	CFuncTank *m_pTank;
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( func_tankcontrols, CFuncTankControls );

TYPEDESCRIPTION	CFuncTankControls::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTankControls, m_pTank, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CFuncTankControls, CBaseEntity );

int	CFuncTankControls::ObjectCaps(void) 
{ 
	return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE; 
}

void CFuncTankControls::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_pTank)// pass the Use command onto the controls
		m_pTank->Use( pActivator, pCaller, useType, value );

	ASSERT( m_pTank != NULL );	// if this fails,  most likely means save/restore hasn't worked properly
}

void CFuncTankControls::Think(void)
{
	edict_t *pTarget = NULL;
	do 
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING(pev->target) );
	} while ( !FNullEnt(pTarget) && strbegin(STRING(pTarget->v.classname), "func_tank") == 0);

	if ( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No tank %s\n", STRING(pev->target) );
		return;
	}

	m_pTank = (CFuncTank *)Instance(pTarget);
}

void CFuncTankControls::Spawn(void)
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(this, pev->mins, pev->maxs);
	UTIL_SetOrigin(this, pev->origin);
	SetNextThink(0.3);// After all the func_tank's have spawned
	CBaseEntity::Spawn();
}
