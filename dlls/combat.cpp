// The retarded bunch of useless letters VALVe(r)(tm)(wtf) called "TEH CODE" was removed from the face of... er... whatever memory it was on.
// The leftovers don't deserve any copyright unless someone wants to take all the shame.
#include "extdll.h"
#include "util.h"
#include "cbase.h"
//#include "monsters.h"
#include "sound.h"
#include "skill.h"
//#include "soundent.h"
#include "decals.h"
//#include "animation.h"
#include "weapons.h"
//#include "pm_materials.h"
//#include "gamerules.h"// XDM
#include "globals.h"
#include "game.h"
//#include "player.h"

//#define _DEBUG_DAMAGE 1
#if defined (_DEBUG_DAMAGE)
#define DBG_PRINT_DMG						DBG_PrintF
#define DBG_PRINT_DMG2						DBG_PrintF
#else
#define DBG_PRINT_DMG
#define DBG_PRINT_DMG2
#endif

MULTIDAMAGE gMultiDamage;

/*
==============================================================================
MULTI-DAMAGE
Collects multiple small damages into a single damage
XHL: never seen it to actually work
==============================================================================
*/

// ClearMultiDamage - resets the global multi damage accumulator
void ClearMultiDamage(void)
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount	= 0;
	gMultiDamage.type = 0;
	//?	gMultiDamage.hitgroup = 0;
}

// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
int ApplyMultiDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker)
{
	if (!gMultiDamage.pEntity)
		return 0;

	//DBG_PRINT_DMG("ApplyMultiDamage\n");
	return gMultiDamage.pEntity->TakeDamage(pInflictor, pAttacker, gMultiDamage.amount, gMultiDamage.type);
}

//size_t g_MultiDamageCounter = 0;

int AddMultiDamage(CBaseEntity *pInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType)
{
	if (pEntity == NULL)
		return 0;

	int iRet = 0;
	if (gMultiDamage.pEntity != NULL && pEntity != gMultiDamage.pEntity)
	{
		//conprintf(1, "AddMultiDamage(): new entity.\n");
		iRet = ApplyMultiDamage(pInflictor, pInflictor); // UNDONE: wrong attacker!
		ClearMultiDamage();

		/*if (g_MultiDamageCounter > 1)
			conprintf(1, "AddMultiDamage(): saved %u iterations\n", g_MultiDamageCounter); never got here lol

		g_MultiDamageCounter = 0;*/
	}
	//else
	//	g_MultiDamageCounter++;

	gMultiDamage.pEntity = pEntity;
	gMultiDamage.amount += flDamage;
	gMultiDamage.type |= bitsDamageType;
	return iRet;
}


//-----------------------------------------------------------------------------
// Purpose: Certain entities may provide special decals to be painted on them
// Input  : *pEntity - 
//			&bitsDamageType - DMG_GENERIC
// Output : int server decal index DECAL_GUNSHOT1
//-----------------------------------------------------------------------------
int DamageDecal(CBaseEntity *pEntity, const int &bitsDamageType)
{
	if (pEntity)
		return pEntity->DamageDecal(bitsDamageType);

	if (FBitSet(bitsDamageType, DMG_BLAST))// XDM
		return (DECAL_LARGESHOT1 + RANDOM_LONG(0,4));

	if (FBitSet(bitsDamageType, DMG_CLUB))
		return (DECAL_BIGSHOT1 + RANDOM_LONG(0,4));

	return (DECAL_GUNSHOT1 + RANDOM_LONG(0,4));
}

//-----------------------------------------------------------------------------
// Purpose: Paint a decal specific to a bullet type
// Input  : *pTrace - trace thet hits some surface
//			&iBulletType - Bullet enum (BULLET_NONE)
//-----------------------------------------------------------------------------
void DecalGunshot(TraceResult *pTrace, const int &iBulletType)
{
	if (!UTIL_IsValidEntity(pTrace->pHit))// Is the entity valid
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance(pTrace->pHit);

	//if (VARS(pTrace->pHit)->solid == SOLID_BSP || VARS(pTrace->pHit)->movetype == MOVETYPE_PUSHSTEP)
	if (pEntity && pEntity->IsBSPModel())
	{
		if (iBulletType == BULLET_12MM)
			UTIL_GunshotDecalTrace(pTrace, DamageDecal(pEntity, DMG_BLAST));// XDM: DMG_BLAST makes DECAL_LARGESHOT
		else if (iBulletType == BULLET_BREAK)
			UTIL_DecalTrace(pTrace, DamageDecal(pEntity, DMG_CLUB));
		else
			UTIL_GunshotDecalTrace(pTrace, DamageDecal(pEntity, DMG_BULLET));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inflict damage upon entities within a certain range (explosions, etc.)
// Warning: Get ready for unexpected engine bugs, misses, crashes. Especially around TraceLine!
// Note   : I've spent almost hundred hours writing this masterpiece, don't ruin it!
// Comment: I think I deserve godmode IRL for a day, or at least a month of sleep.
// UNDONE : Actually it whould be better to sort entities by radius first,
//			THEN do logic instead of trying to guess what's wrong every iteration.
// Input  : &vecSrc - center of the sphere
//			*pInflictor - attacker's gun or a projectile
//			*pAttacker - the attacker
//			flDamage - damage amount, inflicted gradually unless DMG_RADIUS_MAX is specified
//			flRadius - maximum radius at which entities are affected
//			iClassIgnore - entity class to skip (CLASS_NONE)
//			bitsDamageType - damage bits (DMG_GENERIC)
//-----------------------------------------------------------------------------
void RadiusDamage(const Vector &vecSrc, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType)//, char PushPolicy)
{
	if (flRadius <= 0)
	{
		DBG_PRINTF("RadiusDamage() error: bad radius %g!\n", flRadius);
		return;
	}
	if (IsGameOver())// XDM3038c: datagram overload prevention
		return;

	float flAdjustedDamage;//, falloff;
	vec_t targetdist;
	int pc;
	//ntry	size_t ntry;
	CBaseEntity *pHitByTrace;
	CBaseEntity *pEntity = NULL;
	edict_t *pEntIgnore;
	Vector vecTarget;
	Vector vecDelta;
	TraceResult	tr;
	short iDamageInteraction;
	bool bHit;

	// XDM: don't check my PointContents every time!
	/* TODO	if (PushPolicy == POLICY_ALLOW)
		push = true;
	else if (PushPolicy == POLICY_DENY)
		push = false;
	else*/
	bool push = FBitSet(bitsDamageType, DMGM_PUSH);
	bool bInWater = false;// TODO: use tr.fInWater tr.fInOpen
	bool bInSolid = false;
	bool bThroughWalls = FBitSet(bitsDamageType, DMG_RADIATION|DMG_WALLPIERCING);// XDM3038a
	pc = POINT_CONTENTS(vecSrc);
	if (pc < CONTENTS_SOLID && pc > CONTENTS_SKY)
	{
		bInWater = true;
	}
	else if (pc == CONTENTS_SOLID || pc > CONTENTS_EMPTY)// XDM3038: TODO: revisit. What is 0?
	{
		bInSolid = true;
		conprintf(1, "RadiusDamage(%g %g %g, Inf %s, Att %s, dmg %g, rad %g, classIgn %d, bitsType %d): Error: start inside solid!\n", vecSrc.x, vecSrc.y, vecSrc.z, pInflictor?STRING(pInflictor->pev->classname):"", pAttacker?STRING(pAttacker->pev->classname):"", flDamage, flRadius, iClassIgnore, bitsDamageType);
		//return;
	}

	if (!pAttacker)
		pAttacker = pInflictor;

	if (pInflictor)
	{
		pEntIgnore = pInflictor->edict();
		//ASSERT(pInflictor->pev->owner == NULL);
	}
	else
		pEntIgnore = NULL;

	if (flRadius <= 0.0f)// XDM3037
		flRadius = flDamage * DAMAGE_TO_RADIUS_DEFAULT;

	g_vecAttackDir.Clear();// XDM3038a: HACK
	DBG_PRINT_DMG("RadiusDamage(%g %g %g, Inf %s, Att %s, dmg %g, rad %g, classIgn %d, bitsType %d)\n", vecSrc.x, vecSrc.y, vecSrc.z, pInflictor?STRING(pInflictor->pev->classname):"", pAttacker?STRING(pAttacker->pev->classname):"", flDamage, flRadius, iClassIgnore, bitsDamageType);
	//cant see?	GlowSprite(vecSrc, g_iModelIndexAcidDrip, 100, 100, 0);

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
	// iterate on all entities in the vicinity.
	// IMPORTANT: entities are NOT sorted, so we cannot remove obstacles first
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity == pInflictor)// XDM: no self-destruction :) (recursion, endless loop!)
		{
			DBG_PRINT_DMG("RD: SKIP: pEntity == pInflictor\n");
			continue;
		}
		if (pEntity->GetExistenceState() != ENTITY_EXSTATE_WORLD)// XDM3038c: don't damage virtual entities
			continue;
		if (pEntity->IsRemoving())// XDM3038a: skip entities that are about to be removed
			continue;
		// UNDONE: this should check a damage mask, not an ignore
		if ((iClassIgnore != CLASS_NONE) && pEntity->Classify() == iClassIgnore)
		{
			DBG_PRINT_DMG("RD: SKIP %s[%d]: iClassIgnore.\n", STRING(pEntity->pev->classname), pEntity->entindex());
			continue;// houndeyes don't hurt other houndeyes with their attack
		}
		if ((pEntity->pev->takedamage == DAMAGE_NO) && !(push && pEntity->IsPushable()))// do damage OR just push
		{
			DBG_PRINT_DMG("RD: SKIP %s[%d]: DAMAGE_NO and not pushable.\n", STRING(pEntity->pev->classname), pEntity->entindex());
			continue;
		}
		DBG_PRINT_DMG("RD: START %s[%d]\n", STRING(pEntity->pev->classname), pEntity->entindex());
		bHit = false;
		memset(&tr, 0, sizeof(TraceResult));
		// Check if startpoint is inside of target entity
		if (PointInBounds(vecSrc, pEntity->pev->absmin, pEntity->pev->absmax))
		{
			DBG_PRINT_DMG2("RD: start is inside %s[%d]\n", STRING(pEntity->pev->classname), pEntity->entindex());
			bHit = true;
			pHitByTrace = pEntity;
			//tr.fAllSolid = 1;
			tr.flFraction = 0.0f;// do full damage
			tr.pHit = pEntity->edict();
			tr.vecEndPos = vecSrc;
			tr.vecPlaneNormal.Set(0,0,1);
			vecTarget = vecSrc;
		}
		else// vecSrc is outside
		{
			DBG_PRINT_DMG2("RD: tracing %s[%d]\n", STRING(pEntity->pev->classname), pEntity->entindex());
			//ntry for (ntry=0; ntry<2; ++ntry)
			{
				bHit = true;
				/*ntry if (ntry == 0)
				{
					vecTarget = pEntity->Center();
					// XDM3038a: still not reliable vecTarget = GetNearestPointOfABox(vecSrc, pEntity->Center(), pEntity->pev->size*0.5f);
				}
				else
				{*/
					vecTarget = pEntity->BodyTarget(vecSrc);// try something more simple
				//ntry}
				gpGlobals->trace_flags = 0;// need strict hit points
				UTIL_TraceLine(vecSrc, vecTarget, bThroughWalls?ignore_monsters:dont_ignore_monsters, bThroughWalls?ignore_glass:dont_ignore_glass, pEntIgnore, &tr);
#if defined (_DEBUG_DAMAGE)
				BeamEffect(TE_BEAMPOINTS, vecSrc, vecTarget, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 4, 0, Vector(191,191,191), 191, 0);// src dest
				if (tr.pHit != pEntity->edict())
				{
					if (!FNullEnt(pEntity->pev->owner))
						DBG_PRINT_DMG("RD: pEntity->pev->owner is %s[%d]!\n", STRING(pEntity->pev->owner->v.classname), ENTINDEX(pEntity->pev->owner));
					if (pEntity->pev->groupinfo != 0)
						DBG_PRINT_DMG("RD: pEntity->pev->groupinfo is %d!\n", pEntity->pev->groupinfo);
				}
				BeamEffect(TE_BEAMPOINTS, vecSrc, tr.vecEndPos, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 4, 0, Vector(191,191,0), 255, 0);// trace
#endif
				if (tr.flFraction >= 1.0f)
				{
					//ntry DBG_PRINT_DMG2("RD: %d SKIP: flFraction == 1.\n", ntry);
					vecDelta = vecTarget;// vecTarget - vecSrc;
					vecDelta -= vecSrc;
					targetdist = vecDelta.Length();
					if (targetdist < flRadius)// HA!!!!!!!!! HAAAAAAAAAAAAAA!!!!!!!!!!!!!! F*** YOU HALF-LIFE, F*** YOU!!!!!!!!!!!!!
					{
						DBG_PRINT_DMG2("RD: targetdist UNHACK.\n");//ntry DBG_PRINT_DMG2("RD: ntry: %d, targetdist UNHACK.\n", ntry);
						bHit = true;
						pHitByTrace = pEntity;
						tr.flFraction = targetdist/flRadius;// keep this non-zero!
						tr.pHit = pEntity->edict();
						tr.vecEndPos = vecTarget;
						tr.vecPlaneNormal = vecDelta;//.Normalize();
						tr.vecPlaneNormal.NormalizeSelf();
					}
					else
					{
						bHit = false;
						DBG_PRINT_DMG("RD: SKIP %s[%d]: tr.flFraction >= 1.0f.\n", STRING(pEntity->pev->classname), pEntity->entindex());
						continue;
					}
				}
				if (tr.fAllSolid != 0)
				{
					DBG_PRINT_DMG("RD: SKIP %s[%d]: fAllSolid.\n", STRING(pEntity->pev->classname), pEntity->entindex());//DBG_PRINT_DMG2("RD: %d SKIP: fAllSolid.\n", ntry);
					bHit = false;
					continue;
				}
			}//ntry
			if (bHit)
				pHitByTrace = CBaseEntity::Instance(tr.pHit);
		}

		if (!bHit)
		{
			DBG_PRINT_DMG2("RD: SKIP %s[%d]: !bHit.\n", STRING(pEntity->pev->classname), pEntity->entindex());
			continue;
		}
		flAdjustedDamage = flDamage;

		//BeamEffect(TE_BEAMPOINTS, tr.vecEndPos, vecTarget, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 8, 10, Vector(127,0,0), 255, 0);// remaining

		//if (FNullEnt(tr.pHit))
		//	DBG_PRINT_DMG("RD: FNullEnt()\n");

		DBG_PRINT_DMG2("RD: got %s[%d] - %s\n", STRING(tr.pHit->v.classname), ENTINDEX(tr.pHit), (tr.pHit == pEntity->edict())?"TARGET":"OBSTACLE");

		/*if (tr.fAllSolid || !UTIL_IsValidEntity(pHitByTrace))//FNullEnt(tr.pHit))// XDM3037: TESTME
		{
			BeamEffect(TE_BEAMPOINTS, vecSrc, tr.vecEndPos, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(191,0,0), 255, 0);// fail
			continue;
		}*/

		//BeamEffect(TE_BEAMPOINTS, vecSrc, tr.vecEndPos, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(0,191,0), 255, 0);// hit

		// XDM3038b: FAIL! trace can't go through the world even if we set 
		/* this is how it should be!!		size_t nblockers = 0;
		while (tr.pHit != pEntity->edict())
		{
			CBaseEntity *pBlocker = CBaseEntity::Instance(tr.pHit);
			if (pBlocker)
			{
				++nblockers;
				conprintf(1, "RD: %u found blocker: %s[%d]\n", nblockers, STRING(pBlocker->pev->classname), pBlocker->entindex());
				iDamageInteraction = pBlocker->DamageInteraction(bitsDamageType, tr.vecEndPos);
				if (iDamageInteraction == DMGINT_THRU)// trace behind this blocker, eventually we'll hit it too
				{
					conprintf(1, "RD: found blocker: tracing through\n");
					BeamEffect(TE_BEAMPOINTS, tr.vecEndPos, vecTarget, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(0,0,191), 255, 0);
					UTIL_TraceLine(tr.vecEndPos, vecTarget, dont_ignore_monsters, ignore_glass, pBlocker->edict(), &tr);
				}
				else if (iDamageInteraction == DMGINT_ABSORB)// UNDONE
					break;// probably will have to build a list of affected entities here :(
			}
		}*/
		/* old XDM3038		if (tr.pHit != pEntity->edict())// && bThroughMonsters)// manually pierce monsters
		{
			CBaseEntity *pBlocker = CBaseEntity::Instance(tr.pHit);
			if (pBlocker)
			{
				iDamageInteraction = pBlocker->DamageInteraction(bitsDamageType, tr.vecEndPos);
				if (iDamageInteraction == DMGINT_THRU)// trace behind this blocker, eventually we'll hit it too
				{
					//BeamEffect(TE_BEAMPOINTS, tr.vecEndPos, vecTarget, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(0,0,191), 255, 0);
					UTIL_TraceLine(tr.vecEndPos, vecTarget, dont_ignore_monsters, ignore_glass, pBlocker->edict(), &tr);
				}
				//else if (iDamageInteraction == DMGINT_ABSORB)// UNDONE
				// probably will have to build a list of affected entities here :(
			}
		}*/
		if (tr.pHit != pEntity->edict())// manually pierce blockers
		{
			CBaseEntity *pBlocker = CBaseEntity::Instance(tr.pHit);
			ASSERT(pBlocker != pEntity);
			if (pBlocker)
			{
				iDamageInteraction = pBlocker->DamageInteraction(bitsDamageType, tr.vecEndPos);
				if (iDamageInteraction == DMGINT_THRU)// trace behind this blocker, eventually we'll hit it too // HL: cannot trace through the world!
				{
					//BeamEffect(TE_BEAMPOINTS, tr.vecEndPos, vecTarget, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(0,0,191), 255, 0);
					//UTIL_TraceLine(vecTarget, tr.vecEndPos, dont_ignore_monsters, ignore_glass, pEntity->edict(), &tr);// trace backwards?
					// what if there are two or more world walls? Like  O <--||<--||<--(x)-explosion
					// HACKHACKHACK!!! VERY BAD AND UGLY HACK!!! JUST IGNORE ALL SHIT!!!
					pHitByTrace = pEntity;
					tr.flFraction = 0.5;// keep this non-zero!
					tr.pHit = pEntity->edict();
					tr.vecEndPos = vecTarget;
					tr.vecPlaneNormal = vecDelta;//.Normalize();
					tr.vecPlaneNormal.NormalizeSelf();
				}
				//else if (iDamageInteraction == DMGINT_ABSORB)// UNDONE
				// probably will have to build a list of affected entities here :(
			}
		}


		/*if (tr.pHit)
			DBG_PRINT_DMG("RD: Trace: hit %s[%d]\n", STRING(tr.pHit->v.classname), ENTINDEX(tr.pHit));
		else
			DBG_PRINT_DMG("RD: Trace: nothing\n");*/

		if (tr.pHit != pEntity->edict())
		{
			DBG_PRINT_DMG2("RD: Trace: hit something unexpected. SKIP\n");
			continue;// hit something unexpected
		}
		if (tr.flFraction >= 1.0f && !FBitSet(bitsDamageType, DMG_WALLPIERCING))
		{
			DBG_PRINT_DMG2("RD: Trace: empty trace. SKIP\n");
			continue;// missed
		}

		DBG_PRINT_DMG("RD: Trace: OK\n");

		//		true == (a && (b || c))
		//		false == (!a || (!b && !c))
		//if (tr.flFraction < 1.0f || tr.pHit == pEntity->edict() || (bitsDamageType & DMG_WALLPIERCING))// XDM
		//working	if (tr.pHit == pEntity->edict() && (tr.flFraction < 1.0f || (bitsDamageType & DMG_WALLPIERCING)))// hit

		DBG_PRINT_DMG("RD: main check HIT %s[%d]\n", STRING(pEntity->pev->classname), pEntity->entindex());
		//if (bAlive) BeamEffect(TE_BEAMPOINTS, vecSrc, tr.vecEndPos, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(0,255,255), 255, 0);
		if (tr.fStartSolid && tr.flFraction == 0.0f)// XDM3037a
		{
			DBG_PRINT_DMG("RD: Trace: fStartSolid\n");
			if (tr.pHit == pEntity->edict())// if we're stuck inside them, fixup the position and distance
			{
				tr.vecEndPos = vecSrc;// explosion from inside p.1
				//already tr.flFraction = 0.0f;// do full damage
			}
			else if (tr.pHit)
				DBG_PRINT_DMG("RD: Trace: fStartSolid inside different entity: %s[%d]!\n", STRING(tr.pHit->v.classname), ENTINDEX(tr.pHit));
			else
				DBG_PRINT_DMG("RD: Trace: fStartSolid nowhere?!\n");
		}
		vecDelta = tr.vecEndPos;
		vecDelta -= vecSrc;// end of effective trace - center of explosion
		if (vecDelta.IsZero())// HACK: explosion from inside p.2
			vecDelta.z = 1.0f;

		targetdist = vecDelta.Length();// = flRadius;
		if (targetdist != 0.0f)
			vecDelta /= targetdist;// Normalize

		if (tr.pHit != pEntity->edict())// Obstructed, decrease damage. Only DMG_WALLPIERCING should get here!!
		{
			ASSERT(FBitSet(bitsDamageType, DMG_WALLPIERCING));
			flAdjustedDamage *= tr.flFraction;// UNDONE: wall thickness and material
		}

		// Decrease damage for an ent that's farther from the vecSrc.
		if (!FBitSet(bitsDamageType, DMG_RADIUS_MAX))
		{
			//if (len > 2.0f)// hack?
			//	len -= 2.0f;

			// FAIL! flAdjustedDamage *= (1.0f - targetdist/(flRadius*1.2f));// XDM3037: entities beyond flRadius won't get hit, but I want more than zero damage to be dealt to them! So there's a trick: damage gradient ends at 120% radius.
			flAdjustedDamage *= (1.0f - targetdist/flRadius);
		}

		DBG_PRINT_DMG("RD: -> %s[%d], dist %g/%g, adj.dmg %g/%g\n", STRING(pEntity->pev->classname), pEntity->entindex(), targetdist, flRadius, flAdjustedDamage, flDamage);

		if (flAdjustedDamage <= 0.0f)
		{
			DBG_PRINT_DMG2("RD: flAdjustedDamage <= 0.0f; SKIP\n");
			continue;
		}
		else// we DO some damage
			push = false;// XDM3038a: WARNING! Don't conflict with CBaseEntity::TakeDamage() where velocity is added too!

		if (bInWater != (pEntity->pev->waterlevel > WATERLEVEL_WAIST))// decrease damage when entering/leaving water
			flAdjustedDamage *= 0.6f;//continue;

		//float force = 0.0f;
		if (push && pEntity->IsPushable())
		{
			float force = pEntity->DamageForce(flAdjustedDamage);
			pEntity->pev->velocity += vecDelta * force;//flAdjustedDamage * 3.0f;// XDM3035c: +=
			//pEntity->pev->avelocity.x = pEntity->Center().z - vecSrc.z;
			//Vector diff = pEntity->Center() - tr.vecEndPos;
			pEntity->pev->avelocity += (pEntity->Center() - tr.vecEndPos).Normalize() * force;// XDM3035c: +=
			//DBG_PRINT_DMG("RD: %s (%s), norm %f dforce %f\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->netname), flAdjustedDamage * 3.0f, pEntity->DamageForce(flAdjustedDamage)*0.8f);
		}

		/* does not work	if (pEntity->IsPlayer() && FBitSet(bitsDamageType, DMG_BLAST) && (len / flRadius) < 0.5)
		{
			FADE_CLIENT_VOLUME(pEntity->edict(), test1.value, test2.value, 1, test3.value);
		}*/

		if (pEntity->pev->takedamage == DAMAGE_NO)
		{
			DBG_PRINT_DMG2("RD: pEntity->pev->takedamage == DAMAGE_NO; SKIP\n");
			continue;
		}

		g_vecAttackDir = vecDelta; g_vecAttackDir.NormalizeSelf();// XDM3038c: reversed (was -)

		DBG_PRINT_DMG2("RD: HIT: %s[%d] \"%s\", dist: %g (%g), dmg: %g (%g)\n", STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->netname), targetdist, flRadius, flAdjustedDamage, flDamage);
		if (tr.flFraction < 1.0f)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(pInflictor, flAdjustedDamage, vecDelta, &tr, bitsDamageType|DMG_NOHITPOINT);// XDM3037
			ApplyMultiDamage(pInflictor, pAttacker);
		}
		else
			pEntity->TakeDamage(pInflictor, pAttacker, flAdjustedDamage, bitsDamageType);

		if (FBitSet(bitsDamageType, DMG_FREEZE|DMG_PARALYZE))// XDM3035: moved here to make this global effect reusable
		{
			if (!pEntity->IsProjectile() && pEntity->Classify() != CLASS_NONE)
			{
				CBaseMonster *pVictim = pEntity->MyMonsterPointer();
				if (pVictim)
				{
					//pVictim->TakeDamage(this, pOwner, 4, DMG_FREEZE | DMG_NEVERGIB);
					//if (pVictim->IsAlive())// && pVictim->m_MonsterState != MONSTERSTATE_SCRIPT)
					pVictim->FrozenStart(flAdjustedDamage*0.1f);
					//conprintf(1, "RD: %s (%s), dist: %f (%f), dmg: %f (%f))\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->netname), len, flRadius, flAdjustedDamage, flDamage);
				}
			}
		}// freeze
			//else
			//	if (bAlive) BeamEffect(TE_BEAMPOINTS, vecSrc, tr.vecEndPos, g_iModelIndexLaser, 0,10, (int)(10*10.0f), 16, 0, Vector(0,63,63), 255, 0);
	}// while()^
#if defined (USE_EXCEPTIONS)
	}
	catch(...)// GetExceptionCode()?
	{
		printf("RadiusDamage(): exception!\n");// print to stdout in case engine fails to print this warning
		conprintf(1, "RadiusDamage(): exception!\n");
		//DBG_FORCEBREAK
	}
#endif
	g_vecAttackDir.Clear();// XDM3038a: HACK
	DBG_PRINT_DMG2("RadiusDamage: END\n");
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038b: tunable
//-----------------------------------------------------------------------------
const float GetDefaultBulletDistance(void)
{
	return sv_bullet_distance.value;
}

//-----------------------------------------------------------------------------
// Purpose: Go to the trouble of combining multiple pellets into a single damage call.
// Warning: Weapon code should be aware of possibility that m_pPlayer/Owner gets NULLed after a call to this function!
// Input  : cShots - number of bullets
//			&vecSrc - trace source
//			&vecDirShooting - 
//			&vecSpread - 
//			*endpos - output: hit position
//			flDistance - maximum distance
//			iBulletType - Bullet enum
//			fDamage - can override standard skill-based bullet damage
//			*pInflictor - attacker's gun
//			*pAttacker - the attacker
//			shared_rand - for players
// Output : Vector 
//-----------------------------------------------------------------------------
Vector FireBullets(ULONG cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, Vector *endpos, const float flDistance, const int iBulletType, const float fDamage, CBaseEntity *pInflictor, CBaseEntity *pAttacker, int shared_rand)
{
	if (pInflictor == NULL)
	{
#if defined (_DEBUG)
		conprintf(0, "FireBullets() with no pInflictor!\n");
#endif
		return vecSpread;
	}

	if (pAttacker == NULL)
		pAttacker = pInflictor;// the default attacker is ourselves

	int pc;
	int bitsDamageType = DMG_BULLET;// XDM3037a
	float x = 0.0, y = 0.0;
	float fVelMult = 1.0;// XDM3037a
	float flDamage = fDamage, flDamageDecrease;// XDM3037a
	edict_t *pentIgnore = NULL;
	TraceResult tr;

	if (pInflictor && pInflictor->IsBSPModel())// XDM3038: HACK for map guns
		pentIgnore = pInflictor->edict();// XDM3038: ignore Inflictor
	else if (pAttacker)
		pentIgnore = pAttacker->edict();// XDM3037a: ignore Attacker

	// gSkillData references are really obsolete here
	switch (iBulletType)// make distance based!
	{
	case BULLET_9MM:
		{
			bitsDamageType |= DMG_NEVERGIB;
			if (fDamage == 0)
			{
				if (pAttacker->IsPlayer())
					flDamage = gSkillData.Dmg9MM;
				else
					flDamage = gSkillData.monDmg9MM;
			}
		}
		break;
	case BULLET_357:
		{
			if (fDamage == 0)
				flDamage = gSkillData.Dmg357;

			fVelMult = 2.0f;
			if (IsMultiplayer())// XDM3038b
				fVelMult += sv_overdrive.value;
		}
		break;
	case BULLET_BUCKSHOT:
		{
			if (fDamage == 0)
				flDamage = gSkillData.DmgBuckshot;
		}
		break;
	case BULLET_MP5:
		{
			bitsDamageType |= DMG_NEVERGIB;
			if (fDamage == 0)
			{
				if (pAttacker->IsPlayer())
					flDamage = gSkillData.DmgMP5;
				else
					flDamage = gSkillData.monDmgMP5;
			}
		}
		break;
	case BULLET_12MM:
		{
			fVelMult = 1.5f;
			if (fDamage == 0)
				flDamage = gSkillData.Dmg12MM;
		}
		break;
	case BULLET_338:
		{
			fVelMult = 2.0f;
			if (fDamage == 0)
				flDamage = gSkillData.DmgSniperRifle;

			if (IsMultiplayer())// XDM3038b
				fVelMult += sv_overdrive.value;
		}
		break;
	default:
	case BULLET_NONE:
		{
			fVelMult = 0.0;
			flDamage = 32.0;
			//?bitsDamageType = DMG_CLUB;
		}
		break;
	}

	//if (IsMultiplayer())// XDM3038b
	//	fVelMult += sv_overdrive.value;
	CBaseEntity *pEntity;
	ClearMultiDamage();
	gMultiDamage.type = bitsDamageType;// XDM3037a: DMG_BULLET | DMG_NEVERGIB;
	Vector vecDir, vecDst;//, vecStart, vecEnd;
	//ULONG iShotsHit = 0;// XDM3035: count shots made and adjust player velocity accordingly
	for (ULONG iShot = 1; iShot <= cShots; ++iShot)
	{
		pEntity = NULL;
		if (pAttacker->IsPlayer())//Use player's random seed. get circular gaussian spread
		{
			x = UTIL_SharedRandomFloat(shared_rand + /*0+*/iShot, -0.5f, 0.5f) + UTIL_SharedRandomFloat(shared_rand + (1 + iShot), -0.5f, 0.5f);
			y = UTIL_SharedRandomFloat(shared_rand + (2 + iShot), -0.5f, 0.5f) + UTIL_SharedRandomFloat(shared_rand + (3 + iShot), -0.5f, 0.5f);
		}
		else
		{
			x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		}
		// vecDir = vecDirShooting + x*vecSpread.x*gpGlobals->v_right + y*vecSpread.y*gpGlobals->v_up;
		vecDir = x*vecSpread.x*gpGlobals->v_right + y*vecSpread.y*gpGlobals->v_up;
		vecDir += vecDirShooting;
		vecDir.NormalizeSelf();// XDM3038c: !!
		//vecEnd = vecSrc;// XDM3037a
		// vecDst = vecSrc + vecDir * flDistance;
		vecDst = vecDir; vecDst *= flDistance; vecDst += vecSrc;

// UNDONE: pierce through thin walls or entities. Too complicated: MultiDamage issues and also expect lots and lots of TraceLine shit
//		for (short i=0; i<=BULLET_MAX_PIERCE; ++i)// 0th doesn't count
//		{
			// don't hit one entity twice (or more)!
			//UTIL_ShowLine(vecSrc, vecEnd, 2.0, 255,0,0);
			//vecStart = vecEnd;
			UTIL_TraceLine(vecSrc/*vecStart*/, vecDst, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);

			// new end == start for the next iteration
			//vecEnd = tr.vecEndPos;
			//UTIL_ShowLine(vecSrc, tr.vecEndPos, 2.5, 0,0,255);
			if (endpos)// output
			{
				/*endpos->x = tr.vecEndPos.x;// vecEnd?
				endpos->y = tr.vecEndPos.y;
				endpos->z = tr.vecEndPos.z;*/
				*endpos = tr.vecEndPos;
			}

		//	conprintf(0, "FireBullets(%ul (%.2f %.2f %.2f)(%.2f %.2f %.2f)(%.2f %.2f %.2f) dist %f, typ %d, dmg %d, atkr: %d, rnd %d)\n",
		//		cShots, vecSrc.x, vecSrc.y, vecSrc.x, vecDirShooting.x, vecDirShooting.y, vecDirShooting.x, vecSpread.x, vecSpread.y, vecSpread.z, flDistance, iBulletType, iDamage, pAttacker->entindex(), shared_rand);

			switch(iBulletType)// always draw bullet tracer effect
			{
			default:
				{
					MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSrc);
						WRITE_BYTE(TE_TRACER);
						WRITE_COORD(vecSrc.x);
						WRITE_COORD(vecSrc.y);
						WRITE_COORD(vecSrc.z);
						WRITE_COORD(tr.vecEndPos.x);
						WRITE_COORD(tr.vecEndPos.y);
						WRITE_COORD(tr.vecEndPos.z);
					MESSAGE_END();
					//UTIL_ShowLine(vecSrc, tr.vecEndPos, 2.0);
				}
				break;
			case BULLET_NONE:
			case BULLET_BREAK:
			//case BULLET_GAUSS:
				break;
			}

			pc = POINT_CONTENTS(tr.vecEndPos);// XDM3037a: was tr.vecEndPos

			if (tr.flFraction < 1.0f)
			{
				pEntity = CBaseEntity::Instance(tr.pHit);
				if (!pEntity)// TEXTURETYPE_Trace depends on this
					continue;
				if (pEntity == pInflictor)// XDM3038: should we?
					continue;// happens when bad mappers place bsp guns

				// decrease damage over distance
				vec_t flDist = (tr.vecEndPos - vecSrc).Length();// length from the VERY beginning to current hit point
				if (flDist/flDistance > BULLET_FULLDAMAGE_DIST_K)// should always be true: && flDist < flDistance)
				{
					flDamageDecrease = (flDistance - flDist)*BULLET_DAMAGE_DECREASE;
					if (flDamageDecrease >= flDamage)
						continue;// break; - stop all piercing iterations here
				}
				else
					flDamageDecrease = 0.0f;

				// BUGBUG: network code distorts angles when entity is player
				if (pc != CONTENTS_SKY)// XDM3038: don't hit sky
				{
					int tex = (int)TEXTURETYPE_Trace(pEntity, vecSrc, tr.vecEndPos + vecDir);// 1U farther than tr.vecEndPos // XDM3038: new format, pEntity != NULL here
					if (tex != CHAR_TEX_NULL && tex != CHAR_TEX_SKY)// XDM3038: :P
						PLAYBACK_EVENT_FULL(FEV_RELIABLE, NULL, g_usBulletImpact, 0.0, tr.vecEndPos.As3f(), tr.vecPlaneNormal.As3f(), tr.flFraction, 0.0, iBulletType, tex, pEntity->IsBSPModel()?1:0, (pc > CONTENTS_WATER)?1:0);
				}
				pEntity->TraceAttack(pAttacker, flDamage-flDamageDecrease, vecDir, &tr, bitsDamageType);// AddMultiDamage is here

				if (pc != CONTENTS_SKY)// XDM3038: I didn't merge these two IFs because I value packet order.
				{
					if (fDamage > 0.0f)
					{
						DecalGunshot(&tr, iBulletType);
					}
					else if (iBulletType == BULLET_NONE)// ?
					{
						// only decal glass
						if (!FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != kRenderNormal)
							UTIL_DecalTrace(&tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0,2));
					}
				}
			}// tr.flFraction != 1.0
			// Underwater trails: XDM3037: RSBubbles will only create bubbles in water, so we're safe to call FX_BubblesLine at any time
			if (/*tr.fInWater && */(pc <= CONTENTS_WATER && pc > CONTENTS_SKY))// XDM3035c: somehow fInWater is always 0
				FX_BubblesLine(vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0f);
		//}
	}
	/*if (gMultiDamage.pEntity)
	{
		if (gMultiDamage.pEntity->IsPushable() && fVelMult != 0.0f)
		{
			gMultiDamage.pEntity->pev->velocity += vecDirShooting/*.Normalize()* / * fVelMult * gMultiDamage.pEntity->DamageForce(gMultiDamage.amount);//gMultiDamage.amount;// *iShotsHit;
			//conprintf(0, "FireBullets: norm %f dforce %f\n", gMultiDamage.amount, gMultiDamage.pEntity->DamageForce(gMultiDamage.amount));
		}
	}*/
	ApplyMultiDamage(pInflictor, pAttacker);
	return Vector(x * vecSpread.x, y * vecSpread.y, 0.0f);
}
