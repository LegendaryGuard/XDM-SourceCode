//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Client side entity management functions
//#include <memory.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "event_api.h"
#include "r_efx.h"
#include "pmtrace.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_materials.h"
#include "com_model.h"// XDM
#include "r_studioint.h"
#include "studio.h"
#include "studio_util.h"
#include "RenderManager.h"
#include "shared_resources.h"
#include "cl_fx.h"
#include "weapondef.h"
#include "voice_status.h"
#include "customentity.h"
#include "com_weapons.h"
// HL20130901
#include "bench.h"
#include "Exports.h"
#if defined (USE_PARTICLEMAN)
#include "particleman.h"
extern IParticleMan *g_pParticleMan;
#endif

//int g_iAlive = 1;
DLL_GLOBAL int g_iUser1;
DLL_GLOBAL int g_iUser2;// Use this ONLY as camera target, NOT a winner or something logical
DLL_GLOBAL int g_iUser3;// Same here
DLL_GLOBAL double g_cl_gravity;// XDM3035: value is same as sv_gravity (800)
//float g_fEntityHighlightAngle = 0;// XDM3037: TODO?

// WARNING! Find all references to this array and fix size numbers if they change!
model_t **g_pMuzzleFlashSprites[NUM_MUZZLEFLASH_SPRITES+1] =
{
	&g_pSpriteMuzzleflash0,
	&g_pSpriteMuzzleflash1,
	&g_pSpriteMuzzleflash2,
	&g_pSpriteMuzzleflash3,
	&g_pSpriteMuzzleflash4,
	&g_pSpriteMuzzleflash5,
	&g_pSpriteMuzzleflash6,
	&g_pSpriteMuzzleflash7,
	&g_pSpriteMuzzleflash8,
	&g_pSpriteMuzzleflash9,
	NULL
};


//-----------------------------------------------------------------------------
// Purpose: Add entity to visible list for rendering
// WARNING: Angles are affected by SQB here!!! But do not fix them here because more SQB-infected engine code is executed afterwards. There's also a problem with the local player:
// Note   : Local player won't be processed here in the first person view.
// Input  : type - ET_NORMAL, etc.
//			*ent -
//			modelname -
// Output : 1/0 - add entity to visible list
//-----------------------------------------------------------------------------
int CL_DLLEXPORT HUD_AddEntity(int type, struct cl_entity_s *ent, const char *modelname)
{
	DBG_CL_PRINT("HUD_AddEntity(%d, %d)\n", type, ent->index);
//	RecClAddEntity(type, ent, modelname);
	// test if (ent->curstate.effects & EF_NODRAW)
	//	return 0;

	// Dear valve, you're a bunch of morons for clearing attachments of players!!

	// works in all cases except first person which we cannot detect here
	if (ent->player && ent != gHUD.m_pLocalPlayer)//gHUD.m_pLocalPlayer && m_pCurrentEntity->index == gHUD.m_pLocalPlayer->index)
		ent->angles[PITCH] *= -PITCH_CORRECTION_MULTIPLIER;

	DBG_ANGLES_DRAW(6, ent->origin, ent->angles, ent->index, "HUD_AddEntity()");

	// each frame every entity passes this function, so the overview hooks it to filter the overview entities in spectator mode:
	if (gHUD.m_Spectator.ShouldDrawOverview() && (ent->curstate.eflags & EFLAG_ADDTOMAP))
		gHUD.m_Spectator.AddOverviewEntity(ent, /*ent->curstate.iuser4 > 0?IEngineStudio.GetModelByIndex(ent->curstate.iuser4):*/NULL, 0.0f, gHUD.m_flTimeDelta+0.01);// XDM3035c: iuser4 is now icon sprite index

	if (ent->index == g_iUser2 && gHUD.m_pLocalPlayer && (gHUD.m_pLocalPlayer->curstate.effects & EF_DIMLIGHT))// XDM3038b: spectator target highlight (flashlight impulse)
	{
		if (g_iUser1 == OBS_CHASE_LOCKED || g_iUser1 == OBS_CHASE_FREE)
			ent->curstate.effects |= EF_BRIGHTLIGHT;
		// bad else if (g_iUser1 == OBS_IN_EYE)
		//	ent->curstate.effects |= EF_DIMLIGHT;
	}

	if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_iInsetMode == INSET_IN_EYE) && ent->index == g_iUser2)
		return 0;// don't draw the player we are following in eye

	if (gHUD.m_iPaused == 0)// XDM: a chace to draw additional entity effects
	{
		if (type == ET_NORMAL)
		{
#if defined (ENABLE_BENCKMARK)
			Bench_CheckEntity(type, ent, modelname);// HL20130901
#endif

			/*todo	if (ent->curstate.eflags & EFLAG_HIGHLIGHT)
			{
				g_fEntityHighlightAngle += gHUD.m_flTimeDelta*test1->value;
				NormalizeAngle360(&g_fEntityHighlightAngle);
				ent->curstate.angles[YAW] += g_fEntityHighlightAngle;
				NormalizeAngle360(&ent->curstate.angles[YAW]);
			}*/
			//if (ent->curstate.weaponmodel)
			//	CreateTEntWithThatModel()?
		}
		else if (type == ET_BEAM)//(ent->curstate.entityType == ENTITY_BEAM)
		{
			if (ent->curstate.iStepLeft > 0 && ent->curstate.renderamt > 127 && g_pCvarEffects->value > 1.0f)
			{
				int ei = BEAMENT_ENTITY(ent->curstate.skin);// from CBeam::GetEndEntity
				vec_t dl = 0;
				cl_entity_t *pEnd = gEngfuncs.GetEntityByIndex(ei);
				Vector d;
				if (pEnd)
				{
					d = pEnd->origin - ent->curstate.origin;
					dl = d.Length();
				} 
				//gEngfuncs.pEfxAPI->R_SparkEffect(ent->curstate.origin + RANDOM_FLOAT(0,1)*d, 6, -180, 180);
				//gEngfuncs.pEfxAPI->R_ShowLine(ent->curstate.origin, pEnd->origin);//ent->curstate.angles);

				if (ent->curstate.iStepLeft == BEAMFX_GLUON1)// gluon primary
				{
					if (pEnd && RANDOM_FLOAT(0,1600) < dl)// limit number of sparks at close distances
						FX_StreakSplash(ent->curstate.origin + RANDOM_FLOAT(0,1)*d, g_vecZero, ent->curstate.rendercolor, 4, 200.0f, true, true, false);

					if (ent->curstate.playerclass & BEAM_FSPARKS_START)// hit
						gEngfuncs.pEfxAPI->R_StreakSplash(ent->curstate.origin, g_vecZero, 7, 16, 32.0f, -240, 240);// "r_efx.h"
					if (ent->curstate.playerclass & BEAM_FSPARKS_END)
						gEngfuncs.pEfxAPI->R_StreakSplash(ent->curstate.angles, g_vecZero, 7, 16, 32.0f, -240, 240);// "r_efx.h"
				}
				else if (ent->curstate.iStepLeft == BEAMFX_GLUON2)
				{
					if (pEnd && RANDOM_FLOAT(0,1600) < dl)
						FX_StreakSplash(ent->curstate.origin + RANDOM_FLOAT(0,1)*d, g_vecZero, ent->curstate.rendercolor, 6, 200.0f, true, true, false);
					if (ent->curstate.playerclass & BEAM_FSPARKS_START)// hit
						gEngfuncs.pEfxAPI->R_StreakSplash(ent->curstate.origin, g_vecZero, 7, 12, 40.0f, -240, 240);
					if (ent->curstate.playerclass & BEAM_FSPARKS_END)
						gEngfuncs.pEfxAPI->R_StreakSplash(ent->curstate.angles, g_vecZero, 7, 12, 40.0f, -240, 240);
				}
				else if (ent->curstate.iStepLeft == BEAMFX_PLASMA1)// plasma primary
				{
					if (pEnd && RANDOM_FLOAT(0,1600) < dl)
						gEngfuncs.pEfxAPI->R_SparkEffect(ent->curstate.origin + RANDOM_FLOAT(0,1)*d, 4, -180, 180);// beam
					if (ent->curstate.playerclass & BEAM_FSPARKS_START)// hit
					{
						FX_StreakSplash(ent->curstate.origin, g_vecZero, gTracerColors[0], 4, 200.0f, true, true, false);
						if (g_pCvarEffects->value > 1.0f)
							FX_StreakSplash(ent->curstate.origin, g_vecZero, gTracerColors[15], 3, 320.0f, true, true, false);
					}
					if (ent->curstate.playerclass & BEAM_FSPARKS_END)
					{
						FX_StreakSplash(ent->curstate.angles, g_vecZero, gTracerColors[0], 4, 200.0f, true, true, false);
						if (g_pCvarEffects->value > 1.0f)
							FX_StreakSplash(ent->curstate.angles, g_vecZero, gTracerColors[15], 3, 320.0f, true, true, false);
					}
				}
				else if (ent->curstate.iStepLeft == BEAMFX_PLASMA2)
				{
					if (pEnd && RANDOM_FLOAT(0,1600) < dl)
						gEngfuncs.pEfxAPI->R_SparkEffect(ent->curstate.origin + RANDOM_FLOAT(0,1)*d, 6, -180, 180);// beam
					if (ent->curstate.playerclass & BEAM_FSPARKS_START)// hit
					{
						FX_StreakSplash(ent->curstate.origin, g_vecZero, gTracerColors[7], 3, 200.0f, true, true, true);
						if (g_pCvarEffects->value > 1.0f)
							FX_StreakSplash(ent->curstate.origin, g_vecZero, gTracerColors[17], 4, 320.0f, true, true, false);
					}
					if (ent->curstate.playerclass & BEAM_FSPARKS_END)// hit
					{
						FX_StreakSplash(ent->curstate.angles, g_vecZero, gTracerColors[7], 3, 200.0f, true, true, true);
						if (g_pCvarEffects->value > 1.0f)
							FX_StreakSplash(ent->curstate.angles, g_vecZero, gTracerColors[17], 4, 320.0f, true, true, false);
					}
				}
			}
		}
		/*pmtrace_t pmtrace;
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		gEngfuncs.pEventAPI->EV_PlayerTrace(dst->origin, dst->origin-Vector(0,0,1024), PM_STUDIO_BOX, -1, &pmtrace);*/
		//if (ent->curstate.onground == -1)// XDM3037: no data
		//if (ent->player)
		//	ent->curstate.effects |= EF_BRIGHTFIELD;
		//else
		//	ent->curstate.effects &= ~EF_BRIGHTFIELD;
	}// !Paused

	if (ent->player == 0)
	{
		if (type == ET_NORMAL)//(ent->curstate.entityType == ENTITY_)
		{
			if (ent->curstate.effects & EF_DIMLIGHT)// XDM3035c: use rendercolor for light
			{
				if (ent->curstate.rendercolor.r != 0 ||
					ent->curstate.rendercolor.g != 0 ||
					ent->curstate.rendercolor.b != 0)
				{
					// Engine will nullify and return existing light structure with this key
					dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(ent->index);
					if (dl)
					{
						VectorCopy(ent->origin, dl->origin);
						dl->die = gEngfuncs.GetClientTime() + 0.01f; // die on next frame
						dl->color = ent->curstate.rendercolor;
						//dl->color.r = dl->color.g = dl->color.b = br;
						if (ent->curstate.renderamt > 0)
							dl->radius = (float)ent->curstate.renderamt;
						else
							dl->radius = 200;
					}
				}
			}
		}

		/*test fail	if (ent->curstate.eflags & EFLAG_HIGHLIGHT)
		{
			ent->angles[1] += gHUD.m_flTimeDelta*test1->value;
			if (test1->value > 0)
				NormalizeAngle(&(ent->angles[1]));
		}*/
	}
#if defined (_DEBUG)
		ASSERT(g_vecZero.IsZero());
#endif
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
// playerstate update in entity_state_t. In order for these overrides to eventually get to the appropriate playerstate
// structure, we need to copy them into the state structure at this point.
// Input  : *state - destination (player->curstate)
//			*client -
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_TxferLocalOverrides(struct entity_state_s *state, const struct clientdata_s *client)
{
	DBG_CL_PRINT("HUD_TxferLocalOverrides(%d)\n", state->number);
//	RecClTxferLocalOverrides(state, client);

	VectorCopy(client->origin, state->origin);
	//state->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;// XDM3038c: SQB
	//VectorCopy(client->velocity, state->velocity);
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;
	state->iuser3 = client->iuser3;
	state->iuser4 = client->iuser4;
	state->fuser1 = client->fuser1;
	state->fuser2 = client->fuser2;
	state->fuser3 = client->fuser3;
	state->fuser4 = client->fuser4;

	// XDM3038: reverted to this method
	gHUD.m_iWeaponBits = client->weapons;
	gHUD.m_Ammo.UpdateWeaponBits();// XDM3038c
	//conprintf(1, "HUD_TxferLocalOverrides(): RECV weapons: %d\n", client->weapons);
	gHUD.m_Ammo.UpdateCurrentWeapon(client->m_iId);
	//conprintf(1, "HUD_TxferLocalOverrides(): RECV m_iId: %d\n", client->m_iId);

	// kinda slow	memcpy(&gHUD.m_ClientData, client, sizeof(clientdata_s));// XDM3037a
	gHUD.m_fFOVServer = client->fov;// XDM3038a
	gHUD.m_Health.SetHealth(client->health);// XDM3037a

	//if (ent->curstate.onground == -1)// XDM3037: no data
	//if (ent->player)
	//	state->effects |= EF_BRIGHTFIELD;
	//else
	//	ent->curstate.effects &= ~EF_BRIGHTFIELD;
}

//Vector g_lastangles;

//-----------------------------------------------------------------------------
// Purpose: Copy appropriate fields from entity_state_t to the playerstate structure. Called after TxferLocalOverrides()
// Note   : Struct pointers are dynamic, do NOT cache them!
// Input  : *dst - zeroed-out destination (newframe->playerstate)
//			*src - ent->curstate
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_ProcessPlayerState(struct entity_state_s *dst, const struct entity_state_s *src)
{
	DBG_CL_PRINT("HUD_ProcessPlayerState(%d)\n", src->number);
//	RecClProcessPlayerState(dst, src);

	cl_entity_t *pLocalPlayer = gEngfuncs.GetLocalPlayer();// Get the local player's index
	// XDM3035: right now most of this data is invalid and useless
	// Do not copy fields that are possibly set on client side
	//memcpy(dst, src, sizeof(entity_state_s));// DO NOT! Most data in src is invalid!
	// Copy in network data
	dst->entityType				= src->entityType;
	/*dst->number				= src->number;
	dst->msg_time				= src->msg_time;
	dst->messagenum				= src->messagenum;*/
	dst->origin					= src->origin;
	dst->angles					= src->angles;
	//if (dst->number != pLocalPlayer->index)
		//WORKED1 dst->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;// XDM3038c: WARNING!!! REQUIRES DT_FLOAT IN delta.lst!!!
#if defined (SV_NO_PITCH_CORRECTION)
	dst->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;
#else
	dst->angles[PITCH] *= -1.0f;// works with PreThink() correction on the server
#endif
	// TEST dst->angles[0] *= cl_test1->value;
	//dst->angles[0] += cl_test2->value;
	/*Vector vecDest;
	AngleVectors(dst->angles, vecDest, NULL, NULL);
	vecDest *= cl_test3->value; vecDest += dst->origin;
	gEngfuncs.pEfxAPI->R_SparkEffect(vecDest, 1, 0,0);*/
	DBG_ANGLES_DRAW(4, dst->origin, dst->angles, dst->number, "HUD_ProcessPlayerState() dst");//DBG_ANGLES_NPRINT(4, "ProcessPlayerState() pitch: %f -> %f", src->angles[PITCH], dst->angles[PITCH]);

	dst->modelindex				= src->modelindex;
	dst->sequence				= src->sequence;
	dst->frame					= src->frame;
	dst->colormap				= src->colormap;
	dst->skin					= src->skin;
	dst->solid					= src->solid;
	dst->effects				= src->effects;
	dst->scale					= src->scale;
	dst->eflags					= src->eflags;
	dst->rendermode				= src->rendermode;
	dst->renderamt				= src->renderamt;
	dst->rendercolor			= src->rendercolor;
	dst->renderfx				= src->renderfx;
	dst->movetype				= src->movetype;
	dst->animtime				= src->animtime;
	dst->framerate				= src->framerate;
	dst->body					= src->body;
	dst->controller[0]			= src->controller[0];
	dst->controller[1]			= src->controller[1];
	dst->controller[2]			= src->controller[2];
	dst->controller[3]			= src->controller[3];
	dst->blending[0]			= src->blending[0];
	dst->blending[1]			= src->blending[1];
	dst->blending[2]			= src->blending[2];
	dst->blending[3]			= src->blending[3];
	dst->velocity				= src->velocity;
	/*dst->mins					= src->mins;
	dst->maxs					= src->maxs;
	dst->aiment					= src->aiment;
	dst->owner					= src->owner;*/
	dst->friction				= src->friction;
	dst->gravity				= src->gravity;
	// PLAYER SPECIFIC
	dst->team					= src->team;
	dst->playerclass			= src->playerclass;
	dst->health					= src->health;
	dst->spectator				= src->spectator;
	dst->weaponmodel			= src->weaponmodel;
	dst->gaitsequence			= src->gaitsequence;
	//VectorCopy(src->basevelocity, dst->basevelocity);
	dst->basevelocity			= src->basevelocity;
	dst->usehull				= src->usehull;
	//INVALID!	dst->oldbuttons				= src->oldbuttons;// XDM3035c: causes jump bug
	dst->onground				= src->onground;
	/*dst->iStepLeft			= src->iStepLeft;
	dst->flFallVelocity			= src->flFallVelocity;
	XDM3037a: this will NEVER be possible: some morons forgot to register this field in the engine!	dst->fov					= src->fov;
	dst->weaponanim				= src->weaponanim;*/
	dst->startpos				= src->startpos;
	dst->endpos					= src->endpos;
	dst->impacttime				= src->impacttime;
	dst->starttime				= src->starttime;
	dst->iuser1					= src->iuser1;
	dst->iuser2					= src->iuser2;
	dst->iuser3					= src->iuser3;
	dst->iuser4					= src->iuser4;
	dst->fuser1					= src->fuser1;
	dst->fuser2					= src->fuser2;
	dst->fuser3					= src->fuser3;
	dst->fuser4					= src->fuser4;
	dst->vuser1					= src->vuser1;
	dst->vuser2					= src->vuser2;
	dst->vuser3					= src->vuser3;
	dst->vuser4					= src->vuser4;
	// Save off some data so other areas of the Client DLL can get to it
	if (dst->number == pLocalPlayer->index)
	{
		//memcpy(&gHUD.m_LocalPlayerState, src, sizeof(entity_state_s));// XDM3035
		// this always fail ASSERTD(&gHUD.m_pLocalPlayer->curstate == src);
		gHUD.m_iTeamNumber = src->team;// XDM3037: should we?
		//gHUD.m_iTeamNumberLast = src->playerclass;// XDM3038a: OBSOLETE
		gHUD.m_pLocalPlayer = pLocalPlayer;// DANGEROUS!!!
		//gHUD.m_Health.SetHealth(src->health);// XDM3037a
		g_iUser1 = src->iuser1;
		g_iUser2 = src->iuser2;
		g_iUser3 = src->iuser3;
		//conprintf(1, "RECV health: %d %d\n", dst->health, src->health);
	}
	//else if (IsSpectator(dst->number))// XDM3038a: spectators are not sent to the client!
	//	g_PlayerExtraInfo[dst->number].teamnumber = src->team;
}

//-----------------------------------------------------------------------------
// Purpose: Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
// from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
// up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server update is occupying.
// Input  : *ps - player state
//			*pps - predicted player state
//			*pcd - clientdata_s
//			*ppcd - predicted clientdata_s
//			*wd - weapon_data_s[]
//			*pwd - predicted weapon_data_s[]
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_TxferPredictionData(struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd)
{
	DBG_CL_PRINT("HUD_TxferPredictionData(%d)\n", ps->number);
//	RecClTxferPredictionData(ps, pps, pcd, ppcd, wd, pwd);

	//DBG_ANGLES_NPRINT(8, "TxferPredictionData() pitch: %f -> %f", ps->angles[PITCH], pps->angles[PITCH]);
	DBG_ANGLES_DRAW(8, ps->origin, ps->angles, ps->number, "TxferPredictionData() ps");

	// entity_state_s
	ps->oldbuttons				= pps->oldbuttons;
	ps->flFallVelocity			= pps->flFallVelocity;
	ps->iStepLeft				= pps->iStepLeft;
	ps->playerclass				= pps->playerclass;

	// clientdata_s
	pcd->viewmodel				= ppcd->viewmodel;
	pcd->m_iId					= ppcd->m_iId;
#if defined( CLIENT_WEAPONS )
	pcd->ammo_shells			= ppcd->ammo_shells;
	pcd->ammo_nails				= ppcd->ammo_nails;
	pcd->ammo_cells				= ppcd->ammo_cells;
	pcd->ammo_rockets			= ppcd->ammo_rockets;
#endif
	pcd->m_flNextAttack			= ppcd->m_flNextAttack;
	pcd->fov					= ppcd->fov;
	pcd->weaponanim				= ppcd->weaponanim;
	pcd->tfstate				= ppcd->tfstate;
	pcd->maxspeed				= ppcd->maxspeed;
	pcd->deadflag				= ppcd->deadflag;
	// Spectating or not dead == get control over view angles.
	//g_iAlive = (ppcd->iuser1 || (pcd->deadflag == DEAD_NO))?1:0;

	if (gEngfuncs.IsSpectateOnly())
	{
		// in specator mode we tell the engine who we want to spectate and how
		// iuser3 is not used for duck prevention (since the spectator can't duck at all)
		pcd->iuser1 = g_iUser1;	// observer mode
		pcd->iuser2 = g_iUser2; // first target
		pcd->iuser3 = g_iUser3; // second target
	}
	else
	{
		pcd->iuser1 = ppcd->iuser1;// Spectator
		pcd->iuser2 = ppcd->iuser2;
		pcd->iuser3 = ppcd->iuser3;// Duck prevention
	}
	pcd->iuser4 = ppcd->iuser4;// Fire prevention
	pcd->fuser1 = ppcd->fuser1;
	pcd->fuser2 = ppcd->fuser2;
	pcd->fuser3 = ppcd->fuser3;
	pcd->fuser4 = ppcd->fuser4;
	pcd->vuser1 = ppcd->vuser1;
	pcd->vuser2 = ppcd->vuser2;
	pcd->vuser3 = ppcd->vuser3;
	pcd->vuser4 = ppcd->vuser4;

	//	gHUD.m_vecViewOffset = ppcd->view_ofs;// XDM3037: use EV_LocalPlayerViewheight()
	/* weapon_data_s is part of the unpredictable weapon prediction bullshit, highly unreliable to use. DO NOT USE THIS, by the way
	int r;
	conprintf(1, "HUD_TxferPredictionData(): START\n");
	for (int i=0; i<MAX_WEAPONS; ++i)
	{
		r = memcmp(&wd[i], &pwd[i], sizeof(weapon_data_t));
		if (r != 0)
			conprintf(1, "HUD_TxferPredictionData(): wd[%d] id %d %s pwd[%d] id %d\n", i, wd[i].m_iId, r>0?">":"<", i, pwd[i].m_iId);
	}
	conprintf(1, "HUD_TxferPredictionData(): END\n");
	memcpy(wd, pwd, sizeof(weapon_data_t)*MAX_WEAPONS);
	UpdateLocalInventory(wd);*/
}

//-----------------------------------------------------------------------------
// Purpose: Gives us a chance to add additional entities to the render this frame
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_CreateEntities(void)
{
	DBG_CL_PRINT("HUD_CreateEntities()\n");
//	RecClCreateEntities();

	// Only local player should be affected by SQB
	if (gHUD.m_pLocalPlayer)
		gHUD.m_pLocalPlayer->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER*PITCH_CORRECTION_MULTIPLIER;
	//gHUD.m_pLocalPlayer->curstate.angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;//cl_test1->value;

	// Apply related fixes to all players
	for (CLIENT_INDEX cli=1; cli<=gEngfuncs.GetMaxClients(); ++cli)
	{
		cl_entity_t *pPlayer = GetUpdatingEntity(cli);
		if (pPlayer && IsActivePlayer(cli))
		{
/*#if defined (_DEBUG_ANGLES)
			vec_t a = pPlayer->angles[PITCH];
			vec_t ca = pPlayer->curstate.angles[PITCH];
#endif
			pPlayer->angles[PITCH] *= cl_test1->value;//-PITCH_CORRECTION_MULTIPLIER;
			pPlayer->curstate.angles[PITCH] *= cl_test1->value;*/
			if (pPlayer != gHUD.m_pLocalPlayer)//gHUD.m_pLocalPlayer && m_pCurrentEntity->index == gHUD.m_pLocalPlayer->index)
				pPlayer->angles[PITCH] *= -PITCH_CORRECTION_MULTIPLIER;

			DBG_ANGLES_DRAW(7, pPlayer->origin, pPlayer->angles, cli, "HUD_CreateEntities()");

			if (pPlayer->curstate.effects & EF_DIMLIGHT)// XDM3035c: light that is blocked by players!
			{
				if (g_pCvarFlashLightMode && g_pCvarFlashLightMode->value > 0.0f)
				{
					if (!IsSpectator(cli) || (pPlayer == gHUD.m_pLocalPlayer))// XDM: if local player is spectator, he may want to highlight current target
						CL_UpdateFlashlight(pPlayer);// this will get called AFTER the engine creates its light, so that light will be updated... probably.
				}
			}
			//if (!(pPlayer->curstate.effects & EF_NODRAW))
			// XDM3037: take lighting from ceiling if it's closer than the floor
			pmtrace_t trace;
			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
			gEngfuncs.pEventAPI->EV_PushPMStates();
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(pPlayer->index-1);// the main purpose of the entire function
			gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
			int pe_ignore = PM_GetPhysent(pPlayer->index);
			Vector vecEnd(pPlayer->curstate.origin);
			vecEnd.z -= 1024.0f;
			gEngfuncs.pEventAPI->EV_PlayerTrace(pPlayer->curstate.origin, vecEnd, PM_STUDIO_IGNORE|PM_WORLD_ONLY|PM_GLASS_IGNORE, pe_ignore, &trace);
			vec_t down = (trace.endpos - pPlayer->curstate.origin).Length();
			vecEnd = pPlayer->curstate.origin;
			vecEnd.z += 1024.0f;
			gEngfuncs.pEventAPI->EV_PlayerTrace(pPlayer->curstate.origin, vecEnd, PM_STUDIO_IGNORE|PM_WORLD_ONLY|PM_GLASS_IGNORE, pe_ignore, &trace);
			vec_t up = (trace.endpos - pPlayer->curstate.origin).Length();
			gEngfuncs.pEventAPI->EV_PopPMStates();
			if (up < down)
				SetBits(pPlayer->curstate.effects, EF_INVLIGHT);
			else
				ClearBits(pPlayer->curstate.effects, EF_INVLIGHT);
		}
	}
	// e.g., create a persistent cl_entity_t somewhere.
	// Load an appropriate model into it ( gEngfuncs.CL_LoadModel )
	// Call gEngfuncs.CL_CreateVisibleEntity to add it to the visedicts list

#if defined (ENABLE_BENCKMARK)
	Bench_AddObjects();
#endif

	if (g_pRenderManager && g_pCvarRenderSystem->value > 0)// XDM3038a
		g_pRenderManager->CreateEntities();

#if defined (_DEBUG)
	ASSERT(g_vecZero.IsZero());// memory consistency check
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Old HL-compatible version for events 5001, 5011, 5021, 5031
// Note   : We won't normally have [0]th sprite
// Input  : *pEntity - 
//			attachment - 
//			arguments - 
//-----------------------------------------------------------------------------
void FX_MuzzleFlashStudioEvent(const struct cl_entity_s *pEntity, short attachment, int arguments)
{
	if (pEntity)
	{
		// UNDONE: "ab" is handled, but what about "abcd"?
		int scale = (int)(arguments/10);// 2
		int spr = arguments-(scale*10)+1;// 1 - same as in HL
		if (spr >= NUM_MUZZLEFLASH_SPRITES)//ARRAYSIZE(g_pMuzzleFlashSprites))// XDM3038c: safety and access to the 0th sprite
			spr = 0;

		if (*g_pMuzzleFlashSprites[spr])
			FX_MuzzleFlashSprite(GetCEntAttachment(pEntity, attachment), pEntity->index, attachment, *g_pMuzzleFlashSprites[spr], 0.1f*(float)scale, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: The entity's studio model description indicated an event was fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
// Input  : *event - 
//			*entity - 
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_StudioEvent(const struct mstudioevent_s *event, const struct cl_entity_s *entity)
{
	DBG_CL_PRINT("HUD_StudioEvent(%d, %d)\n", event->event, entity->index);
//	RecClStudioEvent(event, entity);
	/*ok, but useless
	if (entity == gEngfuncs.GetViewModel())
	{
		if (g_ThirdPersonView)
			return;
	}*/

	//if (g_pRenderManager)
	//	g_pRenderManager->AddSystem(new CRSSprite(GetCEntAttachment(entity, 0), g_vecZero, *g_pMuzzleFlashSprites[atoi(event->options)], kRenderTransAdd, 255,255,255, 1.0f,-10.0f, 1.0f,2.0f, 16.0f, 0.0f));
	switch (event->event)
	{
	case 5001: FX_MuzzleFlashStudioEvent(entity, 0, atoi(event->options)); break;
	case 5011: FX_MuzzleFlashStudioEvent(entity, 1, atoi(event->options)); break;
	case 5021: FX_MuzzleFlashStudioEvent(entity, 2, atoi(event->options)); break;
	case 5031: FX_MuzzleFlashStudioEvent(entity, 3, atoi(event->options)); break;

	case 5002: gEngfuncs.pEfxAPI->R_SparkEffect(GetCEntAttachment(entity, 0), atoi(event->options), -100, 100); break;
	case 5012: gEngfuncs.pEfxAPI->R_SparkEffect(GetCEntAttachment(entity, 1), atoi(event->options), -100, 100); break;
	case 5022: gEngfuncs.pEfxAPI->R_SparkEffect(GetCEntAttachment(entity, 2), atoi(event->options), -100, 100); break;
	case 5032: gEngfuncs.pEfxAPI->R_SparkEffect(GetCEntAttachment(entity, 3), atoi(event->options), -100, 100); break;

	case 5100:// args: "<attachment> <sprite muzzle index> <scale*100>"
		{
			int attachment, muzzleindex, ascale, scanret;
			char aflags[8];// flags as letters
			memset(aflags, 0, sizeof(aflags));
			bool rotate = false;
			bool light = false;
			scanret = sscanf(event->options, "%d %d %d %7s", &attachment, &muzzleindex, &ascale, &aflags[0]);
			if (scanret >= 3)// 3 required
			{
				if (scanret > 3)// 4th is optional
				{
					for (byte i = 0; i<sizeof(aflags); ++i)
					{
						if (aflags[i] == 'l')
							light = true;
						else if (aflags[i] == 'r')
							rotate = true;
					}
				}
				FX_MuzzleFlashSprite(GetCEntAttachment(entity, clamp(attachment,0,MAXSTUDIOATTACHMENTS)), entity->index, attachment, *g_pMuzzleFlashSprites[clamp(muzzleindex,0,NUM_MUZZLEFLASH_SPRITES-1)], 0.01f*(float)abs(ascale), rotate);
				if (light)
					EntityLight(GetCEntAttachment(entity, clamp(attachment,0,MAXSTUDIOATTACHMENTS)), 32.0f, 255,255,255, 0.1f, 100.0f);
			}
			else
				conprintf(2, "StudioEvent(%d) error: bad options in model \"%s\" sequence %d!\n", event->event, entity->model->name, entity->curstate.sequence);
		}
		break;

	case 5004: gEngfuncs.pfnPlaySoundByNameAtLocation((char *)event->options, 1.0, GetCEntAttachment(entity, 0)); break;
	case 5005:
		{
			//short &s = &entity->curstate.skin; 	*s = atoi(event->options);
			cl_entity_t *pEntity = NULL;// minor hack: stupid trick to override const
			if (entity == gEngfuncs.GetLocalPlayer())//if (entity == gHUD.m_pLocalPlayer)// important
				pEntity = (cl_entity_t *)entity;
			else if (entity == gEngfuncs.GetViewModel())// very important because viewmodel has the same entity index as player!
				pEntity = gEngfuncs.GetViewModel();
			else
				pEntity = gEngfuncs.GetEntityByIndex(entity->index);

			if (pEntity)
				pEntity->curstate.skin = atoi(event->options);
		}
		break; // XDM3038a
	case 7000: gEngfuncs.pEfxAPI->R_RicochetSound(GetCEntAttachment(entity, atoi(event->options))); break;// XDM
	case 7001: gEngfuncs.pEfxAPI->R_BulletImpactParticles(GetCEntAttachment(entity, atoi(event->options))); break;
	case 7002: gEngfuncs.pEfxAPI->R_BlobExplosion(GetCEntAttachment(entity, atoi(event->options))); break;
	case 7003: gEngfuncs.pEfxAPI->R_RocketFlare(GetCEntAttachment(entity, atoi(event->options))); break;
	case 7004: gEngfuncs.pEfxAPI->R_LargeFunnel(GetCEntAttachment(entity, atoi(event->options)), 0); break;
	case 7005: gEngfuncs.pEfxAPI->R_LargeFunnel(GetCEntAttachment(entity, atoi(event->options)), 1); break;
	case 7006: gEngfuncs.pEfxAPI->R_SparkShower(GetCEntAttachment(entity, atoi(event->options))); break;
	case 7007: gEngfuncs.pEfxAPI->R_FlickerParticles(GetCEntAttachment(entity, atoi(event->options))); break;
	case 7008: gEngfuncs.pEfxAPI->R_TeleportSplash(GetCEntAttachment(entity, atoi(event->options))); break;
	case 7009: gEngfuncs.pEfxAPI->R_ParticleExplosion(GetCEntAttachment(entity, atoi(event->options))); break;
	/*case 7010:
		{
			int a1,a2;
			if (sscanf(event->options, "%d %d", &a1, &a2) == 2 && a1 != a2)
				gEngfuncs.pEfxAPI->R_ShowLine(GetCEntAttachment(entity, a1), GetCEntAttachment(entity, a2));
			else
				conprintf(2, "StudioEvent(%d) error: bad options in model \"%s\" sequence %d!\n", event->event, entity->model->name, entity->curstate.sequence);
		}
		break;*/
	case 7011: gEngfuncs.pEfxAPI->R_StreakSplash(GetCEntAttachment(entity, 0), g_vecZero, atoi(event->options), 32, 100, -200, 200); break;
	case 7012:
		{
			float radius, life;
			int att,r,g,b;
			if (sscanf(event->options, "%d %f %d %d %d %f", &att, &radius, &r,&g,&b, &life) == 6)
				EntityLight(GetCEntAttachment(entity, clamp(att,0,MAXSTUDIOATTACHMENTS)), radius, r,g,b, life, 0.0f);
			else
				conprintf(2, "StudioEvent(%d) model design error: bad options in model \"%s\" sequence %d!\n", event->event, entity->model->name, entity->curstate.sequence);
		}
		break;
	default: conprintf(1, "Unknown event '%d' in model \"%s\" sequence %d!\n", event->event, entity->model->name, entity->curstate.sequence); break;
	}
}

#define TE_FX_UPDATE_PERIOD		0.25
#define TE_VELOCITY				entity.baseline.origin// VALVEHATE HACK
static double fAccumulatedPeriod = 0;// accumulate frametime until period is reached

//-----------------------------------------------------------------------------
// Purpose: Simulation and cleanup of temporary entities
// Input  : *Callback_AddVisibleEntity - 
//			*Callback_TempEntPlaySound - 
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_TempEntUpdate(
	double frametime,   // Simulation time
	double client_time, // Absolute time on client
	double cl_gravity,  // True gravity on client
	TEMPENTITY **ppTempEntFree,   // List of freed temporary ents
	TEMPENTITY **ppTempEntActive, // List 
	int (*Callback_AddVisibleEntity)(cl_entity_t *pEntity),
	void (*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp))
{
	DBG_CL_PRINT("HUD_TempEntUpdate()\n");
//	RecClTempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);
//	static int gTempEntFrame = 0;

	if (g_pWorld)
	{
		//if (g_pWorld->curstate.gravity < 10)// XDM3038a: server must send relative value!
			cl_gravity *= g_pWorld->baseline.gravity;// BUGBUG: HL engine does not update curstate.gravity;! We modify cl_gravity because it's used later in code
	}
	g_cl_gravity = cl_gravity;// XDM3035
	// XDM3038?	gHUD.m_flTimeDelta = frametime;

	DBG_CL_PRINT("HUD_TempEntUpdate()\n");
	/* TEST if (gHUD.m_pLocalPlayer)
	{
#if defined (CLDLL_FIX_PLAYER_ATTACHMENTS)
		gEngfuncs.pEfxAPI->R_MuzzleFlash(gHUD.m_pLocalPlayer->baseline.vuser1, 10);
		gEngfuncs.pEfxAPI->R_MuzzleFlash(gHUD.m_pLocalPlayer->baseline.vuser2, 11);
		gEngfuncs.pEfxAPI->R_MuzzleFlash(gHUD.m_pLocalPlayer->baseline.vuser3, 12);
		gEngfuncs.pEfxAPI->R_MuzzleFlash(gHUD.m_pLocalPlayer->baseline.vuser4, 13);
#else
		for (int att=0; att<4; ++att)
			gEngfuncs.pEfxAPI->R_MuzzleFlash(gHUD.m_pLocalPlayer->attachment[att], 10+att);// attachemts are == origin here! WHY??!!!!!
#endif
	}*/

	// Apply SQB fixes to all players
	/*for (CLIENT_INDEX cli=1; cli<=gEngfuncs.GetMaxClients(); ++cli)
	{
		cl_entity_t *pPlayer = GetUpdatingEntity(cli);
		if (pPlayer && IsActivePlayer(cli))
		{
			if (pPlayer != gHUD.m_pLocalPlayer)//gHUD.m_pLocalPlayer && m_pCurrentEntity->index == gHUD.m_pLocalPlayer->index)
				pPlayer->angles[PITCH] *= -PITCH_CORRECTION_MULTIPLIER;

			DBG_ANGLES_DRAW(15, pPlayer->origin, pPlayer->angles, cli, "HUD_TempEntUpdate()");
		}
	}*/
#if defined (_DEBUG_ANGLES)// just print
	int dbg_cl = (int)g_pCvarDbgAnglesClient->value;
	cl_entity_t *pPlayer = GetUpdatingEntity(dbg_cl);
	if (pPlayer && IsActivePlayer(dbg_cl)){
		DBG_ANGLES_DRAW(15, pPlayer->origin, pPlayer->angles, dbg_cl, "HUD_TempEntUpdate()");
	}
#endif

	// XDM3038c: better place
	if (g_pRenderManager && gHUD.m_iActive/* && gHUD.m_iIntermission == 0*/)// XDM: call this AFTER gHUD.Redraw()! // XDM3035c: finally m_iActive!
		g_pRenderManager->Update(gEngfuncs.GetClientTime(), frametime);

#if defined (USE_PARTICLEMAN)
	if (g_pParticleMan)// HL20130901
		g_pParticleMan->SetVariables(cl_gravity, g_vecViewAngles);
#endif

	// Nothing to simulate
	if (!*ppTempEntActive)
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);

	// !!!BUGBUG	-- This needs to be time based
	//gTempEntFrame = (gTempEntFrame+1) & 31;

	TEMPENTITY *pTemp, *pnext, *pprev;
	pTemp = *ppTempEntActive;
	double fastFreq = client_time * 5.5;
	float gravity = -frametime * cl_gravity;

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if (gHUD.m_iPaused > 0)// XDM: revisited :)
	{
		while (pTemp)
		{
			if (!FBitSet(pTemp->flags, FTENT_NOMODEL))
				Callback_AddVisibleEntity(&pTemp->entity);

			pTemp = pTemp->next;
		}
		goto finish;
	}

	pprev = NULL;
	float life;
	//freq = client_time * 0.01;

	if (fAccumulatedPeriod > TE_FX_UPDATE_PERIOD)// previous frame overcame one second interval
		fAccumulatedPeriod -= TE_FX_UPDATE_PERIOD;

	fAccumulatedPeriod += frametime;

	while (pTemp)
	{
		int active = 1;

		if (pTemp->die >= 0)
			life = (float)(pTemp->die - client_time);
		else
			life = 1.0;

		pnext = pTemp->next;
		if (life < 0)
		{
			if (FBitSet(pTemp->flags, FTENT_FADEOUT))
			{
				if (pTemp->entity.curstate.rendermode == kRenderNormal)
					pTemp->entity.curstate.rendermode = kRenderTransTexture;

				pTemp->entity.curstate.renderamt = (int)((float)pTemp->entity.baseline.renderamt * (1.0f + life * pTemp->fadeSpeed));
				if (pTemp->entity.curstate.renderamt <= 0)
					active = 0;
			}
			else 
				active = 0;
		}
		if (!active)		// Kill it
		{
			pTemp->next = *ppTempEntFree;
			*ppTempEntFree = pTemp;
			if (!pprev)// Deleting at head of list
				*ppTempEntActive = pnext;
			else
				pprev->next = pnext;
		}
		else
		{
			pprev = pTemp;
			pTemp->entity.prevstate.origin = pTemp->entity.origin;
			if (FBitSet(pTemp->flags, FTENT_SPARKSHOWER))
			{
				// Adjust speed if it's time
				// Scale is next think time
				if ( client_time > pTemp->entity.baseline.scale )
				{
					// Show Sparks
					gEngfuncs.pEfxAPI->R_SparkEffect(pTemp->entity.origin, 8, -200, 200);
					// Reduce life
					pTemp->entity.baseline.framerate -= 0.1;
					if (pTemp->entity.baseline.framerate <= 0.0)
					{
						pTemp->die = (float)client_time;
					}
					else
					{
						// So it will die no matter what
						pTemp->die = (float)(client_time + 0.5);
						// Next think
						pTemp->entity.baseline.scale = (float)(client_time + 0.1);
					}
				}
			}
			else if (FBitSet(pTemp->flags, FTENT_PLYRATTACHMENT))
			{
				cl_entity_t *pClient = GetUpdatingEntity(pTemp->clientIndex);// XDM3035c: TESTME gEngfuncs.GetEntityByIndex(pTemp->clientIndex);
				if (pClient)
				{
					/*if (pTemp->entity.curstate.aiment)// XDM: UNDONE
						VectorAdd(pClient->attachment[pTemp->entity.curstate.aiment], pTemp->tentOffset, pTemp->entity.origin);
					else*/
						VectorAdd(pClient->origin, pTemp->tentOffset, pTemp->entity.origin);
				}
				else// XDM: remove TENT if entity not found
					pTemp->die = (float)client_time;
			}
			else if (FBitSet(pTemp->flags, FTENT_SINEWAVE))
			{
				// pTemp->xyz += pTemp->TE_VELOCITY * frametime;
				// pTemp->entity.origin = pTemp->xyz + sin(pTemp->TE_VELOCITY + client_time*pTemp->entity.prevstate.frame) * (10*pTemp->entity.curstate.framerate);
				pTemp->x += (float)(pTemp->TE_VELOCITY[0] * frametime);
				pTemp->y += (float)(pTemp->TE_VELOCITY[1] * frametime);
				//pTemp->z += (float)(pTemp->TE_VELOCITY[2] * frametime);
				pTemp->entity.origin[0] = (float)(pTemp->x + sin(pTemp->TE_VELOCITY[2] + client_time * pTemp->entity.prevstate.frame) * (10*pTemp->entity.curstate.framerate));
				pTemp->entity.origin[1] = (float)(pTemp->y + sin(pTemp->TE_VELOCITY[2] + fastFreq + 0.7) * (8*pTemp->entity.curstate.framerate));
				//pTemp->entity.origin[2] = pTemp->z;
				pTemp->entity.origin[2] += (float)(pTemp->TE_VELOCITY[2] * frametime);
			}
			else if (FBitSet(pTemp->flags, FTENT_SPIRAL))
			{
				float s, c;
				SinCos(pTemp->TE_VELOCITY[2] + fastFreq, &s, &c);// XDM
				pTemp->entity.origin[0] += (float)(pTemp->TE_VELOCITY[0] * frametime + 8 * sin(client_time * 20 + (int)pTemp));
				pTemp->entity.origin[1] += (float)(pTemp->TE_VELOCITY[1] * frametime + 4 * sin(client_time * 30 + (int)pTemp));
				pTemp->entity.origin[2] += (float)(pTemp->TE_VELOCITY[2] * frametime);
			}
			else
			{
				pTemp->entity.origin[0] += (float)(pTemp->TE_VELOCITY[0] * frametime);
				pTemp->entity.origin[1] += (float)(pTemp->TE_VELOCITY[1] * frametime);
				pTemp->entity.origin[2] += (float)(pTemp->TE_VELOCITY[2] * frametime);
			}

			if (FBitSet(pTemp->flags, FTENT_SPRANIMATE))
			{
				pTemp->entity.curstate.frame += (float)(frametime * pTemp->entity.curstate.framerate);
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);
					if (!FBitSet(pTemp->flags, FTENT_SPRANIMATELOOP))
					{
						// this animating sprite isn't set to loop, so destroy it.
						pTemp->die = (float)client_time;
						pTemp = pnext;
						continue;
					}
				}
			}
			else if (FBitSet(pTemp->flags, FTENT_SPRCYCLE))
			{
				pTemp->entity.curstate.frame += (float)(frametime * 10.0);
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);
			}

			if (FBitSet(pTemp->flags, FTENT_ROTATE))
			{
				pTemp->entity.angles[0] += (float)(pTemp->entity.baseline.angles[0] * frametime);
				pTemp->entity.angles[1] += (float)(pTemp->entity.baseline.angles[1] * frametime);
				pTemp->entity.angles[2] += (float)(pTemp->entity.baseline.angles[2] * frametime);
				VectorCopy(pTemp->entity.angles, pTemp->entity.latched.prevangles);
			}

			if (FBitSet(pTemp->flags, FTENT_COLLIDEALL | FTENT_COLLIDEWORLD))
			{
				vec3_t	traceNormal;
				float	traceFraction = 1;

				if (FBitSet(pTemp->flags, FTENT_COLLIDEALL))
				{
					pmtrace_t pmtrace;
					physent_t *pe;
					gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX, -1, &pmtrace);
					if (pmtrace.fraction != 1)
					{
						pe = gEngfuncs.pEventAPI->EV_GetPhysent(pmtrace.ent);
						if (!pmtrace.ent || (pe->info != pTemp->clientIndex))
						{
							traceFraction = pmtrace.fraction;
							VectorCopy(pmtrace.plane.normal, traceNormal);

							if (pTemp->hitcallback)
								(*pTemp->hitcallback)(pTemp, &pmtrace);
						}
					}
				}
				else if (FBitSet(pTemp->flags, FTENT_COLLIDEWORLD))
				{
					pmtrace_t pmtrace;
					gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &pmtrace);

					if (pmtrace.fraction != 1)
					{
						traceFraction = pmtrace.fraction;
						VectorCopy(pmtrace.plane.normal, traceNormal);

						if (FBitSet(pTemp->flags, FTENT_SPARKSHOWER))
						{
							// Chop spark speeds a bit more
							VectorScale(pTemp->TE_VELOCITY, 0.6, pTemp->TE_VELOCITY);

							if (Length(pTemp->TE_VELOCITY) < 10)
								pTemp->entity.baseline.framerate = 0.0;								
						}

						if (pTemp->hitcallback)
							(*pTemp->hitcallback)(pTemp, &pmtrace);
					}
				}

				if (traceFraction != 1.0f)	// Decent collision now, and damping works
				{
					float proj, damp;
					// Place at contact point
					VectorMA(pTemp->entity.prevstate.origin, (float)(traceFraction*frametime), pTemp->TE_VELOCITY, pTemp->entity.origin);
					// Damp velocity
					damp = pTemp->bounceFactor;
					if (FBitSet(pTemp->flags, FTENT_GRAVITY|FTENT_SLOWGRAVITY))
					{
						damp *= 0.5f;
						if (traceNormal[2] > 0.9f)// Hit floor?
						{
							if (pTemp->TE_VELOCITY[2] <= 0 && pTemp->TE_VELOCITY[2] >= gravity*3)
							{
								damp = 0;		// Stop
								//pTemp->flags &= ~(FTENT_ROTATE|FTENT_GRAVITY|FTENT_SLOWGRAVITY|FTENT_COLLIDEWORLD|FTENT_SMOKETRAIL);
								ClearBits(pTemp->flags, FTENT_ROTATE|FTENT_GRAVITY|FTENT_SLOWGRAVITY|FTENT_COLLIDEWORLD|FTENT_SMOKETRAIL);
								pTemp->entity.angles[PITCH] = 0.0f;
								pTemp->entity.angles[ROLL] = 0.0f;
							}
						}
					}

					if (pTemp->hitSound)
						CL_TempEntPlaySound(pTemp, damp);// XDM: this one is a lot better

					if (FBitSet(pTemp->flags, FTENT_COLLIDEKILL))
					{
						// die on impact
						ClearBits(pTemp->flags, FTENT_FADEOUT);
						pTemp->die = (float)client_time;			
					}
					else// Reflect velocity
					{
						if (damp != 0.0f)
						{
							proj = DotProduct(pTemp->TE_VELOCITY, traceNormal);
							pTemp->TE_VELOCITY.MultiplyAdd(-proj*2.0f, traceNormal);
							// Reflect rotation (fake)
							pTemp->entity.angles[YAW] = -pTemp->entity.angles[YAW];
						}
						if (damp != 1.0f)
						{
							pTemp->TE_VELOCITY *= damp;
							pTemp->entity.angles *= 0.9f;
						}
					}
				}
				/*XDM: sometimes it affects gibs in the air :(
				else
				{
					int contents = gEngfuncs.PM_PointContents(pTemp->entity.origin, NULL);
					if (contents <= CONTENTS_WATER)
					{
						//pev->velocity = pev->velocity * 0.5;
						//pev->avelocity = pev->avelocity * 0.9;
						// what the shit is this?
						VectorScale(pTemp->TE_VELOCITY, 0.95, pTemp->TE_VELOCITY);// this is VELOCITY!
						VectorScale(pTemp->entity.angles, 0.99, pTemp->entity.angles);
						gravity *= 0.01f;
						if (pTemp->hitSound == BOUNCE_WOOD)
							pTemp->entity.origin[2] += frametime*16;
						else if (pTemp->hitSound == BOUNCE_FLESH)
							pTemp->entity.origin[2] += frametime*8;
							//pev->velocity.z += 8.0;

					}
				}*/
			}// (FBitSet(pTemp->flags, FTENT_COLLIDEALL | FTENT_COLLIDEWORLD))

			int contents = gEngfuncs.PM_PointContents(pTemp->entity.origin, NULL);
			pTemp->entity.curstate.iuser3 = contents;// XDM3038c

			if (FBitSet(pTemp->flags, FTENT_FLICKER))// && gTempEntFrame == pTemp->entity.curstate.effects )
			{
				dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				VectorCopy(pTemp->entity.origin, dl->origin);
				dl->radius = (float)pTemp->entity.baseline.renderamt;// XDM: 60
				dl->color.r = pTemp->entity.baseline.rendercolor.r;// XDM: 255
				dl->color.g = pTemp->entity.baseline.rendercolor.g;// XDM: 120
				dl->color.b = pTemp->entity.baseline.rendercolor.b;// XDM: 0
				dl->die = (float)(client_time + 0.01);
			}

			if (FBitSet(pTemp->flags, FTENT_SMOKETRAIL))
			{
				if (contents >= CONTENTS_EMPTY)
				{
					gEngfuncs.pEfxAPI->R_RocketTrail(pTemp->entity.prevstate.origin, pTemp->entity.origin, pTemp->entity.curstate.usehull);
				}
				else if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME)
				{
					if (g_pCvarParticles->value > 0.0f && fAccumulatedPeriod >= TE_FX_UPDATE_PERIOD)// once per period
						FX_BubblesPoint(pTemp->entity.origin, VECTOR_CONE_15DEGREES, g_pSpriteBubble, 4, 25);
				}
			}

			if (FBitSet(pTemp->flags, FTENT_GRAVITY))// XDM3037: apply some floating force
			{
				if ((contents < CONTENTS_SOLID) && (contents > CONTENTS_SKY) && (pTemp->hitSound == BOUNCE_FLESH || pTemp->hitSound == BOUNCE_WOOD))
					pTemp->TE_VELOCITY[2] -= gravity*0.4f;
				else
					pTemp->TE_VELOCITY[2] += gravity;
			}
			else if (FBitSet(pTemp->flags, FTENT_SLOWGRAVITY))
			{
				if ((contents < CONTENTS_SOLID) && (contents > CONTENTS_SKY) && (pTemp->hitSound == BOUNCE_FLESH || pTemp->hitSound == BOUNCE_WOOD))
					pTemp->TE_VELOCITY[2] -= gravity*0.2f;
				else
					pTemp->TE_VELOCITY[2] += gravity*0.5f;
			}

			if (FBitSet(pTemp->flags, FTENT_CLIENTCUSTOM))
			{
				if (pTemp->callback)
					(*pTemp->callback)(pTemp, (float)frametime, (float)client_time);
			}

			// Cull to PVS (not frustum cull, just PVS)
			if (pTemp->entity.model && (FBitSet(pTemp->entity.baseline.eflags, EFLAG_DRAW_ALWAYS) || !FBitSet(pTemp->flags, FTENT_NOMODEL)))
			{
				// TODO: check bounding box
				if (UTIL_PointIsVisible(pTemp->entity.origin, false) || ((pTemp->entity.curstate.entityType == ENTITY_BEAM) && UTIL_PointIsVisible(pTemp->entity.angles, false)))// XDM3035c: BUGBUG: ents disappear when player enters and exits "world" water // Beams!?? Never seen one here.
				{
					if (!Callback_AddVisibleEntity(&pTemp->entity))
					{
						/* XDM3038a: bad idea: makes gibs disappear	if (!(pTemp->flags & FTENT_PERSIST)) 
						{
							pTemp->die = (float)client_time;			// If we can't draw it this frame, just dump it.
							pTemp->flags &= ~FTENT_FADEOUT;	// Don't fade out, just die
						}
						else*/
						{
							// XDM3035: finally!
							//if ((!(pTemp->flags & FTENT_NOMODEL) && pTemp->entity.model) || (pTemp->entity.baseline.eflags & EFLAG_DRAW_ALWAYS))// XDM3038c: fixed
							//{
								// prevent drawing static entities behind walls UNDONE: check REAL FL_DRAW_ALWAYS for skybox tempents
								pmtrace_t pmtrace;
								gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
								gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.origin, g_vecViewOrigin, PM_STUDIO_IGNORE|PM_GLASS_IGNORE|PM_WORLD_ONLY, -1, &pmtrace);
								// since we can't check BSP leafs and nodes (all NULL), we use this simple method
								if (pmtrace.fraction >= 1.0f || (pmtrace.startsolid && pmtrace.fraction == 0.0f))
									gEngfuncs.CL_CreateVisibleEntity(ET_TEMPENTITY, &pTemp->entity);
							//}
						}
					}
				}
			}
		}
		pTemp = pnext;
	}

finish:
	// Restore state info
	gEngfuncs.pEventAPI->EV_PopPMStates();
}


//-----------------------------------------------------------------------------
// Purpose: If you specify negative numbers for beam start and end point entities, then
// the engine will call back into this function requesting a pointer to a cl_entity_t 
// object that describes the entity to attach the beam onto.
// Note   : Indices must start at 1, not zero.
// Input  : index - already abs()'ed
// Output : cl_entity_t
//-----------------------------------------------------------------------------
cl_entity_t CL_DLLEXPORT *HUD_GetUserEntity(int index)
{
	DBG_CL_PRINT("HUD_GetUserEntity(%d)\n", index);
//	RecClGetUserEntity(index);

#if defined(BEAM_TEST)
	// None by default, you would return a valic pointer if you create a client side beam and attach it to a client side entity.
	if (index > 0 && index <= 1)
		return &beams[index];
	else
		return NULL;
#endif
	return NULL;

#ifdef NOXASH
	// HACK: this crashes Xash3D too often
	if (gHUD.m_pLocalPlayer && index == gHUD.m_pLocalPlayer->index)// XDM3038a
	{
		if (CL_IsThirdPerson())
			return gHUD.m_pLocalPlayer;
		else
			return gEngfuncs.GetViewModel();
	}
	else
		return gEngfuncs.GetEntityByIndex(index);
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Tempentity with hitSound hits a surface
// Input  : *pTemp - 
//			damp - Damp velocity
//-----------------------------------------------------------------------------
void CL_TempEntPlaySound(TEMPENTITY *pTemp, float damp)
{
	if (pTemp == NULL || damp <= 0.0f)
		return;

	float fattn = ATTN_STATIC;
	const char *sample = NULL;
	bool isshell = false;

	switch (pTemp->hitSound)
	{
	case BOUNCE_GLASS:
		fattn = ATTN_NORM;
		sample = gSoundsGlass[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)];
		break;
	case BOUNCE_METAL:
		fattn = ATTN_NORM;
		sample = gSoundsMetal[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)];
		break;
	case BOUNCE_FLESH:
		fattn = ATTN_IDLE;
		sample = gSoundsFlesh[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)];
		break;
	case BOUNCE_WOOD:
		sample = gSoundsWood[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)];
		break;
	case BOUNCE_SHRAP:
		fattn = ATTN_NORM;
		sample = gSoundsRicochet[RANDOM_LONG(0,4)];//ARRAYSIZE(gSoundsRicochet)-1)];
		break;
	case BOUNCE_SHOTSHELL:
		sample = gSoundsShellShotgun[RANDOM_LONG(0,2)];//ARRAYSIZE(gSoundsShellShotgun)-1)];
		isshell = true;
		break;
	case BOUNCE_SHELL:
		sample = gSoundsShell9mm[RANDOM_LONG(0,2)];//ARRAYSIZE(gSoundsShell9mm)-1)];
		isshell = true;
		break;
	case BOUNCE_CONCRETE:
		sample = gSoundsConcrete[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)];
		break;
	default:
		{
		return;
		break;
		}
	}

	float zvel = fabsf(pTemp->TE_VELOCITY[2]);
	if (isshell)
	{	
		if (zvel < 200 && RANDOM_LONG(0,3))
			return;
	}
	else
	{
		if (RANDOM_LONG(0,5)) 
			return;
	}

	float fvol = 1.0f;
	if (isshell)
		fvol *= min((float)VOL_NORM, zvel / 350.0f); 
	else
		fvol *= min((float)VOL_NORM, zvel / 450.0f); 

	int	pitch;
	if (!isshell && RANDOM_LONG(0,3) == 0)
		pitch = RANDOM_LONG(95, 105);
	else
		pitch = PITCH_NORM;

	if (pTemp->entity.curstate.iuser3 < CONTENTS_SOLID && pTemp->entity.curstate.iuser3 > CONTENTS_SKY)// XDM3038c
	{
		fvol *= 0.75f;
		//pitch -= 8; like in old games :) sounds horrible
		fattn *= 1.25f;
	}
	//conprintf(1, "CL_TempEntPlaySound() sample %s hitSound %d\n", sample, pTemp->hitSound);
	gEngfuncs.pEventAPI->EV_PlaySound(pTemp->entity.index, pTemp->entity.origin, CHAN_STATIC, sample, fvol, fattn, 0, pitch);
}

// XDM3037a: sprites that are not precached on server
model_t *g_pSpriteAcidDrip				= NULL;
model_t *g_pSpriteAcidPuff1				= NULL;
model_t *g_pSpriteAcidPuff2				= NULL;
model_t *g_pSpriteAcidPuff3				= NULL;
model_t *g_pSpriteAcidSplash			= NULL;
model_t *g_pSpriteAnimGlow01			= NULL;// XDM3038c
model_t *g_pSpriteBloodDrop				= NULL;
model_t *g_pSpriteBloodSpray			= NULL;
model_t *g_pSpriteBubble				= NULL;
model_t *g_pSpriteDarkPortal			= NULL;// XDM3038b
model_t *g_pSpriteDot					= NULL;// XDM3038c: should be already loaded by the engine
model_t *g_pSpriteFExplo				= NULL;
model_t *g_pSpriteFlameFire				= NULL;// XDM3038b
model_t *g_pSpriteFlameFire2			= NULL;// XDM3038c
model_t *g_pSpriteGExplo				= NULL;
model_t *g_pSpriteHornetExp				= NULL;// XDM3038c
model_t *g_pSpriteIceBall1				= NULL;// XDM3038b
model_t *g_pSpriteIceBall2				= NULL;// XDM3038b
model_t *g_pSpritePSparks				= NULL;
model_t *g_pSpriteLaserBeam				= NULL;// XDM3038
//model_t *g_pSpriteLightPFlare			= NULL;// XDM3038
model_t *g_pSpriteLightPHit				= NULL;// XDM3038
model_t *g_pSpriteLightPRing			= NULL;// XDM3038
model_t *g_pSpriteMuzzleflash0			= NULL;
model_t *g_pSpriteMuzzleflash1			= NULL;
model_t *g_pSpriteMuzzleflash2			= NULL;
model_t *g_pSpriteMuzzleflash3			= NULL;
model_t *g_pSpriteMuzzleflash4			= NULL;
model_t *g_pSpriteMuzzleflash5			= NULL;
model_t *g_pSpriteMuzzleflash6			= NULL;
model_t *g_pSpriteMuzzleflash7			= NULL;
model_t *g_pSpriteMuzzleflash8			= NULL;
model_t *g_pSpriteMuzzleflash9			= NULL;
model_t *g_pSpriteNucBlow				= NULL;
model_t *g_pSpritePExp1					= NULL;
model_t *g_pSpritePExp2					= NULL;
//model_t *g_pSpritePGlow01s				= NULL;// XDM3038c
model_t *g_pSpritePTracer				= NULL;
model_t *g_pSpritePBallHit				= NULL;// XDM3038
model_t *g_pSpriteRicho1				= NULL;
model_t *g_pSpriteSmkBall				= NULL;
model_t *g_pSpriteSpawnBeam				= NULL;
model_t *g_pSpriteTeleFlash				= NULL;
//model_t *g_pSpriteUExplo				= NULL;
model_t *g_pSpriteVoiceIcon				= NULL;// XDM3038a
model_t *g_pSpriteWallPuff1				= NULL;
model_t *g_pSpriteWallPuff2				= NULL;
//model_t *g_pSpriteWallPuff3			= NULL;
model_t *g_pSpriteWarpGlow1				= NULL;
model_t *g_pSpriteXbowExp				= NULL;// XDM3038
model_t *g_pSpriteXbowRic				= NULL;// XDM3038
model_t *g_pSpriteZeroFire				= NULL;
//model_t *g_pSpriteZeroFire2			= NULL;
model_t *g_pSpriteZeroFlare				= NULL;// XDM3038c
model_t *g_pSpriteZeroGlow				= NULL;// XDM3038c
model_t *g_pSpriteZeroSteam				= NULL;
model_t *g_pSpriteZeroWXplode			= NULL;// XDM3038a

//-----------------------------------------------------------------------------
// Purpose: Synchronize server/client resources by indexes and also load some client-only stuff
// Warning: There are limits to this! Don't overuse resources! Remember: mappers love to overload the system too!
//-----------------------------------------------------------------------------
void CL_Precache(void)
{
	conprintf(1, "CL_Precache()\n");
	PrecacheSharedResources();
	g_pSpriteAcidDrip				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/acid_drip.spr"));
	g_pSpriteAcidPuff1				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/acid_puff1.spr"));
	g_pSpriteAcidPuff2				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/acid_puff2.spr"));
	g_pSpriteAcidPuff3				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/acid_puff3.spr"));
	g_pSpriteAcidSplash				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/acid_splash.spr"));
	g_pSpriteAnimGlow01				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/animglow01.spr"));// g_iModelIndexAnimglow01
	g_pSpriteBloodDrop				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/blood.spr"));
	g_pSpriteBloodSpray				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/bloodspray.spr"));
	g_pSpriteBubble					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/bubble.spr"));
	g_pSpriteDarkPortal				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/darkportal.spr"));// g_iModelIndexGravFX
	g_pSpriteDot					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/dot.spr"));
	g_pSpriteFExplo					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/fexplo.spr"));// g_iModelIndexNucExp1
	g_pSpriteFlameFire				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/flamefire.spr"));// g_iModelIndexFlameFire
	g_pSpriteFlameFire2				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/flamefire2.spr"));// g_iModelIndexFlameFire2
	g_pSpriteGExplo					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/gexplo.spr"));// g_iModelIndexNucExp2
	g_pSpriteHornetExp				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/hornetexp.spr"));
	g_pSpriteIceBall1				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/iceball1.spr"));// g_iModelIndexColdball1
	g_pSpriteIceBall2				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/iceball2.spr"));// g_iModelIndexColdball2
	g_pPlayerIDTexture				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/iplayerid.spr"));
	g_pSpriteWarpGlow1				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/warp_glow1.spr"));
	g_pSpriteLaserBeam				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/laserbeam.spr"));// XDM3038
	//g_pSpriteLightPFlare			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/lightp_flare.spr"));// XDM3038
	g_pSpriteLightPHit				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/lightp_hit.spr"));// XDM3038
	g_pSpriteLightPRing				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/lightp_ring.spr"));// XDM3038
	g_pSpriteMuzzleflash0			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash0.spr"));// g_iModelIndexMuzzleFlash0
	g_pSpriteMuzzleflash1			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash1.spr"));
	g_pSpriteMuzzleflash2			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash2.spr"));
	g_pSpriteMuzzleflash3			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash3.spr"));
	g_pSpriteMuzzleflash4			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash4.spr"));
	g_pSpriteMuzzleflash5			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash5.spr"));
	g_pSpriteMuzzleflash6			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash6.spr"));
	g_pSpriteMuzzleflash7			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash7.spr"));
	g_pSpriteMuzzleflash8			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash8.spr"));
	g_pSpriteMuzzleflash9			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/muzzleflash9.spr"));
	g_pSpriteNucBlow				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/nuc_blow.spr"));// g_iModelIndexNucExplode
	g_pSpritePExp1					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/p_exp1.spr"));// g_iModelIndexPExp1
	g_pSpritePExp2					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/p_exp2.spr"));// g_iModelIndexPExp2
	//g_pSpritePGlow01s				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/p_glow01s.spr"));
	g_pSpritePSparks				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/p_sparks.spr"));
	g_pSpritePTracer				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/p_tracer.spr"));// g_iModelIndexPTracer
	g_pSpritePBallHit				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/pball_hit.spr"));// XDM3038
	g_pSpriteRicho1					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/richo1.spr"));
	g_pSpriteSmkBall				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/smkball.spr"));// g_iModelIndexSmkball
	g_pSpriteSpawnBeam				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/spawnbeam.spr"));
	g_pSpriteTeleFlash				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/teleflash.spr"));// g_iModelIndexTeleFlash
	//g_pSpriteUExplo					= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/uexplo.spr"));// g_iModelIndexNucExp3
	g_pSpriteVoiceIcon				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/voiceicon.spr"));// XDM3038a
	g_pSpriteWallPuff1				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/wallpuff1.spr"));// g_iModelIndexWallPuff1
	g_pSpriteWallPuff2				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/wallpuff2.spr"));
	//g_pSpriteWallPuff3				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/wallpuff3.spr"));
	g_pSpriteXbowExp				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/xbow_exp.spr"));// XDM3038
	g_pSpriteXbowRic				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/xbow_ric.spr"));// XDM3038
	g_pSpriteZeroFire				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/zerofire.spr"));// g_iModelIndexZeroFire
	//g_pSpriteZeroFire2				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/zerofire2.spr"));// XDM3038a
	g_pSpriteZeroFlare				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/zeroflare.spr"));// g_iModelIndexZeroFlare
	g_pSpriteZeroGlow				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/zeroglow.spr"));// g_iModelIndexZeroGlow
	g_pSpriteZeroSteam				= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/zerosteam.spr"));// g_iModelIndexZeroSteam
	g_pSpriteZeroWXplode			= (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/zerowxplode.spr"));//g_iModelIndexWExplosion
}
