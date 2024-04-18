//#include <string.h>
#include "extdll.h"
#include "pm_materials.h"

// TODO: write an object-oriented material system with external database

const char *gSoundsSparks[] =
{
	"buttons/spark1.wav",
	"buttons/spark2.wav",
	"buttons/spark3.wav",
	"buttons/spark4.wav",
	"buttons/spark5.wav",
	"buttons/spark6.wav",
	nullptr
};

const char *gSoundsRicochet[] =
{
	"weapons/ric1.wav",
	"weapons/ric2.wav",
	"weapons/ric3.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
	nullptr
};

const char *gSoundsShell9mm[] =
{
	"player/pl_shell1.wav",
	"player/pl_shell2.wav",
	"player/pl_shell3.wav",
	nullptr
};

const char *gSoundsShellShotgun[] =
{
	"weapons/sshell1.wav",
	"weapons/sshell2.wav",
	"weapons/sshell3.wav",
	nullptr
};

const char *gSoundsDropBody[] =
{
	"common/bodydrop1.wav",
	"common/bodydrop2.wav",
	"common/bodydrop3.wav",
	"common/bodydrop4.wav",
	nullptr
};


// NUM_SHARD_SOUNDS
const char *gSoundsWood[] =
{
	"debris/wood1.wav",
	"debris/wood2.wav",
	"debris/wood3.wav",
	"debris/wood4.wav",
	"debris/wood5.wav",
	"debris/wood6.wav",
	"debris/wood7.wav",
	"debris/wood8.wav",
	nullptr
};

const char *gSoundsFlesh[] =
{
	"debris/flesh1.wav",
	"debris/flesh2.wav",
	"debris/flesh3.wav",
	"debris/flesh4.wav",
	"debris/flesh5.wav",
	"debris/flesh6.wav",
	"debris/flesh7.wav",
	"debris/flesh8.wav",
	nullptr
};

const char *gSoundsMetal[] =
{
	"debris/metal1.wav",
	"debris/metal2.wav",
	"debris/metal3.wav",
	"debris/metal4.wav",
	"debris/metal5.wav",
	"debris/metal6.wav",
	"debris/metal7.wav",
	"debris/metal8.wav",
	nullptr
};

const char *gSoundsConcrete[] =
{
	"debris/concrete1.wav",
	"debris/concrete2.wav",
	"debris/concrete3.wav",
	"debris/concrete4.wav",
	"debris/concrete5.wav",
	"debris/concrete6.wav",
	"debris/concrete7.wav",
	"debris/concrete8.wav",
	nullptr
};

const char *gSoundsGlass[] =
{
	"debris/glass1.wav",
	"debris/glass2.wav",
	"debris/glass3.wav",
	"debris/glass4.wav",
	"debris/glass5.wav",
	"debris/glass6.wav",
	"debris/glass7.wav",
	"debris/glass8.wav",
	nullptr
};

const char *gSoundsCeiling[] =
{
	"debris/ceiling1.wav",
	"debris/ceiling2.wav",
	"debris/ceiling3.wav",
	"debris/ceiling4.wav",
	"debris/ceiling1.wav",// can't be helped, duplicate
	"debris/ceiling2.wav",
	"debris/ceiling3.wav",
	"debris/ceiling4.wav",
	nullptr
};

// NUM_PUSH_SOUNDS
const char *gPushSoundsGlass[]		= {"debris/pushglass1.wav", "debris/pushglass2.wav", "debris/pushglass3.wav"};
const char *gPushSoundsWood[]		= {"debris/pushwood1.wav", "debris/pushwood2.wav", "debris/pushwood3.wav"};
const char *gPushSoundsMetal[]		= {"debris/pushmetal1.wav", "debris/pushmetal2.wav", "debris/pushmetal3.wav"};
const char *gPushSoundsFlesh[]		= {"debris/pushflesh1.wav", "debris/pushflesh2.wav", "debris/pushflesh3.wav"};
const char *gPushSoundsConcrete[]	= {"debris/pushstone1.wav", "debris/pushstone2.wav", "debris/pushstone3.wav"};

// NUM_BREAK_SOUNDS
const char *gBreakSoundsGlass[]		= {"debris/bustglass1.wav", "debris/bustglass2.wav"};
const char *gBreakSoundsWood[]		= {"debris/bustcrate1.wav", "debris/bustcrate2.wav"};
const char *gBreakSoundsMetal[]		= {"debris/bustmetal1.wav", "debris/bustmetal2.wav"};
const char *gBreakSoundsFlesh[]		= {"debris/bustflesh1.wav", "debris/bustflesh2.wav"};
const char *gBreakSoundsConcrete[]	= {"debris/bustconcrete1.wav", "debris/bustconcrete2.wav"};
const char *gBreakSoundsCeiling[]	= {"debris/bustceiling1.wav", "debris/bustceiling2.wav"};

// sorted by Materials struct
const char **gPushSounds[] = {
	gPushSoundsGlass,// matGlass
	gPushSoundsWood,
	gPushSoundsMetal,
	gPushSoundsFlesh,
	gPushSoundsConcrete,
	gPushSoundsWood,
	gPushSoundsMetal,
	gPushSoundsGlass,
	gPushSoundsConcrete,
	(const char **)nullptr,// matNone
	(const char **)nullptr// matLastMaterial
};

// Breakable destruction sounds
// sorted by Materials struct
const char **gBreakSounds[] = {
	gBreakSoundsGlass,// matGlass
	gBreakSoundsWood,
	gBreakSoundsMetal,
	gBreakSoundsFlesh,
	gBreakSoundsConcrete,
	gBreakSoundsCeiling,
	gBreakSoundsMetal,
	gBreakSoundsGlass,
	gBreakSoundsConcrete,
	(const char **)nullptr,// matNone
	(const char **)nullptr// matLastMaterial
};

// Breakable damage/shard sounds
// sorted by Materials struct
const char **gShardSounds[] = {
	gSoundsGlass,// matGlass
	gSoundsWood,
	gSoundsMetal,
	gSoundsFlesh,
	gSoundsConcrete,
	gSoundsCeiling,
	gSoundsGlass,
	gSoundsGlass,
	gSoundsConcrete,
	(const char **)nullptr,// matNone
	(const char **)nullptr// matLastMaterial
};

// Gibs models
// sorted by Materials struct
const char *gBreakModels[] = {
	"models/glassgibs.mdl",// matGlass
	"models/woodgibs.mdl",
	"models/metalplategibs.mdl",
	"models/fleshgibs.mdl",
	"models/cindergibs.mdl",
	"models/ceilinggibs.mdl",
	"models/computergibs.mdl",
	(const char *)nullptr,// matUnbreakableGlass,
	"models/rockgibs.mdl",
	(const char *)nullptr,// matNone
	(const char *)nullptr// matLastMaterial
};

// NUM_STEP_SOUNDS
const char *gStepSoundsDefault[] =
{
	"player/pl_step1.wav",
	"player/pl_step3.wav",
	"player/pl_step2.wav",
	"player/pl_step4.wav",
	nullptr
};
const char *gStepSoundsMetal[] =
{
	"player/pl_metal1.wav",
	"player/pl_metal3.wav",
	"player/pl_metal2.wav",
	"player/pl_metal4.wav",
	nullptr
};
const char *gStepSoundsDirt[] =
{
	"player/pl_dirt1.wav",
	"player/pl_dirt3.wav",
	"player/pl_dirt2.wav",
	"player/pl_dirt4.wav",
	nullptr
};
const char *gStepSoundsDuct[] =
{
	"player/pl_duct1.wav",
	"player/pl_duct3.wav",
	"player/pl_duct2.wav",
	"player/pl_duct4.wav",
	nullptr
};
const char *gStepSoundsGrate[] =
{
	"player/pl_grate1.wav",
	"player/pl_grate3.wav",
	"player/pl_grate2.wav",
	"player/pl_grate4.wav",
	nullptr
};
const char *gStepSoundsTile[] =
{
	"player/pl_tile1.wav",
	"player/pl_tile3.wav",
	"player/pl_tile2.wav",
	"player/pl_tile4.wav",
	nullptr
};
const char *gStepSoundsSlosh[] =
{
	"player/pl_slosh1.wav",
	"player/pl_slosh3.wav",
	"player/pl_slosh2.wav",
	"player/pl_slosh4.wav",
	nullptr
};
const char *gStepSoundsWade[] =
{
	"player/pl_wade1.wav",
	"player/pl_wade3.wav",
	"player/pl_wade2.wav",
	"player/pl_wade4.wav",
	nullptr
};
const char *gStepSoundsLadder[] =
{
	"player/pl_ladder1.wav",
	"player/pl_ladder3.wav",
	"player/pl_ladder2.wav",
	"player/pl_ladder4.wav",
	nullptr
};
const char *gStepSoundsWood[] =
{
	"player/pl_wood1.wav",
	"player/pl_wood3.wav",
	"player/pl_wood2.wav",
	"player/pl_wood4.wav",
	nullptr
};
const char *gStepSoundsSnow[] =
{
	"player/pl_snow1.wav",
	"player/pl_snow3.wav",
	"player/pl_snow2.wav",
	"player/pl_snow4.wav",
	nullptr
};
const char *gStepSoundsGrass[] =
{
	"player/pl_grass1.wav",
	"player/pl_grass3.wav",
	"player/pl_grass2.wav",
	"player/pl_grass4.wav",
	nullptr
};
// index overflow catch zone
const char *gStepSoundsNull[] =
{
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

// WARNING! All array indexes must be valid STEP_ IDs!
/*static */const char **gStepSounds[] = {
	(const char **)nullptr,// STEP_NONE == 0
	gStepSoundsDefault,// STEP_CONCRETE == 1
	gStepSoundsMetal,
	gStepSoundsDirt,
	gStepSoundsDuct,
	gStepSoundsGrate,
	gStepSoundsTile,
	gStepSoundsSlosh,
	gStepSoundsWade,
	gStepSoundsLadder,
	gStepSoundsWood,
	gStepSoundsSnow,
	gStepSoundsGrass,
	gStepSoundsNull,
	(const char **)nullptr
};

// Relative values that affect velocity
// sorted by Materials struct
float gMaterialWeight[] = {
	1.1,//Glass = 0,
	0.4,//Wood,
	1.0,//Metal,
	0.5,//Flesh,
	0.8,//CinderBlock,
	0.1,//CeilingTile,
	0.9,//Computer,
	1.2,//UnbreakableGlass,
	0.9,//Rocks,
	0.0,//None
	0.0//LastMaterial
};

// XDM3037: indexes are Materials
// Not a very good idea, since these masks don't define damage multipliers :(
/*const int gMaterialDamageMasks[] = {
	DMGM_BREAK,// matGlass = 0,
	DMGM_BREAK|DMGM_FIRE,// matWood,
	DMGM_BREAK,// matMetal,
	DMGM_BREAK|DMGM_FIRE|DMGM_COLD|DMGM_POISON|DMGM_BLEED|DMG_RADIATION,// matFlesh,
	DMGM_BREAK,// matCinderBlock,
	DMGM_BREAK|DMGM_FIRE,// matCeilingTile,
	DMGM_BREAK|DMG_SHOCK,// matComputer,
	0,// matUnbreakableGlass,
	DMGM_BREAK,// matRocks,
	0,// matNone,
	0// matLastMaterial
};*/

/*
typedef struct
{
	int material;
	const char **sounds;
} stepsounds_t;

stepsounds_t gStepSounds[] = {
	{STEP_CONCRETE,		gStepSoundsDefault},
	{STEP_METAL,		gStepSoundsMetal},
	{STEP_DIRT,			gStepSoundsDirt},
	{STEP_VENT,			gStepSoundsDuct},
	{STEP_GRATE,		gStepSoundsGrate},
	{STEP_TILE,			gStepSoundsTile},
	{STEP_SLOSH,		gStepSoundsSlosh},
	{STEP_WADE,			gStepSoundsWade},
	{STEP_LADDER,		gStepSoundsLadder},
	{STEP_WOOD,			gStepSoundsWood},
	{STEP_SNOW,			gStepSoundsSnow},
	{STEP_GRASS,		gStepSoundsGrass}
};*/


// this table MUST include MAXIMUM amount of possible materials
// Materials must be unique! No duplicate entries allowed except sounds!
// undone: matUnbreakableGlass?
material_t gMaterials[] = 
{
	// texture_id		step_id			breakmat_id		ShardSounds			BreakSounds				PushSounds				StepSounds
	{CHAR_TEX_CONCRETE,	STEP_CONCRETE,	matCinderBlock,	gSoundsConcrete,	gBreakSoundsConcrete,	gPushSoundsConcrete,	gStepSoundsDefault	},
	{CHAR_TEX_METAL,	STEP_METAL,		matMetal,		gSoundsMetal,		gBreakSoundsMetal,		gPushSoundsMetal,		gStepSoundsMetal	},
	{CHAR_TEX_DIRT,		STEP_DIRT,		matRocks,		gSoundsConcrete,	gBreakSoundsConcrete,	gPushSoundsConcrete,	gStepSoundsDirt		},
	{CHAR_TEX_VENT,		STEP_VENT,		matNone,		gSoundsMetal,		gBreakSoundsMetal,		gPushSoundsMetal,		gStepSoundsDuct		},
	{CHAR_TEX_GRATE,	STEP_GRATE,		matNone,		gSoundsMetal,		gBreakSoundsMetal,		gPushSoundsMetal,		gStepSoundsGrate	},
	{CHAR_TEX_TILE,		STEP_TILE,		matNone,		gSoundsConcrete,	gBreakSoundsConcrete,	gPushSoundsConcrete,	gStepSoundsTile		},
	{CHAR_TEX_SLOSH,	STEP_SLOSH,		matNone,		gSoundsFlesh,		gBreakSoundsFlesh,		gPushSoundsFlesh,		gStepSoundsSlosh	},
	{CHAR_TEX_WOOD,		STEP_WOOD,		matWood,		gSoundsWood,		gBreakSoundsWood,		gPushSoundsWood,		gStepSoundsWood		},
	{CHAR_TEX_COMPUTER,	STEP_METAL,		matComputer,	gSoundsMetal,		gBreakSoundsMetal,		gPushSoundsMetal,		gStepSoundsMetal	},
	{CHAR_TEX_GLASS,	STEP_CONCRETE,	matGlass,		gSoundsGlass,		gBreakSoundsGlass,		gPushSoundsGlass,		gStepSoundsDefault	},
	{CHAR_TEX_FLESH,	STEP_SLOSH,		matFlesh,		gSoundsFlesh,		gBreakSoundsFlesh,		gPushSoundsFlesh,		gStepSoundsSlosh	},
	{CHAR_TEX_SNOW,		STEP_SNOW,		matNone,		gSoundsFlesh,		gBreakSoundsFlesh,		gPushSoundsConcrete,	gStepSoundsSnow		},
	{CHAR_TEX_GRASS,	STEP_GRASS,		matNone,		gSoundsCeiling,		gBreakSoundsCeiling,	gPushSoundsConcrete,	gStepSoundsGrass	},
	{CHAR_TEX_CEILING,	STEP_WOOD,		matCeilingTile,	gSoundsCeiling,		gBreakSoundsCeiling,	gPushSoundsConcrete,	gStepSoundsWood		},
	{CHAR_TEX_WATER,	STEP_WADE,		matNone,		gSoundsFlesh,		gBreakSoundsFlesh,		gPushSoundsFlesh,		gStepSoundsWade		},
	{CHAR_TEX_SKY,		STEP_NONE,		matNone,		nullptr,			nullptr,				nullptr,				nullptr				},// XDM3038
	{CHAR_TEX_NULL,		STEP_NONE,		matNone,		nullptr,			nullptr,				nullptr,				nullptr				}
};

// UNDONE Useless. We want something like (gTextureSounds[CHAR_TEX_METAL].fVolWeapon), but those indexes...
/*texturesound_t gTextureSounds[] =
{
	CHAR_TEX_NULL		gSoundsConcrete
	CHAR_TEX_CONCRETE	
	CHAR_TEX_METAL		
	CHAR_TEX_DIRT		
	CHAR_TEX_VENT		
	CHAR_TEX_GRATE		
	CHAR_TEX_TILE		
	CHAR_TEX_SLOSH		
	CHAR_TEX_WOOD		
	CHAR_TEX_COMPUTER	
	CHAR_TEX_GLASS		
	CHAR_TEX_FLESH		
	CHAR_TEX_SNOW		
	CHAR_TEX_GRASS		
	CHAR_TEX_CEILING	
	CHAR_TEX_WATER		
};*/

//-----------------------------------------------------------------------------
// Purpose: simple selector
// Input  : &chTextureType - CHAR_TEX_
// Output : int - STEP_
//-----------------------------------------------------------------------------
int MapTextureTypeStepType(const char &chTextureType)
{
/* slower
	for (int i=0; i<NUM_MATERIALS; ++i)
		if (gMaterials[i].texture_id == chTextureType)
			return gMaterials[i].step_id;
*/
	switch (chTextureType)// most used types should go first
	{
		default:
		case CHAR_TEX_GLASS:
		case CHAR_TEX_CONCRETE:	return STEP_CONCRETE; break;
		case CHAR_TEX_COMPUTER:
		case CHAR_TEX_METAL:	return STEP_METAL; break;
		case CHAR_TEX_DIRT:		return STEP_DIRT; break;
		case CHAR_TEX_VENT:		return STEP_VENT; break;
		case CHAR_TEX_GRATE:	return STEP_GRATE; break;
		case CHAR_TEX_TILE:		return STEP_TILE; break;
		case CHAR_TEX_FLESH:
		case CHAR_TEX_SLOSH:	return STEP_SLOSH; break;
		case CHAR_TEX_WOOD:		return STEP_WOOD; break;
		case CHAR_TEX_SNOW:		return STEP_SNOW; break;
		case CHAR_TEX_GRASS:	return STEP_GRASS; break;
		case CHAR_TEX_SKY:
		case CHAR_TEX_NULL:		return STEP_NONE; break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: get hit sound array and parameters
// Input  : &chTextureType - 
//			breakable - type of sounds to use
// Output : &fvol - volume to play texture sound at
//			&fvolbar - volume to play weapon sound at
//			&fattn - texture sound attenuation
//			**pSoundArray - texture sounds array
//			&arraysize - array size
//-----------------------------------------------------------------------------
void TextureTypeGetSounds(const char &chTextureType, bool breakable, float &fvol, float &fvolbar, float &fattn, const char **pSoundArray[], int &arraysize)
{
	if (breakable)// convenient default values
		arraysize = NUM_SHARD_SOUNDS;
	else
		arraysize = NUM_STEP_SOUNDS;

	switch (chTextureType)
	{
	default:// use concrete sounds
	case CHAR_TEX_CONCRETE:
		{
			fvol = 0.9;	fvolbar = 0.6;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsConcrete;
			else
				*pSoundArray = gStepSoundsDefault;
		}
		break;
	case CHAR_TEX_METAL:
		{
			fvol = 0.9; fvolbar = 0.3;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsMetal;
			else
				*pSoundArray = gStepSoundsMetal;
		}
		break;
	case CHAR_TEX_DIRT:
		{
			fvol = 0.9; fvolbar = 0.1;
			fattn = ATTN_STATIC;
			if (breakable)
			{
				*pSoundArray = gStepSoundsDirt;
				arraysize = NUM_STEP_SOUNDS;
			}
			else
				*pSoundArray = gStepSoundsDirt;
		}
		break;
	case CHAR_TEX_VENT:
		{
			fvol = 0.5; fvolbar = 0.3;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsMetal;
			else
				*pSoundArray = gStepSoundsDuct;
		}
		break;
	case CHAR_TEX_GRATE:
		{
			fvol = 0.9; fvolbar = 0.5;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsMetal;
			else
				*pSoundArray = gStepSoundsGrate;
		}
		break;
	case CHAR_TEX_TILE:
		{
			fvol = 0.8; fvolbar = 0.2;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsCeiling;
			else
				*pSoundArray = gStepSoundsTile;
		}
		break;
	case CHAR_TEX_SLOSH:
		{
			fvol = 0.9; fvolbar = 0.0;
			fattn = ATTN_STATIC;
			*pSoundArray = gStepSoundsSlosh;
			arraysize = NUM_STEP_SOUNDS;
		}
		break;
	case CHAR_TEX_WOOD:
		{
			fvol = 0.9; fvolbar = 0.2;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsWood;
			else
				*pSoundArray = gStepSoundsWood;
		}
		break;
	case CHAR_TEX_COMPUTER:
		{
			fvol = 0.7; fvolbar = 0.4;
			fattn = ATTN_NORM;
			*pSoundArray = gSoundsGlass;
			arraysize = NUM_SHARD_SOUNDS;
		}
		break;
	case CHAR_TEX_GLASS:
		{
			fvol = 0.8; fvolbar = 0.6;
			fattn = ATTN_NORM;
			if (breakable)
				*pSoundArray = gSoundsGlass;
			else
				*pSoundArray = gStepSoundsDefault;
		}
		break;
	case CHAR_TEX_FLESH:
		{
			fvol = 0.6; fvolbar = 0.0;
			fattn = ATTN_STATIC;
			if (breakable)
				*pSoundArray = gSoundsFlesh;
			else
				*pSoundArray = gStepSoundsSlosh;
		}
		break;
	case CHAR_TEX_SNOW:
		{
			fvol = 0.1; fvolbar = 0.0;
			fattn = ATTN_IDLE;
			if (breakable)
				*pSoundArray = gSoundsFlesh;
			else
				*pSoundArray = gStepSoundsSlosh;
		}
		break;
	case CHAR_TEX_GRASS:
		{
			fvol = 0.3; fvolbar = 0.1;
			fattn = ATTN_IDLE;
			*pSoundArray = gStepSoundsGrass;
			arraysize = NUM_STEP_SOUNDS;
		}
		break;
	case CHAR_TEX_SKY:
	case CHAR_TEX_NULL:
		{
			fvol = 0.0; fvolbar = 0.0;
			fattn = ATTN_IDLE;
			pSoundArray = nullptr;
			arraysize = 0;// XDM3038: disable any sounds for these special texture types
		}
		break;
	}
}
