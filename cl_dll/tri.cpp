// Triangle rendering, if any
#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "triangleapi.h"
#include "RenderManager.h"// XDM
#include "gl_dynamic.h"
#include "cl_fx.h"
#include "pm_shared.h"
#include "Exports.h"
#include "tri.h"
#include "particleman.h"
#include "voice_status.h"

extern IParticleMan *g_pParticleMan;
model_t *g_pPlayerIDTexture = NULL;

#define PLAYER_HIGHLIGHT_DISTANCE			800
#define PLAYER_HIGHLIGHT_ICON_OFFSET		8

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: Long-awaited player team highlight sprites
//-----------------------------------------------------------------------------
void RenderPlayerTeamHighlight(void)
{
	if (g_pPlayerIDTexture == NULL)
		return;

	static float fFrame = 0;
	static Vector vecSignPos, delta, v_right, rx, uy, v_up(0,0,1);
	float fSizeX, fSizeY;
	byte r, g, b;

	fFrame = (float)(fFrame + gHUD.m_flTimeDelta*20);
	while (fFrame >= g_pPlayerIDTexture->numframes)
		fFrame -= g_pPlayerIDTexture->numframes;

	// cheat-proof constant sizes
	fSizeX = 4;//	fSizeX = (g_pPlayerIDTexture->maxs[1] - g_pPlayerIDTexture->mins[1])*0.125f;// 0.0625?
	fSizeY = 4;//	fSizeY = (g_pPlayerIDTexture->maxs[2] - g_pPlayerIDTexture->mins[2])*0.125f;

	for (CLIENT_INDEX idx = 1; idx <= gEngfuncs.GetMaxClients(); ++idx)
	{
		if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_iInsetMode == INSET_IN_EYE) && idx == g_iUser2)
			continue;// don't draw the player we are following in eye
		if (!IsActivePlayer(idx))
			continue;

		cl_entity_t *ent = GetUpdatingEntity(idx);
		if (ent == NULL)
			continue;
		if (ent == gHUD.m_pLocalPlayer)
			continue;
		// too far	if (UTIL_PointIsFar(ent->origin))
		if ((ent->origin-g_vecViewOrigin).Length() > PLAYER_HIGHLIGHT_DISTANCE)
			continue;
		//cut by server		if (!CL_CheckVisibility(ent->origin))
		//			continue;

		if (gEngfuncs.pTriAPI->SpriteTexture(g_pPlayerIDTexture, (int)fFrame))
		{
			GetPlayerColor(idx, r,g,b);

			/*somehow, this looks bad			if (ent->curstate.usehull == HULL_PLAYER_CROUCHING)
				vecSignPos = ent->origin + VEC_DUCK_VIEW;
			else*/
				vecSignPos = ent->origin + VEC_VIEW_OFFSET;

			vecSignPos.z += /*fSizeY*0.5 + */PLAYER_HIGHLIGHT_ICON_OFFSET;
			delta = (vecSignPos - g_vecViewOrigin).Normalize();
			v_right = CrossProduct(delta, v_up).Normalize();
			rx = v_right * fSizeX;
			uy = v_up * fSizeY;
			gEngfuncs.pTriAPI->RenderMode(SpriteRenderMode((msprite_t *)g_pPlayerIDTexture->cache.data));
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);
			if (ent->curstate.health == -1 || ent->curstate.solid == SOLID_NOT)// health is always 0 for enemies 
				gEngfuncs.pTriAPI->Brightness(0.25f);
			else
				gEngfuncs.pTriAPI->Brightness(1.0f);

			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.01f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos + rx + uy);
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.99f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos + rx - uy);
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.99f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos - rx - uy);
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.01f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos - rx + uy);
			gEngfuncs.pTriAPI->End();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Replacement for old indicators which were client entities
//-----------------------------------------------------------------------------
void RenderPlayerTalkStatus(void)
{
	if (!g_pSpriteVoiceIcon)
		return;

	cl_entity_s *pClient;
	static float fFrame = 0;
	static Vector vecSignPos, delta, v_right, rx, uy, v_up(0,0,1);
	float fSizeX, fSizeY;
	byte r, g, b;

	fFrame = (float)(fFrame + gHUD.m_flTimeDelta*20);
	while (fFrame >= g_pSpriteVoiceIcon->numframes)
		fFrame -= g_pSpriteVoiceIcon->numframes;

	// cheat-proof constant sizes
	fSizeX = 6;//	fSizeX = (g_pSpriteVoiceIcon->maxs[1] - g_pSpriteVoiceIcon->mins[1])*0.125f;// 0.0625?
	fSizeY = 6;//	fSizeY = (g_pSpriteVoiceIcon->maxs[2] - g_pSpriteVoiceIcon->mins[2])*0.125f;

	for (size_t i=0; i < VOICE_MAX_PLAYERS; i++)
	{
		if (!GetClientVoiceMgr()->m_VoicePlayers[i])
			continue;

		pClient = GetUpdatingEntity(i+1);// XDM3037a: gEngfuncs.GetEntityByIndex(i+1);
		if (pClient == NULL)
			continue;
		// Don't show an icon for the local player unless we're in thirdperson mode.
		if (pClient == gHUD.m_pLocalPlayer && !g_ThirdPersonView)
			continue;
		// Don't show an icon for dead or spectating players (ie: invisible entities).
		if (pClient->curstate.effects & EF_NODRAW)
			continue;

		if (gEngfuncs.pTriAPI->SpriteTexture(g_pSpriteVoiceIcon, (int)fFrame))
		{
			GetPlayerColor(i+1, r,g,b);

			/*somehow, this looks bad	if (pClient->curstate.usehull == HULL_PLAYER_CROUCHING)
				vecSignPos = pClient->origin + VEC_DUCK_VIEW;
			else*/
				vecSignPos = pClient->origin + VEC_VIEW_OFFSET;

				vecSignPos.z += /*fSizeY*0.5 + */clamp(g_pCvarVoiceIconOffset->value,2,64);// OBSOLETE GetClientVoiceMgr()->m_VoiceHeadModelHeight;
			delta = (vecSignPos - g_vecViewOrigin).Normalize();
			v_right = CrossProduct(delta, v_up).Normalize();
			rx = v_right * fSizeX;
			uy = v_up * fSizeY;
			gEngfuncs.pTriAPI->RenderMode(SpriteRenderMode((msprite_t *)g_pSpriteVoiceIcon->cache.data));
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);
			if (pClient->curstate.health == -1 || pClient->curstate.solid == SOLID_NOT)// health is always 0 for enemies 
				gEngfuncs.pTriAPI->Brightness(0.25f);
			else
				gEngfuncs.pTriAPI->Brightness(1.0f);

			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.01f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos + rx + uy);
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.99f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos + rx - uy);
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.99f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos - rx - uy);
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.01f);
			gEngfuncs.pTriAPI->Vertex3fv(vecSignPos - rx + uy);
			gEngfuncs.pTriAPI->End();
		}
	}
}


/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void CL_DLLEXPORT HUD_DrawNormalTriangles(void)
{
//	RecClDrawNormalTriangles();

	if (gHUD.m_iActive > 0)
	{
		if (gHUD.m_Spectator.DrawOverview(false) == false)// don't duplicate in overview window
		{
			//if (!gHUD.m_Spectator.m_OverviewMapDrawPass)
			{
				if (g_pRenderManager)
					g_pRenderManager->RenderOpaque();
			}
		}
	}

	if (gHUD.m_iFogMode > 0 && !gHUD.m_Spectator.m_OverviewMapDrawPass)
		RenderFog(0,0,0,0,0,true);
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void CL_DLLEXPORT HUD_DrawTransparentTriangles(void)
{
//	RecClDrawTransparentTriangles();

	if (gHUD.m_iActive > 0)
	{
		if (gHUD.m_Spectator.DrawOverview(true) == false)// don't duplicate in overview window
		{
			//if (!gHUD.m_Spectator.m_OverviewMapDrawPass)
			{
				if (g_pRenderManager)
					g_pRenderManager->RenderTransparent();

				if (g_pParticleMan)// HL20130901: was outside if() blocks
					g_pParticleMan->Update();

				if (g_pCvarHLPlayers->value > 0 && IsTeamGame(gHUD.m_iGameType))
					RenderPlayerTeamHighlight();// XDM3037a

				RenderPlayerTalkStatus();// XDM3038a
			}
		}
	}
	//does not help	if (gHUD.m_iFogMode > 0 && !gHUD.m_Spectator.m_OverviewMapDrawPass)
	//	RenderFog(0,0,0,0,0,true);
}

extern int g_iWaterLevel;
static float fog_color[3] = {127.0f, 127.0f, 127.0f};// 3 floats, but still 0-255
GLfloat fog_color_gl[4] = {0.5f, 0.5f, 0.5f, 1.0f};// 4 real floats, 0.0 - 1.0
//static float fog_startdist = 0.0f;
//static float fog_enddist = 1024.0f;

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : r,g,b - fog color
//			fStartDist - start drawing fog from N units from view origin (0 intensity)
//			fEndDist - end drawing from N units from view origin (max intensity)
//			updateonly - ignore input parameters, just updates previously set fog
//-----------------------------------------------------------------------------
void RenderFog(byte r, byte g, byte b, float fStartDist, float fEndDist, bool updateonly)
{
	if (updateonly == false)
	{
		fog_color[0] = (float)r;
		fog_color[1] = (float)g;
		fog_color[2] = (float)b;
		fog_color_gl[0] = fog_color[0]/255.0f;
		fog_color_gl[1] = fog_color[1]/255.0f;
		fog_color_gl[2] = fog_color[2]/255.0f;
		gHUD.m_flFogStart = fStartDist;
		gHUD.m_flFogEnd = fEndDist;
		//conprintf(1, "cl: RenderFog(%f %f %f, %f, %f) updateonly == false\n", fog_color[0],fog_color[1],fog_color[2], gHUD.m_flFogStart, gHUD.m_flFogEnd);
	}
	// render fog only when NOT in water!
	bool bFog = g_iWaterLevel < 3 && fStartDist >= 0.0f && fEndDist >= 0.0f;
	//gEngfuncs.pTriAPI->Fog(fog_color, gHUD.m_flFogStart, gHUD.m_flFogEnd, bFog);

	//conprintf(1, "cl: RenderFog() %d\n", hw);
	if (gHUD.m_iHardwareMode == 1)
	{
		if (bFog)
		{
#if defined (CLDLL_NEWFUNCTIONS)
			gEngfuncs.pTriAPI->FogParams(1.0, 0);// XDM3037
#endif
#if !defined (CLDLL_NOFOG)
			if (GL_glEnable)// OpenGL external library was loaded
			{
				GL_glEnable(GL_FOG);
				GL_glFogi(GL_FOG_MODE, GL_LINEAR);// GL_EXP?
				GL_glFogfv(GL_FOG_COLOR, fog_color_gl);
				GL_glFogf(GL_FOG_DENSITY, 1.0f);
				GL_glHint(GL_FOG_HINT, GL_NICEST);
				GL_glFogf(GL_FOG_START, gHUD.m_flFogStart);
				GL_glFogf(GL_FOG_END, gHUD.m_flFogEnd);
			}
#endif
		}
		//else
		//	GL_glDisable(GL_FOG);
	}
	else//if (gHUD.m_iHardwareMode == 0 || gHUD.m_iHardwareMode == 2)// fall back to simple engine function
	{
		gEngfuncs.pTriAPI->Fog(fog_color, gHUD.m_flFogStart, gHUD.m_flFogEnd, bFog);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Disable fog effect and reset all parameters.
//-----------------------------------------------------------------------------
void ResetFog(void)
{
	conprintf(1, "cl: ResetFog\n");
	fog_color[0] = 127.0f;
	fog_color[1] = 127.0f;
	fog_color[2] = 127.0f;
	fog_color_gl[0] = fog_color[0]/255.0f;
	fog_color_gl[1] = fog_color[1]/255.0f;
	fog_color_gl[2] = fog_color[2]/255.0f;
	gHUD.m_flFogStart = 0.0f;
	gHUD.m_flFogEnd = 0.0f;
	if (gHUD.m_iHardwareMode == 1 && GL_glEnable)// OpenGL
		GL_glDisable(GL_FOG);
	else
		gEngfuncs.pTriAPI->Fog(fog_color, 0.0f, 0.0f, 0);
}
