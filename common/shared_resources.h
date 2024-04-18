//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
//
// Precached global model/sprite/sound indexes should be stored here.
// This cpp/h pair must be shared by server and client DLLs.
//
//-----------------------------------------------------------------------------
#ifndef PRECACHED_RES_H
#define PRECACHED_RES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "custom.h"

typedef struct shared_resource_s
{
	char			*filename;
	int				*index;
	int				type;
} shared_resource_t;

extern int g_iModelIndexTestSphere;

extern int g_iModelIndexAGibs;
extern int g_iModelIndexHGibs;
extern int g_iModelIndexGlassGibs;

extern int g_iModelIndexAcidDrip;
//extern int g_iModelIndexAcidPuff1;
//extern int g_iModelIndexAcidPuff2;
//extern int g_iModelIndexAcidPuff3;
//extern int g_iModelIndexAcidSplash;
extern int g_iModelIndexAnimglow01;
extern int g_iModelIndexBallSmoke;
extern int g_iModelIndexBigExplo1;
extern int g_iModelIndexBigExplo2;
extern int g_iModelIndexBigExplo3;
extern int g_iModelIndexBigExplo4;
extern int g_iModelIndexBloodDrop;
extern int g_iModelIndexBloodSpray;
//extern int g_iModelIndexBubble;
extern int g_iModelIndexColdball1;
extern int g_iModelIndexColdball2;
extern int g_iModelIndexFire;
extern int g_iModelIndexFireball;
extern int g_iModelIndexFlameFire;
extern int g_iModelIndexFlameFire2;
//extern int g_iModelIndexGravFX;
extern int g_iModelIndexLaser;
//extern int g_iModelIndexMuzzleFlash0;
//extern int g_iModelIndexMuzzleFlash1;
//extern int g_iModelIndexMuzzleFlash2;
//extern int g_iModelIndexMuzzleFlash3;
//extern int g_iModelIndexMuzzleFlash4;
//extern int g_iModelIndexMuzzleFlash5;
//extern int g_iModelIndexMuzzleFlash6;
//extern int g_iModelIndexMuzzleFlash7;
//extern int g_iModelIndexMuzzleFlash8;
//extern int g_iModelIndexMuzzleFlash9;
//extern int g_iModelIndexNucExplode;
//extern int g_iModelIndexNucExp1;
//extern int g_iModelIndexNucExp2;
extern int g_iModelIndexUExplo;
extern int g_iModelIndexNucFX;
extern int g_iModelIndexNucRing;
//extern int g_iModelIndexPExp1;
//extern int g_iModelIndexPExp2;
//extern int g_iModelIndexPTracer;
extern int g_iModelIndexShockWave;
extern int g_iModelIndexSmkball;
extern int g_iModelIndexSmoke;
//extern int g_iModelIndexTeleFlash;
//extern int g_iModelIndexWallPuff1;
//extern int g_iModelIndexWallPuff2;
//extern int g_iModelIndexWallPuff3;
extern int g_iModelIndexWarpGlow1;
extern int g_iModelIndexWarpGlow2;
extern int g_iModelIndexWExplosion;
extern int g_iModelIndexWExplosion2;
extern int g_iModelIndexWhite;
extern int g_iModelIndexZeroFire;
//extern int g_iModelIndexZeroFlare;
//extern int g_iModelIndexZeroGlow;
extern int g_iModelIndexZeroParts;
//extern int g_iModelIndexZeroSteam;

extern shared_resource_t g_SharedResourcesList[];

void PrecacheSharedResources(void);

extern char g_szDefaultStudioModel[];
extern char g_szDefaultSprite[];
extern char g_szDefaultGibsHuman[];
extern char g_szDefaultGibsAlien[];

#endif // PRECACHED_RES_H
