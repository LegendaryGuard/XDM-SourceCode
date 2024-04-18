//====================================================================
// All shared weapon-related definitions
// Used by all DLLs
//====================================================================
#ifndef WEAPONDEF_H
#define WEAPONDEF_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

// XDM3038: common item/weapon spawn flags
#define SF_ITEM_NOFALL			(1<<9) // 0x0200 // 512 // this item won't fall to the ground after spawning
//#define SF_ITEM_11			(1<<10)// 0x0400 // 1024
//#define SF_ITEM_12			(1<<11)// 0x0800 // 2048 // occupied by "not in mp"

// XDM: moved to header
// env_explosion / grenade flags
#define	SF_NODAMAGE				(1<<0)// 0x0001 // 1  // when set, ENV_EXPLOSION will not actually inflict damage
#define	SF_REPEATABLE			(1<<1)// 0x0002 // 2  // can this entity be refired?
#define SF_NOFIREBALL			(1<<2)// 0x0004 // 4  // don't draw the fireball
#define SF_NOSMOKE				(1<<3)// 0x0008 // 8  // don't draw the smoke
#define SF_NODECAL				(1<<4)// 0x0010 // 16 // don't make a scorch mark
#define SF_NOSPARKS				(1<<5)// 0x0020 // 32 // don't create sparks
#define SF_NOPARTICLES			(1<<6)// 0x0040 // 64 // don't create particles
#define SF_NOSOUND				(1<<7)// 0x0080 // 128// don't play sounds
#define SF_NUCLEAR				(1<<8)// 0x0100 // 256// nuclear env_explosion
// XDM3038b: mask to select only related flags, useful for transmission in limited sizes
#define SFM_EXPLOSION_FLAGS		(SF_NODAMAGE|SF_REPEATABLE|SF_NOFIREBALL|SF_NOSMOKE|SF_NODECAL|SF_NOSPARKS|SF_NOPARTICLES|SF_NOSOUND|SF_NUCLEAR)


// Weapon IDs. Maximum 32 weapons, see MAX_WEAPONS in "protocol.h"
// Warning! HL weapons MUST have same IDs in XHL!
enum WEAPONS
{
	WEAPON_NONE = 0,// 0 must be invalid
	WEAPON_CROWBAR,
	WEAPON_GLOCK,
	WEAPON_PYTHON,
	WEAPON_MP5,
	WEAPON_CHEMGUN,// XDM: this index is unused in HL
	WEAPON_CROSSBOW,
	WEAPON_SHOTGUN,
	WEAPON_RPG,
	WEAPON_GAUSS,
	WEAPON_EGON,// 10
	WEAPON_HORNETGUN,
	WEAPON_HANDGRENADE,
	WEAPON_TRIPMINE,
	WEAPON_SATCHEL,
	WEAPON_SNARK,
	WEAPON_DLAUNCHER,// XDM weapons start here
	WEAPON_GLAUNCHER,
	WEAPON_ALAUNCHER,
	WEAPON_SWORD,
	WEAPON_SNIPERRIFLE,// 20
	WEAPON_STRTARGET,
	WEAPON_PLASMA,
	WEAPON_FLAME,
	WEAPON_DISPLACER,
	WEAPON_BEAMRIFLE,
	WEAPON_BHG,
	WEAPON_UNUSED1,
	WEAPON_UNUSED2,
	WEAPON_CUSTOM1,
	WEAPON_CUSTOM2,// 30
	WEAPON_SUIT// HACK
};

#define WEAPON_ALLWEAPONS			(~(1<<WEAPON_SUIT))

// XDM3038a: UpdWeapons message or weapon_data_t::m_iWeaponState codes
enum weapon_states_e
{
	wstate_undefined = 0,// IMPORTANT: 0 must be invalid because structure is memset() to 0 and may not get filled afterwards
	wstate_current,
	wstate_current_ontarget,
	wstate_reloading,
	wstate_holstered,
	wstate_unusable,
	wstate_inventory,
	// add custom states here
	wstate_error = 127,
};

// bullet types
enum bullet_types_e
{
	BULLET_NONE = 0,
	BULLET_BREAK,
	BULLET_9MM,
	BULLET_357,
	BULLET_BUCKSHOT,
	BULLET_MP5,
	BULLET_12MM,
	BULLET_338,
};

#define BULLET_DEFAULT_DIST				3072
#define BULLET_SNIPER_DIST_MLT			2.0f
#define BULLET_FULLDAMAGE_DIST_K		0.75f
#define BULLET_DAMAGE_DECREASE			0.01f//1/100// Decrease damage per distance unit
#define BULLET_MAX_PIERCE				2

// Player shooting spread
#define SPREADFACTOR_DISABLED		0.1f
#define SPREADFACTOR_CROUCH_IDLE	0.1f
#define SPREADFACTOR_CROUCH_MOVING	0.5f
#define SPREADFACTOR_RUNNING		1.5f
#define SPREADFACTOR_WALKING		1.0f
#define SPREADFACTOR_STANDING		0.25f
#define SPREADFACTOR_AFTERSHOT		0.4f
#define SPREADFACTOR_INAIR			0.6f
#define SPREAD_ATTACK_DECAY_TIME	1.0f
#define SPREAD_DISPLAY_MAX			2.0f

// Global item sounds
#define DEFAULT_PICKUP_SOUND_AMMO			"items/9mmclip1.wav"
#define DEFAULT_PICKUP_SOUND_ITEM			"items/gunpickup2.wav"
#define DEFAULT_PICKUP_SOUND_WEAPON			"items/gunpickup2.wav"
#define DEFAULT_PICKUP_SOUND_CONTAINER		"items/gunpickup4.wav"
#define DEFAULT_RESPAWN_SOUND				"items/respawn.wav"
#define DEFAULT_RESPAWN_SOUND_ITEM			"items/respawn.wav"
#define DEFAULT_DROP_SOUND_AMMO				"items/weapondrop1.wav"
#define DEFAULT_DROP_SOUND_ITEM				"items/weapondrop1.wav"
#define DEFAULT_DROP_SOUND_WEAPON			"items/weapondrop1.wav"

// XDM: p_ models are too small
#define WEAPON_WORLD_SCALE					2.0f

// XDM shells are packed into one model
enum shell_bodies_e
{
	SHELL_BRASS = 0,
	SHELL_BUCKSHOT,
	SHELL_ARGRENADE,
	SHELL_GLAUNCHER
};

// weapon weight factors (for auto-switching) (-1 = noswitch)
#define CROWBAR_WEIGHT				1
#define GLOCK_WEIGHT				8
#define PYTHON_WEIGHT				9
#define MP5_WEIGHT					12
#define SHOTGUN_WEIGHT				10
#define CROSSBOW_WEIGHT				11
#define RPG_WEIGHT					17
#define GAUSS_WEIGHT				18
#define EGON_WEIGHT					16
#define HORNETGUN_WEIGHT			4
#define HANDGRENADE_WEIGHT			11
#define SNARK_WEIGHT				5
#define SATCHEL_WEIGHT				3
#define TRIPMINE_WEIGHT				2
#define DISPLACER_WEIGHT			19
#define DLAUNCHER_WEIGHT			15
#define GLAUNCHER_WEIGHT			16
#define ALAUNCHER_WEIGHT			10
#define SWORD_WEIGHT				2
#define CHEMGUN_WEIGHT				14
#define PLASMA_WEIGHT				18
#define FLAME_WEIGHT				12
#define STRTARGET_WEIGHT			17
#define BEAMRIFLE_WEIGHT			16
#define SNIPERRIFLE_WEIGHT			15
#define BLACKHOLE_WEIGHT			20

// weapon clip/carry ammo capacities
#if defined(OLD_WEAPON_AMMO_INFO)
#define URANIUM_MAX_CARRY			100
#define	_9MM_MAX_CARRY				250
#define _357_MAX_CARRY				36
#define BUCKSHOT_MAX_CARRY			125
#define BOLT_MAX_CARRY				50
#define ROCKET_MAX_CARRY			5
#define HANDGRENADE_MAX_CARRY		10
#define SATCHEL_MAX_CARRY			5
#define TRIPMINE_MAX_CARRY			5
#define SNARK_MAX_CARRY				15
#define HORNET_MAX_CARRY			10
#define M203_GRENADE_MAX_CARRY		10
#define GLAUNCHER_MAX_CARRY			10
#define ALAUNCHER_MAX_CARRY			50
#define LP_MAX_CARRY				100
#define FUEL_MAX_CARRY				100
#define STRTARGET_MAX_CARRY			4
#define BLADES_MAX_CARRY			50
#define SNIPER_MAX_CARRY			50
#endif

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP				-1
#define CROWBAR_MAX_CLIP			WEAPON_NOCLIP
#define GLOCK_MAX_CLIP				17
#define PYTHON_MAX_CLIP				6
#define MP5_MAX_CLIP				50
#define SHOTGUN_MAX_CLIP			8
#define CROSSBOW_MAX_CLIP			5
#define RPG_MAX_CLIP				1
#define GAUSS_MAX_CLIP				WEAPON_NOCLIP
#define DISPLACER_MAX_CLIP			WEAPON_NOCLIP
#define EGON_MAX_CLIP				WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP			WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP		WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP			WEAPON_NOCLIP// XDM3034 MUST be -1!
#define TRIPMINE_MAX_CLIP			WEAPON_NOCLIP
#define SNARK_MAX_CLIP				WEAPON_NOCLIP
#define FREEZEGRENADE_MAX_CLIP		WEAPON_NOCLIP
#define DLAUNCHER_MAX_CLIP			WEAPON_NOCLIP
#define GLAUNCHER_MAX_CLIP			1
#define ALAUNCHER_MAX_CLIP			5
#define SWORD_MAX_CLIP				WEAPON_NOCLIP
#define CHEMGUN_MAX_CLIP			WEAPON_NOCLIP
#define PLASMA_MAX_CLIP				WEAPON_NOCLIP
#define FLAME_MAX_CLIP				WEAPON_NOCLIP
#define STRTARGET_MAX_CLIP			WEAPON_NOCLIP
#define BEAMRIFLE_MAX_CLIP			WEAPON_NOCLIP// !!!
#define SNIPERRIFLE_MAX_CLIP		8
#define BLACKHOLE_MAX_CLIP			WEAPON_NOCLIP// !!!

// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE			GLOCK_MAX_CLIP
#define PYTHON_DEFAULT_GIVE			PYTHON_MAX_CLIP
#define MP5_DEFAULT_GIVE			MP5_MAX_CLIP
#define MP5_M203_DEFAULT_GIVE		0
#define SHOTGUN_DEFAULT_GIVE		SHOTGUN_MAX_CLIP*2
#define CROSSBOW_DEFAULT_GIVE		CROSSBOW_MAX_CLIP
#define RPG_DEFAULT_GIVE			2
#define GAUSS_DEFAULT_GIVE			20
#define EGON_DEFAULT_GIVE			20
#define HANDGRENADE_DEFAULT_GIVE	5
#define SATCHEL_DEFAULT_GIVE		1
#define TRIPMINE_DEFAULT_GIVE		1
#define SNARK_DEFAULT_GIVE			5
#define HIVEHAND_DEFAULT_GIVE		10
#define DISPLACER_DEFAULT_GIVE		20
#define DLAUNCHER_DEFAULT_GIVE		10
#define GLAUNCHER_DEFAULT_GIVE		5
#define ALAUNCHER_DEFAULT_GIVE		10
#define CHEMGUN_DEFAULT_GIVE		20
#define PLASMA_DEFAULT_GIVE			20
#define FLAME_DEFAULT_GIVE			20
#define STRTARGET_DEFAULT_GIVE		1
#define BEAMRIFLE_DEFAULT_GIVE		20
#define SNIPERRIFLE_DEFAULT_GIVE	SNIPERRIFLE_MAX_CLIP
#define BLACKHOLE_DEFAULT_GIVE		20

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE		20
#define AMMO_GLOCKCLIP_GIVE			GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE			PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE			MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE			200
#define AMMO_M203BOX_GIVE			2
#define AMMO_BUCKSHOTBOX_GIVE		12
#define AMMO_CROSSBOWCLIP_GIVE		CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE			RPG_MAX_CLIP
#define AMMO_SNARKBOX_GIVE			SNARK_DEFAULT_GIVE
#define AMMO_GLAUNCHER_GIVE			6
#define AMMO_ALAUNCHER_GIVE			10
#define AMMO_LPBOX_GIVE				50
#define AMMO_FUELTANK_GIVE			30
#define AMMO_RAZORBLADES_GIVE		10
#define AMMO_SNIPERBOX_GIVE			10

//#define WEAPON_IS_ONTARGET			0x40

#define MAX_WEAPON_NAME_LEN				32 // MAX_ENTITY_STRING_LENGTH?

// XDM3037a: Currently these should fit in 2 bytes on server and only 1 byte will be sent to client side
#define ITEM_FLAG_SELECTONEMPTY			(1<<0) // 0x0001 // 1  
#define ITEM_FLAG_NOAUTORELOAD			(1<<1) // 0x0002 // 2  
#define ITEM_FLAG_NOAUTOSWITCHEMPTY		(1<<2) // 0x0004 // 4  
#define ITEM_FLAG_LIMITINWORLD			(1<<3) // 0x0008 // 8  
#define ITEM_FLAG_EXHAUSTIBLE			(1<<4) // 0x0010 // 16  // HACK: since we currently cannot have multiple instances of the same weapon, we delete flagged weapons when ammo is depleted
#define ITEM_FLAG_CANNOTDROP			(1<<5) // 0x0020 // 32  // XDM: cannot be dropped manually
#define ITEM_FLAG_SUPERWEAPON			(1<<6) // 0x0040 // 64  // XDM3035 this is a powerful weapon, may be filtered by game policies
#define ITEM_FLAG_ADDDEFAULTAMMO2		(1<<7) // 0x0080 // 128 // XDM3037: add default amount of secondary ammo too NOTE: will become obsolete when m_iAmmoContained2 is used
// 2nd byte (server only!)
#define ITEM_FLAG_UNDERWATER1			(1<<8) // 0x0100 // 256 // XDM3037a: indicates this weapon can be used underwater
#define ITEM_FLAG_UNDERWATER2			(1<<9) // 0x0200 // 512 // XDM3037a: secondary
#define ITEM_FLAG_USEONLADDER			(1<<10)// 0x0400 // 1024// XDM3037a: ignore mp_laddershooting rule
#define ITEM_FLAG_NOWEAPONSTAY			(1<<11)// 0x0800 // 2048// XDM3037a: ignore mp_weaponstay rule
#define ITEM_FLAG_IMPORTANT				(1<<12)// 0x1000 // 4096// XDM3038c: always drop this weapon, don't allow it to be lost

// XDM3037: compacted
typedef struct ItemInfo_s
{
	short	iId;
	short	iFlags;// ITEM_FLAG_*
	short	iMaxClip;
	short	iWeight;// this value used to determine this weapon's importance in autoselection. XDM3038: byte->short
#if defined (SERVER_WEAPON_SLOTS)
	short	iSlot;// XDM3038: byte->short
	short	iPosition;// XDM3038: byte->short
#endif
	char szName[MAX_WEAPON_NAME_LEN];// XDM3038a: we can't hold pointer to a temporary object! //const char	*pszName;
	const char	*pszAmmo1;
#if defined (OLD_WEAPON_AMMO_INFO)
	short	iMaxAmmo1;
#endif
	const char	*pszAmmo2;// NOTE: we assume exhaustible weapons don't have secondary ammo
#if defined (OLD_WEAPON_AMMO_INFO)
	short	iMaxAmmo2;
#endif
	// UNDONE:	int		iMinAmmo1;
	//int		iMinAmmo2;
	//int		iUsageFlags;// like "can fire underwater", etc. For the AI.
} ItemInfo;


#define MAX_AMMO_NAME_LEN		16
#define AMMOINDEX_NONE			-1

// ID is the array index
typedef struct AmmoInfo_s
{
	short	iMaxCarry;// XDM3038: moved to the beginning
	char	name[MAX_AMMO_NAME_LEN];
	//char	classname[MAX_AMMO_NAME_LEN];// too long MAX_ENTITY_STRING_LENGTH]; // the idea was to use this classname for creation of instanced baselines, but it won't be filled until ammo actually spawns anyway
} AmmoInfo;


// Virtual sound for the AI
#define BIG_EXPLOSION_VOLUME		2048
#define NORMAL_EXPLOSION_VOLUME		1024
#define LOUD_GUN_VOLUME				960
#define SMALL_EXPLOSION_VOLUME		544
#define NORMAL_GUN_VOLUME			512
#define QUIET_GUN_VOLUME			224
#define	WEAPON_ACTIVITY_VOLUME		96
#define ITEM_PICKUP_VOLUME			64
#define	HUMAN_MAX_BODY_VOLUME		480
#define	HUMAN_WALK_VOLUME			48
#define	HUMAN_RUN_VOLUME			64
#define	HUMAN_JUMP_VOLUME			96

// Virtual light for the AI
#define	BRIGHT_GUN_FLASH			512
#define NORMAL_GUN_FLASH			256
#define	DIM_GUN_FLASH				128

// Bullet spread vectors
#define VECTOR_CONE_1DEGREES		Vector(0.00873f, 0.00873f, 0.00873f)
#define VECTOR_CONE_2DEGREES		Vector(0.01745f, 0.01745f, 0.01745f)
#define VECTOR_CONE_3DEGREES		Vector(0.02618f, 0.02618f, 0.02618f)
#define VECTOR_CONE_4DEGREES		Vector(0.03490f, 0.03490f, 0.03490f)
#define VECTOR_CONE_5DEGREES		Vector(0.04362f, 0.04362f, 0.04362f)
#define VECTOR_CONE_6DEGREES		Vector(0.05234f, 0.05234f, 0.05234f)
#define VECTOR_CONE_7DEGREES		Vector(0.06105f, 0.06105f, 0.06105f)
#define VECTOR_CONE_8DEGREES		Vector(0.06976f, 0.06976f, 0.06976f)
#define VECTOR_CONE_9DEGREES		Vector(0.07846f, 0.07846f, 0.07846f)
#define VECTOR_CONE_10DEGREES		Vector(0.08716f, 0.08716f, 0.08716f)
#define VECTOR_CONE_15DEGREES		Vector(0.13053f, 0.13053f, 0.13053f)
#define VECTOR_CONE_20DEGREES		Vector(0.17365f, 0.17365f, 0.17365f)
#define VECTOR_CONE_40DEGREES		Vector(0.34730f, 0.34730f, 0.34730f)
#define VECTOR_CONE_45DEGREES		Vector(0.41792f, 0.41792f, 0.41792f)

// CBasePlayer::AddPlayerItem() result codes
enum item_addresults_e
{
	ITEM_ADDRESULT_NONE = 0,// left alone, don't do anything
	ITEM_ADDRESULT_PICKED,// picked, moved into player's inventory, attach, don't destroy
	ITEM_ADDRESULT_EXTRACTED// extracted ammo, may be destroyed
};

// XDM3038: must be reliable as steel
enum item_states_e
{
	ITEM_STATE_WORLD = 0,
	ITEM_STATE_INVENTORY,
	ITEM_STATE_EQUIPPED,
	ITEM_STATE_HOLSTERED,
	ITEM_STATE_DROPPED
};

// Describes possible fire states
typedef enum weapon_firestates_e
{
	FIRE_OFF = 0,
	FIRE_CHARGE,
	FIRE_ON,
	FIRE_DISCHARGE,
	FIRE_CHANGEMODE
} WEAPON_FIRESTATE;

// For weapons that have mode switching
typedef enum weapon_firemodes_e
{
	FIRE_PRI = 0,
	FIRE_SEC
} WEAPON_FIREMODE;

// For weapons that have progressive zoom
typedef enum weapon_zoommodes_e
{
	ZOOM_OFF = 0,
	ZOOM_ON,
	ZOOM_INCREASING,
	ZOOM_DECREASING
	// if you need fixed modes, add them here, because we need all above to fit in 2 bits and ON/OFF be boolean
} WEAPON_ZOOMMODE;

// Time to play rare animations
#define WEAPON_LONG_IDLE_TIME			10.0f


//-----------------------------------------------------------------------------
// Shared per-weapon definitions
//-----------------------------------------------------------------------------
#define DEFAULT_ATTACK_INTERVAL			1.0f

#define ALAUNCHER_ATTACK_INTERVAL1		0.4f
#define ALAUNCHER_ATTACK_INTERVAL2		0.6f
#define ALAUNCHER_AMMO_USE_DELAY		0.4f
#define ALAUNCHER_GRENADE_TIME			5.0f
#define ALAUNCHER_GRENADE_VELOCITY		1280.0f

#define BEAMRIFLE_USE_AMMO1				2
#define BEAMRIFLE_USE_AMMO2				10
#define BEAMRIFLE_AMMO_USE_INTERVAL		0.15f
#define BEAMRIFLE_CHARGE_INTERVAL1		BEAMRIFLE_AMMO_USE_INTERVAL*BEAMRIFLE_USE_AMMO1
#define BEAMRIFLE_CHARGE_INTERVAL2		BEAMRIFLE_AMMO_USE_INTERVAL*BEAMRIFLE_USE_AMMO2
#define BEAMRIFLE_DISCHARGE_INTERVAL1	0.6f
#define BEAMRIFLE_DISCHARGE_INTERVAL2	1.0f
#define BEAMRIFLE_MAX_TRACES			128

#define BHG_ATTACK_INTERVAL1			1.0f
#define BHG_ATTACK_INTERVAL2			1.0f
#define BHG_CHARGE_SPEED				5.0f// ammo/second
#define	BHG_CHARGE_VOLUME				256
#define	BHG_OVERCHARGE_TIME				5.0f
#define BHG_FIRE_VOLUME					450
#define BHG_AMMO_USE_MIN1				10
#define BHG_AMMO_USE_MIN2				2
#define BHG_AMMO_USE_MAX1				BHG_AMMO_USE_MIN1
#define BHG_AMMO_USE_MAX2				20
#define BHG_AMMO_USE					1
#define BHG_AMMO_USE_INTERVAL			0.1f
#define BHG_MAX_PITCH					200.0f

#define CHEMGUN_ATTACK_INTERVAL1		0.4f
#define CHEMGUN_ATTACK_INTERVAL2		0.25f
#define CHEMGUN_SWITCH_INTERVAL			2.0f

#define CROSSBOW_ATTACK_INTERVAL1		1.0f
#define CROSSBOW_ATTACK_INTERVAL2		0.2f
#define CROSSBOW_RELOAD_DELAY			4.5f
#define CROSSBOW_MAX_ZOOM_FOV			15.0f

#define	CROWBAR_BODYHIT_VOLUME			128
#define	CROWBAR_WALLHIT_VOLUME			512
#define CROWBAR_ATTACK_INTERVAL			0.25f
#define CROWBAR_SWING_INTERVAL			0.5f
#define CROWBAR_HIT_DELAY				0.2f
#define CROWBAR_HIT_DISTANCE			32.0f

#define DISPLACER_CHARGE_TIME1			1.2f
#define DISPLACER_CHARGE_TIME2			1.2f
#define DISPLACER_ANIM_TIME				0.85f
#define DISPLACER_ATTACK_INTERVAL		0.2f
#define DISPLACER_AMMO_USE1				10
#define DISPLACER_AMMO_USE2				20

#define DLAUNCHER_ATTACK_INTERVAL1		0.3f
#define DLAUNCHER_ATTACK_INTERVAL2		0.4f
#define DLAUNCHER_USE_AMMO1				1
#define DLAUNCHER_USE_AMMO2				1

#define EGON_ATTACK_INTERVAL1			0.5f
#define EGON_ATTACK_INTERVAL2			0.6f
#define EGON_SWITCH_INTERVAL1			1.4f
#define EGON_SWITCH_INTERVAL2			0.7f
#define EGON_PULSE_INTERVAL				0.1f
#define EGON_DEFAULT_COLOR1				Vector(103,255,143)
#define EGON_DEFAULT_COLOR2				Vector(103,143,255)
#define EGON_BEAM_WIDTH					24
#define EGON_SPIRAL_WIDTH1				6
#define EGON_SPIRAL_WIDTH2				10
#define EGON_BEAM_SPRITE				"sprites/egonbeam.spr"
#define EGON_SPIRAL_SPRITE				"sprites/egonsprl.spr"
#define EGON_FLARE_SPRITE				"sprites/egonspark.spr"
#define	EGON_ATTACK_VOLUME				450
#define	EGON_DISTANCE1					3072.0f
#define	EGON_DISTANCE2					2560.0f

#define FLAMETHROWER_ATTACK_INTERVAL1	0.5f
#define FLAMETHROWER_ATTACK_INTERVAL2	0.5f
#define FLAMETHROWER_AMMO_USE_INTERVAL1	0.5f
#define FLAMETHROWER_AMMO_USE_INTERVAL2	0.6f
#define	FLAMETHROWER_ATTACK_VOLUME		96
//#define	FLAMETHROWER_FLAME_SPEED1		128.0f
//#define	FLAMETHROWER_FLAME_SPEED2		256.0f
#define	FLAMETHROWER_DISTANCE1			512.0f// flame cone
#define	FLAMETHROWER_DISTANCE2			128.0f// torch
#define	FLAMETHROWER_UPDATE_INTERVAL1	0.05f// flame cone is virtual
#define	FLAMETHROWER_UPDATE_INTERVAL2	0.015625f// torch flame requires frequent updates
#define FLAME_BEAM_SPRITE				"sprites/flamebeam.spr"
#define FLAME_BEAM_WIDTH				20// torch mode beam
#define FLAME_CONE_RADIUS_MAX			64.0f// widest part of the flame cone
#define FLAME_CONE_RADIUS_DELTA			64.0f// how fast flame cone radius increases
#define FLAME_CONE_DIST_DELTA			480.0f// how fast flame distance increases
#define FLAME_CONE_ANGLE_RATE			160.0f

#define GAUSS_ATTACK_INTERVAL1			0.2f
#define GAUSS_ATTACK_INTERVAL2			0.4f
#define GAUSS_CHARGE_TIME1				2.0f
#define GAUSS_CHARGE_TIME2				4.0f
#define	GAUSS_CHARGE_VOLUME				256// how loud gauss is while charging
#define GAUSS_FIRE_VOLUME				450// how loud gauss is when discharged
#define GAUSS_AMMO_USE1					2
#define GAUSS_AMMO_USE2					1
#define GAUSS_AMMO_USE_INTERVAL			0.1f
#define GAUSS_AMMO_USE_INTERVAL_SP		0.3f
#define GAUSS_DEFAULT_COLOR1_R			127
#define GAUSS_DEFAULT_COLOR1_G			255
#define GAUSS_DEFAULT_COLOR1_B			95
#define GAUSS_DEFAULT_COLOR2_R			255
#define GAUSS_DEFAULT_COLOR2_G			255
#define GAUSS_DEFAULT_COLOR2_B			255
#define GAUSS_BEAM_WIDTH1				12
#define GAUSS_BEAM_WIDTH2				24
#define GAUSS_BEAM_MAX_HITS				8
#define GAUSS_BEAM_MAX_DIST				8192.0f
#define GAUSS_SHOCK_TIME				8.0f
#define GAUSS_PUNCH_DEPTH				8
#define GAUSS_MAX_PITCH					150.0f

#define GLAUNCHER_ATTACK_INTERVAL1		0.8f
#define GLAUNCHER_ATTACK_INTERVAL2		0.8f
#define GLAUNCHER_USE_AMMO1				1
#define GLAUNCHER_USE_AMMO2				1
#define GLAUNCHER_RELOAD_DELAY			1.6f
#define GLAUNCHER_GRENADE_TIME			4.0f
#define GLAUNCHER_GRENADE_VELOCITY		1408.0f

#define GLOCK_ATTACK_INTERVAL1			0.3f
#define GLOCK_ATTACK_INTERVAL2			0.2f
#define GLOCK_RELOAD_DELAY				1.5f

#define HANDGRENADE_ATTACK_INTERVAL1	0.5f
#define HANDGRENADE_ATTACK_INTERVAL2	0.5f
#define HANDGRENADE_IGNITE_TIME			3.0f
#define	HANDGRENADE_LAST_TYPE			GREN_RADIOACTIVE// XDM3038c
#define	HANDGRENADE_THROW_VELOCITY		380// was 240 with gravity 0.5
// Handgrenade types
enum grenade_types_e
{
	GREN_EXPLOSIVE = 0,
	GREN_FREEZE,
	GREN_POISON,
	GREN_NAPALM,
	GREN_RADIOACTIVE,
	GREN_TELEPORTER,
	GREN_NUCLEAR,// for network only
};
// Grenade weapon model body groups
enum grenade_bodygroups_e
{
	GRENBG_MAIN = 0,
	GRENBG_RING
};
#define GREN_FREEZE_TIME					8.0f
#define GREN_BURN_TIME						15.0f
#define GREN_BURN_TIMES_PER_SECOND			10
#define GREN_POISON_TIME					3.0f
#define GREN_POISON_AFTERTIME				10.0f
#define GREN_RADIATION_ZOFFSET				64
#define GREN_RADIATION_DELAY				2.0f
#define GREN_RADIATION_AFTERTIME			8.0f
#define GREN_TELEPORT_AFTERTIME				4.0f
#define GREN_TELEPORT_CRITICAL_RADIUS		64.0f
#define GREN_NUCLEAR_RADIUS					2048
#define GREN_NUCLEAR_AFTERTIME				10.0f
#define GREN_DAMAGE_TO_RADIUS				2.5f// radius = damage*this
#define GREN_DAMAGE_TO_RADIUS_POST			5.0f// For DoDamageThink() - radiaiton, poisoned air, etc. where damage amount is small
#define GREN_DAMAGE_TO_RADIUS_FREEZE		5.0f
#define GREN_DAMAGE_TO_RADIUS_POISON		6.0f
#define GREN_DAMAGE_TO_RADIUS_BURN			5.0f
#define GREN_DAMAGE_TO_RADIUS_RADIATION		9.0f
//#define GREN_DAMAGE_TO_RADIUS_GRAVITY		8.0f
#define GREN_TELEPORT_SPRITE				"sprites/wflare2.spr"
#define GREN_RADIATION_SPRITE				"sprites/radiation.spr"

#define HGUN_ATTACK_INTERVAL1			0.25f
#define HGUN_ATTACK_INTERVAL2			0.1f
#define HGUN_RECHARGE_DELAY				0.5f

#define MP5_ATTACK_INTERVAL1			0.1f
#define MP5_ATTACK_INTERVAL2			1.0f
#define MP5_RELOAD_DELAY				1.5f

#define PLASMA_ATTACK_INTERVAL1			0.15f
#define PLASMA_ATTACK_INTERVAL2			0.75f
#define PLASMA_CHARGE_INTERVAL			0.5f
#define PLASMA_PULSE_INTERVAL			0.1f
#define PLASMA_BEAM_DISTANCE			1024
#define PLASMA_BEAM_WIDTH				80
#define PLASMA_NOISE_WIDTH1				16
#define PLASMA_NOISE_WIDTH2				8
#define PLASMA_BEAM_SPRITE				"sprites/plasmabeam.spr"
#define PLASMA_NOISE_SPRITE1			"sprites/plasma.spr"
#define PLASMA_NOISE_SPRITE2			"sprites/xbeam1.spr"
#define PLASMA_END_SPRITE				"sprites/plasmaend.spr"
#define PLASMA_HIT_SPRITE				"sprites/plasmahit.spr"
#define PLASMA_ATTACK_VOLUME			500

#define PYTHON_ATTACK_INTERVAL1			0.75f
#define PYTHON_ATTACK_INTERVAL2			0.5f
#define PYTHON_RELOAD_DELAY				2.4f
#define PYTHON_ZOOM_FOV					40

#define RIFLE_ATTACK_INTERVAL1			1.75f
#define RIFLE_ATTACK_INTERVAL2			0.5f
#define RIFLE_RELOAD_DELAY				2.75f
#define RIFLE_MAX_ZOOM_FOV				10.0f

#define RPG_ATTACK_INTERVAL1			1.25f
#define RPG_ATTACK_INTERVAL2			0.8f
#define RPG_RELOAD_TIME					2.1f

#define SATCHEL_ATTACK_INTERVAL1		0.75f// throw charges
#define SATCHEL_ATTACK_INTERVAL2		0.5f// radio click
#define SATCHEL_CLASSNAME				"monster_satchel"// horrible HL compatibility

enum satchelcharge_texturegroups_e
{
	SATCHEL_TEXGRP_PICKUP = 0,
	SATCHEL_TEXGRP_ARMED,
	SATCHEL_TEXGRP_DETONATE
};

#define SHOTGUN_PUMP_TIME1				0.9f
#define SHOTGUN_PUMP_TIME2				1.3f
#define SHOTGUN_RELOAD_DELAY			0.25f
#define SHOTGUN_USE_AMMO1				1
#define SHOTGUN_USE_AMMO2				2
#define SHOTGUN_PELLETS_PER_AMMO		4
#define SHOTGUN_RECOIL2					80.0f

#define SQUEAK_ATTACK_INTERVAL1			0.3f
#define SQUEAK_ATTACK_INTERVAL2			1.0f
#define SQUEAK_CLASSNAME				"monster_snark"

#define STRTARGET_ATTACK_INTERVAL1		0.5f
#define STRTARGET_ATTACK_INTERVAL2		1.0f

#define	SWORD_HIT_VOLUME				512
#define	SWORD_HIT_MAX_DIST1				48
#define	SWORD_HIT_MAX_DIST2				32
#define SWORD_ATTACK_INTERVAL1			0.5f
#define SWORD_ATTACK_INTERVAL2			0.3f
#define SWORD_SECONDARY_MULTIPLIER		0.5f// 50% damage

#define TRIPMINE_ATTACK_INTERVAL1		0.3f
#define TRIPMINE_ATTACK_INTERVAL2		0.3f
#define TRIPMINE_CLASSNAME				"monster_tripmine"// horrible HL compatibility
#define TRIPMINE_LAST_MODE				TRIPMINE_EX_AGRENADES//TRIPMINE_EX_EXTERNAL
#define TRIPMINE_SEARCH_RADIUS			128
#define TRIPMINE_POWERUP_TIME1			2.5f
#define TRIPMINE_POWERUP_TIME2			2.0f
#define TRIPMINE_BEAM_WIDTH				10
#define SF_TRIPMINE_QUICKPOWERUP		1
// Trip mine modes
enum tripmine_ping_modes_e
{
	TRIPMINE_PM_BEAM = 0,
	TRIPMINE_PM_RADIUS
};

// Currently used by overdrive mode
enum tripmine_explosions_e
{
	TRIPMINE_EX_NORMAL = 0,
	TRIPMINE_EX_BOLTS,
	TRIPMINE_EX_BOLTS_EX,
	TRIPMINE_EX_LGRENADES,
	TRIPMINE_EX_LGRENADES_EX,
	TRIPMINE_EX_AGRENADES,
	TRIPMINE_EX_EXTERNAL// use attached grenade type // TODO: combo tripmine+grenade
};

#define NUKE_AMMO_USE_URANIUM			100
#define NUKE_AMMO_USE_SATCHEL			2


//-----------------------------------------------------------------------------
// shared projectile definitions
//-----------------------------------------------------------------------------
enum gauss_event_e
{
	GAUSS_EV_REFLECT = 0,
	GAUSS_EV_EXPLODE,
};

enum agrenade_event_e
{
	AGRENADE_EV_START = 0,
	AGRENADE_EV_TOUCH,
	AGRENADE_EV_HIT,
	AGRENADE_EV_HITSOLID,
	AGRENADE_EV_HITWATER
};

enum bolt_event_e
{
	BOLT_EV_HIT = 0,
	BOLT_EV_EXPLODE,
};

enum hornet_type_e
{
	HORNET_TYPE_GREEN = 0,
	HORNET_TYPE_BLUE,
	HORNET_TYPE_YELLOW,
	HORNET_TYPE_RED
};

enum hornet_event_e
{
	HORNET_EV_START = 0,
	HORNET_EV_BUZZ,
	HORNET_EV_HIT,
	HORNET_EV_DESTROY
};

enum lightprojectile_event_e
{
	LIGHTP_EV_START = 0,
	LIGHTP_EV_HIT,
	LIGHTP_EV_HITSOLID,
	LIGHTP_EV_HITWATER
};

enum plasmaball_event_e
{
	PLASMABALL_EV_START = 0,
	PLASMABALL_EV_HIT,
	PLASMABALL_EV_HITSOLID,
	PLASMABALL_EV_HITWATER
};

enum razordisk_hittype_e
{
	RAZORDISK_HIT_SOLID = 0,
	RAZORDISK_HIT_ARMOR,
	RAZORDISK_HIT_FLESH,
	RAZORDISK_HIT_STUCK,
};

enum razordisk_mode_e
{
	RAZORDISK_REFLECT = 0,
	RAZORDISK_EXPLODE,
};

enum teleporter_blasttype_e
{
	TELEPORTER_BT_NORMAL = 0,
	TELEPORTER_BT_BEAM,
	TELEPORTER_BT_COL_ENTITY,
	TELEPORTER_BT_COL_PROJECTILE,
	TELEPORTER_BT_COL_TELEPORTER,
	TELEPORTER_BT_COL_PLASMA,
	TELEPORTER_BT_COL_WATER,
};

#endif // WEAPONDEF_H
