#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "eventscripts.h"
#include "event_api.h"
//#include "r_efx.h"
#include "cl_fx.h"
#include "shared_resources.h"
//#include "screenfade.h"
#include "shake.h"
#include "gamedefs.h"
#include "pm_defs.h"// PM_NORMAL
#include "pm_shared.h"
//#include "weapondef.h"// VECTOR_CONE_

#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSSprite.h"
#include "RSCylinder.h"
#include "RSLight.h"
//#include "RSModel.h"
#include "ParticleSystem.h"
#include "PSFlameCone.h"
#include "PSSparks.h"
#include "PSSpiralEffect.h"

extern "C"
{
void EV_PlayerSpawn(struct event_args_s *args);
void EV_CaptureObject(struct event_args_s *args);
void EV_DomPoint(struct event_args_s *args);
}


float FlagPulseLightRadiusCallback(CRSLight *pSystem, const float &time)
{
	return (0.75f + 0.25f*sinf(time*8.0f));
}

//-----------------------------------------------------------------------------
// Purpose: called multiplayer games everytime a player spawns
// entindex - 
// origin - 
// angles - 
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_PlayerSpawn(struct event_args_s *args)
{
	EV_START(args);
#if defined (_DEBUG_GAMERULES)
	EV_PrintParams("EV_PlayerSpawn", args);
#endif
	if (EV_IsLocal(args->entindex))
		CL_ScreenFade(79,101,207,191, 1.0, 0.0, FFADE_IN);// IMPORTANT! This clears any permanent fade effect (disintegration, freez, etc.)

	gEngfuncs.pEfxAPI->R_KillAttachedTents(args->entindex);// XDM3037a

	STOP_SOUND(args->entindex, CHAN_VOICE, "!HEV_DEAD");// TODO: fix me when we have per-character sounds
	//EMIT_SOUND(args->entindex, args->origin, CHAN_VOICE, "common/null.wav", VOL_NORM, ATTN_IDLE, 0, PITCH_NORM);// CHAN_VOICE will stop current long death sound (if any)

	if (g_pCvarTFX->value > 0.0f)
	{
		vec3_t origin;
		VectorCopy(args->origin, origin);
		origin[2] -= 16.0f;// too high
		byte r,g,b;

		if (IsTeamGame(gHUD.m_iGameType))// special case: use specified value (g_PlayerExtraInfo may be not valid right now)
			GetTeamColor(args->iparam1, r,g,b);
		else
			GetPlayerColor(args->entindex, r,g,b);

		if (g_pRenderManager)// (args->iparam2 > 0?args->iparam2:g_iModelIndexAnimglow01)
		{
			CPSSpiralEffect *pSpawnEffect = new CPSSpiralEffect(64, origin, 8.0f, 24.0f, g_pSpritePSparks, kRenderTransAdd, r,g,b, 1.0f,-0.75f, 4.0f);
			if (pSpawnEffect)
			{
				pSpawnEffect->m_fColorDelta[0] = 1.0f;
				pSpawnEffect->m_fColorDelta[1] = 1.0f;
				pSpawnEffect->m_fColorDelta[2] = 1.0f;
				//strcpy(pSpawnEffect->m_szName, "SpawnFx");
				g_pRenderManager->AddSystem(pSpawnEffect, RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS);
			}
			if (g_pCvarEffects->value > 1.0f)
			{
				CRSCylinder *pFx = new CRSCylinder(origin+Vector(0,0,-4), 20.0f,10.0f, 48, 48, IEngineStudio.GetModelByIndex(g_iModelIndexLaser), kRenderTransAdd, r,g,b, 0.6f, -1.0f, 1.0);
				if (pFx)
				{
					pFx->m_vecVelocity.Set(0,0,-10);
					pFx->m_fTextureScrollRate = 2.0;
					//strcpy(pFx->m_szName, "SpawnFxC1");
					g_pRenderManager->AddSystem(pFx, RENDERSYSTEM_FLAG_NOCLIP);// XDM3035
				}
				// XDM3035a: more!
				// top ring
				pFx = new CRSCylinder(origin+Vector(0,0,HULL_MAX), 24.0f, 4.0f, 8, 32, g_pSpriteSpawnBeam, kRenderTransAdd, min(255,r+191),min(255,g+191),min(255,b+191), 0.95f, -1.0f, 1.2f);
				if (pFx)
				{
					pFx->m_vecVelocity.Set(0,0,-28);
					const float z = 0.5f;
					pFx->m_fColorDelta[0] = -z*(255-r);
					pFx->m_fColorDelta[1] = -z*(255-g);
					pFx->m_fColorDelta[2] = -z*(255-b);
					pFx->m_iFrame = 1;
					pFx->m_fFrameRate = 0;
					pFx->m_fTextureScrollRate = 4.0;
					//strcpy(pFx->m_szName, "SpawnFxC2");
					g_pRenderManager->AddSystem(pFx, RENDERSYSTEM_FLAG_NOCLIP);
				}
				// thin ring
				pFx = new CRSCylinder(origin+Vector(0,0,VEC_VIEW-6), 26.0f, 2.0f, 2, 32, g_pSpriteLaserBeam, kRenderTransAdd, min(255,r+223),min(255,g+223),min(255,b+223), 0.9f, -1.0f, 2.0f);
				if (pFx)
				{
					pFx->m_vecVelocity.Set(0,0,16);
					pFx->m_fColorDelta[0] = 2;
					pFx->m_fColorDelta[1] = 2;
					pFx->m_fColorDelta[2] = 2;
					//pFx->m_fFrame = 0;
					pFx->m_fFrameRate = 0;
					pFx->m_fTextureScrollRate = -6.0;
					//strcpy(pFx->m_szName, "SpawnFxC3");
					g_pRenderManager->AddSystem(pFx, RENDERSYSTEM_FLAG_NOCLIP);
				}
				// bottom ring (don't draw too low, or it'll leak through floor and reveal player spawn location)
				pFx = new CRSCylinder(origin+Vector(0,0,HULL_MIN+4), 22.0f, 6.0f, 8, 32, g_pSpriteSpawnBeam, kRenderTransAdd, min(255,r+32),min(255,g+32),min(255,b+32), 1.0f, -1.0f, 1.0f);
				if (pFx)
				{
					pFx->m_vecVelocity.Set(0,0,20);
					pFx->m_iFrame = 0;
					pFx->m_fFrameRate = 0;
					pFx->m_fTextureScrollRate = -4.0;
					//strcpy(pFx->m_szName, "SpawnFxC4");
					g_pRenderManager->AddSystem(pFx, RENDERSYSTEM_FLAG_NOCLIP);
				}
			}
		}

		EMIT_SOUND(args->entindex, origin, CHAN_WEAPON, "player/respawn.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);// CHAN_WEAPON stops looping sounds, like gauss spin sound.

		if (g_pCvarEffects->value > 0.0f)
		{
			DynamicLight(args->origin, 64, r,g,b, 1.0, 128);
			//gEngfuncs.pEfxAPI->R_ParticleBurst(args->origin, 10, 208, 0.1);// TODO: replace with team color

			if (g_pCvarEffects->value > 1.0f)
			{
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,-22), Vector(90,0,0), g_pSpriteWarpGlow1,
					kRenderTransAdd, min(255,r+63),min(255,g+63),min(255,b+63), 0.9, -0.75, 0.6f, -0.6f, 1.0f, 1.0f), RENDERSYSTEM_FLAG_NOCLIP);
				//int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive);
				//gEngfuncs.pEfxAPI->R_TempSprite(origin, Vector(0,0,-2), 0.1f, g_iModelIndexWarpGlow1, kRenderGlow, kRenderFxNoDissipation, 1.0f, 0.0f, FTENT_FADEOUT);
			}
		}
	}
}

static char g_szCaptureMessage[64];
static client_textmessage_t g_CaptureMessage;
//-----------------------------------------------------------------------------
// Purpose: Called everytime the flag state changes
// entindex - 
// origin - 
// angles - 
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_CaptureObject(struct event_args_s *args)
{
	EV_START(args);
#if defined (_DEBUG_GAMERULES)
	EV_PrintParams("EV_CaptureObject", args);
#endif
	int idx = args->iparam1;// flag entity index
	int event = args->iparam2;// CTF_EV_TAKEN
	int team = args->bparam1;// flag team

	//int r,g,b;
	//GetTeamColor(team, r,g,b);

	if (idx > 1)
	{
		if (g_pRenderManager)
		{
			CRSLight *pLight = NULL;
			pLight = (CRSLight *)g_pRenderManager->FindSystemByFollowEntity(idx);
			if (pLight != NULL)// found, update
			{
				VectorCopy(args->origin, pLight->m_vecOrigin);
				//pLight->m_fScale = args->fparam1;
				if (g_pCvarEffects->value > 1.0f)
				{
					if (event == CTF_EV_TAKEN)
						pLight->m_RadiusCallback = FlagPulseLightRadiusCallback;
					else
						pLight->m_RadiusCallback = NULL;
				}
			}
			/*else
			{
				//conprintf(1, "Creating new light for entity %d...\n", idx);
				int r,g,b;
				GetTeamColor(args->iparam1, r,g,b);
				CRSLight *pLight = new CRSLight(args->origin, r,g,b, 128, NULL, 0.0, 0.0, idx);
				if (pLight != NULL)
					g_pRenderManager->AddSystem(pLight, RENDERSYSTEM_FLAG_NOCLIP);
			}*/
		}
	}
	//conprintf(1, " EV_CaptureObject: entindex %d event %d team %d\n", args->entindex, event, team);

	//if (args->entindex > 0 && args->entindex <= MAX_PLAYERS)
	if (gHUD.m_iGameType == GT_CTF)
	{
		client_textmessage_t *msg = NULL;
		if (EV_IsPlayer(args->entindex))
		{
			if (event == CTF_EV_DROP)
			{
				msg = TextMessageGet("CTF_DROPPED");
				// sprite flash
				if (g_pRenderManager && g_pCvarEffects->value > 0.0 && !UTIL_PointIsFar(args->origin))
				{
					byte r,g,b;
					GetTeamColor(args->iparam1, r,g,b);
					g_pRenderManager->AddSystem(new CRSCylinder(args->origin, 8.0f,10.0f, 64, 16, IEngineStudio.GetModelByIndex(g_iModelIndexShockWave), kRenderTransAdd, r,g,b, 0.6f, -1.0f, 1.0), RENDERSYSTEM_FLAG_NOCLIP);
					if (g_pCvarParticles->value > 0.0)
					{
						// TODO: if flag state changes, find THIS system and shut it down. WARNING! There's no way to find system by it's class!
						CPSFlameCone *pSys = new CPSFlameCone(24, args->origin, Vector(0,0,1), VECTOR_CONE_10DEGREES, 400.0f, g_pSpriteIceBall2, kRenderTransAdd, 1.0f, -0.1f, 0.5f, -0.25f, args->fparam1);
						if (pSys)
						{
							pSys->m_color.r = r;//min(255, r+64);
							pSys->m_color.g = g;//min(255, g+64);
							pSys->m_color.b = b;//min(255, b+64);
							pSys->m_fColorDelta[0] = pSys->m_fColorDelta[1] = pSys->m_fColorDelta[2] = 0.5f;
							pSys->m_iTraceFlags = PM_WORLD_ONLY;
							g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS|RENDERSYSTEM_FLAG_ADDGRAVITY);
						}
					}
				}
			}
			else if (event == CTF_EV_RETURN)
			{
				msg = TextMessageGet("CTF_RETURNED");
				//PlaySound("!CTF_RET_PLR", VOL_NORM);
				if (team == gHUD.m_iTeamNumber)
					PlaySound("!CTF_RET_TEAM", VOL_NORM);
				else
					PlaySound("!CTF_RET_ENEMY", VOL_NORM);

				gHUD.m_flNextAnnounceTime = gHUD.m_flTime + 3.0f;

				if (g_pRenderManager)
				{
					// sprite flash
					byte r,g,b;
					GetTeamColor(args->iparam1, r,g,b);
					if (g_pCvarEffects->value > 1.0 && !UTIL_PointIsFar(args->origin))
						g_pRenderManager->AddSystem(new CRSCylinder(args->origin, 20.0f,-10.0f, 64, 16, IEngineStudio.GetModelByIndex(g_iModelIndexShockWave), kRenderTransAdd, r,g,b, 0.6f, -1.0f, 1.0), RENDERSYSTEM_FLAG_NOCLIP);

					r = min(255, r+64);
					g = min(255, g+64);
					b = min(255, b+64);
					g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteIceBall1, kRenderTransAdd, r,g,b, 1.0, -2.0, 1.0,-4.0, 20, 1.0), RENDERSYSTEM_FLAG_NOCLIP);
				}
			}
			else if (event == CTF_EV_CAPTURED)
			{
				if (team == gHUD.m_iTeamNumber)// my flag was captured, aww... :(
				{
					PlaySound("!CTF_CAP_ENEMY", VOL_NORM);
				}
				else
				{
					char sentence[16];
					_snprintf(sentence, 16, "!SCORESOUND%d\0", RANDOM_LONG(0,3));// HACK: hardcoded because the system only plays sentences by name, no randomization
					PlaySound(sentence, VOL_NORM);
					//PlaySound("game/ctf_captured.wav", VOL_NORM);
					//EMIT_SOUND(args->entindex, args->origin, CHAN_STATIC, "game/ctf_captured.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
				}
				if (g_pCvarTFX->value > 0.0f)
				{
					if (g_pRenderManager)// (args->iparam2 > 0?args->iparam2:g_iModelIndexAnimglow01)
					{
						byte r,g,b;
						GetTeamColor(team, r,g,b);
						g_pRenderManager->AddSystem(new CPSSparks((g_pCvarParticles->value > 1.0f)?96:64, args->origin, 1.0f,0.8f,-0.5f, -80.0f, 2.2f, r,g,b,1.0f,-1.0f, IEngineStudio.GetModelByIndex(g_iModelIndexAnimglow01), kRenderTransAdd, 1.0), RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP);
					}
				}

				gHUD.m_flNextAnnounceTime = gHUD.m_flTime + 3.0f;
				msg = TextMessageGet("CTF_CAPTURED");
			}
			else if (event == CTF_EV_TAKEN)
			{
				if (team == gHUD.m_iTeamNumber)// MY flag has been taken! Loud alarm for all teammates!
				{
					PlaySound("!CTF_GOT_ENEMY", VOL_NORM);
					//PlaySound("game/ctf_alarm.wav", VOL_NORM);
				}
				else
				{
					if (EV_IsLocal(args->entindex))//if (args->entindex == localplayer)
					{
						//PlaySound("!CTF_GOT_PLR", VOL_NORM);
						conprintf(0, "You've got the flag!\n");
					}
					else
					{
						PlaySound("!CTF_GOT_TEAM", VOL_NORM);
					}
					EMIT_SOUND(args->entindex, args->origin, CHAN_STATIC, "game/ctf_alarm.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
				}

				msg = TextMessageGet("CTF_TAKEN");
			}

			if (msg)
				_snprintf(g_szCaptureMessage, 64, msg->pMessage, g_PlayerInfoList[args->entindex].name, GetTeamName(team));
		}
		else
		{
			if (event == CTF_EV_RETURN)
			{
				msg = TextMessageGet("CTF_RET_SELF");

				if (msg)
					_snprintf(g_szCaptureMessage, 64, msg->pMessage, GetTeamName(team));
			}
		}

		//ConsolePrint("* ");
		//ConsolePrint(g_szCaptureMessage);// XDM3035a
		//ConsolePrint("\n");
		conprintf(0, "* %s\n", g_szCaptureMessage);
		if (msg)
		{
			memcpy(&g_CaptureMessage, msg, sizeof(client_textmessage_t));
			//g_CaptureMessage = *msg;// copy localized message
			g_CaptureMessage.x = -1;// override some parameters
			//g_CaptureMessage.y = 0.9;
			g_CaptureMessage.a1 = 255;
			msg->holdtime = 3.0;
			g_CaptureMessage.pName = "CTF_MSG";
			g_CaptureMessage.pMessage = g_szCaptureMessage;
			gHUD.m_Message.MessageAdd(&g_CaptureMessage);
		}

		if (&gHUD.m_FlagDisplay)
			gHUD.m_FlagDisplay.SetEntState(idx, idx, event);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player (entindex) touches a domination entity
// entindex - 
// origin - 
// angles - 
// fparam1 - light radius
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_DomPoint(struct event_args_s *args)
{
	EV_START(args);
#if defined (_DEBUG_GAMERULES)
	EV_PrintParams("EV_DomPoint", args);
#endif
	// others must hear this as well PlaySound("game/dom_touch.wav", VOL_NORM);
	EMIT_SOUND(args->entindex, args->origin, CHAN_STATIC, "game/dom_touch.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	int r,g,b;
	GetTeamColor(args->iparam1, r,g,b);

	/* :(	cl_entity_t *ent = gEngfuncs.GetEntityByIndex();
	if (ent != NULL)
	{
		ent->curstate.rendercolor.r = r;
		ent->curstate.rendercolor.g = g;
		ent->curstate.rendercolor.b = b;
	}*/

	if (g_pRenderManager)
	{
		CRSLight *pLight = (CRSLight *)g_pRenderManager->FindSystemByFollowEntity(args->entindex);
		if (pLight)// found, update
		{
			pLight->m_color.r = r;
			pLight->m_color.g = g;
			pLight->m_color.b = b;
			pLight->m_fScale = args->fparam1;
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSLight(args->origin, r,g,b, args->fparam1, NULL, 0.0, 0.0), RENDERSYSTEM_FLAG_NOCLIP, args->entindex, RENDERSYSTEM_FFLAG_ICNF_NODRAW);
		}

		if (g_pCvarEffects->value > 0.0)
		{
			CRSCylinder *pFx = new CRSCylinder(args->origin, 40.0f,20.0f, 32, 32, IEngineStudio.GetModelByIndex(g_iModelIndexLaser), kRenderTransAdd, r,g,b, 1.0f, -2.0f, 1.5);
			if (pFx)
			{
				pFx->m_fFrameRate = 20.0f;
				pFx->m_vecVelocity.Set(0,0,120);
				g_pRenderManager->AddSystem(pFx, RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP);
			}
		}

		if (g_pCvarParticles->value > 0.0f)
		{
			CPSSparks *pSys = new CPSSparks(((g_pCvarParticles->value > 1.0f)?32:24), args->origin + Vector(0,0,16), 2.0f,1.0f,0.5f, 120.0f, 2.0f, r,g,b,1.0f,-2.0f, g_pSpriteIceBall2, kRenderTransAdd, 1.0f);
			if (pSys)
			{
				pSys->m_vecVelocity.Set(0,0,80);
				g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP);
			}
		}
	}

	if (&gHUD.m_DomDisplay)
		gHUD.m_DomDisplay.SetEntTeam(args->entindex, args->iparam1);
}

//-----------------------------------------------------------------------------
// Called when a player (entindex) teleports via trigger_teleport
// origin = dest
// angles = src
// Obsolete: message is now used
//-----------------------------------------------------------------------------
/*void EV_Teleport(struct event_args_s *args)
{
	EV_START(args);
	conprintf(1, "ERROR: EV_Teleport()\n");
}*/
