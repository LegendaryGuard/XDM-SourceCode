//====================================================================
//
// Purpose: XDM doesn't use HL camera methods, utilizing refdef instead.
// Here goes the "click on screen" code
//
//====================================================================
#include "hud.h"
#include "cl_util.h"
//#include "camera.h"
#include "in_defs.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pmtrace.h"
#include "com_model.h"
#include "triangleapi.h"
#include "r_studioint.h"
#include "r_efx.h"
#include "shared_resources.h"
#include "cl_fx.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "vgui_EntityEntryPanel.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSSprite.h"
#include "RSModel.h"
#include "Exports.h"
//#include "SDL2/SDL_mouse.h"
//#include "port.h"

extern int mouse_x, mouse_y;
//extern vec3_t g_vecViewForward;

//const float cam_offset[3] = {0.0f,0.0f,64.0f};// XDM: 3034 ?
const vec3_t cam_offset(0.0f,0.0f,0.0f);


//-----------------------------------------------------------------------------
// Purpose: Camera code is in V_CalcThirdPersonRefdef
// Warning: This is the only global function that is called before adding entities
//-----------------------------------------------------------------------------
void CL_DLLEXPORT CAM_Think(void)
{
//	DBG_CL_PRINT("CAM_Think()\n");
	/* BAD POINTER DURING LEVEL CHANGE if (gHUD.m_pLocalPlayer)
	{
		//gHUD.m_pLocalPlayer->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;
		//gHUD.m_pLocalPlayer->curstate.angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;
		DBG_ANGLES_NPRINT(5, "CAM_Think() pitch: %f, curstate %f", gHUD.m_pLocalPlayer->angles[PITCH], gHUD.m_pLocalPlayer->curstate.angles[PITCH]);
		DBG_ANGLES_DRAW(5, gHUD.m_pLocalPlayer->origin, gHUD.m_pLocalPlayer->angles, "CAM_Think()");
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: ???
// Input  : *ofs - 
//-----------------------------------------------------------------------------
void CL_DLLEXPORT CL_CameraOffset(float *ofs)
{
//	DBG_CL_PRINT("CL_CameraOffset()\n");
//	RecClCL_GetCameraOffsets(ofs);
	VectorCopy(cam_offset, ofs);
}

//-----------------------------------------------------------------------------
// Purpose: Tells the engine that current view is not in first person mode
// Output : int 1 true 0 false
//-----------------------------------------------------------------------------
int CL_DLLEXPORT CL_IsThirdPerson(void)
{
//	DBG_CL_PRINT("CL_IsThirdPerson()\n");
//	RecClCL_IsThirdPerson();

	// XDM3037: this is fine with HL, but causes really bad things in Xash3D
	if (g_ThirdPersonView)// XDM: this tells if we are REALLY TECHNICALLY watching in 3rd person
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: IsThirdPersonAlloed
// Output : byte 1 - yes
//-----------------------------------------------------------------------------
byte CL_IsThirdPersonAllowed(void)
{
	if (gHUD.m_iGameType == GT_SINGLE || (gHUD.m_iGameFlags & GAME_FLAG_ALLOW_CAMERA))//gHUD.m_pCvarDeveloper->value > 0.0f)
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Trace mouse click into real world
// Input  : &mouse_x &mouse_y - mouse coordinates
//			traceFlags - PM_STUDIO_BOX|PM_WORLD_ONLY
//			*ptrace - output: pmtrace_t
//			tracestart - output: use to get screen2world position (optional)
// Output : Returns true on hit in empty space, false otherwise.
//-----------------------------------------------------------------------------
bool TraceClick(int &mousex, int &mousey, int traceFlags, pmtrace_t *ptrace, Vector *tracestart = NULL)
{
	if (ptrace)
	{
		memset(ptrace, 0, sizeof(pmtrace_t));
		vec3_t screen, src, end;
		screen[0] = XUNPROJECT((float)mousex);
		screen[1] = -YUNPROJECT((float)mousey);
		screen[2] = 0.0f;
		gEngfuncs.pTriAPI->ScreenToWorld(screen, src);// WARNING! This doesn't work properly when minimap or spectator windows are active!
		screen[2] = 1.0f;
		gEngfuncs.pTriAPI->ScreenToWorld(screen, end);// end goes into infinity
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, false);// skip local client in THIS case only!
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		//pmtrace_t pmtrace;
		//pmtrace_t *ptrace = &pmtrace;

		// find local player's physent to ignore
		int pe_ignore = PM_GetPhysent(gEngfuncs.GetLocalPlayer()->index);
		//ASSERT(pe_ignore > 0);
#if defined (_DEBUG)
		gEngfuncs.pEfxAPI->R_BeamPoints(src, end, g_iModelIndexLaser, 2.0f, 0.1, 0.0f, 1.0f, 10.0, 0, 30, 1,0,0);
#endif
		gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, traceFlags, pe_ignore, ptrace);// 1st, this can hit players
		//ptrace = gEngfuncs.PM_TraceLine(src, end, PM_TRACELINE_PHYSENTSONLY, 2, -1);
		gEngfuncs.pEventAPI->EV_PopPMStates();

		if (tracestart)
			*tracestart = src;

		if (ptrace->inopen && ptrace->fraction < 1.0f)
			return true;
	}
	return false;
}



uint32 g_iMouseManipulationMode = MMM_NONE;
//int g_iMouseLastEvent = 0;
int g_iUsedEntity = 0;
RS_INDEX g_iEntityCreationRS = 0;
cl_entity_t *g_pPickedEntity = NULL;// TODO: WARNING! This using this pointer is very risky! It may become invalid if the entity gets removed (or the player disconnected)
//cl_entity_t *g_pUsedEntity = NULL;

//-----------------------------------------------------------------------------
// Purpose: Mouse event
// Input  : &button - which button (starting from 0)
//			state - pressed or released
//-----------------------------------------------------------------------------
void CL_MouseEvent(const int &button, byte state)
{
	if (CON_IS_VISIBLE() != 0)// in steam HL it is useless
		return;

	//CON_PRINTF("CL_MouseEvent(%d %d)\n", button, state);
	char str[128];
	str[0] = '\0';

	if (g_iMouseManipulationMode != MMM_NONE && gHUD.m_Spectator.ShouldDrawOverview())
	{
		//_snprintf(str, 128, "* Disable all inset views!\n(minimap, overview, spectator, etc.)\n");
		LocaliseTextString("#MMM_ERROR_INSET", str, 128);
		g_iMouseManipulationMode = MMM_NONE;
		//return;
	}

	if (g_iMouseManipulationMode == MMM_USE)// +USE
	{
		if (state == 1)// pressed
		{
			if (g_iUsedEntity == 0)
			{
				pmtrace_t pmtrace;// trace new location
				Vector vSrc;
				if (TraceClick(mouse_x, mouse_y, PM_STUDIO_BOX, &pmtrace, &vSrc))// PM_WORLD_ONLY ignores func_walls, but otherwise trace hits the player himself
				{
					if (pmtrace.ent)
					{
						g_iUsedEntity = gEngfuncs.pEventAPI->EV_IndexFromTrace(&pmtrace);
						if (g_iUsedEntity > 0)// not the world
						{
							_snprintf(str, 128, ".u %d %d %d", g_iUsedEntity, state, (button == 0)?1:0);// TODO: continuous use?
							SERVER_COMMAND(str);
							str[0] = 0;
							//_snprintf(str, 128, "* Using entity %d\n", g_iUsedEntity);
						}
					}
				}
			}
			//else
				//sprintf(str, "* Already using an entity\n");
		}
		else if (state == 0)// released
		{
			if (g_iUsedEntity)
			{
				_snprintf(str, 128, ".u %d 3 0", g_iUsedEntity);// XDM3037: state '3' means button was released
				SERVER_COMMAND(str);
				str[0] = 0;
				//_snprintf(str, 128, "* Unusing entity %d\n", g_iUsedEntity);
				g_iUsedEntity = 0;
			}
		}
	}
	else if (g_iMouseManipulationMode == MMM_PICK)
	{
		if (button == 0)// MOUSE1: pick
		{
			if (state == 1)// pressed //UNDONE: event should be triggered when the button is released
			{
				vec3_t screen, vSrc, vEnd;
				screen[0] = XUNPROJECT((float)mouse_x);
				screen[1] = -YUNPROJECT((float)mouse_y);
				screen[2] = 0.0f;
				gEngfuncs.pTriAPI->ScreenToWorld(screen, vSrc);
				screen[2] = 1.0f;
				gEngfuncs.pTriAPI->ScreenToWorld(screen, vEnd);// end goes into infinity
				//gEngfuncs.pEfxAPI->R_TempSprite(src, (float *)g_vecZero, 0.01f, g_iModelIndexAnimglow01, kRenderTransAdd, kRenderFxNone, 1.0f, life, 0);
				//gEngfuncs.pEfxAPI->R_BeamPoints(src, end, g_iModelIndexLaser, 1.0f, 0.25f, 0.0f, 1.0f, 10.0, 0, 30, 0,1,0);
				//gEngfuncs.pEfxAPI->R_TempSprite(end, (float *)g_vecZero, 0.1f, g_iModelIndexZeroGlow, kRenderTransAdd, kRenderFxNone, 1.0f, 1.0f, FTENT_FADEOUT);
				// :( gEngfuncs.pNetAPI->SendRequest(
				char scmd[128];
				_snprintf(scmd, 128, ".p %g %g %g %g %g %g\0", vSrc[0], vSrc[1], vSrc[2], vEnd[0], vEnd[1], vEnd[2]);
				SERVER_COMMAND(scmd);
			}
		}
		else if (button == 1)// MOUSE2: move
		{
			if (g_pPickedEntity && g_pPickedEntity->index > 0)
			{
				if (state == 1)// pressed // same here
				{
					pmtrace_t pmtrace;
					//pmtrace_t *ptrace = &pmtrace;
					Vector vSrc;
					if (TraceClick(mouse_x, mouse_y, PM_STUDIO_BOX|PM_WORLD_ONLY, &pmtrace, &vSrc))
					{
						gEngfuncs.pEfxAPI->R_BeamPoints(vSrc, pmtrace.endpos, g_iModelIndexLaser, 2.0f, 0.25, 0.0f, 1.0f, 10.0, 0, 30, 0,0,1);
						_snprintf(str, 128, ".m %d \"%g %g %g\"", g_pPickedEntity->index, pmtrace.endpos.x, pmtrace.endpos.y, pmtrace.endpos.z);// XDM3038c
						// BAD because doesn't use SetOrigin()! _snprintf(str, 128, "searchindex %d set origin \"%g %g %g\"", g_pPickedEntity->index, ptrace->endpos.x, ptrace->endpos.y, ptrace->endpos.z);
						SERVER_COMMAND(str);
						_snprintf(str, 128, "* Moving entity %d to %g %g %g\n", g_pPickedEntity->index, pmtrace.endpos.x, pmtrace.endpos.y, pmtrace.endpos.z);
					}
				}
				else
					str[0] = 0;
			}
			else
				LocaliseTextString("#MMM_NOSELECTION", str, 128);//_snprintf(str, 128, "* Nothing is selected\n");
		}
	}
	else if (g_iMouseManipulationMode == MMM_CREATE)
	{
		if (state == 1)// pressed // same here
		{
			if (button == 0)// MOUSE1: create
			{
				if (g_iEntityCreationRS != 0)// check for old/bogus/invalid entity
				{
					if (g_pRenderManager)
					{
						if (g_pRenderManager->FindSystem(g_iEntityCreationRS) == NULL)
							g_iEntityCreationRS = 0;
					}
				}
				if (g_iEntityCreationRS == 0)// if not in the process
				{
					pmtrace_t pmtrace;// trace new location
					Vector vSrc;
					if (TraceClick(mouse_x, mouse_y, PM_STUDIO_IGNORE|PM_STUDIO_BOX|PM_WORLD_ONLY, &pmtrace, &vSrc))// PM_WORLD_ONLY ignores func_walls, but otherwise trace hits the player himself
					{
						gEngfuncs.pEfxAPI->R_BeamPoints(vSrc, pmtrace.endpos, g_iModelIndexLaser, 2.0f, 0.25, 0.0f, 1.0f, 10.0, 0, 30, 0,0,1);
						//gEngfuncs.pEfxAPI->R_TempSprite(pmtrace.endpos, (float *)g_vecZero, 0.5f, g_iModelIndexAnimglow01, kRenderGlow, kRenderFxNoDissipation, 1.0f, 2.0f, FTENT_FADEOUT);
						if (g_pRenderManager)
						{
							CRenderSystem *pSystem = new CRSSprite(pmtrace.endpos, g_vecZero, g_pSpriteAnimGlow01, kRenderTransAdd, 255,191,255, 1.0f,-0.5f, 0.5f,0.0f, 10.0f, 0.0f);
							if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES) != RS_INDEX_INVALID)
								strcpy(pSystem->m_szName, "MMM_CREATE_S");

							int iModelIndex = 0;
							model_t *pModel = gEngfuncs.CL_LoadModel("models/w_weaponbox.mdl", &iModelIndex);
							ASSERT(iModelIndex > 0);
							if (pModel)
							{
								Vector vEntOrigin(pmtrace.plane.normal);// = pmtrace.endpos + pmtrace.plane.normal*4
								Vector vEntAngles;// = UTIL_VecToAngles(pmtrace.plane.normal);
								vEntOrigin *= 4.0f; vEntOrigin += pmtrace.endpos;
								VectorAngles(pmtrace.plane.normal, vEntAngles);
#if defined (CORRECT_PITCH)
								vEntAngles.x += 90.0f;
#else
								vEntAngles.x += 270.0f;
#endif
								NormalizeAngles(vEntAngles);
								//vEntAngles[YAW] = g_vecViewAngles[YAW];// only works for horizontal surface
								pSystem = new CRSModel(vEntOrigin, vEntAngles, g_vecZero, 0, pModel, 0, 0, 0, kRenderTransTexture, kRenderFxStrobeFast, 191,191,255, 0.75f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
								g_iEntityCreationRS = g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES);
								if (g_iEntityCreationRS != RS_INDEX_INVALID)
								{
									strcpy(pSystem->m_szName, "MMM_CREATE_M");
									CEntityEntryPanel *pPanel = new CEntityEntryPanel(-1,-1, XRES(EEP_WIDTH), YRES(EEP_HEIGHT));
									if (pPanel)
									{
										pPanel->m_vTargetOrigin = vEntOrigin;
										pPanel->m_vTargetAngles = vEntAngles;
										pPanel->m_iRenderSystemIndex = g_iEntityCreationRS;
										gViewPort->ShowMenuPanel(pPanel, false);
									}
								}
							}
						}
						g_iMouseManipulationMode = MMM_NONE;// WARNING! Disallow clicking!
					}
					else
						LocaliseTextString("#MMM_UNREACHABLE", str, 128);//sprintf(str, "* Unreachable location\n");
				}
			}
			else if (button == 1)// MOUSE2: UNDONE: edit
			{
				// TODO: send cmd, pick entity on server, return msg here, open key/value dialog. But we can't retrieve existing KV pairs!
			}
		}// pressed
	}
	else if (g_iMouseManipulationMode == MMM_MEASURE)// XDM3038c
	{
		if (state == 1)// pressed //UNDONE: event should be triggered when the button is released
		{
			pmtrace_t pmtrace;
			Vector vSrc;
			if (TraceClick(mouse_x, mouse_y, PM_STUDIO_IGNORE|PM_STUDIO_BOX|PM_WORLD_ONLY, &pmtrace, &vSrc))// PM_WORLD_ONLY ignores func_walls, but otherwise trace hits the player himself
			{
				Vector vDelta(pmtrace.endpos); vDelta -= vSrc;
				gEngfuncs.pEfxAPI->R_BeamPoints(vSrc, pmtrace.endpos, g_iModelIndexLaser, 2.0f, 0.5, 0.0f, 1.0f, 10.0, 0, 30, 0,1,0);
				//gEngfuncs.pEfxAPI->R_TempSprite(pmtrace.endpos, (float *)g_vecZero, 0.25f, g_iModelIndexAnimglow01, kRenderTransAdd, (button == 1)?kRenderFxStrobeFaster:kRenderFxNone, 1.0f, 2.0f, FTENT_FADEOUT);
				if (g_pRenderManager)
				{
					CRenderSystem *pSystem = new CRSSprite(pmtrace.endpos, g_vecZero, g_pSpriteAnimGlow01, kRenderTransAdd, 255,191,255, 1.0f,-0.5f, 0.5f,0.0f, 10.0f, 0.0f);
					if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, -1, RENDERSYSTEM_FFLAG_DONTFOLLOW) != RS_INDEX_INVALID)
						strcpy(pSystem->m_szName, "MMM_MEASURE");
				}
				vec_t fLen1 = vDelta.Length();
				vDelta = pmtrace.endpos;vDelta -= g_vecViewOrigin;//gHUD.m_vecOrigin;
				if (button == 1)// MOUSE2: remember
				{
					_snprintf(str, 128, "mycoordedit %g %g %g", pmtrace.endpos.x, pmtrace.endpos.y, pmtrace.endpos.z);// z + HULL_MAX??
					SERVER_COMMAND(str);
				}
				_snprintf(str, 128, "* %g %g %g, dist:%g (%g to view point)\n", pmtrace.endpos.x, pmtrace.endpos.y, pmtrace.endpos.z, fLen1, vDelta.Length());
			}
			else
				LocaliseTextString("#MMM_UNREACHABLE", str, 128);//sprintf(str, "* Unreachable location\n");
		}
	}

	if (str[0] != '\0')
	{
		CenterPrint(str);
		ConsolePrint(str);
	}
}

//-----------------------------------------------------------------------------
// Purpose: A reply from server arrives: entity picked
// Input  : entindex - 
//			&hitpoint - 
//-----------------------------------------------------------------------------
void CL_EntitySelected(int entindex, const vec3_t &hitpoint)
{
	if (entindex <= 0)
	{
		//CenterPrint("CL_EntitySelected: server missed\n");
		//?g_pPickedEntity = NULL;
		return;
	}
	cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(entindex);
	if (pEntity == NULL)
	{
		conprintf(0, "CL_EntitySelected(%d): error getting entity by index!\n", entindex);
		return;
	}
	//gEngfuncs.pEfxAPI->R_TempSprite(hitpoint, (float *)g_vecZero, 0.1f, g_iModelIndexAnimglow01, kRenderTransAdd, kRenderFxNone, 1.0f, 2.0f, FTENT_FADEOUT);
	if (g_pRenderManager)
	{
		CRenderSystem *pSystem = new CRSSprite(hitpoint, g_vecZero, g_pSpriteAnimGlow01, kRenderTransAdd, 191,255,191, 1.0f,-0.5f, 0.1f,0.0f, 10.0f, 0.0f);
		if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, -1, RENDERSYSTEM_FFLAG_DONTFOLLOW) != RS_INDEX_INVALID)
			strcpy(pSystem->m_szName, "CL_EntitySelected");
	}
	char str[128];
	// if (g_iMouseManipulationMode == MMM_PICK)
	//{
		if (pEntity == g_pPickedEntity)
		{
			LocaliseTextString("#MMM_RELEASED", str, 128);//_snprintf(str, 128, "* Released %d\n", g_pPickedEntity->index);
			g_pPickedEntity = NULL;
		}
		else
		{
			g_pPickedEntity = pEntity;
			const char *strname = NULL;
			// EV_GetPhysent returns NULL in multiplayer
			//physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent(ptrace->ent);
			//if (pe)
			//	strname = pe->name;
			//else
			if (IsActivePlayer(entindex))
				strname = g_PlayerInfoList[g_iUser2].name;
			else if (pEntity->model)
				strname = pEntity->model->name;
			else
				strname = BufferedLocaliseTextString("#NOINFO");//"no info";

			LocaliseTextString("#MMM_PICKED", str, 128);
			char str2[64];
			_snprintf(str2, 64, " %d (%s) @ (%g %g %g)\n", pEntity->index, strname, pEntity->origin[0], pEntity->origin[1], pEntity->origin[2]);
			str2[63] = '\0';
			strncat(str, str2, min(128-strlen(str),64));
		}
	/*}
	else if (g_iMouseManipulationMode == MMM_CREATE)
	{
		ShowEntEditDialog();
	}*/
	if (str[0] != '\0')
	{
		CenterPrint(str);
		ConsolePrint(str);
	}
}
