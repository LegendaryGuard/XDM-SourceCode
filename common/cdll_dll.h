#ifndef CDLL_DLL_H
#define CDLL_DLL_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

// This header holds information shared by both server and client DLLs

#define TARGET_OPERATOR_CHAR	'#'

// XDM: absolute values
#define MAX_ABS_ORIGIN			16384
//#define MAX_ABS_VELOCITY		65536

#define MAX_PLAYERS				32// XDM3036: must be same as MAX_CLIENTS (or replace)

#define MAX_WEAPON_SLOTS		8
#define MAX_ITEMS				16// for items like antidote, flare, keycard, etc.

#define MAX_AMMO_SLOTS			32

#if defined (DOUBLE_MAX_AMMO)
#define MAX_AMMO				USHRT_MAX-1
#else
#define MAX_AMMO				UCHAR_MAX-1
#endif

// For train controls and HUD
#define TRAIN_NUMSPEEDMODES		4

// Field Of View in degrees
#define DEFAULT_FOV				90

// Centralized, simplified SQB correction method
#define PITCH_CORRECTION_MULTIPLIER		3.0f

// CHUD flags
#define	HIDEHUD_WEAPONS			(1<<0)
#define	HIDEHUD_FLASHLIGHT		(1<<1)
#define	HIDEHUD_ALL				(1<<2)
#define HIDEHUD_HEALTH			(1<<3)
#define HIDEHUD_POWERUPS		(1<<4)// XDM3038a: like longjump

// XDM3037: HUD distortion effects flags
#define HUD_DISTORT_COLOR		(1<<0)//0x01
#define HUD_DISTORT_SPRITE		(1<<1)//0x02
#define HUD_DISTORT_POS			(1<<2)//0x04

// TextMsg types, max of 15 types
enum hud_printdest_e
{
	HUD_PRINTDEFAULT = 0,
	HUD_PRINTNOTIFY,
	HUD_PRINTCONSOLE,
	HUD_PRINTTALK,
	HUD_PRINTCENTER,
	HUD_PRINTHUD
};

#define HUD_PRINTDEST_MASK		0x0F// first half of a byte
#define HUD_PRINTLEVEL_MASK		0xF0// second half of a byte

#define MAX_ID_RANGE			800// status bar ID range
#define SBAR_STRING_SIZE		32// server message string
#define SBAR_MSG_CLEAR			255// send this as "numlines" in StatusData

enum sbar_data
{
	SBAR_ID_TARGETINDEX = 0,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_ID_3,
	SBAR_NUMVALUES
};

#define MAX_NORMAL_BATTERY		100
#define MAX_PLAYER_HEALTH		100
#define MAX_ABS_PLAYER_ARMOR	1000// should fit in 10-bit signed float
#define MAX_ABS_PLAYER_HEALTH	UCHAR_MAX// absolute maximum
#define	MAX_FLASHLIGHT_CHARGE	100// almost useless

#define MAX_MAPNAME				32// XDM3035c: that's the lowest value found in the valve's code, if this ever to be changed, re-validate ALL STRUCTURES AND ARRAYS that use it! (like globalentity_t)
#define MAX_MAPPATH				MAX_MAPNAME+32// XDM3038c: special measure
#define MAX_PLAYER_NAME_LENGTH	32
#define	MAX_MOTD_LENGTH			1536

enum MENU_ID_LIST
{
	MENU_CLOSEALL = 0,// XDM3037a
	MENU_DEFAULT,
	MENU_TEAM,
	MENU_CLASS,
	MENU_MAPBRIEFING,
	MENU_INTRO,// automatic MOTD
	MENU_CLASSHELP,
	MENU_CLASSHELP2,
	MENU_REPEATHELP,
	MENU_SPECHELP,
	MENU_SCOREBOARD,// XDM3037a
	MENU_11,
	MENU_SPY,
	MENU_SPY_SKIN,
	MENU_SPY_COLOR,
	MENU_ENGINEER,
	MENU_ENGINEER_FIX_DISPENSER,
	MENU_ENGINEER_FIX_SENTRYGUN,
	MENU_ENGINEER_FIX_MORTAR,
	MENU_DISPENSER,
	MENU_CLASS_CHANGE,
	MENU_TEAM_CHANGE,
	MENU_INVENTORY,
	MENU_MAPINFO,
	MENU_CONSOLE,
	MENU_REFRESH_RATE,
	MENU_ENTITYCREATE,
	MENU_VOICETWEAK = 50
};

// ShowMenu message flags
#define MENUFLAG_BG_FILL_NONE		1// force fully transparent menu
#define MENUFLAG_BG_FILL_SCREEN		2// force fullscreen menu
// ...
#define MENUFLAG_CLOSE				128// force close menu

// XDM3037: explicit player index (1-32).
typedef int CLIENT_INDEX;// Fits in a byte, but we still use int for backwards-compatibility
#define CLIENT_INDEX_INVALID		0

// WARNING: bits before this one are used to transmit client index!
#define SAYTEXT_BIT_TEAMONLY		128// currently must fit in a byte

// Server-to-client messages
enum MUSICPLAYER_SERVERCMD
{
	MPSVCMD_RESET = 0,// XDM3038c: changed to be compatible with save/restore process
	MPSVCMD_PLAYTRACK,// only track filename (no extension, no playlists)
	MPSVCMD_PLAYTRACKLOOP,
	MPSVCMD_PLAYFILE,// accepts full names, playlists
	MPSVCMD_PLAYFILELOOP,
	MPSVCMD_PAUSE,
	MPSVCMD_STOP,
};

// Teleport S2C message
#define MSG_TELEPORT_FL_DEST		(1<< 0)// this is a destination (otherwise a source)
#define MSG_TELEPORT_FL_SOUND		(1<< 1)
#define MSG_TELEPORT_FL_FADE		(1<< 2)
#define MSG_TELEPORT_FL_NOFX		(1<< 3)
#define MSG_TELEPORT_FL_PARTICLES	(1<< 4)// additional particles

// PartSys S2C message
enum MSG_PARTICLESYS_TYPES
{
	PARTSYSTEM_TYPE_REMOVEANY = 0,
	PARTSYSTEM_TYPE_SYSTEM,
	PARTSYSTEM_TYPE_FLAMECONE,
	PARTSYSTEM_TYPE_BEAM,
	PARTSYSTEM_TYPE_SPARKS,
	PARTSYSTEM_TYPE_DRIPS,
	PARTSYSTEM_TYPE_FLATTRAIL
};

// RenderSys S2C message
enum MSG_RENDERSYS_ACTION
{
	MSGRS_DISABLE = 0,
	MSGRS_ENABLE,
	MSGRS_CREATE,// don't search for existing instances, just create new system every time
	MSGRS_DESTROY,
};

#define ENVRS_MAX_CUSTOMKEYS		8
#define ENVRS_MAX_KEYLEN			64

//#define MSG_RENDERSYS_FLAG_NOTOGGLE			(1 << 0) use RENDERSYSTEM_FLAG_SIMULTANEOUS
//#define MSG_RENDERSYS_FLAG_DRAWALWAYS		(1 << 1) use RENDERSYSTEM_FLAG_DRAWALWAYS


// XDM3038b: client-to-server automatic commands argument indexes (for CMD_ARGV)

enum cmd_args_use_e
{
	CMDARG_USE = 0,
	CMDARG_USE_ENTINDEX,
	CMDARG_USE_STATE,
	CMDARG_USE_VALUE
};

enum cmd_args_pick_e
{
	CMDARG_PICK = 0,
	CMDARG_PICK_START_X,
	CMDARG_PICK_START_Y,
	CMDARG_PICK_START_Z,
	CMDARG_PICK_END_X,
	CMDARG_PICK_END_Y,
	CMDARG_PICK_END_Z
};

enum cmd_args_move_e
{
	CMDARG_MOVE = 0,
	CMDARG_MOVE_ENTINDEX,
	CMDARG_MOVE_ORIGIN
};

enum cmd_args_create_e
{
	CMDARG_CREATE = 0,
	CMDARG_CREATE_CLASSNAME,
	CMDARG_CREATE_CREATE,
	CMDARG_CREATE_ORIGIN,
	CMDARG_CREATE_ANGLES,
	CMDARG_CREATE_KV1// 1st key:value pair
};

enum cmd_args_createplayerstart_e
{
	CMDARG_CREATEPLSTART = 0,
	CMDARG_CREATEPLSTART_TEAM,
	CMDARG_CREATEPLSTART_LANDMARK,
	CMDARG_CREATEPLSTART_TYPE,
	CMDARG_CREATEPLSTART_SPAWNFLAGS,
	CMDARG_CREATEPLSTART_NUMARGS
};




// ==== WARNING: the following stuff was brought by Ghoul ====

//Monster's Projectile trail and explosion effects
typedef enum monster_effects_e
{
	 MONFX_REMOVE = 0,
	 MONFX_FIREBALL,
	 MONFX_GARG_FLAME,
	 MONFX_GARG_STOMP,
	 MONFX_STONE_GARG_STOMP,
	 MONFX_BABYGARG_FLAME,
	 MONFX_SQUIDSPIT,
	 MONFX_GONOMEGUTS,
	 MONFX_PITDRONESPIKE,
	 MONFX_PITWORM_EYEATTACK,
	 MONFX_PITWORM_SCREAM,
	 MONFX_PWTELEPORTER,
	 MONFX_PITWORMSPIT,
	 MONFX_ZSOLDIER_FIREGUN,
	 MONFX_ZROBOT_ATTACK,
	 MONFX_VOLTIGOREBOLT,
	 MONFX_VOLTIGORE_ANIM_EFFECT,
	 MONFX_TENTACLE_ANIM_EFFECT,
	 MONFX_KINGPIN_ANIM_EFFECT,
	 MONFX_KINGPIN_BOLT,
	 MONFX_KINGPIN_SONIC,
	 MONFX_KINGPIN_PARALYZEBEAM,
	 MONFX_SHOCK,
	 MONFX_SHOCKBALL,
	 MONFX_SHOCKTROOPER_SHOOT_EFFECT,
	 MONFX_HGRUNT_FIREGUN,
	 MONFX_RGRUNT_SMOKE,
	 MONFX_HEGRENADE_DETONATE,
	 MONFX_SPOREGRENADE,
	 MONFX_SPOREBALL,
	 MONFX_SPOREBOMB,
	 MONFX_BIGMOMMA_MORTAR,
	 MONFX_BIGMOMMA_LAYCRAB,
	 MONFX_APACHE_SMOKE,
	 MONFX_APACHE_FIREGUN,
	 MONFX_APACHE_BLAST,
	 MONFX_HVR,
	 MONFX_MINIMISSILE,
	 MONFX_C4,
	 MONFX_AFLYER_DMGFX,
	 MONFX_HORNET,
} monfx;

// projectiles_mon.mdl, different bodyes for monster projectiles
typedef enum monster_projectiles_e
{
	MONPROJ_SHOCK = 0,
	MONPROJ_PITDRONESPIKE,
	MONPROJ_SPORE,
	MONPROJ_HORNET,
	MONPROJ_HEGRENADE,
	MONPROJ_SONIC,
	MONPROJ_HVR,
	MONPROJ_MINIMISSILE,
	MONPROJ_SPOREBOMB,
	MONPROJ_C4,
} monproj;

#endif // CDLL_DLL_H
