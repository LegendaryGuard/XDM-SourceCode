#ifndef	XBM_UTIL_H// don't interfere with server DLL headers
#define	XBM_UTIL_H
#ifdef _WIN32
#ifndef __MINGW32__
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "activity.h"
#include "enginecallback.h"
#include "protocol.h"
#include "util_vector.h"
#include "util_common.h"
#include "gamedefs.h"

extern DLL_GLOBAL globalvars_t *gpGlobals;

// Dot products for view cone checking
#define VIEW_FIELD_FULL			(float)-1.0 // +-180 degrees
#define	VIEW_FIELD_WIDE			(float)-0.7 // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks 
#define	VIEW_FIELD_NARROW		(float)0.7 // +-45 degrees, more narrow check used to set up ranged attacks
#define	VIEW_FIELD_ULTRA_NARROW	(float)0.9 // +-25 degrees, more narrow check used to set up ranged attacks

// All monsters need this data
#define DONT_BLEED			-1

#if !defined (PROTOCOL_H)
// hardcode
#define SVC_TEMPENTITY		23
#define SVC_INTERMISSION	30
#define SVC_CDTRACK			32
#define SVC_WEAPONANIM		35
#define SVC_ROOMTYPE		37
#define SVC_HLTV			50
#define SVC_DIRECTOR		51
#endif

// triggers
#define	SF_TRIGGER_ALLOWMONSTERS	1// monsters allowed to fire this trigger
#define	SF_TRIGGER_NOCLIENTS		2// players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES		4// only pushables can fire this trigger
#define SF_TRIGGER_EVERYTHING		8// everything else can fire this trigger (e.g. gibs, rockets)
#define SF_TRIGGER_CLEARVELOCITY	16// XDM: clear velocity
#define SF_TRIGGER_KEEPANGLES		32// XDM: keep view angles
#define SF_TRIGGER_SOUNDACTIVATE	64// XDM: activated by sound

#define SF_LIGHT_START_OFF		1

#define SF_TRIG_PUSH_ONCE		1

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset)		(const char *)(gpGlobals->pStringBase + (int)offset)
#define STRINGV(offset)		(char *)(gpGlobals->pStringBase + (int)offset)
#define MAKE_STRING(str)	((int)str - (int)STRING(0))

// Testing strings for nullity
#define iStringNull 0

inline bool FStringNull(int iString) { return iString == iStringNull; }
inline bool FStrEq(const char *sz1, const char *sz2) { return (strcmp(sz1, sz2) == 0); }
inline bool FStrnEq(const char *sz1, const char *sz2, size_t len) { return (strncmp(sz1, sz2, len) == 0); }

// Conversion among the three types of "entity", including identity-conversions.
#ifdef DEBUG
	extern edict_t *DBG_EntOfVars(const entvars_t *pev);
	inline edict_t *ENT(const entvars_t *pev)	{ return DBG_EntOfVars(pev); }
#else
	inline edict_t *ENT(const entvars_t *pev)	{ return pev->pContainingEntity; }
#endif
inline edict_t *ENT(edict_t *pent)				{ return pent; }
inline edict_t *ENT(EOFFSET eoffset)			{ return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset); }
inline EOFFSET OFFSET(EOFFSET eoffset)			{ return eoffset; }
inline EOFFSET OFFSET(const edict_t *pent)	
{ 
#if _DEBUG
	if (!pent)
		ALERT(at_error, "Bad ent in OFFSET()\n");
#endif
	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent); 
}
inline EOFFSET OFFSET(entvars_t *pev)				
{ 
#if _DEBUG
	if (!pev)
		ALERT( at_error, "Bad pev in OFFSET()\n" );
#endif
	return OFFSET(ENT(pev)); 
}
inline entvars_t *VARS(entvars_t *pev)					{ return pev; }
inline entvars_t *VARS(edict_t *pent)			
{
	if (!pent)
		return NULL;

	return &pent->v; 
}

inline entvars_t* VARS(EOFFSET eoffset)			{ return VARS(ENT(eoffset)); }
#if defined (_DEBUG)
inline int ENTINDEX(const edict_t *pEdict)
{
	//ASSERT(pEdict != NULL);
	//ASSERT(pEdict->pvPrivateData != NULL);
	//ASSERT(pEdict->free == 0);
	if (pEdict == NULL || pEdict->pvPrivateData == NULL || pEdict->free != 0)
	{
		(*g_engfuncs.pfnAlertMessage)(at_console, "XBM: ENTINDEX() called on bad pointer!\n");
		return -1;// ?!
	}
	return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}
#else
inline int ENTINDEX(const edict_t *pEdict)		{ return (*g_engfuncs.pfnIndexOfEdict)(pEdict); }
#endif
inline edict_t* INDEXENT(int iEdictNum)			{ return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum); }
//inline void MESSAGE_BEGIN(int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent) { (*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ENT(ent)); }

// Testing the three types of "entity" for nullity
#define eoNullEntity 0

inline bool FNullEnt(EOFFSET eoffset)			{ return eoffset == 0; }
inline bool FNullEnt(const edict_t *pent)		{ return pent == NULL || FNullEnt(OFFSET(pent)); }
inline bool FNullEnt(entvars_t *pev)			{ return pev == NULL || FNullEnt(OFFSET(pev)); }

inline bool FClassnameIs(edict_t *pent, const char *szClassname) { return FStrEq(STRING(VARS(pent)->classname), szClassname); }
inline bool FClassnameIs(entvars_t *pev, const char *szClassname) { return FStrEq(STRING(pev->classname), szClassname); }

inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent );  // implementation later in this file

inline edict_t *FIND_ENTITY_BY_CLASSNAME(edict_t *entStart, const char *pszName) 
{
	return FIND_ENTITY_BY_STRING(entStart, "classname", pszName);
}

inline edict_t *FIND_ENTITY_BY_TARGETNAME(edict_t *entStart, const char *pszName) 
{
	return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}
// for doing a reverse lookup. Say you have a door, and want to find its button.
inline edict_t *FIND_ENTITY_BY_TARGET(edict_t *entStart, const char *pszName) 
{
	return FIND_ENTITY_BY_STRING(entStart, "target", pszName);
}

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
//extern CBaseEntity	*UTIL_PlayerByIndex( int playerIndex );

#define UTIL_EntitiesInPVS(pent)			(*g_engfuncs.pfnEntitiesInPVS)(pent)

inline void UTIL_MakeVectors(const Vector &vecAngles)
{
	AngleVectors(vecAngles, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up);// XDM3038c: MAKE_VECTORS(vecAngles);
}

//extern void			UTIL_SetOrigin			( entvars_t *pev, const Vector &vecOrigin );
extern void			UTIL_EmitAmbientSound	( edict_t *entity, const Vector &vecOrigin, const char *samp, float vol, float attenuation, int fFlags, int pitch );
extern void			UTIL_ParticleEffect		( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount );
extern void			UTIL_ScreenShake		( const Vector &center, float amplitude, float frequency, float duration, float radius );
extern void			UTIL_ScreenShakeAll		( const Vector &center, float amplitude, float frequency, float duration );
//extern void			UTIL_ShowMessage		( const char *pString, CBaseEntity *pPlayer );
extern void			UTIL_ShowMessageAll		( const char *pString );
extern void			UTIL_ScreenFadeAll		( const Vector &color, float fadeTime, float holdTime, int alpha, int flags );

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

TraceResult UTIL_GetGlobalTrace(void);
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr);
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr);
void UTIL_TraceHull(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr);
void UTIL_TraceModel(const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr);

extern void UTIL_BloodStream(const Vector &origin, const Vector &direction, const int &color, const int &amount);
extern void UTIL_BloodDrips(const Vector &origin, const Vector &direction, const int &color, const int &amount);
extern Vector UTIL_RandomBloodVector( void );
extern bool UTIL_ShouldShowBlood(const int &color);
extern void UTIL_BloodDecalTrace(TraceResult *pTrace, const int &bloodColor);
extern void UTIL_DecalTrace( TraceResult *pTrace, int decalNumber );
extern void UTIL_PlayerDecalTrace( TraceResult *pTrace, int playernum, int decalNumber, bool bIsCustom );
extern void UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber );
extern void UTIL_Sparks( const Vector &position );
extern void UTIL_Ricochet( const Vector &position, float scale );

Vector UTIL_ClampVectorToBox(const Vector &input, const Vector &clampSize);

bool IsValidTeam(const TEAM_ID &team_id);
bool IsActiveTeam(const TEAM_ID &team_id);

bool UTIL_IsValidEntity(const edict_t *pent);
bool UTIL_IsSpectator(const edict_t *pent);

// Search for water transition along a vertical line
float UTIL_WaterLevel(const Vector &position, float minz, float maxz);
//void UTIL_Bubbles(Vector mins, Vector maxs, int count);
//void UTIL_BubbleTrail(Vector from, Vector to, int count);

// prints messages through the HUD
void ClientPrint(entvars_t *client, int msg_dest, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL);

// prints a message to the HUD say (chat)
//extern void UTIL_SayText(const char *pText, CBaseEntity *pEntity = NULL, BOOL reliable = TRUE);
//extern void UTIL_SayTextAll(const char *pText, CBaseEntity *pEntity = NULL, BOOL reliable = TRUE);

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

// Writes message to console with timestamp and FragLog header.
extern void UTIL_LogPrintf(char *fmt, ...);

extern Vector VecBModelOrigin(entvars_t *pevBModel);

// NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100
// is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
// down to 1 is a lower pitch. 150 to 70 is the realistic range.
// EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as
// fast as EMIT_SOUND (the pitchshift mixer is not native coded).

void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch);

inline void EMIT_SOUND(edict_t *entity, int channel, const char *sample, float volume, float attenuation)
{
	EMIT_SOUND_DYN(entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND(edict_t *entity, int channel, const char *sample)
{
	EMIT_SOUND_DYN(entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

#define PLAYBACK_EVENT(flags, who, index) PLAYBACK_EVENT_FULL(flags, who, index, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);
#define PLAYBACK_EVENT_DELAY(flags, who, index, delay) PLAYBACK_EVENT_FULL(flags, who, index, delay, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

int UTIL_SharedRandomLong(unsigned int seed, int low, int high);
float UTIL_SharedRandomFloat(unsigned int seed, float low, float high);
float UTIL_WeaponTimeBase(void);

void BeamEffect(int type, const Vector &vecPos, const Vector &vecAxis, int mdl_idx, int startframe, int fps, int life, int width, int noise, const Vector &color, int brightness, int speed);
void GlowSprite(const Vector &vecPos, int mdl_idx, int life, int scale, int fade);

bool UTIL_LiquidContents(const Vector &vec);
void UTIL_ShowLine(const Vector &start, const Vector &end, float life, byte r, byte g, byte b);
void UTIL_ShowBox(const Vector &origin, const Vector &mins, const Vector &maxs, float life, byte r, byte g, byte b);

bool IsTeamGame(const int &gamerules);
bool IsTeamplay(void);
bool GameRulesHaveGoal(void);


// new UTIL.CPP functions...

edict_t *UTIL_FindEntityInSphere(edict_t *pentStart, const Vector &vecCenter, float flRadius);
edict_t *UTIL_FindEntityByString(edict_t *pentStart, const char *szKeyword, const char *szValue);
edict_t *UTIL_FindEntityByClassname(edict_t *pentStart, const char *szName);
edict_t *UTIL_FindEntityByTargetname(edict_t *pentStart, const char *szName);

//void ClientPrint(edict_t *pEdict, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);
//void UTIL_SayText(const char *pText, edict_t *pEdict);
void BOT_Host_Say(edict_t *pEntity, const char *pText, bool teamonly);
//int UTIL_GetTeam(edict_t *pEntity);
//int UTIL_GetClass(edict_t *pEntity);
bool IsOnLadder(const edict_t *pEdict);
bool IsActiveBot(int bot_index);
bool IsFriend(const struct bot_s *pBot, const edict_t *pEdict);

short UTIL_CountBots(void);
bool IsAlive(edict_t *pEdict);
bool FInViewCone(const Vector &origin, edict_t *pEdict);
bool FVisible(const Vector &vecOrigin, edict_t *pEdict);
Vector Center(edict_t *pEdict);
Vector GetGunPosition(edict_t *pEdict);
void UTIL_SelectItem(edict_t *pEdict, char *item_name);
void UTIL_SelectItem(edict_t *pEdict, const int &iID);
// XDM3038 void UTIL_SelectWeapon(edict_t *pEdict, const int &weapon_index);
bool UpdateSounds(edict_t *pEdict, edict_t *pPlayer);
void UTIL_ShowMenu(edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText);
void UTIL_BuildFileName(char *filename, const char *arg1, const char *arg2);
//void UTIL_Pathname_Convert(char *pathname);
void GetGameDir(char *game_dir);
edict_t *UTIL_ClientByIndex(const CLIENT_INDEX playerIndex);// XDM3035
void GetEntityPrintableName(edict_t *pEntity, char *output, size_t max_len);

#endif	//XBM_UTIL_H
