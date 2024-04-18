#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "globals.h"

enum vanim_sniperrifle_e
{
	RIFLE_DRAW = 0,
	RIFLE_IDLE,
	RIFLE_FIRE,
	RIFLE_FIRE_LAST,
	RIFLE_RELOAD,
	RIFLE_IDLE_LAST,
	RIFLE_HOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CWeaponSniperRifle);

static char g_ViewModelSniperRifle[] = "models/v_sniperrifle.mdl";// for MAKE_STRING

TYPEDESCRIPTION	CWeaponSniperRifle::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponSniperRifle, m_iInZoom, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponSniperRifle, CBasePlayerWeapon);

void CWeaponSniperRifle::Precache(void)
{
	m_iId = WEAPON_SNIPERRIFLE;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = SNIPERRIFLE_DEFAULT_GIVE;
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/p_sniperrifle.mdl");
	if (FStringNull(pev->viewmodel))
		pev->viewmodel = MAKE_STRING(g_ViewModelSniperRifle);
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgSniperRifle;

	m_iZoomSound = PRECACHE_SOUND("weapons/rifle_aim.wav");
	PRECACHE_SOUND("weapons/rifle_fire1.wav");
	PRECACHE_SOUND("weapons/rifle_reload1.wav");
	PRECACHE_SOUND("weapons/rifle_select.wav");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");
	m_iZoomCrosshair = PRECACHE_MODEL("sprites/c_zoom2.spr");

	m_usFire = PRECACHE_EVENT(1, "events/weapons/sniperrifle.sc");
	m_usZC = PRECACHE_EVENT(1, "events/weapons/zoomcrosshair.sc");

	CBasePlayerWeapon::Precache();
}

int CWeaponSniperRifle::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();
	p->iFlags = 0;
	p->iMaxClip = SNIPERRIFLE_MAX_CLIP;
	p->iWeight = SNIPERRIFLE_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_RIFLE;
	p->iPosition = POSITION_RIFLE;
#endif
	p->pszAmmo1 = "sniper";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponSniperRifle::Deploy(void)
{
	m_iInZoom = ZOOM_OFF;// TODO: rewrite as bit mask for zooming modes
	if (m_pPlayer)
		m_pPlayer->pev->fov = 0;// XDM3037a

	return DefaultDeploy(RIFLE_DRAW, "bow", "weapons/rifle_select.wav");
}

void CWeaponSniperRifle::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.
	OnDetached();// <- reset zoom
	if (m_pPlayer)
	{
		SendWeaponAnim(RIFLE_HOLSTER);
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 8, 12);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponSniperRifle::PrimaryAttack(void)
{
	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		return;
	}
	if (UseAmmo(AMMO_CLIP, 1) == 0)
		return;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc(m_pPlayer->GetGunPosition());
	Vector vecEnd(0,0,0);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, m_iInZoom, 0.0, (m_iClip == 0)?RIFLE_FIRE_LAST:RIFLE_FIRE, m_iShell, pev->body, SHELL_BRASS);
	FireBullets(1, vecSrc, m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES), VECTOR_CONE_1DEGREES*m_pPlayer->GetShootSpreadFactor(), &vecEnd, GetDefaultBulletDistance()*BULLET_SNIPER_DIST_MLT, BULLET_338, pev->dmg, this, GetDamageAttacker(), m_pPlayer->random_seed);
	//if (m_pPlayer == NULL)// XDM3038c
	//	return;

	m_pPlayer->pev->velocity -= gpGlobals->v_forward * 64.0f;
	//if (m_iClip == 0)?
	m_flNextPrimaryAttack = GetNextAttackDelay(RIFLE_ATTACK_INTERVAL1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RIFLE_ATTACK_INTERVAL1 + 1.0f;

	if (m_iInZoom == ZOOM_INCREASING)
		m_iInZoom = ZOOM_ON;// stop zooming
}

void CWeaponSniperRifle::SecondaryAttack(void)
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
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usZC, 0.0, g_vecZero, g_vecZero, RIFLE_MAX_ZOOM_FOV, 0, m_iZoomCrosshair, m_iZoomSound, m_iInZoom, kRenderTransColor);
		m_pPlayer->pev->viewmodel = iStringNull;
		m_pPlayer->ResetAutoaim();
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/rifle_aim.wav", VOL_NORM, ATTN_NORM);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RIFLE_ATTACK_INTERVAL2;
	}
	else if (m_iInZoom == ZOOM_ON)// not ZOOM_[IN|DE]CREASING
	{
		m_iInZoom = ZOOM_OFF;
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usZC, 0.0, g_vecZero, g_vecZero, 0, 0, m_iZoomCrosshair, m_iZoomSound, m_iInZoom, 0);
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/rifle_aim.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->pev->viewmodel = MAKE_STRING(g_ViewModelSniperRifle);
		m_pPlayer->ResetAutoaim();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	}
}

void CWeaponSniperRifle::Reload(void)
{
	if (!HasAmmo(AMMO_PRIMARY))//if (m_pPlayer->AmmoInventory(m_iPrimaryAmmoType) <= 0)// XDM
		return;

	if (m_iInZoom != ZOOM_OFF)
		SecondaryAttack();

	DefaultReload(RIFLE_RELOAD, RIFLE_RELOAD_DELAY, "weapons/rifle_reload1.wav");
}

// Once we've got gere, it means player has released ATTACK2 button and, thus, stopped zooming
void CWeaponSniperRifle::WeaponIdle(void)
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

	if (m_iClip == 0)
		SendWeaponAnim(RIFLE_IDLE_LAST, pev->body);
	else
		SendWeaponAnim(RIFLE_IDLE, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 4.0, 8.0);
}

void CWeaponSniperRifle::OnDetached(void)// XDM3038c
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

void CWeaponSniperRifle::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->m_fInZoom = m_iInZoom;
}
