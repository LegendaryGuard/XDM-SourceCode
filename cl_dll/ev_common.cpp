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
// shared event functions
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "eventscripts.h"
#include "event_api.h"
#include "pm_materials.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "decals.h"
#include "in_defs.h"

#define IS_FIRSTPERSON_SPEC (g_iUser1 == OBS_IN_EYE || (g_iUser1 && (gHUD.m_Spectator.m_iInsetMode == INSET_IN_EYE)) )


/*int EV_IndexFromTrace(pmtrace_t *pTrace)
{
	if (pTrace && pTrace->ent >= 0 && pTrace->ent < pmove->numphysent)// ent fails surprisingly often!
		return pmove->physents[pTrace->ent].info;

	return 0;
}*/

//-----------------------------------------------------------------------------
// Purpose: Is the entity a player
// Input  : idx - entindex
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool EV_IsPlayer(int idx)
{
	return IsValidPlayerIndex(idx);
}

//-----------------------------------------------------------------------------
// Purpose: Is the entity == the local player
// Input  : idx - entindex
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool EV_IsLocal(int idx)
{
	// check if we are in some way in first person spec mode
	if (IS_FIRSTPERSON_SPEC)
		return (g_iUser2 == idx);
	else
		return gEngfuncs.pEventAPI->EV_IsLocal(idx - 1) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: IMPORTANT! Fixes SQB in arguments! CALL THIS BEFORE ANYTHING ELSE IN AN EVENT!
// Input  : args - 
//-----------------------------------------------------------------------------
void EV_START(struct event_args_s *args)// XDM3038c
{
	if (EV_IsPlayer(args->entindex))// everyone is affected if (EV_IsLocal(args->entindex))
	{
#if defined (CORRECT_PITCH)
		args->angles[PITCH] /= -PITCH_CORRECTION_MULTIPLIER;
#endif
	}
	else
		args->angles[PITCH] *= -1.0f;

	DBG_ANGLES_DRAW(16, args->origin, args->angles, args->entindex, "EV_START() args");
}

//-----------------------------------------------------------------------------
// Purpose: Get weapon viewmodel or player if 3rd person
// Input  : entindex - 
// Output : cl_entity_t *
//-----------------------------------------------------------------------------
cl_entity_t *EV_GetViewEntity(int entindex)
{
	if (EV_IsLocal(entindex) && !g_ThirdPersonView)
		return gEngfuncs.GetViewModel();
	else
		return gEngfuncs.GetEntityByIndex(entindex);
}

//-----------------------------------------------------------------------------
// Purpose: Get position of a gun for effects, etc.
// Input  : args - 
//			*pos - output
//			*origin - output
//-----------------------------------------------------------------------------
void EV_GetGunPosition(event_args_t *args, float *pos, const float *origin)
{
	Vector view_ofs(0,0,VEC_HANDS_OFFSET);
	if (EV_IsPlayer(args->entindex))
	{
		if (EV_IsLocal(args->entindex) && !IS_FIRSTPERSON_SPEC)// in spec mode use entity viewheight, not own
		{
			cl_entity_t *pViewEnt = EV_GetViewEntity(args->entindex);
			if (pViewEnt)
			{
				view_ofs = GetCEntAttachment(pViewEnt, 0);
				VectorCopy(view_ofs, pos);
				return;
			}
			else// if (g_ThirdPersonView)// Grab predicted result for local player
				gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);// local_state_s::clientdata_t::view_ofs
		}
		else if (args->ducking > 0)
			view_ofs.z = DUCK_VIEW;
	}
	//UTIL_DebugPoint(origin, 1.0, 0,255,0);
	VectorAdd(origin, view_ofs, pos);
	//UTIL_DebugPoint(pos, 1.0, 255,255,0);
}

//-----------------------------------------------------------------------------
// Purpose: Determine where to eject shells from (Legacy HL BS)
// Input  : args - event_args_t
//			origin - input
//			velocity - input
//			ShellVelocity - output
//			ShellOrigin - output
//			forward - input
//			right - input
//			up - input
//			forwardScale - input
//			upScale - input
//			rightScale - input
//-----------------------------------------------------------------------------
void EV_GetDefaultShellInfo(event_args_t *args, const float *origin, const float *velocity, float *ShellVelocity, float *ShellOrigin, const float *forward, const float *right, const float *up, float forwardScale, float upScale, float rightScale)
{
	float fR, fU;
	EV_GetGunPosition(args, ShellOrigin, origin);
	//UTIL_DebugBeam(origin, ShellOrigin, 1.0, 255,0,127);
	fR = RANDOM_FLOAT(50, 70);
	fU = RANDOM_FLOAT(80, 120);
	ShellVelocity[0] = velocity[0] + right[0] * fR + up[0] * fU + forward[0] * 25.0f;
	ShellVelocity[1] = velocity[1] + right[1] * fR + up[1] * fU + forward[1] * 25.0f;
	ShellVelocity[2] = velocity[2] + right[2] * fR + up[2] * fU + forward[2] * 25.0f;
	ShellOrigin[0] += up[0] * upScale + forward[0] * forwardScale + right[0] * rightScale;
	ShellOrigin[1] += up[1] * upScale + forward[1] * forwardScale + right[1] * rightScale;
	ShellOrigin[2] += up[2] * upScale + forward[2] * forwardScale + right[2] * rightScale;
}

//-----------------------------------------------------------------------------
// Purpose: Eject bullet shell casings (Legacy, HL, tempentity)
// Input  : origin - 
//			velocity - 
//			rotation - YAW
//			model - 
//			body - 
//			soundtype - 
//-----------------------------------------------------------------------------
void EV_EjectBrass(float *origin, float *velocity, const float &rotation, const int &model, const int &body, const int &soundtype)
{
	Vector angles(0.0f, rotation, 0.0f);
	TEMPENTITY *pShell = gEngfuncs.pEfxAPI->R_TempModel(origin, velocity, angles, 2.5f, model, soundtype);
	if (pShell)
	{
		// TEST	pShell->flags |= FTENT_SMOKETRAIL;
		//pShell->entity.baseline.usehull = pShell->entity.curstate.usehull = test1->value;
		pShell->entity.baseline.body = pShell->entity.curstate.body = body;// a must!
	}
	// TODO: CRSModel *pShell = new CRSModel(origin, angles, velocity, 0, model, body, skin, -1, kRenderNormal, kRenderFxNone, 255,255,255, 1.0,0.0, scale, 0.0, 1.0, 5.0f)
}

//-----------------------------------------------------------------------------
// Purpose: Flag weapon/view model for muzzle flash (HL, legacy)
// Input  : entindex - 
//-----------------------------------------------------------------------------
void EV_MuzzleFlash(int entindex)
{
	cl_entity_t *ent = EV_GetViewEntity(entindex);
	if (ent)
		ent->curstate.effects |= EF_MUZZLEFLASH;
}

//-----------------------------------------------------------------------------
// Purpose: Print all arguments (for debugging)
//-----------------------------------------------------------------------------
void EV_PrintParams(const char *name, event_args_t *args)
{
	conprintf(0, "EV %s: ei %d, f1 %f, f2 %f, i1 %d, i2 %d, b1 %d, b2 %d\n", name, args->entindex, args->fparam1, args->fparam2, args->iparam1, args->iparam2, args->bparam1, args->bparam2);
}

//-----------------------------------------------------------------------------
// Purpose: Play a strike sound based on the texture that was hit by the attack traceline.
// Input  : chTextureType - 'C'
//			*origin - sound origin
//			breakable - type of sounds to use
// Output : float - volume to play weapon sound at
//-----------------------------------------------------------------------------
float PlayTextureSound(char chTextureType, float *origin, bool breakable)
{
	if (!chTextureType)
		return 0.0f;
	if (chTextureType == CHAR_TEX_NULL)// XDM3037
		return 0.0f;

	float fvol = VOL_NORM;
	float fvolbar = 0.0;
	float fattn = ATTN_NORM;
	const char **pSoundArray = NULL;
	int arraysize = 0;
	TextureTypeGetSounds(chTextureType, breakable, fvol, fvolbar, fattn, &pSoundArray, arraysize);

	if (arraysize > 0 && fvol > 0.0f)
		EMIT_SOUND(0, origin, CHAN_STATIC, pSoundArray[RANDOM_LONG(0,arraysize-1)], fvol, fattn, 0, RANDOM_LONG(96,104));

	return fvolbar;
}

//-----------------------------------------------------------------------------
// Purpose: Pick decal index by bullet type
// Input  : iBulletType - 
//			chTextureType - 
// Output : int g_Decals index, NOT an engine decal index!!
//-----------------------------------------------------------------------------
int EV_DamageDecal(int iBulletType, char chTextureType)
{
	if (chTextureType == CHAR_TEX_NULL || chTextureType == CHAR_TEX_SKY)// XDM3038c
		return -1;

	bool bigshot = false;
	if (iBulletType == BULLET_BREAK || iBulletType == BULLET_357 || iBulletType == BULLET_12MM || iBulletType == BULLET_338)
		bigshot = true;

	int decal = -1;// this is NOT an engine decal index! Default: no decal.
	switch (chTextureType)
	{
	default: break;// CHAR_TEX_NULL, CHAR_TEX_SKY
	case CHAR_TEX_CONCRETE:
	case CHAR_TEX_METAL:
	case CHAR_TEX_DIRT:
	case CHAR_TEX_VENT:
	case CHAR_TEX_GRATE:
	case CHAR_TEX_TILE:
		{
			if (bigshot)
				decal = RANDOM_LONG(DECAL_BIGSHOT1, DECAL_BIGSHOT5);
			else
				decal = RANDOM_LONG(DECAL_GUNSHOT1, DECAL_GUNSHOT5);
		}
		break;

	case CHAR_TEX_COMPUTER:
		{
			if (bigshot)
				decal = RANDOM_LONG(DECAL_LARGESHOT1, DECAL_LARGESHOT5);
			else
				decal = RANDOM_LONG(DECAL_BIGSHOT1, DECAL_BIGSHOT5);
		}
		break;

	case CHAR_TEX_GLASS:
		{
			if (bigshot)
				decal = RANDOM_LONG(DECAL_GLASSBREAK1, DECAL_GLASSBREAK3);
			else
				decal = DECAL_BPROOF1;
		}
		break;

	case CHAR_TEX_WOOD:
		{
			if (bigshot)
				decal = RANDOM_LONG(DECAL_WOODBREAK1, DECAL_WOODBREAK3);
			else
				decal = RANDOM_LONG(DECAL_BIGSHOT1, DECAL_BIGSHOT5);
		}
		break;
	}
	return decal;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iBulletType - 
//			chTextureType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool EV_ShouldDoSmoke(int iBulletType, char chTextureType)
{
	/* UNDONE
	for (int i=0; i<NUM_MATERIALS; ++i)
	{
		if (gMaterials[i].texture_id == chTextureType)
			return gMaterials[i].bulletsmoke;
	}
	*/
	/*bool bigshot = false;
	if (iBulletType == BULLET_BREAK || iBulletType == BULLET_357 || iBulletType == BULLET_12MM || iBulletType == BULLET_338)
		bigshot = true;*/
	switch (chTextureType)
	{
	default: break;
	case CHAR_TEX_CONCRETE:
	case CHAR_TEX_METAL:
	case CHAR_TEX_DIRT:
	case CHAR_TEX_VENT:
	case CHAR_TEX_GRATE:
	case CHAR_TEX_TILE:
	//case CHAR_TEX_WOOD:
	case CHAR_TEX_COMPUTER:
	//case CHAR_TEX_GLASS:
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: OBSOLETE
// Input  : *pTrace - 
//			decalindex - 
//-----------------------------------------------------------------------------
/*void EV_GunshotDecalTrace(pmtrace_t *pTrace, int decalindex)
{
	if (pTrace && decalindex > 0)// && CVAR_GET_FLOAT("r_decals") > 0)
	{
		int entindex = gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace);// XDM3035: compatibility fix
		if (entindex >= 0)// XDM3035c
		{
			cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex(entindex);
			if (pEnt)
			{
				if (pEnt->model->type == mod_brush)
				{
		//physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);
		// Only decal brush models such as the world etc.
		//if (pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
					gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalindex), entindex, 0, pTrace->endpos, 0);
				}
			}
		}
	}
}*/
