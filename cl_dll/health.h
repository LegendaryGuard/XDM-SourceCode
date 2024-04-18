#ifndef HEALTH_H
#define HEALTH_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "damage.h"

#define PAIN_SPRITENAME				"sprites/%d_pain.spr"
#define DAMAGE_SPRITENAME			"sprites/%d_dmg.spr"
//#define HUD_DAMAGE_FULLBRIGHT 20// this amount and above is drawn at full brightness level
#define HEALTH_CRITICAL_VALUE		15
#define DMG_IMAGE_LIFE				2	// seconds that image is up

typedef struct
{
	//int iDmgType; <- just use element index to shift the bit
	float fExpire;
	int	x, y;
} DAMAGE_IMAGE;

enum paindir_sprite_frames_e
{
	SPR_PAIN_FRAME_F = 0,
	SPR_PAIN_FRAME_R,
	SPR_PAIN_FRAME_B,
	SPR_PAIN_FRAME_L,
};


class CHudHealth: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual void Reset(void);
	virtual int Draw(const float &fTime);

	void SetHealth(int value);
	int DrawPain(const float &fTime);
	int DrawDamage(const float &fTime);
	void CalcDamageDirection(const vec3_t &vecDelta);
	void UpdateTiles(uint32 iCurrentBits);

	int MsgFunc_Damage(const char *pszName, int iSize, void *pbuf);

	int m_iHealth;
	//int m_iLastDamage;
protected:
	float m_fAttackFront, m_fAttackRear, m_fAttackLeft, m_fAttackRight;
	float m_fFade;
	uint32 m_bitsDamage;
	HLSPRITE m_hSpriteIcon;
	wrect_t *m_prcIcon;
	int m_iszHUDDmgTypeIcons[MAX_DMG_TYPES];// XDM3038a
	DAMAGE_IMAGE m_dmg[MAX_DMG_TYPES];
	HLSPRITE m_hSprite;
	//HLSPRITE m_hDamage;
	cvar_t *m_pCvarDmgDirScale;// XDM3035
};	

#endif // HEALTH_H
