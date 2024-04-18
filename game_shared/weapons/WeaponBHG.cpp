#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#if !defined (CLIENT_DLL)
#include "blackball.h"
#endif

#define BHG_SPINUP_ANIM_INTERVAL	1.0f// 30 frames / 30 FPS

enum vanim_bhg_e
{
	BHG_DRAW = 0,
	BHG_IDLE1,
	BHG_IDLE2,
	BHG_SPINUP,
	BHG_SPIN,
	BHG_FIRE,
	BHG_HOLSTER
};

enum vtexgroup_bhg_e
{
	BHG_TG_IDLE = 0,
	BHG_TG_CHARGE,
};

LINK_ENTITY_TO_CLASS(weapon_bhg, CWeaponBHG);

TYPEDESCRIPTION	CWeaponBHG::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponBHG, m_flStartCharge, FIELD_TIME),
	DEFINE_FIELD(CWeaponBHG, m_fireMode, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponBHG, m_fireState, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponBHG, CBasePlayerWeapon);

void CWeaponBHG::Precache(void)
{
	m_iId = WEAPON_BHG;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = BLACKHOLE_DEFAULT_GIVE;
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/p_bhg.mdl");
	if (FStringNull(pev->viewmodel))
		pev->viewmodel = MAKE_STRING("models/v_bhg.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgBHG;

	m_usFire = PRECACHE_EVENT(1, "events/weapons/bhg.sc");

	PRECACHE_SOUND("weapons/bhg_fire.wav");
	PRECACHE_SOUND("weapons/bhg_select.wav");
	PRECACHE_SOUND("weapons/bhg_charge.wav");

	CBasePlayerWeapon::Precache();
}

int CWeaponBHG::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();
	p->iFlags = ITEM_FLAG_NOAUTORELOAD|ITEM_FLAG_SUPERWEAPON;// !!!
	p->iMaxClip = BLACKHOLE_MAX_CLIP;
	p->iWeight = BLACKHOLE_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_BLACKHOLE;
	p->iPosition = POSITION_BLACKHOLE;
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

bool CWeaponBHG::Deploy(void)
{
	m_flStartCharge = 0.0f;
	m_fireState = FIRE_OFF;
	m_iClip = 0;
	m_iSoundState = 0;
	pev->skin = BHG_TG_IDLE;
	return DefaultDeploy(BHG_DRAW, "egon", "weapons/bhg_select.wav");
}

void CWeaponBHG::Holster(int skiplocal)
{
	if (m_fireState != FIRE_OFF)
	{
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bhg_charge.wav");
		Fire();
	}
	pev->skin = BHG_TG_IDLE;
	m_iClip = 0;
	m_fireState = FIRE_OFF;
	//PLAYBACK_EVENT_FULL(FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usFire, 0.01, g_vecZero, g_vecZero, 0.0, 0.0, BHG_HOLSTER, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim(BHG_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

bool CWeaponBHG::CanHolster(void) const
{
	if (m_fireState != FIRE_OFF)
		return false;

	return true;
}

bool CWeaponBHG::IsUseable(void) const
{
	if (m_fireState != FIRE_OFF)// don't put away while firing!
		return true;

	if (!HasAmmo(AMMO_PRIMARY, min(BHG_AMMO_USE_MIN1,BHG_AMMO_USE_MIN2)))// minimum required amount
		return false;

	return CBasePlayerWeapon::IsUseable();
}

int CWeaponBHG::GetCurrentChargeLimit(void)
{
	if (m_fireMode == FIRE_PRI)
		return BHG_AMMO_USE_MAX1;
	else// if (m_fireMode == FIRE_SEC)
		return BHG_AMMO_USE_MAX2;
}

// used for zapping, requires proper m_fireMode
float CWeaponBHG::GetFullChargeTime(void)
{
	return BHG_AMMO_USE_INTERVAL * GetCurrentChargeLimit() + BHG_OVERCHARGE_TIME;
}

// charge automatically up to a fixed amount of ammo and fire, no need to hold any buttons
void CWeaponBHG::PrimaryAttack(void)
{
	if (m_fireState == FIRE_OFF)// was off, start charging
	{
		if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || !HasAmmo(AMMO_PRIMARY, BHG_AMMO_USE_MIN1))
		{
			PlayEmptySound();
			m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + BHG_ATTACK_INTERVAL1;
			return;
		}
		//conprintf(2, "CWeaponBHG::PrimaryAttack(): FIRE_OFF\n");
		m_fireMode = FIRE_PRI;
		ChargeStart();
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_pPlayer->m_flNextAttack = m_flNextAmmoBurn;
	}
	else if (m_fireState == FIRE_CHARGE)// was charging, continue
	{
		//conprintf(2, "CWeaponBHG::PrimaryAttack(): FIRE_CHARGE\n");
		if (m_fireMode == FIRE_PRI)
			ChargeUpdate();

		m_flTimeWeaponIdle = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + BHG_AMMO_USE_INTERVAL/2;
	}
}

// charge manually while holding the button, fire on release or overcharge
void CWeaponBHG::SecondaryAttack(void)
{
	if (m_fireState != FIRE_OFF)
	{
		if (m_fireMode == FIRE_PRI)
			return;

		if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
		{
			Fail();
			return;
		}
	}
	if (m_fireState == FIRE_OFF)// was off, start charging
	{
		if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || !HasAmmo(AMMO_PRIMARY, BHG_AMMO_USE_MIN2))
		{
			PlayEmptySound();
			m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + BHG_ATTACK_INTERVAL2;
			return;
		}
		m_fireMode = FIRE_SEC;
		ChargeStart();
		m_flTimeWeaponIdle += BHG_AMMO_USE_INTERVAL;// delay a little bit more so the IN_ATTACK2 won't be released accidentally
		m_pPlayer->m_flNextAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase();
	}
	else if (m_fireState == FIRE_CHARGE)// was charging, continue
	{
		if (m_fireMode == FIRE_SEC)
		{
			ChargeUpdate();
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + BHG_AMMO_USE_INTERVAL;// delay a little bit more so the IN_ATTACK2 won't be released accidentally
		}
	}
}

void CWeaponBHG::ChargeStart(void)
{
	//conprintf(2, "CWeaponBHG::ChargeStart(%d)\n", m_fireMode);
	m_fireState = FIRE_CHARGE;
	m_iClip = 0;
	m_iSoundState = 0;
	m_pPlayer->m_iWeaponVolume = BHG_CHARGE_VOLUME;
	pev->skin = BHG_TG_CHARGE;
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	//SendWeaponAnim(BHG_SPINUP);// DEBUG ONLY
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, GetFullChargeTime(), 0.0, BHG_SPINUP, g_iModelIndexLaser, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bhg_charge.wav", VOL_NORM, ATTN_NORM, m_iSoundState, 101);// HLBUG: pitch won't change if sound started with PITCH_NORM!
	m_iSoundState = SND_CHANGE_PITCH;
	m_flStartCharge = UTIL_WeaponTimeBase();
	m_flNextAmmoBurn = m_flStartCharge;
	m_flPumpTime = m_flStartCharge + BHG_SPINUP_ANIM_INTERVAL;
	m_flTimeWeaponIdle = m_flNextAmmoBurn = m_flStartCharge + BHG_AMMO_USE_INTERVAL;
}

void CWeaponBHG::ChargeUpdate(void)
{
	//conprintf(2, "CWeaponBHG::ChargeUpdate(%d)\n", m_fireMode);
	if (m_pPlayer && m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)// got into water
	{
		Fail();
		return;
	}

	// primary mode fires automatically
	if (m_fireMode == FIRE_PRI && m_iClip >= BHG_AMMO_USE_MAX1)//(pev->teleport_time < UTIL_WeaponTimeBase())// fully charged
	{
		Fire();
		return;
	}

	// during the charging process, eat one bit of ammo every once in a while
	if (m_flNextAmmoBurn != 0.0f && m_flNextAmmoBurn <= UTIL_WeaponTimeBase())
	{
		if (UseAmmo(AMMO_PRIMARY, BHG_AMMO_USE) == 0)
		{
			STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bhg_charge.wav");
			Fire();
			return;
		}
		m_iClip += BHG_AMMO_USE;
		m_flNextAmmoBurn = UTIL_WeaponTimeBase() + BHG_AMMO_USE_INTERVAL;
	}

	// don't eat any more ammo after gun is fully charged.
	if (m_iClip >= GetCurrentChargeLimit())//if (UTIL_WeaponTimeBase() - m_flStartCharge > GetFullChargeTime())
		m_flNextAmmoBurn = 0.0f;

	// overcharget for twice as long
	if (UTIL_WeaponTimeBase()/*gpGlobals->time*/ > m_flStartCharge + GetFullChargeTime()*2.0f)
	{
		Fail();
		return;
	}
	// when "spinup" animation is finished, play looping "spin" animation
	if (m_flPumpTime > 0.0f && m_flPumpTime <= UTIL_WeaponTimeBase())
	{
		SendWeaponAnim(BHG_SPIN);
		m_flPumpTime = 0.0f;
	}
	//int pitch = min(250, PITCH_NORM + (int)((UTIL_WeaponTimeBase()/*gpGlobals->time*/ - m_flStartCharge) * (BHG_MAX_PITCH/GetFullChargeTime())));
	int pitch = PITCH_NORM + ((BHG_MAX_PITCH-PITCH_NORM)*m_iClip)/GetCurrentChargeLimit();
	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bhg_charge.wav", VOL_NORM, ATTN_NORM, m_iSoundState, pitch);
	m_iSoundState = SND_CHANGE_PITCH;// hack for going through level transitions
	m_pPlayer->m_iWeaponVolume = BHG_CHARGE_VOLUME;
	pev->skin = BHG_TG_CHARGE;
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
}

void CWeaponBHG::Fire(void)
{
	//conprintf(2, "CWeaponBHG::Fire(%d)\n", m_fireMode);
	if (m_pPlayer == NULL)
		return;
	m_flPumpTime = 0.0f;
	m_flStartCharge = 0.0f;
	m_iSoundState = 0;
	m_fireState = FIRE_ON;
	if (m_iClip == 0)
	{
		Fail();
		return;
	}
	pev->skin = BHG_TG_IDLE;
	// cl STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bhg_charge.wav");
	//float k = min(1.0f, (UTIL_WeaponTimeBase() - m_flStartCharge)/GetFullChargeTime());
	float k = (float)m_iClip/max(BHG_AMMO_USE_MAX1,BHG_AMMO_USE_MAX2);// WRONG! (float)GetCurrentChargeLimit();// calculate damage multiplier based on actual ammount of used ammo, must be (0...1]
	float fDamage = pev->dmg*k;// maximum is 10x
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, k, fDamage, BHG_FIRE, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
#if !defined (CLIENT_DLL)
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward*GetBarrelLength() + gpGlobals->v_right*1.0f - gpGlobals->v_up*4.0f;
	Vector vecDir(gpGlobals->v_forward);
	CBlackBall::CreateBlackBall(GetDamageAttacker(), GetDamageAttacker(), vecSrc, vecDir, BLACKBALL_FLY_SPEED*k, BLACKBALL_MAX_LIFE*k, k, fDamage);
	UTIL_ScreenShakeOne(m_pPlayer, pev->origin, 4.0, 6.0f, 2.0);
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, fDamage, (m_fireMode == FIRE_PRI)?BHG_ATTACK_INTERVAL1:BHG_ATTACK_INTERVAL2);
#endif
	m_pPlayer->m_iWeaponVolume = BHG_FIRE_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(((m_fireMode == FIRE_PRI)?BHG_ATTACK_INTERVAL1:(BHG_ATTACK_INTERVAL2+k)));
	m_iClip = 0;
	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;// play idle animation right away
}

void CWeaponBHG::Fail(void)
{
	//conprintf(2, "CWeaponBHG::Fail(%d)\n", m_fireMode);
	m_flPumpTime = 0.0f;
	m_flStartCharge = 0.0f;
	m_iSoundState = 0;
	m_fireState = FIRE_OFF;
	pev->skin = BHG_TG_IDLE;
	//pev->teleport_time = -1;
	STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bhg_charge.wav");
	m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + ((m_fireMode == FIRE_PRI)?BHG_ATTACK_INTERVAL1:BHG_ATTACK_INTERVAL2);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;// play idle animation right away
	if (m_iClip > 0)
	{
		m_iClip = 0;
		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/electro5.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(80,143));
#if !defined (CLIENT_DLL)
		if (m_pPlayer->pev->waterlevel < WATERLEVEL_WAIST && g_pGameRules->FAllowEffects())// got into water
		{
			UTIL_ScreenShake(pev->origin, 3.0, 5.0f, 2.0, 100);
			EntityLight(entindex(), pev->origin, 200, 255,255,255, 10, 256);
		}
		RadiusDamage(pev->origin, this, GetDamageAttacker(), gSkillData.DmgBHG*2.0f, 80, CLASS_NONE, DMG_SHOCK|DMG_DISINTEGRATING);
		// Player may have been killed and this weapon dropped, don't execute any more code after this!
		if (m_pPlayer)// player killed self
			UTIL_ScreenFade(m_pPlayer, Vector(255,255,255), 2, 0.5, 128, FFADE_IN);
#endif
	}
	else
		PlayEmptySound();
}

void CWeaponBHG::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fireState == FIRE_CHARGE)
	{
		if (m_fireMode == FIRE_PRI)// charging automatically
		{
			ChargeUpdate();
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + BHG_AMMO_USE_INTERVAL;// don't bother calling WeaponIdle() too soon
		}
		else //if (m_fireMode == FIRE_SEC)// player released the button
			Fire();
	}
	else
	{
		if (UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,1) == 0)
			SendWeaponAnim(BHG_IDLE1, pev->body);
		else
			SendWeaponAnim(BHG_IDLE2, pev->body);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 3, 6);
	}
}

void CWeaponBHG::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
}
