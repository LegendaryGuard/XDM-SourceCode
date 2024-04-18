#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "weapondef.h"
#include "pm_defs.h"
#include "pm_shared.h"

//extern playermove_t *pmove;// XDM3037a: we need movevars

DECLARE_MESSAGE(m_Flash, Flashlight)

int CHudFlashlight::Init(void)
{
	m_hSpriteMain = NULL;
	m_hSpriteFull = NULL;
	m_hSpriteOn = NULL;
	m_hLJM = NULL;
	Reset();
	HOOK_MESSAGE(Flashlight);
	SetActive(true);
	gHUD.AddHudElem(this);
	return 1;
}

void CHudFlashlight::Reset(void)
{
	m_fFadeLJ = 0;
	m_fOn = 0;
}

int CHudFlashlight::VidInit(void)
{
	int HUD_flash_empty = gHUD.GetSpriteIndex("flash_empty");
	int HUD_flash_full = gHUD.GetSpriteIndex("flash_full");
	int HUD_flash_beam = gHUD.GetSpriteIndex("flash_beam");
	int HUD_longjump = gHUD.GetSpriteIndex("item_longjump");// XDM3038a
	if (HUD_flash_empty != HUDSPRITEINDEX_INVALID)// XDM3037
	{
		m_hSpriteMain = gHUD.GetSprite(HUD_flash_empty);
		m_prcMain = &gHUD.GetSpriteRect(HUD_flash_empty);
	}
	if (HUD_flash_full != HUDSPRITEINDEX_INVALID)// XDM3037
	{
		m_hSpriteFull = gHUD.GetSprite(HUD_flash_full);
		m_prcFull = &gHUD.GetSpriteRect(HUD_flash_full);
	}
	if (HUD_flash_beam != HUDSPRITEINDEX_INVALID)// XDM3037
	{
		m_hSpriteOn = gHUD.GetSprite(HUD_flash_beam);
		m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
	}
	if (HUD_longjump != HUDSPRITEINDEX_INVALID)// XDM3038a
	{
		m_hLJM = gHUD.GetSprite(HUD_longjump);
		m_prcLJM = &gHUD.GetSpriteRect(HUD_longjump);
	}
	return 1;
}

int CHudFlashlight::MsgFunc_Flashlight(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_fOn = READ_BYTE();
	m_flBat = ((float)READ_BYTE())/100.0f;
	END_READ();
	//PlaySound("items/flashlight1.wav", 1);// XDM: must be played on server so everybody can hear
	return 1;
}

int CHudFlashlight::Draw(const float &flTime)
{
	if (!gHUD.PlayerHasSuit())// XDM3038
		return 0;

	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return 0;

	int r, g, b, x, y = 0, ret = 0;
	int iWidth = RectWidth(*m_prcFull);

	if (!(gHUD.m_iHideHUDDisplay & HIDEHUD_FLASHLIGHT))
	{
		wrect_t rc;
		/*if (m_fOn)
			a = MAX_ALPHA;
		else
			a = MIN_ALPHA;*/

		// XDM
		r = (int)(255.0f * (1.0f - m_flBat));
		g = (int)(255.0f * m_flBat);
		b = 0;
		if (m_fOn == FALSE)
			ScaleColors(r, g, b, MIN_ALPHA);// MAX_ALPHA is 255 anyway

		y = RectHeight(*m_prcMain)/2;
		x = ScreenWidth - iWidth*1.5;
		m_iWidth = x;// XDM3038a: conflicted with base class

		// Flashlight background layer (all layers are additive so don't mind drawing over)
		if (m_hSpriteMain)
		{
			SPR_Set(m_hSpriteMain, r, g, b);
			SPR_DrawAdditive(0, x, y, m_prcMain);
			++ret;
		}
		// Flashlight "on" indicator (beam of light)
		if (m_fOn && m_hSpriteOn)
		{
			x = ScreenWidth - iWidth/2;
			SPR_Set(m_hSpriteOn, r, g, b);
			SPR_DrawAdditive(0, x, y, m_prcBeam);
			++ret;
		}
		// Flashlight energy level is drawn cropped (x++) over the main layer
		if (m_hSpriteFull)
		{
			x = ScreenWidth - iWidth*1.5f;
			int iOffset = (int)((float)iWidth * (1.0f - m_flBat));
			if (iOffset < iWidth)
			{
				rc = *m_prcFull;
				rc.left += iOffset;
				SPR_Set(m_hSpriteFull, r, g, b);
				SPR_DrawAdditive(0, x + iOffset, y, &rc);
				++ret;
			}
		}
	}

	// XDM3038a: kind of a hack: draw long jump module icon
	if (m_hLJM && !(gHUD.m_iHideHUDDisplay & HIDEHUD_POWERUPS))
	{
		const char *pkv = pmove->PM_Info_ValueForKey(pmove->physinfo, PHYSKEY_LONGJUMP);
		if (pkv && *pkv != '0')
		{
			UnpackRGB(r,g,b, RGB_GREEN);
			ScaleColors(r, g, b, (int)max(MIN_ALPHA, m_fFadeLJ));// is 255 anyway
			SPR_Set(m_hLJM, r, g, b);
			SPR_DrawAdditive(0, m_iWidth - (int)(RectWidth(*m_prcLJM)*1.5f), y, m_prcLJM);// y is 0 if flashlight is hidden
			++ret;
		}
	}
	if (m_fFadeLJ > MIN_ALPHA)
		m_fFadeLJ -= (float)(gHUD.m_flTimeDelta) * 160.0f;

	return ret;
}

void CHudFlashlight::LongJump(void)
{
	m_fFadeLJ = MAX_ALPHA;
}
