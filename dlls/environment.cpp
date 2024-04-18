//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
//#include "util_common.h"
#include "color.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "player.h"
#include "globals.h"
#include "effects.h"
#include "environment.h"
#include "game.h"
#include "gamerules.h"
#include "weapons.h"
#include "flamecloud.h"

//=================================================================
// CEnvFog
// fog effect
// TODO: fade in/out must be synchronized between all clients,
// even if someone has just connected
//=================================================================

LINK_ENTITY_TO_CLASS(env_fog, CEnvFog);

TYPEDESCRIPTION	CEnvFog::m_SaveData[] =
{
	DEFINE_FIELD(CEnvFog, m_iStartDist, FIELD_SHORT),
	DEFINE_FIELD(CEnvFog, m_iEndDist, FIELD_SHORT),
	DEFINE_FIELD(CEnvFog, m_iCurrentStartDist, FIELD_SHORT),
	DEFINE_FIELD(CEnvFog, m_iCurrentEndDist, FIELD_SHORT),
	DEFINE_FIELD(CEnvFog, m_fFadeTime, FIELD_FLOAT),
	DEFINE_FIELD(CEnvFog, m_fFadeStartTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CEnvFog, CBaseEntity);

void CEnvFog::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "startdist"))
	{
		m_iStartDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "enddist"))
	{
		m_iEndDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	/*else if (FStrEq(pkvd->szKeyName, "density")) ??? there's one map with this key, no idea how it should work
	{
		??? = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	/*else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_iFadeIn = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_iFadeOut = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else if (FStrEq(pkvd->szKeyName, "fadetime"))
	{
		m_fFadeTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	/*else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		m_fHoldTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else
		CBaseEntity::KeyValue(pkvd);
}

STATE CEnvFog::GetState(void)
{
	if (pev->impulse == 0)
		return STATE_OFF;

	return STATE_ON;
}

void CEnvFog::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

	Precache();

	if (FBitSet(pev->spawnflags, SF_FOG_ACTIVE))
	{
		pev->impulse = FOG_STATE_ON;
		m_iCurrentStartDist = m_iStartDist;
		m_iCurrentEndDist = m_iEndDist;
		//SendClientData(NULL, MSG_ALL);
	}
	else
	{
		pev->impulse = FOG_STATE_OFF;
		m_iCurrentStartDist = 0;
		m_iCurrentEndDist = 0;
	}

	SetThinkNull();
	SetNextThink(0);
}

void CEnvFog::Precache(void)
{
	if (m_iStartDist == 0) m_iStartDist = 1;
	if (m_iEndDist == 0) m_iEndDist = g_psv_zmax?g_psv_zmax->value:MAX_ABS_ORIGIN;
	if (m_fFadeTime <= 0.0f) m_fFadeTime = 2.0f;
	if (FStringNull(pev->targetname))
		SetBits(pev->spawnflags, SF_FOG_ACTIVE);
}

// this code should be synchromized with client-side code
void CEnvFog::Think(void)
{
	if (pev->impulse == FOG_STATE_FADEIN)
	{
		float k = max(0.01f,gpGlobals->time - m_fFadeStartTime)/m_fFadeTime;
		m_iCurrentStartDist = (m_iStartDist - 0)*k;
		m_iCurrentEndDist = (m_iEndDist - 0)*k;
		CBaseEntity::Think();// XDM3037: allow SetThink()
		SetNextThink(0.1);
	}
	else if (pev->impulse == FOG_STATE_FADEOUT)
	{
		float k = 1.0f-max(0.01f,gpGlobals->time - m_fFadeStartTime)/m_fFadeTime;
		m_iCurrentStartDist = (m_iStartDist - 0)*k;
		m_iCurrentEndDist = (m_iEndDist - 0)*k;
		CBaseEntity::Think();// XDM3037: allow SetThink()
		SetNextThink(0.1);
	}
	else
		SetNextThink(0);
}

void CEnvFog::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	bool bOn = (pev->impulse == FOG_STATE_ON || pev->impulse == FOG_STATE_FADEIN);
	if (!ShouldToggle(useType, bOn))
		return;

	if (pev->impulse == FOG_STATE_OFF || pev->impulse == FOG_STATE_FADEOUT)
	{
		pev->impulse = FOG_STATE_ON;
		//pev->impulse == FOG_STATE_FADEIN;
	}
	else
	{
		pev->impulse = FOG_STATE_OFF;
		//pev->impulse == FOG_STATE_FADEOUT;
	}

	//m_fFadeStartTime = gpGlobals->time;
	//SetNextThink(0.1);
	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

//-----------------------------------------------------------------------------
// Purpose: Send necessary updates to one or many clients as requested
// XDM3037: Added support for entity removal
// Input  : *pClient - 
//			msgtype - 
// Output : int
//-----------------------------------------------------------------------------
int CEnvFog::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (msgtype == MSG_ONE && (pev->impulse == 0 || pClient == NULL))
		return 0;

	if (IsRemoving())// XDM3037
	{
		pev->impulse = 0;
		m_iStartDist = 0;
		m_iEndDist = 0;
	}

	MESSAGE_BEGIN(msgtype, gmsgSetFog, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_BYTE(pev->rendercolor.x);
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_SHORT(m_iStartDist);// current values
		WRITE_SHORT(m_iEndDist);
		WRITE_BYTE(pev->impulse);// 0-off, 1-on, 2-fade out, 3-fade in
		//WRITE_SHORT(m_iFinalStartDist);// target values
		//WRITE_SHORT(m_iFinalEndDist);
		//WRITE_SHORT((int)(m_fFadeTime - (gpGlobals->time - m_fFadeStartTime)));// send remaining time
	MESSAGE_END();
	return 1;
}




//=================================================================
// CEnvFogZone
// OBSOLETE
//=================================================================

LINK_ENTITY_TO_CLASS(env_fogzone, CEnvFogZone);

void CEnvFogZone::Spawn(void)
{
	Precache();
	SET_MODEL(edict(), STRING(pev->model));// set size
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	if (showtriggers.value <= 0)
		pev->effects = EF_NODRAW;

	if (FBitSet(pev->spawnflags, SF_FOGZONE_ACTIVE))
		pev->skin = CONTENTS_FOG;
}

void CEnvFogZone::Precache(void)
{
	if (FStringNull(pev->targetname))
		SetBits(pev->spawnflags, SF_FOGZONE_ACTIVE);
}

void CEnvFogZone::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->skin == CONTENTS_FOG)
		pev->skin = 0;
	else
		pev->skin = CONTENTS_FOG;
}

// update fog params once when the player enters the zone
void CEnvFogZone::Touch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer*)pOther;
	if (pPlayer)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFog, NULL, ENT(pPlayer->pev));
			WRITE_BYTE(pev->rendercolor.x);
			WRITE_BYTE(pev->rendercolor.y);
			WRITE_BYTE(pev->rendercolor.z);
			WRITE_SHORT(m_iStartDist);
			WRITE_SHORT(m_iEndDist);
			WRITE_BYTE(0);
		MESSAGE_END();
	}
}




//=================================================================
// CEnvRain
// rain effect
//=================================================================

LINK_ENTITY_TO_CLASS(env_rain, CEnvRain);
/*
TYPEDESCRIPTION	CEnvRain::m_SaveData[] =
{
	DEFINE_FIELD(CEnvRain, bState, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CEnvRain, CBaseEntity);
*/
void CEnvRain::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "maxdrips"))
	{
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->noise1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "splashsprite"))
	{
		pev->noise2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "splashspriteg"))
	{
		pev->noise3 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "scalex"))
	{
		m_fScaleX = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "scaley"))
	{
		m_fScaleY = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvRain::Spawn(void)
{
	Precache();
	if (sprTexture == 0)
	{
		conprintf(0, "Design error: %s[%d] %s at (%g %g %g) without sprTexture!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->flags = FL_KILLME;
		return;
	}
	SET_MODEL(edict(), STRING(pev->model));// set size
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	if (showtriggers.value <= 0)
		pev->effects |= EF_NODRAW;

	pev->rendermode = kRenderTransAdd;// XDM3038c: required to fix color
	UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);// XDM3035a: IMPORTANT!

	if (pev->renderamt == 0)
		pev->renderamt = 255;
	if (pev->impulse < 1)
		pev->impulse = 1;

	if (FBitSet(pev->spawnflags, SF_RAIN_START_OFF))
		pev->bInDuck = FALSE;
	else
		pev->bInDuck = TRUE;

	if (m_fScaleX <= 0.0f)
		m_fScaleX = 1.0f;
	if (m_fScaleY <= 0.0f)
		m_fScaleY = 1.0f;

	/*if (FBitSet(pev->spawnflags, SF_RAIN_DONTHITMONSTERS))
		ig = dont_ignore_monsters;
	else
		ig = ignore_monsters;*/
}

void CEnvRain::Precache(void)
{
	// optional
	if (!FStringNull(pev->noise1))
		sprTexture = PRECACHE_MODEL(STRINGV(pev->noise1));
	if (!FStringNull(pev->noise2))
		sprHitWater = PRECACHE_MODEL(STRINGV(pev->noise2));
	if (!FStringNull(pev->noise3))
		sprHitGround = PRECACHE_MODEL(STRINGV(pev->noise3));
}

void CEnvRain::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, pev->bInDuck > 0))
		return;
	//conprintf(1, "SERVER RAIN TOGGLE!\n");

	if (pev->bInDuck > 0)
		pev->bInDuck = 0;
	else
		pev->bInDuck = 1;

	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

int CEnvRain::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	unsigned short flags = RENDERSYSTEM_FLAG_LOOPFRAMES;// XDM3038c
	bool bRemove = false;

	if (IsRemoving())// XDM3037
	{
		bRemove = true;
	}
	else
	{
		if (pev->bInDuck > 0)// ON
		{
			if (pev->flags & FL_DRAW_ALWAYS)
				flags |= RENDERSYSTEM_FLAG_DRAWALWAYS;

			if (FBitSet(pev->spawnflags, SF_RAIN_ZROTATION))
				flags |= RENDERSYSTEM_FLAG_ZROTATION;

			//if (FBitSet(pev->spawnflags, SF_RAIN_NOSPLASH))
			flags |= RENDERSYSTEM_FLAG_CLIPREMOVE;

			if (!FBitSet(pev->spawnflags, SF_RAIN_IGNOREMODELS))// since the default behavior was to detect studio models, we need an opposite option here
				flags |= RENDERSYSTEM_FLAG_ADDPHYSICS;// XDM3035c: now it is required to hit players
		}
		else// OFF
		{
			if (msgtype == MSG_ONE)// a client has connected and needs an update
				return 0;

			//flags = RENDERSYSTEM_FLAG_REMOVE;
			bRemove = true;
		}
	}
	if (msgtype == MSG_BROADCAST)
		msgtype = MSG_ALL;// we need this fix in case someone will try to put this update into unreliable message stream

	edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	MESSAGE_BEGIN(msgtype, gmsgSetRain, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_COORD_NOCLAMP(pev->movedir.x*pev->speed);// direction
		WRITE_COORD_NOCLAMP(pev->movedir.y*pev->speed);
		WRITE_COORD_NOCLAMP(pev->movedir.z*pev->speed);
	if (!FNullEnt(pentTarget))
		WRITE_SHORT(ENTINDEX(pentTarget));
	else
		WRITE_SHORT(entindex());

	if (bRemove)// XDM3035
	{
		WRITE_SHORT(0);
		WRITE_SHORT(0);
		WRITE_SHORT(0);
		WRITE_SHORT(0);// XDM3035c
		WRITE_SHORT(0);
	}
	else
	{
		WRITE_SHORT(pev->modelindex);
		WRITE_SHORT(sprTexture);
		if (FBitSet(pev->spawnflags, SF_RAIN_NOSPLASH))
		{
		WRITE_SHORT(0);// XDM3035c
		WRITE_SHORT(0);
		}
		else
		{
		WRITE_SHORT(sprHitWater);
		WRITE_SHORT(sprHitGround);// XDM3035c
		}
		WRITE_SHORT(pev->impulse);
	}
		WRITE_SHORT(0);// life*10
		WRITE_BYTE((int)(m_fScaleX*10.0f));// XDM3034: sprite proportions will also be used
		WRITE_BYTE((int)(m_fScaleY*10.0f));
		WRITE_BYTE(kRenderTransAdd);
		WRITE_BYTE(pev->rendercolor.x);// byte,byte,byte (color)
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_BYTE(pev->renderamt);// byte (brightness)
		WRITE_SHORT(flags);
	MESSAGE_END();
	//conprintf(1, "------------- CEnvRain sent %d\n", pev->impulse);
	return 1;
}




//=================================================================
// CEnvWarpBall
// Teleportation effect
//=================================================================

LINK_ENTITY_TO_CLASS(env_warpball, CEnvWarpBall);

void CEnvWarpBall::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "effectsprite"))
	{
		pev->noise1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "mainsound"))
	{
		pev->noise2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "effectsound"))
	{
		pev->noise3 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "beamcount"))
	{
		pev->oldbuttons = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->health = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "mainsprite"))// old hack
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvWarpBall::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	if (pev->health < WARPBALL_MIN_RADIUS)
		pev->health = WARPBALL_MIN_RADIUS;

	if (FStringNull(pev->model))
		pev->model = ALLOC_STRING(WARPBALL_DEFAULT_SPR_MAIN);

	//if (FStringNull(pev->noise1))
	//	pev->noise1 = ALLOC_STRING(WARPBALL_DEFAULT_SPR_EFFECT);

	if (FStringNull(pev->noise2))
		pev->noise2 = ALLOC_STRING(WARPBALL_DEFAULT_SND_MAIN);

	//if (FStringNull(pev->noise3))
	//	pev->noise3 = ALLOC_STRING(WARPBALL_DEFAULT_SND_EFFECT);

	pev->impulse = 0;
	Precache();
	DontThink();
}

void CEnvWarpBall::Precache(void)
{
	m_iSpriteBeam = PRECACHE_MODEL("sprites/lgtning.spr");
	m_iSprite1 = PRECACHE_MODEL(STRINGV(pev->model));
	if (FStringNull(pev->noise1))
		m_iSprite2 = 0;
	else
		m_iSprite2 = PRECACHE_MODEL(STRINGV(pev->noise1));

	if (!FStringNull(pev->noise2))
		PRECACHE_SOUND(STRINGV(pev->noise2));
	if (!FStringNull(pev->noise3))
		PRECACHE_SOUND(STRINGV(pev->noise3));
}

void CEnvWarpBall::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	pev->impulse = 1;
	PLAYBACK_EVENT_FULL(0, edict(), g_usWarpBall, 0.0, pev->origin, pev->angles, pev->health, pev->oldbuttons, m_iSprite1, m_iSpriteBeam, 1, 0);
	if (!FStringNull(pev->noise2))
		EMIT_SOUND(edict(), CHAN_BODY, STRINGV(pev->noise2), VOL_NORM, ATTN_NORM);
	SetNextThink(0.8);
}

void CEnvWarpBall::Think(void)
{
	if (pev->impulse > 0)
	{
		if (m_iSprite2 != 0)
		{
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_SPRITE);
			WRITE_COORD_NOCLAMP(pev->origin.x);
			WRITE_COORD_NOCLAMP(pev->origin.y);
			WRITE_COORD_NOCLAMP(pev->origin.z);
			WRITE_SHORT(m_iSprite2);
			WRITE_BYTE(10);// scale * 10
			WRITE_BYTE(255);// brightness
		MESSAGE_END();
		}
		if (!FStringNull(pev->noise3))
			EMIT_SOUND(edict(), CHAN_ITEM, STRINGV(pev->noise3), VOL_NORM, ATTN_NORM);

		SUB_UseTargets(this, USE_TOGGLE, 0);
		pev->impulse = 0;
		DontThink();
	}
	CBaseEntity::Think();// XDM3037: allow SetThink()

	if (FBitSet(pev->spawnflags, SF_WARPBALL_ONCE))
		Destroy();
}




//=================================================================
// CEnvShockwave
// shock wave effect
//=================================================================

LINK_ENTITY_TO_CLASS(env_shockwave, CEnvShockwave);

TYPEDESCRIPTION	CEnvShockwave::m_SaveData[] =
{
	DEFINE_FIELD( CEnvShockwave, m_iHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iTime, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iRadius, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iScrollRate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iNoise, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iFrameRate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iStartFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iSpriteTexture, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_cType, FIELD_CHARACTER ),
	DEFINE_FIELD( CEnvShockwave, m_iszPosition, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CEnvShockwave, CBaseEntity);

void CEnvShockwave::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iTime"))
	{
		m_iTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iRadius"))
	{
		m_iRadius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iHeight"))
	{
		m_iHeight = atoi(pkvd->szValue)/2;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iScrollRate"))
	{
		m_iScrollRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iNoise"))
	{
		m_iNoise = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFrameRate"))
	{
		m_iFrameRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartFrame"))
	{
		m_iStartFrame = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_cType"))
	{
		m_cType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvShockwave::Spawn(void)
{
	CPointEntity::Spawn();
}

void CEnvShockwave::Precache(void)
{
	m_iSpriteTexture = PRECACHE_MODEL(STRINGV(pev->netname));
}

void CEnvShockwave::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Vector vecPos(pev->origin);
	if (!FBitSet(pev->spawnflags, SF_SHOCKWAVE_CENTERED))
		vecPos.z += m_iHeight;

	// blast circle
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
	if (m_cType)
		WRITE_BYTE(m_cType);
	else
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(vecPos.x);// coord coord coord (center position)
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_COORD(vecPos.x);// coord coord coord (axis and radius)
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z + m_iRadius);
		WRITE_SHORT(m_iSpriteTexture); // short (sprite index)
		WRITE_BYTE(m_iStartFrame); // byte (starting frame)
		WRITE_BYTE(m_iFrameRate); // byte (frame rate in 0.1's)
		WRITE_BYTE(m_iTime); // byte (life in 0.1's)
		WRITE_BYTE(m_iHeight);  // byte (line width in 0.1's)
		WRITE_BYTE(m_iNoise);   // byte (noise amplitude in 0.01's)
		WRITE_BYTE(pev->rendercolor.x);   // byte,byte,byte (color)
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_BYTE(pev->renderamt);  // byte (brightness)
		WRITE_BYTE(m_iScrollRate);	// byte (scroll speed in 0.1's)
	MESSAGE_END();

	if (!FBitSet(pev->spawnflags, SF_SHOCKWAVE_REPEATABLE))
	{
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
	}
}




//=================================================================
// CEnvDLight
// dynamic light effect
// OBSOLETE: update this to use RenderSystem!
//=================================================================

LINK_ENTITY_TO_CLASS(env_dlight, CEnvDLight);
LINK_ENTITY_TO_CLASS(env_elight, CEnvDLight);

TYPEDESCRIPTION	CEnvDLight::m_SaveData[] =
{
	DEFINE_FIELD(CEnvDLight, m_vecPos, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CEnvDLight, m_hAttach, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CEnvDLight, CPointEntity);

void CEnvDLight::Spawn(void)
{
	CPointEntity::Spawn();
	if (FClassnameIs(edict(), "env_elight"))
		elight = TRUE;
	else
		elight = FALSE;
}

void CEnvDLight::PostSpawn(void)
{
	if (FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_DLIGHT_STARTON))
		DesiredAction();
}

void CEnvDLight::DesiredAction(void)
{
	Use(this, this, USE_ON, 0);
}

void CEnvDLight::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (elight)
	{
		if (pev->target)
		{
			m_hAttach = UTIL_FindEntityByTargetname(NULL, STRING(pev->target)/*, pActivator*/);
			if (m_hAttach.Get() == NULL)
			{
				conprintf(1, "Entity env_elight '%s' can't find target '%s'!\n", STRING(pev->targetname), STRING(pev->target));
				m_hAttach = this;
			}
		}
		else
			m_hAttach = this;
	}

	if (!ShouldToggle(useType, pev->nextthink > 0?TRUE:FALSE))
		return;

	if (pev->health == 0 && pev->nextthink > 0) // if we're thinking, and in switchable mode, then we're on
	{
		// turn off
		SetNextThink(0);
		return;
	}

	int iTime;
	m_vecPos = pev->origin;

	if (pev->health == 0)
	{
		iTime = 10;// 1 second
		SetNextThink(1);
	}
	else if (pev->health > 25)
	{
		iTime = 250;
		pev->takedamage = 25;
		SetNextThink(25);
	}
	else
		iTime = pev->health*10;

	MakeLight(iTime);

	if (FBitSet(pev->spawnflags, SF_DLIGHT_ONLYONCE))
	{
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
	}
}

void CEnvDLight::Think(void)
{
	int iTime;
	if (pev->health == 0)
	{
		iTime = 10;
		SetNextThink(1);
	}
	else
	{
		pev->takedamage += 25;
		if (pev->health > pev->takedamage)
		{
			iTime = 25;
			SetNextThink(25);
		}
		else
		{
			// finished, just do the leftover bit
			iTime = (pev->health - pev->takedamage)*10;
			pev->takedamage = 0;
		}
	}

	MakeLight(iTime);
	CBaseEntity::Think();// XDM3037: allow SetThink() on it!
}

void CEnvDLight::MakeLight(int iTime)
{
	if (elight)
	{
		if (m_hAttach.Get() == NULL)
		{
			SetNextThink(0);
			pev->takedamage = 0;
			return;
		}
	}

	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, m_vecPos);
	if (elight)
	{
		WRITE_BYTE(TE_ELIGHT);
		WRITE_SHORT(m_hAttach->entindex() + 0x1000 * pev->impulse);// entity, attachment
	}
	else
		WRITE_BYTE(TE_DLIGHT);

		WRITE_COORD(m_vecPos.x);// X
		WRITE_COORD(m_vecPos.y);// Y
		WRITE_COORD(m_vecPos.z);// Z
	if (elight)
		WRITE_COORD(pev->renderamt);// radius * 0.1
	else
		WRITE_BYTE(pev->renderamt);

		WRITE_BYTE(pev->rendercolor.x);// r
		WRITE_BYTE(pev->rendercolor.y);// g
		WRITE_BYTE(pev->rendercolor.z);// b
		WRITE_BYTE(iTime);// time * 10
	if (elight)
		WRITE_COORD(pev->frags);// decay * 0.1
	else
		WRITE_BYTE(pev->frags);

	MESSAGE_END();
}


//=================================================================
// CEnvFountain
// Particle system effect
//=================================================================

LINK_ENTITY_TO_CLASS(env_fountain, CEnvFountain);
/*
TYPEDESCRIPTION	CEnvFountain::m_SaveData[] =
{
	DEFINE_FIELD(CEnvFountain, bRandVec, FIELD_BOOLEAN),
//	DEFINE_FIELD(CEnvFountain, bState, FIELD_BOOLEAN),
//	DEFINE_FIELD(CEnvFountain, bEntFound, FIELD_BOOLEAN),
//	DEFINE_FIELD(CEnvFountain, pTargetEnt, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CEnvFountain, CBaseEntity);
*/
void CEnvFountain::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "amount"))
	{
		pev->dmg  = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "direction"))
	{
		if (StringToVec(pkvd->szValue, pev->movedir))
		{
			//pev->gamestate = 0;// random direction
			pkvd->fHandled = TRUE;
		}
		/*else
		{
			// we're not sure if origin was already set			pev->movedir = pev->origin;
			pev->gamestate = 1;
			pkvd->fHandled = FALSE;
		}*/
	}
	else if (FStrEq(pkvd->szKeyName, "offset"))
	{
		if (StringToVec(pkvd->szValue, pev->view_ofs))
			pkvd->fHandled = TRUE;
		else
			pkvd->fHandled = FALSE;
	}
	else if (FStrEq(pkvd->szKeyName, "spread"))// XDM3037a
	{
		if (StringToVec(pkvd->szValue, pev->vuser1))
			pkvd->fHandled = TRUE;
		else
			pkvd->fHandled = FALSE;
	}
	else if (FStrEq(pkvd->szKeyName, "life"))// Entity life, not particle
	{
		pev->health = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sprite"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CEnvFountain::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	CPointEntity::Spawn();// pev->model is null after this!

	if (pev->modelindex == 0)
	{
		conprintf(0, "Design error: %s[%d] %s at (%g %g %g) without sprite!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->flags = FL_KILLME;
		return;
	}

	UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);// XDM3035a: IMPORTANT!
	//bRandVec = FALSE;
	//bEntFound = FALSE;
	if (pev->vuser1.IsZero())// XDM3037a
	{
		if (FBitSet(pev->spawnflags, SF_FOUNTAIN_SPARKS))
			pev->vuser1.Set(0.1, 0.1, 64.0);// X Y size for sparks, Z velocity
		else
			pev->vuser1.Set(0.3, 0.3, 0.3);// spread for flamecone
	}
	//if (pev->gamestate > 0)//bRandVec)
	if (FBitSet(pev->spawnflags, SF_FOUNTAIN_RANDOM_DIR))
		pev->movedir = pev->origin;

	if (FBitSet(pev->spawnflags, SF_FOUNTAIN_START_OFF))
		pev->impulse = FALSE;
	else
		pev->impulse = TRUE;
}

void CEnvFountain::Precache(void)
{
	if (FStringNull(pev->model))
	{
		if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)// XDM3038c: only replace with default sprite if debugging
			pev->modelindex = g_iModelIndexWhite;
	}
	else
		pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

void CEnvFountain::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, pev->impulse > 0))
		return;

	if (pev->impulse > 0)
		pev->impulse = 0;
	else
		pev->impulse = 1;

	if (pev->impulse > 0)
		SendClientData(NULL, MSG_PVS, SCD_SELFUPDATE);// MSG_ALL?
	else
		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

int CEnvFountain::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (pClient == NULL && msgtype == MSG_ONE)// can't be
		return 0;

	unsigned short flags = 0;// XDM3038b: forgot what was that /(^_^)
	bool bRemove = false;// XDM3038b: name conflict

	if (IsRemoving())// XDM3037
	{
		bRemove = true;
		//pev->modelindex = 0;
		pev->rendermode = 0;
	}
	else
	{
		if (pev->impulse > 0)
		{
			if (FBitSet(pev->flags, FL_DRAW_ALWAYS))
				flags |= RENDERSYSTEM_FLAG_DRAWALWAYS;

			if (FBitSet(pev->spawnflags, SF_FOUNTAIN_RANDOM_FRAME))
				flags |= RENDERSYSTEM_FLAG_RANDOMFRAME;

			if (FBitSet(pev->spawnflags, SF_FOUNTAIN_SCALE))
				flags |= RENDERSYSTEM_FLAG_16;// XDM3035: ...

			if (FBitSet(pev->spawnflags, SF_FOUNTAIN_ZROTATION))
				flags |= RENDERSYSTEM_FLAG_ZROTATION;

			if (!FBitSet(pev->spawnflags, SF_FOUNTAIN_FRAMES_ONCE))// loop frames by default
				flags |= RENDERSYSTEM_FLAG_LOOPFRAMES;
		}
		else
		{
			if (msgtype == MSG_ONE)// a client has connected and needs an update
				return 0;

			bRemove = true;
		}
	}

	//conprintf(1, "pev->origin = %f %f %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
	MESSAGE_BEGIN(msgtype, gmsgPartSys, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_COORD_NOCLAMP(pev->origin.x);
		WRITE_COORD_NOCLAMP(pev->origin.y);
		WRITE_COORD_NOCLAMP(pev->origin.z);
		WRITE_COORD_NOCLAMP(pev->movedir.x);
		WRITE_COORD_NOCLAMP(pev->movedir.y);
		WRITE_COORD_NOCLAMP(pev->movedir.z);
	if (FBitSet(pev->spawnflags, SF_FOUNTAIN_SPARKS))
	{
		WRITE_COORD_NOCLAMP(0.1);// X size for sparks
		WRITE_COORD_NOCLAMP(0.1);// Y
		WRITE_COORD_NOCLAMP(64.0);// velocity
	}
	else
	{
		WRITE_COORD_NOCLAMP(0.3);// spread
		WRITE_COORD_NOCLAMP(0.3);
		WRITE_COORD_NOCLAMP(0.3);
	}
		WRITE_SHORT(pev->modelindex);// sprite name
		WRITE_BYTE(pev->rendermode);// render mode
		//WRITE_BYTE(adelta);// brightness decrease per second
	if (bRemove)
		WRITE_BYTE(PARTSYSTEM_TYPE_REMOVEANY);
	else if (FBitSet(pev->spawnflags, SF_FOUNTAIN_SPARKS))
		WRITE_BYTE(PARTSYSTEM_TYPE_SPARKS);
	else
		WRITE_BYTE(PARTSYSTEM_TYPE_FLAMECONE);

		WRITE_SHORT(pev->dmg);// max particles

	if (FBitSet(pev->spawnflags, SF_FOUNTAIN_ONCE))
		WRITE_SHORT(pev->health * 10.0f);// life
	else
		WRITE_SHORT(0);// toggle

		WRITE_SHORT(flags);
		WRITE_SHORT(entindex());// follow entity
	MESSAGE_END();
	/*if (FBitSet(pev->spawnflags, SF_FOUNTAIN_REMOVE))
	{
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(pev->health);// XDM3038a
	}*/
	return 1;
}

/*
void CEnvFountain::UpdatePos(void)
{
	if (pTargetEnt)
	{
		pev->movedir = pTargetEnt->pev->origin - pev->origin;
		pev->origin = pTargetEnt->pev->origin;/* +
			gpGlobals->v_right * pev->view_ofs.x +
			gpGlobals->v_forward * pev->view_ofs.y +
			gpGlobals->v_up * pev->view_ofs.z;*/
		/*UTIL_SetOrigin(this, pev->origin);
	}
	else
	{
		if (bEntFound)
		{
			Destroy();
		}
		else
		{
			if (!FStringNull(pev->target))
				pTargetEnt = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));

			if (pTargetEnt)
				bEntFound = TRUE;
		}
	}
}
*/

//=================================================================
// CEnvSun
// not yet
//=================================================================
/*LINK_ENTITY_TO_CLASS(env_sun, CEnvSun);

void CEnvSun::Precache(void)
{
}

void CEnvSun::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->effects = 0;
}

void CEnvSun::Think(void)
{
}*/

//=================================================================
// CLavaBall
// lava balls
//=================================================================
LINK_ENTITY_TO_CLASS(lava_ball, CLavaBall);

void CLavaBall::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->takedamage = DAMAGE_NO;
	pev->flags = 0;// obsolete FL_IMMUNE_LAVA;
	pev->health = 1.0;
	pev->gravity = 0.8;
	pev->dmg = 2.0;
	SET_MODEL(edict(), "sprites/proj1.spr");
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	PLAYBACK_EVENT_FULL(0, edict(), g_usTrail, 0.0, pev->origin, pev->angles, 4.0, 0.0, entindex(), 1, 0, 0);
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		pev->effects |= EF_DIMLIGHT;

	pev->velocity.Set(0.0f,0.0f,RANDOM_FLOAT(100,150));
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;
	pev->renderfx = kRenderFxPulseFast;
	pev->impulse = MODEL_FRAMES(pev->modelindex);
	SetNextThink(0.5);// XDM3038a
}

void CLavaBall::Precache(void)
{
	PRECACHE_MODEL("sprites/proj1.spr");
}

void CLavaBall::Think(void)
{
	if (pev->renderamt <= 5)
	{
		Destroy();
		return;
	}
	pev->frame = (int)(pev->frame + 1) % pev->impulse;
	pev->renderamt -= 5;
	SetNextThink(0.05);// XDM3038a
	CBaseEntity::Think();// XDM3037: allow SetThink()
}


//=================================================================
// CBaseLava
// OBSOLETE: lava balls spawner
//=================================================================
LINK_ENTITY_TO_CLASS(func_lava, CBaseLava);

void CBaseLava::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "updatetime"))
	{
		pev->animtime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CBaseLava::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	if (showtriggers.value <= 0)
		pev->effects = EF_NODRAW;

	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(this, pev->mins, pev->maxs);
	SetThink(&CBaseLava::LavaThink);
	SetUse(&CBaseLava::LavaUse);

	if (!pev->animtime)
		pev->animtime = 1.0;

	if (FBitSet(pev->spawnflags, SF_LAVA_START_OFF))
	{
		pev->impulse = FALSE;
	}
	else
	{
		pev->impulse = TRUE;
		SetNextThink(0.1);// XDM3038a
	}
}

void CBaseLava::Precache(void)
{
	UTIL_PrecacheOther("lava_ball");
}

void CBaseLava::LavaThink(void)
{
	if (pev->impulse <= 0)
		return;

	Create("lava_ball", RandomVector(pev->absmin, pev->absmax), g_vecZero, UTIL_RandomBloodVector() * RANDOM_LONG(50,100), edict(), SF_NORESPAWN);
	SetNextThink(pev->animtime);// XDM3038a
}

void CBaseLava::LavaUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, pev->impulse > 0))
		return;

	if (pev->impulse > 0)
	{
		pev->impulse = 0;
		DontThink();
	}
	else
	{
		pev->impulse = 1;
		SetNextThink(0.1);
	}
}


//=================================================================
// CEnvLightAttachment
// dlight attacment
//=================================================================

LINK_ENTITY_TO_CLASS(env_lightatt, CEnvLightAttachment);

void CEnvLightAttachment::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->effects = EF_NODRAW | EF_DIMLIGHT;
	SetNextThink(0.1);// XDM3038a
}

void CEnvLightAttachment::Think(void)
{
	if (!pev->aiment)
	{
		if (!FStringNull(pev->target))
		{
			CBaseEntity *pTargetEnt = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
			if (pTargetEnt)
			{
				pev->aiment = pTargetEnt->edict();
				pev->movetype = MOVETYPE_FOLLOW;
			}
			else
				pev->movetype = MOVETYPE_NONE;
		}
	}
	CBaseEntity::Think();// XDM3037: allow SetThink()
	SetNextThink(0.1);// XDM3038a
}




//=================================================================
// CEnvSky
// env_sky, 3D skybox view point
//=================================================================
LINK_ENTITY_TO_CLASS(env_sky, CEnvSky);

void CEnvSky::Spawn(void)
{
	CPointEntity::Spawn();
	pev->impulse = 1;
	DontThink();// XDM3038a
}

int CEnvSky::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (IsRemoving())// XDM3037
	{
		pev->impulse = 0;
	}
	//conprintf(1, "CEnvSky::SendClientData(%d)\n", msgtype);
	MESSAGE_BEGIN(msgtype, gmsgSetSky, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_COORD_NOCLAMP(pev->origin.x);
		WRITE_COORD_NOCLAMP(pev->origin.y);
		WRITE_COORD_NOCLAMP(pev->origin.z);
		WRITE_BYTE(pev->impulse);
	MESSAGE_END();
	return 1;
}




//=================================================================
// CEnvFlameSpawner - creates real flame clouds
// Uses VERY much network bandwidth!
//=================================================================
LINK_ENTITY_TO_CLASS(env_flamespawner, CEnvFlameSpawner);

void CEnvFlameSpawner::Spawn(void)
{
	CPointEntity::Spawn();
	if (FBitSet(pev->spawnflags, SF_ENVFLAMESPAWNER_START_OFF))
	{
		DontThink();// XDM3038a
		pev->impulse = 0;
	}
	else
	{
		SetNextThink(pev->speed);// XDM3038a
		pev->impulse = 1;
	}
}

void CEnvFlameSpawner::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->modelindex = g_iModelIndexFlameFire;
	else
		pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

void CEnvFlameSpawner::Think(void)
{
	CFlameCloud::CreateFlame(pev->origin, pev->movedir, pev->modelindex, 0.1, 0.15, FLAMECLOUD_VELMULT, gSkillData.DmgFlame, 255, 20, 0, this, this);// XDM3038c: fix
	CBaseEntity::Think();// XDM3037: allow SetThink() on it!
	SetNextThink(pev->speed);// XDM3038a
}

void CEnvFlameSpawner::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_ON)
		pev->impulse = 1;
	else if (useType == USE_OFF)
		pev->impulse = 0;
	else if (useType == USE_TOGGLE)
		pev->impulse = !pev->impulse;

	if (pev->impulse > 0)
		SetNextThink(0);// XDM3038a
	else
		DontThink();// XDM3038a
}


//=================================================================
// CEnvStatic - static mesh that does not require to precache model
// pev->impulse - ON/OFF
//=================================================================
LINK_ENTITY_TO_CLASS(env_static, CEnvStatic);

bool CEnvStatic::ValidateModel(void)// XDM3038c
{
	if (FStringNull(pev->model))
	{
		conprintf(0, "Design error: %s[%d] \"%s\" at (%g %g %g) without model!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		return false;
	}
	if (!UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))
	{
		conprintf(0, "Design error: %s[%d] \"%s\": \"%s\" is not a studio model!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->model));
		return false;
	}
	return true;
}

void CEnvStatic::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))// XDM3038: ENVSTATIC_DETAILTYPE_E
	{
		pev->weaponanim = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CEnvStatic::Precache(void)
{
	if (sv_serverstaticents.value > 0.0f)// XDM3038b: finally
	{
		ClearBits(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY);
		CBaseAnimating::Precache();
	}
	else
		SetBits(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY);
}

// Warning: We do not precache or set model if client-only mode is enabled. That't the whole point.
// Warning: Solidity is impossible without model and precaching!
void CEnvStatic::Spawn(void)
{
	if (!ValidateModel())
	{
		pev->flags = FL_KILLME;
		return;
	}

	Precache();
	pev->movetype		= MOVETYPE_NONE;
	pev->solid			= SOLID_NOT;
	//pev->frame			= 0;
	pev->effects		= EF_NOINTERP;
	UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);// XDM3035a: IMPORTANT!
	pev->health			= 65535.0;
	pev->takedamage		= DAMAGE_NO;
	pev->deadflag		= DEAD_NO;

	// No, user must set this manually! pev->colormap = RGB2colormap((int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z);
	if (!FBitSet(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY))
		SET_MODEL(edict(),	STRINGV(pev->model));

	UTIL_SetOrigin(this, pev->origin);// XDM3037
	// CRASH when TraceLined! UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	// this thing does not work	MAKE_STATIC(edict());
	m_flFrameRate = pev->framerate;
	m_flGroundSpeed = 0.0f;

	if (FBitSet(pev->spawnflags, SF_ENVSTATIC_START_INVISIBLE))
		pev->impulse = 0;
	else
		pev->impulse = 1;// visible

	if (!FBitSet(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY))
	{
		if (pev->impulse == 0)
			SetBits(pev->effects, EF_NODRAW);
		else
			ClearBits(pev->effects, EF_NODRAW);
	}
	else
		SetBits(pev->effects, EF_NODRAW);// XDM3035: use impulse to determine visibility, this flag is to NOT to send to client only

	// Keep framerate set by designer!
	pev->frame = 0;
	SetThinkNull();
	DontThink();// XDM3038a
	pev->animtime = gpGlobals->time + 0.1f;
}

void CEnvStatic::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, pev->impulse > 0))
		return;

	if (pev->impulse == 0)// was OFF
	{
		pev->impulse = 1;
		pev->animtime = gpGlobals->time;
		if (!FBitSet(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY))
			ClearBits(pev->effects, EF_NODRAW);
	}
	else// was ON
	{
		if (!FBitSet(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY))
			SetBits(pev->effects, EF_NODRAW);

		pev->impulse = 0;
		pev->animtime = 0.0;
		SetBits(pev->effects, EF_NODRAW);
	}
	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

int CEnvStatic::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (!FBitSet(pev->spawnflags, SF_ENVSTATIC_CLIENTONLY))
		return 0;

	if (msgtype == MSG_ONE)// a client has connected and needs an update
	{
		if (pClient == NULL)
			return 0;

		if (pClient->IsBot())// bots don't need trees =)
			return 0;// game dll-integrated bots will recognize these
	}
	else if (msgtype == MSG_BROADCAST)
		msgtype = MSG_ALL;// we need this fix in case someone will try to put this update into unreliable message stream

#if defined (_DEBUG)
	//if (msgtype == MSG_ALL && sendcase != SCD_GLOBALUPDATE)
		conprintf(2, "%s[%d] %s: s2c(%hd) -> \"%s\"[%d] @(%g %g %g) (%g %g %g)\n", STRING(pev->classname), entindex(), STRING(pev->targetname), sendcase, STRING(pev->model), pev->modelindex, pev->origin.x, pev->origin.y, pev->origin.z, pev->angles.x, pev->angles.y, pev->angles.z);
#endif
	MESSAGE_BEGIN(msgtype, gmsgStaticEnt, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_SHORT(entindex());
		if (IsRemoving())// XDM3037 // SCD_ENTREMOVE
			WRITE_SHORT(0);// 0 == destroy
		else if (pev->modelindex == 0)// XDM3038b: not precached
			WRITE_SHORT(1);
		else
			WRITE_SHORT(pev->modelindex);

		WRITE_FLOAT(pev->origin.x);
		WRITE_FLOAT(pev->origin.y);
		WRITE_FLOAT(pev->origin.z);
		WRITE_ANGLE(pev->angles.x);
		WRITE_ANGLE(pev->angles.y);
		WRITE_ANGLE(pev->angles.z);
		WRITE_BYTE(pev->rendermode);
		WRITE_BYTE(pev->renderfx);
		WRITE_BYTE(pev->rendercolor.x);
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_BYTE(pev->renderamt);
		WRITE_BYTE(pev->body);
		WRITE_BYTE(pev->skin);
	if (pev->impulse > 0)// XDM3037a: ON
		WRITE_COORD_NOCLAMP(pev->scale);
	else
		WRITE_COORD_NOCLAMP(0.0f);// OFF

		WRITE_SHORT(pev->colormap & 0x0000FFFF);// XDM3038c: two bytes!
		WRITE_SHORT(pev->sequence);
	if (pev->pContainingEntity->num_leafs > 0)// just send one because it's a point entity
		WRITE_SHORT(pev->pContainingEntity->leafnums[0]);
	else
		WRITE_SHORT(0);

		//WRITE_BYTE(pev->weaponanim);// XDM3038: TODO
		WRITE_FLOAT(pev->framerate);// XDM3038c
		WRITE_STRING(STRING(pev->model));// XDM3038b
	MESSAGE_END();
	return 1;
}
