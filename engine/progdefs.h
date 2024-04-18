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
#ifndef PROGDEFS_H
#define PROGDEFS_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

// XDM3035c _s
typedef struct globalvars_s
{
	float		time;
	float		frametime;
	float		force_retouch;
	string_t	mapname;
	string_t	startspot;
	float		deathmatch;
	float		coop;
	float		teamplay;
	/*float*/uint32		serverflags;// XDM3038c: HACK! WARNING! It is a float in the engine and its size must not change!
	float		found_secrets;
	vec3_t		v_forward;
	vec3_t		v_up;
	vec3_t		v_right;
	float		trace_allsolid;
	float		trace_startsolid;
	float		trace_fraction;
	vec3_t		trace_endpos;
	vec3_t		trace_plane_normal;
	float		trace_plane_dist;
	edict_t		*trace_ent;
	float		trace_inopen;
	float		trace_inwater;
	int			trace_hitgroup;
	int			trace_flags;
	int			msg_entity;
	int			cdAudioTrack;
	int			maxClients;
	int			maxEntities;
	const char	*pStringBase;

	void		*pSaveData;
	vec3_t		vecLandmarkOffset;
} globalvars_t;

// NOTE: comments are mostly accurate, but not necessary complete, it is safer to assume these vars are heavily used by the engine and are very likely to mess up things if misused
typedef struct entvars_s
{
	string_t	classname;		// XDM3038: DO NOT SET THIS MANUALLY!! Use only SetClassName()!
	string_t	globalname;		// NOTENGINE?

	vec3_t		origin;			// ENGINE: 
	vec3_t		oldorigin;		// NOTENGINE?
	vec3_t		velocity;		// ENGINE: 
	vec3_t		basevelocity;	// ENGINE: conveyors
	vec3_t      clbasevelocity;	// ENGINE: Base velocity that was passed in to server physics so client can predict conveyors correctly. Server zeroes it, so we need to store here, too.
	vec3_t		movedir;		// ENGINE: used by movement code

	vec3_t		angles;			// ENGINE: Model angles
	vec3_t		avelocity;		// ENGINE: angle velocity (degrees per second)
	vec3_t		punchangle;		// ENGINE: copied to/from pmove, auto-decaying view angle adjustment
	vec3_t		v_angle;		// ENGINE: viewing angle (player), offset for MOVETYPE_FOLLOW

	// For parametric entities
	vec3_t		endpos;			// ENGINE: 
	vec3_t		startpos;		// ENGINE: 
	float		impacttime;		// ENGINE: 
	float		starttime;		// ENGINE: 

	int			fixangle;		// ENGINE: 0:nothing, 1:force view angles, 2:add avelocity
	float		idealpitch;		// ENGINE: used by ChangePitch
	float		pitch_speed;	// ENGINE: used by ChangePitch
	float		ideal_yaw;		// ENGINE: used by ChangeYaw
	float		yaw_speed;		// ENGINE: used by ChangeYaw

	int			modelindex;
	string_t	model;			// ENGINE: entity model, can be studio, sprite, bsp

	string_t	viewmodel;		// CLENGINE: player's viewmodel XDM3037: explicit type
	string_t	weaponmodel;	// CLENGINE: what other players see XDM3037: explicit type

	vec3_t		absmin;			// ENGINE: BB max translated to world coord
	vec3_t		absmax;			// ENGINE: BB max translated to world coord
	vec3_t		mins;			// ENGINE: local BB min
	vec3_t		maxs;			// ENGINE: local BB max
	vec3_t		size;			// ENGINE: maxs - mins

	float		ltime;			// ENGINE: 
	float		nextthink;		// ENGINE: 

	int			movetype;		// ENGINE: 
	int			solid;			// ENGINE: 

	int			skin;			// ENGINE: studio model texturegroup or brush model CONTENTS and buoyancy
	int			body;			// sub-model selection for studiomodels
	int 		effects;		// ENGINE: 

	float		gravity;		// ENGINE: % of "normal" gravity
	float		friction;		// ENGINE: inverse elasticity of MOVETYPE_BOUNCE

	int			light_level;	// CLENGINE? not sure if written by the engine

	int			sequence;		// ENGINE: animation sequence
	int			gaitsequence;	// CLENGINE: movement animation sequence for player (255 to disable)
	float		frame;			// CLENGINE? % playback position in animation sequences (0..255) - not a real frame!
	float		animtime;		// ENGINE: world time when frame was set
	float		framerate;		// CLENGINE: animation playback rate (-8x to 8x)
	byte		controller[4];	// ENGINE: bone controller setting (0..255)
	byte		blending[2];	// ENGINE: blending amount between sub-sequences (0..255)

	float		scale;			// ENGINE: sprite/model rendering scale (multiplier), water amplitude for brush models

	int			rendermode;		// ENGINE: kRender*
	float		renderamt;		// ENGINE: amount, actually an integer (0..255)
	vec3_t		rendercolor;	// ENGINE: RGB in byte format (0..255)
	int			renderfx;		// ENGINE: kRenderFx*

	float		health;			// ENGINE: detects dead players
	float		frags;			// ENGINE: used by PlayerInfo
	int			weapons;		// NOTENGINE? bit mask for available weapons
	float		takedamage;		// ENGINE:

	int			deadflag;		// ENGINE: detects dead players
	vec3_t		view_ofs;		// ENGINE: eye position offset (from origin), also used to calculate waterlevel

	int			button;			// ENGINE: from ucmd->buttons
	int			impulse;		// ENGINE: from ucmd->impulse

	edict_t		*chain;			// ENGINE: Entity pointer when linked into a linked list
	edict_t		*dmg_inflictor;	// NOTENGINE
	edict_t		*enemy;			// ENGINE: involved in movement
	edict_t		*aiment;		// ENGINE: entity pointer when MOVETYPE_FOLLOW
	edict_t		*owner;			// ENGINE: messes up tracelines
	edict_t		*groundentity;	// ENGINE: entity we are standing on

	int			spawnflags;		// ENGINE: SF_NOTINDEATHMATCH
	int			flags;			// ENGINE: used and set

	int			colormap;		// ENGINE: only gets reset by the engine, lowbyte topcolor, highbyte bottomcolor
	int			team;			// ENGINE: used for comparison

	float		max_health;		// NOTENGINE
	float		teleport_time;	// ENGINE: = pmove->waterjumptime
	float		armortype;		// NOTENGINE
	float		armorvalue;		// NOTENGINE
	int			waterlevel;		// ENGINE:
	int			watertype;		// ENGINE:

	string_t	target;			// NOTENGINE
	string_t	targetname;		// NOTENGINE
	string_t	netname;		// ENGINE:
	string_t	message;		// NOTENGINE

	float		dmg_take;		// NOTENGINE
	float		dmg_save;		// NOTENGINE
	float		dmg;			// ENGINE: used for water/lava damage
	float		dmgtime;		// ENGINE: used for water/lava damage

	string_t	noise;			// ENGINEREAD: Warning: these are FIELD_SOUNDNAME, don't use for models or sprites!
	string_t	noise1;			// ENGINEREAD: can be used for searching by string
	string_t	noise2;			// ENGINEREAD:
	string_t	noise3;			// ENGINEREAD:

	float		speed;			// ENGINE:
	float		air_finished;	// ENGINE: used for water/lava damage
	float		pain_finished;	// ENGINE: used for water/lava damage
	float		radsuit_finished;// ENGINE: used for water/lava damage

	edict_t		*pContainingEntity;// ENGINE: points back to edict_t which contains this instance of entvars_t

	int			playerclass;	// NOTENGINE??? what about the glass hack??
	float		maxspeed;		// ENGINE: set by SetClientMaxspeed

	float		fov;			// NOTENGINE?
	int			weaponanim;		// NOTENGINE

	int			pushmsec;		// ENGINE: network frame number

	int			bInDuck;		// ENGINE: copied from pmove
	int			flTimeStepSound;// ENGINE: copied from pmove
	int			flSwimTime;		// ENGINE: copied from pmove
	int			flDuckTime;		// ENGINE: copied from pmove
	int			iStepLeft;		// ENGINE: copied from pmove
	float		flFallVelocity;	// ENGINE: copied from pmove

	int			gamestate;		// NOTENGINE?

	int			oldbuttons;		// ENGINE: copied from usercmd

	int			groupinfo;		// ENGINE: used to selectively disable collisions

	// For mods
	int			iuser1;// observer (spectator) mode OBS_NONE
	int			iuser2;// observer primary target entindex
	int			iuser3;// observer secondary target entindex (optional)
	int			iuser4;
	float		fuser1;
	float		fuser2;
	float		fuser3;
	float		fuser4;
	vec3_t		vuser1;
	vec3_t		vuser2;
	vec3_t		vuser3;
	vec3_t		vuser4;
	edict_t		*euser1;
	edict_t		*euser2;// view entity
	edict_t		*euser3;
	edict_t		*euser4;// used by coop
} entvars_t;


#endif // PROGDEFS_H
