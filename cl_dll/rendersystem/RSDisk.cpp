#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RotatingSystem.h"
#include "RSDisk.h"
#include "triangleapi.h"
#include "studio_util.h"// M_PI

// TODO: calculate all vertexes one time and store them in an array!

CRSDisk::CRSDisk(void) : CRSDisk::BaseClass()
{
	DBG_RS_PRINT("CRSDisk()");
	ResetParameters();// non-recursive
}

CRSDisk::CRSDisk(const Vector &origin, const Vector &angles, const Vector &anglesdelta, float radius, float radiusdelta, size_t segments, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive)
{
	DBG_RS_PRINT("CRSDisk(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = origin;
	m_vecAngles = angles;
	m_vecAnglesDelta = anglesdelta;
	m_fScale = radius;
	m_fScaleDelta = radiusdelta;
	m_iSegments = segments;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_iRenderMode = r_mode;

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSDisk::ResetAllParameters(void)
{
	CRSDisk::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSDisk::ResetParameters(void)
{
	m_iDoubleSided = true;// XDM3037
	m_fAngleDelta = 0.0f;
	m_fTexDelta = 0.0f;
	m_iSegments = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize system after all user parameters were set
// Must be called from class constructor or after setting parameters manually.
//-----------------------------------------------------------------------------
void CRSDisk::InitializeSystem(void)
{
	CRSDisk::BaseClass::InitializeSystem();

	if (m_fScale < 2.0f)
		m_fScale = 2.0f;

	if (m_iSegments < 6)
		m_iSegments = 6;

	// precalculations
	m_fAngleDelta = ((float)M_PI*2.0f/m_iSegments);// angle step
	m_fTexDelta = 1.0f/m_iSegments;// texture vertical step
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSDisk::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "iSegments") == 0)
		m_iSegments = strtoul(szValue, NULL, 10);// nothing is allocated so it's ok to change
	else if (strcmp(szKey, "fAngleDelta") == 0)
		m_fAngleDelta = atof(szValue);
	else if (strcmp(szKey, "fTexDelta") == 0)
		m_fTexDelta = atof(szValue);
	else
		return CRSDisk::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRSDisk::Render(void)
{
	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	//gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, 255);//(unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],1.0f);// HL m_fColorCurrent[3]);// XDM3038c
	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());
	gEngfuncs.pTriAPI->CullFace(m_iDoubleSided?TRI_NONE:TRI_FRONT);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);
	float a = 0.0f, s = 0.0f, c = 0.0f;
	float v = 0.0f;
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, v);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin);// should be same as LocalToWorld(Vector(0,0,0))
	Vector p(0,0,0);// current vertex
	size_t i = 0;
	for (i = 0; i <= m_iSegments; ++i)// repeat first vertex twice to finish the disk
	{
		SinCos(a, &s, &c);
		p[0] = s*m_fScale;//p[0] = s*m_fScale + m_vecOrigin[0];
		p[1] = c*m_fScale;//p[1] = c*m_fScale + m_vecOrigin[1];
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v);
		gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(p));
		a += m_fAngleDelta;
		v += m_fTexDelta;
	}
	gEngfuncs.pTriAPI->End();
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
