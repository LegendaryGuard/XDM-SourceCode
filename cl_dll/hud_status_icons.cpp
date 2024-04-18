#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "com_model.h"

DECLARE_MESSAGE(m_StatusIcons, StatusIcon);

int CHudStatusIcons::Init(void)
{
	HOOK_MESSAGE(StatusIcon);
	m_pCvarDrawStatusIcons = CVAR_CREATE("hud_drawstatusicons", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3037a
	gHUD.AddHudElem(this);
	Reset();
	return 1;
}

int CHudStatusIcons::VidInit(void)
{
	return 1;//?
}

void CHudStatusIcons::Reset(void)
{
	memset(m_IconList, 0, sizeof m_IconList);
	SetActive(false);
}

int CHudStatusIcons::Draw(const float &flTime)
{
	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if (m_pCvarDrawStatusIcons && m_pCvarDrawStatusIcons->value <= 0)// XDM3037a
		return 1;

	int x = STATUS_ICONS_MARGIN;
	int y = ScreenHeight/4;
	float k;
	byte r,g,b;
	bool bClear;
	bool bFade;

	for (short i = 0; i < MAX_ICONSPRITES; ++i)
	{
		bClear = false;
		bFade = false;
		if (m_IconList[i].endholdtime > 0)// XDM3037a
		{
			if (m_IconList[i].endholdtime <= flTime)// end of hold
				bClear = true;
			//else// still holding
			//	bClear = false;
		}

		if (bClear && m_IconList[i].endfadetime > 0)
		{
			if (m_IconList[i].endfadetime <= flTime)// end of fade
				bClear = true;
			else// still fading
			{
				float fadetime = (m_IconList[i].endfadetime - m_IconList[i].endholdtime);
				float fadeelapsed = (flTime - m_IconList[i].endholdtime);
				k = 1.0f-(fadeelapsed/fadetime);
				bFade = true;
				bClear = false;
			}
		}
		//else // if (m_IconList[i].endfadetime == 0)

		if (bClear)// turn off, clear
		{
			memset(&m_IconList[i], 0, sizeof(icon_sprite_t));
			continue;
		}

		if (m_IconList[i].spr)
		{
			y += RectHeight(m_IconList[i].rc) + STATUS_ICONS_MARGIN;// XDM3037a (m_IconList[i].rc.bottom - m_IconList[i].rc.top) + 5;
			r = m_IconList[i].color.r;
			g = m_IconList[i].color.g;
			b = m_IconList[i].color.b;
			if (bFade)
				ScaleColorsF(r,g,b, k);

			SPR_Set(m_IconList[i].spr, r,g,b);

			// XDM3037a: TODO: SPR_DrawHoles doesn't work :(
			/*const model_t *pTex = gEngfuncs.GetSpritePointer(m_IconList[i].spr);
			if (pTex && pTex->type == mod_sprite)
			{
				msprite_t *psprite = (msprite_t *)pTex->cache.data;
				if (psprite->texFormat == SPR_NORMAL)
					SPR_Draw(m_IconList[i].frame, x, y, &m_IconList[i].rc);
				else if (psprite->texFormat == SPR_ALPHTEST)
					SPR_DrawHoles(m_IconList[i].frame, x, y, &m_IconList[i].rc);
				else
					SPR_DrawAdditive(m_IconList[i].frame, x, y, &m_IconList[i].rc);
			}
			else*/
				SPR_DrawAdditive(m_IconList[i].frame, x, y, &m_IconList[i].rc);
		}
	}
	return 1;
}

//byte   : 1/0 ENABLE/DISABLE icon
//string : sprite name (from HUD)
//byte   : red
//byte   : green
//byte   : blue
//short  : hold time // XDM3037a
//short  : fade out time
int CHudStatusIcons::MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	byte bShouldEnable = READ_BYTE();
	char *pszIconName = READ_STRING();
	if (bShouldEnable)
	{
		byte r = READ_BYTE();
		byte g = READ_BYTE();
		byte b = READ_BYTE();
		short ht = READ_SHORT();
		short ft = READ_SHORT();
		EnableIcon(pszIconName, r, g, b, ht, ft);
	}
	else
		DisableIcon(pszIconName);

	END_READ();
	return 1;
}

void CHudStatusIcons::EnableIcon(const char *pszIconName, byte r, byte g, byte b, short holdtime, short fadetime)
{
	if (pszIconName == NULL)
		return;

	short i;
	for (i = 0; i < MAX_ICONSPRITES; ++i)// find existing sprite
	{
		if (strcmp(m_IconList[i].szSpriteName, pszIconName) == 0)// XDM3038c
			break;
	}
	if (i == MAX_ICONSPRITES)// not found, find an empty slot
	{
		for (i = 0; i < MAX_ICONSPRITES; ++i)
		{
			if (m_IconList[i].spr == 0)
				break;
		}
	}
	if (i == MAX_ICONSPRITES)// not found, overwrite the first slot
		i = 0;

	int spr_index = gHUD.GetSpriteIndex(pszIconName);
	if (spr_index == HUDSPRITEINDEX_INVALID)
	{
		conprintf(1, "CHudStatusIcons::EnableIcon(%s): sprite not found\n", pszIconName);
		return;
	}
	m_IconList[i].spr = gHUD.GetSprite(spr_index);
	m_IconList[i].rc = gHUD.GetSpriteRect(spr_index);
	m_IconList[i].frame = gHUD.GetSpriteFrame(spr_index);// XDM3037a
	m_IconList[i].color.Set(r,g,b,255);
	if (holdtime > 0)
		m_IconList[i].endholdtime = gHUD.m_flTime + holdtime;
	if (fadetime > 0)
		m_IconList[i].endfadetime = gHUD.m_flTime + holdtime + fadetime;

	strncpy(m_IconList[i].szSpriteName, pszIconName, MAX_SPRITE_NAME_LENGTH);
	SetActive(true);
}

void CHudStatusIcons::DisableIcon(const char *pszIconName)
{
	if (pszIconName == NULL)
		return;

	for (uint16 i = 0; i < MAX_ICONSPRITES; ++i)
	{
		if (strcmp(m_IconList[i].szSpriteName, pszIconName) == 0)// XDM3038c
		{
			memset(&m_IconList[i], 0, sizeof(icon_sprite_t));
			return;
		}
	}
}
