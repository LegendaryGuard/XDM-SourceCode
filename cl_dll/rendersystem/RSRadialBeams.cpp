#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSRadialBeams.h"
#include "r_efx.h"
#include <new>
#ifdef USE_EXCEPTIONS
#include <exception>
#endif

CRSRadialBeams::CRSRadialBeams(void) : CRSRadialBeams::BaseClass()
{
	DBG_RS_PRINT("CRSRadialBeams()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Creates randomly directed beams around target entity
//-----------------------------------------------------------------------------
CRSRadialBeams::CRSRadialBeams(int modelIndex, const Vector &origin, float radius, size_t numbeams, uint32 color4b, float beamlifemin, float beamlifemax, float widthmin, float widthmax, float amplitude, float speed, float framerate, int beamflags, float timetolive)
{
	DBG_RS_PRINT("CRSRadialBeams(...)");
	ResetParameters();
	m_iBeamTextureIndex = modelIndex;
	if (radius <= 0.0f || numbeams == 0 || !InitTexture(IEngineStudio.GetModelByIndex(m_iBeamTextureIndex)))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = origin;
	m_pTexture = NULL;
	m_color.Set(color4b);
	//old m_fBrightness = (float)m_color.a/255.0f;
	m_fScale = radius;
	m_iNumBeams = numbeams;
	m_fBeamLifeMin = beamlifemin;
	m_fBeamLifeMax = beamlifemax;
	m_fBeamWidthMin = widthmin;
	m_fBeamWidthMax = widthmax;
	m_fBeamAmplitude = amplitude;
	m_fBeamSpeed = speed;
	m_fFrameRate = framerate;
	m_iBeamFlags = beamflags & BEAM_FLAGS_EFFECTS_MASK;
	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSRadialBeams::~CRSRadialBeams(void)
{
	DBG_RS_PRINT("~CRSRadialBeams()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSRadialBeams::ResetAllParameters(void)
{
	CRSRadialBeams::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSRadialBeams::ResetParameters(void)
{
	SetBits(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP);
	m_fBeamLifeMin = 1.0f;
	m_fBeamLifeMax = 1.0f;
	m_fBeamWidthMin = 8.0f;
	m_fBeamWidthMax = 8.0f;
	m_fBeamAmplitude = 0.0f;
	m_fBeamSpeed = 0.0f;
	m_iNumBeams = 0;
	m_iBeamFlags = 0;
	m_iBeamTextureIndex = 0;
	m_vszEndPositions = NULL;// dangerous!
	m_pszBeams = NULL;// dangerous!
}

//-----------------------------------------------------------------------------
// Purpose: Initialize system after all user parameters were set
// Warning: R_BeamPoints() may return NULL!!!
// Must be called from class constructor or after setting parameters manually.
//-----------------------------------------------------------------------------
void CRSRadialBeams::InitializeSystem(void)
{
	CRSRadialBeams::BaseClass::InitializeSystem();

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		m_vszEndPositions = new Vector[m_iNumBeams];
#if defined (USE_EXCEPTIONS)
	}
	catch (std::bad_alloc &ba)// XDM3038b
	{
		conprintf(0, " %s[%u] ERROR: unable to allocate memory for %u vectors! %s\n", GetClassName(), GetIndex(), m_iNumBeams, ba.what());
		DBG_FORCEBREAK
		return;
	}
#endif

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		m_pszBeams = new BEAM *[m_iNumBeams];
#if defined (USE_EXCEPTIONS)
	}
	catch (std::bad_alloc &ba)// XDM3038b
	{
		conprintf(0, " %s[%u] ERROR: unable to allocate memory for %u beam pointers! %s\n", GetClassName(), GetIndex(), m_iNumBeams, ba.what());
		DBG_FORCEBREAK
		return;
	}
#endif
	size_t nBeams = 0;
	for (size_t i = 0; i < m_iNumBeams; ++i)
	{
		VectorRandom(m_vszEndPositions[i]);
		m_vszEndPositions[i].NormalizeSelf();
		m_vszEndPositions[i] *= m_fScale;
		m_pszBeams[i] = gEngfuncs.pEfxAPI->R_BeamPoints(m_vecOrigin, m_vecOrigin+m_vszEndPositions[i], m_iBeamTextureIndex, RANDOM_FLOAT(0.0f,m_fBeamLifeMax), m_fBeamWidthMin, m_fBeamAmplitude, m_fColorCurrent[3], m_fBeamSpeed, 0, 0, 0,0.1,0);
		if (m_pszBeams[i] != NULL)
		{
			SetBits(m_pszBeams[i]->flags, FBEAM_FOREVER|FBEAM_ISACTIVE|m_iBeamFlags);
			++nBeams;
		}
		else
			conprintf(2, "%s[%u] failed to allocate beam %u of %u!\n", GetClassName(), GetIndex(), i, m_iNumBeams);
	}
	if (nBeams == 0)// can be true
	{
		conprintf(1, " %s[%u] ERROR: unable to allocate %u beams! Shutting down system.\n", GetClassName(), GetIndex(), m_iNumBeams);
		m_ShuttingDown = true;// use standard safe deallocaiton methods
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSRadialBeams::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSRadialBeams::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSRadialBeams::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	if (m_vszEndPositions != NULL)
	{
		delete [] m_vszEndPositions;
		m_vszEndPositions = NULL;
	}
	if (m_pszBeams)
	{
		for (size_t i = 0; i < m_iNumBeams; ++i)
		{
			if (m_pszBeams[i])
			{
				ClearBits(m_pszBeams[i]->flags, FBEAM_FOREVER);
				m_pszBeams[i]->die = gEngfuncs.GetClientTime();
				m_pszBeams[i] = NULL;
			}
		}
		delete [] m_pszBeams;
		m_pszBeams = NULL;// each beam is allocated and freed by the engine
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSRadialBeams::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "fBeamLifeMin") == 0)
		m_fBeamLifeMin = atof(szValue);
	else if (strcmp(szKey, "fBeamLifeMax") == 0)
		m_fBeamLifeMax = atof(szValue);
	else if (strcmp(szKey, "fBeamWidthMin") == 0)
		m_fBeamWidthMin = atof(szValue);
	else if (strcmp(szKey, "fBeamWidthMax") == 0)
		m_fBeamWidthMax = atof(szValue);
	else if (strcmp(szKey, "fBeamAmplitude") == 0)
		m_fBeamAmplitude = atof(szValue);
	else if (strcmp(szKey, "fBeamSpeed") == 0)
		m_fBeamSpeed = atof(szValue);
	else if (strcmp(szKey, "iNumBeams") == 0)
	{
		if (/*m_iNumBeams == 0 && */m_vszEndPositions == NULL && m_pszBeams == NULL)
			m_iNumBeams = strtoul(szValue, NULL, 10);
		else
		{
			conprintf(0, "ERROR: %s %u: tried to re-initialize already allocated value %s (%u)!\n", GetClassName(), GetIndex(), szKey, m_iNumBeams);
			return false;
		}
	}
	else if (strcmp(szKey, "szTexture") == 0)// !!! Overridden for this system!
	{
		m_iBeamTextureIndex = 0;
		m_pTexture = gEngfuncs.CL_LoadModel(szValue, &m_iBeamTextureIndex);
		if (m_iBeamTextureIndex == 0)
		{
			conprintf(0, "ERROR: %s %u: unable to load %s \"%s\", system requires valid model index!\n", GetClassName(), GetIndex(), szKey, szValue);
			return false;
		}
	}
	else
		return CRSRadialBeams::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSRadialBeams::Update(const float &time, const double &elapsedTime)
{
	CRSRadialBeams::BaseClass::Update(time, elapsedTime);

	for (size_t i = 0; i < m_iNumBeams; ++i)
	{
		if (m_pszBeams[i] == NULL)
			continue;

		if (IsShuttingDown())
		{
			ClearBits(m_pszBeams[i]->flags, FBEAM_FOREVER|FBEAM_ISACTIVE);// nice idea, but doesn't work anyway: |FBEAM_FADEOUT);
			m_pszBeams[i]->die = time;
			m_pszBeams[i] = NULL;// engine will deallocate it
			continue;
		}
		if (m_pszBeams[i]->die <= time)// time to change beam's state
		{
			m_pszBeams[i]->die = time + RANDOM_FLOAT(m_fBeamLifeMin, m_fBeamLifeMax);// next state change
			if (FBitSet(m_pszBeams[i]->flags, FBEAM_ISACTIVE))// disable beam
			{
				ClearBits(m_pszBeams[i]->flags, FBEAM_ISACTIVE);
				m_pszBeams[i]->brightness = 0.0f;
				//conprintf(2, "%s[%u] beam %u OFF\n", GetClassName(), GetIndex(), i);
				continue;
			}
			else// enable beam
			{
				SetBits(m_pszBeams[i]->flags, FBEAM_ISACTIVE);// HL engine doesn't seem to react on this
				// calculate new random point
				VectorRandom(m_vszEndPositions[i]);
				m_vszEndPositions[i].NormalizeSelf();
				m_vszEndPositions[i] *= m_fScale;
				//conprintf(2, "%s[%u] beam %u ON\n", GetClassName(), GetIndex(), i);
			}
		}
		m_pszBeams[i]->startEntity = m_iFollowEntity;// is this safe to set?
		m_pszBeams[i]->source = m_vecOrigin;
		m_pszBeams[i]->target = m_vecOrigin;// + m_vszEndPositions[i]; slower
		m_pszBeams[i]->target += m_vszEndPositions[i];
		if (FBitSet(m_pszBeams[i]->flags, FBEAM_ISACTIVE))
		{
			//m_pszBeams[i]->t = 0.0f;
			if (m_fBeamWidthMin == m_fBeamWidthMax)
				m_pszBeams[i]->width = m_fBeamWidthMin;
			else
				m_pszBeams[i]->width = RANDOM_FLOAT(m_fBeamWidthMin, m_fBeamWidthMax);

			m_pszBeams[i]->amplitude = m_fBeamAmplitude;
			m_pszBeams[i]->r = m_fColorCurrent[0];//(float)m_color.r/255.0f;
			m_pszBeams[i]->g = m_fColorCurrent[1];//(float)m_color.g/255.0f;
			m_pszBeams[i]->b = m_fColorCurrent[2];//(float)m_color.b/255.0f;
			m_pszBeams[i]->brightness = m_fColorCurrent[3];//m_fBrightness;//(float)m_color.a/255.0f;
			m_pszBeams[i]->speed = m_fBeamSpeed;
			//m_pszBeams[i]->frameRate = 0;// we set frames manually
			m_pszBeams[i]->frame = m_iFrame;
		}
		m_fScale += m_fScaleDelta*elapsedTime;
	}
	return IsShuttingDown();
}

//-----------------------------------------------------------------------------
// Purpose: Nothing here. The engine draws beams all by itself.
//-----------------------------------------------------------------------------
void CRSRadialBeams::Render(void)
{
}
