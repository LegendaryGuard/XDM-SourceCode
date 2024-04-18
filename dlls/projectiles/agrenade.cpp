//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"


LINK_ENTITY_TO_CLASS(a_grenade, CAGrenade);

// UNDONE: attach to moving objects
// TODO: use vuser1, iuser2, iuser3 for these
TYPEDESCRIPTION	CAGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CAGrenade, m_hAiment, FIELD_EHANDLE),
	DEFINE_FIELD(CAGrenade, m_vecNormal, FIELD_VECTOR),
	DEFINE_FIELD(CAGrenade, m_iCount, FIELD_INTEGER),
	DEFINE_FIELD(CAGrenade, m_fTouched, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CAGrenade, CGrenade);


CAGrenade *CAGrenade::ShootTimed(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float time, float singledamage, int numSubgrenades)
{
	CAGrenade *pNew = GetClassPtr((CAGrenade *)NULL, "a_grenade");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		//pNew->pev->angles = vecAng;
		pNew->pev->velocity = vecVel;
		VectorAngles(vecVel, pNew->pev->angles);// XDM3038a
		pNew->pev->dmg = singledamage;// XDM3038c
		pNew->pev->dmgtime = gpGlobals->time + time;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;// XDM3037
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		if (numSubgrenades > 0)// XDM3035: was '1'
		{
			pNew->m_iCount = numSubgrenades;
			pNew->pev->scale = AGRENADE_NORMAL_SCALE + AGRENADE_SCALE_FACTOR * numSubgrenades;
			pNew->pev->dmg *= (1 + numSubgrenades);// in case the grenade hits/detonates before dissipating
		}
		else
			pNew->m_iCount = 0;
	}
	return pNew;
}


void CAGrenade::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_NO;
	//UTIL_SetOrigin(this, pev->origin);
	pev->mins.Set(-4,-4,-4);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(4,4,4);
	pev->renderfx = kRenderFxFullBright;// invisible in HL1111 kRenderFxFullBright;
	pev->rendermode = kRenderTransTexture;
	pev->rendercolor.Set(0,255,0);
	pev->renderamt = 191;
	pev->scale = AGRENADE_NORMAL_SCALE;
	pev->gravity *= 0.9f;// XDM3038
	pev->friction *= 2.0f;// XDM3038
	pev->flags |= FL_IMMUNE_SLIME;
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgAcidGrenade;

	CBaseProjectile::Spawn();// XDM3037
	//ResetSequenceInfo();
	m_fTouched = FALSE;
	m_vecNormal.Clear();
	//bMovable = FALSE;
	m_hAiment = NULL;
	//m_bloodColor = BLOOD_COLOR_GREEN;
	SetTouch(&CAGrenade::AcidTouch);
	SetThink(&CAGrenade::AcidThink);
	SetNextThink(0.1);// XDM3038a
}

void CAGrenade::Precache(void)
{
	if (FStringNull(pev->model))	
		pev->model = MAKE_STRING("models/agrenade.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	// XDM3038	PRECACHE_SOUND("weapons/agrenade_hit1.wav");
	// XDM3038	PRECACHE_SOUND("weapons/agrenade_blow.wav");
	m_usAGrenade = PRECACHE_EVENT(1, "events/fx/agrenade.sc");
}

bool CAGrenade::IsAttached(void)
{
	return (m_fTouched > 0);
}

void CAGrenade::AcidThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		Detonate();
		return;
	}

	if (sv_overdrive.value > 0.0f)// XDM3037: J4L
	{
		if (pev->scale < 4.0)
			pev->scale += 0.2;
	}

	if (pev->fuser4 == 0.0f)// XDM3035c: remember custom gravity
		pev->fuser4 = pev->gravity;

	if (pev->movetype == MOVETYPE_NONE)// XDM3034 touched a surface
	{
		if (pev->waterlevel >= WATERLEVEL_WAIST)
		{
			pev->scale += 0.05f;
			if (pev->renderamt > 10)
				pev->renderamt -= 2;
		}
	}
	else// haven't touched any surface
	{
		if (pev->waterlevel >= WATERLEVEL_WAIST)// float emulation
		{
			SetBits(pev->flags, FL_FLOAT);
			//pev->movetype = MOVETYPE_FLY;
			pev->velocity *= 0.4f;
			pev->avelocity *= 0.5f;
			pev->gravity = 0.01f;
			pev->velocity.z += pev->waterlevel*2.0f;
		}
		else// if (pev->waterlevel == WATERLEVEL_NONE)
		{
			ClearBits(pev->flags, FL_FLOAT);
			/*if (pev->movetype == MOVETYPE_FLY)// just exited water
			{
				pev->movetype = MOVETYPE_TOSS;// default
				//pev->velocity.z *= 0.5f;
				pev->velocity.z = 0.0f;
				pev->gravity = 0.1f;
			}
			else*/
				pev->gravity = pev->fuser4;// XDM3035c: reset to default or custom value

		}
	}

	//if (!IsAttached())
	//	conprintf(2, "CAGrenade pev->gravity = %g\n", pev->gravity);

	SetNextThink(0.2);// XDM3038a

	//if (pev->waterlevel != 0)
	//	pev->velocity *= 0.6f;

	//if (IsAttached())// grenade is attached to an entity
	//{
	// this ent may have died upon this monent and can cause crash!		if ((m_pAiment == NULL) || FNullEnt(m_pAiment->edict())/* || FNullEnt(pev->aiment)*/)
	//	Detonate();
		/*if (bMovable)// entity can move, update origin
		{
			//vecSpot = (m_pAiment->pev->origin - m_pAiment->pev->angles) - vecDelta;
			// UNDONE: angular move? If the aiment is moving?
			vecSpot = m_pAiment->pev->origin - vecDelta;
			UTIL_SetOrigin(this, vecSpot);
			pev->angles = m_pAiment->pev->angles;
		}*/
	//}

	/* NO! recursion!	if (!IsAttached() && sv_overdrive.value > 0.0f)
	{
		if (pev->teleport_time <= gpGlobals->time)
		{
			CAGrenade::ShootTimed(pev->origin + RandomVector(4.0f), g_vecZero, (RandomVector(VECTOR_CONE_45DEGREES)) * RANDOM_LONG(700, 900), Instance(pev->owner), pev->dmgtime - gpGlobals->time + RANDOM_FLOAT(1.0, 2.0), 0);
			pev->teleport_time = gpGlobals->time + 0.4f;
		}
	}*/
}

void CAGrenade::AcidTouch(CBaseEntity *pOther)
{
	// undone	if (pev->impulse && pOther == (CBaseEntity *)m_hAiment)
	//	return;

	if (pOther->IsProjectile() && FStrEq(pev->classname, pOther->pev->classname))//pOther->pev->modelindex == pev->modelindex)
		return;

	BOOL bMakeLight = TRUE;

	if (pOther->pev->takedamage > DAMAGE_NO)// XDM3035: half of damage is applied directly to living/destructable things
	{
		pev->dmg *= 0.5f;
		pOther->TakeDamage(this, GetDamageAttacker(), pev->dmg, DMG_ACID | DMG_DONT_BLEED);
	}
	//if (FBitSet(pev->groundentity->v.flags, FL_CONVEYOR))
	if (FBitSet(pOther->pev->flags, FL_CONVEYOR))// XDM3038a
	{
		Detonate();
		return;
	}

	if (pOther->IsMonster() || pOther->IsPlayer() || pOther->IsProjectile())
	{
		// XDM3035	if (pOther->IsAlive())// a little hack here
		// XDM3035		pev->dmg *= 0.8f;
		//	else
		//		pev->velocity.NormalizeSelf();

		Detonate();
		return;
	}
	else if (pOther->IsBSPModel())// worldspawn, func_wall, breakable, etc.
	{
		SetIgnoreEnt(NULL);// XDM3037: start colliding with everything
		m_fEntIgnoreTimeInterval = 0;// and the owner
		pev->movetype = MOVETYPE_NONE;
		if (!FClassnameIs(pOther->pev, "worldspawn"))// because world is 0th entity
		{
			m_fTouched = TRUE;
			m_hAiment = pOther;
			//pev->aiment = pOther->edict();
		}
		if (pOther->IsMovingBSP() || pOther->IsBreakable())// pushable, door, button, etc. XDM3034: breakables may be destroyed!
		{
			/*vecDelta = pOther->pev->origin - pev->origin;
			pev->solid = SOLID_NOT;
			pev->aiment = pOther->edict();// ?
			pev->movetype = MOVETYPE_FOLLOW;
			bMovable = TRUE;*/
			if (pOther->IsMoving())
				bMakeLight = FALSE;// don't make light, because it will not follow me

			Detonate();
			return;
		}
		else
		{
			//pev->solid = SOLID_BBOX;
			pev->takedamage = DAMAGE_YES;
			pev->solid = SOLID_SLIDEBOX;// XDM3038a: crash prevention
			// XDM3035	UTIL_SetOrigin(this, pev->origin);
			//	UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));
			// XDM3035	UTIL_SetSize(this, 8.0f);
		}
	}
	else return;

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity.Normalize() * 8.0f, dont_ignore_monsters, edict(), &tr);
	m_vecNormal = tr.vecPlaneNormal;
	pev->origin += m_vecNormal*2.0f;// XDM3035: pull back 2 units to prevent hurting people behind thin glass
	//CL	UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0,1));
	//EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/agrenade_hit1.wav", 0.8, ATTN_NORM, 0, 100 + RANDOM_FLOAT(0,5));
	//UTIL_DebugBeam(pev->origin, pev->origin+tr.vecPlaneNormal*32.0f, 3, 255,0,255);
	VectorAngles(m_vecNormal, pev->angles);// XDM3038c: transmit angles
	//UTIL_DebugAngles(pev->origin, pev->angles, 2, 16);
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(this, 2.0f);// XDM3035: so it won't be triggered through a thin wall
	PLAYBACK_EVENT_FULL(0, edict(), m_usAGrenade, 0.0, pev->origin, pev->angles/*m_vecNormal*/, pev->dmgtime - gpGlobals->time, (m_iCount > 0)?0.2:0.1, g_iModelIndexAcidDrip, 0/*not usedpOther->entindex()*/, bMakeLight, AGRENADE_EV_TOUCH);

	if (pev->renderfx == kRenderFxNone)
		pev->renderfx = kRenderFxPulseFast;

	if (m_iCount > 0)
	{
		SetThink(&CAGrenade::DissociateThink);
		SetNextThink(0);
	}
}

void CAGrenade::DissociateThink(void)
{
	if (m_iCount <= 0)// 1 because we already have one!
	{
		pev->renderfx = kRenderFxFullBright;// reset back
		SetThink(&CAGrenade::AcidThink);// continue thinkin'
		SetNextThink(0);
		return;
	}

	if (sv_overdrive.value > 0.0f)
		pev->renderfx = kRenderFxExplode;

	// totaldamage = (1 + m_iCount)*singledamage
	float damage = pev->dmg/(1 + m_iCount);
	CAGrenade *pPart = CAGrenade::ShootTimed(pev->origin + m_vecNormal * RANDOM_FLOAT(3.0, 5.0) + RandomVector(), g_vecZero, (m_vecNormal + RandomVector(VECTOR_CONE_45DEGREES)) * RANDOM_FLOAT(200, 300), m_hOwner, this, pev->dmgtime - gpGlobals->time + RANDOM_FLOAT(1.0, 2.0), damage, 0);// c
	if (pPart)
	{
		VectorRandom(pPart->pev->avelocity, -10, 10);
		pev->scale -= AGRENADE_SCALE_FACTOR;
		if (pev->scale < 0.5f)
			pev->scale = 0.5f;

		pev->dmg -= damage;//gSkillData.DmgAcidGrenade;
		--m_iCount;
	}
	SetNextThink(0.1);// XDM3038a
}

int CAGrenade::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (FBitSet(bitsDamageType, DMGM_BREAK|DMGM_FIRE))
		Detonate();

	return 1;
}

void CAGrenade::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	Detonate();
}

void CAGrenade::Detonate(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	SetTouchNull();

	if (IsAttached())// fixed
	{
		pev->origin += m_vecNormal*2.0f;
		// DEBUG	GlowSprite(pev->origin, g_iModelIndexColdball2, 20, 10, 200);
	}

	pev->owner = NULL;// XDM3037
	::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg, 32.0f + pev->dmg, CLASS_NONE, DMG_NEVERGIB | DMG_ACID | DMG_DONT_BLEED/* | DMG_RADIUS_MAX*/);
	//EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/agrenade_blow.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_FLOAT(95,105));
	PLAYBACK_EVENT_FULL(0, edict(), m_usAGrenade, 0, pev->origin, pev->angles, 1, 1, 0, 0, 0, AGRENADE_EV_HIT);
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0.01);// XDM3038a
	// XDM3038: unsafe	Destroy();
}
