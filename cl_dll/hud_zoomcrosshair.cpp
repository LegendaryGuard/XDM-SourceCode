#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "com_model.h"
#include "triangleapi.h"
#include "studio_util.h"
#include "event_api.h"
#include "eventscripts.h"

extern float g_lastFOV;

#define ZOOM_FOV_DELTA		160

// !!! DON'T FORGET TO Begin(TRI_QUADS) !!!
void DrawQuad(const float &xmin, const float &ymin, const float &xmax, const float &ymax)
{
	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3f(xmin, ymin, 0.0f); 
	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3f(xmin, ymax, 0.0f);
	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3f(xmax, ymax, 0.0f);
	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3f(xmax, ymin, 0.0f);
}


// this is called just as launcher loads in steam versions
int CHudZoomCrosshair::Init(void)
{
	m_fFinalFOV = 0.0f;
	m_iRenderMode = -1;
	m_iTextureIndex = -1;
	m_pTexture = NULL;
	m_iFlags = 0;
	gHUD.AddHudElem(this);
	return 1;
}

void CHudZoomCrosshair::InitHUDData(void)
{
	m_iRenderMode = -1;
	m_iFlags = 0;
}

void CHudZoomCrosshair::Reset(void)
{
	m_pTexture = NULL;
	// XDM3037: test
	m_fFinalFOV = gHUD.GetUpdatedDefaultFOV();// no zooming will be done
	SetActive(false);
	gHUD.SetFOV(0.0f);// default
	gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);// XDM3037a
}

int CHudZoomCrosshair::VidInit(void)
{
	//conprintf(1, ("CHudZoomCrosshair::VidInit()\n");
	/*if (m_iRenderMode >= 0)
	{
		m_pTexture = (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/c_zoom.spr"));
	}*/
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Drawing function
// Input  : &flTime - 
// Output : int
//-----------------------------------------------------------------------------
int CHudZoomCrosshair::Draw(const float &flTime)
{
	if (m_iRenderMode < 0)
		return 0;

	if (m_pTexture == NULL)
		return 0;// do not disable, we still need FOV updates!

	//if (IEngineStudio.IsHardware() <= 0)
	if (gHUD.m_iHardwareMode <= 0)// XDM3035: software mode does not support this code
		return 0;

	float left = (ScreenWidth - ScreenHeight)/2.0f;
	float right = left + ScreenHeight;
	float centerx = ScreenWidth/2.0f;
	float centery = ScreenHeight/2.0f;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);//kRenderTransColor for indexalpha
	gEngfuncs.pTriAPI->Brightness(1.0f);
	gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	//   ___   //
	//b |1|2| b//
	//b |4|3| b//
	// screen  //
	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, 0))
	{
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	DrawQuad(left,		0,			centerx,	centery);
	gEngfuncs.pTriAPI->End();
	}
	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, 1))
	{
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	DrawQuad(centerx,	0,			right,		centery);
	gEngfuncs.pTriAPI->End();
	}
	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, 2))
	{
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	DrawQuad(centerx,	centery,	right,		ScreenHeight);
	gEngfuncs.pTriAPI->End();
	}
	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, 3))
	{
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	DrawQuad(left,		centery,	centerx,	ScreenHeight);
	gEngfuncs.pTriAPI->End();
	}
	// XDM3036: TODO: sometimes texture get messed up and completely occludes player's view
	// To prevent this, draw a real hole
	/*float a = 0, fs, fc, uvx,uvy;
	float r = (ScreenHeight*0.4f);
	const int num = 64;
	for (i=0;i<num;++i)
	{
		SinCos(a, fs, fc);
		// circle
		x = centerx + fs*r;// vertex
		y = centery + fc*r;
		uvx = 0.5f + fs*0.5f;// UV
		uvy = 0.5f + fc*0.5f;
		// now draw triangles from circle to screen sides... how?
		sidex = -1 0 1?
		sidey = -1 0 1?

		a += (M_PI*2/num);
	}*/

	//gEngfuncs.pTriAPI->RenderMode(cl_test1->value);

	if (m_pTexture->numframes > 4 && gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, 4))
	{
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(0, 0, left+1.1f, ScreenHeight);// 1.1 little hack to fill gaps
		gEngfuncs.pTriAPI->End();
	}
	else
		FillRGBA(0, 0, left, ScreenHeight, 255,255,255,255);// FAIL!!! This can only draw in additive mode!

	if (m_pTexture->numframes > 5 && gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, 5))
	{
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(right-1.1f, 0, ScreenWidth, ScreenHeight);// 1.1 little hack to fill gaps
		gEngfuncs.pTriAPI->End();
	}
	else
		FillRGBA(right, 0, ScreenWidth, ScreenHeight, 255,255,255,255);

	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	float k = gHUD.GetUpdatedDefaultFOV()/g_lastFOV;
	char str[8];
	byte r,g,b;
	_snprintf(str, 8, "%.2gX\0", k);
	UnpackRGB(r,g,b, RGB_GREEN);
	DrawSetTextColor(r,g,b);
	DrawConsoleString(ScreenWidth*0.75f, ScreenHeight*0.75f, str);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Called externally by weapon code to start/stop zooming
// Input  : spr_idx1 - 4 frames central square crosshair, 2 frames monitor gaps
//			rendermode - 
//			finalfov - 
//-----------------------------------------------------------------------------
void CHudZoomCrosshair::SetParams(int spr_idx, int rendermode, int soundindex, float finalfov)
{
	ASSERT(finalfov >= 0.0f);
	//conprintf(1, ("SetParams: %d %d rm %d ff %f\n", spr_idx1, spr_idx2, rendermode, finalfov);
	if (rendermode >= kRenderNormal)// start zooming
	{
		// XDM3037a: optional
		model_t *pNewSprite;
		// 1
		if (spr_idx > 0)
		{
			pNewSprite = IEngineStudio.GetModelByIndex(spr_idx);
			if (!pNewSprite || pNewSprite->type != mod_sprite)
			{
				pNewSprite = NULL;
				conprintf(1, "CHudZoomCrosshair::SetParams() Warning: unable to load texture %d!\n", spr_idx);
			}
			else
			{
				m_pTexture = pNewSprite;
				m_iTextureIndex = spr_idx;
			}
		}
		else
			pNewSprite = NULL;

		if (pNewSprite == NULL)
		{
			m_pTexture = (model_t *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/c_zoom1.spr"));
			if (m_pTexture == NULL)
			{
				conprintf(1, "CHudZoomCrosshair::SetParams() Warning: unable to load default texture!\n");
				// allow zooming anyway SetActive(false);
			}
			m_iTextureIndex = -1;
		}

		m_fFinalFOV = finalfov;
		if (m_fFinalFOV < 10.0f)
		{
			conprintf(1, "CHudZoomCrosshair::SetParams() Warning: finalfov %g < 10!\n", m_fFinalFOV);
			m_fFinalFOV = 10.0f;
		}
		SetActive(true);
		m_fPlaySoundTime = gEngfuncs.GetClientTime();// XDM3037
		gHUD.SetFOV(0.0f);// default
		//gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);
	}
	else// reset
	{
		m_fFinalFOV = 0;// gHUD.GetUpdatedDefaultFOV(); TODO: restore FOV gradually
		SetActive(false);
		m_fPlaySoundTime = 0.0f;
		gHUD.SetFOV(0.0f);// default
		//gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);
	}
	m_iRenderMode = rendermode;
	m_iSoundIndex = soundindex;// XDM3038a
	gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);// XDM3037a
}

//-----------------------------------------------------------------------------
// Purpose: Per-frame
//-----------------------------------------------------------------------------
void CHudZoomCrosshair::Think(void)
{
	if (IsActive() && (gHUD.m_iPaused <= 0))
	{
		if (gHUD.m_iIntermission > 0)// XDM3037
		{
			SetParams(m_iTextureIndex, 0, 0, 0);
			SetActive(false);
			return;
		}

		float currentfov = gHUD.GetCurrentFOV();// XDM3037a
		if (gHUD.m_iKeyBits & IN_ATTACK2)
		{
			if (currentfov > m_fFinalFOV)// zooming in
			{
				if (m_iSoundIndex > 0 && m_fPlaySoundTime != 0.0f && m_fPlaySoundTime <= gEngfuncs.GetClientTime())
				{
					/*float df = gHUD.GetUpdatedDefaultFOV();
					float k = (df-g_lastFOV)/(df-m_fFinalFOV);// fov will go from 90 to ~30, k = 0...1
					//conprintf(1, ("CHudZoomCrosshair::Think() k = %f!\n", k);
					EMIT_SOUND(gEngfuncs.GetLocalPlayer()->index, gHUD.m_pLocalPlayer?gHUD.m_pLocalPlayer->curstate.origin:WTF, CHAN_WEAPON, "weapons/xbow_aim.wav", VOL_NORM, ATTN_IDLE, 0, 90+(int)(20.0f*k));
					*/
					PlaySoundByIndex(m_iSoundIndex, VOL_NORM);// XDM3038a: unfortunately, this is the only way to play sound by index. Stupid valve.
					m_fPlaySoundTime = gEngfuncs.GetClientTime() + 0.04f;
				}
				g_lastFOV -= (float)(ZOOM_FOV_DELTA * gHUD.m_flTimeDelta);
#if defined (_DEBUG)
				if (g_lastFOV < 0.0f)
					conprintf(1, "CHudZoomCrosshair::Think() g_lastFOV == %g!\n", g_lastFOV);
#endif
				if (g_lastFOV < m_fFinalFOV)
					g_lastFOV = m_fFinalFOV;// XDM3038b
				gHUD.SetFOV(g_lastFOV);// XDM3037
				gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);// XDM3037a
			}
			else if (g_lastFOV < m_fFinalFOV)// overzoom
			{
				gHUD.SetFOV(m_fFinalFOV);// g_lastFOV is there
			}
			//else
			//	ResetFOV(); <- done on server side
		}
		else // XDM3037a: stop zooming
			m_fFinalFOV = currentfov;

		/*else// XDM3037
		{
			if (g_lastFOV < m_fFinalFOV)// overzoom
				gHUD.SetFOV(m_fFinalFOV);// g_lastFOV is there
		}*/
	}
}
