#if !defined( PM_MATERIALSH )
#define PM_MATERIALSH
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif


typedef enum materials_e
{
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matNone,
	matLastMaterial
} Materials;

typedef enum stepsounds_e
{
	STEP_NONE = 0,
	STEP_CONCRETE,	// default step sound
	STEP_METAL,		// metal floor
	STEP_DIRT,		// dirt, sand, rock
	STEP_VENT,		// ventillation duct
	STEP_GRATE,		// metal grating
	STEP_TILE,		// floor tiles
	STEP_SLOSH,		// shallow liquid puddle
	STEP_WADE,		// wading in liquid
	STEP_LADDER,	// climbing ladder
	STEP_WOOD,		// wood
	STEP_SNOW,		// snow
	STEP_GRASS,		// grass
	STEP_LAST		// LIST TERMINATOR, must be last
} stepsounds_t;

#define MATFLAG_DO_RICOCHET	1

// XDM3038: modified to eliminate padding
typedef struct material_s
{
	int texture_id;// CHAR_TEX_CONCRETE // char
	unsigned int step_id;// stepsound_t STEP_CONCRETE
	unsigned int breakmat_id;// Materials struct
	const char **ShardSounds;
//	int ShardSoundsNum;
	const char **BreakSounds;
//	int BreakSoundsNum;
	const char **PushSounds;
//	int PushSoundsNum;
	const char **StepSounds;
//	int StepSoundsNum;
//	float fVolumeStep;
//	float fVolumeHit;
//	float fVolumeTool;
//	int iDamageAcceptMask;
//	int flags;
} material_t;

/*typedef struct texturesound_s
{
	char **ppSoundsArray;
	float fVolTexture;
	float fVolWeapon;
} texturesound_t;*/

#define CBTEXTURENAMEMAX	16// only load first n chars of name
#define CTEXTURESMAX		1024// XDM3037: max number of textures loaded

#define NUM_MATERIALS		16

// texture types
#define CHAR_TEX_NULL		'0'
#define CHAR_TEX_CONCRETE	'C'
#define CHAR_TEX_METAL		'M'
#define CHAR_TEX_DIRT		'D'
#define CHAR_TEX_VENT		'V'
#define CHAR_TEX_GRATE		'G'
#define CHAR_TEX_TILE		'T'
#define CHAR_TEX_SLOSH		'S'
#define CHAR_TEX_WOOD		'W'
#define CHAR_TEX_COMPUTER	'P'
#define CHAR_TEX_GLASS		'Y'
#define CHAR_TEX_FLESH		'F'
#define CHAR_TEX_SNOW		'N'
#define CHAR_TEX_GRASS		'A'
#define CHAR_TEX_CEILING	'E'
#define CHAR_TEX_WATER		'!'
#define CHAR_TEX_SKY		' '// XDM3038: set to space so noone can mess around with it

#define CHAR_TEX_DEFAULT	CHAR_TEX_CONCRETE

// number of sounds in array for every material
#define NUM_STEP_SOUNDS		4
#define NUM_SHARD_SOUNDS	8// actually it varies
#define NUM_PUSH_SOUNDS		3
#define NUM_BREAK_SOUNDS	2

// sound names
extern const char *gSoundsWood[];
extern const char *gSoundsFlesh[];
extern const char *gSoundsGlass[];
extern const char *gSoundsMetal[];
extern const char *gSoundsConcrete[];
extern const char *gSoundsCeiling[];

#define NUM_SPARK_SOUNDS		6
#define NUM_RICOCHET_SOUNDS		5
#define NUM_BODYDROP_SOUNDS		4

extern const char *gSoundsSparks[];
extern const char *gSoundsRicochet[];
extern const char *gSoundsShell9mm[];
extern const char *gSoundsShellShotgun[];
extern const char *gSoundsDropBody[];

extern const char *gPushSoundsGlass[];
extern const char *gPushSoundsWood[];
extern const char *gPushSoundsMetal[];
extern const char *gPushSoundsFlesh[];
extern const char *gPushSoundsConcrete[];

extern const char *gBreakSoundsGlass[];
extern const char *gBreakSoundsWood[];
extern const char *gBreakSoundsMetal[];
extern const char *gBreakSoundsFlesh[];
extern const char *gBreakSoundsConcrete[];
extern const char *gBreakSoundsCeiling[];

extern const char *gStepSoundsDefault[];
extern const char *gStepSoundsMetal[];
extern const char *gStepSoundsDirt[];
extern const char *gStepSoundsDuct[];
extern const char *gStepSoundsGrate[];
extern const char *gStepSoundsTile[];
extern const char *gStepSoundsSlosh[];
extern const char *gStepSoundsWade[];
extern const char *gStepSoundsLadder[];
extern const char *gStepSoundsWood[];
extern const char *gStepSoundsSnow[];
extern const char *gStepSoundsGrass[];

extern const char **gStepSounds[];// all of above
extern const char **gPushSounds[];
extern const char **gBreakSounds[];
extern const char **gShardSounds[];// XDM3038c

extern const char *gBreakModels[];// XDM3038c

extern material_t gMaterials[];

extern float gMaterialWeight[];// XDM3038c

int MapTextureTypeStepType(const char &chTextureType);
void TextureTypeGetSounds(const char &chTextureType, bool breakable, float &fvol, float &fvolbar, float &fattn, const char **pSoundArray[], int &arraysize);

#endif // !PM_MATERIALSH
