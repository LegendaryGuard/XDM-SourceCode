#include "hud.h"
#include "vector.h"
#include "cl_util.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "eventscripts.h"
#include "r_efx.h"
#include "event_api.h"
#include "eiface.h"
#include "shared_resources.h"
#include "cl_fx.h"
#include "colors.h"
#include "weapondef.h"
#include "decals.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeamStar.h"
#include "RSCylinder.h"
#include "RSDelayed.h"
#include "RSLight.h"
#include "RSTeleparts.h"
#include "RotatingSystem.h"
#include "RSDisk.h"
#include "RSRadialBeams.h"
#include "RSSprite.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "PSFlameCone.h"
#include "PSSparks.h"

extern "C"
{
void EV_AcidGrenade(struct event_args_s *args);
void EV_BeamBlast(struct event_args_s *args);
void EV_BeamImpact(struct event_args_s *args);
void EV_BlackBall(struct event_args_s *args);
void EV_BoltHit(struct event_args_s *args);
void EV_BulletImpact(struct event_args_s *args);
void EV_Explosion(struct event_args_s *args);
void EV_Flame(struct event_args_s *args);
void EV_FlameTrail(struct event_args_s *args);
void EV_FuncTankFire(struct event_args_s *args);
void EV_GrenExp(struct event_args_s *args);
void EV_Hornet(struct event_args_s *args);
void EV_LightProjectile(struct event_args_s *args);
void EV_PlasmaBall(struct event_args_s *args);
void EV_RazorDisk(struct event_args_s *args);
void EV_Rocket(struct event_args_s *args);
void EV_SparkShower(struct event_args_s *args);
void EV_Teleporter(struct event_args_s *args);
void EV_TeleporterHit(struct event_args_s *args);
void EV_Trail(struct event_args_s *args);
void EV_WarpBall(struct event_args_s *args);
void EV_PM_Fall(struct event_args_s *args);
void EV_PM_Longjump(struct event_args_s *args);
}

extern color24 g_DisplacerColors[];


//-----------------------------------------------------------------------------
// Purpose: acid grenade event
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - lifetime
// fparam2 - effect scale
// iparam1 - sprite (trail) index
// iparam2 - 
// bparam1 - make light
// bparam2 - touch/detonate
//-----------------------------------------------------------------------------
void EV_AcidGrenade(struct event_args_s *args)
{
	EV_START(args);
	Vector vecSpot(args->origin);
	// XDM3038c: FAIL VectorAdd(args->origin, args->angles, vecSpot);// args->angles is the normal
	if (args->bparam2 == AGRENADE_EV_TOUCH)// touch
	{
		if (!UTIL_PointIsFar(args->origin))
		{
			gEngfuncs.pEfxAPI->R_Sprite_Trail(0, args->origin, vecSpot, args->iparam1, 24, 0.0, args->fparam2, RANDOM_LONG(96,102), 255, RANDOM_LONG(56,64));
			EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "weapons/agrenade_hit1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95, 105));

			if (args->bparam1 > 0)// make light
				DynamicLight(args->origin, 32, 0,255,0, args->fparam1, 0.0);
		}
		//UTIL_DebugAngles(args->origin, args->angles, 3, 16);
		Vector vecDir;
		AngleVectors(args->angles, vecDir, NULL, NULL);
		/*debugVector v1 = args->origin - vecDir * 32;
		Vector v2 = args->origin + vecDir * 32;
		gEngfuncs.pEfxAPI->R_BeamPoints(v1, v2, g_iModelIndexLaser, 5.0f, 0.25f, 0.0f, 1.0f, 10.0, 0, 30, 1,1,0);*/
		DecalTrace(RANDOM_LONG(DECAL_SPIT1, DECAL_SPIT2), args->origin - vecDir, args->origin + vecDir);
	}
	else if (args->bparam2 == AGRENADE_EV_HIT)// detonate
	{
		// TODO
		/*TEMPENTITY *pEnt = gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, args->fparam2, args->iparam1, kRenderTransAdd, kRenderFxNone, 0.85, 2.0, FTENT_SPRANIMATE);
		if (pEnt)
			pEnt->entity.curstate.framerate = 16.0;*/

		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteAcidSplash, kRenderTransAdd, 255,255,255, 1.0f,0.0f, args->fparam2, 0.0f, 20.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

		//gEngfuncs.pEfxAPI->R_StreakSplash(args->origin, g_vecZero, 2, args->iparam2, 32, -128, 128);
		if (!UTIL_PointIsFar(args->origin))
		{
			color24 c = {63,255,0};
			FX_StreakSplash(args->origin, g_vecZero, c, (g_pCvarEffects->value > 1.0f)?48:32, 144);
			EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "weapons/agrenade_blow.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95, 105));
		}

		if (g_pCvarEffects->value > 0)
			DynamicLight(args->origin, 80, 0,255,0, args->fparam1, 80);
	}
}

//-----------------------------------------------------------------------------
// Purpose: beamrifle beam explosion
// entindex - 
// origin - 
// angles - 
// fparam1 - damage
// fparam2 - radius
// iparam1 - m_iStar
// iparam2 - m_iGlow
// bparam1 - 
// bparam2 - m_fireMode
//-----------------------------------------------------------------------------
void EV_BeamBlast(struct event_args_s *args)
{
	EV_START(args);
	color24 tracercolor = {55,60,144};
	if (args->bparam2 > 0)// big explostion
	{
		if (!UTIL_PointIsFar(args->origin))
		{
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSBeamStar(args->origin, IEngineStudio.GetModelByIndex(args->iparam1), 64, kRenderTransAdd, 255,255,255, 1.0,-1.2, 4.0,300.0, 4.0), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES);

			EMIT_SOUND(-1, args->origin, CHAN_STATIC, "weapons/beam_blast.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}

		if (g_pCvarEffects->value > 0.0f)
		{
			//gEngfuncs.pEfxAPI->R_StreakSplash(args->origin, g_vecZero, 7, 64, 64, -320, 320);
			FX_StreakSplash(args->origin, g_vecZero, tracercolor, RANDOM_LONG(72,80), 320.0f);

			if (g_pCvarEffects->value > 1.0f)
				FX_SparkShower(args->origin, g_pSpriteAnimGlow01, 16, 640.0f*VectorRandom(), true, 1.0f);
		}
	}
	else// small
	{
		//gEngfuncs.pEfxAPI->R_StreakSplash(args->origin, g_vecZero, 7, 32, 56.0f, -200, 200);
		FX_StreakSplash(args->origin, g_vecZero, tracercolor, RANDOM_LONG(24,32), 200.0f);
	}

	//gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, (args->bparam2 > 0 ?0.5f:0.1f), args->iparam2, kRenderGlow, kRenderFxNoDissipation, 0.6, 0.0, FTENT_FADEOUT);
	FX_TempSprite(args->origin, (args->bparam2 > 0 ?0.5f:0.1f), IEngineStudio.GetModelByIndex(args->iparam2), kRenderGlow, kRenderFxNoDissipation, 0.6, 0.0, FTENT_FADEOUT);

	if (g_pCvarEffects->value > 0)
		DynamicLight(args->origin, args->fparam2, 159,159,255, 3.0, 120.0);

	int contents = gEngfuncs.PM_PointContents(args->origin, NULL);// XDM3038
	if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME)
	{
		FX_BubblesSphere(args->origin, args->fparam2*0.4f, args->fparam2*0.5f, g_pSpriteBubble, (int)(args->fparam1*0.75f), RANDOM_FLOAT(30,40));

		if (g_pRenderManager && g_pCvarEffects->value > 0.0f)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, Vector(0,0,10), IEngineStudio.GetModelByIndex(g_iModelIndexBallSmoke), kRenderTransAdd, 63,63,63, 1.0f,-0.5f, args->fparam1*0.05f,1.0f, 10.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gauss beam
// entindex - 
// origin - 
// angles - if fGoThrough, server sends exit point as 'angles' and direction otherwise
// fparam1 - flDamage
// fparam2 - fGoThrough
// iparam1 - m_iBalls
// iparam2 - 
// bparam1 - m_fireMode
// bparam2 - 0=reflect, 1=explode
//-----------------------------------------------------------------------------
void EV_BeamImpact(struct event_args_s *args)
{
	//EV_START(args);
	float scale;
	float flDamage = args->fparam1;
	int count;
	int firemode = args->bparam1;
	/*int r = 255;
	int g = 255;
	int b = 255;*/
	Vector origin(args->origin);
	//conprintf(2, "EV_BeamImpact(): mode %d, reflect/explode %d, flDamage %g, fGoThrough %g\n", firemode, args->bparam2, flDamage, args->fparam2);

	if (firemode == FIRE_PRI)
	{
		//tau_pri_rgb is unreachable over the network
		//r = 96; g = 255; b = 96;
		scale = flDamage*0.025f;
		count = (int)(flDamage*0.4f);
	}
	else// secondary
	{
		//tau_sec_rgb is unreachable over the network
		//r = 255; g = 255; b = 255;
		scale = 0.1f + flDamage*0.008f;
		count = (int)(2 + flDamage*0.2f);
	}

	if (args->bparam2 == GAUSS_EV_REFLECT)// reflect
	{
		//conprintf(1, "  reflect\n");
		Vector spot(origin);
		spot += args->angles;// 'angles' is normal
		gEngfuncs.pEfxAPI->R_TempSprite(spot, g_vecZero, scale, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 1.0, 0.0, FTENT_FADEOUT);
		gEngfuncs.pEfxAPI->R_Sprite_Trail(0, origin, spot, args->iparam1, count, 1.0, 0.1, 240, 255, min(64.0f+flDamage*2.0f, 200));

		if (RANDOM_LONG(0,1) == 0)
			EMIT_SOUND(-1, origin, CHAN_STATIC, "weapons/gauss_refl1.wav", VOL_NORM, ATTN_IDLE, 0, PITCH_NORM+RANDOM_LONG(0,5));
		else
			EMIT_SOUND(-1, origin, CHAN_STATIC, "weapons/gauss_refl2.wav", VOL_NORM, ATTN_IDLE, 0, PITCH_NORM+RANDOM_LONG(0,5));

		if (g_pCvarParticles->value > 1.0f)
		{
			color24 c = {255,255,255};//{240,255,200};
			FX_StreakSplash(origin, args->angles, c, 12, 128, true, true, false);
		}
	}
	else// explode
	{
		Vector dir, end;
		if (args->fparam2 > 0)// if fGoThrough, server sends exit point as 'angles' and direction otherwise
		{
			end = args->angles;
			VectorSubtract(end, origin, dir);
			dir.NormalizeSelf();
		}
		else
		{
			dir = args->angles;
			dir.NormalizeSelf();
			VectorMA(origin, 2.0f, dir, end);// move forward little more
		}
// TEST	gEngfuncs.pEfxAPI->R_TempSprite(origin, g_vecZero, 0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr"), kRenderGlow, kRenderFxNoDissipation, 1.0, 2.0, FTENT_FADEOUT);
//		gEngfuncs.pEfxAPI->R_TempSprite(end, g_vecZero, 0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr"), kRenderGlow, kRenderFxNoDissipation, 1.0, 2.0, FTENT_FADEOUT);
//		gEngfuncs.pEfxAPI->R_BeamPoints(origin, end, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), 3.0, 0.2, 0.0, 1.0, 0.0, 0, 1.0, 1.0,0.0,0.0);

		// entry effects: light, decal
		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);	
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		gEngfuncs.pEventAPI->EV_PlayerTrace(origin, end, PM_STUDIO_BOX, -1, &tr);
		DecalTrace(DECAL_GAUSSSHOT1, &tr, origin);
		Vector spot, endspot;
		//VectorCopy(tr.plane.normal, normal);
		VectorAdd(origin, tr.plane.normal, spot);
		VectorMA(origin, 2.0f, tr.plane.normal, endspot);

		if (!UTIL_PointIsFar(spot))
			DynamicLight(spot, scale*64, 255,255,255, 2.0f, 100.0f);

		if (!tr.allsolid && args->fparam2 > 0)// found empty space behind the wall
		{
			//conprintf(1, "  hit through wall\n");
			// entry effects: glow, balls
			gEngfuncs.pEfxAPI->R_TempSprite(spot, g_vecZero, scale*0.6f, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 0.9, 2.0, FTENT_FADEOUT);
			gEngfuncs.pEfxAPI->R_Sprite_Trail(0, spot, endspot, args->iparam1, count/2, 3.0, 0.1, 200, 255, flDamage*2.0f);

			// exit effects: glow, balls, decal
			VectorAdd(end, dir, spot);
			VectorMA(end, 2.0f, dir, endspot);
			gEngfuncs.pEventAPI->EV_PlayerTrace(endspot, origin, PM_STUDIO_BOX, -1, &tr);
			DecalTrace(DECAL_GAUSSSHOT1, &tr, endspot);
			//gEngfuncs.pEfxAPI->R_DecalShoot(decalindex, gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr), 0, tr.endpos, 0);
			gEngfuncs.pEfxAPI->R_TempSprite(spot, g_vecZero, scale*0.6f, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 0.9, 2.0, FTENT_FADEOUT);
			gEngfuncs.pEfxAPI->R_Sprite_Trail(0, spot, endspot, args->iparam1, (int)(count*0.75f), 3.0, 0.1, 200, 255, flDamage*2.0f);
			EMIT_SOUND(-1, end, CHAN_STATIC, "weapons/gauss_hit.wav", VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
			if (g_pCvarEffects->value > 1.0f)
				FX_SparkShower(endspot, g_pSpriteAnimGlow01, 4, args->fparam1*2.0f*VectorRandom(), true, 0.5f);
		}
		else// final hit/continue entry effect
		{
			//conprintf(1, "  final hit\n");
			gEngfuncs.pEfxAPI->R_TempSprite(spot, g_vecZero, scale, args->iparam1, kRenderGlow, kRenderFxNoDissipation, (firemode == FIRE_PRI?1.0f:(0.25f+flDamage*0.01f)), 5.0, FTENT_FADEOUT);
			gEngfuncs.pEfxAPI->R_Sprite_Trail(0, spot, endspot, args->iparam1, count, 3.0, 0.1, 100+flDamage, 255, 40.0f+flDamage*0.5f);
			/*if (!m_fPrimaryFire)
			{
				Vector angles;
				VectorAngles(-normal, angles);
				g_pRenderManager->AddSystem(new CRSSprite(spot, angles, 0.1, 0.2, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 255,255,255, 1.0, 0.5, 0.0), RENDERSYSTEM_FLAG_NOCLIP);
			}*/
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038b: black hole projectile
// entindex - entity
// origin - 
// angles - 
// fparam1 - damage
// fparam2 -
// iparam1 - beam sprite index
// iparam2 - particle sprite index
// bparam1 - 0 - start, 1 - detonate
// bparam2 - water
//-----------------------------------------------------------------------------
void EV_BlackBall(struct event_args_s *args)
{
	EV_START(args);
	if (args->bparam1 == 0)
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSRadialBeams(args->iparam1, args->origin, args->fparam1, 8, 0xFFFFFFFF, 0.5,1.0, 1,2, 0.4f, 1.0f, 0, FBEAM_SHADEOUT, 0.0f), RENDERSYSTEM_FLAG_NOCLIP, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);// RGBA2INT(255,255,255,255)

		//EMIT_SOUND(args->entindex, args->origin, CHAN_VOICE, "ambience/alienfazzle1.wav", 0.85f, ATTN_NORM, 0, PITCH_NORM); sound is OK, but it often hangs
	}
	else if (args->bparam1 == 1)
	{
		if (g_pRenderManager)
			g_pRenderManager->DeleteEntitySystems(args->entindex, true);

		//STOP_SOUND(args->entindex, CHAN_VOICE, "ambience/alienfazzle1.wav");
		if (args->fparam1 <= 0.0f)
		{
			FX_StreakSplash(args->origin, args->origin, 255,255,255, 32, 200.0f, false, false, false);
		}
		else
		{
			float radius = args->fparam1*4.0f;
			gEngfuncs.pEfxAPI->R_FunnelSprite(args->origin, args->iparam2, 0);
			if (g_pCvarParticles->value > 0.0f)
			{
				gEngfuncs.pEfxAPI->R_LargeFunnel(args->origin, 0);
				if (g_pRenderManager && g_pCvarParticles->value > 1.0f)
					g_pRenderManager->AddSystem(new CPSSparks((g_pCvarParticles->value > 1.0f)?192:64, args->origin, 1.8f,0.8f,-0.5f, -radius, 2.2f, 255,255,255,1.0f,-0.75f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 3.0f), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_SIMULTANEOUS);
			}
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSBeamStar(args->origin, args->iparam1>0?IEngineStudio.GetModelByIndex(args->iparam1):g_pSpriteDarkPortal, 96, kRenderTransAlpha, 255,255,255, 1.0f,-0.1f, radius,-100.0f, 6.0f), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_SIMULTANEOUS);

			if (g_pCvarEffects->value > 0.0f)
			{
				dlight_t *pLight = DynamicLight(args->origin, radius, 255,255,255, 1.5f, 200.0f);
				if (pLight)
					pLight->dark = 1;

				gEngfuncs.pEfxAPI->R_Implosion(args->origin, radius*0.75f, 192, 2.0f);
	//				g_pRenderManager->AddSystem(new CRSDisk(vecSpot, Vector(0,90,0), Vector(0,128,0), 128.0f, -32.0f, (g_pCvarEffects->value > 1.0)?16:8, args->iparam1, kRenderTransAdd, 255,255,255, 0.8, -0.25, 2.0));
	//just a test	g_pRenderManager->AddSystem(new CRSDisk(vecSpot, Vector(0,90,0), Vector(0,128,0), 128.0f, -32.0f, (g_pCvarEffects->value > 1.0)?16:8, dspr, kRenderTransAlpha, 255,255,255, 0.8, -0.25, 2.0));
			}
			if (g_pRenderManager)
				g_pRenderManager->ApplyForce(args->origin, Vector(-args->fparam1*0.001f, 0.0f, 0.0f), radius, true);

			EMIT_SOUND(args->entindex, args->origin, CHAN_VOICE, "ambience/particle_suck2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_LOW);
			EMIT_SOUND(args->entindex, args->origin, CHAN_STATIC, "weapons/blackball_blast.wav", VOL_NORM, 0.5, 0, PITCH_LOW);
			DecalTrace("{crack2", args->origin, Vector(0.0f,0.0f,-args->fparam1) + args->origin);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: BoltHit XDM3038
// entindex -
// origin - origin
// angles - angles
// fparam1 - damage
// fparam2 -
// iparam1 - decal index
// iparam2 - hit type: 0 flesh, 1 armor, 2 static BSP, 3 moving BSP | 0 normal explosion, 1 water explosion
// bparam1 - sound randomization index
// bparam2 - bolt type: 0 normal, 1 explosive
//-----------------------------------------------------------------------------
void EV_BoltHit(struct event_args_s *args)
{
	EV_START(args);
	//m_iFireball = PRECACHE_MODEL("sprites/xbow_exp.spr");
	//m_iSpark = PRECACHE_MODEL("sprites/xbow_ric.spr");
	Vector vecForward;
	AngleVectors(args->angles, vecForward, NULL, NULL);
	Vector vecEnd(vecForward); vecEnd *= 12.0f; vecEnd += args->origin;
	//STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_fly1.wav");
	if (args->bparam2 == BOLT_EV_HIT)// non-explosive
	{
		if (args->iparam2 == 0)// body
		{
			if (!UTIL_PointIsFar(args->origin))
			{
				switch (args->bparam1)
				{
				default:
				case 0: EMIT_SOUND(-1, args->origin, CHAN_BODY, "weapons/bullet_hit1.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105)); break;
				case 1: EMIT_SOUND(-1, args->origin, CHAN_BODY, "weapons/bullet_hit2.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105)); break;
				case 2: EMIT_SOUND(-1, args->origin, CHAN_BODY, "weapons/bullet_hit3.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105)); break;
				case 3: EMIT_SOUND(-1, args->origin, CHAN_BODY, "weapons/bullet_hit4.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105)); break;
				}
			}
			// blood is played on server
		}
		else if (args->iparam2 == 1)// armor
		{
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteRicho1, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,1.0f, 24.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

			if (g_pCvarParticles->value > 0.0f)
				FX_StreakSplash(args->origin, -vecForward, gTracerColors[5], RANDOM_LONG(4,8), 48, true, false, false);

			if (!UTIL_PointIsFar(args->origin))
				EMIT_SOUND(-1, args->origin, CHAN_STATIC, gSoundsRicochet[RANDOM_LONG(0,NUM_RICOCHET_SOUNDS-1)], RANDOM_FLOAT(0.75, VOL_NORM), ATTN_NORM, 0, PITCH_NORM);
		}
		else// if (args->iparam2 == 2) or 3 // solid world
		{
			if (!UTIL_PointIsFar(args->origin))
			{
				if (g_pRenderManager)
					g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteXbowRic, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 0.25f,0.0f, 24.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

				if (args->iparam2 == 3)// disintegration
				{
					if (g_pCvarEffects->value > 0.0f)
						gEngfuncs.pEfxAPI->R_SparkEffect(args->origin, RANDOM_LONG(12,16), -200, 200);
				}
				// save 2nd sound for bows EMIT_SOUND(-1, args->origin, CHAN_WEAPON, (args->bparam1 == 0)?"weapons/xbow_hit1.wav":"weapons/xbow_hit2.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
				EMIT_SOUND(-1, args->origin, CHAN_WEAPON, "weapons/xbow_hit1.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
			}
			DecalTrace(args->iparam1, args->origin, vecEnd);

			if (g_pCvarParticles->value > 0.0f)
				FX_StreakSplash(args->origin, -vecForward, gTracerColors[7], RANDOM_LONG(6,10), 56, true, false, false);
		}
		//else if (args->iparam2 == 3)
	}
	else// explosive
	{
		//int contents = gEngfuncs.PM_PointContents(args->origin, NULL);// XDM3038
		if (!UTIL_PointIsFar(args->origin))
		{
			if (g_pCvarEffects->value > 0)
				DynamicLight(args->origin, args->fparam1*1.5f, 255,223,191, 2.0, 144.0);

			EMIT_SOUND(-1, args->origin, CHAN_WEAPON, "weapons/xbow_exp.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96,104));
			if (args->iparam2 == 1)//if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME)
				FX_BubblesSphere(args->origin, args->fparam1*0.4f, args->fparam1*0.5f, g_pSpriteBubble, (int)(args->fparam1*0.6f), RANDOM_FLOAT(30,40));
		}
		if (g_pRenderManager)
		{
			if (args->iparam2 == 1)//if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME)
				g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteZeroWXplode, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,1.0f, 24.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
			else
				g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteXbowExp, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,1.0f, 24.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
		}
		DecalTrace(args->iparam1, args->origin, vecEnd);
		if (g_pCvarParticles->value > 0.0f)
			gEngfuncs.pEfxAPI->R_FlickerParticles(args->origin);
	}
}

//-----------------------------------------------------------------------------
// Purpose: BulletImpact
// entindex - world
// origin - tr.vecEndPos
// angles - tr.vecPlaneNormal
// fparam1 - tr.flFraction
// fparam2 - // (float)tex
// iparam1 - iBulletType
// iparam2 - XDM3037a: texture //spr
// bparam1 - pEntity->IsBSPModel()?1:0
// bparam2 - (pc > CONTENTS_WATER)?1:0
//-----------------------------------------------------------------------------
void EV_BulletImpact(struct event_args_s *args)
{
	// only for angles! EV_START(args);
	Vector src;
	Vector end;
	Vector origin(args->origin);
	Vector direction(args->angles);
	char tex = args->iparam2;//(char)args->fparam2;
	bool hithard = false;
	bool isfar = UTIL_PointIsFar(origin);
	if (tex == CHAR_TEX_CONCRETE ||
		tex == CHAR_TEX_METAL ||
		tex == CHAR_TEX_GRATE ||
		tex == CHAR_TEX_TILE ||
		tex == CHAR_TEX_COMPUTER)
	{
		hithard = true;
	}
	else if (tex == CHAR_TEX_NULL ||
			tex == CHAR_TEX_SKY)// XDM3038: should be prevented on server
		return;

	VectorMA(origin, 4.0f, direction, src);// 4 units backward
	VectorMA(origin, -4.0f, direction, end);// forward (into the brush)
/*
	int radius = 200;
	int iColor = RANDOM_LONG(20,35);
t	g_pRenderManager->AddSystem(new CRSDelayed(
e		new CRSSprite(origin, Vector(0,0,32), g_pSpriteZeroSteam, kRenderTransAlpha,
s		iColor,iColor,iColor, test1->value, test2->value, radius*0.012f,2.0f, 8.0f, 0.0f), 1.5f, RENDERSYSTEM_FLAG_NOCLIP));// fade out
t
	return;
*/
	if (args->bparam1 > 0)// IsBSPModel
	{
		switch (args->iparam1)// Draw special fx for each bullet type
		{
		case BULLET_357:
		case BULLET_12MM:
		case BULLET_338:
			{
				if (hithard && !isfar)
				{
					if (args->bparam2 > 0)// not in water
					{
						//FX_StreakSplash(origin, direction, gTracerColors[6], 24, 200);// better use engine effects if too often
						gEngfuncs.pEfxAPI->R_StreakSplash(origin, direction, 6, (g_pCvarEffects->value > 0.0f)?RANDOM_LONG(24,32):RANDOM_LONG(16,24), RANDOM_LONG(48, 56), -280, 280);
					}

					if (g_pCvarEffects->value > 0.0f)
						DynamicLight(origin, 32, 255,191,127, 0.5, 80);

					/*Vector vel;
					VectorSubtract(vecSrc, origin, vel);
					gEngfuncs.pEfxAPI->R_UserTracerParticle(origin, vel, 1.5, 2, 32, 0, NULL);*/
				}
				gEngfuncs.pEfxAPI->R_BulletImpactParticles(origin);
			}
			break;
		case BULLET_9MM:
		case BULLET_MP5:
			{
				if (hithard && !isfar)
				{
					if (g_pCvarEffects->value > 0.0f)
						gEngfuncs.pEfxAPI->R_SparkEffect(origin, RANDOM_LONG(12,16), -200, 200);
				}
				gEngfuncs.pEfxAPI->R_BulletImpactParticles(origin);
			}
			break;
		case BULLET_BUCKSHOT:
			{
				if (hithard && !isfar)
				{
					if (g_pCvarEffects->value > 0.0f)
						gEngfuncs.pEfxAPI->R_SparkEffect(origin, RANDOM_LONG(4,6), -180, 180);
				}
			}
			break;
		default: break;// nothing here!
		}

		// for all bullet types
		if (args->bparam2 > 0)// not in water
		{
				if (!isfar)// XDM3035c
				{
					if (g_pRenderManager && EV_ShouldDoSmoke(args->iparam1, tex))// draw real smoke
					{
						model_t *pSprite;
						if (args->iparam1 == BULLET_357 || args->iparam1 == BULLET_12MM || args->iparam1 == BULLET_338)
							pSprite = g_pSpriteWallPuff1;
						else
							pSprite = g_pSpriteWallPuff2;

						if (pSprite)
						{
							int iColor = RANDOM_LONG(20,35);
							g_pRenderManager->AddSystem(new CRSSprite(end+direction*2.0f, direction*4.0f, pSprite, kRenderTransAlpha, iColor,iColor,iColor,
								1.0f,0.0f, RANDOM_FLOAT(0.5f,0.6f),0.4f, RANDOM_FLOAT(24.0f, 30.0f), 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
						}
					}
					/*// XDM3035: less temporary entities!
					Vector smk;
					VectorCopy(origin, smk);
					smk[2] -= 16.0f;
					gEngfuncs.pEfxAPI->R_Sprite_Smoke(gEngfuncs.pEfxAPI->R_DefaultSprite(smk, args->iparam2, 25), RANDOM_FLOAT(0.6, 0.8));
					//(float *pos, float *dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags );
					//gEngfuncs.pEfxAPI->R_TempSprite(origin, direction, RANDOM_FLOAT(0.6, 0.8), args->iparam2, kRenderTransAlpha, kRenderFxNone, 0.8, 10.0, FTENT_SPRANIMATE);*/
					if (hithard)
					{
						int ns = NUM_RICOCHET_SOUNDS;// does not work?! ARRAYSIZE(*gSoundsRicochet);
						int si = RANDOM_LONG(0,ns+3);// add some invalid indexes
						if (si < ns)
							EMIT_SOUND(-1, origin, CHAN_STATIC, gSoundsRicochet[si], RANDOM_FLOAT(0.75, VOL_NORM), ATTN_NORM, 0, PITCH_NORM);
					}
				}
		}
		else
		{
			//FX_Bubbles(origin+Vector(-3,-3,-3), origin+Vector(3,3,3), Vector(0,0,1), g_pSpriteBubble, RANDOM_LONG(2,6), RANDOM_FLOAT(8,12));
			FX_BubblesPoint(origin, VECTOR_CONE_10DEGREES, g_pSpriteBubble, RANDOM_LONG(4,8)*g_pCvarParticles->value, RANDOM_FLOAT(30,40));
		}

/*		gEngfuncs.pEfxAPI->R_TempSprite(src, g_vecZero, 0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr"), kRenderGlow, kRenderFxNoDissipation, 1.0, 2.0, FTENT_FADEOUT);
TEST	gEngfuncs.pEfxAPI->R_TempSprite(end, g_vecZero, 0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr"), kRenderGlow, kRenderFxNoDissipation, 1.0, 2.0, FTENT_FADEOUT);
		gEngfuncs.pEfxAPI->R_BeamPoints(src, end, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), 3.0, 0.2, 0.0, 1.0, 0.0, 0, 1.0, 1.0,1.0,1.0);*/
		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
		gEngfuncs.pEventAPI->EV_PopPMStates();

		VectorCopy(origin, tr.endpos);// !! copy original origin into trace result
		int dindex = EV_DamageDecal(args->iparam1, tex);
		if (dindex >= 0)
			DecalTrace(dindex, &tr, origin);

		//EV_GunshotDecalTrace(&tr, EV_DamageDecal(args->iparam1, tex));// smoke and decal

		// STRESS TEST	g_pRenderManager->AddSystem(new CPSSparks(256, origin, 1.0f,0.05f, 0.25f, 320.0f, 2.2f, 255,255,255,1.0f,0.0f, g_iModelIndexPExp1, kRenderTransAdd, 10), RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_ADDPHYSICS, -1);

		/* fail
		Vector normal, angles, spot;
		VectorCopy(tr.plane.normal, normal);
		VectorAdd(tr.endpos, normal, spot);
		normal[0] *= test1->value;//-1.0f;
		normal[1] *= test2->value;//-1.0f;
		normal[2] *= test3->value;//-1.0f;
		VectorAngles(normal, angles);

		g_pRenderManager->AddSystem(new CRSDisk(spot, angles, Vector(0,0,0), 128.0f, -32.0f, 16, g_iModelIndexLaserBeam, kRenderTransAdd, 255,255,255, 0.8, -0.25, 2.0), RENDERSYSTEM_FLAG_NOCLIP);*/

		/* TEST Vector normal, zzz;
		VectorCopy(tr.plane.normal, normal);
		VectorAngles(-normal, zzz);
		g_pRenderManager->AddSystem(new CRenderSystem(args->origin, zzz, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr"), kRenderTransAdd, 255,255,255, 1.0,-1.0, 1.0,8.0, 0.0));*/

		//RenderDisk(args->origin, direction?, g_vecZero, 64.0, 16.0, test1->value, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), kRenderTransAdd, 255,255,255, 1.0, -1.0, 2.0, 0, 0);
		if (!isfar)
			PlayTextureSound(tex, origin, false);// (args->iparam1==BULLET_12MM)?true:false);??
	}
	else if (!isfar)// XDM3037: not BSP; also, no point playing sounds that are too far away
	{
		float volume = 1.0f;
		float attn = ATTN_NORM;
		switch (args->iparam1)
		{
		case BULLET_357:
		case BULLET_12MM:
			{
				volume = 1.0f;
				attn = ATTN_NORM;
			}
			break;
		case BULLET_9MM:
		case BULLET_MP5:
			{
				volume = 0.75f;
				attn = ATTN_STATIC;
			}
			break;
		case BULLET_BUCKSHOT:
			{
				volume = 0.6f;
				attn = ATTN_IDLE;
			}
			break;
		case BULLET_338:
			{
				volume = 1.0f;
				attn = 0.6f;
			}
			break;
		}
		if (RANDOM_LONG(0,1) == 0)
			EMIT_SOUND(0, args->origin, CHAN_STATIC, "weapons/bullet_hit1.wav", volume, attn, 0, RANDOM_LONG(95,105));
		else
			EMIT_SOUND(0, args->origin, CHAN_STATIC, "weapons/bullet_hit2.wav", volume, attn, 0, RANDOM_LONG(95,105));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Generic explosin event
// entindex - 
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - water
//-----------------------------------------------------------------------------
void EV_Explosion(struct event_args_s *args)
{
	EV_START(args);
	Vector org(args->origin);
	//VectorCopy(args->origin, org);
	int spawnflags = (int)args->fparam2;
	int waterlevel = args->bparam2;
	float dmg = args->fparam1;
	float radius = dmg * GREN_DAMAGE_TO_RADIUS;// must match value on server

	/*if (!(spawnflags & SF_NODAMAGE))
	{
		UNDONE: currently it is impossible to add screen shake effect from client DLL
		V_CalcShake, V_ApplyShake are used to GET, but not SET shake info.
	}*/
	if (waterlevel > WATERLEVEL_NONE)
	{
		if (!(spawnflags & SF_NOPARTICLES))
		{
			if (g_pCvarParticles->value > 0.0f && !UTIL_PointIsFar(org))// XDM3035c
				ParticlesCustom(org, dmg * 0.6f, 160, 16, 128, 0.2f);
		}
		if (!(spawnflags & SF_NOSMOKE))
		{
			// since explosions mostly happen at some surface, push the volume a little bit upwards?
			//Vector half(radius*0.3f, radius*0.3f, radius*0.3f);
			//Vector mins = org - half;
			//Vector maxs = org + half;
			//FX_Bubbles(org-half, org+half, g_vecZero, g_pSpriteBubble, (int)(dmg*0.75f), RANDOM_FLOAT(30,40));
			FX_BubblesSphere(org, radius*0.3f, radius*0.5f, g_pSpriteBubble, (int)(dmg*0.75f), RANDOM_FLOAT(30,40));

			/*
			Vector mins = org - Vector(1,1,1)*(radius*0.3f);
			Vector maxs = org + Vector(1,1,1)*(radius*0.3f);
			float flHeight = UTIL_WaterLevel(org, org.z, org.z + 1024.0f);// works fine
			//maxs.z = min(maxs.z, flHeight);// maxs.z cannot be higher than the water level
			flHeight = flHeight - mins.z;//org.z;
//DEBUG OK
//			gEngfuncs.pEfxAPI->R_ParticleBox(mins, maxs, 255,0,0, 4.0f);
//			gEngfuncs.pEfxAPI->R_ShowLine(org, Vector(org.x, org.y, mins.z) + Vector(0.0f,0.0f,flHeight));
			gEngfuncs.pEfxAPI->R_Bubbles(mins, maxs, flHeight, g_pSpriteBubble, (int)dmg, dmg*0.5f);// WTF?!
			*/
			//maxs.z = flHeight + mins.z;
		}
	}
	else
	{
		// Add smoke first, it may have something to do with draw order
		if (!FBitSet(spawnflags, SF_NOSMOKE))// delayed double smoke
		{
			if (g_pRenderManager)
			{
				byte gray = (byte)RANDOM_LONG(20,35);
				g_pRenderManager->AddSystem(new CRSDelayed(new CRSSprite(org, Vector(0,0,20), g_pSpriteZeroSteam, kRenderTransAlpha, gray,gray,gray, 0.0f,0.0f/*useless*/, radius*0.01f,1.5f, 16.0f, 0.0f), 0.1f, RENDERSYSTEM_FLAG_NOCLIP));// fade in
				/*UNDONE if (g_pCvarParticles->value > 1.0f)
				{
					CPSFlameCone *pSys = new CPSFlameCone(32, org, org, Vector(2.0,2.0,2.0), 120.0f, g_pSpriteZeroSteam, kRenderTransAlpha, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f);
					if (pSys)
					{
						pSys->m_color.r = gray;
						pSys->m_color.g = gray;
						pSys->m_color.b = gray;
						pSys->m_color.a = 223;
						pSys->m_fColorDelta[3] = 0.1f;
						pSys->m_fScale = 2.0f;
						pSys->m_fScaleDelta = 56.0f;
						g_pRenderManager->AddSystem(new CRSDelayed(pSys, 0.7f), 0, -1);
					}
				}*/
				//test	g_pRenderManager->AddSystem(new CRSSprite(org, Vector(0,0,32), g_pSpriteZeroSteam, kRenderTransAlpha, iColor,iColor,iColor, 1.0f,-2.0f, radius*0.012f,2.0f, 8.0f, 0.0f));
				gray = (byte)RANDOM_LONG(20,35);
				g_pRenderManager->AddSystem(new CRSDelayed(
					new CRSSprite(org, Vector(0,0,32), g_pSpriteZeroSteam, kRenderTransAlpha, gray,gray,gray, 1.0f,0.0f/*useless*/, radius*0.012f,2.0f, 10.0f, 0.0f), 0.5f, RENDERSYSTEM_FLAG_NOCLIP));// fade out

				if (g_pCvarEffects->value > 1.0f)
					g_pRenderManager->AddSystem(new CRSDelayed(new CRSSprite(org+Vector(0,0,8), Vector(0,0,48), g_pSpriteWallPuff1, kRenderTransAlpha, gray,gray,gray, 1.0f,-0.5f, 4.0f, 1.0f, 10.0f, 0.0f), 0.6f, RENDERSYSTEM_FLAG_NOCLIP));
			}
		}
		if (!(spawnflags & SF_NOFIREBALL) && args->iparam1 > 0)// glow
		{
			TEMPENTITY *pSprite = FX_TempSprite(org, min(dmg * 0.02f, 1.0f), /*IEngineStudio.GetModelByIndex(args->iparam1)*/g_pSpriteZeroGlow, kRenderGlow, kRenderFxNoDissipation, 1.0, 0.1, FTENT_FADEOUT);
			if (pSprite)
			{
				pSprite->frameMax = 0;
				pSprite->entity.baseline.framerate = 0;
				pSprite->entity.curstate.framerate = 0;
			}
			if (!(spawnflags & SF_NODAMAGE) && (g_pCvarEffects->value > 1.0f))// XDM3033: apply force
			{
				if (g_pRenderManager)
					g_pRenderManager->ApplyForce(org, Vector(dmg*0.1f,0.0f,0.0f), radius, true);
			}
		}
		if (!(spawnflags & SF_NOPARTICLES) && args->iparam2 > 0)// particles
		{
			ParticlesCustom(org, dmg*2.0f, 192, 8, 192, 0.5f);
			if (g_pRenderManager && g_pCvarParticles->value > 0.0f)
			{
				CPSFlameCone *pSys = new CPSFlameCone((int)(dmg*0.64f)/*80*/, org, org, g_vecZero, dmg*2.5f/*300.0f*/, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 1.0f,-1.1f, 1.0f,dmg*0.25f, 0.5f);
				if (pSys)
				{
					pSys->m_fParticleSpeedMin = dmg*2.5f;//280.0f;
					pSys->m_fParticleSpeedMax = dmg*3.0f;//320.0f;
					g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);// XDM3038a: adelta
				}
			}
		}
		if (!(spawnflags & SF_NOSPARKS))
		{
			if (g_pCvarEffects->value > 0)
				FX_SparkShower(args->origin, g_pSpriteZeroFlare, (g_pCvarEffects->value > 1)?16:8, 6.0f*VectorRandom()*dmg, true, 1.25f);
		}
		if (spawnflags & SF_NUCLEAR)
		{
			org[2] += 80.0f;
			ParticlesCustom(org, dmg, 192, 8, 256, 2.0f);
			//gEngfuncs.pEfxAPI->R_TempSprite(org, g_vecZero, 20.0f, args->iparam1, kRenderTransAdd, kRenderFxNone, 1.0f, 2.0f, FTENT_SPRANIMATE);
			gEngfuncs.pEfxAPI->R_FunnelSprite(org, args->iparam2, 1);

			if (g_pRenderManager)
			{
				g_pRenderManager->AddSystem(new CRSSprite(org, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam1), kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,2.0f, 16.0f, 2.0f), RENDERSYSTEM_FLAG_NOCLIP);// XDM3035: eventually all sprites should be drawn as rendersystems
				g_pRenderManager->AddSystem(new CPSFlameCone(128, org, org, Vector(4.0,4.0,4.0), 400.0f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 1.0f, -1.5f, 1.0f, 50.0f, 3.0f), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

				if (g_pCvarEffects->value > 0.0f && g_pCvarParticles->value > 0.0f)
					g_pRenderManager->AddSystem(new CPSSparks(160, org, 2.0f, 0.5f, 1.0f, 300.0f, 2.2f, 255,255,255,1.0f,-0.2f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 4.0f), RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, -1);
			}
		}

		if (!UTIL_PointIsFar(org))// XDM3035c
		{
			switch (args->bparam1)
			{
			default:
			case 0: EMIT_SOUND(-1, org, CHAN_VOICE, "weapons/debris1.wav", 0.6, ATTN_NORM, 0, RANDOM_LONG(95,105)); break;
			case 1: EMIT_SOUND(-1, org, CHAN_VOICE, "weapons/debris2.wav", 0.6, ATTN_NORM, 0, RANDOM_LONG(95,105)); break;
			case 2: EMIT_SOUND(-1, org, CHAN_VOICE, "weapons/debris3.wav", 0.6, ATTN_NORM, 0, RANDOM_LONG(95,105)); break;
			}
		}
	}

	if (!(spawnflags & SF_NODECAL))
	{
		Vector forward;
		AngleVectors(args->angles, forward, NULL, NULL);
		Vector end(forward); end *= radius*0.4f; end += org;
		//UTIL_DebugPoint(org, 8.0f, 255,127,0);
		//UTIL_DebugBeam(org, end, 8.0f, 255,127,0);
		if (spawnflags & SF_NUCLEAR)
			DecalTrace(RANDOM_LONG(DECAL_NUCBLOW1, DECAL_NUCBLOW3), org, end);
		else
			DecalTrace(RANDOM_LONG(DECAL_SCORCH1, DECAL_SCORCH3), org, end);
	}
}


bool OnParticleInitializeFlame(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &fInterpolaitonMult)
{
	if (pSystem->m_pLastFollowedEntity)
	{
		if (pSystem->m_pLastFollowedEntity->model == NULL || (pSystem->m_pLastFollowedEntity->model->mins.IsZero() && pSystem->m_pLastFollowedEntity->model->maxs.IsZero()))
		{
			VectorRandom(pParticle->m_vPos, VEC_HULL_MIN, VEC_HULL_MAX);
			pParticle->m_vPos += pSystem->m_pLastFollowedEntity->origin;
		}
	}
	if (RANDOM_LONG(0,2) == 0)
		pParticle->m_fSizeDelta = -8.0f;
	else
		pParticle->m_fSizeDelta = RANDOM_FLOAT(20,40);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Entity is burning
// entindex - XDM3037: entindex is now a damaged entity
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - Fire sprite
// iparam2 - Smoke sprite
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_Flame(struct event_args_s *args)
{
	EV_START(args);
	cl_entity_t *pOther = gEngfuncs.GetEntityByIndex(args->entindex);

	if (g_pCvarEffects->value > 1)
		DynamicLight(args->origin, 128, 255,191,127, args->fparam1, 64.0);

	EMIT_SOUND(args->entindex, args->origin, CHAN_STATIC, "weapons/flame_burn.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(PITCH_NORM,PITCH_NORM+10));

	if (args->bparam1 > 0)
	{
		//gEngfuncs.pEfxAPI->R_Sprite_Smoke(gEngfuncs.pEfxAPI->R_DefaultSprite(args->origin, args->iparam2, 8), RANDOM_FLOAT(0.6f, 1.0f));
		if (g_pRenderManager)
		{
			if (args->iparam1)// fire
			{
				CParticleSystem *pParticleSystem = new CParticleSystem(32, args->origin, Vector(0,0,1), IEngineStudio.GetModelByIndex(args->iparam1), kRenderTransAdd, 1.0f);
				if (pParticleSystem)
				{
					pParticleSystem->m_fScale = 1.0f;
					pParticleSystem->m_fScaleDelta = 0;//test1->value;// -2.0f;
					pParticleSystem->m_fColorDelta[1] = -1.0f;// decrease green
					pParticleSystem->m_fColorDelta[3] = -2.0f;
					pParticleSystem->m_fFrameRate = 12.0f;
					pParticleSystem->m_iStartType = PSSTARTTYPE_ENTITYBBOX;
					pParticleSystem->m_fParticleSpeedMin = 6.0f;
					pParticleSystem->m_fParticleSpeedMax = 8.0f;
					pParticleSystem->m_OnParticleInitialize = OnParticleInitializeFlame;
					//pParticleSystem->m_iFollowAttachment = 0;
					g_pRenderManager->AddSystem(pParticleSystem, RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
				}
			}
			int iColor = RANDOM_LONG(20,35);// smoke
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, Vector(0,0,12), IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAlpha, iColor,iColor,iColor, 1.0f,-4.0f, RANDOM_FLOAT(0.75, 0.8),2.0f, 16.0f, 0.0f), 0, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
		}
		//PartSystem(args->origin, dir, Vector(1.0,1.0,1.0), ent->curstate.modelindex, kRenderTransAdd, PARTSYSTEM_TYPE_FLAMECONE, 16, args->fparam1*0.05f, RENDERSYSTEM_FLAG_CLIPREMOVE);

		//if (pOther && IsValidPlayer(pOther))// this
		// this would be awesome if not the TransAlpha render mode
		//	gEngfuncs.pEfxAPI->R_PlayerSprites(pOther, args->iparam1, 16, 8);
	}

	if (pOther && args->iparam1)// victim
	{
		if (g_pCvarEffects->value > 0)
		{
			//if (cl_test1->value > 0)
			//{
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam1), kRenderTransAdd,
				255,255,255, 1.0f,-1.0f, RANDOM_FLOAT(0.1, 0.5),-1.0f, 16.0f, 0.0f), 0, args->entindex, RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET);// XDM3038a
			/*}
			else
			{
			TEMPENTITY *pSprite = gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, RANDOM_FLOAT(0.2, 0.25), args->iparam1, kRenderTransAdd, kRenderFxNone, 0.5f, args->fparam1, FTENT_SPRANIMATE|FTENT_PLYRATTACHMENT);// FTENT_SMOKETRAIL| looks very bad and reveals respawn points
			if (pSprite)
			{
				pSprite->clientIndex = args->entindex;
				pSprite->entity.baseline.framerate = pSprite->entity.curstate.framerate = RANDOM_FLOAT(12, 16);
				pSprite->entity.baseline.usehull = pSprite->entity.curstate.usehull = 0;
			}
			}*/
		}
	}
	//if (g_pCvarParticles->value > 0)
	//	PartSystem(args->origin, dir, Vector(1.0,1.0,1.0), ent->curstate.modelindex, kRenderTransAdd, PARTSYSTEM_TYPE_FLAMECONE, 16, args->fparam1*0.05f, RENDERSYSTEM_FLAG_CLIPREMOVE);
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - 
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_FlameTrail(struct event_args_s *args)
{
	EV_START(args);
	//Vector vecEnd;
	/*FEV_RELIABLE
	Vector velocity;
	VectorCopy(args->origin, vecSrc);
	VectorCopy(args->angles, velocity);
	VectorMA(vecSrc, 24.0f, velocity, vecEnd);*/

	BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamFollow(args->entindex, g_iModelIndexZeroFire, 0.2f, RANDOM_FLOAT(8.0f,10.0f), 1.0f,1.0f,1.0f, 0.5f);
	if (pBeam)
		pBeam->speed = 10.0f;
	/*Vector angles;
	Vector forward;
	//Vector up;
	VectorCopy(args->angles, angles);
	AngleVectors(angles, forward, NULL, NULL);

	if (g_pCvarParticles->value > 0.0f)
	{
		if (g_pRenderManager)
		{
			CPSFlameCone *pSys = new CPSFlameCone(32, args->origin, forward, VECTOR_CONE_15DEGREES, 320.0f, g_iModelIndexFlameFire, kRenderTransAdd, 1.0f, 50.0f, 0.5f);
			if (pSys)
			{
				pSys->m_color.r = 80;// start blue
				pSys->m_color.g = 72;
				pSys->m_color.b = 255;
				pSys->m_fColorDelta[0] = 1.0f;
				pSys->m_fColorDelta[1] = 0.6f;
				pSys->m_fColorDelta[2] = -1.5f;
				//pSys->m_fColorDelta[3] = -1.0f;
				g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_SIMULTANEOUS, args->entindex);
			}
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - 
// origin - origin
// angles - angles
// fparam1 - scale
// fparam2 - framerate
// iparam1 - flash sprite index
// iparam2 - smoke sprite index
// bparam1 - firemode (0,1)
// bparam2 -
//-----------------------------------------------------------------------------
void EV_FuncTankFire(struct event_args_s *args)
{
	EV_START(args);
	if (g_pRenderManager)
	{
		if (args->iparam1 > 0)
			FX_MuzzleFlashSprite(args->origin, -1, 0, IEngineStudio.GetModelByIndex(args->iparam1), args->fparam1*0.5f, FBitSet(GetGameFlags(), GAME_FLAG_OVERDRIVE));

		if (args->iparam2 > 0)
			FX_Smoke(args->origin, args->iparam2, args->fparam1, args->fparam2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: hand grenade explosion
// entindex - 
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - waterlevel
//-----------------------------------------------------------------------------
void EV_GrenExp(struct event_args_s *args)
{
	EV_START(args);
	Vector vecSpot(args->origin);
	int idx = args->entindex;
	int waterlevel = args->bparam2;
	//VectorCopy(args->origin, vecSpot);
	vecSpot[2] += 24.0f;
	float dmg = args->fparam1;
	switch (args->bparam1)// type
	{
	default:
		{
			conprintf(1, "EV_GrenExp: unknown grenade type %d!\n", args->bparam1);
		}
		break;
	case GREN_EXPLOSIVE:
		{
			float scale = args->fparam2;
			int expflags = TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND;
			if (waterlevel > WATERLEVEL_NONE)
				expflags |= TE_EXPLFLAG_NOPARTICLES;

			model_t *m_pTexture = IEngineStudio.GetModelByIndex(args->iparam1);
			if (m_pTexture)
			{
				float fps;
				if (args->iparam2 == 0)
					fps = 0.75f*(float)m_pTexture->numframes;// play 1.25 seconds
				else
					fps = 0.1f*(float)args->iparam2;// XDM3038a

				gEngfuncs.pEfxAPI->R_Explosion(args->origin, args->iparam1, scale, fps, expflags);
			}
			else
				conprintf(1, "EV_GrenExp: bad sprite index %d!\n", args->iparam1);

			DynamicLight(args->origin, dmg*2.5f, 255,207,127, 1.0f, 300.0f);

			if (g_pRenderManager)
			{
				if (g_pCvarParticles->value > 0.0f)
					g_pRenderManager->AddSystem(new CPSSparks((g_pCvarParticles->value > 0.0f)?128:96, args->origin, 1.0f,0.05f, 0.25f, (waterlevel > WATERLEVEL_NONE)?dmg:(dmg*2.0f), 2.2f, 255,255,255,1.0f,-0.5f, (waterlevel > WATERLEVEL_NONE)?g_pSpritePExp2:g_pSpritePExp1, kRenderTransAdd, 2.5f), RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_ADDPHYSICS|((waterlevel > WATERLEVEL_NONE)?0:RENDERSYSTEM_FLAG_ADDGRAVITY), -1);

				//RenderCylinder(args->origin, dmg*0.25f, dmg*2.0f, dmg*0.5f, 32, args->iparam2, kRenderTransAdd, 255,255,255, 0.6, -0.8, 1.0, 0, -1);
				g_pRenderManager->AddSystem(new CRSCylinder(args->origin, dmg*0.25f, dmg*2.5f, dmg*0.5f, 32, g_pSpriteZeroFire, kRenderTransAdd, 255,255,255, 0.6, -0.8, 1.0), RENDERSYSTEM_FLAG_LOOPFRAMES);
			}
		}
		break;
	case GREN_FREEZE:
		{
			if (g_pCvarEffects->value > 0.0f)
			{
				DynamicLight(args->origin, 400, 95,159,255, 2.0, 300.0f);
				if (g_pCvarEffects->value > 1.0f)
				{
					if (!UTIL_PointIsFar(args->origin))// XDM3035c
						ParticlesCustom(args->origin, 320, 208, 8, 160, 1.0f, true);

					if (g_pRenderManager && g_pCvarParticles->value > 0.0f)
						g_pRenderManager->AddSystem(new CPSSparks(64, args->origin, 2.0f,0.2f, 0.75f, (waterlevel > WATERLEVEL_NONE)?56.0f:80.0f, 2.0f, 220,220,255,1.0f,-1.0f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 1.5f), RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_CLIPREMOVE, -1);
				}
			}
			//if (!FBitSet(args->, SF_NOFIREBALL))
				gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, 1.0, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 1.0f, 0.5f, FTENT_FADEOUT);

			if (!UTIL_PointIsFar(args->origin))// XDM3035c
				gEngfuncs.pEfxAPI->R_Sprite_Trail(0, args->origin, vecSpot, args->iparam2, 40, 0.0, 0.2f, 480, 200, (waterlevel > WATERLEVEL_NONE)?200.0f:400.0f);

			if (waterlevel == WATERLEVEL_NONE)
				EMIT_SOUND(idx, vecSpot, CHAN_VOICE, "weapons/fgrenade_exp1.wav", VOL_NORM, ATTN_NORM, 0, 98 + RANDOM_LONG(0,4));
			else
				EMIT_SOUND(idx, vecSpot, CHAN_VOICE, "weapons/fgrenade_exp2.wav", VOL_NORM, ATTN_NORM, 0, 98 + RANDOM_LONG(0,4));

			DecalTrace(DECAL_SPLAT8, args->origin, Vector(0.0f,0.0f,-dmg) + args->origin);// {mommablob?
		}
		break;
	case GREN_POISON:// XDM3038b: iparam1 - pev->spawnflags, iparam2 - 1/0 explode/smell, bparam1 - GREN_POISON
		{
			if (args->iparam2 == 0)
			{
				if (!FBitSet(args->iparam1, SF_NOFIREBALL))
				{
					if (g_pRenderManager)
						g_pRenderManager->AddSystem(new CRSSprite(vecSpot, g_vecZero, (waterlevel > WATERLEVEL_NONE)?g_pSpriteAcidPuff1:g_pSpriteAcidPuff2, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,2.0f, 16.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
				}
				if (!FBitSet(args->iparam1, SF_NOSOUND))
				{
					EMIT_SOUND(idx, vecSpot, CHAN_VOICE, "weapons/pgrenade_acid.wav", VOL_NORM, ATTN_NORM, 0, 98 + RANDOM_LONG(0,4));
				}
			}
			else// explode
			{
				if (g_pCvarEffects->value > 1.0f)
					DynamicLight(args->origin, 460, 63,255,63, args->fparam2, 240.0);

				if (g_pRenderManager)
				{
					if (!FBitSet(args->iparam1, SF_NOFIREBALL))
						g_pRenderManager->AddSystem(new CRSSprite(vecSpot, g_vecZero, g_pSpriteAcidPuff3, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,2.0f, 16.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

					if (g_pCvarParticles->value > 0.0f)
					{
						// OLD g_pRenderManager->AddSystem(new CPSFlameCone((g_pCvarParticles->value > 1.0f)?160:80, args->origin, args->origin, Vector(2.0,2.0,2.0), 280.0f, g_pSpriteAcidDrip, kRenderTransAdd, 1.0f,-1.5f, 1.0f,50.0f, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
						CParticleSystem *pSystem = new CParticleSystem((g_pCvarParticles->value > 1.0f)?160:128, args->origin, args->origin, g_pSpriteAcidDrip, kRenderTransAdd, 2.0f);
						if (pSystem)
						{
							ASSERT(pSystem->IsRemoving() == false);
							pSystem->m_iTraceFlags = PM_STUDIO_IGNORE;
							pSystem->m_iEmitRate = 0;
							pSystem->m_fParticleScaleMin = 0.25f;
							pSystem->m_fParticleScaleMax = 0.5f;
							pSystem->m_fParticleSpeedMin = 128;
							pSystem->m_fParticleSpeedMax = 224;
							pSystem->m_fParticleWeight = 0;
							pSystem->m_iStartType = PSSTARTTYPE_POINT;
							pSystem->m_iMovementType = PSMOVTYPE_OUTWARDS;
							pSystem->m_fScale = 0.1f;
							pSystem->m_fScaleDelta = 80.0f;
							pSystem->m_fFrameRate = 30.0f;
							pSystem->m_color.Set(255,255,255,255);
							pSystem->m_fColorDelta[3] = -1.0f;
							pSystem->m_fBounceFactor = 0.0f;
							pSystem->m_fFriction = 0.0f;
							g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, args->entindex, RENDERSYSTEM_FFLAG_NOANGLES|RENDERSYSTEM_FFLAG_DONTFOLLOW);
						}
					}
				}
				//if (g_pCvarParticles->value > 0.0f)
					gEngfuncs.pEfxAPI->R_Sprite_Trail(0, args->origin, vecSpot, g_iModelIndexAcidDrip, (g_pCvarEffects->value > 1.0f)?64:40, 0.0, 0.3, 400, 255, (waterlevel > WATERLEVEL_NONE)?100.0f:200.0f);

				if (!FBitSet(args->iparam1, SF_NOSOUND))
					EMIT_SOUND(idx, vecSpot, CHAN_VOICE, (waterlevel == WATERLEVEL_NONE)?"weapons/pgrenade_exp1.wav":"weapons/pgrenade_exp2.wav", VOL_NORM, ATTN_NORM, 0, 98 + RANDOM_LONG(0,4));

				if (g_pRenderManager)// must be visible! && g_pCvarParticles->value > 0.0f)
				{
					CParticleSystem *pSystem = new CParticleSystem(8+(int)(args->fparam1*max(0.5f, g_pCvarParticles->value)), args->origin, args->origin, g_pSpriteAcidDrip, kRenderTransAdd, args->fparam2);
					if (pSystem)
					{
						ASSERT(pSystem->IsRemoving() == false);
						pSystem->m_iTraceFlags = PM_STUDIO_IGNORE;
						pSystem->m_iEmitRate = 0;
						pSystem->m_fParticleScaleMin = 2.0f;
						pSystem->m_fParticleScaleMax = 4.0f;
						pSystem->m_fParticleSpeedMin = 80;
						pSystem->m_fParticleSpeedMax = 256;
						pSystem->m_fParticleWeight = 0;
						pSystem->m_iStartType = PSSTARTTYPE_POINT;
						pSystem->m_iMovementType = PSMOVTYPE_OUTWARDS;
						pSystem->m_fScale = 1.0f;
						pSystem->m_fScaleDelta = 20.0f;
						pSystem->m_fFrameRate = 20.0f;
						pSystem->m_color.Set(255,255,255,191);
						pSystem->m_fColorDelta[3] = -0.125f;
						pSystem->m_fBounceFactor = -1.0f;
						pSystem->m_fFriction = 0.0f;
						g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, args->entindex, RENDERSYSTEM_FFLAG_NOANGLES|RENDERSYSTEM_FFLAG_DONTFOLLOW);
					}
					/*CPSFlameCone *pPS = new CPSFlameCone((g_pCvarParticles->value > 1.0f)?256:128, args->origin, args->origin, Vector(4.0,4.0,4.0), (waterlevel > WATERLEVEL_NONE)?320:480.0f, g_pSpriteAcidDrip, kRenderTransAdd, 0.85f,-1.0f, 10.0f,20.0f, args->fparam2);
					if (pPS)
					{
						pPS->m_fParticleSpeedMin = (waterlevel > WATERLEVEL_NONE)?300:400.0f;
						pPS->m_fParticleSpeedMax = (waterlevel > WATERLEVEL_NONE)?320:480.0f;
						g_pRenderManager->AddSystem(pPS, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDPHYSICS, -1);// XDM3035b
					}*/
				}
				DecalTrace(DECAL_BIGSPLATG1, args->origin, Vector(0.0f,0.0f,-dmg) + args->origin);
			}
		}
		break;
	case GREN_NAPALM:
		{
			if (g_pRenderManager)
			{
				g_pRenderManager->AddSystem(new CRSSprite(vecSpot, Vector(0,0,4), IEngineStudio.GetModelByIndex(args->iparam1), kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,0.0f, 20.0f, args->fparam2), 0, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);

				if (g_pCvarEffects->value > 0.0f)
					g_pRenderManager->AddSystem(new CRSSprite(vecSpot, Vector(0,0,2), IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 255,255,255, 0.85f,-0.05f, 1.5f,0.0f, 24.0f, args->fparam2), 0, args->entindex, RENDERSYSTEM_FFLAG_ICNF_NODRAW);

				if (g_pCvarParticles->value > 0.0f)
				{
					g_pRenderManager->AddSystem(new CPSFlameCone((g_pCvarEffects->value > 0.0f)?400:320, args->origin, args->origin, Vector(2,2,2), 300.0f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 1.0f, -1.5f, 1.0f, 50.0f, args->fparam2), RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_NODRAW);

					if (g_pCvarParticles->value > 1.0f)
						g_pRenderManager->AddSystem(new CPSFlameCone((g_pCvarEffects->value > 0.0f)?320:200, args->origin, args->origin, Vector(2,2,2), 400.0f, g_pSpritePExp1, kRenderTransAdd, 1.0f, -1.5f, 0.5f, 2.0f, args->fparam2), RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_ADDGRAVITY, args->entindex, RENDERSYSTEM_FFLAG_ICNF_NODRAW);
						//g_pRenderManager->AddSystem(new CPSSparks((g_pCvarEffects->value > 0.0f)?128:80, args->origin, 2.0f,0.2f, 0.75f, 64.0f, 2.0f, 255,240,220,1.0f,-1.0f, g_iModelIndexPExp1, kRenderTransAdd, args->fparam2), RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
				}
			}
			if (g_pCvarEffects->value > 1.0f)// extra effects
			{
				BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamFollow(args->entindex, g_iModelIndexZeroFire, 0.6f, 14, 1.0f,1.0f,1.0f, 0.5f);
				if (pBeam)
					pBeam->speed = 10.0f;

				//FX_SparkShower(args->origin, IEngineStudio.GetModelByIndex(args->iparam1), 6, dmg, 1.0f);
			}
			DecalTrace(RANDOM_LONG(DECAL_MDSCORCH1, DECAL_MDSCORCH3), args->origin, Vector(0.0f,0.0f,-dmg) + args->origin);
		}
		break;
	case GREN_RADIOACTIVE:
		{
			if (g_pCvarEffects->value > 0.0f)
				DynamicLight(args->origin, dmg*5.0f, 159,159,255, 1.5, 360.0f);

			gEngfuncs.pEfxAPI->R_Sprite_Trail(0, args->origin, vecSpot, args->iparam2, 40, 0.0f, 0.3f, 600, 255, (waterlevel > WATERLEVEL_NONE)?200.0f:420.0f);
			if (g_pRenderManager && g_pCvarParticles->value > 0.0f && !UTIL_PointIsFar(args->origin))// XDM3035c
				g_pRenderManager->AddSystem(new CPSSparks(96, args->origin, 2.0f,0.1f, 0.5f, 200.0f, 2.5f, 240,200,255,1.0f,-0.75f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 2.5f), RENDERSYSTEM_FLAG_NOCLIP/* | RENDERSYSTEM_FLAG_SIMULTANEOUS*/, -1);

			DecalTrace(DECAL_SPLAT7, args->origin, Vector(0.0f,0.0f,-dmg) + args->origin);
			EMIT_SOUND(idx, vecSpot, CHAN_VOICE, "ambience/port_suckin1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}
		break;
	case GREN_TELEPORTER:// OBSOLETE
		{
			float radius = dmg*4.0f;//480.0f;
			gEngfuncs.pEfxAPI->R_FunnelSprite(args->origin, args->iparam2, 0);
			//gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, 8.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke_loop.spr"), kRenderGlow, kRenderFxNoDissipation, 1.0, 0.4, FTENT_SPRANIMATE|FTENT_FADEOUT);
			if (g_pCvarParticles->value > 0.0f)
			{
				gEngfuncs.pEfxAPI->R_LargeFunnel(args->origin, 0);
				if (g_pRenderManager && g_pCvarParticles->value > 1.0f)
					g_pRenderManager->AddSystem(new CPSSparks((g_pCvarParticles->value > 1.0f)?192:64, vecSpot, 1.8f,0.8f,-0.5f, -radius, 2.2f, 255,255,255,1.0f,-0.75f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 3.0f), RENDERSYSTEM_FLAG_SIMULTANEOUS | RENDERSYSTEM_FLAG_NOCLIP);
			}
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSBeamStar(args->origin, args->iparam1>0?IEngineStudio.GetModelByIndex(args->iparam1):g_pSpriteDarkPortal, 96, kRenderTransAlpha, 255,255,255, 1.0f,-0.1f, radius,-128.0f, 6.0f));

			if (g_pCvarEffects->value > 0.0f)
			{
				//int dspr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/darkportal.spr");
				dlight_t *pLight = DynamicLight(args->origin, radius, 63,255,127, 1.5f, 200.0f);
				if (pLight)
					pLight->dark = 1;

				gEngfuncs.pEfxAPI->R_Implosion(args->origin, radius*0.75f, 192, 2.0f);
				//g_pRenderManager->AddSystem(new CRSDisk(vecSpot, Vector(0,90,0), Vector(0,128,0), 128.0f, -32.0f, (g_pCvarEffects->value > 1.0)?16:8, args->iparam1, kRenderTransAdd, 255,255,255, 0.8, -0.25, 2.0));
				//just a test	g_pRenderManager->AddSystem(new CRSDisk(vecSpot, Vector(0,90,0), Vector(0,128,0), 128.0f, -32.0f, (g_pCvarEffects->value > 1.0)?16:8, dspr, kRenderTransAlpha, 255,255,255, 0.8, -0.25, 2.0));
			}
			//if (g_pCvarEffects->value > 1.0f)// XDM3033: apply force
			//{
				if (g_pRenderManager)
					g_pRenderManager->ApplyForce(vecSpot, Vector(-dmg*0.001f, 0.0f, 0.0f), radius, true);
			//}
			EMIT_SOUND(idx, vecSpot, CHAN_VOICE, "ambience/particle_suck1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_LOW);

			DecalTrace("{crack2", args->origin, Vector(0.0f,0.0f,-dmg) + args->origin);
		}
		break;
	case GREN_NUCLEAR:
		{
			vecSpot[2] += 48.0f;
			EMIT_SOUND(idx, vecSpot, CHAN_STATIC, "weapons/explode_nuc.wav", VOL_NORM, ATTN_NONE, 0, PITCH_NORM);
			// nuc_fx.spr GLOW!
			gEngfuncs.pEfxAPI->R_TempSprite(vecSpot, g_vecZero, 24.0f, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 1.0, 2.4, FTENT_FADEOUT);
			DynamicLight(args->origin, args->fparam2, 255,255,223, 4.0f, 3.2f);
			if (g_pRenderManager)
			{
				// Slow huge cylinder
				CRSCylinder *pEffect = new CRSCylinder(args->origin, dmg*0.1f, dmg*0.25f, dmg*0.5f, 32, g_pSpriteZeroFire, kRenderTransAdd, 255,255,255, 1.0,-0.15, 0);
				if (pEffect)
				{
					pEffect->m_fFrameRate = 20.0f;
					g_pRenderManager->AddSystem(pEffect, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES);
				}

				// uexplo.spr
				if (g_pCvarEffects->value > 0.0f)
				{
					// case 1 g_iModelIndexNucExp3
					g_pRenderManager->AddSystem(new CRSSprite(vecSpot, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 255,255,255, 1.0f,0.0f, 20.0f, 1.0f, 10.0f, 0.0f));

					if (g_pCvarEffects->value > 1.0f)
						FX_SparkShower(vecSpot, g_pSpriteZeroGlow, 16, 1.0f*VectorRandom()*dmg, true, 5.0f);
				}

				// case 2
				g_pRenderManager->AddSystem(new CRSDelayed(new CRSSprite(vecSpot+Vector(0,0,140), Vector(0,0,16), g_pSpriteNucBlow, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 22.0f,2.0f, 10.0f, 0.0f), 0.5f));

				// case 3
				g_pRenderManager->AddSystem(new CRSDelayed(new CRSSprite(vecSpot+Vector(0,0,540), Vector(0,0,32), g_pSpriteGExplo, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 15.0f,2.0f, 10.0f, 0.0f), 0.9f));

				if (g_pCvarParticles->value > 0.0f)
				{
					g_pRenderManager->AddSystem(new CPSSparks(64, args->origin, 1.0f,0.1f, 0.5f, 80.0f, 2.5f, 240,200,255,1.0f,-1.0f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 1.5f), RENDERSYSTEM_FLAG_ADDPHYSICS, -1);//RENDERSYSTEM_FLAG_SIMULTANEOUS

					if (g_pCvarParticles->value > 1.0f)
					{
						g_pRenderManager->AddSystem(new CRSDelayed(
							new CPSSparks(320, vecSpot, 2.0f,0.1f, 0.5f, 800.0f, 2.2f, 255,255,255,1.0f,-0.5f, g_pSpriteZeroGlow, kRenderTransAdd, 2.5f),
							2.0f, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_ADDPHYSICS|((waterlevel > WATERLEVEL_NONE)?0:RENDERSYSTEM_FLAG_ADDGRAVITY), -1));
					}
				}
				// White smoke
				byte gray = (byte)RANDOM_LONG(20,35);
				g_pRenderManager->AddSystem(new CRSDelayed(
					new CRSSprite(vecSpot, Vector(0,0,32), g_pSpriteZeroSteam, kRenderTransAlpha, gray,gray,gray, 0.0f,0.0f, 80,4.0f, 12.0f, 0.0f),
					4.5f, RENDERSYSTEM_FLAG_NOCLIP));

				if (g_pCvarEffects->value > 0.0f)
				{
					// Smoke
					gray = (byte)RANDOM_LONG(20,35);
					g_pRenderManager->AddSystem(new CRSDelayed(
						new CRSSprite(vecSpot, Vector(0,0,48), g_pSpriteZeroSteam, kRenderTransAlpha, gray,gray,gray, 0.0f,0.0f, 100,2.0f, 10.0f, 0.0f),
						6.0f, RENDERSYSTEM_FLAG_NOCLIP));// kRenderTransAlpha alpha is useless in this mode

					if (g_pCvarEffects->value > 1.0f)
					{
						// Very nice effect
						g_pRenderManager->AddSystem(new CRSBeamStar(args->origin, IEngineStudio.GetModelByIndex(g_iModelIndexFlameFire2), 32, kRenderTransAdd, 255,255,255, 1.0,-0.15, 320,480, 0.0), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES);

						if (g_pCvarParticles->value > 1.0f)
						{
							//CPSSparks *pSparkSystem = new CPSSparks(800, vecSpot, 4.0f,0.2f, 0.5f, 800.0f, 2.2f, 255,0,0,1.0f,-0.1f, g_pSpriteZeroGlow, kRenderTransAdd, 10.0f);
							//if (pSparkSystem)
							//{
							//	pSparkSystem->m_OnParticleUpdate = OnParticleUpdateWhirl;
							//	g_pRenderManager->AddSystem(new CRSDelayed(pSparkSystem, 2.0f, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS, -1));
							//}
							g_pRenderManager->AddSystem(new CRSDelayed(
								new CPSSparks(400, vecSpot, 4.0f,0.2f, 0.5f, 1200.0f, 2.2f, 255,240,191,1.0f,-0.2f, g_pSpriteZeroFlare, kRenderTransAdd, 8.0f),
									1.0f, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS, -1));

							g_pRenderManager->AddSystem(new CRSDelayed(
								new CPSSparks(600, vecSpot, 3.0f,0.1f, 0.75f, 800.0f, 2.2f, 224,191,191,1.0f,-0.075f, IEngineStudio.GetModelByIndex(g_iModelIndexZeroParts), kRenderTransAdd, 10.0f),
									1.5f, RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_ADDGRAVITY, -1));
						}
					}
				}
			}
		}
		break;
	}
}


const char *gSoundsHornetBuzz[] =
{
	"hornet/ag_buzz1.wav",
	"hornet/ag_buzz2.wav",
	"hornet/ag_buzz3.wav",
	NULL
};

//-----------------------------------------------------------------------------
// Purpose: light projectile hit
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - pev->dmg
// fparam2 - 
// iparam1 - 
// iparam2 - m_bloodColor
// bparam1 - pev->skin
// bparam2 - hornet_event_e
//-----------------------------------------------------------------------------
void EV_Hornet(struct event_args_s *args)
{
	EV_START(args);
	int streakcolor = 0;
	Color c;
	if (args->bparam1 == HORNET_TYPE_GREEN)
	{
		c.Set(0,255,0);
		streakcolor = 2;
	}
	else if (args->bparam1 == HORNET_TYPE_BLUE)
	{
		c.Set(0,0,255);
		streakcolor = 3;
	}
	else if (args->bparam1 == HORNET_TYPE_YELLOW)
	{
		c.Set(255,255,0);
		streakcolor = 6;
	}
	else if (args->bparam1 == HORNET_TYPE_RED)
	{
		c.Set(255,0,0);
		streakcolor = 1;
	}

	if (args->bparam2 == HORNET_EV_START)
	{
		gEngfuncs.pEfxAPI->R_BeamFollow(args->entindex, args->iparam1, RANDOM_FLOAT(0.1,0.15), 0.2f, (float)c.r/255.0f,(float)c.g/255.0f,(float)c.b/255.0f, RANDOM_FLOAT(0.75,0.875));
	}
	else if (args->bparam2 == HORNET_EV_BUZZ)
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteHornetExp, kRenderTransAdd, c.r,c.g,c.b,1.0f,0.0f, 0.25f,0.1f, 20, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

		EMIT_SOUND(args->entindex, args->origin, CHAN_VOICE, gSoundsHornetBuzz[RANDOM_LONG(0,2)], VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
	}
	else if (args->bparam2 == HORNET_EV_HIT)
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteHornetExp, kRenderTransAdd, c.r,c.g,c.b,1.0f,0.0f, 0.5f,-0.5f, 20, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

		//FX_StreakSplash(args->origin, -args->velocity, c.r,c.g,c.b, (g_pCvarParticles->value > 1)?32:16, 160, true,false,false);
		gEngfuncs.pEfxAPI->R_StreakSplash(args->origin, -args->velocity, streakcolor, (g_pCvarParticles->value > 1)?32:16, 160, -200, 200);
		EMIT_SOUND(args->entindex, args->origin, CHAN_VOICE, "hornet/ag_hornethit3.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
	}
	else if (args->bparam2 == HORNET_EV_DESTROY)
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteHornetExp, kRenderTransAdd, c.r,c.g,c.b,1.0f,0.0f, 0.5f,0.5f, 15, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

		EMIT_SOUND(args->entindex, args->origin, CHAN_VOICE, "hornet/ag_puff.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
	}
}

//-----------------------------------------------------------------------------
// Purpose: light projectile start/hit
// entindex - projectile
// origin - origin
// angles - angles
// fparam1 - 1.6f/*beamwidth*/
// fparam2 - 0.1f/*glowscale*/
// iparam1 - m_iTrail
// iparam2 - m_iFlare OBSOLETE
// bparam1 - pev->skin (type)
// bparam2 - lightprojectile_event_e
//-----------------------------------------------------------------------------
void EV_LightProjectile(struct event_args_s *args)
{
	EV_START(args);
	byte r,g,b;
	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(args->entindex);
	if (ent)// found entity
	{
		r = ent->curstate.rendercolor.r;
		g = ent->curstate.rendercolor.g;
		b = ent->curstate.rendercolor.b;
	}
	else// load default color
	{
		if (args->bparam1 > 0)// type
		{
			r = 0; g = 63; b = 255;
		}
		else
		{
			r = 0; g = 255; b = 63;
		}
	}

	if (args->bparam2 == LIGHTP_EV_START)
	{
		/*BEAM *pBeam = */gEngfuncs.pEfxAPI->R_BeamFollow(args->entindex, args->iparam1, 0.1f, args->fparam1, (float)r/255.0f,(float)g/255.0f,(float)b/255.0f, 0.6f);
		/*if (pBeam && ent)
		{
			pBeam->r = ent->curstate.rendercolor.r/255.0f;
			pBeam->g = ent->curstate.rendercolor.g/255.0f;
			pBeam->b = ent->curstate.rendercolor.b/255.0f;
		}*/
		/* doesn't look nice	if (g_pRenderManager && ent && g_pCvarEffects->value > 0.0f)
		{
			CRSSprite *pSys = new CRSSprite(args->origin, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, r,g,b, 1.0f,-0.5f, args->fparam2, 0.0f, 16.0f, 0.5f);
			g_pRenderManager->AddSystem(pSys, 0, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
		}*/
	}
	else
	{
		Vector vecOrigin(args->origin);
		gEngfuncs.pEfxAPI->R_BeamKill(args->entindex);

		if (!UTIL_PointIsFar(vecOrigin))// XDM3035c
			DynamicLight(vecOrigin, 80, r,g,b, 1.0f, 100.0f);

		if (args->bparam2 != LIGHTP_EV_HITWATER)// XDM3038c
		{
			Vector vecDir;
			AngleVectors(args->angles, vecDir, NULL, NULL);
			Vector vecEnd(vecDir); vecEnd *= 10.0f; vecEnd += vecOrigin;
			pmtrace_t tr;
			gEngfuncs.pEventAPI->EV_PushPMStates();
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);	
			gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
			gEngfuncs.pEventAPI->EV_PlayerTrace(vecOrigin, vecEnd, PM_STUDIO_BOX, -1, &tr);
			gEngfuncs.pEventAPI->EV_PopPMStates();

			if (args->bparam1 == 0)// normal sprite
			{
				if (g_pRenderManager)
					g_pRenderManager->AddSystem(new CRSSprite(vecOrigin, tr.plane.normal*16.0f, g_pSpriteLightPHit/*IEngineStudio.GetModelByIndex(args->iparam2)*/, kRenderTransAdd, 255,255,255, 1.0f,-0.5f, 1.0f,0.0f, (args->bparam2 == LIGHTP_EV_HIT)?32:28, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
	// SPRITE TEST	g_pRenderManager->AddSystem(new CRSSprite(vecOrigin, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,0.0f, 24.0f, 10.0f), RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_NOCLIP);
			}
			else if (args->bparam1 == 1)// flat sprite, parallel to surface
			{
				if (g_pRenderManager)
				{
					CRenderSystem *pSys;
					if (args->bparam2 == LIGHTP_EV_HIT)// hit
					{
						pSys = new CRSSprite(vecOrigin, tr.plane.normal*16.0f, g_pSpriteLightPRing, kRenderTransAdd, 255,255,255, 1.0f,-3.0f, 0.05f,1.0f, 20.0f, 0.0f);
					}
					else// parallel to world surface
					{
						Vector vecAnglesSystem, vecSpot(tr.endpos);
						vecSpot += tr.plane.normal;
						VectorAngles(tr.plane.normal, vecAnglesSystem);
						vecAnglesSystem.z = RANDOM_LONG(0,359);// XDM3038c: looks nicer when rotated
						pSys = new CRenderSystem(vecSpot, g_vecZero, vecAnglesSystem, g_pSpriteLightPRing/*IEngineStudio.GetModelByIndex(args->iparam2)*/, kRenderTransAdd, 255,255,255, 1.0f,-2.0f, 0.05f,1.0f, 20.0f, 0.0f);
					}
					if (pSys)
					{
						pSys->m_iDoubleSided = true;// XDM3038a: what if ball hits some thin column?
						g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_LOOPFRAMES | RENDERSYSTEM_FLAG_NOCLIP);
					}
				}
			}
			DecalTrace(RANDOM_LONG(DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3), &tr, vecOrigin);
			EMIT_SOUND(args->entindex, vecOrigin, CHAN_BODY, "weapons/lp_exp.wav", VOL_NORM, ATTN_IDLE, 0, 98 + RANDOM_LONG(0,4));
		}
		else// water
		{
			if (g_pRenderManager && g_pCvarEffects->value > 0.0f)
				g_pRenderManager->AddSystem(new CRSSprite(vecOrigin, Vector(0,0,10), IEngineStudio.GetModelByIndex(g_iModelIndexBallSmoke), kRenderTransAdd, 63,63,63, 1.0f,-0.5f, args->fparam2,1.0f, 10.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

			EMIT_SOUND(args->entindex, vecOrigin, CHAN_BODY, "common/fire_extinguish.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
		}

		if (g_pCvarEffects->value > 0.0f && !UTIL_PointIsFar(vecOrigin))
		{
			int c, count;
			if (args->bparam2 == LIGHTP_EV_HIT)// hit
			{
				c = 0;
				count = RANDOM_LONG(20,24);
			}
			else if (args->bparam1 == 0)// green
			{
				c = 2;
				count = RANDOM_LONG(14,18);
			}
			else if (args->bparam1 == 1)// blue
			{
				c = 3;
				count = RANDOM_LONG(12,16);
			}
			else// server sent bad data
			{
				c = 4;
				count = 16;
			}
			gEngfuncs.pEfxAPI->R_StreakSplash(vecOrigin, g_vecZero, c, count, RANDOM_LONG(48,56), -200, 200);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: plasma/pulse rifle projectile hit event
// entindex - 
// origin - start
// angles - angles
// fparam1 - pev->dmg
// fparam2 - 
// iparam1 - pev->modelindex
// iparam2 - m_iSpriteHit
// bparam1 - pev->skin
// bparam2 - plasmaball_event_e
//-----------------------------------------------------------------------------
void EV_PlasmaBall(struct event_args_s *args)
{
	EV_START(args);
	Vector vecOrigin(args->origin);
	Vector vecNormal(0,0,0);

	if (args->bparam2 == PLASMABALL_EV_HITWATER)// XDM3038c: hit water
	{
		int contents = gEngfuncs.PM_PointContents(vecOrigin, NULL);
		if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME)
		{
			FX_BubblesSphere(vecOrigin, args->fparam2*0.4f, args->fparam2*0.5f, g_pSpriteBubble, (int)(args->fparam1*0.75f), RANDOM_FLOAT(30,40));

			if (g_pRenderManager && g_pCvarEffects->value > 0.0f)
				g_pRenderManager->AddSystem(new CRSSprite(vecOrigin, Vector(0,0,10), IEngineStudio.GetModelByIndex(g_iModelIndexBallSmoke), kRenderTransAdd, 63,63,63, 1.0f,-0.5f, args->fparam1*0.05f,1.0f, 10.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

			EMIT_SOUND(args->entindex, vecOrigin, CHAN_BODY, "common/fire_extinguish.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));
		}
	}
	else
	{
		if (args->bparam2 == PLASMABALL_EV_HITSOLID)// hit world, explode
		{
			Vector vecDir;
			AngleVectors(args->angles, vecDir, NULL, NULL);
			Vector vecEnd(vecDir); vecEnd *= 16.0f; vecEnd += vecOrigin;
			pmtrace_t tr;
			gEngfuncs.pEventAPI->EV_PushPMStates();
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);	
			gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
			gEngfuncs.pEventAPI->EV_PlayerTrace(vecOrigin, vecEnd, PM_STUDIO_BOX, -1, &tr);
			gEngfuncs.pEventAPI->EV_PopPMStates();

			if (g_pRenderManager)
			{
				Vector vecAnglesSystem, vecSpot(vecOrigin);
				vecNormal = tr.plane.normal;
				vecSpot += vecNormal;
				//UTIL_DebugBeam(vecOrigin, vecSpot+vecNormal*24.0f, 5.0f, 127,255,0);
				VectorAngles(vecNormal, vecAnglesSystem);
				vecAnglesSystem.z = RANDOM_LONG(0,359);// XDM3037: looks nicer when rotated
				CRenderSystem *pSys = new CRenderSystem(vecSpot, g_vecZero, vecAnglesSystem, g_pSpritePBallHit/*IEngineStudio.GetModelByIndex(args->iparam2)*/, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 0.25f,1.0f, 24.0f, 0.0f);
				if (pSys)
				{
					pSys->m_iDoubleSided = true;// XDM3037: what if ball hits some thin column?
					g_pRenderManager->AddSystem(pSys, RENDERSYSTEM_FLAG_NOCLIP);
				}
			}
			//if (IsWorldBrush(tr.ent))
			DecalTrace(RANDOM_LONG(DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3), &tr, vecOrigin);
		}
		if (!UTIL_PointIsFar(vecOrigin))// XDM3035c
		{
			DynamicLight(vecOrigin, 80, 127,191,255, 1.0f, 100.0f);
			int basepitch;
			if (args->bparam2 == PLASMABALL_EV_HITSOLID)// hit world, explode
				basepitch = 95;
			else
				basepitch = 100;

			EMIT_SOUND(args->entindex, vecOrigin, CHAN_BODY, "weapons/pb_exp.wav", VOL_NORM, ATTN_IDLE, 0, basepitch + RANDOM_LONG(0,5));

			if (g_pCvarParticles->value > 0.0)
			{
				int numpart;
				float partvel;
				if (args->bparam2 == PLASMABALL_EV_HITSOLID)// hit world, explode
				{
					numpart = RANDOM_LONG(24, 32);
					partvel = 400.0f;
				}
				else
				{
					numpart = RANDOM_LONG(56, 64);
					partvel = 200.0f;
				}
				if (g_pCvarParticles->value > 1.0f)
					numpart *= 2;

				//FX_StreakSplash(vecOrigin, normal, gTracerColors[7], numpart, partvel, true, true, false);
				if (g_pCvarParticles->value > 1.0)
					FX_StreakSplash(vecOrigin, vecNormal, gTracerColors[0], numpart/2, partvel/2, true, true, false);// XDM3038: more fun

				vecNormal *= -1.0f;
				gEngfuncs.pEfxAPI->R_StreakSplash(vecOrigin, vecNormal, 7, numpart, partvel, -200, 200);// XDM3038c
			}
			if ((args->bparam2 == PLASMABALL_EV_HITSOLID) && g_pCvarEffects->value > 0.0)
			{
				TEMPENTITY *pGlow = gEngfuncs.pEfxAPI->R_TempSprite(vecOrigin, g_vecZero, 1.0f, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 0.75f, 0.25f, FTENT_FADEOUT);
				if (pGlow)
				{
					//pGlow->fadeSpeed = 1;
					pGlow->entity.curstate.scale = 0.25f;
				}
				//gEngfuncs.pEfxAPI->R_StreakSplash(vecOrigin, g_vecZero, 7, 20, 50, -200, 200);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Razor disk hit/bounce event
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - dmg
// fparam2 - cTextureType (actually a char)
// iparam1 - explosion sprite index
// iparam2 - sound pitch
// bparam1 - razordisk_hittype_e: 0 solid, 1 armor, 2 flesh, 3 stuck
// bparam2 - razordisk_event_e: 0 reflect, 1 explode
//-----------------------------------------------------------------------------
void EV_RazorDisk(struct event_args_s *args)
{
	EV_START(args);
	int iDecal = 0;
	bool isfar = UTIL_PointIsFar(args->origin);
	if (args->bparam2 == RAZORDISK_EXPLODE)
	{
		STOP_SOUND(args->entindex, CHAN_BODY, "weapons/blade_fly.wav");
		EMIT_SOUND(-1, args->origin, CHAN_WEAPON, "weapons/xbow_exp.wav", VOL_NORM, ATTN_NORM, 0, args->iparam2);
		iDecal = RANDOM_LONG(DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3);
	}
	else if (args->bparam2 == RAZORDISK_REFLECT)
	{
		char cTextureType = (char)args->fparam2;
		if (args->bparam1 == RAZORDISK_HIT_ARMOR)// hit armor
		{
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, g_pSpriteRicho1, kRenderTransAdd, 255,255,255, 1.0f,0.0f, 1.0f,1.0f, 24.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);

			if (!isfar)
				EMIT_SOUND(-1, args->origin, CHAN_STATIC, gSoundsRicochet[RANDOM_LONG(0,NUM_RICOCHET_SOUNDS-1)], VOL_NORM, ATTN_NORM, 0, args->iparam2);
		}
		else if (args->bparam1 == RAZORDISK_HIT_FLESH || cTextureType == CHAR_TEX_FLESH)// hit flesh
		{
			// don't draw decal because we don't know blood color, the server damage code handles this
			STOP_SOUND(args->entindex, CHAN_BODY, "weapons/blade_fly.wav");
			EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "weapons/blade_hit2.wav", VOL_NORM, ATTN_NORM, 0, args->iparam2);
		}
		else// hit solid
		{
			if (cTextureType == CHAR_TEX_WOOD)
				iDecal = RANDOM_LONG(DECAL_WOODBREAK1, DECAL_WOODBREAK3);
			else if (cTextureType == CHAR_TEX_GLASS)
				iDecal = RANDOM_LONG(DECAL_GLASSBREAK1, DECAL_GLASSBREAK3);
			else if (cTextureType == CHAR_TEX_DIRT || cTextureType == CHAR_TEX_GRASS || cTextureType == CHAR_TEX_CEILING)
				iDecal = RANDOM_LONG(DECAL_WOODBREAK1, DECAL_WOODBREAK3);
			else
				iDecal = RANDOM_LONG(DECAL_LARGESHOT1, DECAL_LARGESHOT5);

			if (!isfar)
			{
				if (cTextureType == CHAR_TEX_CONCRETE || cTextureType == CHAR_TEX_METAL)
					EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "weapons/blade_hit1.wav", VOL_NORM, ATTN_NORM, 0, args->iparam2);
				else
					PlayTextureSound(cTextureType, args->origin, false);
			}
		}
	}
	if (iDecal != 0)
	{
		Vector vecEnd;
		AngleVectors(args->angles, vecEnd, NULL, NULL);
		vecEnd += args->origin;
		DecalTrace(iDecal, args->origin, vecEnd);
	}

	int streakcolor;
	color24 c;
	if (args->bparam2 == RAZORDISK_EXPLODE)
		{c.r=255; c.g=191; c.b=95; streakcolor=5;}
	else if (args->bparam1 == RAZORDISK_HIT_SOLID)
		{c.r=239; c.g=191; c.b=255; streakcolor=7;}
	else
		{c.r=255; c.g=223; c.b=159; streakcolor=9;}

	if (args->bparam1 != RAZORDISK_HIT_FLESH)
	{
		//FX_StreakSplash(args->origin, args->origin, c, args->bparam2==RAZORDISK_REFLECT?(isfar?16:24):(isfar?32:48), args->bparam2==RAZORDISK_REFLECT?96:192, true, (g_pCvarEffects->value > 0), (g_pCvarEffects->value > 0));
		gEngfuncs.pEfxAPI->R_StreakSplash(args->origin, g_vecZero, streakcolor, args->bparam2==RAZORDISK_REFLECT?(isfar?16:24):(isfar?32:48), args->bparam2==RAZORDISK_REFLECT?96:192, -200, 200);
	}

	if (!isfar)
	{
		if (g_pCvarEffects->value > 0)
			DynamicLight(args->origin, args->fparam1*1.5f, c.r,c.g,c.b, args->fparam1*0.25f, 100);

		int contents = gEngfuncs.PM_PointContents(args->origin, NULL);
		if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME)
		{
			float r = args->fparam1*(args->bparam2==RAZORDISK_EXPLODE?0.5f:0.25f);
			FX_BubblesSphere(args->origin, r*0.8f, r, g_pSpriteBubble, (int)(args->fparam1*(args->bparam2==RAZORDISK_EXPLODE?0.75f:0.25f)), RANDOM_FLOAT(30,40));
		}
	}
	if (args->bparam2 == RAZORDISK_REFLECT)
	{
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex(args->entindex);
		if (ent)
			ent->curstate.effects |= EF_MUZZLEFLASH;
	}
	else
	{
		if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam1), kRenderTransAdd, 255,255,255,1.0f,0.0f, args->fparam1*0.02f,0.5f, 24.0f, 0.0f), RENDERSYSTEM_FLAG_NOCLIP);
	}
}

//-----------------------------------------------------------------------------
// Purpose: RPG rocket start/end event
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - beam sprite
// iparam2 - glow sprite
// bparam1 - 0 - start, 1 - destroy
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_Rocket(struct event_args_s *args)
{
	EV_START(args);
	if (args->bparam1 == 0)// create effect
	{
		if (GetUpdatingEntity(args->entindex) == NULL)// XDM3038a: in case event arrives after the rocket dies
			return;

		EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "weapons/rocket1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		gEngfuncs.pEfxAPI->R_BeamFollow(args->entindex, args->iparam1, 1.5, 8.0, 1.0,1.0,1.0, 0.8);

		if (g_pCvarParticles->value > 0.0f)// standard particle trail
			FX_Trail(args->origin, args->entindex, 0, args->fparam1);

		if (g_pCvarEffects->value > 0.0f)// optional effects
		{
			FX_StreakSplash(args->origin, g_vecZero, gTracerColors[5], 20, 240.0f, true, true, false);

			if (g_pCvarParticles->value > 1.0f)// more sparks
				FX_StreakSplash(args->origin, g_vecZero, gTracerColors[0], 8, 240.0f, true, true, false);

			if (g_pRenderManager && g_pCvarEffects->value > 1.0f)// more smoke
				g_pRenderManager->AddSystem(new CRSSprite(args->origin, -0.5f*args->velocity, g_pSpriteWallPuff1, kRenderTransAlpha, 63,63,63, 1.0f,-0.5f, 2.0f, 1.0f, 24.0f, 1.0f), RENDERSYSTEM_FLAG_ADDPHYSICS);
		}

		if (g_pRenderManager && g_pCvarParticles->value > 1.0f)
		{
			g_pRenderManager->AddSystem(new CPSFlameCone((g_pCvarEffects->value > 0.0f)?256:192, args->origin, args->origin,
				VECTOR_CONE_20DEGREES, 10.0f, IEngineStudio.GetModelByIndex(g_iModelIndexSmkball), kRenderTransAlpha, 1.0f,-1.25f, 4,20, 0), RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
		}

		/*if (g_pCvarEffects->value > 1.0f)
		{
			kRenderGlow doesn't work			if (g_pRenderManager)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 255,255,255, 1.0, 0.0,  16.0, 0.0, args->fparam1), 0, args->entindex);
		}*/
		//PartSystem(args->origin, args->origin, args->origin, args->iparam2, kRenderTransAdd, PARTSYSTEM_TYPE_FLAMECONE, 128, args->fparam2, 0, args->entindex);
	}
	else// destroy effect
	{
		gEngfuncs.pEventAPI->EV_StopSound(args->entindex, CHAN_BODY, "weapons/rocket1.wav");
		gEngfuncs.pEfxAPI->R_BeamKill(args->entindex);

		if (g_pRenderManager)
			g_pRenderManager->DeleteEntitySystems(args->entindex, true);// XDM3037: TESTME
	}
}

//-----------------------------------------------------------------------------
// Purpose: TEnt spark shower effect
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - speed
// fparam2 - life
// iparam1 - modelindex
// iparam2 - count
// bparam1 - random
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_SparkShower(struct event_args_s *args)
{
	EV_START(args);
	FX_SparkShower(args->origin, (args->iparam1 > 0)?IEngineStudio.GetModelByIndex(args->iparam1):g_pSpriteZeroFlare, args->iparam2, VectorRandom()*args->fparam1, args->bparam1>0, args->fparam2);
}

//-----------------------------------------------------------------------------
// Purpose: Teleporter ball start
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - TELEPORTER_EFFECT_RADIUS
// fparam2 - pev->teleport_time - gpGlobals->time
// iparam1 - beam sprite index
// iparam2 - trail sprite index
// bparam1 - pev->skin
// bparam2 - palette color
//-----------------------------------------------------------------------------
void EV_Teleporter(event_args_t *args)
{
	EV_START(args);
	FX_DisplacerBallParticles(args->origin, args->entindex, args->fparam1, args->bparam1, args->fparam2, g_DisplacerColors[args->bparam1].r, g_DisplacerColors[args->bparam1].g, g_DisplacerColors[args->bparam1].b);
	if (g_pCvarEffects->value > 0.0f)
	{
		if (g_pRenderManager && g_pCvarEffectsDLight->value > 0)
			g_pRenderManager->AddSystem(new CRSLight(args->origin, g_DisplacerColors[args->bparam1].r, g_DisplacerColors[args->bparam1].g, g_DisplacerColors[args->bparam1].b, args->fparam1*3.0f, NULL, 0, args->fparam2, false),
										RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);//RENDERSYSTEM_FFLAG_ICNF_NODRAW - Bad idea because RS quickly finds another entity with same index

		gEngfuncs.pEfxAPI->R_BeamFollow(args->entindex, args->iparam1, 0.5f, args->fparam1*0.05, 1.0f,1.0f,1.0f, 0.5f);
		if (g_pCvarParticles->value > 0.0f)
		{
			if (g_pRenderManager)
			{
				CParticleSystem *pSystem = new CParticleSystem((g_pCvarEffects->value > 0.0f)?96:80, args->origin, args->origin, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, args->fparam2);
				if (pSystem)
				{
					pSystem->m_iFxLevel = 1;
					pSystem->m_iTraceFlags = PM_STUDIO_IGNORE|PM_WORLD_ONLY;
					pSystem->m_fParticleScaleMin = 0.2f;
					pSystem->m_fParticleScaleMax = 0.5f;
					pSystem->m_fParticleSpeedMin = 2.0f;
					pSystem->m_fParticleSpeedMax = 24.0f;
					pSystem->m_fParticleWeight = 0;
					pSystem->m_iStartType = PSSTARTTYPE_POINT;
					pSystem->m_iMovementType = PSMOVTYPE_OUTWARDS;
					pSystem->m_fScale = (args->bparam1 == 0)?1.0f:0.75f;
					pSystem->m_fScaleDelta = 10.0f;
					pSystem->m_fFrameRate = 20.0f;
					pSystem->m_color.Set(255, 255, 255, 224);
					pSystem->m_fColorDelta[3] = -1.5f;
					pSystem->m_fBounceFactor = 0.0f;
					pSystem->m_fFriction = 0.0f;
					g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
				}
				if (g_pCvarParticles->value > 1.0f)
				{
				CParticleSystem *pSystem = new CParticleSystem((g_pCvarEffects->value > 0.0f)?80:64, args->origin, args->origin, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, args->fparam2);
				if (pSystem)
				{
					pSystem->m_iFxLevel = 2;
					pSystem->m_iTraceFlags = PM_STUDIO_IGNORE|PM_WORLD_ONLY;
					pSystem->m_fParticleScaleMin = 0.1f;
					pSystem->m_fParticleScaleMax = 0.2f;
					pSystem->m_fParticleSpeedMin = 64.0f;
					pSystem->m_fParticleSpeedMax = 96.0f;
					pSystem->m_fParticleWeight = 0;
					pSystem->m_iStartType = PSSTARTTYPE_POINT;
					pSystem->m_iMovementType = PSMOVTYPE_OUTWARDS;
					pSystem->m_fScale = (args->bparam1 == 0)?1.0f:0.75f;
					pSystem->m_fScaleDelta = 10.0f;
					pSystem->m_fFrameRate = 20.0f;
					pSystem->m_color.Set(g_DisplacerColors[args->bparam1].r, g_DisplacerColors[args->bparam1].g, g_DisplacerColors[args->bparam1].b, 255);
					pSystem->m_fColorDelta[3] = -1.5f;
					pSystem->m_fBounceFactor = 0.0f;
					pSystem->m_fFriction = 0.0f;
					g_pRenderManager->AddSystem(pSystem, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES|RENDERSYSTEM_FLAG_STARTRANDOMFRAME, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
				}
				}
				/*g_pRenderManager->AddSystem(new CPSFlameCone((g_pCvarEffects->value > 0.0f)?96:80, args->origin, args->origin,
					VECTOR_CONE_20DEGREES, 12.0f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 0.9f, -1.5f, (args->bparam1 == 0)?4:2, 10, args->fparam2),
						RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);

				if (g_pCvarParticles->value > 1.0f)
				{
					CPSFlameCone *pSmallTrail = new CPSFlameCone((g_pCvarEffects->value > 0.0f)?80:64, args->origin, args->origin,
						VECTOR_CONE_20DEGREES, 30.0f, IEngineStudio.GetModelByIndex(args->iparam2), kRenderTransAdd, 1.0f, -1.25f, 1, -10, args->fparam2);
					if (pSmallTrail)
					{
						pSmallTrail->m_iFxLevel = 2;
						pSmallTrail->m_color.Set(g_DisplacerColors[args->bparam1].r, g_DisplacerColors[args->bparam1].g, g_DisplacerColors[args->bparam1].b, 255);
						g_pRenderManager->AddSystem(pSmallTrail, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_LOOPFRAMES, args->entindex, RENDERSYSTEM_FFLAG_ICNF_REMOVE);
					}
				}*/
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: teleporter hit
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - damage
// fparam2 - radius (~200+)
// iparam1 - pev->modelindex
// iparam2 - TELEPORTER_BLASTTYPE
// bparam1 - pev->skin
// bparam2 - 0 - normal/1 - no effects
//-----------------------------------------------------------------------------
void EV_TeleporterHit(event_args_t *args)
{
	EV_START(args);
	if (g_pRenderManager)
		g_pRenderManager->DeleteEntitySystems(args->entindex, (args->bparam2 == 0));

	if (args->bparam2 > 0)// just kill effects (hit sky/water), we're done here.
		return;

	byte pc, r,g,b;
	if (args->bparam1 > 0)// particle color
	{
		pc = BLOOD_COLOR_BLUE;
		if (args->bparam1 > 1)// array bounds!
			args->bparam1 = 1;
	}
	else
		pc = BLOOD_COLOR_GREEN;

	r = g_DisplacerColors[args->bparam1].r;
	g = g_DisplacerColors[args->bparam1].g;
	b = g_DisplacerColors[args->bparam1].b;

	if (args->bparam1 == 0)
	{
		if (g_pRenderManager)
		{
			CRSCylinder *pFx = new CRSCylinder(args->origin, 32, 128, 64, 32, IEngineStudio.GetModelByIndex(g_iModelIndexShockWave), kRenderTransAdd, r,g,b, 1.0f, -1.0f, 4.0f);
			if (pFx)
			{
				pFx->m_fWidthDelta = -48.0f;// XDM3037a
				g_pRenderManager->AddSystem(pFx, RENDERSYSTEM_FLAG_NOCLIP);
			}
		}
	}
	gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, 1.0f, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 1.0f, 0.4f, FTENT_FADEOUT);

	if (!UTIL_PointIsFar(args->origin))// XDM3035c
		DynamicLight(args->origin, args->fparam2*1.5f, r,g,b, 2.0f, 160.0f);

	int decal1, decal2;
	if (args->iparam2 == TELEPORTER_BT_COL_PROJECTILE ||
		args->iparam2 == TELEPORTER_BT_COL_TELEPORTER ||
		args->iparam2 == TELEPORTER_BT_COL_PLASMA)
	{
		if (g_pRenderManager)
		{
			CRSSprite *pRS = new CRSSprite(args->origin, g_vecZero, g_pSpriteTeleFlash, kRenderTransAdd, 255,255,255, 1.0, -2.0, 1.0,-4.0, 20, 1.0);
			if (pRS)
			{
				pRS->m_fColorDelta[0] = -1.0f;// decrease red over time
				if (args->bparam1 > 0)// skin
					pRS->m_fColorDelta[1] = -1.0f;
				else
					pRS->m_fColorDelta[2] = -1.0f;

				g_pRenderManager->AddSystem(pRS, RENDERSYSTEM_FLAG_NOCLIP);
			}
		}
		EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/teleporter_blast2.wav", VOL_NORM, 0.5, 0, RANDOM_LONG(95, 105));
		decal1 = DECAL_SCORCH1;
		decal2 = DECAL_SCORCH3;
	}
	else if (args->iparam2 == TELEPORTER_BT_COL_WATER)
	{
		EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "common/fire_extinguish.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
	}
	else
	{
		EMIT_SOUND(args->entindex, args->origin, CHAN_WEAPON, "weapons/teleporter_blast.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95, 105));
		decal1 = DECAL_MDSCORCH1;
		decal2 = DECAL_MDSCORCH3;
	}

	if (g_pCvarEffects->value > 0)
	{
		ParticlesCustom(args->origin, 200.0f, pc, 8, 64, 1.0f, true);
		if (args->bparam1 > 0)
		{
			gEngfuncs.pEfxAPI->R_StreakSplash(args->origin, g_vecZero, 7, (g_pCvarEffects->value > 1)?192:96, RANDOM_LONG(90,100), -480, 480);
		}
		if (args->iparam2 != TELEPORTER_BT_COL_WATER && g_pCvarParticles->value > 0)
		{
			if (g_pRenderManager)
				g_pRenderManager->AddSystem(new CPSSparks(64, args->origin, 1.0f,0.1f,0.5f, 120.0f, 1.5f, r,g,b,1.0f,-2.0f, IEngineStudio.GetModelByIndex(g_iModelIndexAnimglow01), kRenderTransAdd, 1.0f), RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_CLIPREMOVE);
			if (g_pCvarParticles->value > 1)
			{
				color24 tracercolor = {r,g,b};//{min(255,r+8),min(255,g+8),min(255,b+8)};
				FX_StreakSplash(args->origin, g_vecZero, tracercolor, (g_pCvarEffects->value > 1)?64:32, 320.0f, false, false, false);
			}
		}
	}
	// Decals in all directions: does it really look good?
	float decalradius = args->fparam2*0.25f;
	DecalTrace(RANDOM_LONG(decal1, decal2), args->origin, args->origin - Vector(0.0f,0.0f,decalradius));// down
	if (args->iparam2 != TELEPORTER_BT_COL_WATER && g_pCvarEffects->value > 0.0f)
	{
		DecalTrace(RANDOM_LONG(decal1, decal2), args->origin, args->origin + Vector(0.0f,0.0f,decalradius));
		if (g_pCvarEffects->value > 1)
		{
			DecalTrace(RANDOM_LONG(decal1, decal2), args->origin, args->origin - Vector(0.0f,decalradius,0.0f));
			DecalTrace(RANDOM_LONG(decal1, decal2), args->origin, args->origin + Vector(0.0f,decalradius,0.0f));
			DecalTrace(RANDOM_LONG(decal1, decal2), args->origin, args->origin - Vector(decalradius,0.0f,0.0f));
			DecalTrace(RANDOM_LONG(decal1, decal2), args->origin, args->origin + Vector(decalradius,0.0f,0.0f));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - life
// fparam2 - 
// iparam1 - optional entindex
// iparam2 - type
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_Trail(event_args_t *args)
{
	EV_START(args);
	int idx;// = 0;
	if (args->iparam1 > 0)
		idx = args->iparam1;
	else
		idx = args->entindex;

	if (idx > 0)
		FX_Trail(args->origin, idx, args->iparam2, args->fparam1);
}

//-----------------------------------------------------------------------------
// Purpose: 
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - radius
// fparam2 - numbeams
// iparam1 - 1st sprite
// iparam2 - beam sprite
// bparam1 - do particles
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_WarpBall(struct event_args_s *args)
{
	EV_START(args);
	DynamicLight(args->origin, args->fparam1 * 0.5f, 0,255,0, 3.0f, 20.0f);
	/*TEMPENTITY *pSpr = gEngfuncs.pEfxAPI->R_TempSprite(args->origin, g_vecZero, 1.0f, args->iparam1, kRenderGlow, kRenderFxNoDissipation, 1.0, 3.0, FTENT_SPRANIMATE);
	if (pSpr)
	{
		pSpr->entity.baseline.rendercolor.r = pSpr->entity.curstate.rendercolor.r = 64;
		pSpr->entity.baseline.rendercolor.g = pSpr->entity.curstate.rendercolor.g = 255;
		pSpr->entity.baseline.rendercolor.b = pSpr->entity.curstate.rendercolor.b = 64;
		pSpr->entity.baseline.framerate = pSpr->entity.curstate.framerate = 20;
	}*/
	if (g_pRenderManager)
		g_pRenderManager->AddSystem(new CRSSprite(args->origin, g_vecZero, IEngineStudio.GetModelByIndex(args->iparam1), kRenderTransAdd, 63,255,63, 1.0f,0.0f, 1.0f,1.0f, 20.0f, 0.0f), 0, args->entindex, RENDERSYSTEM_FFLAG_DONTFOLLOW);
	//g_pRenderManager->AddSystem(new CRSDelayed(new CRSSprite(vecSpot+Vector(0,0,540), Vector(0,0,32), IEngineStudio.GetModelByIndex(spr2), kRenderTransAdd, 255,255,255, 1.0f,0.0f, 15.0f,2.0f, 10.0f, 0.0f), 0.9f));

	if (!UTIL_PointIsFar(args->origin))// XDM3037a: TODO: somehow make beams fade out?
	{
		Vector end;
		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		for (int i = 0; i < (int)args->fparam2; ++i)
		{
			VectorMA(args->origin, args->fparam1, VectorRandom(), end);
			gEngfuncs.pEventAPI->EV_PlayerTrace(args->origin, end, PM_GLASS_IGNORE | PM_WORLD_ONLY, -1, &tr);
			BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamPoints(args->origin, tr.endpos, args->iparam2, RANDOM_FLOAT(1.0, 2.0), RANDOM_FLOAT(1,8), RANDOM_FLOAT(0.1,0.4), RANDOM_FLOAT(0.4, 1.0), 1.0, 0, RANDOM_FLOAT(12,20), RANDOM_FLOAT(0.0, 0.5),1.0,RANDOM_FLOAT(0.0, 0.75));
			if (pBeam)
			{
				pBeam->flags |= FBEAM_SHADEOUT;
				//pBeam->SetThink(SUB_FadeOut);
				//pBeam->SetNextThink(RANDOM_FLOAT(0, 1.0));
			}
		}
	}
	if (args->bparam1 > 0)
		ParticlesCustom(args->origin, args->fparam1*2.0f, BLOOD_COLOR_GREEN, 8, 160, 2.0, true);
}

//-----------------------------------------------------------------------------
// Purpose: called from PM_CheckFalling()
// entindex - entity
// origin - origin
// angles - angles
// fparam1 - pmove->flFallVelocity;
// fparam2 - fvol;
// iparam1 - pmove->waterlevel;
// iparam2 - pmove->dead;
// bparam1 - (pmove->onground == -1)?0:1;
// bparam2 - g_onladder;
//-----------------------------------------------------------------------------
void EV_PM_Fall(struct event_args_s *args)
{
	EV_START(args);
	//conprintf(0, " EV_PM_Fall()\n");
	/*
	eargs.fparam1 = pmove->flFallVelocity;
	eargs.fparam2 = fvol;
	eargs.iparam1 = pmove->waterlevel;
	eargs.iparam2 = 0;
	eargs.bparam1 = (pmove->onground == -1)?0:1;
	eargs.bparam2 = g_onladder;
	*/
	// this will be zero :(	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(args->entindex);
	float flFallVelocity = args->fparam1;
	//if (flFallVelocity >= PLAYER_MAX_SAFE_FALL_SPEED*0.75f)
	{
		int waterlevel = args->iparam1;
		int onground = args->bparam1;
		if (!onground && waterlevel >= 2)
		{
			if (!UTIL_PointIsFar(args->origin))
			{
				Vector halfbox = (VEC_HULL_MAX - VEC_HULL_MIN + Vector(0.0f,0.0f,flFallVelocity*0.05f))/2;
				//UTIL_DebugBox(args->origin-halfbox, args->origin+halfbox, 4.0f, 0,127,255);
				FX_BubblesBox(args->origin - Vector(0,0,HULL_MAX), halfbox, g_pSpriteBubble, (int)floorf(flFallVelocity*0.15f), RANDOM_FLOAT(30,40));
				/*Vector mins = org + VEC_HULL_MIN - Vector(0.0f,0.0f,flFallVelocity*0.05f);
				Vector maxs = org + VEC_HULL_MAX;
				//gEngfuncs.pEfxAPI->R_ParticleBox(mins, maxs, 255,0,0, 4.0f);
				FX_Bubbles(mins, maxs, g_vecZero, g_pSpriteBubble, (int)floorf(flFallVelocity*0.15f), RANDOM_FLOAT(10,20));*/
				/*float flHeight = UTIL_WaterLevel(org, org.z, org.z + 1024.0f);
				flHeight = flHeight - mins.z;
				gEngfuncs.pEfxAPI->R_Bubbles(mins, maxs, flHeight, g_pSpriteBubble, (int)floorf(flFallVelocity*0.1f), flFallVelocity*0.2f);*/
				//UTIL_Bubbles(pev->origin + pev->mins - Vector(0.0f,0.0f,m_flFallVelocity*0.04f), pev->origin + pev->maxs, floor(m_flFallVelocity*0.1f));
			}
			float vel = flFallVelocity*0.4f;
			//gEngfuncs.pEfxAPI->R_StreakSplash(org+Vector(0,0,6), Vector(0,0,2), 0, 40, 20, -(int)vel, (int)vel);
			color24 c = {127,127,127};
			FX_StreakSplash(args->origin + Vector(0,0,6), Vector(0,0,2), c, RANDOM_LONG(32,40), vel, true, (g_pCvarEffects->value > 0.0f), false);
			//StreakSplash(pev->origin+Vector(0,0,6), Vector(0,0,2), 7, 40, 20, floor(m_flFallVelocity*0.4f));
		}
		else if (onground && args->iparam2 > 0)// XDM3037a
		{
			if (flFallVelocity >= PLAYER_MAX_SAFE_FALL_SPEED)
				DecalTrace(RANDOM_LONG(DECAL_BLOODSMEARR1, DECAL_BLOODSMEARR3), args->origin, args->origin + Vector(0,0,HULL_MIN));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Someone activates the long jump module (tm)
// entindex - is 0, use origin only
// origin - origin
// angles - angles
// fparam1 - 
// fparam2 - 
// iparam1 - 
// iparam2 - 
// bparam1 - 
// bparam2 - 
//-----------------------------------------------------------------------------
void EV_PM_Longjump(struct event_args_s *args)
{
	EV_START(args);
	//conprintf(0, " EV_PM_Longjump()\n");
	if (g_pCvarParticles->value > 0.0f)
		gEngfuncs.pEfxAPI->R_FlickerParticles(args->origin);

	if (!UTIL_PointIsFar(args->origin))
	{
		Vector vel(args->velocity);
		if (g_pRenderManager && g_pCvarEffects->value > 0.0f)
			g_pRenderManager->AddSystem(new CRSSprite(args->origin, -0.5f*vel, IEngineStudio.GetModelByIndex(g_iModelIndexBallSmoke), kRenderTransAdd, 63,63,63, 1.0f,-0.5f, 2.0f, 1.0f, 10.0f, 1.0f), RENDERSYSTEM_FLAG_ADDPHYSICS);

		EMIT_SOUND(args->entindex, args->origin, CHAN_BODY, "player/longjump.wav", 0.75, ATTN_NORM, 0, 98 + RANDOM_LONG(0,4));
	}
	gHUD.m_Flash.LongJump();// XDM3038a
}
