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

//
// pm_shared.h
//
#if !defined( PM_SHAREDH )
#define PM_SHAREDH
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#define PHYSENT_NONE	-1// for TraceLines

// HACK: XDM3035: in server dlls: error C2732: linkage specification contradiction
//#ifdef CLIENT_DLL
void PM_InitMaterials(const char *mapname);
void PM_Init(struct playermove_s *ppmove);
void PM_Move(struct playermove_s *ppmove, int server);
char PM_FindTextureType(const char *name);
char PM_FindTextureTypeFull(const char *fullname);
//int PM_MapTextureTypeStepType(const char &chTextureType);
int PM_GetVisEntInfo(int ent);
int PM_GetPhysEntInfo(int ent);
int PM_GetPhysent(int entindex);
struct pmtrace_s PM_PlayerTraceEx(const vec3_t &start, const vec3_t &end, int traceFlags, int usehull, int (*pfnIgnore)(struct physent_s *pe));
struct pmtrace_s *PM_TraceLineEx(const vec3_t &start, const vec3_t &end, int flags, int usehull, int (*pfnIgnore)(struct physent_s *pe));

int PM_ClipVelocity(const vec3_t &in, const vec3_t &normal, float *out, const float &overbounce);// XDM3038c
//#endif

// HLBUG: Operators for SetGroupTrace
#define GROUP_OP_DISABLE	0// XDM3038c: THIS WAS 'AND' AND WRONG! Engine chacks if (groupop){} first!!!
#define GROUP_OP_NAND		1// skip collision when !(entity->groupinfo & other->groupinfo) - with all others
#define GROUP_OP_AND		2// skip collision when (entity->groupinfo & other->groupinfo) // this can be actually any number other than 0 and 1

// Spectator Movement modes (stored in pev->iuser1, so the physics code can get at them)
enum observer_mode_e
{
	OBS_NONE = 0,
	OBS_CHASE_LOCKED,	// 1 use target origin and angles // usual "in-world" modes
	OBS_CHASE_FREE,		// 2 use target origin and camera angles
	OBS_ROAMING,		// 3 use camera origin and camera angles
	OBS_IN_EYE,			// 4 use target view
	OBS_MAP_FREE,		// 5 roaming on a map // fullscreen inset modes
	OBS_MAP_CHASE,		// 6 chase_free on a map
	OBS_INTERMISSION	// 7 intermission camera: focus on the winner
};

// XDM 3034: moved here all usable stuff
// Ducking time
#define TIME_TO_DUCK			0.4

// HACK: XDM 3034: in server dlls these should be vectors (float[3])

#define DUCK_HULL_MIN			-18
#define DUCK_HULL_MAX			18
#define DUCK_VIEW				12
#define HULL_MIN				-36
#define HULL_MAX				36
#define HULL_RADIUS				16
#define PLAYER_HEIGHT			HULL_MAX - HULL_MIN// 72


// XDM3037a: GetHullBounds, SetTraceHull, playermove_s::usehull
enum HULLINDEX
{
	HULL_PLAYER_STANDING = 0,
	HULL_PLAYER_CROUCHING,
	HULL_POINT,
	HULL_3// XDM3038: engine asks for it (o_O)
};

#define VEC_VIEW				32// XDM3035
#define VEC_HANDS_OFFSET		24// default vertical offset from origin for player

// XDM3035c
#define VEC_HULL_MIN			Vector(-HULL_RADIUS,-HULL_RADIUS, HULL_MIN)
#define VEC_HULL_MAX			Vector( HULL_RADIUS, HULL_RADIUS, HULL_MAX)
#define VEC_HUMAN_HULL_MIN		Vector(-HULL_RADIUS,-HULL_RADIUS, 0)
#define VEC_HUMAN_HULL_MAX		Vector( HULL_RADIUS, HULL_RADIUS, PLAYER_HEIGHT)// 72
//#define VEC_HUMAN_HULL_DUCK		Vector( HULL_RADIUS, HULL_RADIUS, DUCK_HULL_MAX-DUCK_HULL_MIN)// 36
#define VEC_DUCK_HULL_MIN		Vector(-HULL_RADIUS,-HULL_RADIUS, DUCK_HULL_MIN)
#define VEC_DUCK_HULL_MAX		Vector( HULL_RADIUS, HULL_RADIUS, DUCK_HULL_MAX)
#define VEC_DUCK_VIEW			Vector(  0,  0, DUCK_VIEW)
#define VEC_VIEW_OFFSET			Vector(  0,  0, VEC_VIEW)
#define VEC_HULL_MIN_3			Vector(-32,-32,-32)// XDM3038
#define VEC_HULL_MAX_3			Vector( 32, 32, 32)
#define VEC_HULL_ITEM_MIN		Vector(-20,-20, 0)// XDM3038c: item touch/trigger box
#define VEC_HULL_ITEM_MAX		Vector( 20, 20, 20)// XDM3038c
#define VEC_HULL_ITEM_TALL_MAX	Vector( 20, 20, 40)// XDM3038c


//#define PM_DEAD_VIEWHEIGHT	-8
#define MAX_CLIMB_SPEED			128
#define STUCK_MOVEUP			1
#define STUCK_MOVEDOWN			-1
#define	STOP_EPSILON			0.1

#define PLAYER_FATAL_FALL_SPEED			1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED		580// approx 20 feet
#define PLAYER_CONSTANT_FALL_DAGAMGE	10.0f// non-realistic option
#define DAMAGE_FOR_FALL_SPEED			(100.0f/(PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED))// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED			(PLAYER_MAX_SAFE_FALL_SPEED*0.3f)// ~174
// XDM: don't mix these!
#define PLAYER_FALL_PUNCH_THRESHHOLD	(float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.
#define PLAYER_LONGJUMP_SPEED			350 // how fast we longjump

#define PLAYER_DUCKING_MULTIPLIER		0.333f// HL20130901
#define PLAYER_MAX_WALK_SPEED			220// otherwise player is considered running
#define PLAYER_DAMAGE_FORCE_MULTIPLIER	3.0f

#define	CONTENTS_CURRENT_0		-9
#define	CONTENTS_CURRENT_90		-10
#define	CONTENTS_CURRENT_180	-11
#define	CONTENTS_CURRENT_270	-12
#define	CONTENTS_CURRENT_UP		-13
#define	CONTENTS_CURRENT_DOWN	-14
#define CONTENTS_TRANSLUCENT	-15

// XDM: from player.h

// Player PHYSICS FLAGS bits
#define	PFLAG_ONLADDER			(1<<0)
#define	PFLAG_ONSWING			(1<<0)
#define	PFLAG_ONTRAIN			(1<<1)
#define	PFLAG_ONBARNACLE		(1<<2)
#define	PFLAG_DUCKING			(1<<3)		// In the process of ducking, but totally squatted yet
#define	PFLAG_USING				(1<<4)		// Using a continuous entity
#define PFLAG_OBSERVER			(1<<5)		// player is locked in stationary cam mode. Spectators can move, observers can't.

// XDM: from monsters.h

// Hit Group standards
#define	HITGROUP_GENERIC		0
#define	HITGROUP_HEAD			1
#define	HITGROUP_CHEST			2
#define	HITGROUP_STOMACH		3
#define HITGROUP_LEFTARM		4
#define HITGROUP_RIGHTARM		5
#define HITGROUP_LEFTLEG		6
#define HITGROUP_RIGHTLEG		7
#define HITGROUP_ARMOR			10// XDM

// SetPhysicsKeyValue
#define PHYSKEY_DEFAULT			"hl"// what does it mean anyway?
#define PHYSKEY_LONGJUMP		"slj"
#define PHYSKEY_IGNORELADDER	"il"

extern struct playermove_s *pmove;// XDM3038c: careful! This is for client AND server

#endif
