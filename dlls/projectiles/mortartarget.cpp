//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"
#include "skill.h"
#include "sound.h"
#include "soundent.h"

#define MTARGET_CALL_TIME		5.0f
#define MTARGET_PITCH_DELTA		5


class CMortarTarget : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	//virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int ObjectCaps(void) {return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;}

	void Ping(void);
	bool CheckSatellite(void);
	void EXPORT MTThink(void);// don't use just Think() - this cannot be disabled
	void EXPORT MTTouch(CBaseEntity *pOther);// same thing
};

LINK_ENTITY_TO_CLASS(strtarget, CMortarTarget);


void CMortarTarget::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;//DAMAGE_YES;
	pev->sequence = 1;// for all p_* models
	pev->friction = 0.8;
	pev->mins.Set(-4,-4,0);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(4,4,2);
	CBaseProjectile::Spawn();// XDM3037
	pev->health = 10;
	pev->button = PITCH_NORM;
	if (pev->dmg == 0)// XDM3038c
		pev->dmg	 = gSkillData.DmgStrikeTarget;//190 + (gSkillData.iSkillLevel * 10);

	pev->dmgtime	 = gpGlobals->time + MTARGET_CALL_TIME;
	pev->spawnflags |= SF_NORESPAWN;// XDM3035
	SetThink(&CMortarTarget::MTThink);
	SetTouch(&CMortarTarget::MTTouch);
	SetNextThink(0.1);
}

void CMortarTarget::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/p_target.mdl");// XDM3037

	CBaseProjectile::Precache();// skip CGrenade
	PRECACHE_SOUND("weapons/mortar.wav");
	PRECACHE_SOUND("weapons/mortar_call.wav");
	//pev->noise = MAKE_STRING("weapons/mortarhit.wav");
	//PRECACHE_SOUND(STRINGV(pev->noise));
	//UTIL_PrecacheOther("monster_mortar");
}

void CMortarTarget::MTTouch(CBaseEntity *pOther)
{
	if (pOther->IsBSPModel()/* && pev->flags & FL_ONGROUND*/)
	{
		AlignToFloor();//pev->angles.Clear();
		SetTouchNull();
	}
}

void CMortarTarget::MTThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		if (g_pGameRules && g_pGameRules->IsGameOver())// XDM3035c: don't call airstrike during intermission
		{
			Disintegrate();
			return;
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT|bits_SOUND_DANGER, pev->origin, NORMAL_EXPLOSION_VOLUME, 1.0f);// XDM3038c
		if (CheckSatellite())
		{
			EMIT_SOUND_DYN(edict(), CHAN_ITEM, "weapons/mortar.wav", VOL_NORM, ATTN_NONE, 0, RANDOM_LONG(95,120));

			Vector vecSpot;
			TraceResult tr;
			for (short i=0; i<3; ++i)
			{
				vecSpot = pev->origin + UTIL_RandomBloodVector()*256.0f;
				vecSpot.z = pev->origin.z + 2.0f;

				// find the sky
				UTIL_TraceLine(vecSpot, vecSpot + Vector(0,0,2048), ignore_monsters, edict(), &tr);
				pev->endpos = tr.vecEndPos;
				// trace down from 'endpos' to find the explosion position
				UTIL_TraceLine(pev->endpos, vecSpot - Vector(0,0,2048), ignore_monsters, edict(), &tr);

				CBaseEntity *pMortar = Create("monster_mortar", tr.vecEndPos, g_vecZero, m_hOwner.Get());
				if (pMortar)
				{
					pMortar->pev->dmg = pev->dmg;// XDM3038c
					pMortar->SetNextThink(2.0f + (float)i);// XDM3038a
				}
			}
		}
		/*else
		{
			char str[] = "Unable to connect to satellite!";
			if (pev->owner != NULL)
				ClientPrint(VARS(pev->owner), HUD_PRINTCENTER, str);
			else
				conprintf(0, str);
		}*/
		//Destroy();
		//DontThink();// XDM3038a

		if (sv_overdrive.value > 0.0f)
		{
			pev->dmg = gSkillData.DmgM203Grenade;
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0,0,4), ignore_monsters, ignore_glass, edict(), &tr);
			if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
				pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * 4.0f);

			//tr.vecPlaneNormal.x *= -1.0f;
			//tr.vecPlaneNormal.y *= -1.0f;
			VectorAngles(tr.vecPlaneNormal, pev->angles);// XDM3037a: explosion code calculates decal direction
			Explode(pev->origin, DMG_SHOCK|DMG_ENERGYBEAM|DMG_IGNITE, g_iModelIndexUExplo, 0, g_iModelIndexUExplo, 0, 0, "weapons/electro4.wav");
		}
		else
			Disintegrate();
	}
	else
	{
		if (FBitSet(pev->flags, FL_ONGROUND))
			pev->owner = NULL;// XDM3037

		Ping();
		SetNextThink(0.5);// XDM3038a
	}
}

/*int CMortarTarget::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType & DMGM_BREAK)
	{
		pev->takedamage = DAMAGE_NO;
		UTIL_Sparks(pev->origin);

		if (pev->button < (PITCH_NORM + MTARGET_PITCH_DELTA*2))
		{
			pev->impulse = 1;
			Smoke();
			//Destroy();
			//DontThink();// XDM3038a
		}
		else// imitation: too late to be destroyed (something needs to call mortar!)
			pev->effects |= EF_NODRAW;

		return 1;
	}
	return 0;
}*/

void CMortarTarget::Ping(void)
{
	if (CheckSatellite())
	{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
		WRITE_BYTE(TE_USERTRACER);
		WRITE_COORD(pev->origin.x);// pos
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(0);// vel
		WRITE_COORD(0);
		WRITE_COORD(3072.0f + pev->button*2.0f);
		WRITE_BYTE(15);// life

		if (sv_overdrive.value > 0.0f)
			WRITE_BYTE(RANDOM_LONG(0,11));
		else
			WRITE_BYTE(2);// green color "r_efx.h"

		WRITE_BYTE(30);// length
	MESSAGE_END();
	}
	if (FBitSet(pev->flags, FL_ONGROUND) && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
	{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
		WRITE_BYTE(TE_BEAMDISK);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 1.0f);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 24.0f);
		WRITE_SHORT(g_iModelIndexLaser);
		WRITE_BYTE(0);//ST
		WRITE_BYTE(100);//FPS
		WRITE_BYTE(18);//LF
		WRITE_BYTE(128);//WD
		WRITE_BYTE(0);//NS
		WRITE_BYTE(255);// R
		WRITE_BYTE(255);// G
		WRITE_BYTE(0);// B
		WRITE_BYTE(100);//A
		WRITE_BYTE(3);//SPD
	MESSAGE_END();
	}
	pev->effects |= EF_MUZZLEFLASH;
	EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/mortar_call.wav", VOL_NORM, ATTN_NORM, 0, pev->button);
	pev->button += MTARGET_PITCH_DELTA;
}

bool CMortarTarget::CheckSatellite(void)
{
	if (g_pWorld)
	{
		if (FBitSet(g_pWorld->pev->spawnflags, SF_WORLD_NOAIRSTRIKE))// XDM3038a
			return false;
	}
	TraceResult tr;
	Vector start(pev->origin);
	Vector end(start);
	start.z += 1.0f;
	end.z += g_psv_zmax->value;
	UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
	if (tr.pHit != NULL)
	{
		char cTextureType = CHAR_TEX_NULL;
		if (tr.pHit->v.solid == SOLID_BSP)
			cTextureType = TEXTURETYPE_Trace(CBaseEntity::Instance(tr.pHit), start, end);// XDM3038c

		if (cTextureType == CHAR_TEX_SKY)//if (tex != NULL && _stricmp(tex, "sky") == 0)// Texture name can be 'sky' or 'SKY' so use strIcmp!
			return true;
	}
	return false;
}

void CMortarTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator->IsPlayer())
		return;

	if (pActivator->edict() == m_hOwner.Get())
	{
		/*CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
		if (pActivator->GiveAmmo(1, "strtarget", STRTARGET_MAX_CARRY) > 0)
		{
			EMIT_SOUND(pActivator->edict(), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
			pPlayer->m_rgItems[ITEM_MORTAR] ++; We need a weapon! Not an ammo!*/
			Create("weapon_strtarget", pev->origin, pev->angles, pev->velocity, NULL, SF_NORESPAWN);
			Destroy();
		//}
	}
}
