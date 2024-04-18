#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "globals.h"
#include "game.h"


//-----------------------------------------------------------------------------
// Purpose: Removes all mines owned by the specified player
// Input  : *pOwner - if NULL, ALL mines are removed
//-----------------------------------------------------------------------------
void DeactivateSatchels(CBasePlayer *pOwner)
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname(pEntity, SATCHEL_CLASSNAME)) != NULL)
	{
		//if (pEntity->IsProjectile())
		CSatchelCharge *pSatchel = (CSatchelCharge *)pEntity;
		if (pSatchel)
		{
			if (pOwner == NULL || pSatchel->m_hOwner == pOwner)// if specified pOwner == NULL, deactivate ALL satchels
				pSatchel->Deactivate(TRUE);// UNDONE: delay this a bit so the charge may detonate
		}
	}
}


LINK_ENTITY_TO_CLASS(monster_satchel, CSatchelCharge);

CSatchelCharge *CSatchelCharge::CreateSatchelCharge(const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fScale, float fDamage)
{
	CSatchelCharge *pNew = GetClassPtr((CSatchelCharge *)NULL, SATCHEL_CLASSNAME);
	if (pNew)
	{
		pNew->pev->origin = vecOrigin;
		pNew->pev->angles = vecAngles;
		pNew->pev->velocity = vecVelocity;
		pNew->pev->scale = fScale;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->pev->dmg = fDamage;// XDM3038c
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		//pNew->pev->velocity = vecVelocity;
	}
	return pNew;
}

// Deactivate - do whatever it is we do to an orphaned satchel when we don't want it in the world anymore.
void CSatchelCharge::Deactivate(bool disintegrate)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->deadflag = DEAD_DYING;// XDM3035
	SetUseNull();// XDM3038
	if (sv_overdrive.value > 0.0f)// XDM3035b
	{
		SetThink(&CGrenade::Detonate);
		SetNextThink(0);
	}
	else
	{
		SetThinkNull();

		if (disintegrate)
			Disintegrate();
		else
			Destroy();
	}
}

// SET_MODEL, UTIL_SetSize, UTIL_SetOrigin >> CBaseProjectile::Spawn()
void CSatchelCharge::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->mins.Set(-4, -4, -2);
	pev->maxs.Set(4, 4, 4);
	//pev->gravity = 0.5f;
	pev->friction = 0.8f;
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgSatchel;

	CBaseProjectile::Spawn();// XDM3037
	//SetModelCollisionBox();// XDM3038: try?
	m_fEntIgnoreTimeInterval = -1;// XDM3037: never reset
	pev->health = 1;
	pev->takedamage = DAMAGE_YES;
	pev->skin = SATCHEL_TEXGRP_ARMED;// XDM3038: this is an armed charge
	// ResetSequenceInfo();
	pev->sequence = 1;// XDM: world
	SetTouch(&CSatchelCharge::SatchelSlide);
	SetUse(&CSatchelCharge::SatchelUse);// Don't call DetonateUse everytime!
	SetThink(&CSatchelCharge::SatchelThink);
	SetNextThink(0.1);// XDM3038a
}

void CSatchelCharge::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/p_satchel.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
	PRECACHE_SOUND("weapons/satchel_detonate.wav");
	PRECACHE_SOUND("weapons/satchel_disarm.wav");
}

void CSatchelCharge::SatchelSlide(CBaseEntity *pOther)
{
	if (FBitSet(pev->flags, FL_ONGROUND) && pOther == m_hOwner && (gpGlobals->time - m_fLaunchTime) > 10.0f)
	{
		if (pOther->IsPlayer())// XDM: if the player left some charges in another level/somehow lost control
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pOther;
			if (pPlayer)// let him restore control by touching them
			{
				if (pPlayer->m_pActiveItem && pPlayer->m_pActiveItem->GetID() == WEAPON_SATCHEL)
				{
					CWeaponSatchel *pSatchel = (CWeaponSatchel *)pPlayer->m_pActiveItem;
					if (pSatchel->m_iChargeReady == 0)
						pSatchel->m_iChargeReady = 1;// let the player take control
				}
				/*else if (pPlayer->GetInventoryItem(WEAPON_SATCHEL) == NULL)//FIXME: player may lose WEAPON_SATCHEL!
				{
					must zero ammo! pPlayer->GiveNamedItem("weapon_satchel");
				}*/
			}
		}
		return;
	}

	//NO! May stuck in his feet!	pev->owner = NULL;// XDM3037: we touched something that is not our owner
	// pev->avelocity.Set(300, 300, 300);
	//pev->gravity = 1;// normal gravity now
	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0,0,10), ignore_monsters, edict(), &tr);

	if (tr.flFraction < 1.0)
	{
		// add a bit of static friction
		pev->velocity *= 0.95f;
		pev->avelocity *= 0.9f;
		// play sliding sound, volume based on velocity
		AlignToFloor();// XDM
	}

	if (!FBitSet(pev->flags, FL_ONGROUND) && pev->velocity.Length2D() > 10)
		BounceSound();

	StudioFrameAdvance();
}

void CSatchelCharge::SatchelThink(void)
{
	StudioFrameAdvance();
	SetNextThink(0.1);// XDM3038a

	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	if (pev->owner && FBitSet(pev->flags, FL_ONGROUND))// XDM3037: owner has gone far enough and is hopefully not touching me, forget him (become solid)
	{
		if ((pev->origin - pev->owner->v.origin).Length() > (pev->size.Length() + pev->owner->v.size.Length() + 4))
			SetIgnoreEnt(NULL);
	}

	if (pev->waterlevel == WATERLEVEL_HEAD)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity *= 0.8f;
		pev->avelocity *= 0.9f;
		pev->velocity.z += 8.0f;
	}
	else if (pev->waterlevel == WATERLEVEL_NONE)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}
}

void CSatchelCharge::BounceSound(void)
{
	AlignToFloor();// XDM: :D
	switch (RANDOM_LONG(0,2))
	{
	case 0:	EMIT_SOUND(edict(), CHAN_BODY, "weapons/g_bounce1.wav", RANDOM_FLOAT(0.8, 1.0), ATTN_STATIC); break;
	case 1:	EMIT_SOUND(edict(), CHAN_BODY, "weapons/g_bounce2.wav", RANDOM_FLOAT(0.8, 1.0), ATTN_STATIC); break;
	case 2:	EMIT_SOUND(edict(), CHAN_BODY, "weapons/g_bounce3.wav", RANDOM_FLOAT(0.8, 1.0), ATTN_STATIC); break;
	}
}

int CSatchelCharge::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (FBitSet(bitsDamageType, DMGM_BREAK))
	{
		SetUseNull();// XDM3038
		SetThink(&CGrenade::Detonate);
		SetNextThink(0);
		return 1;
	}
	return 0;
}

void CSatchelCharge::SatchelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)// XDM
{
	if (!pActivator->IsPlayer())
		return;

	if (pev->deadflag == DEAD_DYING)// XDM3035
		return;

	if (useType == USE_ON)// used by radio detonator
	{
		SetUseNull();// XDM3038: don't allow to be interrupted by subsequent calls
		SetThink(&CGrenade::Detonate);// XDM
		if (sv_satchel_delay.value > 0)// XDM3038
		{
			UTIL_Sparks(Center());// TODO: something better that doesn't require precaching
			CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.3);
			EMIT_SOUND(edict(), CHAN_VOICE, "weapons/satchel_detonate.wav", VOL_NORM, ATTN_NORM);
			pev->effects |= EF_MUZZLEFLASH;
		}
		SetNextThink(min(10, sv_satchel_delay.value));// XDM3038a
		pev->skin = SATCHEL_TEXGRP_DETONATE;// XDM3038
		if (sv_overdrive.value > 0.0f)
		{
			for (short i=0; i<3; ++i)
			{
				CLGrenade *pGrenade = CLGrenade::CreateGrenade(pev->origin + UTIL_RandomBloodVector()*2.0f, pev->angles, pev->velocity + UTIL_RandomBloodVector()*10.0f, m_hOwner, this, gSkillData.DmgGrenadeLauncher*0.8f, RANDOM_FLOAT(0.6f, 1.0f), false);
				if (pGrenade)
					pGrenade->pev->impulse = i;// randomize
			}
		}
	}
	else if (pActivator == m_hOwner)// XDM: used directly by the player
	{
		EMIT_SOUND(edict(), CHAN_BODY, "weapons/satchel_disarm.wav", VOL_NORM, ATTN_NORM);
		pev->health = 0;
		SetUseNull();// XDM3038
		SetThink(&CBaseEntity::SUB_Remove);// XDM3034: alert will be sent to it's owner
		// if (pActivator->IsPlayer()) {}
		Create("weapon_satchel", pev->origin, pev->angles, pev->velocity, NULL, SF_NORESPAWN);
		SetNextThink(0.01);// XDM3038a
	}
}

void CSatchelCharge::UpdateOnRemove(void)// XDM3034
{
	if (m_hOwner)
	{
		//CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
		if (m_hOwner->IsPlayer())// notify owner his charge was removed
		{
			CBasePlayer *pPlayer = (CBasePlayer *)(CBaseEntity *)m_hOwner;
			if (pPlayer)
			{
				CWeaponSatchel *pSatchelWeapon = (CWeaponSatchel *)pPlayer->GetInventoryItem(WEAPON_SATCHEL);
				if (pSatchelWeapon)
				{
					if (pSatchelWeapon->m_iClip >= 1)
						--pSatchelWeapon->m_iClip;
#if defined (_DEBUG)
					conprintf(1, "CSatchelCharge: removed itself from player %d (%d left)\n", pPlayer->entindex(), pSatchelWeapon->m_iClip);
#endif
				}
			}
		}
	}
	CGrenade::UpdateOnRemove();
}
