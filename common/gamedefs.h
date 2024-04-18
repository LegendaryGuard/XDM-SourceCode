//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef GAMEDEFS_H
#define GAMEDEFS_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* !__MINGW32__ */
#endif

//-----------------------------------------------------------------------------
// Common types/constants
//-----------------------------------------------------------------------------

// Game types IMPORTANT: keep this in order shown below!
enum game_rules_id_e
{
	GT_SINGLE = 0,// auto-select in multiplayer
	GT_COOP,// multiplayer game types
	GT_DEATHMATCH,
	GT_LMS,
	GT_TEAMPLAY,// only teamplay game types below
	GT_CTF,// - game types that use extra score below
	GT_DOMINATION,
	GT_ROUND,// round-based game types are derived from this
	// no need to make this special?	GT_ASSAULT,
	NUM_GAMETYPES
};

// For map type detection
typedef struct gametype_prefix_s
{
	char	*prefix;
	int		gametype;
} gametype_prefix_t;

extern gametype_prefix_t gGameTypePrefixes[];
/*= {
	{"DM",		GT_DEATHMATCH},
	{"CO",		GT_COOP},
	{"CTF",		GT_CTF},
	{"DOM",		GT_DOMINATION},
	{"RD",		GT_ROUND},
	{"OP4CTF",	GT_CTF},
	{"AS",		GT_ROUND},
	{NULL,		0}// IMPORTANT! Must end with a null-terminator.
};*/

// Global game state
enum game_state_e
{
	GAME_STATE_WAITING = 0,// waiting for players to join
	GAME_STATE_SPAWNING,// in some game rules, players spawn only at this point
	GAME_STATE_ACTIVE,// the game is on, respawning is not allowed
	GAME_STATE_FINISHED,// round has ended, announce scores, intermission
	GAME_STATE_LOADING// server is loading another map
};

// Server policy
enum game_combat_mode_e
{
	GAME_COMBATMODE_NORMAL = 0,
	GAME_COMBATMODE_NODAMAGE,
	GAME_COMBATMODE_NOSHOOTING
};

// How game rules should be kept between maps
#define GR_PERSIST						(1<<0)//1 keep current instance of game rules
#define GR_PERSIST_KEEP_EXTRASCORE		(1<<1)//2 keep team extra score (for round gaming)
#define GR_PERSIST_KEEP_SCORE			(1<<2)//4 keep player score
#define GR_PERSIST_KEEP_LOSES			(1<<3)//8 keep player loses
#define GR_PERSIST_KEEP_INVENTORY		(1<<4)//16 keep player weapons
#define GR_PERSIST_KEEP_STATS			(1<<5)//32 keep player statistics

#define GAMETITLE_DEFAULT_STRING	"X-Half-Life"
#define GAMETITLE_STRING_NAME		"GAMETITLE"

// Longest time the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120.0f

// Respawn delay constants
#define ITEM_RESPAWN_TIME			30.0f
#define WEAPON_RESPAWN_TIME			20.0f
#define AMMO_RESPAWN_TIME			20.0f

// Number of items added by a server cvar
#define MAX_ADD_DEFAULT_ITEMS		32

// XDM3038a: how to use the spawn spot
enum spawnspot_use_modes_e
{
	SPAWNSPOT_UNDEFINED = 0,
	SPAWNSPOT_FORCE_CLEAR,
	SPAWNSPOT_FORCE_WAIT
};

// gmsgItemSpawn message type
enum event_item_spawn_types_e
{
	EV_ITEMSPAWN_ITEM = 0,
	EV_ITEMSPAWN_WEAPON,
	EV_ITEMSPAWN_AMMO,
	EV_ITEMSPAWN_OTHER
};

// Which weapons may/should be dropped by players
enum player_drop_policy_gun_e
{
	GR_PLR_DROP_GUN_NO = 0,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_ALL
};

// What ammo may/should be dropped by players
enum player_drop_policy_ammo_e
{
	GR_PLR_DROP_AMMO_NO = 0,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_ALL
};

// Policy for picking up owned items
enum player_pickup_policy_e
{
	PICKUP_POLICY_EVERYONE = 0,
	PICKUP_POLICY_OWNER,
	PICKUP_POLICY_FRIENDS,
	PICKUP_POLICY_ENEMIES
};

// Player relationship return codes
enum game_relaitonship_e
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL
};

// Game events
enum game_event_e
{
	GAME_EVENT_UNKNOWN = 0,
	GAME_EVENT_START,
	GAME_EVENT_END,
	GAME_EVENT_ROUND_START,
	GAME_EVENT_ROUND_END,
	GAME_EVENT_AWARD,
	GAME_EVENT_COMBO,
	GAME_EVENT_COMBO_BREAKER,
	GAME_EVENT_FIRST_SCORE,
	GAME_EVENT_TAKES_LEAD,
	GAME_EVENT_HEADSHOT,
	GAME_EVENT_REVENGE,
	GAME_EVENT_REVENGE_RESET,
	GAME_EVENT_LOSECOMBO,
	GAME_EVENT_PLAYER_READY,
	GAME_EVENT_PLAYER_OUT,
	GAME_EVENT_PLAYER_FINISH,
	GAME_EVENT_OVERTIME,
	GAME_EVENT_DISTANTSHOT,
	GAME_EVENT_SECRET
};

#define DISTANTSHOT_DISTANCE			2048.0f// XDM3038b

// GameMode message
#define GAME_FLAG_NOSHOOTING			(1<<0)//1
#define GAME_FLAG_ALLOW_CAMERA			(1<<1)//2
#define GAME_FLAG_ALLOW_SPECTATORS		(1<<2)//4
#define GAME_FLAG_ALLOW_SPECTATE_ALL	(1<<3)//8
#define GAME_FLAG_ALLOW_OVERVIEW		(1<<4)//16
#define GAME_FLAG_ALLOW_HIGHLIGHT		(1<<5)//32
#define GAME_FLAG_OVERDRIVE				(1<<6)//64
#define GAME_FLAG_BALANCETEAMS			(1<<7)//128 XDM3037a

#define MAX_GAME_FLAGS					(sizeof(short)*CHAR_BIT)// bits in two bytes

// Tournament effects
#define SCORE_AWARD_NORMAL				1
#define SCORE_AWARD_TIME				3.0
#define SCORE_AWARD_COMBO				5// wins per combo
#define SCORE_AWARD_MAX					20// maximum fast-score award (n-kill)
#define SCORE_COMBO_MAX					5// max combos

// SendLeftUpdates flags
#define UPDATE_LEFT_SCORE				(1<<0)
#define UPDATE_LEFT_TIME				(1<<1)
#define UPDATE_LEFT_OTHER				(1<<2)
// ...
#define UPDATE_LEFT_FORCE				(1<<6)

// Kill flags
// These hold additional information to be sent to a client
// Clients cannot retrieve these details by themselves
#define KFLAG_TEAMKILL					(1<<0)//1
#define KFLAG_MONSTER					(1<<1)//2

// When player died
#define SINGLEPLAYER_RESTART_DELAY		5.0f
//-----------------------------------------------------------------------------
// Teamplay
//-----------------------------------------------------------------------------

// NOTE
// In teamplay spectators and unassigned players have team == TEAM_NONE.
// But on client side we cannot rely on this value to determine if a player is spectating
// because in other game types ALL players may have team set to TEAM_NONE.
enum team_id_e
{
	TEAM_NONE = 0,// unassigned / spectators
	TEAM_1,
	TEAM_2,
	TEAM_3,
	TEAM_4
};// TEAMS;

// XDM3035: use TEAM_ID instead of just 'int' to make sure nobody use 0 for team 1
typedef short TEAM_ID;// Not an enum typedef, made on purpose

#define MAX_TEAMS				4// does not include virtual team for spectators or not connected players
#define MAX_TEAMNAME_LENGTH		16// maybe MAX_ENTITY_STRING_LENGTH?
//#define MAX_TEAM_NAME			MAX_TEAMNAME_LENGTH// for compatibility

// Don't use less than 255 because pfnDrawSetTextColor doesn't recognize it
const unsigned char g_iTeamColors[MAX_TEAMS + 1][3] =
{
	{255,	255,	255},	// Default
	{0,		255,	0},		// Green
	{0,		0,		255},	// Blue
	{255,	0,		0},		// Red
	{255,	255,	0},	// XDM3035 Yellow
	//{0,	255,	255},	// Cyan
};// see TeamColormap() in server DLL

//-----------------------------------------------------------------------------
// Capture the flag
//-----------------------------------------------------------------------------

// CTF events
enum ctf_events_e
{
	CTF_EV_INIT = 0,
	CTF_EV_DROP,
	CTF_EV_RETURN,
	CTF_EV_CAPTURED,
	CTF_EV_TAKEN
};

#define MAX_CTF_TEAMS				2

#define CTF_LIGHT_RADIUS_DEFAULT	128
#define CTF_LIGHT_RADIUS_CARRIED	80

#define CTF_OBJ_TARGETNAME1			"ctf_cobj1"
#define CTF_OBJ_TARGETNAME2			"ctf_cobj2"

enum captureobject_states_e
{
	CO_STAY = 0,
	CO_CARRIED,
	CO_DROPPED,
	CO_CAPTURED,
	CO_INIT,// client update
	CO_RESET
};

// All flag models should have these animations in the same order
enum flag_anims_e
{
	FANIM_ON_GROUND = 0,// world
	FANIM_NOT_CARRIED,
	FANIM_CARRIED,// player
	FANIM_CARRIED_IDLE,
	FANIM_POSITION
};

// Map use types to explicit names
enum captureobject_usetypes_e
{
	COU_NONE = 0,//USE_OFF,
	COU_CAPTURE,// = USE_ON,
	COU_TAKEN,// = USE_SET,
	COU_DROP,// = USE_TOGGLE,
	COU_RETURN// = USE_KILL
};


//-----------------------------------------------------------------------------
// Domination
//-----------------------------------------------------------------------------
#define DOM_LIGHT_RADIUS	128
#define DOM_MAX_POINTS		8


//-----------------------------------------------------------------------------
// Cooperative
//-----------------------------------------------------------------------------
enum coop_modes_e
{
	COOP_MODE_SWEEP = 0,// players have to defeat all monsters (monster don't resapwn)
	COOP_MODE_MONSTERFRAGS,// player just hunt monsters (monster resapwn)
	COOP_MODE_LEVEL// sequence of levels like in normal single
};


#endif // GAMEDEFS_H
