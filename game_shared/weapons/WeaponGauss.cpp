#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#include "color.h"// XDM3037a

enum vanim_gauss_e
{
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

LINK_ENTITY_TO_CLASS(weapon_gauss, CWeaponGauss);

TYPEDESCRIPTION	CWeaponGauss::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponGauss, m_flStartCharge, FIELD_TIME),
	DEFINE_FIELD(CWeaponGauss, m_flPlayAftershock, FIELD_TIME),
	DEFINE_FIELD(CWeaponGauss, m_fInAttack, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponGauss, m_fireMode, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponGauss, CBasePlayerWeapon);

//-----------------------------------------------------------------------------
// Purpose: Time required to fully charge this weapon
// Output : time
//-----------------------------------------------------------------------------
float CWeaponGauss::GetFullChargeTime(void)
{
	if (IsMultiplayer())
		return GAUSS_CHARGE_TIME1;

	return GAUSS_CHARGE_TIME2;
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponGauss::Precache(void)
{
	m_iId = WEAPON_GAUSS;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = GAUSS_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_gauss.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_gauss.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgGauss;

	m_iBalls = PRECACHE_MODEL("sprites/hotglow.spr");
	m_iBeam = PRECACHE_MODEL("sprites/gaussbeam.spr");

	m_usFire = PRECACHE_EVENT(1, "events/weapons/gauss.sc");
	//m_usGaussSpin = PRECACHE_EVENT(1, "events/weapons/gaussspin.sc");

	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("weapons/electro5.wav");
	PRECACHE_SOUND("weapons/electro6.wav");
	PRECACHE_SOUND("weapons/gauss_fire1.wav");
	PRECACHE_SOUND("weapons/gauss_fire2.wav");
	PRECACHE_SOUND("weapons/gauss_hit.wav");
	PRECACHE_SOUND("weapons/gauss_refl1.wav");
	PRECACHE_SOUND("weapons/gauss_refl2.wav");
	PRECACHE_SOUND("weapons/gauss_select.wav");
	PRECACHE_SOUND("weapons/gauss_spin.wav");

	CBasePlayerWeapon::Precache();// XDM3038
}

//-----------------------------------------------------------------------------
// Purpose: Fill provided structure with weapon info
// Input  : *p - 
// Output : Returns 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
int CWeaponGauss::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_GAUSS;
	p->iFlags = 0;
	p->iMaxClip = GAUSS_MAX_CLIP;
	p->iWeight = GAUSS_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_GAUSS;
	p->iPosition = POSITION_GAUSS;
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
// Purpose: Deploy the weapon
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGauss::Deploy(void)
{
	m_flStartCharge = 0.0f;
	m_flPlayAftershock = 0.0f;
	m_fInAttack = 0;
	m_iSoundState = 0;
	// XDM3037a: cache colors
#if !defined (CLIENT_DLL)
	if (!StringToColor(sv_tau_pri_rgb.string, m_BeamColor1))
#endif
	{
		m_BeamColor1.Set(GAUSS_DEFAULT_COLOR1_R, GAUSS_DEFAULT_COLOR1_G, GAUSS_DEFAULT_COLOR1_B);
	}
#if !defined (CLIENT_DLL)
	if (!StringToColor(sv_tau_sec_rgb.string, m_BeamColor2))
#endif
	{
		m_BeamColor2.Set(GAUSS_DEFAULT_COLOR2_R, GAUSS_DEFAULT_COLOR2_G, GAUSS_DEFAULT_COLOR2_B);
	}
	return DefaultDeploy(GAUSS_DRAW, "gauss", "weapons/gauss_select.wav");
}

void CWeaponGauss::Holster(int skiplocal /* = 0 */)
{
	if (m_fInAttack > 0)// XDM
	{
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_spin.wav");// XDM3035: all ENTs changed to player
		StartFire();
	}
	//PLAYBACK_EVENT_FULL(FEV_GLOBAL, m_pPlayer->edict(), m_usFire, 0.01, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 1);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3f;// XDM3038
	SendWeaponAnim(GAUSS_HOLSTER);
	m_fInAttack = 0;
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGauss::CanHolster(void) const
{
	return (m_fInAttack == 0);
}

//-----------------------------------------------------------------------------
// Purpose: Rapid fire mode, no charging
//-----------------------------------------------------------------------------
void CWeaponGauss::PrimaryAttack(void)
{
	if (m_fInAttack > 0)// XDM3035: don't interrupt secondary attack charging process
	{
		SecondaryAttack();// no cheating!
		return;
	}

	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || UseAmmo(AMMO_PRIMARY, GAUSS_AMMO_USE1) != GAUSS_AMMO_USE1)//m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) < GAUSS_AMMO_USE1)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_pPlayer->m_flNextAttack = GetNextAttackDelay(0.5);
	}
	else
	{
		m_fireMode = FIRE_PRI;
		StartFire();
		m_pPlayer->m_iWeaponVolume = GAUSS_FIRE_VOLUME;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start/continue charging
//-----------------------------------------------------------------------------
void CWeaponGauss::SecondaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		if (m_fInAttack > 0)
		{
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/electro4.wav", VOL_NORM, ATTN_NORM, 0, 80 + RANDOM_LONG(0,0x3f));
#if !defined (CLIENT_DLL) // XDM
			DBG_PRINTF("CWeaponGauss::SecondaryAttack(): RadiusDamage() in water\n");
			RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg*2.0f, pev->dmg*100.0f, CLASS_NONE, DMG_SHOCK | DMG_NEVERGIB);
			if (m_pPlayer)
				UTIL_ScreenFade(m_pPlayer, Vector(159,159,255), 2, 0.5, 128, FFADE_IN);
#endif
			SendWeaponAnim(GAUSS_IDLE);
			m_fInAttack = 0;
		}
		else
			PlayEmptySound();

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	if (m_fInAttack == 0)
	{
		if (!HasAmmo(AMMO_PRIMARY, GAUSS_AMMO_USE2))
		{
			PlayEmptySound();
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
			return;
		}
		m_fireMode = FIRE_SEC;
		m_pPlayer->m_iWeaponVolume = GAUSS_CHARGE_VOLUME;
		SendWeaponAnim(GAUSS_SPINUP);
		UseAmmo(AMMO_PRIMARY, GAUSS_AMMO_USE2);// take one ammo just to start the spin
		m_fInAttack = 1;
		//PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 110, 0, 0, 0);
		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_spin.wav", VOL_NORM, ATTN_NORM, 0, 102);// HLBUG: pitch won't change if sound started with PITCH_NORM!
		m_iSoundState = SND_CHANGE_PITCH;
		m_flNextAmmoBurn = UTIL_WeaponTimeBase();
		m_flStartCharge = UTIL_WeaponTimeBase();//gpGlobals->time;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
	}
	else if (m_fInAttack == 1)
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		{
			SendWeaponAnim(GAUSS_SPIN);
			m_fInAttack = 2;
		}
	}
	else// if (m_fInAttack > 1)
	{
		if (!HasAmmo(AMMO_PRIMARY))
		{
			STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_spin.wav");
			// out of ammo! force the gun to fire
			StartFire();
			return;
		}
		// during the charging process, eat one bit of ammo every once in a while
		if (m_flNextAmmoBurn != 0.0f && m_flNextAmmoBurn <= UTIL_WeaponTimeBase())
		{
			UseAmmo(AMMO_PRIMARY, 1);
			if (IsMultiplayer())
				m_flNextAmmoBurn = UTIL_WeaponTimeBase() + GAUSS_AMMO_USE_INTERVAL;
			else
				m_flNextAmmoBurn = UTIL_WeaponTimeBase() + GAUSS_AMMO_USE_INTERVAL_SP;
		}
		// don't eat any more ammo after gun is fully charged.
		if (UTIL_WeaponTimeBase() - m_flStartCharge > GetFullChargeTime())
			m_flNextAmmoBurn = 0.0f;

		int pitch = min(250, PITCH_NORM + (int)((UTIL_WeaponTimeBase()/*gpGlobals->time*/ - m_flStartCharge) * (GAUSS_MAX_PITCH/GetFullChargeTime())));
		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_spin.wav", VOL_NORM, ATTN_NORM, m_iSoundState, pitch);
		m_iSoundState = SND_CHANGE_PITCH;// hack for going through level transitions
		m_pPlayer->m_iWeaponVolume = GAUSS_CHARGE_VOLUME;
		//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;

		if (m_flStartCharge < UTIL_WeaponTimeBase()/*gpGlobals->time*/ - GAUSS_SHOCK_TIME)
		{
			// Player charged up too long. Zap him.
			m_fInAttack = 0;
			STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_spin.wav");
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/electro5.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(80,143));
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + GAUSS_ATTACK_INTERVAL2;
#if !defined (CLIENT_DLL)
			UTIL_ScreenFade(m_pPlayer, Vector(255,255,255), 2, 0.5, 128, FFADE_IN);
			DBG_PRINTF("CWeaponGauss::SecondaryAttack(): RadiusDamage() overcharge\n");
			RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg*2.0f, 80, CLASS_NONE, DMG_SHOCK);
			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			UTIL_ScreenShake(pev->origin, 3, 5, 2, 100);
			EntityLight(entindex(), pev->origin, 200, m_BeamColor2.r, m_BeamColor2.g, m_BeamColor2.b, 10, 256);
#endif
			// better without? SendWeaponAnim(GAUSS_IDLE);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Intermediate fire function, common code for all fire modes
//-----------------------------------------------------------------------------
void CWeaponGauss::StartFire(void)
{
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	m_iSoundState = 0;
	uint32 color_int;
	float flDamage;
	if (m_fireMode == FIRE_PRI)
	{
		flDamage = pev->dmg;
		m_iBeamAlpha = 255;
		color_int = m_BeamColor1;
		m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(GAUSS_ATTACK_INTERVAL1);
	}
	else
	{
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_spin.wav");// 3035? Wtf? 3030 moved
		float k = min(1.0f, (UTIL_WeaponTimeBase() - m_flStartCharge)/GetFullChargeTime());
		flDamage = pev->dmg*10.0f*k;// maximum is 10x
		m_iBeamAlpha = (int)(k*255.0f);
		color_int = m_BeamColor2;
		//conprintf(1, "k = %f, m_iBeamAlpha = %d\n", k, m_iBeamAlpha);
#if !defined (CLIENT_DLL)
		if (!IsMultiplayer() || mp_allowgaussjump.value <= 0)
			m_pPlayer->pev->velocity -= gpGlobals->v_forward * (flDamage * 2.0f);
		else
			m_pPlayer->pev->velocity -= gpGlobals->v_forward * (flDamage * sv_tau_velocity_mult.value);
#endif
		m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(GAUSS_ATTACK_INTERVAL2);
	}
	m_pPlayer->m_iWeaponVolume = GAUSS_FIRE_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_fInAttack = 0;// XDM

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	// XDM3038a: TESTME: HACK: fparam: 18 bits, iparam: 16 bits, have to transfer 24 bits of color...
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, ((uint16)(color_int & 0xFFFF)), ((uint16)((color_int>>16) & 0xFFFF)), GAUSS_FIRE2, 0, pev->body, m_fireMode);

	Fire(m_pPlayer->GetGunPosition() + gpGlobals->v_forward*GetBarrelLength(), gpGlobals->v_forward, flDamage);// + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;
	if (m_pPlayer == NULL)// player killed self
		return;

#if !defined (CLIENT_DLL)
	if (!IsMultiplayer())
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, flDamage, 0.4);// XDM
#endif // XDM
	// time until aftershock 'static discharge' sound
	m_flPlayAftershock = gpGlobals->time + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.6, 0.8);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: The actual fire code, very complicated!
// BUGBUG: is a player is shooting another player from a very close distance (crouching together), he will shoot self in 2nd iteration (because of box intersection/overlapping)
// Warning: m_pPlayer may become NULL any moment here!
// Input  : *p - FIRE_PRI
// Output : Returns 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
void CWeaponGauss::Fire(const Vector &vecOrigSrc, const Vector &vecFireDir, float flDamage)
{
#if !defined (CLIENT_DLL)
	CBaseEntity *pEntity;
	edict_t *pentIgnore = m_pPlayer->edict();
	float flSavedDamage = flDamage;
	//not used float flMaxFrac = 1.0f;
	float n;
	int	nMaxHits = (int)sv_tau_max_reflections.value;// XDM3038c
	if (nMaxHits < 0 || nMaxHits > 32)
		nMaxHits = GAUSS_BEAM_MAX_HITS;

	Vector vecSrc(vecOrigSrc);
	Vector vecDir(vecFireDir);
	Vector vecDest(vecDir);
	vecDest *= GAUSS_BEAM_MAX_DIST;
	vecDest += vecSrc;
	TraceResult tr, beam_tr;
	bool fFirstBeam = true;
	bool fHasPunched = false;
	bool fGoThrough = false;
	///conprintf(1, "CWeaponGauss::Fire(mode: %d, nMaxHits: %d, flDamage: %g)\n", m_fireMode, nMaxHits, flDamage);

	while (flDamage > 10 && nMaxHits >= 0)
	{
		///ASSERT(UTIL_IsValidEntity(pentIgnore));//ASSERT(pentIgnore->free == 0);
		nMaxHits--;
		/* this does not cause the crash if (pentIgnore == NULL)
		{
			DBG_PRINTF("CWeaponGauss::Fire(): replacing NULL with THIS\n");
			pentIgnore = edict();
		}*/
		//memset(&tr, 0, sizeof(TraceResult));
		///DBG_PRINTF("CWeaponGauss::Fire(): %d starting trace(%g %g %g, %g %g %g, im %d, ig %d, ei %d)\n", nMaxHits, vecSrc.x,vecSrc.y,vecSrc.z, vecDest.x,vecDest.y,vecDest.z, dont_ignore_monsters, ignore_glass, pentIgnore?ENTINDEX(pentIgnore):0);
		try
		{
			UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, ignore_glass, pentIgnore, &tr);// XDM: just ignore glass for now because it will be difficult to rewrite double-trace code and synchronize it with client-side code.
		}
		catch(...)
		{
			DBG_PRINTF("CWeaponGauss::Fire(): %d TraceLine exception!!!\n", nMaxHits);
		}
		//DBG_PRINTF("CWeaponGauss::Fire(): %d end trace\n", nMaxHits);

		if (tr.fAllSolid)
			break;

		//UTIL_DebugBeam(vecSrc, tr.vecEndPos, 8.0f+(GAUSS_BEAM_MAX_HITS-nMaxHits));
		pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity == NULL)
		{
			///conprintf(1, "CWeaponGauss::Fire(%d): trace NULL\n", nMaxHits);
			break;
		}
		else if (pEntity->IsRemoving())
		{
			///conprintf(1, "CWeaponGauss::Fire(%d): skipping removed %d %s %s\n", nMaxHits, pEntity->entindex(), STRING(pEntity->pev->classname), STRING(pEntity->pev->netname));
			continue;
		}
		///else
			///conprintf(1, "CWeaponGauss::Fire(%d): trace %d %s %s\n", nMaxHits, pEntity->entindex(), STRING(pEntity->pev->classname), STRING(pEntity->pev->netname));

		// Don't draw beam until the damn thing looks like it's coming out of the barrel
		//MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		//MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, (vecSrc + tr.vecEndPos)/2);
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSrc);
		if (fFirstBeam)
		{
			fFirstBeam = FALSE;
			WRITE_BYTE(TE_BEAMENTPOINT);
			WRITE_SHORT(m_pPlayer->entindex() + 0x1000);
		}
		else
		{
			WRITE_BYTE(TE_BEAMPOINTS);
			WRITE_COORD(vecSrc.x);
			WRITE_COORD(vecSrc.y);
			WRITE_COORD(vecSrc.z);
		}
			WRITE_COORD(tr.vecEndPos.x);
			WRITE_COORD(tr.vecEndPos.y);
			WRITE_COORD(tr.vecEndPos.z);
			WRITE_SHORT(m_iBeam);
			WRITE_BYTE(0);// Startframe
			WRITE_BYTE(20);// Framerate
			WRITE_BYTE(1);// Life
		if (m_fireMode == FIRE_PRI)
		{
			WRITE_BYTE(GAUSS_BEAM_WIDTH1);
		if (sv_overdrive.value > 0.0f)
			WRITE_BYTE(RANDOM_LONG(10, 20));
		else
			WRITE_BYTE(0);

			WRITE_BYTE(m_BeamColor1.r);
			WRITE_BYTE(m_BeamColor1.g);
			WRITE_BYTE(m_BeamColor1.b);
		}
		else
		{
			WRITE_BYTE(GAUSS_BEAM_WIDTH2);
		if (sv_overdrive.value > 0.0f)
			WRITE_BYTE(RANDOM_LONG(20, 30));
		else
			WRITE_BYTE(0);

			WRITE_BYTE(m_BeamColor2.r);// XDM3038a: typo
			WRITE_BYTE(m_BeamColor2.g);
			WRITE_BYTE(m_BeamColor2.b);
		}
			WRITE_BYTE(m_iBeamAlpha);
			WRITE_BYTE(100);// Speed
		MESSAGE_END();

		if (pEntity->IsBSPModel())// XDM3035: don't reflect or do any effects if beam hits the sky
		{
			const char *tex = NULL;// runtime libraries fault protection
			tex = TRACE_TEXTURE(tr.pHit, vecSrc, vecDest);
			if (tex && _stricmp(tex, "sky") == 0)// Texture name can be 'sky' or 'SKY' so use strIcmp!
				break;
		}

		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			///conprintf(1, "CWeaponGauss::Fire(%d): hit %d %s\n", nMaxHits, pEntity->entindex(), STRING(pEntity->pev->classname));
			ClearMultiDamage();
			pEntity->TraceAttack(GetDamageAttacker(), flDamage, vecDir, &tr, DMG_BULLET | DMG_ENERGYBEAM | DMG_NEVERGIB);
			ApplyMultiDamage(this, GetDamageAttacker());
			if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM3037
				UTIL_Sparks(tr.vecEndPos);
/*#if defined (_DEBUG)
			if (pEntity->IsRemoving())
			{
				conprintf(1, "CWeaponGauss::Fire(%d): hit %d %s destroyed.\n", nMaxHits, pEntity->entindex(), STRING(pEntity->pev->classname));
				// NO! this will draw next beam over this one! continue;
			}
#endif*/
		}

		if (!pEntity->IsRemoving() && pEntity->IsBSPModel() && pEntity->pev->takedamage == DAMAGE_NO)
		{
			pentIgnore = NULL;
			n = -DotProduct(tr.vecPlaneNormal, vecDir);
			if (n < 0.5f) // 60 degrees
			{
				///conprintf(1, "CWeaponGauss::Fire(%d): reflect\n", nMaxHits);
				// reflect
				//Vector r = vecDir + tr.vecPlaneNormal *2.0f*n;
				//not used flMaxFrac -= tr.flFraction;
				//vecDir = r;
				vecDir += tr.vecPlaneNormal *2.0f*n;//= r;
				vecSrc = vecDir; vecSrc *= 8.0f; vecSrc += tr.vecEndPos;//vecSrc = tr.vecEndPos + vecDir * 8.0f;
				vecDest = vecDir; vecDest *= g_psv_zmax->value; vecDest += vecSrc;//vecDest = vecSrc + vecDir * g_psv_zmax->value;
				// explode a bit
				///DBG_PRINTF("CWeaponGauss::Fire(): RadiusDamage() reflection\n");
				RadiusDamage(tr.vecEndPos, this, GetDamageAttacker(), flDamage*n, flDamage*n*2, CLASS_NONE, DMG_BLAST | DMG_NEVERGIB);
				///DBG_PRINTF("CWeaponGauss::Fire(): PlaybackEvent()  reflection\n");
				PLAYBACK_EVENT_FULL(FEV_RELIABLE, NULL, g_usBeamImpact, 0.0, tr.vecEndPos, tr.vecPlaneNormal.As3f(), flDamage*n, 0.0, m_iBalls, m_iGlow, m_fireMode, GAUSS_EV_REFLECT);
				// lose energy
				if (n == 0) n = 0.1f;

				flDamage *= (1.0f - n);
				///DBG_PRINTF("CWeaponGauss::Fire(): reflection end\n");
			}
			else
			{
				///conprintf(1, "CWeaponGauss::Fire(%d): straight, fHasPunched: %d\n", nMaxHits, fHasPunched);
				// CL: entry effects
				if (fHasPunched)
				{
					PLAYBACK_EVENT_FULL(FEV_RELIABLE, NULL, g_usBeamImpact, 0.0, tr.vecEndPos.As3f(), vecDir.As3f(), flDamage, 0.0, m_iBalls, m_iGlow, m_fireMode, GAUSS_EV_EXPLODE);
					break;
				}

				fHasPunched = true;
				fGoThrough = false;
				flSavedDamage = flDamage;// XDM

				// Slug doesn't punch through ever with primary fire, so leave a little glowy bit and make some balls
				if (m_fireMode == FIRE_SEC)// try punching through wall if secondary attack (primary is incapable of breaking through)
				{
					// does not help memset(&beam_tr, 0, sizeof(TraceResult));
					UTIL_TraceLine(tr.vecEndPos + vecDir*GAUSS_PUNCH_DEPTH, vecDest, dont_ignore_monsters, ignore_glass, pentIgnore, &beam_tr);
					if (!beam_tr.fAllSolid)
					{
						///conprintf(1, "CWeaponGauss::Fire(%d): enter\n", nMaxHits);
						// trace backwards to find exit point
						// NO! memset(&beam_tr, 0, sizeof(TraceResult));
						UTIL_TraceLine(beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &beam_tr);
						vec_t l = (beam_tr.vecEndPos - tr.vecEndPos).Length();
						if (l < flDamage)
						{
							///conprintf(1, "CWeaponGauss::Fire(%d): found exit, l: %g\n", nMaxHits, l);
							if (l == 0) l = 1.0f;
							flDamage -= l;
							//Vector vecOrg = beam_tr.vecEndPos + vecDir * 8.0f;
							vecSrc = beam_tr.vecEndPos + vecDir;
							///DBG_PRINTF("CWeaponGauss::SecondaryAttack(): RadiusDamage() through wall\n");
							RadiusDamage(vecSrc + vecDir*6.0f, this, GetDamageAttacker(), flDamage, flDamage*2.0f, CLASS_NONE, DMG_BLAST | DMG_NEVERGIB);// exit blast damage
							fGoThrough = true;
							CSoundEnt::InsertSound(bits_SOUND_COMBAT, vecSrc, NORMAL_EXPLOSION_VOLUME, 1.0);
						}
						else
							flDamage = 0.0f;
					}
					else
					{
						///conprintf(1, "CWeaponGauss::Fire(%d): beam_tr.fAllSolid\n", nMaxHits);
						flDamage = 0.0f;
					}
				}
				PLAYBACK_EVENT_FULL(FEV_RELIABLE, NULL, g_usBeamImpact, 0.0f, tr.vecEndPos, (fGoThrough?vecSrc.As3f():vecDir.As3f()), flSavedDamage, (fGoThrough?GAUSS_PUNCH_DEPTH:0), m_iBalls, m_iGlow, m_fireMode, GAUSS_EV_EXPLODE);
				if (fGoThrough && (g_pGameRules == NULL || g_pGameRules->FAllowEffects())) UTIL_ScreenShake(tr.vecEndPos, 3.0, 4.0, 1.5, flDamage*DAMAGE_TO_RADIUS_DEFAULT);
				if (m_fireMode == FIRE_PRI)
					break;
			}
			///DBG_PRINTF("CWeaponGauss::Fire(): 4\n");
		}
		else// if (IsBSPModel() && pEntity->pev->takedamage == DAMAGE_NO)
		{
			///conprintf(1, "CWeaponGauss::Fire(%d): through %d %s\n", nMaxHits, pEntity->entindex(), STRING(pEntity->pev->classname));
			vecSrc = tr.vecEndPos;// + vecDir;
			vecSrc += vecDir;
			if (!pEntity->IsRemoving())
				pentIgnore = pEntity->edict();
		}
		///DBG_PRINTF("CWeaponGauss::Fire(): next iteration\n");
	}
	if (GetHost() == NULL)
	{
		conprintf(1, "CWeaponGauss(%d)::Fire(%d): no owner!! Probably killed self in Fire()!\n", entindex(), m_fireMode);
		//DBG_FORCEBREAK
		return;
	}
	///conprintf(1, "CWeaponGauss::Fire(%d): END\n", nMaxHits);
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Nothing is pressed, play aftershock, release charge
//-----------------------------------------------------------------------------
void CWeaponGauss::WeaponIdle(void)
{
	// play aftershock static discharge
	if (m_flPlayAftershock > 0.0f && m_flPlayAftershock < gpGlobals->time)
	{
		int flags = 0;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 3);
		m_flPlayAftershock = 0.0f;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fInAttack != 0)
	{
		StartFire();
		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	}
	else
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 2;
		else
			rmax = 1;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
		if (r == 2)
			SendWeaponAnim(GAUSS_FIDGET, pev->body);
		else if (r == 1)
			SendWeaponAnim(GAUSS_IDLE2, pev->body);
		else // if (r == 0)
			SendWeaponAnim(GAUSS_IDLE, pev->body);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 6, 10);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server-side only. Pack data.
// Warning: Derived weapon classes MUST CALL this!
// Input  : *player - receiver
//			*weapondata - pack data into this structure
//-----------------------------------------------------------------------------
void CWeaponGauss::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fInAttack;
}
