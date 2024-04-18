//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"

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
	void EV_FireSniperRifle(event_args_t *args);// XDM3038b
	void EV_FireTripmine(struct event_args_s *args);

	void EV_GrenMode(struct event_args_s *args);
	void EV_TripMode(struct event_args_s *args);
	void EV_TrainPitchAdjust(struct event_args_s *args);
	void EV_ZoomCrosshair(struct event_args_s *args);
	void EV_NuclearDevice(struct event_args_s *args);

	void EV_AcidGrenade(struct event_args_s *args);
	void EV_BeamBlast(struct event_args_s *args);
	void EV_BeamImpact(struct event_args_s *args);
	void EV_BlackBall(struct event_args_s *args);// XDM3038b
	void EV_BoltHit(struct event_args_s *args);// XDM3038
	void EV_BulletImpact(struct event_args_s *args);
	void EV_Explosion(struct event_args_s *args);
	void EV_Flame(struct event_args_s *args);
	void EV_FlameTrail(struct event_args_s *args);
	void EV_FuncTankFire(struct event_args_s *args);// XDM3038a
	void EV_GrenExp(struct event_args_s *args);
	void EV_Hornet(struct event_args_s *args);// XDM3038c
	void EV_LightProjectile(struct event_args_s *args);// XDM3038c
	void EV_PlasmaBall(struct event_args_s *args);// XDM3038c
	void EV_RazorDisk(struct event_args_s *args);
	void EV_Rocket(struct event_args_s *args);
	void EV_SparkShower(struct event_args_s *args);
	void EV_Teleporter(struct event_args_s *args);
	void EV_TeleporterHit(struct event_args_s *args);
	void EV_Trail(struct event_args_s *args);
	//void EV_WallPuff(struct event_args_s *args);
	void EV_WarpBall(struct event_args_s *args);

	void EV_CaptureObject(struct event_args_s *args);
	void EV_DomPoint(struct event_args_s *args);
	//void EV_ItemSpawn(struct event_args_s *args);
	void EV_PlayerSpawn(struct event_args_s *args);
	//void EV_Teleport(struct event_args_s *args);

	void EV_PM_Fall(struct event_args_s *args);
	void EV_PM_Longjump(struct event_args_s *args);
}


/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
/*
===================
EV_HookEvents

See if game specific code wants to hook any events.
===================
*/
void EV_HookEvents(void)
{
	HOOK_EVENT("events/weapons/alauncher.sc",	EV_FireALauncher);
	HOOK_EVENT("events/weapons/firebeam.sc",	EV_FireBeam);
	HOOK_EVENT("events/weapons/beamrifle.sc",	EV_FireBeamRifle);
	HOOK_EVENT("events/weapons/bhg.sc",			EV_FireBHG);
	HOOK_EVENT("events/weapons/firechemgun.sc",	EV_FireChemGun);
	HOOK_EVENT("events/weapons/crossbow.sc",	EV_FireCrossbow);
	HOOK_EVENT("events/weapons/crowbar.sc",		EV_FireCrowbar);
	HOOK_EVENT("events/weapons/displacer.sc",	EV_FireDisplacer);
	HOOK_EVENT("events/weapons/fireflame.sc",	EV_FireFlame);
	HOOK_EVENT("events/weapons/gauss.sc",		EV_FireGauss);
	HOOK_EVENT("events/weapons/glauncher.sc",	EV_FireGLauncher);
	HOOK_EVENT("events/weapons/glock.sc",		EV_FireGlock);
	HOOK_EVENT("events/weapons/firehornet.sc",	EV_FireHornetGun);
	HOOK_EVENT("events/weapons/mp5.sc",			EV_FireMP5);
	HOOK_EVENT("events/weapons/mp52.sc",		EV_FireMP52);
	HOOK_EVENT("events/weapons/plasmagun.sc",	EV_FirePlasmaGun);
	HOOK_EVENT("events/weapons/python.sc",		EV_FirePython);
	HOOK_EVENT("events/weapons/disklauncher.sc",EV_FireRazorDisk);
	HOOK_EVENT("events/weapons/rpg.sc",			EV_FireRpg);
	HOOK_EVENT("events/weapons/shotgun1.sc",	EV_FireShotGunSingle);
	HOOK_EVENT("events/weapons/shotgun2.sc",	EV_FireShotGunDouble);
	HOOK_EVENT("events/weapons/snarkfire.sc",	EV_FireSnark);
	HOOK_EVENT("events/weapons/sniperrifle.sc",	EV_FireSniperRifle);
	HOOK_EVENT("events/weapons/tripfire.sc",	EV_FireTripmine);
	HOOK_EVENT("events/weapons/grenmode.sc",	EV_GrenMode);
	HOOK_EVENT("events/weapons/tripmode.sc",	EV_TripMode);
	HOOK_EVENT("events/weapons/nucleardev.sc",	EV_NuclearDevice);
	HOOK_EVENT("events/weapons/zoomcrosshair.sc",EV_ZoomCrosshair);

	HOOK_EVENT("events/train.sc",				EV_TrainPitchAdjust);

	HOOK_EVENT("events/fx/agrenade.sc",			EV_AcidGrenade);
	HOOK_EVENT("events/fx/beamblast.sc",		EV_BeamBlast);
	HOOK_EVENT("events/fx/bmimpact.sc",			EV_BeamImpact);
	HOOK_EVENT("events/fx/blackball.sc",		EV_BlackBall);
	HOOK_EVENT("events/fx/bolthit.sc",			EV_BoltHit);
	HOOK_EVENT("events/fx/blimpact.sc",			EV_BulletImpact);
	//HOOK_EVENT("events/fx/bolthit.sc",			EV_BoltHit);
	HOOK_EVENT("events/fx/explosion.sc",		EV_Explosion);
	HOOK_EVENT("events/fx/flame.sc",			EV_Flame);
	HOOK_EVENT("events/fx/flametrail.sc",		EV_FlameTrail);
	HOOK_EVENT("events/fx/func_tank_fire.sc",	EV_FuncTankFire);
	HOOK_EVENT("events/fx/gren_exp.sc",			EV_GrenExp);
	HOOK_EVENT("events/fx/hornet.sc",			EV_Hornet);
	HOOK_EVENT("events/fx/lightp.sc",			EV_LightProjectile);
	HOOK_EVENT("events/fx/plasmaball.sc",		EV_PlasmaBall);
	HOOK_EVENT("events/fx/razordisk.sc",		EV_RazorDisk);
	HOOK_EVENT("events/fx/rocket.sc",			EV_Rocket);
	HOOK_EVENT("events/fx/sparkshower.sc",		EV_SparkShower);
	HOOK_EVENT("events/fx/teleporter.sc",		EV_Teleporter);
	HOOK_EVENT("events/fx/teleporterhit.sc",	EV_TeleporterHit);
	HOOK_EVENT("events/fx/trail.sc",			EV_Trail);
	//HOOK_EVENT("events/fx/wallpuff.sc",			EV_WallPuff);
	HOOK_EVENT("events/fx/warpball.sc",			EV_WarpBall);

	HOOK_EVENT("events/game/captureobject.sc",	EV_CaptureObject);
	HOOK_EVENT("events/game/dompoint.sc",		EV_DomPoint);
	//HOOK_EVENT("events/game/itemspawn.sc",		EV_ItemSpawn);
	HOOK_EVENT("events/game/playerspawn.sc",	EV_PlayerSpawn);
	//HOOK_EVENT("events/game/teleport.sc",		EV_Teleport);

	HOOK_EVENT("events/pm/fall.sc",				EV_PM_Fall);
	HOOK_EVENT("events/pm/longjump.sc",			EV_PM_Longjump);
}

//-----------------------------------------------------------------------------
// Purpose: Since we cannot invoke events from client side by index (HL BUGBUG?)
//			It is possible to call event functions directly using this wrapper
// Input  : flags - 
//			clientindex - 
//			(*EventFunc) - event function itself
//			ducking - is the invoker ducking?
//			all other parameters are same as usual
//-----------------------------------------------------------------------------
void CL_PlaybackEventDirect(int flags, int clientindex, void (*EventFunc)(struct event_args_s *args), int ducking, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
	//conprintf(0, "CL_PlaybackEventDirect(flags %d, clientindex %d)\n", flags, clientindex);
	if (EventFunc && (flags & FEV_CLIENT))
	{
		event_args_t eargs;
		eargs.flags = flags;
		eargs.entindex = clientindex;
		VectorCopy(origin, eargs.origin);
		VectorCopy(angles, eargs.angles);
		//VectorCopy(velocity, eargs.velocity);
		VectorClear(eargs.velocity);
		eargs.ducking = ducking;
		eargs.fparam1 = fparam1;
		eargs.fparam2 = fparam2;
		eargs.iparam1 = iparam1;
		eargs.iparam2 = iparam2;
		eargs.bparam1 = bparam1;
		eargs.bparam2 = bparam2;
		EventFunc(&eargs);
	}
}
