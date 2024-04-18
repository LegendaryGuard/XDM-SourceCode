#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeamStar.h"
#include "triangleapi.h"
#include "studio_util.h"// M_PI
#include "cl_fx.h"

CRSBeamStar::CRSBeamStar(void) : CRSBeamStar::BaseClass()
{
	DBG_RS_PRINT("CRSBeamStar()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CRSBeamStar::CRSBeamStar(const Vector &origin, struct model_s *pTexture, size_t number, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	DBG_RS_PRINT("CRSBeamStar(...)");
	ResetParameters();
	if (number <= 0 || !InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_iCount = number;
	m_vecOrigin = origin;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;
	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSBeamStar::~CRSBeamStar(void)
{
	DBG_RS_PRINT("~CRSBeamStar()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSBeamStar::ResetAllParameters(void)
{
	CRSBeamStar::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSBeamStar::ResetParameters(void)
{
	m_ang1 = NULL;
	m_ang2 = NULL;
	m_Coords = NULL;
	m_iCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize system after all user parameters were set
// Must be called from class constructor or after setting parameters manually.
//-----------------------------------------------------------------------------
void CRSBeamStar::InitializeSystem(void)
{
	if (m_iCount > 0)
	{
		ASSERT(m_ang1 == NULL);
		ASSERT(m_ang2 == NULL);
		ASSERT(m_Coords == NULL);
		CRSBeamStar::BaseClass::InitializeSystem();
		m_ang1 = new float[m_iCount];
		m_ang2 = new float[m_iCount];
		m_Coords = new Vector[m_iCount];
		size_t i;
		for (i=0; i<m_iCount; ++i)
		{
			m_ang1[i] = RANDOM_FLOAT(-M_PI, M_PI);
			m_ang2[i] = RANDOM_FLOAT(-M_PI, M_PI);
			m_Coords[i].Clear();
		}
	}
	else
	{
		conprintf(0, "CRSBeamStar::InitializeSystem() ERROR: m_iCount == 0!\n");
		m_RemoveNow = true;
		DBG_FORCEBREAK
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSBeamStar::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSBeamStar::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSBeamStar::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	if (m_Coords)
	{
		delete [] m_Coords;
		m_Coords = NULL;
	}
	if (m_ang1)
	{
		delete [] m_ang1;
		m_ang1 = NULL;
	}
	if (m_ang2)
	{
		delete [] m_ang2;
		m_ang2 = NULL;
	}
	m_iCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSBeamStar::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "iCount") == 0)
		m_iCount = strtoul(szValue, NULL, 10);
	else
		return CRSBeamStar::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
//			This function may not be called every frame by the engine.
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRSBeamStar::Update(const float &time, const double &elapsedTime)
{
	if (CRSBeamStar::BaseClass::Update(time, elapsedTime) == 0)
	{
		if (m_fScale <= 0.000001 && m_fScaleDelta < 0.0f)// special fix for dark teleporter effect
		{
			ShutdownSystem();
		}
		else
		{
			float s1, s2, c1, c2;
			size_t i;
			for (i=0; i<m_iCount; ++i)
			{
				SinCos(time + m_ang1[i], &s1, &c1);
				SinCos(time + m_ang2[i], &s2, &c2);
				m_Coords[i][0] = c1*c2*m_fScale;
				m_Coords[i][1] = s1*c2*m_fScale;
				m_Coords[i][2] = -s2*m_fScale;
			}
		}
		//conprintf(1, "CRSBeamStar::Update(): m_fScale = %f, m_fBrightness = %f, dying = %d\n", m_fScale, m_fBrightness, IsShuttingDown());
	}
	return IsShuttingDown();
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CRSBeamStar::Render(void)
{
	if (!m_pTexture)
		return;

	if (!PointIsVisible(m_vecOrigin))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	//m_vecAngles = g_vecViewAngles;

	Vector v_forward;
	AngleVectors(g_vecViewAngles, v_forward, NULL, NULL);

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	//gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],m_fColorCurrent[3]);// XDM3038c: normally we supply 1.0 alpha here (HL), but this system looks ugly with that
	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());// XDM3038c
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);
	//gEngfuncs.pTriAPI->Begin(TRI_LINES);
	//gEngfuncs.pTriAPI->TexCoord2f(0.5, 0.99998);// hack!
	//gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin);

	Vector pos, cross;
	size_t i;
	for (i=0; i<m_iCount; ++i)
	{
		CrossProduct(m_Coords[i], v_forward, cross);
		//VectorAdd(m_vecOrigin, m_Coords[i], pos);
		pos = m_vecOrigin;
		pos += m_Coords[i];

		// debug	DrawParticle(pos, 208, 0);

		gEngfuncs.pTriAPI->TexCoord2f(0.5f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin);

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pos + cross);

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pos - cross);
	}

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
