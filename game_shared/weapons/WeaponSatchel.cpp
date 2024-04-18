#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "skill.h"

// XDM3038b: now the satchel uses one model... finally
enum vanim_satchel_e
{
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP,
	SATCHEL_RADIO_IDLE1,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

// CWeaponSatchel::m_iChargeReady
enum satchel_ready_e {
	SATCHEL_READY_TO_THROW = 0,
	SATCHEL_READY_TO_CLICK,
	SATCHEL_READY_CLICKED
};

#define VSATCHEL_BODY_CHARGE		0
// :) #define VSATCHEL_BODY_CHARGE_AND_DETONATOR 10
#define VSATCHEL_BODY_DETONATOR		15

LINK_ENTITY_TO_CLASS(weapon_satchel, CWeaponSatchel);

TYPEDESCRIPTION	CWeaponSatchel::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponSatchel, m_iChargeReady, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponSatchel, CBasePlayerWeapon);

bool CWeaponSatchel::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))// XDM: changed from CBasePlayerItem
	{
		m_iChargeReady = SATCHEL_READY_TO_THROW;// this satchel charge weapon now forgets that any satchels are deployed by it.
		m_iClip = 0;// XDM3035: forget about previous owner's charges UNDONE: should be somehow connected to DeactivateSatchels()
		return true;
	}
	return false;
}

bool CWeaponSatchel::AddDuplicate(CBasePlayerItem *pOriginal)
{
	if (IsMultiplayer())
	{
		CBasePlayerWeapon *pSatchel = pOriginal->GetWeaponPtr();
		if (pSatchel && pSatchel->m_iClip >= MaxAmmoCarry(PrimaryAmmoIndex()))// XDM3038b: FIXED!
			return false;// player has some satchels deployed. Refuse to add more.
	}
	return CBasePlayerWeapon::AddDuplicate(pOriginal);
}

void CWeaponSatchel::Spawn(void)
{
	CBasePlayerWeapon::Spawn();// XDM3038a: Precache() inside
	pev->skin = SATCHEL_TEXGRP_PICKUP;// 'off' world model skin
	//pev->body = VSATCHEL_BODY_CHARGE;// leave current body - it can be a detonator :D

#if !defined (CLIENT_DLL)
	if (m_pPlayer)// this is an owned satchel that spawns in player's inventory on level change
	{
		m_iClip = 0;
		//int numActiveCharges = 0;
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, STRING(pev->message))) != NULL)// SLOW but fair!
		{
			if (pEntity->m_hOwner == m_pPlayer)
				m_iClip++;
		}
		//if (numActiveCharges >= SATCHEL_MAX_CARRY)// XDM3034
//#else
		//if (pSatchel->m_iChargeReady != 0)
	}
#endif
}

// Warning: Remember! Models may be customized by level designer!
void CWeaponSatchel::Precache(void)
{
	m_iId = WEAPON_SATCHEL;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = SATCHEL_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_satchel.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_satchel.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgSatchel;

	PRECACHE_SOUND("weapons/c4_button.wav");
	PRECACHE_SOUND("weapons/c4_select.wav");

	pev->message = MAKE_STRING("monster_satchel");// XDM3037: projectile
	//UTIL_PrecacheOther("monster_satchel");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponSatchel::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_SATCHEL;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | /* no need to ITEM_FLAG_NOAUTOSWITCHEMPTY | */ITEM_FLAG_LIMITINWORLD;// XDM | ITEM_FLAG_EXHAUSTIBLE;
	p->iMaxClip = SATCHEL_MAX_CLIP;// XDM3034 active charges indicator, MUST BE WEAPON_NOCLIP!!
	p->iWeight = SATCHEL_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_SATCHEL;
	p->iPosition = POSITION_SATCHEL;
#endif
	p->pszAmmo1 = "Satchel Charge";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = SATCHEL_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponSatchel::IsUseable(void) const
{
	if (!IsMultiplayer())
		return true;// allow player to use the detonator anytime (after level change, etc.)

	if (HasAmmo(AMMO_PRIMARY))//if (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) > 0) 
		return true;// player is carrying some satchels

	if (m_iChargeReady > 0)
		return true;// player isn't carrying any satchels, but has some out

	if (m_iClip > 0)
		return true;// XDM

	return false;
}

// Checked by GetNextBestWeapon()
bool CWeaponSatchel::CanDeploy(void) const
{
	if (!IsMultiplayer())
		return true;// XDM: always show detonator - player may want to reactivate satchels put across level transitions

	return CBasePlayerWeapon::CanDeploy();
}

bool CWeaponSatchel::Deploy(void)
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;// XDM3035: TESTME: a long time ago it caused a crash!

	if (m_iChargeReady || !HasAmmo(AMMO_PRIMARY|AMMO_CLIP))
	{
		SendWeaponAnim(SATCHEL_RADIO_DRAW);
		pev->body = VSATCHEL_BODY_DETONATOR;
		return DefaultDeploy(SATCHEL_RADIO_DRAW, "hive", "weapons/c4_select.wav", 0, pev->body);
	}
	else
	{
		SendWeaponAnim(SATCHEL_DRAW);
		pev->body = VSATCHEL_BODY_CHARGE;
		return DefaultDeploy(SATCHEL_DRAW, "trip", "weapons/c4_select.wav", 0, pev->body);
	}
}

void CWeaponSatchel::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if (!IsHolstered())// hand is not down
	{
		if (pev->body == VSATCHEL_BODY_CHARGE)
			SendWeaponAnim(SATCHEL_DROP);
		else
			SendWeaponAnim(SATCHEL_RADIO_HOLSTER);// XDM3034: only state 1!
	}
	//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponSatchel::PrimaryAttack(void)
{
	if (m_iChargeReady == 1 || !HasAmmo(AMMO_PRIMARY/*no |AMMO_CLIP*/))
		RadioClick();
	else if (m_iChargeReady == 0)
		Throw();
	else
		WeaponIdle();// XDM3035: force reset
}

void CWeaponSatchel::SecondaryAttack(void)
{
	if (m_iChargeReady != 2)
		Throw();
}

void CWeaponSatchel::RadioClick(void)
{
	SendWeaponAnim(SATCHEL_RADIO_FIRE);
	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/c4_button.wav", VOL_NORM, ATTN_NORM);
	// nope, this isn't viewmodel. and it uses event 5005 now		pev->skin = 1;
	CBaseEntity *pSatchel = NULL;
	CBaseEntity *pAttacker = GetDamageAttacker();
	while ((pSatchel = UTIL_FindEntityInSphere(pSatchel, m_pPlayer->pev->origin, MAX_ABS_ORIGIN)) != NULL)
	{
		if (FClassnameIs(pSatchel->pev, STRING(pev->message)))
		{
			if (pSatchel->m_hOwner == pAttacker)
				pSatchel->Use(pAttacker, pAttacker, USE_ON, 0);
		}
	}
	m_iChargeReady = 2;
	// each satchel charge must report		m_iClip = 0;// XDM3034
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SATCHEL_ATTACK_INTERVAL2);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;// ensure that the animation can finish playing
}

void CWeaponSatchel::Throw(void)
{
	if (!HasAmmo(AMMO_PRIMARY))
		return;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward * GetBarrelLength();
	CSatchelCharge *pSatchel = CSatchelCharge::CreateSatchelCharge(vecSrc, vecAng, m_pPlayer->pev->velocity+gpGlobals->v_forward * 274.0f, GetDamageAttacker(), GetDamageAttacker(), pev->scale, pev->dmg);
	if (pSatchel)
	{
		pSatchel->pev->avelocity.y = 400.0f;
#endif
		SwitchToRadio();

		if (m_iClip < 0)// !
			m_iClip = 0;
		m_iClip += UseAmmo(AMMO_PRIMARY, 1);// XDM3034
		m_iChargeReady = 1;

		m_flNextPrimaryAttack = GetNextAttackDelay(SATCHEL_ATTACK_INTERVAL1);
		m_flNextSecondaryAttack = GetNextAttackDelay(SATCHEL_ATTACK_INTERVAL2);
#if !defined (CLIENT_DLL)
	}
#endif
}

void CWeaponSatchel::SwitchToRadio(void)
{
	pev->body = VSATCHEL_BODY_DETONATOR;
	SendWeaponAnim(SATCHEL_RADIO_DRAW, pev->body);
	if (m_pPlayer)
		m_pPlayer->SetWeaponAnimType("hive");
}

void CWeaponSatchel::SwitchToCharge(void)
{
	pev->body = VSATCHEL_BODY_CHARGE;
	SendWeaponAnim(SATCHEL_DRAW);
	if (m_pPlayer)
		m_pPlayer->SetWeaponAnimType("trip");
}

void CWeaponSatchel::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iChargeReady == 0)// ready to throw charges
	{
		if (pev->body == VSATCHEL_BODY_CHARGE)//HasAmmo(AMMO_PRIMARY))
		{
			SendWeaponAnim(SATCHEL_FIDGET1);
			m_pPlayer->SetWeaponAnimType("trip");
		}
		else
		{
			SendWeaponAnim(SATCHEL_RADIO_FIDGET1);
			m_pPlayer->SetWeaponAnimType("hive");
		}
	}
	else if (m_iChargeReady == 1)// some charges are thrown and active
	{
		SendWeaponAnim(SATCHEL_RADIO_FIDGET1);
		m_pPlayer->SetWeaponAnimType("hive");
	}
	else if (m_iChargeReady == 2)// just used the detonator, take the charge again
	{
		// nope	pev->skin = 0;// XDM: reset button skin
		if (!HasAmmo(AMMO_PRIMARY|AMMO_CLIP))
		{
			//if (IsMultiplayer())
				RetireWeapon();
			/*else
			{
				Holster();
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SATCHEL_ATTACK_INTERVAL2);
			}*/
			//m_iChargeReady = 1;// XDM: allow to reuse detonator?
		}
		else
		{
			SwitchToCharge();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		}
		m_iChargeReady = 0;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);// how long till we do this again.
}

void CWeaponSatchel::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_iChargeReady;
}
