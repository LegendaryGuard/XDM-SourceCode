//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
// Spirit compatibility mostly. Needs to be revisited.
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "decals.h"
#include "player.h"
#include "globals.h"
#include "shared_resources.h"


// Body queue class here.... It's really just CBaseEntity
class CCorpse : public CBaseEntity
{
	virtual int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(bodyque, CCorpse);



//=================================================================
// Used by SHL maps
// env_model: like env_sprite, except you can specify a sequence.
//=================================================================
#define SF_ENVMODEL_OFF				1
#define SF_ENVMODEL_DROPTOFLOOR		2
#define SF_ENVMODEL_SOLID			4

class CEnvModel : public CBaseAnimating
{
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual STATE GetState(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void SetSequence(void);

	string_t m_iszSequence_On;
	string_t m_iszSequence_Off;
	int m_iAction_On;
	int m_iAction_Off;
};

TYPEDESCRIPTION CEnvModel::m_SaveData[] =
{
	DEFINE_FIELD(CEnvModel, m_iszSequence_On, FIELD_STRING),
	DEFINE_FIELD(CEnvModel, m_iszSequence_Off, FIELD_STRING),
	DEFINE_FIELD(CEnvModel, m_iAction_On, FIELD_INTEGER),
	DEFINE_FIELD(CEnvModel, m_iAction_Off, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CEnvModel, CBaseAnimating);

LINK_ENTITY_TO_CLASS(env_model, CEnvModel);

void CEnvModel::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszSequence_On"))
	{
		m_iszSequence_On = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSequence_Off"))
	{
		m_iszSequence_Off = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_On"))
	{
		m_iAction_On = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_Off"))
	{
		m_iAction_Off = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CEnvModel::Spawn(void)
{
	Precache();
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetOrigin(this, pev->origin);

	if (FBitSet(pev->spawnflags, SF_ENVMODEL_SOLID))
	{
		pev->solid = SOLID_SLIDEBOX;
		UTIL_SetSize(this, Vector(-10, -10, -10), Vector(10, 10, 10));// wtf??
	}

	if (FBitSet(pev->spawnflags, SF_ENVMODEL_DROPTOFLOOR))
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR(edict());
	}

	SetBoneController(0, 0);
	SetBoneController(1, 0);
	SetSequence();
	SetNextThink(0.1);
}

void CEnvModel::Precache(void)
{
	if (FStringNull(pev->model))
	{
		pev->model = MAKE_STRING(g_szDefaultStudioModel);
		//pev->modelindex = g_iModelIndexTestSphere;
	}
	PRECACHE_MODEL(STRINGV(pev->model));
}

STATE CEnvModel::GetState(void)
{
	if (FBitSet(pev->spawnflags, SF_ENVMODEL_OFF))
		return STATE_OFF;
	else
		return STATE_ON;
}

void CEnvModel::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (ShouldToggle(useType, !FBitSet(pev->spawnflags, SF_ENVMODEL_OFF)))
	{
		if (FBitSet(pev->spawnflags, SF_ENVMODEL_OFF))
			ClearBits(pev->spawnflags, SF_ENVMODEL_OFF);
		else
			SetBits(pev->spawnflags, SF_ENVMODEL_OFF);

		SetSequence();
		SetNextThink(0.1);
	}
}

void CEnvModel::Think(void)
{
	//conprintf(1, "env_model Think fr=%f\n", pev->framerate);
	StudioFrameAdvance(); // set m_fSequenceFinished if necessary

	//if (m_fSequenceLoops)
	//{
	//	SetNextThink(1E6);
	//	return; // our work here is done.
	//}
	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		int iTemp;
		if (FBitSet(pev->spawnflags, SF_ENVMODEL_OFF))
			iTemp = m_iAction_Off;
		else
			iTemp = m_iAction_On;

		switch (iTemp)
		{
		//case 1: // loop
			//pev->animtime = gpGlobals->time;
			//m_fSequenceFinished = FALSE;
			//m_flLastEventCheck = gpGlobals->time;
			//pev->frame = 0;
			//break;
		case 2: // change state
			if (FBitSet(pev->spawnflags, SF_ENVMODEL_OFF))
				ClearBits(pev->spawnflags, SF_ENVMODEL_OFF);
			else
				SetBits(pev->spawnflags, SF_ENVMODEL_OFF);

			SetSequence();
			break;
		default: //remain frozen
			return;
		}
	}
	CBaseEntity::Think();// XDM3037: allow SetThink()
	SetNextThink(0.1);
}

void CEnvModel::SetSequence(void)
{
	int iszSeq;
	if (FBitSet(pev->spawnflags, SF_ENVMODEL_OFF))
		iszSeq = m_iszSequence_Off;
	else
		iszSeq = m_iszSequence_On;

	if (!iszSeq)
		return;

	pev->sequence = LookupSequence(STRING(iszSeq));
	if (pev->sequence == -1)
	{
		if (!FStringNull(pev->targetname))
			conprintf(1, "env_model %s: unknown sequence \"%s\"\n", STRING(pev->targetname), STRING(iszSeq));
		else
			conprintf(1, "env_model: unknown sequence \"%s\"\n", STRING(iszSeq));

		pev->sequence = 0;
	}

	pev->frame = 0;
	ResetSequenceInfo();

	if (FBitSet(pev->spawnflags, SF_ENVMODEL_OFF))
	{
		if (m_iAction_Off == 1)
			m_fSequenceLoops = 1;
		else
			m_fSequenceLoops = 0;
	}
	else
	{
		if (m_iAction_On == 1)
			m_fSequenceLoops = 1;
		else
			m_fSequenceLoops = 0;
	}
}




//=========================================================
// SHL - Beam Trail effect
//=========================================================
#define SF_BEAMTRAIL_OFF 1

class CEnvBeamTrail : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual STATE GetState(void);
	void EXPORT StartTrailThink(void);
	void Affect(CBaseEntity *pTarget, USE_TYPE useType);
protected:
	int		m_iSprite;	// Don't save, precache
};

void CEnvBeamTrail::Precache(void)
{
	//if (pev->target)
	//	PRECACHE_MODEL("sprites/null.spr");
	if (pev->netname)
		m_iSprite = PRECACHE_MODEL(STRINGV(pev->netname));
}

LINK_ENTITY_TO_CLASS(env_beamtrail, CEnvBeamTrail);

STATE CEnvBeamTrail::GetState(void)
{
	if (FBitSet(pev->spawnflags, SF_BEAMTRAIL_OFF))
		return STATE_OFF;
	else
		return STATE_ON;
}

void CEnvBeamTrail::StartTrailThink(void)
{
	SetBits(pev->spawnflags, SF_BEAMTRAIL_OFF); // fake turning off, so the Use turns it on properly
	Use(this, this, USE_ON, 0);
}

void CEnvBeamTrail::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (!FStringNull(pev->target))
	{
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target)/*, pActivator */);
		while (pTarget)
		{
			Affect(pTarget, useType);
			pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(pev->target)/*, pActivator */);
		}
	}
	else
	{
		if (!ShouldToggle(useType, GetState() != STATE_OFF))// XDM3037: cast from STATE to bool
			return;
		Affect(this, useType);
	}

	if (useType == USE_ON)
		ClearBits(pev->spawnflags, SF_BEAMTRAIL_OFF);
	else if (useType == USE_OFF)
		SetBits(pev->spawnflags, SF_BEAMTRAIL_OFF);
	else if (useType == USE_TOGGLE)
	{
		if (FBitSet(pev->spawnflags, SF_BEAMTRAIL_OFF))
			ClearBits(pev->spawnflags, SF_BEAMTRAIL_OFF);
		else
			SetBits(pev->spawnflags, SF_BEAMTRAIL_OFF);
	}
}

void CEnvBeamTrail::Affect(CBaseEntity *pTarget, USE_TYPE useType)
{
	if (pTarget == NULL)
		return;

	if (useType == USE_ON || FBitSet(pev->spawnflags, SF_BEAMTRAIL_OFF))
	{
		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(TE_BEAMFOLLOW);
			WRITE_SHORT(pTarget->entindex());// entity
			WRITE_SHORT(m_iSprite);// model
			WRITE_BYTE(pev->health*10);// life
			WRITE_BYTE(pev->armorvalue);// width
			WRITE_BYTE(pev->rendercolor.x);// r, g, b
			WRITE_BYTE(pev->rendercolor.y);
			WRITE_BYTE(pev->rendercolor.z);
			WRITE_BYTE(pev->renderamt);// brightness
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(TE_KILLBEAM);
			WRITE_SHORT(pTarget->entindex());
		MESSAGE_END();
	}
}

void CEnvBeamTrail::Spawn(void)
{
	CPointEntity::Spawn();
	if (!FBitSet(pev->spawnflags, SF_BEAMTRAIL_OFF))
	{
		SetThink(&CEnvBeamTrail::StartTrailThink);
		SetNextThink(0);
	}
}




//=========================================================
// SHL Decal effect
//=========================================================
class CEnvDecal : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(env_decal, CEnvDecal);

void CEnvDecal::Spawn(void)
{
	//CPointEntity::Spawn();
	if (pev->impulse == 0)
	{
		pev->skin = DECAL_INDEX(STRING(pev->noise));
		if (pev->skin == 0)
			conprintf(1, "env_decal \"%s\" can't find texture \"%s\"\n", STRING(pev->targetname), STRING(pev->noise));// XDM3038: fixed
	}
}

void CEnvDecal::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	int iTexture = 0;
	switch (pev->impulse)
	{
		case 1: iTexture = DECAL_GUNSHOT1	+	RANDOM_LONG(0,4); break;
		case 2: iTexture = DECAL_BLOOD1		+	RANDOM_LONG(0,5); break;
		case 3: iTexture = DECAL_YBLOOD1	+	RANDOM_LONG(0,5); break;
		case 4: iTexture = DECAL_GLASSBREAK1+	RANDOM_LONG(0,2); break;
		case 5: iTexture = DECAL_BIGSHOT1	+	RANDOM_LONG(0,4); break;
		case 6: iTexture = DECAL_SCORCH1	+	RANDOM_LONG(0,2); break;
		case 7: iTexture = DECAL_SPIT1		+	RANDOM_LONG(0,1); break;
		case 8: iTexture = DECAL_LARGESHOT1	+	RANDOM_LONG(0,4); break;
	}

	if (pev->impulse)
		iTexture = g_Decals[iTexture].index;
	else
		iTexture = pev->skin; // custom texture

	Vector vecPos = pev->origin;
	UTIL_MakeVectors(pev->angles);
	Vector vecOffs(gpGlobals->v_forward.Normalize() * 4000);

	TraceResult trace;
	UTIL_TraceLine(vecPos, vecPos+vecOffs, ignore_monsters, NULL, &trace);

	if (trace.flFraction == 1.0)
		return; // didn't hit anything, oh well

	int entityIndex = ENTINDEX(trace.pHit);

	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BSPDECAL);
		WRITE_COORD(trace.vecEndPos.x);
		WRITE_COORD(trace.vecEndPos.y);
		WRITE_COORD(trace.vecEndPos.z);
		WRITE_SHORT(iTexture);
		WRITE_SHORT(entityIndex);
		if (entityIndex)
			WRITE_SHORT((int)VARS(trace.pHit)->modelindex);
	MESSAGE_END();
}


//=========================================================
// Old HL decals
//=========================================================
#define SF_DECAL_NOTINDEATHMATCH 2048

class CDecal : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
	void EXPORT StaticDecal(void);
	void EXPORT TriggerDecal(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	TraceResult trace;
};

LINK_ENTITY_TO_CLASS(infodecal, CDecal);

void CDecal::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->skin = DECAL_INDEX(pkvd->szValue);
		if (pev->skin < 0)
			conprintf(1, "Can't find decal %s\n", pkvd->szValue);

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

// UNDONE:  These won't get sent to joining players in multi-player
// XDM3035: used new update mechanism on these
void CDecal::Spawn(void)
{
	if (pev->skin < 0 || (gpGlobals->deathmatch && FBitSet(pev->spawnflags, SF_DECAL_NOTINDEATHMATCH)))
	{
		REMOVE_ENTITY(edict());
		return;
	}

	pev->effects = EF_NODRAW;// XDM3038
	memset(&trace, NULL, sizeof(TraceResult));

	if (FStringNull(pev->targetname))
	{
		pev->impulse = 1;
		SetThink(&CDecal::StaticDecal);
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		SetNextThink(0);
	}
	else
	{
		pev->impulse = 0;
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		//SetThink(&CBaseEntity::SUB_DoNothing);
		SetThinkNull();// XDM: TESTME
		SetUse(&CDecal::TriggerDecal);
	}
}

// this is set up as a USE function for infodecals that have targetnames, so that the decal doesn't get applied until it is fired. (usually by a scripted sequence)
void CDecal::TriggerDecal(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(TriggerDecal);
	UTIL_TraceLine(pev->origin - Vector(5,5,5), pev->origin + Vector(5,5,5),  ignore_monsters, edict(), &trace);
	pev->impulse = 1;
	SendClientData(NULL, MSG_BROADCAST, SCD_SELFUPDATE);
	SetThinkNull();// XDM: TESTME
	//SetThink(&CBaseEntity::SUB_Remove);
	//SetNextThink(0.1);
}

// XDM: does this affect joining player too?
void CDecal::StaticDecal(void)
{
	int entityIndex, modelIndex;
	UTIL_TraceLine(pev->origin - Vector(5,5,5), pev->origin + Vector(5,5,5), ignore_monsters, dont_ignore_glass, edict(), &trace);// XDM3037a
	entityIndex = ENTINDEX(trace.pHit);

	if (entityIndex)
		modelIndex = (int)VARS(trace.pHit)->modelindex;
	else
		modelIndex = 0;

	STATIC_DECAL(pev->origin, (int)pev->skin, entityIndex, modelIndex);
	SUB_Remove();
}

int CDecal::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (msgtype == MSG_ONE && (pev->impulse == 0 || pClient == NULL))
		return 0;
	if (trace.pHit == NULL)
	{
		DBG_PRINT_ENT("trace.pHit == NULL");
		return 0;
	}
	/*if (IsRemoving())// XDM3037
	{
		???
	}*/
	//MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
	MESSAGE_BEGIN(msgtype, svc_temp_entity, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_BYTE(TE_BSPDECAL);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT((int)pev->skin);
		int entityIndex = (short)ENTINDEX(trace.pHit);
		WRITE_SHORT(entityIndex);
		if (entityIndex)
			WRITE_SHORT((int)VARS(trace.pHit)->modelindex);
	MESSAGE_END();
	return 1;
}



//-----------------------------------------------------------------------------
// Purpose: script-based particle system
// UNDONE: trigger_changetarget will make client systems inoperable!
//-----------------------------------------------------------------------------
#define SF_ENVRS_START_ON			0x0001
#define SF_ENVRS_PORTIONAL			0x0002
#define SF_ENVRS_TARGETACTIVATOR	0x0004
#define SF_ENVRS_USEATTACHMENT		0x0008
#define SF_ENVRS_ADDPHYSICS			0x0010
#define SF_ENVRS_ADDGRAVITY			0x0020

class CEnvRenderSystem : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);

	// don't save these as they are constant during the game
	char m_szCustomKeyName[ENVRS_MAX_CUSTOMKEYS][ENVRS_MAX_KEYLEN];
	char m_szCustomKeyValue[ENVRS_MAX_CUSTOMKEYS][ENVRS_MAX_KEYLEN];
	int m_iNumCustomKeys;
	int m_iFxLevel;
};

LINK_ENTITY_TO_CLASS(env_rendersystem, CEnvRenderSystem);

void CEnvRenderSystem::KeyValue(KeyValueData *pkvd)
{
	CPointEntity::KeyValue(pkvd);
	if (pkvd->fHandled == TRUE)// XDM3038b: try to catch changes of "target" and other fields
	{
		if (g_ServerActive)// this is a dynamic in-game change
		{
			if (FStrEq(pkvd->szKeyName, "target") || FStrEq(pkvd->szKeyName, "button"))
				SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
		}
	}
	else if (FStrEq(pkvd->szKeyName, "iFxLevel"))// Important: must match the RS script key name!
	{
		m_iFxLevel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		pkvd->fHandled = TRUE;
		if (m_iNumCustomKeys < ENVRS_MAX_CUSTOMKEYS)
		{
			strncpy(m_szCustomKeyName[m_iNumCustomKeys], pkvd->szKeyName, ENVRS_MAX_KEYLEN);
			strncpy(m_szCustomKeyValue[m_iNumCustomKeys], pkvd->szValue, ENVRS_MAX_KEYLEN);
			++m_iNumCustomKeys;
		}
		else
			conprintf(1, "%s[%d] \"%s\" has too many custom keys! \"%s\" is ignored.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pkvd->szKeyName);
	}
}

void CEnvRenderSystem::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	//UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);
	CPointEntity::Spawn();

	if (FBitSet(pev->spawnflags, SF_ENVRS_START_ON))
		pev->impulse = 1;
	else
		pev->impulse = 0;

	pev->dmg_inflictor = NULL;
}

void CEnvRenderSystem::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (FBitSet(pev->spawnflags, SF_ENVRS_PORTIONAL))
	{
		if (FBitSet(pev->spawnflags, SF_ENVRS_TARGETACTIVATOR))
		{
			if (pActivator)
				pev->dmg_inflictor = pActivator->edict();
			else
				pev->dmg_inflictor = NULL;
		}
		pev->impulse++;// flag as enabled, also count
		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
		return;
	}

	if (!ShouldToggle(useType, pev->impulse > 0))
		return;

	if (pev->impulse == 0)
	{
		pev->impulse = 1;
		if (FBitSet(pev->spawnflags, SF_ENVRS_TARGETACTIVATOR))
		{
			if (pActivator)
				pev->dmg_inflictor = pActivator->edict();
			else
				pev->dmg_inflictor = NULL;
		}
	}
	else
		pev->impulse = 0;

	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

int CEnvRenderSystem::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (!pClient && msgtype == MSG_ONE)// a client has connected and needs an update
		return 0;

	//unsigned short flags = 0;// temporary for now
	int command = MSGRS_ENABLE;
	int rsflags = 0;

	if (IsRemoving())// XDM3037
	{
		command = MSGRS_DESTROY;
	}
	else
	{
		if (pev->impulse == 0)// not enabled
		{
			if (sendcase == SCD_CLIENTCONNECT)// a client has connected and needs an update
				return 0;

			command = MSGRS_DISABLE;// disable this RS on all client machines
		}
	}

	if (command == MSGRS_ENABLE)
	{
		if (FBitSet(pev->flags, FL_DRAW_ALWAYS))// flag this one to be drawn always
		{
			rsflags |= RENDERSYSTEM_FLAG_DRAWALWAYS;
			if (msgtype == MSG_PVS)
				msgtype = MSG_ALL;
		}
		if (FBitSet(pev->spawnflags, SF_ENVRS_PORTIONAL))// one-shot system: always create new instance
			command = MSGRS_CREATE;
	}

	if (FBitSet(pev->spawnflags, SF_ENVRS_ADDPHYSICS))
		rsflags |= RENDERSYSTEM_FLAG_ADDPHYSICS;
	if (FBitSet(pev->spawnflags, SF_ENVRS_ADDGRAVITY))
		rsflags |= RENDERSYSTEM_FLAG_ADDGRAVITY;

	CBaseEntity *pTarget = NULL;
	if (pev->dmg_inflictor && FBitSet(pev->spawnflags, SF_ENVRS_TARGETACTIVATOR))
		pTarget = CBaseEntity::Instance(pev->dmg_inflictor);
	else
		pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target), this);

#if defined (_DEBUG)
	DBG_PRINTF("sv: sending RenderSys \"%s\": e %d, target %s[%d], tgtmodel %d, cmd %d\n", STRING(pev->message), entindex(), pTarget?STRING(pev->target):"self", pTarget?pTarget->entindex():0, pTarget?pTarget->pev->modelindex:0, command);
#endif
	MESSAGE_BEGIN(msgtype, gmsgRenderSys, pev->origin, (pClient == NULL)?NULL:pClient->edict());
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(entindex());// emitter
		WRITE_SHORT(pTarget?pTarget->entindex():0);// follow entity
		WRITE_SHORT(pTarget?pTarget->pev->modelindex:0);// follow model (in case it's not sent to client e.g. invisible)
		WRITE_BYTE(FBitSet(pev->spawnflags, SF_ENVRS_USEATTACHMENT)?pev->button:MAXSTUDIOATTACHMENTS);// attachment
		WRITE_BYTE(m_iFxLevel);
		//WRITE_SHORT(ttl*10.0f);
		WRITE_BYTE(command);// state to achieve
		WRITE_SHORT(rsflags);
		WRITE_STRING(STRING(pev->message));
		if (m_iNumCustomKeys > 0 && command != MSGRS_DESTROY)// beware MAX_USER_MSG_DATA!
		{
			char buffer[ENVRS_MAX_CUSTOMKEYS*(ENVRS_MAX_KEYLEN*2+2)];// names + values + separators
			buffer[0] = 0;
			for (int i=0; i<m_iNumCustomKeys; ++i)
			{
				if (i > 0) strcat(buffer, ";");
				strcat(buffer, m_szCustomKeyName[i]);
				strcat(buffer, ":");
				strcat(buffer, m_szCustomKeyValue[i]);
			}
#if defined (_DEBUG)
			ASSERT(strlen(buffer) <= MAX_USER_MSG_DATA);
#endif
			WRITE_STRING(buffer);// "key:value;..."
			DBG_PRINTF("%s[%d] \"%s\" custom data: \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->targetname), buffer);
		}
	MESSAGE_END();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: HUD icon. Either toggle or hold-and-fade.
// KV:
// message - sprite name in hud.txt
// rendercolor - color
// dmg_save - holdtime
// dmg_take - fadetime
//-----------------------------------------------------------------------------
#define SF_HUDICON_ALL		0x0002

class CEnvHudIcon : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
};

LINK_ENTITY_TO_CLASS(hud_icon, CEnvHudIcon);

void CEnvHudIcon::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	CPointEntity::Spawn();
}

void CEnvHudIcon::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (pev->dmg_save == 0 && pev->dmg_take == 0)// toggle
	{
		if (!ShouldToggle(useType, pev->impulse > 0))
			return;

		if (pev->impulse > 0)
			pev->impulse = 0;
		else
			pev->impulse = 1;
	}

	if (FBitSet(pev->spawnflags, SF_HUDICON_ALL))
	{
		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);//MSG_BROADCAST?
	}
	else if (pCaller->IsPlayer())
	{
		SendClientData((CBasePlayer *)pCaller, MSG_ONE, SCD_SELFUPDATE);
	}
}

int CEnvHudIcon::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (!pClient && msgtype == MSG_ONE)// a client has connected and needs an update
		return 0;

	//allow client sprite to persist?
	//if (IsRemoving())
	//	pev->impulse = 0;

	MESSAGE_BEGIN(msgtype, gmsgStatusIcon, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_BYTE(pev->impulse);// enable
		WRITE_STRING(STRING(pev->message));// sprite name to display
		WRITE_BYTE((int)pev->rendercolor.x);
		WRITE_BYTE((int)pev->rendercolor.y);
		WRITE_BYTE((int)pev->rendercolor.z);
		WRITE_SHORT(pev->dmg_save);// holdtime
		WRITE_SHORT(pev->dmg_take);// fadetime
	MESSAGE_END();
	return 1;
}



//-----------------------------------------------------------------------------
// Purpose: Displays a countdown HUD timer and fires its target.
// KV:
// target - fire when ring
// health - time
// message - optional title
//-----------------------------------------------------------------------------
#define SF_HUDTIMER_ALL		0x0002

class CEnvHudTimer : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Think(void);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
};

LINK_ENTITY_TO_CLASS(hud_timer, CEnvHudTimer);

void CEnvHudTimer::Spawn(void)
{
	if (pev->health <= 0)
	{
		pev->flags = FL_KILLME;//Destroy();
		return;
	}
	pev->teleport_time = 0.0f;
	CBaseDelay::Spawn();
	SetNextThink(0);
}

void CEnvHudTimer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (!ShouldToggle(useType, pev->impulse > 0))
		return;

	m_hActivator = pActivator;
	if (pev->impulse > 0)// was ON, turn OFF
	{
		pev->impulse = 0;
		pev->teleport_time = 0;
	}
	else// was OFF, turn ON
	{
		pev->impulse = 1;
		pev->teleport_time = gpGlobals->time + pev->health;
		SetNextThink(1.0);
	}

	if (FBitSet(pev->spawnflags, SF_HUDTIMER_ALL))
	{
		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
	}
	else if (pCaller->IsPlayer())
	{
		SendClientData((CBasePlayer *)pCaller, MSG_ONE, SCD_SELFUPDATE);
	}
}

void CEnvHudTimer::Think(void)
{
	if (pev->impulse > 0 && pev->teleport_time > 0)
	{
		if (gpGlobals->time >= pev->teleport_time)
		{
			SUB_UseTargets(m_hActivator, USE_TOGGLE, 0.0);
			pev->impulse = 0;
			pev->teleport_time = 0;
			SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
			DontThink();
		}
		else
			SetNextThink(1.0);
	}
}

int CEnvHudTimer::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (!pClient && msgtype == MSG_ONE)// a client has connected and needs an update
		return 0;

	//allow client sprite to persist?
	//if (IsRemoving())
	//	pev->impulse = 0;

	MESSAGE_BEGIN(msgtype, gmsgHUDTimer, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		if (pev->impulse > 0)// enabled
			WRITE_SHORT(pev->teleport_time - gpGlobals->time);// time (some players may join long after timer fas started!!)
		else
			WRITE_SHORT(0);

		WRITE_STRING(STRING(pev->message));// sprite name to display
	MESSAGE_END();
	return 1;
}
