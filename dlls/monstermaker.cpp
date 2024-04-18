#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"

// TESTME // UNTESTED // NEW: maker has now ability to copy entity specified by targetname

// spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
//#define SF_MONSTERMAKER_PVS			2 // UNDONE?
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip
#define SF_MONSTERMAKER_NOFADE		16 // Monsters don't fade out

// MonsterMaker - this entity creates monsters during the game
class CMonsterMaker : public CBaseMonster
{
public:
	virtual void KeyValue(KeyValueData* pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Activate(void);
	virtual void DeathNotice(CBaseEntity *pChild);// monster maker children use this to tell the monster maker that they have died.
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035: TODO: revisit
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual	bool IsMonster(void) const { return false; }
	virtual bool IsHuman(void)/* const*/ { return false; }
	virtual bool IsPushable(void) { return false; }
	virtual void ReportState(int printlevel);

	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT CyclicUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT MakerThink(void);

	void MakeEntity(void);
	CBaseEntity *MakeEntityByClassname(void);
	CBaseEntity *MakeEntityByCopy(void);

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMonsterClassname;// classname of the monster(s) that will be created.
	string_t m_iszSourceName;// XDM3038c: monster targetname to clone (very convinient for custom monsters)
	EHANDLE m_hCopySource;// XDM3038c

	int m_cNumMonsters;// max number of monsters this ent can create

	int m_cLiveChildren;// how many monsters made by this monster maker that are currently alive
	int	m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.

	float m_flGround;// z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child

	BOOL m_fActive;
	BOOL m_fFadeChildren;// should children fadeout?
};

LINK_ENTITY_TO_CLASS(monstermaker, CMonsterMaker);

TYPEDESCRIPTION	CMonsterMaker::m_SaveData[] =
{
	DEFINE_FIELD(CMonsterMaker, m_iszMonsterClassname, FIELD_STRING),
	DEFINE_FIELD(CMonsterMaker, m_iszSourceName, FIELD_STRING),// XDM3038c
	DEFINE_FIELD(CMonsterMaker, m_hCopySource, FIELD_EHANDLE),// XDM3038c
	DEFINE_FIELD(CMonsterMaker, m_cNumMonsters, FIELD_INTEGER),
	DEFINE_FIELD(CMonsterMaker, m_cLiveChildren, FIELD_INTEGER),
	DEFINE_FIELD(CMonsterMaker, m_flGround, FIELD_FLOAT),
	DEFINE_FIELD(CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER),
	DEFINE_FIELD(CMonsterMaker, m_fActive, FIELD_BOOLEAN),
	DEFINE_FIELD(CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CMonsterMaker, CBaseMonster);

void CMonsterMaker::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "monstercount"))
	{
		m_cNumMonsters = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_imaxlivechildren"))
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		m_iszMonsterClassname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sourcename"))// XDM3038c
	{
		m_iszSourceName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

void CMonsterMaker::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->takedamage = DAMAGE_NO;

	if (FStringNull(m_iszMonsterClassname) && FStringNull(m_iszSourceName))// XDM3037a
	{
		conprintf(1, "Error: %s[%d] %s at (%g %g %g) with nothing to spawn!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->flags = FL_KILLME;//Destroy();
		return;
	}

	m_cLiveChildren = 0;
	Precache();
	if (!FStringNull(pev->targetname))
	{
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_CYCLIC))
			SetUse(&CMonsterMaker::CyclicUse);// drop one monster each time we fire
		else
			SetUse(&CMonsterMaker::ToggleUse);// so can be turned on/off

		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_START_ON))
		{
			m_fActive = TRUE;
			SetThink(&CMonsterMaker::MakerThink);
		}
		else// wait to be activated.
		{
			m_fActive = FALSE;
			SetThink(&CBaseEntity::SUB_DoNothing);
		}
	}
	else// no targetname, just start.
	{
			SetNextThink(m_flDelay);
			m_fActive = TRUE;
			SetThink(&CMonsterMaker::MakerThink);
	}

	if (m_cNumMonsters == 1 || FBitSet(pev->spawnflags, SF_MONSTERMAKER_NOFADE))
		m_fFadeChildren = FALSE;
	else
		m_fFadeChildren = TRUE;

	m_flGround = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Precache resources required by child entity
//-----------------------------------------------------------------------------
void CMonsterMaker::Precache(void)
{
	//? CBaseMonster::Precache();
	if (FStringNull(m_iszSourceName) && !FStringNull(m_iszMonsterClassname))// XDM3037a
		UTIL_PrecacheOther(STRING(m_iszMonsterClassname), pev->model);
}

//-----------------------------------------------------------------------------
// Purpose: Find source to copy and make in virtual
//-----------------------------------------------------------------------------
void CMonsterMaker::Activate(void)
{
	CBaseMonster::Activate();
	if (!FStringNull(m_iszSourceName))
	{
		CBaseEntity *pEntity = UTIL_FindEntityByTargetname(NULL, STRING(m_iszSourceName), m_hActivator.Get()?(CBaseEntity *)m_hActivator:this);
		if (pEntity)
		{
			// best to place entity in some remote location pEntity->pev->origin = pev->origin;
			pEntity->MakeDormant();// unreliable?
			SetBits(pEntity->pev->effects, EF_NODRAW);
			pEntity->pev->solid = SOLID_NOT;
			pEntity->pev->movetype = MOVETYPE_NONE;
			pEntity->pev->takedamage = DAMAGE_NO;
			pEntity->pev->owner = edict();
			pEntity->pev->flags = FL_NOTARGET;// don't add, replace
			pEntity->SetTouchNull();// no touch
			pEntity->SetUseNull();
			pEntity->SetBlockedNull();
			pEntity->DontThink();// XDM3038a
			m_hCopySource = pEntity;
			conprintf(1, "%s[%d] %s found \"%s\": \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(m_iszSourceName), STRING(pEntity->pev->classname));
		}
		else
			conprintf(1, "%s[%d] %s Error: cannot find entity \"%s\" to copy!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(m_iszSourceName));
	}
	//else
	//	m_hCopySource = NULL;// ?
}

//-----------------------------------------------------------------------------
// Purpose: Make entity at my location
// WARNING: Some mappers use this for NON-MONSTERS!!!
//-----------------------------------------------------------------------------
void CMonsterMaker::MakeEntity(void)
{
	DBG_PRINTF("CMonsterMaker::MakeEntity()\n");
	if (m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren)
		return;// not allowed to make a new one yet. Too many live ones out right now.

	if (!m_flGround)
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin - Vector(0,0,2048), ignore_monsters, edict(), &tr);
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins(pev->origin); mins += Vector(-32, -32, 0);// mins.z = m_flGround; // XDM3038c 34 -> 32
	Vector maxs(pev->origin); maxs += Vector(32, 32, 0);// maxs.z = pev->origin.z;
	CBaseEntity *pList[2];
	size_t count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_CLIENT|FL_MONSTER);
	if (count)// found some obstacles
	{
		for (size_t i=0; i<count; ++i)
		{
			if (pList[i]->IsPlayer())// || pList[i]->pev->classname == m_iszMonsterClassname)
				return;
			if (pList[i]->pev->solid != SOLID_NOT && pList[i]->pev->solid != SOLID_TRIGGER)
				return;
			if (pList[i]->IsAlive())
				return;
		}
	}

	DBG_PRINTF("CMonsterMaker::MakeEntity(): creating entity\n");
	CBaseEntity *pEntity;
	if (!FStringNull(m_iszSourceName))
		pEntity = MakeEntityByCopy();
	else if (!FStringNull(m_iszMonsterClassname))
		pEntity = MakeEntityByClassname();
	else
		pEntity = NULL;

	if (pEntity == NULL)
	{
		DBG_PRINTF("CMonsterMaker::MakeEntity(): failed to make entity\n");
		return;
	}
	int sf = SF_NOREFERENCE|SF_NORESPAWN;
	pEntity->pev->owner = edict();
	pEntity->m_hOwner.Set(pEntity->pev->owner);// XDM3037: OWNER // NOTE: we could pass m_hActivator here, but that will break DeathNotice mechanism and the maker will stop
	pEntity->pev->origin = pev->origin;
	pEntity->pev->angles = pev->angles;
	pEntity->pev->velocity = pev->velocity;
	ClearBits(pEntity->pev->effects, EF_NODRAW);
	ClearBits(pEntity->pev->flags, FL_NOTARGET);
	// XDM3037a: allow me to override target's values
	if (pev->rendermode != 0)
		pEntity->pev->rendermode = pev->rendermode;

	pEntity->pev->renderamt = pev->renderamt;
	pEntity->pev->rendercolor = pev->rendercolor;

	if (pev->renderfx > 0)
		pEntity->pev->renderfx = pev->renderfx;

	if (pev->sequence > 0)
		pEntity->pev->sequence = pev->sequence;

	CBaseMonster *pMonster = pEntity->MyMonsterPointer();
	if (pMonster)// Some mappers use this for ammo (test map: c2a1, target: ammo3)
	{
		SetBits(sf, SF_MONSTER_FALL_TO_GROUND);
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_MONSTERCLIP))
			SetBits(sf, SF_MONSTER_HITMONSTERCLIP);
		if (m_fFadeChildren)
			SetBits(sf, SF_MONSTER_FADECORPSE);

		if (m_iClass > 0)
			pMonster->m_iClass = m_iClass;
		if (m_iTriggerCondition > 0)
			pMonster->m_iTriggerCondition = m_iTriggerCondition;
		if (!FStringNull(m_iszTriggerTarget))
			pMonster->m_iszTriggerTarget = m_iszTriggerTarget;
	}
	else
		conprintf(2, "CMonsterMaker::MakeEntity(): Warning: %s is not a monster!\n", STRING(pEntity->pev->classname));// XDM3038b: fix

	if (!FStringNull(pev->netname))// if I have a netname (overloaded), give the child entity that name as a targetname
		pEntity->pev->targetname = pev->netname;// XDM3038: moved before spawning/firing

	pEntity->pev->spawnflags = sf;
	// set in Spawn->Materialize() pEntity->m_vecSpawnSpot = pev->origin;
	//DBG_PRINTF("CMonsterMaker::MakeEntity(): entity prepared, spawning\n");
	if (DispatchSpawn(pEntity->edict()) < 0)
	{
		conprintf(1, "CMonsterMaker::MakeEntity(): Failed: DispatchSpawn() denied %s!\n", STRING(pEntity->pev->classname));
		return;
	}
#if defined(_DEBUG)
	if (pEntity->entindex() >= gpGlobals->maxEntities)//2048)// MAX_EDICTS is purely theoretical :(
	{
		SERVER_PRINT("MakeEntity WARNING! ENTITY INDEX TOO BIG!\n");
		conprintf(0, "WARNING: CMonsterMaker::MakeEntity(%s): created index >= 2048!\n", STRING(pEntity->pev->targetname));// XDM3038b: fix
	}
#endif
	//if (pMonster)// XDM3038: HACK to override "named monster wait bug" -- does not work because frame passes before InitThink() is called
	//	ChangeSchedule(GetScheduleOfType(SCHED_IDLE_STAND));

	// If I have a target, fire!
	if (!FStringNull(pev->target))
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	}

	m_cLiveChildren++;
	m_cNumMonsters--;

	if (m_cNumMonsters == 0)
	{
		DBG_PRINTF("CMonsterMaker::MakeEntity(): disabling\n");
		// Disable forever. Don't kill because it still gets death notices
		SetThinkNull();
		SetUseNull();
		DontThink();// XDM3038b
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make entity by cloning a source
// Output : CBaseEntity *
//-----------------------------------------------------------------------------
CBaseEntity *CMonsterMaker::MakeEntityByCopy(void)
{
	if (m_hCopySource.IsValid())
	{
		CBaseEntity *pEntity = CreateCopy(STRING(m_hCopySource->pev->classname), m_hCopySource, SF_NOREFERENCE|SF_NORESPAWN, false);
		if (pEntity == NULL)
			conprintf(1, "%s[%d] error: failed to copy \"%s\"!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z, STRING(m_iszSourceName));

		return pEntity;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Make entity by creating it from scratch
// Output : CBaseEntity *
//-----------------------------------------------------------------------------
CBaseEntity *CMonsterMaker::MakeEntityByClassname(void)
{
	/* int sf = SF_NOREFERENCE|SF_NORESPAWN;// XDM3037: reliably don't respawn
	CBaseEntity *pNew = Create(m_iszMonsterClassname, pev->origin, pev->angles, pev->velocity, edict(), sf);// XDM3037
	if (pNew == NULL)
	{
		conprintf(1, "CMonsterMaker::MakeEntity() error: NULL Ent in MonsterMaker!\n");
		return;
	}*/
	edict_t	*pent = NULL;
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		pent = CREATE_NAMED_ENTITY(STRING(m_iszMonsterClassname));

		if (!UTIL_IsValidEntity(pent))// XDM3038 //if (FNullEnt(pent))
		{
			conprintf(0, "CMonsterMaker::MakeEntityByClassname() error: unable to create \"%s\"!\n", STRING(m_iszMonsterClassname));
			return NULL;
		}

		CBaseEntity *pEntity = Instance(pent);
		if (pEntity == NULL)
		{
			conprintf(1, "CMonsterMaker::MakeEntityByClassname() error: NULL Ent in MonsterMaker!\n");
			return NULL;
		}

		if (!FStringNull(pev->model))
			pEntity->pev->model = pev->model;

		return pEntity;
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		fprintf(stderr, "CMonsterMaker::MakeEntity() CREATE_NAMED_ENTITY exception!\n");
		return NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Makes another entity each call
//-----------------------------------------------------------------------------
void CMonsterMaker::CyclicUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_hActivator = pActivator;
	MakeEntity();
}

//-----------------------------------------------------------------------------
// Purpose: Activate/deactivate
//-----------------------------------------------------------------------------
void CMonsterMaker::ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, (m_fActive == TRUE)))
		return;

	m_hActivator = pActivator;
	if (m_fActive)
	{
		m_fActive = FALSE;
		SetThinkNull();
	}
	else
	{
		m_fActive = TRUE;
		SetThink(&CMonsterMaker::MakerThink);
	}
	SetNextThink(0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: THINK: Creates a new entity each N seconds
//-----------------------------------------------------------------------------
void CMonsterMaker::MakerThink(void)
{
	MakeEntity();
	SetNextThink(m_flDelay);
}

//-----------------------------------------------------------------------------
// Purpose: Calles by children when they are removed
// Warning: What if this gets called twice?
//-----------------------------------------------------------------------------
void CMonsterMaker::DeathNotice(CBaseEntity *pChild)
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	if (pChild->m_hOwner == this)// XDM3038b
	{
		pChild->pev->owner = NULL;
		pChild->m_hOwner = NULL;
		--m_cLiveChildren;
	}
	/* STFU&GTFO!!!
	m_cLiveChildren--;

	if (!m_fFadeChildren)
	{
		pChild->pev->owner = NULL;
		pChild->m_hOwner = NULL;// XDM3037
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CMonsterMaker::ReportState(int printlevel)
{
	CBaseMonster::ReportState(printlevel);
	conprintf(printlevel, "active: %d, soruce: %s/%s, children: %d (%d max), left to create: %d, fade: %d\n",
		m_fActive?1:0, STRING(m_iszSourceName), STRING(m_iszMonsterClassname), m_cLiveChildren, m_iMaxLiveChildren, m_cNumMonsters, m_fFadeChildren?1:0);
}
