#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "gamerules.h"
#include "skill.h"

enum vanim_python_e
{
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

// compatibility
LINK_ENTITY_TO_CLASS(weapon_python, CWeaponPython);
LINK_ENTITY_TO_CLASS(weapon_357, CWeaponPython);

static char g_ViewModelPython[] = "models/v_357.mdl";// XDM: for MAKE_STRING

TYPEDESCRIPTION	CWeaponPython::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponPython, m_fInZoom, FIELD_BOOLEAN),
};
IMPLEMENT_SAVERESTORE(CWeaponPython, CBasePlayerWeapon);

int CWeaponPython::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_PYTHON;
	p->iFlags = 0;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iWeight = PYTHON_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_PYTHON;
	p->iPosition = POSITION_PYTHON;
#endif
	p->pszAmmo1 = "357";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = _357_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

void CWeaponPython::Spawn(void)
{
	SetClassName("weapon_357");//pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	CBasePlayerWeapon::Spawn();// XDM3038a
}

void CWeaponPython::Precache(void)
{
	m_iId = WEAPON_PYTHON;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_357.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING(g_ViewModelPython);
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.Dmg357;

	PRECACHE_SOUND("weapons/357_select.wav");
	PRECACHE_SOUND("weapons/357_reload0.wav");
	PRECACHE_SOUND("weapons/357_reload1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("weapons/357_shot2.wav");

	m_usFire = PRECACHE_EVENT(1, "events/weapons/python.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

bool CWeaponPython::Deploy(void)
{
	//if (IsMultiplayer())
			pev->body = 1;// enable laser sight geometry.
	//else
	//	pev->body = 0;

	//if (HasAmmo(AMMO_PRIMARY|AMMO_CLIP))
	//	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/357_select.wav", VOL_NORM, ATTN_NORM);

	return DefaultDeploy(PYTHON_DRAW, "python", "weapons/357_select.wav", UseDecrement(), pev->body);
}

void CWeaponPython::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress
	OnDetached();// <- reset zoom
	if (m_pPlayer)
	{
		SendWeaponAnim(PYTHON_HOLSTER);
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 8, 12);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponPython::PrimaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		return;
	}

	if (m_iClip <= 0)
	{
		if (HasAmmo(AMMO_PRIMARY))
		{
			Reload();
		}
		else
		{
			EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		}
		return;
	}
	if (UseAmmo(AMMO_CLIP, 1) != 1)
		return;

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc(m_pPlayer->GetGunPosition());

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST|FEV_UPDATE;
#else
	flags = FEV_UPDATE;// XDM3038a: TESTME
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, (m_fInZoom?15.0f:10.0f)/*punchX*/, (m_fInZoom?-4.0f:0.0f)/*punchY*/, (m_fInZoom?PYTHON_IDLE1:PYTHON_FIRE1), 0, pev->body, RANDOM_LONG(0,1)/*sound*/);
	FireBullets(1, vecSrc, m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES), VECTOR_CONE_2DEGREES*m_pPlayer->GetShootSpreadFactor(), NULL, GetDefaultBulletDistance(), BULLET_357, pev->dmg, this, GetDamageAttacker(), m_pPlayer->random_seed);
	if (m_pPlayer == NULL)// XDM3038c
		return;

	m_flNextPrimaryAttack = GetNextAttackDelay(PYTHON_ATTACK_INTERVAL1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 15);
}

void CWeaponPython::SecondaryAttack(void)
{
	if (m_pPlayer->pev->fov != PYTHON_ZOOM_FOV)
	{
		m_pPlayer->pev->viewmodel = iStringNull;// XDM
		m_pPlayer->pev->fov/* = m_pPlayer->m_iFOV*/ = PYTHON_ZOOM_FOV;
		m_fInZoom = TRUE;
	}
	else// if (m_pPlayer->pev->fov != 0)
	{
		m_pPlayer->pev->viewmodel = MAKE_STRING(g_ViewModelPython);// XDM: safe to use MAKE
		m_pPlayer->pev->fov/* = m_pPlayer->m_iFOV*/ = 0;// 0 means reset to default fov
		m_fInZoom = FALSE;
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + PYTHON_ATTACK_INTERVAL2;
}

void CWeaponPython::Reload(void)
{
	//if (m_iClip > 0)// allow reloading only when empty?
	//	return;

	if (DefaultReload(PYTHON_RELOAD, PYTHON_RELOAD_DELAY, "weapons/357_reload0.wav"))
	{
		if (m_fInZoom || m_pPlayer->pev->fov != 0)//DEFAULT_FOV?)// XDM3037a
		{
			m_pPlayer->pev->viewmodel = MAKE_STRING(g_ViewModelPython);// XDM: safe to use MAKE
			m_pPlayer->pev->fov = 0;// = m_pPlayer->m_iFOV = DEFAULT_FOV;  // 0 means reset to default fov
			m_fInZoom = FALSE;
		}
		// HL20130901	m_flSoundDelay = 1.5;
	}
}

void CWeaponPython::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	//conprintf(1, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	/*HL20130901	if (m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase())
	{
		EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;
	}*/

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fInZoom)// XDM: player doesn't see animations anyway
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 3;
	else
		rmax = 2;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 3)
		SendWeaponAnim(PYTHON_FIDGET, pev->body);
	else if (r == 2)
		SendWeaponAnim(PYTHON_IDLE3, pev->body);
	else if (r == 1)
		SendWeaponAnim(PYTHON_IDLE2, pev->body);
	else // if (r == 0)
		SendWeaponAnim(PYTHON_IDLE1, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
}

void CWeaponPython::OnDetached(void)// XDM3038c
{
	CBasePlayerWeapon::OnDetached();// XDM3038c
	if (m_pPlayer)
	{
		m_pPlayer->pev->fov/* = m_pPlayer->m_iFOV*/ = 0;// 0 means reset to default fov
		m_fInZoom = FALSE;
	}
	m_fInZoom = FALSE;// XDM3037a
}

void CWeaponPython::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->m_fInZoom = m_fInZoom;
}
