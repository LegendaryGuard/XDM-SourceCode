#include "extdll.h"
//#include <time.h>
#include "eiface.h"
#include "util.h"
#include "game.h"
#include "gamerules.h"
#include "skill.h"
#include "cbase.h"
#include "globals.h"
#include "environment.h"
//#include "basemonster.h"
#include "entconfig.h"
#include "player.h"

//
// GENERAL NOTE on CVars
// It is highly recommended to name cvars starting with the following prefixes:
// sv - server, cl - client (not in this project), mp - multiplayer-related
//

DLL_GLOBAL FILE *g_pServerAutoFile = NULL;// XDM3035: holds current map information for restoring the dedicated server
DLL_GLOBAL const char g_szServerAutoFileName[] = "server_run.cfg";
DLL_GLOBAL time_t g_tServerStartTime;

// ugly shit, but no time to rewrite
/*void SecToDate(unsigned int elapsed_time, unsigned int &days, unsigned int &hours, unsigned int &minutes, unsigned int &seconds)
{
    seconds = elapsed_time % 60;
    elapsed_time /= 60;
    minutes = elapsed_time % 60;
    elapsed_time /= 60;
    hours = elapsed_time % 24;
    elapsed_time /= 24;
    days = elapsed_time;
}*/

#if defined (_DEBUG)
// These can be used to quickly tune something from the console
cvar_t	test1		= {"test1",		"0",	FCVAR_UNLOGGED, 0.0f, NULL};
cvar_t	test2		= {"test2",		"0",	FCVAR_UNLOGGED, 0.0f, NULL};
cvar_t	test3		= {"test3",		"0",	FCVAR_UNLOGGED, 0.0f, NULL};
#endif

//cvar_t	bot_allow				= {"sv_allowbots",				"1",			FCVAR_SERVER,	1.0f, NULL};

cvar_t	displaysoundlist			= {"sv_displaysoundlist",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	sv_nodegraphdisable			= {"sv_nodegraphdisable",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	sv_showpointentities		= {"sv_showpointentities",		"0",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_SPONLY,		0.0f,	NULL};
cvar_t	showtriggers				= {"sv_showtriggers",			"0",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_CHEAT,		0.0f,	NULL};

cvar_t	sv_bullet_distance			= {"sv_bullet_distance",		"3072",			FCVAR_EXTDLL/* | FCVAR_CHEAT*/,					3072.f,	NULL};// XDM3038b
cvar_t	sv_script_cmd_affectplayers	= {"sv_script_cmd_affect_players","0",			FCVAR_EXTDLL/* | FCVAR_CHEAT*/,					0.0f,	NULL};// XDM3038c
cvar_t	sv_script_cmd_printoutput	= {"sv_script_cmd_printoutput",	"1",			FCVAR_EXTDLL/* | FCVAR_CHEAT*/,					1.0f,	NULL};// XDM3038c
cvar_t	sv_flame_effect				= {"sv_flame_effect",			"2",			FCVAR_SERVER | FCVAR_EXTDLL,					2.0f,	NULL};
cvar_t	sv_glu_pri_rgb				= {"sv_glu_pri_rgb",			"95 143 255",	FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,				0.0f,	NULL};
cvar_t	sv_glu_sec_rgb				= {"sv_glu_sec_rgb",			"95 255 143",	FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,				0.0f,	NULL};
cvar_t	sv_razordisc_max_bounce		= {"sv_razordisc_max_bounce",	"8",			FCVAR_SERVER | FCVAR_EXTDLL,					8.0f,	NULL};// XDM3038b
cvar_t	sv_rpg_nuclear				= {"sv_rpg_nuclear",			"0",			FCVAR_SERVER | FCVAR_EXTDLL/*| FCVAR_CHEAT*/,	0.0f,	NULL};
cvar_t	sv_tau_pri_rgb				= {"sv_tau_pri_rgb",			"255 207 0",	FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,				0.0f,	NULL};
cvar_t	sv_tau_sec_rgb				= {"sv_tau_sec_rgb",			"255 255 255",	FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,				0.0f,	NULL};
cvar_t	sv_tau_max_reflections		= {"sv_tau_max_reflections",	"8",			FCVAR_EXTDLL,									8.0f,	NULL};// XDM3038c
cvar_t	sv_tau_velocity_mult		= {"sv_tau_velocity_mult",		"4",			FCVAR_EXTDLL,									4.0f,	NULL};
cvar_t	sv_trip_delay				= {"sv_trip_delay",				"0.1",			FCVAR_EXTDLL/* | FCVAR_CHEAT*/,					0.1f,	NULL};// XDM3038c
cvar_t	sv_trip_rgb					= {"sv_trip_rgb",				"15 143 207",	FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,				0.0f,	NULL};
cvar_t	sv_satchel_delay			= {"sv_satchel_delay",			"0",			FCVAR_EXTDLL/* | FCVAR_CHEAT*/,					0.0f,	NULL};// XDM3038
//cvar_t	sv_serverflame				= {"sv_serverflame",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_jumpaccuracy				= {"sv_jumpaccuracy",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_strings_alloc			= {"sv_strings_alloc",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038c: dangerous! experimental!

// multiplayer server rules
cvar_t	mp_scoreleft				= {"mp_scoreleft",				"0",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_UNLOGGED,	0.0f,	NULL};// Don't spam console/log files/users with this changing
cvar_t	mp_timeleft					= {"mp_timeleft",				"0",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_UNLOGGED,	0.0f,	NULL};
cvar_t	mp_gamerules				= {"mp_gamerules",				"0",			FCVAR_SERVER/* | FCVAR_SPONLY*/,				0.0f,	NULL};
cvar_t	mp_maprules					= {"mp_maprules",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_teamplay					= {"mp_teamplay",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_effects					= {"mp_effects",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_effects					= {"sv_effects",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_clientgibs				= {"sv_clientgibs",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_playerpushable			= {"sv_playerpushable",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038: 0=nobody, 1=teammates, 2=everyone
cvar_t	mp_texsnd					= {"mp_texsnd",					"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t  mp_allowhighlight			= {"mp_allowhighlight",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038c
cvar_t  mp_allowminimap				= {"mp_allowminimap",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_allowmusicevents			= {"mp_allowmusicevents",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_allowpowerfulweapons		= {"mp_allowpowerfulweapons",	"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t  mp_allowspectators			= {"mp_allowspectators",		"1",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_SPONLY,		1.0f,	NULL};
cvar_t  sv_radardist				= {"sv_radardist",				"640",			FCVAR_SERVER | FCVAR_EXTDLL,					640.0f,	NULL};// XDM3038a
cvar_t	sv_logchat					= {"sv_logchat",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038c
cvar_t	sv_lognotice				= {"sv_lognotice",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_scorelimit				= {"mp_scorelimit",				"100",			FCVAR_SERVER | FCVAR_EXTDLL,					100.0f,	NULL};
cvar_t	mp_capturelimit				= {"mp_capturelimit",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_fraglimit				= {"mp_fraglimit",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_deathlimit				= {"mp_deathlimit",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_timelimit				= {"mp_timelimit",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_overtime					= {"mp_overtime",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_friendlyfire				= {"mp_friendlyfire",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_falldamage				= {"mp_falldamage",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_weaponstay				= {"mp_weaponstay",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_ammorespawntime			= {"mp_ammorespawntime",		"20",			FCVAR_SERVER | FCVAR_EXTDLL,					20.0f,	NULL};
cvar_t	mp_itemrespawntime			= {"mp_itemrespawntime",		"30",			FCVAR_SERVER | FCVAR_EXTDLL,					30.0f,	NULL};
cvar_t	mp_weaponrespawntime		= {"mp_weaponrespawntime",		"20",			FCVAR_SERVER | FCVAR_EXTDLL,					20.0f,	NULL};
cvar_t	mp_forcerespawn				= {"mp_forcerespawn",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_forcerespawntime			= {"mp_forcerespawntime",		"5.0",			FCVAR_SERVER | FCVAR_EXTDLL,					5.0f,	NULL};
cvar_t	mp_respawntime				= {"mp_respawntime",			"1.0",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_jointime					= {"mp_jointime",				"30",			FCVAR_SERVER | FCVAR_EXTDLL,					30.0f,	NULL};// XDM3037a
cvar_t	mp_flashlight				= {"mp_flashlight",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_specteammates			= {"mp_specteammates",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_spectoggle				= {"mp_spectoggle",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_teamlist					= {"mp_teamlist",				"Team1;Team2;Team3;Team4",	FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY | FCVAR_NOEXTRAWHITEPACE,	0.0f,	NULL};
cvar_t	mp_teamcolor1				= {"mp_teamcolor1",				"0 255 0",		FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,	0.0f,	NULL};// XDM3035
cvar_t	mp_teamcolor2				= {"mp_teamcolor2",				"0 0 255",		FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,	0.0f,	NULL};
cvar_t	mp_teamcolor3				= {"mp_teamcolor3",				"255 0 0",		FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,	0.0f,	NULL};
cvar_t	mp_teamcolor4				= {"mp_teamcolor4",				"255 255 0",	FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY,	0.0f,	NULL};
cvar_t	mp_teambalance				= {"mp_teambalance",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_teamchange				= {"mp_teamchange",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_teamchangemax			= {"mp_teamchangemax",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3037
cvar_t	mp_teamchangekill			= {"mp_teamchangekill",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_maxteams					= {"mp_maxteams",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_defaultteam				= {"mp_defaultteam",			"",				FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_allowmonsters			= {"mp_allowmonsters",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_monsterfrags				= {"mp_monsterfrags",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_monstersrespawn			= {"mp_monstersrespawn",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_monstersrespawntime		= {"mp_monstersrespawntime",	"10",			FCVAR_SERVER | FCVAR_EXTDLL,					10.0f,	NULL};
cvar_t	mp_chattime					= {"mp_chattime",				"15",			FCVAR_SERVER | FCVAR_EXTDLL,					15.0f,	NULL};
cvar_t	mp_weapondrop				= {"mp_weapondrop",				"2",			FCVAR_SERVER | FCVAR_EXTDLL,					2.0f,	NULL};
cvar_t	mp_ammodrop					= {"mp_ammodrop",				"2",			FCVAR_SERVER | FCVAR_EXTDLL,					2.0f,	NULL};
cvar_t	mp_adddefault				= {"mp_adddefault",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038c: 0 = no, 1 = every time, 2 = once
cvar_t	mp_dropdefault				= {"mp_dropdefault",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_defaultitems				= {"mp_defaultitems",			"weapon_crowbar;weapon_9mmhandgun",	FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY | FCVAR_NOEXTRAWHITEPACE,	0.0f,	NULL};
cvar_t	mp_noweapons				= {"mp_noweapons",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_pickuppolicy				= {"mp_pickuppolicy",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038c
cvar_t	mp_nofriction				= {"mp_nofriction",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// obsolete?
cvar_t	mp_bannedentities			= {"mp_bannedentities",			"",				FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_PRINTABLEONLY | FCVAR_NOEXTRAWHITEPACE,	0.0f,	NULL};// XDM3037a
cvar_t	mp_flagstay					= {"mp_flagstay",				"20",			FCVAR_SERVER | FCVAR_EXTDLL,					20.0f,	NULL};
cvar_t	mp_domscoreperiod			= {"mp_domscoreperiod",			"3",			FCVAR_SERVER | FCVAR_EXTDLL,					3.0f,	NULL};
cvar_t	mp_teammines				= {"mp_teammines",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_droppeditemstaytime		= {"mp_droppeditemstaytime",	"60",			FCVAR_SERVER | FCVAR_EXTDLL,					60.0f,	NULL};
cvar_t	mp_weaponboxbreak			= {"mp_weaponboxbreak",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_allowgaussjump			= {"mp_allowgaussjump",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_allowcamera				= {"mp_allowcamera",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_allowenemiesonmap		= {"mp_allowenemiesonmap",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038
cvar_t	mp_laddershooting			= {"mp_laddershooting",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_teammenu					= {"mp_teammenu",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_instagib					= {"mp_instagib",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_telegib					= {"mp_telegib",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_playerexplode			= {"mp_playerexplode",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_playerstartarmor			= {"mp_playerstartarmor",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_playerstarthealth		= {"mp_playerstarthealth",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_spawnkill				= {"mp_spawnkill",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038: ON by default
cvar_t	mp_spawnpointforce			= {"mp_spawnpointforce",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	mp_spawnprotectiontime		= {"mp_spawnprotectiontime",	"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_noshooting				= {"mp_noshooting",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_revengemode				= {"mp_revengemode",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// 0 = normal game, 1 = extra score, 2 = hunt mode (ability to kill)
cvar_t	mp_round_allowlatejoin		= {"mp_round_allowlatejoin",	"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_round_minplayers			= {"mp_round_minplayers",		"2",			FCVAR_SERVER | FCVAR_EXTDLL,					2.0f,	NULL};// XDM3038a
cvar_t	mp_round_time				= {"mp_round_time",				"15",			FCVAR_SERVER | FCVAR_EXTDLL,					15.0f,	NULL};// XDM3038a
cvar_t	mp_showgametitle			= {"mp_showgametitle",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_firetargets_player		= {"mp_firetargets_player",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038
//cvar_t	mp_intermissiontime		= {"mp_intermissiontime",		"15",			FCVAR_SERVER | FCVAR_EXTDLL,					15.0f,	NULL};
cvar_t	mp_coop_allowneutralspots	= {"mp_coop_allowneutralspots",	"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_coop_checktransition		= {"mp_coop_checktransition",	"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038c
cvar_t	mp_coop_commonspawnspot		= {"mp_coop_commonspawnspot",	"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038c
cvar_t	mp_coop_eol_firstwin		= {"mp_coop_eol_firstwin",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_coop_eol_spectate		= {"mp_coop_eol_spectate",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	mp_coop_keepinventory		= {"mp_coop_keepnventory",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038b
cvar_t	mp_coop_keepscore			= {"mp_coop_keepscore",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a
cvar_t	mp_coop_usemaptransition	= {"mp_coop_usemaptransition",	"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t  mp_lms_allowspectators		= {"mp_lms_allowspectators",	"1",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_SPONLY,		1.0f,	NULL};// XDM3038a: players who did not press ready become spectators permanently
cvar_t	mp_precacheammo				= {"mp_precacheammo",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038: dangerous! experimental!
cvar_t	mp_precacheitems			= {"mp_precacheitems",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038: dangerous! experimental!
cvar_t	mp_precacheweapons			= {"mp_precacheweapons",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038: dangerous! experimental!
cvar_t	sv_allowstealvehicles		= {"sv_allowstealvehicles",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038
cvar_t	sv_overdrive				= {"sv_overdrive",				"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	sv_serverstaticents			= {"sv_serverstaticents",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	sv_modelhitboxes			= {"sv_modelhitboxes",			"0",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_CHEAT,		0.0f,	NULL};
cvar_t	sv_partialpickup			= {"sv_partialpickup",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038c: allow picking up some of pickup capacity without removing it
cvar_t	sv_playermaxhealth			= {"sv_playermaxhealth",		"100",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_SPONLY,		100.0f,	NULL};// XDM3037
cvar_t	sv_playermaxarmor			= {"sv_playermaxarmor",			"200",			FCVAR_SERVER | FCVAR_EXTDLL | FCVAR_SPONLY,		200.0f,	NULL};// XDM3038
cvar_t	sv_weaponsscale				= {"sv_weaponsscale",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_generategamecfg			= {"sv_generategamecfg",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	sv_loadentfile				= {"sv_loadentfile",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_usemapequip				= {"sv_usemapequip",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038c: 0 = add only default items, 1 = add map equip instead of default items, 2 = add both, 3 = always add map equip, but add default only when game_player_equip has SF_PLAYEREQUIP_ADDTODEFAULT
//cvar_t	sv_specnoclip			= {"sv_specnoclip",				"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};
cvar_t	sv_reliability				= {"sv_reliability",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};
cvar_t	sv_deleteoutsiteworld		= {"sv_deleteoutsiteworld",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038a: delete entities that fall outsite world
cvar_t	sv_entinitinterval			= {"sv_entinitinterval",		"0.1",			FCVAR_SERVER | FCVAR_EXTDLL,					0.1f,	NULL};
cvar_t	sv_precacheammo				= {"sv_precacheammo",			"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038: dangerous! experimental!
cvar_t	sv_precacheitems			= {"sv_precacheitems",			"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038: dangerous! experimental!
cvar_t	sv_precacheweapons			= {"sv_precacheweapons",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038: dangerous! experimental!
//#if defined(CLIENT_WEAPONS)?
//cvar_t	sv_oldweaponupdate		= {"sv_oldweaponupdate",		"0",			FCVAR_SERVER | FCVAR_EXTDLL,					0.0f,	NULL};// XDM3038a: experimental
//cvar_t	sv_hack_triggerauto		= {"sv_hack_triggerauto",		"1",			FCVAR_SERVER | FCVAR_EXTDLL,					1.0f,	NULL};// XDM3038: fixes trigger_auto start with USE_OFF
cvar_t	sv_decalfrequency			= {"decalfrequency",			"30",			FCVAR_SERVER | FCVAR_EXTDLL,					30.0f,	NULL};// how often players may draw their logos
#if defined (_DEBUG_INFILOOPS)
cvar_t	sv_dbg_maxcollisionloops	= {"sv_dbg_maxcollisionloops",	"128",				FCVAR_SERVER | FCVAR_EXTDLL,				128.0f,	NULL};// XDM3038a: how many engine to DLL ShouldCollide() calls allowed per entity
#endif

// Engine Cvars
cvar_t	*g_pCvarDeveloper = NULL;
cvar_t	*g_psv_aim = NULL;
cvar_t	*g_pviolence_hblood = NULL;
cvar_t	*g_pviolence_ablood = NULL;
cvar_t	*g_pviolence_hgibs = NULL;
cvar_t	*g_pviolence_agibs = NULL;
cvar_t 	*g_psv_gravity = NULL;
cvar_t 	*g_psv_friction = NULL;
cvar_t 	*g_psv_maxspeed = NULL;
cvar_t	*g_pmp_footsteps = NULL;// does the engine still provide this??
cvar_t	*g_psv_zmax = NULL;
cvar_t	*g_psv_cheats = NULL;

//-----------------------------------------------------------------------------
// Purpose: Server command
// HACK? indicates that all following commands in config file are rejected in non-multiplayer mode
//-----------------------------------------------------------------------------
void Cmd_MultiplayerOnly(void)
{
	if (g_MapConfigCommands)// valid only in map.cfg
	{
		if (CMD_ARGC() > 1)// == 2) no arguments, just enable
			g_MultiplayerOnlyCommands = (atoi(CMD_ARGV(1)) > 0);
		else
			g_MultiplayerOnlyCommands = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server command
// HACK? indicates that all following commands in config file are rejected in other game rules mode
// Provides basic comparison and bitwise operators
//-----------------------------------------------------------------------------
void Cmd_GamerulesOnly(void)
{
	if (g_MapConfigCommands)// valid only in map.cfg
	{
		if (CMD_ARGC() > 2)
		{
			if (CMD_ARGV(1))
				strncpy(g_iGamerulesSpecificCommandsOp, CMD_ARGV(1), 4);

			g_iGamerulesSpecificCommands = atoi(CMD_ARGV(2));
		}
		else if (CMD_ARGC() > 1)// GT_TEAMPLAY, etc
		{
			if (strcmp(CMD_ARGV(1), "end") == 0)
				g_iGamerulesSpecificCommands = -1;// to disable
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_SetKV(void)// moved here to be used in map configs
{
	if (!ENTCONFIG_ValidateCommand())
		return;

	if (CMD_ARGC() > 2)// XDM3035b: allow empty argument
	{
		KeyValueData kvd;
		kvd.szKeyName = (char *)CMD_ARGV(2);
		kvd.szValue = (char *)CMD_ARGV(3);
		kvd.fHandled = FALSE;// IMPORTANT!!
		edict_t	*pEntity = NULL;
		while ((pEntity = FIND_ENTITY_BY_TARGETNAME(pEntity, CMD_ARGV(1))) != NULL)
		{
			if (FNullEnt(pEntity))// wrong! if (pEntity == NULL)
				break;
			kvd.szClassName = STRINGV(pEntity->v.classname);
			DispatchKeyValue(pEntity, &kvd);
			SERVER_PRINTF("%s: found %s\n", CMD_ARGV(0), STRING(pEntity->v.classname));
		}
	}
	else
		SERVER_PRINTF("usage: %s <targetname> <key> <\"value\">\n", CMD_ARGV(0));
}


EHANDLE g_WriteEntityToFile;// since console command functions cannot return anything, we need this global

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_SpawnEnt(void)
{
	if (!ENTCONFIG_ValidateCommand())
		return;

	g_WriteEntityToFile = NULL;// mark new attempt anyway

	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return;

	if (CMD_ARGC() > 1)
	{
		Vector origin;
		if (StringToVec(CMD_ARGV(2), origin))
		{
			Vector angles;
			if (!StringToVec(CMD_ARGV(3), angles))
				angles.Clear();

			CBaseEntity *pEntity = CBaseEntity::Create((char *)CMD_ARGV(1), origin, angles, NULL);
			if (pEntity)
			{
				// XDM3038: 20141121 pEntity->pev->classname = ALLOC_STRING(CMD_ARGV(1));// very important thing!
				if (CMD_ARGC() > 4)
					pEntity->pev->targetname = ALLOC_STRING(CMD_ARGV(4));

				g_WriteEntityToFile = pEntity;
				conprintf(2, "Added %s\n", CMD_ARGV(1));
			}
			else
				conprintf(2, "Failed to create %s!\n", CMD_ARGV(1));
		}
		else
			conprintf(2, "Unable to create %s without origin!\n", CMD_ARGV(1));
	}
	else
		conprintf(1, "usage: %s <classname> <\"x y z\" origin> [\"x y z\" angles] [targetname]\n", CMD_ARGV(0));
}

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_FixEntities(void)
{
	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return;

	CBaseEntity *pEntity = NULL;
	edict_t *pEdict = INDEXENT(1);
	int iNumFound = 0;
	bool bRemove = false;

	if (CMD_ARGC() > 0 && CMD_ARGV(1) > 0)
		bRemove = true;

	for (int i = 1; i < gpGlobals->maxEntities; ++i, ++pEdict)
	{
		if (!UTIL_IsValidEntity(pEdict))
			continue;

		pEntity = CBaseEntity::Instance(pEdict);
		if (pEntity)
		{
			if (!pEntity->IsInWorld() || (pEntity->IsProjectile() && POINT_CONTENTS(pEntity->Center()) == CONTENTS_SOLID))
			{
				conprintf(1, " - %d %s %s\n", i, STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname));
				++iNumFound;
				if (bRemove)
					UTIL_Remove(pEntity);
			}
		}
	}
	if (bRemove)
		conprintf(1, " Removed %d bad entities.\n", iNumFound);
	else
		conprintf(1, " Found %d bad entities. Use \"%s 1\" to remove.\n", iNumFound, CMD_ARGV(0));
}

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_ListEnts(void)
{
	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return;

	if (CMD_ARGC() > 1)
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByTargetname(pEntity, CMD_ARGV(1))) != NULL)
		{
			conprintf(1, " - %s at (%f %f %f)\n", STRING(pEntity->pev->classname), pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z);
			if (CMD_ARGC() > 2 && atoi(CMD_ARGV(2)) > 0)
			{
				conprintf(1, "removed\n");
				UTIL_Remove(pEntity);
			}
		}
	}
	else
		conprintf(1, "usage: %s <targetname> [1/0 - remove]\n", CMD_ARGV(0));
}

/*void Cmd_ClientCmd(void)
{
	if (CMD_ARGC() > 2)
	{
		edict_t	*pEdict = UTIL_ClientEdictByIndex(atoi(CMD_ARGV(1)));
		if (pEdict)
		{
			SERVER_PRINT("Executing client command\n");
			CLIENT_COMMAND(pEdict, (char *)CMD_ARGV(2));
		}
		else
			SERVER_PRINT("Client not found.\n");
	}
	else
		SERVER_PRINTF("usage: %s <index> <\"command\">\n", CMD_ARGV(0));
}*/

/*void Cmd_BotCreate(void)
{
	if (bot_allow.value > 0.0)
	{
		if (CMD_ARGC() > 1)
		{
			//BotCreate(CMD_ARGV(1));
		}
		else
			conprintf(1, "usage: %s <name>\n", CMD_ARGV(0));
	}
	else
		SERVER_PRINT("Botmatch is not allowed.\n");
}*/

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_EndGame(void)
{
	if (g_pGameRules)
	{
		SERVER_PRINT("End multiplayer game\n");
		if (g_pGameRules->IsGameOver() && gpGlobals->time > g_pGameRules->GetIntermissionStartTime() + 3.0f)// XDM3037: someone wants to forcibly skip intermission, don't let this command be accidentally used too soon
			g_pGameRules->ChangeLevel();
		else
			g_pGameRules->EndMultiplayerGame();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_ServerDir(void)
{
	UTIL_ListFiles(CMD_ARGV(1));
}

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_GameRulesInfo(void)
{
	if (g_pGameRules)
	{
		g_pGameRules->DumpInfo();
		SERVER_PRINT("NoGameRulesInfo end.\n");
	}
	else
		SERVER_PRINT("No game rules installed.\n");
}

//-----------------------------------------------------------------------------
// Purpose: Server command
//-----------------------------------------------------------------------------
void Cmd_GameRulesUpdate(void)
{
	if (g_pGameRules)
		g_pGameRules->UpdateGameMode(NULL);
	else
		SERVER_PRINT("No game rules installed.\n");
}

//-----------------------------------------------------------------------------
// Purpose: Modify user rights of a player
//-----------------------------------------------------------------------------
void Cmd_ModRights(void)
{
	int nArgs = CMD_ARGC();
	if (nArgs > 2)
	{
		CBasePlayer *pClient;
		const char *arg1 = CMD_ARGV(1);
		if (*arg1 == '#')
			pClient = UTIL_ClientByIndex(atoi(arg1+1));
		else
			pClient = UTIL_ClientByName(arg1);

		if (pClient)
		{
			int a;
			const char *pArgument;
			const char *pName;
			for (a=2; a<nArgs; ++a)
			{
				pArgument = CMD_ARGV(a);
				pName = pArgument+1;// skip operator + or -
				if (*pName && isalnum((unsigned char)*pName))
				{
					uint32 iRightIndex = UserRightByName(pName);
					if (iRightIndex != USER_RIGHTS_NONE)
					{
						char op = pArgument[0];// operator
						if (op == '+')
							pClient->RightsAssign(iRightIndex);
						else if (op == '-')
							pClient->RightsRemove(iRightIndex);
						else
							SERVER_PRINTF("Unknown operator in argument: %s\n", pArgument);
					}
					else
						SERVER_PRINTF("Unknown right in argument: %s\n", pArgument);
				}
				else
					SERVER_PRINTF("Bad argument: %s\n", pArgument);
			}
		}
		else
			SERVER_PRINT("Client not found.\n");
	}
	else
	{
		SERVER_PRINTF("usage: %s <#index|name> <\"[+-]rights\"> ...\nPossible rights:", CMD_ARGV(0));
		for (uint32 iRightIndex = USER_RIGHTS_NONE+1; iRightIndex < USER_RIGHTS_COUNT; ++iRightIndex)
		{
			SERVER_PRINT(" ");
			SERVER_PRINT(g_UserRightsNames[iRightIndex]);
		}
		SERVER_PRINT("\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show user rights of a player
//-----------------------------------------------------------------------------
void Cmd_ShowRights(void)
{
	int nArgs = CMD_ARGC();
	if (nArgs > 1)
	{
		CBasePlayer *pClient;
		const char *arg1 = CMD_ARGV(1);
		if (*arg1 == '#')
			pClient = UTIL_ClientByIndex(atoi(arg1+1));
		else
			pClient = UTIL_ClientByName(arg1);

		if (pClient)
		{
			SERVER_PRINTF("Client %d rights:", pClient->entindex());
			for (uint32 iRightIndex = USER_RIGHTS_NONE+1; iRightIndex < USER_RIGHTS_COUNT; ++iRightIndex)
				if (pClient->RightsCheck(iRightIndex))
					SERVER_PRINTF(" %s", g_UserRightsNames[iRightIndex]);
			SERVER_PRINT("\n");
		}
	}
	else
		SERVER_PRINTF("usage: %s <#index|name>\n", CMD_ARGV(0));
}

//-----------------------------------------------------------------------------
// Purpose: UNDONE: Server command
//-----------------------------------------------------------------------------
/*void Cmd_Uptime(void)
{
	char buff[80];
	time_t tCurrent;
	time_t tDiff;
	time(&tCurrent);
	tDiff = difftime(tCurrent, g_tServerStartTime);
	ctime_s(buff, 80, &g_tServerStartTime);// secure version of ctime
	SERVER_PRINTF("Start time: %s", buff);// ctime includes '\n'
	ctime_s(buff, 80, &tDiff);
	SERVER_PRINTF("Run time: %s", buff);// ctime includes '\n'

	/ * same	tm tmDiff;
	errno_t err = _localtime64_s(&tmDiff, &tDiff);
	if (err)
	{
		printf(" _localtime64_s failed due to invalid arguments.");
	}
	else
	{
		strftime(buff, 80, "%Yy %mm %dd %H:%M:%S.000", &tmDiff);
		SERVER_PRINT(buff);
	}* /

}*/

/* BAD BAD BAD! VERY VERY BAD!!
void Cmd_ServerInitialize(void)
{
	if (g_pGameRules)
	{
		SERVER_PRINT("GameRules force Initialize()\n");
		g_pGameRules->Initialize();
	}
}*/

/*void Cmd_NextMap(void)
{
	if (g_pGameRules)
	{
		SERVER_PRINT("Instant changelevel\n");
		g_pGameRules->ChangeLevel(); - move to public and stuff...
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: Server DLL initialization
// Register your console variables here
// This gets called one time when the game is initialized
//-----------------------------------------------------------------------------
void GameDLLInit(void)
{
	if (ASSERTSZ(sizeof(float) == sizeof(uint32), "CRITICAL ERROR: globalvars_t structure size mismatch due to compiler type size difference! Memory will be corrupted!!!\n") == false)
	{
		DBG_FORCEBREAK
		gpGlobals->maxClients = 0;
		return;
	}
	if (ASSERTSZ(sizeof(int) == 4, "CRITICAL ERROR: integer type size mismatch! Memory will be corrupted!!!\n") == false)
	{
		DBG_FORCEBREAK
		gpGlobals->maxClients = 0;
		return;
	}
	SERVER_PRINTF("Initializing X-Half-Life server DLL (%s build %s)\nRegistering variables...\n", BUILD_DESC, __DATE__);

	// Register cvars here:
	g_pCvarDeveloper = CVAR_GET_POINTER("developer");

	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_pviolence_hblood = CVAR_GET_POINTER("violence_hblood");
	g_pviolence_ablood = CVAR_GET_POINTER("violence_ablood");
	g_pviolence_hgibs = CVAR_GET_POINTER("violence_hgibs");
	g_pviolence_agibs = CVAR_GET_POINTER("violence_agibs");
	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_friction = CVAR_GET_POINTER("sv_friction");
	g_psv_maxspeed = CVAR_GET_POINTER("sv_maxspeed");
	g_pmp_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_zmax = CVAR_GET_POINTER("sv_zmax");
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	/*cvar_t *skin = CVAR_GET_POINTER("skin");// XDM: ??
	if (skin != NULL)// for dedicated server
		skin->flags |= FCVAR_USERINFO;*/

	/*cvar_t *pHostGameLoaded = CVAR_GET_POINTER("host_gameloaded");// Xash detection
	if (pHostGameLoaded)
	{
	}*/
#if defined (_DEBUG)
	SERVER_PRINT("XDM: DEBUG MODE\n");
	CVAR_REGISTER(&test1);
	CVAR_REGISTER(&test2);
	CVAR_REGISTER(&test3);
#endif
	//CVAR_REGISTER(&bot_allow);

	CVAR_REGISTER(&displaysoundlist);
	CVAR_REGISTER(&sv_nodegraphdisable);
	CVAR_REGISTER(&sv_showpointentities);
	CVAR_REGISTER(&showtriggers);

	//CVAR_REGISTER(&mut_snark);
	CVAR_REGISTER(&sv_bullet_distance);
	CVAR_REGISTER(&sv_script_cmd_affectplayers);
	CVAR_REGISTER(&sv_script_cmd_printoutput);
	CVAR_REGISTER(&sv_flame_effect);
	CVAR_REGISTER(&sv_glu_pri_rgb);
	CVAR_REGISTER(&sv_glu_sec_rgb);
	CVAR_REGISTER(&sv_rpg_nuclear);
	CVAR_REGISTER(&sv_tau_pri_rgb);
	CVAR_REGISTER(&sv_tau_sec_rgb);
	CVAR_REGISTER(&sv_tau_max_reflections);
	CVAR_REGISTER(&sv_tau_velocity_mult);
	CVAR_REGISTER(&sv_trip_delay);
	CVAR_REGISTER(&sv_trip_rgb);
	CVAR_REGISTER(&sv_razordisc_max_bounce);
	CVAR_REGISTER(&sv_satchel_delay);
	//CVAR_REGISTER(&sv_serverflame);
	CVAR_REGISTER(&sv_jumpaccuracy);
	CVAR_REGISTER(&sv_strings_alloc);

	CVAR_REGISTER(&mp_scoreleft);
	CVAR_REGISTER(&mp_timeleft);
	CVAR_REGISTER(&mp_gamerules);
	CVAR_REGISTER(&mp_maprules);
	CVAR_REGISTER(&mp_teamplay);
	CVAR_REGISTER(&mp_effects);
	CVAR_REGISTER(&sv_effects);
	CVAR_REGISTER(&sv_clientgibs);
	CVAR_REGISTER(&sv_playerpushable);
	CVAR_REGISTER(&mp_texsnd);
	CVAR_REGISTER(&mp_allowhighlight);
	CVAR_REGISTER(&mp_allowminimap);
	CVAR_REGISTER(&mp_allowmusicevents);
	CVAR_REGISTER(&mp_allowpowerfulweapons);
	CVAR_REGISTER(&mp_allowspectators);
	CVAR_REGISTER(&sv_radardist);
	CVAR_REGISTER(&sv_logchat);
	CVAR_REGISTER(&sv_lognotice);
	CVAR_REGISTER(&mp_scorelimit);
	CVAR_REGISTER(&mp_capturelimit);
	CVAR_REGISTER(&mp_fraglimit);
	CVAR_REGISTER(&mp_deathlimit);
	CVAR_REGISTER(&mp_timelimit);
	CVAR_REGISTER(&mp_overtime);
	CVAR_REGISTER(&mp_friendlyfire);
	CVAR_REGISTER(&mp_falldamage);
	CVAR_REGISTER(&mp_weaponstay);
	CVAR_REGISTER(&mp_ammorespawntime);
	CVAR_REGISTER(&mp_itemrespawntime);
	CVAR_REGISTER(&mp_weaponrespawntime);
	CVAR_REGISTER(&mp_forcerespawn);
	CVAR_REGISTER(&mp_forcerespawntime);
	CVAR_REGISTER(&mp_respawntime);
	CVAR_REGISTER(&mp_jointime);
	CVAR_REGISTER(&mp_flashlight);
	CVAR_REGISTER(&mp_specteammates);
	CVAR_REGISTER(&mp_spectoggle);
	CVAR_REGISTER(&mp_teamlist);
	CVAR_REGISTER(&mp_teamcolor1);
	CVAR_REGISTER(&mp_teamcolor2);
	CVAR_REGISTER(&mp_teamcolor3);
	CVAR_REGISTER(&mp_teamcolor4);
	CVAR_REGISTER(&mp_teambalance);
	CVAR_REGISTER(&mp_teamchange);
	CVAR_REGISTER(&mp_teamchangemax);
	CVAR_REGISTER(&mp_teamchangekill);
	CVAR_REGISTER(&mp_maxteams);
	CVAR_REGISTER(&mp_defaultteam);
	CVAR_REGISTER(&mp_allowmonsters);
	CVAR_REGISTER(&mp_monsterfrags);
	CVAR_REGISTER(&mp_monstersrespawn);
	CVAR_REGISTER(&mp_monstersrespawntime);
	CVAR_REGISTER(&mp_chattime);
	CVAR_REGISTER(&mp_weapondrop);
	CVAR_REGISTER(&mp_ammodrop);
	CVAR_REGISTER(&mp_adddefault);
	CVAR_REGISTER(&mp_dropdefault);
	CVAR_REGISTER(&mp_defaultitems);
	CVAR_REGISTER(&mp_noweapons);
	CVAR_REGISTER(&mp_pickuppolicy);
	CVAR_REGISTER(&mp_nofriction);
	CVAR_REGISTER(&mp_bannedentities);
	CVAR_REGISTER(&mp_flagstay);
	CVAR_REGISTER(&mp_domscoreperiod);
	CVAR_REGISTER(&mp_teammines);
	CVAR_REGISTER(&mp_droppeditemstaytime);
	CVAR_REGISTER(&mp_weaponboxbreak);
	CVAR_REGISTER(&mp_allowgaussjump);
	CVAR_REGISTER(&mp_allowcamera);
	CVAR_REGISTER(&mp_allowenemiesonmap);
	CVAR_REGISTER(&mp_laddershooting);
	CVAR_REGISTER(&mp_teammenu);
	CVAR_REGISTER(&mp_instagib);
	CVAR_REGISTER(&mp_telegib);
	CVAR_REGISTER(&mp_playerexplode);
	CVAR_REGISTER(&mp_playerstartarmor);
	CVAR_REGISTER(&mp_playerstarthealth);
	CVAR_REGISTER(&mp_spawnkill);
	CVAR_REGISTER(&mp_spawnpointforce);
	CVAR_REGISTER(&mp_spawnprotectiontime);
	CVAR_REGISTER(&mp_noshooting);
	CVAR_REGISTER(&mp_revengemode);
	CVAR_REGISTER(&mp_round_allowlatejoin);
	CVAR_REGISTER(&mp_round_minplayers);
	CVAR_REGISTER(&mp_round_time);
	CVAR_REGISTER(&mp_showgametitle);
	CVAR_REGISTER(&mp_firetargets_player);
	CVAR_REGISTER(&mp_coop_allowneutralspots);
	CVAR_REGISTER(&mp_coop_checktransition);
	CVAR_REGISTER(&mp_coop_commonspawnspot);
	CVAR_REGISTER(&mp_coop_eol_firstwin);
	CVAR_REGISTER(&mp_coop_eol_spectate);
	CVAR_REGISTER(&mp_coop_keepinventory);
	CVAR_REGISTER(&mp_coop_keepscore);
	CVAR_REGISTER(&mp_coop_usemaptransition);
	CVAR_REGISTER(&mp_lms_allowspectators);
	CVAR_REGISTER(&mp_precacheammo);
	CVAR_REGISTER(&mp_precacheitems);
	CVAR_REGISTER(&mp_precacheweapons);
	CVAR_REGISTER(&sv_allowstealvehicles);
	CVAR_REGISTER(&sv_overdrive);
	CVAR_REGISTER(&sv_serverstaticents);
	CVAR_REGISTER(&sv_modelhitboxes);
	CVAR_REGISTER(&sv_partialpickup);
	CVAR_REGISTER(&sv_playermaxhealth);
	CVAR_REGISTER(&sv_playermaxarmor);
	CVAR_REGISTER(&sv_weaponsscale);
	if (g_iProtocolVersion <= 46)// Old HL does this!
	{
		sv_generategamecfg.string = "0";
		sv_generategamecfg.value = 0.0f;
	}
	/* TEST ONLY!!!!!!!!!	else
	{
		sv_generategamecfg.string = "1";
		sv_generategamecfg.value = 1.0f;
	}*/
	CVAR_REGISTER(&sv_generategamecfg);
	CVAR_REGISTER(&sv_loadentfile);
	CVAR_REGISTER(&sv_usemapequip);
	//UNDONE	CVAR_REGISTER(&sv_specnoclip);
	CVAR_REGISTER(&sv_reliability);
	CVAR_REGISTER(&sv_deleteoutsiteworld);
	CVAR_REGISTER(&sv_entinitinterval);
	CVAR_REGISTER(&sv_precacheammo);
	CVAR_REGISTER(&sv_precacheitems);
	CVAR_REGISTER(&sv_precacheweapons);
	//CVAR_REGISTER(&sv_oldweaponupdate);
	//CVAR_REGISTER(&sv_hack_triggerauto);
	CVAR_REGISTER(&sv_decalfrequency);
#if defined (_DEBUG_INFILOOPS)
	CVAR_REGISTER(&sv_dbg_maxcollisionloops);
#endif

	SkillRegisterCvars();

	ADD_SERVER_COMMAND("multiplayer_only", Cmd_MultiplayerOnly);
	ADD_SERVER_COMMAND("gamerules_only", Cmd_GamerulesOnly);
	//OBSOLETE	ADD_SERVER_COMMAND("map_globalfog", Cmd_GlobalFog);
	ADD_SERVER_COMMAND("setkvbytn", Cmd_SetKV);
	ADD_SERVER_COMMAND("listents", Cmd_ListEnts);
	ADD_SERVER_COMMAND("spawn_ent", Cmd_SpawnEnt);
	ADD_SERVER_COMMAND("dbg_fixentities", Cmd_FixEntities);
	//ADD_SERVER_COMMAND("clcmd", Cmd_ClientCmd);
	ADD_SERVER_COMMAND("endgame", Cmd_EndGame);
	ADD_SERVER_COMMAND("sv_dir", Cmd_ServerDir);
	ADD_SERVER_COMMAND("svls", Cmd_ServerDir);
	ADD_SERVER_COMMAND("modrights", Cmd_ModRights);
	ADD_SERVER_COMMAND("showrights", Cmd_ShowRights);
	//ADD_SERVER_COMMAND("sv_init", Cmd_ServerInitialize);
	//ADD_SERVER_COMMAND("nextmap", Cmd_NextMap);
	//ADD_SERVER_COMMAND("bot_create", Cmd_BotCreate);
	//ADD_SERVER_COMMAND("uptime", Cmd_Uptime);
	ADD_SERVER_COMMAND("gamerulesinfo", Cmd_GameRulesInfo);
	ADD_SERVER_COMMAND("gamerulesupdate", Cmd_GameRulesUpdate);

	SERVER_COMMAND("exec skill.cfg\n");

	/*g_pServerAutoFile = LoadFile(g_szServerAutoFileName, "rwt");
	if (g_pServerAutoFile == NULL)
	{
		SERVER_PRINT("Error creating/opening server autofile!\n");
	}*/

	// XDM3035: causes infinite server reloads. This file must be used externally (from init.d script)
	/*if (IS_DEDICATED_SERVER() && STRING(gpGlobals->mapname) == "")
	{
		SERVER_PRINT("Executing server recovery config file\n");
		char szCommand[256];
		_snprintf(szCommand, 256, "exec %s\n", g_szServerAutoFileName);
		SERVER_COMMAND(szCommand);
	}*/
	SERVER_EXECUTE();// XDM3037
	//time(&g_tServerStartTime);// XDM3038a
	UTIL_LoadRawPalette("gfx/palette.lmp");// XDM3038c

	char szDirName[MAX_PATH];
	GET_GAME_DIR(szDirName);
	szDirName[MAX_PATH-1] = '\0';
	SERVER_PRINTF("Done. Server DLL initialized.\nGame directory: \"%s\"\n", szDirName);
}

//-----------------------------------------------------------------------------
// Purpose: Server DLL shutdown procedure
// WARNING: Only available through NEW_DLL_FUNCTIONS!!
//-----------------------------------------------------------------------------
void GameDLLShutdown(void)// XDM
{
	SERVER_PRINT("XDM: Server shutting down.\n");// UNDONE: print uptime
	//Cmd_Uptime();
	g_tServerStartTime = 0;
	if (g_pGameRules)
	{
		delete g_pGameRules;
		g_pGameRules = NULL;
	}
	/*if (g_pServerAutoFile)
	{
		fclose(g_pServerAutoFile);
		g_pServerAutoFile = NULL;
	}*/
	// XDM3035 crash recovery
	g_pServerAutoFile = LoadFile(g_szServerAutoFileName, "w");// Opens an empty file for writing. If the given file exists, its contents are destroyed.
	if (g_pServerAutoFile)
	{
		/*fseek(g_pServerAutoFile, 0L, SEEK_SET);
		time_t ltime;
		// Get UNIX-style time
		time(&ltime);
		fprintf(g_pServerAutoFile, "// SERVER WAS SHUT DOWN PROPERLY AT %s\n", ctime(&ltime));
		fflush(g_pServerAutoFile);// IMPORTANT: write to disk
		*/
		fclose(g_pServerAutoFile);
		g_pServerAutoFile = NULL;
		remove(g_szServerAutoFileName);// XDM3035 201101
		SERVER_PRINT("XDM: Server recovery config erased.\n");
	}
}
