#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RotatingSystem.h"
#include "triangleapi.h"
#include "studio_util.h"

// UNDONE

CRotatingSystem::CRotatingSystem(void) : CRotatingSystem::BaseClass()
{
	DBG_RS_PRINT("CRotatingSystem()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor for external everyday use.
// Input  : origin - 
//			velocity - 
//			angles - 
//			anglesdelta - 
//			pTexture - 
//			r_mode - 
//			r g b - 
//			a - 
//			adelta - 
//			scale - 
//			scaledelta - 
//			timetolive - 0 means the system removes itself after the last frame
//-----------------------------------------------------------------------------
CRotatingSystem::CRotatingSystem(const Vector &origin, const Vector &velocity, const Vector &angles, const Vector &anglesdelta, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	DBG_RS_PRINT("CRotatingSystem(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = origin;
	m_vecVelocity = velocity;
	m_vecAngles = angles;
	m_vecAnglesDelta = anglesdelta;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
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
/*void CRotatingSystem::ResetAllParameters(void)
{
	CRotatingSystem::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRotatingSystem::ResetParameters(void)
{
	m_vecAnglesDelta.Clear();
	memset(m_fMatrix, 0, sizeof(float)*3*4);
	/*for (short i=0; i<3; ++i)
		for (short j=0; j<4; ++j)
			m_fMatrix[i][j] = 0.0f;*/
}

//-----------------------------------------------------------------------------
// Purpose: Initialize system after all user parameters were set
// Must be called from class constructor or after setting parameters manually.
//-----------------------------------------------------------------------------
void CRotatingSystem::InitializeSystem(void)
{
	CRotatingSystem::BaseClass::InitializeSystem();
	UpdateAngleMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRotatingSystem::Update(const float &time, const double &elapsedTime)
{
	bool ret = CRotatingSystem::BaseClass::Update(time, elapsedTime);

	if (ret == 0)
		UpdateAngles(elapsedTime, true);

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CRotatingSystem::Render(void)
{
	// XDM3037a: OBSOLETE	if (!InitTexture(texindex))
	//		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	//gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, 255);//(unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],1.0f);// HL m_fColorCurrent[3]);// XDM3038c
	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
/*
	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(m_fScale*m_pTexture->mins[1], m_fScale*m_pTexture->mins[2], 0.0f));// - -
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(m_fScale*m_pTexture->mins[1], m_fScale*m_pTexture->maxs[2], 0.0f));// - +
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(m_fScale*m_pTexture->maxs[1], m_fScale*m_pTexture->maxs[2], 0.0f));// + +
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(m_fScale*m_pTexture->maxs[1], m_fScale*m_pTexture->mins[2], 0.0f));// + -
*/
	float sx = m_fScale*m_fSizeX;
	float sy = m_fScale*m_fSizeY;
	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(-sx, -sy, 0.0f));// - -
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(-sx, sy, 0.0f));// - +
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(sx, sy, 0.0f));// + +
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(sx, -sy, 0.0f));// + -

	gEngfuncs.pTriAPI->End();
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate angle matrix using local origin and angles
//-----------------------------------------------------------------------------
void CRotatingSystem::UpdateAngleMatrix(void)
{
	AngleMatrix(m_vecOrigin, m_vecAngles, m_fMatrix);
}

//-----------------------------------------------------------------------------
// Purpose: Convert local RS coordinates into absolute world coordinates
// Input  : local - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CRotatingSystem::LocalToWorld(const Vector &local)
{
	Vector v;
	VectorTransform(local, m_fMatrix, v);
	return v;
}

//-----------------------------------------------------------------------------
// Purpose: Convert local RS coordinates into absolute world coordinates
// Input  : localx - 
//			localy - 
//			localz - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CRotatingSystem::LocalToWorld(float localx, float localy, float localz)
{
	Vector local;
	local[0] = localx;
	local[1] = localy;
	local[2] = localz;
	return LocalToWorld(local);
}

//-----------------------------------------------------------------------------
// Purpose: Update angles
// Input  : timedelta - 
//			updatematrix - 
//-----------------------------------------------------------------------------
void CRotatingSystem::UpdateAngles(const double &timedelta, bool updatematrix/* = TRUE*/)
{
	if (!m_vecAnglesDelta.IsZero())
	{
		VectorMA(m_vecAngles, (float)timedelta, m_vecAnglesDelta, m_vecAngles);
		if (updatematrix)
			UpdateAngleMatrix();
	}
}
