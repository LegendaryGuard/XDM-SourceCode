#ifndef BOT_H
#define BOT_H

#include "studio.h"
#include "waypoint.h"// waypointindex_t

// stuff for Win32 vs. Linux builds

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define WIN32_EXTRALEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME

#if defined(_MSC_VER)
#define VC_EXTRALEAN
#endif
//#include <SDKDDKVer.h>
#include <windows.h>

//windows.h->windef.h->winnt.h->basetsd.h

//typedef int (FAR *GETENTITYAPI)(DLL_FUNCTIONS *, int);
//typedef int (FAR *GETENTITYAPI2)(DLL_FUNCTIONS *, int *);
//typedef int (FAR *GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef void (STDCALL *GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef int (STDCALL *SERVER_GETBLENDINGINTERFACE)(int, struct sv_blending_interface_s **, struct engine_studio_api_s *, float (*)[3][4], float (*)[MAXSTUDIOBONES][3][4]);
typedef void (STDCALL *SAVEGAMECOMMENTFN)(char *, int);
typedef int (STDCALL *XDM_ENTITY_RELATIONSHIP_FUNC)(edict_t *, edict_t*);// XDM3035a
typedef int (STDCALL *XDM_ENTITY_IS_FUNC)(edict_t *);// XDM3035a
typedef int (STDCALL *XDM_CAN_HAVE_ITEM_FUNC)(edict_t *entity, edict_t *item);// XDM3035c

typedef void (*LINK_ENTITY_FUNC)(entvars_t *);

#else

#include <dlfcn.h>
#define GetProcAddress dlsym
#define Sleep sleep

//typedef int BOOL;

//typedef int (*GETENTITYAPI)(DLL_FUNCTIONS *, int);
//typedef int (*GETENTITYAPI2)(DLL_FUNCTIONS *, int *);
//typedef int (*GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef void (*GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef int (*SERVER_GETBLENDINGINTERFACE)(int, struct sv_blending_interface_s **, struct engine_studio_api_s *, float (*)[3][4], float (*)[MAXSTUDIOBONES][3][4]);
typedef void (*SAVEGAMECOMMENTFN)(char *, int);
typedef void (*LINK_ENTITY_FUNC)(entvars_t *);
typedef int (*XDM_ENTITY_RELATIONSHIP_FUNC)(edict_t *, edict_t*);// XDM3035a
typedef int (*XDM_ENTITY_IS_FUNC)(edict_t *);// XDM3035a
typedef int (*XDM_CAN_HAVE_ITEM_FUNC)(edict_t *entity, edict_t *item);// XDM3035c

#endif

//#define _DEBUG_BOTDLL

#if defined (_DEBUG_BOTDLL)
#define DBG_BOT_PRINT	DBG_PrintF
#else
#define DBG_BOT_PRINT
#endif

#define FAKE_CMD_ARG_LEN		128
#define FAKE_CMD_ARGS			4
#define GLOBAL_ARGV_SIZE		FAKE_CMD_ARGS*FAKE_CMD_ARG_LEN

// define constants used to identify the MOD we are playing...
enum xbm_mod_id
{
	GAME_UNKNOWN = 0,
	GAME_VALVE_DLL,// XDM3038: resolved conflict with VALVE_DLL constant
	GAME_XDM_DLL
};

enum bot_ladder_e
{
	LADDER_UNKNOWN = 0,
	LADDER_UP,
	LADDER_DOWN
};

#define WANDER_LEFT				1
#define WANDER_RIGHT			2

// These indicate AUTOSPAWN mode! bad names, really
enum bot_autospawn_e
{
	RESPAWN_NO = 0,// XDM3037a: nothing to spawn, empty slot
	RESPAWN_IDLE,// normal operation of active bot
	RESPAWN_NEED_TO_SPAWN,// this bot_t needs to be added to the game
	RESPAWN_IS_SPAWNING// set before automatically creating bot
};

#define BOT_MODEL_LEN			32
#define BOT_NAME_LEN			MAX_PLAYER_NAME_LENGTH// 32

#define BOT_SKILL_LEVELS		5
#define BOT_REACTION_LEVELS		3

//#define BOT_FOLLOW_DISTANCE		80.0f

#define PLAYER_SEARCH_RADIUS	40.0
#define MAX_BOT_LOGOS			100

#define BOT_SEE_MAX_DISTANCE	4096// the farthest distance a bot can see over
#define BOT_SEARCH_MAX_DISTANCE	BOT_SEE_MAX_DISTANCE// max possible distance on map that bot care about
#define BOT_SEARCH_ENEMY_RADIUS	960// for UTIL_FindEntityInSphere
#define BOT_SEARCH_ITEMS_RADIUS	512// for UTIL_FindEntityInSphere
#define BOT_BULLET_DISTANCE		1600
#define BOT_FAR_AIM_DISTANCE	2560

//#define MAX_SAYTEXT MAX_USER_MSG_DATA-2
#define MAX_BOT_CHAT			256// max lines per section
#define BOT_CHAT_RECENT_ITEMS	8
#define BOT_CHAT_STRING_LEN		MAX_USER_MSG_DATA-2// 192 - 2
#define BOT_CHAT_TRY_INTERVAL	10// in XBM attempts are now made properly

#define BOT_JOIN_DELAY			0.5

// XDM3038c: look for inflictor when taking any of these damage types
#define BOT_DMGM_REACT			(DMG_BULLET | DMG_SLASH | DMG_BURN | DMG_BLAST | DMG_CLUB | DMG_SHOCK | DMG_SONIC | DMG_ENERGYBEAM)

enum
{
	BOT_USE_NONE = 0,
	BOT_USE_FOLLOW,
	BOT_USE_HOLD,
	BOT_USE_LASTTYPE
};

typedef struct
{
	bool can_modify;
	char text[BOT_CHAT_STRING_LEN];
} bot_chat_t;

// Weapon Usage Flags (one per firemode)
#define WEAPON_UF_STRAIGHT		0x00000000// dummy flag
#define WEAPON_UF_UNDERWATER	0x00000001// can be used under water
#define WEAPON_UF_HOLD			0x00000002// button should always be held
#define WEAPON_UF_CHARGE		0x00000004// weapon does not fire immediately; if combined with WEAPON_UF_HOLD, must hold +attack to charge
#define WEAPON_UF_SWITCH		0x00000008// does not fire but changes mode
#define WEAPON_UF_TOSS			0x00000010// projectile with toss physics
#define WEAPON_UF_PLANT			0x00000020// projectile that has to be planted on a surface
#define WEAPON_UF_COMBO1		0x00000040// when one mode should be used first...
#define WEAPON_UF_COMBO2		0x00000080// ...and the other one afterwards
#define WEAPON_UF_NODAMAGE		0x00000100// does no damage - switch,zoom or a pseudo-weapon_custom function
#define WEAPON_UF_THRUGLASS		0x00000200// can shoot through glass
#define WEAPON_UF_THRUWALLS		0x00000400// can shoot through walls
#define WEAPON_UF_ENDRADIUS		0x00000800// radial damage at the end of traceline
// end of 16 bits

typedef struct
{
	int iId;						// the weapon ID value
	int skill_level;				// bot skill must be less than or equal to this value // XDM: TODO: do we need this??
	vec_t primary_min_distance;		// 0 = no minimum
	vec_t primary_max_distance;		// 9999 = no maximum
	vec_t secondary_min_distance;	// 0 = no minimum
	vec_t secondary_max_distance;	// 9999 = no maximum
	int use_percent;				// times out of 100 to use this weapon when available
	int primary_fire_percent;		// times out of 100 to use primary fire
	int min_primary_ammo;			// minimum ammout of primary ammo needed to fire
	int min_secondary_ammo;			// minimum ammout of seconday ammo needed to fire
	float primary_charge_delay;		// time to charge weapon
	float secondary_charge_delay;	// time to charge weapon
	uint16 primary_usage_flags;
	uint16 secondary_usage_flags;
} bot_weapon_select_t;

typedef struct
{
	int iId;
	float primary_base_delay;
	float primary_min_delay[BOT_SKILL_LEVELS];
	float primary_max_delay[BOT_SKILL_LEVELS];
	float secondary_base_delay;
	float secondary_min_delay[BOT_SKILL_LEVELS];
	float secondary_max_delay[BOT_SKILL_LEVELS];
} bot_fire_delay_t;

typedef struct
{
	int iId;// weapon ID
	int iClip;// amount of ammo in the clip
	//int iState;// wstate_*
	int weapon_select_table_index;// XDM3035
} bot_current_weapon_t;

typedef struct bot_postition_s
{
	uint32 flags;
	Vector origin;
	Vector angles;
} bot_postition_t;

typedef struct bot_s
{
	edict_t *pEdict;

	short model_skin;
	short bot_skill;
	//float f_kick_time; XDM3038c: OBSOLETE
	float f_create_time;
	float f_frame_time;
	float idle_angle;
	float idle_angle_time;
	float blinded_time;
	float f_max_speed;// from real bot, not template
	float f_prev_speed;
	float f_speed_check_time;
	Vector v_prev_origin;
	float f_find_item;
	edict_t *pBotPickupItem;

	float f_start_use_ladder_time;
	float f_end_use_ladder_time;
	bool waypoint_top_of_ladder;
	byte ladder_dir;
	byte wander_dir;
	byte strafe_percent;

	float f_wall_check_time;
	float f_wall_on_right;
	float f_wall_on_left;
	float f_dont_avoid_wall_time;
	float f_look_for_waypoint_time;
	float f_jump_time;
	float f_drop_check_time;

	float f_strafe_direction;// 0 = none, negative = left, positive = right
	float f_strafe_time;
	float f_exit_water_time;
	float cmd_upmove;// XDM3038


	float f_bot_say;
	float f_bot_chat_time;
	float f_duck_time;// this is "duck until" time
	float f_sniper_aim_time;
	float f_shoot_time;
	float f_primary_charging;
	float f_secondary_charging;
	char bot_say_msg[BOT_CHAT_STRING_LEN];
	bool b_bot_say;

	bool waypoint_near_flag;
	Vector waypoint_origin;
	float f_waypoint_time;
	float f_random_waypoint_time;
	float f_waypoint_goal_time;
	//Vector waypoint_flag_origin;
	float prev_waypoint_distance;
	uint32 weapon_points[6];// five weapon locations + 1 null
	waypointindex_t curr_waypoint_index;
	waypointindex_t prev_waypoint_index[5];
	waypointindex_t waypoint_goal;

	short m_iQueuedSelectWeaponID;
	//int current_weapon_id;// XDM3037: reliable data from client update
	bot_current_weapon_t current_weapon;// one current weapon for each bot
	int m_iWeaponState[MAX_WEAPONS];// XDM3038c: wstate_* indexed by WEAPON ID!
	int m_rgAmmo[MAX_AMMO_SLOTS];// total ammo amounts (1 array for each bot)

	//float f_aim_tracking_time;
	float f_aim_x_angle_delta;
	float f_aim_y_angle_delta;

	edict_t *pBotEnemy;
	float f_bot_see_enemy_time;
	float f_bot_find_enemy_time;
	Vector v_enemy_previous_origin;
	//int enemy_attack_count;
	byte enemy_visible;

	byte chat_percent;
	byte taunt_percent;
	byte whine_percent;
	/*byte chat_tag_percent;
	byte chat_drop_percent;
	byte chat_swap_percent;
	byte chat_lower_percent;*/

	float f_bot_spawn_time;
	edict_t *pBotKiller;

	edict_t *pBotUser;// XDM: a player who commands this bot
	int use_type;// XDM: follow, hold position, etc.
	bot_postition_t posHold;// XDM3038a
	float f_bot_use_time;
	//bool b_follow_look;
	//float f_follow_look_time;

	int charging_weapon_id;
	int grenade_time;// min time between grenade throws
	float f_gren_throw_time;
	//float f_gren_check_time;
	float f_grenade_search_time;
	float f_grenade_found_time;
	//float f_medic_check_time;
	//float f_medic_pause_time;
	//float f_medic_yell_time;
	float f_move_speed;
	float f_pause_time;
	float f_sound_update_time;
	bool b_primary_holding;
	bool b_secondary_holding;

	bool b_see_tripmine;
	bool b_shoot_tripmine;
	bool b_use_health_station;
	bool b_use_HEV_station;
	bool b_lift_moving;
	bool b_use_button;

	float f_use_health_time;
	float f_use_HEV_time;
	float f_use_button_time;
	Vector v_tripmine;

	float f_spray_logo_time;
	char logo_name[16];
	byte logo_percent;
	bool b_spray_logo;

	byte top_color;
	byte bottom_color;

	float f_reaction_target_time;// time when enemy targeting starts
	short reaction;

	short m_iCurrentMenu;// XDM3037a: last GUI menu this bot have recieved

	char name[BOT_NAME_LEN];
	char model[BOT_MODEL_LEN];

	byte autospawn_state;
	bool need_to_avenge;// XDM3035c
	bool is_used;// XDM3037a: this indicated that bot SLOT is used, filled with profile data. Check pEdict to see if bot is really active!
	bool not_started;
	bool need_to_initialize;
	bool bot_has_flag;
} bot_t;

typedef struct
{
	char identification[4];// should be WAD2 (or 2DAW) or WAD3 (or 3DAW)
	int numlumps;
	int infotableofs;
} wadinfo_t;

typedef struct
{
	int filepos;
	int disksize;
	int size;// uncompressed
	char type;
	char compression;
	char pad1, pad2;
	char name[16];// must be null terminated
} lumpinfo_t;

#ifdef DEBUG_USERMSG
typedef struct msgstats_s
{
//	uint32 total;
	uint32 count[MSG_SPEC+1];
} msgstats_t;

extern msgstats_t g_UserMessagesStats;
#endif


// define some function prototypes...
BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
void ClientPutInServer(edict_t *pEntity);
void ClientCommand(edict_t *pEntity);
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
void FakeClientCommand(edict_t *pBot, const char *pcmd, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL);

const char *Cmd_Args(void);
const char *Cmd_Argv(int argc);
int Cmd_Argc(void);

inline edict_t *CREATE_FAKE_CLIENT(const char *netname)
{
	return (*g_engfuncs.pfnCreateFakeClient)(netname);
}
inline char *GET_INFOKEYBUFFER(edict_t *e)
{
	return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}
inline char *GET_INFO_KEYVALUE(char *infobuffer, char *key)
{
	return (g_engfuncs.pfnInfoKeyValue(infobuffer, key));
}
inline void SET_CLIENT_KEYVALUE(int clientIndex, char *infobuffer, char *key, char *value)
{
	(*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}

void BotPrintf(bot_t *pBot, char *szFmt, ...);// XDM3037: use to print bot AI messages
short UTIL_GetBotIndex(const edict_t *pEdict);
bot_t *UTIL_GetBotPointer(const edict_t *pEdict);


extern short g_ServerActive;
extern short mod_id;
extern short IsDedicatedServer;
// unused extern short debug_engine;
extern bot_t bots[];
extern bool g_waypoint_on;
extern bool g_auto_waypoint;
extern bool g_path_waypoint;
extern bool g_path_waypoint_enable;
extern bool g_observer_mode;
extern bool g_intermission;

extern int g_iGameType;
extern int g_iGameMode;
extern int g_iGameState;
extern int g_iSkillLevel;
extern int g_iPlayerMaxHealth;// XDM3038
extern int g_iPlayerMaxArmor;// XDM3038

extern int g_iModelIndexAnimglow01;
extern int g_iModelIndexLaser;

extern int num_logos;

extern size_t bot_chat_count;
extern size_t bot_taunt_count;
extern size_t bot_whine_count;

// TheFatal's method for calculating the msecval
extern int msecnum;
extern float msecdel;
extern float msecval;

extern float react_delay_min[BOT_REACTION_LEVELS][BOT_SKILL_LEVELS];
extern float react_delay_max[BOT_REACTION_LEVELS][BOT_SKILL_LEVELS];

extern bot_weapon_select_t *g_pWeaponSelectTable;// XDM3035

extern pfnXHL_Classify_t				XDM_Classify;
extern pfnXHL_EntityIs_t				XDM_EntityIs;
extern pfnXHL_EntityRelationship_t		XDM_EntityRelationship;
extern pfnXHL_EntityObjectCaps_t		XDM_EntityObjectCaps;
extern pfnXHL_CanHaveItem_t				XDM_CanHaveItem;
extern pfnXHL_GetActiveGameRules_t		XDM_GetActiveGameRules;

#endif // BOT_H
