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
#ifndef CONST_H
#define CONST_H
//
// Constants shared by the engine and dlls
// This header file included by engine files and DLL files.
// Most came from server.h

// entvars_s::flags
// WARNING: Those which are marked with ENGINE: are used and/or changed by the engine and may affect lots of other things!
#define	FL_FLY					(1<<0)	// ENGINE: Changes the SV_Movestep() behavior to not need to be on ground
#define	FL_SWIM					(1<<1)	// ENGINE: Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
#define	FL_CONVEYOR				(1<<2)	// ENGINE: This is a conveyor which adds its velocity to other entities
#define	FL_CLIENT				(1<<3)	// ENGINE: A player, network client
#define	FL_INWATER				(1<<4)	// ENGINE: In CONTENTS_WATER, when this flag changes, pl_wade sound is played.
#define	FL_MONSTER				(1<<5)	// ENGINE: A monster, used by movement, PVS code, etc.
#define	FL_GODMODE				(1<<6)	// ENGINE: Does not take any damage, cheat.
#define	FL_NOTARGET				(1<<7)	// ENGINE: Is not visible for monsters, cheat.
#define	FL_SKIPLOCALHOST		(1<<8)	// ENGINE:? Don't send entity to local host, it's predicting this entity itself
#define	FL_ONGROUND				(1<<9)	// ENGINE: At rest / on the ground
#define	FL_PARTIALGROUND		(1<<10)	// ENGINE: not all corners are valid
#define	FL_WATERJUMP			(1<<11)	// ENGINE: player jumping out of water
#define FL_FROZEN				(1<<12) // ENGINE: Player is frozen for 3rd person camera
#define FL_FAKECLIENT			(1<<13)	// ENGINE: JAC: fake client, simulated server side; don't send network messages to them
#define FL_DUCKING				(1<<14)	// ENGINE: Player flag -- Player is fully crouched
#define FL_FLOAT				(1<<15)	// ENGINE: Apply floating force to this entity when in water
#define FL_GRAPHED				(1<<16) // worldgraph has this ent listed as something that blocks a connection
#define FL_IMMUNE_WATER			(1<<17) // ENGINE: Whoever left these three flags in the engine is a lazy bum
#define FL_IMMUNE_SLIME			(1<<18) // ENGINE: Because of these pev->dmg gets corrupted!
#define FL_IMMUNE_LAVA			(1<<19) // ENGINE: And to prevent that, try to always set these ON!
#define FL_PROXY				(1<<20)	// ENGINE: This is a spectator proxy
#define FL_ALWAYSTHINK			(1<<21)	// ENGINE: Brush model flag -- call think every frame regardless of nextthink - ltime (for constantly changing velocity/path)
#define FL_BASEVELOCITY			(1<<22)	// ENGINE: Base velocity has been applied this frame (used to convert base velocity into momentum)
#define FL_MONSTERCLIP			(1<<23)	// ENGINE: Only collide in with monsters who have FL_MONSTERCLIP set
#define FL_ONTRAIN				(1<<24) // Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
#define FL_WORLDBRUSH			(1<<25)	// ENGINE: Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something), blocks all tracelines.
#define FL_SPECTATOR			(1<<26) // ENGINE: This client is a spectator, don't run touch functions, etc.
#define FL_DRAW_ALWAYS			(1<<27) // XDM: Draw entity even when it is not in PVS XDM3038c: had to change this value because of engine conflicts
#define FL_HIGHLIGHT			(1<<28) // XDM: Clients use this for highlighting XDM3038c: same here
#define FL_CUSTOMENTITY			(1<<29)	// ENGINE: This is a custom entity
#define FL_KILLME				(1<<30)	// ENGINE: This entity is marked for death -- This allows the engine to kill ents at the appropriate time
#define FL_DORMANT				(1<<31)	// ENGINE: Entity is dormant, no updates to client

// Engine edict_s::spawnflags
#define SF_NOTINDEATHMATCH		(1<<11)// 0x0800 // 2048	// Do not spawn when deathmatch and loading entities from a file

// Goes into globalvars_t::trace_flags
#define FTRACE_SIMPLEBOX		(1<<0)	// Traceline with a simple box
//#define FTRACE_IGNORE_GLASS		(1<<1)	// traceline will be ignored entities with rendermode != kRenderNormal

// pfnWalkMove modes
#define	WALKMOVE_NORMAL		0 // normal walkmove
#define WALKMOVE_WORLDONLY	1 // doesn't hit ANY entities, no matter what the solid type
#define WALKMOVE_CHECKONLY	2 // move, but don't touch triggers

// entvars_s::movetype values
#define	MOVETYPE_NONE			0		// never moves
//#define	MOVETYPE_ANGLENOCLIP	1
//#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// Player only - moving on the ground
#define	MOVETYPE_STEP			4		// gravity, special edge handling -- monsters use this
#define	MOVETYPE_FLY			5		// No gravity, but still collides with stuff
#define	MOVETYPE_TOSS			6		// gravity/collisions
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8		// No gravity, no collisions, still do velocity/avelocity
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10		// Just like Toss, but reflect velocity when contacting surfaces
#define MOVETYPE_BOUNCEMISSILE	11		// bounce w/o gravity
#define MOVETYPE_FOLLOW			12		// track movement of aiment
#define	MOVETYPE_PUSHSTEP		13		// BSP model that needs physics/world collisions (uses nearest hull for world collision)

// entvars_s::solid values
// NOTE: Some movetypes will cause collisions independent of SOLID_NOT/SOLID_TRIGGER when the entity moves
// SOLID only effects OTHER entities colliding with this one when they move - UGH!
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking. WARNING: use for brush models and don't let triggers intersect!
#define	SOLID_BBOX				2		// touch on edge, block. WARNING: requires model or will crash on TraceLine!
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground WARNING: requires model!
#define	SOLID_BSP				4		// bsp clip, touch on edge, block WARNING! Can ONLY be used with MOVETYPE_PUSH[STEP] 

// entvars_s::deadflag
#define	DEAD_NO					0		// alive
#define	DEAD_DYING				1		// playing death animation or still falling off of a ledge waiting to hit ground
#define	DEAD_DEAD				2		// dead. lying still.
#define DEAD_RESPAWNABLE		3
#define DEAD_DISCARDBODY		4

// entvars_s::takedamage
#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

// entvars_s::effects
#define	EF_BRIGHTFIELD			1	// swirling cloud of particles
#define	EF_MUZZLEFLASH 			2	// single frame ELIGHT on entity attachment 0
#define	EF_BRIGHTLIGHT 			4	// DLIGHT centered at entity origin
#define	EF_DIMLIGHT 			8	// player flashlight
#define EF_INVLIGHT				16	// get lighting from ceiling
#define EF_NOINTERP				32	// don't interpolate the next frame
#define EF_LIGHT				64	// rocket flare glow sprite
#define EF_NODRAW				128	// don't draw entity
#define EF_NIGHTVISION			256 // player nightvision // HL20130901
#define EF_SNIPERLASER			512 // sniper laser effect
#define EF_FIBERCAMERA			1024// fiber camera

#define EF_MERGE_VISIBILITY		(1<<29)	// this entity allowed to merge vis (e.g. env_sky or portal camera)

// entity_state_s::eflags
#define EFLAG_SLERP				1	// do studio interpolation of this entity
#define EFLAG_DRAW_ALWAYS		2	// XDM: same as FL_DRAW_ALWAYS
#define EFLAG_HIGHLIGHT			4	// XDM3035c: probably a hack...
#define EFLAG_ADDTOMAP			8	// XDM3038: explicit
#define EFLAG_GAMEGOAL			16	// XDM3038: special

// svc_temp_entity
//
// temp entity events
//
#define	TE_BEAMPOINTS		0		// beam effect between two points
// coord coord coord (start position) 
// coord coord coord (end position) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define	TE_BEAMENTPOINT		1		// beam effect between point and entity
// short (start entity) 
// coord coord coord (end position) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define	TE_GUNSHOT			2		// particle effect plus ricochet sound
// coord coord coord (position) 

#define	TE_EXPLOSION		3		// additive sprite, 2 dynamic lights, flickering particles, explosion sound, move vertically 8 pps
// coord coord coord (position) 
// short (sprite index)
// byte (scale in 0.1's)
// byte (framerate)
// byte (flags)
//
// The Explosion effect has some flags to control performance/aesthetic features:
#define TE_EXPLFLAG_NONE		0	// all flags clear makes default Half-Life explosion
#define TE_EXPLFLAG_NOADDITIVE	1	// sprite will be drawn opaque (ensure that the sprite you send is a non-additive sprite)
#define TE_EXPLFLAG_NODLIGHTS	2	// do not render dynamic lights
#define TE_EXPLFLAG_NOSOUND		4	// do not play client explosion sound
#define TE_EXPLFLAG_NOPARTICLES	8	// do not draw particles


#define	TE_TAREXPLOSION		4		// Quake1 "tarbaby" explosion with sound
// coord coord coord (position) 

#define	TE_SMOKE			5		// alphablend sprite, move vertically 30 pps
// coord coord coord (position) 
// short (sprite index)
// byte (scale in 0.1's)
// byte (framerate)

#define	TE_TRACER			6		// tracer effect from point to point
// coord, coord, coord (start) 
// coord, coord, coord (end)

#define	TE_LIGHTNING		7		// TE_BEAMPOINTS with simplified parameters
// coord, coord, coord (start) 
// coord, coord, coord (end) 
// byte (life in 0.1's) 
// byte (width in 0.1's) 
// byte (amplitude in 0.01's)
// short (sprite model index)

#define	TE_BEAMENTS			8		
// short (start entity) 
// short (end entity) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define	TE_SPARKS			9		// 8 random tracers with gravity, ricochet sprite
// coord coord coord (position) 

#define	TE_LAVASPLASH		10		// Quake1 lava splash
// coord coord coord (position) 

#define	TE_TELEPORT			11		// Quake1 teleport splash
// coord coord coord (position) 

#define TE_EXPLOSION2		12		// Quake1 colormaped (base palette) particle explosion with sound
// coord coord coord (position) 
// byte (starting color)
// byte (num colors)

#define TE_BSPDECAL			13		// Decal from the .BSP file 
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// short (texture index of precached decal texture name)
// short (entity index)
// [optional - only included if previous short is non-zero (not the world)] short (index of model of above entity)

#define TE_IMPLOSION		14		// tracers moving toward a point
// coord, coord, coord (position)
// byte (radius)
// byte (count)
// byte (life in 0.1's) 

#define TE_SPRITETRAIL		15		// line of moving glow sprites with gravity, fadeout, and collisions
// coord, coord, coord (start) 
// coord, coord, coord (end) 
// short (sprite index)
// byte (count)
// byte (life in 0.1's) 
// byte (scale in 0.1's) 
// byte (velocity along vector in 10's)
// byte (randomness of velocity in 10's)

#define TE_BEAM				16		// obsolete

#define TE_SPRITE			17		// additive sprite, plays 1 cycle
// coord, coord, coord (position) 
// short (sprite index) 
// byte (scale in 0.1's) 
// byte (brightness)

#define TE_BEAMSPRITE		18		// A beam with a sprite at the end
// coord, coord, coord (start position) 
// coord, coord, coord (end position) 
// short (beam sprite index) 
// short (end sprite index) 

#define TE_BEAMTORUS		19		// screen aligned beam ring, expands to max radius over lifetime
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_BEAMDISK			20		// disk that expands to max radius over lifetime
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_BEAMCYLINDER		21		// cylinder that expands to max radius over lifetime
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_BEAMFOLLOW		22		// create a line of decaying beam segments until entity stops moving
// short (entity:attachment to follow)
// short (sprite index)
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte,byte,byte (color)
// byte (brightness)

#define TE_GLOWSPRITE		23
// coord, coord, coord (pos)
// short (model index)
// byte (life /10)
// byte (scale / 10)
// byte (fade time / 10)

#define TE_BEAMRING			24		// connect a beam ring to two entities
// short (start entity) 
// short (end entity) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_STREAK_SPLASH	25		// oriented shower of tracers
// coord coord coord (start position) 
// coord coord coord (direction vector) 
// byte (color)
// short (count)
// short (base speed)
// short (ramdon velocity)

#define TE_BEAMHOSE			26		// obsolete

#define TE_DLIGHT			27		// dynamic light, effect world, minor entity effect
// coord, coord, coord (pos) 
// byte (radius in 10's) 
// byte byte byte (color)
// byte (brightness)
// byte (life in 10's)
// byte (decay rate in 10's)

#define TE_ELIGHT			28		// point entity light, no world effect
// short (entity:attachment to follow)
// coord coord coord (initial position) 
// coord (radius)
// byte byte byte (color)
// byte (life in 0.1's)
// coord (decay rate)

#define TE_TEXTMESSAGE		29
// short 1.2.13 x (-1 = center)
// short 1.2.13 y (-1 = center)
// byte Effect 0 = fade in/fade out
			// 1 is flickery credits
			// 2 is write out (training room)

// 4 bytes r,g,b,a color1	(text color)
// 4 bytes r,g,b,a color2	(effect color)
// ushort 8.8 fadein time
// ushort 8.8  fadeout time
// ushort 8.8 hold time
// optional ushort 8.8 fxtime	(time the highlight lags behing the leading text in effect 2)
// string text message		(512 chars max sz string)
#define TE_LINE				30
// coord, coord, coord		startpos
// coord, coord, coord		endpos
// short life in 0.1 s
// 3 bytes r, g, b

#define TE_BOX				31
// coord, coord, coord		boxmins
// coord, coord, coord		boxmaxs
// short life in 0.1 s
// 3 bytes r, g, b

#define TE_KILLBEAM			99		// kill all beams attached to entity
// short (entity)

#define TE_LARGEFUNNEL		100
// coord coord coord (funnel position)
// short (sprite index) 
// short (flags) 

#define	TE_BLOODSTREAM		101		// particle spray
// coord coord coord (start position)
// coord coord coord (spray vector)
// byte (color)
// byte (speed)

#define	TE_SHOWLINE			102		// line of particles every 5 units, dies in 30 seconds
// coord coord coord (start position)
// coord coord coord (end position)

#define TE_BLOOD			103		// particle spray
// coord coord coord (start position)
// coord coord coord (spray vector)
// byte (color)
// byte (speed)

#define TE_DECAL			104		// Decal applied to a brush entity (not the world)
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name)
// short (entity index)

#define TE_FIZZ				105		// create alpha sprites inside of entity, float upwards
// short (entity)
// short (sprite index)
// byte (density)

#define TE_MODEL			106		// create a moving model that bounces and makes a sound when it hits
// coord, coord, coord (position) 
// coord, coord, coord (velocity)
// angle (initial yaw)
// short (model index)
// byte (bounce sound type)
// byte (life in 0.1's)

#define TE_EXPLODEMODEL		107		// spherical shower of models, picks from set
// coord, coord, coord (origin)
// coord (velocity)
// short (model index)
// short (count)
// byte (life in 0.1's)

#define TE_BREAKMODEL		108		// box of models or sprites
// coord, coord, coord (position)
// coord, coord, coord (size)
// coord, coord, coord (velocity)
// byte (random velocity in 10's)
// short (sprite or model index)
// byte (count)
// byte (life in 0.1 secs)
// byte (flags)

#define TE_GUNSHOTDECAL		109		// decal and ricochet sound
// coord, coord, coord (position)
// short (entity index???)
// byte (decal???)

#define TE_SPRITE_SPRAY		110		// spay of alpha sprites
// coord, coord, coord (position)
// coord, coord, coord (velocity)
// short (sprite index)
// byte (count)
// byte (speed)
// byte (noise)

#define TE_ARMOR_RICOCHET	111		// quick spark sprite, client ricochet sound. 
// coord, coord, coord (position)
// byte (scale in 0.1's)

#define TE_PLAYERDECAL		112		// ???
// byte (playerindex)
// coord, coord, coord (position)
// short (entity???)
// byte (decal number???)
// [optional] short (model index???)

#define TE_BUBBLES			113		// create alpha sprites inside of box, float upwards
// coord, coord, coord (min start position)
// coord, coord, coord (max start position)
// coord (float height)
// short (model index)
// byte (count)
// coord (speed)

#define TE_BUBBLETRAIL		114		// create alpha sprites along a line, float upwards
// coord, coord, coord (min start position)
// coord, coord, coord (max start position)
// coord (float height)
// short (model index)
// byte (count)
// coord (speed)

#define TE_BLOODSPRITE		115		// spray of opaque sprite1's that fall, single sprite2 for 1..2 secs (this is a high-priority tent)
// coord, coord, coord (position)
// short (sprite1 index)
// short (sprite2 index)
// byte (color)
// byte (scale)

#define TE_WORLDDECAL		116		// Decal applied to the world brush
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name)

#define TE_WORLDDECALHIGH	117		// Decal (with texture index > 256) applied to world brush
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name - 256)

#define TE_DECALHIGH		118		// Same as TE_DECAL, but the texture index was greater than 256
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name - 256)
// short (entity index)

#define TE_PROJECTILE		119		// Makes a projectile (like a nail) (this is a high-priority tent)
// coord, coord, coord (position)
// coord, coord, coord (velocity)
// short (modelindex)
// byte (life)
// byte (owner)  projectile won't collide with owner (if owner == 0, projectile will hit any client).

#define TE_SPRAY			120		// Throws a shower of sprites or models
// coord, coord, coord (position)
// coord, coord, coord (direction)
// short (modelindex)
// byte (count)
// byte (speed)
// byte (noise)
// byte (rendermode)

#define TE_PLAYERSPRITES	121		// sprites emit from a player's bounding box (ONLY use for players!)
// byte (playernum)
// short (sprite modelindex)
// byte (count)
// byte (variance) (0 = no variance in size) (10 = 10% variance in size)

#define TE_PARTICLEBURST	122		// very similar to lavasplash.
// coord (origin)
// short (radius)
// byte (particle color)
// byte (duration * 10) (will be randomized a bit)

#define TE_FIREFIELD			123		// makes a field of fire.
// coord (origin)
// short (radius) (fire is made in a square around origin. -radius, -radius to radius, radius)
// short (modelindex)
// byte (count)
// byte (flags)
// byte (duration (in seconds) * 10) (will be randomized a bit)
//
// to keep network traffic low, this message has associated flags that fit into a byte:
#define TEFIRE_FLAG_ALLFLOAT	1 // all sprites will drift upwards as they animate
#define TEFIRE_FLAG_SOMEFLOAT	2 // some of the sprites will drift upwards. (50% chance)
#define TEFIRE_FLAG_LOOP		4 // if set, sprite plays at 15 fps, otherwise plays at whatever rate stretches the animation over the sprite's duration.
#define TEFIRE_FLAG_ALPHA		8 // if set, sprite is rendered alpha blended at 50% else, opaque
#define TEFIRE_FLAG_PLANAR		16 // if set, all fire sprites have same initial Z instead of randomly filling a cube. 
#define TEFIRE_FLAG_ADDITIVE	32 // if set, sprite is rendered non-opaque with additive

#define TE_PLAYERATTACHMENT			124 // attaches a TENT to a player (this is a high-priority tent)
// byte (entity index of player)
// coord (vertical offset) ( attachment origin.z = player origin.z + vertical offset )
// short (model index)
// short (life * 10 );

#define TE_KILLPLAYERATTACHMENTS	125 // will expire all TENTS attached to a player.
// byte (entity index of player)

#define TE_MULTIGUNSHOT				126 // much more compact shotgun message
// This message is used to make a client approximate a 'spray' of gunfire.
// Any weapon that fires more than one bullet per frame and fires in a bit of a spread is
// a good candidate for MULTIGUNSHOT use. (shotguns)
//
// NOTE: This effect makes the client do traces for each bullet, these client traces ignore
//		 entities that have studio models.Traces are 4096 long.
//
// coord (origin)
// coord (origin)
// coord (origin)
// coord (direction)
// coord (direction)
// coord (direction)
// coord (x noise * 100)
// coord (y noise * 100)
// byte (count)
// byte (bullethole decal texture index)

#define TE_USERTRACER				127 // larger message than the standard tracer, but allows some customization.
// coord (origin)
// coord (origin)
// coord (origin)
// coord (velocity)
// coord (velocity)
// coord (velocity)
// byte ( life * 10 )
// byte ( color ) this is an index into an array of color vectors in the engine. (0 - )
// byte ( length * 10 )


// CRenderSystem base flags
#define RENDERSYSTEM_FLAG_RANDOMFRAME			(1<< 0)// 0x00000001 // 1    // random frame sequence
#define RENDERSYSTEM_FLAG_CLIPREMOVE			(1<< 1)// 0x00000002 // 2    // remove upon touching architecture (currently requires ADDPHYSICS)
#define RENDERSYSTEM_FLAG_NOCLIP				(1<< 2)// 0x00000004 // 4    // persist inside architecture
#define RENDERSYSTEM_FLAG_LOOPFRAMES			(1<< 3)// 0x00000008 // 8    // don't remove after displaying last frame (when no life time set)
#define RENDERSYSTEM_FLAG_ADDPHYSICS			(1<< 4)// 0x00000010 // 16   // interact with world (bounce upon collisions, etc.)
#define RENDERSYSTEM_FLAG_DRAWALWAYS			(1<< 5)// 0x00000020 // 32   // ignore visibility checks (draw behind walls, outside visible area)
#define RENDERSYSTEM_FLAG_HARDSHUTDOWN			(1<< 6)// 0x00000040 // 64   // don't wait/fade, remove instantly
#define RENDERSYSTEM_FLAG_SIMULTANEOUS			(1<< 7)// 0x00000080 // 128  // emit all particles at start
#define RENDERSYSTEM_FLAG_ADDGRAVITY			(1<< 8)// 0x00000100 // 256  // add world gravity to system acceleration
#define RENDERSYSTEM_FLAG_ZROTATION				(1<< 9)// 0x00000200 // 512  // rotate around Z axis only (when parallel to viewport)
#define RENDERSYSTEM_FLAG_NODRAW				(1<<10)// 0x00000400 // 1024 // don't draw (but still update normally)
#define RENDERSYSTEM_FLAG_12					(1<<11)// 0x00000800 // 2048 // 
#define RENDERSYSTEM_FLAG_STARTRANDOMFRAME		(1<<12)// 0x00001000 // 4096 // start at random frame, continue according to other settings
#define RENDERSYSTEM_FLAG_UPDATEOUTSIDEPVS		(1<<13)// 0x00002000 // 8192 // update even when system or emitter is outside PVS
#define RENDERSYSTEM_FLAG_15					(1<<14)// 0x00004000 // 16384//
#define RENDERSYSTEM_FLAG_16					(1<<15)// 0x00008000 // 32768//

// CRenderSystem follow flags, ICNF == "if cannot follow"
#define RENDERSYSTEM_FFLAG_ICNF_KEEPSEARCHING	0// default: keep searching
#define RENDERSYSTEM_FFLAG_ICNF_REMOVE			(1<< 0)// 0x00000001 // 1    // remove if can not find entity
#define RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET	(1<< 1)// 0x00000002 // 2    // stop copying origin and just stay at last coordinates
#define RENDERSYSTEM_FFLAG_ICNF_NODRAW			(1<< 2)// 0x00000004 // 4    // hide if can not find entity
#define RENDERSYSTEM_FFLAG_CLIPREMOVE			(1<< 3)// 0x00000008 // 8    // remove upon touching architecture (even if entity still exists)
#define RENDERSYSTEM_FFLAG_USEOFFSET			(1<< 4)// 0x00000010 // 16   // system origin = entity origin + offset
#define RENDERSYSTEM_FFLAG_NOANGLES				(1<< 5)// 0x00000020 // 32   // don't copy angles
#define RENDERSYSTEM_FFLAG_DONTFOLLOW			(1<< 6)// 0x00000040 // 64   // don't copy origin even if entity is found
#define RENDERSYSTEM_FFLAG_DIRECTTOTARGET		(1<< 7)// 0x00000080 // 128  // PSMOVTYPE_TOWARDSPOINT: entity is our destination

// ParticleSystem particle start type
enum particlesystem_start_types_e
{
	PSSTARTTYPE_POINT = 0,
	PSSTARTTYPE_SPHERE,
	PSSTARTTYPE_BOX,
	PSSTARTTYPE_LINE,
	PSSTARTTYPE_ENTITYBBOX,
	PSSTARTTYPE_CYLINDER
};

// ParticleSystem particle move type
enum particlesystem_move_types_e
{
	PSMOVTYPE_DIRECTED = 0,
	PSMOVTYPE_OUTWARDS,// explosion
	PSMOVTYPE_INWARDS,// implosion
	PSMOVTYPE_RANDOM,
	PSMOVTYPE_TOWARDSPOINT
};

// network message destination
#define	MSG_BROADCAST		0		// unreliable to all
#define	MSG_ONE				1		// reliable to one (msg_entity)
#define	MSG_ALL				2		// reliable to all
#define	MSG_INIT			3		// write to the init string
#define MSG_PVS				4		// Ents in PVS of org
#define MSG_PAS				5		// Ents in PAS of org
#define MSG_PVS_R			6		// Reliable to PVS
#define MSG_PAS_R			7		// Reliable to PAS
#define MSG_ONE_UNRELIABLE	8		// Send to one client, but don't put in reliable stream, put in unreliable datagram ( could be dropped )
#define	MSG_SPEC			9		// Sends to all spectator proxies

// contents of a spot in the world
#define	CONTENTS_EMPTY		-1
#define	CONTENTS_SOLID		-2
#define	CONTENTS_WATER		-3
#define	CONTENTS_SLIME		-4
#define	CONTENTS_LAVA		-5
#define	CONTENTS_SKY		-6
// These additional contents constants are defined in bspfile.h
#define	CONTENTS_ORIGIN				-7		// removed at csg time
#define	CONTENTS_CLIP				-8		// changed to contents_solid
#define	CONTENTS_CURRENT_0			-9// START: only available as "true contents"
#define	CONTENTS_CURRENT_90			-10
#define	CONTENTS_CURRENT_180		-11
#define	CONTENTS_CURRENT_270		-12
#define	CONTENTS_CURRENT_UP			-13
#define	CONTENTS_CURRENT_DOWN		-14// END: only available as "true contents"
#define CONTENTS_TRANSLUCENT		-15
#define	CONTENTS_LADDER				-16
#define	CONTENTS_FLYFIELD			-17
#define	CONTENTS_GRAVITY_FLYFIELD	-18
#define	CONTENTS_FOG				-19
#define CONTENTS_SPECIAL1			-20 //SHL: used by particle systems
#define CONTENTS_SPECIAL2			-21
#define CONTENTS_SPECIAL3			-22

// entvars_s::waterlevel
enum waterlevel_e
{
	WATERLEVEL_NONE = 0,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
};

// channels
#define CHAN_AUTO				0
#define CHAN_WEAPON				1
#define	CHAN_VOICE				2
#define CHAN_ITEM				3
#define	CHAN_BODY				4
#define CHAN_STREAM				5		// allocate stream channel from the static or dynamic area
#define CHAN_STATIC				6		// allocate channel from the static area 
#define CHAN_NETWORKVOICE_BASE	7		// voice data coming across the network
#define CHAN_NETWORKVOICE_END	500		// network voice data reserves slots (CHAN_NETWORKVOICE_BASE through CHAN_NETWORKVOICE_END).
#define CHAN_BOT				501		// channel used for bot chatter.

// attenuation values
#define ATTN_NONE		0.0f// everywhere
#define	ATTN_NORM		0.8f// large radius
#define ATTN_STATIC		1.25f// medium radius
#define ATTN_IDLE		2.0f// small radius

// pitch values
#define	PITCH_NORM		100			// non-pitch shifted
#define PITCH_LOW		95			// other values are possible - 0-255, where 255 is very high
#define PITCH_HIGH		120
#define PITCH_MIN		1
#define PITCH_MAX		255

// volume values
#define VOL_NORM		1.0

// plats
#define	PLAT_LOW_TRIGGER	1

// buttons
#ifndef IN_BUTTONS_H
#include "in_buttons.h"
#endif

// Break Model Defines

#define BREAK_TYPEMASK	0x4F
#define BREAK_GLASS		0x01
#define BREAK_METAL		0x02
#define BREAK_FLESH		0x04
#define BREAK_WOOD		0x08

#define BREAK_SMOKE		0x10
#define BREAK_TRANS		0x20
#define BREAK_CONCRETE	0x40
#define BREAK_2			0x80

// XDM3037: R_RocketTrail() FTENT_SMOKETRAIL
enum RocketTrail_e
{
	teTrailThickRocket = 0,
	teTrailThickSmoke,
	teTrailThickBlood,
	teTrailDoubleTrail1,
	teTrailThickBloodLess,
	teTrailDoubleTrail2,
	teTrailThinRocket
};

// Colliding temp entity sounds

#define BOUNCE_GLASS	BREAK_GLASS
#define	BOUNCE_METAL	BREAK_METAL
#define BOUNCE_FLESH	BREAK_FLESH
#define BOUNCE_WOOD		BREAK_WOOD
#define BOUNCE_SHRAP	0x10
#define BOUNCE_SHELL	0x20
#define	BOUNCE_CONCRETE BREAK_CONCRETE
#define BOUNCE_SHOTSHELL 0x80

// Temp entity bounce sound types
#define TE_BOUNCE_NULL		0
#define TE_BOUNCE_SHELL		1
#define TE_BOUNCE_SHOTSHELL	2


// Palette colors
#define	BLOOD_COLOR_RED			(byte)247//(104,0,0)
#define	BLOOD_COLOR_YELLOW		(byte)192//(255,255,0)
#define	BLOOD_COLOR_GREEN		(byte)216//(0,255,0)
#define	BLOOD_COLOR_BLUE		(byte)208//(0,0,255)


#define DEFAULT_GRAVITY		800

// entvars_s::rendermode
enum 
{	
	kRenderNormal,			// src
	kRenderTransColor,		// c*a+dest*(1-a)
	kRenderTransTexture,	// src*a+dest*(1-a)
	kRenderGlow,			// src*a+dest -- No Z buffer checks
	kRenderTransAlpha,		// src*srca+dest*(1-srca)
	kRenderTransAdd,		// src*a+dest
//	kRenderWorldGlow,
};

// entvars_s::renderfx
enum 
{	
	kRenderFxNone = 0,
	kRenderFxPulseSlow,
	kRenderFxPulseFast,
	kRenderFxPulseSlowWide,
	kRenderFxPulseFastWide,
	kRenderFxFadeSlow,
	kRenderFxFadeFast,
	kRenderFxSolidSlow,
	kRenderFxSolidFast,
	kRenderFxStrobeSlow,
	kRenderFxStrobeFast,// 10
	kRenderFxStrobeFaster,
	kRenderFxFlickerSlow,
	kRenderFxFlickerFast,
	kRenderFxNoDissipation,		// Don't scale up when the viewer goes farther away
	kRenderFxDistort,			// Distort/scale/translate flicker
	kRenderFxHologram,			// kRenderFxDistort + distance fade
	kRenderFxDeadPlayer,		// kRenderAmt is the player index
	kRenderFxExplode,			// Scale up really big!
	kRenderFxGlowShell,			// Glowing Shell
	kRenderFxClampMinScale,		// 20 Keep this sprite from getting very small (SPRITES only!)
//too late	kRenderFxLightMultiplier,//CTM !!!CZERO added to tell the studiorender that the value in iuser2 is a lightmultiplier
	kRenderFxFullBright,		// XDM: face flag STUDIO_NF_FULLBRIGHT
//	kRenderFxFlatShade,			// XDM: face flag STUDIO_NF_FLATSHADE
	kRenderFxDisintegrate,		// XDM: client-side disintegration effect
};


//typedef unsigned int	func_t;
typedef unsigned int	string_t;

typedef unsigned char 		byte;
typedef unsigned short 		word;
#define _DEF_BYTE_

#undef true
#undef false

#ifndef __cplusplus
typedef enum {false, true}	qboolean;
#else 
typedef int qboolean;
#endif

typedef struct
{
	byte r, g, b;
} color24;

typedef struct
{
	unsigned r, g, b, a;
} colorVec;
/*
#ifdef _WIN32
#pragma pack(push,2)
#endif

typedef struct
{
	unsigned short r, g, b, a;
} PackedColorVec;

#ifdef _WIN32
#pragma pack(pop)
#endif
*/
typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

typedef struct edict_s edict_t;

typedef struct
{
	vec3_t	normal;
	float	dist;
} plane_t;

typedef struct
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	qboolean	inopen, inwater;
	float	fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t	endpos;			// final position
	plane_t	plane;			// surface normal at impact
	edict_t	*ent;			// entity the surface is on
	int		hitgroup;		// 0 == generic, non zero is specific body part
} trace_t;

#endif
