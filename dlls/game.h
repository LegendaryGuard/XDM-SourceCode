#ifndef GAME_H
#define GAME_H

extern void GameDLLInit(void);
extern void GameDLLShutdown(void);// XDM

// Registered console commands
void Cmd_GlobalFog(void);
void Cmd_SetKV(void);
void Cmd_ListEnts(void);
void Cmd_SpawnEnt(void);
void Cmd_ClientCmd(void);
void Cmd_EndGame(void);
void Cmd_ServerDir(void);
void Cmd_ServerInitialize(void);
//void Cmd_NextMap(void);
//void Cmd_BotCreate(void);


extern DLL_GLOBAL FILE *g_pServerAutoFile;
extern DLL_GLOBAL const char g_szServerAutoFileName[];

#if defined (_DEBUG)
extern cvar_t test1;
extern cvar_t test2;
extern cvar_t test3;
#endif

//extern cvar_t bot_allow;

// Game settings
extern cvar_t displaysoundlist;
extern cvar_t sv_nodegraphdisable;
extern cvar_t sv_showpointentities;
extern cvar_t showtriggers;

extern cvar_t sv_bullet_distance;
extern cvar_t sv_script_cmd_affectplayers;
extern cvar_t sv_script_cmd_printoutput;
extern cvar_t sv_flame_effect;
extern cvar_t sv_glu_pri_rgb;
extern cvar_t sv_glu_sec_rgb;
extern cvar_t sv_razordisc_max_bounce;
extern cvar_t sv_rpg_nuclear;
extern cvar_t sv_tau_pri_rgb;
extern cvar_t sv_tau_sec_rgb;
extern cvar_t sv_tau_max_reflections;
extern cvar_t sv_tau_velocity_mult;
extern cvar_t sv_trip_delay;
extern cvar_t sv_trip_rgb;
extern cvar_t sv_satchel_delay;
//extern cvar_t sv_serverflame;

extern cvar_t sv_jumpaccuracy;
extern cvar_t sv_strings_alloc;

// multiplayer server rules
extern cvar_t mp_scoreleft;
extern cvar_t mp_timeleft;
extern cvar_t mp_gamerules;
extern cvar_t mp_maprules;
extern cvar_t mp_teamplay;
extern cvar_t mp_effects;
extern cvar_t sv_effects;
extern cvar_t sv_clientgibs;
extern cvar_t sv_playerpushable;
extern cvar_t mp_texsnd;
extern cvar_t mp_allowhighlight;
extern cvar_t mp_allowminimap;
extern cvar_t mp_allowmusicevents;
extern cvar_t mp_allowpowerfulweapons;
extern cvar_t mp_allowspectators;
extern cvar_t sv_radardist;
extern cvar_t sv_logchat;
extern cvar_t sv_lognotice;
extern cvar_t mp_scorelimit;
extern cvar_t mp_capturelimit;
extern cvar_t mp_fraglimit;
extern cvar_t mp_deathlimit;
extern cvar_t mp_timelimit;
extern cvar_t mp_overtime;
extern cvar_t mp_friendlyfire;
extern cvar_t mp_falldamage;
extern cvar_t mp_weaponstay;
extern cvar_t mp_ammorespawntime;
extern cvar_t mp_itemrespawntime;
extern cvar_t mp_weaponrespawntime;
extern cvar_t mp_forcerespawn;
extern cvar_t mp_forcerespawntime;
extern cvar_t mp_respawntime;
extern cvar_t mp_jointime;
extern cvar_t mp_flashlight;
extern cvar_t mp_spectoggle;
extern cvar_t mp_teamlist;
extern cvar_t mp_teamcolor1;
extern cvar_t mp_teamcolor2;
extern cvar_t mp_teamcolor3;
extern cvar_t mp_teamcolor4;
extern cvar_t mp_teambalance;
extern cvar_t mp_teamchange;
extern cvar_t mp_teamchangemax;
extern cvar_t mp_teamchangekill;
extern cvar_t mp_maxteams;
extern cvar_t mp_defaultteam;
extern cvar_t mp_allowmonsters;
extern cvar_t mp_monsterfrags;
extern cvar_t mp_monstersrespawn;
extern cvar_t mp_monstersrespawntime;
extern cvar_t mp_chattime;
extern cvar_t mp_weapondrop;
extern cvar_t mp_ammodrop;
extern cvar_t mp_adddefault;
extern cvar_t mp_dropdefault;
extern cvar_t mp_defaultitems;
extern cvar_t mp_noweapons;
extern cvar_t mp_pickuppolicy;
extern cvar_t mp_nofriction;
extern cvar_t mp_bannedentities;
extern cvar_t mp_flagstay;
extern cvar_t mp_domscoreperiod;
extern cvar_t mp_teammines;
extern cvar_t mp_droppeditemstaytime;
extern cvar_t mp_weaponboxbreak;
extern cvar_t mp_allowgaussjump;
extern cvar_t mp_allowcamera;
extern cvar_t mp_allowenemiesonmap;
extern cvar_t mp_laddershooting;
extern cvar_t mp_teammenu;
extern cvar_t mp_specteammates;
extern cvar_t mp_instagib;
extern cvar_t mp_telegib;
extern cvar_t mp_playerexplode;
extern cvar_t mp_playerstartarmor;
extern cvar_t mp_playerstarthealth;
extern cvar_t mp_spawnkill;
extern cvar_t mp_spawnpointforce;
extern cvar_t mp_spawnprotectiontime;
extern cvar_t mp_noshooting;
extern cvar_t mp_revengemode;
extern cvar_t mp_round_allowlatejoin;
extern cvar_t mp_round_minplayers;
extern cvar_t mp_round_time;
extern cvar_t mp_showgametitle;
extern cvar_t mp_firetargets_player;
extern cvar_t mp_coop_allowneutralspots;
extern cvar_t mp_coop_checktransition;
extern cvar_t mp_coop_commonspawnspot;
extern cvar_t mp_coop_eol_firstwin;
extern cvar_t mp_coop_eol_spectate;
extern cvar_t mp_coop_keepinventory;
extern cvar_t mp_coop_keepscore;
extern cvar_t mp_coop_usemaptransition;
extern cvar_t mp_lms_allowspectators;
extern cvar_t mp_precacheammo;
extern cvar_t mp_precacheitems;
extern cvar_t mp_precacheweapons;
extern cvar_t sv_allowstealvehicles;
extern cvar_t sv_overdrive;
extern cvar_t sv_serverstaticents;
extern cvar_t sv_modelhitboxes;
extern cvar_t sv_partialpickup;
extern cvar_t sv_playermaxhealth;
extern cvar_t sv_playermaxarmor;
extern cvar_t sv_weaponsscale;
extern cvar_t sv_generategamecfg;
extern cvar_t sv_loadentfile;
extern cvar_t sv_usemapequip;
//extern cvar_t sv_specnoclip;
extern cvar_t sv_reliability;
extern cvar_t sv_deleteoutsiteworld;
extern cvar_t sv_entinitinterval;
extern cvar_t sv_precacheammo;
extern cvar_t sv_precacheitems;
extern cvar_t sv_precacheweapons;
//extern cvar_t sv_oldweaponupdate;
//extern cvar_t sv_hack_triggerauto;
extern cvar_t sv_decalfrequency;
#if defined (_DEBUG_INFILOOPS)
extern cvar_t sv_dbg_maxcollisionloops;
#endif

// Engine Cvars
extern cvar_t *g_pCvarDeveloper;
extern cvar_t *g_psv_aim;
extern cvar_t *g_pviolence_hblood;
extern cvar_t *g_pviolence_ablood;
extern cvar_t *g_pviolence_hgibs;
extern cvar_t *g_pviolence_agibs;
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_friction;
extern cvar_t *g_psv_maxspeed;
extern cvar_t *g_pmp_footsteps;
extern cvar_t *g_psv_zmax;
extern cvar_t *g_psv_cheats;

#endif// GAME_H
