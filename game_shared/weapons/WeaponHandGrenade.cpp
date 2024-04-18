#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "globals.h"
#include "monsters.h"

enum vanim_handgrenade_e
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,// toss
	HANDGRENADE_THROW2,// medium
	HANDGRENADE_THROW3,// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};

LINK_ENTITY_TO_CLASS(weapon_handgrenade, CWeaponHandGrenade);

TYPEDESCRIPTION	CWeaponHandGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponHandGrenade, m_flStartThrow, FIELD_TIME),
	DEFINE_FIELD(CWeaponHandGrenade, m_flReleaseThrow, FIELD_TIME),
};
IMPLEMENT_SAVERESTORE(CWeaponHandGrenade, CBasePlayerWeapon);

void CWeaponHandGrenade::Precache(void)
{
	m_iId = WEAPON_HANDGRENADE;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_grenade.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_grenade.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgHandGrenadeExplosive;

	PRECACHE_SOUND("weapons/hgrenade_select.wav");
	PRECACHE_SOUND("weapons/hgrenade_pinpull.wav");

	m_usGrenMode = PRECACHE_EVENT(1, "events/weapons/grenmode.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

/*#if !defined (CLIENT_DLL)
int CWeaponHandGrenade::Restore(CRestore &restore)
{
CRASHCRASHCRASH
	int r = CBaseEntity::Restore(restore);
	MESSAGE_BEGIN(MSG_ONE, gmsgViewModel, NULL, m_pPlayer->pev);
		WRITE_BYTE(kRenderNormal);
		WRITE_BYTE(kRenderFxNone);
		WRITE_BYTE(0);// r
		WRITE_BYTE(0);// g
		WRITE_BYTE(0);// b
		WRITE_BYTE(0);// amt
		WRITE_BYTE(pev->skin);// skin
	MESSAGE_END();
	return r;
}
#endif*/
// XDM3035
/*int CWeaponHandGrenade::UpdateClientData(BOOL writedataonly)
{
	int send = CBasePlayerWeapon::UpdateClientData();
	/* TODO: does not work
	if (send && pev->skin > 0)
	{
		int flags;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = FEV_HOSTONLY;
#endif
		conprintf(1, "CWeaponHandGrenade::UpdateClientData() send!\n");
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usGrenMode, 0.5f, g_vecZero, g_vecZero, 0.0f, 0.0f, pev->skin, 0, 0, 0);
	}* /
	return send;
}*/

int CWeaponHandGrenade::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_HANDGRENADE;
	p->iFlags = ITEM_FLAG_NOAUTORELOAD | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE | ITEM_FLAG_NOAUTOSWITCHEMPTY;// don't switch immediately
	p->iMaxClip = HANDGRENADE_MAX_CLIP;
	p->iWeight = HANDGRENADE_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_HANDGRENADE;
	p->iPosition = POSITION_HANDGRENADE;
#endif
	p->pszAmmo1 = "Hand Grenade";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponHandGrenade::Deploy(void)
{
	m_flReleaseThrow = -1;
	if (DefaultDeploy(HANDGRENADE_DRAW, "crowbar", "weapons/hgrenade_select.wav"))
	{
		int flags;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = FEV_HOSTONLY;
#endif

		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usGrenMode, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, pev->skin, 0, 1, 0);// XDM3038a
	/*#if !defined (CLIENT_DLL)
		MESSAGE_BEGIN(MSG_ONE, gmsgViewModel, NULL, m_pPlayer->pev);
			WRITE_BYTE(kRenderNormal);
			WRITE_BYTE(kRenderFxNone);
			WRITE_BYTE(0);// r
			WRITE_BYTE(0);// g
			WRITE_BYTE(0);// b
			WRITE_BYTE(0);// amt
			WRITE_BYTE(pev->skin);// skin
		MESSAGE_END();
	#endif*/
		return true;
	}
	return false;
}

bool CWeaponHandGrenade::CanHolster(void) const
{
	// can only holster hand grenades when not primed!
	return (m_flStartThrow == 0.0f);
}

void CWeaponHandGrenade::Holster(int skiplocal /* = 0 */)
{
	if (m_flStartThrow > 0.0f)// XDM3035: occurs when a player is killed
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		m_flReleaseThrow = UTIL_WeaponTimeBase();
		WeaponIdle();
	}
	else
	{
		if (m_pPlayer->AmmoInventory(m_iPrimaryAmmoType) > 0)
			SendWeaponAnim(HANDGRENADE_HOLSTER);
	}
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponHandGrenade::PrimaryAttack(void)
{
	if (m_flStartThrow == 0.0f && HasAmmo(AMMO_PRIMARY))
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0.0f;
		EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/hgrenade_pinpull.wav", VOL_NORM, ATTN_IDLE);
		SendWeaponAnim(HANDGRENADE_PINPULL);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7f;// XDM3034
	}
}

void CWeaponHandGrenade::SecondaryAttack(void)
{
	if (m_flStartThrow > 0.0f)// XDM3037: no switching after pulling the ring
		return;
	if (!HasAmmo(AMMO_PRIMARY))
		return;

	if (pev->skin >= HANDGRENADE_LAST_TYPE)
		pev->skin = 0;
	else
		pev->skin++;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = FEV_HOSTONLY;// XDM: undone: create an array of skins for each player on client side for p_ models.
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usGrenMode, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, pev->skin, 0, 1, 0);// XDM
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + HANDGRENADE_ATTACK_INTERVAL2;
}

void CWeaponHandGrenade::WeaponIdle(void)
{
	if (m_flReleaseThrow == 0 && m_flStartThrow > 0.0f)
		m_flReleaseThrow = UTIL_WeaponTimeBase();// gpGlobals->time; ?

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_flStartThrow > 0.0f)
	{
		Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		float flVel = (float)HANDGRENADE_THROW_VELOCITY * (1.0f + cosf(DEG2RAD(vecAng.x*0.75f + 10.0f)));// 0.75 widens and 10 shifts the result graph
		float grentime = m_flStartThrow - gpGlobals->time + HANDGRENADE_IGNITE_TIME;
		if (grentime < 0)
			grentime = 0;
		// XDM3035c: now the peak velocity is at ang.x ~= -10 (slightly upwards)
		Vector vecSrc(m_pPlayer->GetGunPosition() + gpGlobals->v_forward * GetBarrelLength());
		Vector vecThrow(gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity);

		if (flVel < 500)
			SendWeaponAnim(HANDGRENADE_THROW1);
		else if (flVel < 800)
			SendWeaponAnim(HANDGRENADE_THROW2);
		else
			SendWeaponAnim(HANDGRENADE_THROW3);

		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#if !defined (CLIENT_DLL)
		CGrenade *pGrenade = CGrenade::ShootTimed(vecSrc, vecThrow, grentime, pev->skin, GetDamageAttacker(), GetDamageAttacker());
		if (pGrenade)
		{
			pGrenade->pev->scale = pev->scale;// XDM3035b: weapon world scale
			if (pev->skin == GREN_EXPLOSIVE)
				pGrenade->pev->dmg = pev->dmg;
		}
#endif
		m_flReleaseThrow = 0.0f;
		m_flStartThrow = 0.0f;
		m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(HANDGRENADE_ATTACK_INTERVAL1);// ensure that the animation can finish playing
		UseAmmo(AMMO_PRIMARY, 1);
		return;// let the animation finish, until next WeaponIdle()
	}
	else if (m_flReleaseThrow >= 0.0f)// fixed
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;
		if (HasAmmo(AMMO_PRIMARY))
		{
			m_flReleaseThrow = -1;
			SendWeaponAnim(HANDGRENADE_DRAW);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		}
		else
			RetireWeapon();

		return;
	}

	if (HasAmmo(AMMO_PRIMARY))
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 1;
		else
			rmax = 0;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
		if (r == 1)
			SendWeaponAnim(HANDGRENADE_FIDGET, pev->body);
		else
			SendWeaponAnim(HANDGRENADE_IDLE, pev->body);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 3, 5);
	}
}

void CWeaponHandGrenade::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->fuser2 = m_flStartThrow;
	weapondata->fuser3 = m_flReleaseThrow;
}
