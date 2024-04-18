#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSCylinder.h"
#include "triangleapi.h"

CRSCylinder::CRSCylinder(void) : CRSCylinder::BaseClass()
{
	DBG_RS_PRINT("CRSCylinder()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor.
// Input  : origin - center
//			radius - 
//			radiusdelta - 
//			width - actually height
//			segments - nothing is truly round in CG
//-----------------------------------------------------------------------------
CRSCylinder::CRSCylinder(const Vector &origin, float radius, float radiusdelta, float width, size_t segments, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive)
{
	DBG_RS_PRINT("CRSCylinder(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = origin;
	m_fScale = radius;
	m_fScaleDelta = radiusdelta;
	m_fWidth = width;
	//m_fWidthDelta = 0.0f;
	m_iSegments = segments;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	if (m_fScale < 2.0f)
		m_fScale = 2.0f;
	if (m_iSegments < 6)
		m_iSegments = 6;

	//int iTexY = (m_pTexture->maxs[2] - m_pTexture->mins[2]);// height
	//float fNumTextureTiles = fCircleLen/iTexY;// we assume 1 texel per 1 world unit
	//m_fTextureStep = fNumTextureTiles/(float)m_usSegments;

/*undone	float fTexAspect = (m_pTexture->maxs[1] - m_pTexture->mins[1])/iTexY;// Texture width/height in texels
	m_fTextureStep = ((fCircleLen * fTexAspect)/(m_fWidth)
		/(m_pTexture->maxs[2] - m_pTexture->mins[2]))
		/(float)m_usSegments;// initial width may not reflect visible beam width
*/
	m_iFollowEntity = -1;
	m_iRenderMode = r_mode;

	if (timetolive <= 0.0f)// XDM3038a: was <
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSCylinder::~CRSCylinder(void)
{
	DBG_RS_PRINT("~CRSCylinder()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSCylinder::ResetAllParameters(void)
{
	CRSCylinder::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSCylinder::ResetParameters(void)
{
	m_iDoubleSided = true;// XDM3037
	m_fWidth = 0.0f;
	m_fWidthDelta = 0.0f;// XDM3037
	m_fTextureStart = 0.0f;// XDM3037a
	m_fTextureScrollRate = 0.0f;// XDM3037a
	m_fTextureStep = 0.25f;// XDM3037a
	m_iSegments = 0;
	m_pv2dPoints = NULL;// dangerous!
}

//-----------------------------------------------------------------------------
// Purpose: Initialize system after all user parameters were set
// Must be called from class constructor or after setting parameters manually.
//-----------------------------------------------------------------------------
void CRSCylinder::InitializeSystem(void)
{
	CRSCylinder::BaseClass::InitializeSystem();

	float fCircleLen = M_PI*m_fScale*2.0f;// C = Pi*D
	float fTexAspect = (m_pTexture->maxs[1] - m_pTexture->mins[1])/(m_pTexture->maxs[2] - m_pTexture->mins[2]);// Texture width/height in texels
	float fNumTextureTiles = (fCircleLen * fTexAspect)/(m_fWidth+m_fWidthDelta);// initial width may not reflect visible beam width
	m_fTextureStep = fNumTextureTiles/(float)m_iSegments;
	float angle = 0.0f;
	float step = (M_PI_F*2)/(float)m_iSegments;
	m_pv2dPoints = new Vector2D[m_iSegments];
	for (unsigned short i=0; i<m_iSegments; ++i)
	{
		SinCos(angle, &m_pv2dPoints[i].x, &m_pv2dPoints[i].y);
		angle += step;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSCylinder::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSCylinder::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSCylinder::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	if (m_pv2dPoints)
	{
		delete [] m_pv2dPoints;
		m_pv2dPoints = NULL;
	}
	m_iSegments = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSCylinder::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "iSegments") == 0)
	{
		if (/*m_iSegments == 0 &&*/m_pv2dPoints == NULL)
		{
			m_iSegments = strtoul(szValue, NULL, 10);
		}
		else
		{
			conprintf(0, "ERROR: %s %u: tried to re-initialize already allocated value %s (%u)!\n", GetClassName(), GetIndex(), szKey, m_iSegments);
			return false;
		}
	}
	else if (strcmp(szKey, "fWidth") == 0)
		m_fWidth = atof(szValue);
	else if (strcmp(szKey, "fWidthDelta") == 0)
		m_fWidthDelta = atof(szValue);
	else if (strcmp(szKey, "fTextureStart") == 0)
		m_fTextureStart = atof(szValue);
	else if (strcmp(szKey, "fTextureScrollRate") == 0)
		m_fTextureScrollRate = atof(szValue);
	else if (strcmp(szKey, "fTextureStep") == 0)
		m_fTextureStep = atof(szValue);
	else
		return CRSCylinder::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSCylinder::Update(const float &time, const double &elapsedTime)
{
	if (CRSCylinder::BaseClass::Update(time, elapsedTime) == 0)
		m_fWidth += m_fWidthDelta * elapsedTime;// XDM3037

	m_fTextureStart += m_fTextureScrollRate * elapsedTime;// XDM3037a
	//NormalizeValueF(m_fTextureStart, 0.0, 1.0);
	if (m_fTextureStart > 1.0)
		m_fTextureStart -= (int)m_fTextureStart;
	else if (m_fTextureStart < 0.0)
		m_fTextureStart = 1.0f - (m_fTextureStart - (int)m_fTextureStart);

	return IsShuttingDown();
}

//-----------------------------------------------------------------------------
// Purpose: Render
// Warning: Requires texture tiling enabled in GL!
//-----------------------------------------------------------------------------
void CRSCylinder::Render(void)
{
	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	//gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, 255);//(unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],1.0f);// HL m_fColorCurrent[3]);// XDM3038c
	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());
	gEngfuncs.pTriAPI->CullFace(m_iDoubleSided?TRI_NONE:TRI_FRONT);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	float h = m_fWidth/2.0f;
	float x = 0.0f, y = 0.0f;
	float v = m_fTextureStart;
	//float vs = 0.25f;// step
	size_t i2;
	for (size_t i=0; i<m_iSegments; ++i)
	{
		x = m_vecOrigin.x + m_pv2dPoints[i].x*m_fScale;
		y = m_vecOrigin.y + m_pv2dPoints[i].y*m_fScale;

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, v);
		gEngfuncs.pTriAPI->Vertex3f(x, y, m_vecOrigin.z+h);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v);// exchange these to rotate by 90
		gEngfuncs.pTriAPI->Vertex3f(x, y, m_vecOrigin.z-h);
		//gEngfuncs.pEfxAPI->R_ShowLine(Vector(x,y,m_vecOrigin.z+h), Vector(x,y,m_vecOrigin.z-h));

		i2 = (i<m_iSegments-1)?(i+1):0;// warp
		x = m_vecOrigin.x + m_pv2dPoints[i2].x*m_fScale;
		y = m_vecOrigin.y + m_pv2dPoints[i2].y*m_fScale;

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v+m_fTextureStep);
		gEngfuncs.pTriAPI->Vertex3f(x, y, m_vecOrigin.z-h);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, v+m_fTextureStep);// exchange these to rotate by 90
		gEngfuncs.pTriAPI->Vertex3f(x, y, m_vecOrigin.z+h);
		//gEngfuncs.pEfxAPI->R_ShowLine(Vector(x,y,m_vecOrigin.z+h), Vector(x,y,m_vecOrigin.z-h));

		v += m_fTextureStep;
		//conprint("i=%d\n", i);
	}

	/*float h = m_fWidth/2.0f;
	float step = ((float)M_PI*2.0f)/m_usSegments;
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
	float v = 0.0f;
	float vs = 0.25f;
	for (float a = 0.0f; a < M_PI*2.0f; a += step)
	{
		SinCos(a, &x1, &y1);
		x1 = x1*m_fScale + m_vecOrigin[0];
		y1 = y1*m_fScale + m_vecOrigin[1];
		SinCos(a + step, &x2, &y2);
		x2 = x2*m_fScale + m_vecOrigin[0];
		y2 = y2*m_fScale + m_vecOrigin[1];
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, v);
		gEngfuncs.pTriAPI->Vertex3f(x1, y1, m_vecOrigin[2]+h);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v);// exchange these to rotate by 90
		gEngfuncs.pTriAPI->Vertex3f(x1, y1, m_vecOrigin[2]-h);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v+vs);
		gEngfuncs.pTriAPI->Vertex3f(x2, y2, m_vecOrigin[2]-h);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, v+vs);// exchange these to rotate by 90
		gEngfuncs.pTriAPI->Vertex3f(x2, y2, m_vecOrigin[2]+h);
		v += vs;
	}*/
	gEngfuncs.pTriAPI->End();
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
