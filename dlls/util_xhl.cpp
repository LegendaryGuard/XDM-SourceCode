//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "animation.h"
#include "weapons.h"
#include "globals.h"
#include "trains.h"
#include "pm_materials.h"


//-----------------------------------------------------------------------------
// Purpose: Break loading at any time no matter what (HACK).
// Warning: return after calling this!
//-----------------------------------------------------------------------------
void UTIL_FAIL(void)
{
	(*g_engfuncs.pfnSetModel)(g_pWorld->edict(), ".");// breaks loading
}

//-----------------------------------------------------------------------------
// Purpose: Engine bullshit override for crash/exploit prevention
// Input  : *szClassname - temporary classname for checks, set with MAKE_STRING!!
// Output : edict_t * - can be NULL
//-----------------------------------------------------------------------------
edict_t *CREATE_NAMED_ENTITY(const char *szClassname)
{
	if (szClassname == NULL)
		return NULL;
	if (szClassname[0] == '?')
	{
		UTIL_LogPrintf("SECURITY WARNING: tried to create classname \"%s\"!\n", szClassname);
		return NULL;
	}
	uint32 i = 0;
	while (g_szNonEntityExports[i] != NULL)
	{
		if (strcmp(szClassname, g_szNonEntityExports[i]) == 0)
		{
			UTIL_LogPrintf("SECURITY WARNING: tried to create classname \"%s\"!\n", szClassname);
			return NULL;
		}
		++i;
	}
	return g_engfuncs.pfnCreateNamedEntity(MAKE_STRING(szClassname));// allocate edict and call DLL export function. String is TEMPORARY!!!
}

//-----------------------------------------------------------------------------
// Purpose: Not a brush model
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsPointEntity(CBaseEntity *pEnt)
{
	if (pEnt == NULL)
		return false;
	if (pEnt->pev->modelindex == 0)
		return true;
	// Nobody writes hacks on MY watch! if (FClassnameIs(pEnt->pev, "info_target") || FClassnameIs(pEnt->pev, "info_landmark") || FClassnameIs(pEnt->pev, "path_corner"))
	//	return true;
	if (!pEnt->IsBSPModel())// XDM3035a
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Show to nearby clients
// Input  : *pString - 
//			&center - 
//			radius - 
//-----------------------------------------------------------------------------
void UTIL_ShowMessageRadius(const char *pString, const Vector &center, int radius)
{
	if (!ASSERT(pString != NULL))
		return;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			Vector delta(center - pPlayer->pev->origin);
			if (delta.Length() <= radius)
				ClientPrint(pPlayer->pev, HUD_PRINTHUD, pString);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Internal, Universal, for all music APIs on client side
// Input  : *pPlayer - NULL for all
//			*pTrackName - track/playlist
//			iCommand - MUSICPLAYER_CMD enum
//			iOffset - offset in seconds
//-----------------------------------------------------------------------------
void PlayAudioTrack(CBasePlayer *pPlayer, const char *pTrackName, int iCommand, const float &fTimeOffset)
{
	if ((iCommand == MPSVCMD_PLAYTRACK || iCommand == MPSVCMD_PLAYTRACKLOOP) && (pTrackName == NULL || pTrackName[0] == 0))// some commands are allowed without name
	{
		conprintf(2, "SV: PlayAudioTrack(%d \"%s\" %d %g) error: no track specified!\n", pPlayer?pPlayer->entindex():0, pTrackName, iCommand, fTimeOffset);
		return;
	}
	else
		conprintf(2, "SV: PlayAudioTrack(%d \"%s\" %d %g)\n", pPlayer?pPlayer->entindex():0, pTrackName, iCommand, fTimeOffset);

	if (pPlayer == NULL)// everyone
	{
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CBasePlayer *pClient = UTIL_ClientByIndex(i);
			if (pClient)
				pClient->PlayAudioTrack(pTrackName, iCommand, fTimeOffset);
		}
	}
	else
		pPlayer->PlayAudioTrack(pTrackName, iCommand, fTimeOffset);
}

//-----------------------------------------------------------------------------
// Purpose: gmsgParticles message wrapper
//-----------------------------------------------------------------------------
void ParticlesCustom(const Vector &vecPos, float rnd_vel, float life, byte color_pal, byte number)
{
	MESSAGE_BEGIN(MSG_PVS, gmsgParticles, vecPos);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_SHORT(((int)rnd_vel)*10);
		WRITE_SHORT(((int)life)*10);
		WRITE_BYTE(color_pal);
		WRITE_BYTE(number);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_GLOWSPRITE message wrapper
//-----------------------------------------------------------------------------
void GlowSprite(const Vector &vecPos, int mdl_idx, int life, int scale, int fade)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(TE_GLOWSPRITE);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_SHORT(mdl_idx);
		WRITE_BYTE(min(life, 255));
		WRITE_BYTE(min(scale, 255));
		WRITE_BYTE(min(fade, 255));
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_SPRITETRAIL message wrapper
//-----------------------------------------------------------------------------
void SpriteTrail(const Vector &vecPos, const Vector &vecEnd, int mdl_idx, int count, int life, int scale, int vel, int rnd_vel)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(TE_SPRITETRAIL);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_COORD(vecEnd.x);
		WRITE_COORD(vecEnd.y);
		WRITE_COORD(vecEnd.z);
		WRITE_SHORT(mdl_idx);
		WRITE_BYTE(count);
		WRITE_BYTE(life);
		WRITE_BYTE(scale);
		WRITE_BYTE(vel);
		WRITE_BYTE(rnd_vel);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_DLIGHT message wrapper
// TODO: radius and decay should probably be floats
//-----------------------------------------------------------------------------
void DynamicLight(const Vector &vecPos, int radius, int r, int g, int b, int life, int decay)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_BYTE(min(radius, 255));
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
		WRITE_BYTE(life);
		WRITE_BYTE(decay);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_ELIGHT message wrapper
// TODO: radius and decay should probably be floats
//-----------------------------------------------------------------------------
void EntityLight(int entidx, const Vector &vecPos, int radius, int r, int g, int b, int life, int decay)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(TE_ELIGHT);
		WRITE_SHORT(entidx);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_COORD((float)radius);
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
		WRITE_BYTE(life);
		WRITE_COORD((float)decay);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: ParticleSystem message wrapper
//-----------------------------------------------------------------------------
void PartSystem(const Vector &vecPos, const Vector &vecDir, const Vector &vecSpreadSize, int sprindex, int rendermode, int type, int max_parts, int life, int flags, int ent)
{
	int msg;
	if (type == PARTSYSTEM_TYPE_REMOVEANY)// send 'remove' message to all clients
		msg = MSG_ALL;
	else
		msg = MSG_PVS;

	MESSAGE_BEGIN(msg, gmsgPartSys, vecPos);
		WRITE_COORD(vecPos.x);// origin
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_COORD(vecDir.x);// direction
		WRITE_COORD(vecDir.y);
		WRITE_COORD(vecDir.z);
		WRITE_COORD(vecSpreadSize.x);// spread for flame
		WRITE_COORD(vecSpreadSize.y);// or spark size
		WRITE_COORD(vecSpreadSize.z);
		WRITE_SHORT(sprindex);	// sprite name
		WRITE_BYTE(rendermode);	// render mode
		//WRITE_BYTE(adelta);	// brightness decrease per second
		WRITE_BYTE(type);		// flame/sparks
		WRITE_SHORT(max_parts);	// max particles
		WRITE_SHORT(life);		// life
		WRITE_SHORT(flags);		// flags XDM3035: extended
		WRITE_SHORT(ent);		// follow entity index
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_BEAM_ message wrapper
// use TE_BEAMPOINTS, TE_BEAMTORUS, TE_BEAMDISK or TE_BEAMCYLINDER as types, Parameters are NOT converted (e.g. life*0.1, etc.)
//-----------------------------------------------------------------------------
void BeamEffect(int type, const Vector &vecPos, const Vector &vecAxis, int spriteindex, int startframe, int fps, int life, int width, int noise, const Vector &color, int brightness, int speed)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(type);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_COORD(vecAxis.x);
		WRITE_COORD(vecAxis.y);
		WRITE_COORD(vecAxis.z);
		WRITE_SHORT(spriteindex);
		WRITE_BYTE(min(startframe, 255));
		WRITE_BYTE(min(fps, 255));
		WRITE_BYTE(min(life, 255));
		WRITE_BYTE(min(width, 255));
		WRITE_BYTE(min(noise, 255));
		WRITE_BYTE(min(color.x, 255));
		WRITE_BYTE(min(color.y, 255));
		WRITE_BYTE(min(color.z, 255));
		WRITE_BYTE(min(brightness, 255));
		WRITE_BYTE(min(speed, 255));
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_STREAK_SPLASH message wrapper
// see "r_efx.h" for colors
//-----------------------------------------------------------------------------
void StreakSplash(const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, origin);
		WRITE_BYTE(TE_STREAK_SPLASH);
		WRITE_COORD(origin.x);
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_BYTE(min(color, 255));
		WRITE_SHORT(count);
		WRITE_SHORT(speed);
		WRITE_SHORT(velocityRange);// Random velocity modifier
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_PARTICLEBURST message wrapper
//-----------------------------------------------------------------------------
void ParticleBurst(const Vector &origin, int radius, int color, int duration)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, origin);
		WRITE_BYTE(TE_PARTICLEBURST);
		WRITE_COORD(origin.x);
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_SHORT(radius);
		WRITE_BYTE(min(color, 255));
		WRITE_BYTE(min(duration, 255));
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_LINE message wrapper (for debugging mostly)
//-----------------------------------------------------------------------------
void UTIL_ShowLine(const Vector &start, const Vector &end, float life, byte r, byte g, byte b)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, start);
		WRITE_BYTE(TE_LINE);
		WRITE_COORD(start.x);
		WRITE_COORD(start.y);
		WRITE_COORD(start.z);
		WRITE_COORD(end.x);
		WRITE_COORD(end.y);
		WRITE_COORD(end.z);
		WRITE_SHORT((int)(life*10.0f));
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: TE_BOX message wrapper (for debugging mostly)
//-----------------------------------------------------------------------------
void UTIL_ShowBox(const Vector &absmins, const Vector &absmaxs, float life, byte r, byte g, byte b)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, (absmins + absmaxs) * 0.5f);
		WRITE_BYTE(TE_BOX);
		WRITE_COORD(absmins.x);
		WRITE_COORD(absmins.y);
		WRITE_COORD(absmins.z);
		WRITE_COORD(absmaxs.x);
		WRITE_COORD(absmaxs.y);
		WRITE_COORD(absmaxs.z);
		WRITE_SHORT((int)(life*10.0f));
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: 
// how to make this work properly?
//-----------------------------------------------------------------------------
void UTIL_ShowBox(const Vector &origin, const Vector &mins, const Vector &maxs, float life, byte r, byte g, byte b)
{
	UTIL_ShowBox(origin + mins, origin + maxs, life, r, g, b);
}

//-----------------------------------------------------------------------------
// Purpose: Place a decal by tracing from one vector to another
//-----------------------------------------------------------------------------
void UTIL_DecalPoints(const Vector &src, const Vector &end, edict_t *pent, int decalIndex)
{
	TraceResult tr;
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
	UTIL_TraceLine(src, end, dont_ignore_monsters, pent, &tr);
	UTIL_DecalTrace(&tr, decalIndex);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
}

//-----------------------------------------------------------------------------
// Purpose: Black hole effect
// WARNING! Can modify pInflictor's pev->dmg!!! <--- UNDONE
// Input  : vecSpot - origin of attack
//			*pInflictor - a weapon, projectile, trigger, etc.
//			*pAttacker - player, monster, or anything else owning the gun
//			flDamage - amount of damage
//			flRadius - radius of implosion
//			bKeepWeapons - don't disintegrate weapons
//-----------------------------------------------------------------------------
void BlackHoleImplosion(const Vector &vecSpot, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, float flRadius, bool bKeepWeapons)
{
	//ASSERT(pInflictor != NULL);
	ASSERT(pAttacker != NULL);
	CBaseEntity *pEntity = NULL;
	int iDamageType;
	vec_t distance;
	Vector delta;
	TraceResult tr;
	bool fractionpassed = false;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSpot, flRadius)) != NULL)
	{
		if (pEntity == pInflictor)// recursion?
		{
			//DBG_PRINT("BlackHoleImplosion: SKIP: pEntity == pInflictor\n");
			continue;
		}
		if (pEntity->GetExistenceState() != ENTITY_EXSTATE_WORLD)// XDM3038c: don't damage virtual entities
			continue;
		if (pEntity->IsRemoving())// XDM3038a: skip entities that are about to be removed
			continue;

		UTIL_TraceLine(vecSpot, pEntity->Center(), ignore_monsters, dont_ignore_glass, pInflictor?pInflictor->edict():NULL, &tr);

		//fractionpassed = false;// XDM3035
		if (tr.flFraction == 1.0f)
			fractionpassed = true;
		else if (tr.pHit == pEntity->edict())//if (pEntity->IsBSPModel() && pEntity->IsPushable())
			fractionpassed = true;// the entity is blocking trace line itself
		else
			fractionpassed = false;

		iDamageType = DMG_RADIATION | DMG_NOHITPOINT | DMG_DONT_BLEED;// XDM3037: no grenade-headshots!

		if (fractionpassed && !FBitSet(pEntity->pev->effects, EF_NODRAW)/* && !pEntity->IsBSPModel() *//*&& pEntity->IsAlive()*/ && pEntity != pInflictor)
		{
			if (pEntity->IsProjectile())// String comparison is MUCH slower...
			{
				if (pEntity->pev->impulse == 1 && FClassnameIs(pEntity->pev, "blackball"))// found a black hole that is not exploding yet
				{
					//pInflictor->pev->dmg += pEntity->pev->dmg;// FAIL! Because we must modify effect radius on client side!
					//pEntity->pev->dmg = 0.0f;
					pEntity->pev->air_finished = gpGlobals->time;// make it shut down immediately
				}
				else if (pEntity->pev->solid != SOLID_NOT && pEntity->pev->velocity.Length() < 2000.0f)// XDM3038c // Disintegration effect for slow projectiles
				{
					/*if (FClassnameIs(pEntity->pev, "grenade") ||
					FClassnameIs(pEntity->pev, "l_grenade") ||
					FClassnameIs(pEntity->pev, "strtarget"))*/
					if (pEntity->pev->rendermode == kRenderNormal)// some ents are already transparent and not only kRenderTransTexture!
						pEntity->pev->rendermode = kRenderTransTexture;

					pEntity->pev->velocity.Clear();
					pEntity->Disintegrate();
					continue;// don't try to push
				}
				else if (FClassnameIs(pEntity->pev, "teleporter"))
				{
					if (g_pGameRules->FAllowEffects())
					{
					MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
					if (pInflictor)
					{
						WRITE_BYTE(TE_BEAMENTS);
						WRITE_SHORT(pInflictor->entindex());
						WRITE_SHORT(pEntity->entindex());
					}
					else
					{
						WRITE_BYTE(TE_BEAMENTPOINT);
						WRITE_SHORT(pEntity->entindex());
						WRITE_COORD(vecSpot.x);
						WRITE_COORD(vecSpot.y);
						WRITE_COORD(vecSpot.z);
					}
						WRITE_SHORT(g_iModelIndexLaser);
						WRITE_BYTE(0);// framestart
						WRITE_BYTE(20);// framerate
						WRITE_BYTE(5);// life
						WRITE_BYTE(20);// width
						WRITE_BYTE(60);// noise
						WRITE_BYTE(255);// r
						WRITE_BYTE(255);// g
						WRITE_BYTE(255);// b
						WRITE_BYTE(RANDOM_LONG(159,191));// brightness
						WRITE_BYTE(40);// speed
					MESSAGE_END();
					}
					pEntity->pev->teleport_time = gpGlobals->time;
					pEntity->Touch(pInflictor);//DispatchTouch(edict(), pEntity->edict());// XDM3037: special disintegration TELEPORTER_BT_COL_PROJECTILE
					continue;// don't try to push
				}
				else// if (FClassnameIs(pEntity->pev, SATCHEL_CLASSNAME) || FClassnameIs(pEntity->pev, TRIPMINE_CLASSNAME))
				{
					pEntity->Killed(pInflictor, pAttacker, GIB_DISINTEGRATE);// Detonate()!
					continue;// don't try to push
				}
			}
			else if (pEntity->IsBSPModel())
			{
				delta = vecSpot;
				delta -= pEntity->Center();
			}
			/*else if (pEntity->IsPickup())
			{
				delta = vecSpot;
				delta -= pEntity->pev->origin;
			}*/
			else
			{
				delta = vecSpot;
				delta -= pEntity->pev->origin;
			}

			distance = delta.Length();
			if (pEntity->IsPushable())
			{
				vec_t k = distance*4.0f;// (radius-distance) adds more velocity to nearby entities, we need the opposite effect
				//pEntity->MyMonsterPointer()->IsRunningScript?
				if (pEntity->IsMonster() && !FBitSet(pEntity->pev->flags, FL_GODMODE))// XDM3035: invincible entities (scripted/multiplayer)
				{
					k *= 0.8f;// slow down a little?
					pEntity->pev->movetype = MOVETYPE_TOSS;// TODO: set for all ents, not just monsters? But this disables noclip for players =)
					if (pEntity->IsAlive() && pEntity->pev->takedamage != DAMAGE_NO)
					{
						pEntity->TakeDamage(pInflictor, pAttacker, flDamage*0.1f, DMG_DROWN);// XDM3035: replace this with disintegration?
					}
					else
					{
						if (distance < GREN_TELEPORT_CRITICAL_RADIUS)
						{
							pEntity->pev->velocity.Clear();
							pEntity->MyMonsterPointer()->FadeMonster();// !!! this clears out velocity and movetype!!
							//pEntity->pev->movetype = MOVETYPE_NONE;
						}
					}

					if (distance < GREN_TELEPORT_CRITICAL_RADIUS)// XDM3035: only disintegrate those who close enough
						iDamageType |= DMG_DISINTEGRATING;// NOT both!
					else
						iDamageType |= DMG_NEVERGIB;// NOT both!
				}
				else if (pEntity->IsPlayer() && ((CBasePlayer *)pEntity)->IsOnLadder())
				{
					((CBasePlayer *)pEntity)->DisableLadder(1.0f);
				}
				else if (pEntity->IsPickup()/*IsPlayerItem()*/ && !bKeepWeapons)
				{
					if (distance < GREN_TELEPORT_CRITICAL_RADIUS && FBitSet(pEntity->pev->spawnflags, SF_NORESPAWN))
					{
						if (pEntity->GetExistenceState() != ENTITY_EXSTATE_CONTAINER && pEntity->GetExistenceState() != ENTITY_EXSTATE_CARRIED)
						{
							pEntity->pev->velocity.Clear();
							pEntity->Disintegrate();
						}
					}
				}
				//conprintf(0, "--- tgrenade: pushable %s at %f, k = %f\n", STRING(pEntity->pev->classname), d, k);

				if (pEntity->pev->waterlevel > WATERLEVEL_FEET)// water helps a little
					k *= 0.8f;

				if (FBitSet(pEntity->pev->flags, FL_DUCKING))// pretend he's holding onto the ground
					k *= 0.8f;

				pEntity->pev->velocity = pEntity->pev->velocity*0.5f + delta.Normalize()*k;
			}

			if (pEntity->pev->takedamage != DAMAGE_NO)
				pEntity->TakeDamage(pInflictor, pAttacker, flDamage*max(0, 1.0f - distance/flRadius), iDamageType);

		}// visible
	}// while
}

//-----------------------------------------------------------------------------
// Purpose: moved here from crowbar.cpp because it's now a shared function.
// Input  : &vecSrc -
//			&tr -
//			*mins -
//			*maxs -
//			*pEntity -
//-----------------------------------------------------------------------------
void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity)
{
	Vector vecHullEnd(vecSrc + ((tr.vecEndPos - vecSrc)*2.0f));// XDM3037: fixed
	TraceResult tmpTrace;
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0f)
	{
		tr = tmpTrace;
		return;
	}
	vec_t thisDistance;
	vec_t distance = 1e6f;
	float *minmaxs[2] = {mins, maxs};
	Vector vecEnd;
	short i, j, k;
	for (i = 0; i < 2; ++i)
	{
		for (j = 0; j < 2; ++j)
		{
			for (k = 0; k < 2; ++k)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];
				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0f)
				{
					thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simplified function
// Input  : *pMe -
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *UTIL_FindEntityForward(CBaseEntity *pMe)
{
	if (pMe != NULL)
	{
		TraceResult tr;
		Vector forward;
		if (pMe->IsPlayer())
			AngleVectors(pMe->pev->v_angle, forward, NULL, NULL);
		else
			AngleVectors(pMe->pev->angles, forward, NULL, NULL);

		UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + forward * g_psv_zmax->value, dont_ignore_monsters, pMe->edict(), &tr);
		if (tr.flFraction != 1.0f && !FNullEnt(tr.pHit))
			return CBaseEntity::Instance(tr.pHit);
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find entity by globalname, validate its class
// Input  : classname
//			globalname
// Output : CBaseEntity * - NULL if not valid
//-----------------------------------------------------------------------------
CBaseEntity *UTIL_FindGlobalEntity(string_t classname, string_t globalname)
{
	CBaseEntity *pReturn = UTIL_FindEntityByString(NULL, "globalname", STRING(globalname));
	if (pReturn)
	{
		if (!FClassnameIs(pReturn->pev, STRING(classname)))
		{
			conprintf(1, "Error: Global entity found \"%s\", wrong class: \"%s\" expected: \"%s\"\n", STRING(globalname), STRING(pReturn->pev->classname), STRING(classname));
			pReturn = NULL;
		}
	}
	return pReturn;
}

//-----------------------------------------------------------------------------
// Purpose: Number of body groups (root)
// Input  : *ent -
// Output : int
//-----------------------------------------------------------------------------
int GetEntBodyGroupsCount(edict_t *ent)
{
	if (!FNullEnt(ent))
	{
		void *pmodel = GET_MODEL_PTR(ent);
		if (pmodel)
			return GetBodyGroupsCount(pmodel);
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Number of texture groups (skins)
// Input  : *ent -
// Output : int
//-----------------------------------------------------------------------------
int GetEntTextureGroupsCount(edict_t *ent)
{
	if (!FNullEnt(ent))
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)GET_MODEL_PTR(ent);
		if (pstudiohdr)
			return pstudiohdr->numskinfamilies;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Number of submodels in a body group
// Input  : *ent -
// Input  : bodygroup -
// Output : int
//-----------------------------------------------------------------------------
int GetEntBodyCount(edict_t *ent, int bodygroup)
{
	if (!FNullEnt(ent))
	{
		void *pmodel = GET_MODEL_PTR(ent);
		if (pmodel)
			return GetBodyCount(pmodel, bodygroup);
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Precache all resources needed by the specified material
// Input  : *pMaterial
//-----------------------------------------------------------------------------
void UTIL_PrecacheMaterial(material_t *pMaterial)
{
	if (pMaterial == NULL)
		return;

	size_t i;

	//pStringList = pMaterial->ShardSounds;
	//count = ARRAYSIZE(*pStringList);// pMaterial->ShardSoundsNum
	for(i = 0; i < NUM_SHARD_SOUNDS; ++i)
		PRECACHE_SOUND((char *)pMaterial->ShardSounds[i]);

	//pStringList = pMaterial->BreakSounds;
	//count = ARRAYSIZE(*pStringList);// pMaterial->BreakSoundsNum
	for(i = 0; i < NUM_BREAK_SOUNDS; ++i)
		PRECACHE_SOUND((char *)pMaterial->BreakSounds[i]);

	//count = ARRAYSIZE(*pStringList);// pMaterial->PushSoundsNum
	for(i = 0; i < NUM_PUSH_SOUNDS; ++i)
		PRECACHE_SOUND((char *)pMaterial->PushSounds[i]);

	//count = ARRAYSIZE(*pStringList);// pMaterial->BreakStepSounds
	for(i = 0; i < NUM_STEP_SOUNDS; ++i)
		PRECACHE_SOUND((char *)pMaterial->StepSounds[i]);
}

//-----------------------------------------------------------------------------
// Purpose: Returns players and bots as edict_t pointer
// Warning: Minimal safety checks only! Avoid using this function!!!
// Input  : playerIndex - player/entity index
//-----------------------------------------------------------------------------
edict_t	*UTIL_ClientEdictByIndex(CLIENT_INDEX playerIndex)
{
	//if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	if (IsValidPlayerIndex(playerIndex))
	{
		edict_t *pEdict = INDEXENT(playerIndex);
		if (UTIL_IsValidEntity(pEdict))
			return pEdict;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns players and bots as CBasePlayer pointer
// Warning: Must filter out disconnected and kicked clients/bots!
// WARNING: serialnumber must be set to GETPLAYERUSERID!!!!
// Input  : playerIndex - player/entity index
//-----------------------------------------------------------------------------
CBasePlayer	*UTIL_ClientByIndex(CLIENT_INDEX playerIndex)
{
	//if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	if (IsValidPlayerIndex(playerIndex))
	{
		edict_t *pPlayerEdict = INDEXENT(playerIndex);
		if (UTIL_IsValidEntity(pPlayerEdict))
		{
			if (pPlayerEdict->serialnumber != 0)// XDM3038c
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pPlayerEdict);// XDM3037a
				if (pEntity->IsPlayer())
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					if (pPlayer->m_iSpawnState != SPAWNSTATE_UNCONNECTED)// XDM3038c
						return pPlayer;
				}
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns players and bots as CBasePlayer pointer
// Warning: Must filter out disconnected and kicked clients/bots!
// WARNING: serialnumber must be set to GETPLAYERUSERID!!!!
// Input  : playerIndex - player/entity index
//-----------------------------------------------------------------------------
CBasePlayer	*UTIL_ClientByName(const char *pName)
{
	CBasePlayer *pPlayer;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pPlayer = UTIL_ClientByIndex(i);// this also validates the client
		if (pPlayer)
		{
			if (strcmp(STRING(pPlayer->pev->netname), pName) == 0)
				return pPlayer;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns entity as CBaseEntity
// Input  : index - entity index
//-----------------------------------------------------------------------------
CBaseEntity	*UTIL_EntityByIndex(int index)
{
	if (index >= 0 && index <= gpGlobals->maxEntities)// allow world
	{
		edict_t *pEdict = INDEXENT(index);
		if (UTIL_IsValidEntity(pEdict))
			return CBaseEntity::Instance(pEdict);// XDM3035c
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Validate entity by all means! Edict version.
// Warning: Do NOT use FNullEnt()! It is total bullshit.
// Warning: Maintain explicit order of checks!
// Input  : *pEdict - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsValidEntity(const edict_t *pEdict)
{
	if (pEdict == NULL)
		return false;
	if (pEdict->free)
		return false;
	if (FBitSet(pEdict->v.flags, FL_KILLME))// XDM3037
		return false;
	if (pEdict->pvPrivateData == NULL)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Validate entity by all means!
// Warning: MUST BE BULLETPROOF!
// Input  : *pEntity - test subject
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsValidEntity(CBaseEntity *pEntity)
{
	if (pEntity)
	{
		if (pEntity->pev == NULL)
			return false;

		return UTIL_IsValidEntity(pEntity->edict());
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Special command handler for multi_manager
// Undone : "(a|b)" instead of just value, where a/b are toggled. Impossible because we can't GetKeyValue() for entities
// Input  : *strTarget - "SET targetname keyname value"
//			bOn - use the first value, if many specified
// Output : int - number of entities affected
//-----------------------------------------------------------------------------
/* OBSOLETE int UTIL_SetTargetValue(const char *strTarget, bool bOn)
{
	int count = 0;
	char strBufTargetName[64];// UNDONE: FIXME! must be a real constant here! like MAX_STRING
	char strBufKeyName[32];
	char strBufValue[160];// 256 bytes total
	// parse manager's string for "targetname key value"
	size_t ofs = 4;// source string offset
	size_t bofs = 0;// buffer offset
	while (!isspace(strTarget[ofs]))
	{
		if (strTarget[ofs] == 0)
			return 0;

		strBufTargetName[bofs] = strTarget[ofs];
		++ofs;++bofs;
	}
	if (bofs == 0)
	{
		conprintf(2, "UTIL_SetTargetValue(%s): bad SET command!\n", strTarget);
		goto finishloop;
	}
	strBufTargetName[bofs] = 0;
	while (isspace(strTarget[ofs]) && strTarget[ofs] != 0)
		++ofs;
	bofs = 0;
	while (!isspace(strTarget[ofs]) && strTarget[ofs] != 0)
	{
		strBufKeyName[bofs] = strTarget[ofs];
		++ofs;++bofs;
	}
	if (bofs == 0 || strTarget[ofs] == 0)
	{
		conprintf(2, "UTIL_SetTargetValue(%s): bad SET command argument 1!\n", strTarget);
		goto finishloop;
	}
	strBufKeyName[bofs] = 0;
	while (isspace(strTarget[ofs]) && strTarget[ofs] != 0)
		++ofs;
	bofs = 0;
	while (/*!isspace(strTarget[ofs]) && * / strTarget[ofs] != 0)// copy till the very end
	{
		strBufValue[bofs] = strTarget[ofs];
		++ofs;++bofs;
	}
	// source string should've ended by now
	if (bofs == 0)
	{
		conprintf(2, "UTIL_SetTargetValue(%s): bad SET command argument 2!\n", strTarget);
		goto finishloop;
	}
	strBufValue[bofs] = 0;

	// ?? now we can parse the value for special instructions?
	if (strBufTargetName[0])// just to avoid compiler error
	{
		KeyValueData kvd;
		kvd.szKeyName = strBufKeyName;
		kvd.szValue = strBufValue;
		kvd.fHandled = FALSE;
		edict_t	*eEntity = NULL;
		while ((eEntity = FIND_ENTITY_BY_TARGETNAME(eEntity, strBufTargetName)) != NULL)
		{
			if (FNullEnt(eEntity))
				break;
			kvd.szClassName = STRINGV(eEntity->v.classname);
			conprintf(2, "UTIL_SetTargetValue(): setting \"%s\" to \"%s\" for \"%s\"[%d]\n", strBufKeyName, strBufValue, strBufTargetName, ENTINDEX(eEntity));
			DispatchKeyValue(eEntity, &kvd);
			++count;
		}
	}
finishloop:
	return count;
}*/

//-----------------------------------------------------------------------------
// Purpose: Returns the next item in this file, and bumps the buffer pointer accordingly
// Input  : *dest - out
//			**line - from fgets()?
//			delimiter - '=' for .ini files
//			comment - '/'
//-----------------------------------------------------------------------------
/*void ParseNextItem(char *dest, char **line, char delimiter, char comment)
{
	while(**line != delimiter && **line && **line != comment && !isspace(**line))
	{
		*dest = **line;
		dest++;
		(*line)++;
	}
	if (**line != comment && strlen(*line) != 0)
		(*line)++; //preserve comments
	*dest = '\0';	//add null terminator
}*/
//const char separators[] = "\"\n";

//-----------------------------------------------------------------------------
// Purpose: Another method of getting key-value pair from a script.
// Input  : name - file name (not absolute path!)
//			*kvcallback - function which handles KV pair
//-----------------------------------------------------------------------------
/*void ParseFileKV(const char *name, void (*kvcallback) (char *key, char *value, unsigned short structurestate))
{
	FILE *pFile = LoadFile(name, "rt");
	if (!pFile)// error message already displayed
		return;

	char str[256];
	char *param = NULL;
	char *value = NULL;
	unsigned short structurestate = 0;
	unsigned int line = 0;
	while (!feof(pFile))
	{
		param = NULL;
		value = NULL;
		fgets(str, 256, pFile);
		line ++;

		if (str[0] == '/' || str[0] == ';' || str[0] == '#')
			continue;

		if (str[0] == '{')
		{
			if (structurestate == 0)
				structurestate = 1;
			else
				conprintf(0, "WARNING: Found unexpected '{' while parsing '%s' (line %d)!\n", name, line);

			continue;
		}

		if (str[0] == '}')
		{
			if (structurestate == 1)
				structurestate = 2;
			else
				conprintf(0, "WARNING: Found unexpected '}' while parsing '%s' (line %d)!\n", name, line);

			//continue;
		}
		else
		{
			param = strtok(str, separators);
			if (param == NULL)
				break;

			strtok(NULL, separators);

			value = strtok(NULL, separators);
			if (value == NULL)
				continue;
		}

		kvcallback(param, value, structurestate);

		if (structurestate == 2)
			structurestate = 0;
	}
	fclose(pFile);
}*/

//-----------------------------------------------------------------------------
// Purpose: Tosses a brass shell from passed origin at passed velocity
//-----------------------------------------------------------------------------
void EjectBrass(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int body, int soundtype)
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.
	//MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecOrigin);
	//	WRITE_BYTE(TE_MODEL);
	MESSAGE_BEGIN(MSG_PVS, gmsgTEModel, vecOrigin);
		WRITE_COORD(vecOrigin.x);
		WRITE_COORD(vecOrigin.y);
		WRITE_COORD(vecOrigin.z);
		WRITE_COORD(vecVelocity.x);
		WRITE_COORD(vecVelocity.y);
		WRITE_COORD(vecVelocity.z);
		WRITE_ANGLE(rotation);
		WRITE_SHORT(model);
		WRITE_BYTE(body);
		WRITE_BYTE(soundtype);
		WRITE_BYTE(25);// 2.5 seconds
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// SHL - randomized vectors of the form "x y z .. a b c"
// Purpose: LRC sure did write a lot of useless code...
// Output : Returns true on success, false on failure (and also zeroes the output vector).
//-----------------------------------------------------------------------------
bool UTIL_StringToRandomVector(float *pVector, const char *str)
{
	float pVecMin[3];
	float pVecMax[3];
	int fields = sscanf(str, "%f %f %f .. %f %f %f", &pVecMin[0], &pVecMin[1], &pVecMin[2], &pVecMax[0], &pVecMax[1], &pVecMax[2]);
	if (fields == 6)
	{
		pVector[0] = RANDOM_FLOAT(pVecMin[0], pVecMax[0]);
		pVector[1] = RANDOM_FLOAT(pVecMin[1], pVecMax[1]);
		pVector[2] = RANDOM_FLOAT(pVecMin[2], pVecMax[2]);
		return true;
	}
	/*if (StringToVec(str, pVecMax))
	{
		pVector[0] = RANDOM_FLOAT(0, pVecMax[0]);
		pVector[1] = RANDOM_FLOAT(0, pVecMax[1]);
		pVector[2] = RANDOM_FLOAT(0, pVecMax[2]);
	}*/
	else
	{
		pVector[0] = 0.0f;
		pVector[1] = 0.0f;
		pVector[2] = 0.0f;
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: FIX: wc/hammer and other editors may set color to "0 0 0" by default
// Warning: Use only in KeyValue/Spawn functions! rendermode must be set!
// Input  : &rendermode - 
//			*rendercolor - 
//-----------------------------------------------------------------------------
void UTIL_FixRenderColor(const int &rendermode, float *rendercolor)
{
	if (rendermode == kRenderTransAdd || rendermode == kRenderTransTexture || rendermode == kRenderGlow)
	{
		if (rendercolor[0] == 0.0f && rendercolor[1] == 0.0f && rendercolor[2] == 0.0f)
			rendercolor[0] = rendercolor[1] = rendercolor[2] = 255.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: GetWeaponWorldScale. May get called while g_pGameRules is NULL!
// Output : float
//-----------------------------------------------------------------------------
float UTIL_GetWeaponWorldScale(void)
{
	if (g_pGameRules)
		return g_pGameRules->GetWeaponWorldScale();
	else if (gpGlobals->deathmatch > 0.0f && sv_weaponsscale.value > 0.0f)// XDM3035b
		return clamp(sv_weaponsscale.value, 1.0f, WEAPON_WORLD_SCALE);
	else
		return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Use this instead of SET_VIEW!
// Input  : *pClient - 
//			*pViewent - 
//-----------------------------------------------------------------------------
void UTIL_SetView(edict_t *pClient, edict_t *pViewEnt)
{
	if (pClient)
	{
		if (UTIL_IsValidEntity(pViewEnt))
		{
			(*g_engfuncs.pfnSetView)(pClient, pViewEnt);
			if (pClient == pViewEnt)
				pClient->v.euser2 = NULL;
			else
				pClient->v.euser2 = pViewEnt;
		}
		else
			pClient->v.euser2 = NULL;// XDM3038a
	}
}

//-----------------------------------------------------------------------------
// Purpose: Advanced bubble effects
//-----------------------------------------------------------------------------
void FX_BubblesPoint(const Vector &center, const Vector &spread, int count)
{
	MESSAGE_BEGIN(MSG_PVS, gmsgBubbles, center);
		WRITE_BYTE(PSSTARTTYPE_POINT);
		WRITE_BYTE(count);
		WRITE_COORD(center.x);
		WRITE_COORD(center.y);
		WRITE_COORD(center.z);
		WRITE_COORD(spread.x);
		WRITE_COORD(spread.y);
		WRITE_COORD(spread.z);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: Advanced bubble effects
//-----------------------------------------------------------------------------
void FX_BubblesSphere(const Vector &center, float radius, int count)
{
	MESSAGE_BEGIN(MSG_PVS, gmsgBubbles, center);
		WRITE_BYTE(PSSTARTTYPE_SPHERE);
		WRITE_BYTE(count);
		WRITE_COORD(center.x);
		WRITE_COORD(center.y);
		WRITE_COORD(center.z);
		WRITE_COORD(radius);
		WRITE_COORD(radius);
		WRITE_COORD(radius);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: Advanced bubble effects
//-----------------------------------------------------------------------------
void FX_BubblesBox(const Vector &center, const Vector &halfbox, int count)
{
	MESSAGE_BEGIN(MSG_PVS, gmsgBubbles, center);
		WRITE_BYTE(PSSTARTTYPE_BOX);
		WRITE_BYTE(count);
		WRITE_COORD(center.x);
		WRITE_COORD(center.y);
		WRITE_COORD(center.z);
		WRITE_COORD(halfbox.x);
		WRITE_COORD(halfbox.y);
		WRITE_COORD(halfbox.z);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: Advanced bubble effects
//-----------------------------------------------------------------------------
void FX_BubblesLine(const Vector &start, const Vector &end, int count)
{
	MESSAGE_BEGIN(MSG_PVS, gmsgBubbles, (start+end)/2);
		WRITE_BYTE(PSSTARTTYPE_LINE);
		WRITE_BYTE(count);
		WRITE_COORD(start.x);
		WRITE_COORD(start.y);
		WRITE_COORD(start.z);
		WRITE_COORD(end.x);
		WRITE_COORD(end.y);
		WRITE_COORD(end.z);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: Abstraction between both DLLs
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsMultiplayer(void)
{
	if (g_pGameRules)
		return g_pGameRules->IsMultiplayer();

	return (gpGlobals->maxClients > 1);
}

//-----------------------------------------------------------------------------
// Purpose: Abstraction between both DLLs
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsGameOver(void)
{
	if (g_pGameRules)
		return g_pGameRules->IsGameOver();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: GTFO
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsSafeToRemove(edict_t *pent)
{
	if (pent && !FBitSet(pent->v.flags, FL_CLIENT) && FBitSet(pent->v.spawnflags, SF_NORESPAWN) && FStringNull(pent->v.targetname) && FStringNull(pent->v.globalname) && FStringNull(pent->v.target) && pent->v.aiment == NULL && pent->v.owner == NULL)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(pent);
		if (pEntity)
		{
#if defined(MOVEWITH)// if this bunch of hacks is desired
			if (pEntity->m_pChildMoveWith == NULL)
			{
#endif
			if (pEntity->GetExistenceState() == ENTITY_EXSTATE_WORLD && pEntity->m_hOwner.Get() == NULL)
			{
				if (!pEntity->IsTrigger() && !pEntity->IsGameGoal() && !pEntity->IsMovingBSP() && !pEntity->ShouldRespawn())
				{
					if (pEntity->IsPickup() || pEntity->IsMonster() || (pEntity->IsPushable() && pEntity->IsBreakable()))
						return true;
				}
			}
#if defined(MOVEWITH)// if this bunch of hacks is desired
			}
#endif
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Callback for FireTargets
// ...
// Output : Returns true if we should use this entity
//-----------------------------------------------------------------------------
bool FSetTeamColor(CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller)// XDM3037
{
	if (pEntity && pCaller)
	{
		pEntity->pev->rendercolor = pCaller->pev->rendercolor;
		pEntity->pev->skin = pCaller->pev->team;
		pEntity->pev->team = pCaller->pev->team;
		return true;// use it
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Perform some actions on an entity
// Note   : Can be called by ClientCommand and possibly from somewhere else
// Warning: Printed text may overflow and drop the player!!! Save every byte!
// Input  : start_argument - start reading commands from this one
//			argc - total number of arguments
//			args[] - array arguments (pointers to terminated strings)
//			*pEntity - target
//			*pUser - user, activator (can be NULL)
// Output : last processed argument + 1
//-----------------------------------------------------------------------------
size_t Cmd_EntityAction(size_t start_argument, size_t argc, const char *args[], CBaseEntity *pEntity, CBaseEntity *pUser, bool bInteractive)
{
	if (pEntity == NULL)
		return 0;

	size_t option;// = start_argument;
	int printlevel = bInteractive?0:2;
	const int msg_dest = HUD_PRINTCONSOLE;
	//int argc = CMD_ARGC();// includes the command itself
	entvars_t *pevClient;
	const char *act;
	bool printtext;// special measure to prevent netchan overflow
	if (sv_script_cmd_printoutput.value > 0.0f)
		printtext = true;
	else
		printtext = false;

	if (bInteractive && pUser && pUser->IsPlayer())
		pevClient = pUser->pev;
	else
		pevClient = NULL;// print to all

	if (bInteractive && pEntity->IsPlayer() && sv_script_cmd_affectplayers.value <= 0.0f)
	{
		if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, "Skipping: %s[%d]: not allowed\n", STRING(pEntity->pev->classname), pEntity->entindex());}
		return 0;
	}
	if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, "%s[%d] %s (%s) <%s>\n", STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->targetname), STRING(pEntity->pev->model), FBitSet(pEntity->pev->spawnflags, SF_NOREFERENCE)?"noref":"ref");}

	for (option = start_argument; option < argc; ++option)//while (option < argc)
	{
		act = args[option];
		if (act == NULL || *act == 0)
		{
			option = argc;
			//break;
		}
		else if (strcmp(act, "noecho") == 0)
		{
			if (bInteractive)
				printtext = false;//printlevel = 10;
		}
		else if (strcmp(act, "del") == 0)
		{
			if (pEntity->GetExistenceState() == ENTITY_EXSTATE_WORLD)// don't delete items from someone's inventory!
			{
				if (!pEntity->IsPlayer())
				{
					SetBits(pEntity->pev->spawnflags, SF_NORESPAWN);// XDM3038c: prevent mass respawning in multiplayer
					UTIL_Remove(pEntity);
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - deleted;\n");}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: skipped player!\n", act);}

				return option;// SKIP ALL SUBSEQUENT ACTIONS!
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - skipped (not in world);\n");}
		}
		else if (strcmp(act, "show") == 0)
		{
			if (pUser)
			{
				uint32 es = pEntity->GetExistenceState();
				if (es == ENTITY_EXSTATE_WORLD || es == ENTITY_EXSTATE_CONTAINER)
				{
					Vector c(pEntity->Center());// we assume that entity follows container physically when es == ENTITY_EXSTATE_CONTAINER
					if (pUser->IsPlayer())
						MESSAGE_BEGIN(MSG_ONE_UNRELIABLE/*prevent overflow*/, svc_temp_entity, pUser->pev->origin, pUser->edict());
					else
						MESSAGE_BEGIN(MSG_PVS/*prevent overflow*/, svc_temp_entity, pUser->pev->origin, NULL);

						WRITE_BYTE(TE_BEAMENTPOINT);// BEAMENTS doesn't recognize brushes
						WRITE_SHORT(pUser->entindex());
						WRITE_COORD(c.x);
						WRITE_COORD(c.y);
						WRITE_COORD(c.z);
						WRITE_SHORT(g_iModelIndexLaser);
						WRITE_BYTE(0);// framestart
						WRITE_BYTE(16);// framerate
						WRITE_BYTE(100);// life
						WRITE_BYTE(5);// width
						WRITE_BYTE(0);// noise
						WRITE_BYTE(255);// r
						WRITE_BYTE(255);// g
						WRITE_BYTE((es == ENTITY_EXSTATE_CONTAINER)?127:255);// b
						WRITE_BYTE(200);// brightness
						WRITE_BYTE(10);// speed
					MESSAGE_END();
				}
			}
		}
		else if (strcmp(act, "box") == 0)
		{
			if (pUser)
			{
				uint32 es = pEntity->GetExistenceState();
				if (es == ENTITY_EXSTATE_WORLD)//???? || es == ENTITY_EXSTATE_CONTAINER)
					UTIL_ShowBox(pEntity->pev->absmin, pEntity->pev->absmax, 10.0f, 255,255,255);
			}
		}
		else if (strcmp(act, "hl") == 0)
		{
			if (argc > option + 1)
			{
				if (atoi(args[option+1]) > 0)
				{
					SetBits(pEntity->pev->effects, EF_BRIGHTFIELD);
					if (FBitSet(pEntity->pev->effects, EF_NODRAW))
					{
						//ClearBits(pEntity->pev->effects, EF_NODRAW);
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - entity is invisible;\n");}
					}
					else
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - highlighted;\n");}
				}
				else
				{
					ClearBits(pEntity->pev->effects, EF_BRIGHTFIELD);
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - unhighlighted;\n");}
				}
				++option;// skip argument
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad argument! <1/0>\n", act);}
		}
		else if (strcmp(act, "hide") == 0)
		{
			if (argc > option + 1)
			{
				if (atoi(args[option+1]) > 0)
				{
					SetBits(pEntity->pev->effects, EF_NODRAW);
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - hid;\n");}
				}
				else
				{
					ClearBits(pEntity->pev->effects, EF_NODRAW);
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - unhid;\n");}
				}
				++option;// skip argument
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad argument! <1/0>\n", act);}
		}
		else if (strcmp(act, "use") == 0)
		{
			if (argc > option + 1)
			{
				float val;
				USE_TYPE utype = (USE_TYPE)atoi(args[option+1]);
				if (argc > option + 2)
				{
					val = atof(args[option+2]);
					option += 2;// skip two arguments
				}
				else
				{
					val = 0.0f;
					++option;// skip single argument
				}
				pEntity->Use(pUser, pUser, utype, val);// USE_TOGGLE?
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - used;\n");}
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad arguments! <use type> [value]\n", act);}
		}
		else if (strcmp(act, "damage") == 0)
		{
			if (argc > option + 1)// damage 100
			{
				float val = atof(args[option+1]);
				uint32 dmgtype;
				if (argc > option + 2)// damage 100 1
				{
					dmgtype = atoi(args[option+2]);
					option += 2;// skip two arguments
				}
				else
				{
					dmgtype = DMG_GENERIC;
					++option;// skip single argument
				}
				if (pEntity->TakeDamage(pUser, pUser, val, dmgtype) > 0)
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - damaged;\n");}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - not damaged;\n");}
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad arguments! <amount> [type bits]\n", act);}
		}
		else if (strcmp(act, "set") == 0)
		{
			if (argc > option + 2)// set mykey myvalue
			{
				KeyValueData kvd;
				kvd.szClassName = STRINGV(pEntity->pev->classname);
				kvd.szKeyName = (char *)args[option+1];
				kvd.szValue = (char *)args[option+2];
				kvd.fHandled = FALSE;
				DispatchKeyValue(pEntity->edict(), &kvd);
				if (kvd.fHandled)
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - set value\n");}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - value rejected\n");}

				option += 2;
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad arguments! <key> <value>\n", act);}
		}
		else if (strcmp(act, "modflags") == 0)
		{
			if (argc > option + 1)// modflags "+a"
			{
				const char *pArgument = args[option+1];
				const char *pValue = pArgument+1;// skip operator + or -
				if (*pValue)// && isalnum((unsigned char)*pValue))
				{
					uint32 argflags = strtoul(pValue, NULL, 10);
					if (argflags != 0)
					{
						char op = pArgument[0];// operator
						if (op == '+')
							SetBits(pEntity->pev->flags, argflags);
						else if (op == '-')
							ClearBits(pEntity->pev->flags, argflags);
						else
							if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad operator '%c'!\n", act, op);}
					}
					else
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: empty <flags>!\n", act);}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad argument! <flags>\n", act);}

				++option;// skip argument
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: no argument! <[+-]flags>\n", act);}
		}
		else if (strcmp(act, "pick") == 0)
		{
			if (bInteractive && pUser && pUser->IsPlayer())
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pUser;
				pPlayer->m_pPickEntity = pEntity;
				MESSAGE_BEGIN(MSG_ONE, gmsgPickedEnt, NULL, pUser->edict());
					WRITE_SHORT(pPlayer->m_pPickEntity->entindex());
					WRITE_COORD(pPlayer->m_pPickEntity->pev->origin.x);
					WRITE_COORD(pPlayer->m_pPickEntity->pev->origin.y);
					WRITE_COORD(pPlayer->m_pPickEntity->pev->origin.z);
				MESSAGE_END();
				if (printtext) {ClientPrint(pevClient, msg_dest, " - picked (last from list);\n");}
			}
			else
				conprintf(printlevel, " - \"%s\" is interactive-only\n", act);
		}
		else if (strcmp(act, "dist") == 0)
		{
			if (bInteractive && pUser && pUser->IsPlayer())
			{
				Vector vecDelta = pEntity->Center(); vecDelta -= pUser->Center();
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - distance: %g;\n", vecDelta.Length());}
			}
			else
				conprintf(printlevel, " - \"%s\" is interactive-only\n", act);
		}
		else if (strcmp(act, "rotate") == 0)
		{
			if (argc > option + 1)// rotate "-90 0 90"
			{
				Vector vecAngOffset;
				if (StringToVec(args[option+1], vecAngOffset))
				{
					pEntity->pev->angles += vecAngOffset;
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - and rotated;\n");}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad argument! <\"p y r\">\n", act);}

				++option;// skip argument
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: no argument! <\"p y r\">\n", act);}
		}
		else if (strcmp(act, "move") == 0)
		{
			if (argc > option + 1)// move "-1 0 1"
			{
				Vector vecOffset;
				if (StringToVec(args[option+1], vecOffset))
				{
					if (pEntity->pev->solid != SOLID_BSP)
					{
						vecOffset += pEntity->pev->origin;
						UTIL_SetOrigin(pEntity, vecOffset);
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - and moved;\n");}
					}
					else if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - cannot move BSP;\n");}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: bad argument! <\"x y z\">\n", act);}

				++option;// skip argument
			}
			else
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s: no argument! <\"x y z\">\n", act);}
		}
		else if (strcmp(act, "touch") == 0)
		{
			if (argc > option + 1)// touch "mytarget"
			{
				const char *pTargetName = args[option+1];
				++option;// skip single argument
				CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, pTargetName, pUser);// XDM3038c: TESTME: not sure if pUser should be here
				if (pTarget)
				{
					DispatchTouch(pEntity->edict(), pTarget->edict());// pEntity->Touch(pTarget);
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - touched %s[%d] \"%s\";\n", STRING(pTarget->pev->classname), pTarget->entindex(), STRING(pTarget->pev->targetname));}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - target \"%s\" not found!\n", pTargetName);}
			}
			else// touch
			{
				if (pUser)
				{
					DispatchTouch(pEntity->edict(), pUser->edict());// pEntity->Touch(pUser);
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - touched user %s[%d];\n", STRING(pUser->pev->classname), pUser->entindex());}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s failed: no targetname and no user!\n", act);}
			}
		}
		else if (strcmp(act, "moveto") == 0)
		{
			if (argc > option + 1)// moveto "mytarget"
			{
				const char *pTargetName = args[option+1];
				++option;// skip single argument
				CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, pTargetName, pUser);// XDM3038c: TESTME: not sure if pUser should be here
				if (pTarget)
				{
					if (pEntity->pev->solid != SOLID_BSP)
					{
						pEntity->pev->angles = pTarget->pev->angles;
						UTIL_SetOrigin(pEntity, pTarget->pev->origin);
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - moved to %s[%d] \"%s\";\n", STRING(pTarget->pev->classname), pTarget->entindex(), STRING(pTarget->pev->targetname));}
					}
					else if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - cannot move BSP;\n");}
				}
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - target \"%s\" not found!\n", pTargetName);}
			}
			else// moveto
			{
				if (bInteractive && pUser && pUser->IsPlayer() && ((CBasePlayer *)pUser)->m_CoordsRemembered)
				{
					if (pEntity->pev->solid != SOLID_BSP)
					{
						CBasePlayer *pPlayer = (CBasePlayer *)pUser;
						pEntity->pev->angles = pPlayer->m_MemAngles;
						UTIL_SetOrigin(pEntity, pPlayer->m_MemOrigin);
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - moved to mem coords (%s);\n", STRING(pUser->pev->netname));}
					}
					else if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - cannot move BSP;\n");}
				}
				else
				{
					if (pUser)
					{
						if (pEntity->pev->solid != SOLID_BSP)
						{
							pEntity->pev->angles = pUser->pev->angles;
							UTIL_SetOrigin(pEntity, pUser->pev->origin);
							if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - moved to user %s[%d];\n", STRING(pUser->pev->classname), pUser->entindex());}
						}
						else if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - cannot move BSP;\n");}
					}
					else
						if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - %s failed: no targetname and no user!\n", act);}
				}
			}
		}
		else if (strcmp(act, "info") == 0)
		{
			if (bInteractive)
			{
				if (IS_DEDICATED_SERVER() == 0)// useless because it prints to server console
					pEntity->ReportState(1);
				else
					if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - \"%s\" is only available on local server\n", act);}
			}
			else
				conprintf(printlevel, " - \"%s\" is interactive-only\n", act);
		}
#if defined(_DEBUG)
		else if (strcmp(act, "debug") == 0)
		{
			if (bInteractive && IS_DEDICATED_SERVER() == 0)
			{
				if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - breaking execution for %s;\n", STRING(pEntity->pev->classname));}
				DBG_FORCEBREAK
			}
			else
				conprintf(printlevel, " - \"%s\" is interactive-only\n", act);
		}
#endif
		else// printlevel does this if (bInteractive || g_pCvarDeveloper && g_pCvarDeveloper->value >= 2)
			if (printtext) {ClientPrintF(pevClient, msg_dest, printlevel, " - unknown action: %s\n", act);}

		//++option;
	}
	return option;
}
