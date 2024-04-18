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
//
// Misc utility code
//
#ifndef	UTIL_H
#define	UTIL_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "activity.h"
#include "enginecallback.h"
#include "protocol.h"
#include "util_vector.h"
#include "util_common.h"
//#include "gloabls.h"

extern DLL_GLOBAL globalvars_t *gpGlobals;

class CBaseEntity;
class CBasePlayer;
class CBasePlayerItem;

// XDM3037: gpGlobals->serverflags. Hopefully this won't interfere
#define FSERVER_RESTORE				0x00001000

// Dot products for view cone checking
#define VIEW_FIELD_FULL				(float)-1.0 // +-180 degrees
#define	VIEW_FIELD_WIDE				(float)-0.7 // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks
#define	VIEW_FIELD_NARROW			(float)0.7 // +-45 degrees, more narrow check used to set up ranged attacks
#define	VIEW_FIELD_ULTRA_NARROW		(float)0.9 // +-25 degrees, more narrow check used to set up ranged attacks

// All monsters need this data
#define	DONT_BLEED					-1

// triggers
#define	SF_TRIGGER_ALLOWMONSTERS	(1 << 0)// monsters allowed to fire this trigger
#define	SF_TRIGGER_NOCLIENTS		(1 << 1)// players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES		(1 << 2)// only pushables can fire this trigger
#define SF_TRIGGER_EVERYTHING		(1 << 3)// everything else can fire this trigger (e.g. gibs, rockets)
#define SF_TRIGGER_CLEARVELOCITY	(1 << 4)// XDM: (teleport) clear velocity
#define SF_TRIGGER_KEEPANGLES		(1 << 5)// XDM: (teleport) keep view angles
#define SF_TRIGGER_SOUNDACTIVATE	(1 << 6)// XDM: activated by sound
#define SF_TRIGGER_START_OFF		(1 << 7)// XDM3035c: start disabled
#define SF_TRIGGER_REQALLPLAYERS	(1 << 15)// XDM3038c: require all players to touch it before it activates

#define SF_TRIG_PUSH_ONCE			(1 << 0)

#define SF_LIGHT_START_OFF			(1 << 0)

// when we are within this close to running out of entities,  items
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

#ifndef SWAP
#define SWAP(a,b,temp)	((temp)=(a),(a)=(b),(b)=(temp))
#endif

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset)		(const char *)(gpGlobals->pStringBase + (string_t)offset)
#define STRINGV(offset)		(char *)(gpGlobals->pStringBase + (string_t)offset)// HACK to be accepted by the engine API, the string is still CONSTANT!!!
//#define MAKE_STRING(str)	((uint64)str - (uint64)STRING(0))
/*inline*/string_t MAKE_STRING(const char *str);

//#define FREE_STRING(str)	str = iStringNull// STUB: there's no real memory deallocation in the engine

// This is the glue that hooks .MAP entity class names to our CPP classes
// The _declspec forces them to be exported by name so we can do a lookup with GetProcAddress()
// The function is used to intialize / allocate the object for the entity
#define LINK_ENTITY_TO_CLASS(mapClassName, DLLClassName) \
	extern "C" void DLLEXPORT mapClassName(entvars_t *pev); \
	void mapClassName(entvars_t *pev) { GetClassPtr((DLLClassName *)pev, #mapClassName); }// XDM3038a: IMPORTANT: classname!

//
// Conversion among the three types of "entity", including identity-conversions.
//
#if defined(_DEBUG)
	edict_t *DBG_EntOfVars(const entvars_t *pev);
	inline edict_t *ENT(const entvars_t *pev)	{ return DBG_EntOfVars(pev); }
#else
	inline edict_t *ENT(const entvars_t *pev)	{ return pev?pev->pContainingEntity:NULL; }
#endif


// Testing the three types of "entity" for nullity
#define eoNullEntity 0

////////////////////////////////////////////////////////////////////////////////
//
// WARNING!!!
// This is a special validation method for ENGINE "find/get/whatever" functions!
// They return WORLD (eoffset 0) as invalid resoult!
////////////////////////////////////////////////////////////////////////////////
inline bool FNullEnt(const edict_t *pent) { return (pent == NULL || OFFSET(pent) == 0); }

// Testing strings for nullity
#define iStringNull 0

inline bool FStringNull(string_t iString) { return iString == iStringNull; }
inline bool FStrEq(const char *sz1, const char *sz2) { return (strcmp(sz1, sz2) == 0); }
inline bool FStrEq(string_t isz1, string_t isz2) { return (strcmp(STRING(isz1), STRING(isz2)) == 0); }// XDM3038
inline bool FStrnEq(const char *sz1, const char *sz2, size_t len) { return (strncmp(sz1, sz2, len) == 0); }
inline bool FClassnameIs(edict_t *pent, const char *szClassname) { return FStrEq(STRING(VARS(pent)->classname), szClassname); }
inline bool FClassnameIs(entvars_t *pev, const char *szClassname) { return FStrEq(STRING(pev->classname), szClassname); }

edict_t *CREATE_NAMED_ENTITY(const char *szClassname);// XDM3038c

// Misc. Prototypes
// OLD void UTIL_SetSize(entvars_t *pev, const Vector &vecMin, const Vector &vecMax);
void UTIL_SetSize(CBaseEntity *pEntity, const Vector &vecMin, const Vector &vecMax);// XDM: WTF? Why extern?
void UTIL_SetSize(CBaseEntity *pEntity, const float &radius);// XDM3034

void UTIL_MoveToOrigin(edict_t *pent, const Vector &vecGoal, float flDist, int iMoveType);

int PRECACHE_MODEL(char *szModel);// XDM3037a
void SET_MODEL(edict_t *pEntity, const char *szModel);// XDM3037a
inline void MESSAGE_BEGIN(int msg_dest, int msg_type, const float *pOrigin = NULL, edict_t *ed = NULL)
{
#if defined(_DEBUG)
	if (msg_type > 0 && msg_type < MAX_NET_MESSAGES)// XDM3037
#endif
		(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
#if defined(_DEBUG)
	else
		(*g_engfuncs.pfnAlertMessage)(at_warning, "MESSAGE_BEGIN(%d, %d) message ID too high!\n", msg_dest, msg_type);
#endif
}

inline void WRITE_COORD(const float &flValue)// XDM3037a: COORD is 2 bytes, but value gets messed up if outside of +- 4096
{
	(*g_engfuncs.pfnWriteCoord)(clamp(flValue, -4095, 4095));
}

inline void WRITE_ANGLE(const float &flValue)// XDM3038b: engine version destroys angles badly...
{
	WRITE_BYTE((NormalizeAngle360r(flValue)*256.0f)/360.0f);
}

// XDM3038
void WRITE_FLOAT(const float &flValue);

// TODO: use UTIL_FindEntityByClassname
inline edict_t *FIND_ENTITY_BY_CLASSNAME(edict_t *entStart, const char *pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "classname", pszName);
}
// TODO: use UTIL_FindEntityByTargetname
inline edict_t *FIND_ENTITY_BY_TARGETNAME(edict_t *entStart, const char *pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}
// TODO: use UTIL_FindEntityByTarget
inline edict_t *FIND_ENTITY_BY_TARGET(edict_t *entStart, const char *pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "target", pszName);
}

// Pass in an array of pointers and an array size, it fills the array and returns the number inserted
size_t UTIL_EntitiesInBox(CBaseEntity **pList, size_t listMax, const Vector &mins, const Vector &maxs, int flagMask);
//int UTIL_MonstersInSphere(CBaseEntity **pList, int listMax, const Vector &center, float radius);
CBaseEntity *UTIL_FindEntityInBox(CBaseEntity *pStartEntity, const Vector &mins, const Vector &maxs);// XDM3037
CBaseEntity	*UTIL_FindEntityInSphere(CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius);
CBaseEntity	*UTIL_FindEntityByString(CBaseEntity *pStartEntity, const char *szKeyword, const char *szValue);
CBaseEntity	*UTIL_FindEntityByClassname(CBaseEntity *pStartEntity, const char *szName);
CBaseEntity	*UTIL_FindEntityByTargetname(CBaseEntity *pStartEntity, const char *szName);
CBaseEntity *UTIL_FindEntityByTargetname(CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pActivator);
CBaseEntity *UTIL_FindEntityByTarget(CBaseEntity *pStartEntity, const char *szName);
CBaseEntity	*UTIL_FindEntities(CBaseEntity *pStartEntity, const char *szKeyword, const char *szValue, const Vector &vecCenter, float flRadius);

// TODO: use AngleVectors instead of these crappy hacks!
inline void UTIL_MakeVectors(const Vector &vecAngles)
{
	AngleVectors(vecAngles, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up);// XDM3038c: MAKE_VECTORS(vecAngles);
}
void UTIL_MakeAimVectors(const Vector &vecAngles);// SQB HACK
void UTIL_MakeInvVectors(const Vector &vecAngles, globalvars_t *pgv);

void UTIL_EmitAmbientSound(edict_t *entity, const Vector &vecOrigin, const char *samp, float vol, float attenuation, int fFlags, int pitch);

void UTIL_ScreenShakeOne(CBaseEntity *pPlayer, const Vector &center, float amplitude, float frequency, float duration);// XDM
void UTIL_ScreenShake(const Vector &center, float amplitude, float frequency, float duration, float radius);
void UTIL_ScreenShakeAll(const Vector &center, float amplitude, float frequency, float duration);
void UTIL_ScreenFade(CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags);
void UTIL_ScreenFadeAll(const Vector &color, float fadeTime, float holdTime, int alpha, int flags);

// Too many parameters to pass into function separately
typedef struct hudtextparms_s
{
	float		x;
	float		y;
	int			effect;
	byte		r1, g1, b1, a1;
	byte		r2, g2, b2, a2;
	float		fadeinTime;
	float		fadeoutTime;
	float		holdTime;
	float		fxTime;
	int			channel;
} hudtextparms_t;

// prints as transparent 'title' to the HUD
void UTIL_HudMessage(CBaseEntity *pEntity, const hudtextparms_t &textparms, const char *pMessage);
void UTIL_HudMessageAll(const hudtextparms_t &textparms, const char *pMessage);

// prints messages through the HUD (sends to everyone if client is NULL)
void ClientPrint(entvars_t *client, int msg_dest, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL);
void ClientPrintF(entvars_t *client, int msg_dest, int devlevel, const char *fmt, ...);

// for handy use with ClientPrint params
char *UTIL_dtos1(const int d);
char *UTIL_dtos2(const int d);

// Writes message to console with timestamp and FragLog header.
void UTIL_LogPrintf(char *fmt, ...);
void SERVER_PRINTF(char *format, ...);


// UTIL_TraceLine stuff (really useless)
typedef enum
{
	dont_ignore_monsters = 0,
	ignore_monsters,
	missile
} IGNORE_MONSTERS;

typedef enum
{
	dont_ignore_glass = 0,
	ignore_glass
} IGNORE_GLASS;

//TraceResult UTIL_GetGlobalTrace(void);
void UTIL_GetGlobalTrace(TraceResult *pTrace);
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr);
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr);
void UTIL_TraceHull(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr);
void UTIL_TraceModel(const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr);

const Vector &UTIL_RandomBloodVector(void);
void UTIL_ParticleEffect(const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount);
void UTIL_BloodStream(const Vector &origin, const Vector &direction, const int &color, const int &amount);
void UTIL_BloodDrips(const Vector &origin, const Vector &direction, const int &color, const int &amount);
bool UTIL_ShouldShowBlood(const int &color);
int UTIL_BloodDecalIndex(const int &bloodColor);
void UTIL_BloodDecalTrace(TraceResult *pTrace, const int &bloodColor);
void UTIL_DecalTrace(TraceResult *pTrace, int decalNumber);
void UTIL_PlayerDecalTrace(TraceResult *pTrace, int playernum, int decalNumber, bool bIsCustom);
void UTIL_GunshotDecalTrace(TraceResult *pTrace, int decalNumber);
void UTIL_Sparks(const Vector &position);
void UTIL_Ricochet(const Vector &position, float scale);

//const Vector &UTIL_GetAimVector(edict_t *pent, float flSpeed);

bool UTIL_IsMasterTriggered(const char *szMaster, const CBaseEntity *pActivator);

void UTIL_Remove(CBaseEntity *pEntity);

// Search for water transition along a vertical line
float UTIL_WaterLevel(const Vector &position, float minz, float maxz);
//void UTIL_Bubbles(const Vector &mins, const Vector &maxs, int count);
//void UTIL_BubbleTrail(const Vector &from, const Vector &to, int count);
void FX_BubblesPoint(const Vector &center, const Vector &spread, int count);
void FX_BubblesSphere(const Vector &center, float radius, int count);
void FX_BubblesBox(const Vector &center, const Vector &halfbox, int count);
void FX_BubblesLine(const Vector &start, const Vector &end, int count);

// allows precacheing of other entities
void UTIL_PrecacheOther(const char *szClassname, const string_t iszCustomModel = iStringNull);
void UTIL_PrecacheWeapon(const char *szClassname);
void UTIL_PrecacheAmmo(const char *szClassname);
void UTIL_PrecacheMaterial(struct material_s *pMaterial);

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected otherwise returns NULL. Index is 1 based
edict_t	*UTIL_ClientEdictByIndex(CLIENT_INDEX playerIndex);
CBasePlayer	*UTIL_ClientByIndex(CLIENT_INDEX playerIndex);
CBasePlayer	*UTIL_ClientByName(const char *pName);
CBaseEntity	*UTIL_EntityByIndex(int index);
bool UTIL_IsValidEntity(const edict_t *pEdict);
bool UTIL_IsValidEntity(CBaseEntity *pEntity);
// OBSOLETE int UTIL_SetTargetValue(const char *strTarget, bool bOn);

// Sorta like FInViewCone, but for nonmonsters.
float UTIL_DotPoints(const Vector &vecSrc, const Vector &vecCheck, const Vector &vecDir);
void UTIL_StripToken(const char *pKey, char *pDest);// for redundant keynames

// Misc functions
void SetMovedir(entvars_t *pev);
Vector VecBModelOrigin(entvars_t *pevBModel);

void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch);

inline void EMIT_SOUND(edict_t *entity, int channel, const char *sample, float volume, float attenuation)
{
	EMIT_SOUND_DYN(entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND(edict_t *entity, int channel, const char *sample)
{
	EMIT_SOUND_DYN(entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

void EMIT_SOUND_SUIT(edict_t *entity, const char *sample);
void EMIT_GROUPID_SUIT(edict_t *entity, int isentenceg);
void EMIT_GROUPNAME_SUIT(edict_t *entity, const char *groupname);

#define PRECACHE_SOUND_ARRAY(a) { for (size_t i = 0; i < ARRAYSIZE(a); ++i) PRECACHE_SOUND((char *)a[i]); }
#define RANDOM_SOUND_ARRAY(sarray) (sarray)[RANDOM_LONG(0,ARRAYSIZE((sarray))-1)]
#define EMIT_SOUND_ARRAY_DYN(chan, sarray) EMIT_SOUND_DYN(edict(), chan, sarray[RANDOM_LONG(0,ARRAYSIZE(sarray)-1)], GetSoundVolume(), ATTN_NORM, 0, GetSoundPitch());// XDM3038b: to be used by monsters
#define EMIT_SOUND_ARRAY_DYN2(chan, sarray, attn, pitch) EMIT_SOUND_DYN(edict(), chan, sarray[RANDOM_LONG(0,ARRAYSIZE(sarray)-1)], GetSoundVolume(), attn, 0, pitch);

#define PLAYBACK_EVENT(flags, who, index) PLAYBACK_EVENT_FULL(flags, who, index, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);
//#define PLAYBACK_EVENT_DELAY(flags, who, index, delay) PLAYBACK_EVENT_FULL(flags, who, index, delay, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);


extern int g_groupmask;
extern int g_groupop;

/* UNUSED class UTIL_GroupTrace
{
public:
	UTIL_GroupTrace(int groupmask, int op);
	~UTIL_GroupTrace(void);

private:
	int m_oldgroupmask, m_oldgroupop;
};*/

void UTIL_SetGroupTrace(const int &groupmask, const int &op);
void UTIL_UnsetGroupTrace(void);

void UTIL_FAIL(void);// XDM3038c

bool IsPointEntity(CBaseEntity *pEnt);

CBaseEntity *UTIL_FindEntityForward(CBaseEntity *pMe);
CBaseEntity *UTIL_FindGlobalEntity(string_t classname, string_t globalname);

void UTIL_ShowMessageRadius(const char *pString, const Vector &center, int radius);

void PlayAudioTrack(CBasePlayer *pPlayer, const char *pTrackName, int iCommand, const float &fTimeOffset);// XDM3038c

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

void ParticlesCustom(const Vector &vecPos, float rnd_vel, float life, byte color_pal, byte number);
void GlowSprite(const Vector &vecPos, int mdl_idx, int life, int scale, int fade);
void SpriteTrail(const Vector &vecPos, const Vector vecEnd, int mdl_idx, int count, int life, int scale, int vel, int rnd_vel);
void DynamicLight(const Vector &vecPos, int radius, int r, int g, int b, int life, int decay);
void EntityLight(int entidx, const Vector &vecPos, int radius, int r, int g, int b, int life, int decay);
void PartSystem(const Vector &vecPos, const Vector &vecDir, const Vector &vecSpreadSize, int sprindex, int rendermode, int type, int max_parts, int life, int flags, int ent);
void BeamEffect(int type, const Vector &vecPos, const Vector &vecAxis, int mdl_idx, int startframe, int fps, int life, int width, int noise, const Vector &color, int brightness, int speed);

void UTIL_ShowLine(const Vector &start, const Vector &end, float life, byte r, byte g, byte b);
void UTIL_ShowBox(const Vector &absmins, const Vector &absmaxs, float life, byte r, byte g, byte b);
void UTIL_ShowBox(const Vector &origin, const Vector &mins, const Vector &maxs, float life, byte r, byte g, byte b);

void UTIL_DecalPoints(const Vector &src, const Vector &end, edict_t *pent, int decalIndex);
void BlackHoleImplosion(const Vector &vecSpot, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, float flRadius, bool bKeepWeapons);// XDM3038

void EjectBrass(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int body, int soundtype);

int GetEntBodyGroupsCount(edict_t *ent);
int GetEntTextureGroupsCount(edict_t *ent);
int GetEntBodyCount(edict_t *ent, int bodygroup);

bool UTIL_StringToRandomVector(float *pVector, const char *str);// SHL compatibility

void UTIL_FixRenderColor(const int &rendermode, float *rendercolor);// XDM3035a: fix default zeroes

void StreakSplash(const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange);
void ParticleBurst(const Vector &origin, int radius, int color, int duration);

char *memfgets(byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize);

void ExtractCommandString(char *s, char *szCommand);

float UTIL_GetWeaponWorldScale(void);// XDM3035b

void UTIL_SetView(edict_t *pClient, edict_t *pViewEnt);// XDM3035c

bool IsMultiplayer(void);// XDM3038a
bool IsGameOver(void);// XDM3038a

bool IsSafeToRemove(edict_t *pent);// XDM3038a

// FireTargets callbacks
bool FSetTeamColor(CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller);// XDM3037

size_t Cmd_EntityAction(size_t start_argument, size_t argc, const char *args[], CBaseEntity *pEntity, CBaseEntity *pUser, bool bInteractive);// XDM3038c

#endif // UTIL_H
