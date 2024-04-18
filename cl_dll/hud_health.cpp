#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "weapondef.h"
#include "pm_shared.h"// HITGROUP_
#include "shake.h"

// XDM3037a DECLARE_MESSAGE(m_Health, Health)
DECLARE_MESSAGE(m_Health, Damage)

const char *giDmgTypeNames[MAX_DMG_TYPES] = 
{
/*	DMG_CRUSH			*/"dmg_disarm",// little hack: many types use this for DMGM_DISARM
/*	DMG_BULLET			*/"",
/*	DMG_SLASH			*/"",
/*	DMG_BURN			*/"dmg_burn",
/*	DMG_FREEZE			*/"dmg_freeze",
/*	DMG_FALL			*/"",
/*	DMG_BLAST			*/"",
/*	DMG_CLUB			*/"",
/*	DMG_SHOCK			*/"dmg_shock",
/*	DMG_SONIC			*/"dmg_sonic",
/*	DMG_ENERGYBEAM		*/"",
/*	DMG_MICROWAVE		*/"dmg_microwave",
/*	DMG_NEVERGIB		*/"",
/*	DMG_ALWAYSGIB		*/"",
/*	DMG_DROWN			*/"dmg_drown",
/*	DMG_PARALYZE		*/"",
/*	DMG_NERVEGAS		*/"dmg_nervegas",
/*	DMG_POISON			*/"dmg_poison",
/*	DMG_RADIATION		*/"dmg_radiation",
/*	DMG_DROWNRECOVER	*/"",
/*	DMG_ACID			*/"dmg_acid",
/*	DMG_SLOWBURN		*/"dmg_slowburn",
/*	DMG_SLOWFREEZE		*/"dmg_slowfreeze",
/*	DMG_NOHITPOINT		*/"",
/*	DMG_IGNITE			*/"dmg_ignite",
/*	DMG_RADIUS_MAX		*/"",
/*	DMG_DONT_BLEED		*/"",
/*	DMG_IGNOREARMOR		*/"",
/*	DMG_VAPOURIZING		*/"",
/*	DMG_WALLPIERCING	*/"",
/*	DMG_DISINTEGRATING	*/"",
/*	DMG_CUSTOM			*/"dmg_custom",
};

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
int CHudHealth::Init(void)
{
	HOOK_MESSAGE(Damage);
	//memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * MAX_DMG_TYPES);// this is wrong!
	for (size_t i=0; i<MAX_DMG_TYPES; ++i)
		m_iszHUDDmgTypeIcons[i] = HUDSPRITEINDEX_INVALID;

	m_pCvarDmgDirScale = CVAR_CREATE("hud_dmgdir_scale", "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	//InitHUDData();
	Reset();
	m_iHealth = gHUD.m_iPlayerMaxHealth;
	gHUD.AddHudElem(this);
	return 1;
}

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
int CHudHealth::VidInit(void)
{
	int iSpriteIcon = gHUD.GetSpriteIndex("health");
	// obsolete? m_hSprite1 = m_hSprite2 = 0;// delaying get sprite handles until we know the sprites are loaded
	if (iSpriteIcon != HUDSPRITEINDEX_INVALID)
	{
		m_hSpriteIcon = gHUD.GetSprite(iSpriteIcon);
		m_prcIcon = &gHUD.GetSpriteRect(iSpriteIcon);
	}
	else
	{
		m_hSpriteIcon = 0;
		m_prcIcon = NULL;
	}
	m_hSprite = LoadSprite(PAIN_SPRITENAME);
	for (size_t i=0; i<MAX_DMG_TYPES; ++i)
	{
		if (giDmgTypeNames[i] && giDmgTypeNames[i][0])
			m_iszHUDDmgTypeIcons[i] = gHUD.GetSpriteIndex(giDmgTypeNames[i]);
		else
			m_iszHUDDmgTypeIcons[i] = HUDSPRITEINDEX_INVALID;
	}
	return 1;
}

//-------------------------------------------------------------------------
// Purpose: 
//-------------------------------------------------------------------------
void CHudHealth::Reset(void)
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	m_fFade = 0;
	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for (size_t i = 0; i < MAX_DMG_TYPES; ++i)
	{
		//m_dmg[i].iDmgType = 1<<i;
		m_dmg[i].fExpire = 0;
	}
}

//-------------------------------------------------------------------------
// Purpose: Drawing code
// Input  : flTime - client time in seconds
// Output : int - Return 1 on success, 0 on failure.
//-------------------------------------------------------------------------
int CHudHealth::Draw(const float &flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly())
		return 0;

	if (!gHUD.PlayerHasSuit())// XDM3038: draw health only if we have suit
		return 0;

	int x, y, y_icon;
	byte r,g,b,a;

	if (m_iHealth <= HEALTH_CRITICAL_VALUE)// If health is getting low, make it bright red
	{
		a = 255;
		m_fFade = 0;
	}
	else if (m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (float)(gHUD.m_flTimeDelta * HUD_FADE_SPEED);
		if (m_fFade <= 0)
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}
		else
			a = MIN_ALPHA + (int)((m_fFade/FADE_TIME)*(MAX_ALPHA-MIN_ALPHA));
	}
	else
		a = MIN_ALPHA;

	GetMeterColor((float)m_iHealth/(float)gHUD.m_iPlayerMaxHealth, r, g, b);
	ScaleColors(r, g, b, a);

	x = RectWidth(gHUD.GetSpriteRect(gHUD.m_HUD_number_0));// XDM3037a: digit width
	y = gHUD.GetHUDBottomLine();

	if (m_hSpriteIcon)
	{
		y_icon = y + (gHUD.m_iFontHeight - RectHeight(*m_prcIcon))/2;// make icon and digits centered vertically by the same horizontal axis
		SPR_Set(m_hSpriteIcon, r, g, b);
		SPR_DrawAdditive(0, x, y_icon, m_prcIcon);
		x += RectWidth(*m_prcIcon);
	}

	if (gHUD.m_pCvarDrawNumbers->value > 0)
		x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b);

	m_iWidth = x;
	m_iHeight = y;

	DrawDamage(flTime);
	DrawPain(flTime);
	return 1;
}

//-------------------------------------------------------------------------
// Purpose: XDM3037a: for external use
//-------------------------------------------------------------------------
void CHudHealth::SetHealth(int value)
{
	SetActive(true);
	if (value != m_iHealth)// Only update the fade if we've changed health
	{
		m_iHealth = value;
		m_fFade = FADE_TIME;
	}
}

//-------------------------------------------------------------------------
// Purpose: Populate m_fAttack* vars
// Input  : vecDelta - (target-local)
//-------------------------------------------------------------------------
void CHudHealth::CalcDamageDirection(const vec3_t &vecDelta)
{
	if (vecDelta.IsZero())//if (!vecFrom[0] && !vecFrom[1] && !vecFrom[2])
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}
	vec_t side, front;
	Vector forward, right, up;
	Vector vecDir(vecDelta);
	vec_t flDistToTarget = vecDir.NormalizeSelf();
	//AngleVectors(gHUD.m_vecAngles, forward, right, up);
	AngleVectors(g_vecViewAngles, forward, right, up);
	front = DotProduct(vecDir, right);
	side = DotProduct(vecDir, forward);

	if (flDistToTarget <= HULL_RADIUS*2.0f)// XDM3038c
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if (side > 0)
		{
			if (side > 0.3f)
				m_fAttackFront = max(m_fAttackFront, side);
		}
		else
		{
			float f = fabs(side);
			if (f > 0.3f)
				m_fAttackRear = max(m_fAttackRear, f);
		}

		if (front > 0)
		{
			if (front > 0.3f)
				m_fAttackRight = max(m_fAttackRight, front);
		}
		else
		{
			float f = fabs(front);
			if (f > 0.3f)
				m_fAttackLeft = max(m_fAttackLeft, f);
		}
	}
}

//-------------------------------------------------------------------------
// XDM: TODO: move this to TriAPI or RenderSystem?
//-------------------------------------------------------------------------
int CHudHealth::DrawPain(const float &flTime)
{
	if (!(m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight))
		return 1;

	//if (!m_hSprite)
	//	m_hSprite = LoadSprite(PAIN_SPRITENAME);

	int x, y, r,g,b;//,a, shade;
	if (m_bitsDamage == DMG_NOHITPOINT)// XDM3038c: exactly this value
		UnpackRGB(r,g,b,RGB_BLUE);
	else
		UnpackRGB(r,g,b,RGB_RED);

	//TODO: get the shift value of the health
	//a = 255;	// max brightness until then
	float fFade = gHUD.m_flTimeDelta * 2.0;

	/*  _   Sprites are drawn like this:
	  _|F|_
	 |L|_|R|
	   |B|
	*/
	if (m_fAttackFront > 0.4)
	{
		//shade = a * max(m_fAttackFront, 0.5);
		ScaleColorsF(r, g, b, max(m_fAttackFront, 0.5f));
		SPR_Set(m_hSprite, r, g, b);
		x = -SPR_Width(m_hSprite, SPR_PAIN_FRAME_F)/2;
		// still no perspective correction		
		y = -(m_pCvarDmgDirScale->value*SPR_Height(m_hSprite, SPR_PAIN_FRAME_R)/2 + SPR_Height(m_hSprite, SPR_PAIN_FRAME_F));// XDM3035: was SPR_PAIN_FRAME_F)*3
		/*
		gEngfuncs.pfnSPR_Height(m_hSprite, SPR_PAIN_FRAME_F) 16
		gEngfuncs.pfnSPR_Height(m_hSprite, SPR_PAIN_FRAME_B) 48
		1/3
		*/
		// TODO: correct perspective (if any) using difference in frame sizes
		x += ScreenWidth/2;// XDM3035: center of screen
		y += ScreenHeight/2;
		SPR_DrawAdditive(SPR_PAIN_FRAME_F, x, y, NULL);
		m_fAttackFront = max(0, m_fAttackFront - fFade);
	} else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		//shade = a * max( m_fAttackRight, 0.5 );
		ScaleColorsF(r, g, b, max(m_fAttackRight, 0.5f));
		SPR_Set(m_hSprite, r, g, b);
		x = m_pCvarDmgDirScale->value * SPR_Width(m_hSprite, SPR_PAIN_FRAME_F)/2;// XDM3035: was SPR_PAIN_FRAME_R)*2;
		y = - SPR_Height(m_hSprite, SPR_PAIN_FRAME_R)/2;
		x += ScreenWidth/2;// XDM3035: center of screen
		y += ScreenHeight/2;
		SPR_DrawAdditive(SPR_PAIN_FRAME_R, x, y, NULL);
		m_fAttackRight = max(0, m_fAttackRight - fFade);
	} else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		//shade = a * max( m_fAttackRear, 0.5 );
		ScaleColorsF(r, g, b, max(m_fAttackRear, 0.5f));
		SPR_Set(m_hSprite, r, g, b);
		x = - SPR_Width(m_hSprite, SPR_PAIN_FRAME_B)/2;
		y = m_pCvarDmgDirScale->value * SPR_Height(m_hSprite, SPR_PAIN_FRAME_R)/2;// XDM3035: was SPR_PAIN_FRAME_B)*2;
		x += ScreenWidth/2;// XDM3035: center of screen
		y += ScreenHeight/2;
		SPR_DrawAdditive(SPR_PAIN_FRAME_B, x, y, NULL);
		m_fAttackRear = max(0, m_fAttackRear - fFade);
	} else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		//shade = a * max( m_fAttackLeft, 0.5 );
		ScaleColorsF(r, g, b, max(m_fAttackLeft, 0.5f));
		SPR_Set(m_hSprite, r, g, b);
		x = - m_pCvarDmgDirScale->value * SPR_Width(m_hSprite, SPR_PAIN_FRAME_F)/2 - SPR_Width(m_hSprite, SPR_PAIN_FRAME_L);// XDM3035: was SPR_PAIN_FRAME_L)*3;
		y = - SPR_Height(m_hSprite, SPR_PAIN_FRAME_L)/2;
		x += ScreenWidth/2;// XDM3035: center of screen
		y += ScreenHeight/2;
		SPR_DrawAdditive(SPR_PAIN_FRAME_L, x, y, NULL);
		m_fAttackLeft = max(0, m_fAttackLeft - fFade);
	} else
		m_fAttackLeft = 0;

	return 1;
}

//-------------------------------------------------------------------------
// Draw damage TYPE indicators
//-------------------------------------------------------------------------
int CHudHealth::DrawDamage(const float &flTime)
{
	int r, g, b, a;
	UnpackRGB(r,g,b, RGB_YELLOW);// XDM: green?
	a = (int)(fabs(sin(flTime*2.0f))*255.0f);
	ScaleColors(r, g, b, a);

	// Draw all the items
	uint32 i;
	for (i = 0; i < MAX_DMG_TYPES; ++i)
	{
		if ((m_bitsDamage & (1<<i)) && m_iszHUDDmgTypeIcons[i] >= 0)
		{
			SPR_Set(gHUD.GetSprite(m_iszHUDDmgTypeIcons[i]), r, g, b);
			SPR_DrawAdditive(0, m_dmg[i].x, m_dmg[i].y, &gHUD.GetSpriteRect(m_iszHUDDmgTypeIcons[i]));
		}
	}

	DAMAGE_IMAGE *pdmg;
	// check for bits that should be expired
	for (i = 0; i < MAX_DMG_TYPES; ++i)
	{
		pdmg = &m_dmg[i];
		if (m_bitsDamage & (1<<i))//pdmg->iDmgType)
		{
			pdmg->fExpire = min(flTime + DMG_IMAGE_LIFE, pdmg->fExpire);
			if (pdmg->fExpire <= flTime && a < 40)// when the time has expired // and the flash is at the low point of the cycle
			{
				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;
				pdmg->fExpire = 0;
				// move everyone above down
				for (uint32 j = 0; j < MAX_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if ((pdmg->y > 0) && (pdmg->y < y))
						pdmg->y += RectHeight(gHUD.GetSpriteRect(m_iszHUDDmgTypeIcons[j]));// XDM3037
				}
				m_bitsDamage &= ~(1<<i);//pdmg->iDmgType;//~giDmgFlags[i];  // clear the bits
			}
		}
	}
	return 1;
}

//-------------------------------------------------------------------------
// Add damage type sprites
//-------------------------------------------------------------------------
void CHudHealth::UpdateTiles(uint32 iCurrentBits)
{
	// Which types are new?
	uint32 bitsOn = ~m_bitsDamage & iCurrentBits;
	int h;
	DAMAGE_IMAGE *pdmg;
	for (uint32 i = 0; i < MAX_DMG_TYPES; ++i)
	{
		pdmg = &m_dmg[i];
		if (m_bitsDamage & (1<<i))//m_dmg[i].iDmgType)// if this one is already on, extend its duration
		{
			if (pdmg->fExpire >= gHUD.m_flTime)
				pdmg->fExpire += DMG_IMAGE_LIFE;
			else
				pdmg->fExpire = gHUD.m_flTime + DMG_IMAGE_LIFE;

			//if (!pdmg->fBaseline)
			//	pdmg->fBaseline = gHUD.m_flTime;
			continue;
		}

		// Are we just turning it on?
		if ((bitsOn & (1<<i)) && m_iszHUDDmgTypeIcons[i] >= 0)//m_dmg[i].iDmgType)
		{
			if (m_iszHUDDmgTypeIcons[i] < 0)// XDM3038a: no sprite for this type
				continue;
			// put this one at the bottom
			h = RectHeight(gHUD.GetSpriteRect(m_iszHUDDmgTypeIcons[i]));// XDM3037
			pdmg->x = RectWidth(gHUD.GetSpriteRect(m_iszHUDDmgTypeIcons[i]))/8;// XDM3037
			pdmg->y = ScreenHeight - h * 2;
			pdmg->fExpire = gHUD.m_flTime + DMG_IMAGE_LIFE;
			// move everyone else up
			for (uint32 j = 0; j < MAX_DMG_TYPES; ++j)
			{
				if (j != i)
				{
					pdmg = &m_dmg[j];
					if (pdmg->y)
						pdmg->y -= h;
				}
			}
			//?	pdmg = &m_dmg[i];
		}
	}
	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= iCurrentBits;
}

//-----------------------------------------------------------------------------
// Purpose: Damage information from server. TODO: is it called properly?
// Output : int
//-----------------------------------------------------------------------------
int CHudHealth::MsgFunc_Damage(const char *pszName, int iSize, void *pbuf)
{
	Vector vecFrom;
	BEGIN_READ(pbuf, iSize);
	byte hithgoup = READ_BYTE();
	//m_iLastDamage = READ_BYTE();
	uint32 bitsDamage = READ_UINT32();// XDM3038a
	READ_COORD3(vecFrom);// absolute coordinates
	END_READ();
	//conprintf(1, "CHudAmmo::MsgFunc_Damage() %d %d %d\n", hithgoup, damageTaken, bitsDamage);
	UpdateTiles(bitsDamage);

	// XDM3035: this message is passed to the local client only, so don't invoke any damage effect code
	//if (bitsDamage & DMG_SLOWBURN) DoEffect();

	bool bFrozen = (hithgoup & 0x80) > 0;// last bit in byte
	hithgoup &= ~0x80;// convert it back to normal value
	if ((gHUD.m_bFrozen > 0) != bFrozen)
	{
		if (bFrozen && gHUD.PlayerIsAlive() && gHUD.m_pLocalPlayer)
			CL_ScreenFade(gHUD.m_pLocalPlayer->curstate.rendercolor.r,gHUD.m_pLocalPlayer->curstate.rendercolor.g,gHUD.m_pLocalPlayer->curstate.rendercolor.b, 127, 1.0f, 1.0f, FFADE_OUT | FFADE_STAYOUT);
		//	UTIL_ScreenFade(this, pev->rendercolor, 1.0, 1.0, 127, FFADE_OUT | FFADE_STAYOUT);
		//else// there's no else because there's no second gmsgDamage call
		//	CL_ScreenFade(gHUD.m_pLocalPlayer->curstate.rendercolor.r,gHUD.m_pLocalPlayer->curstate.rendercolor.g,gHUD.m_pLocalPlayer->curstate.rendercolor.b, 127, 4.0f, 0.0f, FFADE_IN);
		//	UTIL_ScreenFade(this, pev->rendercolor, 4, 0, 128, FFADE_IN);

		gHUD.m_bFrozen = bFrozen;
	}
	//XDM3037a: FIXME: damage is accumulated before being transmitted, also screenfade is bugged
	if (!gHUD.m_bFrozen && gHUD.PlayerIsAlive())// && m_iLastDamage > 0)// immediate screen damage indication
	{
		if (bitsDamage & DMG_DROWN)
			CL_ScreenFade(207,207,207, 191/*clamp(damageTaken*6, 10, 255)*/, 1.0f, 0.1f, FFADE_IN);
		/*else if (bitsDamage & DMG_POISON)
			CL_ScreenFade(127,255,31, clamp(damageTaken*4, 10, 255), 1.0f, 0.2f, FFADE_IN);
		else if (bitsDamage & DMG_NERVEGAS)
			CL_ScreenFade(207,207,95, clamp(damageTaken*4, 10, 255), 1.0f, 0.2f, FFADE_IN);
		//else if (hithgoup == HITGROUP_GENERIC)
		//	CL_ScreenFade(191,0,0, clamp(damageTaken*2, 10, 191), 1.0f, 0.1f, FFADE_IN);
		else if (hithgoup == HITGROUP_HEAD)
			CL_ScreenFade(255,20,0, clamp(damageTaken*4, 10, 255), 1.0f, 0.1f, FFADE_IN);
		else if (hithgoup == HITGROUP_CHEST || hithgoup == HITGROUP_STOMACH)
			CL_ScreenFade(200,80,0, clamp(damageTaken*3, 10, 255), 1.0f, 0.1f, FFADE_IN);
		//else if (hithgoup == HITGROUP_ARMOR && HaveBattery())
		//	ShowShieldActiveIcon();*/
	}

	// Actually took damage?
	//if (damageTaken > 0)// || armor > 0){
		vecFrom -= g_vecViewOrigin;//vecFrom -= gHUD.m_vecOrigin;// XDM3038c: now it's delta
		CalcDamageDirection(vecFrom);

	return 1;
}
