#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSTeleparts.h"
#include "r_efx.h"
#include "cl_fx.h"

// Old system: uses engine particles and dynamic light
// Replaced by CParticleSystem + RSLight

CRSTeleparts::CRSTeleparts(void) : CRSTeleparts::BaseClass()
{
	DBG_RS_PRINT("CRSTeleparts()");
	ResetParameters();// non-recursive
}

CRSTeleparts::CRSTeleparts(const Vector &origin, float radius, byte pal_color, byte type, float timetolive, byte r, byte g, byte b)
{
	DBG_RS_PRINT("CRSTeleparts(...)");
	ResetParameters();
	m_vecOrigin = origin;
	m_colorpal = pal_color;
	m_iType = type;
	//m_iFollowEntity = followentity;
	m_color.Set(r,g,b);
	//old m_fBrightness = 0.0f;
	m_fColorDelta[3] = 0.0f;
	m_fScale = radius;
	m_fScaleDelta = 0.0f;

	m_pTexture = NULL;
	m_iRenderMode = 0;
	// XDM3037a: OBSOLETE	texindex = 0;

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	//conprintf(1, "> m_pLight = %d (%x)\n", m_pLight, m_pLight);
	if (m_pLight)
	{
		VectorCopy(m_vecOrigin, m_pLight->origin);
		m_pLight->radius = m_fScale*2.5f;// XDM3037: magic number: unfortunately, this depens on surrounding world textures scale
		m_pLight->color.r = r;
		m_pLight->color.g = g;
		m_pLight->color.b = b;
		m_pLight->decay = 0.0f;
		m_pLight->die = gEngfuncs.GetClientTime()+m_fDieTime;
		m_pLight->dark = true;
	}
#if defined (_DEBUG)
	else
		conprintf(1, "CRSTeleparts failed to allocate dynamic light!\n");
#endif

	uint32 i = 0;
	for (i = 0; i < NUMVERTEXNORMALS; ++i)
	{
		m_pParticles[i] = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);// WARNING! Don't let engine particles "die" before the system!!! Particles are allocated dynamically!
		if (m_pParticles[i] != NULL)
		{
			m_pParticles[i]->die = m_fDieTime;
			m_pParticles[i]->color = m_colorpal;
			VectorCopy(m_vecOrigin, m_pParticles[i]->org);
//should be already 0 0 0			VectorClear(m_pParticles[i]->vel);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&m_pParticles[i]->packedColor, m_pParticles[i]->color);
		}
	}

	if (type > 0)
	{
		for (i = 0; i < NUMVERTEXNORMALS; ++i)// init array first!
		{
			m_vecAng[i][0] = RANDOM_FLOAT(-M_PI, M_PI);
			m_vecAng[i][1] = RANDOM_FLOAT(-M_PI, M_PI);
			//m_vecAng[i][2] = RANDOM_FLOAT(-M_PI, M_PI);
		}
	}
	/*else
	{
		for (i = 0; i < NUMVERTEXNORMALS; ++i)// init array first!
		{
			VectorMultiply(VectorRandom(), VectorRandom(), m_vecAng[i]);
			VectorNormalize(ang[i]);
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSTeleparts::~CRSTeleparts(void)
{
	DBG_RS_PRINT("~CRSTeleparts()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSTeleparts::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSTeleparts::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSTeleparts::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	for (uint32 i = 0; i < NUMVERTEXNORMALS; ++i)
	{
		if (m_pParticles[i] != NULL)
		{
			m_pParticles[i]->die = 0.0f;
			m_pParticles[i] = NULL;// remove all pointers
		}
	}
	//conprintf(1, "~ m_pLight = %d (%x)\n", m_pLight, m_pLight);
	if (m_pLight != NULL)
	{
		m_pLight->die = gEngfuncs.GetClientTime();
		m_pLight = NULL;
	}
	m_colorpal = 0;
	m_iType = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSTeleparts::ResetAllParameters(void)
{
	CRSTeleparts::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSTeleparts::ResetParameters(void)
{
	m_pLight = NULL;
	m_colorpal = 0;
	m_iType = 0;
	for (uint32 i = 0; i < NUMVERTEXNORMALS; ++i)
	{
		m_pParticles[i] = NULL;
		//m_vecAng[i][0] = 0.0f;
		//m_vecAng[i][1] = 0.0f;
		//m_vecAng[i][2] = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSTeleparts::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "type") == 0)
		m_iType = atoi(szValue);
	else if (strcmp(szKey, "colorindex") == 0)
		m_colorpal = atoi(szValue);
	else
		return CRSTeleparts::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSTeleparts::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
	{
		DBG_RS_PRINTF("%s[%d]\"%s\": Update() shutting down: m_fDieTime <= time\n", GetClassName(), GetIndex(), GetUID());
		ShutdownSystem();
	}

	if (!CheckFxLevel())// XDM3038c
		return 0;

	FollowEntity();

	if (m_pLight)
	{
		if (IsShuttingDown())
		{
			m_pLight->decay = 0.0f;// this RS doesn't need any fading
			m_pLight->die = time;//gEngfuncs.GetClientTime();
		}
		else
		{
			if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)// no need to restore as it'll be updated automatically
			{
				m_pLight->radius = 1.0f;
			}// don't recreate if nodraw
			else
			{
				if (!IEngineStudio.IsHardware())// software mode fixed lights fix?
				{
					m_pLight->die = time;// remove previous frame light
					m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
					// may return null
					if (m_pLight)
					{
						m_pLight->die = time + 0.001f;
					}
				}
				if (m_pLight)
				{
					VectorCopy(m_vecOrigin, m_pLight->origin);
					m_pLight->radius = m_fScale*2.5f;// XDM3037
					m_pLight->color.r = m_color.r;
					m_pLight->color.g = m_color.g;
					m_pLight->color.b = m_color.b;
					if (m_fDieTime <= 0.0f)// don't let the light die
						m_pLight->die = time + 1.0f;
				}
			}
		}
	}

	if (IsShuttingDown())
		return 1;

	uint32 i = 0;
	float dist = 0.0f;

	if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)// HACK?
	{
		for (i = 0; i < NUMVERTEXNORMALS; ++i)
		{
			if (m_pParticles[i])
				VectorCopy(m_vecOrigin, m_pParticles[i]->org);
		}
	}
	else
	{
		if (m_iType == 0)
		{
			for (i = 0; i < NUMVERTEXNORMALS; ++i)
			{
				if (m_pParticles[i])
				{
					dist = sinf(time + i)*m_fScale;
					VectorMA(m_vecOrigin, dist, (float *)vdirs[i], m_pParticles[i]->org);
				}
			}
		}
		else
		{
			float s1, s2, c1, c2;
			Vector dir;
			for (i = 0; i < NUMVERTEXNORMALS; ++i)
			{
				if (m_pParticles[i])
				{
					SinCos(time*1.5f + m_vecAng[i][0], &s1, &c1);
					//dir[0] = s*c;
					SinCos(time*1.5f + m_vecAng[i][1], &s2, &c2);
					//dir[1] = s*c;
					//SinCos(time*1.5f + m_vecAng[i][2], &s, &c);
					//dir[2] = s*c;

					dir[0] = c1*c2;
					dir[1] = s1*c2;
					dir[2] = -s2;

					//dist = sinf(time + i)*m_fScale;
					dist = m_fScale;// <-- COOL!
					VectorMA(m_vecOrigin, dist*0.6f, dir, m_pParticles[i]->org);
					// causes lots of glitches m_pParticles[i]->type = pt_explode;
				}
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Nothing here. The engine draws particles all by itself.
//-----------------------------------------------------------------------------
void CRSTeleparts::Render(void)
{
}
