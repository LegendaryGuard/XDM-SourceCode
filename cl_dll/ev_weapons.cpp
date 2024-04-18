#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "eventscripts.h"
#include "event_api.h"
#include "r_efx.h"
#include "cl_fx.h"
#include "color.h"
#include "weapondef.h"
#include "shared_resources.h"
#include "decals.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeam.h"
#include "RSLight.h"
#include "RSSprite.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "PSFlameCone.h"
#include "PSSparks.h"

color24 g_DisplacerColors[] =
{
	{31,255,143},
	{31,143,255},
	{0,0,0},
	{0,0,0}
};

extern "C"
{
	void EV_FireALauncher(struct event_args_s *args);
	void EV_FireBeam(struct event_args_s *args);
	void EV_FireBeamRifle(struct event_args_s *args);
	void EV_FireBHG(struct event_args_s *args);
	void EV_FireChemGun(struct event_args_s *args);
	void EV_FireCrossbow(struct event_args_s *args);
	void EV_FireCrowbar(struct event_args_s *args);
	void EV_FireDisplacer(struct event_args_s *args);
	void EV_FireFlame(struct event_args_s *args);
	void EV_FireGauss(struct event_args_s *args);
	void EV_FireGLauncher(struct event_args_s *args);
	void EV_FireGlock(struct event_args_s *args);
	void EV_FireHornetGun(struct event_args_s *args);
	void EV_FireMP5(struct event_args_s *args);
	void EV_FireMP52(struct event_args_s *args);
	void EV_FirePlasmaGun(struct event_args_s *args);
	void EV_FirePython(struct event_args_s *args);
	void EV_FireRazorDisk(struct event_args_s *args);
	void EV_FireRpg(struct event_args_s *args);
	void EV_FireShotGunSingle(struct event_args_s *args);
	void EV_FireShotGunDouble(struct event_args_s *args);
	void EV_FireSnark(struct event_args_s *args);
	void EV_FireSniperRifle(struct event_args_s *args);
	void EV_FireTripmine(struct event_args_s *args);

	void EV_GrenMode(struct event_args_s *args);
	void EV_TripMode(struct event_args_s *args);
	void EV_TrainPitchAdjust(struct event_args_s *args);
	void EV_ZoomCrosshair(struct event_args_s *args);
	void EV_NuclearDevice(struct event_args_s *args);
}




//-----------------------------------------------------------------------------
// Weapons
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - ALAUNCHER_FIRE
// iparam2 - modelIndex
// bparam1 - m_iClip
// bparam2 - sound
//-----------------------------------------------------------------------------
void EV_FireALauncher(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);
	//Vector vecSrc;
	Vector forward;

	// light?!	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, 0);
		V_PunchAxis(PITCH, 4.0f);
	}

	if (args->bparam2 == 0)// RANDOM_LONG(0,1))
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/a_launch1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98-args->bparam1, 102));
	else
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/a_launch2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98-args->bparam1, 102));

	//EV_GetGunPosition(args, vecSrc, origin);

	if (g_pCvarEffects->value > 0.0f)
	{
		AngleVectors(args->angles, forward, NULL, NULL);
		Vector spot;
		VectorAdd(origin, forward, spot);
		gEngfuncs.pEfxAPI->R_Sprite_Trail(0, origin, spot, args->iparam2, args->bparam1*3, 0.0, 0.1, 20, 191, 100);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FireBeam(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 2.0f);
	}

	if (args->bparam2 == FIRE_PRI)
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/beamrifle_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));
	else
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/beamrifle_fire2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));

	if (g_pCvarEffects->value > 0.0f)
		EntityLight(args->origin, (args->bparam2 == FIRE_PRI)?12.0f:24.0f, 230,230,255, 0.1f, 40.0f);
	else
		EV_MuzzleFlash(idx);

	if (g_pRenderManager)
		g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 255,255,255, 1.0f,-1.0f,  0.01,0.02, 24.0f, 2.0f));

	/*EV_GetGunPosition(args, vecSrc, args->origin);

	pmtrace_t tr;
	Vector vecDest, angles, forward, right;
	gEngfuncs.GetViewAngles(angles);

	AngleVectors(angles, forward, right, NULL);
	VectorMA(vecSrc, 8192, forward, vecDest);

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(args->entindex-1);	
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr);
	gEngfuncs.pEventAPI->EV_PopPMStates();

	DecalTrace("{gaussshot1", &tr);*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - firemode
//-----------------------------------------------------------------------------
void EV_FireBeamRifle(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);
	//Vector vecSrc;
	Vector forward;

	if (EV_IsLocal(idx))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

	if (args->bparam2 == FIRE_SEC)// m_fireMode
	{
		if (g_pCvarEffects->value > 0.0f)
		{
			float life = args->fparam1;
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSLight(args->origin, 220,220,255, 16, NULL, 40.0, life, true), RENDERSYSTEM_FLAG_NOCLIP, /*view->index ?*/args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);// don't follow viewmodel!

	//ok, but doesn't follow				EntityLight(args->origin, 32, 220,220,255, life, 40);
		}
		else
			EV_MuzzleFlash(idx);

		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/beamrifle_charge.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - ball life
// fparam2 - 
// iparam1 - animation
// iparam2 - 
// bparam1 - body
// bparam2 - firemode, firestate bits
//-----------------------------------------------------------------------------
void EV_FireBHG(event_args_t *args)
{
	EV_START(args);
	int numattachments;
	cl_entity_t *pAttachmentModel;
	byte firemode, firestate;
	BitSplit2x4bit(args->bparam2, firemode, firestate);
	if (firemode > 1)// array bounds!
		firemode = 1;

	if (EV_IsLocal(args->entindex) && g_ThirdPersonView == 0)
	{
		pAttachmentModel = gEngfuncs.GetViewModel();
		numattachments = 3;
	}
	else
	{
		pAttachmentModel = gEngfuncs.GetEntityByIndex(args->entindex);
		numattachments = 2;// there are 3 attachments total on a player model, 0=start, 1,2=ends
	}

	if (EV_IsLocal(args->entindex))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

	if (firestate == FIRE_OFF)
	{
		STOP_SOUND(args->entindex, CHAN_WEAPON, "weapons/bhg_charge.wav");
		gEngfuncs.pEfxAPI->R_BeamKill(pAttachmentModel->index);
	}
	else if (firestate == FIRE_CHARGE)
	{
		//EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/bhg_charge.wav", VOL_NORM, ATTN_NORM, 0, args->iparam2);
		if (pAttachmentModel != NULL)
		{
			for (int i=0; i<numattachments; ++i)
				gEngfuncs.pEfxAPI->R_BeamEnts(pAttachmentModel->index | 0x1000, pAttachmentModel->index | (0x1000 * (1+i)), g_iModelIndexLaser, 1.0f/*life*/, 0.25, 0.4, 1.0, 0.5, 0, 20.0f, 1.0f,1.0f,1.0f);
				//gEngfuncs.pEfxAPI->R_BeamEnts(view->index | 0x1000, view->index | 0x2000, sprIndex, life, 0.5, 0.2, 1.0, 0.5, 0, args->iparam2, (float)r/255, g/255, b/255);
				//gEngfuncs.pEfxAPI->R_BeamEnts(view->index | 0x1000, view->index | 0x3000, sprIndex, life, 0.5, 0.2, 1.0, 0.5, 0, args->iparam2, r/255, g/255, b/255);
				//gEngfuncs.pEfxAPI->R_BeamEnts(view->index | 0x1000, view->index | 0x4000, sprIndex, life, 0.5, 0.2, 1.0, 0.5, 0, args->iparam2, r/255, g/255, b/255);
		}
		if (g_pCvarEffects->value > 0.0f)
		{
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSLight(args->origin, 223,223,255, 16, NULL, 40.0, args->fparam1/*life*/, true), RENDERSYSTEM_FLAG_NOCLIP, /*view->index ?*/args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);// don't follow viewmodel!
		}
		/*if (g_pCvarParticles->value > 0.0f && g_pRenderManager)
		{
			CPSSparks *pSparks = new CPSSparks((g_pCvarParticles->value > 1.0f)?192:64, args->origin,
					0.5f,0.2f,-0.5f, 32.0f, 2.0f, 255,255,255,1.0f,-0.75f, g_pSpriteIceBall2, kRenderTransAdd, 3.0f);// IEngineStudio.GetModelByIndex(g_iModelIndexColdball2)

			if (pSparks)
			{
				//pSparks->m_OnParticleUpdate = PS_MoveAlong_OnParticleUpdate;
				g_pRenderManager->AddSystem(pSparks, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
			}
		}*/
	}
	else if (firestate == FIRE_ON)
	{
		if (EV_IsLocal(args->entindex))
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);//pAttachmentModel->curstate.body);
			//V_PunchAxis(ROLL, 1.0f);
		}
		EV_MuzzleFlash(args->entindex);
		STOP_SOUND(args->entindex, CHAN_WEAPON, "weapons/bhg_charge.wav");
		EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/bhg_fire.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		if (g_pCvarParticles->value > 0.0f && g_pRenderManager)
		{
			Vector vForward;
			AngleVectors(args->angles, vForward, NULL, NULL);
			CParticleSystem *pSys = new CParticleSystem((g_pCvarParticles->value > 1.0f)?160:80, args->origin, vForward, g_pSpriteIceBall2, kRenderTransAdd, 3.0f);
			if (pSys)
			{
				pSys->m_iStartType = PSSTARTTYPE_POINT;
				pSys->m_iMovementType = PSMOVTYPE_DIRECTED;
				pSys->m_fColorDelta[3] = -1.0f;
				pSys->m_fScale = 0.1f;
				pSys->m_fScaleDelta = -1.0f;
				pSys->m_fParticleSpeedMin = 160;
				pSys->m_fParticleSpeedMax = 180;
				pSys->m_vSpread = VECTOR_CONE_45DEGREES;
				pSys->m_iFollowAttachment = 1;
				if (g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE) != RS_INDEX_INVALID)
					pSys->InitializeSystem();
			}
		}
		if (g_pCvarEffects->value > 0)
			DynamicLight(args->origin, 128.0f, 255,255,255, 0.5f, 100);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Crowbar attacks
// entindex - player
// origin - origin
// angles - angles
// fparam1 - fvolbar
// fparam2 - edamage
// iparam1 - iAnim
// iparam2 - tex
// bparam1 - pev->body
// bparam2 - ev_sound
//-----------------------------------------------------------------------------
void EV_FireCrowbar(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	float vol = args->fparam1;
	if (EV_IsLocal(idx))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

	if (args->bparam2 == 0)
	{
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", vol, ATTN_NORM, 0, RANDOM_LONG(95, 105));
	}
	else if (args->bparam2 == 1)
	{
		switch (RANDOM_LONG(0,2))
		{
		case 0: EMIT_SOUND(idx, args->origin, CHAN_ITEM, "weapons/cbar_hitbod1.wav", vol, ATTN_NORM, 0, RANDOM_LONG(98, 102)); break;
		case 1: EMIT_SOUND(idx, args->origin, CHAN_ITEM, "weapons/cbar_hitbod2.wav", vol, ATTN_NORM, 0, RANDOM_LONG(98, 102)); break;
		case 2: EMIT_SOUND(idx, args->origin, CHAN_ITEM, "weapons/cbar_hitbod3.wav", vol, ATTN_NORM, 0, RANDOM_LONG(98, 102)); break;
		}
	}
	else
	{
		char tex = args->iparam2;
		if (tex != 0)
		{
			if (vol < 1.0f)// vol 1.0 means full bar sound and NO texture sound
				vol = PlayTextureSound(tex, args->origin, (args->fparam2 > 0)?true:false);

			if (tex == CHAR_TEX_COMPUTER)
			{
				FX_StreakSplash(args->origin, g_vecZero, gTracerColors[5], 16, 120.0f, true, true, false);
				EMIT_SOUND(idx, args->origin, CHAN_STATIC, gSoundsSparks[RANDOM_LONG(0,NUM_SPARK_SOUNDS-1)], RANDOM_FLOAT(0.7, 1.0), ATTN_NORM, 0, RANDOM_LONG(95, 105));
			}
		}
		if (vol > 0.0f)
		{
			if (RANDOM_LONG(0,1) == 0)
				EMIT_SOUND(idx, args->origin, CHAN_ITEM, "weapons/cbar_hit1.wav", vol, ATTN_NORM, 0, RANDOM_LONG(98, 102));
			else
				EMIT_SOUND(idx, args->origin, CHAN_ITEM, "weapons/cbar_hit2.wav", vol, ATTN_NORM, 0, RANDOM_LONG(98, 102));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Grenade launcher attacks
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - animaiton
// iparam2 - 
// bparam1 - body
// bparam2 - firemode
//-----------------------------------------------------------------------------
void EV_FireGLauncher(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);
	Vector vecSrc;

	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 10.0f);
	}

	if (args->bparam2 == FIRE_PRI)// firemode
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/g_launch1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));
	else
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/g_launch2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));

	EV_GetGunPosition(args, vecSrc, origin);
	FX_Smoke(vecSrc, g_iModelIndexSmkball, RANDOM_FLOAT(1.0, 1.25), 20);

	Vector ShellOrigin, ShellVelocity;
	Vector up, right, forward;
	EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -14, 6);
	EV_EjectBrass(ShellOrigin, ShellVelocity, args->angles[YAW], args->iparam2, SHELL_GLAUNCHER, TE_BOUNCE_SHOTSHELL);
}

//-----------------------------------------------------------------------------
// Purpose: Glock primary attack
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - animaiton
// iparam2 - 
// bparam1 - body
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FireGlock(event_args_t *args)
{
	EV_START(args);
	Vector origin(args->origin);
	Vector angles(args->angles);
	Vector ShellOrigin, ShellVelocity;
	Vector up, right, forward;
	AngleVectors(angles, forward, right, up);

	EV_MuzzleFlash(args->entindex);
	if (EV_IsLocal(args->entindex))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 2.0f);
	}

	EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -14, 4);
	EMIT_SOUND(args->entindex, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, RANDOM_LONG(98, 104));
	FX_Smoke(ShellOrigin, g_iModelIndexSmkball, RANDOM_FLOAT(0.4, 0.6), 20);
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], args->iparam2, SHELL_BRASS/* no args->bparam2*/, TE_BOUNCE_SHELL);
}


//-----------------------------------------------------------------------------
// Purpose: 357 primary attack
// entindex - player
// origin - origin
// angles - angles
// fparam1 - punch x
// fparam2 - punch y
// iparam1 - animaiton
// iparam2 - 
// bparam1 - body
// bparam2 - sound selection
//-----------------------------------------------------------------------------
void EV_FirePython(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);
	Vector vecSrc;

	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, args->fparam1);
		V_PunchAxis(YAW, args->fparam2);
	}

	if (args->bparam2 == 0)// RANDOM_LONG(0,1))
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));
	else
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));

	EV_GetGunPosition(args, vecSrc, origin);
	FX_Smoke(vecSrc, g_iModelIndexSmkball, RANDOM_FLOAT(0.6, 0.8), 20);
}

//-----------------------------------------------------------------------------
// Purpose: MP5 primary attack
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - animaiton
// iparam2 - shell model
// bparam1 - body
// bparam2 - shell body
//-----------------------------------------------------------------------------
void EV_FireMP5(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);
	Vector angles(args->angles);
	Vector ShellOrigin, ShellVelocity;
	Vector up, right, forward;
	AngleVectors(angles, forward, right, up);

	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, RANDOM_FLOAT(0.1f, 2.0f));
	}

	EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -4, 4);
	FX_Smoke(ShellOrigin+Vector(RANDOM_FLOAT(-1,1),RANDOM_FLOAT(-1,1), RANDOM_FLOAT(0,1)), g_iModelIndexSmkball, RANDOM_FLOAT(0.3, 0.4), RANDOM_FLOAT(24,32));

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/hks1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105)); break;
	case 1: EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/hks2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105)); break;
	case 2: EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/hks3.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105)); break;
	}

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], args->iparam2, args->bparam2, TE_BOUNCE_SHELL);
}

//-----------------------------------------------------------------------------
// Purpose: MP5 secondary attack
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - animaiton
// iparam2 - 
// bparam1 - body
// bparam2 - sound selection
//-----------------------------------------------------------------------------
void EV_FireMP52(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector ShellOrigin, ShellVelocity;
	Vector up, right, forward;
	AngleVectors(args->angles, forward, right, up);

	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 6.0f);
	}

	EV_GetDefaultShellInfo(args, args->origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -14, 4);
	FX_Smoke(ShellOrigin+Vector(RANDOM_FLOAT(-1,1),RANDOM_FLOAT(-1,1), RANDOM_FLOAT(0,1)), g_iModelIndexSmkball, RANDOM_FLOAT(0.4, 0.6), RANDOM_FLOAT(18,22));

	if (args->bparam2 == 0)// RANDOM_LONG(0,1))
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/glauncher.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95, 105));
	else
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/glauncher2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95, 105));

	// looks very distracting EV_EjectBrass(ShellOrigin, ShellVelocity, args->angles[YAW], args->iparam2, SHELL_ARGRENADE, TE_BOUNCE_SHOTSHELL);
}

//-----------------------------------------------------------------------------
// Purpose: Shotgun primary attack
// entindex - player
// origin - origin
// angles - angles
// fparam1 - vecDir.x
// fparam2 - vecDir.y
// iparam1 - animaiton
// iparam2 - shell model
// bparam1 - body
// bparam2 - shell body
//-----------------------------------------------------------------------------
void EV_FireShotGunSingle(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);
	Vector angles(args->angles);
	Vector ShellOrigin, ShellVelocity;
	Vector up, right, forward;
	AngleVectors(angles, forward, right, up);

	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 5.0f);
	}

	EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -14, 6);
	EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/shotgun_fire1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0x1f));
	FX_Smoke(ShellOrigin, g_iModelIndexSmkball, RANDOM_FLOAT(0.4, 0.6), 20);
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], args->iparam2, args->bparam2, TE_BOUNCE_SHOTSHELL);
}

//-----------------------------------------------------------------------------
// Purpose: Shotgun secondary attack
// entindex - player
// origin - origin
// angles - angles
// fparam1 - vecDir.x
// fparam2 - vecDir.y
// iparam1 - animaiton
// iparam2 - shell model
// bparam1 - body
// bparam2 - shell body
//-----------------------------------------------------------------------------
void EV_FireShotGunDouble(event_args_t *args)
{
	EV_START(args);
	Vector origin(args->origin);
	Vector angles(args->angles);
	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector up, right, forward;
	int idx = args->entindex;

	AngleVectors(angles, forward, right, up);

	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 10.0f);
	}

	EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/shotgun_fire2.wav", RANDOM_FLOAT(0.98, 1.0), ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

	for (uint32 j = 0; j < 2; ++j)
	{
		EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -14, (float)(6-j));
		EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], args->iparam2, args->bparam2, TE_BOUNCE_SHOTSHELL);
	}
	//EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);
	//EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], args->iparam2, args->bparam2, TE_BOUNCE_SHOTSHELL);
	FX_Smoke(ShellOrigin, g_iModelIndexSmkball, RANDOM_FLOAT(0.8, 1.0), 18);
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 2 bytes of color
// iparam2 - 2 bytes of color
// bparam1 - pev->body
// bparam2 - m_fireMode
//-----------------------------------------------------------------------------
void EV_FireGauss(event_args_t *args)
{
	EV_START(args);
	Vector origin(args->origin);
	Vector vecSrc;
	int idx = args->entindex;
	if (args->bparam2 > FIRE_SEC)// XDM3035: discharge
	{
		const char *sample = NULL;
		switch (RANDOM_LONG(0,2))// XDM: CHAN_WEAPON stops the fire sound. It's bad. Really.
		{
		default:
		case 0:	sample = "weapons/electro4.wav"; break;
		case 1:	sample = "weapons/electro5.wav"; break;
		case 2:	sample = "weapons/electro6.wav"; break;
		}
		EMIT_SOUND(idx, origin, CHAN_BODY, sample, VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96,104));

		EV_MuzzleFlash(idx);

		EV_GetGunPosition(args, vecSrc, origin);
		color24 c = {255,255,191};
		FX_StreakSplash(vecSrc, Vector(0,0,2), c, 10, 112);

		//gEngfuncs.pEfxAPI->R_StreakSplash(origin, g_vecZero, 7, 4, 24.0f, -80,80);// "r_efx.h"

		if (g_pCvarEffects->value > 1.0f)// extra effects
		{
			EntityLight(origin, 12, 255,255,255, 0.2, 32);
			//FX_SparkShower(origin, g_pSpriteZeroFlare, 1, forward, false, 1.0f);
		}
	}
	else// fire
	{
		if (EV_IsLocal(idx))
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
			V_PunchAxis(PITCH, 2.0f);
		}
		STOP_SOUND(idx, CHAN_WEAPON, "weapons/gauss_spin.wav");
		//EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/gauss_spin.wav", 0.0f, 0.0f, SND_STOP, 0);
		gEngfuncs.pEventAPI->EV_StopAllSounds(idx, CHAN_WEAPON);

		if (args->bparam2 == FIRE_PRI)// primary fire
			EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/gauss_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96,104));
		else
			EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/gauss_fire2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96,104));

		uint32 color_int = (((uint16)args->fparam1)<<16) + ((uint16)args->fparam1);
		EntityLight(origin, 18, RGB2R(color_int),RGB2G(color_int),RGB2B(color_int), 1.0, 32);// XDM3037: 'angles' is a bad place to store data because of CODEC procedures
		EV_MuzzleFlash(idx);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - animation
// iparam2 - 
// bparam1 - pev->body
// bparam2 - m_fInZoom
//-----------------------------------------------------------------------------
void EV_FireCrossbow(event_args_t *args)
{
	EV_START(args);
	EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));

	if (EV_IsLocal(args->entindex))
	{
		if (args->bparam1 == 0)// HLBUG: fire animation will be cached and played too late!
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

		//if (args->bparam2)
		//	EMIT_SOUND( idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF) );
		V_PunchAxis(PITCH, 2.0f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - DLAUNCHER_ATTACK_INTERVAL
// fparam2 - 
// iparam1 - animation
// iparam2 - sound pitch
// bparam1 - pev->body
// bparam2 - firemode
//-----------------------------------------------------------------------------
void EV_FireRazorDisk(event_args_t *args)
{
	EV_START(args);
	EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/razordisk_fire.wav", VOL_NORM, ATTN_NORM, 0, args->iparam2);
	if (EV_IsLocal(args->entindex))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 1.0f);
	}
	Vector vecForward, vecSrc;
	AngleVectors(args->angles, vecForward, NULL, NULL);
	EV_GetGunPosition(args, vecSrc, args->origin);
	EV_MuzzleFlash(args->entindex);
	color24 c;
	if (args->bparam1 == 0)
		{c.r=255; c.g=239; c.b=191;}
	else
		{c.r=255; c.g=223; c.b=159;}
	FX_StreakSplash(vecSrc+vecForward*2.0f, vecForward, c, args->bparam1==0?10:14, args->bparam1==0?112:128, true, false, false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FireRpg(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/rpg_fire.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 5.0f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FireHornetGun(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		V_PunchAxis(PITCH, RANDOM_FLOAT(0.1f, 2.0f));
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
	}
	FX_MuzzleFlashSprite(args->origin, idx, 0, *g_pMuzzleFlashSprites[3], 0.02f, true);
	EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/hgun_fire.wav", VOL_NORM, ATTN_NORM, 0, 100);
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - v_animation
// iparam2 - 
// bparam1 - v_body
// bparam2 - firemode
//-----------------------------------------------------------------------------
void EV_FireChemGun(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		V_PunchAxis(PITCH, RANDOM_FLOAT(0.1f, 2.0f));
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
	}
	//FX_MuzzleFlashSprite(args->origin, idx, 0, *g_pMuzzleFlashSprites[(int)test1->value], 0.02f, true);

	if (args->bparam2 == FIRE_PRI)
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/chemgun_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98,102));
	else
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, "weapons/chemgun_fire2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98,102));
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FireTripmine(event_args_t *args)
{
	EV_START(args);
	if (EV_IsLocal(args->entindex))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

	//EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/trip_fire.wav", VOL_NORM, ATTN_NORM, 0, 100);
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FireSnark(event_args_t *args)
{
	EV_START(args);
	int idx = args->entindex;
	Vector origin(args->origin);

	if (EV_IsLocal(idx))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND(idx, origin, CHAN_WEAPON, "squeek/sqk_hunt1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM); break;
	case 1: EMIT_SOUND(idx, origin, CHAN_WEAPON, "squeek/sqk_hunt2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM); break;
	case 2: EMIT_SOUND(idx, origin, CHAN_WEAPON, "squeek/sqk_hunt3.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM); break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - m_fInZoom
// fparam2 - 
// iparam1 - animation
// iparam2 - shell model
// bparam1 - body
// bparam2 - shell body
//-----------------------------------------------------------------------------
void EV_FireSniperRifle(event_args_t *args)
{
	EV_START(args);
	Vector origin(args->origin);
	Vector angles(args->angles);
	Vector ShellOrigin, ShellVelocity;
	Vector up, right, forward;
	AngleVectors(angles, forward, right, up);

	EV_MuzzleFlash(args->entindex);
	if (EV_IsLocal(args->entindex))
	{
		if (args->fparam1 == 0.0f)// !m_fInZoom // HLBUG: fire animation will be cached and played too late!
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

		V_PunchAxis(PITCH, 4.0f);
	}
	EV_GetDefaultShellInfo(args, origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -14, 4);
	FX_Smoke(ShellOrigin+Vector(RANDOM_FLOAT(-1,1),RANDOM_FLOAT(-1,1), RANDOM_FLOAT(0,1)), g_iModelIndexSmkball, RANDOM_FLOAT(0.2, 0.4), RANDOM_FLOAT(24,30));
	EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/rifle_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], args->iparam2, args->bparam2, TE_BOUNCE_SHELL);
}

// hack to move particles along with player
bool PS_MoveAlong_OnParticleUpdate(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	cl_entity_t *pFollow = GetUpdatingEntity(pSystem->m_iFollowEntity);
	if (pFollow)
	{
		int prevpos = pFollow->current_position - 1;// we assume position moves to the next slot each frame and is wrapped around array size
		if (prevpos < 0)
			prevpos = HISTORY_MAX;
		else if (prevpos >= HISTORY_MAX)
			prevpos = 0;

		pParticle->m_vPos += pFollow->ph[pFollow->current_position].origin - pFollow->ph[prevpos].origin;//(pFollow->origin - pFollow->prevstate.origin);
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - life
// fparam2 - 
// iparam1 - animation
// iparam2 - beam modelindex
// bparam1 - body
// bparam2 - firemode+firestate
//-----------------------------------------------------------------------------
void EV_FireDisplacer(event_args_t *args)
{
	EV_START(args);
	int numattachments;
	cl_entity_t *pAttachmentModel;
	byte r,g,b;
	byte firemode, firestate;
	BitSplit2x4bit(args->bparam2, firemode, firestate);
	if (firemode > 1)// array bounds!
		firemode = 1;
	r = g_DisplacerColors[firemode].r;
	g = g_DisplacerColors[firemode].g;
	b = g_DisplacerColors[firemode].b;
	if (EV_IsLocal(args->entindex) && g_ThirdPersonView == 0)
	{
		pAttachmentModel = gEngfuncs.GetViewModel();
		numattachments = 3;
	}
	else
	{
		pAttachmentModel = gEngfuncs.GetEntityByIndex(args->entindex);
		numattachments = 2;// there are 3 attachments total on a player model, 0=start, 1,2=ends
	}

	if (firestate == FIRE_ON)
	{
		if (EV_IsLocal(args->entindex))
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);//pAttachmentModel->curstate.body);
			V_PunchAxis(PITCH, 1.0f);
		}
		EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/displacer_fire.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		if (g_pCvarEffects->value > 0)
			DynamicLight(args->origin, 128.0f, r,g,b, 0.4f, 100);

		return;
	}

	EV_MuzzleFlash(args->entindex);

	//float life = args->fparam1;// + 0.5;
	if (pAttachmentModel != NULL)
	{
		for (int i=0; i<numattachments; ++i)
			gEngfuncs.pEfxAPI->R_BeamEnts(pAttachmentModel->index | 0x1000, pAttachmentModel->index | (0x1000 * (2+i)), args->iparam2, args->fparam1/*life*/, 0.25, 0.4, 1.0, 0.5, 0, (float)args->iparam2, (float)r/255.0f, (float)g/255.0f, (float)b/255.0f);
			//gEngfuncs.pEfxAPI->R_BeamEnts(view->index | 0x1000, view->index | 0x2000, sprIndex, life, 0.5, 0.2, 1.0, 0.5, 0, args->iparam2, (float)r/255, g/255, b/255);
			//gEngfuncs.pEfxAPI->R_BeamEnts(view->index | 0x1000, view->index | 0x3000, sprIndex, life, 0.5, 0.2, 1.0, 0.5, 0, args->iparam2, r/255, g/255, b/255);
			//gEngfuncs.pEfxAPI->R_BeamEnts(view->index | 0x1000, view->index | 0x4000, sprIndex, life, 0.5, 0.2, 1.0, 0.5, 0, args->iparam2, r/255, g/255, b/255);
	}
	if (g_pCvarEffects->value > 0.0f)
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSLight(args->origin, r,g,b, 16, NULL, 40.0, args->fparam1/*life*/, true), RENDERSYSTEM_FLAG_NOCLIP, /*view->index ?*/args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);// don't follow viewmodel!
	}
	if (EV_IsLocal(args->entindex))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		//gEngfuncs.pEfxAPI->R_BeamEnts(int startEnt, int endEnt, int modelIndex, f life, f width, f amplitude, f bright, f spd, i startFrm, f framerate, f r, g, b
	}
	EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/displacer_spinup.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	if (g_pRenderManager && g_pCvarParticles->value > 0.0f)
	{
		CPSSparks *pSparks = new CPSSparks((g_pCvarParticles->value > 1.0f)?192:64, args->origin, 0.5f,0.2f,-0.5f, -32, 2.0f, r,g,b,1.0f,-0.75f, g_pSpriteIceBall2, kRenderTransAdd, 3.0f);// IEngineStudio.GetModelByIndex(g_iModelIndexColdball2)
		if (pSparks)
		{
			pSparks->m_OnParticleUpdate = PS_MoveAlong_OnParticleUpdate;
			g_pRenderManager->AddSystem(pSparks, RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
		}
	}
}


//-----------------------------------------------------------------------------
// Flame definitions
// UNDONE: these should not be hardcoded, but calculated based on server data
//-----------------------------------------------------------------------------
#define FLACT_CREATE				(1 << 0)
#define FLACT_SHUTDOWN				(1 << 1)
#define FLACT_UPDATE				(1 << 2)
// flame parameters: _F - flame, _S - smoke
// FLAME MODE 2 WORKS NOW FOR DISTANCE 256 AND SPEED 256
#define FLAME_ALPHA_DELTA1			-0.8f
#define FLAME_ALPHA_DELTA2			-0.8f
#define FLAME_ALPHA_DELTA_C			-1.0f// collided
#define FLAME_ALPHA_START_F1		255
#define FLAME_ALPHA_START_F2		207
#define FLAME_SCALE_START_F1		0.0125f
#define FLAME_SCALE_START_F2		0.00625f
#define FLAME_SCALE_START_S1		0.28f
#define FLAME_SCALE_DELTA_F1_1		72.0f
#define FLAME_SCALE_DELTA_F1_2		72.0f
#define FLAME_SCALE_DELTA_F2_1		96.0f
#define FLAME_SCALE_DELTA_F2_2		96.0f
#define FLAME_SCALE_DELTA_S1_1		112.0f
#define FLAME_SCALE_DELTA_S1_2		112.0f
#define FLAME_SCALE_DELTA_C			40.0f// collided
#define FLAME_SYSTEM_LIFE_TIME		0.5f// time is limited so the system won't hang if something goes wrong
#define FLAME_ATTACHMENT			0//1023// HACK: BUGBUG: attachments are erased in HL engine, but we cannot use offset instead (looks very bad)
#define FLAME_SPEED_MULTIPLIER		1.0f// we use this to speed up particle flight, but still retain the distance
#define FLAME_RS_PREFIX				"PLFLAMERSYS"

//-----------------------------------------------------------------------------
// Purpose: Custom per-particle colliison function for flame particles
// Note: this is called at least once per frame for each particle
//-----------------------------------------------------------------------------
bool FX_FlameCloud_OnParticleCollide(CParticleSystem *pSystem, CParticle *pParticle, void *pData, pmtrace_s *pTrace)
{
/*#if defined(_DEBUG)
	pParticle->m_fColor[0] = 0;// TEST
	pParticle->m_fColor[1] = 1;
	pParticle->m_fColor[2] = 0;
#endif*/
	pParticle->m_vVel *= 0.125f;// slow down fast
	pParticle->m_fSizeDelta = FLAME_SCALE_DELTA_C;// change growing rate
	pParticle->m_fColorDelta[3] = FLAME_ALPHA_DELTA_C;// fade faster // don't '+=' or '*=', just '=' !
	return false;// don't do standard collisions, a few ops faster
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
bool FX_FlameCloud_OnParticleUpdateF1(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	pParticle->m_vVel *= 1.0f - (float)(0.02*FLAME_SPEED_MULTIPLIER*elapsedTime);//*cl_test1->value);// decrease velocity over time
	return true;
}

bool FX_FlameCloud_OnParticleUpdateF2(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	pParticle->m_vVel *= 1.0f - (float)(0.02*FLAME_SPEED_MULTIPLIER*elapsedTime);//*cl_test1->value);
	pParticle->m_fSizeX += elapsedTime*0.5;
	pParticle->m_fSizeY += elapsedTime*0.5;
	return true;
}

bool FX_FlameCloud_OnParticleUpdateS1(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	pParticle->m_vVel *= 1.0f - (float)(0.02*FLAME_SPEED_MULTIPLIER*elapsedTime);//*cl_test1->value);
	pParticle->m_fSizeX += elapsedTime*0.55;
	pParticle->m_fSizeY += elapsedTime*0.55;
	return true;
}

float FlameLightRadiusCallback(CRSLight *pSystem, const float &time)
{
	return RANDOM_FLOAT(0.75f, 1.0f);
}

//-----------------------------------------------------------------------------
// Purpose: called in ON/OFF/SWITCH manner
// entindex - player
// origin - origin
// angles - angles
// fparam1 - distance
// fparam2 - 
// iparam1 - seq
// iparam2 - 
// bparam1 - body
// bparam2 - firemode|firestate
//-----------------------------------------------------------------------------
void EV_FireFlame(event_args_t *args)
{
	EV_START(args);
	//EV_PrintParams("EV_FireFlame", args);
	/* POINT IN CONE TEST (visual test only)
	Vector vConeTop = args->origin;
	Vector vForward, vRight, vUp;
	AngleVectors(args->angles, vForward, vRight, vUp);
	Vector vConeBase = vConeTop + vForward * 64.0f;
	Vector vDot = vConeTop + vForward * cl_test1->value + vRight * cl_test2->value + vUp * cl_test3->value;
	float fConeRadius = 48.0f;
	bool bHit = PointInCone(vDot, vConeTop, vConeBase, fConeRadius);
	g_pRenderManager->AddSystem(new CRSSprite(vConeTop, g_vecZero, g_pSpriteMuzzleflash2, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 0.1,0.0, 24.0f, 10.0f));
	g_pRenderManager->AddSystem(new CRSSprite(vConeBase, g_vecZero, g_pSpriteMuzzleflash2, kRenderTransAdd, 255,255,255, 0.5,0.0f, 0.02f*fConeRadius,0.0, 24.0f, 10.0f));
	g_pRenderManager->AddSystem(new CRSBeam(vConeBase, vConeTop, g_pSpriteLaserBeam, kRenderTransAdd, 255,255,255, fConeRadius,0.0f,  0.25,0.0, 24.0f, 10.0f));
	g_pRenderManager->AddSystem(new CRSBeam(vConeBase - vRight*fConeRadius, vConeBase + vRight*fConeRadius, g_pSpriteLaserBeam, kRenderTransAdd, 255,255,255, 0.5f,0.0f, 0.1,0.0, 24.0f, 10.0f));
	g_pRenderManager->AddSystem(new CRSSprite(vDot, g_vecZero, g_pSpriteMuzzleflash2, kRenderTransAdd, bHit?0:255,bHit?255:0,0, 1.0f,0.0f,  0.1,0.0, 24.0f, 10.0f));
	conprintf(1, "PointInCone(): %s\n", bHit?"in":"out");*/
	uint32 action = 0;
	int idx = args->entindex;
	//const char *sound = NULL;
	const char *sound1 = "weapons/flame_run1.wav";
	const char *sound2 = "weapons/flame_run2.wav";
#if defined(_DEBUG)
	const char *pFireStateDesc = "UNKNOWN";
#endif
	cl_entity_t *pClient = gEngfuncs.GetEntityByIndex(idx);
	Vector vecSrc;
	EV_GetGunPosition(args, vecSrc, args->origin);// gun position or default origin
//#if defined(CLIENT_FLAME)
	char sys_name[RENDERSYSTEM_USERNAME_LENGTH];// XDM3038b: now we find system reliably by NAME
	_snprintf(sys_name, RENDERSYSTEM_USERNAME_LENGTH, "%s%d", FLAME_RS_PREFIX, idx);//%02X ?
	bool bSystemFound = false;
	// Find existing systems (if any) attached to this weapon/player
	if (g_pRenderManager)
	{
		CRenderSystem *pSystem = g_pRenderManager->FindSystemByName(sys_name);// one is enough right now
		if (pSystem && !pSystem->IsShuttingDown() && !pSystem->IsRemoving())// don't count systems that were turned off
		{
			//DBG_PRINTF("CL: EV_FireFlame(%d): found: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
			ASSERT(pSystem->m_iFollowEntity == idx);
			if (pSystem->m_iFollowEntity == idx)
				bSystemFound = true;
		}
	}
//#endif // CLIENT_FLAME
	byte firemode, firestate;
	BitSplit2x4bit(args->bparam2, firemode, firestate);
	/*if (firemode == FIRE_PRI)
		sound = sound1;
	else
		sound = sound2;*/

	// Read fire mode and decide what to do
	if (firestate == FIRE_OFF)// stop fire
	{
#if defined(_DEBUG)
		pFireStateDesc = "FIRE_OFF";
#endif
		if (EV_IsLocal(idx))
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);

		STOP_SOUND(idx, CHAN_WEAPON, (firemode == FIRE_PRI)?sound1:sound2);
		action |= FLACT_SHUTDOWN;
	}
	else if (firestate == FIRE_CHARGE)// start fire/update
	{
#if defined(_DEBUG)
		pFireStateDesc = "FIRE_CHARGE";
#endif
		//UTIL_DebugAngles(args->origin, args->angles, 1.0, 32);// DEBUG
		EV_MuzzleFlash(idx);
//#if defined(CLIENT_FLAME)
		if (EV_IsLocal(idx))
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
			V_PunchAxis(PITCH, 0.1f);
		}
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, (firemode == FIRE_PRI)?sound1:sound2, VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
		DBG_PRINTF("CL: EV_FireFlame(%d): firemode: %d, PLAYING SOUND %s\n", idx, (int)(firemode), (firemode == FIRE_PRI)?sound1:sound2);
		action |= FLACT_CREATE;
//#endif CLIENT_FLAME
	}
	else if (firestate == FIRE_ON)// update
	{
#if defined(_DEBUG)
		pFireStateDesc = "FIRE_ON";
#endif
		action |= FLACT_UPDATE;
		if (!bSystemFound)// system was not found, recreate
			action |= FLACT_CREATE;
	}
	/*else if (firestate == FIRE_CHANGEMODE)
	{
#if defined(_DEBUG)
		pFireStateDesc = "FIRE_CHANGEMODE";
#endif
		EV_MuzzleFlash(idx);
		action |= FLACT_CREATE|FLACT_SHUTDOWN;// Suitable if both modes use RenderSystem: action |= FLACT_UPDATE;// should we recreate?
		STOP_SOUND(idx, CHAN_WEAPON, (firemode == FIRE_PRI)?sound2:sound1);// stop old sound
		EMIT_SOUND(idx, args->origin, CHAN_WEAPON, (firemode == FIRE_PRI)?sound1:sound2, VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);// play new sound
	}*/
#if defined(_DEBUG)
	DBG_PRINTF("CL: EV_FireFlame(%d): firemode: %d, firestate: %s, action: %u\n", idx, (int)(firemode), pFireStateDesc, action);
#endif

//#if defined(CLIENT_FLAME)
	if (g_pRenderManager)// This is the part where we work with RSystems. Warning! pSystem becomes NULL after search.
	{
		float fSpeed = args->fparam1 * FLAME_SPEED_MULTIPLIER;
		if (action & FLACT_SHUTDOWN)// MUST BE FIRST IN ORDER
		{
			if (bSystemFound)// soft-shutdown found systems and make them unfindable in the future
			{
				char sys_name_disable[RENDERSYSTEM_USERNAME_LENGTH];// WARNING: this may be called too fast so entindex and NAME may repeat! Use GetIndex().
				CRenderSystem *pSystem = NULL;
				while ((pSystem = g_pRenderManager->FindSystemByName(sys_name, pSystem)) != NULL)// there may be many systems (fire, smoke, etc)
				{
					DBG_PRINTF("CL: EV_FireFlame(%d): removing system: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
					pSystem->m_iFollowFlags = RENDERSYSTEM_FFLAG_ICNF_REMOVE|RENDERSYSTEM_FFLAG_DONTFOLLOW;// it won't emit any new particles so prevent it from searching for owner
					_snprintf(sys_name_disable, RENDERSYSTEM_USERNAME_LENGTH, "-%s_D%d%u", FLAME_RS_PREFIX, idx, pSystem->GetIndex());// so it won't be found again
					strcpy(pSystem->m_szName, sys_name_disable);
					if (pSystem->IsShuttingDown() == false)// just in case
						pSystem->ShutdownSystem();
				}
			}
			else
				conprintf(1, "CL: EV_FireFlame(%d): error: nothing to shut down!\n", idx);
		}
		if (firemode == FIRE_PRI && (action & FLACT_UPDATE))// MUST BE BEFORE "CREATE"
		{
			if (pClient)
				pClient->baseline.velocity = pClient->curstate.velocity = args->velocity;
			if (bSystemFound)// just update existing particle systems with new parameters
			{
				CRenderSystem *pSystem = NULL;
				CParticleSystem *pPartSystem;
				while ((pSystem = g_pRenderManager->FindSystemByName(sys_name, pSystem)) != NULL)
				{
					//DBG_PRINTF("CL: EV_FireFlame(%d): updating system: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
					//UTIL_DebugBeam(args->origin, vecSrc, 4.0f, 255,191,0);// DEBUG
					//if (cl_test2->value > 0) pSystem->m_vecOrigin = vecSrc;

					pSystem->m_fDieTime = gEngfuncs.GetClientTime() + FLAME_SYSTEM_LIFE_TIME;// extend lifetime of this system
					const char *pFoundUID = pSystem->GetUID();
					/* usable only with different flame modes
					size_t offset = strbegin(pFoundUID, FLAME_RS_PREFIX);// "PLFLAMERSYS_F1_"// can also be smoke, etc.
					if (offset == 0)
					{
						conprintf(1, "CL: EV_FireFlame(%d): error: found system by name \"%s\" with bad UID: \"%s\"!\n", idx, sys_name, pFoundUID);
						continue;// error!
					}
					pFoundUID += offset;
					if (strbegin(pFoundUID, "_F1_") > 0)// These parameters are for the flame1
					{
						pSystem->m_fScaleDelta = (firemode == FIRE_PRI)?FLAME_SCALE_DELTA_F1_1:FLAME_SCALE_DELTA_F1_2;// mode may have changed!
						//pSystem->m_color.a = FLAME_ALPHA_START_F1;
						pSystem->m_fColorDelta[3] = (firemode == FIRE_PRI)?FLAME_ALPHA_DELTA1:FLAME_ALPHA_DELTA2;
					}
					else if (strbegin(pFoundUID, "_F2_") > 0)// flame2
					{
						pSystem->m_fScaleDelta = (firemode == FIRE_PRI)?FLAME_SCALE_DELTA_F2_1:FLAME_SCALE_DELTA_F2_2;// mode may have changed!
						//pSystem->m_color.a = FLAME_ALPHA_START_F2;
						pSystem->m_fColorDelta[3] = (firemode == FIRE_PRI)?FLAME_ALPHA_DELTA1:FLAME_ALPHA_DELTA2;
					}
					else if (strbegin(pFoundUID, "_S1_") > 0)// smoke
					{
						pSystem->m_fScaleDelta = (firemode == FIRE_PRI)?FLAME_SCALE_DELTA_S1_1:FLAME_SCALE_DELTA_S1_2;
						pSystem->m_fColorDelta[3] = ((firemode == FIRE_PRI)?FLAME_ALPHA_DELTA1:FLAME_ALPHA_DELTA2)+0.08f;// fade a little slower
					}*/
					if (strcmp(pSystem->GetClassName(), "CParticleSystem") == 0)// PS-only member vars
					{
						pPartSystem = (CParticleSystem *)pSystem;// Unsafe?
						if (pPartSystem)
						{
							pPartSystem->m_fParticleSpeedMin = fSpeed;
							pPartSystem->m_fParticleSpeedMax = pPartSystem->m_fParticleSpeedMin * 1.2f;
						}
					}
				}
			}
			else
			{
				DBG_PRINTF("CL: EV_FireFlame(%d): FLACT_UPDATE: system not found!\n", idx);
				action |= FLACT_CREATE;// RECREATE!
			}
		}
		if (firemode == FIRE_PRI && (action & FLACT_CREATE))// MUST BE AFTER "UPDATE"
		{
			if (!bSystemFound)
			{
				Vector vecForward;
				AngleVectors(args->angles, vecForward, NULL, NULL);
//#if defined(_DEBUG)
//				UTIL_DebugBeam(args->origin, vecSrc, 4.0f, 255,191,0);// DEBUG
//				UTIL_DebugBeam(vecSrc, vecSrc+vecForward*160.0f, 4.0f, 255,127,0);// DEBUG
//#endif
				if (g_pCvarEffects->value >= 0.0f)
				{
					CParticleSystem *pSystem = new CParticleSystem(144, vecSrc, vecForward, g_pSpriteFlameFire, kRenderTransAdd, FLAME_SYSTEM_LIFE_TIME);
					if (pSystem)
					{
						//pSystem->m_vOffset.Set(0,8,VEC_HANDS_OFFSET);
						pSystem->m_vSpread = VECTOR_CONE_5DEGREES;
						pSystem->m_fScale = FLAME_SCALE_START_F1;
						pSystem->m_fScaleDelta = (firemode == FIRE_PRI)?FLAME_SCALE_DELTA_F1_1:FLAME_SCALE_DELTA_F1_2;
						pSystem->m_color.Set(79,71,255,FLAME_ALPHA_START_F1);// start blue
						pSystem->m_fColorDelta[0] = 1.25f; pSystem->m_fColorDelta[1] = 0.75f; pSystem->m_fColorDelta[2] = -0.01f;
						pSystem->m_fColorDelta[3] = (firemode == FIRE_PRI)?FLAME_ALPHA_DELTA1:FLAME_ALPHA_DELTA2;
						pSystem->m_fBounceFactor = 0.0f;
						pSystem->m_iFollowAttachment = FLAME_ATTACHMENT;
						pSystem->m_iTraceFlags = PM_STUDIO_BOX;// don't ignore glass or players
						pSystem->m_iMovementType = PSMOVTYPE_DIRECTED;
						pSystem->m_fParticleScaleMin = 0.9f;
						pSystem->m_fParticleScaleMax = 1.1f;
						pSystem->m_fParticleSpeedMin = fSpeed * 0.8f;
						pSystem->m_fParticleSpeedMax = fSpeed * 1.0f;
						pSystem->m_fParticleWeight = -0.01f;// float upwards
						pSystem->m_fParticleCollisionSize = 16.0f;
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_WATER);
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_SLIME);
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_LAVA);
						//pSystem->m_OnParticleInitialize = FX_FlameCloud_OnParticleInitialize;
						//pSystem->m_pOnParticleUpdateData = ?? where do I store it
						pSystem->m_OnParticleUpdate = FX_FlameCloud_OnParticleUpdateF1;
						pSystem->m_OnParticleCollide = FX_FlameCloud_OnParticleCollide;
						strncpy(pSystem->m_szName, sys_name, RENDERSYSTEM_USERNAME_LENGTH-1);
						if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, idx, RENDERSYSTEM_FFLAG_ICNF_NODRAW/*|RENDERSYSTEM_FFLAG_USEOFFSET*/) != RS_INDEX_INVALID)
						{
							pSystem->InitializeSystem();//!
							char sys_uid[RENDERSYSTEM_UID_LENGTH];// XDM3038b: now we find system reliably by NAME
							_snprintf(sys_uid, RENDERSYSTEM_UID_LENGTH, "%s_F1_%d%u", FLAME_RS_PREFIX, idx, pSystem->GetIndex());// ID as fire1
							ASSERT(pSystem->SetUID(sys_uid) == true);
							DBG_PRINTF("CL: EV_FireFlame(%d): created system: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
						}
					}
					else
						conprintf(1, "CL: EV_FireFlame(%d): error: failed to create system F1!\n", idx);
				}// g_pCvarEffects
				if (g_pCvarEffects->value >= 1.0f)
				{
					CParticleSystem *pSystem = new CParticleSystem(96, vecSrc, vecForward, g_pSpriteFlameFire2, kRenderTransAdd, FLAME_SYSTEM_LIFE_TIME);
					if (pSystem)
					{
						//pSystem->m_vOffset.Set(0,8,VEC_HANDS_OFFSET);
						pSystem->m_vSpread = VECTOR_CONE_5DEGREES;
						pSystem->m_fScale = FLAME_SCALE_START_F2;
						pSystem->m_fScaleDelta = (firemode == FIRE_PRI)?FLAME_SCALE_DELTA_F2_1:FLAME_SCALE_DELTA_F2_2;
						pSystem->m_color.Set(225,255,207,FLAME_ALPHA_START_F2);
						pSystem->m_fColorDelta[0] = 0.0f; pSystem->m_fColorDelta[1] = 0.0f; pSystem->m_fColorDelta[2] = 0.8f;
						pSystem->m_fColorDelta[3] = (firemode == FIRE_PRI)?FLAME_ALPHA_DELTA1:FLAME_ALPHA_DELTA2;
						pSystem->m_fBounceFactor = 0.0f;
						pSystem->m_iFollowAttachment = FLAME_ATTACHMENT;
						pSystem->m_iTraceFlags = PM_STUDIO_BOX;// don't ignore glass or players
						pSystem->m_iMovementType = PSMOVTYPE_DIRECTED;
						pSystem->m_fParticleScaleMin = 0.9f;
						pSystem->m_fParticleScaleMax = 1.2f;
						pSystem->m_fParticleSpeedMin = fSpeed * 1.0f;
						pSystem->m_fParticleSpeedMax = fSpeed * 1.25f;
						pSystem->m_fParticleWeight = -0.25f;// float upwards
						pSystem->m_fParticleCollisionSize = 14.0f;
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_WATER);
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_SLIME);
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_LAVA);
						//pSystem->m_OnParticleInitialize = FX_FlameCloud_OnParticleInitialize;
						//pSystem->m_pOnParticleUpdateData = ?? where do I store it
						pSystem->m_OnParticleUpdate = FX_FlameCloud_OnParticleUpdateF2;
						pSystem->m_OnParticleCollide = FX_FlameCloud_OnParticleCollide;
						strncpy(pSystem->m_szName, sys_name, RENDERSYSTEM_USERNAME_LENGTH-1);
						if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, idx, RENDERSYSTEM_FFLAG_ICNF_NODRAW/*|RENDERSYSTEM_FFLAG_USEOFFSET*/) != RS_INDEX_INVALID)
						{
							pSystem->InitializeSystem();//!
							char sys_uid[RENDERSYSTEM_UID_LENGTH];// XDM3038b: now we find system reliably by NAME
							_snprintf(sys_uid, RENDERSYSTEM_UID_LENGTH, "%s_F2_%d%u", FLAME_RS_PREFIX, idx, pSystem->GetIndex());// ID as fire2
							ASSERT(pSystem->SetUID(sys_uid) == true);
							DBG_PRINTF("CL: EV_FireFlame(%d): created system: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
						}
					}
					else
						conprintf(1, "CL: EV_FireFlame(%d): error: failed to create system F2!\n", idx);
				}// g_pCvarEffects
				if (g_pCvarEffects->value >= 0.0f)
				{
					CParticleSystem *pSystem = new CParticleSystem(128, vecSrc, vecForward, g_pSpriteSmkBall, kRenderTransAlpha, FLAME_SYSTEM_LIFE_TIME);
					if (pSystem)
					{
						//pSystem->m_vOffset.Set(0,8,VEC_HANDS_OFFSET);
						pSystem->m_vSpread = VECTOR_CONE_7DEGREES;
						pSystem->m_fScale = FLAME_SCALE_START_S1;
						pSystem->m_fScaleDelta = (firemode == FIRE_PRI)?FLAME_SCALE_DELTA_S1_1:FLAME_SCALE_DELTA_S1_2;
						pSystem->m_color.Set(31,31,31,255);
						pSystem->m_fColorDelta[0] = 0.0f; pSystem->m_fColorDelta[1] = 0.0f; pSystem->m_fColorDelta[2] = 0.0f;
						pSystem->m_fColorDelta[3] = ((firemode == FIRE_PRI)?FLAME_ALPHA_DELTA1:FLAME_ALPHA_DELTA2)+0.08f;// fade a little slower
						pSystem->m_fBounceFactor = 0.0f;
						pSystem->m_iFollowAttachment = FLAME_ATTACHMENT;
						pSystem->m_iTraceFlags = PM_STUDIO_BOX;// don't ignore glass or players
						pSystem->m_iMovementType = PSMOVTYPE_DIRECTED;
						pSystem->m_fParticleScaleMin = 0.9f;
						pSystem->m_fParticleScaleMax = 1.2f;
						pSystem->m_fParticleSpeedMin = fSpeed;
						pSystem->m_fParticleSpeedMax = fSpeed * 1.2f;
						pSystem->m_fParticleWeight = -0.08f;// float upwards
						pSystem->m_fParticleCollisionSize = 16.0f;
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_WATER);
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_SLIME);
						pSystem->ContentsArrayAdd(pSystem->m_iDestroyContents, CONTENTS_LAVA);
						//pSystem->m_OnParticleInitialize = FX_FlameCloud_OnParticleInitialize;
						//pSystem->m_pOnParticleUpdateData = ?? where do I store it
						pSystem->m_OnParticleUpdate = FX_FlameCloud_OnParticleUpdateS1;
						pSystem->m_OnParticleCollide = FX_FlameCloud_OnParticleCollide;
						strncpy(pSystem->m_szName, sys_name, RENDERSYSTEM_USERNAME_LENGTH-1);
						if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS|RENDERSYSTEM_FLAG_ADDGRAVITY|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, idx, RENDERSYSTEM_FFLAG_ICNF_NODRAW/*|RENDERSYSTEM_FFLAG_USEOFFSET*/) != RS_INDEX_INVALID)
						{
							pSystem->InitializeSystem();//!
							char sys_uid[RENDERSYSTEM_UID_LENGTH];// XDM3038b: now we find system reliably by NAME
							_snprintf(sys_uid, RENDERSYSTEM_UID_LENGTH, "%s_S1_%d%u", FLAME_RS_PREFIX, idx, pSystem->GetIndex());// ID as smoke1
							ASSERT(pSystem->SetUID(sys_uid) == true);
							DBG_PRINTF("CL: EV_FireFlame(%d): created system: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
						}
					}
					else
						conprintf(1, "CL: EV_FireFlame(%d): error: failed to create system S1!\n", idx);
				}// g_pCvarEffects
				if (g_pCvarEffects->value >= 2.0f)
				{
					CRenderSystem *pSystem = new CRSLight(vecSrc, 255,191,63, 192, FlameLightRadiusCallback, 32.0f, 2.0f);
					if (pSystem)
					{
						pSystem->m_vOffset.Set(0,144,2);// this one doesn't use attachments
						strncpy(pSystem->m_szName, sys_name, RENDERSYSTEM_USERNAME_LENGTH-1);
						if (g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS, idx, RENDERSYSTEM_FFLAG_ICNF_NODRAW|RENDERSYSTEM_FFLAG_USEOFFSET) != RS_INDEX_INVALID)
						{
							pSystem->InitializeSystem();//!
							char sys_uid[RENDERSYSTEM_UID_LENGTH];// XDM3038b: now we find system reliably by NAME
							_snprintf(sys_uid, RENDERSYSTEM_UID_LENGTH, "%s_L1_%d%u", FLAME_RS_PREFIX, idx, pSystem->GetIndex());// ID as smoke1
							ASSERT(pSystem->SetUID(sys_uid) == true);
							DBG_PRINTF("CL: EV_FireFlame(%d): created system: uid:%s ei:%d\n", idx, pSystem->GetUID(), pSystem->m_iFollowEntity);
						}
					}
				}// g_pCvarEffects
			}
			else
				conprintf(2, "CL: EV_FireFlame(%d): error: FLACT_CREATE with active system(s)!\n", idx);
		}
	}
//#endif // CLIENT_FLAME
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FirePlasmaGun(event_args_t *args)
{
	EV_START(args);
	Vector vecSrc;
	Vector forward;
	int idx = args->entindex;
	Vector origin(args->origin);
	byte firemode, firestate;
	BitSplit2x4bit(args->bparam2, firemode, firestate);

	EV_MuzzleFlash(idx);
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(args->iparam1, args->bparam1);
		V_PunchAxis(PITCH, 1.0f);
	}
	if (firemode == FIRE_PRI)
	{
		EMIT_SOUND(idx, origin, CHAN_WEAPON, "weapons/plasma_fire.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(98, 102));
		//EV_GetGunPosition(args, vecSrc, origin);
		if (g_pCvarEffects->value > 0.0)
			DynamicLight(origin, 48, 127,127,255, 1.0, 128.0);
	}
}


#define GRENMSGSIZE 128
static char g_szGrenModeMessageName[MAX_TITLE_NAME];
static char g_szGrenModeMessage[GRENMSGSIZE];
static client_textmessage_t g_GrenModeMessage;

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - skin
// iparam2 - 
// bparam1 - show message
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_GrenMode(event_args_t *args)
{
	EV_START(args);
	int skin = args->iparam1;
	/*cl_entity_t *pClient = gEngfuncs.GetEntityByIndex(args->entindex);
	if (IsActivePlayer(pClient))
	{
		pClient->prevstate.iuser4 = skin;
		pClient->curstate.iuser4 = skin;// XDM3035b TEST ONLY!
		pClient->baseline.iuser4 = skin;
	}*/

	if (EV_IsLocal(args->entindex))
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();
		if (view != NULL)
			view->baseline.skin = view->curstate.skin = skin;

		if (args->bparam1 > 0)
		{
			//char msgname[32];// message/sound name + NULLterm
			memset(g_szGrenModeMessage, 0, sizeof(g_szGrenModeMessage));
			memset(&g_GrenModeMessage, 0, sizeof(client_textmessage_t));
			_snprintf(g_szGrenModeMessageName, MAX_TITLE_NAME, "GREN_MODE%d\0", skin);
			client_textmessage_t *msg = TextMessageGet(g_szGrenModeMessageName);
			if (msg)
			{
				memcpy(&g_GrenModeMessage, msg, sizeof(client_textmessage_t));// copy localized message
				strncpy(g_szGrenModeMessage, msg->pMessage, GRENMSGSIZE);// store message TEXT
				g_szGrenModeMessage[GRENMSGSIZE-1] = 0;
				g_GrenModeMessage.pMessage = g_szGrenModeMessage;
			}
			else// somebody destroyed all subtitles!
			{
				char *s;
				int r,g,b;
				if (skin == 0)
				{
					s = "Explosion grenade\n";
					r = 127; g = 191; b = 127;
				}
				else if (skin == 1)
				{
					s = "Freeze grenade\n";
					r = 0; g = 0; b = 255;
				}
				else if (skin == 2)
				{
					s = "Poison grenade\n";
					r = 0; g = 255; b = 0;
				}
				else if (skin == 3)
				{
					s = "Fire grenade\n";
					r = 255; g = 191; b = 63;
				}
				else if (skin == 4)
				{
					s = "Radioactive grenade\n";
					r = 255; g = 255; b = 0;
				}
				else if (skin == 5)
				{
					s = "Gravity grenade\n";
					r = 255; g = 255; b = 255;
				}
				else
				{
					s = "Error!\n";
					r = 255; g = 0; b = 0;
				}
				strncpy(g_szGrenModeMessage, s, GRENMSGSIZE);
				//DrawSetTextColor(r, g, b);
				//CenterPrint(s);
				g_GrenModeMessage.x = 0.8;
				g_GrenModeMessage.y = 0.9;
				g_GrenModeMessage.r1 = r;
				g_GrenModeMessage.g1 = g;
				g_GrenModeMessage.b1 = b;
				g_GrenModeMessage.a1 = 255;
				g_GrenModeMessage.r2 = 31;
				g_GrenModeMessage.g2 = 31;
				g_GrenModeMessage.b2 = 31;
				g_GrenModeMessage.a2 = 127;
				g_GrenModeMessage.holdtime = 2.0f;
				g_GrenModeMessage.fxtime = 0.0f;
				g_GrenModeMessage.fadein = 0.0f;
				g_GrenModeMessage.fadeout = 0.5f;
				//g_GrenModeMessage.pName = "GREN_MODE";
				g_GrenModeMessage.pMessage = g_szGrenModeMessage;
			}
			//already cached?	g_GrenModeMessage.pName = g_szGrenModeMessageName;

			if (g_GrenModeMessage.holdtime < 1.0f)
				g_GrenModeMessage.holdtime = 2.0f;// override
			gHUD.m_Message.MessageAdd(&g_GrenModeMessage);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM: same as grenade mode.
// Warning: Uses same globals! Acceptable.
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_TripMode(event_args_t *args)
{
	EV_START(args);
	int skin = args->iparam1;
	if (EV_IsLocal(args->entindex))
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();
		if (view != NULL)
			view->baseline.skin = view->curstate.skin = skin;

		if (args->bparam1 > 0)
		{
			//char msgname[32];// message/sound name + NULLterm
			memset(g_szGrenModeMessage, 0, sizeof(g_szGrenModeMessage));
			memset(&g_GrenModeMessage, 0, sizeof(client_textmessage_t));
			_snprintf(g_szGrenModeMessageName, MAX_TITLE_NAME, "TRIP_MODE%d\0", skin);
			client_textmessage_t *msg = TextMessageGet(g_szGrenModeMessageName);
			if (msg)
			{
				memcpy(&g_GrenModeMessage, msg, sizeof(client_textmessage_t));// copy localized message
				strncpy(g_szGrenModeMessage, msg->pMessage, GRENMSGSIZE);// store message TEXT
				g_szGrenModeMessage[GRENMSGSIZE-1] = 0;
				g_GrenModeMessage.pMessage = g_szGrenModeMessage;
				//already cached?	g_GrenModeMessage.pName = g_szGrenModeMessageName;

				if (g_GrenModeMessage.holdtime < 1.0f)
					g_GrenModeMessage.holdtime = 2.0f;// override
				gHUD.m_Message.MessageAdd(&g_GrenModeMessage);
			}
			else// no default text... lazy
				ConsolePrint(g_szGrenModeMessageName);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sent when the player STARTS zooming and RESETS zoom
// entindex - player
// origin - origin
// angles - angles
// fparam1 - final FOV
// fparam2 - 
// iparam1 - sprite index
// iparam2 - sound index
// bparam1 - 
// bparam2 - render mode
//-----------------------------------------------------------------------------
void EV_ZoomCrosshair(struct event_args_s *args)
{
	EV_START(args);
	qboolean local = EV_IsLocal(args->entindex);
	if (local)
	{
		gHUD.m_ZoomCrosshair.SetParams(args->iparam1, (args->bparam1>0)?args->bparam2:-1, args->iparam2, args->fparam1);// XDM3038a
		if (args->iparam2 > 0)
			PlaySoundByIndex(args->iparam2, VOL_NORM);
	}
	//if (local == false || args->bparam1 == 0)// 'zoom in' sound is played in gHUD.m_ZoomCrosshair (for local player only)
		//EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/xbow_aim.wav", VOL_NORM, ATTN_IDLE, 0, (args->bparam1 == 0)?90:PITCH_NORM);
}

//-----------------------------------------------------------------------------
// Purpose: Notifier
// entindex - player
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_NuclearDevice(struct event_args_s *args)
{
	EV_START(args);
	if (EV_IsLocal(args->entindex))
	{
		const char *msgname = NULL;
		client_textmessage_t *msg = NULL;
		//memset(g_szNDDMessage, 0, sizeof(g_szNDDMessage));
		//memset(&g_NDDMessage, 0, sizeof(client_textmessage_t));

		if (args->iparam1 == 0)
		{
			msgname = "ATOMIC_NOAMMO\0";
			msg = TextMessageGet(msgname);
			//TODO	_snprintf(sz, msg->pMessage, szlen, NUKE_AMMO_USE_URANIUM, NUKE_AMMO_USE_SATCHEL);
			if (msg == NULL)
				CenterPrint("Not enough ammo to set up atomic device!\n");
		}
		else if (args->iparam1 == 1)
		{
			msgname = "ATOMIC_SET\0";
			msg = TextMessageGet(msgname);
			if (msg == NULL)
				CenterPrint("Atomic device set up!\n");

			EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/mine_deploy.wav", VOL_NORM, ATTN_NORM, 0, PITCH_LOW);// XDM3037
		}

		if (msg)
		{
			CenterPrint(msg->pMessage);
			/*memcpy(&g_NDDMessage, msg, sizeof(client_textmessage_t));// copy localized message
			strncpy(g_szNDDMessage, msg->pMessage, GRENMSGSIZE);// store message TEXT
			g_szNDDMessage[GRENMSGSIZE-1] = 0;
			g_NDDMessage.pMessage = g_szNDDMessage;
			//g_NDDMessage.pName = "ATOMIC";

			if (g_NDDMessage.holdtime < 1.0f)
				g_NDDMessage.holdtime = 2.0f;// override
			gHUD.m_Message.MessageAdd(&g_NDDMessage);//BUGBUG: WTF? Works only for the first time
			*/
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: OBSOLETE
//-----------------------------------------------------------------------------
void EV_TrainPitchAdjust(event_args_t *args)
{
	EV_START(args);
	conprintf(1, "EV_TrainPitchAdjust was called! This should not happen!\n");
#if 0
	unsigned short us_params;
	int noise;
	int pitch;
	int stop;
	float m_flVolume;
	char sz[24];
	int idx = args->entindex;
	Vector origin(args->origin);

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0f;
	noise		= (int)(((us_params) >> 12) & 0x0007);
	pitch		= (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1: strcpy(sz, "plats/ttrain1.wav"); break;
	case 2: strcpy(sz, "plats/ttrain2.wav"); break;
	case 3: strcpy(sz, "plats/ttrain3.wav"); break; 
	case 4: strcpy(sz, "plats/ttrain4.wav"); break;
	case 5: strcpy(sz, "plats/ttrain6.wav"); break;// WTF?!
	case 6: strcpy(sz, "plats/ttrain7.wav"); break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	else
		EMIT_SOUND(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
#endif
}
