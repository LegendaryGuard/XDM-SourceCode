#include "hud.h"
#include "cl_util.h"
#include "cl_fx.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSBubbles.h"

CPSBubbles::CPSBubbles(void) : CPSBubbles::BaseClass()
{
	DBG_RS_PRINT("CPSBubbles()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : maxParticles - 
//			type - PSSTARTTYPE_*
//			&vector1 - 
//			&vector2 - 
//			velocity - scalar velocity for particles
//-----------------------------------------------------------------------------
CPSBubbles::CPSBubbles(uint32 maxParticles, uint32 type, const Vector &vector1, const Vector &vector2, float velocity, struct model_s *pTexture, int r_mode, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	DBG_RS_PRINT("CPSBubbles(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_iStartType = type;
	if (m_iStartType == PSSTARTTYPE_POINT)
	{
		int contents = gEngfuncs.PM_PointContents(m_vecOrigin, NULL);
		if (contents == CONTENTS_SOLID)// && (m_iFlags & RENDERSYSTEM_FLAG_CLIPREMOVE))
		{
			m_RemoveNow = true;
			return;
		}
		if (m_iDestroyContents != 0 && ContentsArrayHas(m_iDestroyContents, contents))
		{
			m_RemoveNow = true;
			return;
		}
		m_vecOrigin = vector1;
		//m_vStartMins = m_vStartMaxs = vector1;
		m_vSpread = vector2;
	}
	else if (m_iStartType == PSSTARTTYPE_SPHERE)// vector1 is center, vector2.z is radius
	{
		m_vecOrigin = vector1;
		m_vStartMins = vector1;
		m_vStartMins.x -= vector2.z;
		m_vStartMins.y -= vector2.z;
		m_vStartMins.z -= vector2.z;
		m_vStartMaxs = vector1;
		m_vStartMaxs.x += vector2.z;
		m_vStartMaxs.y += vector2.z;
		m_vStartMaxs.z += vector2.z;
		m_fStartRadiusMin = vector2.x;// radius
		m_fStartRadiusMax = vector2.y;// radius
	}
	else if (m_iStartType == PSSTARTTYPE_BOX)
	{
		m_vecOrigin = vector1;
		m_vStartMins = -vector2;
		m_vStartMaxs = vector2;
	}
	else if (m_iStartType == PSSTARTTYPE_LINE)
	{
		m_vecOrigin.Clear();// these are absolute coordinates
		m_vStartMins = vector1;
		m_vStartMaxs = vector2;
	}
	else if (m_iStartType == PSSTARTTYPE_ENTITYBBOX)
	{
		m_vecOrigin = vector1;// should be replaced with entity origin
		m_vStartMins = vector1;
		m_vStartMaxs = vector2;
	}
	else if (m_iStartType == PSSTARTTYPE_CYLINDER)// v2.x - radius, v2.z - half height
	{
		m_vecOrigin = vector1;
		m_vStartMins = vector1 - vector2;// Z is used from these
		m_vStartMaxs = vector1 + vector2;
		m_fStartRadiusMax = m_fStartRadiusMin = vector2.x;// radius
	}
	m_fParticleSpeedMin = velocity * 0.9f;
	m_fParticleSpeedMax = velocity * 1.1f;
	m_iRenderMode = r_mode;
	m_color.a = a*255.0f;
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_fBounceFactor = 0.0f;// stick

	//already m_fEnergyStart = 1.0f;// XDM3037a: to start drawing
	m_iMaxParticles = maxParticles;
	if (m_iMaxParticles == 0)// Don't fail, autodetect
	{
		if (m_iStartType == PSSTARTTYPE_POINT)
			m_iMaxParticles = 8;// ???
		else if (m_iStartType == PSSTARTTYPE_SPHERE)// vector1 is center, vector2.z is radius
			m_iMaxParticles = (4*M_PI*vector2.z*vector2.z*vector2.z/3)/(BUBLES_DEFAULT_DENSITY*BUBLES_DEFAULT_DENSITY*BUBLES_DEFAULT_DENSITY);// volume / particle volume
		else if (m_iStartType == PSSTARTTYPE_BOX)// vector1 is center, vector2 is quarter box
			m_iMaxParticles = vector2.Volume()/(BUBLES_DEFAULT_DENSITY*BUBLES_DEFAULT_DENSITY*BUBLES_DEFAULT_DENSITY);
		else if (m_iStartType == PSSTARTTYPE_LINE)// vector1 is start, vector2 is end
			m_iMaxParticles = (vector2-vector1).Length()/BUBLES_DEFAULT_DENSITY;
		/*else if (m_iStartType == PSSTARTTYPE_ENTITYBBOX)
			we cannot initialize system without knowing entity size */
		else if (m_iStartType == PSSTARTTYPE_CYLINDER)// vector1 is bottom center, v2.x - radius, v2.z - height
			m_iMaxParticles = (M_PI*vector2.x*vector2.x*vector2.z)/(BUBLES_DEFAULT_DENSITY*BUBLES_DEFAULT_DENSITY*BUBLES_DEFAULT_DENSITY);// volume / particle volume
		else
		{
			m_RemoveNow = true;
			return;
		}
		DBG_PRINTF("CPSBubbles(): m_iMaxParticles automatically set to %u\n", m_iMaxParticles);
	}

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSBubbles::ResetAllParameters(void)
{
	CPSBubbles::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSBubbles::ResetParameters(void)
{
	m_color.SetWhite();
	m_fBounceFactor = 0.0f;// don't bounce
	SetBits(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_SIMULTANEOUS|RENDERSYSTEM_FLAG_ADDGRAVITY);// obsolete |RENDERSYSTEM_FLAG_INCONTENTSONLY;
	m_iTraceFlags = PM_WORLD_ONLY | PM_STUDIO_IGNORE;
	m_iMovementType = PSMOVTYPE_DIRECTED;
	m_fParticleWeight = -0.1f;// a little hack since we don't have bouyancy for particles
	ContentsArrayAdd(m_iDestroyContents, CONTENTS_EMPTY);
	ContentsArrayAdd(m_iDestroyContents, CONTENTS_SOLID);
	ContentsArrayAdd(m_iDestroyContents, CONTENTS_SKY);
	ContentsArrayAdd(m_iDrawContents, CONTENTS_WATER);
	ContentsArrayAdd(m_iDrawContents, CONTENTS_SLIME);
	ContentsArrayAdd(m_iDrawContents, CONTENTS_LAVA);
	m_vDirection.Set(0,0,1);
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : index - particle index in array
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSBubbles::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	CParticleSystem::InitializeParticle(pParticle, fInterpolaitonMult);
	//pParticle->m_vVel.z += FX_GetBubbleSpeed();
	pParticle->m_vAccel.Set(0.0f, 0.0f, 1.0f);
	float r = RANDOM_FLOAT(0.25f, 1.0f);
	pParticle->m_fSizeX *= r;
	pParticle->m_fSizeY *= r;
}
