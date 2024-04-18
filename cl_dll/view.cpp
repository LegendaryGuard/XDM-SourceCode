//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "hud.h"
#include "cl_util.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "util_vector.h"
#include "event_api.h"
#include "bench.h"
#include "hltv.h"
#include "r_studioint.h"
#include "com_model.h"
#include "eventscripts.h"
#include "studio_util.h"
#include "cl_fx.h"
#include "Exports.h"
#include "kbutton.h"

//const float g_viewheight = VEC_VIEW;

extern Vector vJumpOrigin;
extern Vector vJumpAngles;

/*
The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.
*/

#define	CAM_MODE_RELAX			1
#define CAM_MODE_FOCUS			2
#define CAM_SIDE_ANGLE			22.5f
#define CAM_FRONT_ANGLE			180.0f

//#define IDLESCALE_NORMAL		0.5f// XDM3035
#define IDLESCALE_IDLE			0.0f// XDM3037: generates too much traffic
#define IDLESCALE_MOVING		1.0f
#define IDLESCALE_INWATER		2.0f// * per level
#define IDLESCALE_INTERMISSION	1.0f

Vector g_vecViewOrigin;// Current view point (including camera modes)
Vector g_vecViewAngles;// Current view angles (including camera modes)
Vector g_vecViewForward;// These can be calculated from angles
Vector g_vecViewRight;
Vector g_vecViewUp;

// BAD IDEA! VALVe engine deletes it every frame! ref_params_t *g_pRefParams = NULL;

int		g_iDrawCycle = 0;// XDM3037a
int		v_cameraMode = CAM_MODE_FOCUS;
int		v_resetCamera = 1;
float	v_frametime;
vec_t	v_lastDistance;
float	v_cameraRelaxAngle	= 5.0f;
float	v_cameraFocusAngle	= 35.0f;
float	v_idlescale;  // used by TFC for concussion grenade effect
vec3_t	v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles, v_cam_angles, ev_punchangle;
short	g_ThirdPersonView = false;

cvar_t	*cl_bobcycle = NULL;
cvar_t	*cl_bob = NULL;
cvar_t	*cl_bobup = NULL;
cvar_t	*cl_waterdist = NULL;
cvar_t	*cl_chasedist = NULL;
cvar_t	*cl_chaseclip = NULL;
cvar_t	*cl_chaseang = NULL;// XDM3035
cvar_t	*v_centermove = NULL;
cvar_t	*v_centerspeed = NULL;
cvar_t	*v_ipitch_level = NULL;// XDM3038c
cvar_t	*v_iyaw_level = NULL;
cvar_t	*v_iroll_level = NULL;

// These cvars are not registered (so users can't cheat), so set the ->value field directly
// Register these cvars in V_Init() if needed for easy tweaking
cvar_t	v_ipitch_cycle		= {"v_ipitch_cycle",	"1.0",	FCVAR_CLIENTDLL, 1.0f, NULL};
cvar_t	v_iyaw_cycle		= {"v_iyaw_cycle",		"2.0",	FCVAR_CLIENTDLL, 2.0f, NULL};
cvar_t	v_iroll_cycle		= {"v_iroll_cycle",		"0.5",	FCVAR_CLIENTDLL, 0.5f, NULL};
cvar_t	cv_ipitch_level		= {"v_ipitch_level",	"0.3",	FCVAR_CLIENTDLL | FCVAR_CHEAT, 0.3f, NULL};
cvar_t	cv_iyaw_level		= {"v_iyaw_level",		"0.3",	FCVAR_CLIENTDLL | FCVAR_CHEAT, 0.3f, NULL};
cvar_t	cv_iroll_level		= {"v_iroll_level",		"0.3",	FCVAR_CLIENTDLL | FCVAR_CHEAT, 0.3f, NULL};

// Decrease punch angle over time
static void V_DropPunchAngle(float &frametime, Vector &punchangle)
{
	float len = VectorNormalize(punchangle);
	len -= (10.0f + len * 0.5f) * frametime;
	len = max(len, 0.0f);
	punchangle *= len;
}

// Client-side punch effect
void V_PunchAxis(int axis, float angle)
{
#if !defined(CORRECT_PITCH)
	if (axis == PITCH)// SQB only affects pitch
		ev_punchangle[axis] = -angle;// TODO FIXME UNDONE TESTME ????????????????
	else
#endif
		ev_punchangle[axis] = angle;
}

// XDM3038c: centralized place, like GetEyePosition()
float V_GetViewHeight(cl_entity_t *ent)
{
	if (ent)
	{
		if (ent->curstate.solid == SOLID_NOT)
			return 0.0f;
		else if (ent->curstate.usehull == HULL_PLAYER_CROUCHING)
			return DUCK_VIEW;
		else if (ent->curstate.usehull == HULL_PLAYER_STANDING)
			return VEC_VIEW;// exacty eye position can't be caluculated since it depends on client values like cl_bobcycle, this offset matches the default values
	}
	return 0.0f;
}

static float MaxAngleBetweenAngles(float *a1, float *a2)
{
	float d, maxd = 0.0f;
	NormalizeAngles(a1);
	NormalizeAngles(a2);
	for (short i = 0 ; i < 3 ; ++i)
	{
		d = a2[i] - a1[i];
		NormalizeAngle180(&d);
		d = fabs(d);
		if (d > maxd)
			maxd=d;
	}
	return maxd;
}

// Quakeworld bob code, this fixes jitters in the mutliplayer since the clock (pparams->time) isn't quite linear
static float V_CalcBob(struct ref_params_s *pparams)
{
	static double bobtime;
	static float bob;
	static float lasttime;
	float cycle;
	vec3_t vel;
	
	if (pparams->onground == -1 || pparams->time == lasttime )
		return bob;// just use old value

	lasttime = pparams->time;

	bobtime += pparams->frametime;
	cycle = bobtime - (int)(bobtime / cl_bobcycle->value);// * cl_bobcycle->value;
	// WTF!? cycle /= cl_bobcycle->value;

	if (cycle < cl_bobup->value)
		cycle = M_PI * cycle / cl_bobup->value;
	else
		cycle = M_PI + M_PI * ( cycle - cl_bobup->value )/( 1.0f - cl_bobup->value );

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)
	VectorCopy( pparams->simvel, vel );
	vel[2] = 0;

	bob = sqrt(vel[0] * vel[0] + vel[1] * vel[1]) * cl_bob->value * (0.3f + 0.7f * (float)sin(cycle));// XDM3038a
	bob = clamp(bob, -7.0f, 4.0f);
	return bob;
}

/*
===============
V_CalcRoll
Used by view and sv_user
===============
*/
static float V_CalcRoll(vec3_t angles, vec3_t velocity, float &rollangle, float &rollspeed)
{
    float   sign;
    float   side;
    float   value;
	vec3_t  right;
	AngleVectors(angles, NULL, right, NULL);
	side = DotProduct (velocity, right);
    sign = side < 0 ? -1.0f : 1.0f;
    side = fabs( side );

	value = rollangle;
    if (side < rollspeed)
		side = side * value / rollspeed;
    else
		side = value;

	return side * sign;
}

typedef struct pitchdrift_s
{
	float		pitchvel;
	int			nodrift;
	float		driftmove;
	double		laststop;
} pitchdrift_t;

static pitchdrift_t pd;

void V_StartPitchDrift(void)
{
	if ( pd.laststop == gEngfuncs.GetClientTime() )
		return;		// something else is keeping it from drifting

	if ( pd.nodrift || !pd.pitchvel )
	{
		pd.pitchvel = v_centerspeed->value;
		pd.nodrift = 0;
		pd.driftmove = 0;
	}
}

void V_StopPitchDrift(void)
{
	pd.laststop = gEngfuncs.GetClientTime();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
static void V_DriftPitch( struct ref_params_s *pparams )
{
	float		delta, move;

	if ( gEngfuncs.IsNoClipping() || !pparams->onground || pparams->demoplayback || pparams->spectator )
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	// don't count small mouse motion
	if (pd.nodrift)
	{
		if ( v_centermove->value > 0 && !(in_mlook.state & 1) )
		{		
			// this is for lazy players. if they stopped, looked around and then continued
			// to move the view will be centered automatically if they move more than
			// v_centermove units. 

			if ( fabs( pparams->cmd->forwardmove ) < cl_forwardspeed->value )
				pd.driftmove = 0;
			else
				pd.driftmove += pparams->frametime;
		
			if ( pd.driftmove > v_centermove->value)
				V_StartPitchDrift ();
			else
				return;	// player didn't move enough
		}
		return;	// don't drift view
	}
	
	delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if (!delta)
	{
		pd.pitchvel = 0;
		return;
	}

	move = pparams->frametime * pd.pitchvel;
	//pd.pitchvel += pparams->frametime * v_centerspeed->value;
	pd.pitchvel *= (1.0f+(pparams->frametime*0.25f)); // get faster by time

	if (delta > 0)
	{
		if (move > delta)
		{
			pd.pitchvel = 0;
			move = delta;
		}
		pparams->cl_viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			pd.pitchvel = 0;
			move = -delta;
		}
		pparams->cl_viewangles[PITCH] -= move;
	}
}

/* 
============================================================================== 
						VIEW RENDERING 
============================================================================== 
*/ 

/*
==================
V_CalcGunAngle

Calculate angles for a weapon view model (pev->viewmodel)
==================
*/
static void V_CalcGunAngle(struct ref_params_s *pparams, cl_entity_t *viewent)
{	
	if (!viewent)
		return;

#if defined(CORRECT_PITCH)
	viewent->angles[PITCH]	= pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25f;
#else
	viewent->angles[PITCH]	= -pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25f;
#endif
	viewent->angles[YAW]	= pparams->viewangles[YAW] + pparams->crosshairangle[YAW];
	viewent->angles[ROLL]	= pparams->viewangles[ROLL] + pparams->crosshairangle[ROLL];

	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH]	-= v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * (v_ipitch_level->value * 0.5f);
	viewent->angles[YAW]	-= v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level->value;
	viewent->angles[ROLL]	-= v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level->value;

	viewent->curstate.angles = viewent->angles;
	viewent->latched.prevangles = viewent->angles;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
static void V_AddIdle(struct ref_params_s *pparams)
{
	pparams->viewangles[PITCH]	+= v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * v_ipitch_level->value;
	pparams->viewangles[YAW]	+= v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level->value;
	pparams->viewangles[ROLL]	+= v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level->value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
static void V_CalcViewRoll ( struct ref_params_s *pparams )
{
	cl_entity_t *viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
	if ( !viewentity )
		return;

	float side = V_CalcRoll ( viewentity->angles, pparams->simvel, pparams->movevars->rollangle, pparams->movevars->rollspeed );

	if (pparams->health > 0)// XDM3035
		pparams->viewangles[ROLL] += side;

	/*// XDM3035a
	if (pparams->health <= 0 && ( pparams->viewheight[2] != 0 ))
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero 
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[ROLL] = 80;	// dead view angle
		return;
	}*/
}














/*
==================
V_CalcRefdef

==================
*/

#define ORIGIN_BACKUP 64
#define ORIGIN_MASK ( ORIGIN_BACKUP - 1 )

typedef struct 
{
	float Origins[ ORIGIN_BACKUP ][3];
	float OriginTime[ ORIGIN_BACKUP ];
	float Angles[ ORIGIN_BACKUP ][3];
	float AngleTime[ ORIGIN_BACKUP ];
	int CurrentOrigin;
	int CurrentAngle;
} viewinterp_t;


static viewinterp_t ViewInterp;
static float oldz = 0.0f;
static float lasttime = 0.0f;



// Get the origin of the Observer based around the target's position and angles
void V_GetChaseOrigin(const vec3_t &angles, const vec3_t &origin, const float &distance, float *returnvec)
{
	Vector	vecEnd;
	Vector	forward;
	// Trace back from the target using the player's view angles
	AngleVectors(angles, forward, NULL, NULL);
	Vector vecStart(origin);
	VectorMA(vecStart, -distance, forward, vecEnd);
	if (cl_chaseclip->value <= 0 || gEngfuncs.IsNoClipping())// XDM
	{
		VectorCopy(vecEnd, returnvec);
		v_lastDistance = (origin - vecEnd).Length();
		return;
	}

	int maxLoops = 8;
	int ignoreent = -1;	// first, ignore no entity
	pmtrace_t *trace = NULL;
	cl_entity_t *ent = NULL;
	while (maxLoops > 0)
	{
		trace = gEngfuncs.PM_TraceLine(vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent);
		// WARNING! trace->ent is the number in physent list not the normal entity number
		if (trace->ent <= 0)
			break;	// we hit the world or nothing, stop trace

		ent = gEngfuncs.GetEntityByIndex(PM_GetPhysEntInfo(trace->ent));

		if (ent == NULL)
			break;

		// hit non-player solid BSP , stop here
		if (ent->curstate.solid == SOLID_BSP && !ent->player)
			break;

		// if close enough to end pos, stop, otherwise continue trace
		//if (Distance(trace->endpos, vecEnd) < 1.0f)
		if ((vecEnd - trace->endpos).Length() < 1.0f)
		{
			break;
		}
		else
		{
			ignoreent = trace->ent;	// ignore last hit entity
			VectorCopy(trace->endpos, vecStart);
		}
		--maxLoops;
	}  
	//if (ent)
	//	gEngfuncs.Con_Printf("Trace loops %i , entity %i, model %s, solid %i\n",(8-maxLoops),ent->curstate.number, ent->model->name , ent->curstate.solid ); 

	if (trace)
	{
		VectorMA(trace->endpos, 4, trace->plane.normal, returnvec);
		//v_lastDistance = Distance(trace->endpos, origin);	// real distance without offset
		v_lastDistance = (origin - trace->endpos).Length();	// real distance without offset
	}
}


void V_GetSingleTargetCam(cl_entity_t *ent1, float *angle, float *origin, int flags)
{
	float newAngle[3];
	float newOrigin[3]; 
	//int flags = gHUD.m_Spectator.m_iObserverFlags;
	float dfactor = (flags & DRC_FLAG_DRAMATIC)? -1.0f : 1.0f;
	float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;
	// see is target is a dead player
	bool deadPlayer = (ent1->player > 0) && (ent1->curstate.health <= 0 || ent1->curstate.solid == SOLID_NOT);

	// go away in final scenes or if player just died
	/*if (flags & DRC_FLAG_FINAL)
		distance *= 2.0f;	
	else */if (deadPlayer)
		distance *= 1.5f;	

	// let v_lastDistance float smoothly away
	v_lastDistance += v_frametime * 32.0f;	// move unit per seconds back

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if (ent1->player)
	{
		if (deadPlayer)  
			newOrigin[2] += 2.0f;	//laying on ground
		else
			newOrigin[2] += 17.0f; // head level of living player // g_viewheight?
	}
	else
		newOrigin[2] += 8.0f;	// object, tricky, must be above bomb in CS

	// we have no second target, choose view direction based on
	// show front of primary target
	VectorCopy(ent1->angles, newAngle);

	// if final scene (bomb), show from real high pos
	if (flags & DRC_FLAG_FINAL)
		newAngle[PITCH] = CAM_SIDE_ANGLE;
	else
		newAngle[PITCH] += 12.5f * dfactor; // lower angle if dramatic

	// show from front, normal players back
	if (flags & DRC_FLAG_FACEPLAYER)
	{
		newAngle[YAW] += CAM_FRONT_ANGLE;
		//newAngle[YAW] *= cl_test1->value;
		//newAngle[YAW] += cl_test2->value;
	}
	else if (flags & DRC_FLAG_SIDE)// choose side of object/player
		newAngle[YAW] += CAM_SIDE_ANGLE;
	else
		newAngle[YAW] -= CAM_SIDE_ANGLE;

	V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 120.0f, v_frametime);
/*nonono
	// show from front, normal players back
	if (flags & DRC_FLAG_FACEPLAYER)
		angle[YAW] += CAM_FRONT_ANGLE;
	else if (flags & DRC_FLAG_SIDE)// choose side of object/player
		angle[YAW] += CAM_SIDE_ANGLE;
	else
		angle[YAW] -= CAM_SIDE_ANGLE;
*/
	// HACK, if player is dead don't clip against his dead body, can't check this
	V_GetChaseOrigin(angle, newOrigin, distance, origin);
}

void V_GetDoubleTargetsCam(cl_entity_t *ent1, cl_entity_t *ent2, float *angle, float *origin, int flags)
{
	float dfactor  = (flags & DRC_FLAG_DRAMATIC)? -1.0f : 1.0f;
	float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;
	float newAngle[3]; float newOrigin[3];// float tempVec[3];

	// go away in final scenes or if player just died
	if (flags & DRC_FLAG_FINAL)
		distance *= 2.0f;	

	// let v_lastDistance float smoothly away
	v_lastDistance += v_frametime * 32.0f;	// move unit per seconds back

	if (distance > v_lastDistance)
		distance = v_lastDistance;

	VectorCopy(ent1->origin, newOrigin);

	if (ent1->player)
		newOrigin[2] += 17.0f;// head level of living player
	else
		newOrigin[2] += 8.0f;// object, tricky, must be above bomb in CS

	// get new angle towards second target
	VectorSubtract(ent2->origin, ent1->origin, newAngle);

	VectorAngles(newAngle, newAngle);
#if !defined (CORRECT_PITCH)
	newAngle[PITCH] = -newAngle[0];
#endif

	// set angle diffrent in Dramtaic scenes
	newAngle[PITCH] += 12.5f * dfactor; // lower angle if dramatic

	if (flags & DRC_FLAG_SIDE)
		newAngle[YAW] += CAM_SIDE_ANGLE;
	else
		newAngle[YAW] -= CAM_SIDE_ANGLE;

	float d = MaxAngleBetweenAngles(v_lastAngles, newAngle);

	if ((d < v_cameraFocusAngle) && (v_cameraMode == CAM_MODE_RELAX))
		VectorCopy(v_lastAngles, newAngle);// difference is to small and we are in relax camera mode, keep viewangles
	else if ((d < v_cameraRelaxAngle) && (v_cameraMode == CAM_MODE_FOCUS))
		v_cameraMode = CAM_MODE_RELAX;// we catched up with our target, relax again
	else
		v_cameraMode = CAM_MODE_FOCUS;// target move too far away, focus camera again

	// and smooth view, if not a scene cut
	if (v_resetCamera || v_cameraMode == CAM_MODE_RELAX)
		VectorCopy(newAngle, angle);
	else
		V_SmoothInterpolateAngles(v_lastAngles, newAngle, angle, 180.0f, v_frametime);

	V_GetChaseOrigin(newAngle, newOrigin, distance, origin);

	// move position up, if very close at target
	if (v_lastDistance < 64.0f)
		origin[2] += 16.0f*(1.0f - (v_lastDistance / 64.0f));
/* ?
	// calculate angle to second target
	VectorSubtract(ent2->origin, origin, tempVec);
	VectorAngles(tempVec, tempVec);
#if !defined (CORRECT_PITCH)
	tempVec[0] = -tempVec[0];
#endif
*/
	/* take middle between two viewangles
	InterpolateAngles( newAngle, tempVec, newAngle, 0.5f); */
}

void V_GetDirectedChasePosition(cl_entity_t	*ent1, cl_entity_t *ent2, float *angle, float *origin, int flags)
{
	if (v_resetCamera)
	{
		v_lastDistance = 4096.0f;
		// v_cameraMode = CAM_MODE_FOCUS;
	}

	if ((ent2 == (cl_entity_t*)0xFFFFFFFF) || (ent1->player && (ent1->curstate.solid == SOLID_NOT)))
	{
		// we have no second target or player just died
		V_GetSingleTargetCam(ent1, angle, origin, flags);
	}
	else if (ent2)
	{
		// keep both target in view
		V_GetDoubleTargetsCam(ent1, ent2, angle, origin, flags);
	}
	else
	{
		// second target disappeard somehow (dead)
		// keep last good viewangle
		float newOrigin[3];
		//int flags 	   = gHUD.m_Spectator.m_iObserverFlags;
		float dfactor  = (gHUD.m_Spectator.m_iObserverFlags & DRC_FLAG_DRAMATIC)? -1.0f : 1.0f;
		float distance = 112.0f + (16.0f * dfactor); // get close if dramatic;
	
		// go away in final scenes or if player just died
		if (gHUD.m_Spectator.m_iObserverFlags & DRC_FLAG_FINAL)
			distance *= 2.0f;	
	
		// let v_lastDistance float smoothly away
		v_lastDistance += v_frametime * 32.0f;	// move unit per seconds back

		if (distance > v_lastDistance)
			distance = v_lastDistance;
		
		VectorCopy(ent1->origin, newOrigin);

		if (ent1->player)
			newOrigin[2] += 17.0f;// head level of living player
		else
			newOrigin[2] += 8.0f;// object, tricky, must be above bomb in CS

		V_GetChaseOrigin(angle, newOrigin, distance, origin);
	}
	VectorCopy(angle, v_lastAngles);
}

void V_GetChasePos(cl_entity_t *ent, float *cl_angles, float *origin, float *angles, int flags)
{
	if (!ent)
	{
		// just copy a save in-map position
		VectorCopy(vJumpAngles, angles);
		VectorCopy(vJumpOrigin, origin);
		return;
	}

	if (gHUD.IsSpectator() && gHUD.m_Spectator.GetAutoDirector())// XDM3035: or pparams->spectator ?
	{
		if (gHUD.m_pLocalPlayer && gHUD.m_pLocalPlayer->curstate.iuser3)// XDM3038a: TESTME!
			V_GetDirectedChasePosition(ent, gEngfuncs.GetEntityByIndex(gHUD.m_pLocalPlayer->curstate.iuser3), angles, origin, flags);
		else
			V_GetDirectedChasePosition(ent, (cl_entity_t *)0xFFFFFFFF, angles, origin, flags);
	}
	else
	{
			/*Vector tmp;
			StringToVec(cl_chaseang->string, tmp);// SLOW!!
			VectorAdd(angles, tmp, angles);
			NormalizeAngles(angles);*/

		/*if (cl_angles == NULL)// no mouse angles given, use entity angles ( locked mode )
		{
			VectorCopy(ent->origin, origin);
			VectorCopy(ent->angles, angles);
?#if !defined (CORRECT_PITCH)
			angles[0] *= -1;
#endif?
		}
		else*/
		{
			VectorCopy(ent->origin, origin);
			origin[2] += V_GetViewHeight(ent); // DEFAULT_VIEWHEIGHT - some offset

			if (cl_angles == NULL)
			{
				VectorCopy(ent->angles, angles);
//#if !defined (CORRECT_PITCH)?				angles[0] *= -1;
			}
			else
				VectorCopy(cl_angles, angles);

			//if (gHUD.m_iCameraMode == CAM_MODE_LOCKED)// not for intermission, etc
			{
				if (!(flags & DRC_FLAG_FINAL))// don't modify during intermission
					angles[YAW] += cl_chaseang->value;

				V_GetChaseOrigin(angles, origin, cl_chasedist->value, origin);
				//V_GetChasePos -< g_iUser2, v_cl_angles, v_origin, v_angles, gHUD.m_Spectator.m_iObserverFlags);
				//V_GetSingleTargetCam(ent, angles, origin, flags | DRC_FLAG_FACEPLAYER);
			}
		}
	}
	v_resetCamera = false;	
}

void V_ResetChaseCam(void)
{
	v_resetCamera = true;
}

void V_GetInEyePos(int target, float *origin, float *angles)
{
	if (target == 0)// just copy a save in-map position
	{
		VectorCopy(vJumpAngles, angles);
		VectorCopy(vJumpOrigin, origin);
		return;
	}

	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(target);
	if (ent == NULL)
		return;

	VectorCopy(ent->origin, origin);
	VectorCopy(ent->angles, angles);
	angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;// see CL_ProcessEntityUpdate() // SQB
	origin[2] += V_GetViewHeight(ent);// XDM3038c
}

void V_GetMapFreePosition(float *cl_angles, float *origin, float *angles)
{
	vec3_t forward;
	vec3_t zScaledTarget;

	VectorCopy(cl_angles, angles);
	// modify angles since we don't wanna see map's bottom
	angles[0] = 51.25f + 38.75f*(angles[PITCH]/90.0f);

	zScaledTarget[0] = gHUD.m_Spectator.m_mapOrigin[0];
	zScaledTarget[1] = gHUD.m_Spectator.m_mapOrigin[1];
	zScaledTarget[2] = gHUD.m_Spectator.m_mapOrigin[2] * ((90.0f - angles[PITCH]) / 90.0f);

	AngleVectors(angles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(zScaledTarget, -(4096.0f / gHUD.m_Spectator.m_mapZoom), forward, origin);
}

void V_GetMapChasePosition(int target, float *cl_angles, float *origin, float *angles)
{
	vec3_t forward;
	if (target)
	{
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex(target);
		if (gHUD.m_Spectator.GetAutoDirector())
		{
			// this is done to get the angles made by director mode
			V_GetChasePos(ent, cl_angles, origin, angles, 0);
			VectorCopy(ent->origin, origin);
			// keep fix chase angle horizontal
			angles[PITCH] = 45.0f;
		}
		else
		{
			VectorCopy(cl_angles, angles);
			VectorCopy(ent->origin, origin);
			// modify angles since we don't wanna see map's bottom
			angles[PITCH] = 51.25f + 38.75f*(angles[0]/90.0f);
		}
	}
	else
	{
		// keep out roaming position, but modify angles
		VectorCopy(cl_angles, angles);
		angles[PITCH] = 51.25f + 38.75f*(angles[0]/90.0f);
	}

	origin[2] *= ((90.0f - angles[PITCH]) / 90.0f);
	angles[ROLL] = 0.0f;// don't roll angle (if chased player is dead)
	AngleVectors(angles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(origin, -1536.0f, forward, origin); 
}

// XDM: TESTME! untested!
int V_FindViewModelByWeaponModel(int weaponindex)
{
	model_t *weaponModel = IEngineStudio.GetModelByIndex(weaponindex);
	if (weaponModel)
	{
		char vmodel[MAX_MODEL_NAME];
		strcpy(vmodel, weaponModel->name);
		char *found = strstr(vmodel, "/p_");
		if (found)
		{
			*(found+sizeof(char)) = 'v';
			return gEngfuncs.pEventAPI->EV_FindModelIndex(vmodel);
		}
		/*size_t len = strlen(weaponModel->name);
		int i = 0;
		while (modelmap[i] != NULL)
		{
			if (!strnicmp( weaponModel->name, modelmap[i][0], len))
				return gEngfuncs.pEventAPI->EV_FindModelIndex(modelmap[i][1]);

			++i;
		}*/
	}
	return 0;
}














/*
==================
V_CalcNormalRefdef

  Normal first person view mode
==================
*/
void V_CalcNormalRefdef(struct ref_params_s *pparams)
{
//	conprintf(1, "V_CalcNormalRefdef() start: vangles: (%g %g %g) cl (%g %g %g)\n", pparams->viewangles[0], pparams->viewangles[1], pparams->viewangles[2], pparams->cl_viewangles[0], pparams->cl_viewangles[1], pparams->cl_viewangles[2]);
	static float bob, waterOffset;// XDM3035: static
	int i = 0;
	vec3_t angles;//, camAngles, camForward, camRight, camUp;
	g_ThirdPersonView = false;
	if (!gEngfuncs.IsNoClipping())// XDM3037: no camera movement while noclipping
	{
		if (pparams->waterlevel >= WATERLEVEL_WAIST)
		{
			v_idlescale = (float)pparams->waterlevel*IDLESCALE_INWATER;// XDM3035a
		}
		else// error C2181: illegal else without matching if - WTF?!
		{
			if (fabs(pparams->cmd->forwardmove) < pparams->movevars->maxspeed*0.75f)
				v_idlescale = IDLESCALE_IDLE;
			else
				v_idlescale = IDLESCALE_MOVING;// XDM3035: no one resets this
		}
	}

	// transform the view offset by the model's matrix to get the offset from model origin for the view
	bob = V_CalcBob(pparams);

	//pparams->simorg[2] += 4.0f;// XDM3035: HACK? TESTME!

	// refresh position
	VectorCopy(pparams->simorg, pparams->vieworg);
	pparams->vieworg[2] += (bob);
	VectorAdd(pparams->vieworg, pparams->viewheight, pparams->vieworg);
	VectorCopy(pparams->cl_viewangles, pparams->viewangles);

	gEngfuncs.V_CalcShake();
	// XDM3038a	gEngfuncs.V_ApplyShake(pparams->vieworg, pparams->viewangles, 1.0);

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	pparams->vieworg[0] += 1.0f/32.0f;
	pparams->vieworg[1] += 1.0f/32.0f;
	pparams->vieworg[2] += 1.0f/32.0f;

	// Check for problems around water, move the viewer artificially if necessary 
	// -- this prevents drawing errors in GL due to waves
	cl_entity_t *pwater;
	waterOffset = 0;
	if (pparams->waterlevel >= WATERLEVEL_WAIST)
	{
		int contents, waterDist, waterEntity;
		vec3_t point;
		waterDist = (int)cl_waterdist->value;

		if (pparams->hardware)
		{
			waterEntity = gEngfuncs.PM_WaterEntity( pparams->simorg );
			if ( waterEntity >= 0 && waterEntity < pparams->max_entities )
			{
				pwater = gEngfuncs.GetEntityByIndex( waterEntity );
				if (pwater && (pwater->model != NULL))
					waterDist += (int)(pwater->curstate.scale * 16.0f);	// Add in wave height
			}
		}
		else
			waterEntity = 0;	// Don't need this in software

		VectorCopy(pparams->vieworg, point);

		// Eyes are above water, make sure we're above the waves
		if (pparams->waterlevel == WATERLEVEL_WAIST)
		{
			point[2] -= waterDist;
			for (i = 0; i < waterDist; ++i)
			{
				contents = gEngfuncs.PM_PointContents( point, NULL );
				if (contents > CONTENTS_WATER)
					break;
				point[2] += 1;
			}
			waterOffset = (point[2] + waterDist) - pparams->vieworg[2];
		}
		else// eyes are under water.  Make sure we're far enough under
		{
			point[2] += waterDist;

			for (i = 0; i < waterDist; ++i)
			{
				contents = gEngfuncs.PM_PointContents( point, NULL );
				if ( contents <= CONTENTS_WATER )
					break;
				point[2] -= 1;
			}
			waterOffset = (point[2] - waterDist) - pparams->vieworg[2];
		}
	}

	pparams->vieworg[2] += waterOffset;
	V_CalcViewRoll(pparams);
	V_AddIdle(pparams);

	cl_entity_t *viewmodel = gEngfuncs.GetViewModel();
	// offsets
	VectorCopy(pparams->cl_viewangles, angles);
	AngleVectors(angles, pparams->forward, pparams->right, pparams->up);
	// Give gun our viewangles
	if (viewmodel)
	{
	VectorCopy(pparams->cl_viewangles, viewmodel->angles);
	// set up gun position
	V_CalcGunAngle(pparams, viewmodel);

	// Use predicted origin as view origin.
	VectorCopy(pparams->simorg, viewmodel->origin);
	viewmodel->origin[2] += (waterOffset);
	VectorAdd(viewmodel->origin, pparams->viewheight, viewmodel->origin);

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake(viewmodel->origin, viewmodel->angles, 0.9f);

	for (i = 0; i < 3; ++i)
		viewmodel->origin[i] += bob * 0.4f * pparams->forward[i];

	viewmodel->origin[2] += bob;

	// throw in a little tilt.
	viewmodel->angles[PITCH] -= bob * 0.3f;
	viewmodel->angles[YAW]   -= bob * 0.5f;
	viewmodel->angles[ROLL]  -= bob;// * 1;

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	viewmodel->origin[2] -= 1.0f;

	// fudge position around to keep amount of weapon visible roughly equal with different FOV
	if (pparams->viewsize >= 110)// XDM3035: was == in all ifs
		viewmodel->origin[2] += 1;
	else if (pparams->viewsize >= 100)
		viewmodel->origin[2] += 2;
	else if (pparams->viewsize >= 90)
		viewmodel->origin[2] += 1;
	else if (pparams->viewsize >= 80)
		viewmodel->origin[2] += 0.5f;
	}
	// Add in the punchangle, if any
	VectorAdd(pparams->viewangles, pparams->punchangle, pparams->viewangles);
	// Include client side punch, too
	VectorAdd(pparams->viewangles, (float *)&ev_punchangle, pparams->viewangles);

	if (gHUD.m_iPaused == 0)// XDM3035b
		V_DropPunchAngle(pparams->frametime, ev_punchangle);

	// smooth out stair step ups
#if 1
	if (!pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0)
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if (steptime < 0)
	//FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 150.0f;
		if (oldz > pparams->simorg[2])
			oldz = pparams->simorg[2];
		if (pparams->simorg[2] - oldz > 18)
			oldz = pparams->simorg[2]- 18;
		pparams->vieworg[2] += oldz - pparams->simorg[2];
		if (viewmodel)
			viewmodel->origin[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}
#endif

	{
		static float lastorg[3];
		vec3_t delta;
		VectorSubtract(pparams->simorg, lastorg, delta);
		if (Length(delta) != 0.0f)
		{
			VectorCopy( pparams->simorg, ViewInterp.Origins[ ViewInterp.CurrentOrigin & ORIGIN_MASK ] );
			ViewInterp.OriginTime[ ViewInterp.CurrentOrigin & ORIGIN_MASK ] = pparams->time;
			ViewInterp.CurrentOrigin++;
			VectorCopy(pparams->simorg, lastorg);
		}
	}

	// Smooth out whole view in multiplayer when on trains, lifts
	if (cl_vsmoothing && cl_vsmoothing->value > 0 && (pparams->smoothing && (pparams->maxclients > 1)))
	{
		if (cl_vsmoothing->value < 0.0f)
			gEngfuncs.Cvar_SetValue("cl_vsmoothing", 0.0f);

		int foundidx = 0;
		float t = pparams->time - cl_vsmoothing->value;

		for (i = 1; i < ORIGIN_MASK; ++i)
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;
			if (ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ] <= t)
				break;
		}

		if (i < ORIGIN_MASK &&  ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ] != 0.0f)
		{
			// Interpolate
			double frac;
			double dt;
			vec3_t delta;
			vec3_t neworg;

			dt = ViewInterp.OriginTime[ (foundidx + 1) & ORIGIN_MASK ] - ViewInterp.OriginTime[ foundidx & ORIGIN_MASK ];
			if ( dt > 0.0 )
			{
				frac = ( t - ViewInterp.OriginTime[ foundidx & ORIGIN_MASK] ) / dt;
				frac = min( 1.0, frac );
				VectorSubtract( ViewInterp.Origins[ ( foundidx + 1 ) & ORIGIN_MASK ], ViewInterp.Origins[ foundidx & ORIGIN_MASK ], delta );
				VectorMA( ViewInterp.Origins[ foundidx & ORIGIN_MASK ], frac, delta, neworg );

				// Dont interpolate large changes
				if (Length(delta) < 64)
				{
					VectorSubtract(neworg, pparams->simorg, delta);
					VectorAdd(pparams->simorg, delta, pparams->simorg);
					VectorAdd(pparams->vieworg, delta, pparams->vieworg);
					if (viewmodel)
						VectorAdd(viewmodel->origin, delta, viewmodel->origin);
				}
			}
		}
	}

	cl_entity_t *viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
	if (viewentity)
	{
		if (viewmodel)// XDM3037
		{
			viewmodel->curstate.rendermode = viewentity->curstate.rendermode;
			viewmodel->curstate.renderamt = viewentity->curstate.renderamt;
			viewmodel->curstate.rendercolor = viewentity->curstate.rendercolor;
			viewmodel->curstate.renderfx = viewentity->curstate.renderfx;
		}
		if (pparams->viewentity > pparams->maxclients)// override all previous settings if the viewent isn't the client
		{
			//VectorCopy(viewentity->origin, pparams->vieworg);
			// Entities don't have view_ofs, so we replace player's VO with viewent VO on the server
			VectorAdd(viewentity->origin, pparams->viewheight, pparams->vieworg);// XDM3037
			VectorCopy(viewentity->angles, pparams->viewangles);
			// Store off overridden viewangles
	// done V look down \V/	VectorCopy(pparams->viewangles, v_angles);
		}
	}

	// Store off v_angles before munging for third person
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->viewangles, v_lastAngles);
	VectorCopy(pparams->cl_viewangles, v_cl_angles);// keep old user mouse angles !

	lasttime = pparams->time;
	VectorCopy(pparams->vieworg, v_origin);
}


/*
==================
V_CalcIntermissionRefdef

  XDM: moved here after needed functions have been declared
==================
*/
void V_CalcIntermissionRefdef(struct ref_params_s *pparams)
{
	VectorCopy(pparams->simorg, v_sim_org);
	VectorCopy(pparams->cl_viewangles, v_cl_angles);
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->vieworg, v_origin);

	v_frametime = pparams->frametime;

	g_ThirdPersonView = true;
	//float old = v_idlescale;
	// view is the weapon model (only visible from inside body)
	cl_entity_t	*view = gEngfuncs.GetViewModel();
	if (view)
		view->model = NULL;

	// allways idle in intermission
	v_idlescale = IDLESCALE_INTERMISSION;

	if (g_iUser2 > 0)
	{
		cl_entity_t *ent1 = gEngfuncs.GetEntityByIndex(g_iUser2);// this is the winner of something
		if (g_iUser3 > 0)// keep both target in view
		{
			cl_entity_t *ent2 = gEngfuncs.GetEntityByIndex(g_iUser3);
			V_GetDirectedChasePosition(ent1, ent2, v_angles, v_origin, DRC_FLAG_FACEPLAYER|DRC_FLAG_FINAL);
			//V_GetDoubleTargetsCam(ent1, ent2, v_angles, v_origin);
		}
		else// we have no second target or player just died
		{
			V_GetDirectedChasePosition(ent1, (cl_entity_t *)0xFFFFFFFF, v_angles, v_origin, DRC_FLAG_FACEPLAYER|DRC_FLAG_FINAL);// XDM3038c: this version saves last angles
			//V_GetSingleTargetCam(ent1, v_angles, v_origin, DRC_FLAG_FACEPLAYER|DRC_FLAG_FINAL);
		}
	}
	// write new values into pparams
	VectorCopy(v_cl_angles, pparams->cl_viewangles);
	VectorCopy(v_angles, pparams->viewangles);
	VectorCopy(v_origin, pparams->vieworg);
}


/*
==================
V_CalcSpectatorRefdef

Draw what spectator sees
==================
*/
void V_CalcSpectatorRefdef(struct ref_params_s *pparams)
{
	static int lastWeaponModelIndex = 0;
	static int lastViewModelIndex = 0;
	static vec3_t velocity(0,0,0);
		
	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(g_iUser2);

	pparams->onlyClientDraw = false;

	// refresh position
	VectorCopy(pparams->simorg, v_sim_org);
	// get old values
	VectorCopy(pparams->cl_viewangles, v_cl_angles);
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->vieworg, v_origin);

	if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_iInsetMode == INSET_IN_EYE) && ent)
	{
		// calculate player velocity
		float timeDiff = ent->curstate.msg_time - ent->prevstate.msg_time;
		if (timeDiff > 0.0f)
		{
			Vector distance(ent->prevstate.origin);
			distance -= ent->curstate.origin;//VectorSubtract(ent->prevstate.origin, ent->curstate.origin, distance);
			distance /= timeDiff;//VectorScale(distance, 1/timeDiff, distance);
			velocity[0] = velocity[0]*0.9f + distance[0]*0.1f;
			velocity[1] = velocity[1]*0.9f + distance[1]*0.1f;
			velocity[2] = velocity[2]*0.9f + distance[2]*0.1f;
			VectorCopy(velocity, pparams->simvel);
		}

		// predict missing client data and set weapon model ( in HLTV mode or inset in eye mode )
		if (gEngfuncs.IsSpectateOnly())
		{
			V_GetInEyePos(g_iUser2, pparams->simorg, pparams->cl_viewangles);

			pparams->health = 1;
			cl_entity_t	*gunModel = gEngfuncs.GetViewModel();
			if (lastWeaponModelIndex != ent->curstate.weaponmodel)
			{
				// weapon model changed
				lastWeaponModelIndex = ent->curstate.weaponmodel;
				lastViewModelIndex = V_FindViewModelByWeaponModel(lastWeaponModelIndex);
				if (lastViewModelIndex)
				{
					gEngfuncs.pfnWeaponAnim(0,0);	// reset weapon animation
				}
				else// model not found
				{
					if (gunModel)
						gunModel->model = NULL;	// disable weapon model
					lastWeaponModelIndex = lastViewModelIndex = 0;
				}
			}
			if (gunModel)
			{
				if (lastViewModelIndex)
				{
					gunModel->model = IEngineStudio.GetModelByIndex( lastViewModelIndex );
					gunModel->curstate.modelindex = lastViewModelIndex;
					gunModel->curstate.frame = 0;
					gunModel->curstate.colormap = 0; 
					gunModel->index = g_iUser2;
				}
				else
					gunModel->model = NULL;	// disable weaopn model
			}
		}
		else
		{
			//VectorCopy(ent->origin, origin);// XDM: wtf?
			// only get viewangles from entity
			VectorCopy(ent->angles, pparams->cl_viewangles);
			pparams->cl_viewangles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;// see CL_ProcessEntityUpdate() // SQB
		}
	}

	v_frametime = pparams->frametime;

	// first renderer cycle, full screen
	if (g_iUser1 == OBS_CHASE_LOCKED)
	{
		g_ThirdPersonView = true;
		V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), NULL, v_origin, v_angles, gHUD.m_Spectator.m_iObserverFlags);
	}
	else if (g_iUser1 == OBS_CHASE_FREE)
	{
		g_ThirdPersonView = true;
		V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), v_cl_angles, v_origin, v_angles, gHUD.m_Spectator.m_iObserverFlags);
	}
	else if (g_iUser1 == OBS_ROAMING)
	{
		g_ThirdPersonView = false;
		v_angles = v_cl_angles;
		v_origin = v_sim_org;
		// override values if director is active
		gHUD.m_Spectator.GetDirectorCamera(v_origin, v_angles);
	}
	else if (g_iUser1 == OBS_IN_EYE)
	{
		V_CalcNormalRefdef(pparams);// inside:		g_ThirdPersonView = false;
	}
	else if (g_iUser1 == OBS_MAP_FREE)
	{
		g_ThirdPersonView = false;
		pparams->onlyClientDraw = true;
		V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
	}
	else if (g_iUser1 == OBS_MAP_CHASE)
	{
		g_ThirdPersonView = true;
		pparams->onlyClientDraw = true;
		V_GetMapChasePosition(g_iUser2, v_cl_angles, v_origin, v_angles);
	}
	gHUD.m_Spectator.m_OverviewMapDrawPass = (pparams->onlyClientDraw > 0);// XDM3037a

	// write back new values into pparams
	VectorCopy(v_cl_angles, pparams->cl_viewangles);
	VectorCopy(v_angles, pparams->viewangles);
	VectorCopy(v_origin, pparams->vieworg);
}


/*
==================
V_CalcThirdPersonRefdef

  XDM: simplified version for non-spectators and deathcam
==================
*/
//static struct model_s *savedplayermodel;// XDM
void V_CalcThirdPersonRefdef(struct ref_params_s *pparams)
{
//	CON_DPRINTF("V_CalcThirdPersonRefdef() start: vangles: (%g %g %g) cl (%g %g %g)\n", pparams->viewangles[0], pparams->viewangles[1], pparams->viewangles[2], pparams->cl_viewangles[0], pparams->cl_viewangles[1], pparams->cl_viewangles[2]);
	g_ThirdPersonView = true;
	cl_entity_t *ent = gEngfuncs.GetLocalPlayer();// EntityByIndex(pparams->viewentity)
#if defined (_DEBUG)
	ASSERT(ent != NULL);
#endif

	VectorCopy(pparams->simorg, v_sim_org);
	VectorCopy(pparams->cl_viewangles, v_cl_angles);
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->vieworg, v_origin);
	v_frametime = pparams->frametime;

	if (pparams->health > 0)
	{
		if (gHUD.m_iCameraMode == CAM_MODE_LOCKED)
			V_GetChasePos(ent, v_cl_angles, v_origin, v_angles, DRC_FLAG_NO_RANDOM);
		else if (gHUD.m_iCameraMode == CAM_MODE_ROTATE)
			V_GetChasePos(ent, v_cl_angles, v_origin, v_angles, DRC_FLAG_NO_RANDOM);
		else if (gHUD.m_iCameraMode == CAM_MODE_FREE)
		{
			v_angles = v_cl_angles;
			v_origin = v_sim_org;
		}
	}
	else// death cam
		V_GetDirectedChasePosition(ent, (cl_entity_t *)0xFFFFFFFF, v_angles, v_origin, gHUD.m_Spectator.m_iObserverFlags);

	// write new values into pparams
	//TODO: use cl_testN t oswitch everything if (gHUD.m_iCameraMode != CAM_MODE_ROTATE)
	//if (test1->value > 0)
		VectorCopy(v_cl_angles, pparams->cl_viewangles);// goes to entity
	//if (test2->value > 0)
		VectorCopy(v_angles, pparams->viewangles);// goes to camera/screen
	//if (test3->value > 0)
		VectorCopy(v_origin, pparams->vieworg);

	if (gHUD.m_iPaused == 0)// XDM3038c
		V_DropPunchAngle(pparams->frametime, ev_punchangle);

	//conprintf(1, "V_CalcThirdPersonRefdef() finish: vangles: (%g %g %g) cl (%g %g %g)\n", pparams->viewangles[0], pparams->viewangles[1], pparams->viewangles[2], pparams->cl_viewangles[0], pparams->cl_viewangles[1], pparams->cl_viewangles[2]);
}














static short g_ViewPassedSkyBox = 0;// 0 - hide viewmodel and draw skybox, 1 - restore viewmodel
static short g_ViewPassedWorld = 0;
static short g_ViewPassedOverview = 0;
//static short g_ViewPassedSpectator = 0;

static struct model_s *savedviewmodel = NULL;// XDM
//vec3_t v_forward;

/*
==================
V_CalcRefdefSky

  XDM: global function for drawing sky only, must be passed twice
==================
*/
void V_CalcRefdefSky(ref_params_t *pparams)
{
	cl_entity_t *view = gEngfuncs.GetViewModel();// view is the weapon model (only visible from inside body )
	pparams->onlyClientDraw = false;
	if (g_ViewPassedSkyBox == 0)// first time, draw only sky
	{
		savedviewmodel = view->model;
		if (g_iUser1 == OBS_CHASE_LOCKED)
		{
			g_ThirdPersonView = true;
			V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), NULL, pparams->vieworg, pparams->viewangles, gHUD.m_Spectator.m_iObserverFlags);
		}
		else
			V_CalcNormalRefdef(pparams);

		VectorCopy(gHUD.m_vecSkyPos, pparams->vieworg);
		view->model = NULL;
	}
	else// restore params after the sky has been drawn
	{
		view->model = savedviewmodel;
		//VectorCopy(v_origin, pparams->vieworg);
		//VectorCopy(v_angles, pparams->viewangles);
		//V_DriftPitch(pparams);
	}
	++g_ViewPassedSkyBox;
}

/*
==================
V_CalcRefdefWorld

  XDM: global function for drawing real world refdef
==================
*/
void V_CalcRefdefWorld(ref_params_t *pparams)
{
//	g_iDrawCycle = 0;

	if (gHUD.m_iFogMode > 0)
		RenderFog(0,0,0,0,0,true);

	pparams->onlyClientDraw = false;
	gEngfuncs.V_CalcShake();

	if (pparams->intermission)// intermission / finale rendering
	{	
		V_CalcIntermissionRefdef(pparams);
	}
	else if (pparams->spectator || gHUD.IsSpectator())// spectator mode
	{
		V_CalcSpectatorRefdef(pparams);
	}
	else// normal view mode
	{
		if (g_pCvarDeathView->value && CL_IsDead() && (pparams->viewentity == gEngfuncs.GetLocalPlayer()->index))// death cam
			V_CalcThirdPersonRefdef(pparams);
		else if (gHUD.m_iCameraMode > 0)// user-enabled cam
			V_CalcThirdPersonRefdef(pparams);
		else
			V_CalcNormalRefdef(pparams);
	}

	// XDM: for external use
	VectorCopy(pparams->vieworg, g_vecViewOrigin);
	//if (g_ThirdPersonView)// XDM3035a: test
	//	VectorCopy(pparams->cl_viewangles, g_vecViewAngles);
	//else
		VectorCopy(pparams->viewangles, g_vecViewAngles);

	gEngfuncs.V_ApplyShake(pparams->vieworg, pparams->viewangles, 1.0);// XDM3038a: now apply some noise
	AngleVectors(pparams->viewangles, g_vecViewForward, g_vecViewRight, g_vecViewUp);

	//Vector ea;
	//gEngfuncs.GetViewAngles(ea);
	//conprintf(1, "VA: %g %g %g\t\tEA: %g %g %g\n", g_vecViewAngles.x, g_vecViewAngles.y, g_vecViewAngles.z, ea.x, ea.y, ea.z);

	//g_pRefParams = pparams;
	++g_ViewPassedWorld;
}

/*
==================
V_CalcRefdefWorld

  XDM: global function for drawing the overview window refdef
==================
*/
void V_CalcRefdefOverview(ref_params_t *pparams)
{
//	g_iDrawCycle = 1;

	VectorCopy(pparams->simorg, v_sim_org);
	VectorCopy(pparams->cl_viewangles, v_cl_angles);
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->vieworg, v_origin);

	//v_frametime = pparams->frametime;
	/*pparams->viewport[0] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowX);
	pparams->viewport[1] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowY);
	pparams->viewport[2] = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowWidth);
	pparams->viewport[3] = YRES(gHUD.m_Spectator.m_OverviewData.insetWindowHeight);*/
	gHUD.m_Spectator.GetInsetWindowBounds(&pparams->viewport[0],&pparams->viewport[1],&pparams->viewport[2],&pparams->viewport[3]);// XDM3038a

	if (!gHUD.IsSpectator())// not a spectator, must be minimap
	{
		int minimapmode = (int)gHUD.m_pCvarMiniMap->value;// XDM
		if (minimapmode == 2)// static
		{
			pparams->onlyClientDraw = true;
			V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
			//V_GetMapChasePosition(0, v_cl_angles, v_origin, v_angles);
		}
		else if (minimapmode == 1)// follow
		{
			pparams->onlyClientDraw = true;
			V_GetMapChasePosition(0, v_cl_angles, v_origin, v_angles);
			//V_GetMapChasePosition(g_iUser2, v_cl_angles, v_origin, v_angles);
		}
	}
	else
	{
		switch (gHUD.m_Spectator.m_iInsetMode)
		{
			case INSET_CHASE_FREE:
			{
				pparams->onlyClientDraw = false;
				//?	g_ThirdPersonView = true;
				V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), v_cl_angles, v_origin, v_angles, DRC_FLAG_NO_RANDOM);
				break;
			}
			case INSET_IN_EYE:
			{
				pparams->onlyClientDraw = false;
				g_ThirdPersonView = false;
				V_CalcNormalRefdef(pparams);
				break;
			}
			case INSET_ROAMING:// XDM3037a: follow spectator position and view
			{
				pparams->onlyClientDraw = false;
				g_ThirdPersonView = false;
				v_angles = v_cl_angles;
				v_origin = v_sim_org;
				// override values if director is active
				gHUD.m_Spectator.GetDirectorCamera(v_origin, v_angles);
				//V_CalcNormalRefdef(pparams);
				break;
			}
			case INSET_MAP_FREE:
			{
				pparams->onlyClientDraw = true;
				V_GetMapFreePosition(v_cl_angles, v_origin, v_angles);
				break;
			}
			case INSET_MAP_CHASE:
			{
				pparams->onlyClientDraw = true;
				if (g_iUser1 == OBS_ROAMING)
					V_GetMapChasePosition(0, v_cl_angles, v_origin, v_angles);
				else
					V_GetMapChasePosition(g_iUser2, v_cl_angles, v_origin, v_angles);

				break;
			}
		}
	}
	/*if (pparams->onlyClientDraw > 0) UNDONE
		pparams->fov = gHUD.GetUpdatedDefaultFOV();
	else
		pparams->fov = gHUD.GetCurrentFOV();*/

	gHUD.m_Spectator.m_OverviewMapDrawPass = (pparams->onlyClientDraw > 0);// XDM3037a

	// write back new values into pparams
	VectorCopy(v_origin, pparams->vieworg);
	VectorCopy(v_cl_angles, pparams->cl_viewangles);
	VectorCopy(v_angles, pparams->viewangles);
	//VectorCopy(v_angles, gHUD.m_Spectator.m_vecViewPortAngles);// XDM3035c

	g_ViewPassedOverview++;
}



/*
=============
V_CalcRefdef

  XDM: main
=============
*/
void CL_DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams )
{
	DBG_CL_PRINT("V_CalcRefdef(%d)\n", pparams->nextView);
//	RecClCalcRefdef(pparams);

//	g_pRefParams = pparams;
	//fail	if (pparams->nextView == 0)// once per frame!
	if (g_ViewPassedWorld == 0)
	{
		if (gHUD.m_iPaused != pparams->paused)
			gHUD.OnGamePaused(pparams->paused);
	}

	gHUD.m_Spectator.m_OverviewMapDrawPass = false;

	// first of all, we may want to draw the sky box
	//if (gHUD.m_iSkyMode > 0 || g_ViewPassedSkyBox == 1)
	if (gHUD.m_iSkyMode > 0 && /*g_ViewPassedSkyBox == 0 && */!gHUD.m_Spectator.OverviewIsFullscreen())//(g_iUser1 >= OBS_MAP_FREE && g_iUser1 <= OBS_MAP_CHASE))// XDM3037a: and NOT going to draw fullscreen MAP
	{
		V_CalcRefdefSky(pparams);// <- need to get there twice
		if (g_ViewPassedSkyBox == 1)
		{
			pparams->nextView = 1;
			return;// return after first sky pass
		}
	}

	// obsolete
	// next, we may either draw the fullscreen map or the world
	// WARNING: we assume there can be ONLY ONE OVERVIEW pass every frame, not two!
	/*short iSpecPass = gHUD.m_Spectator.GetOverviewDrawCycle();
	if (gHUD.m_Spectator.ShouldDrawOverview() && iSpecPass == 0 && g_ViewPassedOverview == 0)
	{
		V_CalcRefdefOverview(pparams);// XDM: need to fix flashlight after this!!!
		pparams->nextView = 1;
		if (g_ViewPassedOverview == 1)
			return;// return after first overview pass
	}*/

	// after drawing the sky, we want to draw the fullscreen world
	if (g_ViewPassedWorld == 0)
	{
		g_iDrawCycle = 0;
		V_CalcRefdefWorld(pparams);// this draws the world OR the fullscreen map for spectator
		pparams->nextView = 0;// try to finish the frame, don't return;
	}

	// after drawing the world (or map), we MAY want to draw the windowed minimap/overview
	if (gHUD.m_Spectator.ShouldDrawOverviewWindow() && g_ViewPassedWorld == 1)// && iSpecPass == 1)
	{
		if (g_ViewPassedOverview == 0)// world has just been drawn, not the overview
		{
			g_ViewPassedOverview++;
			pparams->nextView = 1;// return here in the next pass to really draw the overview
			return;
		}
		else if (g_ViewPassedOverview == 1)// we returned after drawing the world
		{
			V_CalcRefdefOverview(pparams);// this draws the world OR the windowed map for spectator
			// XDM: need to fix flashlight after this!!!
			pparams->nextView = 0;// finally, finish the frame
			// UNDONE: draw 3D skybox here! (if world is in the window)
		}
		/*else if (g_ViewPassedOverview == 2)
		{
			pparams->onlyClientDraw = true;// don't draw ANYTHING, just restore angles
			VectorCopy(g_vecViewAngles, pparams->viewangles);
			pparams->nextView = 0;
		}*/
	}

	if (pparams->nextView == 0)
	{
		g_ViewPassedSkyBox = 0;
		g_ViewPassedWorld = 0;
		g_ViewPassedOverview = 0;
		//g_ViewPassedSpectator = 0;
	}
}










void CMD_ThirdPerson(void)
{
	if (CL_IsThirdPersonAllowed())
		gHUD.m_iCameraMode = CAM_MODE_LOCKED;
}

void CMD_FirstPerson(void)
{
	gHUD.m_iCameraMode = CAM_MODE_OFF;
}

void CMD_CamToggle(void)
{
	if (gHUD.m_iCameraMode != CAM_MODE_OFF)
		gHUD.m_iCameraMode = CAM_MODE_OFF;
	else
	{
		if (CL_IsThirdPersonAllowed())
			gHUD.m_iCameraMode = CAM_MODE_LOCKED;
	}
}

void CMD_CamMode(void)
{
	if (CL_IsThirdPersonAllowed())
	{
		gHUD.m_iCameraMode = atoi(CMD_ARGV(1));
		if (gHUD.m_iCameraMode < CAM_MODE_OFF)
			gHUD.m_iCameraMode = CAM_MODE_OFF;
		else if (gHUD.m_iCameraMode > CAM_MODE_FREE)
			gHUD.m_iCameraMode = CAM_MODE_FREE;
	}
}


/*
=============
V_Init
=============
*/
void V_Init(void)
{
	DBG_CL_PRINT("V_Init()\n");
	ADD_DOMMAND("centerview", V_StartPitchDrift);
	ADD_DOMMAND("thirdperson", CMD_ThirdPerson);
	ADD_DOMMAND("firstperson", CMD_FirstPerson);
	ADD_DOMMAND("cam_toggle", CMD_CamToggle);
	ADD_DOMMAND("cam_mode", CMD_CamMode);

	v_centermove		= CVAR_CREATE("v_centermove", "0.15", FCVAR_CLIENTDLL);
	v_centerspeed		= CVAR_CREATE("v_centerspeed","500", FCVAR_CLIENTDLL);
	cl_bob				= CVAR_CREATE("cl_bob","0.01", FCVAR_CLIENTDLL | FCVAR_CHEAT);// best default for my experimental gun wag (sjb)
	cl_bobcycle			= CVAR_CREATE("cl_bobcycle","0.8", FCVAR_CLIENTDLL | FCVAR_CHEAT);// best default for my experimental gun wag (sjb)
	cl_bobup			= CVAR_CREATE("cl_bobup","0.5", FCVAR_CLIENTDLL | FCVAR_CHEAT);
	cl_chaseclip		= CVAR_CREATE("cl_chaseclip","1", FCVAR_CLIENTDLL | FCVAR_CHEAT);
	cl_chasedist		= CVAR_CREATE("cl_chasedist","112", FCVAR_CLIENTDLL | FCVAR_CHEAT);
	cl_chaseang			= CVAR_CREATE("cl_chaseang","0", FCVAR_CLIENTDLL | FCVAR_CHEAT);
	cl_waterdist		= CVAR_CREATE("cl_waterdist","4", FCVAR_CLIENTDLL | FCVAR_CHEAT);

	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0)//CVAR_GET_FLOAT("developer") > 0)
	{
		v_ipitch_level		= CVAR_CREATE("v_ipitch_level",		"0.3",	FCVAR_CLIENTDLL | FCVAR_CHEAT);
		v_iyaw_level		= CVAR_CREATE("v_iyaw_level",		"0.3",	FCVAR_CLIENTDLL | FCVAR_CHEAT);
		v_iroll_level		= CVAR_CREATE("v_iroll_level",		"0.3",	FCVAR_CLIENTDLL | FCVAR_CHEAT);
	}
	else
	{
		v_ipitch_level		= &cv_ipitch_level;
		v_iyaw_level		= &cv_iyaw_level;
		v_iroll_level		= &cv_iroll_level;
	}
}
