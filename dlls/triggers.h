//====================================================================
//
// Purpose: trigger definitions
//
//====================================================================
#ifndef TRIGGERS_H
#define TRIGGERS_H
#if defined (_WIN32)
#pragma once
#endif


class CBaseTrigger;

// HACK! REWRITE!
class CInOutRegister : public CPointEntity
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	bool IsEmpty(void) { return m_pNext == NULL; }
	bool IsRegistered(CBaseEntity *pValue);// returns true if found in the list

	CInOutRegister *Add(CBaseEntity *pValue);// adds a new entry to the list
	CInOutRegister *Prune(void);// removes all invalid entries from the list, trigger their targets as appropriate, returns new list

	static TYPEDESCRIPTION m_SaveData[];

protected:
	CBaseTrigger *m_pField;
	CInOutRegister *m_pNext;
	EHANDLE m_hValue;
};


class CBaseTrigger : public CBaseToggle
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual bool IsTrigger(void) const { return true; }// XDM3035a
	virtual int	ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void FireOnEntry(CBaseEntity *pOther);// SHL
	virtual void FireOnLeave(CBaseEntity *pOther);// SHL
	virtual void FireOnTouch(CBaseEntity *pOther);// SHL

	bool RegisterToucher(CBaseEntity *pOther);// XDM3038c
	int FindToucher(CBaseEntity *pOther);// XDM3038c
	bool CountTouchers(void);// XDM3038c

	void ActivateMultiTrigger(CBaseEntity *pActivator);
	void InitTrigger(void);
	bool CanTouch(CBaseEntity *pToucher);// XDM
	void EXPORT MultiWaitOver(void);
	void EXPORT CounterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TouchUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	//char m_TouchIndexBits[MAX_EDICTS/CHAR_BIT];// XDM3038c: registered entindexes? Fail because one entity may be removed and another created
	CInOutRegister *m_pIORegister;// XDM3038c: SHL
	EHANDLE m_hzTouchedPlayers[MAX_CLIENTS];// XDM3038c
	//std::vector<EHANDLE> m_TouchedEntities;
	//std::vector<EHANDLE>::iterator m_TouchedEntitiesIterator;
};


// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets
#define SF_AUTO_FIREONCE		0x0001

class CTriggerAuto : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Think(void);
	virtual int ObjectCaps(void) { return (CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_MUST_SPAWN; }// XDM3038c: make it spawn again (test: c1a4*)
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_globalstate;
	USE_TYPE triggerType;
};

class CFireAndDie : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual int ObjectCaps(void) { return CBaseDelay::ObjectCaps() | (FCAP_ACROSS_TRANSITION | FCAP_FORCE_TRANSITION); }// Always go across transitions
};


#define SF_RELAY_FIREONCE		0x0001

class CTriggerRelay : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE triggerType;
};


class CFrictionModifier : public CBaseEntity// UNDONE: CBaseTrigger
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	void EXPORT	ChangeFriction(CBaseEntity *pOther);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];
	float m_frictionFraction;// Sorry, couldn't resist this name :)
};


class CTriggerMonsterJump : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual void Think(void);
};


#define SF_TRIGGERCDAUDIO_LOOP			0x0001
#define SF_TRIGGERCDAUDIO_EVERYONE		0x0002
#define SF_TRIGGERCDAUDIO_REPEATABLE	0x0004

class CTriggerCDAudio : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Touch(CBaseEntity *pOther);
};

class CTargetCDAudio : public CTriggerCDAudio
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	void EXPORT SearchThink(void);
};


#define SF_TRIGGER_HURT_TARGETONCE		1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF		2
//#define	SF_TRIGGER_HURT_			4
#define	SF_TRIGGER_HURT_NO_CLIENTS		8
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.

class CTriggerHurt : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);
	void EXPORT RadiationThink(void);
};


#define TRIGGER_ACTIVATE_VOLUME		128

class CTriggerMultiple : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);// SHL cmp
	virtual void Touch(CBaseEntity *pOther);// XDM
	virtual void FireOnEntry(CBaseEntity *pOther);// SHL cmp
	virtual void FireOnLeave(CBaseEntity *pOther);// SHL cmp
};

class CTriggerOnce : public CTriggerMultiple
{
public:
	virtual void Spawn(void);
};


class CTriggerCounter : public CBaseTrigger
{
public:
	virtual void Spawn(void);
};


class CTriggerVolume : public CBaseTrigger
{
public:
	virtual void Spawn(void);
};



// We can only ever move 512 entities across a transition
#define TRANSITION_MAX_ENTS			512
#define SF_CHANGELEVEL_USEONLY		0x0002

class CChangeLevel : public CBaseTrigger
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsGameGoal(void) const;
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	void EXPORT UseChangeLevel(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TriggerChangeLevel(void);
	void EXPORT TouchChangeLevel(CBaseEntity *pOther);

	void ChangeLevelNow(CBaseEntity *pActivator);

	char m_szMapName[MAX_MAPNAME];		// trigger_changelevel only:  next map
	char m_szLandmarkName[MAX_MAPNAME];		// trigger_changelevel only:  landmark on next map
	string_t m_changeTarget;
	float m_changeTargetDelay;

	static TYPEDESCRIPTION m_SaveData[];
};


#define SF_LADDER_START_OFF					2

class CLadder : public CBaseTrigger
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// XDM3035a: toggle ladder functionality
};


#define	SF_TRIGGER_PUSH_START_OFF			2//spawnflag that makes trigger_push spawn turned OFF
#define	SF_TRIGGER_PUSH_ONLY_NOTONGROUND	4// XDM

class CTriggerPush : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);
};


enum TRIGGER_TELEPORT_CTF_POLICY
{
	TRIGGER_TELEPORT_P_ACCEPT = 0,
	TRIGGER_TELEPORT_P_DENY,
	TRIGGER_TELEPORT_P_DROP,
	TRIGGER_TELEPORT_P_KILL
};

class CTriggerTeleport : public CBaseTrigger
{
public:
	virtual void KeyValue(KeyValueData *pkvd);// XDM3037
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);// XDM
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	EHANDLE m_hLastTargetSpot;
};


class CTriggerSave : public CBaseTrigger
{
public:
	//virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);// XDM
	virtual bool IsTriggered(const CBaseEntity *pEntity);
	//void EXPORT SaveTouch(CBaseEntity *pOther);
};


#define SF_ENDSECTION_USEONLY		0x0001

// IsTrigger() must be true!
class CTriggerEndSection : public CBaseTrigger
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	void EXPORT EndSectionTouch(CBaseEntity *pOther);
	void EXPORT EndSectionUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


class CTriggerGravity : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	void EXPORT GravityTouch(CBaseEntity *pOther);
};


class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};


class CTriggerChangeTarget : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_iszNewTarget;
};


#define SF_CAMERA_PLAYER_POSITION		1
#define SF_CAMERA_PLAYER_TARGET			2
#define SF_CAMERA_PLAYER_TAKECONTROL	4

// TODO: rewrite to support multiple players
class CTriggerCamera : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool ShouldBeSentTo(CBasePlayer *pClient);// XDM3035c
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void EXPORT FollowTarget(void);
	void Move(void);
	void Deactivate(void);

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	int m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int m_state;
	int	m_iszViewEntity;
	static TYPEDESCRIPTION m_SaveData[];
};


#define SF_BOUNCE_CUTOFF 16

class CTriggerBounce : public CBaseTrigger
{
public:
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);
};


class CTriggerSound : public CBaseToggle
{
public:
	virtual void KeyValue(KeyValueData* pkvd);
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual int	ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};



class CTriggerSecret : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

#endif // TRIGGERS_H
