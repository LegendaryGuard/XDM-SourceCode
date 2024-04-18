//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "soundent.h"
#include "sound.h"
#include "game.h"
#include "globals.h"
#include "pm_shared.h"

LINK_ENTITY_TO_CLASS(weapon_sword, CWeaponSword);

enum vanim_sword_e
{
	SWORD_DRAW = 0,
	SWORD_HOLSTER,
	SWORD_ATTACK_L,
	SWORD_ATTACK_R,
	SWORD_ATTACK_R_KICK,
	SWORD_IDLE1,
	SWORD_IDLE2,
	SWORD_IDLE3,
};

void CWeaponSword::Precache(void)
{
	m_iId = WEAPON_SWORD;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = 0;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_sword.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_sword.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgSword;

	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/sword_hit1.wav");
	PRECACHE_SOUND("weapons/sword_hit2.wav");
	PRECACHE_SOUND("weapons/sword_miss.wav");
	PRECACHE_SOUND("weapons/sword_select.wav");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponSword::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_SWORD;
	p->iFlags = ITEM_FLAG_CANNOTDROP|ITEM_FLAG_SELECTONEMPTY;
	p->iMaxClip = SWORD_MAX_CLIP;
	p->iWeight = SWORD_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_SWORD;
	p->iPosition = POSITION_SWORD;
#endif
	p->pszAmmo1 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = -1;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponSword::Deploy(void)
{
	pev->button = 0;// XDM3038: pev->impulse conflicts with CBasePlayerWeapon
	return DefaultDeploy(SWORD_DRAW, "crowbar", "weapons/sword_select.wav");
}

void CWeaponSword::Holster(int skiplocal /* = 0 */)
{
	pev->button = 0;
	SendWeaponAnim(SWORD_HOLSTER);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

// more distant, powerful, slow and loud attack mode
void CWeaponSword::PrimaryAttack(void)
{
	SendWeaponAnim(RANDOM_INT2(SWORD_ATTACK_L, SWORD_ATTACK_R));
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/sword_miss.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96,104));
	pev->button = 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.25f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SWORD_ATTACK_INTERVAL1);
}

// less distant, powerful but quiet and fast attack mode
void CWeaponSword::SecondaryAttack(void)
{
	SendWeaponAnim(SWORD_ATTACK_R_KICK);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/sword_miss.wav", 0.8, ATTN_IDLE, 0, RANDOM_LONG(96,104));
	pev->button = 2;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.20f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SWORD_ATTACK_INTERVAL2);
}

void CWeaponSword::Attack(WEAPON_FIREMODE mode, vec_t distance)
{
	TraceResult tr;
	m_pPlayer->SetWeaponAnimType("crowbar");
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc(m_pPlayer->GetGunPosition());
	Vector vecEnd(gpGlobals->v_forward); vecEnd*=distance; vecEnd+=vecSrc;//Vector vecEnd = vecSrc + gpGlobals->v_forward * distance;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);
#if !defined (CLIENT_DLL)
	if (tr.flFraction >= 1.0f)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr);
		if (tr.flFraction < 1.0f)// Calculate the point of intersection of the line (or hull) and the object we hit
		{
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			if (pHit == NULL || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
		}
	}
#endif
	if (tr.flFraction < 1.0f)// hit
	{
#if !defined (CLIENT_DLL)
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity)
		{
			float flDamage = pev->dmg;
			if (mode == FIRE_SEC)
				flDamage *= SWORD_SECONDARY_MULTIPLIER;

			ClearMultiDamage();
			pEntity->TraceAttack(GetDamageAttacker(), flDamage, gpGlobals->v_forward, &tr, DMG_CLUB); 
			ApplyMultiDamage(this, GetDamageAttacker());// XDM3034 inflictor = this
			if (pEntity->IsPushable())// XDM3037
			{
				float force = pEntity->DamageForce(flDamage);
				pEntity->pev->velocity += gpGlobals->v_forward * force;
				//pEntity->pev->avelocity += (pEntity->Center() - vecEnd).Normalize() * force;// XDM3035c: +=
			}

			if ((pEntity->IsMonster() || pEntity->IsHuman()) && pEntity->BloodColor() != DONT_BLEED)
			{
				switch (RANDOM_LONG(0,2))
				{
				case 0: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", VOL_NORM, ATTN_NORM, 0, 80); break;
				case 1: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/cbar_hitbod2.wav", VOL_NORM, ATTN_NORM, 0, 80); break;
				case 2: EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/cbar_hitbod3.wav", VOL_NORM, ATTN_NORM, 0, 80); break;
				}
				CSoundEnt::InsertSound(bits_SOUND_COMBAT|bits_SOUND_MEAT, pev->origin, SMALL_EXPLOSION_VOLUME, SWORD_ATTACK_INTERVAL1);
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
			}
			else
			{
				if (RANDOM_LONG(0,1) == 0)
					EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/sword_hit1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(80,83));
				else
					EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/sword_hit2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(80,83));

				CSoundEnt::InsertSound(bits_SOUND_COMBAT|bits_SOUND_WORLD, pev->origin, SMALL_EXPLOSION_VOLUME, SWORD_ATTACK_INTERVAL2);
				m_pPlayer->m_iWeaponVolume = SWORD_HIT_VOLUME;
				vecEnd = gpGlobals->v_forward; vecEnd *= distance*1.5f; vecEnd += vecSrc;// XDM3038c: trace further, a little hack to reliably identify the texture
				int tex = (int)TEXTURETYPE_Trace(pEntity, vecSrc, vecEnd);// XDM3038
				PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), g_usBulletImpact, 0, tr.vecEndPos, tr.vecPlaneNormal.As3f(), 0, 0, BULLET_BREAK, tex, 1, 1);
			}
		}
#endif
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + SWORD_ATTACK_INTERVAL1;
}

void CWeaponSword::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;// fire delay too

	if (pev->button > 0)
	{
		if (pev->button == 1)
			Attack(FIRE_PRI, SWORD_HIT_MAX_DIST1);
		else if (pev->button == 2)
			Attack(FIRE_SEC, SWORD_HIT_MAX_DIST2);

		pev->button = 0;
		return;
	}

	int iAnim = SWORD_IDLE1;
	switch(UTIL_SharedRandomLong(m_pPlayer->random_seed,0,2))
	{
	case 0: iAnim = SWORD_IDLE1; break;
	case 1: iAnim = SWORD_IDLE2; break;
	case 2: iAnim = SWORD_IDLE3; break;
	}
	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25f;
}
