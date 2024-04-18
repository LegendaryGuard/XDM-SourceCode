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
#ifndef CBASE_H
#define CBASE_H
#if defined(_WIN32)
#if !defined(__MINGW32__)
#pragma once
#endif /* !__MINGW32__ */
#endif /* _WIN32 */

/*
Class Hierachy

CBaseEntity
	CBaseDelay
		CBaseAnimating
			CBaseToggle
				CBaseMonster
					CBasePlayer
*/

#include <stddef.h>// XDM3037a: offsetof

#ifndef SAVERESTORE_H
#include "saverestore.h"
#endif

#ifndef SCHEDULE_H
#include "schedule.h"
#endif

#ifndef MONSTEREVENT_H
#include "monsterevent.h"
#endif

#include "damage.h"
#include "studio.h"
#include "gamedefs.h"// XDM3038a: MAX_TEAMS

#if defined (_DEBUG_SERVERDLL)
#define DBG_SV_PRINT	DBG_PrintF
#else
#define DBG_SV_PRINT
#endif

#define MAX_PATH_SIZE	10 // max number of nodes available for a path.

// C functions for external declarations that call the appropriate C++ methods
extern int DispatchSpawn(edict_t *pent);
extern void DispatchKeyValue(edict_t *pentKeyvalue, KeyValueData *pkvd);
extern void DispatchTouch(edict_t *pentTouched, edict_t *pentOther);
extern void DispatchUse(edict_t *pentUsed, edict_t *pentOther);
extern void DispatchThink(edict_t *pent);
extern void DispatchBlocked(edict_t *pentBlocked, edict_t *pentOther);
extern void DispatchSave(edict_t *pent, SAVERESTOREDATA *pSaveData);
extern int  DispatchRestore(edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity);
extern void	DispatchAbsBox(edict_t *pent);
extern void SaveWriteFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount);
extern void SaveReadFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount);
extern void SaveGlobalState(SAVERESTOREDATA *pSaveData);
extern void RestoreGlobalState(SAVERESTOREDATA *pSaveData);
extern void ResetGlobalState(void);

// Use() types
typedef enum use_types_e
{
	USE_OFF = 0,
	USE_ON = 1,
	USE_SET = 2,
	USE_TOGGLE = 3,
	USE_KILL = 4,
// special signals, never actually get sent:
	USE_SAME = 5,
	USE_NOT = 6,
} USE_TYPE;

// the values used for the new "global states" mechanism.
typedef enum states_e
{
	STATE_OFF = 0,	// disabled, inactive, invisible, closed, or stateless. Or non-alert monster.
	STATE_TURN_ON,  // door opening, env_fade fading in, etc.
	STATE_ON,		// enabled, active, visisble, or open. Or alert monster.
	STATE_TURN_OFF, // door closing, monster dying (?).
	STATE_IN_USE,	// player is in control (train/tank/barney/scientist).
					// In_Use isn't very useful, I'll probably remove it.
} STATE;

// Things that toggle (buttons/triggers/doors) need this
typedef enum toggle_states_e
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
} TOGGLE_STATE;

// XDM3037: SendClientData() sendcase
enum sendclientdata_cases_e
{
	SCD_GLOBALUPDATE = 0,	// some external global update request
	SCD_SELFUPDATE,			// entity changed state and wants to tell everyone about it
	SCD_CLIENTUPDATEREQUEST,// a client is requesting current entity state
	SCD_CLIENTRESTORE,		// a client is requesting current entity state after loading a game
	SCD_CLIENTCONNECT,		// a client is requesting current entity state upon connecting
	SCD_ENTREMOVE			// entity is about to be removed, it may send OFF signal to clients
};

// For Classify() IRelationship() g_iRelationshipTable
enum entity_classes_e
{
	CLASS_NONE = 0,
	CLASS_MACHINE,
	CLASS_PLAYER,
	CLASS_HUMAN_PASSIVE,
	CLASS_HUMAN_MILITARY,
	CLASS_ALIEN_MILITARY,
	CLASS_ALIEN_PASSIVE,
	CLASS_ALIEN_MONSTER,
	CLASS_ALIEN_PREY,
	CLASS_ALIEN_PREDATOR,
	CLASS_INSECT,
	CLASS_PLAYER_ALLY,
	CLASS_PLAYER_BIOWEAPON, // hornets and snarks.launched by players
	CLASS_ALIEN_BIOWEAPON, // hornets and snarks.launched by the alien menace
	CLASS_GRENADE, // XDM: dangerous
	CLASS_GIB, // XDM: om-nom-nom
	CLASS_BARNACLE, // special because no one pays attention to it, and it eats a wide cross-section of creatures.
	CLASS_HUMAN_MONSTER,// XDM3038a: >:)
	NUM_RELATIONSHIP_ClASSES// leave this last in the enum
};

// Entity to entity relationship types
enum entity_relationship_e
{
	R_AL = -2,// (ALLY) pals. Good alternative to R_NO when applicable.
	R_FR = -1,// (FEAR) will run away
	R_NO = 0,// (NO RELATIONSHIP) disregard
	R_DL = 1,// (DISLIKE) will attack
	R_HT = 2,// (HATE) will attack this character instead of any visible DISLIKEd characters
	R_NM = 3// (NEMESIS) a monster Will ALWAYS attack its nemsis, no matter what
};


// ObjectCaps() These are caps bits to indicate what an object's capabilities
//#define FCAP_CUSTOMSAVE			(1 << 0 )//0x00000001// unused
#define FCAP_ACROSS_TRANSITION		(1 << 1 )//0x00000002// should transfer between transitions
#define FCAP_MUST_SPAWN				(1 << 2 )//0x00000004// Spawn after restore
#define FCAP_IMPULSE_USE			(1 << 3 )//0x00000008// can be used by the player
#define FCAP_CONTINUOUS_USE			(1 << 4 )//0x00000010// can be used by the player
#define FCAP_ONOFF_USE				(1 << 5 )//0x00000020// can be used by the player
#define FCAP_DIRECTIONAL_USE		(1 << 6 )//0x00000040// Player sends +/- 1 when using (currently only tracktrains)
#define FCAP_MASTER					(1 << 7 )//0x00000080// Can be used to "master" other entities (like multisource)
#define FCAP_ONLYDIRECT_USE			(1 << 8 )//0x00000100// XDM: this entity can be used only if it's visible
#define FCAP_FORCE_TRANSITION		(1 << 9 )//0x00000200// ALWAYS goes across transitions // UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define FCAP_HOLD_ANGLES			(1 << 10)//0x00000400// xhashmod: hold angles at spawn to let parent system right attach the child
#define FCAP_NOT_MASTER				(1 << 11)//0x00000800// xhashmod: this entity can't be used as master directly (only through multi_watcher)
#define FCAP_IGNORE_PARENT			(1 << 12)//0x00001000// xhashmod: this entity won't to attached
#define FCAP_DONT_SAVE				(1 << 31)//0x80000000// Don't save this

// m_iSkill exclusion flags
#define SKF_NOTEASY					(1 << 0)
#define SKF_NOTMEDIUM				(1 << 1)
#define SKF_NOTHARD					(1 << 2)

// Absolutely common entity spawn flags
// MUST NOT CONFLICT WITH ENGINE'S SF_NOTINDEATHMATCH!!!
#define SF_NOREFERENCE				(1 << 29)// XDM3035 this entity does not have a world reference (a weapon that was not picked up but added by game rules, etc. or a monster created by monstermaker)
#define	SF_NORESPAWN				(1 << 30)// !!!set this bit on guns and stuff that should never respawn.
// don't add flags here! #define SF_MONSTER_FALL_TO_GROUND		(1 << 31)

class CBaseEntity;
class CBaseMonster;
class CBasePlayer;
class CBasePlayerItem;
class CBaseAlias;// SHL
class CSquadMonster;

// Common callback types
typedef void (CBaseEntity::*BASEPTR)(void);
typedef void (CBaseEntity::*ENTITYFUNCPTR)(CBaseEntity *pOther);
typedef void (CBaseEntity::*USEPTR)(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
//typedef void (CBaseEntity::*MOVEDONEPTR)(int direction);

//-----------------------------------------------------------------------------
// Purpose: HACKFIX: engine does not initialize entvars with adequate values!
// Input  : pEntVars -
//-----------------------------------------------------------------------------
inline void InitEntVars(entvars_t *pEntVars)
{
	pEntVars->gravity = 1.0f;// XDM3038
	pEntVars->friction = 1.0f;// XDM3038
	pEntVars->framerate = 1.0f;// XDM3038c: many entities (like env_static) rely on this
	pEntVars->scale = 1.0f;// pEdict->v.scale// only for newly created entities
	pEntVars->renderamt = 255.0f;// XDM3038b
	pEntVars->rendercolor.Set(255,255,255);// XDM3038b
}

//-----------------------------------------------------------------------------
// Purpose: Converts entvars_t * to a class pointer
// It will allocate the class and entity if necessary
//
// XDM: tried to replace this with edict_t version. But better keep compatibility.
//
// Input  : T is the desired class
//			*a - can be null (new entity) or existing edict_t (get entity pointer)
//			szName - name of the entity to create
// Output : <class T> * - new or existing CBaseEntity
//-----------------------------------------------------------------------------
template <class T> T *GetClassPtr(T *a, const char *szName)// XDM
{
	entvars_t *pEntVars = (entvars_t *)a;// HACK: a is actually entvars_t, but its type is a class
	edict_t *pEdict = ENT(pEntVars);

	// allocate entity if necessary
	if (pEdict == NULL)
	{
		//dbg	conprintf(1, "GetClassPtr(): allocating edict %s\n", szName);
#if defined (USE_EXCEPTIONS)
		try
		{
#endif
			ASSERT(szName != NULL);
			if (szName == NULL)
				return NULL;// XDM3038c //pEdict = CREATE_ENTITY();// this just allocates an edict
			else
				pEdict = CREATE_NAMED_ENTITY(szName);//pEdict = CREATE_NAMED_ENTITY(static_cast<int>(MAKE_STRING(szName)));// this also calls linked classname() exported by this DLL
#if defined (USE_EXCEPTIONS)
		}
		catch(...)
		{
			conprintf(0, "GetClassPtr(%s): exception!\n", szName);
			//DBG_FORCEBREAK
		}
#endif
		pEntVars = &pEdict->v;
		InitEntVars(pEntVars);
	}
#if defined (_DEBUG)
	else// just getting pointer to an existing entity, check for classname match
	{
		if (szName != NULL)
			ASSERT(strcmp(STRING(pEdict->v.classname), szName) == 0);
	}
#endif

	// Get the private data - the CBaseEntity instance
	a = (T *)GET_PRIVATE(pEdict);

	if (a == NULL)
	{
		// allocate private data
		a = new(pEdict) T();
		if (a)// Now, when constructor was called, it's safe to set variables
		{
			a->pev = pEntVars;
			pEdict->pvPrivateData = a;
			if (szName)
				a->SetClassName(szName);// XDM3038: 20141121
		}
	}
	return a;
}


//-----------------------------------------------------------------------------
//
// EHANDLE
// Because entity indexes may be reused.
// Safe way to point to CBaseEntities who may die between frames.
// Self-invalidates (when edict's serialnumber changes)
//
//-----------------------------------------------------------------------------
class EHANDLE
{
private:
	edict_t *m_pent;
	int		m_serialnumber;
public:
#if defined(_DEBUG)
	int		m_debuglevel;
#endif
	EHANDLE();// XDM3035
	EHANDLE(CBaseEntity *pEntity);// XDM3038a
	EHANDLE(edict_t *pent);// XDM3038a

	edict_t *Get(void);
	edict_t *Set(edict_t *pent);
	const bool IsValid(void) const;// XDM3038c
	//CBaseEntity *GetEntity(void);

	operator const int();// const;
	operator CBaseEntity *();
	operator const CBaseEntity *();

	CBaseEntity *operator = (CBaseEntity *pEntity);
	CBaseEntity *operator ->();

	inline bool operator==(const EHANDLE &eh) const;
	inline bool operator==(/*const */CBaseEntity *pEntity) const;
};


// XDM3038: must be reliable s steel
enum entity_existence_states_e
{
	ENTITY_EXSTATE_WORLD = 0,// visible, full interaction
	ENTITY_EXSTATE_CONTAINER,// invisible, no interaction (stored)
	ENTITY_EXSTATE_CARRIED,// may be visible, interacts with owner, can be used (item, weapon)
	/*ENTITY_EXSTATE_DROPPED,// visible, no interaction with owner
	ENTITY_EXSTATE_4,
	ENTITY_EXSTATE_5,
	ENTITY_EXSTATE_6,*/
	ENTITY_EXSTATE_VOID = 7// placeholder, empty space, nothing here
};


//-----------------------------------------------------------------------------
//
// Base Entity.  All entity types derive from this.
// Optimize as much as possible!
//
//-----------------------------------------------------------------------------
class CBaseEntity
{
public:
	// Constructor.  Set engine to use C/C++ callback functions
	CBaseEntity();// XDM3035c: the only way to pre-set variables before KeyValue() takes place
#if !defined(_MSC_VER) || _MSC_VER > 1200// VC6 does not allow this
	virtual ~CBaseEntity();
#endif

	// pointers to engine data
	entvars_t		*pev;// Don't need to save/restore this pointer, the engine resets it
	// path corners
	CBaseEntity		*m_pGoalEnt;// path corner we are heading towards
	CBaseEntity		*m_pLink;// used for temporary link-list operations.

	int				m_iSkill;// XDM
	string_t		m_iszGameRulesPolicy;// XDM3035c: a special set of game rules IDs at which this entity is allowed to spawn
	//string_t		m_iszIcon;// XDM3035c: overview/minimap icon
	float			m_flBurnTime;// XDM
	float			m_flRemoveTime;// XDM3038a: when set to real time, entity will be forcibly removed
	Vector			m_vecSpawnSpot;// XDM3035: first map spawn spot (where put by map editor)
	EHANDLE			m_hOwner;// XDM3037: to avoid traceline confusion (get rid of pev->owner)
	EHANDLE			m_hTBDAttacker;// XDM3037: attacker of time-based damage
	char			m_szClassName[MAX_ENTITY_STRING_LENGTH];
	//char			m_szTargetName[MAX_ENTITY_STRING_LENGTH];
	//char			m_szTarget[MAX_ENTITY_STRING_LENGTH];
// WTF?! #if defined (_WIN32)
protected:// XDM3038
//#endif
	uint32			m_iExistenceState;// XDM3038: for everything
public:
	// fundamental callbacks
	BASEPTR			m_pfnThink;
	ENTITYFUNCPTR	m_pfnTouch;
	USEPTR			m_pfnUse;
	ENTITYFUNCPTR	m_pfnBlocked;

#if defined(MOVEWITH)// if this bunch of hacks is desired
	CBaseEntity *m_pMoveWith;// XDM
	CBaseEntity *m_pChildMoveWith;// SHL: one of the entities that's moving with me
	CBaseEntity	*m_pSiblingMoveWith;// SHL: another entity that's Moving With the same ent as me. (linked list.)
	CBaseEntity *m_pAssistLink;// SHL: link to the next entity which needs to be Assisted before physics are applied
	string_t	m_iszMoveWith;
	//Vector	m_vecMoveOriginDelta;

	Vector		m_vecPostAssistVel;// SHL
	Vector		m_vecPostAssistAVel;// SHL
	Vector		m_vecPostAssistOrg;// SHL: child postorigin
	Vector		m_vecPostAssistAng;// SHL: child postangles

	Vector		m_vecOffsetOrigin;	// spawn offset origin
	Vector		m_vecOffsetAngles;	// spawn offset angles
	Vector		m_vecParentOrigin;	// temp container
	Vector		m_vecParentAngles;	// temp container

	float		m_fNextThink;// SHL: for SetNextThink and SetPhysThink. Marks the time when a think will be performed - not necessarily the same as pev->nextthink!
	float		m_fPevNextThink;// SHL: always set equal to pev->nextthink, so that we can tell when the latter gets changed by the @#$^¬! engine.
	Vector		m_vecSpawnOffset;// SHL: To fix things which (for example) MoveWith a door which Starts Open.
	BOOL		m_activated;// SHL:- moved here from func_train. Signifies that an entity has already been activated. (and hence doesn't need reactivating.)
#endif // MOVEWITH
#if defined(SHL_LIGHTS)
	int			m_iStyle;// generic lightstyles for any brush entity
#endif // SHL_LIGHTS

public:
	// initialization functions
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Spawn(byte restore);
	virtual void Precache(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void OnFreePrivateData(void);// XDM3035
	virtual void PrepareForTransfer(void);// XDM3037
	virtual int ObjectCaps(void);
	virtual void Activate(void) {}
	virtual void PostSpawn(void) {}// SHL
	virtual void Materialize(void);// XDM3038c
	virtual void Destroy(void);// XDM3038c
	virtual CBaseEntity *StartRespawn(void);// XDM3038c
	virtual void OnRespawn(void);// XDM3038c
	//virtual void InitMoveWith(void);
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3035
	virtual bool ShouldBeSentTo(CBasePlayer *pClient) { return true; }// XDM3035c
	// Setup the object->object collision box (pev->mins / pev->maxs is the object->world collision box)
	virtual void SetObjectCollisionBox(void);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);// XDM3035: return value 0 means no data were sent
	virtual void DesiredAction(void) {}// XDM3037

	virtual void UpdateOnRemove(void);// XDM3034 virtual, needed by things like satchels
	virtual void Disintegrate(void);// XDM3035
	virtual STATE GetState(void) { return STATE_OFF; }
	virtual	int GetToggleState(void) { return TS_AT_TOP; }
	virtual void SetToggleState(int state) {}// This is ONLY used by the node graph to test movement through a door
	virtual int ShouldToggle(USE_TYPE useType, bool currentState);

	virtual int Classify(void) { return CLASS_NONE; }
	virtual void DeathNotice(CBaseEntity *pChild) {}// monster maker children use this to tell the monster maker that they have died.
	// XDM3034: replaced old shitty functions with brand new
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual float TakeHealth(const float &flHealth, const int &bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int GetScoreAward(void) { return 0; }// XDM3038c

	virtual int BloodColor(void) { return DONT_BLEED; }
	virtual void TraceBleed(const float &flDamage, const Vector &vecDir, TraceResult *ptr, const int &bitsDamageType);

	virtual float DamageForce(const float &damage);// XDM3035
	virtual short DamageInteraction(const int &bitsDamageType, const Vector &vecSrc);// XDM3037
	virtual CBaseEntity *GetDamageAttacker(void);// XDM3037

	// Do the bounding boxes of these two intersect?
	virtual int Intersects(CBaseEntity *pOther);
	virtual void MakeDormant(void);

	virtual float GetDelay(void) { return 0; }
	virtual void OverrideReset(void) {}
	virtual int DamageDecal(const int &bitsDamageType);

	virtual bool OnControls(entvars_t *onpev) { return false; }
	virtual bool IsTriggered(const CBaseEntity *pActivator) { return true; }
	virtual bool IsDormant(void) const;
	virtual bool IsLockedByMaster(const CBaseEntity *pActivator = NULL) { return false; }
	virtual bool IsMoving(void) const { return !(pev->velocity.IsZero()); }
	virtual bool IsAlive(void) const { return (pev->deadflag == DEAD_NO) && (pev->health > 0.0f); }
	virtual bool IsBSPModel(void) const;
	virtual bool IsInWorld(void) const;
	virtual	bool IsPlayer(void) const;
	virtual bool IsNetClient(void) const { return false; }
	virtual	bool IsMonster(void) const { return false; }// XDM
	virtual bool IsMovingBSP(void) const { return false; }// XDM
	virtual bool IsProjectile(void) const { return false; }// XDM
	virtual bool IsPushable(void) { return FALSE; }// XDM
	virtual bool IsHuman(void)/* const*/ { return false; }// XDM
	virtual bool IsBot(void) const { return false; }// XDM
	virtual bool IsBreakable(void) const { return false; }// XDM
	virtual bool IsPlayerItem(void) const { return false; }// XDM3035
	virtual bool IsPlayerWeapon(void) const { return false; }// XDM3035
	virtual bool IsTrigger(void) const { return false; }// XDM3035a
	virtual bool IsAlias(void) const { return false; }// SHL
	virtual bool IsGameGoal(void) const { return false; }// XDM3036/7
	virtual bool IsPickup(void) const { return false; }// XDM3037
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual bool ShowOnMap(CBasePlayer *pPlayer) const { return false; }// XDM3038c

	virtual bool FBecomeProne(void);// XDM3038c

	virtual bool HasTarget(string_t targetname);
	virtual CBaseEntity *GetNextTarget(void);

	virtual int AddPlayerItem(CBasePlayerItem *pItem) { return 0; }
	virtual bool RemovePlayerItem(CBasePlayerItem *pItem) { return false; }
#if defined(OLD_WEAPON_AMMO_INFO)
	virtual int GiveAmmo(const int &iAmount, const int &iIndex, const int &iMax) { return -1; }
#else
	virtual int GiveAmmo(const int &iAmount, const int &iIndex) { return -1; }
#endif

	virtual void SetModelCollisionBox(void);// XDM
	virtual void AlignToFloor(void);// XDM
	virtual void CheckEnvironment(void);// XDM3035b
	virtual int Illumination(void) { return GETENTITYILLUM(edict()); }

	virtual Vector Center(void) { return (pev->absmin + pev->absmax) * 0.5f; } // center point of entity
	virtual Vector EyePosition(void) { return (pev->origin + pev->view_ofs); }	// position of eyes
	virtual Vector EarPosition(void) { return (pev->origin + pev->view_ofs); }	// position of ears

	// XDM3038a: now these two are not monster-specific
	virtual void BarnacleVictimBitten(CBaseEntity *pBarnacle) {}
	virtual void BarnacleVictimReleased(void) {}

	// SHL stuff
	virtual Vector BodyTarget(const Vector &posSrc)			{ return Center(); }		// position to shoot at
	virtual Vector CalcPosition(CBaseEntity *pLocus)		{ return pev->origin; }
	virtual Vector CalcVelocity(CBaseEntity *pLocus)		{ return pev->velocity; }
	virtual float CalcRatio(CBaseEntity *pLocus, int mode)	{ return 0.0f; }

	virtual	bool FVisible(const Vector &vecOrigin);
	virtual	bool FVisible(CBaseEntity *pEntity);
	virtual bool FBoxVisible(CBaseEntity *pTarget, Vector &vecTargetOrigin, float flSize);// vecTargetOrigin is output

	virtual void LinearMove(const Vector &vecDest, const float &flSpeed) {}

	virtual int IRelationship(CBaseEntity *pTarget);// XDM3037: R_NO
	virtual CBaseMonster *MyMonsterPointer(void) { return NULL; }
	virtual CSquadMonster *MySquadMonsterPointer(void) { return NULL; }

	// WARNING: overriding these may lead to malfunction! Corresponding Set() functions won't work!
	virtual void EXPORT Think(void);
	virtual void EXPORT Touch(CBaseEntity *pOther);
	virtual void EXPORT Blocked(CBaseEntity *pOther);
	virtual void EXPORT Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	// common member functions
	virtual void SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value);
	virtual void ReportState(int printlevel);// XDM3038c

	// "Think" callbacks
	void EXPORT SUB_Remove(void);
	void EXPORT SUB_DoNothing(void);
	void EXPORT SUB_StartFadeOut(void);
	void EXPORT SUB_FadeOut(void);
	void EXPORT SUB_Disintegrate(void);// XDM3035: don't call or SetThink to this directly, use Disintegrate() instead
	void EXPORT SUB_CallUseToggle(void);// XDM3035c: don't export from headers
	void EXPORT SUB_Respawn(void);// XDM3035

	// internal mechanisms
	void ScheduleRespawn(const float delay);// XDM3038c
	void DontThink(void);// XDM3037
	void SetNextThink(const float &delay);// XDM: SHL compatibility
	bool IsRemoving(void);// XDM3037
	void SetClassName(const char *szClassName);// XDM3038
	const uint32 GetExistenceState(void) const { return m_iExistenceState; }// XDM3038

	// Engine structures retrieval
	inline edict_t *edict(void) const { return ENT(pev); }
	inline EOFFSET eoffset(void) { return OFFSET(edict()); }
#if defined(_DEBUG)
	inline int entindex(void) const
	{
		ASSERT(pev != NULL);
		edict_t *pEdict = edict();
		ASSERT(pEdict != NULL);
		if (pEdict)
		{
			ASSERT(pEdict->pvPrivateData != NULL);
			ASSERT(pEdict->free == 0);
			return ENTINDEX(edict());
		}
		return -1;
	}
#else
	inline int entindex(void) const { return ENTINDEX(edict()); }
#endif
	// Allow engine to allocate instance data
    void *operator new(size_t stAllocateBlock, edict_t *pEdict);
	// don't use this directly!
#if _MSC_VER >= 1200// only build this code if MSVC++ 6.0 or higher
	void operator delete(void *pMem, edict_t *pEdict);
	//void operator delete(void *pMem);
#endif

	inline void PunchPitchAxis(const float &angle)// XDM3038c: SQB abstraction layer, accepts CORRECT value. SQB only affects pitch.
	{
#if defined (NOSQB)
		pev->punchangle.x = angle;
#else
		pev->punchangle.x = -angle;
#endif
	}
	inline void SetThinkNull(void)// no effect on overloaded Think()
	{
		m_pfnThink = NULL;
	}
	inline void SetTouchNull(void)// no effect on overloaded Touch()
	{
		m_pfnTouch = NULL;
	}
	inline void SetUseNull(void)// no effect on overloaded Use()
	{
		m_pfnUse = NULL;
	}
	inline void SetBlockedNull(void)// no effect on overloaded Blocked()
	{
		m_pfnBlocked = NULL;
	}

	// Ugly code to lookup all functions to make sure they are exported when set.
#if defined(_DEBUG)
	void FunctionCheck(void *pFunction, char *name)
	{
		if (pFunction && !NAME_FOR_FUNCTION((unsigned long)(pFunction)))
			ALERT(at_error, "No EXPORT: %s:%s (%08lx)\n", STRING(pev->classname), name, (unsigned long)pFunction);
	}

	BASEPTR ThinkSet(BASEPTR func, char *name)
	{
		ASSERT(func != &CBaseEntity::Think);
		m_pfnThink = func;
		FunctionCheck((void *)*((int *)((char *)this + (offsetof(CBaseEntity,m_pfnThink)))), name);
		return func;
	}
	ENTITYFUNCPTR TouchSet(ENTITYFUNCPTR func, char *name)
	{
		ASSERT(func != &CBaseEntity::Touch);
		m_pfnTouch = func;
		FunctionCheck((void *)*((int *)((char *)this + (offsetof(CBaseEntity,m_pfnTouch)))), name);
		return func;
	}
	USEPTR UseSet(USEPTR func, char *name)
	{
		ASSERT(func != &CBaseEntity::Use);
		m_pfnUse = func;
		FunctionCheck((void *)*((int *)((char *)this + (offsetof(CBaseEntity,m_pfnUse)))), name);
		return func;
	}
	ENTITYFUNCPTR BlockedSet(ENTITYFUNCPTR func, char *name)
	{
		ASSERT(func != &CBaseEntity::Blocked);
		m_pfnBlocked = func;
		FunctionCheck((void *)*((int *)((char *)this + (offsetof(CBaseEntity,m_pfnBlocked)))), name);
		return func;
	}
#else
	BASEPTR ThinkSet(BASEPTR func)
	{
		m_pfnThink = func;
		return func;
	}
	ENTITYFUNCPTR TouchSet(ENTITYFUNCPTR func)
	{
		m_pfnTouch = func;
		return func;
	}
	USEPTR UseSet(USEPTR func)
	{
		m_pfnUse = func;
		return func;
	}
	ENTITYFUNCPTR BlockedSet(ENTITYFUNCPTR func)
	{
		m_pfnBlocked = func;
		return func;
	}
#endif

	static CBaseEntity *Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner = NULL);// XDM
	static CBaseEntity *Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVeloity, edict_t *pentOwner = NULL, int spawnflags = 0);
	static CBaseEntity *CreateCopy(const char *szNewClassName, CBaseEntity *pSource, int spawnflags, bool spawn = true);// XDM3035a: provide a source entvars to be copied to a new entity
	static CBaseEntity *Instance(edict_t *pent);

	static TYPEDESCRIPTION m_SaveData[];
};

/*
template <typename To, typename From>

inline To unsafe_union_cast(From from)
{
	struct union_cast_holder
	{
		union
		{
			From from;
			To to;
		};
	};
	ASSERT(sizeof(From) == sizeof(To) && sizeof(From) == sizeof(union_cast_holder));
	union_cast_holder holder;
	holder.from = from;
	return holder.to;
}
*/

// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a
// member function of a base class.  static_cast is a sleezy way around that problem.
#if defined(_MSC_VER) && _MSC_VER >= 1200// only build this code if MSVC++ 6.0 or higher

#if defined(_DEBUG)

#define SetThink(a) ThinkSet(static_cast <BASEPTR>(a), #a)
#define SetTouch(a) TouchSet(static_cast <ENTITYFUNCPTR>(a), #a)
#define SetUse(a) UseSet(static_cast <USEPTR> (a), #a)
#define SetBlocked(a) BlockedSet(static_cast <ENTITYFUNCPTR>(a), #a)
#define SetMoveDone(a) m_pfnCallWhenMoveDone = static_cast <BASEPTR>(a)

#else

/* error C2248: 'CBaseEntity::m_pfnThink' : cannot access protected member declared in class 'CBaseEntity'
#define SetThink(a) m_pfnThink = static_cast <BASEPTR>(a)
#define SetTouch(a) m_pfnTouch = static_cast <ENTITYFUNCPTR>(a)
#define SetUse(a) m_pfnUse = static_cast <USEPTR>(a)
#define SetBlocked(a) m_pfnBlocked = static_cast <ENTITYFUNCPTR>(a)
#define SetMoveDone(a) m_pfnCallWhenMoveDone = static_cast <BASEPTR>(a)*/

#define SetThink(a) ThinkSet(static_cast <BASEPTR>(a))
#define SetTouch(a) TouchSet(static_cast <ENTITYFUNCPTR>(a))
#define SetUse(a) UseSet(static_cast <USEPTR> (a))
#define SetBlocked(a) BlockedSet(static_cast <ENTITYFUNCPTR>(a))
#define SetMoveDone(a) m_pfnCallWhenMoveDone = static_cast <BASEPTR>(a)

#endif// _DEBUG

#else// _MSC_VER

#define SetThink(a) m_pfnThink = static_cast <BASEPTR>(a)
#define SetTouch(a) m_pfnTouch = static_cast <ENTITYFUNCPTR>(a)
#define SetUse(a) m_pfnUse = static_cast <USEPTR>(a)
#define SetBlocked(a) m_pfnBlocked = static_cast <ENTITYFUNCPTR>(a)
#define SetMoveDone(a) m_pfnCallWhenMoveDone = static_cast <BASEPTR>(a)

#endif// _MSC_VER

/* the original
#define SetThink(a) m_pfnThink = static_cast <void (CBaseEntity::*)(void)> (a)
#define SetTouch(a) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define SetUse(a) m_pfnUse = static_cast <void (CBaseEntity::*)(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)> (a)
#define SetBlocked(a) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
*/




//-----------------------------------------------------------------------------
//
// Base class for entities which have delayed targets for activation
//
//-----------------------------------------------------------------------------
class CBaseDelay : public CBaseEntity
{
public:
	CBaseDelay();// XDM3035c: the only way to pre-set variables before KeyValue() takes place
	//virtual ~CBaseDelay();

	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);// XDM3035c
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value);// default version, uses pev->target
	virtual void UseTargets(string_t iszTarget, string_t iszKillTarget, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// the real function
	virtual STATE GetState(void) { return m_iState; }
	virtual void SetState(STATE newstate);
	virtual void OnStateChange(STATE oldstate);
	virtual bool IsLockedByMaster(const CBaseEntity *pActivator = NULL);
	virtual bool IsTriggered(const CBaseEntity *pActivator);// XDM3038
	virtual CBaseEntity *GetDamageAttacker(void);// XDM3037
	virtual void ReportState(int printlevel);// XDM3038c

	void EXPORT DelayThink(void);

	float		m_flDelay;
	int			m_iszKillTarget;// targetname of entities to delete on activation
	string_t	m_iszMaster;// If this button has a master switch, this is the targetname.
							// If master have been triggered, then the button will be allowed to operate. Otherwise, it will be deactivated.
	EHANDLE		m_hActivator;// XDM3035

	static TYPEDESCRIPTION m_SaveData[];

private:
	STATE		m_iState;// XDM3035c: SHL
};




//-----------------------------------------------------------------------------
//
// Base class for entities which have to control their animations
//
//-----------------------------------------------------------------------------
class CBaseAnimating : public CBaseDelay
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);// XDM
	virtual void Precache(void);// XDM3038a
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void ReportState(int printlevel);// XDM3038c

	// Basic Monster Animation functions
	float StudioFrameAdvance(float flInterval = 0.0f); // accumulate animation frame time from last time called until now
	int	GetSequenceFlags(void);
	int LookupActivity(int activity);
	int LookupActivityHeaviest(int activity);
	int LookupSequence(const char *label);
	void ResetSequenceInfo(void);
	void DispatchAnimEvents(float flFutureInterval = 0.1); // Handle events that have happend since last time called up until X seconds into the future
	float SetBoneController(int iController, float flValue);
	void InitBoneControllers(void);
	float SetBlending(byte iBlender, float flValue);
	void GetBonePosition(int iBone, Vector &origin, Vector &angles);
	//void GetAutomovement(Vector &origin, Vector &angles, float flInterval = 0.1);
	int  FindTransition(int iEndingSequence, int iGoalSequence, int *piDir);
	void GetAttachment(int iAttachment, Vector &origin, Vector &angles);
	void SetBodygroup(int iGroup, int iValue);
	int GetBodygroup(int iGroup);
	int ExtractBbox(int sequence, Vector &mins, Vector &maxs);
	void SetSequenceBox(void);
	void UpdateFrame(void);// XDM
	void ValidateBodyGroups(bool settolast);// XDM3038

	// animation needs
	float		m_flFrameRate;		// computed FPS for current sequence
	float		m_flGroundSpeed;	// computed linear movement rate for current sequence
	float		m_flLastEventCheck;	// last time the event list was checked
	BOOL		m_fSequenceFinished;// flag set when StudioAdvanceFrame moves across a frame boundry
	BOOL		m_fSequenceLoops;	// true if the sequence loops
	int			m_nFrames;// XDM: sprites

	static TYPEDESCRIPTION m_SaveData[];
};




#define	SF_ITEM_USE_ONLY	256 //  ITEM_USE_ONLY = BUTTON_USE_ONLY = DOOR_USE_ONLY!!!

//-----------------------------------------------------------------------------
//
// Base class for entities which have determined toggle states and movement
//
//-----------------------------------------------------------------------------
class CBaseToggle : public CBaseAnimating
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int GetToggleState(void) { return m_toggle_state; }
	virtual float GetDelay(void) { return m_flWait; }
	virtual void ReportState(int printlevel);// XDM3038c

	// common member functions
	virtual void LinearMove(const Vector &vecDest, const float &flSpeed);
	virtual void AngularMove(const Vector &vecDestAngles, const float &flSpeed);
	//virtual void InterpolatedMove(Vector vecDest, float flStartSpeed, float flEndSpeed);// XDM

	void EXPORT LinearMoveDone(void);
	void EXPORT AngularMoveDone(void);
	//void EXPORT InterMove(void);// XDM

	void AxisDir(void);

	static float AxisValue(int flags, const Vector &angles);
	static float AxisDelta(int flags, const Vector &angle1, const Vector &angle2);

	/*BASEPTR MoveDoneSet(BASEPTR func)
	{
		m_pfnCallWhenMoveDone = func;
		return func;
	};*/

	TOGGLE_STATE		m_toggle_state;
	float				m_flActivateFinished;//like attack_finished, but for doors
	float				m_flMoveDistance;// how far a door should slide or rotate
	float				m_flWait;
	float				m_flLip;
	float				m_flTWidth;// for plats
	float				m_flTLength;// for plats

	Vector				m_vecPosition1;
	Vector				m_vecPosition2;
	Vector				m_vecAngle1;
	Vector				m_vecAngle2;

	int					m_cTriggersLeft;		// trigger_counter only, # of activations remaining
	float				m_flHeight;
	BASEPTR				m_pfnCallWhenMoveDone;// XDM3035
	Vector				m_vecFinalDest;
	Vector				m_vecFinalAngle;

	int					m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does

	static TYPEDESCRIPTION m_SaveData[];
};




//-----------------------------------------------------------------------------
//
// Base class for entities without physical/visual models, size or movement
//
//-----------------------------------------------------------------------------
class CPointEntity : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};


#define SF_WORLD_DARK				0x0001// Fade from black at startup
#define SF_WORLD_TITLE				0x0002// Display game title at startup
#define SF_WORLD_FORCETEAM			0x0004// Force teams
#define SF_WORLD_STARTSUIT			0x0008// XDM: start with HEV suit on
#define SF_WORLD_NOMONSTERS			0x0010// XDM: does the map allow monsters?
#define SF_WORLD_NOAIRSTRIKE		0x0020// XDM3038a: map forbids airstrikes (xen)
#define SF_WORLD_NEWUNIT			0x0040// XDM3038c: now used to indicate new single/coop map sequence

//-----------------------------------------------------------------------------
//
// Represents the world (entity #0), spawns first when a map is loaded
//
//-----------------------------------------------------------------------------
class CWorld : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(byte restore);// XDM
	virtual void Precache(void);
	virtual int ObjectCaps(void) { return FCAP_MUST_SPAWN; }// XDM3034: Calls CWorld::Spawn() after loading the game, making g_pWorld != NULL
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);// XDM3035c
	virtual void OnFreePrivateData(void);// XDM3036
	virtual bool IsBSPModel(void) const { return true; }// XDM
	virtual bool IsMovingBSP(void) const { return false; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	CBaseAlias *m_pFirstAlias;// SHL
	int m_iRoomType;// XDM3038c: better use this separately
	int m_iMaxVisibleRange;// XDM3038c
	int m_iMaxMapTeams;// XDM3037a
	int m_iNumSecrets;// XDM3038c: don't save, updates automatically
	int m_iAudioTrack;// XDM3038c: avoid letting engine know
	string_t m_iszGamePrefix;// XDM3038c
	string_t m_iszTitleSprite;// XDM3038c
	string_t m_rgiszAmmoTypes[MAX_AMMO_SLOTS];// XDM3038c: save indexes so saved games may retain ammo ID compatibility if text file is changed
	uint32 m_rgAmmoMaxCarry[MAX_AMMO_SLOTS];// XDM3038c
	uint32 m_iTeamDelay[MAX_TEAMS+1];// XDM3038a
	static TYPEDESCRIPTION m_SaveData[];
};


// These operators must be defined AFTER CBaseEntity

// XDM3037: explicit. TODO: find out if checking m_serialnumber is a good idea
inline bool EHANDLE::operator==(const EHANDLE &eh) const
{
	return (m_pent == eh.m_pent && m_serialnumber == eh.m_serialnumber);
}

// XDM3038: avoid C6011
inline bool EHANDLE::operator==(/*const */CBaseEntity *pEntity) const
{
	if (pEntity == NULL)
	{
		if (m_pent == NULL)
			return true;
	}
	else
	{
		if (pEntity->pev)
			return (pEntity->edict() == m_pent && pEntity->edict()->serialnumber == m_serialnumber);
	}
	return false;
}


// Common entity-related utility functions

inline void UTIL_SetOrigin(CBaseEntity *pEntity, const Vector &vecOrigin)
{
#if defined (_DEBUG)
	if (pEntity == NULL)
		return;

	//if (!pEntity->IsInWorld())
	if((pEntity->pev->origin.x > MAX_ABS_ORIGIN)
	|| (pEntity->pev->origin.y > MAX_ABS_ORIGIN)
	|| (pEntity->pev->origin.z > MAX_ABS_ORIGIN)
	|| (pEntity->pev->origin.x < -MAX_ABS_ORIGIN)
	|| (pEntity->pev->origin.y < -MAX_ABS_ORIGIN)
	|| (pEntity->pev->origin.z < -MAX_ABS_ORIGIN))
		conprintf(1, "UTIL_SetOrigin(%s[%d]) error: outside the world!\n", STRING(pEntity->pev->classname), pEntity->entindex());
#endif
	if (pEntity)
		SET_ORIGIN(pEntity->edict(), vecOrigin);
}

inline void UTIL_SetAngles(CBaseEntity *pEntity, const Vector &vecAngles)
{
#if defined (_DEBUG)
	if (!ASSERT(pEntity != NULL))
		return;
#endif
	if (pEntity)
		pEntity->pev->angles = vecAngles;
}


typedef bool (*fnEntityUseCallback)(CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller);// XDM3037

unsigned int FireTargets(const char *pTargetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value, fnEntityUseCallback Callback = NULL);
void SetObjectCollisionBox(entvars_t *pev);// XDM

// XDM: can't move these to util.h becuase of types used
const char *GetStringForUseType(USE_TYPE useType);
const char *GetStringForState(STATE state);// XDM3035c
STATE GetStateForString(const char *string);// XDM3035c


// Global data

extern int g_iRelationshipTable[NUM_RELATIONSHIP_ClASSES][NUM_RELATIONSHIP_ClASSES];

#endif // CBASE_H
