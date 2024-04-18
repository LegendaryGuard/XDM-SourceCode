#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE(m_Train, Train)

int CHudTrain::Init(void)
{
	HOOK_MESSAGE(Train);
	m_iPos = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);
	return 1;
}

int CHudTrain::VidInit(void)
{
	m_hSprite = LoadSprite("sprites/%d_train.spr");// XDM3038
	return 1;
}

// Sprite frames must represent -4(lowest)...0...+4(highest) speed, e.g. 9 frames total.
int CHudTrain::Draw(const float &flTime)
{
	if (!gHUD.PlayerHasSuit())// XDM3038
		return 0;

//	ASSERT(m_hSprite != NULL);
	if (m_iPos > 0)
	{
		int x, y;
		byte r, g, b;
		UnpackRGB(r,g,b, RGB_BLUE);
		SPR_Set(m_hSprite, r, g, b);
		y = ScreenHeight - SPR_Height(m_hSprite, 0) - gHUD.m_iFontHeight;
		x = ScreenWidth/2 - SPR_Width(m_hSprite, 0)/2;
		SPR_DrawAdditive((m_iPos & 0x7F) - 1,  x, y, NULL);// XDM3038a: strip flag bits
	}
	return 1;
}

int CHudTrain::MsgFunc_Train(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iPos = READ_BYTE();
	END_READ();
	SetActive((m_iPos > 0));
	return 1;
}
