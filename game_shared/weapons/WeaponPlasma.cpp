//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "weaponslots.h"
#include "customentity.h"
#include "effects.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"
#include "skill.h"
#include "pm_shared.h"
#if !defined (CLIENT_DLL)
#include "plasmaball.h"
#endif

#define PLASMA_OLD_CODE		1// recreate beams everytime (new code is good, but it causes crashes in huge multiplayer battles)

enum vanim_plasma_e
{
	PLASMA_IDLE1 = 0,
	PLASMA_IDLE2,
	PLASMA_SPINUP,
	PLASMA_SPIN,
	PLASMA_SPINDOWN,
	PLASMA_SWITCH,
	PLASMA_DRAW,
	PLASMA_HOLSTER
};

// TODO: LATER: update discharging beam position

LINK_ENTITY_TO_CLASS(weapon_plasma, CWeaponPlasma);

TYPEDESCRIPTION CWeaponPlasma::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponPlasma, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponPlasma, m_pNoise1, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponPlasma, m_pNoise2, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponPlasma, m_pSprite, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponPlasma, m_fireMode, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponPlasma, m_fireState, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponPlasma, m_fSpriteActive, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponPlasma, CBasePlayerWeapon);

void CWeaponPlasma::Spawn(void)
{
	m_pBeam = NULL;
	m_pNoise1 = NULL;
	m_pNoise2 = NULL;
	m_pSprite = NULL;
	CBasePlayerWeapon::Spawn();// XDM3038a
}

void CWeaponPlasma::Precache(void)
{
	m_iId = WEAPON_PLASMA;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = PLASMA_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_plasma.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_plasma.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgPlasmaBall;

	PRECACHE_MODEL(PLASMA_BEAM_SPRITE);
	PRECACHE_MODEL(PLASMA_NOISE_SPRITE1);
	PRECACHE_MODEL(PLASMA_NOISE_SPRITE2);
	m_iSpriteIndexEnd = PRECACHE_MODEL(PLASMA_END_SPRITE);
	m_iSpriteIndexHit = PRECACHE_MODEL(PLASMA_HIT_SPRITE);
	m_usFire = PRECACHE_EVENT(1, "events/weapons/plasmagun.sc");
	PRECACHE_SOUND("weapons/plasma_select.wav");
	//PRECACHE_SOUND("weapons/plasma_windup.wav");
	PRECACHE_SOUND("weapons/plasma_startrun.wav");
	PRECACHE_SOUND("weapons/plasma_off.wav");
	PRECACHE_SOUND("weapons/plasma_switch.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	//UTIL_PrecacheOther("plasmaball");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponPlasma::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_PLASMA;
	p->iFlags = ITEM_FLAG_SUPERWEAPON|ITEM_FLAG_ADDDEFAULTAMMO2;// XDM3037
	p->iMaxClip = PLASMA_MAX_CLIP;
	p->iWeight = PLASMA_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_PLASMA;
	p->iPosition = POSITION_PLASMA;
#endif
	p->pszAmmo1 = "lightp";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = LP_MAX_CARRY;
#endif
	p->pszAmmo2 = "uranium";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = URANIUM_MAX_CARRY;
#endif
	return 1;
}

bool CWeaponPlasma::Deploy(void)
{
	m_fireState = FIRE_OFF;
	//bool ret = DefaultDeploy("models/v_plasma.mdl", "models/p_plasma.mdl", PLASMA_DRAW, "hive", "weapons/plasma_select.wav");
	bool ret = DefaultDeploy(PLASMA_DRAW, "hive", "weapons/plasma_select.wav");
#if !defined (PLASMA_OLD_CODE)
	if (ret)
		CreateEffect();// XDM3034
#endif
	return ret;
}

void CWeaponPlasma::Holster(int skiplocal /* = 0 */)
{
	if (m_fireMode == FIRE_SEC && m_fireState != FIRE_OFF)
		EndAttack();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(PLASMA_HOLSTER);
	DestroyEffect();// XDM3034
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

bool CWeaponPlasma::CanHolster(void) const
{
	if (m_fireState != FIRE_OFF)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: We cannot use PRIMARY with only one type of ammo, but we can use SECONDARY if we have secontary ammo
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPlasma::IsUseable(void) const
{
	if (!HasAmmo(AMMO_SECONDARY))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Ball attack
//-----------------------------------------------------------------------------
void CWeaponPlasma::PrimaryAttack(void)
{
	if (m_fireMode == FIRE_SEC && m_fireState != FIRE_OFF && m_fireState != FIRE_DISCHARGE)// || m_pBeam != NULL || m_pNoise1 != NULL || m_pNoise2 != NULL)
	{
		EndAttack();
		return;
	}

	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		if (m_fireState != FIRE_OFF)
			EndAttack();
		else
			PlayEmptySound();

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + PLASMA_ATTACK_INTERVAL1;
		return;
	}

	if (UseAmmo(AMMO_PRIMARY|AMMO_SECONDARY, 1) == 1)
	{
		m_fireMode = FIRE_PRI;
		m_fireState = FIRE_ON;
		int flags;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero, 0.0, 0.0, PLASMA_SPINDOWN, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));

#if !defined (CLIENT_DLL)
		Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward*GetBarrelLength() + gpGlobals->v_right*2.0f;
		CPlasmaBall::CreatePB(vecSrc, vecAng, gpGlobals->v_forward, PLASMABALL_SPEED, pev->dmg, GetDamageAttacker(), GetDamageAttacker(), 0);
#endif
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}
	else
		PlayEmptySound();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(PLASMA_ATTACK_INTERVAL1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: beam attack
//-----------------------------------------------------------------------------
void CWeaponPlasma::SecondaryAttack(void)
{
	if (m_fireMode == FIRE_PRI && m_fireState != FIRE_OFF)
		return;

	m_fireMode = FIRE_SEC;
	Attack();
}

//-----------------------------------------------------------------------------
// Purpose: Attack
//-----------------------------------------------------------------------------
void CWeaponPlasma::Attack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
//#if !defined (PLASMA_OLD_CODE)
		if (m_fireState != FIRE_OFF)// || m_pBeam != NULL || m_pNoise1 != NULL || m_pNoise2 != NULL)
			EndAttack();
		else
			PlayEmptySound();

		return;
	}

	if (m_fireState == FIRE_OFF)
	{
		if (!HasAmmo(AMMO_SECONDARY))
		{
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;
			PlayEmptySound();
			return;
		}
		m_flNextAmmoBurn = gpGlobals->time;// start using ammo ASAP.
		m_pPlayer->m_iWeaponVolume = PLASMA_ATTACK_VOLUME/2;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		SendWeaponAnim(PLASMA_SPINUP);
		pev->dmgtime = gpGlobals->time + PLASMA_PULSE_INTERVAL;
		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/plasma_startrun.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);// XDM3038
		UseAmmo(AMMO_SECONDARY, 1);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + PLASMA_CHARGE_INTERVAL;
		m_fireState = FIRE_CHARGE;
	}
	else if (m_fireState == FIRE_CHARGE)
	{
		m_pPlayer->m_iWeaponVolume = PLASMA_ATTACK_VOLUME;
		SendWeaponAnim(PLASMA_SPIN);
		CreateEffect();
		m_fireState = FIRE_ON;
	}
	else if (m_fireState == FIRE_ON)
	{
		//UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		Vector forward;//, up, right;
		AngleVectors(m_pPlayer->pev->v_angle/* + m_pPlayer->pev->punchangle*/, forward, NULL, NULL);// XDM3035a
		//Vector vecSrc = m_pPlayer->GetGunPosition() + up * -8.0f + right * 3.0f;

		/*static lasttime;
		conprintf(1, "zazaza delta %g\n", gpGlobals->time - lasttime);
		lasttime = gpGlobals->time;*/

		Fire(m_pPlayer->GetGunPosition(), forward);

		if (!HasAmmo(AMMO_SECONDARY))
		{
			EndAttack();
			//m_fireState = FIRE_OFF;
		}
		else
		{
			m_pPlayer->m_flNextAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + PLASMA_PULSE_INTERVAL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigSrc - 
//			&vecDir - 
//-----------------------------------------------------------------------------
void CWeaponPlasma::Fire(const Vector &vecOrigSrc, const Vector &vecDir)
{
	TraceResult tr;
	UTIL_TraceLine(vecOrigSrc, vecOrigSrc + vecDir*PLASMA_BEAM_DISTANCE, dont_ignore_monsters, m_pPlayer->edict(), &tr);

	if (tr.fAllSolid)
		return;

#if !defined (CLIENT_DLL)
	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity && m_pSprite)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO && !m_fSpriteActive)
		{
			if (IsMultiplayer())
			{
				ClearBits(m_pSprite->pev->effects, EF_NODRAW);
				EMIT_SOUND_DYN(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav", VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
			}
			m_pSprite->SetTexture(m_iSpriteIndexHit);
			m_fSpriteActive = TRUE;
		}
		else if (pEntity->pev->takedamage == DAMAGE_NO && m_fSpriteActive)
		{
			if (IsMultiplayer())
			{
				STOP_SOUND(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav");
				SetBits(m_pSprite->pev->effects, EF_NODRAW);
			}
			m_pSprite->SetTexture(m_iSpriteIndexEnd);
			m_fSpriteActive = FALSE;
		}
	}

	if (!m_pPlayer->IsAlive())// XDM3035
		return;

	if (pev->dmgtime < gpGlobals->time)
	{
		ClearMultiDamage();
		if (pEntity)
		{
			if (pEntity->pev->takedamage != DAMAGE_NO)
			{
				tr.iHitgroup = HITGROUP_GENERIC;// XDM3035c: ignore all hitgroups
				pEntity->TraceAttack(GetDamageAttacker(), gSkillData.DmgPlasma, vecDir, &tr, (sv_overdrive.value > 0.0f)?(DMG_BURN|DMG_ENERGYBEAM|DMG_ALWAYSGIB|DMG_IGNITE):(DMG_BURN|DMG_ENERGYBEAM|DMG_NEVERGIB));
			}
			ApplyMultiDamage(this, GetDamageAttacker());// XDM3034 inflictor = this
		}

		if (gpGlobals->time >= m_flNextAmmoBurn)
		{
			//SendWeaponAnim(PLASMA_SPIN);// animation is looping. don't play every frame!
			UseAmmo(AMMO_SECONDARY, 1);
			m_flNextAmmoBurn = gpGlobals->time + 0.2f;
		}
		pev->dmgtime = gpGlobals->time + PLASMA_PULSE_INTERVAL;

		if (m_pBeam)
		{
			if (pEntity && pEntity->pev->takedamage != DAMAGE_NO && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
				m_pBeam->pev->playerclass = BEAM_FSPARKS_START;// cl sparks
			else
				m_pBeam->pev->playerclass = 0;
		}
	}
	UpdateEffect(vecOrigSrc, tr.vecEndPos);
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Turn off all effects, sounds, etc.
//-----------------------------------------------------------------------------
void CWeaponPlasma::EndAttack(void)
{
	if (m_fireState == FIRE_DISCHARGE)
		return;

	if (m_fireMode == FIRE_PRI)// XDM3037: in case somehow we get here in primary mode
	{
		m_fireState = FIRE_OFF;
		return;
	}

	if (m_fireState != FIRE_OFF)
	{
		SendWeaponAnim(PLASMA_SPINDOWN);
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/plasma_startrun.wav");
		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/plasma_off.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

#if defined (PLASMA_OLD_CODE)// XDM3035: cut all post-effects
		DestroyEffect();
#endif // PLASMA_OLD_CODE

#if !defined (CLIENT_DLL)
		if (m_pBeam)
		{
			m_pBeam->SetFlags(BEAM_FSHADEIN);
			// XDM3035		m_pBeam->Expand(10, 500);
#ifdef PLASMA_OLD_CODE
			m_pBeam = NULL;
#endif // PLASMA_OLD_CODE
		}
		if (m_pNoise1)
		{
			//m_pNoise1->Expand(2, 600);
			SetBits(m_pNoise1->pev->effects, EF_NODRAW);
#ifdef PLASMA_OLD_CODE
			UTIL_Remove(m_pNoise1);
			m_pNoise1 = NULL;
#endif // PLASMA_OLD_CODE
		}
		if (m_pNoise2)
		{
			SetBits(m_pNoise2->pev->effects, EF_NODRAW);
#ifdef PLASMA_OLD_CODE
			UTIL_Remove(m_pNoise2);
			m_pNoise2 = NULL;
#endif // PLASMA_OLD_CODE
		}
		if (m_pSprite)
		{
// XDM3035			m_pSprite->Expand(5, 500);
#ifdef PLASMA_OLD_CODE
			m_pSprite = NULL;
#endif // PLASMA_OLD_CODE
		}
#endif // !CLIENT_DLL
	}

//	DestroyEffect();
	m_fireState = FIRE_DISCHARGE;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + PLASMA_ATTACK_INTERVAL2;
}

//-----------------------------------------------------------------------------
// Purpose: Create data AND/OR reset parameters
//-----------------------------------------------------------------------------
void CWeaponPlasma::CreateEffect(void)
{
#if !defined (CLIENT_DLL)

#if defined (PLASMA_OLD_CODE)
	DestroyEffect();
#endif // PLASMA_OLD_CODE

	// ---------------- BEAM ----------------
	if (m_pBeam == NULL)
		m_pBeam = CBeam::BeamCreate(PLASMA_BEAM_SPRITE, PLASMA_BEAM_WIDTH);
	if (m_pBeam)
	{
		if (m_fireState == FIRE_OFF)
			SetBits(m_pBeam->pev->effects, EF_NODRAW);
		else
			ClearBits(m_pBeam->pev->effects, EF_NODRAW);

		m_pBeam->PointEntInit(pev->origin, m_pPlayer->entindex());
		m_pBeam->SetEndAttachment(1);
		m_pBeam->SetScrollRate(25);
		m_pBeam->SetWidth(PLASMA_BEAM_WIDTH);// !!
		m_pBeam->SetNoise(0);
		if (m_fireMode == FIRE_PRI)
		{
			m_pBeam->SetColor(255, 255, 255);
			m_pBeam->SetBrightness(191);
		}
		else
		{
			m_pBeam->SetColor(255, 255, 255);
			m_pBeam->SetBrightness(255);
		}
		m_pBeam->pev->frame = 0;
		SetBits(m_pBeam->pev->spawnflags, SF_BEAM_TEMPORARY);// Flag these to be destroyed on save/restore or level transition
		m_pBeam->pev->owner = m_pPlayer->edict();
		m_pBeam->SetFlags(0);// reset shade effect
		m_pBeam->pev->iStepLeft = BEAMFX_PLASMA1+m_fireMode;
		m_pBeam->pev->playerclass = 0;
	}
	// ---------------- NOISE 1 ----------------
	if (m_pNoise1 == NULL)
		m_pNoise1 = CBeam::BeamCreate(PLASMA_NOISE_SPRITE1, PLASMA_NOISE_WIDTH1);
	if (m_pNoise1)
	{
		if (m_fireState == FIRE_OFF)
			SetBits(m_pNoise1->pev->effects, EF_NODRAW);
		else
			ClearBits(m_pNoise1->pev->effects, EF_NODRAW);

		m_pNoise1->PointEntInit(pev->origin, m_pPlayer->entindex());
		m_pNoise1->SetEndAttachment(1);
		m_pNoise1->SetScrollRate(25);
		m_pNoise1->SetWidth(PLASMA_NOISE_WIDTH1);// !!
		if (m_fireMode == FIRE_PRI)
		{
			m_pNoise1->SetNoise(10 + 8*sv_overdrive.value);
			m_pNoise1->SetColor(191, 63, 255);
			m_pNoise1->SetBrightness(191);
		}
		else
		{
			m_pNoise1->SetNoise(6 + 4*sv_overdrive.value);
			m_pNoise1->SetColor(191, 255, 0);
			m_pNoise1->SetBrightness(207);
		}
		m_pNoise1->pev->frame = 0;
		SetBits(m_pNoise1->pev->spawnflags, SF_BEAM_TEMPORARY);
		m_pNoise1->pev->owner = m_pPlayer->edict();
	}
	// ---------------- NOISE 2 ----------------
	if (m_pNoise2 == NULL && g_pGameRules && g_pGameRules->FAllowEffects())
		m_pNoise2 = CBeam::BeamCreate(PLASMA_NOISE_SPRITE2, PLASMA_NOISE_WIDTH2);
	if (m_pNoise2)
	{
		if (m_fireState == FIRE_OFF)
			SetBits(m_pNoise2->pev->effects, EF_NODRAW);
		else
			ClearBits(m_pNoise2->pev->effects, EF_NODRAW);

		m_pNoise2->PointEntInit(pev->origin, m_pPlayer->entindex());
		m_pNoise2->SetEndAttachment(1);
		m_pNoise2->SetScrollRate(20);
		m_pNoise2->SetWidth(PLASMA_NOISE_WIDTH2);// !!
		if (m_fireMode == FIRE_PRI)
		{
			m_pNoise2->SetNoise(18 + 16*sv_overdrive.value);
			m_pNoise2->SetColor(143, 207, 255);
			m_pNoise2->SetBrightness(191);
		}
		else
		{
			m_pNoise2->SetNoise(14 + 12*sv_overdrive.value);
			m_pNoise2->SetColor(0, 0, 255);
			m_pNoise2->SetBrightness(255);
		}
		m_pNoise2->SetFlags(BEAM_FSHADEIN);
		m_pNoise2->pev->frame = 0;
		SetBits(m_pNoise2->pev->spawnflags, SF_BEAM_TEMPORARY);
		m_pNoise2->pev->owner = m_pPlayer->edict();
	}
	// ---------------- SPRITE ----------------
	if (m_pSprite == NULL)
		m_pSprite = CSprite::SpriteCreate(PLASMA_END_SPRITE, pev->origin, TRUE);
	else
		UTIL_SetOrigin(m_pSprite, pev->origin);

	if (m_pSprite)
	{
		if (m_fireState == FIRE_OFF)
			SetBits(m_pSprite->pev->effects, EF_NODRAW);
		else
			ClearBits(m_pSprite->pev->effects, EF_NODRAW);

		m_pSprite->SetTransparency(kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation);
		m_pSprite->pev->frame = 0;
		m_pSprite->pev->scale = 0.6;
		SetBits(m_pSprite->pev->spawnflags, SF_SPRITE_TEMPORARY);
		m_pSprite->pev->owner = m_pPlayer->edict();

		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
			m_pSprite->pev->framerate = 16.0f;// XDM3035c: vital
			//SetBits(m_pSprite->pev->effects, EF_DIMLIGHT);
		}
		else
			m_pSprite->pev->framerate = 12.0f;

		if (IsMultiplayer())
			SetBits(m_pSprite->pev->effects, EF_NODRAW);

		if (!FBitSet(m_pSprite->pev->effects, EF_NODRAW))// XDM3035a
			EMIT_SOUND_DYN(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav", VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
	}
#if !defined (PLASMA_OLD_CODE)
	if (m_pBeam)
		SetBits(m_pBeam->pev->flags, FL_GODMODE);// XDM3034: flag to not to be removed after Expand()
	if (m_pNoise1)
		SetBits(m_pNoise1->pev->flags, FL_GODMODE);
	if (m_pNoise2)
		SetBits(m_pNoise2->pev->flags, FL_GODMODE);
	if (m_pSprite)
		SetBits(m_pSprite->pev->flags, FL_GODMODE);
#endif // !PLASMA_OLD_CODE
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Destroy data, no effects
//-----------------------------------------------------------------------------
void CWeaponPlasma::DestroyEffect(void)
{
#if !defined (CLIENT_DLL)
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}
	if (m_pNoise1)
	{
		UTIL_Remove(m_pNoise1);
		m_pNoise1 = NULL;
	}
	if (m_pNoise2)
	{
		UTIL_Remove(m_pNoise2);
		m_pNoise2 = NULL;
	}
	if (m_pSprite)
	{
		STOP_SOUND(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav");
		UTIL_Remove(m_pSprite);
		m_pSprite = NULL;
	}
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Update beams, sprites, sounds...
// Input  : &startPoint - 
//			&endPoint - 
//-----------------------------------------------------------------------------
void CWeaponPlasma::UpdateEffect(const Vector &startPoint, const Vector &endPoint)
{
#if !defined (CLIENT_DLL)
	if (m_pBeam == NULL || m_pNoise1 == NULL || m_pNoise2 == NULL || m_pSprite == NULL)
		CreateEffect();

	if (m_pBeam && !FBitSet(m_pBeam->pev->effects, EF_NODRAW))
		m_pBeam->SetStartPos(endPoint);

	if (m_pNoise1 && !FBitSet(m_pNoise1->pev->effects, EF_NODRAW))
	{
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// traffic
			m_pNoise1->pev->renderamt += 16.0f*sinf(gpGlobals->time);

		m_pNoise1->SetStartPos(endPoint);
		UTIL_SetOrigin(m_pNoise1, endPoint);
	}
	if (m_pNoise2 && !FBitSet(m_pNoise2->pev->effects, EF_NODRAW))
	{
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// traffic
			m_pNoise2->pev->renderamt += 10.0f*sinf(gpGlobals->time*2.0f);

		m_pNoise2->SetStartPos(endPoint);
		UTIL_SetOrigin(m_pNoise2, endPoint);
	}
	if (m_pSprite && !FBitSet(m_pSprite->pev->effects, EF_NODRAW))
	{
		UTIL_SetOrigin(m_pSprite, endPoint);
		m_pSprite->AnimateThink();
		/*m_pSprite->pev->frame += 16.0f * gpGlobals->frametime;

		if (m_pSprite->pev->frame > m_pSprite->Frames())
			m_pSprite->pev->frame = 0;*/
	}
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM3038c
		SetBits(m_pPlayer->pev->effects, EF_MUZZLEFLASH);
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Player have released all buttons
//-----------------------------------------------------------------------------
void CWeaponPlasma::WeaponIdle(void)
{
	if (m_fireMode == FIRE_PRI)// XDM3037: shot-based mode shuts down when WeaponIdle() gets called
		m_fireState = FIRE_OFF;

	if (m_fireState == FIRE_DISCHARGE)
	{
#if !defined (CLIENT_DLL)
#if !defined (PLASMA_OLD_CODE)
		if (m_pBeam && m_pNoise1 && m_pNoise2 && m_pSprite)
		{
			Vector forward;
			Vector vecSrc = m_pPlayer->GetGunPosition();
			TraceResult tr;
			AngleVectors(m_pPlayer->pev->v_angle/* + m_pPlayer->pev->punchangle*/, forward, NULL, NULL);// XDM3035a
			//UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
			UTIL_TraceLine(vecSrc, vecSrc + forward * g_psv_zmax->value, dont_ignore_monsters, ignore_glass, m_pPlayer->edict(), &tr);
			UpdateEffect(vecSrc, tr.vecEndPos);

			// wait until all beams and sprites finish to Expand()
			if (FBitSet)m_pBeam->pev->effects, EF_NODRAW) &&
				FBitSet)m_pNoise1->pev->effects, EF_NODRAW) &&
				FBitSet)m_pNoise2->pev->effects, EF_NODRAW) &&
				FBitSet)m_pSprite->pev->effects, EF_NODRAW))
			{
				m_fireState = FIRE_OFF;
			}
		}
		else
#endif // !PLASMA_OLD_CODE
#endif // !CLIENT_DLL
			m_fireState = FIRE_OFF;
	}

	if (m_fireState != FIRE_OFF)
		EndAttack();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_pBeam || m_pNoise1 || m_pNoise2 || m_pSprite)// XDM3036: delete hung beams
		DestroyEffect();

	SendWeaponAnim(RANDOM_INT2(PLASMA_IDLE1, PLASMA_IDLE2));

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 3.5, 4.0);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + PLASMA_ATTACK_INTERVAL1;
}

//-----------------------------------------------------------------------------
// Purpose: Entity is detached from its former owner
//-----------------------------------------------------------------------------
void CWeaponPlasma::OnDetached(void)// XDM3038c
{
	CBasePlayerWeapon::OnDetached();// XDM3038c
	DestroyEffect();
}

//-----------------------------------------------------------------------------
// Purpose: Server-side only. Pack data.
// Warning: Weapon classes must call their parent's functions!
// Input  : *player - receiver
//			*weapondata - pack data into this structure
//-----------------------------------------------------------------------------
void CWeaponPlasma::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
}
