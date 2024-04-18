//====================================================================
//
// Purpose: entities that do logic
//
//====================================================================
#ifndef ENT_FUNCTIONAL_H
#define ENT_FUNCTIONAL_H
#if defined (_WIN32)
#pragma once
#endif

//-----------------------------------------------------------------------------
// MultiSouce
//-----------------------------------------------------------------------------
#define MS_MAX_TARGETS		32

#define SF_MULTISOURCE_ON			0x00000001// XDM3035c: start in enabled state
//#define SF_MULTISOURCE_F2			0x00000002
//#define SF_MULTISOURCE_INIT		0x80000000// OBSOLETE: initializing (now pev->impulse)

class CMultiSource : public CBaseDelay// XDM3038c: CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	ObjectCaps(void) { return (CBaseDelay::ObjectCaps() | FCAP_MASTER); }//{ return (CPointEntity::ObjectCaps() | FCAP_MASTER); }
	virtual bool IsTriggered(const CBaseEntity *pActivator);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void ReportState(int printlevel);// XDM3038c

	void EXPORT Register(void);

	EHANDLE		m_rgEntities[MS_MAX_TARGETS];
	BOOL		m_rgTriggered[MS_MAX_TARGETS];// watch out for printf's if you change the type!
	uint32		m_iTotal;
	string_t	m_globalstate;

	static	TYPEDESCRIPTION m_SaveData[];
};


//-----------------------------------------------------------------------------
// Purpose: The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
//-----------------------------------------------------------------------------
#define SF_MULTIMAN_THREAD			0x00000001// (create clones when triggered)
//#define SF_MULTIMAN_F2			0x00000002// ?
#define SF_MULTIMAN_LOOP			0x00000004// XDM3035c: repeat continuously
#define SF_MULTIMAN_ONLYONCE		0x00000008
#define SF_MULTIMAN_SPAWNFIRE		0x00000010

#define SF_MULTIMAN_CLONE			0x80000000// (this is a clone for a threaded execution)

#define MAX_MULTI_TARGETS			32 // XDM: maximum number of targets a single multi_manager entity may be assigned.// 32 is still not enough on c1a0d :D

class CMultiManager : public CBaseToggle
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual int ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void Activate(void);
	virtual bool HasTarget(string_t targetname);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void OnStateChange(STATE oldstate);// XDM3037
	virtual void ReportState(int printlevel);// XDM3038c

	void EXPORT ManagerThink(void);
	void EXPORT ManagerUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);


	uint32 m_cTargets; // the total number of targets in this manager's fire list.
	uint32 m_index; // Current target
	float m_startTime;// Time we started firing
	//string_t m_sMaster;

	string_t m_iTargetName[MAX_MULTI_TARGETS];// list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];// delay (in seconds) from time of manager fire to target fire
	string_t m_iszThreadName;// SHL
	string_t m_iszLocusThread;// SHL

private:
	inline bool IsActive(void) { return (GetState() == STATE_ON); }//(pev->impulse > 0); }
	inline bool IsLooping(void) { return FBitSet(pev->spawnflags, SF_MULTIMAN_LOOP); }
	inline bool IsClone(void) { return FBitSet(pev->spawnflags, SF_MULTIMAN_CLONE); }
	inline bool ShouldClone(void)
	{
		if (IsClone())
			return false;

		return FBitSet(pev->spawnflags, SF_MULTIMAN_THREAD);
	}
	CMultiManager *Clone(void);

public:
	static TYPEDESCRIPTION m_SaveData[];
};


//-----------------------------------------------------------------------------
// CEnvGlobal
//-----------------------------------------------------------------------------
#define SF_GLOBAL_SET				0x00000001// Set global state to initial state on spawn

class CEnvGlobal : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	string_t	m_globalstate;
	int			m_triggermode;
	int			m_initialstate;
	static TYPEDESCRIPTION m_SaveData[];
};


//-----------------------------------------------------------------------------
// CEnvState
//-----------------------------------------------------------------------------
#define SF_ENVSTATE_START_ON		0x00000001
#define SF_ENVSTATE_DEBUG			0x00000002

class CEnvState : public CBaseToggle
{
public:
	virtual void Spawn(void);
	virtual void Think(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	float		m_fTurnOnTime;
	float		m_fTurnOffTime;
	static TYPEDESCRIPTION m_SaveData[];
};



//-----------------------------------------------------------------------------
// Beverage Dispenser
// overloaded pev->frags, is now a flag for whether or not a can is stuck in the dispenser.
// overloaded pev->health, is now how many cans remain in the machine.
//-----------------------------------------------------------------------------
class CEnvBeverage : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void DeathNotice(CBaseEntity *pChild);// XDM3038c
};


//-----------------------------------------------------------------------------
// Ammo Dispenser
//-----------------------------------------------------------------------------
class CFuncAmmoDispenser : public CPointEntity
{
public:
//	virtual void Spawn(void);
//	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


//-----------------------------------------------------------------------------
// CEnvCache - entity that precaches other entities
// TODO
//-----------------------------------------------------------------------------
#define ENVCACHE_MAX_MODELS			32
#define ENVCACHE_MAX_SOUNDS			32
#define ENVCACHE_MAX_ENTITIES		32

class CEnvCache : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
//	virtual void Spawn(void);
	virtual void Precache(void);
	// do we need to save these?
	uint32 m_iNumModels;
	uint32 m_iNumSounds;
	uint32 m_iNumEnts;
	char m_MdlNames[ENVCACHE_MAX_ENTITIES][64];
	char m_SndNames[ENVCACHE_MAX_ENTITIES][64];
	char m_EntNames[ENVCACHE_MAX_ENTITIES][64];
};


//-----------------------------------------------------------------------------
// CMultiMaster - multi_watcher
//-----------------------------------------------------------------------------
class CMultiMaster : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Think(void);
//	virtual STATE GetState(CBaseEntity *pActivator) { return EvalLogic(pActivator) ? STATE_ON : STATE_OFF; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	int GetLogicModeForString(const char *string);
	bool CheckState(STATE state, uint32 targetnum);
	bool EvalLogic(CBaseEntity *pEntity);

	uint32 m_cTargets; // the total number of targets in this manager's fire list.
	string_t m_iTargetName[MAX_MULTI_TARGETS]; // list of indexes into global string array
	STATE m_iTargetState[MAX_MULTI_TARGETS]; // list of wishstate targets
	STATE m_iSharedState;
	int m_iLogicMode;
	bool m_bGlobalState;

	static TYPEDESCRIPTION m_SaveData[];
};


//-----------------------------------------------------------------------------
// CSwitcher - multi_switcher
//-----------------------------------------------------------------------------
#define SF_SWITCHER_START_ON		0x00000001

class CSwitcher : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Spawn(void);
	virtual void Think(void);
	virtual void Next(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	string_t m_iTargetName[MAX_MULTI_TARGETS]; // list if indexes into global string array
	uint32 m_cTargets; // the total number of targets in this manager's fire list.
	uint32 m_index; // Current target

	static TYPEDESCRIPTION m_SaveData[];
};


#endif // ENT_FUNCTIONAL_H
