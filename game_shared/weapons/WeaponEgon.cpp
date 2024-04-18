#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"

// differs from HL
enum vanim_egon_e
{
	EGON_IDLE = 0,
	EGON_FIDGET,
	EGON_ALTIDLE,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRECYCLE,
	EGON_DRAW,
	EGON_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_egon, CWeaponEgon);

TYPEDESCRIPTION	CWeaponEgon::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponEgon, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponEgon, m_pNoise, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponEgon, m_pSprite, FIELD_CLASSPTR),
	DEFINE_FIELD(CWeaponEgon, timedist, FIELD_TIME),
	DEFINE_FIELD(CWeaponEgon, m_flAmmoUseTime, FIELD_TIME),
	DEFINE_FIELD(CWeaponEgon, m_fireMode, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponEgon, m_fireState, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponEgon, CBasePlayerWeapon);

void CWeaponEgon::Spawn(void)
{
	m_pBeam = NULL;
	m_pNoise = NULL;
	m_pSprite = NULL;
	CBasePlayerWeapon::Spawn();// XDM3038a
}

void CWeaponEgon::Precache(void)
{
	m_iId = WEAPON_EGON;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = EGON_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_egon.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_egon.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgEgon;

	PRECACHE_MODEL(EGON_BEAM_SPRITE);
	PRECACHE_MODEL(EGON_SPIRAL_SPRITE);
	PRECACHE_MODEL(EGON_FLARE_SPRITE);
	PRECACHE_SOUND("weapons/egon_select.wav");
	PRECACHE_SOUND("weapons/egon_startrun1.wav");// XDM3038
	PRECACHE_SOUND("weapons/egon_startrun2.wav");
	PRECACHE_SOUND("weapons/egon_off1.wav");
	PRECACHE_SOUND("weapons/egon_off2.wav");
	PRECACHE_SOUND("weapons/egon_switch1.wav");
	PRECACHE_SOUND("weapons/egon_switch2.wav");
	// OBSOLETE	PRECACHE_SOUND("weapons/egon_windup1.wav");
	//PRECACHE_SOUND("weapons/egon_windup2.wav");
	//PRECACHE_SOUND("weapons/xbow_fly1.wav");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponEgon::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_EGON;
	p->iFlags = ITEM_FLAG_SUPERWEAPON;// XDM3035
	p->iMaxClip = EGON_MAX_CLIP;
	p->iWeight = EGON_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_EGON;
	p->iPosition = POSITION_EGON;
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponEgon::Deploy(void)
{
	m_fireState = FIRE_OFF;
	//return DefaultDeploy("models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon", "weapons/egon_select.wav");
	return DefaultDeploy(EGON_DRAW, "egon", "weapons/egon_select.wav");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::Holster(int skiplocal /* = 0 */)
{
	EndAttack();
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(EGON_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponEgon::CanHolster(void) const
{
	if (m_fireState != FIRE_OFF)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::PrimaryAttack(void)
{
	Attack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::SecondaryAttack(void)
{
	if (m_fireState != FIRE_OFF || m_pBeam != NULL || m_pNoise != NULL)
	{
		EndAttack();
		return;
	}

	if (m_fireMode == FIRE_PRI)
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/egon_switch1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		SendWeaponAnim(EGON_ALTFIREON);
		m_fireMode = FIRE_SEC;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + EGON_SWITCH_INTERVAL1;
	}
	else
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/egon_switch2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		SendWeaponAnim(EGON_ALTFIREOFF);
		m_fireMode = FIRE_PRI;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + EGON_SWITCH_INTERVAL2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::Attack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		if (m_fireState != FIRE_OFF || m_pBeam != NULL || m_pNoise != NULL)
			EndAttack();
		else
			PlayEmptySound();

		return;
	}

	if (m_fireState == FIRE_OFF)
	{
		if (!HasAmmo(AMMO_PRIMARY))
		{
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + EGON_ATTACK_INTERVAL1;
			PlayEmptySound();
			return;
		}

		m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.
		//PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usEgonFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, m_fireState, m_fireMode, 1, 0);
		m_pPlayer->m_iWeaponVolume = EGON_ATTACK_VOLUME / 2;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		UseAmmo(AMMO_PRIMARY, 1);// just to start
		if (m_fireMode == FIRE_PRI)
		{
			SendWeaponAnim(EGON_FIRECYCLE);
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_startrun1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);// XDM3038
			//EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/egon_windup1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + EGON_ATTACK_INTERVAL1;
		}
		else
		{
			SendWeaponAnim(EGON_ALTFIRECYCLE);
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_startrun2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);// XDM3038
			//EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/egon_windup2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + EGON_ATTACK_INTERVAL2;
		}
#if !defined (CLIENT_DLL)
		UTIL_ScreenShakeOne(m_pPlayer, m_pPlayer->pev->origin, ((m_fireMode == FIRE_PRI)?3.0f:4.0f), 2.0f, 1.0f);
#endif
		pev->dmgtime = gpGlobals->time + EGON_PULSE_INTERVAL;
		m_fireState = FIRE_CHARGE;
	}
	else if (m_fireState == FIRE_CHARGE)
	{
		/*if (m_fireMode == FIRE_PRI)
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_run1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		else
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_run2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);*/

		m_pPlayer->m_iWeaponVolume = EGON_ATTACK_VOLUME;
		m_fireState = FIRE_ON;
	}
	else if (m_fireState == FIRE_ON)
	{
		//UTIL_MakeVectors(m_pPlayer->pev->v_angle/* + m_pPlayer->pev->punchangle*/);
		//Fire(m_pPlayer->GetGunPosition(), gpGlobals->v_forward);
		Vector forward;//, up, right;
		AngleVectors(m_pPlayer->pev->v_angle/* + m_pPlayer->pev->punchangle*/, forward, NULL, NULL);// up, right);// XDM3035a
		//Vector vecSrc = m_pPlayer->GetGunPosition() + up * -8.0f + right * 3.0f;
		//Fire(vecSrc, forward);
		Fire(m_pPlayer->GetGunPosition(), forward);// XDM3038: simple and precise

		if (HasAmmo(AMMO_PRIMARY))
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + EGON_PULSE_INTERVAL;
		else
			EndAttack();
	}
	//else if (m_fireState == FIRE_DISCHARGE)// LATER: update discharging beam potition
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigSrc - 
//			&vecDir - 
//-----------------------------------------------------------------------------
void CWeaponEgon::Fire(const Vector &vecOrigSrc, const Vector &vecDir)
{
	TraceResult tr;
	Vector vecEnd(vecDir);
	vecEnd *= ((m_fireMode == FIRE_PRI)?EGON_DISTANCE1:EGON_DISTANCE2);
	vecEnd += vecOrigSrc;
	UTIL_TraceLine(vecOrigSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

	if (tr.fAllSolid)
		return;

#if !defined (CLIENT_DLL)
	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	if (IsMultiplayer() && m_pSprite)
	{
		if (pEntity && pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (FBitSet(m_pSprite->pev->effects, EF_NODRAW))
			{
				ClearBits(m_pSprite->pev->effects, EF_NODRAW);
				//EMIT_SOUND_DYN(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav", VOL_NORM, ATTN_IDLE, 0, 75);
			}
		}
		else
		{
			if (!FBitSet(m_pSprite->pev->effects, EF_NODRAW))
			{
				//STOP_SOUND(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav");
				SetBits(m_pSprite->pev->effects, EF_NODRAW);
			}
		}
	}

	if (pEntity == NULL)
		return;

	if (!m_pPlayer->IsAlive())// XDM: ???
		return;
#endif

	CBaseEntity *pAttacker = GetDamageAttacker();
	if (m_fireMode == FIRE_PRI)// PRIMARY fire mode: beam inflicts half direct damage and half radius damage
	{
		if (pev->dmgtime < gpGlobals->time)
		{
#if !defined (CLIENT_DLL)
			//if (m_pBeam)
			//	m_pBeam->pev->playerclass = 0;
			// wide mode does damage to the ent, and radius damage
			ClearMultiDamage();
			if (pEntity->pev->takedamage != DAMAGE_NO)
				pEntity->TraceAttack(pAttacker, pev->dmg*0.5f, vecDir, &tr, DMG_ENERGYBEAM);

			ApplyMultiDamage(this, pAttacker);
			// radius damage a little more potent.
			::RadiusDamage(tr.vecEndPos, this, pAttacker, pev->dmg*0.5f, 32, CLASS_NONE, DMG_ENERGYBEAM | DMG_RADIUS_MAX);

			if (m_pPlayer == NULL)// player killed self
			{
				DestroyEffect();
				m_fireState = FIRE_OFF;
				return;
			}

			/*if (pEntity->pev->takedamage && g_pGameRules->FAllowEffects())
			{
				//if (pEntity->IsPlayer())
				//	UTIL_ScreenShakeOne(pEntity, pEntity->pev->origin, 3.0f, 2.0f, 0.5f);
				//if (g_pGameRules->FAllowEffects())
				//	StreakSplash(tr.vecEndPos, RandomVector(), 7, 32, 32, 240);// "r_efx.h"
				if (m_pBeam)
					m_pBeam->pev->playerclass = BEAM_FSPARKS_START;
			}*/
#endif
			if (gpGlobals->time >= m_flAmmoUseTime)// use 4 ammo per second
			{
				UseAmmo(AMMO_PRIMARY, 1);
				//SendWeaponAnim(EGON_FIRECYCLE);// XDM: animation is looping
				m_flAmmoUseTime = gpGlobals->time + 0.15f;
			}
			pev->dmgtime = gpGlobals->time + EGON_PULSE_INTERVAL;
		}
	}
	else// SECONDARY fire mode: beam inflicts only direct damage and eats less ammo
	{
		if (pev->dmgtime < gpGlobals->time)
		{
#if !defined (CLIENT_DLL)
			//if (m_pBeam)
			//	m_pBeam->pev->playerclass = 0;
			if (pEntity->pev->takedamage != DAMAGE_NO)
			{
				ClearMultiDamage();// XDM3035b: TESTME
				pEntity->TraceAttack(pAttacker, pev->dmg, vecDir, &tr, DMG_ENERGYBEAM);
				ApplyMultiDamage(this, pAttacker);
				/*if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
				{
					//if (g_pGameRules->FAllowEffects())
					//	StreakSplash(tr.vecEndPos, RandomVector(), 7, 24, 40, 240);// "r_efx.h"
					if (m_pBeam)
						m_pBeam->pev->playerclass = BEAM_FSPARKS_START;
				}*/
			}
#endif
			if (gpGlobals->time >= m_flAmmoUseTime)// use about 3 ammo per second
			{
				UseAmmo(AMMO_PRIMARY, 1);
				//SendWeaponAnim(EGON_ALTFIRECYCLE);
				m_flAmmoUseTime = gpGlobals->time + 0.2f;
			}
			pev->dmgtime = gpGlobals->time + EGON_PULSE_INTERVAL;
		}
	}
#if !defined (CLIENT_DLL)
	if (m_pBeam)
	{
		if (pEntity && pEntity->pev->takedamage != DAMAGE_NO && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
			m_pBeam->pev->playerclass = BEAM_FSPARKS_START;// cl sparks
		else
			m_pBeam->pev->playerclass = 0;
	}
#endif
	timedist = (pev->dmgtime - gpGlobals->time) / EGON_PULSE_INTERVAL;

	if (timedist < 0.0f)
		timedist = 0.0f;
	else if (timedist > 1.0f)
		timedist = 1.0f;

	timedist = 1.0f - timedist;
	UpdateEffect(vecOrigSrc, tr.vecEndPos, timedist);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::EndAttack(void)
{
	if (m_fireState != FIRE_OFF)
	{
		if (m_fireMode == FIRE_PRI)
		{
			STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_startrun1.wav");// XDM3038
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_off1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + EGON_ATTACK_INTERVAL1;
		}
		else
		{
			STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_startrun2.wav");
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/egon_off2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + EGON_ATTACK_INTERVAL2;
		}
	}
	m_fireState = FIRE_OFF;
	DestroyEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::CreateEffect(void)
{
#if !defined (CLIENT_DLL)
	DestroyEffect();
	m_pBeam = CBeam::BeamCreate(EGON_SPIRAL_SPRITE, EGON_SPIRAL_WIDTH1);// XDM: normal spiral beam
	if (m_pBeam)
	{
		m_pBeam->PointEntInit(pev->origin, m_pPlayer->entindex());
		m_pBeam->SetFlags(BEAM_FSINE);
		m_pBeam->SetEndAttachment(1);
		m_pBeam->SetBrightness(255);
		SetBits(m_pBeam->pev->spawnflags, SF_BEAM_TEMPORARY);// Flag these to be destroyed on save/restore or level transition
		m_pBeam->pev->owner = m_pPlayer->edict();
		m_pBeam->pev->iStepLeft = BEAMFX_GLUON1+m_fireMode;
		m_pBeam->pev->playerclass = 0;
	}
	m_pNoise = CBeam::BeamCreate(EGON_BEAM_SPRITE, EGON_BEAM_WIDTH);// XDM: central beam with cool sprite
	if (m_pNoise)
	{
		m_pNoise->PointEntInit(pev->origin, m_pPlayer->entindex());
		m_pNoise->SetScrollRate(25);
		m_pNoise->SetEndAttachment(1);
		m_pNoise->SetBrightness(255);
		SetBits(m_pNoise->pev->spawnflags, SF_BEAM_TEMPORARY);
		m_pNoise->pev->owner = m_pPlayer->edict();
	}
	m_pSprite = CSprite::SpriteCreate(EGON_FLARE_SPRITE, pev->origin, FALSE);
	if (m_pSprite)
	{
		m_pSprite->SetTransparency(kRenderGlow, 255,255,255, 255, kRenderFxNoDissipation);
		m_pSprite->pev->scale = 1.0f;
		SetBits(m_pSprite->pev->spawnflags, SF_SPRITE_TEMPORARY);
		m_pSprite->pev->owner = m_pPlayer->edict();
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
			m_pSprite->pev->framerate = 16.0f;// XDM3035c: vital
			SetBits(m_pSprite->pev->effects, EF_DIMLIGHT);
		}
		else
			m_pSprite->pev->framerate = 12.0f;
	}
	if (m_fireMode == FIRE_PRI)
	{
		if (m_pBeam)
		{
			m_pBeam->SetWidth(EGON_SPIRAL_WIDTH1);
			m_pBeam->SetScrollRate(60);
			m_pBeam->SetNoise(8 + 8*sv_overdrive.value);
			if (!StringToVec(sv_glu_pri_rgb.string, m_pBeam->pev->rendercolor))
				m_pBeam->pev->rendercolor = EGON_DEFAULT_COLOR1;
		}
		if (m_pNoise)
		{
			m_pNoise->SetNoise(0);
			if (!StringToVec(sv_glu_sec_rgb.string, m_pNoise->pev->rendercolor))
				m_pNoise->pev->rendercolor = EGON_DEFAULT_COLOR2;
		}
	}
	else
	{
		if (m_pBeam)
		{
			m_pBeam->SetWidth(EGON_SPIRAL_WIDTH2);
			m_pBeam->SetScrollRate(100);
			m_pBeam->SetNoise(16 + 16*sv_overdrive.value);
			if (!StringToVec(sv_glu_sec_rgb.string, m_pBeam->pev->rendercolor))
				m_pBeam->pev->rendercolor = EGON_DEFAULT_COLOR2;
		}
		if (m_pNoise)
		{
			m_pNoise->SetNoise(4);
			if (!StringToVec(sv_glu_pri_rgb.string, m_pNoise->pev->rendercolor))
				m_pNoise->pev->rendercolor = EGON_DEFAULT_COLOR1;
		}
	}

	if (m_pSprite)
		m_pSprite->pev->rendercolor = m_pBeam->pev->rendercolor*0.75f + m_pNoise->pev->rendercolor*0.25f;
#ifdef CLIENT_WEAPONS
	if (m_pBeam)
		SetBits(m_pBeam->pev->flags, FL_SKIPLOCALHOST);
	if (m_pNoise)
		SetBits(m_pNoise->pev->flags, FL_SKIPLOCALHOST);
	if (m_pSprite)
		SetBits(m_pSprite->pev->flags, FL_SKIPLOCALHOST);
#endif // CLIENT_WEAPONS

//	if (!FBitSet(m_pSprite->pev->effects, EF_NODRAW))// XDM3035a
//		EMIT_SOUND_DYN(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav", VOL_NORM, ATTN_IDLE, 0, 75);

#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &startPoint - 
//			&endPoint - 
//			timeBlend - 
//-----------------------------------------------------------------------------
void CWeaponEgon::UpdateEffect(const Vector &startPoint, const Vector &endPoint, float timeBlend)
{
#if !defined (CLIENT_DLL)
	if (m_pBeam == NULL || m_pNoise == NULL || m_pSprite == NULL)
		CreateEffect();

	if (m_pBeam)
	{
		m_pBeam->SetStartPos(endPoint);
		UTIL_SetOrigin(m_pBeam, endPoint);// endPoint?
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// traffic
		{
			m_pBeam->SetBrightness(255 - (int)(timeBlend*80.0f));
			//m_pBeam->SetWidth(EGON_SPIRAL_WIDTH - (int)(timeBlend*2.0f));
		}
	}
	if (m_pNoise)
	{
		m_pNoise->SetStartPos(endPoint);
		UTIL_SetOrigin(m_pNoise, endPoint);// this function keeps the beam across BSP leafs
	}
	if (m_pSprite)
	{
		UTIL_SetOrigin(m_pSprite, endPoint);
		if (!FBitSet(m_pSprite->pev->effects, EF_NODRAW))
			m_pSprite->AnimateThink();
		/*m_pSprite->pev->frame += 16.0f * gpGlobals->frametime;
		if (m_pSprite->pev->frame > m_pSprite->Frames())
			m_pSprite->pev->frame = 0.0f;*/
	}
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM3038c
		SetBits(m_pPlayer->pev->effects, EF_MUZZLEFLASH);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::DestroyEffect(void)
{
#if !defined (CLIENT_DLL)
	if (m_pBeam != NULL)
	{
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
			m_pBeam->SetFlags(BEAM_FSHADEOUT);
			m_pBeam->Expand(10, 600);
		}
		else
			UTIL_Remove(m_pBeam);

		m_pBeam = NULL;
	}
	if (m_pNoise != NULL)
	{
		/*if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
			m_pNoise->SetFlags(BEAM_FSHADEOUT);
			m_pNoise->Expand(10, 600);
		}
		else*/
			UTIL_Remove(m_pNoise);

		m_pNoise = NULL;
	}
	if (m_pSprite != NULL)
	{
		//STOP_SOUND(m_pSprite->edict(), CHAN_STREAM, "weapons/xbow_fly1.wav");
		m_pSprite->Expand(10, 500);
		m_pSprite = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::WeaponIdle(void)
{
	if (m_fireState != FIRE_OFF)
		EndAttack();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fireMode == FIRE_SEC)
	{
		SendWeaponAnim(EGON_ALTIDLE);
	}
	else
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 1;
		else
			rmax = 0;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
		if (r == 1)
			SendWeaponAnim(EGON_FIDGET);
		else
			SendWeaponAnim(EGON_IDLE);
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 3.0, 4.0);
	//m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8;
	//m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;// let the player switch the firemode
}

//-----------------------------------------------------------------------------
// Purpose: entity is detached from its former owner
//-----------------------------------------------------------------------------
void CWeaponEgon::OnDetached(void)// XDM3038c
{
	CBasePlayerWeapon::OnDetached();// XDM3038c
	// this makes beams disappear slowly: DestroyEffect();
#if !defined (CLIENT_DLL)
	if (m_pBeam != NULL)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}
	if (m_pNoise != NULL)
	{
		UTIL_Remove(m_pNoise);
		m_pNoise = NULL;
	}
	if (m_pSprite != NULL)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: entity is about to be erased from world and server memory
//-----------------------------------------------------------------------------
void CWeaponEgon::OnFreePrivateData(void)// XDM3037
{
	// NO! Calls to NULLs!	DestroyEffect();
	CBasePlayerWeapon::OnFreePrivateData();
}

//-----------------------------------------------------------------------------
// Purpose: Server-side only. Pack data.
// Warning: Weapon classes must call their parent's functions!
// Input  : *player - receiver
//			*weapondata - pack data into this structure
//-----------------------------------------------------------------------------
void CWeaponEgon::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
}
