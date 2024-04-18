#if defined(CLIENT_DLL)
#include "hud.h"
#include "cl_util.h"
#include "r_efx.h"
#else
#include "extdll.h"
#include "util.h"
#endif
#include "com_model.h"
#include "decals.h"
#include "shared_resources.h"

// Used when unable to load proper resource
char g_szDefaultStudioModel[] = "models/testsphere.mdl";
char g_szDefaultSprite[] = "sprites/white.spr";
char g_szDefaultGibsHuman[] = "models/hgibs.mdl";
char g_szDefaultGibsAlien[] = "models/agibs.mdl";

DLL_GLOBAL int g_iModelIndexTestSphere = 0;
DLL_GLOBAL int g_iModelIndexAGibs = 0;
DLL_GLOBAL int g_iModelIndexHGibs = 0;
DLL_GLOBAL int g_iModelIndexGlassGibs = 0;
// XDM: common sprite indexes
DLL_GLOBAL int g_iModelIndexAcidDrip = 0;// acid_*.spr
//DLL_GLOBAL int g_iModelIndexAcidPuff1 = 0;
//DLL_GLOBAL int g_iModelIndexAcidPuff2 = 0;
//DLL_GLOBAL int g_iModelIndexAcidPuff3 = 0;
//DLL_GLOBAL int g_iModelIndexAcidSplash = 0;
DLL_GLOBAL int g_iModelIndexAnimglow01 = 0;
DLL_GLOBAL int g_iModelIndexBallSmoke = 0;
DLL_GLOBAL int g_iModelIndexBigExplo1 = 0;
DLL_GLOBAL int g_iModelIndexBigExplo2 = 0;
DLL_GLOBAL int g_iModelIndexBigExplo3 = 0;
DLL_GLOBAL int g_iModelIndexBigExplo4 = 0;
DLL_GLOBAL int g_iModelIndexBloodDrop = 0;// holds the sprite index for the initial blood
DLL_GLOBAL int g_iModelIndexBloodSpray = 0;// holds the sprite index for splattered blood
//DLL_GLOBAL int g_iModelIndexBubble = 0;// holds the index for the bubbles model
DLL_GLOBAL int g_iModelIndexColdball1 = 0;
DLL_GLOBAL int g_iModelIndexColdball2 = 0;
DLL_GLOBAL int g_iModelIndexFire = 0;
DLL_GLOBAL int g_iModelIndexFireball = 0;// holds the index for the fireball
DLL_GLOBAL int g_iModelIndexFlameFire = 0;
DLL_GLOBAL int g_iModelIndexFlameFire2 = 0;
//DLL_GLOBAL int g_iModelIndexGravFX = 0;// XDM3035
DLL_GLOBAL int g_iModelIndexLaser = 0;// holds the index for the laser beam
//DLL_GLOBAL int g_iModelIndexMuzzleFlash0 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash1 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash2 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash3 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash4 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash5 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash6 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash7 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash8 = 0;
//DLL_GLOBAL int g_iModelIndexMuzzleFlash9 = 0;
//DLL_GLOBAL int g_iModelIndexNucExplode = 0;
//DLL_GLOBAL int g_iModelIndexNucExp1 = 0;
//DLL_GLOBAL int g_iModelIndexNucExp2 = 0;
DLL_GLOBAL int g_iModelIndexUExplo = 0;
DLL_GLOBAL int g_iModelIndexNucFX = 0;
DLL_GLOBAL int g_iModelIndexNucRing = 0;
//DLL_GLOBAL int g_iModelIndexPExp1 = 0;
//DLL_GLOBAL int g_iModelIndexPExp2 = 0;
//DLL_GLOBAL int g_iModelIndexPTracer = 0;
DLL_GLOBAL int g_iModelIndexShockWave = 0;
DLL_GLOBAL int g_iModelIndexSmkball = 0;
DLL_GLOBAL int g_iModelIndexSmoke = 0;// holds the index for the smoke cloud
//DLL_GLOBAL int g_iModelIndexTeleFlash = 0;
//DLL_GLOBAL int g_iModelIndexWallPuff1 = 0;
//DLL_GLOBAL int g_iModelIndexWallPuff2 = 0;
//DLL_GLOBAL int g_iModelIndexWallPuff3 = 0;
DLL_GLOBAL int g_iModelIndexWarpGlow1 = 0;
DLL_GLOBAL int g_iModelIndexWarpGlow2 = 0;
DLL_GLOBAL int g_iModelIndexWExplosion = 0;// holds the index for the underwater explosion
DLL_GLOBAL int g_iModelIndexWExplosion2 = 0;
DLL_GLOBAL int g_iModelIndexWhite = 0;
DLL_GLOBAL int g_iModelIndexZeroFire = 0;
//DLL_GLOBAL int g_iModelIndexZeroFlare = 0;
//DLL_GLOBAL int g_iModelIndexZeroGlow = 0;
DLL_GLOBAL int g_iModelIndexZeroParts = 0;
//DLL_GLOBAL int g_iModelIndexZeroSteam = 0;// holds the index for the smokeball

shared_resource_t g_SharedResourcesList[] =
{
	{g_szDefaultStudioModel,		&g_iModelIndexTestSphere,		t_model },
	{g_szDefaultSprite,				&g_iModelIndexWhite,			t_model },
	{g_szDefaultGibsAlien,			&g_iModelIndexAGibs,			t_model },// TODO: get rid of this
	{g_szDefaultGibsHuman,			&g_iModelIndexHGibs,			t_model },
	{"models/glassgibs.mdl",		&g_iModelIndexGlassGibs,		t_model },
	{"sprites/acid_drip.spr",		&g_iModelIndexAcidDrip,			t_model },
//	{"sprites/acid_puff1.spr",		&g_iModelIndexAcidPuff1,		t_model },
//	{"sprites/acid_puff2.spr",		&g_iModelIndexAcidPuff2,		t_model },
//	{"sprites/acid_puff3.spr",		&g_iModelIndexAcidPuff3,		t_model },
//	{"sprites/acid_splash.spr",		&g_iModelIndexAcidSplash,		t_model },
	{"sprites/animglow01.spr",		&g_iModelIndexAnimglow01,		t_model },
	{"sprites/ballsmoke.spr",		&g_iModelIndexBallSmoke,		t_model },
	{"sprites/zerogxplode1.spr",	&g_iModelIndexBigExplo1,		t_model },
	{"sprites/zerogxplode2.spr",	&g_iModelIndexBigExplo2,		t_model },
	{"sprites/zerogxplode3.spr",	&g_iModelIndexBigExplo3,		t_model },
	{"sprites/zerogxplode4.spr",	&g_iModelIndexBigExplo4,		t_model },
	{"sprites/blood.spr",			&g_iModelIndexBloodDrop,		t_model },
	{"sprites/bloodspray.spr",		&g_iModelIndexBloodSpray,		t_model },
//	{"sprites/bubble.spr",			&g_iModelIndexBubble,			t_model },
	{"sprites/iceball1.spr",		&g_iModelIndexColdball1,		t_model },// for glow
	{"sprites/iceball2.spr",		&g_iModelIndexColdball2,		t_model },// for funnel effect
	{"sprites/fire.spr",			&g_iModelIndexFire,				t_model },
	{"sprites/zerogxplode.spr",		&g_iModelIndexFireball,			t_model },
	{"sprites/flamefire.spr",		&g_iModelIndexFlameFire,		t_model },
	{"sprites/flamefire2.spr",		&g_iModelIndexFlameFire2,		t_model },
//	{"sprites/darkportal.spr",		&g_iModelIndexGravFX,			t_model },
	{"sprites/laserbeam.spr",		&g_iModelIndexLaser,			t_model },
//	{"sprites/muzzleflash0.spr",	&g_iModelIndexMuzzleFlash0,		t_model },
//	{"sprites/muzzleflash1.spr",	&g_iModelIndexMuzzleFlash1,		t_model },
//	{"sprites/muzzleflash2.spr",	&g_iModelIndexMuzzleFlash2,		t_model },
//	{"sprites/muzzleflash3.spr",	&g_iModelIndexMuzzleFlash3,		t_model },
//	{"sprites/muzzleflash4.spr",	&g_iModelIndexMuzzleFlash4,		t_model },
//	{"sprites/muzzleflash5.spr",	&g_iModelIndexMuzzleFlash5,		t_model },
//	{"sprites/muzzleflash6.spr",	&g_iModelIndexMuzzleFlash6,		t_model },
//	{"sprites/muzzleflash7.spr",	&g_iModelIndexMuzzleFlash7,		t_model },
//	{"sprites/muzzleflash8.spr",	&g_iModelIndexMuzzleFlash8,		t_model },
//	{"sprites/muzzleflash9.spr",	&g_iModelIndexMuzzleFlash9,		t_model },
//	{"sprites/nuc_blow.spr",		&g_iModelIndexNucExplode,		t_model },
//	{"sprites/fexplo.spr",			&g_iModelIndexNucExp1,			t_model },
//	{"sprites/gexplo.spr",			&g_iModelIndexNucExp2,			t_model },
	{"sprites/uexplo.spr",			&g_iModelIndexUExplo,			t_model },
	{"sprites/nuc_fx.spr",			&g_iModelIndexNucFX,			t_model },
	{"sprites/nuc_ring.spr",		&g_iModelIndexNucRing,			t_model },
//	{"sprites/p_exp1.spr",			&g_iModelIndexPExp1,			t_model },
//	{"sprites/p_exp2.spr",			&g_iModelIndexPExp2,			t_model },
//	{"sprites/p_tracer.spr",		&g_iModelIndexPTracer,			t_model },
	{"sprites/shockwave.spr",		&g_iModelIndexShockWave,		t_model },
	{"sprites/smkball.spr",			&g_iModelIndexSmkball,			t_model },
	{"sprites/steam1.spr",			&g_iModelIndexSmoke,			t_model },
//	{"sprites/teleflash.spr",		&g_iModelIndexTeleFlash,		t_model },
//	{"sprites/wallpuff1.spr",		&g_iModelIndexWallPuff1,		t_model },
//	{"sprites/wallpuff2.spr",		&g_iModelIndexWallPuff2,		t_model },
//	{"sprites/wallpuff3.spr",		&g_iModelIndexWallPuff3,		t_model },
//	{"sprites/warp_glow1.spr",		&g_iModelIndexWarpGlow1,		t_model },
//	{"sprites/warp_glow2.spr",		&g_iModelIndexWarpGlow2,		t_model },
	{"sprites/zerowxplode.spr",		&g_iModelIndexWExplosion,		t_model },
	{"sprites/zerowxplode2.spr",	&g_iModelIndexWExplosion2,		t_model },
	{"sprites/zerofire.spr",		&g_iModelIndexZeroFire,			t_model },
//	{"sprites/zeroflare.spr",		&g_iModelIndexZeroFlare,		t_model },
//	{"sprites/zeroglow.spr",		&g_iModelIndexZeroGlow,			t_model },
	{"sprites/zeroparts.spr",		&g_iModelIndexZeroParts,		t_model },
//	{"sprites/zerosteam.spr",		&g_iModelIndexZeroSteam,		t_model },
	{"",							nullptr,						0		}// good for termination, but not for ARRAYSIZE!
};

//
// This list must be same in both server and client DLLs
//
DLL_DECALLIST g_Decals[] = {// [DECAL_ENUM_SIZE]
	{ "{acid_splat1", 0 },	// DECAL_ACID_SPLAT1
	{ "{acid_splat2", 0 },	// DECAL_ACID_SPLAT2
	{ "{acid_splat3", 0 },	// DECAL_ACID_SPLAT3
	{ "{shot1",	0 },		// DECAL_GUNSHOT1
	{ "{shot2",	0 },		// DECAL_GUNSHOT2
	{ "{shot3",	0 },		// DECAL_GUNSHOT3
	{ "{shot4",	0 },		// DECAL_GUNSHOT4
	{ "{shot5",	0 },		// DECAL_GUNSHOT5
	{ "{largeshot1", 0 },	// DECAL_LARGESHOT1
	{ "{largeshot2", 0 },	// DECAL_LARGESHOT2
	{ "{largeshot3", 0 },	// DECAL_LARGESHOT3
	{ "{largeshot4", 0 },	// DECAL_LARGESHOT4
	{ "{largeshot5", 0 },	// DECAL_LARGESHOT5
	{ "{lambda01", 0 },		// DECAL_LAMBDA1
	{ "{lambda02", 0 },		// DECAL_LAMBDA2
	{ "{lambda03", 0 },		// DECAL_LAMBDA3
	{ "{lambda04", 0 },		// DECAL_LAMBDA4
	{ "{lambda05", 0 },		// DECAL_LAMBDA5
	{ "{lambda06", 0 },		// DECAL_LAMBDA6
	{ "{scorch1", 0 },		// DECAL_SCORCH1
	{ "{scorch2", 0 },		// DECAL_SCORCH2
	{ "{scorch3", 0 },		// DECAL_SCORCH3
	{ "{blood1", 0 },		// DECAL_BLOOD1
	{ "{blood2", 0 },		// DECAL_BLOOD2
	{ "{blood3", 0 },		// DECAL_BLOOD3
	{ "{blood4", 0 },		// DECAL_BLOOD4
	{ "{blood5", 0 },		// DECAL_BLOOD5
	{ "{blood6", 0 },		// DECAL_BLOOD6
	{ "{yblood1", 0 },		// DECAL_YBLOOD1
	{ "{yblood2", 0 },		// DECAL_YBLOOD2
	{ "{yblood3", 0 },		// DECAL_YBLOOD3
	{ "{yblood4", 0 },		// DECAL_YBLOOD4
	{ "{yblood5", 0 },		// DECAL_YBLOOD5
	{ "{yblood6", 0 },		// DECAL_YBLOOD6
	{ "{break1", 0 },		// DECAL_GLASSBREAK1
	{ "{break2", 0 },		// DECAL_GLASSBREAK2
	{ "{break3", 0 },		// DECAL_GLASSBREAK3
	{ "{bigshot1", 0 },		// DECAL_BIGSHOT1
	{ "{bigshot2", 0 },		// DECAL_BIGSHOT2
	{ "{bigshot3", 0 },		// DECAL_BIGSHOT3
	{ "{bigshot4", 0 },		// DECAL_BIGSHOT4
	{ "{bigshot5", 0 },		// DECAL_BIGSHOT5
	{ "{gaussshot1", 0 },	// DECAL_GAUSSSHOT1
	{ "{spit1", 0 },		// DECAL_SPIT1
	{ "{spit2", 0 },		// DECAL_SPIT2
	{ "{splat7", 0 },		// DECAL_SPLAT7
	{ "{splat8", 0 },		// DECAL_SPLAT8
	{ "{bigsplatg1", 0 },	// DECAL_BIGSPLATG1
	{ "{bproof1", 0 },		// DECAL_BPROOF1
	{ "{gargstomp", 0 },	// DECAL_GARGSTOMP1
	{ "{smscorch1", 0 },	// DECAL_SMALLSCORCH1
	{ "{smscorch2", 0 },	// DECAL_SMALLSCORCH2
	{ "{smscorch3", 0 },	// DECAL_SMALLSCORCH3
	{ "{mommablob", 0 },	// DECAL_MOMMABIRTH
	{ "{mommasplat", 0 },	// DECAL_MOMMASPLAT
	{ "{nucblow1", 0 },		// DECAL_NUCBLOW1
	{ "{nucblow1", 0 },		// DECAL_NUCBLOW2
	{ "{nucblow1", 0 },		// DECAL_NUCBLOW3
	{ "{woodbreak1", 0 },	// DECAL_WOODBREAK1
	{ "{woodbreak2", 0 },	// DECAL_WOODBREAK2
	{ "{woodbreak3", 0 },	// DECAL_WOODBREAK3
	{ "{xhl", 0 },			// DECAL_XHL
	{ "{bblood1", 0 },		// DECAL_BBLOOD1
	{ "{bblood2", 0 },		// DECAL_BBLOOD2
	{ "{bblood3", 0 },		// DECAL_BBLOOD3
	{ "{gblood1", 0 },		// DECAL_GBLOOD1
	{ "{gblood2", 0 },		// DECAL_GBLOOD2
	{ "{gblood3", 0 },		// DECAL_GBLOOD3
	{ "{mdscorch1", 0 },	// DECAL_MDSCORCH1
	{ "{mdscorch2", 0 },	// DECAL_MDSCORCH2
	{ "{mdscorch3", 0 },	// DECAL_MDSCORCH3
	{ "{biohaz", 0 },		// DECAL_BIOHAZ
	{ "{bloodsmearr1", 0 },	// DECAL_BLOODSMEARR1
	{ "{bloodsmearr2", 0 },	// DECAL_BLOODSMEARR2
	{ "{bloodsmearr3", 0 },	// DECAL_BLOODSMEARR3
	{ "{bloodsmeary1", 0 },	// DECAL_BLOODSMEARY1
	{ "{bloodsmeary2", 0 },	// DECAL_BLOODSMEARY2
	{ "{bloodsmeary3", 0 },	// DECAL_BLOODSMEARY3
	{ "{blow", 0 },			// DECAL_BLOW
	{ "", 0 },				// terminator (not used for iteration now because we have DECAL_ENUM_SIZE)
};

#include "event_api.h"

//-----------------------------------------------------------------------------
// Purpose: Precache and cet indexes for fast use of common resources.
// Warning: Avoid using ARRAYSIZE()!
// NOTE: These must have been precached on server! Otherwise CL_LoadModel() -1
//-----------------------------------------------------------------------------
void PrecacheSharedResources(void)
{
	conprintf(0, "Precaching shared resources...\n");
	int i = 0;
#if defined(CLIENT_DLL)
	for (i = 0; g_SharedResourcesList[i].index != nullptr; ++i)//for (i = 0; i < ARRAYSIZE(g_SharedResourcesList); ++i)
	{
		if (g_SharedResourcesList[i].type == t_model)
		{
			/*if (g_SharedResourcesList[i].client_only > 0 && UTIL_FileExtensionIs(g_SharedResourcesList[i].filename, ".spr"))
			{
				const model_t *pSprite = gEngfuncs.GetSpritePointer(SPR_Load(g_SharedResourcesList[i].filename));
				if (pSprite || test1->value > 0)
					*g_SharedResourcesList[i].index = gEngfuncs.pEventAPI->EV_FindModelIndex(g_SharedResourcesList[i].filename);
			}
			else*/
				gEngfuncs.CL_LoadModel(g_SharedResourcesList[i].filename, g_SharedResourcesList[i].index);
				//Mod_ForName()->index? not needed for SERVER-PRECACHED resources
		}
		else if (g_SharedResourcesList[i].type == t_decal)
			*g_SharedResourcesList[i].index = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(g_SharedResourcesList[i].filename);
		else if (g_SharedResourcesList[i].type == t_sound)
			if (g_SharedResourcesList[i].index)
				*g_SharedResourcesList[i].index = 0;// no GET for client sound indexes available :(

		if (g_SharedResourcesList[i].index == 0)
			conprintf(0, "CL: Warning! resource \"%s\" not loaded!\n", g_SharedResourcesList[i].filename);
	}		

	//g_StudioRenderer.m_pIndicatorSprite = gEngfuncs.CL_LoadModel("sprites/iplayer.spr", nullptr);//&i);
	for (i = 0; i < DECAL_ENUM_SIZE; ++i)//ARRAYSIZE(g_Decals); ++i)
	{
		g_Decals[i].index = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(g_Decals[i].name);// this is a DECAL idex, not TEXTURE!
		if (g_Decals[i].index <= 0)
			conprintf(1, "CL: unable to load decal \"%s\"\n", g_Decals[i].name);
	}
#else// Now same for the server DLL
	for (i = 0; g_SharedResourcesList[i].index != nullptr; ++i)//for (i = 0; i < ARRAYSIZE(g_SharedResourcesList); ++i)
	{
		//if (g_SharedResourcesList[i].client_only > 0)
		//	continue;

		if (g_SharedResourcesList[i].type == t_model)
			*g_SharedResourcesList[i].index = PRECACHE_MODEL(g_SharedResourcesList[i].filename);
		else if (g_SharedResourcesList[i].type == t_decal)
			*g_SharedResourcesList[i].index = DECAL_INDEX(g_SharedResourcesList[i].filename);
		else if (g_SharedResourcesList[i].type == t_sound)
			*g_SharedResourcesList[i].index = PRECACHE_SOUND(g_SharedResourcesList[i].filename);

		if (g_SharedResourcesList[i].index == 0)
			conprintf(0, "SV: Warning! resource \"%s\" not loaded!\n", g_SharedResourcesList[i].filename);
	}
	for (i = 0; i < DECAL_ENUM_SIZE; ++i)//ARRAYSIZE(g_Decals); ++i)
	{
		g_Decals[i].index = DECAL_INDEX(g_Decals[i].name);
		if (g_Decals[i].index <= 0)
			conprintf(1, "SV: unable to load decal \"%s\"\n", g_Decals[i].name);
	}
#endif
}
