/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "globals.h"
#include "game.h"
#include "gamerules.h"
#include "shake.h"
#include "decals.h"
#include "colors.h"// XDM
#include "sound.h"
#include <assert.h>

// Only valve knows how this shit works
int g_groupmask = 0;
int g_groupop = 0;

// Normal overrides
void UTIL_SetGroupTrace(const int &groupmask, const int &op)
{
	g_groupmask		= groupmask;
	g_groupop		= op;
	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

void UTIL_UnsetGroupTrace(void)
{
	g_groupmask		= 0;
	g_groupop		= 0;
	ENGINE_SETGROUPMASK(0, 0);
}

// Smart version, it'll clean itself up when it pops off stack
/* UNUSED: requires container UTIL_GroupTrace::UTIL_GroupTrace(int groupmask, int op)
{
	m_oldgroupmask	= g_groupmask;
	m_oldgroupop	= g_groupop;
	g_groupmask		= groupmask;
	g_groupop		= op;
	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

UTIL_GroupTrace::~UTIL_GroupTrace(void)
{
	g_groupmask		=	m_oldgroupmask;
	g_groupop		=	m_oldgroupop;
	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}*/


#if defined (_DEBUG)
edict_t *DBG_EntOfVars(const entvars_t *pev)
{
	if (pev == NULL)
		return NULL;

	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;

	conprintf(1, "FindEntityByVars() ERROR: pContainingEntity is NULL, calling into engine!\n");
	DBG_FORCEBREAK
	edict_t *pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t *)pev);
	if (pent == NULL)
		conprintf(0, "FindEntityByVars() ERROR: pent == NULL!\n");

	((entvars_t *)pev)->pContainingEntity = pent;
	return pent;
}
#endif // _DEBUG


/*void UTIL_ParametricRocket(entvars_t *pev, Vector vecOrigin, Vector vecAngles, edict_t *owner)
{	
	pev->startpos = vecOrigin;
	// Trace out line to end pos
	TraceResult tr;
	UTIL_MakeVectors(vecAngles);
	UTIL_TraceLine(pev->startpos, pev->startpos + gpGlobals->v_forward * 8192, ignore_monsters, owner, &tr);
	pev->endpos = tr.vecEndPos;

	// Now compute how long it will take based on current velocity
	Vector vecTravel = pev->endpos - pev->startpos;
	float travelTime = 0.0;
	if (pev->velocity.Length() > 0)
	{
		travelTime = vecTravel.Length() / pev->velocity.Length();
	}
	pev->starttime = gpGlobals->time;
	pev->impacttime = gpGlobals->time + travelTime;
}*/

//-----------------------------------------------------------------------------
// Purpose: Used by monsters to move
//-----------------------------------------------------------------------------
void UTIL_MoveToOrigin(edict_t *pent, const Vector &vecGoal, float flDist, int iMoveType)
{
	float rgfl[3];
	vecGoal.CopyToArray(rgfl);
	MOVE_TO_ORIGIN(pent, rgfl, flDist, iMoveType); 
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: experimentsl override: allocate all strings via the engine
// Input  : *str
// Output : string_t
//-----------------------------------------------------------------------------
string_t MAKE_STRING(const char *str)
{
	if (sv_strings_alloc.value > 0)
		return ALLOC_STRING(str);
	else
		return (uint64)str - (uint64)STRING(0);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: consistency checks
// Input  : *szModel - HL accepts models and sprites... FINE!.. (=_=)
// Output : int - modelindex
//-----------------------------------------------------------------------------
int PRECACHE_MODEL(char *szModel)
{
	if (szModel && *szModel)
	{
		if (*szModel != '*')// internal BSP model
		{
			/* XDM: this method is bad and you should feel bad!
			int length = 0;
			byte *pFile = LOAD_FILE_FOR_ME(szModel, &length);// this method checks mod and base directories
			if (pFile && length)
			{
				FREE_FILE(pFile);
			}
			else*/
			if (!UTIL_FileExists(szModel))
			{
				conprintf(0, "PRECACHE_MODEL() ERROR: \"%s\" not found!\n", szModel);
				if (UTIL_FileExtensionIs(szModel, ".mdl"))
					szModel = g_szDefaultStudioModel;
				else
					szModel = g_szDefaultSprite;
			}
		}
		return (*g_engfuncs.pfnPrecacheModel)(szModel);
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: second part. Without this game will crash
// Warning: pfnSetModel changes pev->model, pev->modelindex and mins/maxs!
// Input  : *pEntity - 
//			*szModel - 
//-----------------------------------------------------------------------------
void SET_MODEL(edict_t *pEntity, const char *szModel)
{
	if (pEntity && szModel && *szModel)
	{
		if (*szModel != '*')// internal BSP model
		{
			/* XDM: this method is bad and you should feel bad!
			int length = 0;
			byte *pFile = LOAD_FILE_FOR_ME((char *)szModel, &length);// this method checks mod and base directories
			if (pFile && length) 
			{
				FREE_FILE(pFile);
			}
			else*/
			if (!UTIL_FileExists(szModel))
			{
				conprintf(0, "SET_MODEL() ERROR: \"%s\" not found!\n", szModel);
				if (UTIL_FileExtensionIs(szModel, ".mdl"))
				{
					szModel = g_szDefaultStudioModel;
					if (!IsMultiplayer() && g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)// make it easier to find
					{
						//pEntity->v.rendermode = kRenderTransAdd;
						pEntity->v.renderamt = 255;
						pEntity->v.renderfx = kRenderFxFullBright;
					}
				}
				else
				{
					szModel = g_szDefaultSprite;
					if (!IsMultiplayer() && g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)// make it easier to find
					{
						pEntity->v.rendermode = kRenderTransAdd;
						pEntity->v.renderamt = 255;
						pEntity->v.renderfx = kRenderFxPulseFastWide;
					}
				}
			}
		}
		(*g_engfuncs.pfnSetModel)(pEntity, szModel);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Transmit float as 4 bytes using int as a container
// Warning: Must match the recieving side!
// Input  : flValue - 
//-----------------------------------------------------------------------------
void WRITE_FLOAT(const float &flValue)
{
	//static int iData;
	//memcpy(&iData, &flValue, sizeof(iData));
	/*static */union
	{
		int i;
		float f;
	} data;
	data.f = flValue;
	(*g_engfuncs.pfnWriteLong)(data.i);
}

//-----------------------------------------------------------------------------
// Purpose: limited version of UTIL_FindEntityInBox, should be replaced
// Input  : **pList - 
//			listMax - 
//			&mins &maxs - 
//			flagMask - 
// Output : size_t
//-----------------------------------------------------------------------------
size_t UTIL_EntitiesInBox(CBaseEntity **pList, size_t listMax, const Vector &mins, const Vector &maxs, int flagMask)
{
	edict_t *pEdict = INDEXENT(1);
	if (!pEdict)
		return 0;

	CBaseEntity *pEntity;
	size_t count = 0;
	for (int i = 1; i < gpGlobals->maxEntities; ++i, ++pEdict)
	{
		if (!UTIL_IsValidEntity(pEdict))
			continue;

		if (flagMask && FBitSet(pEdict->v.flags, flagMask) == 0)	// Does it meet the criteria?
			continue;

		if (BoundsIntersect(pEdict->v.absmin, pEdict->v.absmax, mins, maxs) == false)// XDM3037
			 continue;

		pEntity = CBaseEntity::Instance(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		++count;

		if (count >= listMax)
			return count;
	}
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: Returns next entity inside or intersecting specified volume
// Input  : *pStartEntity - start search from it (NULL to restart)
//			&mins, &maxs - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *UTIL_FindEntityInBox(CBaseEntity *pStartEntity, const Vector &mins, const Vector &maxs)
{
	int startindex;
	if (pStartEntity)
		startindex = pStartEntity->entindex()+1;
	else
		startindex = 0;

	//DBG UTIL_ShowBox(mins, maxs, 12, 0,0,255);
	int i;
	edict_t *pEdict;
	for (i = startindex; i < gpGlobals->maxEntities; ++i, ++pEdict)
	{
		pEdict = INDEXENT(i);
		if (!UTIL_IsValidEntity(pEdict))
			continue;

		if (!BoundsIntersect(pEdict->v.absmin, pEdict->v.absmax, mins, maxs))// XDM3037
		{
			//DBG UTIL_ShowBox(pEdict->v.absmin, pEdict->v.absmax, 10, 255,255,0);
			continue;
		}
		return CBaseEntity::Instance(pEdict);
	}
	return NULL;
}

CBaseEntity *UTIL_FindEntityInSphere(CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius)
{
	if (flRadius <= 0)
	{
		DBG_PRINTF("UTIL_FindEntityInSphere() error: bad radius %g!\n", flRadius);
		return NULL;
	}
	edict_t	*pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_IN_SPHERE(pentEntity, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::Instance(pentEntity);

	return NULL;
}

CBaseEntity *UTIL_FindEntityByString(CBaseEntity *pStartEntity, const char *szKeyword, const char *szValue)
{
	if (szKeyword == NULL || szValue == NULL)
	{
		DBG_PRINTF("UTIL_FindEntityByString() ERROR: missing string name or value!\n");
		return NULL;
	}
	edict_t	*pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_BY_STRING(pentEntity, szKeyword, szValue);

	if (!FNullEnt(pentEntity))// XDM: NOTE: this requires EOFFSET check :(
		return CBaseEntity::Instance(pentEntity);

	return NULL;
}

CBaseEntity *UTIL_FindEntityByClassname(CBaseEntity *pStartEntity, const char *szName)
{
	return UTIL_FindEntityByString(pStartEntity, "classname", szName);
}

CBaseEntity *UTIL_FindEntityByTargetname(CBaseEntity *pStartEntity, const char *szName)
{
	return UTIL_FindEntityByString(pStartEntity, "targetname", szName);
}

// Spirit of Half-Life compatibility
CBaseEntity *UTIL_FindEntityByTargetname(CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pActivator)
{
	if (szName && FStrEq(szName, "*locus"))
	{
		if (pActivator && (pStartEntity == NULL || pActivator->eoffset() > pStartEntity->eoffset()))
			return pActivator;
		else
			return NULL;
	}
	else 
		return UTIL_FindEntityByTargetname(pStartEntity, szName);
}

CBaseEntity *UTIL_FindEntityByTarget(CBaseEntity *pStartEntity, const char *szName)
{
	return UTIL_FindEntityByString(pStartEntity, "target", szName);
}

//-----------------------------------------------------------------------------
// Purpose: Universal search function
// Input  : *pStartEntity - optional
//			*szKeyword - required
//			*szValue - required
//			&vecCenter - optional
//			flRadius - optional
//-----------------------------------------------------------------------------
CBaseEntity	*UTIL_FindEntities(CBaseEntity *pStartEntity, const char *szKeyword, const char *szValue, const Vector &vecCenter, float flRadius)
{
	CBaseEntity *pEntity = pStartEntity;
	CBaseEntity *pFound = NULL;
	while ((pEntity = UTIL_FindEntityByString(pEntity, szKeyword, szValue)) != NULL)
	{
		if (flRadius > 0.0f)// limit by radius
		{
			if ((pEntity->pev->origin - vecCenter).Length() <= flRadius)
			{
				pFound = pEntity;
				break;
			}
		}
		else// first found is ok
		{
			pFound = pEntity;
			break;
		}
	}
	return pFound;
}

void UTIL_MakeAimVectors(const Vector &vecAngles)
{
#if defined (NOSQB)
	UTIL_MakeVectors(vecAngles);// XDM3038c: MAKE_VECTORS(vecAngles);
#else
	Vector rgflVec(vecAngles);
	rgflVec.x = -rgflVec.x;
	UTIL_MakeVectors(rgflVec);
#endif
}

void UTIL_MakeInvVectors(const Vector &vecAngles, globalvars_t *pgv)
{
	AngleVectors(vecAngles, pgv->v_forward, pgv->v_right, pgv->v_up);;// XDM3038c: MAKE_VECTORS was actually using gpGlobals regardless of what pgv is
	float tmp;
	pgv->v_right *= -1.0f;
	SWAP(pgv->v_forward.y, pgv->v_right.x, tmp);
	SWAP(pgv->v_forward.z, pgv->v_up.x, tmp);
	SWAP(pgv->v_right.z, pgv->v_up.y, tmp);
}

void UTIL_EmitAmbientSound(edict_t *entity, const Vector &vecOrigin, const char *samp, float vol, float attenuation, int fFlags, int pitch)
{
	if (!ASSERT(entity != NULL))
		return;

	if (samp && *samp == '!')// replace sentence name with index
	{
		char sentence_id[32];
		if (SENTENCEG_Lookup(samp, sentence_id) >= 0)
			EMIT_AMBIENT_SOUND(entity, vecOrigin, sentence_id, vol, attenuation, fFlags, pitch);
	}
	else
		EMIT_AMBIENT_SOUND(entity, vecOrigin, samp, vol, attenuation, fFlags, pitch);
}

static unsigned short FixedUnsigned16(float value, float scale)
{
	int output = (int)(value * scale);
	if (output < 0)
		output = 0;
	else if (output > USHRT_MAX)// XDM3037a: optimization
		output = USHRT_MAX;

	return (unsigned short)output;
}

static short FixedSigned16(float value, float scale)
{
	int output = (int)(value * scale);
	if (output > SHRT_MAX)
		output = SHRT_MAX;
	else if (output < SHRT_MIN)// XDM3037a: optimization
		output = SHRT_MIN;

	return (short)output;
}

void UTIL_ScreenShakeOne(CBaseEntity *pPlayer, const Vector &center, float amplitude, float frequency, float duration)
{
#if defined (_DEBUG)
	if (!ASSERT(pPlayer != NULL))
		return;
#endif
	if (pPlayer)
	{
		ScreenShake	shake;
		shake.duration = FixedUnsigned16(duration, 1<<12);// 4.12 fixed
		shake.frequency = FixedUnsigned16(frequency, 1<<8);// 8.8 fixed
		shake.amplitude = FixedUnsigned16(amplitude, 1<<12);// 4.12 fixed
		MESSAGE_BEGIN(MSG_ONE, gmsgShake, center, pPlayer->edict());
			WRITE_SHORT(shake.amplitude);// shake amount
			WRITE_SHORT(shake.duration);// shake lasts this long
			WRITE_SHORT(shake.frequency);// shake noise frequency
		MESSAGE_END();
	}
}

// Shake the screen of all clients within radius
// radius == 0, shake all clients
// XDM: fixed
// UNDONE: Fix falloff model (disabled)?
// UNDONE: Affect user controls?
void UTIL_ScreenShake(const Vector &center, float amplitude, float frequency, float duration, float radius)
{
	float localAmplitude;
	ScreenShake	shake;
	shake.duration = FixedUnsigned16(duration, 1<<12);// 4.12 fixed
	shake.frequency = FixedUnsigned16(frequency, 1<<8);// 8.8 fixed
	bool inwater = UTIL_LiquidContents(center);
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer == NULL)
			continue;

		localAmplitude = 0;

		if (radius <= 0)
			localAmplitude = amplitude;
		else
		{
			Vector delta(center); delta -= pPlayer->pev->origin;
			vec_t distance = delta.Length();
			// Had to get rid of this falloff - it didn't work well
			if (distance < radius)
				localAmplitude = amplitude;//radius - distance;
		}
		if (localAmplitude)
		{
			if (UTIL_LiquidContents(pPlayer->pev->origin) != inwater)// XDM3038c: shake is neutralized when going throuch border of environment
				localAmplitude *= 0.2f;
			if (!FBitSet(pPlayer->pev->flags, FL_ONGROUND))// XDM: FIXed: not on ground?
				localAmplitude *= 0.5f;

			shake.amplitude = FixedUnsigned16(localAmplitude, 1<<12);// 4.12 fixed
			MESSAGE_BEGIN(MSG_ONE, gmsgShake, NULL, pPlayer->edict());// use the magic #1 for "one client"
				WRITE_SHORT(shake.amplitude);// shake amount
				WRITE_SHORT(shake.duration);// shake lasts this long
				WRITE_SHORT(shake.frequency);// shake noise frequency
			MESSAGE_END();
			//UTIL_ScreenShakeOne(pPlayer, center, localAmplitude, duration, frequency);// XDM: change method
		}
	}
}

void UTIL_ScreenShakeAll(const Vector &center, float amplitude, float frequency, float duration)
{
	UTIL_ScreenShake(center, amplitude, frequency, duration, 0);
}

void UTIL_ScreenFadeBuild(ScreenFade &fade, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags)
{
	fade.duration = FixedUnsigned16(fadeTime, 1<<12);// 4.12 fixed
	fade.holdTime = FixedUnsigned16(fadeHold, 1<<12);// 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}

void UTIL_ScreenFadeWrite(const ScreenFade &fade, CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgFade, NULL, pEntity->edict());// use the magic #1 for "one client"
		WRITE_SHORT(fade.duration);// fade lasts this long
		WRITE_SHORT(fade.holdTime);// fade lasts this long
		WRITE_SHORT(fade.fadeFlags);// fade type (in / out)
		WRITE_BYTE(fade.r);
		WRITE_BYTE(fade.g);
		WRITE_BYTE(fade.b);
		WRITE_BYTE(fade.a);
	MESSAGE_END();
}

void UTIL_ScreenFade(CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags)
{
	if (!ASSERT(pEntity != NULL))
		return;
	ScreenFade fade;
	UTIL_ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);
	UTIL_ScreenFadeWrite(fade, pEntity);
}

void UTIL_ScreenFadeAll(const Vector &color, float fadeTime, float fadeHold, int alpha, int flags)
{
	ScreenFade	fade;
	UTIL_ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		UTIL_ScreenFadeWrite(fade, pPlayer);
	}
}

void UTIL_HudMessage(CBaseEntity *pEntity, const hudtextparms_t &textparms, const char *pMessage)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, svc_temp_entity, NULL, pEntity->edict());
		WRITE_BYTE(TE_TEXTMESSAGE);
		WRITE_BYTE(textparms.channel & 0xFF);
		WRITE_SHORT(FixedSigned16(textparms.x, 1<<13));
		WRITE_SHORT(FixedSigned16(textparms.y, 1<<13));
		WRITE_BYTE(textparms.effect);
		WRITE_BYTE(textparms.r1);
		WRITE_BYTE(textparms.g1);
		WRITE_BYTE(textparms.b1);
		WRITE_BYTE(textparms.a1);
		WRITE_BYTE(textparms.r2);
		WRITE_BYTE(textparms.g2);
		WRITE_BYTE(textparms.b2);
		WRITE_BYTE(textparms.a2);
		WRITE_SHORT(FixedUnsigned16(textparms.fadeinTime, 1<<8));
		WRITE_SHORT(FixedUnsigned16(textparms.fadeoutTime, 1<<8));
		WRITE_SHORT(FixedUnsigned16(textparms.holdTime, 1<<8));

		if (textparms.effect == 2)
			WRITE_SHORT(FixedUnsigned16(textparms.fxTime, 1<<8));
		
		if (strlen(pMessage) < MAX_MESSAGE_STRING)
		{
			WRITE_STRING(pMessage);
		}
		else
		{
			char tmp[MAX_MESSAGE_STRING];
			strncpy(tmp, pMessage, MAX_MESSAGE_STRING-1);
			tmp[MAX_MESSAGE_STRING-1] = 0;
			WRITE_STRING(tmp);
		}
	MESSAGE_END();
}


const Vector &UTIL_RandomBloodVector(void)
{
	static Vector direction;
	direction.x = RANDOM_FLOAT(-1.0f, 1.0f);
	direction.y = RANDOM_FLOAT(-1.0f, 1.0f);
	direction.z = RANDOM_FLOAT(0.0f, 1.0f);
	return direction;
}

void UTIL_ParticleEffect(const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount)
{
	PARTICLE_EFFECT(vecOrigin, vecDirection, (float)ulColor, (float)ulCount);
}

void UTIL_BloodStream(const Vector &origin, const Vector &direction, const int &color, const int &amount)
{
	if (amount == 0 || !UTIL_ShouldShowBlood(color))
		return;

	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, origin);
		WRITE_BYTE(TE_BLOODSTREAM);
		WRITE_COORD(origin.x);
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_BYTE(color);
		WRITE_BYTE(min(amount, 255));
	MESSAGE_END();
}				

void UTIL_BloodDrips(const Vector &origin, const Vector &direction, const int &color, const int &amount)
{
	if (amount == 0 || !UTIL_ShouldShowBlood(color))
		return;

	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, origin);
		WRITE_BYTE(TE_BLOODSPRITE);
		WRITE_COORD(origin.x);
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_SHORT(g_iModelIndexBloodSpray);// initial sprite model
		WRITE_SHORT(g_iModelIndexBloodDrop);// droplet sprite models
		WRITE_BYTE(color);// color index into host_basepal
		WRITE_BYTE(clamp(amount/80, 1, 255));// XDM3038a WRITE_BYTE(min(max(3, min(255,amount)/10), 16));		// scale
	MESSAGE_END();
}

bool UTIL_ShouldShowBlood(const int &color)
{
	if (color != DONT_BLEED)
	{
		if (IsMultiplayer())// XDM3035c: nobody cares these days. Performance!
			return true;//return (g_pGameRules->FAllowEffects())

		if (color == BLOOD_COLOR_RED)
		{
			if (g_pviolence_hblood == NULL || g_pviolence_hblood->value > 0)
				return true;
		}
		else if (color == BLOOD_COLOR_YELLOW)// ?
		{
			if (g_pviolence_ablood == NULL || g_pviolence_ablood->value > 0)
				return true;
		}
		else
			return true;
	}
	return false;
}

void UTIL_BloodDecalTrace(TraceResult *pTrace, const int &bloodColor)
{
	if (!ASSERT(pTrace != NULL))
		return;
	if (UTIL_ShouldShowBlood(bloodColor))
		UTIL_DecalTrace(pTrace, UTIL_BloodDecalIndex(bloodColor));
}

void UTIL_DecalTrace(TraceResult *pTrace, int decalNumber)
{
	if (decalNumber < 0)
		return;
	if (!ASSERT(pTrace != NULL))
		return;
	if (pTrace->flFraction == 1.0)
		return;

	int index = g_Decals[decalNumber].index;
	if (index < 0)
		return;

	short entityIndex;
	// Only decal BSP models
	if (pTrace->pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(pTrace->pHit);
		if (pEntity && !pEntity->IsBSPModel())
			return;
		entityIndex = ENTINDEX(pTrace->pHit);
	}
	else 
		entityIndex = 0;

	int message = TE_DECAL;
	if (entityIndex != 0)
	{
		if (index > UCHAR_MAX)
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if (index > UCHAR_MAX)
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(message);
		WRITE_COORD(pTrace->vecEndPos.x);
		WRITE_COORD(pTrace->vecEndPos.y);
		WRITE_COORD(pTrace->vecEndPos.z);
		WRITE_BYTE(index);
		if (entityIndex)
			WRITE_SHORT(entityIndex);
	MESSAGE_END();
}

void UTIL_GunshotDecalTrace(TraceResult *pTrace, int decalNumber)
{
	if (decalNumber < 0)
		return;
	if (!ASSERT(pTrace != NULL))
		return;
	if (pTrace->flFraction == 1.0)
		return;

	int index = g_Decals[decalNumber].index;
	if (index < 0)
		return;

	MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, pTrace->vecEndPos);
		WRITE_BYTE(TE_GUNSHOTDECAL);
		WRITE_COORD(pTrace->vecEndPos.x);
		WRITE_COORD(pTrace->vecEndPos.y);
		WRITE_COORD(pTrace->vecEndPos.z);
		WRITE_SHORT((short)ENTINDEX(pTrace->pHit));
		WRITE_BYTE(index);
	MESSAGE_END();
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace(TraceResult *pTrace, int playernum, int decalNumber, bool bIsCustom)
{
	if (!ASSERT(pTrace != NULL))
		return;
	if (pTrace->flFraction == 1.0)
		return;

	int index;
	if (!bIsCustom)
	{
		if (decalNumber < 0)
			return;

		index = g_Decals[decalNumber].index;
		if (index < 0)
			return;
	}
	else
		index = decalNumber;

	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_PLAYERDECAL);
		WRITE_BYTE(playernum);
		WRITE_COORD(pTrace->vecEndPos.x);
		WRITE_COORD(pTrace->vecEndPos.y);
		WRITE_COORD(pTrace->vecEndPos.z);
		WRITE_SHORT((short)ENTINDEX(pTrace->pHit));
		WRITE_BYTE(index);
	MESSAGE_END();
}

void UTIL_Sparks(const Vector &position)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, position);
		WRITE_BYTE(TE_SPARKS);
		WRITE_COORD(position.x);
		WRITE_COORD(position.y);
		WRITE_COORD(position.z);
	MESSAGE_END();
	//not here	UTIL_EmitAmbientSound(g_pWorld?g_pWorld->edict():???, position, gSoundsSparks[RANDOM_LONG(0,NUM_SPARK_SOUNDS-1)], RANDOM_FLOAT(0.05, 0.5), ATTN_STATIC, 0, RANDOM_LONG(95,105));
}

void UTIL_Ricochet(const Vector &position, float scale)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, position);
		WRITE_BYTE(TE_ARMOR_RICOCHET);
		WRITE_COORD(position.x);
		WRITE_COORD(position.y);
		WRITE_COORD(position.z);
		WRITE_BYTE((int)(scale*10.0f));
	MESSAGE_END();
}

// Obsolete
/*void UTIL_Bubbles(const Vector &mins, const Vector &maxs, int count)
{
	Vector mid = (mins + maxs) * 0.5;
	float flHeight = UTIL_WaterLevel(mid,  mid.z, mid.z + 1024);
	flHeight = flHeight - mins.z;
	MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, mid);
		WRITE_BYTE(TE_BUBBLES);
		WRITE_COORD(mins.x);	// mins
		WRITE_COORD(mins.y);
		WRITE_COORD(mins.z);
		WRITE_COORD(maxs.x);	// maxz
		WRITE_COORD(maxs.y);
		WRITE_COORD(maxs.z);
		WRITE_COORD(flHeight);			// height
		WRITE_SHORT(g_iModelIndexBubble);
		WRITE_BYTE(count); // count
		WRITE_COORD(8); // speed
	MESSAGE_END();
}

// Obsolete
void UTIL_BubbleTrail(const Vector &from, const Vector &to, int count)
{
	float flHeight = UTIL_WaterLevel(from,  from.z, from.z + 256);
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = UTIL_WaterLevel(to,  to.z, to.z + 256);
		flHeight = flHeight - to.z;
		if (flHeight < 8)
			return;

		// UNDONE: do a ploink sound
		flHeight = flHeight + to.z - from.z;
	}

	if (count > 255) 
		count = 255;

	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BUBBLETRAIL);
		WRITE_COORD(from.x);	// mins
		WRITE_COORD(from.y);
		WRITE_COORD(from.z);
		WRITE_COORD(to.x);	// maxz
		WRITE_COORD(to.y);
		WRITE_COORD(to.z);
		WRITE_COORD(flHeight);			// height
		WRITE_SHORT(g_iModelIndexBubble);
		WRITE_BYTE(count); // count
		WRITE_COORD(8); // speed
	MESSAGE_END();
}*/

void UTIL_HudMessageAll(const hudtextparms_t &textparms, const char *pMessage)
{
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
			UTIL_HudMessage(pPlayer, textparms, pMessage);
	}
}

// client - NULL == everyone
void ClientPrint(entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4)
{
	if (msg_name == NULL)// allow developer to debug bogus call
	{
		DBG_FORCEBREAK
		return;
	}
//#if defined (FIX_SECURITY)
	// Stupid HL engine stops when a message is too long! Prevent players from crashing the server.
	size_t len = 2;
	len += strlen(msg_name);
	if (param1)
		len += strlen(param1);
	if (param2)
		len += strlen(param2);
	if (param3)
		len += strlen(param3);
	if (param4)
		len += strlen(param4);

	len *= sizeof(char);
	if (len > MAX_USER_MSG_DATA)
	{
		conprintf(0, "ClientPrint(%d) error: Message too long: %u!\n", client?ENTINDEX(ENT(client)):0, len);
		return;
	}
//#endif
	if (client)
		MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, NULL, ENT(client));
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgTextMsg);

		WRITE_BYTE(msg_dest);
		WRITE_STRING(msg_name);
		if (param1)
			WRITE_STRING(param1);
		if (param2)
			WRITE_STRING(param2);
		if (param3)
			WRITE_STRING(param3);
		if (param4)
			WRITE_STRING(param4);

	MESSAGE_END();
}

// client - NULL == everyone
void ClientPrintF(entvars_t *client, int msg_dest, int devlevel, const char *fmt, ...)
{
	//if (g_pCvarDeveloper && g_pCvarDeveloper->value < devlevel)// filter on client, not on server!
	//	return;

	va_list argptr;
	static char string[1024];
	va_start(argptr, fmt);
	vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);
	int bdata = (msg_dest & HUD_PRINTDEST_MASK) | (devlevel << 4);
	ClientPrint(client, bdata, string);
}


// the purpose of many similar functions was for them to have different buffers so they could be used in a single printf-like call
char *UTIL_dtos1(const int d)
{
	static char buf[8];
	_snprintf(buf, 8, "%d", d);
	return buf;
}

char *UTIL_dtos2(const int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}

/*char *UTIL_dtos3(const int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}

char *UTIL_dtos4(const int d)
{
	static char buf[8];
	sprintf(buf, "%d", d);
	return buf;
}*/

//-----------------------------------------------------------------------------
// Purpose: Prints a logged message to console. Preceded by LOG: (timestamp) < message >
// Input  : format - printf-like arguments
//-----------------------------------------------------------------------------
void UTIL_LogPrintf(char *fmt, ...)
{
	static char string[1024];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);
	// Print to server console
	ALERT(at_logged, "%s", string);
	DBG_PRINTF(string);// _DEBUG only
}

//-----------------------------------------------------------------------------
// Purpose: More convinient server-only print function
// Input  : format - printf-like arguments
//-----------------------------------------------------------------------------
void SERVER_PRINTF(char *format, ...)
{
	static char string[1024];
	va_list argptr;
	va_start(argptr, format);
	vsnprintf(string, 1024, format, argptr);
	va_end(argptr);
	SERVER_PRINT(string);
}

// TODO: find out what's in globals
void UTIL_GetGlobalTrace(TraceResult *pTrace)
{
	pTrace->fAllSolid		= gpGlobals->trace_allsolid;
	pTrace->fStartSolid		= gpGlobals->trace_startsolid;
	pTrace->fInOpen			= gpGlobals->trace_inopen;
	pTrace->fInWater		= gpGlobals->trace_inwater;
	pTrace->flFraction		= gpGlobals->trace_fraction;
	pTrace->vecEndPos		= gpGlobals->trace_endpos;
	pTrace->flPlaneDist		= gpGlobals->trace_plane_dist;
	pTrace->vecPlaneNormal	= gpGlobals->trace_plane_normal;
	pTrace->pHit			= gpGlobals->trace_ent;
	pTrace->iHitgroup		= gpGlobals->trace_hitgroup;
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr)
{
	edict_t *pOwner;
	if (pentIgnore)// XDM3037: UNHACK
	{
		pOwner = pentIgnore->v.owner;
		pentIgnore->v.owner = NULL;
	}
	//else
	//	pOwner = NULL;
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRACE_IGNORE_MONSTERS:TRACE_IGNORE_NOTHING) | (ignoreGlass == ignore_glass?TRACE_IGNORE_GLASS:TRACE_IGNORE_NOTHING), pentIgnore, ptr);
	if (pentIgnore)// pOwner)// faster
		pentIgnore->v.owner = pOwner;// Warning C4701 is OK
}

void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRACE_IGNORE_MONSTERS:TRACE_IGNORE_NOTHING), pentIgnore, ptr);
}

void UTIL_TraceHull(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr)
{
	TRACE_HULL(vecStart, vecEnd, (igmon == ignore_monsters ? TRACE_IGNORE_MONSTERS:TRACE_IGNORE_NOTHING), hullNumber, pentIgnore, ptr);
}

void UTIL_TraceModel(const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr)
{
	TRACE_MODEL(vecStart, vecEnd, hullNumber, pentModel, ptr);
}

/*void UTIL_SetSize(entvars_t *pev, const Vector &vecMin, const Vector &vecMax)
{
	SET_SIZE(edict(), vecMin, vecMax);
}*/

void UTIL_SetSize(CBaseEntity *pEntity, const Vector &vecMin, const Vector &vecMax)// XDM
{
	if (pEntity)
		SET_SIZE(pEntity->edict(), vecMin, vecMax);

	// XDM3038c: UNDONE: Must be done in the engine, not here!
	/*static Vector vmin, vmax;
	vmin = vecMin; vmin *= pEntity->pev->scale;
	vmax = vecMax; vmax *= pEntity->pev->scale;
	SET_SIZE(pEntity->edict(), vmin, vmax);*/
}

void UTIL_SetSize(CBaseEntity *pEntity, const float &radius)// XDM
{
	if (pEntity)
		SET_SIZE(pEntity->edict(), Vector(-radius, -radius, -radius), Vector(radius, radius, radius));
}

/* unused const Vector &UTIL_GetAimVector(edict_t *pent, float flSpeed)
{
	static Vector tmp;
	GET_AIM_VECTOR(pent, flSpeed, tmp);
	return tmp;
}*/

//-----------------------------------------------------------------------------
// Purpose: UTIL_IsMasterTriggered
// Input  : *szMaster - master's name (logic reverses if starts with '~')
//			*pActivator - may affect target's decision
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsMasterTriggered(const char *szMaster, const CBaseEntity *pActivator)
{
	//if (FStringNull(iszMaster))
	if (szMaster == NULL || szMaster == '\0')
		return true;

	size_t i, j;
	char szBuf[80];
	CBaseEntity *pMaster;
	bool found = false;
	bool reverse = false;

	//if (pActivator == NULL)// XDM3035a
	//	return TRUE;// TRUE?!!

	//conprintf(0, "IsMasterTriggered(%s, %s \"%s\")\n", STRING(iszMaster), STRING(pActivator->pev->classname), STRING(pActivator->pev->targetname));
	//szMaster = STRING(iszMaster);
	if (szMaster[0] == '~') //inverse master
	{
		reverse = true;
		szMaster++;
	}

	pMaster = UTIL_FindEntityByTargetname(NULL, szMaster);
	if (!pMaster)
	{
		for (i = 0; szMaster[i]; ++i)
		{
			if (szMaster[i] == '(')
			{
				for (j = i+1; szMaster[j]; ++j)
				{
					if (szMaster[j] == ')')
					{
						strncpy(szBuf, szMaster+i+1, (j-i)-1);
						szBuf[(j-i)-1] = 0;
						/*pActivator*/pMaster = UTIL_FindEntityByTargetname(NULL, szBuf);// XDM3035a: TODO: was this wrong?!?! HLFIX
						found = true;
						break;
					}
				}
				if (!found) // no) found
				{
					conprintf(0, "Missing ')' in master \"%s\"!\n", szMaster);
					return false;
				}
				break;
			}
		}
		if (!found) // no (found
		{
			conprintf(0, "Master \"%s\" not found!\n", szMaster);
			return true;
		}

		strncpy(szBuf, szMaster, i);
		szBuf[i] = 0;
		pMaster = UTIL_FindEntityByTargetname(NULL, szBuf);
	}

	if (pMaster)// XDM3037: TODO: check for real capabilities && pMaster->ObjectCaps() & FCAP_MASTER)
	{
		if (reverse)
			return (pMaster->IsTriggered(pActivator) == false);
		else
			return (pMaster->IsTriggered(pActivator) == true);
	}
	else
		conprintf(1, "Master \"%s\" was null or not a master!\n", szMaster);

	// if this isn't a master entity, just say yes.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Properly remove entity
// Warning: !!! THIS MUST BE LAST CALL from entity's code! Just return; and no single line after UTIL_Remove(this)!
// Warning: Don't call from Touch() functions!!
// Warning: REMOVE_ENTITY() should not be called too often! Example: if during RadiusDamage() some entity is erased, pStart/pNext search entity pointer is corrupted which leads to an infinite loop.
// Note   : FL_KILLME must be set here!
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void UTIL_Remove(CBaseEntity *pEntity)
{
	if (pEntity == NULL)
		return;

	if (pEntity->IsRemoving())
	{
		DBG_PRINTF("Error: UTIL_Remove() called on already removed entity!\n");
		return;
	}
	if (pEntity->IsPlayer())
	{
		DBG_PRINTF("Error: UTIL_Remove() called on player!\n");
		DBG_FORCEBREAK
		return;
	}
// TODO?	pEntity->SetThink(&CBaseEntity::SUB_Remove);
//	pEntity->SetNextThink(0);// remove it ASAP
//	conprintf(1, "UTIL_Remove(%s[%d] %s)\n", STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->targetname));
//bad	pEntity->SUB_Remove();// XDM3037: NUM_FOR_EDICT: bad pointer
	// XDM3038a: BUGBUG: why do gluon beams have ShouldRespawn() NULL during changelevel, while UpdateOnRemove() is not NULL!!!?
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
	if (pEntity->ShouldRespawn())// XDM3035
	{
		if (pEntity->StartRespawn() != NULL)
			return;
		//pEntity->SetTouchNull();
		/*pEntity->SetUseNull();
		pEntity->SetBlockedNull();
		// BAD pEntity->m_iExistenceState = ENTITY_EXSTATE_VOID;
		pEntity->SetThink(&CBaseEntity::SUB_Respawn);
		if (pEntity->IsMonster())
			pEntity->SetNextThink(mp_monstersrespawntime.value);// XDM: TODO
		else
		{
			DBG_PRINTF("UTIL_Remove(%s[%d]): respawning a non-monster!\n", STRING(pEntity->pev->classname), pEntity->entindex());
			pEntity->SetNextThink(10.0f);// unknown entity is respawning!
		}*/
	}

	pEntity->UpdateOnRemove();
	// NUM_FOR_EDICT: bad pointer		REMOVE_ENTITY(ENT(pEntity->pev));

#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		SERVER_PRINT("UTIL_Remove() exception!!!\n");
		DBG_FORCEBREAK
		SetBits(pEntity->pev->flags, FL_KILLME);// XDM3037: give entities a chance to update client side
		pEntity->pev->ltime = 0;// XDM3038a
		pEntity->DontThink();// XDM3038a
		//pEntity->SendClientData(NULL, MSG_ALL, SCD_ENTREMOVE);
		pEntity->SetThinkNull();// XDM3037
		pEntity->SetTouchNull();
		pEntity->SetUseNull();
		pEntity->SetBlockedNull();
		pEntity->pev->targetname = MAKE_STRING("???ERROR???");// XDM3038a: AWESOME debugging idea!!
		pEntity->pev->target = iStringNull;// XDM3038a
		// we can't pEntity->m_iExistenceState = ENTITY_EXSTATE_VOID;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Precache resources used by specified entity
// XDM: TODO: FIXME: this is very slow and eats resources and edicts!
// Avoid using this at all costs! Especially at runtime!
// Input  : *szClassname - classname
//			iszCustomModel - try to override default model of this entity
//-----------------------------------------------------------------------------
void UTIL_PrecacheOther(const char *szClassname, const string_t iszCustomModel)
{
	if (szClassname == NULL || *szClassname == '\0')// XDM3037a
		return;

	edict_t	*pent = CREATE_NAMED_ENTITY(szClassname);//MAKE_STRING(szClassname));// should be safe because we remove this entity before returning
	if (FNullEnt(pent))
	{
		conprintf(1, "UTIL_PrecacheOther(%s): CreateNamedEntity() failed.\n", szClassname);
		return;
	}
	pent->v.spawnflags = SF_NOREFERENCE|SF_NORESPAWN;
	pent->v.flags = FL_NOTARGET;
	pent->v.effects = EF_NOINTERP|EF_NODRAW;
	CBaseEntity *pEntity = CBaseEntity::Instance(pent);
	if (pEntity)
	{
		if (!FStringNull(iszCustomModel))// XDM3037a
			pEntity->pev->model = iszCustomModel;

		pEntity->Precache();
		pEntity->pev->flags |= FL_KILLME;
		// SLOW! but can save a few ASSERTs pEntity->UpdateOnRemove();
		REMOVE_ENTITY(pent);
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the 2D dot product of a line from src to check and vecdir
//-----------------------------------------------------------------------------
float UTIL_DotPoints(const Vector &vecSrc, const Vector &vecCheck, const Vector &vecDir)
{
	Vector2D vec2LOS((vecCheck - vecSrc).Make2D());
	vec2LOS.NormalizeSelf();
	return DotProduct(vec2LOS, (vecDir.Make2D()));
}

//-----------------------------------------------------------------------------
// Purpose: The game editor cannot add duplicate key names, it adds keyname#1, keyname#2 instead. Get rid of these numbers.
//-----------------------------------------------------------------------------
void UTIL_StripToken(const char *pKey, char *pDest)
{
	size_t i = 0;
	while (pKey[i] && pKey[i] != '#')
	{
		pDest[i] = pKey[i];
		++i;
	}
	pDest[i] = 0;
}
