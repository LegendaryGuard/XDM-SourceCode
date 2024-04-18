#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "effects.h"
#include "gamerules.h"
#include "game.h"

enum vanim_tripmine_e
{
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_PLACE,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
};

LINK_ENTITY_TO_CLASS(weapon_tripmine, CWeaponTripmine);

void CWeaponTripmine::Spawn(void)
{
	CBasePlayerWeapon::Spawn();// XDM3038a

	if (!IsMultiplayer())
		UTIL_SetSize(this, Vector(-16, -16, 0), Vector(16, 16, 28));
}

void CWeaponTripmine::Precache(void)
{
	m_iId = WEAPON_TRIPMINE;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = TRIPMINE_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_tripmine.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_tripmine.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgTripmine;

	PRECACHE_SOUND("weapons/mine_select.wav");
	m_usFireMode = PRECACHE_EVENT(1, "events/weapons/tripmode.sc");
	//m_usFire = PRECACHE_EVENT(1, "events/weapons/tripfire.sc");
	//UTIL_PrecacheOther("monster_tripmine");// XDM: world model precached here
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponTripmine::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_TRIPMINE;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iMaxClip = TRIPMINE_MAX_CLIP;
	p->iWeight = TRIPMINE_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_TRIPMINE;
	p->iPosition = POSITION_TRIPMINE;
#endif
	p->pszAmmo1 = "Trip Mine";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = TRIPMINE_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponTripmine::Deploy(void)
{
	return DefaultDeploy(TRIPMINE_DRAW, "trip", "weapons/mine_select.wav");
}

void CWeaponTripmine::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (HasAmmo(AMMO_ANYTYPE))// XDM3038a
		SendWeaponAnim(TRIPMINE_HOLSTER);

	//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponTripmine::PrimaryAttack(void)
{
	PlaceMine(TRIPMINE_PM_BEAM, pev->skin);
}

#ifdef CLIENT_DLL
short GetGameMode(void);
short GetGameFlags(void);
#endif

void CWeaponTripmine::SecondaryAttack(void)
{
#if defined (CLIENT_DLL)
	if (FBitSet(GetGameFlags(), GAME_FLAG_OVERDRIVE))
#else
	if (sv_overdrive.value > 0.0f)
#endif
	{
		if (pev->skin >= TRIPMINE_LAST_MODE)
			pev->skin = 0;
		else
			pev->skin++;

		int flags;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = FEV_HOSTONLY;// XDM: undone: create an array of skins for each player on client side for p_ models.
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireMode, 0, g_vecZero, g_vecZero, 0, 0, pev->skin, 0, 1, 0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + TRIPMINE_ATTACK_INTERVAL2;
	}
	else// OLD code
		PlaceMine(TRIPMINE_PM_RADIUS, pev->skin);
}

bool CWeaponTripmine::PlaceMine(short pingmode, short explodetype)
{
	if (!HasAmmo(AMMO_PRIMARY))
		return false;

	bool success = false;
#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition());
	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecSrc + gpGlobals->v_forward * 128.0f, dont_ignore_monsters, m_pPlayer->edict(), &tr);
	if (tr.flFraction < 1.0f)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity && !FBitSet(pEntity->pev->flags, FL_CONVEYOR))
		{
			if (pingmode == TRIPMINE_PM_BEAM)
				SendWeaponAnim(TRIPMINE_PLACE);
			else
				SendWeaponAnim(TRIPMINE_ARM1);

			Vector vMineAngles;
			VectorAngles(tr.vecPlaneNormal, vMineAngles);// XDM3038
			CBaseEntity *pMine = CBaseEntity::Create(TRIPMINE_CLASSNAME, tr.vecEndPos + tr.vecPlaneNormal * 6, vMineAngles, g_vecZero, GetDamageAttacker()->edict(), SF_NORESPAWN);// | SF_TRIPMINE_QUICKPOWERUP);
			if (pMine)
			{
				pMine->pev->team = m_pPlayer->pev->team;
				pMine->pev->impulse = pingmode;
				pMine->pev->skin = explodetype;
				pMine->pev->scale = pev->scale;
				pMine->pev->dmg = pev->dmg;
				UseAmmo(AMMO_PRIMARY, 1);
				m_pPlayer->SetAnimation(PLAYER_ATTACK1);
				if (HasAmmo(AMMO_ANYTYPE))// XDM3038a
					SendWeaponAnim(TRIPMINE_DRAW);
				else// tripmine doesn't need any animations
				{
					RetireWeapon();
					return true;
				}
				success = true;
			}
		}
	}
#endif
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(TRIPMINE_ATTACK_INTERVAL1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	return success;
}

void CWeaponTripmine::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (HasAmmo(AMMO_ANYTYPE))// XDM3038a
		SendWeaponAnim(TRIPMINE_DRAW);
	else
	{
		RetireWeapon();
		return;
	}

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 2;
	else
		rmax = 1;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 2)
		SendWeaponAnim(TRIPMINE_FIDGET, pev->body);
	else if (r == 1)
		SendWeaponAnim(TRIPMINE_IDLE2, pev->body);
	else // if (r == 0)
		SendWeaponAnim(TRIPMINE_IDLE1, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 3,6);
}
