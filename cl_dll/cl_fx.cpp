#include "hud.h"
#include "cl_util.h"
#include "cdll_int.h"
#include "r_efx.h"
#include "event_api.h"
#include "eventscripts.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "pmtrace.h"
#include "shared_resources.h"
#include "cl_fx.h"
#include "decals.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSSprite.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "PSFlameCone.h"
#include "PSBubbles.h"
#include "PSSparks.h"
#include "weapondef.h"
#include "triangleapi.h"

// Pre-defined effects, mostly engine-based when RenderSystem is not applicable

#define FLASHLIGHT_DISTANCE		864
#define FLASHLIGHT_RADIUS1		56
#define FLASHLIGHT_RADIUS2		288

color24 gTracerColors[] =
{
	{ 255, 255, 255 },	// 0 White
	{ 255, 0, 0 },		// Red
	{ 0, 255, 0 },		// Green
	{ 0, 0, 255 },		// Blue
	{ 0, 0, 0 },		// Tracer default, filled in from cvars, etc.
	{ 255, 167, 17 },	// Yellow-orange sparks
	{ 255, 130, 90 },	// Yellowish streaks (garg)
	{ 55, 60, 144 },	// Blue egon streak
	{ 255, 130, 90 },	// More Yellowish streaks (garg)
	{ 255, 140, 90 },	// More Yellowish streaks (garg)
	{ 200, 130, 90 },	// 10 More red streaks (garg)
	{ 255, 120, 70 },	// Darker red streaks (garg)
	// XDM-specific
	{ 255, 191, 191 },	// Very bright red
	{ 191, 255, 191 },	// Very bright green
	{ 191, 191, 255 },	// Very bright blue
	{ 255, 255, 191 },	// Very bright yellow
	{ 255, 191, 255 },	// Very bright magenta
	{ 191, 255, 255 },	// Very bright cyan
};


/*int *g_iMuzzleFlashSprites[] =
{
	&g_iModelIndexMuzzleFlash0,
	&g_iModelIndexMuzzleFlash1,
	&g_iModelIndexMuzzleFlash2,
	&g_iModelIndexMuzzleFlash3,
	&g_iModelIndexMuzzleFlash4,
	&g_iModelIndexMuzzleFlash5,
	&g_iModelIndexMuzzleFlash6,
	&g_iModelIndexMuzzleFlash7,
	&g_iModelIndexMuzzleFlash8,
	&g_iModelIndexMuzzleFlash9
};*/

dlight_t *DynamicLight(const Vector &vecPos, float radius, byte r, byte g, byte b, float life, float decay)
{
	if (g_pCvarEffectsDLight->value <= 0.0f)
		return NULL;

	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	if (dl)
	{
		dl->origin = vecPos;
		dl->radius = radius;
		dl->color.r = r;
		dl->color.g = g;
		dl->color.b = b;
		dl->decay = decay;
		dl->die = gEngfuncs.GetClientTime() + life;
	}
	return dl;
}

dlight_t *EntityLight(const Vector &vecPos, float radius, byte r, byte g, byte b, float life, float decay)
{
	if (g_pCvarEffectsDLight->value <= 0.0f)
		return NULL;

	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocElight(0);
	if (dl)
	{
		dl->origin = vecPos;
		dl->radius = radius;
		dl->color.r = r;
		dl->color.g = g;
		dl->color.b = b;
		dl->decay = decay;
		dl->die = gEngfuncs.GetClientTime() + life;
	}
	return dl;
}

//-----------------------------------------------------------------------------
// Purpose: Flashlight effect override, so players can block the light!
// Warning: Assumes that engine uses player indexes for light keys
// Input  : *pEnt - 
//-----------------------------------------------------------------------------
dlight_t *CL_UpdateFlashlight(cl_entity_t *pEnt)
{
	Vector vecSrc, vecEnd, vecForward, view_ofs;
	Vector vecAngles(pEnt->angles);// do not modify original value
	//constructor already sets to 0	VectorClear(view_ofs);

	if (pEnt->index == gEngfuncs.GetLocalPlayer()->index)// cl.playernum)
	{
		//vecForward = g_vecViewForward;
		gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);
	}
	/*else
	{
		vec3_t	v_angle;
		// restore viewangles from angles
		v_angle[PITCH] = -pEnt->angles[PITCH] * 3;
		v_angle[YAW] = pEnt->angles[YAW];
		v_angle[ROLL] = 0; 	// no roll
		AngleVectors(v_angle, vecForward, NULL, NULL);
	}*/

	/*if (pEnt == gHUD.m_pLocalPlayer)
	{
#if defined (SV_NO_PITCH_CORRECTION)
		vecAngles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;// SQB
#else
		vecAngles[PITCH] *= -1.0f;
#endif
	}*/
	DBG_ANGLES_DRAW(14, pEnt->origin, pEnt->angles, pEnt->index, "CL_UpdateFlashlight()");
	AngleVectors(vecAngles, vecForward, NULL, NULL);

	vecSrc = pEnt->origin; vecSrc += view_ofs;//VectorAdd(pEnt->origin, view_ofs, vecSrc);
	//VectorMA(vecSrc, FLASHLIGHT_DISTANCE, forward, vecEnd);
	//vecEnd = vecSrc + vecForward*FLASHLIGHT_DISTANCE;
	vecEnd = vecForward; vecEnd *= FLASHLIGHT_DISTANCE; vecEnd += vecSrc;

	pmtrace_t trace;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);// the main purpose of the entire function
	gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
	int pe_ignore = PM_GetPhysent(pEnt->index);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX|PM_GLASS_IGNORE, pe_ignore, &trace);
	gEngfuncs.pEventAPI->EV_PopPMStates();

	//if (trace.fraction == 1.0f)// XDM3035c: don't
	//	return NULL;

	// EXPERIMENTAL: Idea is great, but looks very bad. Needs glow, not additive overlay.
	/*if (trace.ent)// XDM3037: someone's pointing flashlight at my face
	{
		int hitentindex = gEngfuncs.pEventAPI->EV_IndexFromTrace(&trace);
		if (IsValidPlayerIndex(hitentindex))
		{
			if (hitentindex != pEnt->index && hitentindex == gEngfuncs.GetLocalPlayer()->index)
			{
				vec3_t screen;
				if (gEngfuncs.pTriAPI->WorldToScreen(vecSrc, screen) != 1)// TODO: also check if player is looking at me
					CL_ScreenFade(255,255,255,200, 0.5, 0.0, FFADE_IN);
			}
		}
	}*/
	//TEST	gEngfuncs.pEfxAPI->R_ShowLine(vecSrc, trace.endpos);

	/*float falloff = trace.fraction * FLASHLIGHT_DISTANCE;

	if (falloff < 250.0f)
		falloff = 1.0f;
	else
	{
		falloff = 250.0f / falloff;
		falloff *= falloff;
	}*/
	int	lightkey;
	//if (gEngfuncs.GetMaxClients() <= 1)
	//	lightkey = gEngfuncs.GetLocalPlayer()->index;
	//else
		lightkey = pEnt->index;

	// Engine will nullify and return existing light structure with this key
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(lightkey);
	if (dl)
	{
		//byte br = clamp((byte)(255 * (1.0f-trace.fraction*1.5f)), 0, 255);
		byte br;
		if (trace.fraction == 1.0f)// XDM3036: try to lighen up some space
			br = 63;
		else
			br = 63+(byte)(192.0f * (1.0f - clamp(trace.fraction*1.5f-0.5f, 0.0f, 1.0f)));
			//br = (byte)(255.0f * (1.0f - clamp(trace.fraction*1.5f-0.5f, 0.0f, 1.0f)));

		dl->origin = vecForward; dl->origin *= -8.0f; dl->origin += trace.endpos;
		dl->die = gEngfuncs.GetClientTime() + 0.01f; // die on next frame
		//dl->color.r = dl->color.g = dl->color.b = clamp((byte)(255 * falloff), 0, 255);
		dl->color.r = dl->color.g = dl->color.b = br;
		dl->radius = FLASHLIGHT_RADIUS1 + FLASHLIGHT_RADIUS2*trace.fraction;//72 + (1.0f-);
		//conprintf(0, "light: fr = %g (x1.5=%g), br = %d, r = %g\n", trace.fraction, trace.fraction*1.5f, br, dl->radius);
	}
	return dl;
}

//-----------------------------------------------------------------------------
// Warning: This has different meaning than engine parameter
//-----------------------------------------------------------------------------
/*float FX_GetBubbleSpeed(void)//float height)
{
	return (144.0f + (float)(DEFAULT_GRAVITY-g_cl_gravity));
}*/

//-----------------------------------------------------------------------------
// Purpose: client version
// Input  : *ptr - 
//			&vecSrc - 
//			&vecEnd - 
// Output : char
//-----------------------------------------------------------------------------
char CL_TEXTURETYPE_Trace(pmtrace_t *ptr, const Vector &vecSrc, const Vector &vecEnd)
{
	int entindex = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);
	if (entindex > 0)
	{
		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex(entindex);
		if (pEnt)
			if (pEnt->model)
				if (pEnt->model->name[0] != '*')// not a BSP model
					return CHAR_TEX_NULL;
	}

	char chTextureType = CHAR_TEX_CONCRETE;
	const char *pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture(entindex, vecSrc, vecEnd);

	if (pTextureName)// XDM: HACK to avoid playing 'concrete' sounds when missing
	{
		//conprintf(0, "CL TRACE_TEXTURE returned %s\n", pTextureName);
		// strip leading '-0' or '+0~' or '{' or '!'
		/*if (*pTextureName == '-' || *pTextureName == '+')
			pTextureName += 2;

		if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			pTextureName++;

		chTextureType = PM_FindTextureType(pTextureName);*/
		chTextureType = PM_FindTextureTypeFull(pTextureName);// XDM3038c
	}

	//conprintf(0, "CL_TEXTURETYPE_Trace returning %c\n", chTextureType);
	return chTextureType;
}

//-----------------------------------------------------------------------------
// Purpose: Fore debugging
// Input  : origin -
//			color - palette index
//			life -
//-----------------------------------------------------------------------------
particle_t *DrawParticle(const Vector &origin, short color, float life)
{
	particle_t *p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);
	if (p != NULL)
	{
		VectorCopy(origin, p->org);
		VectorClear(p->vel);
		p->color = color;
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
		p->die += life;
	}
	return p;
}

//-----------------------------------------------------------------------------
// Purpose: Per-particle and per-frame
// Input  : *particle -
//			frametime - frame length
//-----------------------------------------------------------------------------
void ParticleCallback(struct particle_s *particle, float frametime)
{
	if (gHUD.m_iPaused <= 0)// fixes sparks in multiplayer
		particle->org += particle->vel*frametime;
}

//-----------------------------------------------------------------------------
// Purpose: An expanding cloud of particles
// Input  : origin -
//			rnd_vel - scalar velocity
//			color - color index from palette
//			color_range - number of colors in palette to use
//			number - of particles
//			life - in seconds
//			normalize - normalize radius (sphere)
//-----------------------------------------------------------------------------
void ParticlesCustom(const Vector &origin, float rnd_vel, int color, int color_range, size_t number, float life, bool normalize)
{
	particle_t *p = NULL;
	for (size_t i=0; i<number; ++i)
	{
		p = gEngfuncs.pEfxAPI->R_AllocParticle(ParticleCallback);
		if (!p)
			continue;

		p->org = origin;
		VectorRandom(p->vel);
		if (normalize)
		{
			//VectorAdd(VectorRandom(), VectorRandom(), p->vel); ???
			p->vel.NormalizeSelf();
		}
		else
		{
			//VectorRandom(rnd);
			//VectorMultiply(p->vel, rnd, p->vel);
		}

		VectorScale(p->vel, rnd_vel, p->vel);
		//p->vel = (VectorRandom() + VectorRandom()).Normalize()*rnd_vel;
		p->color = color + (short)RANDOM_LONG(0, color_range-1);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
		p->die += life;
	}
}

//-----------------------------------------------------------------------------
// Purpose: smoke trail spawner callback function
// Input  : *ent -
//			frametime - frame length
//			currenttime -
//-----------------------------------------------------------------------------
void TrailCallback(struct tempent_s *ent, float frametime, float currenttime)
{
	if (currenttime < ent->entity.baseline.fuser1)
		return;

	// FIX: temp entity disappears when it hits the sky
	if (ent->entity.origin == ent->entity.attachment[0])
		ent->die = currenttime;
	else
	{
		VectorCopy(ent->entity.origin, ent->entity.attachment[0]);// hack
		/*looks kinda lame
		if (ent->entity.baseline.iuser1)
			gEngfuncs.pEfxAPI->R_SparkStreaks(ent->entity.origin, 24, -100, 100);
			//gEngfuncs.pEfxAPI->R_RocketFlare(ent->entity.origin);
		ent->entity.baseline.iuser1 = !ent->entity.baseline.iuser1;// every second frame
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: particle smoke trail
// Input  : origin - start
//			entindex - entity to follow
//			type - RocketTrail_e:
//				0 thick rocket trail (gray & orange)
//				1 thick smoke trail (gray)
//				2 thick blood trail (dark red)
//				3 V-like two thin trails (gray)
//				4 thick slight blood trail (dark red)
//				5 V-like two thin trails (red & orange)
//				6 thin rocket trail (gray & orange)
//			life - 
//-----------------------------------------------------------------------------
TEMPENTITY *FX_Trail(vec3_t origin, int entindex, unsigned short type, float life)
{
	TEMPENTITY *pTrailSpawner = NULL;
	pTrailSpawner = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel(origin);
	if (pTrailSpawner != NULL)
	{
		pTrailSpawner->flags |= (FTENT_PLYRATTACHMENT | FTENT_NOMODEL | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL/* | FTENT_COLLIDEWORLD*/);
		pTrailSpawner->callback = TrailCallback;
		pTrailSpawner->clientIndex = entindex;
		//pTrailSpawner->entity.trivial_accept = type;
		pTrailSpawner->entity.baseline.usehull = pTrailSpawner->entity.curstate.usehull = type;
		pTrailSpawner->die = gEngfuncs.GetClientTime() + life;
		pTrailSpawner->entity.baseline.fuser1 = gEngfuncs.GetClientTime() + 0.1f;
	}
	return pTrailSpawner;
}

//-----------------------------------------------------------------------------
// Purpose: Spark effect spawner per-frame callback function
// curstate.fuser2 - decay rate
// curstate.fuser3 - real renderamt in float
// Input  : *ent - spark shower emitter
//			frametime - 
//			currenttime - 
//-----------------------------------------------------------------------------
void SparkShowerCallback(struct tempent_s *ent, float frametime, float currenttime)
{
	if (ent->entity.curstate.renderamt > 1)
	{
		if (ent->entity.curstate.renderamt > 48 && frametime > 0.0f)
		{
			//if (ent->entity.curstate.renderamt > 64 && g_pCvarEffects->value > 0)
			//	DynamicLight(ent->entity.origin, 32, 255,200,180, 0.001, 16);

			if (!UTIL_PointIsFar(ent->entity.origin))// XDM3035c
				gEngfuncs.pEfxAPI->R_SparkEffect(ent->entity.origin, (ent->entity.curstate.renderamt > 127)?2:1, -128, 128);

			//if (g_pCvarEffects->value > 0)
			//	gEngfuncs.pEfxAPI->R_RunParticleEffect(ent->entity.origin, ent->entity.ph[2].origin, 192, 32);
		}
		ent->entity.curstate.fuser3 -= (frametime*8.0f) * ent->entity.curstate.fuser2;//RANDOM_LONG(1,3);// XDM3038a
		ent->entity.curstate.renderamt = (int)ent->entity.curstate.fuser3;// XDM3038a: just CAN'T calculate in INT

		if (ent->entity.curstate.scale >= ent->entity.baseline.scale)
			ent->entity.curstate.scale -= frametime*5.0f;// RANDOM_FLOAT(4,6);
		else if (RANDOM_LONG(0,3) == 0)
			ent->entity.curstate.scale = ent->entity.baseline.scale * RANDOM_FLOAT(2,10);
		//ent->entity.curstate.scale = ent->entity.baseline.scale * RANDOM_FLOAT(0.5,10);
	}
	else
	{
		ent->die = currenttime;
		gEngfuncs.pEfxAPI->R_BeamKill(ent->entity.index);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spark effect spawner per-collision callback function
// curstate.health prevents early collisions (inside the explosion)
// curstate.fuser1 prevents often collisions
// Input  : *ent - spark shower emitter
//			*ptr - impact trace
//-----------------------------------------------------------------------------
void SparkShowerHitCallback(struct tempent_s *ent, struct pmtrace_s *ptr)
{
	ent->entity.curstate.health++;
	if (ent->entity.curstate.health > 1 && (gHUD.m_flTime - ent->entity.curstate.fuser1) > 0.25f)
	{
		if (!UTIL_PointIsFar(ent->entity.origin))
		{
			gEngfuncs.pEfxAPI->R_BulletImpactParticles(ent->entity.origin);
			//gEngfuncs.pEfxAPI->R_FlickerParticles(ent->entity.origin);
			//FX_StreakSplash(ent->entity.origin, g_vecZero, gTracerColors[6], 32, 200.0f, true, true, false);
			if (g_pCvarEffects->value > 1.0f)
			{
				//gEngfuncs.pEfxAPI->R_BeamPoints(ptr->endpos, ent->entity.origin, g_iModelIndexLaser, 4.0f, 0.25f, 0.0f, 1.0f, 10.0, 0, 30, 1,0,1);
				DecalTrace(RANDOM_LONG(DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3), ptr, ent->entity.origin);
				//int decalindex = gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName("{smscorch1"));
				//gEngfuncs.pEfxAPI->R_DecalShoot(decalindex, gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr), 0, tr.endpos, 0);
			}
		}
	}
	ent->entity.curstate.fuser1 = gHUD.m_flTime;// last impact time
}

//-----------------------------------------------------------------------------
// Purpose: Client-side spark_shower entity replacement. Glow.
// Input  : model - sprite
//			velocity - somewhere around 640 - 800
//-----------------------------------------------------------------------------
void FX_SparkShower(const Vector &origin, struct model_s *model, size_t count, const Vector &velocity, bool random, float life)
{
	if (!model)
		return;

	TEMPENTITY *pEnt;
	Vector vel;
	Vector dir;
	vec_t fv = velocity.Length();
	for (size_t i = 0; i < count; ++i)
	{
		VectorRandom(dir);// = VectorRandom() + VectorRandom();?
		if (random)
		{
			vel = dir; vel *= fv;//VectorScale(dir, fv, vel);// RANDOM_FLOAT(640, 800)
		}
		else
		{
			vel = velocity;	vel += dir;
		}
		pEnt = gEngfuncs.pEfxAPI->CL_TempEntAlloc(origin, model);
        if (pEnt != NULL)
        {
			pEnt->flags |= FTENT_GRAVITY | FTENT_ROTATE | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_CLIENTCUSTOM;
			if (g_pCvarEffects->value > 0.0f)
				pEnt->hitcallback = SparkShowerHitCallback;// XDM3035c

			pEnt->callback = SparkShowerCallback;
			pEnt->priority = TENTPRIORITY_LOW;
			pEnt->entity.angles.Set(RANDOM_FLOAT(-255,255),RANDOM_FLOAT(-255,255),RANDOM_FLOAT(-255,255));
			pEnt->entity.curstate.angles.Set(RANDOM_FLOAT(-180,180),RANDOM_FLOAT(-180,180),RANDOM_FLOAT(-180,180));
			pEnt->entity.baseline.origin = vel;
			pEnt->entity.baseline.velocity		= pEnt->entity.curstate.velocity = vel;
			pEnt->entity.baseline.scale			= pEnt->entity.curstate.scale = 0.1f;
			pEnt->entity.baseline.rendermode	= pEnt->entity.curstate.rendermode = kRenderGlow;
			pEnt->entity.baseline.renderamt		= pEnt->entity.curstate.renderamt = RANDOM_LONG(240, 255);
			pEnt->entity.baseline.renderfx		= pEnt->entity.curstate.renderfx = kRenderFxNoDissipation;
			pEnt->entity.baseline.framerate		= pEnt->entity.curstate.framerate = RANDOM_LONG(20, 40);
			pEnt->entity.baseline.health		= pEnt->entity.curstate.health = 1;// XDM3035c: bounce counter
			pEnt->entity.baseline.fuser1		= pEnt->entity.curstate.fuser1 = gHUD.m_flTime;
			pEnt->entity.baseline.fuser2		= pEnt->entity.curstate.fuser2 = RANDOM_FLOAT(4.0f, 50.0f);// decay rate
			pEnt->entity.baseline.fuser3		= pEnt->entity.curstate.fuser3 = pEnt->entity.baseline.renderamt;
			if (life > 0.0f)
				pEnt->die = gEngfuncs.GetClientTime() + life;
			else
				pEnt->die = gEngfuncs.GetClientTime() + (model->numframes * 0.1f) + 1.0f;
			// doesn't work for tempents 8(		gEngfuncs.pEfxAPI->R_BeamFollow(pEnt->entity.index, g_iModelIndexSmoke, 2.0, 8.0, 1.0,1.0,1.0,1.0);
		}
		else
			break;// if engine fails to allocate once, all subsequent attempts will fail too
	}
}

//-----------------------------------------------------------------------------
// Fixed smoke effect
//-----------------------------------------------------------------------------
TEMPENTITY *FX_Smoke(const Vector &origin, int spriteindex, float scale, float framerate)
{
	if (spriteindex <= 0)
	{
		conprintf(2, "FX_Smoke() error: bad sprite index %d!\n", spriteindex);
		return NULL;
	}
	if (g_pRenderManager)
	{
		byte gray = (byte)RANDOM_LONG(20,35);
		CRSSprite *pSprite = new CRSSprite(origin, Vector(0,0,4), IEngineStudio.GetModelByIndex(spriteindex), kRenderTransAlpha, gray,gray,gray, 0.0f,0.0f/*useless*/, scale,1.0f, framerate, 0.0f);
		if (pSprite)
		{
			//pSprite->
			g_pRenderManager->AddSystem(pSprite, RENDERSYSTEM_FLAG_NOCLIP, -1, RENDERSYSTEM_FFLAG_DONTFOLLOW);
		}
		return NULL;// old shit
	}
	else
	{
		TEMPENTITY *pTempEnt = gEngfuncs.pEfxAPI->R_DefaultSprite(origin, spriteindex, framerate);
		if (pTempEnt)
		{
			gEngfuncs.pEfxAPI->R_Sprite_Smoke(pTempEnt, scale);
			pTempEnt->entity.origin.z -= 12.0f;
			//pTempEnt->z -= 12.0f;
			pTempEnt->entity.baseline.origin[2] = 4.0f;// HACK for velocity
		}
		return pTempEnt;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Internal core function for placing decals
// Input  : decalindex - Engine decal index for Draw_DecalIndex
//			*pTrace - 
//			*pvecStart - made optional to preserve compatibility
//-----------------------------------------------------------------------------
void FX_DecalTrace(int decalindex, struct pmtrace_s *pTrace, const Vector &vecStart)
{
	if (pTrace == NULL || pTrace->allsolid)
		return;
	if (decalindex <= 0)
	{
		conprintf(2, "FX_DecalTrace() error: bad decal index %d!\n", decalindex);
		return;
	}

	//test	gEngfuncs.pEfxAPI->R_BeamPoints(vecStart, pTrace->endpos, g_iModelIndexLaser, 4.0f, 0.25f, 0.0f, 1.0f, 10.0, 0, 30, 1,0,0);
	char chTex = CL_TEXTURETYPE_Trace(pTrace, vecStart, pTrace->endpos);// XDM3037
	if (chTex == CHAR_TEX_NULL)
		return;

	int entindex = gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace);
	/* TODO: find out: this is done in the engine. probably.
	int modelindex = 0;
	cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex(entindex);
	if (pEnt)
		modelindex = pEnt->curstate.modelindex;// wtf is this for?*/
	gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalindex), entindex, 0, pTrace->endpos, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Internal function
// Input  : decalindex - Engine decal index for Draw_DecalIndex
//			&start - 
//			&end - 
//-----------------------------------------------------------------------------
void FX_DecalTrace(int decalindex, const Vector &start, const Vector &end)
{
	if (decalindex <= 0)
	{
		conprintf(2, "FX_DecalTrace() error: bad decal index %d!\n", decalindex);
		return;
	}
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
	gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, PM_STUDIO_IGNORE, -1, &tr);
	gEngfuncs.pEventAPI->EV_PopPMStates();
	//if (tr.allsolid)
	//	return;

	FX_DecalTrace(decalindex, &tr, start);
	//gEngfuncs.pEfxAPI->R_DecalShoot(textureindex, gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr), 0, tr.endpos, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Simplified, uses existing trace line
// Input  : decal - Game DLL decal like DECAL_GUNSHOT1
//			*pTrace - trace to use
//-----------------------------------------------------------------------------
void DecalTrace(int decal, struct pmtrace_s *pTrace, const Vector &vecStart)
{
	if (decal < 0 || decal >= DECAL_ENUM_SIZE)
	{
		conprintf(2, "DecalTrace() error: bad decal %d!\n", decal);
		return;
	}
	FX_DecalTrace(g_Decals[decal].index, pTrace, vecStart);
	//int texindex = gEngfuncs.pEfxAPI->Draw_DecalIndex(g_Decals[decal].index);
	//gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname)), gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace), 0, pTrace->endpos, 0);
	//gEngfuncs.pEfxAPI->R_DecalShoot(texindex, gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace), 0, pTrace->endpos, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Simplified, uses existing trace line
// Input  : *decalname - find decal by name (SLOW!)
//			*pTrace - trace to use
//-----------------------------------------------------------------------------
/*void DecalTrace(char *decalname, struct pmtrace_s *pTrace)
{
	FX_DecalTrace(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname), pTrace, pTrace->endpos);
}*/

//-----------------------------------------------------------------------------
// Purpose: Draw decal on a surface 
// Input  : decal - Game DLL decal like DECAL_GUNSHOT1
//			&start - trace start
//			&end - trace end
//-----------------------------------------------------------------------------
void DecalTrace(int decal, const Vector &start, const Vector &end)
{
	if (decal < 0 || decal >= DECAL_ENUM_SIZE)
	{
		conprintf(2, "DecalTrace() error: bad decal %d!\n", decal);
		return;
	}
	FX_DecalTrace(g_Decals[decal].index, start, end);
}

//-----------------------------------------------------------------------------
// Purpose: Simplified, uses vectors to build a new trace line
// Input  : *decalname - find decal by name (SLOW!)
//			&start - trace start
//			&end - trace end
//-----------------------------------------------------------------------------
void DecalTrace(char *decalname, const Vector &start, const Vector &end)
{
	FX_DecalTrace(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname), start, end);
	//gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname)), gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr), 0, tr.endpos, 0);
}


//-----------------------------------------------------------------------------
// Purpose: Allocate glow as a temporary entity (because we have no other means to draw glow sprites)
// Note   : Like R_TempSprite, but takes model as a pointer.
// Output : TEMPENTITY
//-----------------------------------------------------------------------------
TEMPENTITY *FX_TempSprite(const Vector &origin, float scale, struct model_s *model, int rendermode, int renderfx, float a, float life, int flags)
{
	if (!model)
		return NULL;
	TEMPENTITY *pSprite = gEngfuncs.pEfxAPI->CL_TempEntAllocHigh(origin, model);
	if (pSprite)
	{
		pSprite->entity.curstate.scale			= pSprite->entity.baseline.scale = scale;
		pSprite->entity.curstate.rendermode		= pSprite->entity.baseline.rendermode = rendermode;
		pSprite->entity.curstate.renderamt		= pSprite->entity.baseline.renderamt = (int)(a*255.0f);
		pSprite->entity.curstate.renderfx		= pSprite->entity.baseline.renderfx = renderfx;
		pSprite->entity.curstate.movetype		= pSprite->entity.baseline.movetype = MOVETYPE_NONE;
		pSprite->entity.curstate.framerate		= pSprite->entity.baseline.framerate = 10.0f;
		pSprite->flags |= flags;
		if (life > 0.0f)
			pSprite->die = gEngfuncs.GetClientTime() + life;
		else
			pSprite->die = gEngfuncs.GetClientTime() + (model->numframes * 0.1f) + 1.0f;
	}
	return pSprite;
}


// TODO: FX_Tracer

// XDM3038c: Add some variance
bool OnParticleInitializeRandomizeColor(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &fInterpolaitonMult)
{
	// cannot become < 0 anyway 
	pParticle->m_fColor[0] = min(pParticle->m_fColor[0] * RANDOM_FLOAT(0.95f, 1.05f), 1.0f);
	pParticle->m_fColor[1] = min(pParticle->m_fColor[1] * RANDOM_FLOAT(0.95f, 1.05f), 1.0f);
	pParticle->m_fColor[2] = min(pParticle->m_fColor[2] * RANDOM_FLOAT(0.95f, 1.05f), 1.0f);
	pParticle->m_fColor[3] = min(pParticle->m_fColor[3] * RANDOM_FLOAT(0.95f, 1.05f), 1.0f);
	return true;
}

//-----------------------------------------------------------------------------
// R_StreakSplash replacement color24 version
//-----------------------------------------------------------------------------
RS_INDEX FX_StreakSplash(const Vector &pos, const Vector &dir, color24 color, int count, float velocity, bool gravity, bool clip, bool bounce)
{
	return FX_StreakSplash(pos, dir, color.r,color.g,color.b, count, velocity, gravity, clip, bounce);
}

//-----------------------------------------------------------------------------
// R_StreakSplash replacement RGB version
// Warning: Do not overuse these!
//-----------------------------------------------------------------------------
RS_INDEX FX_StreakSplash(const Vector &pos, const Vector &dir, byte r, byte g, byte b, int count, float velocity, bool gravity, bool clip, bool bounce)
{
	if (g_pRenderManager == NULL)
	{
		gEngfuncs.pEfxAPI->R_StreakSplash(pos, dir, 0, count, 1.0f, -velocity, velocity);
		return 0;
	}
	//float velocity = fabs(0.5f*(velocityMin + velocityMax));
	CPSSparks *pSys = new CPSSparks(count, pos, 1.0f,-0.02f, 0.0f, velocity, 1.0f, r,g,b, 1.0f, -1.5f, g_pSpritePTracer, kRenderTransAdd, RANDOM_FLOAT(1.0f, 1.5f));
	if (pSys)
		pSys->m_OnParticleInitialize = OnParticleInitializeRandomizeColor;

	return g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_SIMULTANEOUS|(clip?0:RENDERSYSTEM_FLAG_NOCLIP)|(bounce?RENDERSYSTEM_FLAG_ADDPHYSICS:RENDERSYSTEM_FLAG_CLIPREMOVE)|RENDERSYSTEM_FLAG_LOOPFRAMES|(gravity?RENDERSYSTEM_FLAG_ADDGRAVITY:0));
	//return 1;//	return g_pRenderManager->AddSystem(new CPSStreaks(count, pos, dir, speed, velocityMin, velocityMax, 0.05f, 1.0f, 0.1f, color, 1.0f, -0.01f, g_iModelIndexWhite, kRenderTransAdd, RANDOM_FLOAT(1.0f, 1.5f)));;
}

/*void PMFX_WaterSplash(playermove_t *pmove)
{
}

// footsteps, dust, etc.
void PMFX_Land(playermove_t *pmove)
{
}*/

//-----------------------------------------------------------------------------
// Purpose: Draw muzzle flash sprite by normal sprite index
// Input  : &pos - 
//			entindex - entity to follow
//			sprite_index - 
//			scale - 
// Output : RS_INDEX
//-----------------------------------------------------------------------------
RS_INDEX FX_MuzzleFlashSprite(const Vector &pos, int entindex, short attachment, struct model_s *pSprite, float scale, bool rotation)
{
	if (g_pRenderManager == NULL)
		return 0;
	if (pSprite == NULL)
		return 0;

	CRSSprite *pSys = new CRSSprite(pos, g_vecZero, pSprite, kRenderTransAdd, 255,255,255, 1.0f,0.0f, scale, 0.25f, 20.0f, 0.002f);
	if (pSys)
	{
		if (rotation)
			pSys->m_vecAngles.z = RANDOM_FLOAT(0.0f, 359.9f);

		pSys->m_iFollowAttachment = attachment;
		/*pSys->m_iFollowEntity = entindex;
		pSys->FollowEntity();// trick to keep offset if pos was specified outside of entity origin
		pSys->m_vecOffset = pos - pSys->m_vecOrigin;*/
		return g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_NOCLIP, entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE|RENDERSYSTEM_FFLAG_ICNF_NODRAW);//|RENDERSYSTEM_FFLAG_USEOFFSET);
	}
	return 0;
}

/*struct bubbleparams_s
{
	Vector mins;
	Vector maxs;
};*/

//-----------------------------------------------------------------------------
// Purpose: Called by CParticleSystem::InitializeParticle in the last place
// Input  : *pSystem - 
//			*pParticle - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool FX_Bubbles_OnParticleInitialize(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &fInterpolaitonMult)
{
	Vector v;
	VectorRandom(v, *(Vector *)pData);// appear in a specified volume
	pParticle->m_vPos += v;
	//if (((CPSFlameCone *)pSystem)->m_flRandomDir)
	//	pParticle->m_vVel *= 0.25f;

	pParticle->m_vVel.z += FX_GetBubbleSpeed();
	pParticle->m_vAccel.Set(0.0f,0.0f,1.0f);
	pParticle->m_fSizeX *= RANDOM_FLOAT(0.1f, 2.0f);
	pParticle->m_fSizeY = pParticle->m_fSizeX;
	return true;
}*/

RS_INDEX FX_BubblesPoint(const Vector &center, const Vector &spread, struct model_s *pTexture, int count, float speed)
{
	if (g_pRenderManager == NULL)
		return 0;

	return g_pRenderManager->AddSystem(new CPSBubbles(count, PSSTARTTYPE_POINT, center, spread, speed, pTexture, kRenderTransAlpha, 1.0f, 0.0f, BUBBLE_SCALE, 0.0f, BUBBLE_LIFE),
		RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_SIMULTANEOUS/*|RENDERSYSTEM_FLAG_INCONTENTSONLY*/, -1);
}

RS_INDEX FX_BubblesSphere(const Vector &center, float fRadiusMin, float fRadiusMax, struct model_s *pTexture, int count, float speed)
{
	if (g_pRenderManager == NULL)
		return 0;

	return g_pRenderManager->AddSystem(new CPSBubbles(count, PSSTARTTYPE_SPHERE, center, Vector(fRadiusMin,fRadiusMax,fRadiusMax), speed, pTexture, kRenderTransAlpha, 1.0f, 0.0f, BUBBLE_SCALE, 0.0f, BUBBLE_LIFE),
		RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_SIMULTANEOUS/*|RENDERSYSTEM_FLAG_INCONTENTSONLY*/, -1);
}

RS_INDEX FX_BubblesBox(const Vector &center, const Vector &halfbox, struct model_s *pTexture, int count, float speed)
{
	if (g_pRenderManager == NULL)
		return 0;

	return g_pRenderManager->AddSystem(new CPSBubbles(count, PSSTARTTYPE_BOX, center, halfbox, speed, pTexture, kRenderTransAlpha, 1.0f, 0.0f, BUBBLE_SCALE, 0.0f, BUBBLE_LIFE),
		RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_SIMULTANEOUS/*|RENDERSYSTEM_FLAG_INCONTENTSONLY*/, -1);
}

RS_INDEX FX_BubblesLine(const Vector &start, const Vector &end, struct model_s *pTexture, int count, float speed)
{
	if (g_pRenderManager == NULL)
		return 0;

	return g_pRenderManager->AddSystem(new CPSBubbles(count, PSSTARTTYPE_LINE, start, end, speed, pTexture, kRenderTransAlpha, 1.0f, 0.0f, BUBBLE_SCALE, 0.0f, BUBBLE_LIFE),
		RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_SIMULTANEOUS/*|RENDERSYSTEM_FLAG_INCONTENTSONLY*/, -1);
}

// UNDONE TODO
// Replacement for R_BloodSprite( float * org, int colorindex, int modelIndex, int modelIndex2, float size );
RS_INDEX FX_BloodSpray(const Vector &start, const Vector &direction, struct model_s *pTexture, uint32 numparticles, uint32 color4b, float scale)
{
	if (g_pRenderManager == NULL)
		return 0;

	CParticleSystem *pSystem = new CParticleSystem(numparticles, start, direction, pTexture, kRenderTransAdd, 1.0f);
	if (pSystem)
		pSystem->m_color.Set(color4b);

	return g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_ADDGRAVITY);
}


#define NUMVERTEXNORMALS	162
static const float vdirs[NUMVERTEXNORMALS][3] =
{
#include "anorms.h"
};

//-----------------------------------------------------------------------------
// Purpose: Custom per-particle update function for flame particles
// Note   : Should copy CFlameCloud::Think()
// Input  : *pSystem - 
//			*pParticle - 
//			*pData - 
//			&time - 
//			&elapsedTime - 
// Output : unused
//-----------------------------------------------------------------------------
bool FX_DisplacerBall_OnParticleUpdate0(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	//VectorMA(m_vecOrigin, dist, (float *)vdirs[i], m_pParticles[i]->org);
	//pParticle->m_vPos = pSystem->m_vecOrigin + dist * vdirs[i];
	pParticle->m_vPos = vdirs[pParticle->index];
	pParticle->m_vPos *= sinf(time*pSystem->m_fParticleSpeedMax + pParticle->index)*pSystem->m_fStartRadiusMax;
	pParticle->m_vPos += pSystem->m_vecOrigin;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Custom per-particle update function for flame particles
// Note   : Should copy CFlameCloud::Think()
// Input  : *pSystem - 
//			*pParticle - 
//			*pData - 
//			&time - 
//			&elapsedTime - 
// Output : unused
//-----------------------------------------------------------------------------
bool FX_DisplacerBall_OnParticleUpdate1(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	if (pData == NULL)
		return false;

	float s1, s2, c1, c2;
	//Vector dir;

	//SinCos(time*1.5f + pSystem->m_vecAng[pParticle->index][0], &s1, &c1);
	SinCos(time*pSystem->m_fParticleSpeedMax + ((Vector2D *)pData)[pParticle->index].x, &s1, &c1);
	//dir[0] = s*c;
	//SinCos(time*1.5f + pSystem->m_vecAng[pParticle->index][1], &s2, &c2);
	SinCos(time*pSystem->m_fParticleSpeedMax + ((Vector2D *)pData)[pParticle->index].y, &s2, &c2);
	//dir[1] = s*c;
	//SinCos(time*1.5f + pSystem->m_vecAng[pParticle->index][2], &s, &c);
	//dir[2] = s*c;

	/*dir[0] = c1*c2;
	dir[1] = s1*c2;
	dir[2] = -s2;*/

	//pParticle->m_vPos = dir;
	pParticle->m_vPos.Set(c1*c2, s1*c2, -s2);
	pParticle->m_vPos *= pSystem->m_fStartRadiusMax;
	pParticle->m_vPos += pSystem->m_vecOrigin;
	//pParticle->m_vPos = pSystem->m_vecOrigin + pSystem->m_fStartRadiusMax * 0.6f * dir;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Replacement for the old engine-made effect
// Input  : origin - 
//			radius - 
//			type - 0/1
//			timetolive - 0
//			r,g,b - 
// Output : RS_INDEX
//-----------------------------------------------------------------------------
RS_INDEX FX_DisplacerBallParticles(const Vector &origin, int followentindex, float radius, byte type, float timetolive, byte r, byte g, byte b)
{
	if (g_pRenderManager == NULL)
		return 0;

	// OLD g_pRenderManager->AddSystem(new CRSTeleparts(args->origin, args->fparam1, args->bparam2, args->bparam1, args->fparam2, 31,args->bparam1==0?255:127,args->bparam1==0?127:255), RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);//RENDERSYSTEM_FFLAG_ICNF_NODRAW - Bad idea because RS quickly finds another entity with same index
	CParticleSystem *pSystem = new CParticleSystem(NUMVERTEXNORMALS, origin, origin, g_pSpritePSparks, kRenderTransAdd, timetolive);
	if (pSystem)
	{
		ASSERT(pSystem->IsRemoving() == false);
		pSystem->m_fScale = 0.03125f;//0.0625 ?
		//pSystem->m_fFrameRate = 20;
		pSystem->m_color.Set(r,g,b);
		pSystem->m_iStartType = PSSTARTTYPE_POINT;//PSSTARTTYPE_SPHERE; replaced anyway
		pSystem->m_iMovementType = PSMOVTYPE_OUTWARDS;
		/*pSystem->m_fStartRadiusMin = */pSystem->m_fStartRadiusMax = radius;
		/*pSystem->m_fParticleSpeedMin = */pSystem->m_fParticleSpeedMax = 0;
		if (type == 0)
		{
			pSystem->m_fParticleSpeedMax = 1.0f;
			pSystem->m_OnParticleUpdate = FX_DisplacerBall_OnParticleUpdate0;
		}
		else if (type == 1)
		{
			pSystem->m_fParticleSpeedMax = 1.5f;
			Vector2D *pAngVectors = new Vector2D[NUMVERTEXNORMALS];
			if (pAngVectors)
			{
				for (size_t i=0; i<NUMVERTEXNORMALS; ++i)
				{
					pAngVectors[i].x = RANDOM_FLOAT(-M_PI_F, M_PI_F);
					pAngVectors[i].y = RANDOM_FLOAT(-M_PI_F, M_PI_F);
				}
				pSystem->m_pOnParticleUpdateData = pAngVectors;
				pSystem->m_bOnParticleUpdateDataFree = true;
				pSystem->m_OnParticleUpdate = FX_DisplacerBall_OnParticleUpdate1;
			}
			else
				conprintf(1, "FX_DisplacerBallParticles() ERROR: failed to allocate Vector2D array!\n");
		}
	}
	else
		conprintf(1, "FX_DisplacerBallParticles() error: unable to create CParticleSystem!\n");

	return g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_HARDSHUTDOWN|RENDERSYSTEM_FLAG_SIMULTANEOUS, followentindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE, true);
}
