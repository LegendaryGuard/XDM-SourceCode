//====================================================================
//
// Purpose: Render System: particles.
// This code affects performance thousands times, optimization is critical!
//
//====================================================================

#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "triangleapi.h"
#ifndef COM_MODEL_H
#include "com_model.h"
#endif

// NOTE: This code is extremely performance-critical! Think 8 times before adding something!

//-----------------------------------------------------------------------------
// Purpose: Default constructor, no need for others
//-----------------------------------------------------------------------------
CParticle::CParticle()
{
//NO! this destroys function pointers!	memset(this,0,sizeof(CParticle));
	m_vPos.Clear();
	m_vPosPrev.Clear();
	m_vVel.Clear();
	m_vAccel.Clear();
	m_fEnergy = 0.0f;
	m_fSizeX = 1.0f;
	m_fSizeY = 1.0f;
	m_fSizeDelta = 0.0f;
#if defined (_DEBUG)
	m_fColor[0] = 1.0f;
	m_fColor[1] = 1.0f;
	m_fColor[2] = 1.0f;
	m_fColor[3] = 1.0f;
	m_fColorDelta[0] = 0.0f;
	m_fColorDelta[1] = 0.0f;
	m_fColorDelta[2] = 0.0f;
	m_fColorDelta[3] = 0.0f;
#endif
	m_iFlags = 0;
	m_iFrame = 0;
	m_pTexture = NULL;
	index = UINT_MAX;// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CParticle::~CParticle()
{
#if defined (_DEBUG)
	m_vPos.Clear();
	/*m_vPosPrev.Clear();
	m_vVel.Clear();
	m_vVelAdd.Clear();
	m_vAccel.Clear();
	//m_vAngles.Clear();
	m_fEnergy = 0.0;
	m_fSizeX = 0.0;
	m_fSizeY = 0.0;
	m_fSizeDelta = 0.0;
	//m_weight = 0.0;
	//m_weightDelta = 0.0;
	m_iFrame = 0;*/
	m_iFlags = 0;
	m_pTexture = NULL;
	index = UINT_MAX;// XDM3037
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Is this particle free or allocated by system?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CParticle::IsFree(void) const
{
	return (index == UINT_MAX);
}

//-----------------------------------------------------------------------------
// Purpose: Has this particle done its purpose and ready to be freed?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CParticle::IsExpired(void) const
{
	return (m_fEnergy <= 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Update color and brightness depending on m_fSizeDelta
// Warning: Fasterize!
//-----------------------------------------------------------------------------
void CParticle::UpdateColor(const float &elapsed_time)
{
	m_fColor[0] += m_fColorDelta[0] * elapsed_time;
	if (m_fColor[0] < 0.0f) m_fColor[0] = 0.0f;// clamp for software renderer
	m_fColor[1] += m_fColorDelta[1] * elapsed_time;
	if (m_fColor[1] < 0.0f) m_fColor[1] = 0.0f;
	m_fColor[2] += m_fColorDelta[2] * elapsed_time;
	if (m_fColor[2] < 0.0f) m_fColor[2] = 0.0f;
	m_fColor[3] += m_fColorDelta[3] * elapsed_time;
	if (m_fColor[3] < 0.0f) m_fColor[3] = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Update size depending on m_fSizeDelta
//-----------------------------------------------------------------------------
void CParticle::UpdateSize(const float &elapsed_time)
{
	m_fSizeX += m_fSizeDelta * elapsed_time;
	m_fSizeY += m_fSizeDelta * elapsed_time;
}

//-----------------------------------------------------------------------------
// Purpose: Carefully update energy depending on m_fColor[3]
//-----------------------------------------------------------------------------
void CParticle::UpdateEnergyByBrightness(void)
{
	if (m_fColorDelta[3] < 0.0f)
		m_fEnergy = m_fColor[3];// fade out: erase when become invisible
	else if (m_fColorDelta[3] > 0.0f)
		m_fEnergy = max(0.0f, 1.0f-m_fColor[3]);// fade in: erase when fully visible?
	//else 0
}

//-----------------------------------------------------------------------------
// Purpose: Default particle render procedure. Basically, a particle gets drawn
//  as a sprite.
// Input  : &rt - AngleVectors() outputs - right and
//			&up - up
//			rendermode - kRenderTransAdd
//			doubleside - draw both front and back sides (useful for faces not
//						aligned parallel to the screen
//-----------------------------------------------------------------------------
#if !defined (RS_SORTED_PARTICLES_RENDER)
void CParticle::Render(const Vector &rt, const Vector &up, const int &rendermode, const bool &doubleside)
{
	if (m_pTexture == NULL)
		return;

	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
	{
		gEngfuncs.pTriAPI->RenderMode(rendermode);
		gEngfuncs.pTriAPI->CullFace(doubleside == false?TRI_FRONT:TRI_NONE);// TRI_NONE - two-sided
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		gEngfuncs.pTriAPI->Color4f(m_fColor[0], m_fColor[1], m_fColor[2], m_fColor[3]);

		if (rendermode != kRenderTransAlpha)// sprites like smoke need special care // TODO: revisit
			gEngfuncs.pTriAPI->Brightness(m_fColor[3]);

		Vector rs(rt);// = rt * m_fSizeX;
		rs *= m_fSizeX;
		Vector us(up);// = up * m_fSizeY;
		us *= m_fSizeY;

		// really stupid thing: when drawing with proper coords order, face is reversed
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos + rs + us);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos + rs - us);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos - rs - us);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos - rs + us);

		gEngfuncs.pTriAPI->End();
		//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}
}
#else
void CParticle::Render(const Vector &rt, const Vector &up, const int &rendermode, const bool &doubleside)
{
	if (RenderBegin(rendermode, doubleside))
	{
		RenderWrite(rt, up, rendermode);
		RenderEnd();
	}
}

bool CParticle::RenderBegin(const int &rendermode, const bool &doubleside)
{
	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
	{
		gEngfuncs.pTriAPI->RenderMode(rendermode);
		gEngfuncs.pTriAPI->CullFace(doubleside == false?TRI_FRONT:TRI_NONE);// TRI_NONE - two-sided
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		return true;
	}
	return false;
}

void CParticle::RenderWrite(const Vector &rt, const Vector &up, const int &rendermode)
{
	gEngfuncs.pTriAPI->Color4f(m_fColor[0], m_fColor[1], m_fColor[2], m_fColor[3]);

	if (rendermode != kRenderTransAlpha)// sprites like smoke need special care // TODO: revisit
		gEngfuncs.pTriAPI->Brightness(m_fColor[3]);

	Vector rs(rt);// = rt * m_fSizeX;
	rs *= m_fSizeX;
	Vector us(up);// = up * m_fSizeY;
	us *= m_fSizeY;

	// really stupid thing: when drawing with proper coords order, face is reversed
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3fv(m_vPos + rs + us);
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3fv(m_vPos + rs - us);
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3fv(m_vPos - rs - us);
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3fv(m_vPos - rs + us);
}

void CParticle::RenderEnd(void)
{
	gEngfuncs.pTriAPI->End();
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Wrapped frame++
// Output : Returns true if last frame is displayed
//-----------------------------------------------------------------------------
bool CParticle::FrameIncrease(void)
{
	if (m_pTexture == NULL || m_pTexture->numframes <= 1)
		return false;

	if (m_iFrame < m_pTexture->numframes - 1)
	{
		++m_iFrame;
		if (m_iFrame == m_pTexture->numframes - 1)
			return true;
	}
	else
		m_iFrame = 0;//-= maxframes;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Set current frame to random possible value
//-----------------------------------------------------------------------------
void CParticle::FrameRandomize(void)
{
	if (m_pTexture == NULL || m_pTexture->numframes <= 1)
		return;

	m_iFrame = RANDOM_LONG(0, m_pTexture->numframes - 1);
}

//-----------------------------------------------------------------------------
// Purpose: An easy way to reset color
//-----------------------------------------------------------------------------
void CParticle::SetDefaultColor(void)
{
	m_fColor[0] = 1.0f;
	m_fColor[1] = 1.0f;
	m_fColor[2] = 1.0f;
	m_fColor[3] = 1.0f;
	m_fColorDelta[0] = 0.0f;
	m_fColorDelta[1] = 0.0f;
	m_fColorDelta[2] = 0.0f;
	m_fColorDelta[3] = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Set render color (4 floats)
// Input  : &rgba - floats (0-1)
//-----------------------------------------------------------------------------
void CParticle::SetColor(const float rgba[4])
{
	m_fColor[0] = rgba[0];
	m_fColor[1] = rgba[1];
	m_fColor[2] = rgba[2];
	m_fColor[3] = rgba[3];
}

//-----------------------------------------------------------------------------
// Purpose: Set render color (4 bytes)
// Input  : &rgba - bytes (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColor(const ::Color &rgba)
{
	m_fColor[0] = (float)rgba.r/255.0f;
	m_fColor[1] = (float)rgba.g/255.0f;
	m_fColor[2] = (float)rgba.b/255.0f;
	m_fColor[3] = (float)rgba.a/255.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Set render color (3 bytes + 1 float)
// Input  : &rgb - bytes (0-255)
//			a - byte (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColor(const ::Color &rgb, const float &a)
{
	m_fColor[0] = (float)rgb.r/255.0f;
	m_fColor[1] = (float)rgb.g/255.0f;
	m_fColor[2] = (float)rgb.b/255.0f;
	m_fColor[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Set render color
// Input  : &rgb - bytes (0-255)
//			a - byte (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColor(const color24 &rgb, const float &a)
{
	m_fColor[0] = (float)rgb.r/255.0f;
	m_fColor[1] = (float)rgb.g/255.0f;
	m_fColor[2] = (float)rgb.b/255.0f;
	m_fColor[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Color will be updated with  += m_fColorDelta * frametime
// Input  : &rgb - bytes (0-255)
//			a - byte (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColorDelta(const ::Color &rgb, const float &a)
{
	m_fColorDelta[0] = (float)rgb.r/255.0f;
	m_fColorDelta[1] = (float)rgb.g/255.0f;
	m_fColorDelta[2] = (float)rgb.b/255.0f;
	m_fColorDelta[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Color will be updated with  += m_fColorDelta * frametime
// Input  : &rgb - bytes (0-255)
//			a - byte (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColorDelta(const color24 &rgb, const float &a)
{
	m_fColorDelta[0] = (float)rgb.r/255.0f;
	m_fColorDelta[1] = (float)rgb.g/255.0f;
	m_fColorDelta[2] = (float)rgb.b/255.0f;
	m_fColorDelta[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Color will be updated with  += m_fColorDelta * frametime
// Input  : *rgb - floats (0-1)
//			a - float (0-1)
//-----------------------------------------------------------------------------
void CParticle::SetColorDelta(const float *rgb, const float &a)
{
	m_fColorDelta[0] = rgb[0];
	m_fColorDelta[1] = rgb[1];
	m_fColorDelta[2] = rgb[2];
	m_fColorDelta[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Color will be updated with  += m_fColorDelta * frametime
// Input  : *rgb - floats (0-1)
//			a - float (0-1)
//-----------------------------------------------------------------------------
void CParticle::SetColorDelta(const float *rgba)
{
	m_fColorDelta[0] = rgba[0];
	m_fColorDelta[1] = rgba[1];
	m_fColorDelta[2] = rgba[2];
	m_fColorDelta[3] = rgba[3];
}

//-----------------------------------------------------------------------------
// Purpose: Used to display sprite using its texture size, so the bitmap always has 1:1 scale
// Input  : multipl_x - post-multipliers
//			multipl_y - 
//-----------------------------------------------------------------------------
void CParticle::SetSizeFromTexture(const float &multipl_x, const float &multipl_y)
{
	if (m_pTexture)
	{
		m_fSizeX = (m_pTexture->maxs[1] - m_pTexture->mins[1])*multipl_x;
		m_fSizeY = (m_pTexture->maxs[2] - m_pTexture->mins[2])*multipl_y;
	}
	else
	{
		conprintf(1, "CParticle::SetSizeFromTexture(%g %g) called without texture!\n", multipl_x, multipl_y);
		DBG_FORCEBREAK
	}
}
