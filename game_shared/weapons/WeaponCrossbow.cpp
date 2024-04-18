#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "globals.h"
#if !defined (CLIENT_DLL)
#include "crossbowbolt.h"// XDM
#endif

enum vanim_crossbow_e
{
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2,	// empty
};

LINK_ENTITY_TO_CLASS(weapon_crossbow, CWeaponCrossbow);

static char g_ViewModelCrossbow[] = "models/v_crossbow.mdl";// XDM: for MAKE_STRING

TYPEDESCRIPTION	CWeaponCrossbow::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponCrossbow, m_iInZoom, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponCrossbow, CBasePlayerWeapon);

void CWeaponCrossbow::Precache(void)
{
	m_iId = WEAPON_CROSSBOW;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_crossbow.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING(g_ViewModelCrossbow);
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgBolt;

	m_iZoomSound = PRECACHE_SOUND("weapons/xbow_aim.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");
	PRECACHE_SOUND("weapons/xbow_select.wav");
	m_iZoomCrosshair = PRECACHE_MODEL("sprites/c_zoom1.spr");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/crossbow.sc");
	m_usZC = PRECACHE_EVENT(1, "events/weapons/zoomcrosshair.sc");
	//UTIL_PrecacheOther("bolt");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponCrossbow::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iWeight = CROSSBOW_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_CROSSBOW;
	p->iPosition = POSITION_CROSSBOW;
#endif
	p->pszAmmo1 = "bolts";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponCrossbow::Deploy(void)
{
	m_iInZoom = ZOOM_OFF;
	if (m_pPlayer)
		m_pPlayer->pev->fov = 0;// XDM3037a
	//m_pPlayer->m_iFOV = 90;// normal fov

	return DefaultDeploy(((m_iClip > 0)?CROSSBOW_DRAW1:CROSSBOW_DRAW2), "bow", "weapons/xbow_select.wav");
}

void CWeaponCrossbow::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.
	OnDetached();// <- reset zoom
	if (m_pPlayer)
	{
		if (m_iClip > 0)
			SendWeaponAnim(CROSSBOW_HOLSTER1);
		else
			SendWeaponAnim(CROSSBOW_HOLSTER2);

		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 8, 12);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponCrossbow::PrimaryAttack(void)
{
	FireBolt();
	if (m_iInZoom == ZOOM_INCREASING)
		m_iInZoom = ZOOM_ON;// stop zooming
}

void CWeaponCrossbow::FireBolt(void)
{
	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}
	if (UseAmmo(AMMO_CLIP, 1) == 0)
		return;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	// XDM: event only plays sounds and animations
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, (m_iClip > 0)?CROSSBOW_FIRE1:CROSSBOW_FIRE2, 0, pev->body, m_iInZoom);

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	CCrossbowBolt::BoltCreate(m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 10 - gpGlobals->v_up * 2, vecAng, gpGlobals->v_forward, GetDamageAttacker(), GetDamageAttacker(), pev->dmg, m_iInZoom != ZOOM_OFF);
#endif

	if (m_iClip == 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;

	m_flNextPrimaryAttack = GetNextAttackDelay(CROSSBOW_ATTACK_INTERVAL1);
}

void CWeaponCrossbow::SecondaryAttack(void)
{
	int flags = 0;
#if defined(CLIENT_WEAPONS)
	flags = FEV_CLIENT;
#else
	flags = FEV_HOSTONLY;
#endif
	if (m_iInZoom == ZOOM_OFF)// was off, start zooming
	{
		m_iInZoom = ZOOM_INCREASING;
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usZC, 0.0, g_vecZero, g_vecZero, CROSSBOW_MAX_ZOOM_FOV, 0, m_iZoomCrosshair, m_iZoomSound, m_iInZoom, kRenderTransColor);
		m_pPlayer->pev->viewmodel = iStringNull;
		m_pPlayer->ResetAutoaim();
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/xbow_aim.wav", VOL_NORM, ATTN_NORM);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + CROSSBOW_ATTACK_INTERVAL2;
	}
	else if (m_iInZoom == ZOOM_ON)// not ZOOM_[IN|DE]CREASING
	{
		m_iInZoom = ZOOM_OFF;
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usZC, 0.0, g_vecZero, g_vecZero, 0, 0, m_iZoomCrosshair, m_iZoomSound, m_iInZoom, 0);
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/xbow_aim.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->pev->viewmodel = MAKE_STRING(g_ViewModelCrossbow);
		m_pPlayer->ResetAutoaim();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	}
}

void CWeaponCrossbow::Reload(void)
{
	if (!HasAmmo(AMMO_PRIMARY))
		return;

	if (m_iInZoom != ZOOM_OFF)
		SecondaryAttack();

	DefaultReload(CROSSBOW_RELOAD, CROSSBOW_RELOAD_DELAY, "weapons/xbow_reload1.wav");
}

// Once we've got gere, it means player has released ATTACK2 button and, thus, stopped zooming
void CWeaponCrossbow::WeaponIdle(void)
{
	if (m_iInZoom == ZOOM_INCREASING)// stop zooming
		m_iInZoom = ZOOM_ON;
	else if (m_iInZoom == ZOOM_OFF)
		m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);// get the autoaim vector but ignore it; used for autoaim crosshair in DM
	else// should not get here
		m_pPlayer->ResetAutoaim();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 1;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, rmax);
	if (r == 1)
		SendWeaponAnim((m_iClip > 0)?CROSSBOW_FIDGET1:CROSSBOW_FIDGET2, pev->body);
	else
		SendWeaponAnim((m_iClip > 0)?CROSSBOW_IDLE1:CROSSBOW_IDLE2, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 4.0, 8.0);
}

void CWeaponCrossbow::OnDetached(void)// XDM3038c
{
	CBasePlayerWeapon::OnDetached();// XDM3038c
	if (m_pPlayer)
	{
		if (m_iInZoom != ZOOM_OFF)
			PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), m_usZC, 0.0, g_vecZero, g_vecZero, 0, 0, m_iZoomCrosshair, m_iZoomSound, ZOOM_OFF, 0);

		m_pPlayer->pev->fov = 0;// XDM3037a
	}
	m_iInZoom = ZOOM_OFF;// XDM3037a
}

void CWeaponCrossbow::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->m_fInZoom = m_iInZoom;
}
