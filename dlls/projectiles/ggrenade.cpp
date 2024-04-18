#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "projectile.h"
#include "game.h"
#include "gamerules.h"
#include "skill.h"
#include "globals.h"
#include "soundent.h"
#include "decals.h"
#include "effects.h"
#include "shake.h"

// Spawnflags are in "weapondef.h"

LINK_ENTITY_TO_CLASS(grenade, CGrenade);

//-----------------------------------------------------------------------------
// Purpose: Create a handgrenade properly
// Input  : &vecStart - ...
// Output : CGrenade *
//-----------------------------------------------------------------------------
CGrenade *CGrenade::ShootTimed(const Vector &vecStart, const Vector &vecVelocity, float time, int type, CBaseEntity *pOwner, CBaseEntity *pEntIgnore)
{
	CGrenade *pGrenade = GetClassPtr((CGrenade *)NULL, "grenade");
	if (pGrenade == NULL)
		return NULL;

	pGrenade->pev->origin = vecStart;
	SetBits(pGrenade->pev->spawnflags, SF_NORESPAWN);// XDM3035b
	pGrenade->pev->model = MAKE_STRING("models/p_grenade.mdl");// XDM3037
	if (pOwner)
	{
		pGrenade->m_hOwner = pOwner;
		pGrenade->pev->team = pOwner->pev->team;// XDM3037
	}
	pGrenade->SetIgnoreEnt(pEntIgnore);// XDM3037
	switch (type)
	{
		default: conprintf(1, "CGrenade: invalid type (%d)! Using default.\n", type);
		type = GREN_EXPLOSIVE;
		case GREN_EXPLOSIVE:
			{
				pGrenade->SetThink(&CGrenade::TumbleThink);
				pGrenade->pev->dmg = gSkillData.DmgHandGrenadeExplosive;
				pGrenade->pev->takedamage = DAMAGE_YES;
			}
			break;
		case GREN_FREEZE:
			{
				pGrenade->SetThink(&CGrenade::FreezeThink);
				pGrenade->pev->dmg = gSkillData.DmgHandGrenadeFreeze;
			}
			break;
		case GREN_POISON:
			{
				pGrenade->SetThink(&CGrenade::PoisonThink);
				pGrenade->pev->dmg = gSkillData.DmgHandGrenadePoison;
			}
			break;
		case GREN_NAPALM:
			{
				pGrenade->SetThink(&CGrenade::BurnThink);
				pGrenade->pev->dmg = gSkillData.DmgHandGrenadeBurn;
			}
			break;
		case GREN_RADIOACTIVE:
			{
				pGrenade->SetThink(&CGrenade::RadiationThink);
				pGrenade->pev->dmg = gSkillData.DmgHandGrenadeRadiation;
			}
			break;
		/*case GREN_TELEPORTER:
			{
				pGrenade->SetThink(&CGrenade::TeleportationThink);
				pGrenade->pev->dmg = gSkillData.DmgHandGrenadeGravity;
			}
			break;*/
	}
	pGrenade->pev->skin = type;// XDM: p_, v_ and w_ models skins must match this!
	pGrenade->Spawn();
	pGrenade->pev->scale = UTIL_GetWeaponWorldScale();// XDM3038a
	//UTIL_SetOrigin(pGrenade, vecStart);
	// XDM3038	pGrenade->pev->gravity = 0.75f;
	pGrenade->pev->friction *= 0.8f;
	pGrenade->pev->velocity = vecVelocity;
	//pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	VectorAngles(pGrenade->pev->velocity, pGrenade->pev->angles);
	//if (sv_overdrive.value > 0.0f)
	//	pGrenade->pev->movetype = MOVETYPE_BOUNCEMISSILE;

	pGrenade->SetTouch(&CGrenade::BounceTouch);	// Bounce if touched
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	if (time < 0.1f)
	{
		pGrenade->SetNextThink(0);
		pGrenade->pev->velocity.Clear();
	}
	else
		pGrenade->SetNextThink(0.1);// XDM3038a

	pGrenade->pev->sequence = pGrenade->LookupActivity(ACT_FALL);
	//pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1.0f;
	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;
	pGrenade->SetBodygroup(GRENBG_RING, 1);// XDM: without ring
	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: Used as inline for subclasses AFTER customizing some parameters
//-----------------------------------------------------------------------------
void CGrenade::Spawn(void)
{
	//Never! This may get called from a subclass!	pev->classname = MAKE_STRING("grenade");
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	//pev->takedamage = DAMAGE_NO; DO NOT set here!
	pev->impulse = 0;// XDM
	if (pev->dmg == 0.0f && !FBitSet(pev->spawnflags, SF_NOREFERENCE))// XDM3038c: ignore runtime?
		conprintf(2, "%s[%d] without damage!\n", STRING(pev->classname), entindex());//pev->dmg = 100.0f;?

	m_fRegisteredSound = FALSE;
	pev->mins.Set(-1,-1,-1);// XDM3037a: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(1,1,1);
	CBaseProjectile::Spawn();// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: Precache common grenade resources
//-----------------------------------------------------------------------------
void CGrenade::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/testsphere.mdl");// XDM3038c

	CBaseProjectile::Precache();// XDM3038a
	PRECACHE_MODEL(GREN_TELEPORT_SPRITE);
	PRECACHE_MODEL(GREN_RADIATION_SPRITE);
	PRECACHE_SOUND("ambience/flameburst1.wav");// fire
	PRECACHE_SOUND("ambience/particle_suck1.wav");// radiation
	//PRECACHE_SOUND("ambience/port_suckin1.wav");
	PRECACHE_SOUND("weapons/explode_nuc.wav");
	PRECACHE_SOUND("weapons/explode_uw.wav");// underwater explosion
	PRECACHE_SOUND("weapons/fgrenade_exp1.wav");
	PRECACHE_SOUND("weapons/fgrenade_exp2.wav");// underwater
	PRECACHE_SOUND("weapons/pgrenade_acid.wav");// poison
	PRECACHE_SOUND("weapons/pgrenade_exp1.wav");
	PRECACHE_SOUND("weapons/pgrenade_exp2.wav");// underwater
	PRECACHE_SOUND("weapons/debris1.wav");// explosion aftermaths
	PRECACHE_SOUND("weapons/debris2.wav");
	PRECACHE_SOUND("weapons/debris3.wav");
	PRECACHE_SOUND("weapons/grenade_hit1.wav");// grenade
	PRECACHE_SOUND("weapons/grenade_hit2.wav");
	PRECACHE_SOUND("weapons/grenade_hit3.wav");
}

//-----------------------------------------------------------------------------
// Purpose: The main explosion function throughout the game
// Note   : Custom explosion sound is taken from pev->noise
// Warning: use vecOrigin, not pev->origin inside!
//-----------------------------------------------------------------------------
void CGrenade::Explode(const Vector &vecOrigin, int bitsDamageType, int spr, int fps, int spr_uw, int fps_uw, float scale, const char *customsound)
{
	//ASSERT(pTrace != NULL);
	SetIgnoreEnt(NULL);// XDM3037 //pev->owner = NULL;// can't traceline attack owner if this is set
	if (!CheckContents(vecOrigin))// XDM3038c: this also sets pev->waterlevel // same if (pTrace->fAllSolid)
	{
		// may be stuck inside exploding breakable which should be pEntIgnore		Destroy();
		//return;
		TraceResult tr;
		SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3035c: decorative, simplify
		UTIL_TraceLine(vecOrigin, vecOrigin + Vector(0,0,1), ignore_monsters, edict(), &tr);
		ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
		if (tr.pHit)
		{
			if (g_pWorld && tr.pHit == g_pWorld->edict())// XDM3038c: no chance to pull out
			{
				conprintf(2, "%s[%d]::Explode(%g %g %g) failed: inside world!\n", STRING(pev->classname), entindex(), vecOrigin.x, vecOrigin.y, vecOrigin.z);
				// BAD: no explosion! return;
			}
			else
			{
				conprintf(2, "%s[%d]::Explode() warning: inside %s[%d]!\n", STRING(pev->classname), entindex(), STRING(tr.pHit->v.classname), ENTINDEX(tr.pHit));
				SetIgnoreEnt(Instance(tr.pHit));
			}
		}
		else
			conprintf(2, "%s[%d]::Explode() warning: inside solid!\n", STRING(pev->classname), entindex());
	}

	int iSpriteIndex = spr;
	int iSpriteFPS = 16;
	int iDebrisSoundIndex = 0;

	//float flRndSound;// sound randomizer
	pev->model = iStringNull;//invisible
	pev->movetype = MOVETYPE_NONE;// XDM
	pev->solid = SOLID_NOT;// intangible
	pev->effects |= EF_NODRAW;
	pev->takedamage	= DAMAGE_NO;// IMPORTANT!!! prevents recursion
	m_fEntIgnoreTimeInterval = 0;// XDM3037: so RadiusDamage() trace hits everything
	UTIL_SetSize(this, g_vecZero, g_vecZero);// XDM3037

	if (scale <= 0.0f)// if scale is 0, calculate the default value
	{
		if (pev->dmg > 0.0f)
			scale = pev->dmg * 0.018f;
		else
			scale = 1.0f;

		scale += RANDOM_FLOAT(-0.02, 0.02);
	}

	float radius = pev->dmg * GREN_DAMAGE_TO_RADIUS;
	CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037

	/*if (pTrace->flFraction < 1.0f)// Pull out of the wall a bit
		vecOrigin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 4.0f);// XDM3037
	//pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * pev->dmg * 0.4f);

	/* TODO:
	MESSAGE_BEGIN(MSG_PVS, gmsgExplosion, vecOrigin);
		WRITE_COORD(vecOrigin.x);
		WRITE_COORD(vecOrigin.y);
		WRITE_COORD(vecOrigin.z);
		WRITE_SHORT(ceil(dmg));
		WRITE_SHORT(iSpriteIndexExplosion);
		WRITE_SHORT(iSpriteIndexGlow);
		WRITE_SHORT(iSpriteIndexParticles1);
		WRITE_SHORT(iSpriteIndexParticles2);
		WRITE_SHORT(iSpriteIndexShockwave);
		WRITE_SHORT(iDecalIndex);
		WRITE_BYTE(scale*10.0f);
		WRITE_BYTE(fps);
		WRITE_BYTE(sound_id);
		WRITE_BYTE(pev->waterlevel);
		WRITE_BYTE(expflags);
	MESSAGE_END();*/

	if (!FBitSet(pev->spawnflags, SF_NOFIREBALL))
	{
		if (pev->waterlevel > WATERLEVEL_NONE)
		{
			iSpriteIndex = spr_uw;

			if (fps_uw <= 0)
				fps_uw = (int)(MODEL_FRAMES(spr_uw) * 0.75f);

			iSpriteFPS = fps_uw;
		}
		else
		{
			if (fps <= 0)
				fps = (int)(MODEL_FRAMES(spr) * 0.75f);

			iSpriteFPS = fps;
		}

		if (iSpriteFPS > 2)// XDM3038a
			iSpriteFPS += RANDOM_LONG(-2,2);

		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// LAGLAG
			PLAYBACK_EVENT_FULL(0, edict(), g_usGrenExp, 0.0f, vecOrigin, pev->angles, pev->dmg, scale, iSpriteIndex, (int)(10*iSpriteFPS), GREN_EXPLOSIVE, pev->waterlevel);
	}// no fireball

	if (!FBitSet(pev->spawnflags, SF_NOSOUND))
	{
		if (pev->waterlevel > WATERLEVEL_NONE)
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/explode_uw.wav", VOL_NORM, 0.6f, 0, RANDOM_LONG(95, 105));
		else
		{
			if (customsound)
				EMIT_SOUND_DYN(edict(), CHAN_BODY, customsound, VOL_NORM, 0.3f, 0, RANDOM_LONG(95, 105));
			else
			{
				char sample[32];
				_snprintf(sample, 32, "weapons/explode%d.wav\0", RANDOM_LONG(3,5));// explode3.wav ...5
				sample[31] = '\0';
				EMIT_SOUND_DYN(edict(), CHAN_BODY, sample, VOL_NORM, 0.3f, 0, RANDOM_LONG(95, 105));
			}
		}
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, vecOrigin, NORMAL_EXPLOSION_VOLUME + (int)pev->dmg, 1.0f);
	}

	//if (!FBitSet(pev->spawnflags, SF_NODECAL))// XDM3035b: in event
	//	UTIL_DecalTrace(pTrace, DECAL_SCORCH1 + RANDOM_LONG(0,2));

	if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
	{
		//if (FBitSet(pev->flags, FL_ONGROUND))// XDM: for RadiusDamage
		//	pev->origin.z += (2 + pev->size.z);

		if (pev->waterlevel > WATERLEVEL_NONE)
		{
			::RadiusDamage(vecOrigin, this, pAttacker, pev->dmg, radius*0.75f, CLASS_NONE, bitsDamageType);
			UTIL_ScreenShake(vecOrigin, pev->dmg*0.4f, 0.8f, 1.6f, radius*2.0f);
		}
		else
		{
			::RadiusDamage(vecOrigin, this, pAttacker, pev->dmg, radius, CLASS_NONE, bitsDamageType);
			UTIL_ScreenShake(vecOrigin, pev->dmg*0.2f, 0.6f, 1.2f, radius*2.0f);
		}
		iDebrisSoundIndex = RANDOM_LONG(0,2);// must be same for all clients
	}

	pev->velocity.Clear();
	//pev->impulse = 0;// XDM: for smoke

	//if (pOwner)// XDM3034: IMPORTANT! Restore owner, satchels need this in UpdateOnRemove()
	//	pev->owner = pOwner->edict();

	if (!FBitSet(pev->spawnflags, SF_NOFIREBALL) || !FBitSet(pev->spawnflags, SF_NOPARTICLES))
	{
		if (pev->angles.IsZero())// for decals
			pev->angles.Set(-90,0,0);// face down // potential SQB here

		// XDM3037a: only first 16 bits of flags can be transmitted. Other flags will corrupt the data!
		PLAYBACK_EVENT_FULL(0, edict(), g_usExplosion, 0.0f, vecOrigin, pev->angles, pev->dmg, pev->spawnflags & SFM_EXPLOSION_FLAGS, /*g_iModelIndexZeroGlow*/0, g_iModelIndexZeroParts, iDebrisSoundIndex, pev->waterlevel);
	}

	/*test	Vector forward, vecEnd;
	AngleVectors(pev->angles, forward, NULL, NULL);
	vecEnd = vecOrigin + forward*(radius*0.25f);
	BeamEffect(TE_BEAMPOINTS, vecOrigin, vecEnd, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 4, 0, Vector(0,255,0), 255, 0);*/
	if (FBitSet(pev->spawnflags, SF_REPEATABLE))
	{
		SetThinkNull();
		DontThink();// XDM3037
	}
	else
	{
		m_iExistenceState = ENTITY_EXSTATE_VOID;// ?
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.1);// XDM3038a: delay a little so effect messages may pass to clients
	}
}

//-----------------------------------------------------------------------------
// Purpose: Grenade is taking damage
//-----------------------------------------------------------------------------
int CGrenade::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->takedamage != DAMAGE_NO)
	{
		if (pev->skin == GREN_EXPLOSIVE && (bitsDamageType & DMGM_BREAK))
		{
			Killed(pInflictor, pAttacker, GIB_NORMAL);// XDM3037: fix
			return 1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destroyed by something
// Input  : *pInflictor - weapon/projectile
//			*pAttacker - weapon owner
//			iGib - GIBMODE
//-----------------------------------------------------------------------------
void CGrenade::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	Detonate();
	//SetThink(&CGrenade::Detonate);
	//SetNextThink(0);
}

//-----------------------------------------------------------------------------
// Purpose: Think function to delay detonation
//-----------------------------------------------------------------------------
void CGrenade::Detonate(void)
{
	TraceResult tr;
	//Vector vecSpot;// trace starts here!
	//vecSpot = pev->origin + Vector(0,0,6);
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3035c: decorative, simplify
	UTIL_TraceLine(pev->origin + Vector(0,0,6), pev->origin - Vector(0,0,42), ignore_monsters, edict(), &tr);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (pev->dmg * GREN_DAMAGE_TO_RADIUS * 0.1f));

	//tr.vecPlaneNormal.x *= -1.0f;// SQB ?
	//tr.vecPlaneNormal.y *= -1.0f;
	VectorAngles(tr.vecPlaneNormal, pev->angles);// XDM3037a: explosion code calculates decal direction
	Explode(pev->origin, DMG_BLAST, g_iModelIndexFireball, 0, g_iModelIndexWExplosion, 0, 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Contact grenade, explode when it touches something
//-----------------------------------------------------------------------------
void CGrenade::ExplodeTouch(CBaseEntity *pOther)
{
	//if (pOther == m_hOwner && m_fLaunchTime < (gpGlobals->time + 0.25f))// XDM3037
	//	return;

	TraceResult tr;
	Vector vecSpot;
	pev->enemy = pOther->edict();
	vecSpot = pev->origin - pev->velocity.Normalize() * 8.0f;
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3035c: decorative, simplify
	UTIL_TraceLine(vecSpot, pev->origin + pev->velocity.Normalize() * 64.0f, ignore_monsters, edict(), &tr);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	//StreakSplash(pev->origin, tr.vecPlaneNormal, 9, 64, 64, 300);// XDM
	SetBits(pev->spawnflags, SF_NOSPARKS|SF_NOPARTICLES);

	if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (pev->dmg * GREN_DAMAGE_TO_RADIUS * 0.1f));

	//tr.vecPlaneNormal.x *= -1.0f;// SQB ?
	//tr.vecPlaneNormal.y *= -1.0f;
	//SQB?VectorAngles(tr.vecPlaneNormal, pev->angles);// XDM3037a: explosion code calculates decal direction
	Explode(pev->origin, DMG_BLAST, g_iModelIndexBigExplo4, 16, g_iModelIndexWExplosion2, 24, 1.5f);// XDM
}

//-----------------------------------------------------------------------------
// Purpose: Emit virtual "danger" sound hearable by monsters
//-----------------------------------------------------------------------------
void CGrenade::DangerSoundThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}
	if (!m_fRegisteredSound)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5f, (int)pev->velocity.Length(), 0.2f);
		m_fRegisteredSound = TRUE;
	}
	SetNextThink(0.2);// XDM3038a

	if (pev->waterlevel > WATERLEVEL_NONE)
		pev->velocity *= 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: Emulate physical interaction, roll and make bounce sounds
//-----------------------------------------------------------------------------
void CGrenade::BounceTouch(CBaseEntity *pOther)
{
	// XDM3037: if (pOther->edict() == pev->owner)
	//		return;

	// only do damage if we're moving fairly fast
	if (pev->impacttime < gpGlobals->time && fabs(pev->velocity.Volume()) > 1000.0f)// faster than pev->velocity.Length() > 100)
	{
		if (pOther != g_pWorld)
		{
			TraceResult tr;
			UTIL_GetGlobalTrace(&tr);
			ClearMultiDamage();
			pOther->TraceAttack(GetDamageAttacker(), 1.0f*(1.0f+sv_overdrive.value), gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB);// XDM3038b: :)
			ApplyMultiDamage(this, GetDamageAttacker());
		}
		pev->impacttime = gpGlobals->time + 1.0f;// debounce
	}

	Vector vecTestVelocity(pev->velocity);
	// pev->avelocity.Set(300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	//vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45f;

	if (!m_fRegisteredSound && vecTestVelocity.Length() <= 60)
	{
		//conprintf(1, "Grenade Registered!: %f\n", vecTestVelocity.Length());
		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving.
		// go ahead and emit the danger sound.
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, (int)(pev->dmg*2.5f), 0.3f);
		m_fRegisteredSound = TRUE;
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		// add a bit of static friction
		pev->velocity *= 0.8f;
		pev->sequence = LookupActivity(ACT_WALK);//RANDOM_LONG(1, 2);
	}
	else
		BounceSound();

	pev->framerate = pev->velocity.Length() / 200.0f;

	if (pev->framerate > 1.0f)
		pev->framerate = 1.0f;
	else if (pev->framerate < 0.0f)
		pev->framerate = 0.0f;
	//else if (pev->framerate < 0.5f)
	//	pev->framerate = 0.0f;
}

/* unused
void CGrenade::SlideTouch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	//pev->avelocity.Set(300, 300, 300);
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		// add a bit of static friction
		pev->velocity *= 0.95f;
		//if (pev->velocity.x != 0 || pev->velocity.y != 0)
			// maintain sliding sound
	}
	else
		BounceSound();
}*/

//-----------------------------------------------------------------------------
// Purpose: Overloadable
//-----------------------------------------------------------------------------
void CGrenade::BounceSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND(edict(), CHAN_VOICE, "weapons/grenade_hit1.wav", RANDOM_FLOAT(0.4, 0.6), ATTN_STATIC); break;
	case 1:	EMIT_SOUND(edict(), CHAN_VOICE, "weapons/grenade_hit2.wav", RANDOM_FLOAT(0.4, 0.6), ATTN_STATIC); break;
	case 2:	EMIT_SOUND(edict(), CHAN_VOICE, "weapons/grenade_hit3.wav", RANDOM_FLOAT(0.4, 0.6), ATTN_STATIC); break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if point is in liquid or solid space, updates pev->waterlevel
// Input  : vecOrigin - point in world
// Output : Returns true on success, false if vecOrigin is in solid
//-----------------------------------------------------------------------------
bool CGrenade::CheckContents(const Vector &vecOrigin)// XDM3038c
{
	int pc = UTIL_PointContents(vecOrigin);
	if (pc == CONTENTS_SOLID)
	{
		//Destroy(); not here
		return false;
	}
	else if (pc < CONTENTS_SOLID && pc > CONTENTS_SKY)
		pev->waterlevel = WATERLEVEL_HEAD;
	else
		pev->waterlevel = WATERLEVEL_NONE;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Delayed detonation
//-----------------------------------------------------------------------------
void CGrenade::TumbleThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	StudioFrameAdvance();
	SetNextThink(0.1);// XDM3038a

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	else if (!m_fRegisteredSound && pev->dmgtime - 1.0f < gpGlobals->time)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), NORMAL_GUN_VOLUME, 0.2);
		m_fRegisteredSound = TRUE;// XDM3038a: don't generate 10 sounds per second!
	}
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		pev->velocity *= 0.5f;
		pev->framerate = 0.2f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do some damage after the projectile has exploded (e.g. gas, acid, radiation)
// Input  : &life - life in seconds
//			&updatetime - RadiusDamage() intervals
//			&damagedelta - this value is added to pev->dmg after each interval
//			&bitsDamageType - DMG_GENERIC
// XDM3038b: difference:
// 1) previously we used existing pev->dmg and divided it by 10, now we deal 100% of damage specified in the argument
// 2) delta is calculated on per-second basis, not per-cycle
//-----------------------------------------------------------------------------
void CGrenade::DoDamageInit(const float &life, const float &updatetime, const float &damage, const float &damagedelta, const float &radius, const float &radiusdelta, int bitsDamageType)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	pev->takedamage = DAMAGE_NO;// !!
	pev->button = bitsDamageType;
	pev->owner = NULL;// XDM3037: for RadiusDamage()
	pev->dmg = damage;
	pev->dmg_save = damagedelta;
	pev->dmgtime = gpGlobals->time + life;
	pev->fuser1 = radius;
	pev->fuser2 = radiusdelta;
	pev->fuser3 = updatetime;// XDM3038: conflicts
	SetThink(&CGrenade::DoDamageThink);
	SetNextThink(0);
}

//-----------------------------------------------------------------------------
// Purpose: The process of making damage after disappearing
//-----------------------------------------------------------------------------
void CGrenade::DoDamageThink(void)
{
	if (pev->dmgtime <= gpGlobals->time)
	{
		if (FBitSet(pev->spawnflags, SF_REPEATABLE))// TESTME!!!
		{
			SetThink(&CBaseEntity::SUB_DoNothing);
			SetNextThink(1.0f);
		}
		else
		{
			Destroy();
			return;
		}
	}

	if (pev->waterlevel > WATERLEVEL_NONE && pev->dmg > 0.5f && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()) && RANDOM_LONG(0,1)==0)
		FX_BubblesPoint(pev->origin, VECTOR_CONE_20DEGREES, max(2, min((int)(pev->dmg*0.5f), 64)));
		//UTIL_Bubbles(pev->origin - Vector(8,8,8), pev->origin + Vector(8,8,8), max(2, min((int)(pev->dmg*0.5f), 64)));

	if (FBitSet(pev->button, DMG_RADIATION))// update geiger counter
	{
		//edict_t *pentPlayer = FIND_CLIENT_IN_PVS(edict());
		//if (!FNullEnt(pentPlayer))
		//	CBasePlayer *pPlayer = NULL;
		//	pPlayer = GetClassPtr((CBasePlayer *)VARS(pentPlayer));

		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)// only players need this.
		{
			CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
			if (pPlayer)
			{
				vec_t flRange = (pev->origin - pPlayer->pev->origin).Length();// (Center() - pPlayer->Center()).Length(); origin is enough (and faster)
				if (flRange <= pev->fuser1)// in radius
				{
					if (pPlayer->m_flgeigerRange > flRange)
						pPlayer->m_flgeigerRange = flRange;
				}
			}
		}
	}

	/* XDM3038b: rely on client effects	if (pev->button & (DMG_POISON | DMG_ACID))// emit some clouds
	{
		if (g_pGameRules->FAllowEffects() && RANDOM_LONG(0,2)>1)// 1 of 3
		{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_SPRITE);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z);
				WRITE_SHORT(g_iModelIndexAcidPuff2);
				WRITE_BYTE(max(10, min(245, (int)pev->dmg)+RANDOM_LONG(-10,10)));// scale in 0.1's
				WRITE_BYTE(15+RANDOM_LONG(0,8));// brightness
			MESSAGE_END();
		}
	}*/
	if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
		::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg/*XDM3038b * 0.1f*/, pev->fuser1, CLASS_NONE, pev->button);

	pev->dmg += pev->dmg_save*pev->fuser3;// XDM3038b: these are now in units per second
	pev->fuser1 += pev->fuser2*pev->fuser3;// per-second basis
	if (pev->dmg_save < 0.0f && pev->dmg < 0.0f)// stop subtracting damage
	{
		pev->dmg = 0;
		pev->dmg_save = 0;
	}
	SetNextThink(pev->fuser3);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Delay freezing explosion
//-----------------------------------------------------------------------------
void CGrenade::FreezeThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	StudioFrameAdvance();
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		pev->velocity *= 0.5f;
		pev->framerate = 0.2f;
	}

	if (!m_fRegisteredSound && (pev->dmgtime - 2.0f < gpGlobals->time))
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1f);
		m_fRegisteredSound = TRUE;
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		pev->impulse = 0;
		SetThink(&CGrenade::FreezeThinkEnd);
	}

	SetNextThink(0.1);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Freezing explosion
//-----------------------------------------------------------------------------
void CGrenade::FreezeThinkEnd(void)
{
	float radius = pev->dmg * GREN_DAMAGE_TO_RADIUS_FREEZE;
	Vector vecSpot(pev->origin);
	if (FBitSet(pev->flags, FL_ONGROUND))// XDM: for RadiusDamage
		vecSpot.z += 2.0f;

	pev->owner = NULL;// XDM3037
	if (!CheckContents(vecSpot))
	{
		Destroy();
		return;
	}

	BeamEffect(TE_BEAMCYLINDER, pev->origin + Vector(0,0,32), pev->origin + Vector(0,20,460), g_iModelIndexShockWave, 0, 0, 5, 32, 0, Vector(100,160,255), (pev->waterlevel == WATERLEVEL_NONE)?200:150, 2);
	PLAYBACK_EVENT_FULL(0, edict(), g_usGrenExp, 0.0, pev->origin, pev->angles, pev->dmg, 0.0, g_iModelIndexColdball1, g_iModelIndexColdball2, GREN_FREEZE, pev->waterlevel);
	if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
	{
		if (pev->waterlevel == WATERLEVEL_NONE)
			::RadiusDamage(vecSpot, this, GetDamageAttacker(), pev->dmg, radius, CLASS_MACHINE, DMG_FREEZE | DMG_PARALYZE | DMG_SLOWFREEZE | DMG_NEVERGIB | DMG_DONT_BLEED);
		else
			::RadiusDamage(vecSpot, this, GetDamageAttacker(), pev->dmg*0.75f, radius*0.8f, CLASS_MACHINE, DMG_FREEZE | DMG_PARALYZE | DMG_SLOWFREEZE | DMG_NEVERGIB | DMG_DONT_BLEED);
	}
	UTIL_ScreenShake(pev->origin, 6, 32, 2, radius*0.5f);
	if (!FBitSet(pev->spawnflags, SF_NOSOUND))
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME + (int)pev->dmg, 3.0f);

	if (sv_overdrive.value <= 0.0f || pev->impulse > 0)// double explosion :)
	{
		Destroy();
		return;
	}
	else
	{
		++pev->impulse;
		SetNextThink(0.25);// XDM3038a
	}
}

//-----------------------------------------------------------------------------
// Purpose: Small poisonous clouds
//-----------------------------------------------------------------------------
void CGrenade::PoisonThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	StudioFrameAdvance();
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		pev->velocity *= 0.5f;
		pev->framerate = 0.2f;
	}

	if (!m_fRegisteredSound && (pev->dmgtime - 2 < gpGlobals->time))
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
		m_fRegisteredSound = TRUE;
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		if (pev->impulse > 0)
		{
			SetThink(&CGrenade::PoisonThinkEnd);
		}
		else
		{
			pev->impulse = 1;
			pev->pain_finished = gpGlobals->time;// start stinkin'!
			pev->dmgtime = gpGlobals->time + GREN_POISON_TIME;
		}
	}

	if (pev->impulse > 0 && pev->pain_finished <= gpGlobals->time)
	{
		/*MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_SPRITE);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
		if (pev->waterlevel > WATERLEVEL_NONE)
			WRITE_SHORT(g_iModelIndexAcidPuff1);
		else
			WRITE_SHORT(g_iModelIndexAcidPuff2);

			WRITE_BYTE(10);// scale in 0.1's
			WRITE_BYTE(160);// brightness
		MESSAGE_END();*/

		Vector vecSpot(pev->origin);
		if (FBitSet(pev->flags, FL_ONGROUND))// XDM: for RadiusDamage
			vecSpot.z += 2.0f;

		pev->owner = NULL;// XDM3037
		if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
			::RadiusDamage(vecSpot, this, GetDamageAttacker(), pev->dmg*0.5f, pev->dmg, CLASS_MACHINE, DMG_ACID | DMG_NEVERGIB | DMG_DONT_BLEED);

		pev->owner = m_hOwner.Get();// XDM3038c: continue ignoring owner
		PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), g_usGrenExp, 0.0, vecSpot, pev->angles, pev->dmg*0.5f, 0.0f, pev->spawnflags & SFM_EXPLOSION_FLAGS, 0, GREN_POISON, (pev->waterlevel > 0)?1:0);
		pev->pain_finished = gpGlobals->time + 0.5f;
	}

	if (pev->impulse > 0 && sv_overdrive.value > 0.0f)
	{
		if (pev->teleport_time <= gpGlobals->time)
		{
			CAGrenade::ShootTimed(pev->origin + UTIL_RandomBloodVector()*4.0f, g_vecZero,
				(/*m_vecNormal + */RandomVector(VECTOR_CONE_45DEGREES)) * RANDOM_FLOAT(850, 950),
				m_hOwner, this, pev->dmgtime - gpGlobals->time + RANDOM_FLOAT(1.0, 2.0), 0.0f, 0);// wath out for overload!

			pev->teleport_time = gpGlobals->time + 0.4f;
		}
	}
	SetNextThink(0.1);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Final large poisonous explosion
//-----------------------------------------------------------------------------
void CGrenade::PoisonThinkEnd(void)
{
	Vector vecSpot(pev->origin);
	if (FBitSet(pev->flags, FL_ONGROUND))// XDM: for RadiusDamage
		vecSpot.z += 8.0f;

	pev->owner = NULL;// XDM3037
	if (!CheckContents(vecSpot))
	{
		Destroy();
		return;
	}

	BeamEffect(TE_BEAMTORUS, vecSpot, vecSpot + Vector(0,20,420), g_iModelIndexShockWave, 0, 0, 5, 40, 0, Vector(0,255,0), 64, 2);
	float time = GREN_POISON_AFTERTIME;
	//PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), g_usGrenExp, 0.0, pev->origin, pev->angles, pev->dmg, time, g_iModelIndexAcidPuff3, g_iModelIndexAcidDrip, GREN_POISON, pev->waterlevel);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), g_usGrenExp, 0.0, pev->origin, pev->angles, pev->dmg, time, pev->spawnflags & SFM_EXPLOSION_FLAGS, 1, GREN_POISON, pev->waterlevel);

	if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
		::RadiusDamage(vecSpot, this, GetDamageAttacker(), pev->dmg, pev->dmg * GREN_DAMAGE_TO_RADIUS_POISON, CLASS_MACHINE, DMG_ACID | DMG_NERVEGAS | DMG_NEVERGIB | DMG_DONT_BLEED);

	if (!FBitSet(pev->spawnflags, SF_NOSOUND))
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME + (int)pev->dmg, 3.0f);

	DoDamageInit(time, 0.25f, pev->dmg*0.1f, (pev->dmg*0.1f)/time, pev->dmg*GREN_DAMAGE_TO_RADIUS_POISON, 64.0f, DMG_ACID|DMG_NEVERGIB|DMG_DONT_BLEED);
}

//-----------------------------------------------------------------------------
// Purpose: Burn process
//-----------------------------------------------------------------------------
void CGrenade::BurnThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	StudioFrameAdvance();

	// First step: start burning!
	if (pev->impulse == 0 && pev->dmgtime <= gpGlobals->time)
	{
		if (pev->waterlevel <= WATERLEVEL_NONE)
		{
			/*TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0,0,-40), ignore_monsters, edict(), &tr);
			UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0,2));*/
			PLAYBACK_EVENT_FULL(0, edict(), g_usGrenExp, 0.0, pev->origin, pev->angles, pev->dmg, GREN_BURN_TIME, g_iModelIndexFire, g_iModelIndexFlameFire, GREN_NAPALM, 200);
			pev->effects |= EF_BRIGHTLIGHT;
		}
		pev->impulse = 1;
		pev->dmgtime = gpGlobals->time + GREN_BURN_TIME;
		if (!FBitSet(pev->spawnflags, SF_NOSOUND))
			CSoundEnt::InsertSound(bits_SOUND_COMBAT | bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}
	// This 'if' is passed the whole time when grenade is burning
	if (pev->impulse > 0)
	{
		if (pev->waterlevel > WATERLEVEL_NONE)// in water: just bubbles
		{
			//pev->velocity *= 0.75f;
			pev->framerate = 0.25f;

			if (pev->dmg > 0.5f && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()) && RANDOM_LONG(0,2)==0)
				FX_BubblesPoint(pev->origin, VECTOR_CONE_20DEGREES, max(2, min((int)(pev->dmg*0.5f), 32)));

			if (pev->dmgtime < gpGlobals->time)
				SetThink(&CBaseEntity::SUB_Remove);// BurnThinkEnd

			//SetNextThink(0.1);// XDM3038a
		}
		else// not in water
		{
			pev->framerate = 1.0f;

			if (!FBitSet(pev->spawnflags, SF_NOSOUND) && gpGlobals->time >= pev->pain_finished)
			{
				//pev->effects |= EF_MUZZLEFLASH;
				EMIT_SOUND(edict(), CHAN_VOICE, "ambience/flameburst1.wav", VOL_NORM, ATTN_NORM);
				pev->pain_finished = gpGlobals->time + 1.0f;
			}
			Vector vecSpot(pev->origin);
			if (FBitSet(pev->flags, FL_ONGROUND))// XDM: for RadiusDamage
				vecSpot.z += 2.0f;

			pev->owner = NULL;// XDM3037
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(vecSpot, this, GetDamageAttacker(), pev->dmg/GREN_BURN_TIMES_PER_SECOND, pev->dmg*GREN_DAMAGE_TO_RADIUS_BURN, CLASS_NONE, DMG_BURN | DMG_SLOWBURN | DMG_NEVERGIB | DMG_IGNITE | DMG_DONT_BLEED);

			pev->owner = m_hOwner.Get();// XDM3038c: continue ignoring owner
		}

		// This check should be done regardless of water level
		if (pev->dmgtime < gpGlobals->time)
		{
			SetThink(&CBaseEntity::SUB_Remove);//BurnThinkEnd);
			SetNextThink(0);
			return;
		}
	}
	SetNextThink(1.0f/GREN_BURN_TIMES_PER_SECOND);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Growing radioactive glow
//-----------------------------------------------------------------------------
void CGrenade::RadiationThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	StudioFrameAdvance();
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		pev->velocity *= 0.5f;
		pev->framerate = 0.2f;
	}

	if (pev->dmgtime <= gpGlobals->time)// time to change phase
	{
		if (pev->impulse == 0)// first phase
		{
			CSprite *m_pGlow = CSprite::SpriteCreate(GREN_RADIATION_SPRITE, pev->origin + Vector(0,0,GREN_RADIATION_ZOFFSET), FALSE);
			if (m_pGlow)
			{
				m_pGlow->SetAttachment(edict(), 0);
				m_pGlow->SetTransparency(kRenderGlow, 255,255,255,255, kRenderFxNoDissipation);
				m_pGlow->Expand(3, 90);
			}
			BeamEffect(TE_BEAMTORUS, pev->origin, pev->origin + Vector(0,20,420), g_iModelIndexShockWave, 0, 0, 10, 40, 0, Vector(200,200,255), 80, 0);
			pev->impulse = 1;
			pev->speed = 0;// 0...1
			pev->button = DMG_RADIATION | DMG_NEVERGIB | DMG_NOHITPOINT | DMG_DONT_BLEED | DMG_WALLPIERCING;
			pev->dmgtime = gpGlobals->time + GREN_RADIATION_DELAY;
			if (!FBitSet(pev->spawnflags, SF_NOSOUND))
				EMIT_SOUND(edict(), CHAN_VOICE, "ambience/particle_suck1.wav", VOL_NORM, ATTN_NORM);

			CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
		}
		else//if (pev->impulse > 0)// second phase
		{
			SetThink(&CGrenade::RadiationThinkEnd);
		}
	}
	if (pev->impulse > 0)// damage enabled
	{
		if (pev->speed < 1.0f)
			pev->speed += 0.1f;

		pev->owner = NULL;// XDM3037
		// no need for offset since it is wall-piercing damage
		if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
			::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg*pev->speed, pev->dmg*pev->speed*GREN_DAMAGE_TO_RADIUS_RADIATION, CLASS_NONE, pev->button);

		pev->owner = m_hOwner.Get();// XDM3038c: continue ignoring owner
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
			DynamicLight(pev->origin, (int)pev->speed, 160, 160, 255, 0, 40);
	}
	SetNextThink(0.1);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Final radioactive explosion
//-----------------------------------------------------------------------------
void CGrenade::RadiationThinkEnd(void)
{
	pev->owner = NULL;// XDM3037
	if (!CheckContents(pev->origin))
	{
		// allow radiation Destroy();
		return;
	}
	PLAYBACK_EVENT_FULL(0, edict(), g_usGrenExp, 0.0f, pev->origin, pev->angles, pev->dmg, 0.0f, g_iModelIndexColdball1, g_iModelIndexColdball2, GREN_RADIOACTIVE, pev->waterlevel);
	if (!FBitSet(pev->spawnflags, SF_NOSOUND))
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME + (int)pev->dmg, 2.0f);

	if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
		::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg, pev->dmg*GREN_DAMAGE_TO_RADIUS_RADIATION+32, CLASS_NONE, pev->button);

	//old DoDamageInit(GREN_RADIATION_AFTERTIME, 0.8f, -1.0f, pev->button);
	DoDamageInit(GREN_RADIATION_AFTERTIME, 0.8f, pev->dmg*0.2f, -1.0f, pev->dmg*GREN_DAMAGE_TO_RADIUS_RADIATION, -4.0f, pev->button);
}

//-----------------------------------------------------------------------------
// Purpose: Nuclear explosion
//-----------------------------------------------------------------------------
void CGrenade::NuclearExplodeThink(void)
{
	CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037
	switch (pev->oldbuttons)
	{
	default:
	case 0:
		{
			pev->dmg = gSkillData.DmgNuclear;// regardless of what was before
			pev->movetype = MOVETYPE_NONE;
			pev->solid = SOLID_NOT;
			pev->effects |= EF_NODRAW;
			pev->takedamage = DAMAGE_NO;
			pev->owner = NULL;// XDM3037
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(pev->origin, this, pAttacker, pev->dmg*0.1f, pev->dmg*GREN_DAMAGE_TO_RADIUS, CLASS_NONE, DMG_BURN | DMG_IGNITE);

			PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), g_usGrenExp, 0.0f, pev->origin, pev->angles, pev->dmg, pev->dmg*GREN_DAMAGE_TO_RADIUS, g_iModelIndexNucFX, g_iModelIndexUExplo, GREN_NUCLEAR, UTIL_LiquidContents(pev->origin));
			CSoundEnt::InsertSound(bits_SOUND_COMBAT|bits_SOUND_DANGER, pev->origin, pev->dmg*GREN_DAMAGE_TO_RADIUS, 3.0);
			SetNextThink(0.25);// XDM3038a
		}
		break;
	case 1:
		{
			BeamEffect(TE_BEAMDISK, pev->origin, pev->origin + Vector(0.0f,0.0f,pev->dmg*(float)GREN_DAMAGE_TO_RADIUS), g_iModelIndexNucRing, 0,100,20,128,0, Vector(207,183,47), 200, 2);// disk
			SetNextThink(0.3);// XDM3038a
		}
		break;
	case 2:
		{
			BeamEffect(TE_BEAMCYLINDER, pev->origin + Vector(0,0,60), pev->origin + Vector(0.0f,0.0f,pev->dmg*(float)GREN_DAMAGE_TO_RADIUS), g_iModelIndexNucRing, 0,100,20,160,0, Vector(255,191,95), 200, 1);// shockwave

#if !defined(CLIENT_DLL)
			if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
				UTIL_ScreenShake(pev->origin, 32.0f, 4.0f, 3.0f, pev->dmg);
			//UTIL_ScreenShakeAll(pev->origin, 32.0f, 10.0f, 3.0f);
#endif
			SetNextThink(0.4);// XDM3038a
		}
		break;
	case 3:
		{
			BeamEffect(TE_BEAMTORUS, pev->origin + Vector(0.0f,0.0f,80.0f), pev->origin + Vector(0.0f,0.0f,pev->dmg*(float)GREN_DAMAGE_TO_RADIUS * 1.5f), g_iModelIndexNucRing, 0,100,20,180,0, Vector(255,180,80), 200, 1);// big shockwave
			BeamEffect(TE_BEAMCYLINDER, pev->origin + Vector(0.0f,0.0f,640.0f), pev->origin + Vector(0.0f,0.0f,pev->dmg*(float)GREN_DAMAGE_TO_RADIUS), g_iModelIndexNucRing, 0,100,18,128,0, Vector(0,100,255), 100, 1);// blue ring
			SetNextThink(0.5);// XDM3038a
		}
		break;
	case 4:
		{
			SetNextThink(0.2);// XDM3038a
		}
		break;
	case 5:
		{
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(pev->origin, this, pAttacker, pev->dmg*0.25f, pev->dmg*GREN_DAMAGE_TO_RADIUS, CLASS_NONE, DMG_BLAST | DMG_RADIATION | DMG_NEVERGIB);
			SetNextThink(0.2);// XDM3038a
		}
		break;
	case 6:
		{
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(pev->origin, this, pAttacker, pev->dmg*0.5f, pev->dmg*GREN_DAMAGE_TO_RADIUS * 1.5, CLASS_NONE, DMG_BLAST | DMG_BURN | DMG_VAPOURIZING);
			SetNextThink(0.25);// XDM3038a
		}
		break;
	case 7:
		{
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(pev->origin, this, pAttacker, pev->dmg*0.75f, pev->dmg*GREN_DAMAGE_TO_RADIUS * 2.0, CLASS_NONE, DMG_BURN | DMG_RADIATION | DMG_NEVERGIB | DMG_VAPOURIZING);
			SetNextThink(0.25);// XDM3038a
		}
		break;
	case 8:
		{
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(pev->origin, this, pAttacker, pev->dmg, pev->dmg*GREN_DAMAGE_TO_RADIUS * 2.5, CLASS_NONE, DMG_RADIATION | DMG_NEVERGIB);
			SetNextThink(0.25);// XDM3038a
		}
		break;
	case 9:
		{
			// blast wave
			//::RadiusDamage(pev->origin, this, pAttacker, pev->dmg, pev->dmg*GREN_DAMAGE_TO_RADIUS * 3.0f, CLASS_NONE, DMG_BLAST | DMG_BURN);
			if (!FBitSet(pev->spawnflags, SF_NODAMAGE))
				::RadiusDamage(pev->origin, this, pAttacker, pev->dmg, pev->dmg*GREN_DAMAGE_TO_RADIUS*3.0f, CLASS_NONE, DMG_BLAST|DMG_NERVEGAS|DMG_RADIATION);
			SetNextThink(0.25);// XDM3038a
		}
		break;
	case 10:
		{
#if !defined(CLIENT_DLL)
			UTIL_ScreenShakeAll(pev->origin, 15.0f, 6.0f, 4.0f);//, pev->dmg*3.0f);
			UTIL_ScreenFadeAll(Vector(255, 207, 127), 1, 0.5f, 128, FFADE_IN);
			UTIL_DecalPoints(pev->origin, pev->origin - Vector(0,0,256), edict(), DECAL_NUCBLOW1 + RANDOM_LONG(0,2));
#endif
			SetNextThink(0.1);// XDM3038a
		}
		break;
	case 11:
		{
			BeamEffect(TE_BEAMCYLINDER, pev->origin + Vector(0,0,100), pev->origin + Vector(0.0f,0.0f,pev->dmg*(float)GREN_DAMAGE_TO_RADIUS), g_iModelIndexNucRing, 0,100,20,100,0, Vector(255,200,100), 80, 1);// small shockwave
			SetNextThink(0.5);// XDM3038a
		}
		break;
	case 12:
		{
			MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_SMOKE);
				WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-80, 80));
				WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-80, 80));
				WRITE_COORD(pev->origin.z + 64);
				WRITE_SHORT(g_iModelIndexSmoke);
				WRITE_BYTE(pev->dmg*0.125);//240);// scale * 10
				WRITE_BYTE(8);// framerate
			MESSAGE_END();
			SetNextThink(0.3);// XDM3038a
			break;
		}
	case 13:
		{
			if (!FStringNull(pev->target))
				FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);

			if (FBitSet(pev->spawnflags, SF_REPEATABLE))
			{
				SetThink(&CBaseEntity::SUB_DoNothing);
				SetNextThink(0);// XDM3038a
			}
			else
			{
				DoDamageInit(GREN_NUCLEAR_AFTERTIME, 1.0f, pev->dmg*0.1f, -pev->dmg*0.01f, pev->dmg*GREN_DAMAGE_TO_RADIUS*2.0f, -pev->dmg*0.5f, DMG_RADIATION|DMG_NERVEGAS|DMG_NEVERGIB);
			}
		}
		break;
	}
	++pev->oldbuttons;
}
