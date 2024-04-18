//=========================================================
// skill.cpp - code for skill level concerns
// XDM: great resource savings: 1 cvar instead of 3!
// XDM3038c: added "score" to each monster
//=========================================================
#include "extdll.h"
#include "util.h"
#include "skill.h"

static char SK_NULL[] = "0 0 0";

//DLL_GLOBAL short g_iSkillLevel = SKILL_HARD;// >)
DLL_GLOBAL skilldata_t gSkillData;

// WEAPONS
cvar_t	sk_plr_357_bullet				= {"sk_plr_357_bullet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_9mm_bullet				= {"sk_plr_9mm_bullet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_9mmAR_bullet				= {"sk_plr_9mmAR_bullet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_9mmAR_grenade			= {"sk_plr_9mmAR_grenade", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_acid_grenade				= {"sk_plr_acid_grenade", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_beamrifle				= {"sk_plr_beamrifle", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_bhg						= {"sk_plr_bhg", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_bolt						= {"sk_plr_bolt", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_bolt_explode				= {"sk_plr_bolt_explode", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};// XDM3037a
cvar_t	sk_plr_buckshot					= {"sk_plr_buckshot", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_chemgun					= {"sk_plr_chemgun", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_crowbar					= {"sk_plr_crowbar", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_displacer_beam			= {"sk_plr_displacer_beam", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_displacer_blast			= {"sk_plr_displacer_blast", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_egon						= {"sk_plr_egon", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_flame					= {"sk_plr_flame", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_gauss					= {"sk_plr_gauss", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_glauncher				= {"sk_plr_glauncher", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_hand_grenade				= {"sk_plr_hand_grenade", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_hand_grenade_freeze		= {"sk_plr_hand_grenade_freeze", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_hand_grenade_poison		= {"sk_plr_hand_grenade_poison", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_hand_grenade_burn		= {"sk_plr_hand_grenade_burn", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_hand_grenade_radiation	= {"sk_plr_hand_grenade_radiation", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_hornet					= {"sk_plr_hornet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_nuclear					= {"sk_plr_nuclear", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_plasma					= {"sk_plr_plasma", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_plasmaball				= {"sk_plr_plasmaball", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_razordisk				= {"sk_plr_razordisk", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_sniperrifle				= {"sk_plr_sniperrifle", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_rpg						= {"sk_plr_rpg", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_satchel					= {"sk_plr_satchel", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_strtarget				= {"sk_plr_strtarget", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_sword					= {"sk_plr_sword", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_tripmine					= {"sk_plr_tripmine", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_plr_flashlight_charge		= {"sk_plr_flashlight_charge", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};// XDM3038c
cvar_t	sk_plr_flashlight_drain			= {"sk_plr_flashlight_drain", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};// XDM3038c
cvar_t	sk_plr_air_time					= {"sk_plr_air_time", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};// XDM3038c

// WORLD WEAPONS
cvar_t	sk_12mm_bullet				= {"sk_12mm_bullet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_9mm_bullet				= {"sk_9mm_bullet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_9mmAR_bullet				= {"sk_9mmAR_bullet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_mon_hornet				= {"sk_mon_hornet", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_dmg_mortar				= {"sk_dmg_mortar", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};

// Player damage adjusters
cvar_t	sk_player_head				= {"sk_player_head",	"3 3 3", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_player_chest				= {"sk_player_chest",	"2 2 2", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_player_stomach			= {"sk_player_stomach",	"2 2 2", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_player_arm				= {"sk_player_arm",		"1 1 1", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_player_leg				= {"sk_player_leg",		"1 1 1", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_player_score				= {"sk_player_score",	"10 20 30", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};// XDM3038c: for CoOp

// Monster damage adjusters
cvar_t	sk_monster_head				= {"sk_monster_head",	"3.5 3 3", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_monster_chest			= {"sk_monster_chest",	"2.3 2.2 2.2", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_monster_stomach			= {"sk_monster_stomach","2 2 2", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_monster_arm				= {"sk_monster_arm",	"1 1 1", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_monster_leg				= {"sk_monster_leg",	"1 1 1", FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};

// HEALTH/CHARGE
cvar_t	sk_suitcharger				= {"sk_suitcharger", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_battery					= {"sk_battery", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_healthcharger			= {"sk_healthcharger", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_healthkit				= {"sk_healthkit", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_scientist_heal			= {"sk_scientist_heal", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_food_heal				= {"sk_food_heal", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};


// Agrunt
cvar_t	sk_agrunt_dmg_punch			= {"sk_agrunt_dmg_punch", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_agrunt_health			= {"sk_agrunt_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_agrunt_score				= {"sk_agrunt_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Apache
cvar_t	sk_apache_health			= {"sk_apache_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_apache_score				= {"sk_apache_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Barnacle
cvar_t	sk_barnacle_dmg_bite		= {"sk_barnacle_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_barnacle_health			= {"sk_barnacle_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_barnacle_score			= {"sk_barnacle_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Barney
cvar_t	sk_barney_health			= {"sk_barney_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_barney_score				= {"sk_barney_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Big Momma
cvar_t	sk_bigmomma_health_factor	= {"sk_bigmomma_health_factor", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bigmomma_dmg_slash		= {"sk_bigmomma_dmg_slash", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bigmomma_dmg_blast		= {"sk_bigmomma_dmg_blast", SK_NULL, FCVAR_EXTDLL, 0, NULL};
//cvar_t	sk_bigmomma_radius_blast	= {"sk_bigmomma_radius_blast", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bigmomma_max_children	= {"sk_bigmomma_max_children", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bigmomma_score			= {"sk_bigmomma_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Bullsquid
cvar_t	sk_bullsquid_dmg_bite		= {"sk_bullsquid_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bullsquid_dmg_spit		= {"sk_bullsquid_dmg_spit", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bullsquid_dmg_whip		= {"sk_bullsquid_dmg_whip", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bullsquid_health			= {"sk_bullsquid_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_bullsquid_score			= {"sk_bullsquid_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Controller
cvar_t	sk_controller_dmgball		= {"sk_controller_dmgball", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_controller_dmgzap		= {"sk_controller_dmgzap", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_controller_health		= {"sk_controller_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_controller_score			= {"sk_controller_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Gargantua
cvar_t	sk_gargantua_dmg_fire		= {"sk_gargantua_dmg_fire", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gargantua_dmg_fireball	= {"sk_gargantua_dmg_fireball", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gargantua_dmg_slash		= {"sk_gargantua_dmg_slash", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gargantua_dmg_stomp		= {"sk_gargantua_dmg_stomp", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gargantua_health			= {"sk_gargantua_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gargantua_score			= {"sk_gargantua_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Gonome
cvar_t	sk_gonome_dmg_bite			= {"sk_gonome_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gonome_dmg_jump			= {"sk_gonome_dmg_jump", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gonome_dmg_spit			= {"sk_gonome_dmg_spit", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gonome_health			= {"sk_gonome_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_gonome_score				= {"sk_gonome_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Hassassin
cvar_t	sk_hassassin_health			= {"sk_hassassin_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hassassin_score			= {"sk_hassassin_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_massassin_health			= {"sk_massassin_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_massassin_score			= {"sk_massassin_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Headcrab
cvar_t	sk_headcrab_dmg_bite		= {"sk_headcrab_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_headcrab_health			= {"sk_headcrab_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_headcrab_score			= {"sk_headcrab_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Hgrunt
cvar_t	sk_hgrunt_health			= {"sk_hgrunt_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hgrunt_kick				= {"sk_hgrunt_kick", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hgrunt_gspeed			= {"sk_hgrunt_gspeed", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hgrunt_score				= {"sk_hgrunt_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Heavy grunt
/*cvar_t	sk_hwgrunt_dmg_bullet		= {"sk_hwgrunt_dmg_bullet", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hwgrunt_dmg_grenade		= {"sk_hwgrunt_dmg_grenade", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hwgrunt_health			= {"sk_hwgrunt_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_hwgrunt_score			= {"sk_hwgrunt_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};*/
// Robo Grunt
cvar_t	sk_rgrunt_health			= {"sk_rgrunt_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_rgrunt_score				= {"sk_rgrunt_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Houndeye
cvar_t	sk_houndeye_dmg_blast		= {"sk_houndeye_dmg_blast", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_houndeye_health			= {"sk_houndeye_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_houndeye_score			= {"sk_houndeye_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// ISlave
cvar_t	sk_islave_dmg_claw			= {"sk_islave_dmg_claw", SK_NULL, FCVAR_EXTDLL, 0, NULL};
//cvar_t	sk_islave_dmg_clawrake		= {"sk_islave_dmg_clawrake", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_islave_dmg_zap			= {"sk_islave_dmg_zap", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_islave_health			= {"sk_islave_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_islave_score				= {"sk_islave_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Icthyosaur
cvar_t	sk_ichthyosaur_health		= {"sk_ichthyosaur_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_ichthyosaur_shake		= {"sk_ichthyosaur_shake", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_ichthyosaur_score		= {"sk_ichthyosaur_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Kingpin
/*cvar_t	sk_kingpin_dmg_bite			= {"sk_kingpin_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_kingpin_dmg_bolt			= {"sk_kingpin_dmg_bolt", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_kingpin_health			= {"sk_kingpin_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_kingpin_score			= {"sk_kingpin_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};*/
// Leech
cvar_t	sk_leech_dmg_bite			= {"sk_leech_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_leech_health				= {"sk_leech_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_leech_score				= {"sk_leech_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Osprey
cvar_t	sk_osprey_health			= {"sk_osprey_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_osprey_score				= {"sk_osprey_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Pit drone
cvar_t	sk_pitdrone_dmg_bite		= {"sk_pitdrone_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitdrone_dmg_spit		= {"sk_pitdrone_dmg_spit", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitdrone_health			= {"sk_pitdrone_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitdrone_score			= {"sk_pitdrone_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Pit worm
/*cvar_t	sk_pitworm_dmg_slash		= {"sk_pitworm_dmg_slash", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitworm_dmg_scream		= {"sk_pitworm_dmg_scream", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitworm_dmg_spit			= {"sk_pitworm_dmg_spit", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitworm_dmg_beam			= {"sk_pitworm_dmg_beam", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitworm_dmg_teleport		= {"sk_pitworm_dmg_teleport", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitworm_health			= {"sk_pitworm_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_pitworm_score			= {"sk_pitworm_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};*/
// Nihilanth
cvar_t	sk_nihilanth_health			= {"sk_nihilanth_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_nihilanth_score			= {"sk_nihilanth_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_nihilanth_zap			= {"sk_nihilanth_zap", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Panther
cvar_t	sk_panther_dmg_slash		= {"sk_panther_dmg_slash", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_panther_dmg_jump			= {"sk_panther_dmg_jump", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_panther_health			= {"sk_panther_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_panther_score			= {"sk_panther_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Rat
cvar_t	sk_rat_dmg_bite				= {"sk_rat_dmg_bite", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_rat_health				= {"sk_rat_health", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_rat_score				= {"sk_rat_score", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
// Scientist
cvar_t	sk_scientist_health			= {"sk_scientist_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_scientist_score			= {"sk_scientist_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Shock trooper
/*cvar_t	sk_shocktrooper_health		= {"sk_shocktrooper_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_shocktrooper_kick		= {"sk_shocktrooper_kick", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_shocktrooper_shock		= {"sk_shocktrooper_shock", SK_NULL, FCVAR_EXTDLL, 0, NULL};
//cvar_t	sk_shocktrooper_spore	= {"sk_shocktrooper_spore", SK_NULL, FCVAR_EXTDLL, 0, NULL};
//cvar_t	sk_shocktrooper_hornet	= {"sk_shocktrooper_hornet", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_shocktrooper_grenade		= {"sk_shocktrooper_grenade", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_shocktrooper_score		= {"sk_shocktrooper_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};*/
// Snark
cvar_t	sk_snark_health				= {"sk_snark_health", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_snark_dmg_bite			= {"sk_snark_dmg_bite", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_snark_dmg_pop			= {"sk_snark_dmg_pop", SK_NULL, FCVAR_SPONLY|FCVAR_EXTDLL, 0, NULL};
// Tentacle
cvar_t	sk_tentacle_dmg				= {"sk_tentacle_dmg", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_tentacle_health			= {"sk_tentacle_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_tentacle_hide_time		= {"sk_tentacle_hide_time", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_tentacle_score			= {"sk_tentacle_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Turret
cvar_t	sk_turret_health			= {"sk_turret_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_turret_score				= {"sk_turret_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// MiniTurret
cvar_t	sk_miniturret_health		= {"sk_miniturret_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_miniturret_score			= {"sk_miniturret_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Sentry Turret
cvar_t	sk_sentry_health			= {"sk_sentry_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_sentry_score				= {"sk_sentry_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Voltigore
cvar_t	sk_voltigore_dmg_bite		= {"sk_voltigore_dmg_bite", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_voltigore_dmg_bolt		= {"sk_voltigore_dmg_bolt", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_voltigore_health			= {"sk_voltigore_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_voltigore_score			= {"sk_voltigore_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Zombie
cvar_t	sk_zombie_dmg_slash			= {"sk_zombie_dmg_slash", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zombie_health			= {"sk_zombie_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zombie_score				= {"sk_zombie_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};
// Zombie robot
/*cvar_t	sk_zrobot_dmg_slash			= {"sk_zrobot_dmg_slash", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zrobot_dmg_fire			= {"sk_zrobot_dmg_fire", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zrobot_dmg_laser			= {"sk_zrobot_dmg_laser", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zrobot_dmg_jump			= {"sk_zrobot_dmg_jump", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zrobot_health			= {"sk_zrobot_health", SK_NULL, FCVAR_EXTDLL, 0, NULL};
cvar_t	sk_zrobot_score				= {"sk_zrobot_score", SK_NULL, FCVAR_EXTDLL, 0, NULL};*/


//-----------------------------------------------------------------------------
// Purpose: Register all skill vars in the engine
//-----------------------------------------------------------------------------
void SkillRegisterCvars(void)
{
	// PLAYER WEAPONS
	// Crowbar whack
	CVAR_REGISTER(&sk_plr_357_bullet);
	CVAR_REGISTER(&sk_plr_9mm_bullet);
	CVAR_REGISTER(&sk_plr_9mmAR_bullet);
	CVAR_REGISTER(&sk_plr_9mmAR_grenade);
	CVAR_REGISTER(&sk_plr_acid_grenade);
	CVAR_REGISTER(&sk_plr_beamrifle);
	CVAR_REGISTER(&sk_plr_bhg);
	CVAR_REGISTER(&sk_plr_bolt);
	CVAR_REGISTER(&sk_plr_bolt_explode);
	CVAR_REGISTER(&sk_plr_buckshot);
	CVAR_REGISTER(&sk_plr_chemgun);
	CVAR_REGISTER(&sk_plr_crowbar);
	CVAR_REGISTER(&sk_plr_displacer_beam);
	CVAR_REGISTER(&sk_plr_displacer_blast);
	CVAR_REGISTER(&sk_plr_egon);
	CVAR_REGISTER(&sk_plr_flame);
	CVAR_REGISTER(&sk_plr_gauss);
	CVAR_REGISTER(&sk_plr_glauncher);
	CVAR_REGISTER(&sk_plr_hand_grenade);
	CVAR_REGISTER(&sk_plr_hand_grenade_freeze);
	CVAR_REGISTER(&sk_plr_hand_grenade_poison);
	CVAR_REGISTER(&sk_plr_hand_grenade_burn);
	CVAR_REGISTER(&sk_plr_hand_grenade_radiation);
	//CVAR_REGISTER(&sk_plr_hand_grenade_gravity);
	CVAR_REGISTER(&sk_plr_hornet);
	CVAR_REGISTER(&sk_plr_nuclear);
	CVAR_REGISTER(&sk_plr_plasma);
	CVAR_REGISTER(&sk_plr_plasmaball);
	CVAR_REGISTER(&sk_plr_razordisk);
	CVAR_REGISTER(&sk_plr_sniperrifle);
	CVAR_REGISTER(&sk_plr_rpg);
	CVAR_REGISTER(&sk_plr_satchel);
	CVAR_REGISTER(&sk_plr_strtarget);
	CVAR_REGISTER(&sk_plr_sword);
	CVAR_REGISTER(&sk_plr_tripmine);
	CVAR_REGISTER(&sk_plr_flashlight_charge);
	CVAR_REGISTER(&sk_plr_flashlight_drain);
	CVAR_REGISTER(&sk_plr_air_time);

	// WORLD WEAPONS
	CVAR_REGISTER(&sk_12mm_bullet);
	CVAR_REGISTER(&sk_9mmAR_bullet);
	CVAR_REGISTER(&sk_9mm_bullet);
	CVAR_REGISTER(&sk_mon_hornet);
	CVAR_REGISTER(&sk_dmg_mortar);

	// Monster damage adjusters
	CVAR_REGISTER(&sk_monster_head);
	CVAR_REGISTER(&sk_monster_chest);
	CVAR_REGISTER(&sk_monster_stomach);
	CVAR_REGISTER(&sk_monster_arm);
	CVAR_REGISTER(&sk_monster_leg);

	// Player damage adjusters
	CVAR_REGISTER(&sk_player_head);
	CVAR_REGISTER(&sk_player_chest);
	CVAR_REGISTER(&sk_player_stomach);
	CVAR_REGISTER(&sk_player_arm);
	CVAR_REGISTER(&sk_player_leg);
	CVAR_REGISTER(&sk_player_score);

	// HEALTH/SUIT CHARGE DISTRIBUTION
	CVAR_REGISTER(&sk_suitcharger);
	CVAR_REGISTER(&sk_battery);
	CVAR_REGISTER(&sk_healthcharger);
	CVAR_REGISTER(&sk_healthkit);
	CVAR_REGISTER(&sk_scientist_heal);
	CVAR_REGISTER(&sk_food_heal);// XDM3037

	// MONSTERS
	// Agrunt
	CVAR_REGISTER(&sk_agrunt_dmg_punch);
	CVAR_REGISTER(&sk_agrunt_health);
	CVAR_REGISTER(&sk_agrunt_score);
	// Apache
	CVAR_REGISTER(&sk_apache_health);
	CVAR_REGISTER(&sk_apache_score);
	// Barnacle
	CVAR_REGISTER(&sk_barnacle_dmg_bite);
	CVAR_REGISTER(&sk_barnacle_health);
	CVAR_REGISTER(&sk_barnacle_score);
	// Barney
	CVAR_REGISTER(&sk_barney_health);
	CVAR_REGISTER(&sk_barney_score);
	// Big momma
	CVAR_REGISTER(&sk_bigmomma_dmg_slash);
	CVAR_REGISTER(&sk_bigmomma_dmg_blast);
	CVAR_REGISTER(&sk_bigmomma_health_factor);
	//CVAR_REGISTER(&sk_bigmomma_radius_blast);
	CVAR_REGISTER(&sk_bigmomma_max_children);
	CVAR_REGISTER(&sk_bigmomma_score);
	// Bullsquid
	CVAR_REGISTER(&sk_bullsquid_dmg_bite);
	CVAR_REGISTER(&sk_bullsquid_dmg_whip);
	CVAR_REGISTER(&sk_bullsquid_dmg_spit);
	CVAR_REGISTER(&sk_bullsquid_health);
	CVAR_REGISTER(&sk_bullsquid_score);
	// Controller
	CVAR_REGISTER(&sk_controller_dmgzap);
	CVAR_REGISTER(&sk_controller_dmgball);
	CVAR_REGISTER(&sk_controller_health);
	CVAR_REGISTER(&sk_controller_score);
	// Gargantua
	CVAR_REGISTER(&sk_gargantua_dmg_fire);
	CVAR_REGISTER(&sk_gargantua_dmg_fireball);
	CVAR_REGISTER(&sk_gargantua_dmg_slash);
	CVAR_REGISTER(&sk_gargantua_dmg_stomp);
	CVAR_REGISTER(&sk_gargantua_health);
	CVAR_REGISTER(&sk_gargantua_score);
	// Gonome
	CVAR_REGISTER(&sk_gonome_dmg_bite);
	CVAR_REGISTER(&sk_gonome_dmg_jump);
	CVAR_REGISTER(&sk_gonome_dmg_spit);
	CVAR_REGISTER(&sk_gonome_health);
	CVAR_REGISTER(&sk_gonome_score);
	// Hassassin
	CVAR_REGISTER(&sk_hassassin_health);
	CVAR_REGISTER(&sk_hassassin_score);
	CVAR_REGISTER(&sk_massassin_health);
	CVAR_REGISTER(&sk_massassin_score);
	// Headcrab
	CVAR_REGISTER(&sk_headcrab_dmg_bite);
	CVAR_REGISTER(&sk_headcrab_health);
	CVAR_REGISTER(&sk_headcrab_score);
	// Hgrunt
	CVAR_REGISTER(&sk_hgrunt_gspeed);
	CVAR_REGISTER(&sk_hgrunt_health);
	CVAR_REGISTER(&sk_hgrunt_score);
	CVAR_REGISTER(&sk_hgrunt_kick);
	// Heavy weapons grunt
	/*CVAR_REGISTER(&sk_hwgrunt_dmg_bullet);
	CVAR_REGISTER(&sk_hwgrunt_dmg_grenade);
	CVAR_REGISTER(&sk_hwgrunt_health);
	CVAR_REGISTER(&sk_hwgrunt_score);*/
	// Robo grunt
	CVAR_REGISTER(&sk_rgrunt_health);
	CVAR_REGISTER(&sk_rgrunt_score);
	// Houndeye
	CVAR_REGISTER(&sk_houndeye_dmg_blast);
	CVAR_REGISTER(&sk_houndeye_health);
	CVAR_REGISTER(&sk_houndeye_score);
	// ISlave
	CVAR_REGISTER(&sk_islave_dmg_claw);
	//CVAR_REGISTER(&sk_islave_dmg_clawrake);
	CVAR_REGISTER(&sk_islave_dmg_zap);
	CVAR_REGISTER(&sk_islave_health);
	CVAR_REGISTER(&sk_islave_score);
	// Icthyosaur
	CVAR_REGISTER(&sk_ichthyosaur_health);
	CVAR_REGISTER(&sk_ichthyosaur_score);
	CVAR_REGISTER(&sk_ichthyosaur_shake);
	// Kingpin
	/*CVAR_REGISTER(&sk_kingpin_dmg_bite);
	CVAR_REGISTER(&sk_kingpin_dmg_bolt);
	CVAR_REGISTER(&sk_kingpin_health);
	CVAR_REGISTER(&sk_kingpin_score);*/
	// Leech
	CVAR_REGISTER(&sk_leech_dmg_bite);
	CVAR_REGISTER(&sk_leech_health);
	CVAR_REGISTER(&sk_leech_score);
	// Osprey
	CVAR_REGISTER(&sk_osprey_health);
	CVAR_REGISTER(&sk_osprey_score);
	// Pit drone
	CVAR_REGISTER(&sk_pitdrone_dmg_bite);
	CVAR_REGISTER(&sk_pitdrone_dmg_spit);
	CVAR_REGISTER(&sk_pitdrone_health);
	CVAR_REGISTER(&sk_pitdrone_score);
	// Pit worm
	/*CVAR_REGISTER(&sk_pitworm_dmg_beam);
	CVAR_REGISTER(&sk_pitworm_dmg_slash);
	CVAR_REGISTER(&sk_pitworm_dmg_scream);
	CVAR_REGISTER(&sk_pitworm_dmg_spit);
	CVAR_REGISTER(&sk_pitworm_dmg_teleport);
	CVAR_REGISTER(&sk_pitworm_health);
	CVAR_REGISTER(&sk_pitworm_score);*/
	// Nihilanth
	CVAR_REGISTER(&sk_nihilanth_health);
	CVAR_REGISTER(&sk_nihilanth_score);
	CVAR_REGISTER(&sk_nihilanth_zap);
	// Panther
	CVAR_REGISTER(&sk_panther_dmg_slash);
	CVAR_REGISTER(&sk_panther_dmg_jump);
	CVAR_REGISTER(&sk_panther_health);
	CVAR_REGISTER(&sk_panther_score);
	// Rat
	CVAR_REGISTER(&sk_rat_dmg_bite);
	CVAR_REGISTER(&sk_rat_health);
	CVAR_REGISTER(&sk_rat_score);
	// Scientist
	CVAR_REGISTER(&sk_scientist_health);
	CVAR_REGISTER(&sk_scientist_score);
	// Shock trooper
	/*CVAR_REGISTER(&sk_shocktrooper_health);
	CVAR_REGISTER(&sk_shocktrooper_score);
	CVAR_REGISTER(&sk_shocktrooper_kick);
	CVAR_REGISTER(&sk_shocktrooper_shock);
	//CVAR_REGISTER(&sk_shocktrooper_spore);
	//CVAR_REGISTER(&sk_shocktrooper_hornet);
	CVAR_REGISTER(&sk_shocktrooper_grenade);*/
	// Snark
	CVAR_REGISTER(&sk_snark_health);
	CVAR_REGISTER(&sk_snark_dmg_bite);
	CVAR_REGISTER(&sk_snark_dmg_pop);
	// Tentacle
	CVAR_REGISTER(&sk_tentacle_dmg);
	CVAR_REGISTER(&sk_tentacle_health);
	CVAR_REGISTER(&sk_tentacle_hide_time);
	CVAR_REGISTER(&sk_tentacle_score);
	// Turret
	CVAR_REGISTER(&sk_turret_health);
	CVAR_REGISTER(&sk_turret_score);
	// MiniTurret
	CVAR_REGISTER(&sk_miniturret_health);
	CVAR_REGISTER(&sk_miniturret_score);
	// Sentry Turret
	CVAR_REGISTER(&sk_sentry_health);
	CVAR_REGISTER(&sk_sentry_score);
	// Voltigore
	CVAR_REGISTER(&sk_voltigore_dmg_bite);
	CVAR_REGISTER(&sk_voltigore_dmg_bolt);
	CVAR_REGISTER(&sk_voltigore_health);
	CVAR_REGISTER(&sk_voltigore_score);
	// Zombie
	CVAR_REGISTER(&sk_zombie_dmg_slash);
	CVAR_REGISTER(&sk_zombie_health);
	CVAR_REGISTER(&sk_zombie_score);
	// Zombie robot
	/*CVAR_REGISTER(&sk_zrobot_dmg_slash);
	CVAR_REGISTER(&sk_zrobot_dmg_fire);
	CVAR_REGISTER(&sk_zrobot_dmg_laser);
	CVAR_REGISTER(&sk_zrobot_dmg_jump);
	CVAR_REGISTER(&sk_zrobot_health);
	CVAR_REGISTER(&sk_zrobot_score);*/
}

//-----------------------------------------------------------------------------
// Purpose: Get value based on the skill level
// Input  : *pCVar - 
//			fValueMin - failsafe min/max values in case something goes wrong
//			fValueMax - 
// Output : float value
//-----------------------------------------------------------------------------
float GetSkillCvar(const cvar_t *pCVar, const float &fValueMin, const float &fValueMax)
{
	if (pCVar == NULL)
	{
		conprintf(0, "GetSkillCvar() ERROR: got NULL cvar pointer!\n");
		return fValueMin;
	}
	float v1 = fValueMin;
	float v2 = fValueMin;
	float v3 = fValueMin;

	if (sscanf(pCVar->string, "%f %f %f", &v1, &v2, &v3) != 3)
		conprintf(0, "GetSkillCVar(%s): Error parsing value!\n", pCVar->name);
	else if (v1 == 0.0f && v2 == 0.0f && v3 == 0.0f)
		conprintf(0, "GetSkillCVar(%s): Warning: Got a zero!\n", pCVar->name);

	if (gSkillData.iSkillLevel == SKILL_EASY)
		return min(v1, fValueMax);
	else if (gSkillData.iSkillLevel == SKILL_MEDIUM)
		return min(v2, fValueMax);
	else
		return min(v3, fValueMax);
}

//-----------------------------------------------------------------------------
// Purpose: Read all cvars and retrieve values based on the skill level
// Input  : iSkill - 
//-----------------------------------------------------------------------------
void SkillUpdateData(int iSkill)
{
	SERVER_COMMAND("exec skill.cfg\n");
	SERVER_EXECUTE();

	if (gSkillData.iSkillLevel != iSkill)
		conprintf(1, "SkillUpdateData(%d): skill level changed from %d.\n", iSkill, gSkillData.iSkillLevel);

	gSkillData.iSkillLevel = iSkill;
	//g_iSkillLevel = iSkill;

	//Agrunt
	gSkillData.agruntDmgPunch = GetSkillCvar(&sk_agrunt_dmg_punch, 1.0f, 65535.0f);
	gSkillData.agruntHealth = GetSkillCvar(&sk_agrunt_health, 1.0f, 65535.0f);
	gSkillData.agruntScore = GetSkillCvar(&sk_agrunt_score, 0.0f, 65535.0f);
	// Apache
	gSkillData.apacheHealth = GetSkillCvar(&sk_apache_health, 1.0f, 65535.0f);
	gSkillData.apacheScore = GetSkillCvar(&sk_apache_score, 0.0f, 65535.0f);
	// Barnacle
	gSkillData.barnacleDmgBite = GetSkillCvar(&sk_barnacle_dmg_bite, 1.0f, 65535.0f);
	gSkillData.barnacleHealth = GetSkillCvar(&sk_barnacle_health, 1.0f, 65535.0f);
	gSkillData.barnacleScore = GetSkillCvar(&sk_barnacle_score, 0.0f, 65535.0f);
	// Barney
	gSkillData.barneyHealth = GetSkillCvar(&sk_barney_health, 1.0f, 65535.0f);
	gSkillData.barneyScore = GetSkillCvar(&sk_barney_score, 0.0f, 65535.0f);
	// Big Momma
	gSkillData.bigmommaDmgSlash = GetSkillCvar(&sk_bigmomma_dmg_slash, 1.0f, 65535.0f);
	gSkillData.bigmommaDmgBlast = GetSkillCvar(&sk_bigmomma_dmg_blast, 1.0f, 65535.0f);
	gSkillData.bigmommaHealthFactor = GetSkillCvar(&sk_bigmomma_health_factor, 1.0f, 65535.0f);
	//gSkillData.bigmommaRadiusBlast = GetSkillCvar(&sk_bigmomma_radius_blast, 1.0f, 65535.0f, 1.0f, 65535.0f);
	gSkillData.bigmommaMaxChildren = GetSkillCvar(&sk_bigmomma_max_children, 1.0f, 65535.0f);
	gSkillData.bigmommaScore = GetSkillCvar(&sk_bigmomma_score, 0.0f, 65535.0f);
	// Bullsquid
	gSkillData.bullsquidDmgBite = GetSkillCvar(&sk_bullsquid_dmg_bite, 1.0f, 65535.0f);
	gSkillData.bullsquidDmgWhip = GetSkillCvar(&sk_bullsquid_dmg_whip, 1.0f, 65535.0f);
	gSkillData.bullsquidDmgSpit = GetSkillCvar(&sk_bullsquid_dmg_spit, 1.0f, 65535.0f);
	gSkillData.bullsquidHealth = GetSkillCvar(&sk_bullsquid_health, 1.0f, 65535.0f);
	gSkillData.bullsquidScore = GetSkillCvar(&sk_bullsquid_score, 0.0f, 65535.0f);
	// Controller
	gSkillData.controllerDmgZap = GetSkillCvar(&sk_controller_dmgzap, 1.0f, 65535.0f);
	gSkillData.controllerDmgBall = GetSkillCvar(&sk_controller_dmgball, 1.0f, 65535.0f);
	gSkillData.controllerHealth = GetSkillCvar(&sk_controller_health, 1.0f, 65535.0f);
	gSkillData.controllerScore = GetSkillCvar(&sk_controller_score, 0.0f, 65535.0f);
	// Gargantua
	gSkillData.gargantuaDmgFire = GetSkillCvar(&sk_gargantua_dmg_fire, 1.0f, 65535.0f);
	gSkillData.gargantuaDmgFireBall = GetSkillCvar(&sk_gargantua_dmg_fireball, 1.0f, 65535.0f);
	gSkillData.gargantuaDmgSlash = GetSkillCvar(&sk_gargantua_dmg_slash, 1.0f, 65535.0f);
	gSkillData.gargantuaDmgStomp = GetSkillCvar(&sk_gargantua_dmg_stomp, 1.0f, 65535.0f);
	gSkillData.gargantuaHealth = GetSkillCvar(&sk_gargantua_health, 1.0f, 65535.0f);
	gSkillData.gargantuaScore = GetSkillCvar(&sk_gargantua_score, 0.0f, 65535.0f);
	// Gonome
	gSkillData.gonomeDmgBite = GetSkillCvar(&sk_gonome_dmg_bite, 1.0f, 65535.0f);
	gSkillData.gonomeDmgSpit = GetSkillCvar(&sk_gonome_dmg_spit, 1.0f, 65535.0f);
	gSkillData.gonomeDmgJump = GetSkillCvar(&sk_gonome_dmg_jump, 1.0f, 65535.0f);
	gSkillData.gonomeHealth = GetSkillCvar(&sk_gonome_health, 1.0f, 65535.0f);
	gSkillData.gonomeScore = GetSkillCvar(&sk_gonome_score, 0.0f, 65535.0f);
	// Hassassin
	gSkillData.hassassinHealth = GetSkillCvar(&sk_hassassin_health, 1.0f, 65535.0f);
	gSkillData.hassassinScore = GetSkillCvar(&sk_hassassin_score, 1.0f, 65535.0f);
	gSkillData.massassinHealth = GetSkillCvar(&sk_massassin_health, 1.0f, 65535.0f);
	gSkillData.massassinScore = GetSkillCvar(&sk_massassin_score, 0.0f, 65535.0f);
	// Headcrab
	gSkillData.headcrabDmgBite = GetSkillCvar(&sk_headcrab_dmg_bite, 1.0f, 65535.0f);
	gSkillData.headcrabHealth = GetSkillCvar(&sk_headcrab_health, 1.0f, 65535.0f);
	gSkillData.headcrabScore = GetSkillCvar(&sk_headcrab_score, 1.0f, 65535.0f);
	// Hgrunt
	gSkillData.hgruntGrenadeSpeed = GetSkillCvar(&sk_hgrunt_gspeed, 1.0f, 65535.0f);
	gSkillData.hgruntDmgKick = GetSkillCvar(&sk_hgrunt_kick, 1.0f, 65535.0f);
	gSkillData.hgruntHealth = GetSkillCvar(&sk_hgrunt_health, 1.0f, 65535.0f);
	gSkillData.hgruntScore = GetSkillCvar(&sk_hgrunt_score, 0.0f, 65535.0f);
	// Houndeye
	gSkillData.houndeyeDmgBlast = GetSkillCvar(&sk_houndeye_dmg_blast, 1.0f, 65535.0f);
	gSkillData.houndeyeHealth = GetSkillCvar(&sk_houndeye_health, 1.0f, 65535.0f);
	gSkillData.houndeyeScore = GetSkillCvar(&sk_houndeye_score, 0.0f, 65535.0f);
	// Heavy weapons grunt
	/*gSkillData.hwgruntDmgBullet = GetSkillCvar(&sk_hwgrunt_dmg_bullet, 1.0f, 65535.0f);
	gSkillData.hwgruntDmgGrenade = GetSkillCvar(&sk_hwgrunt_dmg_grenade, 1.0f, 65535.0f);
	gSkillData.hwgruntHealth = GetSkillCvar(&sk_hwgrunt_health, 1.0f, 65535.0f);
	gSkillData.hwgruntScore = GetSkillCvar(&sk_hwgrunt_score, 0.0f, 65535.0f);*/
	// Icthyosaur
	gSkillData.ichthyosaurHealth = GetSkillCvar(&sk_ichthyosaur_health, 1.0f, 65535.0f);
	gSkillData.ichthyosaurScore = GetSkillCvar(&sk_ichthyosaur_score, 0.0f, 65535.0f);
	gSkillData.ichthyosaurDmgShake = GetSkillCvar(&sk_ichthyosaur_shake, 1.0f, 65535.0f);
	// Kingpin
	/*gSkillData.kingpinDmgBite = GetSkillCvar(&sk_kingpin_dmg_bite, 1.0f, 65535.0f);
	gSkillData.kingpinDmgBolt = GetSkillCvar(&sk_kingpin_dmg_bolt, 1.0f, 65535.0f);
	gSkillData.kingpinHealth = GetSkillCvar(&sk_kingpin_health, 1.0f, 65535.0f);
	gSkillData.kingpinScore = GetSkillCvar(&sk_kingpin_score, 0.0f, 65535.0f);*/
	// Leech
	gSkillData.leechDmgBite = GetSkillCvar(&sk_leech_dmg_bite, 1.0f, 65535.0f);
	gSkillData.leechHealth = GetSkillCvar(&sk_leech_health, 1.0f, 65535.0f);
	gSkillData.leechScore = GetSkillCvar(&sk_leech_score, 0.0f, 65535.0f);
	// Osprey
	gSkillData.ospreyHealth = GetSkillCvar(&sk_osprey_health, 1.0f, 65535.0f);
	gSkillData.ospreyScore = GetSkillCvar(&sk_osprey_score, 0.0f, 65535.0f);
	// Panther
	gSkillData.pantherDmgSlash = GetSkillCvar(&sk_panther_dmg_slash, 1.0f, 65535.0f);
	gSkillData.pantherDmgJump = GetSkillCvar(&sk_panther_dmg_jump, 1.0f, 65535.0f);
	gSkillData.pantherHealth = GetSkillCvar(&sk_panther_health, 1.0f, 65535.0f);
	gSkillData.pantherScore = GetSkillCvar(&sk_panther_score, 0.0f, 65535.0f);
	// Pit drone
	gSkillData.pitdroneDmgBite = GetSkillCvar(&sk_pitdrone_dmg_bite, 1.0f, 65535.0f);
	gSkillData.pitdroneDmgSpit = GetSkillCvar(&sk_pitdrone_dmg_spit, 1.0f, 65535.0f);
	gSkillData.pitdroneHealth = GetSkillCvar(&sk_pitdrone_health, 1.0f, 65535.0f);
	gSkillData.pitdroneScore = GetSkillCvar(&sk_pitdrone_score, 0.0f, 65535.0f);
	// Pit worm
	/*gSkillData.pitwormDmgSlash = GetSkillCvar(&sk_pitworm_dmg_slash, 1.0f, 65535.0f);
	gSkillData.pitwormDmgScream = GetSkillCvar(&sk_pitworm_dmg_scream, 1.0f, 65535.0f);
	gSkillData.pitwormDmgSpit = GetSkillCvar(&sk_pitworm_dmg_spit, 1.0f, 65535.0f);
	gSkillData.pitwormDmgBeam = GetSkillCvar(&sk_pitworm_dmg_beam, 1.0f, 65535.0f);
	gSkillData.pitwormDmgTeleport = GetSkillCvar(&sk_pitworm_dmg_teleport, 1.0f, 65535.0f);
	gSkillData.pitwormHealth = GetSkillCvar(&sk_pitworm_health, 1.0f, 65535.0f);
	gSkillData.pitwormScore = GetSkillCvar(&sk_pitworm_score, 0.0f, 65535.0f);*/
	// Nihilanth
	gSkillData.nihilanthHealth = GetSkillCvar(&sk_nihilanth_health, 1.0f, 65535.0f);
	gSkillData.nihilanthScore = GetSkillCvar(&sk_nihilanth_score, 0.0f, 65535.0f);
	gSkillData.nihilanthZap = GetSkillCvar(&sk_nihilanth_zap, 1.0f, 65535.0f);
	// Rat
	gSkillData.ratHealth = GetSkillCvar(&sk_rat_health, 1.0f, 65535.0f);
	gSkillData.ratScore = GetSkillCvar(&sk_rat_score, 0.0f, 65535.0f);
	gSkillData.ratDmgBite = GetSkillCvar(&sk_rat_dmg_bite, 1.0f, 65535.0f);
	// Robo grunt
	gSkillData.rgruntHealth = GetSkillCvar(&sk_rgrunt_health, 1.0f, 65535.0f);
	gSkillData.rgruntScore = GetSkillCvar(&sk_rgrunt_score, 0.0f, 65535.0f);
	// Scientist
	gSkillData.scientistHealth = GetSkillCvar(&sk_scientist_health, 1.0f, 65535.0f);
	gSkillData.scientistScore = GetSkillCvar(&sk_scientist_score, 0.0f, 65535.0f);
	// Shock trooper
	/*gSkillData.shocktrooperDmgGrenade = GetSkillCvar(&sk_shocktrooper_grenade, 1.0f, 65535.0f);
	gSkillData.shocktrooperDmgKick = GetSkillCvar(&sk_shocktrooper_kick, 1.0f, 65535.0f);
	gSkillData.shocktrooperDmgShock = GetSkillCvar(&sk_shocktrooper_shock, 1.0f, 65535.0f);
	gSkillData.shocktrooperHealth = GetSkillCvar(&sk_shocktrooper_health, 1.0f, 65535.0f);
	gSkillData.shocktrooperScore = GetSkillCvar(&sk_shocktrooper_score, 0.0f, 65535.0f);*/
	// Slave
	gSkillData.slaveDmgClaw = GetSkillCvar(&sk_islave_dmg_claw, 1.0f, 65535.0f);
	//gSkillData.slaveDmgClawrake = GetSkillCvar(&sk_islave_dmg_clawrake, 1.0f, 65535.0f);
	gSkillData.slaveDmgZap = GetSkillCvar(&sk_islave_dmg_zap, 1.0f, 65535.0f);
	gSkillData.slaveHealth = GetSkillCvar(&sk_islave_health, 1.0f, 65535.0f);
	gSkillData.slaveScore = GetSkillCvar(&sk_islave_score, 0.0f, 65535.0f);
	// Snark
	gSkillData.snarkDmgBite = GetSkillCvar(&sk_snark_dmg_bite, 1.0f, 65535.0f);
	gSkillData.snarkDmgPop = GetSkillCvar(&sk_snark_dmg_pop, 1.0f, 65535.0f);
	gSkillData.snarkHealth = GetSkillCvar(&sk_snark_health, 1.0f, 65535.0f);
	// Tentacle
	gSkillData.tentacleDmg = GetSkillCvar(&sk_tentacle_dmg, 1.0f, 65535.0f);
	gSkillData.tentacleHealth = GetSkillCvar(&sk_tentacle_health, 1.0f, 65535.0f);
	gSkillData.tentacleHideTime = GetSkillCvar(&sk_tentacle_hide_time, 1.0f, 65535.0f);
	gSkillData.tentacleScore = GetSkillCvar(&sk_tentacle_score, 0.0f, 65535.0f);
	// Turret
	gSkillData.turretHealth = GetSkillCvar(&sk_turret_health, 1.0f, 65535.0f);
	gSkillData.turretScore = GetSkillCvar(&sk_turret_score, 0.0f, 65535.0f);
	// MiniTurret
	gSkillData.miniturretHealth = GetSkillCvar(&sk_miniturret_health, 1.0f, 65535.0f);
	gSkillData.miniturretScore = GetSkillCvar(&sk_miniturret_score, 0.0f, 65535.0f);
	// Sentry Turret
	gSkillData.sentryHealth = GetSkillCvar(&sk_sentry_health, 1.0f, 65535.0f);
	gSkillData.sentryScore = GetSkillCvar(&sk_sentry_score, 0.0f, 65535.0f);
	// Zombie
	gSkillData.zombieDmgSlash = GetSkillCvar(&sk_zombie_dmg_slash, 1.0f, 65535.0f);
	gSkillData.zombieHealth = GetSkillCvar(&sk_zombie_health, 1.0f, 65535.0f);
	gSkillData.zombieScore = GetSkillCvar(&sk_zombie_score, 0.0f, 65535.0f);
	// Zombie robot
	/*gSkillData.zrobotDmgFire = GetSkillCvar(&sk_zrobot_dmg_fire, 1.0f, 65535.0f);
	gSkillData.zrobotDmgJump = GetSkillCvar(&sk_zrobot_dmg_jump, 1.0f, 65535.0f);
	gSkillData.zrobotDmgLaser = GetSkillCvar(&sk_zrobot_dmg_laser, 1.0f, 65535.0f);
	gSkillData.zrobotDmgSlash = GetSkillCvar(&sk_zrobot_dmg_slash, 1.0f, 65535.0f);
	gSkillData.zrobotHealth = GetSkillCvar(&sk_zrobot_health, 1.0f, 65535.0f);
	gSkillData.zrobotScore = GetSkillCvar(&sk_zrobot_score, 0.0f, 65535.0f);*/

	// PLAYER WEAPONS
	gSkillData.DmgCrowbar = GetSkillCvar(&sk_plr_crowbar, 1.0f, 65535.0f);
	gSkillData.Dmg9MM = GetSkillCvar(&sk_plr_9mm_bullet, 1.0f, 65535.0f);
	gSkillData.Dmg357 = GetSkillCvar(&sk_plr_357_bullet, 1.0f, 65535.0f);
	gSkillData.DmgMP5 = GetSkillCvar(&sk_plr_9mmAR_bullet, 1.0f, 65535.0f);
	gSkillData.DmgM203Grenade = GetSkillCvar(&sk_plr_9mmAR_grenade, 1.0f, 65535.0f);
	gSkillData.DmgBolt = GetSkillCvar(&sk_plr_bolt, 1.0f, 65535.0f);
	gSkillData.DmgBoltExplode = GetSkillCvar(&sk_plr_bolt_explode, 1.0f, 65535.0f);
	gSkillData.DmgBuckshot = GetSkillCvar(&sk_plr_buckshot, 1.0f, 65535.0f);
	gSkillData.DmgRPG = GetSkillCvar(&sk_plr_rpg, 1.0f, 65535.0f);
	gSkillData.DmgGauss = GetSkillCvar(&sk_plr_gauss, 1.0f, 65535.0f);
	gSkillData.DmgEgon = GetSkillCvar(&sk_plr_egon, 1.0f, 65535.0f);
	gSkillData.DmgHornet = GetSkillCvar(&sk_plr_hornet, 1.0f, 65535.0f);
	gSkillData.DmgGrenadeLauncher = GetSkillCvar(&sk_plr_glauncher, 1.0f, 65535.0f);
	gSkillData.DmgHandGrenadeExplosive = GetSkillCvar(&sk_plr_hand_grenade, 1.0f, 65535.0f);
	gSkillData.DmgHandGrenadeFreeze = GetSkillCvar(&sk_plr_hand_grenade_freeze, 1.0f, 65535.0f);
	gSkillData.DmgHandGrenadePoison = GetSkillCvar(&sk_plr_hand_grenade_poison, 1.0f, 65535.0f);
	gSkillData.DmgHandGrenadeBurn = GetSkillCvar(&sk_plr_hand_grenade_burn, 1.0f, 65535.0f);
	gSkillData.DmgHandGrenadeRadiation = GetSkillCvar(&sk_plr_hand_grenade_radiation, 1.0f, 65535.0f);
	gSkillData.DmgSatchel = GetSkillCvar(&sk_plr_satchel, 1.0f, 65535.0f);
	gSkillData.DmgStrikeTarget = GetSkillCvar(&sk_plr_strtarget, 1.0f, 65535.0f);
	gSkillData.DmgTripmine = GetSkillCvar(&sk_plr_tripmine, 1.0f, 65535.0f);
	gSkillData.DmgAcidGrenade = GetSkillCvar(&sk_plr_acid_grenade, 1.0f, 65535.0f);
	gSkillData.DmgDisplacerBeam = GetSkillCvar(&sk_plr_displacer_beam, 1.0f, 65535.0f);
	gSkillData.DmgDisplacerBlast = GetSkillCvar(&sk_plr_displacer_blast, 1.0f, 65535.0f);
	gSkillData.DmgChemgun = GetSkillCvar(&sk_plr_chemgun, 1.0f, 65535.0f);
	gSkillData.DmgPlasma = GetSkillCvar(&sk_plr_plasma, 1.0f, 65535.0f);
	gSkillData.DmgFlame = GetSkillCvar(&sk_plr_flame, 1.0f, 65535.0f);
	gSkillData.DmgNuclear = GetSkillCvar(&sk_plr_nuclear, 1.0f, 65535.0f);
	gSkillData.DmgSword = GetSkillCvar(&sk_plr_sword, 1.0f, 65535.0f);
	gSkillData.DmgBeamRifle = GetSkillCvar(&sk_plr_beamrifle, 1.0f, 65535.0f);
	gSkillData.DmgPlasmaBall = GetSkillCvar(&sk_plr_plasmaball, 1.0f, 65535.0f);
	gSkillData.DmgRazorDisk = GetSkillCvar(&sk_plr_razordisk, 1.0f, 65535.0f);
	gSkillData.DmgSniperRifle = GetSkillCvar(&sk_plr_sniperrifle, 1.0f, 65535.0f);
	gSkillData.DmgBHG = GetSkillCvar(&sk_plr_bhg, 1.0f, 65535.0f);
	gSkillData.FlashlightCharge = GetSkillCvar(&sk_plr_flashlight_charge, 1, MAX_FLASHLIGHT_CHARGE);// disallow charging/draining faster than in one second
	gSkillData.FlashlightDrain = GetSkillCvar(&sk_plr_flashlight_drain, 1, MAX_FLASHLIGHT_CHARGE);
	gSkillData.PlrAirTime = GetSkillCvar(&sk_plr_air_time, 1.0f, 65535.0f);

	// MONSTER WEAPONS
	gSkillData.Dmg12MM = GetSkillCvar(&sk_12mm_bullet, 1.0f, 65535.0f);
	gSkillData.monDmgMP5 = GetSkillCvar(&sk_9mmAR_bullet, 1.0f, 65535.0f);
	gSkillData.monDmg9MM = GetSkillCvar(&sk_9mm_bullet, 1.0f, 65535.0f);
	gSkillData.monDmgHornet = GetSkillCvar(&sk_mon_hornet, 1.0f, 65535.0f);
	gSkillData.DmgMortar = GetSkillCvar(&sk_dmg_mortar, 1.0f, 65535.0f);

	// HEALTH/CHARGE
	gSkillData.suitchargerCapacity = GetSkillCvar(&sk_suitcharger, 1.0f, 65535.0f);
	gSkillData.batteryCapacity = GetSkillCvar(&sk_battery, 1.0f, 65535.0f);
	gSkillData.healthchargerCapacity = GetSkillCvar(&sk_healthcharger, 1.0f, 65535.0f);
	gSkillData.healthkitCapacity = GetSkillCvar(&sk_healthkit, 1.0f, 65535.0f);
	gSkillData.scientistHeal = GetSkillCvar(&sk_scientist_heal, 1.0f, 65535.0f);
	gSkillData.foodHeal = GetSkillCvar(&sk_food_heal, 1.0f, 65535.0f);// XDM3037

	// Monster damage adj
	gSkillData.monHead = GetSkillCvar(&sk_monster_head, 0.01f, 1000.0f);
	gSkillData.monChest = GetSkillCvar(&sk_monster_chest, 0.01f, 1000.0f);
	gSkillData.monStomach = GetSkillCvar(&sk_monster_stomach, 0.01f, 1000.0f);
	gSkillData.monLeg = GetSkillCvar(&sk_monster_leg, 0.01f, 1000.0f);
	gSkillData.monArm = GetSkillCvar(&sk_monster_arm, 0.01f, 1000.0f);

	// Player damage adj
	gSkillData.plrHead = GetSkillCvar(&sk_player_head, 0.01f, 1000.0f);
	gSkillData.plrChest = GetSkillCvar(&sk_player_chest, 0.01f, 1000.0f);
	gSkillData.plrStomach = GetSkillCvar(&sk_player_stomach, 0.01f, 1000.0f);
	gSkillData.plrArm = GetSkillCvar(&sk_player_arm, 0.01f, 1000.0f);
	gSkillData.plrLeg = GetSkillCvar(&sk_player_leg, 0.01f, 1000.0f);
	gSkillData.plrScore = GetSkillCvar(&sk_player_score, 0.0f, 65535.0f);
}
