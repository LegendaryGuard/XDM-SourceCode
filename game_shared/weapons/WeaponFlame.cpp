//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "gamerules.h"
#include "globals.h"
#include "util_vector.h"
#if !defined (CLIENT_DLL)
#include "customentity.h"
#include "effects.h"
//#include "game.h"
#include "flamecloud.h"
#endif

// I know this flamethrower is bad, but there are no good alternatives for now :(
// The realistic way: create real flame clouds that are pushable by wind (triggers), but THIS LAGS and overflows sooner or later.
// The so-so way: create particle systems on client side, but Stupid Quake Bug(tm) in the engine prevents us from using player angles!! Also we have to emulate something physic-like on the server side.
// The better-than-nothing way: as you see here. :(
// BAD: it's the vecEndPost that should be lagged, not angles!
// UNDONE: use pushable physical entity to emulate flame end position

// class CPushablePhysicalEntity?

enum vanim_flame_e
{
	FLAME_DRAW = 0,
	FLAME_IDLE,
	FLAME_FIDGET,
	FLAME_FIRE,
	FLAME_SHOOT,
	FLAME_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_flame, CWeaponFlame);

TYPEDESCRIPTION CWeaponFlame::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponFlame, m_fFlameDist, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponFlame, m_fFlameDistMax, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponFlame, m_fFlameDelta, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponFlame, m_fConeRadius, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponFlame, m_vFlameAngles, FIELD_VECTOR),
	DEFINE_FIELD(CWeaponFlame, m_vFlameAngVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CWeaponFlame, m_hBeam, FIELD_EHANDLE),
	DEFINE_FIELD(CWeaponFlame, m_fireMode, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponFlame, m_fireState, FIELD_INTEGER),
};
IMPLEMENT_SAVE(CWeaponFlame, CBasePlayerWeapon);

//-----------------------------------------------------------------------------
// Purpose: Custom restore fn to re-send client event
//-----------------------------------------------------------------------------
int CWeaponFlame::Restore(CRestore &restore)
{
	int status = CBasePlayerWeapon::Restore(restore);
	if (status == 0)
		return 0;

#if !defined(CLIENT_DLL)
	status = restore.ReadFields("CWeaponFlame", this, m_SaveData, ARRAYSIZE(m_SaveData));
#endif
	if (status != 0 && m_fireState != FIRE_OFF)
	{
		int flags = FEV_UPDATE;
#if defined(CLIENT_WEAPONS)
		SetBits(flags, FEV_NOTHOST);
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero/*m_pPlayer->GetGunPosition(), m_vFlameAngles*/, m_fFlameDistMax, 0.0f, FLAME_FIRE, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
	}
	return status;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlame::Spawn(void)
{
	m_fFlameDist = 0.0f;
	m_fConeRadius = 0.0f;
	m_hBeam = NULL;
	m_fireState = FIRE_OFF;
	CBasePlayerWeapon::Spawn();// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlame::Precache(void)
{
	m_iId = WEAPON_FLAME;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = FLAME_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_flame.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_flame.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgFlame;

	PRECACHE_MODEL(FLAME_BEAM_SPRITE);
	PRECACHE_SOUND("weapons/flame_select.wav");
	PRECACHE_SOUND("weapons/flame_run1.wav");
	PRECACHE_SOUND("weapons/flame_run2.wav");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/fireflame.sc");
	CBasePlayerWeapon::Precache();// XDM3038
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFlame::Deploy(void)
{
	m_fFlameDist = 0.0f;
	m_fFlameDelta = 0.0f;
	m_vFlameAngVelocity.Clear();
	m_fireState = FIRE_OFF;
	return DefaultDeploy(FLAME_DRAW, "gauss", "weapons/flame_select.wav");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlame::Holster(int skiplocal /* = 0 */)
{
	EndAttack();
	m_fFlameDelta = 0.0f;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(FLAME_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWeaponFlame::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_FLAME;
	p->iFlags = 0;
	p->iMaxClip = FLAME_MAX_CLIP;
	p->iWeight = FLAME_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_FLAME;
	p->iPosition = POSITION_FLAME;
#endif
	p->pszAmmo1 = "fuel";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = FUEL_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Flame mode
//-----------------------------------------------------------------------------
void CWeaponFlame::PrimaryAttack(void)
{
	Attack(FIRE_PRI);
}

//-----------------------------------------------------------------------------
// Purpose: Torch mode
//-----------------------------------------------------------------------------
void CWeaponFlame::SecondaryAttack(void)
{
	Attack(FIRE_SEC);
}

//-----------------------------------------------------------------------------
// Purpose: Common attack layer
// Input  : mode - 
//-----------------------------------------------------------------------------
void CWeaponFlame::Attack(WEAPON_FIREMODE mode)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || (m_fireMode != mode && m_fireState != FIRE_OFF) || !HasAmmo(AMMO_PRIMARY))// quick fix: m_fireMode != mode
	{
		if (m_fireState != FIRE_OFF)
			EndAttack();

		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		return;
	}

	// XDM3035a: allow fast changes without having to stop fire
	Vector vecAngles(m_pPlayer->pev->v_angle)/* SUX + m_pPlayer->pev->punchangle*/;
	UTIL_MakeVectors(vecAngles);
	Vector vecAiming(gpGlobals->v_forward);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward*8.0f + gpGlobals->v_right*2.0f - gpGlobals->v_up*4.0f;
	int eventflags = 0;
#if defined(CLIENT_WEAPONS)
	SetBits(eventflags, FEV_NOTHOST);
#endif
//#if !defined(CLIENT_DLL)
//	m_fFlameDistMax = test1.value;
//#endif
	if (m_fireMode == FIRE_PRI)
		m_fFlameDistMax = FLAMETHROWER_DISTANCE1;
	else
		m_fFlameDistMax = FLAMETHROWER_DISTANCE2;

	if (m_fireState == FIRE_OFF)
	{
		m_pPlayer->m_iWeaponVolume = FLAMETHROWER_ATTACK_VOLUME / 2;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		m_flNextAmmoBurn = gpGlobals->time;// start using ammo ASAP
		m_fireMode = mode;
		m_fireState = FIRE_CHARGE;
		//m_hPhysEntFlameEnd = CBaseE
		m_fFlameDist = 16.0f;
		m_fFlameDelta = 1.0f;
		m_fConeRadius = 4.0f;
		m_vFlameAngles = vecAngles;
		PLAYBACK_EVENT_FULL(eventflags, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero/*vecSrc, m_vFlameAngles*/, m_fFlameDistMax, 0.0f, FLAME_FIRE, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
		//UTIL_DebugBeam(vecSrc, vecSrc+gpGlobals->v_forward*16.0f, 10.0f, 0,255,0);
		CreateEffect(vecSrc, vecAiming);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.01f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
	}
	else// if (m_fireState == FIRE_CHARGE || m_fireState == FIRE_ON)
	{
		if (m_fireMode != mode)// old code
			m_fireState = FIRE_CHANGEMODE;
		else
			m_fireState = FIRE_ON;

		m_fireMode = mode;
		if (m_fireMode == FIRE_PRI)
			Fire1(vecSrc + gpGlobals->v_forward*2.0f, vecAiming);
		else
			Fire2(vecSrc + gpGlobals->v_forward*2.0f, vecAiming);

		SetBits(eventflags, FEV_UPDATE);// only merge these events, not the first one
		PLAYBACK_EVENT_FULL(eventflags, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero/*vecSrc, m_vFlameAngles*/, m_fFlameDistMax, 0.0f, FLAME_FIRE, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
		m_pPlayer->m_iWeaponVolume = FLAMETHROWER_ATTACK_VOLUME;
		m_fireState = FIRE_ON;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + ((m_fireMode == FIRE_PRI)?FLAMETHROWER_UPDATE_INTERVAL1:FLAMETHROWER_UPDATE_INTERVAL2);
		if (m_flNextAmmoBurn <= gpGlobals->time)
		{
			UseAmmo(AMMO_PRIMARY, 1);
			m_flNextAmmoBurn = gpGlobals->time + ((m_fireMode == FIRE_PRI)? FLAMETHROWER_AMMO_USE_INTERVAL1 : FLAMETHROWER_AMMO_USE_INTERVAL2);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Flame cone mode
// Input  : vecOrigSrc - 
//			vecDir - 
//-----------------------------------------------------------------------------
void CWeaponFlame::Fire1(const Vector &vecOrigSrc, const Vector &vecDir)
{
	//SendWeaponAnim(FLAME_FIRE);
#if !defined(CLIENT_DLL)
#if !defined(FLAMETHROWER_USE_FLAMECLOUDS)
	// Flame lag emulation
	Vector vTargetAngles(m_pPlayer->pev->v_angle);
	const float m_fPitchRate = FLAME_CONE_ANGLE_RATE;
	const float m_fYawRate = FLAME_CONE_ANGLE_RATE;
	int iResetDelta = 0;
	// PITCH
	m_vFlameAngVelocity.x = UTIL_AngleDistance(vTargetAngles.x, m_vFlameAngles.x) * 10.0f;
	if (m_vFlameAngVelocity.x > m_fPitchRate)
	{
		m_vFlameAngVelocity.x = m_fPitchRate;
		if (m_fFlameDelta <= 0.0f)
			m_fFlameDelta = -1.0f;// decrease flame length while turning
	}
	else if (m_vFlameAngVelocity.x < -m_fPitchRate)
	{
		m_vFlameAngVelocity.x = -m_fPitchRate;
		if (m_fFlameDelta <= 0.0f)
			m_fFlameDelta = -1.0f;// decrease flame length while turning
	}
	else if (m_fFlameDelta < 0.0f)// restore
		iResetDelta += 1;

	// YAW
	m_vFlameAngVelocity.y = UTIL_AngleDistance(vTargetAngles.y, m_vFlameAngles.y) * 10.0f;
	if (m_vFlameAngVelocity.y > m_fYawRate)
	{
		m_vFlameAngVelocity.y = m_fYawRate;
		if (m_fFlameDelta <= 0.0f)
			m_fFlameDelta = -1.0f;// decrease flame length while turning
	}
	else if (m_vFlameAngVelocity.y < -m_fYawRate)
	{
		m_vFlameAngVelocity.y = -m_fYawRate;
		if (m_fFlameDelta <= 0.0f)
			m_fFlameDelta = -1.0f;// decrease flame length while turning
	}
	else if (m_fFlameDelta < 0.0f)
		iResetDelta += 1;

	if (iResetDelta == 2)// both axes are stabilized
		m_fFlameDelta = 1.0f;// restore flame length after turning

	if (m_fFlameDist < 1.0f)
		return;

	// m_vFlameAngles is now properly delayed
	Vector vForward, vRight, vUp;
	AngleVectors(m_vFlameAngles, vForward, vRight, vUp);
	Vector vAiming(vForward);
	Vector vSrc(vecOrigSrc);//Vector vSrc(m_pPlayer->GetGunPosition()); vSrc += vForward*8.0f + vRight*2.0f - vUp*4.0f;
	Vector vEnd(vForward); vEnd *= m_fFlameDist; vEnd += vSrc;
	//UTIL_ShowLine(vSrc, vEnd, 1.0, 0,255,255);// DEBUG
	TraceResult m_FlameTrace, trace;
	UTIL_TraceLine(vSrc, vEnd, dont_ignore_monsters, dont_ignore_glass, m_pPlayer->edict(), &m_FlameTrace);
	vEnd = m_FlameTrace.vecEndPos;
	float m_fCurrentDist;
	if (m_FlameTrace.flFraction != 1.0f)
		m_fCurrentDist = (vEnd-vSrc).Length();
	else
		m_fCurrentDist = m_fFlameDist;

	//UTIL_DebugBeam(vSrc, vEnd, 1.0, g_Palette[216].r,g_Palette[216].g,g_Palette[216].b);// DEBUG
	//UTIL_DebugAngles(vEnd, m_vFlameAngles, 1.0f, m_fConeRadius);// DEBUG
	// The purpose of m_hPhysEntFlameEnd is to be pushable by everything, but always (try to) return to vEnd
	/*if (FVisible(m_hPhysEntFlameEnd->pev->origin))// flame is behind an obstacle
		m_hPhysEntFlameEnd->MoveToOrigin(vEnd);
	else
		UTIL_SetOrigin(m_hPhysEntFlameEnd, vEnd);

	vEnd = m_hPhysEntFlameEnd->pev->origin;//Center()?*/
	Vector vDot;
	// WTF AngleVectors(m_pPlayer->pev->angles, vForward, vRight, vUp);
	CBaseEntity *pEntity = NULL;
	CBaseEntity *pPrevEntity = m_pPlayer;
	//UTIL_ShowLine(vEnd-vRight*m_fConeRadius, vEnd+vRight*m_fConeRadius, 1.0, g_Palette[110].r,g_Palette[110].g,g_Palette[110].b);// DEBUG
	//UTIL_ShowLine(vEnd-vUp*m_fConeRadius, vEnd+vUp*m_fConeRadius, 1.0, g_Palette[110].r,g_Palette[110].g,g_Palette[110].b);// DEBUG
	//UTIL_DebugAngles(vSrc, m_vFlameAngles, 2.0, 32);
	Vector vecSpot(vForward); vecSpot *= m_fCurrentDist*0.65f; vecSpot += vSrc;// search sphere is located between the player and vecEnd
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSpot, m_fCurrentDist*0.6f)) != NULL)
	{
		if (pEntity == this || pEntity == GetHost())// || pEntity == m_hBeam)
			continue;
		if (pEntity->GetExistenceState() != ENTITY_EXSTATE_WORLD)// XDM3038c: don't damage virtual entities
			continue;
		if (pEntity->IsRemoving())// XDM3038a: skip entities that are about to be removed
			continue;
		if (pEntity->pev->takedamage == DAMAGE_NO)
			continue;

		//conprintf(2, "Flame: FOUND: %s\n", STRING(pEntity->pev->classname));
		vDot = pEntity->Center();
		if (pEntity->edict() != m_FlameTrace.pHit && !PointInCone(vDot, vSrc, vEnd, m_fConeRadius))// exclusively pass entity that is blocking the flame
		{
			//UTIL_ShowLine(vSrc, vDot, 3.0, 255,0,0);// DEBUG
			vDot = GetNearestPointOfABox(vSrc, vDot/*pEntity->Center()*/, pEntity->pev->size*0.5f);
			if (!PointInCone(vDot, vSrc, vEnd, m_fConeRadius))
				continue;
		}

		//conprintf(2, "Flame: IN CONE: %s\n", STRING(pEntity->pev->classname));
		//UTIL_ShowBox(pEntity->pev->absmin, pEntity->pev->absmax, 1.0, 255,0,0);// DEBUG
		//UTIL_ShowLine(vSrc, vDot, 3.0, 255,255,0);// DEBUG
		if (pEntity->edict() == m_FlameTrace.pHit)// flame trace already hit this entity
			trace = m_FlameTrace;
		else
			UTIL_TraceLine(vDot, pEntity->Center(), dont_ignore_monsters, dont_ignore_glass, pPrevEntity?pPrevEntity->edict():NULL, &trace);

		if (trace.flFraction < 1.0f && trace.pHit == pEntity->edict())
		{
			//conprintf(1, "Flame: VISIBLE: %s\n", STRING(pEntity->pev->classname));
			ClearMultiDamage();
			pEntity->TraceAttack(GetDamageAttacker(), pev->dmg*FLAMETHROWER_UPDATE_INTERVAL1, (pEntity->Center()-vDot).Normalize(), &trace, DMG_BURN|DMG_NEVERGIB|DMG_NOHITPOINT|DMG_IGNITE|DMG_DONT_BLEED);
			ApplyMultiDamage(this, GetDamageAttacker());
			pPrevEntity = pEntity;
		}
		//pEntity->TakeDamage(this, m_pPlayer, gSkillData.DmgFlame, DMG_BURN|DMG_NEVERGIB|DMG_NOHITPOINT|DMG_IGNITE|DMG_DONT_BLEED);
	}
#else // FLAMETHROWER_USE_FLAMECLOUDS
	Vector vel;
	if (m_fireMode == FIRE_PRI)
		vel = vecDir*FLAMETHROWER_FLAME_SPEED1 + RandomVector();//Vector(RANDOM_FLOAT(-1,1), RANDOM_FLOAT(-1,1), 0.0f);
	else
		vel = vecDir*FLAMETHROWER_FLAME_SPEED2 + RandomVector();//Vector(RANDOM_FLOAT(-1,1), RANDOM_FLOAT(-1,1), 0.0f);

	vec_t vellen = m_pPlayer->pev->velocity.Length();
	Vector pvn = m_pPlayer->pev->velocity * 1/(vellen>0.0f?vellen:1.0f);// Normalize() but we already know length so this is some optimization
	float angdiff = AngleBetweenVectors(gpGlobals->v_forward, pvn);// Difference between view and movement in degrees
	float kadd = 0.0f;// add this amount of forward velocity
	if (vellen > 0.25f)// throw away some unneeded values
		kadd = (180.0f - angdiff)/180.0f;// 1.0 when vectors are of the same direction

	//conprintf(1, "CWeaponFlame:: kadd = %f\n", kadd);
	vel *= 8.0f;
	vel += m_pPlayer->pev->velocity*kadd;
	int ffx = 0;
	if (RANDOM_LONG(0,3) == 0)
	{
		SetBits(ffx, EF_NODRAW);
	}
	else if (sv_flame_effect.value > 0.0f)
	{
		if (RANDOM_LONG(0,1) == 0)
			SetBits(ffx, EF_DIMLIGHT);
	}
	CFlameCloud::CreateFlame(vecOrigSrc, vel, g_iModelIndexFlameFire, FLAMECLOUD_SCALE, FLAMECLOUD_SCALEADD, FLAMECLOUD_VELMULT, gSkillData.DmgFlame, 255, FLAMECLOUD_ADELTA, ffx, m_pPlayer, m_pPlayer);
#endif // FLAMETHROWER_USE_FLAMECLOUDS
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Torch mode that can break metal
// Input  : vecOrigSrc - 
//			vecDir - 
//-----------------------------------------------------------------------------
void CWeaponFlame::Fire2(const Vector &vecOrigSrc, const Vector &vecDir)
{
	m_vFlameAngles = m_pPlayer->pev->v_angle;
#if !defined(CLIENT_DLL)
	if (m_fFlameDist < 1.0f)
		return;

	Vector vEnd(vecDir); vEnd *= m_fFlameDist; vEnd += vecOrigSrc;
	TraceResult trace;
	UTIL_TraceLine(vecOrigSrc, vEnd, dont_ignore_monsters, dont_ignore_glass, m_pPlayer->edict(), &trace);
	vEnd = trace.vecEndPos;
	if (trace.flFraction < 1.0f && trace.pHit)
	{
		CBaseEntity *pHit = CBaseEntity::Instance(trace.pHit);
		if (pHit)
		{
			if (pHit->pev->takedamage != DAMAGE_NO)
				pHit->TakeDamage(this, GetDamageAttacker(), pev->dmg*FLAMETHROWER_UPDATE_INTERVAL2, DMG_BURN|DMG_ENERGYBEAM|DMG_NEVERGIB|DMG_NOHITPOINT|DMG_IGNITE|DMG_DONT_BLEED);// DMG_ENERGYBEAM to break metal
		}
	}
	if (m_hBeam.IsValid() && !FBitSet(m_hBeam->pev->effects, EF_NODRAW))
		((CBeam *)(CBaseEntity *)m_hBeam)->SetStartPos(vEnd);

	/*just an idea how to pack delta vector into a float. 
	CBaseEntity *pFlame = NULL;
	Vector vDest;
	Vector vDelta(pFlame->pev->origin - m_pPlayer->pev->origin);
	float fCoded = vDelta.y + clamp(100.0f, vDelta.x)*100.0f*/
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM3038c
		SetBits(m_pPlayer->pev->effects, EF_MUZZLEFLASH);
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: End all attacks
//-----------------------------------------------------------------------------
void CWeaponFlame::EndAttack(void)
{
	if (m_fireState != FIRE_OFF)// XDM3035: event triggers animation that interrupts Holster() so don't send it everytime
	{
		m_fireState = FIRE_OFF;
		int flags = FEV_RELIABLE | FEV_UPDATE;// reliable to stop systems!
#if defined(CLIENT_WEAPONS)
		SetBits(flags, FEV_NOTHOST);
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0f, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0f, 0.0f, FLAME_IDLE, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
		DestroyEffect();
	}
	m_fFlameDelta = -1.0f;
	m_vFlameAngVelocity.Clear();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + ((m_fireMode == FIRE_PRI)?FLAMETHROWER_ATTACK_INTERVAL1:FLAMETHROWER_ATTACK_INTERVAL2);
}

//-----------------------------------------------------------------------------
// Purpose: Update angular velocity every frame
//-----------------------------------------------------------------------------
void CWeaponFlame::ItemPreFrame(void)
{
#if !defined(CLIENT_DLL)
	if (m_fireState != FIRE_OFF)
	{
		if (!m_vFlameAngVelocity.IsZero())
			m_vFlameAngles.MultiplyAdd(gpGlobals->frametime, m_vFlameAngVelocity);

		int bStabilized = 0;
		if (m_fFlameDelta > 0.0f)
		{
			if (m_fFlameDist < m_fFlameDistMax)
				m_fFlameDist += m_fFlameDelta * gpGlobals->frametime * FLAME_CONE_DIST_DELTA;
			else
			{
				m_fFlameDist = m_fFlameDistMax;
				bStabilized += 1;
			}
			if (m_fConeRadius < FLAME_CONE_RADIUS_MAX)
				m_fConeRadius += m_fFlameDelta * gpGlobals->frametime * FLAME_CONE_RADIUS_DELTA;
			else
			{
				m_fConeRadius = FLAME_CONE_RADIUS_MAX;
				bStabilized += 1;
			}
		}
		else if (m_fFlameDelta < 0.0f)
		{
			if (m_fFlameDist > 0.0f)
				m_fFlameDist += m_fFlameDelta * gpGlobals->frametime * FLAME_CONE_DIST_DELTA;
			else
			{
				m_fFlameDist = 0.0f;
				bStabilized += 1;
			}

			if (m_fConeRadius > 0.0f)
				m_fConeRadius += m_fFlameDelta * gpGlobals->frametime * FLAME_CONE_RADIUS_DELTA;
			else
			{
				m_fConeRadius = 0.0f;
				bStabilized += 1;
			}
		}
		if (bStabilized == 2)// both parameters are at peak
			m_fFlameDelta = 0.0f;
	}
#endif // !CLIENT_DLL
	CBasePlayerWeapon::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Player released all weapon buttons
//-----------------------------------------------------------------------------
void CWeaponFlame::WeaponIdle(void)
{
	if (m_fireState != FIRE_OFF)
		EndAttack();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/flame_run1.wav");// XDM3038c: hacky way to stop hanging sounds
	STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/flame_run2.wav");
	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 1;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 1)
		SendWeaponAnim(FLAME_FIDGET);
	else
		SendWeaponAnim(FLAME_IDLE);

	DestroyEffect();

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

//-----------------------------------------------------------------------------
// Purpose: Create data AND/OR reset parameters
// Input  : vecOrigSrc - 
//			vecDir - 
//-----------------------------------------------------------------------------
void CWeaponFlame::CreateEffect(const Vector &vecOrigSrc, const Vector &vecDir)
{
#if !defined (CLIENT_DLL)
	DestroyEffect();

	if (m_fireMode == FIRE_SEC)
	{
		CBeam *m_pBeam = (CBeam *)(CBaseEntity *)m_hBeam;
		if (m_pBeam == NULL)
			m_pBeam = CBeam::BeamCreate(FLAME_BEAM_SPRITE, FLAME_BEAM_WIDTH);
		if (m_pBeam)
		{
			m_pBeam->PointEntInit(vecOrigSrc + vecDir*m_fFlameDist, m_pPlayer->entindex());
			m_pBeam->SetEndAttachment(1);
			m_pBeam->SetScrollRate(25);
			m_pBeam->SetWidth(FLAME_BEAM_WIDTH);// !!
			m_pBeam->SetNoise(0);
			m_pBeam->SetColor(255,255,255);
			m_pBeam->SetBrightness(255);
			m_pBeam->pev->frame = 0;
			SetBits(m_pBeam->pev->spawnflags, SF_BEAM_TEMPORARY|SF_NORESPAWN);// destroy on save/restore or level transition
			m_pBeam->pev->owner = m_pPlayer->edict();
			m_pBeam->SetFlags(BEAM_FSHADEIN);// reset shade effect
			m_pBeam->pev->playerclass = 0;
			m_hBeam = m_pBeam;
		}
	}
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: destroy data, no effects
//-----------------------------------------------------------------------------
void CWeaponFlame::DestroyEffect(void)
{
#if !defined (CLIENT_DLL)
	if (m_hBeam)
	{
		UTIL_Remove(m_hBeam);
		m_hBeam = NULL;
	}
	//STOP_SOUND(edict(), CHAN_, "weapons/.wav");
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: entity is detached from its former owner
//-----------------------------------------------------------------------------
void CWeaponFlame::OnDetached(void)// XDM3038c
{
	CBasePlayerWeapon::OnDetached();// XDM3038c
	DestroyEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlame::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
	weapondata->fuser1 = m_fFlameDelta;
	weapondata->fuser2 = m_fFlameDist;
	weapondata->fuser3 = m_fConeRadius;
	//weapondata->fuser4 = m_fFlameDistMax;
}
