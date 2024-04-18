#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSLight.h"
#include "r_efx.h"

CRSLight::CRSLight(void) : CRSLight::BaseClass()
{
	DBG_RS_PRINT("CRSLight()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Advanced dynamic light
// Input  : origin - 
//			r,g,b - light color
//			radius - 
//			(*RadiusFn) - makes light radius time-dependant. May be NULL, or, for example, sinf
//-----------------------------------------------------------------------------
CRSLight::CRSLight(const Vector &origin, byte r, byte g, byte b, float radius, float (*RadiusFn)(CRSLight *, const float &), float decay, float timetolive, bool elight)
{
	DBG_RS_PRINT("CRSLight(...)");
	ResetParameters();

	if (elight)
		m_pLight = gEngfuncs.pEfxAPI->CL_AllocElight(LIGHT_INDEX_TE_RSLIGHT);
	else
		m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(LIGHT_INDEX_TE_RSLIGHT);// if key != 0, the engine will overwrite existing dlight with the same key

	if (m_pLight == NULL)
	{
		conprintf(1, "CRSLight failed to allocate dynamic light!\n");
		m_RemoveNow = true;
		return;// light is vital to this system
	}

	m_vecOrigin = origin;
	m_color.Set(r,g,b,decay*255.0f);
	//old m_fBrightness = decay;
	m_fScale = radius;
	//m_fScaleDelta = radiusdelta;
	//m_iFollowEntity = followentity;
	m_RadiusCallback = RadiusFn;
	m_iLightType = elight?1:0;

	m_pTexture = NULL;
	m_iRenderMode = 0;

	VectorCopy(m_vecOrigin, m_pLight->origin);
	m_pLight->radius = radius;
	m_pLight->color.r = r;
	m_pLight->color.g = g;
	m_pLight->color.b = b;
	m_pLight->decay = 0.0f;
	if (timetolive <= 0.0f)
	{
		m_fDieTime = 0.0f;
		m_pLight->die = gEngfuncs.GetClientTime() + 10.0f;// just start
	}
	else
	{
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
		m_pLight->die = m_fDieTime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSLight::~CRSLight(void)
{
	DBG_RS_PRINT("~CRSLight()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSLight::ResetAllParameters(void)
{
	CRSLight::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSLight::ResetParameters(void)
{
	m_RadiusCallback = NULL;
	m_pLight = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSLight::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "iLightType") == 0)
		m_iLightType = atoi(szValue);
	else
		return CRSLight::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSLight::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSLight::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSLight::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	if (m_pLight != NULL)
	{
		m_pLight->decay = m_fColorCurrent[3];//m_fBrightness;
		m_pLight->die = gEngfuncs.GetClientTime();
		m_pLight = NULL;
	}
	m_RadiusCallback = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Update light position and parameters. NOTE: light needs to be
//   recreated every frame in software mode.
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSLight::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		m_ShuttingDown = true;

	if (IsShuttingDown())
	{
		if (m_pLight)
		{
			m_pLight->die = time;
			m_pLight->decay = m_fColorCurrent[3];//m_fBrightness;// allow "last" light to fade out properly
			m_pLight = NULL;
		}
	}
	else
	{
		FollowEntity();

		int contents = gEngfuncs.PM_PointContents(m_vecOrigin, NULL);// XDM3038c
		if (contents == CONTENTS_SOLID)
		{
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))// XDM3038c: hack?
			{
				ShutdownSystem();
				goto finish;
			}
			else if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))
				goto finish;// just stop updating the light
				// no, may interfere SetBits(m_iFlags, RENDERSYSTEM_FLAG_NODRAW);
		}
		/*else
		{
		don't	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))
				ClearBits(m_iFlags, RENDERSYSTEM_FLAG_NODRAW);
		}*/

		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NODRAW))// no need to restore as it'll be updated automatically
		{
			if (m_pLight)
				m_pLight->radius = 1.0f;
		}// don't recreate if nodraw
		else
		{
			if (m_pLight == NULL || !IEngineStudio.IsHardware())// [re]create. Software mode fixed lights fix?
			{
				if (m_pLight)
				{
					m_pLight->decay = 0.0f;
					m_pLight->die = time;// remove previous frame light
				}
				if (m_iLightType == 1)
					m_pLight = gEngfuncs.pEfxAPI->CL_AllocElight(LIGHT_INDEX_TE_RSLIGHT);// m_pLight->key?
				else
					m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(LIGHT_INDEX_TE_RSLIGHT);

				if (m_pLight)
				{
					//m_pLight->decay = 0.0f;
					m_pLight->die = /*gEngfuncs.GetClientTime()*/time + 1.0f / RENDERSYSTEM_REFERENCE_FPS;
				}
				else// unable to allocate the light
				{
					conprintf(1, "CRSLight failed to allocate dynamic light!\n");
					m_ShuttingDown = true;
					return 1;
				}
			}

			if (m_pLight)
			{
				if (m_RadiusCallback)
					m_pLight->radius = m_fScale * m_RadiusCallback(this, time);
				else
					m_pLight->radius = m_fScale;

				m_pLight->color.r = m_color.r;
				m_pLight->color.g = m_color.g;
				m_pLight->color.b = m_color.b;
				m_pLight->origin = m_vecOrigin;

				if (m_fDieTime <= 0.0f)// don't let the light die
					m_pLight->die = time + 1.0f;
				else
					m_pLight->die = m_fDieTime;// XDM3038c: in case it has been changed externally
			}
		}
	}
finish:
	if (IsShuttingDown())
		return 1;

	//m_fScale += m_fScaleDelta*elapsedTime;
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: do nothing
//-----------------------------------------------------------------------------
void CRSLight::Render(void)
{
}
