#ifndef MAPRULES_H
#define MAPRULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#define SUPERDEFAULT_MP_START			"info_player_deathmatch"
#define SUPERDEFAULT_SP_START			"info_player_start"

#define SPAWNSPOT_RANDOMIZE				0x0001// 50% chance to use this entity (if exists)
#define SPAWNSPOT_CLEARAREA				0x0002// disintegrate everybody in the way (on second try)
#define SPAWNSPOT_DONTSAVE				0x0004// don't save into g_pLastSpawn

typedef struct spawnspot_s
{
	const char *classname;
	uint32 flags;
} spawnspot_t;


// Common base class
class CRuleEntity : public CBaseDelay// XDM3037
{
public:
	virtual void Spawn(void);
	bool TeamMatch(const CBaseEntity *pActivator);

//?protected:
	bool CanFireForActivator(CBaseEntity *pActivator, bool bCheckTeam);
};

class CRulePointEntity : public CRuleEntity
{
public:
	virtual void Spawn(void);
};

class CRuleBrushEntity : public CRuleEntity
{
public:
	virtual void Spawn(void);
};


#define SF_GAMESCORE_NEGATIVE			0x0001
#define SF_GAMESCORE_TEAM				0x0002
#define SF_GAMESCORE_EVERYONE			0x0004// XDM3038a

class CGameScore : public CRulePointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


class CGameEnd : public CRulePointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


#define SF_ENVTEXT_ALLPLAYERS			0x0001

class CGameText : public CRulePointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	hudtextparms_t	m_textParms;
};


#define SF_TEAMMASTER_FIREONCE			0x0001
#define SF_TEAMMASTER_ANYTEAM			0x0002

class CGameTeamMaster : public CRulePointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int ObjectCaps(void) { return CRulePointEntity::ObjectCaps() | FCAP_MASTER; }

	virtual bool IsTriggered(const CBaseEntity *pActivator);
private:
	USE_TYPE	triggerType;
};


#define SF_TEAMSET_FIREONCE				0x0001
#define SF_TEAMSET_CLEARTEAM			0x0002
#define SF_TEAMSET_SETCOLOR				0x0004

class CGameTeamSet : public CRulePointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


class CGamePlayerZone : public CRuleBrushEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	string_t	m_iszInTarget;
	string_t	m_iszOutTarget;
	string_t	m_iszInCount;
	string_t	m_iszOutCount;
};


#define SF_PKILL_FIREONCE				0x0001

class CGamePlayerHurt : public CRulePointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


#define SF_GAMECOUNT_FIREONCE			0x0001
#define SF_GAMECOUNT_RESET				0x0002

class CGameCounter : public CRulePointEntity
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	inline void CountUp(void) { pev->frags++; }
	inline void CountDown(void) { pev->frags--; }
	inline void ResetCount(void) { pev->frags = pev->dmg; }
	inline int CountValue(void) { return pev->frags; }
	inline int LimitValue(void) { return pev->health; }

private:
	inline void SetCountValue(int value) { pev->frags = value; }
	inline void SetInitialValue(int value) { pev->dmg = value; }
};


#define SF_GAMECOUNTSET_FIREONCE		0x0001

class CGameCounterSet : public CRulePointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


#define SF_PLAYEREQUIP_USEONLY			0x0001
#define SF_PLAYEREQUIP_ADDTODEFAULT		0x0002// XDM3037a

#define MAX_EQUIP		PLAYER_INVENTORY_SIZE

class CGamePlayerEquip : public CRulePointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Precache(void);// XDM3038a
	virtual void Touch(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
	void EquipPlayer(CBaseEntity *pPlayer);

	string_t	m_weaponNames[MAX_EQUIP];
	uint32		m_weaponCount[MAX_EQUIP];
};


#define SF_PTEAM_FIREONCE				0x0001
#define SF_PTEAM_KILL    				0x0002
#define SF_PTEAM_GIB     				0x0004

class CGamePlayerTeam : public CRulePointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
	TEAM_ID GetTargetTeam(const char *pszTargetName);
};


class CGameRestart : public CRulePointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


// XDM3038a
#define SF_STRIPWEAPONS_FIREONCE		0x0001
#define SF_STRIPWEAPONS_EVERYONE		0x0002
#define SF_STRIPWEAPONS_SUIT			0x0004// XDM3038c
#define SF_STRIPWEAPONS_QUESTITEMS		0x0008

class CStripWeapons : public CRulePointEntity// XDM3038a: new base class
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};


class CRevertSaved : public CPointEntity
{
public:
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT MessageThink(void);
	void EXPORT LoadThink(void);
	inline float Duration(void) { return pev->dmg_take; }
	inline float HoldTime(void) { return pev->dmg_save; }
	inline float MessageTime(void) { return m_messageTime; }
	inline float LoadTime(void) { return m_loadTime; }
	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};


#define INFOINTERMISSION_MAX_SEARCH_ATTEMPTS	4
// these are stored in pev->watertype
#define INFOINTERMISSION_TYPE_ATSTART			1
#define INFOINTERMISSION_TYPE_ATEND				2

class CInfoIntermission : public CPointEntity
{
	virtual void Spawn(void);
	virtual void Think(void);
	virtual void Activate(void);
};




// XDM3038a
enum playerstart_types_e
{
	PLAYERSTART_TYPE_RANDOM = 0,// this one does not require landmark
	PLAYERSTART_TYPE_MAPSTART,
	PLAYERSTART_TYPE_MAPEND,
	PLAYERSTART_TYPE_MAPJUNCTION,// as many as designer may need
	//PLAYERSTART_TYPE_MAPJUNCTION2, ... (no need to define)
};

#define SF_PLAYERSTART_ALLOWSTART		0x0002// allow start when playing new map (not changing levels)

class CBasePlayerStart : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual int	ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }// XDM3035c: fixfix
	virtual bool IsTriggered(const CBaseEntity *pActivator);
};



uint32 BuildChangeList(LEVELLIST *pLevelList, uint32 maxList);
bool AddTransitionToList(LEVELLIST *pLevelList, uint32 listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark);
bool InTransitionVolume(CBaseEntity *pEntity, const char *pVolumeName);
edict_t *FindLandmark(const char *pLandmarkName);
bool ChangeLevel(const char *pMapName, const char *pLandmarkName, CBaseEntity *pActivator, string_t iszChangeTarget, float fChangeTargetDelay);


bool SpawnPointCheckObstacles(CBaseEntity *pSpawnEntity, const Vector &vecSpawnOrigin, bool bClear, bool bKill);
bool SpawnPointValidate(CBaseEntity *pSpawnEntity, CBaseEntity *pSpot, bool clear);
void SpawnPointInitialize(void);

CBaseEntity *SpawnPointEntSelect(CBaseEntity *pPlayer, bool bSpectator);
//CBaseEntity *SpawnPointEntSelectSpectator(CBaseEntity *pPlayer);

bool VerifyGameRulesPolicy(const char *pPolicyString);


// Related global variables
extern spawnspot_t		*g_pSpotList;
extern CBaseEntity		*g_pLastSpawn[];
extern size_t			g_iSpawnPointTypePlayers;
extern size_t			g_iSpawnPointTypeSpectators;
extern size_t			g_iMapHasLandmarkedSpawnSpots;

#endif // MAPRULES_H
