#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "globals.h"

enum vanim_mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
	MP5_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_mp5, CWeaponMP5);
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CWeaponMP5);

void CWeaponMP5::Spawn(void)
{
	SetClassName("weapon_9mmAR");// compatibility
	CBasePlayerWeapon::Spawn();// XDM3038a
}

void CWeaponMP5::Precache(void)
{
	m_iId = WEAPON_MP5;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = MP5_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_9mmar.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_9mmar.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgMP5;

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell TE_MODEL
	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");
	PRECACHE_SOUND("weapons/mp5_reload.wav");
	PRECACHE_SOUND("weapons/mp5_select.wav");

	m_usFire1 = PRECACHE_EVENT(1, "events/weapons/mp5.sc");
	m_usFire2 = PRECACHE_EVENT(1, "events/weapons/mp52.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponMP5::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_MP5;
	p->iFlags = 0;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iWeight = MP5_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_MP5;
	p->iPosition = POSITION_MP5;
#endif
	p->pszAmmo1 = "9mm";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
#endif
	p->pszAmmo2 = "ARgrenades";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
#endif
	return 1;
}

bool CWeaponMP5::Deploy(void)
{
	return DefaultDeploy(MP5_DEPLOY, "mp5", "weapons/mp5_select.wav");
}

void CWeaponMP5::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(MP5_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponMP5::PrimaryAttack(void)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + MP5_ATTACK_INTERVAL1;
		return;
	}

	if (UseAmmo(AMMO_CLIP, 1) == 0)
		return;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	//m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;// XDM3035a
	//m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + MP5_ATTACK_INTERVAL1;
	Vector vecSrc(m_pPlayer->GetGunPosition());
	Vector vecAiming(m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES));

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST|FEV_UPDATE;
#else
	flags = FEV_UPDATE;// XDM3038a: TESTME
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire1, 0.0, g_vecZero, g_vecZero, /*vecDir.x, vecDir.y*/0,0, RANDOM_LONG(MP5_FIRE1, MP5_FIRE3), m_iShell, pev->body, SHELL_BRASS);

	/*Vector vecDir(*/FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES*m_pPlayer->GetShootSpreadFactor(), NULL, GetDefaultBulletDistance(), BULLET_MP5, pev->dmg, this, GetDamageAttacker(), m_pPlayer->random_seed);//);
	if (m_pPlayer == NULL)// XDM3038c
		return;

	m_flNextPrimaryAttack = GetNextAttackDelay(MP5_ATTACK_INTERVAL1);
	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.05;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 4.0f, 8.0f);
}

void CWeaponMP5::SecondaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || !HasAmmo(AMMO_SECONDARY))
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + MP5_ATTACK_INTERVAL2;
		return;
	}

	if (UseAmmo(AMMO_SECONDARY, 1) == 0)
		return;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2f;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire2, 0.0, g_vecZero, g_vecZero, 0.0f, 0.0f, MP5_LAUNCH, m_iShell, pev->body, RANDOM_LONG(0,1)/*sound*/);

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	// we don't add in player velocity anymore?
	CARGrenade::CreateGrenade(m_pPlayer->GetGunPosition() + gpGlobals->v_forward*GetBarrelLength(), vecAng, gpGlobals->v_forward * 800.0f, GetDamageAttacker(), GetDamageAttacker(), 0);
#endif
	m_flNextPrimaryAttack = GetNextAttackDelay(MP5_ATTACK_INTERVAL1);
	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.05;

	m_flNextSecondaryAttack = GetNextAttackDelay(MP5_ATTACK_INTERVAL2);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5.0f, 10.0f);
}

void CWeaponMP5::Reload(void)
{
	//if (m_iClip > 0)// allow reloading only when empty?
	//	return;

	DefaultReload(MP5_RELOAD, MP5_RELOAD_DELAY, "weapons/mp5_reload.wav");// XDM3038a: this sound can be heared by other players
}

void CWeaponMP5::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 1;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, rmax);
	if (r == 1)
		SendWeaponAnim(MP5_LONGIDLE, pev->body);
	else // if (r == 0)
		SendWeaponAnim(MP5_IDLE1, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 4, 10);// how long till we do this again. // XDM3038a: fix typo
}
