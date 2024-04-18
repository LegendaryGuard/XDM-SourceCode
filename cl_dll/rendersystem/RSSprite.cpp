#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSSprite.h"

CRSSprite::CRSSprite(void) : CRSSprite::BaseClass()
{
	DBG_RS_PRINT("CRSSprite()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Sprite. 
// Input  : origin - 
//			velocity - 
//			pTexture - sprite to use
//			r_mode - kRenderGlow does not work (impossible)
//			r g b - 
//			a - this value has no effect with kRenderTransAlpha (GoldSource)
//			adelta - 
//			scale - 
//			scaledelta - 
//			framerate - 
//			timetolive - 0 means the system removes itself after the last frame
// Accepts flags: RENDERSYSTEM_FLAG_RANDOMFRAME | LOOPFRAMES | etc.
//-----------------------------------------------------------------------------
CRSSprite::CRSSprite(const Vector &origin, const Vector &velocity, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive)
{
	DBG_RS_PRINT("CRSSprite(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_ShuttingDown = true;
		m_RemoveNow = true;
		return;
	}
	m_iFollowFlags |= RENDERSYSTEM_FFLAG_NOANGLES;// XDM3037: sprite should always be aligned to camera
	m_vecOrigin = origin;
	m_vecVelocity = velocity;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;
	m_fFrameRate = framerate;
	m_vecAngles.x = g_vecViewAngles.x + ev_punchangle.x;
	m_vecAngles.y = g_vecViewAngles.y + ev_punchangle.y;
	//m_vecAngles.z = 0.0f;// keep Z

	if (m_iRenderMode == kRenderGlow)// XDM3035a: HACK: TriAPI does not support glow mode
		m_iRenderMode = kRenderTransAdd;

	if (timetolive <= 0.0f)// if 0, just display all frames
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CRSSprite::Render(void)
{
	// XDM3037: draw oriented sprites properly. UNDONE: other types (requires setting and keeping m_vecAngles from the very beginning)

	//crash msprite_t *psprite1 = (msprite_t *)IEngineStudio.Mod_Extradata(m_pTexture);
	msprite_t *psprite = (msprite_t *)m_pTexture->cache.data;
	//ASSERT(psprite1->frames == psprite->frames);
	if ((m_iFlags & RENDERSYSTEM_FLAG_ZROTATION) || (psprite && psprite->type == SPR_VP_PARALLEL_UPRIGHT))
	{
		/*UNDONE	v_up.Set(0.0f, 0.0f, 1.0f);
		v_right.Set(RI.vforward[1], -RI.vforward[0], 0.0f);
		v_right.NormalizeSelf();*/
		m_vecAngles.x = 0;
		//m_vecAngles.x = (g_vecViewAngles.x + ev_punchangle.x)*test1->value;
		m_vecAngles.y = (g_vecViewAngles.y + ev_punchangle.y);//*test2->value;
		//m_vecAngles.z = (g_vecViewAngles.z + ev_punchangle.z)*test3->value;
		m_vecAngles.z = 0;
	}
	else if (psprite && psprite->type == SPR_FACING_UPRIGHT)
	{
	/*UNDONE	v_right.Set(origin[1] - RI.vieworg[1], -(origin[0] - RI.vieworg[0]), 0.0f);
		v_right.NormalizeSelf();
		v_up.Clear();*/
	}
	else if (psprite && psprite->type == SPR_ORIENTED)
	{
		// keep angles
	}
	/*UNDONE	else if (psprite && psprite->type == SPR_VP_PARALLEL_ORIENTED)
	{
		float sr, cr;
		SinCos(m_vecAngles[ROLL] * (M_PI/180.0f), &sr, &cr);
		for (i = 0; i < 3; ++i)
		{
			v_right[i] = RI.vright[i] * cr + RI.vup[i] * sr;
			v_up[i] = RI.vright[i] * -sr + RI.vup[i] * cr;
		}
	}*/
	else //if (psprite && psprite->type == SPR_VP_PARALLEL)
	{
		m_vecAngles.x = g_vecViewAngles.x + ev_punchangle.x;
		m_vecAngles.y = g_vecViewAngles.y + ev_punchangle.y;
		//m_vecAngles.z = g_vecViewAngles.z + ev_punchangle.z;// keep Z
	}
	CRenderSystem::Render();
}
