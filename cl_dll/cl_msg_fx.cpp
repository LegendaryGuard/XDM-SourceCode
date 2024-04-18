#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "cl_fx.h"
#include "shared_resources.h"
#include "con_nprint.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "eventscripts.h"
#include "decals.h"
#include "damage.h"
#include "shake.h"
#include "studio.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeam.h"
#include "RSModel.h"
#include "RSSprite.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "PSBeam.h"
#include "PSDrips.h"
#include "PSFlameCone.h"
#include "PSBubbles.h"
#include "PSFlatTrail.h"
#include "PSSparks.h"
#include "PSCustom.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int __MsgFunc_ViewModel(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	cl_entity_t *pView = gEngfuncs.GetViewModel();
	if (pView)
	{
		pView->curstate.rendermode		= READ_BYTE();
		pView->curstate.renderfx		= READ_BYTE();
		pView->curstate.rendercolor.r	= READ_BYTE();
		pView->curstate.rendercolor.g	= READ_BYTE();
		pView->curstate.rendercolor.b	= READ_BYTE();
		pView->curstate.renderamt		= READ_BYTE();
		pView->curstate.skin			= READ_BYTE();
	}
	// don't display errors	END_READ();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int __MsgFunc_Particles(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	Vector origin;
	READ_COORD3(origin);
	float rnd_vel= (float)READ_SHORT()*0.1f;
	float life = (float)READ_SHORT()*0.1f;
	byte color = READ_BYTE();
	byte number = READ_BYTE();
	END_READ();
	ParticlesCustom(origin, rnd_vel, color, 0, number, life);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Obsolete: Legacy: XDM3030 compatibility
//-----------------------------------------------------------------------------
int __MsgFunc_PartSys(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (!g_pRenderManager)
		return 1;

	Vector origin, v2, v3;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	READ_COORD3(v2);
	READ_COORD3(v3);// spread for flamecone, size for sparks and direction for drips
	short sprindex	= READ_SHORT();
	byte r_mode		= READ_BYTE();
	byte type		= READ_BYTE();
	short maxpart	= READ_SHORT();
	float life		= (float)READ_SHORT()*0.1f;
	short flags		= READ_SHORT();// XDM3035: extended
	short entindex	= READ_SHORT();
	// TODO: server defines unique ID for future use	short sys_uid =	READ_SHORT();
	END_READ();
	DBG_PRINTF("__MsgFunc_%s(%d): ei %hd, spr %hd, parts %hd, life %g, flags %hd, type %hd\n", pszName, iSize, entindex, sprindex, maxpart, life, flags, (short)type);
	CParticleSystem *pSystem = NULL;

	if (type == PARTSYSTEM_TYPE_REMOVEANY)// XDM3035: !!!
	{
		// XDM3035c: kill all? :P
		//while (psystem = (CParticleSystem *)g_pRenderManager->FindSystemByFollowEntity(entindex))
		pSystem = (CParticleSystem *)g_pRenderManager->FindSystemByFollowEntity(entindex);// FIFO: first in list will be found and removed
		if (pSystem)// found
		{
			pSystem->ShutdownSystem();
			pSystem->m_fDieTime = gEngfuncs.GetClientTime();
		}
	}
	else
	{
		//conprintf(1, "PartSys origin = %f %f %f\n", origin[0], origin[1], origin[2]);
		model_t *pSprite = IEngineStudio.GetModelByIndex(sprindex);
		if (pSprite)
		{
			switch (type)
			{
			case PARTSYSTEM_TYPE_SYSTEM:	pSystem = new CParticleSystem(maxpart, origin, v2, pSprite, r_mode, life); break;
			case PARTSYSTEM_TYPE_FLAMECONE:	pSystem = new CPSFlameCone(maxpart, origin, v2, v3, 300.0f, pSprite, r_mode, 1.0f, -1.5f, 1.0f, (flags & RENDERSYSTEM_FLAG_16)?50.0f:0.0f, life); break;// TODO: transmit scaledelta at least as some flag!
			case PARTSYSTEM_TYPE_BEAM:		pSystem = new CPSBeam(maxpart, origin, v2, pSprite, r_mode, life); break;
			case PARTSYSTEM_TYPE_SPARKS:	pSystem = new CPSSparks(maxpart, origin, v3[0], v3[1], 0.5f, v3[2], 1.0f, 255,255,255,1.0f,-1.0f, pSprite, r_mode, life); break;
			}
			if (pSystem)
			{
				_snprintf(pSystem->m_szName, RENDERSYSTEM_USERNAME_LENGTH, "%s_e%hd", pszName, entindex);
				g_pRenderManager->AddSystem(pSystem, flags, entindex);
			}
			else
				conprintf(0, "MsgFunc_%s ERROR: unable to create particle system!\n", pszName);
		}
		else
			conprintf(1, "MsgFunc_%s ERROR: unable to get texture by index (%d)!\n", pszName, sprindex);
	}
	return 1;
}

// RSPreInitAction current kv parsing mode
enum
{
	RSKVREAD_NOTHING = 0,
	RSKVREAD_KEY,
	RSKVREAD_VALUE
};

//-----------------------------------------------------------------------------
// Purpose: Callback for LoadRenderSystems() and FindLoadedSystems()
// Can set system state and parse server keys into system
// Input  : *pSystem - current system
//			*pData1 - int command
//			*pData2 - char *keys
//-----------------------------------------------------------------------------
void RSPreInitAction(CRenderSystem *pSystem, void *pData1, void *pData2)
{
	int command = *(int *)pData1;
	if (command == MSGRS_DESTROY)
	{
		pSystem->ShutdownSystem();
	}
	else
	{
		if (command == MSGRS_DISABLE)
		{
			pSystem->SetState(RSSTATE_DISABLED);// already
		}
		else if (command == MSGRS_ENABLE)
		{
			pSystem->SetState(RSSTATE_ENABLED);// already
		}
		// parse custom key:value; pairs from the message
		const char *c = (char *)pData2;
		if (c && *c)
		{
			DBG_PRINTF("RSPreInitAction(%s): custom data: \"%s\"\n", pSystem->GetClassName(), c);
			uint32 readmode = RSKVREAD_KEY;
			char kname[ENVRS_MAX_KEYLEN], kvalue[ENVRS_MAX_KEYLEN];
			char *dest = kname;
			size_t len = strlen(c);
			size_t dest_offset = 0;
			bool error = false;
			for (size_t i=0; i<=len; ++i)// "k1:v1;k2:v2;...kN:vN\0"
			{
				if (*c == ':')// key/value separator
				{
					if (readmode == RSKVREAD_KEY)
					{
						if (dest)
						{
							dest[dest_offset] = 0;// terminate
							//dest = NULL;
						}
						else
						{
							conprintf(1, "RSPreInitAction(%s): RSKVREAD_KEY: no destination!\n", pSystem->GetClassName());
							error = true;
							break;
						}
						// start reading the value next time
						readmode = RSKVREAD_VALUE;
						dest = kvalue;
						dest_offset = 0;
					}
					else
					{
						conprintf(1, "RSPreInitAction(%s): unexpected read mode: %u!\n", pSystem->GetClassName(), readmode);
						error = true;
						break;
					}
				}
				else if (*c == ';' || *c == '\0')// pair separator
				{
					if (readmode == RSKVREAD_VALUE)
					{
						if (dest)
						{
							dest[dest_offset] = 0;// terminate
							//dest = NULL;
						}
						else
						{
							conprintf(1, "RSPreInitAction(%s): RSKVREAD_VALUE: no destination!\n", pSystem->GetClassName());
							error = true;
							break;
						}
						// start reading key name next time
						readmode = RSKVREAD_KEY;
						dest = kname;
						dest_offset = 0;

						//conprintf(2, "RSPreInitAction(): ParseKeyValue(%s, %s);\n", kname, kvalue);
						pSystem->ParseKeyValue(kname, kvalue);
					}
					else
					{
						conprintf(1, "RSPreInitAction(%s): unexpected read mode: %u!\n", pSystem->GetClassName(), readmode);
						error = true;
						break;
					}
				}
				else// no special symbols found, copy actual data
				{
					if (dest)
					{
						if (dest_offset < ENVRS_MAX_KEYLEN)
						{
							dest[dest_offset] = *c;
							++dest_offset;
						}
						else
						{
							conprintf(1, "RSPreInitAction(%s): error: key is too long!\n", pSystem->GetClassName());
							error = true;
							break;
						}
					}
#if defined (_DEBUG)
					else
					{
						conprintf(1, "RSPreInitAction(%s): error: no destination!\n", pSystem->GetClassName());
						error = true;
						break;
					}
#endif
				}
				c++;
			}
			if (error)
				conprintf(1, "RSPreInitAction(%s): error detected while parsing server message data!\n", pSystem->GetClassName());
		}
	}
}

uint32 g_iMsgRS_UID_Postfix = 0;// TODO: UNDONE: rewrite this! BAD! Must not be global for all systems! But we use this to avoid duplicate UID conflicts.

//-----------------------------------------------------------------------------
// Purpose: XDM3038b: Load render systems from a file
// Eventually this may replace rain, particles, static models, etc.
//-----------------------------------------------------------------------------
int __MsgFunc_RenderSys(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (g_pRenderManager == NULL)
		return 1;

	Vector origin;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	int emitterindex		= READ_SHORT();
	int targetindex			= READ_SHORT();
	int targetmodelindex	= READ_SHORT();
	int attachment			= READ_BYTE();
	int iFxLevel			= READ_BYTE();
	//float timetolive		= READ_SHORT()*0.1f;// UNDONE: when restoring, lifetime of a system may need to be shortened
	int command				= READ_BYTE();
	unsigned short sysflags	= READ_WORD();
	//char *filename		= READ_STRING();
	char *keys = NULL;
	char filename[RENDERSYSTEM_UID_LENGTH];
	if (READ_STRING(filename, RENDERSYSTEM_UID_LENGTH) < RENDERSYSTEM_UID_LENGTH)// entire buffer was filled, possible remaining bytes are NOT of this string
	{
		if (READ_REMAINING() > 0)
			keys = READ_STRING();
	}
	int systemstate = RSSTATE_ENABLED;
	// now do stuff
	if (command == MSGRS_DISABLE)
		systemstate = RSSTATE_DISABLED;
	else if (command == MSGRS_ENABLE)
		systemstate = RSSTATE_ENABLED;
	else if (command == MSGRS_CREATE)
		systemstate = RSSTATE_ENABLED;
	else if (command == MSGRS_DESTROY)
		systemstate = RSSTATE_DISABLED;
	else
	{
		conprintf(0, "__MsgFunc_%s(%s): bad command %d!\n", pszName, filename, command);
		return 1;
	}
	uint32 numfound = 0;
	//if (command != MSGRS_CREATE)// find, update, toggle, etc.
		numfound = FindLoadedSystems(filename, emitterindex, targetindex, attachment, systemstate, FINDRS_UPDATE_EVERYTHING, RSPreInitAction, &command, keys);

	if (numfound == 0 || command == MSGRS_CREATE)// nothing found
	{
		if (command == RSSTATE_ENABLED || command == MSGRS_CREATE)//if (!(flags & RENDERSYSTEM_FLAG_NODRAW))// enable/CREATE
		{
			if (g_iMsgRS_UID_Postfix == 0)
				g_iMsgRS_UID_Postfix = numfound;
			else
				g_iMsgRS_UID_Postfix++;

			LoadRenderSystems(filename, origin, emitterindex, targetindex, targetmodelindex, attachment, iFxLevel, sysflags, 0.0f, g_iMsgRS_UID_Postfix, RSPreInitAction, &command, keys);
		}
		else
			conprintf(1, "__MsgFunc_%s(%s): no render systems found!\n", pszName, filename);
	}
	END_READ();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int __MsgFunc_DLight(const char *pszName, int iSize, void *pbuf)
{
	Vector origin;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	short radius	= READ_SHORT();
	byte r			= READ_BYTE();
	byte g			= READ_BYTE();
	byte b			= READ_BYTE();
	byte minlight	= READ_BYTE();
	byte decay		= READ_BYTE();
	byte dark		= READ_BYTE();
	short life		= READ_SHORT();
	END_READ();

	//conprintf(1, "MsgFunc_DLight: rad %d, r %d, g %d, b %d, ml %d, dc %d, dr %d, lf %d\n", radius,r,g,b,minlight,decay,dark,life);
	dlight_t *pLight = DynamicLight(origin, (float)radius*0.1f, r,g,b, (float)life*0.1f, (float)decay*0.1f);
	if (pLight)
	{
		pLight->minlight = (float)minlight;

		if (dark > 0)
			pLight->dark = true;
		else
			pLight->dark = false;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int __MsgFunc_ELight(const char *pszName, int iSize, void *pbuf)
{
	Vector origin;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	short radius	= READ_SHORT();
	byte r			= READ_BYTE();
	byte g			= READ_BYTE();
	byte b			= READ_BYTE();
	byte minlight	= READ_BYTE();
	byte decay		= READ_BYTE();
	byte dark		= READ_BYTE();
	short life		= READ_SHORT();
	END_READ();

	dlight_t *pLight = gEngfuncs.pEfxAPI->CL_AllocElight(0);
	if (pLight)
	{
		VectorCopy(origin, pLight->origin);
		pLight->radius = (float)radius * 0.1f;
		pLight->color.r = r;
		pLight->color.g = g;
		pLight->color.b = b;
		pLight->minlight = (float)minlight * 0.1f;
		pLight->decay = (float)decay * 0.1f;
		if (dark > 0)
			pLight->dark = true;
		else
			pLight->dark = false;

		pLight->die = gEngfuncs.GetClientTime() + (float)life * 0.1f;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int __MsgFunc_SetFog(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	byte cr = READ_BYTE();
	byte cg = READ_BYTE();
	byte cb = READ_BYTE();
	float startdist = (float)READ_SHORT();
	float enddist = (float)READ_SHORT();
	byte mode = READ_BYTE();
	END_READ();

	//conprintf(1, ">>>> CL RECV: SetFog %d %d %d, %f, %f, %d\n", cr,cg,cb, startdist, enddist, mode);
	gHUD.m_iFogMode = mode;

	if (mode > 0)
		RenderFog(cr,cg,cb, startdist, enddist, false);
	else
		ResetFog();

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int __MsgFunc_SetSky(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(gHUD.m_vecSkyPos);
	gHUD.m_iSkyMode = READ_BYTE();
	END_READ();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: env_rain msg, legacy
// XDM3038: update: now system can be enabled and disabled any time
//-----------------------------------------------------------------------------
int __MsgFunc_SetRain(const char *pszName, int iSize, void *pbuf)
{
	if (!g_pRenderManager)
		return 1;

	Vector v;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(v);
	short entindex	= READ_SHORT();// just an index, GetModelByIndex() will not find entity with EF_NODRAW on client side
	short mdlindex	= READ_SHORT();// transmit this separately (mins/maxs)
	short sprindex1	= READ_SHORT();
	short sprindex2	= READ_SHORT();
	short sprindex3	= READ_SHORT();// XDM3035c
	short maxpart	= READ_SHORT();
	float life		= (float)READ_SHORT()*0.1f;
	float scalex	= (float)READ_BYTE()*0.1f;
	float scaley	= (float)READ_BYTE()*0.1f;
	byte r_mode		= READ_BYTE();
	byte r			= READ_BYTE();
	byte g			= READ_BYTE();
	byte b			= READ_BYTE();
	byte a			= READ_BYTE();
	short flags		= READ_SHORT();// XDM3035: extended
	END_READ();
	DBG_PRINTF("__MsgFunc_%s(%d): ei %hd, mdl %hd, parts %hd, life %g, flags %hd\n", pszName, iSize, entindex, mdlindex, maxpart, life, flags);
	if (entindex <= 0)
	{
		conprintf(1, "CL: %s: bad entindex!\n", pszName);
		return 1;
	}

	//conprintf(1, "CL: %s: entindex: %d modelindex: %d\n", pszName, entindex, mdlindex);
	CPSDrips *pSystem = NULL;// search for system even if the entindex is not valid
	//pSystem = (CPSDrips *)g_pRenderManager->FindSystemByFollowEntity(entindex);// don't allow more than one system per entity (in case something goes out of sync)
	char uid[RENDERSYSTEM_UID_LENGTH];// XDM3038b: now we find system reliably by UID
	_snprintf(uid, RENDERSYSTEM_UID_LENGTH, "%s%d", pszName, entindex);// Warning! message name can take up to 12 characters!

	bool bShutDown = (mdlindex == 0);

	pSystem = (CPSDrips *)g_pRenderManager->FindSystemByUID(uid);
	if (pSystem != NULL && strcmp(pSystem->GetClassName(), "CPSDrips")==0)// found
	{
		conprintf(2, "CL: %s: found system: uid:%s ei:%d mdl:%d\n", pszName, uid, entindex, mdlindex);
		//if (type == PARTSYSTEM_TYPE_REMOVEANY)// XDM3035: !!!
		if (bShutDown)//if (mdlindex == 0 && sprindex1 == 0 && sprindex2 == 0)// XDM3035: !!! since we don't transmit system type
		{
			conprintf(2, "%s: disabling system %u.\n", pszName, pSystem->GetIndex());
			pSystem->SetState(RSSTATE_DISABLED);
			// XDM3038b: psystem->ShutdownSystem();
			// XDM3038b: psystem->m_fDieTime = gEngfuncs.GetClientTime();
		}
		else// if (psystem->IsShuttingDown())
		{
			if (pSystem->GetState() == RSSTATE_DISABLED)
			{
				conprintf(2, "%s: enabling system %u.\n", pszName, pSystem->GetIndex());
				pSystem->SetState(RSSTATE_ENABLED);
			}
			else
				conprintf(2, "%s: updating system %u.\n", pszName, pSystem->GetIndex());

			if (sprindex1 > 0)
				pSystem->InitTexture(IEngineStudio.GetModelByIndex(sprindex1));

			pSystem->m_color.Set(r,g,b,a);
			pSystem->m_iRenderMode = r_mode;
			pSystem->m_iFlags = flags;
			pSystem->m_fSizeX = scalex;
			pSystem->m_fSizeY = scaley;
			if (life <= 0.0f)
				pSystem->m_fDieTime = 0.0f;
			else
				pSystem->m_fDieTime = gEngfuncs.GetClientTime() + life;
		}
		return 1;
	}
	//else
	// looks like an error :)	conprintf(1, "CL: %s: system not found: uid:%s ei:%d mdl:%d\n", pszName, uid, entindex, mdlindex);

	if (bShutDown)
	{
		conprintf(1, "CL: %s WARNING: unable to find Particle System by UID %s!\n", pszName, uid);
		return 1;
	}

	model_s *pModel = IEngineStudio.GetModelByIndex(mdlindex);
	if (!pModel)
	{
		conprintf(1, "CL: %s WARNING: unable to get model by index %d!\n", pszName, mdlindex);
		return 1;
	}
	if (pModel->type != mod_brush)
	{
		conprintf(1, "CL: %s WARNING: SetRain: %d is not a brush model!\n", pszName, mdlindex);
		//return 1;
	}

	conprintf(1, "CL: %s: creating %d %d\n", pszName, entindex, mdlindex);

	if (g_pRenderManager)
	{
		Vector org(0,0,0);
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex(entindex);
		if (ent != NULL)
			org = ent->curstate.origin;

		// FAIL: it appears HL doesn't hold any brush information except size.
		//conprintf(1, "mins %f %f %f  maxs %f %f %f\n", pModel->mins[0], pModel->mins[1], pModel->mins[2], pModel->maxs[0], pModel->maxs[1], pModel->maxs[2]);
		pSystem = new CPSDrips(maxpart, org, pModel->mins, pModel->maxs, v, IEngineStudio.GetModelByIndex(sprindex1), IEngineStudio.GetModelByIndex(sprindex2), IEngineStudio.GetModelByIndex(sprindex3), r_mode, scalex, scaley,/*2.0, 32.0,*/ 0.0f, life);
		if (pSystem)
		{
			pSystem->m_color.Set(r,g,b,a);
			_snprintf(pSystem->m_szName, RENDERSYSTEM_USERNAME_LENGTH, "%s_e%hd", pszName, entindex);
			if (g_pRenderManager->AddSystem(pSystem, flags | RENDERSYSTEM_FLAG_LOOPFRAMES, entindex) != RS_INDEX_INVALID)// XDM3038c: loop frames
				ASSERT(pSystem->SetUID(uid) == true);// XDM3038b
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: beam rifle effect
//-----------------------------------------------------------------------------
int __MsgFunc_FireBeam(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	Vector vecSrc, vecEnd;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(vecSrc);
	READ_COORD3(vecEnd);
	short spr_ring =READ_SHORT();
	short spr_beam =READ_SHORT();
	byte flags =	READ_BYTE();// right now it's just firemode
	END_READ();

	if (g_pRenderManager)
		g_pRenderManager->AddSystem(new CRSBeam(vecSrc, vecEnd, IEngineStudio.GetModelByIndex(spr_beam), kRenderTransAdd, 80,80,255, 1.0,-1.2, (flags > 0)?2.0:1.5, (flags > 0)?1.0:-0.4, 20.0f, 0.0), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES);
	if (flags > 0)
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CPSFlatTrail(vecSrc, vecEnd, IEngineStudio.GetModelByIndex(spr_ring), kRenderTransAdd, 255,255,255, 1.0,-0.8, 0.05,4.0, (g_pCvarEffects->value > 0)?12.0:16.0, 0.0), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS);

		if (g_pCvarEffects->value > 0)
		{
			gEngfuncs.pEfxAPI->R_BeamPoints(vecSrc, vecEnd, spr_beam, 0.25f, 0.25, 0.0f, 0.25f, 10.0, 0, 30, 0.9,0.9,1.0);
			if (g_pCvarEffects->value > 1)
			{
				BEAM *pSpiral = gEngfuncs.pEfxAPI->R_BeamPoints(vecSrc, vecEnd, spr_beam, 0.25f, 0.25, .01f, 0.25f, 20.0, 0, 30, 0.75,0.75,1.0);
				if (pSpiral)
					pSpiral->flags |= FBEAM_SINENOISE;
			}
			/*Vector vel;
			VectorSubtract(vecEnd, vecSrc, vel);
			VectorNormalize(vel);
			VectorScale(vel, 500, vel);
			gEngfuncs.pEfxAPI->R_UserTracerParticle(vecSrc, vel, 1.5, 2, 48, 0, NULL);
			//(  * org,  * vel, float life, int colorIndex, float length, unsigned char deathcontext, void ( *deathfunc)( struct particle_s *particle ) );
			*/
		}

		//test	g_pRenderManager->AddSystem(new CPSDrips(80, vecSrc, -Vector(2,2,2), Vector(2,2,2),
		//	(vecEnd-vecSrc).Normalize()*200, g_iModelIndexPTracer, 0, kRenderTransAdd, 0.2,0.2, 0.0, 1.0f), RENDERSYSTEM_FLAG_DONTFOLLOW);
	}

	/*if (g_pCvarEffects->value > 1.0)
	{
	Vector v;
	int zzz = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rain.spr");
	for (int i=0; i<8; i++)
	{
		v = VectorRandom(-3.0, 3.0);
		g_pRenderManager->AddSystem(new CRSBeam(vecSrc+v, vecEnd+v, zzz, kRenderTransAdd, RANDOM_LONG(200,255),RANDOM_LONG(200,255),255, RANDOM_FLOAT(0.8,1.0), -1.2, 0.1, 0.5, 20.0f, 0.0));
	}
	}*/
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_GLASS_IGNORE, -1, &tr);
	gEngfuncs.pEventAPI->EV_PopPMStates();
	if (flags > 0)
		DecalTrace(RANDOM_LONG(DECAL_SCORCH1, DECAL_SCORCH3), &tr, vecSrc);
	else
		DecalTrace(DECAL_GAUSSSHOT1, &tr, vecSrc);

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: create static architecture using a temporary entity to save network bandwidth and prevent "no free edicts" crash
// Output : int
//-----------------------------------------------------------------------------
int __MsgFunc_StaticEnt(const char *pszName, int iSize, void *pbuf)
{
	if (g_pRenderManager == NULL)
		return 1;

	Vector origin, angles;
	::Color rendercolor;
	BEGIN_READ(pbuf, iSize);
	short entindex	= READ_SHORT();
	short modelindex= READ_SHORT();
	origin[0]		= READ_FLOAT();// special version that works outside map mins/maxs
	origin[1]		= READ_FLOAT();
	origin[2]		= READ_FLOAT();//+1.0f;// HACK
	angles[0]		= READ_ANGLE();
	angles[1]		= READ_ANGLE();
	angles[2]		= READ_ANGLE();
	byte rendermode	= READ_BYTE();
	byte renderfx	= READ_BYTE();
	rendercolor.r	= READ_BYTE();
	rendercolor.g	= READ_BYTE();
	rendercolor.b	= READ_BYTE();
	rendercolor.a	= READ_BYTE();
	//byte effects	= READ_BYTE();
	byte body		= READ_BYTE();
	byte skin		= READ_BYTE();
	float scale		= READ_COORD();
	int colormap	= (int)(READ_SHORT() & 0x0000FFFF);// XDM3038c: 2->4 bytes
	short sequence	= READ_SHORT();
	short bsp_leaf	= READ_SHORT();
	float framerate	= READ_FLOAT();// XDM3038c
	char *modelname	= READ_STRING();
	END_READ();
	bool hide = (scale == 0.0f);

	DBG_PRINTF("__MsgFunc_%s(): [%d] creating model \"%s\"[%d] @(%g %g %g) (%g %g %g)\n", pszName, entindex, modelname, modelindex, origin.x, origin.y, origin.z, angles.x, angles.y, angles.z);

	// search for system even if the entindex is not valid
	CRSModel *pSystem = (CRSModel *)g_pRenderManager->FindSystemByFollowEntity(entindex);// don't allow more than one system per entity
	if (pSystem)// found, update
	{
		if (modelindex == 0)// signal to remove
		{
			pSystem->ShutdownSystem();
			pSystem->m_fDieTime = gEngfuncs.GetClientTime();
		}
		else// update existing system
		{
			if (pSystem->InitModel(IEngineStudio.Mod_ForName(modelname,0)))
			{
				pSystem->m_vecOrigin = origin;
				pSystem->m_vecAngles = angles;
				pSystem->m_iRenderMode = rendermode;
				pSystem->m_color = rendercolor;// 4b
				//old pSystem->m_fBrightness = (float)rendercolor.a/255.0f;
				pSystem->m_fFrameRate = framerate;// XDM3038c
				cl_entity_t *pClEntity = pSystem->GetEntity();
				if (pClEntity)
				{
					pClEntity->curstate.renderfx = renderfx;
					//pClEntity->curstate.effects = effects;
					pClEntity->curstate.body = body;
					pClEntity->curstate.skin = skin;
					pClEntity->curstate.colormap = colormap;// 2->4 bytes
					pClEntity->curstate.sequence = sequence;
					pClEntity->curstate.framerate = pClEntity->baseline.framerate = framerate;// XDM3038c
				}
				else
					conprintf(0, "%s: %u (\"%s\") error: no client entity!\n", pszName, pSystem->GetIndex(), modelname);

				if (scale > 0.0f)
					pSystem->m_fScale = scale;

				pSystem->m_iBSPLeaf = bsp_leaf;
			}
			else
				conprintf(0, "%s: unable to init model \"%s\"!\n", pszName, modelname);
		}
	}
	else// create new system
	{
		if (modelname == NULL || *modelname == '\0')//modelindex == 0)
			conprintf(0, "CL: MsgFunc_%s() ERROR: No model! (entindex %d)\n", pszName, entindex);
		else
		{
			if (UTIL_FileExists(modelname))//if (pmove && pmove->COM_FileSize(modelname) > 0)
			{
				pSystem = new CRSModel(origin, angles, g_vecZero, entindex, IEngineStudio.Mod_ForName(modelname,0), body, skin, sequence, rendermode, renderfx, rendercolor.r, rendercolor.g, rendercolor.b, (float)rendercolor.a/255.0f, 0.0f, scale, 0.0f, framerate, 0.0f);
				if (pSystem)
				{
					_snprintf(pSystem->m_szName, RENDERSYSTEM_USERNAME_LENGTH, "%s_e%hd", pszName, entindex);
					pSystem->m_iFxLevel = 0;// draw always because it is standard sprite replacement
					pSystem->m_iBSPLeaf = bsp_leaf;
					RS_INDEX idx = g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP, entindex, RENDERSYSTEM_FFLAG_DONTFOLLOW);
					if (idx != RS_INDEX_INVALID && pSystem)
					{
						cl_entity_t *pClEntity = pSystem->GetEntity();
						if (pClEntity)
						{
							pClEntity->baseline.colormap = pClEntity->curstate.colormap = colormap;
							//pClEntity->index = entindex;// XDM3038c: !
						}
#if defined (_DEBUG)
						conprintf(2, "CL: created static model %d (%s): RenderSystem %d\n", modelindex, pSystem->m_pModel?pSystem->m_pModel->name:"", idx);
#endif
					}
					else
					{
						if (g_pCvarRenderSystem->value <= 0)// XDM3038c: a temporary STUB or HACK to prevent cheating
						{
							conprintf(1, "CL: failed to create static model %d (%s): falling back to TEnt\n", modelindex, modelname);
							TEMPENTITY *pTEnt = gEngfuncs.pEfxAPI->CL_TempEntAllocHigh(origin, IEngineStudio.Mod_ForName(modelname,0));
							if (pTEnt)
							{
								conprintf(2, "CL: Warning: TEnts cannot be toggled or removed!\n");
								pTEnt->entity.angles = angles;
								pTEnt->entity.baseline.solid			= SOLID_NOT;
								pTEnt->entity.curstate.origin			= origin;// NO! VALVEHACK! pTEnt->entity.baseline.origin = origin;
								pTEnt->entity.curstate.angles			= pTEnt->entity.baseline.angles = angles;
								pTEnt->entity.curstate.sequence			= pTEnt->entity.baseline.sequence = sequence;
								pTEnt->entity.curstate.frame			= pTEnt->entity.baseline.frame = 0;
								pTEnt->entity.curstate.colormap			= pTEnt->entity.baseline.colormap = colormap;
								pTEnt->entity.curstate.skin				= pTEnt->entity.baseline.skin = skin;
								pTEnt->entity.curstate.scale			= pTEnt->entity.baseline.scale = scale;
								pTEnt->entity.curstate.eflags			= pTEnt->entity.baseline.eflags = EFLAG_DRAW_ALWAYS;
								pTEnt->entity.curstate.rendermode		= pTEnt->entity.baseline.rendermode = rendermode;
								pTEnt->entity.curstate.renderamt		= pTEnt->entity.baseline.renderamt = rendercolor.a;
								pTEnt->entity.curstate.rendercolor.r	= pTEnt->entity.baseline.rendercolor.r = rendercolor.r;
								pTEnt->entity.curstate.rendercolor.g	= pTEnt->entity.baseline.rendercolor.g = rendercolor.g;
								pTEnt->entity.curstate.rendercolor.b	= pTEnt->entity.baseline.rendercolor.b = rendercolor.b;
								pTEnt->entity.curstate.renderfx			= pTEnt->entity.baseline.renderfx = renderfx;
								pTEnt->entity.curstate.movetype			= pTEnt->entity.baseline.movetype = MOVETYPE_NONE;
								pTEnt->entity.curstate.framerate		= pTEnt->entity.baseline.framerate = framerate;
								pTEnt->entity.curstate.body				= pTEnt->entity.baseline.body = body;
							}
							else
								conprintf(1, "CL: failed to create temporary entity %d (%s)!\n", modelindex, modelname);
						}
					}
				}
			}
			else
			{
				conprintf(0, "%s: unable to load model \"%s\"!\n", pszName, modelname);
				if (g_pCvarDeveloper && g_pCvarDeveloper->value > 1.0f)// in developer mode draw placeholders to aid map designer
				{
					pSystem = new CRSModel(origin, angles, g_vecZero, entindex, IEngineStudio.GetModelByIndex(g_iModelIndexTestSphere), body, skin, sequence, rendermode, renderfx, rendercolor.r, rendercolor.g, rendercolor.b, (float)rendercolor.a/255.0f, 0.0f, scale, 0.0f, framerate, 0.0f);
					if (pSystem)
					{
						_snprintf(pSystem->m_szName, RENDERSYSTEM_USERNAME_LENGTH, "%s_e%hd", pszName, entindex);
						pSystem->m_iBSPLeaf = bsp_leaf;
						g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP, entindex, RENDERSYSTEM_FFLAG_DONTFOLLOW);
					}
				}
			}
		}
	}
	if (pSystem)// XDM3038c: server requests to hide it
	{
		if (hide)
			SetBits(pSystem->m_iFlags, RENDERSYSTEM_FLAG_NODRAW);
		else
			ClearBits(pSystem->m_iFlags, RENDERSYSTEM_FLAG_NODRAW);
	}
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: XDM3035a: create static sprite using RenderSystem to save network bandwidth
// Output : int
//-----------------------------------------------------------------------------
int __MsgFunc_StaticSpr(const char *pszName, int iSize, void *pbuf)
{
	if (g_pRenderManager == NULL)
		return 1;

	Vector origin, angles;
	::Color rendercolor;
	BEGIN_READ(pbuf, iSize);
	short entindex	= READ_SHORT();
	short sprindex	= READ_SHORT();
	origin[0]		= READ_COORD();
	origin[1]		= READ_COORD();
	origin[2]		= READ_COORD();//+1.0f;// HACK
	angles[0]		= READ_ANGLE();
	angles[1]		= READ_ANGLE();
	angles[2]		= READ_ANGLE();
	byte rendermode	= READ_BYTE();
	byte renderfx	= READ_BYTE();
	rendercolor.r	= READ_BYTE();
	rendercolor.g	= READ_BYTE();
	rendercolor.b	= READ_BYTE();
	rendercolor.a	= READ_BYTE();
	//byte effects	= READ_BYTE();
	float scale		= READ_COORD();
	byte framerate	= READ_BYTE();
	/*short bsp_leaf	= */READ_SHORT();
	END_READ();

	// must be done on server:	UTIL_FixRenderColor(rendermode, rendercolor);

	// search for system even if the entindex is not valid
	CRSSprite *pSystem = (CRSSprite *)g_pRenderManager->FindSystemByFollowEntity(entindex);// don't allow more than one system per entity
	if (pSystem)// found, update
	{
		if (sprindex == 0)// signal to remove
		{
			pSystem->ShutdownSystem();
			pSystem->m_fDieTime = gEngfuncs.GetClientTime();
		}
		else// update existing system
		{
			//should we?	pSystem->InitTexture(sprindex);
			pSystem->m_vecOrigin = origin;
			pSystem->m_vecAngles = angles;
			pSystem->m_iRenderMode = rendermode;
			pSystem->m_iRenderEffects = renderfx;
			pSystem->m_color = rendercolor;// 4b
			//old pSystem->m_fBrightness = (float)rendercolor.a/255.0f;
			pSystem->m_fScale = scale;
			pSystem->m_fFrameRate = (float)framerate;
			//pSystem->m_iBSPLeaf = bsp_leaf;
			if (scale == 0.0f)// XDM3037a: new method			if (effects & EF_NODRAW)
				pSystem->m_iFlags |= RENDERSYSTEM_FLAG_NODRAW;
			else
				pSystem->m_iFlags &= ~RENDERSYSTEM_FLAG_NODRAW;
		}
	}
	else// create new system
	{
		if (sprindex == 0)
			conprintf(0, "CL: MsgFunc_%s() ERROR: No sprite! (entindex %d)\n", pszName, entindex);
		else
		{
			pSystem = new CRSSprite(origin, g_vecZero, IEngineStudio.GetModelByIndex(sprindex), rendermode, rendercolor.r, rendercolor.g, rendercolor.b, (float)rendercolor.a/255.0f, 0.0f, scale, 0.0f, (float)framerate, 0.0f);
			if (pSystem)
			{
				pSystem->m_iFxLevel = 0;// draw always because it is standard sprite replacement
				//pSystem->m_iBSPLeaf = bsp_leaf;
				if (renderfx != kRenderFxNoDissipation)// XDM3037a: useless anyway
					pSystem->m_iRenderEffects = renderfx;

				_snprintf(pSystem->m_szName, RENDERSYSTEM_USERNAME_LENGTH, "%s_s%hd_e%hd", pszName, sprindex, entindex);
				RS_INDEX idx = g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, entindex, RENDERSYSTEM_FFLAG_DONTFOLLOW);
				if (idx == RS_INDEX_INVALID || pSystem == NULL)
					conprintf(2, "CL: %s: failed to create static sprite %d!\n", pszName, sprindex);
#if defined (_DEBUG)
				else
					conprintf(2, "CL: %s: created static sprite %d (%s): RenderSystem %d\n", pszName, sprindex, pSystem->m_pTexture?pSystem->m_pTexture->name:"", idx);
#endif
			}
			else
				conprintf(0, "MsgFunc_%s ERROR: unable to create system!\n", pszName);
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: make this packet as small as possible
// Output : int
//-----------------------------------------------------------------------------
/*int __MsgFunc_EgonSparks(const char *pszName, int iSize, void *pbuf)
{
	Vector origin;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	int firemode = READ_BYTE();
	END_READ();
	//conprintf("__MsgFunc_EgonSparks %d\n", firemode);

	if (firemode == 0)// gluon primary
		//FX_StreakSplash(origin, g_vecZero, gTracerColors[7], 32, 200.0f, true, true, false);
		gEngfuncs.pEfxAPI->R_StreakSplash(origin, g_vecZero, 7, 32, 32, -240, 240);// "r_efx.h"
	else if (firemode == 1)
		//FX_StreakSplash(origin, g_vecZero, gTracerColors[7], 24, 200.0f, true, true, false);
		gEngfuncs.pEfxAPI->R_StreakSplash(origin, g_vecZero, 7, 24, 40, -240, 240);
	else if (firemode == 2)// plasma primary
	{
		FX_StreakSplash(origin, g_vecZero, gTracerColors[0], 24, 200.0f, true, true, false);
		if (g_pCvarEffects->value > 1.0f)
			FX_StreakSplash(origin, g_vecZero, gTracerColors[15], 6, 320.0f, false, true, false);
	}
	else
	{
		FX_StreakSplash(origin, g_vecZero, gTracerColors[7], 32, 200.0f, true, true, true);
		if (g_pCvarEffects->value > 1.0f)
			FX_StreakSplash(origin, g_vecZero, gTracerColors[17], 8, 320.0f, false, true, false);
	}
	return 1;
}*/

//-----------------------------------------------------------------------------
// Purpose: XDM3035: TE_MODEL replacement
// Output : int
//-----------------------------------------------------------------------------
int __MsgFunc_TEModel(const char *pszName, int iSize, void *pbuf)
{
	Vector origin, velocity;
	float angle;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	READ_COORD3(velocity);
	angle			= READ_ANGLE();
	short modelindex= READ_SHORT();
	byte body		= READ_BYTE();
	byte soundtype	= READ_BYTE();
	byte lifetime	= READ_BYTE();
	END_READ();
	Vector angles(0.0f, angle, 0.0f);
	TEMPENTITY *pShell = gEngfuncs.pEfxAPI->R_TempModel(origin, velocity, angles, (float)lifetime * 0.1f, modelindex, soundtype);
	if (pShell)
	{
		pShell->entity.baseline.body = body;
		pShell->entity.curstate.body = body;// a must!
	}
	/*model_s *pModel = IEngineStudio.GetModelByIndex(modelindex);
	if (!pModel)// || pModel->type != mod_studio)
		return 0;
	TEMPENTITY	*pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(origin, pModel);
	if (pTemp == NULL)
		return 0;

	if (soundtype == TE_BOUNCE_SHELL)
		pTemp->hitSound = BOUNCE_SHELL;
	else if (soundtype == TE_BOUNCE_SHOTSHELL)
		pTemp->hitSound = BOUNCE_SHOTSHELL;

	VectorCopy(origin, pTemp->entity.origin);
	pTemp->entity.angles[0] = 0.0f;
	pTemp->entity.angles[1] = angle;
	pTemp->entity.angles[2] = 0.0f;
	VectorCopy(velocity, pTemp->entity.baseline.origin);
	pTemp->entity.baseline.body = pTemp->entity.curstate.body = body;
	pTemp->flags = (FTENT_COLLIDEWORLD|FTENT_GRAVITY|FTENT_ROTATE);
	pTemp->entity.baseline.angles[0] = RANDOM_FLOAT(-180, 180);
	pTemp->entity.baseline.angles[1] = RANDOM_FLOAT(-180, 180);
	pTemp->entity.baseline.angles[2] = RANDOM_FLOAT(-180, 180);
	pTemp->entity.curstate.rendermode = kRenderNormal;
	pTemp->entity.baseline.renderamt = 255;
	pTemp->die = gEngfuncs.GetClientTime() + (float)lifetime * 0.1f;*/
	return 1;
}

int __MsgFunc_Bubbles(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (g_pRenderManager == NULL)
		return 1;

	Vector v1, v2;
	BEGIN_READ(pbuf, iSize);
	byte type = READ_BYTE();
	byte count = READ_BYTE();
	READ_COORD3(v1);
	READ_COORD3(v2);
	END_READ();
	g_pRenderManager->AddSystem(new CPSBubbles(count, type, v1, v2, BUBBLE_SPEED, g_pSpriteBubble, kRenderTransAlpha, 1.0f, 0.0f, BUBBLE_SCALE, 0.0f, BUBBLE_LIFE),
									RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_SIMULTANEOUS/*|RENDERSYSTEM_FLAG_INCONTENTSONLY*/, -1);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Item/weapon/ammo respawn effect (used to be an event, but that was unreliable)
// Output : int
//-----------------------------------------------------------------------------
int __MsgFunc_ItemSpawn(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	Vector origin, angles;
	BEGIN_READ(pbuf, iSize);
	byte type		= READ_BYTE();
	short entindex	= READ_SHORT();
	READ_COORD3(origin);//.z += 1.0f;// HACK
	angles[0]		= READ_ANGLE();
	angles[1]		= READ_ANGLE();
	angles[2]		= READ_ANGLE();
	short modelindex= READ_SHORT();
	float scale		= 0.1f*(float)READ_BYTE();
	byte body		= READ_BYTE();
	byte skin		= READ_BYTE();
	byte sequence	= READ_BYTE();
	END_READ();
	color24 c;
	bool toofar = UTIL_PointIsFar(origin);
	if (type == EV_ITEMSPAWN_ITEM)
	{
		c.r = 127; c.g = 127; c.b = 255;
		EMIT_SOUND(entindex, origin, CHAN_WEAPON, DEFAULT_RESPAWN_SOUND_ITEM, VOL_NORM, ATTN_IDLE, 0, 110);
		if (g_pCvarParticles->value > 0.0f)
		{
			if (!toofar)
				FX_StreakSplash(origin, Vector(0,0,1), c, 24, 56.0f, false, true, false);

			if (g_pCvarParticles->value > 1.0f)
			{
				if (g_pRenderManager)
					g_pRenderManager->AddSystem(new CPSSparks(((g_pCvarParticles->value > 1.0f)?48:32), origin, 2.0f,1.0f,-0.5f, -20.0f/*radius*/, 2.0f, c.r,c.g,c.b,1.0f,-0.75f, g_pSpriteMuzzleflash3, kRenderTransAdd, 2.0f),
												RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP);
			}
		}
	}
	else if (type == EV_ITEMSPAWN_WEAPON)
	{
		c.r = 255; c.g = 255; c.b = 127;
		EMIT_SOUND(entindex, origin, CHAN_WEAPON, DEFAULT_RESPAWN_SOUND_ITEM, VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
		if (g_pCvarParticles->value > 0.0f)
		{
			if (!toofar)
				FX_StreakSplash(origin, Vector(0,0,1), c, 24, 48.0f, false, true, false);

			if (g_pCvarParticles->value > 1.0f)
			{
				if (g_pRenderManager)
					g_pRenderManager->AddSystem(new CPSSparks(((g_pCvarParticles->value > 1.0f)?48:32), origin, 2.0f,1.0f,-0.5f, -16.0f/*radius*/, 2.0f, c.r,c.g,c.b,1.0f,-0.75f, g_pSpriteMuzzleflash3, kRenderTransAdd, 2.0f),
												RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP);
			}
		}
	}
	else if (type == EV_ITEMSPAWN_AMMO)
	{
		c.r = 127; c.g = 255; c.b = 127;
		EMIT_SOUND(entindex, origin, CHAN_WEAPON, DEFAULT_RESPAWN_SOUND_ITEM, VOL_NORM, ATTN_IDLE, 0, 150);
		if (g_pCvarParticles->value > 0.0f)
			FX_StreakSplash(origin, Vector(0,0,1), c, 24, 48.0f, false, true, false);
	}
	else// if (type == EV_ITEMSPAWN_OTHER)
	{
		c.r = 255; c.g = 255; c.b = 255;
		EMIT_SOUND(entindex, origin, CHAN_WEAPON, DEFAULT_RESPAWN_SOUND_ITEM, VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
		gEngfuncs.pEfxAPI->R_ParticleBurst(origin, 4, 208, 10);
	}

	if (!toofar && g_pRenderManager && modelindex > 0)// shrinking model effect
	{
		float scaletime = 0.6f;
		float startscale = 2.5f;
		float scaledelta = -fabs(scale-startscale)/scaletime;// target scale
		//float adelta = -fabs(scale-startscale)/scaletime;
		CRSModel *pModelSystem = new CRSModel(origin, angles, g_vecZero, entindex, IEngineStudio.GetModelByIndex(modelindex), body, skin, sequence, kRenderTransTexture, ((g_pCvarEffects->value > 1.0f)?kRenderFxGlowShell:kRenderFxNone), c.r,c.g,c.b, 0.1f, 1.5f, startscale, scaledelta, 1.0f, scaletime);
		if (pModelSystem)
			g_pRenderManager->AddSystem(pModelSystem, RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP, entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE|RENDERSYSTEM_FFLAG_DONTFOLLOW);
	}
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Load particle system from a file
// WARNING: Many systems may be attached to one target entity!!!
//-----------------------------------------------------------------------------
/*int __MsgFunc_EnvParticle(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (g_pRenderManager == NULL)
		return 1;

	Vector origin;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(origin);
	int emitterindex		= READ_SHORT();
	int targetindex			= READ_SHORT();
	int attachment			= READ_BYTE();
	//int ttlx10				= READ_SHORT();
	//int state				= READ_BYTE();
	unsigned short flags	= READ_WORD();
	char *filename			= READ_STRING();
	END_READ();

	if (UTIL_FileExtensionIs(filename, ".aur"))
	{
		conprintf(1, "__MsgFunc_%s(%s): \"aurora\" is bad and you should feel bad.\n", pszName, filename);
	}
	else
	{
		uint32 numfound = FindLoadedSystems(filename, emitterindex, targetindex, (flags & RENDERSYSTEM_FLAG_NODRAW)?0:1);
		if (numfound == 0)// nothing found
		{
			if (!(flags & RENDERSYSTEM_FLAG_NODRAW))// enable/CREATE
				LoadParticleSystems(filename, origin, emitterindex, targetindex, attachment, flags, 0);//ttlx10*0.1f);
			else
				conprintf(1, "__MsgFunc_%s(%s): no systems found to disable!\n", pszName, filename);
		}
	}
	return 1;
}*/

//-----------------------------------------------------------------------------
// Purpose: Environmental damage was applied to some entity
//-----------------------------------------------------------------------------
int __MsgFunc_DamageFx(const char *pszName, int iSize, void *pbuf)
{
	//DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	//Vector origin, angles;
	BEGIN_READ(pbuf, iSize);
	short entindex			= READ_SHORT();
	long bitsDamageType		= READ_LONG();
	float flDamage			= READ_COORD();
	END_READ();

	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(entindex);
	if (ent == NULL)
		return 1;

	bool bSoundPlayed = false;
	if (bitsDamageType & (DMGM_FIRE | DMG_IGNITE))
	{
		g_pRenderManager->AddSystem(new CRSSprite(ent->origin, g_vecZero, g_pSpriteFlameFire, kRenderTransAdd,
				255,255,255, 1.0f,-1.0f, RANDOM_FLOAT(0.1, 0.25),-1.0f, 16.0f, 0.0f), 0, entindex, RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET);// XDM3038c

		if ((bitsDamageType & DMG_IGNITE) && g_pCvarParticles->value > 0)// XDM3038c
		{
			CParticleSystem *pParticleSystem = new CParticleSystem(32, ent->origin, Vector(0,0,1), g_pSpriteFlameFire2, kRenderTransAdd, 1.0f);
			if (pParticleSystem)
			{
				pParticleSystem->m_fScale = 0.0625f;
				pParticleSystem->m_fScaleDelta = -0.05f;//test1->value;// -2.0f;
				//old pParticleSystem->m_fBrightness = 1.0f;
				pParticleSystem->m_fColorDelta[1] = -1.0f;// decrease green
				pParticleSystem->m_fColorDelta[3] = -2.0f;// brightness
				pParticleSystem->m_fFrameRate = 12.0f;
				pParticleSystem->m_iMovementType = PSMOVTYPE_DIRECTED;
				if (IsActivePlayer(entindex))
				{
					pParticleSystem->m_iStartType = PSSTARTTYPE_BOX;// unfortunately, players don't have model mins/maxs
					pParticleSystem->m_vStartMins = VEC_HULL_MIN;
					pParticleSystem->m_vStartMaxs = VEC_HULL_MAX;
				}
				else
					pParticleSystem->m_iStartType = PSSTARTTYPE_ENTITYBBOX;

				pParticleSystem->m_fParticleSpeedMin = 6.0f;
				pParticleSystem->m_fParticleSpeedMax = 8.0f;
				//pParticleSystem->m_OnParticleInitialize = OnParticleInitializeFlame;
				//pParticleSystem->m_iFollowAttachment = 0;
				g_pRenderManager->AddSystem(pParticleSystem, RENDERSYSTEM_FLAG_LOOPFRAMES, entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
			}
		}
		if (!bSoundPlayed)
		{
			EMIT_SOUND(entindex, ent->origin, CHAN_STATIC, "weapons/flame_burn.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(PITCH_NORM,PITCH_NORM+10));
			bSoundPlayed = true;
		}
	}

	if (bitsDamageType & (DMGM_FIRE | DMG_VAPOURIZING))
	{
		byte gray = (byte)RANDOM_LONG(20,35);
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(ent->origin, Vector(0,0,18), IEngineStudio.GetModelByIndex(g_iModelIndexSmoke), kRenderTransAlpha, gray,gray,gray, 0.0f,0.0f/*useless*/, clamp(flDamage*0.02f, 0.1f,16.0f),1.5f, 8.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP, entindex, RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET);

		if (!bSoundPlayed)
		{
			EMIT_SOUND(entindex, ent->origin, CHAN_STATIC, "weapons/flame_burn.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(PITCH_NORM,PITCH_NORM+10));
			bSoundPlayed = true;
		}
	}

	if (bitsDamageType & DMGM_COLD)
	{
		FX_StreakSplash(ent->origin, g_vecZero, gTracerColors[14], (int)(flDamage*0.5f), flDamage, true, true, false);
		if (!bSoundPlayed)
		{
			EMIT_SOUND(entindex, ent->origin, CHAN_STATIC, "debris/glass6.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(PITCH_NORM,PITCH_NORM+10));
			bSoundPlayed = true;
		}
	}

	if (bitsDamageType & DMGM_POISON)
	{
		if (g_pRenderManager)
		{
			g_pRenderManager->AddSystem(new CRSSprite(ent->origin, Vector(0,0,8), g_pSpriteAcidPuff2, kRenderTransAdd, 255,255,255, min(flDamage*0.01f, 0.4f),0.0f, flDamage*0.01f, 1.0f, 12.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP, entindex, RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET);
			if (g_pCvarParticles->value > 0.0f)
				g_pRenderManager->AddSystem(new CPSFlameCone((g_pCvarParticles->value > 1.0f)?72:48, ent->origin, ent->origin, Vector(4.0,4.0,4.0), 80.0f,
					g_pSpriteAcidDrip, kRenderTransAdd, 1.0f,-1.5f, 0.25f,-0.5f, 0.25f), RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS, -1);// XDM3038a
		}
		if (!bSoundPlayed)
		{
			EMIT_SOUND(entindex, ent->origin, CHAN_STATIC, "weapons/pgrenade_acid.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(PITCH_NORM,PITCH_NORM+10));
			bSoundPlayed = true;
		}
	}
	/*if (bitsDamageType & DMG_VAPOURIZING)
	{
	}*/
	if (bitsDamageType & DMG_DISINTEGRATING)
		FX_StreakSplash(ent->origin, g_vecZero, gTracerColors[0], (int)(flDamage*0.5f), flDamage, true, true, true);

	return 1;
}



//-----------------------------------------------------------------------------
// Purpose: Custom per-particle init function for teleporter particles
// Input  : *pSystem - 
//			*pParticle - 
//			*pData - 
// Output : unused
//-----------------------------------------------------------------------------
bool FX_Teleport_OnParticleInitialize(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &fInterpolaitonMult)
{
	if (pParticle->index & 1)// half of particles will have different size
		pParticle->m_fSizeDelta *= 0.5f;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Custom per-particle update function for teleporter particles
// Input  : *pSystem - 
//			*pParticle - 
//			*pData - 
//			&time - 
//			&elapsedTime - 
// Output : unused
//-----------------------------------------------------------------------------
bool FX_Teleport_OnParticleUpdate(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	/*float s,c;
	SinCos(-6.0f*time// + (float)(i*2)//, &s, &c);
	pParticle->m_vPos.x += pSystem->m_fScale * s;
	pParticle->m_vPos.y += pSystem->m_fScale * c;
	pParticle->m_vPos.z += -pSystem->m_fScale * s;
	*/
	pParticle->m_fColor[3] = RANDOM_FLOAT(0.1, 1.0);

	if (pSystem->m_fDieTime + RANDOM_FLOAT(0,1) <= time)// since energy gets updated by brightness, we can't depend on it
		pParticle->m_fEnergy = 0;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player (entindex) teleports via trigger_teleport
// byte 0/1 src/dest
// coord3x
//-----------------------------------------------------------------------------
int __MsgFunc_Teleport(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	Vector pos;
	BEGIN_READ(pbuf, iSize);
	byte flags	= READ_BYTE();
	short ent	= READ_SHORT();
	READ_COORD3(pos);
	END_READ();

	//gEngfuncs.pEfxAPI->R_BeamPoints(src, dst, g_iModelIndexLaser, 2.0f, 0.25, 0.0f, 1.0f, 10.0, 0, 30, 1,0,0);
	if (!UTIL_PointIsFar(pos))
	{
		if (flags & MSG_TELEPORT_FL_DEST)
		{
			// old shit	gEngfuncs.pEfxAPI->R_TeleportSplash(pos);
			if (g_pCvarParticles->value > 0.0)
			{
				if (g_pRenderManager)
				{
					CPSSparks *pSystem = new CPSSparks(((g_pCvarParticles->value > 1.0f)?96:64), pos, 0.5f,0.5f,-2.0f, 80.0f/*velocity*/, 2.0f, 255,255,255,1.0f,-0.5f, g_pSpriteMuzzleflash3, kRenderTransAdd, 0.75f);
					if (pSystem)
					{
						pSystem->m_OnParticleInitialize = FX_Teleport_OnParticleInitialize;
						pSystem->m_OnParticleUpdate = FX_Teleport_OnParticleUpdate;
						g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_CLIPREMOVE);
					}
				}
			}
			if (g_pCvarEffects->value > 0.0)
				DynamicLight(pos, 64, 255,255,255, 1.0, 128);

			if (flags & MSG_TELEPORT_FL_PARTICLES)
				ParticlesCustom(pos, 200, BLOOD_COLOR_GREEN, 8, 192, 2.0, true);
		}
		else// src
		{
			if (g_pRenderManager)
			{
				CRSSprite *pSystem = new CRSSprite(pos, g_vecZero, g_pSpriteTeleFlash, kRenderTransAdd, 255,255,255, 1.0, -2.0, 1.0,-4.0, 20, 1.0);
				if (pSystem)
				{
					pSystem->m_fColorDelta[0] = -1.0f;// decrease R&G over time
					pSystem->m_fColorDelta[1] = -1.0f;
					g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP);
				}
			}
			if (g_pCvarEffects->value > 0.0)
				DynamicLight(pos, 64, 255,255,255, 1.0, 128);

			if (flags & MSG_TELEPORT_FL_PARTICLES)
				ParticlesCustom(pos, 120, 40, 8, 192, 1.0, true);
		}
	}
	if (EV_IsLocal(ent))
	{
		if (flags & MSG_TELEPORT_FL_FADE)
		{
			if ((flags & MSG_TELEPORT_FL_DEST) && !gHUD.m_bFrozen)
				CL_ScreenFade(240,240,255,191, 0.5, 0.0, FFADE_IN);
		}
		if (flags & MSG_TELEPORT_FL_SOUND)
		{
			if (flags & MSG_TELEPORT_FL_DEST)
				PlaySound("common/teleport_local.wav", VOL_NORM);// not ambient!
			//EMIT_SOUND(-1, pos, CHAN_STATIC, "common/teleport_local.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}
	}
	else
	{
		if (flags & MSG_TELEPORT_FL_SOUND)
		{
			if (flags & MSG_TELEPORT_FL_DEST)
				EMIT_SOUND(-1, pos, CHAN_STATIC, "common/teleport.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			else
				EMIT_SOUND(-1, pos, CHAN_STATIC, "common/teleport.wav", VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
		}
	}
	return 1;
}

void TE_GibTouch(struct tempent_s *ent, struct pmtrace_s *ptr)
{
	ent->entity.curstate.health++;
	if (ent->entity.curstate.health > 1 && (gHUD.m_flTime - ent->entity.curstate.fuser1) > 0.25f)
	{
		if (ent->entity.curstate.playerclass == matFlesh)
		{
			// blood & decal
			if (!UTIL_PointIsFar(ent->entity.origin))
			{
				//if (red)
				gEngfuncs.pEfxAPI->R_Blood(ent->entity.origin, ptr->plane.normal, BLOOD_COLOR_RED, ent->bounceFactor);
				DecalTrace(RANDOM_LONG(DECAL_BLOOD1, DECAL_BLOOD6), ptr, ent->entity.origin);
			}
		}
	}
	ent->entity.curstate.fuser1 = gHUD.m_flTime;// last impact time
}

// TODO
int __MsgFunc_Gibs(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	Vector vecCenter, vecVol, vecVel;
	BEGIN_READ(pbuf, iSize);
	READ_COORD3(vecCenter);
	READ_COORD3(vecVol);
	READ_COORD3(vecVel);
	//short ent			= READ_SHORT();
	short modelindex	= READ_SHORT();
	byte count			= READ_BYTE();
	float life			= READ_BYTE()*0.1f;
	byte flags			= READ_BYTE();
	byte material		= READ_BYTE();
	END_READ();

	model_s *pModel = IEngineStudio.GetModelByIndex(modelindex);
	if (pModel == NULL)
	{
		conprintf(1, "ERROR: MsgFunc_Gibs: NULL model!\n");
		return 1;
	}
	studiohdr_t *pStudioHeader;
	if (pModel->type == mod_studio)
		pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(pModel);
	else
		pStudioHeader = NULL;

	/*if (pStudioHeader == NULL)
	{
		conprintf(1, "ERROR: MsgFunc_Gibs: bad model data!\n");
	}*/

	Vector vecDir;
	float vel1d;
	/*wrong	if (vecVel.IsZero())
	{
		vecDir.x = UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, -1, 1);
		vecDir.y = UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, -1, 1);
		vecDir.z = UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, -1, 1);
	}
	else*/
		vecDir = vecVel;

	vel1d = vecDir.NormalizeSelf();

	Vector absmins, absmaxs, vecGibSrc, vecGibDir;
	absmins = vecCenter - vecVol*0.5f;
	absmaxs = vecCenter + vecVol*0.5f;

	int gFlag = FTENT_NONE;
	int gHitSound = 0;
	int gTrailType = 0;
	int gRenderMode = kRenderNormal;
	int gRenderAmt = 255;
	int gType = 0;
	switch (material)
	{
	case matGlass:
		gHitSound = BOUNCE_GLASS;
		gRenderMode = kRenderTransTexture;
		gRenderAmt = 127;
		break;

	case matWood:
		gHitSound = BOUNCE_WOOD;
		break;

	case matComputer:
		gFlag = FTENT_SPARKSHOWER;
		gHitSound = BOUNCE_METAL;
		break;

	case matMetal:
		gHitSound = BOUNCE_METAL;
		//gRenderMode = kRenderTransAlpha;
		break;

	case matFlesh:
		gHitSound = BOUNCE_FLESH;
		gTrailType = teTrailThickBlood;
		if (flags & 1)
			gType = 1;// human
		else if (flags & 2)
			gType = 2;// alien

		break;

	case matRocks:
	case matCinderBlock:
		gHitSound = BOUNCE_CONCRETE;
		break;

	case matCeilingTile:
		gHitSound = BOUNCE_CONCRETE;
		break;
	}
	if (flags & BREAK_SMOKE)
		gTrailType = teTrailThickSmoke;

	if (gTrailType > 0)
		gFlag |= FTENT_SMOKETRAIL;
	
	if (flags & BREAK_2)
		gFlag |= FTENT_SPARKSHOWER;

	TEMPENTITY *pGib;
	for (int i = 0; i < count; ++i)
	{
		int j;
		for (j=0; j<16; ++j)// try to find some empty space
		{
			vecGibSrc = absmins + vecVol*UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, 0,1);
			if (gEngfuncs.PM_PointContents(vecGibSrc, NULL) != CONTENTS_SOLID)
				break;
		}
		if (j == 16)// failed to find space in requested volume
			vecGibSrc = vecCenter;

		vecGibDir = (vecGibSrc - vecCenter) + vecDir;
		vecGibDir.NormalizeSelf();

		pGib = gEngfuncs.pEfxAPI->CL_TempEntAlloc(vecGibSrc, pModel);
		if (pGib)
		{
			pGib->flags |= (gFlag | FTENT_COLLIDEALL | FTENT_ROTATE | FTENT_HITSOUND | FTENT_SPRANIMATELOOP);
			pGib->entity.baseline.angles[0] = UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, -255, 255);
			pGib->entity.baseline.angles[1] = UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, -255, 255);
			pGib->entity.baseline.angles[2] = UTIL_SharedRandomFloat(gHUD.m_iRandomSeed, -255, 255);
			pGib->entity.baseline.rendermode = pGib->entity.curstate.rendermode = gRenderMode;
			pGib->entity.baseline.renderamt = pGib->entity.curstate.renderamt = gRenderAmt;
			if (pStudioHeader)
				pGib->entity.baseline.body = pGib->entity.curstate.body = UTIL_SharedRandomLong(gHUD.m_iRandomSeed, 0, pStudioHeader->numbodyparts-1);

			pGib->entity.baseline.playerclass = pGib->entity.curstate.playerclass = material;
			pGib->entity.baseline.gaitsequence = pGib->entity.curstate.gaitsequence = gType;
			pGib->entity.baseline.health = pGib->entity.curstate.health = 1;
			//pGib->hitcallback = TE_GibTouch;
			pGib->hitSound = gHitSound;
			pGib->entity.baseline.usehull = pGib->entity.curstate.usehull = gTrailType;
			pGib->die = gEngfuncs.GetClientTime() + life + RANDOM_FLOAT(0,1);
		}
		else
			break;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Register all defined messages so the engine may use them
//-----------------------------------------------------------------------------
void HookFXMessages(void)
{
	HOOK_MESSAGE(ViewModel);
	HOOK_MESSAGE(Particles);
	HOOK_MESSAGE(SetFog);
	//HOOK_MESSAGE(WallSprite);
	HOOK_MESSAGE(PartSys);
	HOOK_MESSAGE(RenderSys);
	//HOOK_MESSAGE(Snow);
	HOOK_MESSAGE(DLight);
	HOOK_MESSAGE(ELight);
	HOOK_MESSAGE(SetSky);
	HOOK_MESSAGE(SetRain);
	HOOK_MESSAGE(FireBeam);
	HOOK_MESSAGE(StaticEnt);
	HOOK_MESSAGE(StaticSpr);
	//HOOK_MESSAGE(EgonSparks);
	HOOK_MESSAGE(TEModel);
	HOOK_MESSAGE(Bubbles);
	HOOK_MESSAGE(ItemSpawn);
	//HOOK_MESSAGE(EnvParticle);
	HOOK_MESSAGE(DamageFx);
	HOOK_MESSAGE(Teleport);
	HOOK_MESSAGE(Gibs);
}
