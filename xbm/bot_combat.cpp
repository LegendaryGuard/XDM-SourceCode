#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamedefs.h"// XDM
#include "bot.h"
#include "bot_cvar.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "pm_shared.h"// XDM3035c: VEC_HULL_*

// ~X~
// This all code completely sucks. It lacks handling of critical weapon properties and abilities, switching and aiming
// The only way to fix this would be to rewrite everything as object-oriented CBaseEntity code in the server DLL.

DLL_GLOBAL bot_weapon_select_t *g_pWeaponSelectTable = NULL;// XDM3035
DLL_GLOBAL bot_fire_delay_t *g_pWeaponDelayTable = NULL;
DLL_GLOBAL AmmoInfo g_AmmoInfoArray[MAX_AMMO_SLOTS];// XDM3037
DLL_GLOBAL int giAmmoIndex = 0;
// UNDONE: nobody uses maxammo info!!! probably CanPickup allows us to.

float react_delay_min[BOT_REACTION_LEVELS][BOT_SKILL_LEVELS] = {
	{0.01, 0.02, 0.03, 0.04, 0.05},
	{0.07, 0.09, 0.12, 0.14, 0.17},
	{0.10, 0.12, 0.15, 0.18, 0.21}};
float react_delay_max[BOT_REACTION_LEVELS][BOT_SKILL_LEVELS] = {
	{0.04, 0.06, 0.08, 0.10, 0.12},
	{0.11, 0.14, 0.18, 0.21, 0.25},
	{0.15, 0.18, 0.22, 0.25, 0.30}};

float aim_tracking_x_scale[BOT_SKILL_LEVELS] = {4.5, 4.0, 3.2, 2.5, 2.0};
float aim_tracking_y_scale[BOT_SKILL_LEVELS] = {4.5, 4.0, 3.2, 2.5, 2.0};

// weapons are stored in priority order, most desired weapon should be at
// the start of the array and least desired should be at the end

// This stuff should really be loaded from some spreadsheet...
// XDM3035b: finally got my hands on this mess
// XDM3038: turned bools into flags,
// NOTE: WEAPON_UF_NODAMAGE will make bots IGNORE this firemode!
// UNDONE: most flags are ignored by now :(
bot_weapon_select_t xdm_weapon_select[] =
{
//	id					skill	1MINdst	1MAXdst	2MINdst	2MAXdst		use%	pri%	minammo1 2	chrgtime1 2							useflags1				useflags2
	{WEAPON_BHG,			5,	480,		1024,	512,	1024,		100,	50,	BHG_AMMO_USE_MIN1,
																				BHG_AMMO_USE_MIN2,
																					BHG_AMMO_USE_INTERVAL*BHG_AMMO_USE_MAX1,
																					BHG_AMMO_USE_INTERVAL*BHG_AMMO_USE_MAX2,		WEAPON_UF_CHARGE|WEAPON_UF_ENDRADIUS,		WEAPON_UF_HOLD|WEAPON_UF_CHARGE|WEAPON_UF_ENDRADIUS},
	{WEAPON_BEAMRIFLE,		5,	64,			BOT_FAR_AIM_DISTANCE,
								512,		BOT_FAR_AIM_DISTANCE,		100,	25,	BEAMRIFLE_USE_AMMO1,BEAMRIFLE_USE_AMMO2,
																					BEAMRIFLE_CHARGE_INTERVAL1,
																					BEAMRIFLE_CHARGE_INTERVAL2,						WEAPON_UF_THRUGLASS,	WEAPON_UF_CHARGE|WEAPON_UF_THRUGLASS|WEAPON_UF_ENDRADIUS},
	{WEAPON_DISPLACER,		5,	320,		1024,	320,	1024,		100,	50,	DISPLACER_AMMO_USE1,DISPLACER_AMMO_USE2,
																					DISPLACER_CHARGE_TIME1,DISPLACER_CHARGE_TIME2,	WEAPON_UF_CHARGE|WEAPON_UF_ENDRADIUS,	WEAPON_UF_CHARGE|WEAPON_UF_ENDRADIUS},
	{WEAPON_RPG,			5,	400,		BOT_FAR_AIM_DISTANCE,
								800,		BOT_FAR_AIM_DISTANCE,		100,	95,		1,		0,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_ENDRADIUS,	WEAPON_UF_UNDERWATER},
	{WEAPON_PLASMA,			5,	128,		1024,	80,PLASMA_BEAM_DISTANCE,100,50,		1,		1,	0.0,	0.0,						WEAPON_UF_ENDRADIUS,						WEAPON_UF_HOLD},
	{WEAPON_FLAME,			5,	HULL_RADIUS*2,	FLAMETHROWER_DISTANCE1,
								HULL_RADIUS*4,	FLAMETHROWER_DISTANCE2,	100,	30,		1,		1,	0.0,	0.0,						WEAPON_UF_HOLD,								WEAPON_UF_HOLD},
	{WEAPON_STRTARGET,		5,	32,			256,	32,		256,		100,	100,	1,		0,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_TOSS,		WEAPON_UF_UNDERWATER},
	{WEAPON_EGON,			5,	56,			EGON_DISTANCE1,
								56,			EGON_DISTANCE2,				100,	80,		1,		0,	0.0,	0.0,						WEAPON_UF_HOLD,								WEAPON_UF_SWITCH},
	{WEAPON_GAUSS,			5,	HULL_RADIUS,	BOT_FAR_AIM_DISTANCE,
								HULL_RADIUS,	BOT_FAR_AIM_DISTANCE,	100,	50,	GAUSS_AMMO_USE1,GAUSS_AMMO_USE2,
																					0.0,	GAUSS_SHOCK_TIME-1,	WEAPON_UF_THRUGLASS,	WEAPON_UF_HOLD|WEAPON_UF_CHARGE|WEAPON_UF_THRUGLASS|WEAPON_UF_THRUWALLS},
	{WEAPON_SHOTGUN,		5,	0,			800,	8,		800,		100,	40,		1,		2,	0.0,	0.0,						WEAPON_UF_STRAIGHT,							WEAPON_UF_STRAIGHT},
	{WEAPON_PYTHON,			5,	0,			BOT_BULLET_DISTANCE,
								512,		BOT_BULLET_DISTANCE,		100,	100,	1,		0,	0.0,	0.0,						WEAPON_UF_STRAIGHT,							/*WEAPON_UF_SWITCH| don't*/WEAPON_UF_NODAMAGE},
	{WEAPON_GLAUNCHER,		5,	420,		1200,	420,	1280,		100,	70,		1,		1,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_TOSS|WEAPON_UF_ENDRADIUS,		WEAPON_UF_UNDERWATER|WEAPON_UF_TOSS|WEAPON_UF_ENDRADIUS},
	{WEAPON_ALAUNCHER,		5,	160,		800,	192,	864,		100,	50,		1,		2,	0.0,ALAUNCHER_AMMO_USE_DELAY*ALAUNCHER_MAX_CLIP,	WEAPON_UF_UNDERWATER|WEAPON_UF_TOSS|WEAPON_UF_ENDRADIUS,		WEAPON_UF_UNDERWATER|WEAPON_UF_CHARGE|WEAPON_UF_TOSS|WEAPON_UF_ENDRADIUS},// ALAUNCHER_AMMO_USE_DELAY*ALAUNCHER_MAX_CLIP, but no restriciton afterwards
	{WEAPON_DLAUNCHER,		5,	80,			1024,	80,		1024,		100,	60,		1,		0,	0.0,	0.0,						WEAPON_UF_STRAIGHT,							WEAPON_UF_STRAIGHT},
	{WEAPON_MP5,			5,	0,			BOT_BULLET_DISTANCE,
								400,		BOT_BULLET_DISTANCE,		100,	90,		1,		1,	0.0,	0.0,						WEAPON_UF_STRAIGHT,							WEAPON_UF_TOSS|WEAPON_UF_ENDRADIUS},
	{WEAPON_HANDGRENADE,	5,	256,		768,	256,	768,		100,	80,		1,		1,	HANDGRENADE_IGNITE_TIME/2,	0.0,	WEAPON_UF_UNDERWATER|WEAPON_UF_CHARGE|WEAPON_UF_TOSS|WEAPON_UF_ENDRADIUS,		WEAPON_UF_UNDERWATER|WEAPON_UF_SWITCH},
	{WEAPON_TRIPMINE,		5,	0,			10,		0,		10,			100,	100,	1,		0,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_PLANT,		WEAPON_UF_UNDERWATER|WEAPON_UF_PLANT},
	{WEAPON_SATCHEL,		5,	320,		600,	320,	600,		100,	60,		0,		1,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_PLANT|WEAPON_UF_TOSS|WEAPON_UF_COMBO2,	WEAPON_UF_UNDERWATER|WEAPON_UF_COMBO1},
	{WEAPON_SNARK,			5,	160,		500,	160,	256,		100,	40,		1,		5,	0.0,	0.0,						WEAPON_UF_TOSS,								WEAPON_UF_TOSS},
	{WEAPON_SNIPERRIFLE,	5,	256,		BOT_FAR_AIM_DISTANCE,
								1024,		BOT_FAR_AIM_DISTANCE,		100,	80,		1,		0,	0.0,	0.0,						WEAPON_UF_STRAIGHT,							WEAPON_UF_NODAMAGE},
	{WEAPON_CROSSBOW,		5,	256,		3072,	1024,	3600,		100,	80,		1,		0,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_ENDRADIUS,	WEAPON_UF_UNDERWATER|/*WEAPON_UF_SWITCH|*/WEAPON_UF_NODAMAGE},
	{WEAPON_HORNETGUN,		5,	16,			1800,	16,		1024,		100,	75,		1,		4,	0.0,	0.0,						WEAPON_UF_UNDERWATER,						WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD},
	{WEAPON_CHEMGUN,		5,	80,			1280,	320,	1280,		100,	70,		1,		0,	0.0,	0.0,						WEAPON_UF_ENDRADIUS,						WEAPON_UF_SWITCH},
	{WEAPON_GLOCK,			5,	0,			BOT_BULLET_DISTANCE,
								0,			BOT_BULLET_DISTANCE,		100,	70,		1,		1,	0.0,	0.0,						WEAPON_UF_UNDERWATER,						WEAPON_UF_UNDERWATER},
	{WEAPON_CUSTOM1,		5,	256,		2048,	256,	2048,		100,	50,		1,		1,	0.0,	0.0,						WEAPON_UF_UNDERWATER,						WEAPON_UF_UNDERWATER},// totally useless information
	{WEAPON_CUSTOM2,		5,	256,		2048,	256,	2048,		100,	50,		1,		1,	0.0,	0.0,						WEAPON_UF_UNDERWATER,						WEAPON_UF_UNDERWATER},// totally useless information
	{WEAPON_SWORD,			5,	0,			64,		0,		56,			100,	90,		0,		0,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD,		WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD},
	{WEAPON_CROWBAR,		5,	0,			56,		0,		56,			100,	100,	0,		0,	0.0,	0.0,						WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD,		WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD},
	{WEAPON_NONE,			0,	0,			0,		0,		0,			0,		0,		1,		1,	0.0,	0.0,						0,	0}// terminator
};

bot_fire_delay_t xdm_fire_delay[] =
{
	{WEAPON_BHG,
	BHG_AMMO_USE_INTERVAL*BHG_AMMO_USE_MAX1, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8},
	BHG_AMMO_USE_INTERVAL*BHG_AMMO_USE_MAX2, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8}},

	{WEAPON_BEAMRIFLE,
	BEAMRIFLE_CHARGE_INTERVAL1, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8},
	BEAMRIFLE_CHARGE_INTERVAL2, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8}},

	{WEAPON_DISPLACER,
	DISPLACER_CHARGE_TIME1, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8},
	DISPLACER_CHARGE_TIME2, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8}},

	{WEAPON_RPG,
	RPG_ATTACK_INTERVAL1, {1.0, 2.0, 3.0, 4.0, 5.0}, {2.0, 3.0, 4.0, 5.0, 6.0},
	RPG_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.1, 0.1, 0.1, 0.1, 0.1}},

	{WEAPON_PLASMA,
	PLASMA_ATTACK_INTERVAL1, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	PLASMA_ATTACK_INTERVAL2, {0.0, 0.0, 0.1, 0.1, 0.1}, {0.0, 0.1, 0.1, 0.1, 0.1}},

	{WEAPON_FLAME,
	FLAMETHROWER_ATTACK_INTERVAL1, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	FLAMETHROWER_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_STRTARGET,
	STRTARGET_ATTACK_INTERVAL1, {1.0, 1.1, 1.1, 1.2, 1.2}, {3.0, 4.0, 5.0, 6.0, 7.0},
	STRTARGET_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_EGON,
	EGON_ATTACK_INTERVAL1, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	EGON_ATTACK_INTERVAL2, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.4, 0.5, 0.6, 0.7, 0.8}},

	{WEAPON_GAUSS,
	GAUSS_ATTACK_INTERVAL1, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.6, 0.7, 0.8, 0.9, 1.0},
	GAUSS_ATTACK_INTERVAL2, {0.2, 0.3, 0.5, 0.8, 1.0}, {0.6, 0.8, 1.0, 1.5, 2.0}},

	{WEAPON_SHOTGUN,
	SHOTGUN_PUMP_TIME1, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.2, 0.5, 0.8, 1.2, 2.0},
	SHOTGUN_PUMP_TIME2, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.2, 0.5, 0.8, 1.2, 2.0}},

	{WEAPON_PYTHON,
	PYTHON_ATTACK_INTERVAL1, {0.0, 0.2, 0.4, 1.0, 1.5}, {0.25, 0.5, 0.8, 1.3, 2.2},
	PYTHON_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_GLAUNCHER,
	GLAUNCHER_ATTACK_INTERVAL1, {0.3, 0.9, 1.1, 1.3, 1.5}, {1.2, 1.4, 1.6, 1.8, 2.0},
	GLAUNCHER_ATTACK_INTERVAL2, {0.3, 0.9, 1.1, 1.3, 1.5}, {1.2, 1.4, 1.6, 1.8, 2.0}},

	{WEAPON_ALAUNCHER,
	ALAUNCHER_ATTACK_INTERVAL1, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.4, 0.6, 0.8, 1.0, 1.2},
	ALAUNCHER_ATTACK_INTERVAL2, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.4, 0.6, 0.8, 1.0, 1.2}},

	{WEAPON_DLAUNCHER,
	DLAUNCHER_ATTACK_INTERVAL1, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.4, 0.6, 0.8, 1.0, 1.2},
	DLAUNCHER_ATTACK_INTERVAL2, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.4, 0.6, 0.8, 1.0, 1.2}},

	{WEAPON_MP5,
	MP5_ATTACK_INTERVAL1, {0.0, 0.1, 0.2, 0.4, 0.5}, {0.1, 0.3, 0.4, 0.6, 0.8},
	MP5_ATTACK_INTERVAL2, {0.0, 0.4, 0.7, 1.0, 1.4}, {0.3, 0.7, 1.0, 1.6, 2.0}},

	{WEAPON_HANDGRENADE,
	HGUN_ATTACK_INTERVAL1, {1.0, 1.1, 1.1, 1.2, 1.2}, {3.0, 4.0, 5.0, 6.0, 7.0},
	HGUN_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.1, 0.1}, {0.0, 0.0, 0.1, 0.1, 0.1}},

	{WEAPON_TRIPMINE,
	TRIPMINE_ATTACK_INTERVAL1, {0.0, 0.1, 0.2, 0.4, 0.6}, {0.1, 0.2, 0.5, 0.7, 0.9},
	TRIPMINE_ATTACK_INTERVAL2, {0.0, 0.1, 0.2, 0.4, 0.6}, {0.1, 0.2, 0.5, 0.7, 0.9}},

	{WEAPON_SATCHEL,
	SATCHEL_ATTACK_INTERVAL1, {0.5, 0.6, 0.7, 0.8, 0.9}, {0.7, 0.8, 0.9, 1.0, 1.1},
	SATCHEL_ATTACK_INTERVAL2, {0.2, 0.3, 0.4, 0.5, 0.6}, {0.3, 0.4, 0.5, 0.6, 0.7}},

	{WEAPON_SNARK,
	SQUEAK_ATTACK_INTERVAL1, {0.0, 0.1, 0.2, 0.4, 0.6}, {0.1, 0.2, 0.5, 0.7, 1.0},
	SQUEAK_ATTACK_INTERVAL2, {0.5, 0.6, 0.7, 0.8, 0.9}, {1.0, 1.1, 1.2, 1.3, 1.4}},

	{WEAPON_SNIPERRIFLE,
	RIFLE_ATTACK_INTERVAL1, {0.0, 0.2, 0.5, 0.8, 1.0}, {0.3, 0.4, 0.7, 1.0, 1.3},
	RIFLE_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_CROSSBOW,
	CROSSBOW_ATTACK_INTERVAL1, {0.0, 0.2, 0.5, 0.8, 1.0}, {0.3, 0.4, 0.7, 1.0, 1.3},
	CROSSBOW_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_HORNETGUN,
	HGUN_ATTACK_INTERVAL1, {0.0, 0.3, 0.4, 0.6, 1.0}, {0.1, 0.4, 0.7, 1.0, 1.5},
	HGUN_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_CHEMGUN,
	CHEMGUN_ATTACK_INTERVAL1, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5},
	CHEMGUN_SWITCH_INTERVAL, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5}},

	{WEAPON_GLOCK,
	GLOCK_ATTACK_INTERVAL1, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5},
	GLOCK_ATTACK_INTERVAL2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4}},

	{WEAPON_CUSTOM1,
	0.5, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5},
	0.5, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5}},

	{WEAPON_CUSTOM2,
	0.5, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5},
	0.5, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5}},

	{WEAPON_SWORD,
	SWORD_ATTACK_INTERVAL1, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	SWORD_ATTACK_INTERVAL2, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_CROWBAR,
	CROWBAR_ATTACK_INTERVAL, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	CROWBAR_ATTACK_INTERVAL, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},

	{WEAPON_NONE,
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}}
};
// weapon firing delay based on skill (min and max delay for each weapon)
// THESE MUST MATCH THE SAME ORDER AS THE WEAPON SELECT ARRAY!!!
// f_shoot_time = gpGlobals->time + base_delay + RANDOM_FLOAT(min_delay[skill], max_delay[skill]);
/*
	primary_base_delay;		primary_min_delay[5];		primary_max_delay[5];
	secondary_base_delay;	secondary_min_delay[5];		secondary_max_delay[5];
*/



bot_weapon_select_t valve_weapon_select[] = {
	{VALVE_WEAPON_CROWBAR,		2,	0.0,	50.0,	0.0,	0.0,	100,	100,	0,		0,	0.0, 0.0,	WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD,	WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD},
	{VALVE_WEAPON_HANDGRENADE,	5,	250.0,	750.0,	0.0,	0.0,	30,		100,	1,		0,	0.0, 0.0,	WEAPON_UF_UNDERWATER|WEAPON_UF_TOSS,	WEAPON_UF_UNDERWATER|WEAPON_UF_SWITCH},
	{VALVE_WEAPON_SNARK,		5,	150.0,	500.0,	0.0,	0.0,	50,		100,	1,		0,	0.0, 0.0,	WEAPON_UF_TOSS,							WEAPON_UF_TOSS|WEAPON_UF_NODAMAGE},// in HL there is no secondary mode
	{VALVE_WEAPON_EGON,			5,	0.0,	9999.0,	0.0,	0.0,	100,	100,	1,		0,	0.0, 0.0,	WEAPON_UF_HOLD,							WEAPON_UF_SWITCH},
	{VALVE_WEAPON_GAUSS,		5,	0.0,	9999.0,	0.0,	9999.0,	100,	80,		1,		10,	0.0, 0.8,	WEAPON_UF_THRUGLASS,					WEAPON_UF_HOLD|WEAPON_UF_CHARGE|WEAPON_UF_THRUGLASS|WEAPON_UF_THRUWALLS},
	{VALVE_WEAPON_SHOTGUN,		5,	30.0,	150.0,	30.0,	150.0,	100,	70,		1,		2,	0.0, 0.0,	WEAPON_UF_STRAIGHT,						WEAPON_UF_STRAIGHT},
	{VALVE_WEAPON_PYTHON,		5,	30.0,	700.0,	0.0,	0.0,	100,	100,	1,		0,	0.0, 0.0,	WEAPON_UF_STRAIGHT,						/*WEAPON_UF_SWITCH| don't*/WEAPON_UF_NODAMAGE},
	{VALVE_WEAPON_HORNETGUN,	5,	30.0,	1000.0,	30.0,	1000.0,	100,	50,		1,		4,	0.0, 0.0,	WEAPON_UF_UNDERWATER,					WEAPON_UF_UNDERWATER|WEAPON_UF_HOLD},
	{VALVE_WEAPON_MP5,			5,	0.0,	250.0,	300.0,	600.0,	100,	90,		1,		1,	0.0, 0.0,	WEAPON_UF_STRAIGHT,						WEAPON_UF_TOSS},
	{VALVE_WEAPON_CROSSBOW,		5,	100.0,	1000.0,	0.0,	0.0,	100,	100,	1,		0,	0.0, 0.0,	WEAPON_UF_UNDERWATER,					WEAPON_UF_UNDERWATER|/*WEAPON_UF_SWITCH|*/WEAPON_UF_NODAMAGE},
	{VALVE_WEAPON_RPG,			5,	300.0,	9999.0,	0.0,	0.0,	100,	100,	1,		0,	0.0, 0.0,	WEAPON_UF_UNDERWATER,					WEAPON_UF_UNDERWATER},
	{VALVE_WEAPON_GLOCK,		5,	0.0,	1200.0,	0.0,	1200.0,	100,	70,		1,		1,	0.0, 0.0,	WEAPON_UF_UNDERWATER,					WEAPON_UF_UNDERWATER},
	{VALVE_WEAPON_NONE,			0,	0.0,	0.0,	0.0,	0.0,	0,		0,		1,		1,	0.0, 0.0,	0,	0}/* terminator */
};

bot_fire_delay_t valve_fire_delay[] = {
	{VALVE_WEAPON_CROWBAR,
	0.3, {0.0, 0.2, 0.3, 0.4, 0.6}, {0.1, 0.3, 0.5, 0.7, 1.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_HANDGRENADE,
	0.1, {1.0, 2.0, 3.0, 4.0, 5.0}, {3.0, 4.0, 5.0, 6.0, 7.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_SNARK,
	0.1, {0.0, 0.1, 0.2, 0.4, 0.6}, {0.1, 0.2, 0.5, 0.7, 1.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_EGON,
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_GAUSS,
	0.2, {0.0, 0.2, 0.3, 0.5, 1.0}, {0.1, 0.3, 0.5, 0.8, 1.2},
	1.0, {0.2, 0.3, 0.5, 0.8, 1.2}, {0.5, 0.7, 1.0, 1.5, 2.0}},
	{VALVE_WEAPON_SHOTGUN,
	0.75, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.25, 0.5, 0.8, 1.2, 2.0},
	1.5, {0.0, 0.2, 0.4, 0.6, 0.8}, {0.25, 0.5, 0.8, 1.2, 2.0}},
	{VALVE_WEAPON_PYTHON,
	0.75, {0.0, 0.2, 0.4, 1.0, 1.5}, {0.25, 0.5, 0.8, 1.3, 2.2},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_HORNETGUN,
	0.25, {0.0, 0.25, 0.4, 0.6, 1.0}, {0.1, 0.4, 0.7, 1.0, 1.5},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_MP5,
	0.1, {0.0, 0.1, 0.25, 0.4, 0.5}, {0.1, 0.3, 0.45, 0.65, 0.8},
	1.0, {0.0, 0.4, 0.7, 1.0, 1.4}, {0.3, 0.7, 1.0, 1.6, 2.0}},
	{VALVE_WEAPON_CROSSBOW,
	0.75, {0.0, 0.2, 0.5, 0.8, 1.0}, {0.25, 0.4, 0.7, 1.0, 1.3},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_RPG,
	1.5, {1.0, 2.0, 3.0, 4.0, 5.0}, {3.0, 4.0, 5.0, 6.0, 7.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
	{VALVE_WEAPON_GLOCK,
	0.3, {0.0, 0.1, 0.2, 0.3, 0.4}, {0.1, 0.2, 0.3, 0.4, 0.5},
	0.2, {0.0, 0.0, 0.1, 0.1, 0.2}, {0.1, 0.1, 0.2, 0.2, 0.4}},
	/* terminator */
	{0, 0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
	0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}}
};


//-----------------------------------------------------------------------------
// Purpose: once per DLL
//-----------------------------------------------------------------------------
bool BotWeaponInit(void)
{
	DBG_BOT_PRINT("BotWeaponInit(%hd)\n", mod_id);
	if (mod_id == GAME_XDM_DLL)// XDM
	{
		ASSERT(ARRAYSIZE(xdm_weapon_select) == ARRAYSIZE(xdm_fire_delay));
		g_pWeaponSelectTable = &xdm_weapon_select[0];
		g_pWeaponDelayTable = &xdm_fire_delay[0];
	}
	else
	{
		ASSERT(ARRAYSIZE(valve_weapon_select) == ARRAYSIZE(valve_fire_delay));
		g_pWeaponSelectTable = &valve_weapon_select[0];
		g_pWeaponDelayTable = &valve_fire_delay[0];
	}
	uint32 i = 0;
	bool error = false;
	while (g_pWeaponSelectTable[i].iId != WEAPON_NONE)
	{
		if (ASSERT(g_pWeaponSelectTable[i].iId == g_pWeaponDelayTable[i].iId) == false)
		{
			conprintf(0, "XBM: BotWeaponInit() ERROR: g_pWeaponSelectTable[%d] has ID %d while g_pWeaponDelayTable[%d] has ID %d!\n", i, g_pWeaponSelectTable[i].iId, i, g_pWeaponDelayTable[i].iId);
			error = true;
		}
		++i;
	}
	return !error;
}

//-----------------------------------------------------------------------------
// Purpose: Get weapon index in bot_weapon_select_t/bot_fire_delay_t array
// Input  : iId - WEAPON_NONE
// Output : int -1 == failure
//-----------------------------------------------------------------------------
int GetWeaponSelectIndex(const int &iId)
{
	int select_index = 0;// XDM3035: find this item's index in selection priority array
	while (g_pWeaponSelectTable[select_index].iId != WEAPON_NONE)
	{
		if (g_pWeaponSelectTable[select_index].iId == iId)
			return select_index;

		++select_index;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Get next best item (generic)
// In theory, it is possible to personalize this function for every bot. In practice, we don't bother.
// Input  : *pBot - 
//			&iCurrentId - 
// Output : int
//-----------------------------------------------------------------------------
int BotGetNextBestWeapon(const bot_t *pBot, const int &iCurrentId)
{
	DBG_BOT_PRINT("BotGetNextBestWeapon(%s, %d)\n", pBot->name, iCurrentId);
	int i = 0;// weapons are already sorted by preference for bots
	while (g_pWeaponSelectTable[i].iId != WEAPON_NONE)
	{
		if (g_pWeaponSelectTable[i].iId != iCurrentId)
		{
			if (pBot == NULL || pBot->is_used == false || pBot->pEdict == NULL || BotHasWeapon(pBot, g_pWeaponSelectTable[i].iId))// && WeaponIsSelectable() :(
			{
				return g_pWeaponSelectTable[i].iId;
				break;
			}
		}
		++i;
	}
	return WEAPON_NONE;// we failed so hard
}

//-----------------------------------------------------------------------------
// Purpose: Does this bot have specified weapon?
// Input  : *pBot - 
//			&iId - WEAPON_NONE
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BotHasWeapon(const bot_t *pBot, const int &iId)
{
	if (pBot && pBot->pEdict)
		return FBitSet(pBot->pEdict->v.weapons, 1<<iId);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Does this bot have any weapons?
// Input  : *pBot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BotHasWeapons(const bot_t *pBot)
{
	if (pBot && pBot->pEdict)
		return (FBitExclude(pBot->pEdict->v.weapons, (1<<WEAPON_SUIT))) != 0;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame if this bot has a weapon to find an enemy
// Input  : *pBot - 
// Output : edict_t - new enemy
//-----------------------------------------------------------------------------
edict_t *BotFindEnemy(bot_t *pBot)
{
	if (g_bot_dont_shoot.value > 0.0f || (g_pmp_noshooting && g_pmp_noshooting->value > 0.0f))// XDM3035c
		return NULL;

	DBG_BOT_PRINT("BotFindEnemy(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	if (pBot->pBotEnemy)// does the bot already have an enemy?
	{
		// if the enemy is dead?
		if (!IsAlive(pBot->pBotEnemy))  // is the enemy dead?, assume bot killed it
		{
			pBot->pBotEnemy = NULL;// don't have an enemy anymore so null out the pointer...
		}
		else// enemy is still alive
		{
			Vector vecEnd = pBot->pBotEnemy->v.origin + pBot->pBotEnemy->v.view_ofs;

			if (FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict) && (g_bot_check_lightlevel.value == 0.0f || pBot->pBotEnemy->v.light_level >= 14))
			{
				// if enemy is still visible and in field of view, keep it face the enemy
				//Vector v_enemy = /*pBot->pBotEnemy->v.origin*/vecEnd - pEdict->v.origin;
				//Vector bot_angles = UTIL_VecToAngles(v_enemy);
				//pEdict->v.ideal_yaw = bot_angles.y;
				pEdict->v.ideal_yaw = VecToYaw(vecEnd - pEdict->v.origin);
				BotFixIdealYaw(pEdict);
				// keep track of when we last saw an enemy
				pBot->f_bot_see_enemy_time = gpGlobals->time;
				pBot->enemy_visible = 1;
				return pBot->pBotEnemy;
			}
			else  // enemy has gone out of bot's line of sight
				pBot->enemy_visible = 0;
		}
	}

	edict_t *pNewEnemy = NULL;
	vec_t nearestdistance;
	//pBot->enemy_attack_count = 0;  // don't limit number of attacks
	if (pNewEnemy == NULL)
	{
		Vector vecEnd;
		nearestdistance = BOT_SEARCH_MAX_DISTANCE;
		pBot->enemy_visible = 0;

		if (g_iGameType != GT_COOP)// search for players
		{
			CLIENT_INDEX player_index;
			// search the world for players...
			for (player_index = 1; player_index <= gpGlobals->maxClients; ++player_index)
			{
				edict_t *pPlayer = INDEXENT(player_index);
				// skip invalid players and skip self (i.e. this bot)
				if (pPlayer && !pPlayer->free && (pPlayer != pEdict))
				{
					if (pBot->need_to_avenge)// XDM3035c: in revenge-only mode
					{
						if (g_pmp_revengemode && g_pmp_revengemode->value > 1.0f)// XDM3035c: revenge-only mode!
						{
							if (pBot->pBotKiller && pPlayer != pBot->pBotKiller)// ignore everyone else for now
								continue;
						}
					}
					if (!IsAlive(pPlayer))// skip this player if not alive (i.e. dead or dying)
						continue;
					//if ((b_observer_mode) && !(pPlayer->v.flags & FL_FAKECLIENT) && !(pPlayer->v.flags & FL_THIRDPARTYBOT))
					//	continue;

					if (pBot->pBotUser != NULL)
					{
						if (pPlayer == pBot->pBotUser)
							continue;// XDM: this 'player' is bot's 'user'

						bot_t *pNewEnemyBot = UTIL_GetBotPointer(pPlayer);
						if (pNewEnemyBot && (pNewEnemyBot->pBotUser == pBot->pBotUser))
							continue;// XDM: this 'player' is a bot and following the same 'user'
					}
					vecEnd = pPlayer->v.origin; vecEnd += pPlayer->v.view_ofs;
					// see if bot can't see the player...
					if (!FInViewCone(vecEnd, pEdict) || !FVisible(vecEnd, pEdict))
						continue;

					// is team play enabled?
					if (IsTeamplay() && pEdict->v.team > 0)
					{
						if (pEdict->v.team == pPlayer->v.team)// don't target your teammates...
							continue;
					}

					vec_t distance = (pPlayer->v.origin - pEdict->v.origin).Length();
					if (distance < nearestdistance)
					{
						nearestdistance = distance;
						pNewEnemy = pPlayer;
					// XDMpBot->pBotUser = NULL;  // don't follow user when enemy found
						pBot->enemy_visible = 1;
					}
				}
			}// for
		}// !COOP

		if (pNewEnemy == NULL)// no player enemies found
		{
			edict_t *pent = NULL;
			char ent_name[40];
			nearestdistance = BOT_SEARCH_MAX_DISTANCE;
			// search the world for monsters...
			while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, BOT_SEARCH_ENEMY_RADIUS)) != NULL)
			{
				if (pent->v.effects & EF_NODRAW)// faster checks go first!
					continue;
				if (pent->v.takedamage == DAMAGE_NO || pent->v.health <= 0)
					continue;
				if (!(pent->v.flags & FL_MONSTER))
					continue; // discard anything that is not a monster
				if (!IsAlive(pent))
					continue; // discard dead or dying monsters

				vecEnd = pent->v.origin;// + pent->v.view_ofs;
				vecEnd += pent->v.view_ofs;
				if (!FVisible(vecEnd, pEdict) || !FInViewCone(vecEnd, pEdict))
					continue;

				// NEW: XDM3035a: server DLL allows advanced and fast checks on entities
				if (mod_id == GAME_XDM_DLL && XDM_EntityIs && XDM_EntityRelationship)
				{
					int entitytype = XDM_EntityIs(pent);
					if (entitytype == ENTIS_PLAYER || entitytype == ENTIS_MONSTER || entitytype == ENTIS_HUMAN)// monsters
					{
						if (pent->v.framerate <= 0)// is this turret active?
							continue;
						if (pent->v.flags & FL_GODMODE)// XDM3035c: avoid invulnerable monsters
						{
							pBot->f_grenade_found_time = gpGlobals->time;
							pBot->f_move_speed = pBot->f_max_speed;// will be reversed later
							continue;
						}
						if (IsTeamplay())// don't shoot at your own turrets
						{
							if (pEdict->v.team > 0 && pEdict->v.team == pent->v.team)
								continue;
						}
						if (XDM_EntityRelationship(pent, pEdict) > 0)// R_DL, etc.
						{
							vec_t distance = (vecEnd - pEdict->v.origin).Length();
							if (distance < nearestdistance)
							{
								nearestdistance = distance;
								pNewEnemy = pent;
							}
						}
					}
					else
						continue;
				}
				else// ordinary HL uses older and slower methods
				{
					strcpy(ent_name, STRING(pent->v.classname));
					if (FStrnEq("monster_", ent_name, 8))// check if entity is a monster
					{
						if (FStrEq("monster_satchel", ent_name) ||
							FStrnEq("monster_barney", ent_name, 14) ||
							FStrEq("monster_bloater", ent_name) ||
							FStrnEq("monster_cat", ent_name, 11) ||
							FStrEq("monster_cockroach", ent_name) ||
							FStrnEq("monster_flyer", ent_name, 13) ||
							FStrEq("monster_furniture", ent_name) ||
							FStrEq("monster_generic", ent_name) ||
							FStrEq("monster_gman", ent_name) ||
							FStrEq("monster_hevsuit_dead", ent_name) ||
							FStrEq("monster_hgrunt_dead", ent_name) ||
							FStrEq("monster_leech", ent_name) ||
							FStrEq("monster_mortar", ent_name) ||
							FStrEq("monster_player", ent_name) ||
							FStrEq("monster_rat", ent_name) ||
							FStrEq("monster_satchel", ent_name) ||
							FStrnEq("monster_scientist", ent_name, 17) ||
							FStrEq("monster_sitting_scientist", ent_name) ||
							FStrEq("monster_space_ship", ent_name) ||
							FStrEq("monster_target", ent_name) ||
							FStrnEq("monster_tentacle", ent_name, 16) ||
							FStrEq("monster_tripmine", ent_name))
						{
							continue;
						}
						else if (FStrEq("monster_miniturret", ent_name) ||
								FStrEq("monster_sentry", ent_name) ||
								FStrEq("monster_turret", ent_name))
						{
							if (pent->v.framerate <= 0)// is this turret active?
								continue;

							if (IsTeamplay())// don't shoot at your own turrets
							{
								if (pEdict->v.team > 0 && pEdict->v.team == pent->v.team)
									continue;
							}
						}
						else// already if (pent->v.flags & FL_MONSTER)
						{
							vec_t distance3 = (vecEnd - pEdict->v.origin).Length();
							if (distance3 < nearestdistance)
							{
								nearestdistance = distance3;
								pNewEnemy = pent;
							}
						}
					}// strings
				}// XDM

			}// while
		}// !pNewEnemy
	}
	if (pNewEnemy)
	{
		Vector v_enemy(pNewEnemy->v.origin); v_enemy -= pEdict->v.origin;
		Vector bot_angles;
		VectorAngles(v_enemy, bot_angles);
//#if !defined (NOSQB)
		bot_angles.x = -bot_angles.x;// adjust the view angle pitch to aim correctly
//#endif
		pEdict->v.ideal_yaw = bot_angles.y;
		BotFixIdealYaw(pEdict);
		// keep track of when we last saw an enemy
		pBot->f_bot_see_enemy_time = gpGlobals->time;
		if (pBot->reaction > 0 && pBot->f_reaction_target_time == 0.0f)// XDM3035c: don't constantly increase this delay!!
			pBot->f_reaction_target_time = gpGlobals->time + RANDOM_FLOAT(react_delay_min[pBot->reaction][pBot->bot_skill], react_delay_max[pBot->reaction][pBot->bot_skill]);
	}
	// has the bot NOT seen an ememy for at least 5 seconds (time to reload)?
	if ((pBot->f_bot_see_enemy_time > 0.0) && ((pBot->f_bot_see_enemy_time + 5.0) <= gpGlobals->time))
	{
		pBot->f_bot_see_enemy_time = -1.0f;// so we won't keep reloading
		//if (pBot->current_weapon.iClip < maxClip ???)
		pEdict->v.button |= IN_RELOAD;// press reload button
		// initialize aim tracking angles...
		pBot->f_aim_x_angle_delta = 5.0f;
		pBot->f_aim_y_angle_delta = 5.0f;
	}
	return (pNewEnemy);
}

//-----------------------------------------------------------------------------
// Purpose: // XDM3037: I hate all this code!!!
// Input  : *pBot - 
//			&fDistToTarget - 
// Output : int - weapon ID
//-----------------------------------------------------------------------------
int BotChooseWeapon(bot_t *pBot, const vec_t &fDistToTarget)// , int weapon_choice)
{
	DBG_BOT_PRINT("BotChooseWeapon(%s)\n", pBot->name);
	int select_index = 0;
	int iId = WEAPON_NONE;
	int use_percent = 0;
	edict_t *pEdict = pBot->pEdict;
	bool use_primary, use_secondary;
	bool primary_in_range, secondary_in_range;
	// loop through all the weapons until terminator is found...
	while (g_pWeaponSelectTable[select_index].iId)
	{
		// was a weapon choice specified? (and if so do they NOT match?)
		/*if ((weapon_choice != 0) && (weapon_choice != g_pWeaponSelectTable[select_index].iId))
		{
			select_index++;  // skip to next weapon
			continue;
		}*/
		// is the bot NOT carrying this weapon?
		if (!BotHasWeapon(pBot, g_pWeaponSelectTable[select_index].iId))
		{
			++select_index;
			continue;
		}

		// XDM3038c
		if (pBot->m_iWeaponState[g_pWeaponSelectTable[select_index].iId] == wstate_unusable)
		{
			++select_index;
			continue;
		}

		// is the bot NOT skilled enough to use this weapon?
		if ((pBot->bot_skill+1) > g_pWeaponSelectTable[select_index].skill_level)
		{
			++select_index;
			continue;
		}
		// is the bot underwater and does this weapon NOT work under water?
		if ((pEdict->v.waterlevel == 3) && !(g_pWeaponSelectTable[select_index].primary_usage_flags & WEAPON_UF_UNDERWATER)
										&& !(g_pWeaponSelectTable[select_index].secondary_usage_flags & WEAPON_UF_UNDERWATER))
		{
			++select_index;
			continue;
		}
		use_percent = RANDOM_LONG(1, 100);
		// is use percent greater than weapon use percent?
		if (use_percent > g_pWeaponSelectTable[select_index].use_percent)
		{
			++select_index;
			continue;
		}

		iId = g_pWeaponSelectTable[select_index].iId;
		primary_in_range = (fDistToTarget >= g_pWeaponSelectTable[select_index].primary_min_distance) && (fDistToTarget <= g_pWeaponSelectTable[select_index].primary_max_distance);
		secondary_in_range = (fDistToTarget >= g_pWeaponSelectTable[select_index].secondary_min_distance) && (fDistToTarget <= g_pWeaponSelectTable[select_index].secondary_max_distance);

		/*if (weapon_choice != 0)
		{
			primary_in_range = true;
			secondary_in_range = true;
		}*/

		use_primary = false;
		use_secondary = false;
		// weapon HAS primary mode AND
		// player is NOT underwater OR mode can be used underwater
		// no ammo required for this weapon OR
		// enough ammo available to fire AND
		// the bot is far enough away to use primary fire AND
		// the bot is close enough to the enemy to use primary fire
		if ((g_pWeaponSelectTable[select_index].primary_fire_percent > 0) &&
			((pEdict->v.waterlevel < 3) || (g_pWeaponSelectTable[select_index].primary_usage_flags & WEAPON_UF_UNDERWATER)) &&
				((weapon_defs[iId].iAmmo1 == AMMOINDEX_NONE) ||
				(pBot->m_rgAmmo[weapon_defs[iId].iAmmo1] >= g_pWeaponSelectTable[select_index].min_primary_ammo)) &&
			(primary_in_range && !(g_pWeaponSelectTable[select_index].primary_usage_flags & WEAPON_UF_NODAMAGE)))// XDM3038c: fixed typo, +switch-only mode check (useless without ammo anyway)
		{
			use_primary = true;
		}// otherwise see if there is enough secondary ammo AND the bot is far enough away to use secondary fire AND the bot is close enough to the enemy to use secondary fire
		else if (((pEdict->v.waterlevel < 3) || (g_pWeaponSelectTable[select_index].secondary_usage_flags & WEAPON_UF_UNDERWATER)) &&
				((weapon_defs[iId].iAmmo2 == AMMOINDEX_NONE) ||
				(pBot->m_rgAmmo[weapon_defs[iId].iAmmo2] >= g_pWeaponSelectTable[select_index].min_secondary_ammo)) &&
			(secondary_in_range && !(g_pWeaponSelectTable[select_index].secondary_usage_flags & WEAPON_UF_NODAMAGE)))// XDM3038c: fixed typo, +check
		{
			use_secondary = true;
		}

		// see if there wasn't enough ammo to fire the weapon...
		if ((use_primary == false) && (use_secondary == false))
		{
			++select_index;// skip to next weapon
			continue;
		}

		// select this weapon if it isn't already selected
		if (pBot->current_weapon.iId != iId)
		{
			if (mod_id == GAME_XDM_DLL)// XDM3035: new fast protocol
			{
				//UTIL_SelectItem(pEdict, g_pWeaponSelectTable[select_index].iId);
				//UTIL_SelectWeapon(pEdict, g_pWeaponSelectTable[select_index].iId);// XDM3037a
				pBot->m_iQueuedSelectWeaponID = g_pWeaponSelectTable[select_index].iId;// XDM3038
			}
			else
				UTIL_SelectItem(pEdict, weapon_defs[g_pWeaponSelectTable[select_index].iId].szClassname);// XDM3038b
		}
		DBG_BOT_PRINT("BotChooseWeapon(%s): return %d\n", pBot->name, iId);
		return iId;
	}
	DBG_BOT_PRINT("BotChooseWeapon(%s): fail\n", pBot->name);
	return WEAPON_NONE;// didn't have any available weapons or ammo, don't return iId as it may contain random stuff by now
}

//-----------------------------------------------------------------------------
// Purpose: Fire/start charging/continue charging/use current weapon. Actually presses buttons.
// Input  : *pBot - 
//			&fDistToTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BotFireWeapon(bot_t *pBot, const vec_t &fDistToTarget)//, const int &weapon_choice)
{
	if (g_pWeaponSelectTable == NULL)
		return false;

	int iId = pBot->current_weapon.iId;
	DBG_BOT_PRINT("BotFireWeapon(%s) iId = %d\n", pBot->name, iId);
	if (iId == WEAPON_NONE)
		return false;
	int select_index = GetWeaponSelectIndex(iId);
	if (select_index < 0)
		return false;

	edict_t *pEdict = pBot->pEdict;
	// are we charging the primary fire?
	if (pBot->f_primary_charging > 0)
	{
		//iId = pBot->current_weapon.iId;//pBot->charging_weapon_id;
		// is it time to fire the charged weapon?
		if (pBot->f_primary_charging <= gpGlobals->time)
		{
			// we DON'T set pEdict->v.button here to release the fire button which will fire the charged weapon
			pBot->f_primary_charging = -1.0f;// -1 means not charging
			// find the correct fire delay for this weapon
			// set next time to shoot
			pBot->f_shoot_time = gpGlobals->time + g_pWeaponDelayTable[select_index].primary_base_delay +
						RANDOM_FLOAT(g_pWeaponDelayTable[select_index].primary_min_delay[pBot->bot_skill],
									g_pWeaponDelayTable[select_index].primary_max_delay[pBot->bot_skill]);
			return true;
		}
		else
		{
			pEdict->v.button |= IN_ATTACK;// charge the weapon
			pBot->f_shoot_time = gpGlobals->time;// keep charging
			return true;
		}
	}
	else if (pBot->b_primary_holding)// no! (g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].primary_fire_hold)
	{
		if (pBot->m_rgAmmo[weapon_defs[pBot->current_weapon.iId].iAmmo1] >= g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].min_primary_ammo)
		{
			pEdict->v.button |= IN_ATTACK;// charge the weapon
			pBot->f_shoot_time = gpGlobals->time;// keep charging
			return true;		
		}
	}

	// are we charging the secondary fire?
	if (pBot->f_secondary_charging > 0)
	{
		//iId = pBot->current_weapon.iId;//pBot->charging_weapon_id;
		// is it time to fire the charged weapon?
		if (pBot->f_secondary_charging <= gpGlobals->time)
		{
			// we DON'T set pEdict->v.button here to release the fire button which will fire the charged weapon
			pBot->f_secondary_charging = -1.0f;// -1 means not charging
			// find the correct fire delay for this weapon
			// set next time to shoot
			pBot->f_shoot_time = gpGlobals->time + g_pWeaponDelayTable[select_index].secondary_base_delay +
						RANDOM_FLOAT(g_pWeaponDelayTable[select_index].secondary_min_delay[pBot->bot_skill],
									g_pWeaponDelayTable[select_index].secondary_max_delay[pBot->bot_skill]);
			return true;
		}
		else
		{
			pEdict->v.button |= IN_ATTACK2;  // charge the weapon
			pBot->f_shoot_time = gpGlobals->time;  // keep charging
			return true;
		}
	}
	else if (pBot->b_secondary_holding)
	{
		if (pBot->m_rgAmmo[weapon_defs[pBot->current_weapon.iId].iAmmo2] >= g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].min_secondary_ammo)
		{
			pEdict->v.button |= IN_ATTACK2;
			pBot->f_shoot_time = gpGlobals->time;
			return true;		
		}
	}
	DBG_BOT_PRINT("BotFireWeapon(%s): not charging.\n", pBot->name);

	// p1
	int primary_percent = RANDOM_LONG(1, 100);
	bool primary_in_range = (fDistToTarget >= g_pWeaponSelectTable[select_index].primary_min_distance) && (fDistToTarget <= g_pWeaponSelectTable[select_index].primary_max_distance);
	bool secondary_in_range = (fDistToTarget >= g_pWeaponSelectTable[select_index].secondary_min_distance) && (fDistToTarget <= g_pWeaponSelectTable[select_index].secondary_max_distance);
	bool use_primary = false;
	bool use_secondary = false;

	// is primary percent less than weapon primary percent AND
	// no ammo required for this weapon OR enough ammo/clip available to fire AND
	// the bot is far enough away to use primary fire AND
	// the bot is close enough to the enemy to use primary fire
	if ((primary_percent <= g_pWeaponSelectTable[select_index].primary_fire_percent) &&
		((weapon_defs[iId].iAmmo1 == AMMOINDEX_NONE) ||
			(pBot->m_rgAmmo[weapon_defs[iId].iAmmo1] >= g_pWeaponSelectTable[select_index].min_primary_ammo) ||
			(pBot->current_weapon.iClip >= g_pWeaponSelectTable[select_index].min_primary_ammo)) &&
		(primary_in_range))// XDM3037a: added clip!
	{
		use_primary = true;
	}// otherwise see if there is enough secondary ammo AND the bot is far enough away to use secondary fire AND the bot is close enough to the enemy to use secondary fire
	else if (((weapon_defs[iId].iAmmo2 == AMMOINDEX_NONE) || (pBot->m_rgAmmo[weapon_defs[iId].iAmmo2] >= g_pWeaponSelectTable[select_index].min_secondary_ammo)) &&
		(secondary_in_range))
	{
		use_secondary = true;
	}

	// see if there wasn't enough ammo to fire the weapon...
	if ((use_primary == false) && (use_secondary == false))
	{
		BotPrintf(pBot, "BotFireWeapon() failed: cannot use any mode.\n");
		return false;
	}
	if (g_pWeaponDelayTable[select_index].iId != iId)
	{
		BotPrintf(pBot, "BotFireWeapon() failed: fire_delay mismatch for weapon id=%d!\n", iId);
		return false;
	}

	if ((use_primary && g_pWeaponSelectTable[select_index].primary_max_distance < 50) ||
		(use_secondary && g_pWeaponSelectTable[select_index].secondary_max_distance < 50))
	{
		// check if bot needs to duck down to hit enemy...
		if (pBot->pBotEnemy->v.origin.z < (pEdict->v.origin.z - 30))
			pBot->f_duck_time = gpGlobals->time + 1.0f;
	}

	// p2
	pBot->b_primary_holding = false;
	pBot->b_secondary_holding = false;

	// ATTACK!
	if (use_primary)
	{
		pEdict->v.button |= IN_ATTACK;// primary attack
		if (g_pWeaponSelectTable[select_index].primary_usage_flags & WEAPON_UF_CHARGE)
		{
			pBot->charging_weapon_id = iId;
			// release primary fire after the appropriate delay...
			if (g_pWeaponSelectTable[select_index].primary_usage_flags & WEAPON_UF_HOLD)// XDM3038c
			{
				pBot->f_primary_charging = gpGlobals->time + g_pWeaponSelectTable[select_index].primary_charge_delay;
				pBot->f_shoot_time = gpGlobals->time;// keep charging
			}
		}
		else if (g_pWeaponSelectTable[select_index].primary_usage_flags & WEAPON_UF_HOLD)
		{
			//conprintf(1, "XBM: BotFireWeapon() id=%d primary_fire_hold\n", iId);
			pBot->b_primary_holding = true;
			pBot->f_shoot_time = gpGlobals->time;// don't let button up
		}
		else// set next time to shoot
		{
			pBot->f_shoot_time = gpGlobals->time + g_pWeaponDelayTable[select_index].primary_base_delay +
				RANDOM_FLOAT(g_pWeaponDelayTable[select_index].primary_min_delay[pBot->bot_skill], g_pWeaponDelayTable[select_index].primary_max_delay[pBot->bot_skill]);
		}
	}
	else// MUST be use_secondary...
	{
		pEdict->v.button |= IN_ATTACK2;// secondary attack
		if (g_pWeaponSelectTable[select_index].secondary_usage_flags & WEAPON_UF_CHARGE)
		{
			pBot->charging_weapon_id = iId;
			// release secondary fire after the appropriate delay...
			if (g_pWeaponSelectTable[select_index].secondary_usage_flags & WEAPON_UF_HOLD)// XDM3038c
			{
				pBot->f_secondary_charging = gpGlobals->time + g_pWeaponSelectTable[select_index].secondary_charge_delay;
				pBot->f_shoot_time = gpGlobals->time;// keep charging
			}
		}
		else if (g_pWeaponSelectTable[select_index].secondary_usage_flags & WEAPON_UF_HOLD)
		{
			//conprintf(1, "XBM: BotFireWeapon() id=%d secondary_fire_hold\n", iId);
			pBot->b_secondary_holding = true;
			pBot->f_shoot_time = gpGlobals->time;// don't let button up
		}
		else// set next time to shoot
		{
			pBot->f_shoot_time = gpGlobals->time + g_pWeaponDelayTable[select_index].secondary_base_delay +
				RANDOM_FLOAT(g_pWeaponDelayTable[select_index].secondary_min_delay[pBot->bot_skill], g_pWeaponDelayTable[select_index].secondary_max_delay[pBot->bot_skill]);
		}
	}
	DBG_BOT_PRINT("BotFireWeapon(%s): used %s mode.\n", pBot->name, use_primary?"primary":"secondary");
	return true;  // weapon was fired
}

//-----------------------------------------------------------------------------
// Purpose: Aim and call BotFireWeapon()
// Input  : *pBot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BotShootAtEnemy(bot_t *pBot)
{
	if (pBot->pBotEnemy == NULL)
		return false;
	if (pBot->current_weapon.iId == WEAPON_NONE)
	{
/*#if defined (_DEBUG)
		conprintf(2, "XBM: BotShootAtEnemy() bot %s has no weapon selected!\n", pBot->name);
#endif*/
		return false;
	}
	DBG_BOT_PRINT("BotShootAtEnemy(%s)\n", pBot->name);

	vec_t f_distance;
	Vector v_enemy;
	edict_t *pEdict = pBot->pEdict;
	// do we need to aim at the feet?
	//if (pBot->current_weapon.iId == WEAPON_RPG)
	if (g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].primary_usage_flags & WEAPON_UF_TOSS)// XDM3038: TODO UNDONE FAIL! Structural flaw: we have to choose fire mode beforehand!
	{
		TraceResult tr;
		Vector v_src(pEdict->v.origin);
		v_src += pEdict->v.view_ofs;  // bot's eyes
		Vector v_dest(pBot->pBotEnemy->v.origin);// XDM3038: forget it. Also, good for monsters! - pBot->pBotEnemy->v.view_ofs;
		SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
		UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
		ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);

		// can the bot see the enemies feet?
		if ((tr.flFraction >= 1.0f) || ((tr.flFraction >= 0.95f) && (tr.pHit->v.flags & (FL_CLIENT|FL_FAKECLIENT))))// XDM3035c (strcmp("player", STRING(tr.pHit->v.classname)) == 0)))
			v_enemy = (pBot->pBotEnemy->v.origin - pBot->pBotEnemy->v.view_ofs) - GetGunPosition(pEdict);// aim at the feet for RPG type weapons
		else
			v_enemy = GetGunPosition(pBot->pBotEnemy) - GetGunPosition(pEdict);

		v_enemy.z += (g_psv_gravity->value/DEFAULT_GRAVITY)*96.0f;//v_enemy.Length()????
	}
	else// aim for the head...
		v_enemy = GetGunPosition(pBot->pBotEnemy) - GetGunPosition(pEdict);

	Vector enemy_angle;
	VectorAngles(v_enemy, enemy_angle);// XDM3037: faster

	if (enemy_angle.x > 180.0f)
		enemy_angle.x -=360.0f;

	if (enemy_angle.y > 180.0f)
		enemy_angle.y -=360.0f;

//#if !defined (NOSQB)
//	enemy_angle.x = -enemy_angle.x;// adjust the view angle pitch to aim correctly
//#endif

	float d_x = (enemy_angle.x - pEdict->v.v_angle.x);
	if (d_x > 180.0f)
		d_x = 360.0f - d_x;// this code is suspicious!
	else if (d_x < -180.0f)
		d_x = 360.0f + d_x;

	float d_y = (enemy_angle.y - pEdict->v.v_angle.y);
	if (d_y > 180.0f)
		d_y = 360.0f - d_y;
	else if (d_y < -180.0f)
		d_y = 360.0f + d_y;

	vec_t delta_dist_x = fabs(d_x / pBot->f_frame_time);
	vec_t delta_dist_y = fabs(d_y / pBot->f_frame_time);

	if ((delta_dist_x > 100.0f) && (RANDOM_LONG(1, 100) < 40))
		pBot->f_aim_x_angle_delta += aim_tracking_x_scale[pBot->bot_skill] * pBot->f_frame_time * 0.8f;
	else
		pBot->f_aim_x_angle_delta -= aim_tracking_x_scale[pBot->bot_skill] * pBot->f_frame_time;

	if (RANDOM_LONG(1, 100) < ((pBot->bot_skill+1) * 10))
		pBot->f_aim_x_angle_delta += aim_tracking_x_scale[pBot->bot_skill] * pBot->f_frame_time * 0.5f;

	if ((delta_dist_y > 100.0) && (RANDOM_LONG(1, 100) < 40))
		pBot->f_aim_y_angle_delta += aim_tracking_y_scale[pBot->bot_skill] * pBot->f_frame_time * 0.8f;
	else
		pBot->f_aim_y_angle_delta -= aim_tracking_y_scale[pBot->bot_skill] * pBot->f_frame_time;

	if (RANDOM_LONG(1, 100) < ((pBot->bot_skill+1) * 10))
		pBot->f_aim_y_angle_delta += aim_tracking_y_scale[pBot->bot_skill] * pBot->f_frame_time * 0.5f;

	if (pBot->f_aim_x_angle_delta > 5.0f)
		pBot->f_aim_x_angle_delta = 5.0f;
	else if (pBot->f_aim_x_angle_delta < 0.01f)
		pBot->f_aim_x_angle_delta = 0.01f;

	if (pBot->f_aim_y_angle_delta > 5.0f)
		pBot->f_aim_y_angle_delta = 5.0f;
	else if (pBot->f_aim_y_angle_delta < 0.01f)
		pBot->f_aim_y_angle_delta = 0.01f;

	if (d_x < 0.0)
		d_x = d_x - pBot->f_aim_x_angle_delta;
	else
		d_x = d_x + pBot->f_aim_x_angle_delta;

	if (d_y < 0.0)
		d_y = d_y - pBot->f_aim_y_angle_delta;
	else
		d_y = d_y + pBot->f_aim_y_angle_delta;

	pEdict->v.idealpitch = pEdict->v.v_angle.x + d_x;
	BotFixIdealPitch(pEdict);

	pEdict->v.ideal_yaw = pEdict->v.v_angle.y + d_y;
	BotFixIdealYaw(pEdict);

	v_enemy.z = 0;// ignore z component (up & down)
	f_distance = v_enemy.Length();// how far away is the enemy scum?

/*#if defined (_DEBUG)
	if (pEdict->v.button & IN_ATTACK)
		conprintf(2, "XBM: BotShootAtEnemy(): IN_ATTACK\n");
	if (pEdict->v.button & IN_ATTACK2)
		conprintf(2, "XBM: BotShootAtEnemy(): IN_ATTACK2\n");
#endif*/

	// is it time to shoot yet?
	if (pBot->f_shoot_time <= gpGlobals->time)
	{
		vec_t maxd = max(g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].primary_max_distance,
						g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].secondary_max_distance);

		vec_t mind = min(g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].primary_min_distance,
						g_pWeaponSelectTable[pBot->current_weapon.weapon_select_table_index].secondary_min_distance);

		//does not work!	if (pBot->use_type != BOT_USE_HOLD)// XDM: don't run away from our spot
		{
			if (f_distance > maxd)// run if distance to enemy is far
			{
				pBot->f_move_speed = pBot->f_max_speed;
			}
			else// can fire
			{
				if (f_distance > (mind + HULL_RADIUS))// walk if distance is closer
					pBot->f_move_speed = pBot->f_max_speed * 0.5f;
				else
					pBot->f_move_speed = pBot->f_max_speed * 0.1f;// don't move if very close

				// select the best weapon to use at this distance and fire...
				if (!BotFireWeapon(pBot, f_distance))//, 0))
				{
					//not here!	pBot->pBotEnemy = NULL;// Oh, shoot! I failed to shoot! Forget about the enemy, TODO: run away
					//pBot->f_bot_find_enemy_time = gpGlobals->time + 3.0f;
					//pBot->f_move_speed = pBot->f_max_speed;
					DBG_BOT_PRINT("BotShootAtEnemy(%s) BotFireWeapon() failed.\n", pBot->name);
					return false;// XDM3037: TESTME
				}
			}
		}
	}
	else
		pBot->f_move_speed = pBot->f_max_speed;

	DBG_BOT_PRINT("BotShootAtEnemy(%s) succeeded.\n", pBot->name);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Hard way to 'disarm' a trip mine
// Input  : *pBot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BotShootTripmine(bot_t *pBot)
{
	if (!pBot->b_shoot_tripmine)
		return false;

	DBG_BOT_PRINT("BotShootTripmine(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	// aim at the tripmine and fire the glock...
	Vector v_enemy(pBot->v_tripmine); v_enemy -= GetGunPosition(pEdict);
	VectorAngles(v_enemy, pEdict->v.v_angle);
	NormalizeAngles(pEdict->v.v_angle);
	// Paulo-La-Frite - START bot aiming bug fix
	// set the body angles to point the gun correctly
#if defined (SV_NO_PITCH_CORRECTION)
	pEdict->v.angles.x = pEdict->v.v_angle.x / PITCH_CORRECTION_MULTIPLIER;// see CL_ProcessEntityUpdate() // SQB!!!
#endif
	pEdict->v.angles.y = pEdict->v.v_angle.y;
	pEdict->v.angles.z = 0.0f;
	// adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
//#if !defined (NOSQB)
	pEdict->v.v_angle.x = -pEdict->v.v_angle.x;
//#endif
	// Paulo-La-Frite - END
	pEdict->v.ideal_yaw = pEdict->v.v_angle.y;
	BotFixIdealYaw(pEdict);
	return BotFireWeapon(pBot, v_enemy.Length());//, VALVE_WEAPON_GLOCK));// TODO: pick some long-range weapon
}




// XDM3037: really, it probably DOES work, but nobody uses it anyway...

#if 0
//-----------------------------------------------------------------------------
// Purpose: Precaches the ammo and queues the ammo info for sending to clients
// Input  : *szAmmoname - 
// Output : int index or -1
//-----------------------------------------------------------------------------
int AddAmmoToRegistry(const char *szAmmoName, int iMaxCarry)
{
	if (szAmmoName == NULL)
		return -1;

	// make sure it's not already in the registry
	int i = GetAmmoIndexFromRegistry(szAmmoName);
	if (i != -1)
	{
		if (iMaxCarry > g_AmmoInfoArray[i].iMaxCarry)// somebody else says player should carry more ammo of this type
		{
			conprintf(1, "Warning: AddAmmoToRegistry(%s) updating iMaxCarry from %d to %d!\n", szAmmoName, g_AmmoInfoArray[i].iMaxCarry, iMaxCarry);
			g_AmmoInfoArray[i].iMaxCarry = iMaxCarry;
		}
		return i;// found
	}

	// XDM?	giAmmoIndex++;
	ASSERT(giAmmoIndex < MAX_AMMO_SLOTS);

	if (giAmmoIndex < MAX_AMMO_SLOTS)
	{
		strncpy(g_AmmoInfoArray[giAmmoIndex].name, szAmmoName, MAX_AMMO_NAME_LEN);
		g_AmmoInfoArray[giAmmoIndex].iMaxCarry = iMaxCarry;
		//g_AmmoInfoArray[giAmmoIndex].pszName = szAmmoname;
		//g_AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
		++giAmmoIndex;
		return giAmmoIndex-1;
	}
	//giAmmoIndex = 0;
	return -1;// 0?
}

//-----------------------------------------------------------------------------
// Purpose: GetAmmoIndex
// Input  : *psz - 
// Output : int
//-----------------------------------------------------------------------------
int GetAmmoIndexFromRegistry(const char *szAmmoName)
{
	if (szAmmoName == NULL)
		return -1;

	int i;
	//for (i = 0; i < MAX_AMMO_SLOTS; ++i)// XDM: start from 0
	for (i = 0; i < giAmmoIndex; ++i)// XDM3037: array should never be fragmented
	{
		if (g_AmmoInfoArray[i].name[0] == 0)// 1st char is empty
			continue;

		if (_stricmp(szAmmoName, g_AmmoInfoArray[i].name) == 0)// case-insensitive for stupid mappers
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: maximum amount of that type of ammunition that a player can carry.
// Input  : ammoID - 
// Output : int
//-----------------------------------------------------------------------------
int MaxAmmoCarry(int ammoID)
{
	if (ammoID >= 0)
		return g_AmmoInfoArray[ammoID].iMaxCarry;

	return 0;// XDM3035c: TESTME!! was -1
}

//-----------------------------------------------------------------------------
// Purpose: MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a player can carry.
// Input  : *szName - 
// Output : int
//-----------------------------------------------------------------------------
int MaxAmmoCarry(const char *szName)
{
	int ammoID = GetAmmoIndexFromRegistry(szName);
	int c = MaxAmmoCarry(ammoID);
	if (c <= 0)
		conprintf(1, "MaxAmmoCarry() doesn't recognize '%s'!\n", szName);

	return c;
}
#endif
