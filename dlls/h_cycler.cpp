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
/*

===== h_cycler.cpp ========================================================

  The Halflife Cycler Monsters

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "shared_resources.h"
#include "globals.h"


#define SF_CYCLER_NOTSOLID 0x0001// XDM

class CCycler : public CBaseMonster
{
public:
	void GenericCyclerSpawn(const char *szModel, const Vector &vecMin, const Vector &vecMax);
	virtual int	ObjectCaps(void) { return (CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE); }
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool IsAlive(void) const { return false; }// Don't treat as a live target
	virtual bool IsMonster(void) const { return false; }// XDM: ?
	virtual bool IsPushable(void) { return false; }// XDM
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int			m_animate;
};

TYPEDESCRIPTION	CCycler::m_SaveData[] =
{
	DEFINE_FIELD( CCycler, m_animate, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCycler, CBaseMonster );

// Cycler member functions
void CCycler::GenericCyclerSpawn(const char *szModel, const Vector &vecMin, const Vector &vecMax)
{
	if (!szModel || !*szModel)
	{
		conprintf(1, "Design error: %s[%d] %s at %g %g %g missing modelname!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);// XDM: targetname
		pev->flags = FL_KILLME;
		REMOVE_ENTITY(edict());//Destroy();?
		return;
	}

	if (!UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))// XDM3037: hardcore crash prevention!
	{
		conprintf(0, "ERROR: %s[%d] %s: \"%s\" is not a studio model!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->model));
		if (UTIL_FileExtensionIs(STRING(pev->model), ".spr"))
			CreateCopy("cycler_sprite", this, pev->spawnflags);

		pev->flags = FL_KILLME;//Destroy();
		return;
	}

	// Why? If we REALLY need this, use SetClassName instead	pev->classname = MAKE_STRING("cycler");// XDM3038: 20141121
	PRECACHE_MODEL((char *)szModel);
	SET_MODEL(edict(),	szModel);
	CCycler::Spawn();
	UTIL_SetSize(this, vecMin, vecMax);
}

void CCycler::Spawn(void)
{
	UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);// XDM3035a: IMPORTANT!
	if (FBitSet(pev->spawnflags, SF_CYCLER_NOTSOLID))// XDM
		pev->solid		= SOLID_NOT;
	else
		pev->solid		= SOLID_SLIDEBOX;

	pev->movetype		= MOVETYPE_NONE;

	if ((g_pGameRules == NULL || !g_pGameRules->IsMultiplayer()) && g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)// XDM
		pev->takedamage	= DAMAGE_YES;
	else
		pev->takedamage	= DAMAGE_NO;

	pev->health			= 65535;// no cycler should die
	pev->effects			= 0;
	pev->yaw_speed		= 5;
	pev->ideal_yaw		= pev->angles.y;
	InitBoneControllers();
	ChangeYaw( 360 );

	m_flFrameRate		= 75;
	m_flGroundSpeed		= 0;
	pev->nextthink		+= 1.0;
	ResetSequenceInfo();

	if (pev->sequence != 0 || pev->frame != 0)
	{
		m_animate = 0;
		pev->framerate = 0;
	}
	else
	{
		m_animate = 1;
	}
}

// XDM3035a: may be called externally
void CCycler::Precache(void)
{
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

//
// cycler think
//
void CCycler::Think(void)
{
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM
		SetNextThink(0.1);
	else
		SetNextThink(0.25);

	if (m_animate)
		StudioFrameAdvance();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if (!m_animate)
			pev->framerate = 0.0;	// FIX: don't reset framerate
	}
	CBaseEntity::Think();// XDM3037: allow SetThink() on it!
}

//
// CyclerUse - starts a rotation trend
//
void CCycler::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	if (m_animate)
		pev->framerate = 1.0;
	else
		pev->framerate = 0.0;
}

//
// CyclerPain , changes sequences when shot
//
int CCycler::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (IsMultiplayer())// XDM
		return 0;

	if (m_animate)
	{
		pev->sequence++;
		ResetSequenceInfo( );
		if (m_flFrameRate == 0.0)
		{
			pev->sequence = 0;
			ResetSequenceInfo( );
		}
		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1.0;
		StudioFrameAdvance ( 0.1 );
		pev->framerate = 0;
		conprintf(2, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame );// XDM: don't allow lamers to see this (AIconsole)
	}
	return 0;
}

// we should get rid of all the other cyclers and replace them with this.
class CGenericCycler : public CCycler
{
public:
	virtual void Spawn(void) { GenericCyclerSpawn(STRING(pev->model), Vector(-16, -16, 0), Vector(16, 16, 72)); }
};

LINK_ENTITY_TO_CLASS( cycler, CGenericCycler );



class CCyclerSprite : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	ObjectCaps(void) { return (CBaseEntity::ObjectCaps() | FCAP_DONT_SAVE | FCAP_IMPULSE_USE); }
	virtual int	TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Animate( float frames );
	inline int		ShouldAnimate(void) { return m_animate && m_maxFrame > 1.0; }
	int			m_animate;
	float		m_lastTime;
	float		m_maxFrame;
	BOOL		m_bClientOnly;
};

LINK_ENTITY_TO_CLASS( cycler_sprite, CCyclerSprite );

TYPEDESCRIPTION	CCyclerSprite::m_SaveData[] =
{
	DEFINE_FIELD( CCyclerSprite, m_animate, FIELD_INTEGER ),
	DEFINE_FIELD( CCyclerSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CCyclerSprite, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CCyclerSprite, CBaseEntity );

void CCyclerSprite::Spawn(void)
{
	if (FStringNull(pev->model))
	{
		conprintf(1, "ERROR: CCyclerSprite %s[%d] %s without sprite! Removing!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
		pev->flags = FL_KILLME;//Destroy();
		return;
	}

	Precache();
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_YES;
	pev->effects		= 0;

	if (!FStringNull(pev->model))
	{
		if (UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))// XDM3035a: some stupid mappers use this entity to place models!
		{
			conprintf(0, "ERROR: %s[%d] %s: %s is not a sprite!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->model));
			CreateCopy("env_static", this, pev->spawnflags);
			pev->flags = FL_KILLME;//Destroy();
			return;
		}
		else
			SET_MODEL(edict(), STRING(pev->model));
	}

	pev->frame			= 0;
	SetNextThink(0.1);
	m_animate			= 1;
	m_lastTime			= gpGlobals->time;
	UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);// XDM3035a: IMPORTANT!
	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	if (sv_serverstaticents.value > 0.0f)
	{
		m_bClientOnly = FALSE;
	}
	else
	{
		m_bClientOnly = TRUE;
		pev->effects |= EF_NODRAW;
	}
}

// XDM3035a: may be called externally
void CCyclerSprite::Precache(void)
{
	// UNDONE: reliably check if this really is a sprite!
	PRECACHE_MODEL(STRINGV(pev->model));
}

void CCyclerSprite::Think(void)
{
	if ( ShouldAnimate() )
		Animate( pev->framerate * (gpGlobals->time - m_lastTime) );

	SetNextThink(0.1);
	m_lastTime = gpGlobals->time;
	CBaseEntity::Think();// XDM3037: allow SetThink() on it!
}

void CCyclerSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_animate = !m_animate;
	//conprintf(1, "Sprite: %s\n", STRING(pev->model) );
	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

int	CCyclerSprite::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if ( m_maxFrame > 1.0 )
	{
		Animate( 1.0 );
	}
	return 1;
}

void CCyclerSprite::Animate( float frames )
{
	pev->frame += frames;
	if ( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame, m_maxFrame );
}

// XDM3035a: awesome traffic economy on some maps!
// Called by clients connecting to the game
int CCyclerSprite::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (m_bClientOnly == FALSE)// server entity mode
		return 0;

#if defined (_DEBUG)
	conprintf(2, "CCyclerSprite: Creating client sprite %s\n", STRING(pev->model));
#endif

	if (msgtype == MSG_ONE)// a client has connected and needs an update
	{
		if (pClient == NULL)
			return 0;

		if (pClient->IsBot())// bots don't need sprites =)
			return 0;
	}
	else if (msgtype == MSG_BROADCAST)
		msgtype = MSG_ALL;// we need this fix in case someone will try to put this update into unreliable message stream

	/*if (IsRemoving())// XDM3037
	{
	???
	}*/

	MESSAGE_BEGIN(msgtype, gmsgStaticSprite, pev->origin, (pClient == NULL)?NULL : ENT(pClient->pev));
		WRITE_SHORT(entindex());// TODO: use pev->pContainingEntity->serialnumber?
		WRITE_SHORT(pev->modelindex);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_ANGLE(pev->angles.x);
		WRITE_ANGLE(pev->angles.y);
		WRITE_ANGLE(pev->angles.z);
		WRITE_BYTE(pev->rendermode);
		WRITE_BYTE(pev->renderfx);
		WRITE_BYTE(pev->rendercolor.x);
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_BYTE(pev->renderamt);
		// XDM3037a: new method		WRITE_BYTE(pev->effects);
	if (pev->impulse > 0)// XDM3037a: ON
		WRITE_COORD(pev->scale);
	else
		WRITE_COORD(0.0f);// OFF

		WRITE_BYTE(m_animate?(int)pev->framerate:0);

	if (pev->pContainingEntity->num_leafs > 0)// just send one because it's a point entity
		WRITE_SHORT(pev->pContainingEntity->leafnums[0]);
	else
		WRITE_SHORT(0);

	MESSAGE_END();
	return 1;
}




// Flaming Wreakage
// OBSOLETE
class CWreckage : public CCycler
{
	virtual void Spawn(void);
	virtual void Think(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	int m_flStartTime;
};

TYPEDESCRIPTION	CWreckage::m_SaveData[] =
{
	DEFINE_FIELD( CWreckage, m_flStartTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CWreckage, CCycler );


LINK_ENTITY_TO_CLASS( cycler_wreckage, CWreckage );

void CWreckage::Spawn(void)
{
	GenericCyclerSpawn(STRING(pev->model), Vector(-16, -16, 0), Vector(16, 16, 32));
	m_flStartTime = gpGlobals->time;
}

void CWreckage::Think(void)
{
	StudioFrameAdvance();
	SetNextThink(0.2);

	if (pev->dmgtime)
	{
		if (pev->dmgtime < gpGlobals->time)
		{
			Destroy();
			return;
		}
		else if (RANDOM_FLOAT(0, pev->dmgtime - m_flStartTime) > pev->dmgtime - gpGlobals->time)
		{
			return;
		}
	}

	Vector VecSrc;
	VecSrc.x = RANDOM_FLOAT( pev->absmin.x, pev->absmax.x );
	VecSrc.y = RANDOM_FLOAT( pev->absmin.y, pev->absmax.y );
	VecSrc.z = RANDOM_FLOAT( pev->absmin.z, pev->absmax.z );
	MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, VecSrc );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( VecSrc.x );
		WRITE_COORD( VecSrc.y );
		WRITE_COORD( VecSrc.z );
		WRITE_SHORT( g_iModelIndexSmoke );
		WRITE_BYTE( RANDOM_LONG(0,49) + 50 ); // scale * 10
		WRITE_BYTE( RANDOM_LONG(0, 3) + 8  ); // framerate
	MESSAGE_END();
}
