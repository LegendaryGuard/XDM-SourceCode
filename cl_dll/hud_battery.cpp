#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "weapondef.h"

DECLARE_MESSAGE(m_Battery, Battery)

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
int CHudBattery::Init(void)
{
	HOOK_MESSAGE(Battery);
	m_iBat = 0;
	// TOO EARLY!	m_iMaxBat = gHUD.m_iPlayerMaxArmor;// HL20130901
	//InitHUDData();
	Reset();
	gHUD.AddHudElem(this);
	return 1;
}

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
int CHudBattery::VidInit(void)
{
	int iSpriteEmpty = gHUD.GetSpriteIndex("suit_empty");
	int iSpriteFull = gHUD.GetSpriteIndex("suit_full");
	int iSpriteDivider = gHUD.GetSpriteIndex("divider");
	// obsolete? m_hSprite1 = m_hSprite2 = 0;// delaying get sprite handles until we know the sprites are loaded

	if (iSpriteEmpty != HUDSPRITEINDEX_INVALID)
	{
		m_hSpriteEmpty = gHUD.GetSprite(iSpriteEmpty);
		m_prcEmpty = &gHUD.GetSpriteRect(iSpriteEmpty);
	}
	else
	{
		m_hSpriteEmpty = 0;
		m_prcEmpty = NULL;
	}
	if (iSpriteFull != HUDSPRITEINDEX_INVALID)
	{
		m_hSpriteFull = gHUD.GetSprite(iSpriteFull);
		m_prcFull = &gHUD.GetSpriteRect(iSpriteFull);
	}
	else
	{
		m_hSpriteFull = 0;
		m_prcFull = NULL;
	}
	if (iSpriteDivider != HUDSPRITEINDEX_INVALID)
	{
		m_hSpriteDivider = gHUD.GetSprite(iSpriteDivider);
		m_prcDivider = &gHUD.GetSpriteRect(iSpriteDivider);
	}
	else
	{
		m_hSpriteDivider = 0;
		m_prcDivider = NULL;
	}

	if (m_prcEmpty && m_prcFull)
		m_iHeight = m_prcFull->bottom - m_prcEmpty->top;

	m_fFade = 0;
	return 1;
}

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
void CHudBattery::Reset(void)
{
	m_fFade = 0;
	m_iFlags = 0;
}

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
int CHudBattery::Draw(const float &flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly())
		return 0;

	if (!gHUD.PlayerHasSuit())// XDM3038
		return 0;

	if (!m_hSpriteEmpty)
		return 0;//m_hSprite1 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_empty"));
	if (!m_hSpriteFull)
		return 0;//m_hSprite2 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_full"));

	int x, y, y_icon;
	byte r,g,b,a;
	//wrect_t rc = *m_prcFull;// copy data
	//rc.top += m_iHeight * (int)((float)(gHUD.m_iPlayerMaxArmor-(min(gHUD.m_iPlayerMaxArmor,m_iBat))) * 0.01f);	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1
	// WTF is this? int iOffset = (m_prcEmpty->bottom - m_prcEmpty->top)/6;

	// Battery indicator position
	int iDigitWidth = RectWidth(gHUD.GetSpriteRect(gHUD.m_HUD_number_0));
	x = gHUD.m_Health.m_iWidth + iDigitWidth;// XDM3038a //ScreenWidth/5;
	y = gHUD.GetHUDBottomLine();
	if (m_hSpriteDivider)
	{
		y_icon = y + (gHUD.m_iFontHeight - RectHeight(*m_prcDivider))/2;// make icon and digits centered vertically by the same horizontal axis
		a = MIN_ALPHA;
		UnpackRGB(r,g,b, RGB_GREEN);
		ScaleColors(r, g, b, a);
		SPR_Set(m_hSpriteDivider, r, g, b);
		SPR_DrawAdditive(0, x, y_icon, m_prcDivider);
		x += RectWidth(*m_prcDivider) + iDigitWidth/2;
	}

	GetMeterColor((float)m_iBat/(float)gHUD.m_iPlayerMaxArmor, r, g, b);

	if (m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (float)(gHUD.m_flTimeDelta * HUD_FADE_SPEED);
		if (m_fFade <= 0)
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}
		a = MIN_ALPHA + (int)((m_fFade/FADE_TIME)*(MAX_ALPHA-MIN_ALPHA));
	}
	else
		a = MIN_ALPHA;

	y_icon = y + (gHUD.m_iFontHeight - RectHeight(*m_prcEmpty))/2;
	ScaleColors(r, g, b, a);
	SPR_Set(m_hSpriteEmpty, r, g, b);
	SPR_DrawAdditive(0, x, y_icon, m_prcEmpty);

	if (m_iBat > 0)//if (rc.bottom > rc.top)
	{
		wrect_t rc = *m_prcFull;
		int iOffset = (int)((float)RectHeight(rc) * (1.0f - (float)m_iBat/(float)gHUD.m_iPlayerMaxArmor));// offset is the empty part
		rc.top += iOffset;
		SPR_Set(m_hSpriteFull, r, g, b);
		SPR_DrawAdditive(0, x, y_icon + iOffset, &rc);
	}

	x += RectWidth(*m_prcEmpty);
	if (gHUD.m_pCvarDrawNumbers->value > 0)
		x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);

	m_iWidth = x;
	m_iHeight = y;
	return 1;
}

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
int CHudBattery::MsgFunc_Battery(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iNewBat = READ_BYTE();// XDM3038: was READ_SHORT();
	//int y = READ_SHORT();
	END_READ();

	if (iNewBat != m_iBat)// || y != m_iMaxBat)// HL20130901
	{
		m_fFade = FADE_TIME;
		m_iBat = iNewBat;
		//m_iMaxBat = y;// HL20130901
	}
	SetActive(true);
	return 1;
}
