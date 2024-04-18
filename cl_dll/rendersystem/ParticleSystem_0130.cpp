#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "pm_shared.h"
#include "event_api.h"
#include "triangleapi.h"
#include "bsputil.h"
#include <new>
#ifdef USE_EXCEPTIONS
#include <exception>
#endif
// TEST #include "shared_resources.h"
// TODO: relative coordinates? (move all particles with the entity) but this would look stupid

// Purpose: A real entindex to compare and ignore inside custom TraceLine
// Warning: MULTITHREAD KILLER!
static int g_CParticleSystem_PhysEntIgnoreIndex = -1;// 
// Purpose: Filter callback for PlayerTraceEx
static int CParticleSystem_PhysEntIgnore(physent_t *pe)
{
	if (pe->info == g_CParticleSystem_PhysEntIgnoreIndex)
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Default constructor.
// Warning: All constructors must call ResetParameters()!
//-----------------------------------------------------------------------------
CParticleSystem::CParticleSystem(void) : CParticleSystem::BaseClass()
{
	DBG_RS_PRINT("CParticleSystem()");
	m_pParticleList = NULL;
	m_iMaxParticles = 0;// don't allocate anything if errors will be encountered
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Default particle system
// Input  : maxParticles - 
//			origin - 
//			direction - 
//			pTexture - 
//			rendermode - 
//			timetolive - 0 means the system removes itself after the last frame
//-----------------------------------------------------------------------------
CParticleSystem::CParticleSystem(uint32 maxParticles, const Vector &origin, const Vector &direction, struct model_s *pTexture, int rendermode, float timetolive)
{
	DBG_RS_PRINT("CParticleSystem(...)");
	m_pParticleList = NULL;
	ResetParameters();
	if (maxParticles == 0)
	{
		conprintf(1, "%s[%u]::CParticleSystem() error: maxParticles == 0!\n", GetClassName(), GetIndex());
		m_RemoveNow = true;
		return;
	}
	if (!InitTexture(pTexture))
	{
		conprintf(1, "%s[%u]::CParticleSystem() error: InitTexture(%s) failed!\n", GetClassName(), GetIndex(), pTexture?pTexture->name:"NULL");
		m_RemoveNow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_vDirection = direction;
	m_fParticleSpeedMax = m_fParticleSpeedMin = m_vDirection.NormalizeSelf();// XDM3038b
	m_iRenderMode = rendermode;
	m_fScale = 4.0f;// default scale for backward compatibility
	//m_fSizeX = 1.0f;// XDM3038b
	//m_fSizeY = 1.0f;
	if (timetolive <= 0.0f)// XDM3038a: was <
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CParticleSystem::~CParticleSystem(void)
{
	DBG_RS_PRINT("~CParticleSystem()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CParticleSystem::ResetAllParameters(void)
{
	DBG_RS_PRINT("ResetAllParameters()");
	CParticleSystem::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CParticleSystem::ResetParameters(void)
{
	DBG_RS_PRINT("ResetParameters()");
	m_iTraceFlags = PM_STUDIO_IGNORE;//PM_STUDIO_BOX
	m_iStartType = PSSTARTTYPE_POINT;// XDM3038b
	m_iMovementType = PSMOVTYPE_DIRECTED;// XDM3038b
	m_iEmitRate = 0;// auto
	m_fStartRadiusMin = 0.0f;// XDM3038c
	m_fStartRadiusMax = 0.0f;
	m_fParticleScaleMin = 1.0f;// XDM3038c
	m_fParticleScaleMax = 1.0f;
	m_fParticleSpeedMin = 1.0f;// XDM3038c
	m_fParticleSpeedMax = 1.0f;
	m_fParticleWeight = 1.0f;// XDM3038c: *gravity
	m_fParticleCollisionSize = 1.0f;// XDM3038c
	m_fParticleLife = 0.0f;// auto
	m_vDirection.Clear();
	m_vSpread.Clear();
	m_vDestination.Clear();// XDM3038b
	m_vStartMins.Clear();// XDM3038c
	m_vStartMaxs.Clear();
	m_iMaxParticles = 0;// !!! must be invalid by default !!!
	m_iNumParticles = 0;// !!! must be invalid by default !!!
	m_fAccumulatedEmit = 0;
	m_fEnergyStart = 1.0f;
	//m_pParticleList = NULL;// DANGEROUS!

	m_OnParticleInitialize = NULL;
	m_pOnParticleInitializeData = NULL;
	m_bOnParticleInitializeDataFree = false;
	m_OnParticleUpdate = NULL;
	m_pOnParticleUpdateData = NULL;
	m_bOnParticleUpdateDataFree = false;
	m_OnParticleCollide = NULL;
	m_pOnParticleCollideData = NULL;
	m_bOnParticleCollideDataFree = false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CParticleSystem::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "iTraceFlags") == 0)
		m_iTraceFlags = atoi(szValue);
	else if (strcmp(szKey, "fEnergyStart") == 0)
		m_fEnergyStart = atof(szValue);
	else if (strcmp(szKey, "iMaxParticles") == 0)
	{
		if (/*m_iMaxParticles == 0 && */m_pParticleList == NULL)// m_iMaxParticles can be changed many times before the array is actually allocated
			m_iMaxParticles = strtoul(szValue, NULL, 10);
		else
		{
			conprintf(0, "ERROR: %s[%u]: tried to re-initialize already allocated value %s (%u)!\n", GetClassName(), GetIndex(), szKey, m_iMaxParticles);
			return false;
		}
	}
	else if (strcmp(szKey, "fMaxParticlesK") == 0)
	{
		if (/*m_iMaxParticles == 0 && */m_pParticleList == NULL)// m_iMaxParticles can be changed many times before the array is actually allocated
		{
			float k = abs(atof(szValue));
			if (k != 0)
				m_iMaxParticles *= k;
		}
		else
		{
			conprintf(0, "ERROR: %s[%u]: tried to re-initialize already allocated value %s (%u)!\n", GetClassName(), GetIndex(), szKey, m_iMaxParticles);
			return false;
		}
	}
	else if (strcmp(szKey, "iStartType") == 0)
		m_iStartType = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "iMovementType") == 0)
		m_iMovementType = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "iEmitRate") == 0)
		m_iEmitRate = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "fParticleScaleMin") == 0)
		m_fParticleScaleMin = atof(szValue);
	else if (strcmp(szKey, "fParticleScaleMax") == 0)
		m_fParticleScaleMax = atof(szValue);
	else if (strcmp(szKey, "fParticleSpeedMin") == 0)
		m_fParticleSpeedMin = atof(szValue);
	else if (strcmp(szKey, "fParticleSpeedMax") == 0)
		m_fParticleSpeedMax = atof(szValue);
	else if (strcmp(szKey, "fStartRadiusMin") == 0)
		m_fStartRadiusMin = atof(szValue);
	else if (strcmp(szKey, "fStartRadiusMax") == 0)
		m_fStartRadiusMax = atof(szValue);
	else if (strcmp(szKey, "vStartMins") == 0)// -x, -y, -z
		return StringToVec(szValue, m_vStartMins);
	else if (strcmp(szKey, "vStartMaxs") == 0)// +x, +y, +z
		return StringToVec(szValue, m_vStartMaxs);
	else if (strcmp(szKey, "fParticleWeight") == 0)
		m_fParticleWeight = atof(szValue);
	else if (strcmp(szKey, "fParticleCollisionSize") == 0)
		m_fParticleCollisionSize = atof(szValue);
	else if (strcmp(szKey, "fParticleLife") == 0)
		m_fParticleLife = atof(szValue);
	else if (strcmp(szKey, "vDirection") == 0)
	{
		if (StringToVec(szValue, m_vDirection))
			m_vDirection.NormalizeSelf();// must be normalized!
		else
			return false;
	}
	else if (strcmp(szKey, "vSpread") == 0)
		return StringToVec(szValue, m_vSpread);
	else if (strcmp(szKey, "vDestination") == 0)
		return StringToVec(szValue, m_vDestination);
	else
		return CParticleSystem::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize SYSTEM (non-user) startup variables.
// Note:    Must be called from/after class constructor (after ResetParameters).
// Warning: Be very careful when overriding this function!!
// Warning: Must be safe to call multiple times!
//-----------------------------------------------------------------------------
void CParticleSystem::InitializeSystem(void)
{
	DBG_RS_PRINT("InitializeSystem()");
	if (m_pParticleList != NULL)// 0x00000001
	{
		delete [] m_pParticleList;// possible Access Violation.
		m_pParticleList = NULL;
	}

	ASSERT(m_ShuttingDown == false);
	ASSERT(m_RemoveNow == false);

	if (m_iMaxParticles > 0)
	{
		if (!AllocateParticles())// XDM3038
		{
			conprintf(1, "%s[%u]::InitializeSystem() error: AllocateParticles(%u) failed!\n", GetClassName(), GetIndex(), m_iMaxParticles);
			m_RemoveNow = true;
			return;
		}
	}
	else if (!IsVirtual())
		m_iState = RSSTATE_DISABLED;// XDM3038c

	m_iNumParticles = 0;
	m_fAccumulatedEmit = 0;

	// UNDONE: WTF: HELP: F1: Calculate average particle life
	if (m_fParticleLife <= 0.0f)// "auto"
	{
		// particle_life_Sec = (m_fBrightness/m_fColorDelta[3])
		// particle_spawn_delay_Sec = particle_life_Sec/m_iMaxParticles;
		if (m_fColorDelta[3] != 0.0f)// defines the number of iterations in the gradient
		{
			float fBrightnessRange = (m_fColorDelta[3] < 0.0f) ? (m_color.a/255.0f - 0.0f) : (1.0f - m_color.a/255.0f);// range that actual brightness travels through
			m_fAvgParticleLife = fBrightnessRange / fabs(m_fColorDelta[3]);// we assume one gradient step per frame
		}
		else if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_LOOPFRAMES) && m_pTexture && m_fFrameRate > 0.0f)
		{
			m_fAvgParticleLife = m_pTexture->numframes / m_fFrameRate;
		}
		else// eternal system
		{
			m_fAvgParticleLife = 0.0f;
		}
		//if (FBitSetAll(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE|RENDERSYSTEM_FLAG_ADDPHYSICS))
		//	m_fAvgParticleLife *= 0.8f;// HACK: just a very rough guess: some particles may disappear upon colliding with something
	}
	else
		m_fAvgParticleLife = m_fParticleLife;

	//ASSERT(m_fAvgParticleLife > 0.0f);

	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_SIMULTANEOUS))// XDM3038c
	{
		m_iEmitRate = m_iMaxParticles * RENDERSYSTEM_REFERENCE_FPS + 2;// emit all particles in ONE FRAME, not in one second
		m_fAccumulatedEmit = m_iMaxParticles;
	}
	else if (m_iEmitRate == 0)
	{
		if (m_fAvgParticleLife > 0.0f)
			m_iEmitRate = __max(1u, (uint32)((float)m_iMaxParticles/(m_fAvgParticleLife+0.05f)));//1/(m_fAvgParticleLife/m_iMaxParticles);
		else// eternal particles
			m_iEmitRate = m_iMaxParticles*0.5f;// emit them all in 2 seconds
	}

	DBG_RS_PRINTF("%s[%d]\"%s\"::InitializeSystem() m_fAvgParticleLife %g, m_iEmitRate %u\n", GetClassName(), GetIndex(), GetUID(), m_fAvgParticleLife, m_iEmitRate);
	//conprintf(1, "%s[%u]::InitializeSystem() m_fAvgParticleLife %g, m_iEmitRate %u\n", GetClassName(), GetIndex(), m_fAvgParticleLife, m_iEmitRate);

	if (m_iMovementType != PSMOVTYPE_DIRECTED)// XDM3038c: prevent multiplication by zero
	{
		if (m_vDirection.IsZero())
			m_vDirection.Set(1,1,1);
	}

	/* FollowUpdatePosition(); will do this: if (m_iStartType == PSSTARTTYPE_ENTITYBBOX)
	{
		if (m_iFollowEntity && m_pLastFollowedEntity)
		{
			if (m_pLastFollowedEntity->model->type == mod_brush)// brush models have absolute coordinates in mins/maxs
			{
				m_vStartMins = m_pLastFollowedEntity->model->mins-m_pLastFollowedEntity->origin;
		(...)
				conprintf(0, " ERROR: %s[%u]: model index %d was not found!\n", GetClassName(), GetIndex(), m_iFollowModelIndex);
				m_iFollowModelIndex = 0;// not found anyway
			}
		}
		//else entity may have gone out of sight, temporarily
	}
	else */if (m_iStartType == PSSTARTTYPE_CYLINDER)
	{
		m_vStartMins.x = -m_fStartRadiusMax;
		m_vStartMins.y = -m_fStartRadiusMax;
		m_vStartMaxs.x = m_fStartRadiusMax;
		m_vStartMaxs.y = m_fStartRadiusMax;
		// .z is customized by the user
	}
	CParticleSystem::BaseClass::InitializeSystem();// <- FollowUpdatePosition();
	// NOTE: by this point ShutdownSystem() may get called and manager->AddSystem() may fail, that is somewhat normal
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CParticleSystem::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CParticleSystem::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CParticleSystem::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	if (m_pParticleList != NULL)
	{
		delete [] m_pParticleList;
		m_pParticleList = NULL;
	}
	m_iMaxParticles = 0;
	m_iNumParticles = 0;
	m_fAccumulatedEmit = 0;
	if (m_bOnParticleInitializeDataFree && m_pOnParticleInitializeData != NULL)
	{
		delete m_pOnParticleInitializeData;
		m_pOnParticleInitializeData = NULL;
	}
	if (m_bOnParticleUpdateDataFree && m_pOnParticleUpdateData != NULL)
	{
		delete m_pOnParticleUpdateData;
		m_pOnParticleUpdateData = NULL;
	}
	if (m_bOnParticleCollideDataFree && m_pOnParticleCollideData != NULL)
	{
		delete m_pOnParticleCollideData;
		m_pOnParticleCollideData = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Virtual allocation function to allow custom particle classes to be used
// Output : Returns true on success
//-----------------------------------------------------------------------------
bool CParticleSystem::AllocateParticles(void)
{
	DBG_RS_PRINTF("CParticleSystem(%u)::AllocateParticles(%u)\n", index, m_iMaxParticles);
	if (m_iMaxParticles > 0)
	{
#if defined (USE_EXCEPTIONS)
		try
		{
#endif
			m_pParticleList = new CParticle[m_iMaxParticles];
#if defined (USE_EXCEPTIONS)
		}
		catch (std::bad_alloc &ba)// XDM3038c: WARNING: ACTUALLY HAPPENED!!!
		{
			conprintf(0, "%s[%u] ERROR: unable to allocate memory for %u particles! %s\n", GetClassName(), GetIndex(), m_iMaxParticles, ba.what());
			DBG_FORCEBREAK
			return false;
		}
#endif
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Emit particles. It's the tricky part to not to release all particles at the same time.
// Note   : Calls AllocateParticle(), InitializeParticle()
// Warning: Must NOT be called from constructor because it depends on flags!
// Warning: Assumed to be called only once per frame!
// Concept: Each particle has deterministic lifespan (unless forcibly removed),
// the aim is to keep all particles active at any frame (except for the start).
// By that time LAST particle spawns, 1st one is about to be removed and so on.
// RATE (particles per second) should prevent particles from emerging too often
// (ugly bursts) and too seldom (m_fAccumulatedEmit overflow).
// ALL particles should be created during 1st one's lifespan!
// Output : uint32 - number of particles actually created
//-----------------------------------------------------------------------------
uint32 CParticleSystem::Emit(void)
{
	if (IsShuttingDown() || IsRemoving())// Important!
		return 0;
	if (GetState() != RSSTATE_ENABLED)// XDM3038b
		return 0;

	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_SIMULTANEOUS))// XDM3038c
	{
		m_fAccumulatedEmit += (float)(gHUD.m_flTimeDelta * (double)m_iEmitRate);// we need this fractional value!

		/*if (m_iNumParticles < m_iMaxParticles)// throw in some unused particle slots (when particles are removed upon collisions, etc.)
		{
			uint32 iUnusedParticles = m_iMaxParticles - m_iNumParticles;
			if ((int32)m_fAccumulatedEmit < iUnusedParticles)
				m_fAccumulatedEmit += (float)iUnusedParticles - m_fAccumulatedEmit;
		} BAD!! */
	}
	CParticle *pParticle = NULL;
	uint32 iNumCreated = 0;
	uint32 iNumEmit = (uint32)m_fAccumulatedEmit;// number of particles will be created this time
	Vector vecOriginRange(m_vecOrigin); vecOriginRange -= m_vecOriginPrev;
	bool bInterpolate = vecOriginRange.Length() > 4.0f;// interpolate when trajectory interrupts visually, can be tuned
	for (; m_fAccumulatedEmit >= 1; m_fAccumulatedEmit -= 1)//while ((m_fAccumulatedEmit >= 1) && iNumCreated < m_iMaxParticles)// && (m_iNumParticles < m_iMaxParticles))
	{
		pParticle = AllocateParticle(pParticle?pParticle->index:0);// speed up: there's NO free index before (less than) previously allocated particle
		if (pParticle)
		{
#if defined (_DEBUG)
			ASSERT(pParticle->IsFree() == false);
#endif
			pParticle->m_fEnergy = 1.0f;
			// Warning: since 0 means "disabled" and current origin will be used, 0 during interpolation should also mean "closest to current origin"
			// Note: particles should be created and interpolated in order from old system origin to current, so [1...0]
			InitializeParticle(pParticle, bInterpolate?((float)m_fAccumulatedEmit/(float)iNumEmit):0.0f);
			//--m_fAccumulatedEmit;
			++iNumCreated;
		}
		else// no more particles will be created anyway
		{
			//conprintf(1, "CParticleSystem::Emit(iNumEmit %u) cannot allocate particles! AccumulatedEmit = %u\n", iNumEmit, m_fAccumulatedEmit);
			break;
		}
	}
	if ((uint32)m_fAccumulatedEmit > m_iMaxParticles+1)
	{
		conprintf(2, "CParticleSystem::Emit(iNumEmit %u) warning: m_fAccumulatedEmit(%f) > m_iMaxParticles(%u)!\n", iNumEmit, m_fAccumulatedEmit, m_iMaxParticles);
		if (g_pCvarPSAutoRate->value > 0)
		{
			uint32 overflow = (uint32)m_fAccumulatedEmit - m_iMaxParticles;
			if (m_iEmitRate > overflow+1)// keep rate above 0!
				m_iEmitRate -= overflow;
			else
				m_iEmitRate /= 2;

			m_fAccumulatedEmit -= 1;
		}
		else
			m_fAccumulatedEmit = m_iMaxParticles;
	}

	// NOTE: displacer particles must persist!
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_SIMULTANEOUS))
	{
		if (m_iNumParticles == 0 && m_fAccumulatedEmit <= 0.0f && m_iEmitRate == 0 && m_fAvgParticleLife > 0.0f)// one-shot system has shown all its particles, nothing left.
		{
			DBG_RS_PRINTF("%s[%d]\"%s\": Emit() shutting down: finite simultaneous system exhausted particles\n", GetClassName(), GetIndex(), GetUID());
			ShutdownSystem();
		}
		else// ALWAYS! if (iNumCreated == m_iMaxParticles)//(allparticleswereused)
			m_iEmitRate = 0;// save initial value?? // NO! ShutdownSystem();
	}
	// already m_fAccumulatedEmit -= iNumCreated;// keep fractional leftovers
	return iNumCreated;
}

//-----------------------------------------------------------------------------
// Purpose: One more layer for advanced use in cases when Emit() won't do.
// Warning: Does NOT really allocate any memory, just uses another free particle slot.
// Input  : iStartIndex - speedup process by specifying last index before which is ABSOLUTELY STRICTLY KNOWN there are NO FREE PARTICLES.
// Output : CParticle or ANY OTHER CLASS that is REALLY stored in m_pParticleList
//-----------------------------------------------------------------------------
CParticle *CParticleSystem::AllocateParticle(uint32 iStartIndex)
{
	// TODO: we could use some multithreading and better algorithm here?
	// UNDONE: wrap around maximum index and continue searching from 0
	//{
		for (uint32 i = iStartIndex; i < m_iMaxParticles; ++i)// SLOW! But we have no choice
		{
			if (m_pParticleList[i].index == -1)
			{
				m_pParticleList[i].index = i;
				m_pParticleList[i].m_vPos.Clear();// safety
				return &m_pParticleList[i];
				//conprintf(1, "CParticleSystem::AllocateParticle() failed to find a free particle (%u)!!\n", i);
			}
		}
	//}
	return NULL;//pParticle;
}

//-----------------------------------------------------------------------------
// Purpose: Because particles are stored in a plain array, this function just
//			clears out particle's data and decreases m_iNumParticles
// Warning: 'this' may be a virtual system that is managing this particle, but not actually containing it!
// Input  : *pParticle - 
//-----------------------------------------------------------------------------
void CParticleSystem::RemoveParticle(CParticle *pParticle)
{
	if (m_iNumParticles == 0)
	{
		conprintf(1, "%s[%u]::RemoveParticle() ERROR: m_iNumParticles == %u! BAD CODE!\n", GetClassName(), GetIndex(), m_iNumParticles);
		return;
	}
	if (pParticle)
	{
		if (pParticle->IsFree() == false)
		{
			pParticle->m_fEnergy = -1.0f;
			pParticle->index = UINT_MAX;
			--m_iNumParticles;
		}
		else
			conprintf(1, "%s[%u]::RemoveParticle(%u) error: pParticle->IsFree()!\n", GetClassName(), GetIndex(), pParticle->index);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
// Warning: DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CParticleSystem::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
	{
		if (!IsShuttingDown())
		{
			DBG_RS_PRINTF("%s[%d]\"%s\": Update() shutting down: m_fDieTime <= time\n", GetClassName(), GetIndex(), GetUID());
			ShutdownSystem();// don't return because we have to update remaining particles
		}
	}
	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;

	// WARNING! If you call CRenderSystem::Update(), all starting parameters for particles will be ruined! (origin. velocity, color, brightness, etc.)
	FollowEntity();
	Emit();

	if (GetState() != RSSTATE_ENABLED && m_iNumParticles == 0)// XDM3038b: optimization: don't iterate
		return 0;

	if (IsShuttingDown() && FBitSet(m_iFlags, RENDERSYSTEM_FLAG_HARDSHUTDOWN))// XDM3038c
		return 1;

	uint32 iLastFrame = m_iFrame;
	UpdateFrame(time, elapsedTime);// update system's frame according to desired frame rate, required to detect proper time to update frames in particles
	int contents;
	//int ignore_pe;
	pmtrace_t pmtrace;
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS) && !FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// XDM3038c
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, (m_pLastFollowedEntity == gHUD.m_pLocalPlayer)?false:true);// must be called before SetSolidPlayers()
		//gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);// TEST
		gEngfuncs.pEventAPI->EV_PushPMStates();
		if (m_pLastFollowedEntity)
		{
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(m_pLastFollowedEntity->index-1);// actually "SetNONSolidPlayers", shouldn't be bad if provided index is not a player
			//gEngfuncs.pEventAPI->EV_SetSolidPlayers(cl_test2->value);// TEST
			//ignore_pe = PM_GetPhysent(m_pLastFollowedEntity->index);
		}
		else
		{
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);// index is NOT entindex! just array of players[MAX_PLAYERS];
			//ignore_pe = PHYSENT_NONE;
		}
		//gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		g_CParticleSystem_PhysEntIgnoreIndex = m_pLastFollowedEntity?m_pLastFollowedEntity->index:-1;
	}
	//else
	//	ignore_pe = PHYSENT_NONE;

	CParticle *pCurPart;
	Vector vToDest;
	// TEST
	//pCurPart = &m_pParticleList[0];
	//if (!pCurPart->IsFree())// track lifetime of one particle
	//	conprintf(1, "%s[%u]::Update(%f): nrg %f, a %f, ad: %f, frame %u\n", GetClassName(), GetIndex(), time, pCurPart->m_fEnergy, pCurPart->m_fColor[3], pCurPart->m_fColorDelta[3], pCurPart->m_iFrame);
	m_iNumParticles = m_iMaxParticles;// XDM3037: new allocation mechanism: counter
	for (uint32 i = 0; i < m_iMaxParticles; ++i)
	{
		pCurPart = &m_pParticleList[i];
		if (pCurPart->IsFree())// XDM3037
		{
			--m_iNumParticles;
			continue;
		}
		if (pCurPart->IsExpired())
		{
			RemoveParticle(pCurPart);// --m_iNumParticles inside
			continue;
		}
		pCurPart->m_vPosPrev = pCurPart->m_vPos;
		if (m_iMovementType == PSMOVTYPE_TOWARDSPOINT)
		{
			vToDest = m_vDestination; vToDest -= pCurPart->m_vPos;
			if (vToDest.Length() < __max(2.0f, m_fParticleCollisionSize))// less than 2 units
			{
				RemoveParticle(pCurPart);// XDM3037
				continue;
			}
			vToDest.SetLength(m_fParticleSpeedMax);
			pCurPart->m_vVel += vToDest;
		}
		pCurPart->m_vPos.MultiplyAdd(elapsedTime, pCurPart->m_vVel);//pCurPart->m_vPos += elapsedTime*pCurPart->m_vVel;

		contents = gEngfuncs.PM_PointContents(pCurPart->m_vPos, NULL);// check contents right after updating origin, because we may not need anything else
		if (contents == CONTENTS_SOLID && FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))// XDM3038c: hack?
		{
			RemoveParticle(pCurPart);// XDM3037
			continue;
		}
		if (m_iDestroyContents != 0 && ContentsArrayHas(m_iDestroyContents, contents))
		{
			RemoveParticle(pCurPart);
			continue;
		}

		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS) && !FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// XDM3038c
		{
			/* gets stuck in local player anyway if (m_fParticleCollisionSize > 0.0f)
				gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos + pCurPart->m_vVel.Normalize()*m_fParticleCollisionSize, m_iTraceFlags, ignore_pe, &pmtrace);// XDM3038c
			else
				gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, ignore_pe, &pmtrace);// OLD: FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP)?PM_WORLD_ONLY:PM_STUDIO_BOX*/
			if (m_fParticleCollisionSize > 0.0f)
				pmtrace = PM_PlayerTraceEx(pCurPart->m_vPosPrev, pCurPart->m_vPos + pCurPart->m_vVel.Normalize()*m_fParticleCollisionSize, m_iTraceFlags, HULL_POINT, CParticleSystem_PhysEntIgnore);
			else
				pmtrace = PM_PlayerTraceEx(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, HULL_POINT, CParticleSystem_PhysEntIgnore);

			if (pmtrace.fraction != 1.0f)// collision detected
			{
				if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))
				{
					RemoveParticle(pCurPart);
					continue;// XDM3037
				}
				else if (m_OnParticleCollide == NULL || m_OnParticleCollide(this, pCurPart, m_pOnParticleCollideData, &pmtrace) == true)// custom callback can disallow further frocessing
				{
					if (m_fBounceFactor > 0.0f)// reflect velocity, bounce
					{
						pCurPart->m_vVel.MirrorByVector(pmtrace.plane.normal);
						pCurPart->m_vVel *= m_fBounceFactor;
					}
					else if (m_fBounceFactor == 0.0f)// stick
						pCurPart->m_vVel.Clear();
					else// slide
					{
						PM_ClipVelocity(pCurPart->m_vVel, pmtrace.plane.normal, pCurPart->m_vVel, 1.0f);
						vec_t fSpeed = pCurPart->m_vVel.NormalizeSelf();
						fSpeed -= (float)(m_fFriction*elapsedTime);// friction is applied constantly every frame, so make it time-based
						pCurPart->m_vVel *= fSpeed;
					}
				}
			}
			//already included in acceleration	else
			//	pCurPart->m_vPos.z -= elapsedTime * g_cl_gravity;
		}

		if (!pCurPart->IsExpired())
		{
			// VELOCITY
			pCurPart->m_vVel.MultiplyAdd(elapsedTime, pCurPart->m_vAccel);//pCurPart->m_vVel += elapsedTime*pCurPart->m_vAccel;
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDGRAVITY))// XDM3038a
				pCurPart->m_vVel.z -= elapsedTime*g_cl_gravity*m_fParticleWeight;

			if (m_OnParticleUpdate == NULL || m_OnParticleUpdate(this, pCurPart, m_pOnParticleUpdateData, time, elapsedTime) == true)
			{
				// FRAME
				if (iLastFrame != m_iFrame)// time to change the frame
				{
					if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
						pCurPart->FrameRandomize();
					else if (m_fFrameRate != 0.0f)
						pCurPart->FrameIncrease();// TODO? RENDERSYSTEM_FLAG_LOOPFRAMES ?
				}
				// COLOR
				pCurPart->UpdateColor(elapsedTime);
				pCurPart->UpdateSize(elapsedTime);
				pCurPart->UpdateEnergyByBrightness();// FIXME: this concept is bad
			}
		}
	}
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS) && !FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// XDM3038c
		gEngfuncs.pEventAPI->EV_PopPMStates();

	if (IsShuttingDown())// XDM3037
	{
		if (m_iNumParticles == 0)// FAIL!!!! || (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_SIMULTANEOUS) && m_iNumParticles == m_iMaxParticles))
			return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Warning: Do not change system settings here (e.g. m_vStartMaxs)
// Input  : pParticle - a new particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CParticleSystem::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	Vector vecRand(0,0,0);
	// ORIGIN
	// WARNING! Don't add system origin to particle origin yet!
	if (m_iStartType == PSSTARTTYPE_POINT)
	{
		VectorRandom(vecRand, -1, 1);
		vecRand.NormalizeSelf();
		pParticle->m_vPos.Clear();
	}
	else if (m_iStartType == PSSTARTTYPE_SPHERE)
	{
		Vector rangles;
		VectorRandom(rangles, 0.0f, 360.0f);
		AngleVectors(rangles, vecRand, NULL, NULL);
		vecRand *= RANDOM_FLOAT(m_fStartRadiusMin, m_fStartRadiusMax);//*m_fScale?
		pParticle->m_vPos = vecRand;
	}
	else if (m_iStartType == PSSTARTTYPE_BOX)
	{
		//if (!m_vStartMins.IsZero() && !m_vStartMaxs.IsZero())// particles start inside a box
		{
			VectorRandom(vecRand, m_vStartMins, m_vStartMaxs);
			pParticle->m_vPos = vecRand;
		}
	}
	else if (m_iStartType == PSSTARTTYPE_LINE)
	{
		Vector vLine(m_vStartMaxs); vLine -= m_vStartMins;//Vector vLine = m_vStartMaxs - m_vStartMins;
		//pParticle->m_vPos = m_vStartMins + vLine * (float)pParticle->index/(float)m_iMaxParticles;
		pParticle->m_vPos = vLine;
		pParticle->m_vPos *= (float)pParticle->index/(float)m_iMaxParticles;
		pParticle->m_vPos += m_vStartMins;
	}
	else if (m_iStartType == PSSTARTTYPE_ENTITYBBOX)
	{
		if (m_pLastFollowedEntity)
		{
			if (m_pLastFollowedEntity->model)
				VectorRandom(vecRand, m_pLastFollowedEntity->model->mins, m_pLastFollowedEntity->model->maxs);
			else
				VectorRandom(vecRand, m_pLastFollowedEntity->curstate.mins, m_pLastFollowedEntity->curstate.maxs);

			pParticle->m_vPos = vecRand;
		}
		else if (m_iFollowModelIndex)
		{
			VectorRandom(pParticle->m_vPos, m_vStartMins, m_vStartMaxs);
		}
		else
		{
			DBG_RS_PRINT("InitializeParticle(): PSSTARTTYPE_ENTITYBBOX without m_pLastFollowedEntity!\n");
			pParticle->m_vPos.Clear();
		}
	}
	else if (m_iStartType == PSSTARTTYPE_CYLINDER)
	{
		Vector rangles(0.0f, RANDOM_FLOAT(0,360), 0.0f);
		AngleVectors(rangles, vecRand, NULL, NULL);
		vecRand *= RANDOM_FLOAT(m_fStartRadiusMin, m_fStartRadiusMax);
		vecRand.z = RANDOM_FLOAT(m_vStartMins.z, m_vStartMaxs.z);
		pParticle->m_vPos = vecRand;
	}
	else
		pParticle->m_vPos.Clear();// will be = m_vecOrigin V;

	// DIRECTION, VELOCITY
	// move towards the center point
	if (m_iMovementType == PSMOVTYPE_DIRECTED)
	{
		// fail pParticle->m_vVel.Clear();
		pParticle->m_vVel = vecRand;
		pParticle->m_vVel *= m_vSpread;
		pParticle->m_vVel += m_vDirection;
	}
	else if (m_iMovementType == PSMOVTYPE_OUTWARDS)
	{
		pParticle->m_vVel = vecRand;//FAIL! = 0//pParticle->m_vVel = pParticle->m_vPos;// added later! - m_vecOrigin;
		pParticle->m_vVel.NormalizeSelf();
//		pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);
	}
	else if (m_iMovementType == PSMOVTYPE_INWARDS)
	{
		pParticle->m_vVel = /*m_vecOrigin <- still local coords! added later!*/ - pParticle->m_vPos;//==vecRand;//==pParticle->m_vPos - m_vecOrigin;
//		pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);
	}
	else if (m_iMovementType == PSMOVTYPE_RANDOM)
	{
		VectorRandom(pParticle->m_vVel);
//		pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);
	}
	else if (m_iMovementType == PSMOVTYPE_TOWARDSPOINT)// same as DIRECTED, but direction is changed dynamically
	{
		m_vDirection = m_vDestination; m_vDirection -= m_vecOrigin;
		pParticle->m_vVel = m_vDirection;
//		pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);
		pParticle->m_vVel += vecRand;
	}

	if (!m_vSpread.IsZero())
		pParticle->m_vVel *= m_vSpread;// this fails if velocity is zero
	if (m_iMovementType == PSMOVTYPE_DIRECTED)// compatibility: add direction after limiting spread
		pParticle->m_vVel += m_vDirection;

	pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);
	if (!FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_DONTFOLLOW) && m_pLastFollowedEntity != NULL)// XDM3038b: add some of the moving entity velocity
		pParticle->m_vVel += m_pLastFollowedEntity->curstate.velocity;

	// To absolute coordinates: now it's safe to add system origin!
	if (fInterpolaitonMult > 0.0f)// interpolate position between CURRENT and PREVIOUS origins
	{
		Vector vDelta(m_vecOrigin); vDelta -= m_vecOriginPrev; vDelta *= fInterpolaitonMult;// we're moving point backwards
		pParticle->m_vPos -= vDelta;
	}
	pParticle->m_vPos += m_vecOrigin;
	pParticle->m_vPosPrev = pParticle->m_vPos;
	pParticle->m_vPosPrev -= pParticle->m_vVel.Normalize();// just to start drawing in proper direction

	//int contents = gEngfuncs.PM_PointContents(pCurPart->m_vPos, NULL);// this would be okay, but overall particle distribution will be uneven
	//if (m_iDestroyContents != 0 && ContentsArrayHas(m_iDestroyContents, contents))
	//	return false;

	// ACCELERATION
	pParticle->m_vAccel.Clear();

	// OTHER PARAMETERS
	pParticle->m_fEnergy = m_fEnergyStart;
	pParticle->m_iFlags = 0;
	pParticle->m_pTexture = m_pTexture;

	// CALLBACK
	if (m_OnParticleInitialize == NULL || m_OnParticleInitialize(this, pParticle, m_pOnParticleInitializeData, fInterpolaitonMult) == true)
	{
		float s = m_fScale*RANDOM_FLOAT(m_fParticleScaleMin, m_fParticleScaleMax);// randomize both, don't distort proportions
		pParticle->SetSizeFromTexture(s, s);// keep proportions, use same scale for X and for Y. IMPORTANT: particles may have different textures so don't use system's m_fSize*!
		pParticle->m_fSizeDelta = m_fScaleDelta;

		pParticle->SetColor(m_fColorCurrent);
		pParticle->SetColorDelta(m_fColorDelta);
		//pParticle->m_fWeight = m_fParticleWeight;
		//pParticle->m_fWeightDelta = 0.0;

		// FRAME
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))
			pParticle->FrameRandomize();
		else
			pParticle->m_iFrame = m_iFrame;
	}
}

//-----------------------------------------------------------------------------
// Purpose: extremely useful for explosion and wind effects
// Input  : origin - 
//			force - 
//			radius - 
//			point - 
//-----------------------------------------------------------------------------
void CParticleSystem::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
	/*UNDONEfor (uint32 i = 0; i < m_iNumParticles; ++i)
	{
		float dist = Length(m_pParticleList[i].m_vPos - origin);
		if (dist < radius)
		{
			if (point)
				m_pParticleList[i].m_vAccel += force*((radius-dist)/radius);
			else
				m_pParticleList[i].m_vAccel += force;
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Check if user setting allows this system
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CParticleSystem::CheckFxLevel(void) const
{
	return ((int)g_pCvarParticles->value >= m_iFxLevel);
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
// m_vecAngles defines GLOBAL PS orientation, not for particles!
//-----------------------------------------------------------------------------
void CParticleSystem::Render(void)
{
	// Particles may be far away from their initial origin, but the designer should've thought of that and take measures
	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_DRAWALWAYS))
	{
		if (m_iStartType == PSSTARTTYPE_POINT || m_iStartType == PSSTARTTYPE_SPHERE)
		{
			if (!Mod_CheckBoxInPVS(m_vecOrigin, m_vecOrigin))// Bad check: any obstruction hides the effect! if (!PointIsVisible(m_vecOrigin))
				return;
		}
		else if (m_iStartType == PSSTARTTYPE_BOX || m_iStartType == PSSTARTTYPE_LINE || m_iStartType == PSSTARTTYPE_ENTITYBBOX || m_iStartType == PSSTARTTYPE_CYLINDER)
		{
			if (!Mod_CheckBoxInPVS(m_vecOrigin+m_vStartMins, m_vecOrigin+m_vStartMaxs))
				return;
		}
		else if (m_iStartType == PSSTARTTYPE_CYLINDER)
		{
			if (!Mod_CheckBoxInPVS(Vector(m_vecOrigin.x - m_fStartRadiusMax, m_vecOrigin.y - m_fStartRadiusMax, m_vecOrigin.z + m_vStartMins.z),
								Vector(m_vecOrigin.x + m_fStartRadiusMax, m_vecOrigin.y + m_fStartRadiusMax, m_vecOrigin.z + m_vStartMaxs.z)))
				return;
		}
	}

	CParticle *p = NULL;
	Vector v_up, v_right, v_up_dir, v_right_dir, v_up_dir2, v_ang(g_vecViewAngles);
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ZROTATION))
	{
		v_ang.x = 0.0f;
		v_ang.z = 0.0f;
	}
	AngleVectors(v_ang, NULL, v_right, v_up);
	v_right_dir.Set(1.0f,0.0f,0.0f);// UNDONE: use normal? currently we don't care much about right direction

#if defined (RS_SORTED_PARTICLES_RENDER)
	for (pTexture == ??? textures)// Need a list of used textures (PSCustom may use textures from other systems!)
	{
		for (int frame = 0; frame < pTexture->numframes; ++frame)
		{
			p->RenderBegin(m_iRenderMode, true);
			for (p = ?; )
				p->RenderWrite(v_right_dir, v_up_dir2, m_iRenderMode);
			p->RenderEnd();
		}
	}
#else
	for (uint32 i = 0; i < m_iMaxParticles; ++i)// XDM3037: unstable m_iNumParticles; ++i)
	{
		p = &m_pParticleList[i];
		if (p->IsExpired())
			continue;

		if (!PointIsVisible(p->m_vPos))// faster?
			continue;

		if (m_iDrawContents != 0 && !ContentsArrayHas(m_iDrawContents, gEngfuncs.PM_PointContents(p->m_vPos, NULL)))
			continue;

		if (FBitSet(p->m_iFlags, PARTICLE_ORIENTED))// R&U in p->Render() must point along face/texture sides
		{
			//delta = (p->m_vPos - g_vecViewOrigin).Normalize();
			v_up_dir = p->m_vPosPrev;// This vector points to particle's previous position. Remember: Velocity is zero!
			v_up_dir -= p->m_vPos;
			v_up_dir.NormalizeSelf();
			v_up_dir2 = CrossProduct(v_right_dir, v_up_dir);
			//v_right_dir.NormalizeSelf();
			// DEBUG gEngfuncs.pEfxAPI->R_BeamPoints(p->m_vPos, p->m_vPos+v_right_dir*32, g_iModelIndexLaser, 4.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0, 0.0f, 1,0,0);
			//gEngfuncs.pEfxAPI->R_BeamPoints(p->m_vPos, p->m_vPos+v_up_dir2*32, g_iModelIndexLaser, 4.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0, 0.0f, 0,1,0);
			//gEngfuncs.pEfxAPI->R_BeamPoints(p->m_vPos, p->m_vPos+v_up_dir*32, g_iModelIndexLaser, 4.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0, 0.0f, 0,0,1);
			p->Render(v_right_dir, v_up_dir2, m_iRenderMode, true);
		}
		else
			p->Render(v_right, v_up, m_iRenderMode);
	}
#endif
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);// ?
}

//-----------------------------------------------------------------------------
// Purpose: Fast and safe to call often.
// Some systems may require two or even three points, so leave it as virtual.
//-----------------------------------------------------------------------------
void CParticleSystem::FollowUpdatePosition(void)
{
	CParticleSystem::BaseClass::FollowUpdatePosition();// call this first to set m_vecAngles

	// Update mins/maxs
	if (m_iStartType == PSSTARTTYPE_ENTITYBBOX && !FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_DONTFOLLOW))//if (m_StartInsideEntity)// have to redo this every frame so moving entities will work
	{
		if (m_iFollowEntity)// should be following something
		{
			if (m_pLastFollowedEntity)
			{
				//m_fStartRadiusMin = 0;
				//m_fStartRadiusMax = 0;
				// UNDONE: start inside brush volume (needs 3D triangulation and choosing of pyramids by volume)
				if (m_pLastFollowedEntity->model == NULL)
				{
					m_vStartMins = m_pLastFollowedEntity->baseline.mins-m_pLastFollowedEntity->origin;// curstate is zero!
					m_vStartMaxs = m_pLastFollowedEntity->baseline.maxs-m_pLastFollowedEntity->origin;// TODO: UNDONE: TESTME: Absolute????
				}
				else if (m_pLastFollowedEntity->model->type == mod_brush)// brush models have absolute coordinates in mins/maxs
				{
					m_vStartMins = m_pLastFollowedEntity->model->mins-m_pLastFollowedEntity->origin;
					m_vStartMaxs = m_pLastFollowedEntity->model->maxs-m_pLastFollowedEntity->origin;
				}
				else
				{
					m_vStartMins = m_pLastFollowedEntity->model->mins;
					m_vStartMaxs = m_pLastFollowedEntity->model->maxs;
				}
			}
			else if (m_iFollowModelIndex > 0)// XDM3038c
			{
				model_s *pModel = IEngineStudio.GetModelByIndex(m_iFollowModelIndex);
				if (pModel)
				{
#if defined (_DEBUG)
					cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(m_iFollowEntity);// don't care if it's up to date
					if (pEntity)
					{
						if (pEntity->model != NULL && pEntity->model != pModel)
							conprintf(0, " %s[%u] ERROR: model index %d does not belong to entity %d!\n", GetClassName(), GetIndex(), m_iFollowModelIndex, m_iFollowEntity);
					}
#endif
					if (pModel->type == mod_brush)// brush models have absolute coordinates in mins/maxs
					{
						m_vStartMins = pModel->mins - m_vecOrigin;// m_vecOrigin will be added later when converting from relative to absolute coordinates
						m_vStartMaxs = pModel->maxs - m_vecOrigin;
					}
					else
					{
						m_vStartMins = pModel->mins;
						m_vStartMaxs = pModel->maxs;
					}
				}
				else
				{
					conprintf(0, " ERROR: %s %u: model index %d was not found!\n", GetClassName(), GetIndex(), m_iFollowModelIndex);
					m_iFollowModelIndex = 0;// not found anyway
				}
			}
			/*else entity may have gone out of sight
			{
				conprintf(2, "CParticleSystem::FollowUpdatePosition() error: PSSTARTTYPE_ENTITYBBOX without entity!\n");
				m_iStartType = PSSTARTTYPE_POINT;//m_StartInsideEntity = 0;
			}*/
		}
	}
	// Try setting direction from entity's angles
	if (m_iFollowEntity > 0 && m_pLastFollowedEntity)
	{
		if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_DIRECTTOTARGET))// XDM3038c
		{
			if (m_iMovementType == PSMOVTYPE_TOWARDSPOINT)
				m_vDestination = m_pLastFollowedEntity->origin;
		}
		if (!FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_NOANGLES|RENDERSYSTEM_FFLAG_DONTFOLLOW))
		{
			AngleVectors(m_vecAngles, m_vDirection, NULL, NULL);
			m_vDirection.NormalizeSelf();
		}
	}
}
