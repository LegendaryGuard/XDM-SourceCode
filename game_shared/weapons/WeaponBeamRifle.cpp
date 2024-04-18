//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "skill.h"
#include "globals.h"
#include "game.h"
#include "sound.h"
#include "soundent.h"

enum vanim_beamrifle_e
{
	BEAMRIFLE_DRAW = 0,
	BEAMRIFLE_HOLSTER,
	BEAMRIFLE_IDLE,
	BEAMRIFLE_FIDGET1,
	BEAMRIFLE_FIDGET2,
	BEAMRIFLE_FIDGET3,
	BEAMRIFLE_FIRE1,
	BEAMRIFLE_FIRE2,
	BEAMRIFLE_SPINUP,
	BEAMRIFLE_SPIN,
	BEAMRIFLE_SPINDOWN,
};

LINK_ENTITY_TO_CLASS(weapon_beamrifle, CWeaponBeamRifle);

TYPEDESCRIPTION	CWeaponBeamRifle::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponBeamRifle, m_fireMode, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponBeamRifle, m_fireState, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponBeamRifle, CBasePlayerWeapon);

void CWeaponBeamRifle::Precache(void)
{
	m_iId = WEAPON_BEAMRIFLE;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = BEAMRIFLE_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_beamrifle.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_beamrifle.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgBeamRifle;

	PRECACHE_SOUND("weapons/beamrifle_charge.wav");
	PRECACHE_SOUND("weapons/beamrifle_fire1.wav");
	PRECACHE_SOUND("weapons/beamrifle_fire2.wav");
	PRECACHE_SOUND("weapons/beamrifle_select.wav");
	PRECACHE_SOUND("weapons/beam_blast.wav");
	PRECACHE_SOUND("weapons/electro5.wav");

	m_iGlow = PRECACHE_MODEL("sprites/iceball1.spr");
	m_iBeam = PRECACHE_MODEL("sprites/gaussbeam.spr");
	m_iCircle = PRECACHE_MODEL("sprites/flat_ring1.spr");// alternative mode powerful beam
	m_iStar = PRECACHE_MODEL("sprites/beamstar1.spr");// big explosion

	m_usFire = PRECACHE_EVENT(1, "events/weapons/beamrifle.sc");
	m_usFireBeam = PRECACHE_EVENT(1, "events/weapons/firebeam.sc");
	m_usBeamBlast = PRECACHE_EVENT(1, "events/fx/beamblast.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponBeamRifle::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_BEAMRIFLE;
	p->iFlags = ITEM_FLAG_NOAUTORELOAD|ITEM_FLAG_SUPERWEAPON;// XDM3038c
	p->iMaxClip = BEAMRIFLE_MAX_CLIP;
	p->iWeight = BEAMRIFLE_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_BEAMRIFLE;
	p->iPosition = POSITION_BEAMRIFLE;
#endif
	p->pszAmmo1 = "uranium";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponBeamRifle::Deploy(void)
{
	m_iClip = 0;
	m_fireState = FIRE_OFF;
	//return DefaultDeploy("models/v_beamrifle.mdl", "models/p_beamrifle.mdl", BEAMRIFLE_DRAW, "gauss", "weapons/beamrifle_select.wav");
	return DefaultDeploy(BEAMRIFLE_DRAW, "gauss", "weapons/beamrifle_select.wav");
}

void CWeaponBeamRifle::Holster(int skiplocal /* = 0 */)
{
	m_iClip = 0;
	m_fireState = FIRE_OFF;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(BEAMRIFLE_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

bool CWeaponBeamRifle::CanHolster(void) const
{
	if (m_fireState != FIRE_OFF)
		return false;

	return true;
}

bool CWeaponBeamRifle::IsUseable(void) const
{
	if (m_fireState != FIRE_OFF)// don't put away while firing!
		return true;

	if (!HasAmmo(AMMO_PRIMARY, min(BHG_AMMO_USE_MIN1,BHG_AMMO_USE_MIN2)))// minimum required amount
		return false;

	return CBasePlayerWeapon::IsUseable();
}

void CWeaponBeamRifle::PrimaryAttack(void)
{
	if (m_fireState != FIRE_OFF)
		return;

	if (!HasAmmo(AMMO_PRIMARY, BEAMRIFLE_USE_AMMO1) || (m_pPlayer->pev->waterlevel == WATERLEVEL_HEAD))
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		return;
	}

	m_fireMode = FIRE_PRI;
	StartFire();
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(BEAMRIFLE_CHARGE_INTERVAL1);
}

void CWeaponBeamRifle::SecondaryAttack(void)
{
	if (m_fireState != FIRE_OFF)
		return;

	if (!HasAmmo(AMMO_PRIMARY, BEAMRIFLE_USE_AMMO2) || (m_pPlayer->pev->waterlevel == WATERLEVEL_HEAD))
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		return;
	}

	m_fireMode = FIRE_SEC;
	StartFire();
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(BEAMRIFLE_CHARGE_INTERVAL2);
}

void CWeaponBeamRifle::StartFire(void)
{
	m_iClip = 0;
	m_fireState = FIRE_CHARGE;
	//SendWeaponAnim(BEAMRIFLE_SPINUP);

	//if (m_fireMode == FIRE_SEC)
	//	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/beamrifle_charge.wav", VOL_NORM, ATTN_NORM);
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, ((m_fireMode == FIRE_PRI)?BEAMRIFLE_CHARGE_INTERVAL1:BEAMRIFLE_CHARGE_INTERVAL2), 0.0f, BEAMRIFLE_SPINUP, m_fireState, pev->body, m_fireMode);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

void CWeaponBeamRifle::Fire(void)
{
	if (m_pPlayer == NULL)
		return;
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		m_fireState = FIRE_OFF;
		PlayEmptySound();
		return;
	}
	m_fireState = FIRE_ON;

	m_pPlayer->SetWeaponAnimType("gauss");
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	float damage = pev->dmg;
	float radius = 64.0f;
	if (m_fireMode == FIRE_PRI)
	{
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		damage *= 4.0f;
		radius *= 4.0f;
	}

	m_iClip = 0;
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition() + gpGlobals->v_forward*GetBarrelLength() + gpGlobals->v_right*4.0f + gpGlobals->v_up*-2.0f);
	//GET_ATTACHMENT(edict(), 0, vecSrc, pev->angles);
	Vector vecDir(gpGlobals->v_forward);
#ifdef CLIENT_DLL
	Vector vecDst(m_pPlayer->GetGunPosition() + vecDir*8192);// TEMP HACK
#else
	Vector vecDst(m_pPlayer->GetGunPosition() + vecDir*g_psv_zmax->value);
#endif
	Vector vecEnd(vecSrc);// a temporary start and end for each trace, must be kept after last trace!
	Vector vecCurrentStart;
	CBaseEntity *pEntity = m_pPlayer;// hit and ignore entity after that
	TraceResult tr;
	//bool bHit = false;
	for (unsigned int i=0; i<BEAMRIFLE_MAX_TRACES; ++i)
	{
		// don't hit one entity twice (or more)!
		vecCurrentStart = vecEnd;
		UTIL_TraceLine(vecCurrentStart, vecDst, dont_ignore_monsters, dont_ignore_glass, pEntity->edict(), &tr);// XDM3038: detect glass manually
		if (tr.fStartSolid || tr.fAllSolid)
			break;// no explosion whatsoever

		vecEnd = tr.vecEndPos;// remember last hit position

		if (tr.flFraction < 1.0f && tr.pHit)// beam can lose all it's energy before hitting anything?
		{
			//if (&tr.pHit->v == pEntity->pev)// somehow?
			//	continue;

			pEntity = CBaseEntity::Instance(tr.pHit);
			if (pEntity->IsRemoving())// XDM3038a
				continue;
			//if (pEntity->pev->solid == SOLID_NOT)
			//	continue;

			char cTextureType = CHAR_TEX_NULL;
#if !defined (CLIENT_DLL)
			if (pEntity->IsBSPModel())//pev->solid == SOLID_BSP)// XDM3038: detect glass (can be transparent metal, bushes, etc.)
			{
				//UTIL_DebugBeam(vecCurrentStart, vecEnd, 3.0, 255,127,0);
				cTextureType = TEXTURETYPE_Trace(pEntity, vecCurrentStart, vecEnd);// vecEnd is already set to tr.vecEndPos
				if (cTextureType == CHAR_TEX_GLASS && pEntity->pev->rendermode != kRenderNormal)
					continue;
			}
#endif
			if (pEntity->pev->takedamage != DAMAGE_NO)// punch through entities that take damage
			{
				if (pEntity->pev->rendermode == kRenderNormal && pEntity->IsBSPModel() && (cTextureType == CHAR_TEX_CONCRETE || cTextureType == CHAR_TEX_METAL || cTextureType == CHAR_TEX_COMPUTER))// XDM3038
				{
					// strong type breakables
				}
				else
				{
					ClearMultiDamage();
					pEntity->TraceAttack(GetDamageAttacker(), damage, vecDir, &tr, DMG_SHOCK | DMG_ENERGYBEAM);
#if !defined (CLIENT_DLL)
					StreakSplash(vecEnd, vecDir, 7, damage, 60, 240);// "r_efx.h"
#endif
					ApplyMultiDamage(this, GetDamageAttacker());
					continue;
				}
			}

			//const char *tex = NULL;
			if (pEntity->IsBSPModel())//pev->solid == SOLID_BSP)
			{
				// XDM3038	tex = TRACE_TEXTURE(tr.pHit, vecSrc, vecDst);
				vecEnd += tr.vecPlaneNormal * (radius * 0.1f);// XDM3037: since this is the last iteration anyway, we can fix the explosion position
			}

			// XDM3038	if (tex == NULL || tex && _stricmp(tex, "sky") != 0)// beam has not gone into the sky, make a blast
			if (/*true cTextureType == CHAR_TEX_NULL || */cTextureType != CHAR_TEX_SKY)
			{
				//bHit = true;
				PLAYBACK_EVENT_FULL(FEV_RELIABLE, NULL, m_usBeamBlast, 0.0f, vecEnd, g_vecZero, damage, radius, m_iStar, m_iGlow, 0, m_fireMode);
#if !defined (CLIENT_DLL)
				::RadiusDamage(vecEnd, this, GetDamageAttacker(), damage, radius, CLASS_NONE, DMG_BLAST | DMG_SHOCK | DMG_ENERGYBEAM | DMG_IGNOREARMOR);
				if (m_pPlayer == NULL)// player killed self
					break;
				if (m_fireMode == FIRE_SEC)
				{
					UTIL_ScreenShake(vecEnd, 4.0, 3.0, 2.0, radius*2);
					CSoundEnt::InsertSound(bits_SOUND_COMBAT, vecEnd, damage, 1.0f);
				}
#endif
			}
			break;
		}
		else// finished hitting
		{
			break;
		}
	}

	if (m_pPlayer == NULL)// player killed self
	{
		m_fireState = FIRE_OFF;
		return;
	}

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags | FEV_RELIABLE, m_pPlayer->edict(), m_usFireBeam, 0.0, vecSrc, g_vecZero, 0.0f, 0.0f, (m_fireMode == 0 ? BEAMRIFLE_FIRE1:BEAMRIFLE_FIRE2), m_iGlow, pev->body, m_fireMode);
#if !defined (CLIENT_DLL)
	MESSAGE_BEGIN(MSG_PVS, gmsgFireBeam, vecSrc);
		WRITE_COORD(vecSrc.x);// TODO: WRITE_FLOAT (+2 bytes per float, +client code)
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_COORD(vecEnd.x);
		WRITE_COORD(vecEnd.y);
		WRITE_COORD(vecEnd.z);
		WRITE_SHORT(m_iCircle);
		WRITE_SHORT(m_iBeam);
		WRITE_BYTE(m_fireMode);
	MESSAGE_END();
#endif

	m_fireState = FIRE_DISCHARGE;
	// discharge sound? animation?
	if (m_fireMode == FIRE_SEC)// XDM3035
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(BEAMRIFLE_DISCHARGE_INTERVAL2);
	else
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(BEAMRIFLE_DISCHARGE_INTERVAL1);

	m_flTimeWeaponIdle = m_flNextPrimaryAttack + 1.0f;// allow animaiton to finish
}

void CWeaponBeamRifle::WeaponIdle(void)
{
	if (m_fireState == FIRE_CHARGE)
	{
		if (m_pPlayer && m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
		{
			m_fireState = FIRE_OFF;
			//PlayEmptySound();
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/electro5.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
			SendWeaponAnim(BEAMRIFLE_SPINDOWN);
			m_fireState = FIRE_DISCHARGE;
#if !defined (CLIENT_DLL)
			RadiusDamage(pev->origin, this, GetDamageAttacker(), m_iClip*2.0f, 128, CLASS_NONE, DMG_SHOCK | DMG_NEVERGIB);
			UTIL_ScreenFade(m_pPlayer, Vector(160,160,255), 2.0, 0.5, 128, 0);// FFADE_IN
#endif
			m_iClip = 0;
			//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + BEAMRIFLE_DISCHARGE_INTERVAL;
			return;
		}
		else
		{
			if ((m_fireMode == FIRE_PRI && m_iClip >= BEAMRIFLE_USE_AMMO1) ||
				(m_fireMode == FIRE_SEC && m_iClip >= BEAMRIFLE_USE_AMMO2))// already charged
			{
				Fire();
			}
			else if (m_flNextAmmoBurn <= UTIL_WeaponTimeBase())
			{
				UseAmmo(AMMO_PRIMARY, 1);
				m_iClip++;
				m_flNextAmmoBurn = UTIL_WeaponTimeBase() + BEAMRIFLE_AMMO_USE_INTERVAL;
			}
		}
		return;
	}
	else if (m_fireState == FIRE_DISCHARGE)
	{
		m_fireState = FIRE_OFF;
		if (m_fireMode == FIRE_SEC)// XDM3035
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + BEAMRIFLE_DISCHARGE_INTERVAL2;
		else
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + BEAMRIFLE_DISCHARGE_INTERVAL1;

		return;// not necessary, but skips next if()
	}

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 5;// lessen the probability of playing the "fidget" animation
		else
			rmax = 0;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, rmax);
		if (r == 3)
			SendWeaponAnim(BEAMRIFLE_FIDGET3, pev->body);
		else if (r == 2)
			SendWeaponAnim(BEAMRIFLE_FIDGET2, pev->body);
		else if (r == 1)
			SendWeaponAnim(BEAMRIFLE_FIDGET1, pev->body);
		else
			SendWeaponAnim(BEAMRIFLE_IDLE, pev->body);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 6, 10);
	}
}

void CWeaponBeamRifle::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
}
