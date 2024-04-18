#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSSphere.h"
#include "triangleapi.h"

CRSSphere::CRSSphere(void) : CRSSphere::BaseClass()
{
	DBG_RS_PRINT("CRSSphere()");
	ResetParameters();// non-recursive
}

CRSSphere::CRSSphere(const Vector &origin, float radius, float radiusdelta, size_t nhorz, size_t nvert, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive)
{
	DBG_RS_PRINT("CRSSphere(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = origin;
	m_iSegmentsH = nhorz;
	m_iSegmentsV = nvert;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = radius;
	m_fScaleDelta = radiusdelta;
	if (m_fScale < 2)
		m_fScale = 2;
	if (m_iSegmentsH < 6)
		m_iSegmentsH = 6;
	if (m_iSegmentsV < 6)
		m_iSegmentsV = 6;

	m_iRenderMode = r_mode;
	if (timetolive <= 0.0f)// XDM3038a: was <
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSSphere::ResetAllParameters(void)
{
	CRSSphere::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSSphere::ResetParameters(void)
{
	m_iSegmentsH = 0;
	m_iSegmentsV = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSSphere::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "iSegmentsH") == 0)
		m_iSegmentsH = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "iSegmentsV") == 0)
		m_iSegmentsV = strtoul(szValue, NULL, 10);
	else
		return CRSSphere::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: render frame to screen
//-----------------------------------------------------------------------------
void CRSSphere::Render(void)
{
	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	//gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, 255);//(unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],1.0f);// HL m_fColorCurrent[3]);// XDM3038c
	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	//gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);// TRI_TRIANGLE_STRIP?
	size_t i, j;
	Vector vecPos;
	for (i = 0; i < m_iSegmentsH; ++i)
	{
		for (j = 0; j < m_iSegmentsV; ++j)
		{
			// UNDONE: put this all into precalculated array
			float theta = 2.0f * (float)M_PI * ((float)j / (float)(m_iSegmentsV - 1));
			float phi = M_PI * ((float)i / (float)(m_iSegmentsH - 1));
			vecPos.x = sin(phi) * cos(theta);
			vecPos.y = sin(phi) * sin(theta); 
			vecPos.z = cos(phi);
			//Vector vecNormal = vecPos;
			//VectorNormalize(vecNormal);
			VectorMA(m_vecOrigin, m_fScale, vecPos, vecPos);
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
			//gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
			gEngfuncs.pTriAPI->Vertex3fv(vecPos);
			/*particle_t *p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);
			if (!p)
				continue;

			p->org = vecPos;
			VectorClear(p->vel);
			p->color = i + j;
			//p->die = */
			/*xreal=x*radius/sqr(x*x+y*y+z*z)
			yreal=y*radius/sqr(x*x+y*y+z*z)
			zreal=z*radius/sqr(x*x+y*y+z*z)*/
		}
	}
	gEngfuncs.pTriAPI->End();
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
