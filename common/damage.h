//====================================================================
//
// Purpose: damage constants
//
//====================================================================

#ifndef DAMAGE_H
#define DAMAGE_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* !__MINGW32__ */
#endif

// Damage types (bits, can be combined)

#define DMG_GENERIC			0// generic damage was done, never ignored
#define DMG_CRUSH			(1 << 0)// crushed by falling or moving object
#define DMG_BULLET			(1 << 1)// shot
#define DMG_SLASH			(1 << 2)// cut, clawed, stabbed
#define DMG_BURN			(1 << 3)// heat burned
#define DMG_FREEZE			(1 << 4)// frozen
#define DMG_FALL			(1 << 5)// fell too far
#define DMG_BLAST			(1 << 6)// explosive blast damage
#define DMG_CLUB			(1 << 7)// crowbar, punch, headbutt
#define DMG_SHOCK			(1 << 8)// electric shock
#define DMG_SONIC			(1 << 9)// sound pulse shockwave
#define DMG_ENERGYBEAM		(1 << 10)// laser or other high energy beam
#define DMG_MICROWAVE		(1 << 11)// XDM3038: high energy radio waves
#define DMG_NEVERGIB		(1 << 12)// with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB		(1 << 13)// with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_DROWN			(1 << 14)// drowning
#define DMG_PARALYZE		(1 << 15)// slows affected creature down
#define DMG_NERVEGAS		(1 << 16)// nerve toxins, very bad
#define DMG_POISON			(1 << 17)// blood poisioning
#define DMG_RADIATION		(1 << 18)// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)// drowning recovery
#define DMG_ACID			(1 << 20)// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)// in an oven
#define DMG_SLOWFREEZE		(1 << 22)// in a subzero freezer
#define DMG_NOHITPOINT		(1 << 23)// XDM3037: direction is still registered, but no headshots
#define DMG_IGNITE			(1 << 24)// Entities hit by this begin to burn
#define DMG_RADIUS_MAX		(1 << 25)// Radius damage with this flag doesn't decrease over distance
#define DMG_DONT_BLEED		(1 << 26)// Don't spawn blood (radiation, etc.)
#define DMG_IGNOREARMOR		(1 << 27)// Damage ignores target's armor
#define DMG_VAPOURIZING		(1 << 28)// XDM3034: DMG_ALWAYSGIB with no gibs
#define DMG_WALLPIERCING	(1 << 29)// Blast Damages ents through walls
#define DMG_DISINTEGRATING	(1 << 30)// XDM3035: makes entities Disintegrate()
#define DMG_CUSTOM			(1 << 31)// XDM3038a

#define MAX_DMG_TYPES		(CHAR_BIT*sizeof(int32)/sizeof(byte))// 32 bits (int32 limit)

// Damage masks (bits, for certain checks)

// time-based damage
// wrong! this just ignores 1st 14 types  #define DMG_TIMEBASED		(~(0x3fff))	// mask for time-based damage
#define DMGM_TIMEBASED		(DMG_DROWN | DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN | DMG_SLOWFREEZE | DMG_IGNITE)
// these can break objects (like crates)
#define DMGM_BREAK			(DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_FALL | DMG_BLAST | DMG_CLUB | DMG_SONIC | DMG_ENERGYBEAM | DMG_ALWAYSGIB)
// these are the damage types that are allowed to gib corpses
#define DMGM_GIB_CORPSE		(DMG_CRUSH | DMG_SLASH | DMG_BLAST | DMG_SONIC | DMG_CLUB | DMG_ALWAYSGIB)// DMG_FALL
// these can push some objects (monsters, crates)
#define DMGM_PUSH			(DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_SONIC)
// any of these will shake screen via pev->punchangle
#define DMGM_PUNCHVIEW		(DMG_BULLET | DMG_SLASH | DMG_CLUB | DMG_ENERGYBEAM)
// any of these can be classified as fire damage
#define DMGM_FIRE			(DMG_BURN | DMG_SLOWBURN)
// any of these can be classified as frost damage
#define DMGM_COLD			(DMG_FREEZE | DMG_SLOWFREEZE)
// any of these can be classified as poison damage
#define DMGM_POISON			(DMG_NERVEGAS | DMG_POISON | DMG_ACID)
// these make victims bleed
#define DMGM_BLEED			(DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)
// right now: any of these can prevent victim from using weapons
#define DMGM_DISARM			(DMG_CRUSH | DMG_SLASH | DMG_ENERGYBEAM | DMG_DROWN | DMG_NERVEGAS)
// any of these can prevent victim from climbing ladders
#define DMGM_LADDERFALL		(DMG_CRUSH | DMG_SLASH | DMG_BLAST | DMG_PARALYZE)
// these produce visible effects (smoke clouds, gas, splashes, etc.)
#define DMGM_VISIBLE		(DMGM_FIRE | DMGM_POISON | DMG_IGNITE | DMG_VAPOURIZING | DMG_DISINTEGRATING)
// these are the damage types that have client hud art
#define DMGM_SHOWNHUD		(DMG_BURN | DMG_FREEZE | DMG_SHOCK | DMG_SONIC | DMG_MICROWAVE | DMG_DROWN | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_ACID | DMG_SLOWBURN | DMG_SLOWFREEZE | DMGM_DISARM)
// these make player HUD malfunction in different ways
#define DMGM_HUDFAIL_COLOR	(DMG_BURN | DMG_MICROWAVE | DMG_RADIATION | DMG_IGNITE | DMG_DISINTEGRATING)
#define DMGM_HUDFAIL_SPR	(DMG_SHOCK | DMG_MICROWAVE | DMG_DISINTEGRATING)
#define DMGM_HUDFAIL_POS	(DMG_BLAST | DMG_DISINTEGRATING)
#define DMGM_HUD_DISTORT	(DMGM_HUDFAIL_COLOR | DMGM_HUDFAIL_SPR | DMGM_HUDFAIL_POS)
// bits that don't define damage types, but rather add special properties
#define DMGM_MODIFIERS		(DMG_NEVERGIB | DMG_ALWAYSGIB | DMG_PARALYZE | DMG_NOHITPOINT | DMG_IGNITE | DMG_RADIUS_MAX | DMG_DONT_BLEED | DMG_IGNOREARMOR | DMG_VAPOURIZING | DMG_WALLPIERCING | DMG_DISINTEGRATING)
// bits that do ricochet effect upon hitting armor
#define DMGM_RICOCHET		(DMG_BULLET | DMG_SLASH | DMG_CLUB)
//#define DMGM_TYPES			not DMGM_MODIFIERS


// Damage radius = amount * this value:
#define DAMAGE_TO_RADIUS_DEFAULT		2.5f

// How often time-based damage is inflicted
#define TIMEBASED_DAMAGE_INTERVAL		2.0f

// NOTE: tweak these values based on gameplay feedback:
#define TD_PARALYZE_DURATION	2	// number of ^ intervals to take damage
#define TD_PARALYZE_DAMAGE		1.0	// damage to take each interval

#define TD_NERVEGAS_DURATION	2
#define TD_NERVEGAS_DAMAGE		5.0

#define TD_POISON_DURATION		5
#define TD_POISON_DAMAGE		2.0

#define TD_RADIATION_DURATION	2
#define TD_RADIATION_DAMAGE		1.0

#define TD_ACID_DURATION		2
#define TD_ACID_DAMAGE			5.0

#define TD_SLOWBURN_DURATION	2
#define TD_SLOWBURN_DAMAGE		5.0

#define TD_SLOWFREEZE_DURATION	2
#define TD_SLOWFREEZE_DAMAGE	1.0

enum ITBD
{
	itbd_Paralyze = 0,
	itbd_NerveGas,
	itbd_Poison,
	itbd_Radiation,
	itbd_DrownRecover,
	itbd_Acid,
	itbd_SlowBurn,
	itbd_SlowFreeze,
	CDMG_TIMEBASED
};

// How much damage takes armor
//#define ARMOR_BONUS					0.5// Each point of armor is work 1/x points of health
// Armor takes % of these types of damage:
#define ARMOR_TAKE_GENERIC				0.5f
#define ARMOR_TAKE_BULLET				0.9f// adaptive armor was made to absorb bullet damage
#define ARMOR_TAKE_CLUB					0.75f

// How much of pev->max_health must be taken before gibbing a monster's body
#define GIB_HEALTH_PERCENT				0.75f// for GIB_NORMAL
#define GIB_HEALTH_PERCENT_BREAK		0.8f// for DMGM_BREAK
#define GIB_HEALTH_PERCENT_GIB			0.5f// for DMGM_GIB_CORPSE

// Suit notifications use this
#define HEALTH_PERCENT_TRIVIAL			0.75f
#define HEALTH_PERCENT_CRITICAL			0.3f
#define HEALTH_PERCENT_NEARDEATH		0.1f

// % of max_health monsters consider as heavy damage
#define HEAVY_DAMAGE_PERCENT		0.25f

// When calling Killed(), a value that governs gib behavior is expected to be one of these values
enum GIBMODE
{
	GIB_NORMAL = 0,		// gib if entity was overkilled
	GIB_NEVER,			// never gib, no matter how much death damage is done
	GIB_ALWAYS,			// always gib
	GIB_DISINTEGRATE,	// no gibs, disintegration
	GIB_FADE,			// no gibs, fade body
	GIB_REMOVE			// no gibs, remove body instantly
};

// Returned by DamageInteraction(), this value affects RadiusDamage() behavior
enum DMG_INTERACTION
{
	DMGINT_NONE = 0,
	DMGINT_THRU,
	DMGINT_REFLECT,
	DMGINT_ABSORB
};

#endif // DAMAGE_H
