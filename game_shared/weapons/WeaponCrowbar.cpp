#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "soundent.h"
#include "sound.h"
#include "globals.h"// XDM
#include "pm_shared.h"

// TODO: merge with sword (its code is better)

LINK_ENTITY_TO_CLASS(weapon_crowbar, CWeaponCrowbar);

enum vanim_crowbar_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT
};

void CWeaponCrowbar::Precache(void)
{
	m_iId = WEAPON_CROWBAR;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = 0;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_crowbar.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_crowbar.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgCrowbar;

	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_SOUND("weapons/cbar_select.wav");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/crowbar.sc");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponCrowbar::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_CROWBAR;
	p->iFlags = ITEM_FLAG_CANNOTDROP|ITEM_FLAG_SELECTONEMPTY;// XDM
	p->iMaxClip = CROWBAR_MAX_CLIP;
	p->iWeight = CROWBAR_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_CROWBAR;
	p->iPosition = POSITION_CROWBAR;
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

bool CWeaponCrowbar::Deploy(void)
{
	return DefaultDeploy(CROWBAR_DRAW, "crowbar", "weapons/cbar_select.wav");
}

void CWeaponCrowbar::Holster(int skiplocal /* = 0 */)
{
	SetThinkNull();// XDM3037
	DontThink();// XDM3038a
	SendWeaponAnim(CROWBAR_HOLSTER);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponCrowbar::PrimaryAttack(void)
{
	Swing();
}

void CWeaponCrowbar::SecondaryAttack(void)
{
	Swing();
}

void CWeaponCrowbar::Smack(void)
{
	DecalGunshot(&m_Trace, BULLET_BREAK);
	SetThinkNull();
	DontThink();// XDM3038a
}

void CWeaponCrowbar::Swing(void)
{
	int iAnim = 0;
	float fvolbar = 1.0;
	float edamage = 0.0f;
	//TraceResult tr;
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc(m_pPlayer->GetGunPosition());
	Vector vecEnd(gpGlobals->v_forward); vecEnd *= CROWBAR_HIT_DISTANCE; vecEnd += vecSrc;
	byte ev_sound = 0;// XDM3037: 0-miss, 1-hitbody, 2-hitworld
	char tex = 0;
	bool hit = false;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &m_Trace);
	if (m_Trace.flFraction < 1.0)
	{
		hit = true;
	}
	else
	{
#if !defined (CLIENT_DLL)
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &m_Trace);
		if (m_Trace.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance(m_Trace.pHit);

			if (pHit == NULL || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, m_Trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());

			hit = true;
		}
#endif
	}

	//if (hit)
	//too close		vecEnd = m_Trace.vecEndPos;
	if (hit == false)
	{
		switch (m_iSwing)
		{
		default:
		case 0: iAnim = CROWBAR_ATTACK1MISS; break;
		case 1: iAnim = CROWBAR_ATTACK2MISS; break;
		case 2: iAnim = CROWBAR_ATTACK3MISS; break;
		}
		ev_sound = 0;
		fvolbar = 1.0f;
		m_flNextPrimaryAttack =  m_flNextSecondaryAttack = GetNextAttackDelay(CROWBAR_SWING_INTERVAL);
	}
	else
	{
		switch (m_iSwing)
		{
		default:
		case 0: iAnim = CROWBAR_ATTACK1HIT; break;
		case 1: iAnim = CROWBAR_ATTACK2HIT; break;
		case 2: iAnim = CROWBAR_ATTACK3HIT; break;
		}
#if !defined (CLIENT_DLL)
		CBaseEntity *pEntity = CBaseEntity::Instance(m_Trace.pHit);
		if (pEntity)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(GetDamageAttacker(), pev->dmg, gpGlobals->v_forward, &m_Trace, DMG_CLUB);
			ApplyMultiDamage(this, GetDamageAttacker());
			if (pEntity->IsPushable())// XDM3037
			{
				pEntity->pev->velocity += gpGlobals->v_forward * pEntity->DamageForce(pev->dmg);
				//pEntity->pev->avelocity += (pEntity->Center() - vecEnd).Normalize() * force;// XDM3035c: +=
			}
			edamage = pEntity->pev->takedamage;

			if ((pEntity->IsMonster() || pEntity->IsHuman()) && pEntity->BloodColor() != DONT_BLEED)
			{
				CSoundEnt::InsertSound(bits_SOUND_COMBAT|bits_SOUND_MEAT, pev->origin, SMALL_EXPLOSION_VOLUME, CROWBAR_ATTACK_INTERVAL);
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				ev_sound = 1;
				fvolbar = 1.0f;
				//?m_trHit.pHit = NULL;
			}
			else
			{
				CSoundEnt::InsertSound(bits_SOUND_COMBAT|bits_SOUND_WORLD, pev->origin, SMALL_EXPLOSION_VOLUME, CROWBAR_ATTACK_INTERVAL);
				m_pPlayer->m_iWeaponVolume = CROWBAR_WALLHIT_VOLUME;
				ev_sound = 2;
				fvolbar = 0.5f;
				if (g_pGameRules == NULL || g_pGameRules->PlayTextureSounds())
				{
					vecEnd = gpGlobals->v_forward; vecEnd *= CROWBAR_HIT_DISTANCE*1.75f; vecEnd += vecSrc;// XDM3038c: trace further, a little hack to reliably identify the texture
					tex = TEXTURETYPE_Trace(pEntity, vecSrc, vecEnd);// XDM3038
				}
				else
					fvolbar = 1.0f;
			}
		}// there is no possible 'else' here
#endif
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(CROWBAR_ATTACK_INTERVAL);
		SetThink(&CWeaponCrowbar::Smack);
		SetNextThink(CROWBAR_HIT_DELAY);// XDM3038c
	}

	int flags = FEV_RELIABLE;
#if defined(CLIENT_WEAPONS)
	SetBits(flags, FEV_NOTHOST);
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0, m_Trace.vecEndPos, m_pPlayer->pev->angles, fvolbar, edamage, iAnim, tex, pev->body, ev_sound);

	if (m_iSwing >= 2)
		m_iSwing = 0;
	else
		++m_iSwing;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	//return fDidHit;
}

void CWeaponCrowbar::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(CROWBAR_IDLE, pev->body);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(8, 16);
}
