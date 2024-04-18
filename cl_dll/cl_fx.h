#ifndef CL_FX_H
#define CL_FX_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#define NUM_MUZZLEFLASH_SPRITES		10

#define BUBBLE_SCALE	0.1f
#define BUBBLE_SPEED	32.0f
#define BUBBLE_LIFE		10.0f

extern uint32 g_iMsgRS_UID_Postfix;// XDM3038c: HACK

extern struct model_s *g_pSpriteAcidDrip;
extern struct model_s *g_pSpriteAcidPuff1;
extern struct model_s *g_pSpriteAcidPuff2;
extern struct model_s *g_pSpriteAcidPuff3;
extern struct model_s *g_pSpriteAcidSplash;
extern struct model_s *g_pSpriteAnimGlow01;
extern struct model_s *g_pSpriteBloodDrop;
extern struct model_s *g_pSpriteBloodSpray;
extern struct model_s *g_pSpriteBubble;
extern struct model_s *g_pSpriteDarkPortal;
extern struct model_s *g_pSpriteDot;
extern struct model_s *g_pSpriteFExplo;
extern struct model_s *g_pSpriteFlameFire;// XDM3038b
extern struct model_s *g_pSpriteFlameFire2;// XDM3038c
extern struct model_s *g_pSpriteGExplo;
extern struct model_s *g_pSpriteHornetExp;// XDM3038c
extern struct model_s *g_pSpriteIceBall1;// XDM3038b
extern struct model_s *g_pSpriteIceBall2;// XDM3038b
extern struct model_s *g_pPlayerIDTexture;
extern struct model_s *g_pSpriteLaserBeam;
//extern struct model_s *g_pSpriteLightPFlare;
extern struct model_s *g_pSpriteLightPHit;
extern struct model_s *g_pSpriteLightPRing;
extern struct model_s *g_pSpriteMuzzleflash0;
extern struct model_s *g_pSpriteMuzzleflash1;
extern struct model_s *g_pSpriteMuzzleflash2;
extern struct model_s *g_pSpriteMuzzleflash3;
extern struct model_s *g_pSpriteMuzzleflash4;
extern struct model_s *g_pSpriteMuzzleflash5;
extern struct model_s *g_pSpriteMuzzleflash6;
extern struct model_s *g_pSpriteMuzzleflash7;
extern struct model_s *g_pSpriteMuzzleflash8;
extern struct model_s *g_pSpriteMuzzleflash9;
extern struct model_s *g_pSpriteNucBlow;
extern struct model_s *g_pSpritePExp1;
extern struct model_s *g_pSpritePExp2;
extern struct model_s *g_pSpritePGlow01s;
extern struct model_s *g_pSpritePSparks;
extern struct model_s *g_pSpritePTracer;
extern struct model_s *g_pSpritePBallHit;
extern struct model_s *g_pSpriteRicho1;
extern struct model_s *g_pSpriteSmkBall;
extern struct model_s *g_pSpriteSpawnBeam;
extern struct model_s *g_pSpriteTeleFlash;
//extern struct model_s *g_pSpriteUExplo;
extern struct model_s *g_pSpriteVoiceIcon;
extern struct model_s *g_pSpriteWallPuff1;
extern struct model_s *g_pSpriteWallPuff2;
//extern struct model_s *g_pSpriteWallPuff3;
extern struct model_s *g_pSpriteWarpGlow1;
extern struct model_s *g_pSpriteXbowExp;
extern struct model_s *g_pSpriteXbowRic;
extern struct model_s *g_pSpriteZeroFire;
//extern struct model_s *g_pSpriteZeroFire2;
extern struct model_s *g_pSpriteZeroFlare;
extern struct model_s *g_pSpriteZeroGlow;
extern struct model_s *g_pSpriteZeroSteam;
extern struct model_s *g_pSpriteZeroWXplode;

//extern int *g_iMuzzleFlashSprites[];
extern struct model_s **g_pMuzzleFlashSprites[];


struct particle_s *DrawParticle(const Vector &origin, short color, float life);
void ParticlesCustom(const Vector &origin, float rnd_vel, int color, int color_range, size_t number, float life, bool normalize = 0);

void HookFXMessages(void);

struct dlight_s *DynamicLight(const Vector &vecPos, float radius, byte r, byte g, byte b, float life, float decay);
struct dlight_s *EntityLight(const Vector &vecPos, float radius, byte r, byte g, byte b, float life, float decay);
struct dlight_s *CL_UpdateFlashlight(cl_entity_t *pEnt);
//float FX_GetBubbleSpeed(void);
char CL_TEXTURETYPE_Trace(struct pmtrace_s *ptr, const Vector &vecSrc, const Vector &vecEnd);

struct tempent_s *FX_Trail(vec3_t origin, int entindex, unsigned short type, float life);
void FX_SparkShower(const Vector &origin, struct model_s *model, size_t count, const Vector &velocity, bool random, float life);
struct tempent_s *FX_Smoke(const Vector &origin, int spriteindex, float scale, float framerate);

void FX_DecalTrace(int decalindex, struct pmtrace_s *pTrace, const Vector &vecStart);
void FX_DecalTrace(int decalindex, const Vector &start, const Vector &end);
void DecalTrace(int decal, struct pmtrace_s *pTrace, const Vector &vecStart);
//void DecalTrace(char *decalname, struct pmtrace_s *pTrace);
void DecalTrace(int decal, const Vector &start, const Vector &end);
void DecalTrace(char *decalname, const Vector &start, const Vector &end);

TEMPENTITY *FX_TempSprite(const Vector &origin, float scale, struct model_s *model, int rendermode, int renderfx, float a, float life, int flags);

/*RS_INDEX*/uint32 FX_StreakSplash(const Vector &pos, const Vector &dir, color24 color, int count, float velocity, bool gravity = true, bool clip = true, bool bounce = true);//speed, int velocityMin, int velocityMax);
/*RS_INDEX*/uint32 FX_StreakSplash(const Vector &pos, const Vector &dir, byte r, byte g, byte b, int count, float velocity, bool gravity, bool clip, bool bounce);
/*RS_INDEX*/uint32 FX_MuzzleFlashSprite(const Vector &pos, int entindex, short attachment, struct model_s *pSprite, float scale, bool rotation);
/*RS_INDEX*///uint32 FX_Bubbles(const Vector &mins, const Vector &maxs, const Vector &direction, struct model_s *pTexture, int count, float speed);
/*RS_INDEX*/uint32 FX_BubblesPoint(const Vector &center, const Vector &spread, struct model_s *pTexture, int count, float speed);
/*RS_INDEX*/uint32 FX_BubblesSphere(const Vector &center, float fRadiusMin, float fRadiusMax, struct model_s *pTexture, int count, float speed);
/*RS_INDEX*/uint32 FX_BubblesBox(const Vector &center, const Vector &halfbox, struct model_s *pTexture, int count, float speed);
/*RS_INDEX*/uint32 FX_BubblesLine(const Vector &start, const Vector &end, struct model_s *pTexture, int count, float speed);

/*RS_INDEX*/uint32 FX_BloodSpray(const Vector &start, const Vector &direction, struct model_s *pTexture, uint32 numparticles, uint32 color4b, float scale);

/*RS_INDEX*/uint32 FX_DisplacerBallParticles(const Vector &origin, int followentindex, float radius, byte type, float timetolive, byte r, byte g, byte b);

void RenderFog(byte r, byte g, byte b, float fStartDist, float fEndDist, bool updateonly);
void ResetFog(void);

#endif // CL_FX_H
