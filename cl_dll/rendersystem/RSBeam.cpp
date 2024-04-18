#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeam.h"
#include "triangleapi.h"

CRSBeam::CRSBeam(void) : CRSBeam::BaseClass()
{
	DBG_RS_PRINT("CRSBeam()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// UNDONE: this code should eventually replace engine beams
//-----------------------------------------------------------------------------
CRSBeam::CRSBeam(const Vector &start, const Vector &end, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive)
{
	DBG_RS_PRINT("CRSBeam(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = start;
	m_vEnd = end;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_fFrameRate = framerate;
	//m_iFollowEntity = -1;
	m_iRenderMode = r_mode;

	Vector d;
	VectorSubtract(end, start, d);
	float fTexAspect = (m_pTexture->maxs[1] - m_pTexture->mins[1])/(m_pTexture->maxs[2] - m_pTexture->mins[2]);// Texture width/height in texels
	float avgLifeTime;
	if (m_fColorDelta[3] < 0.0f)
		avgLifeTime = (a/*-0*/)/-m_fColorDelta[3];
	else if (m_fColorDelta[3] > 0)
		avgLifeTime = (1.0f - a)/m_fColorDelta[3];
	else
		avgLifeTime = 1.0f;// wtf

	m_fTextureTile = (Length(d) * fTexAspect)/(m_fScale+m_fScaleDelta*avgLifeTime);// initial width may not reflect visible beam width
	//conprintf(1, " m_fTextureTile = %f\n", m_fTextureTile);

	if (m_fTextureTile <= 0)// ???
		m_fTextureTile = 1.0f;// For texture tiling. May (and will) screw the whole effect! (because of valve's strange GL setup?)

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSBeam::ResetAllParameters(void)
{
	CRSBeam::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSBeam::ResetParameters(void)
{
	m_vEnd.Clear();
	m_fTextureTile = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSBeam::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "vEnd") == 0)
		return StringToVec(szValue, m_vEnd);
	else if (strcmp(szKey, "fTextureTile") == 0)
		m_fTextureTile = atof(szValue);
	else
		return CRSBeam::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen.
// Warning: Requires texture tiling enabled in GL!
//-----------------------------------------------------------------------------
void CRSBeam::Render(void)
{
	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	Vector rt, up;
	AngleVectors(g_vecViewAngles, NULL, rt, up);

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	//gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, 255);//(unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],1.0f);// HL m_fColorCurrent[3]);// XDM3038c
	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(0.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin + rt*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin - rt*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vEnd - rt*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(0.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vEnd + rt*m_fScale);

	gEngfuncs.pTriAPI->TexCoord2f(0.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin + up*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin - up*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vEnd - up*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(0.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vEnd + up*m_fScale);

	gEngfuncs.pTriAPI->End();
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
