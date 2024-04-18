/*----------------------------------------------------------------
*	Render System
*
*	Copyright © 2001-2017, Xawari. All rights reserved.
*	Created for XDM, a Half-Life modification.
*	http://x.netheaven.ru
*
*	Render System code is provided under the terms of
*	Mozilla Public License version 2.0
*
*	This code partially uses software technology created by
*	Valve LLC, but not required to.
*
*	Provided terms are aplicable to all RenderSystem-related files
*	which contain code directly or indirectly derived from
*	CRenderSystem, CRenderManager and CParticle classes.
*
*   This source code contains no secret or confidential information.
*
*  USAGE NOTES:
* - Always check everything for NULLs.
* - Please do not use vectors other than class Vector.
* - Try to reuse as much code as possible and support OOP model.
* - Optimize your code at the time you write it, consider every operator.
* - Please standardize commentaries as you see it is done here.
*
*  NEW:
* - InitializeSystem() should no longer be called from constructor.
* - most functionality is now in CParticleSystem and CPSCustom,
*   other PS classes are used for compatibility.
*
*  TODO:
* - implement proper dynamic vector of gravity (and apply it every frame).
*---------------------------------------------------------------*/
/*#include "vector.h"
#include "const.h"
#include "cl_dll.h"
#include "cl_entity.h"
#include "cl_enginefuncs.h"*/
#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "event_api.h"
#include "studio.h"
#include "bsputil.h"
#include "cl_fx.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include <new>
#include <exception>

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: custom fail-safe allocation mechanism
// Warning: This exception actually happened! Compile with exception support!!!
// Input  : stAllocateBlock - 
// Output : void *
//-----------------------------------------------------------------------------
void *CRenderSystem::operator new(size_t stAllocateBlock)
{
	try
	{
		void *pNewMemBlock = ::operator new(stAllocateBlock);// will overflow eventually
		//void *pNewMemBlock = IEngineStudio.Mem_Calloc(1, stAllocateBlock);// good idea, but we have no corresponding Free() function
		ASSERT(pNewMemBlock != NULL);
		//memset(pNewMemBlock, 0, stAllocateBlock);
		//((CRenderSystem *)pNewMemBlock)->index = RS_INDEX_INVALID;
		if (g_pRenderManager)
			g_pRenderManager->m_iAllocatedSystems++;

		return pNewMemBlock;
	}
	catch (std::bad_alloc &ba)
	{
		//DBG_PRINTF(ba.what());
		conprintf(0, "CRenderSystem::new ERROR: unable to allocate memory! %s\n", ba.what());
		DBG_FORCEBREAK
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: custom fail-safe allocation mechanism
// Input  : *pMem - the memory block
//-----------------------------------------------------------------------------
void CRenderSystem::operator delete(void *pMem)
{
	try
	{
		if (g_pRenderManager)
			g_pRenderManager->m_iAllocatedSystems--;
			//g_pRenderManager->OnSystemRemoved((CRenderSystem *)pMem);// !?!?! this);

		::operator delete(pMem);
	}
	catch (std::bad_alloc &ba)
	{
		conprintf(0, "CRenderSystem::delete ERROR: unable to free memory! %s\n", ba.what());
		DBG_FORCEBREAK
	}
}

//-----------------------------------------------------------------------------
// Purpose: Default constructor.
// Warning: All constructors must call ResetParameters()!
// Warning: All derived classes must have constructors call this one like: : CMySystem::BaseClass()
//-----------------------------------------------------------------------------
CRenderSystem::CRenderSystem(void)
{
	m_szName[0] = 0;
	index = RS_INDEX_INVALID;
	m_UUID[0] = 0;
	DBG_RS_PRINT("CRenderSystem()");
	m_ShuttingDown = false;
	m_RemoveNow = false;
	m_pNext = NULL;
	ResetParameters();
	//////////////////////////////////////////////////////////////////
	// NOTE: KB00001818: C++ bullshit
	// Here typeof(this) == CRenderSystem even if the constructor is forward-called by a subclass!
	// This means CRenderSystem::ResetParameters() will get called and NOT CMySystem::ResetParameters()!!
	// BUT If we call ResetParameters() manually from a constructor, it will recursively call ResetParameters() of its superclasses!
	// If we insert ResetParameters() in every constructor, It will be called 1 time in top class and N times in base class, where N is level of inheritance.
	// BUT If we make ResetParameters() non-virtual for every class, we lose the ability to reset ALL parameters!!
	// SO we gonna write even more abstractions! YAY!
	//////////////////////////////////////////////////////////////////
}

//-----------------------------------------------------------------------------
// Purpose: Main constructor for external use (with all nescessary parameters)
//			e,g, g_pRenderManager->AddSystem(new CRenderSystem(a,b,...), 0, -1);
// Warning: All constructors must call ResetParameters()!
// Input  : origin - absolute position
//			velocity - 
//			angles - 
//			pTexture - loaded sprite (used as texture)
//			r_mode - kRenderTransAdd
//			r,g,b - RGB (0...255 each)
//			a - alpha (0...1)
//			adelta - alpha velocity, any value is acceptable
//			scale - positive values base on texture size, negative are absolute
//			scaledelta - scale velocity, any value is acceptable
//			framerate - texture frame rate (if animated), == FPS if negative
//			timetolive - 0 means the system removes itself after the last frame
// Accepts flags: RENDERSYSTEM_FLAG_RANDOMFRAME | LOOPFRAMES | etc.
//-----------------------------------------------------------------------------
CRenderSystem::CRenderSystem(const Vector &origin, const Vector &velocity, const Vector &angles, model_t *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive)
{
	m_szName[0] = 0;
	index = RS_INDEX_INVALID;// the only good place for this
	m_UUID[0] = 0;
	DBG_RS_PRINT("CRenderSystem(...)");
	m_ShuttingDown = false;
	m_RemoveNow = false;
	m_pNext = NULL;
	ResetParameters();// should be called in all constructors so no parameters will left uninitialized
	if (!InitTexture(pTexture))
	{
		ShutdownSystem();
		m_RemoveNow = true;// tell render manager to delete this system
		return;// no texture - no system
	}
	m_vecOrigin = origin;
	m_vecVelocity = velocity;
	m_vecAngles = angles;
	NormalizeAngles(m_vecAngles);
	m_color.Set(r,g,b,a*255.0f);// compatibility
	//m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;
	m_fFrameRate = framerate;
	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;// persist forever OR cycle through all texture frames and die (depends on Update function)
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor. Destroy data by calling FreeData(), not directly!
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRenderSystem::~CRenderSystem(void)
{
	DBG_RS_PRINT("~CRenderSystem()");
	FreeData();
	if (g_pRenderManager)
		g_pRenderManager->OnSystemRemoved(this);
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
// Warning: NEVER RESET: m_pNext, m_RemoveNow, index!
//-----------------------------------------------------------------------------
/*void CRenderSystem::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRenderSystem::FreeData(void)// XDM3038c
{
	DBG_RS_PRINT("FreeData()");
	//m_fStartTime = 0.0f;
	m_iRenderMode = 0;
	m_iFrame = 0;
	m_iFlags = 0;
	m_pLastFollowedEntity = NULL;
	m_pTexture = NULL;
	m_pOnUpdateData = NULL;
	m_OnUpdate = NULL;
	m_iState = RSSTATE_DISABLED;// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRenderSystem::ResetAllParameters(void)
{
	DBG_RS_PRINT("ResetAllParameters()");
	//CRenderSystem::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRenderSystem::ResetParameters(void)
{
	DBG_RS_PRINT("ResetParameters()");
	m_vecOrigin.Clear();
	m_vecVelocity.Clear();
	m_vecAngles.Clear();
	m_vecOriginPrev.Clear();
	m_vecOriginStart.Clear();
	m_vOffset.Clear();
	m_fStartTime = 0.0f;// !
	m_fDieTime = 0.0f;
	m_iFrame = 0;
	//m_iFramePrevious = 0;
	m_fFrameAccumulator = 0.0f;
	m_fFrameRate = 20.0f;
	m_fScale = 1.0f;
	m_fScaleDelta = 0.0f;
	m_fSizeX = 0.0f;// XDM3038b: so it will be reset to texture dimensions
	m_fSizeY = 0.0f;
	m_color.SetWhite();
	//m_fColorCurrent will be set later, don't waste time on it
	m_fColorDelta[0] = 0.0f;
	m_fColorDelta[1] = 0.0f;
	m_fColorDelta[2] = 0.0f;
	m_fColorDelta[3] = 0.0f;
	//m_fBrightness = 1.0f;
	m_fBounceFactor = 0.8f;// XDM3038c
	m_fFriction = 0.0f;// XDM3038c
	m_iRenderMode = 0;
	m_iRenderEffects = 0;
	m_iDestroyContents = 0;
	m_iDrawContents = 0;
	m_iDoubleSided = 0;
	m_iFxLevel = 0;// XDM3038c
	m_iFlags = 0;
	m_iFollowFlags = 0;
	m_iFollowEntity = -1;// does not follow any entities by default
	m_iFollowModelIndex = 0;// XDM3038c
	m_iFollowAttachment = SHRT_MAX;// anything's fine as long as larger than MAXSTUDIOWHATEVER
	//m_iFollowBoneIndex = 0;
	m_pLastFollowedEntity = NULL;
	m_pTexture = NULL;
	m_pOnUpdateData = NULL;// XDM3037a
	m_OnUpdate = NULL;// XDM3037a
	m_iState = RSSTATE_DISABLED;// XDM3038c
	//m_ShuttingDown = false;
	//m_RemoveNow = false;
	// DO NOT TOUCH m_pNext
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRenderSystem::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "vOrigin") == 0)// allow overriding of origin?
		return StringToVec(szValue, m_vecOrigin);
	else if (strcmp(szKey, "vVelocity") == 0)
		return StringToVec(szValue, m_vecVelocity);
	else if (strcmp(szKey, "vAngles") == 0)
	{
		if (StringToVec(szValue, m_vecAngles))
			NormalizeAngles(m_vecAngles);
		else
			return false;
	}
	//else if (strcmp(szKey, "vAnglesDelta") == 0)
	//	return StringToVec(szValue, m_vecAnglesDelta);
	else if (strcmp(szKey, "vOffset") == 0)
		return StringToVec(szValue, m_vOffset);
	else if (strcmp(szKey, "fScale") == 0)
		m_fScale = atof(szValue);
	else if (strcmp(szKey, "fScaleDelta") == 0)
		m_fScaleDelta = atof(szValue);
	/*else if (strcmp(szKey, "fBrightness") == 0)
		m_fBrightness = atof(szValue);
	else if (strcmp(szKey, "fBrightnessDelta") == 0)
		m_fColorDelta[3] = atof(szValue);*/
	else if (strcmp(szKey, "iFrame") == 0)
		m_iFrame = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "fFrameRate") == 0)
		m_fFrameRate = atof(szValue);
	else if (strcmp(szKey, "Color4b") == 0)// it should not be used during runtime so it's useless to implement per-element access
		return StringToColor(szValue, m_color);//m_fBrightness = (float)(m_color.a)/255.0f;
	else if (strbegin(szKey, "fColorCurrent4f") > 0)
		return ParseArray4f(szKey, szValue, m_fColorCurrent);
	else if (strbegin(szKey, "ColorDelta4f") > 0)
		return ParseArray4f(szKey, szValue, m_fColorDelta);
	else if (strcmp(szKey, "fSizeX") == 0)
		m_fSizeX = atof(szValue);
	else if (strcmp(szKey, "fSizeY") == 0)
		m_fSizeY = atof(szValue);
	else if (strcmp(szKey, "fBounceFactor") == 0)// XDM3038c
		m_fBounceFactor = clamp(atof(szValue), -1.0f, 20.0f);
	else if (strcmp(szKey, "fFriction") == 0)// XDM3038c
		m_fFriction = clamp(atof(szValue), 0.0f, 1.0f);
	else if (strcmp(szKey, "iRenderMode") == 0)
		m_iRenderMode = atoi(szValue);
	else if (strcmp(szKey, "iRenderEffects") == 0)
		m_iRenderEffects = atoi(szValue);
	else if (strcmp(szKey, "iDrawContents") == 0)
		m_iDrawContents = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "iDestroyContents") == 0)
		m_iDestroyContents = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "iDoubleSided") == 0)
		m_iDoubleSided = atoi(szValue);
	else if (strcmp(szKey, "iFxLevel") == 0)
		m_iFxLevel = atoi(szValue);
	else if (strcmp(szKey, "iState") == 0)
		SetState(strtoul(szValue, NULL, 10));
	else if (strcmp(szKey, "iFlags") == 0)
		m_iFlags = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "iFollowFlags") == 0)
		m_iFollowFlags = strtoul(szValue, NULL, 10);
	// pointless	else if (strcmp(szKey, "FollowEntity") == 0)
	//	m_iFollowEntity = atoi(szValue);
	else if (strcmp(szKey, "iFollowAttachment") == 0)
		m_iFollowAttachment = atoi(szValue);
	else if (strcmp(szKey, "szTexture") == 0)
	{
		// bad m_pSprite = gEngfuncs.CL_LoadModel(szToken, &m_iSprite);
		// works when precached on server m_iSprite = gEngfuncs.pEventAPI->EV_FindModelIndex(szToken);
		m_pTexture = (model_t *)gEngfuncs.GetSpritePointer(SPR_Load(szValue));// client-only, no modelindex
		if (m_pTexture == NULL)//if (m_iSprite <= 0)
		{
			// TODO: replace bad sprite with default white sprite? nah, no time
			conprintf(0, " ERROR: %s %u has invalid %s \"%s\"! Using default.\n", GetClassName(), GetIndex(), szKey, szValue);
			m_pTexture = g_pSpritePTracer;
			ASSERT(m_pTexture != NULL);
		}
		else
		{
			if (!InitTexture(m_pTexture))//m_pSprite = IEngineStudio.GetModelByIndex(m_iSprite);
			{
				conprintf(0, " ERROR: %s %u InitTexture(\"%s\") failed!\n", GetClassName(), GetIndex(), szValue);
				return false;
			}
		}
	}
	else if (strcmp(szKey, "fLifeTime") == 0)
	{
		float fLifeTime = atof(szValue);
		if (fLifeTime <= 0.0f)
			m_fDieTime = 0.0f;
		else
			m_fDieTime = gEngfuncs.GetClientTime() + fLifeTime;
	}
	else return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize system after all user parameters were set
// Must be called from class constructor or after setting parameters manually.
// Warning: Be very careful when overriding this function!!
// Warning: Must be safe to call multiple times!
//-----------------------------------------------------------------------------
void CRenderSystem::InitializeSystem(void)
{
	DBG_RS_PRINT("InitializeSystem()");

	if (m_pTexture && FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))// XDM3038c
		m_iFrame = RANDOM_LONG(0, m_pTexture->numframes - 1);
	else
		m_iFrame = 0;

	//m_iFramePrevious = 0;
	m_fFrameAccumulator = 0.0f;
	m_ShuttingDown = false;
	m_RemoveNow = false;
	m_fStartTime = gHUD.m_flTime;// XDM3035b: easy tracking of system lifetime

	// XDM3035: derived classes depend on this code
	if (m_fScale > 0.0f)// get texture sizes
	{
		if (m_pTexture && m_fSizeX <= 0.0f && m_fSizeY <= 0.0f)// some systems do not have texture
		{
			m_fSizeX = (m_pTexture->maxs[1] - m_pTexture->mins[1])*0.5f;
			m_fSizeY = (m_pTexture->maxs[2] - m_pTexture->mins[2])*0.5f;
		}
	}
	else
		m_fScale = -m_fScale;

	if (m_fColorDelta[3] < 0){
		ASSERT(m_color.a > 0);//ASSERT(m_fBrightness > 0.0f);
	}
	else if (m_fColorDelta[3] > 0){
		ASSERT(m_color.a < UCHAR_MAX);//ASSERT(m_fBrightness < 1.0f);
	}
	m_color.Get4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],m_fColorCurrent[3]);// m_color -> m_fColorCurrent
	//SetState(RSSTATE_ENABLED);// no, let Manager decide

	m_vecOriginStart = m_vecOrigin;// remember initial position
	// Useless if Initialize() is called from constructor when follow index is not set yet
	FollowEntity();// XDM3038c: get initial origin <- FollowUpdatePosition();
	m_vecOriginPrev = m_vecOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: Check and set texture (sprite) for this system.
// Input  : *pTexture - an existing (loaded/precached) sprite
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::InitTexture(model_t *pTexture)
{
	DBG_RS_PRINT("InitTexture()");
	if (pTexture == NULL)
	{
		conprintf(1, "CRenderSystem::InitTexture(NULL) failed!\n");
		return false;
	}
	if (pTexture->type != mod_sprite)
	{
		conprintf(1, "CRenderSystem::InitTexture(%s) failed: not a sprite!\n", pTexture->name);
		return false;
	}
	m_pTexture = pTexture;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Load texture by index. WARNING: sprite must be precached on server!
// Input  : texture_index - precached sprite index
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
#if 0
bool CRenderSystem::InitTexture(const int &texture_index)
{
	if (texture_index <= 0)
		return false;

	return InitTexture(IEngineStudio.GetModelByIndex(texture_index));
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
//			This function may not be called every frame by the engine.
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRenderSystem::Update(const float &time, const double &elapsedTime)
{
//	DBG_RS_PRINT("Update()");
	// XDM3038c: revisited m_ShuttingDown = false;// TODO: revisit?

	if (m_fDieTime > 0.0f && m_fDieTime <= time)
	{
		DBG_RS_PRINTF("%s[%d]\"%s\": Update() shutting down: m_fDieTime <= time\n", GetClassName(), GetIndex(), GetUID());
		ShutdownSystem();
	}
	// this system is lightweight if (!CheckFxLevel())// XDM3038c
	//	return 0;

	m_fColorCurrent[3] += m_fColorDelta[3]*(float)elapsedTime;// fix for software mode: don't allow to be negative
	// old m_fBrightness += m_fColorDelta[3]*(float)elapsedTime;
	// old m_color.a += (int)(m_fColorDelta[3]*255.0f*(float)elapsedTime);

	if (!IsShuttingDown())// TODO: revisit, this is common for all render systems
	{
		if (m_fColorDelta[3] < 0.0f && m_fColorCurrent[3] <= 0)//m_fBrightness <= 0.0f)// should we?
		{
			m_fColorCurrent[3] = 0.0f;//m_fBrightness = 0.0f;
			DBG_RS_PRINTF("%s[%d]\"%s\": Update() shutting down: m_fColorDelta[3] < 0\n", GetClassName(), GetIndex(), GetUID());
			ShutdownSystem();
		}
		else if (m_fColorDelta[3] > 0.0f && m_fColorCurrent[3] >= 1.0f)//m_fBrightness >= 1.0f)// overbrightening?!	
		{
			m_fColorCurrent[3] = 1.0f;//m_fBrightness = 1.0f;
			//ShutdownSystem();
		}
		//else dangerous condition! eternal system possible!
	}

	// allow inversion?	if (!IsShuttingDown() && m_fScale <= 0.000001 && m_fScaleDelta < 0.0)
	//		ShutdownSystem();
	// overscaling?	else if (m_fScale > 65536 && m_fScaleDelta > 0.0)
	//		ShutdownSystem();

	UpdateFrame(time, elapsedTime);

	if (!IsShuttingDown() && CheckFxLevel())// all vital calculations and checks should be made before this point
	{
		FollowEntity();
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDGRAVITY))// update only if not following???
			m_vecVelocity.z -= elapsedTime*g_cl_gravity;// XDM3038a: since we don't have directed gravity

		m_vecOrigin.MultiplyAdd(elapsedTime, m_vecVelocity);
		// no acceleration right now (nobody needed it)
		int contents = gEngfuncs.PM_PointContents(m_vecOrigin, NULL);
		if (contents == CONTENTS_SOLID && FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))// XDM3038c: hack?
		{
			ShutdownSystem();
			goto finish;
		}
		if (m_iDestroyContents != 0 && ContentsArrayHas(m_iDestroyContents, contents))
		{
			ShutdownSystem();
			goto finish;
		}
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS))// don't apply physics to hit effects // && !(pCurPart->m_iFlags & (PARTICLE_FWATERCIRCLE|PARTICLE_FHITSURFACE))
		{
			pmtrace_t pmtrace;
			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
			gEngfuncs.pEventAPI->EV_PushPMStates();
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
			gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
			gEngfuncs.pEventAPI->EV_PlayerTrace(m_vecOriginPrev, m_vecOrigin + m_vecVelocity.Normalize()*m_fSizeX/* sphere size is UNDONE! */, FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP)?PM_WORLD_ONLY:PM_STUDIO_BOX, m_pLastFollowedEntity?PM_GetPhysent(m_pLastFollowedEntity->index):PHYSENT_NONE, &pmtrace);
			gEngfuncs.pEventAPI->EV_PopPMStates();
			if (pmtrace.fraction != 1.0f)// hit surface
			{
				if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))
				{
					ShutdownSystem();
					goto finish;
				}
				else if (m_fBounceFactor > 0.0f)// reflect velocity, bounce
				{
					m_vecVelocity.MirrorByVector(pmtrace.plane.normal);
					m_vecVelocity *= m_fBounceFactor;
				}
				else if (m_fBounceFactor == 0.0f)// stick
					m_vecVelocity.Clear();
				else// slide
				{
					PM_ClipVelocity(m_vecVelocity, pmtrace.plane.normal, m_vecVelocity, 1.0f);
					m_vecVelocity *= m_fFriction*elapsedTime;// friction is applied constantly every frame, so make it time-based
				}
			}
		}
	}
	if (!IsShuttingDown())
	{
		if (m_fColorDelta[0] != 0.0f) m_fColorCurrent[0] += m_fColorDelta[0]*elapsedTime;//m_color.r = (int)((float)m_color.r + m_fColorDelta[0] * elapsedTime);
		if (m_fColorDelta[1] != 0.0f) m_fColorCurrent[1] += m_fColorDelta[1]*elapsedTime;//m_color.g = (int)((float)m_color.g + m_fColorDelta[1] * elapsedTime);
		if (m_fColorDelta[2] != 0.0f) m_fColorCurrent[2] += m_fColorDelta[2]*elapsedTime;//m_color.b = (int)((float)m_color.b + m_fColorDelta[2] * elapsedTime);
		// alpha / m_fBrightness is updated
		if (m_fScaleDelta != 0.0f) m_fScale += m_fScaleDelta*elapsedTime;
	}
finish:
	if (m_OnUpdate && CheckFxLevel())// allow to override 'IsShuttingDown()'
	{
		if (m_OnUpdate(this, m_pOnUpdateData, time, elapsedTime) != 0)
		{
			DBG_RS_PRINTF("%s[%d]\"%s\": Update() shutting down: by m_OnUpdate()\n", GetClassName(), GetIndex(), GetUID());
			ShutdownSystem();
		}
	}
	// good place for it, but not all systems call this "Update": m_vecOriginPrev = m_vecOrigin;
	//conprintf(1, "CRenderSystem(%d)::Update(): %d %d %d %g sc %g fm %g\n", index, m_color.r, m_color.g, m_color.b, m_fBrightness, m_fScale, m_fFrame);
	return IsShuttingDown();
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine, allows to add user entities to render list
// For HL-specific studio models only.
//-----------------------------------------------------------------------------
void CRenderSystem::CreateEntities(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: A system is being deleted, we may react to it
// WARNING: DO NOT DELETE SYSTEMS HERE! ONLY FORCE SOFT-SHUTDOWN!
// Input  : *pSystem - 
//-----------------------------------------------------------------------------
void CRenderSystem::OnSystemRemoved(const CRenderSystem *pSystem)
{
}

//-----------------------------------------------------------------------------
// Purpose: Find and follow specified entity
// Output : followed entity or NULL
//-----------------------------------------------------------------------------
cl_entity_t *CRenderSystem::FollowEntity(void)
{
//	DBG_RS_PRINT("FollowEntity()");
	//if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_DONTFOLLOW))// XDM3038b: fix // XDM3038c: we need the entity after all
	//	return NULL;

	if (m_iFollowEntity > 0 && !IsShuttingDown())
	{
		m_pLastFollowedEntity = GetUpdatingEntity(m_iFollowEntity);// XDM3035c: TESTME: gEngfuncs.GetEntityByIndex(m_iFollowEntity);
		if (GetState() != RSSTATE_DISABLED)
		{
			if (m_pLastFollowedEntity != NULL)
			{
				//v m_pLastFollowedEntity = ent;
				if (m_pLastFollowedEntity->model && m_iFollowModelIndex == 0)// XDM3038c
				{
					m_iFollowModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex(m_pLastFollowedEntity->model->name);// HACK
					DBG_RS_PRINTF("FollowEntity(%d): acquiring follow model index from entity: %d\n", m_iFollowEntity, m_iFollowModelIndex);
				}
				//v FollowUpdatePosition();// Update system position now, some code may depend on it. Another call should be made from drawing function in some cases.

				if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_ICNF_NODRAW))
				{
#if defined (_DEBUG_RENDERSYSTEM)
					if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NODRAW))
						DBG_RS_PRINTF("FollowEntity(%d): found follow entity: unhiding\n", m_iFollowEntity);
#endif
					ClearBits(m_iFlags, RENDERSYSTEM_FLAG_NODRAW);// search was successful, unhide
				}
			}
			else// Entity may got out of the PVS, or has been removed and this index will soon be occupied again!! Whad should we do?
			{
				//v m_pLastFollowedEntity = NULL;
				if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET))
				{
					DBG_RS_PRINTF("FollowEntity(%d): lost follow entity: stopping\n", m_iFollowEntity);
					m_iFollowEntity = -1;// stop following, keep current location
				}
				if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_ICNF_NODRAW))
				{
					DBG_RS_PRINTF("FollowEntity(%d): lost follow entity: hiding\n", m_iFollowEntity);
					SetBits(m_iFlags, RENDERSYSTEM_FLAG_NODRAW);// hide
				}
				if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_ICNF_REMOVE))
				{
					DBG_RS_PRINTF("FollowEntity(%d): lost follow entity: shutting down\n", m_iFollowEntity);
					ShutdownSystem();// remove softly
				}
			}
			if (!IsRemoving() && !IsShuttingDown())// XDM3038c: TESTME
			{
				FollowUpdatePosition();// XDM3038c
				// bad: m_vecOriginPrev = m_vecOrigin; because origin may be modified further by Update()
				//DBG_RS_PRINTF("FollowEntity(%d): success\n", m_iFollowEntity);
			}
		}
		return m_pLastFollowedEntity;// can be NULL
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Actualize drawing position.
// Warning: This function must be fast and safe to call often!
// Some systems may require two or even more points, so leave it as virtual.
//-----------------------------------------------------------------------------
void CRenderSystem::FollowUpdatePosition(void)
{
	//DBG_RS_PRINT("FollowUpdatePosition()");
	// XDM3035c: Update() is slow and we need position updated EVERY FRAME.
	if (/*m_iFollowEntity > 0 && */m_pLastFollowedEntity && !FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_DONTFOLLOW))
	{
		// Mod_Extradata() Causes "Cache_UnlinkLRU: NULL link" error eventually
		//studiohdr_t *pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(ent->model);// a little bit slow, but safe and reliable way to validate attachment
		bool bDoOffset = true;
		if (m_iFollowAttachment < MAXSTUDIOATTACHMENTS)// < pStudioHeader->numattachments)// attachment was requested
		{
			bDoOffset = false;
			if (m_pLastFollowedEntity->model && /*(m_iFollowAttachment < MAXSTUDIOATTACHMENTS) &&*/ m_pLastFollowedEntity->model->type == mod_studio)//pStudioHeader->numattachments)
			{
				// done inside if (m_pLastFollowedEntity == gHUD.m_pLocalPlayer && g_ThirdPersonView == 0)
				//	m_vecOrigin = gEngfuncs.GetViewModel()->attachment[m_iFollowAttachment];
				//else
					m_vecOrigin = GetCEntAttachment(m_pLastFollowedEntity, m_iFollowAttachment);//VectorCopy(m_pLastFollowedEntity->attachment[m_iFollowAttachment], m_vecOrigin);
			}
			else// no attachment found
			{
				m_vecOrigin = m_pLastFollowedEntity->origin;
				bDoOffset = true;
			}
		}
		else// no attachment found
			m_vecOrigin = m_pLastFollowedEntity->origin;

		if (bDoOffset && FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_USEOFFSET))// XDM3035b: use user-specified offset in entity local coordinates (rotates according to entity angles)
		{
			Vector a(m_pLastFollowedEntity->angles), efw, ert, eup;// TODO: revisit
#if defined (SV_NO_PITCH_CORRECTION)
			if (m_pLastFollowedEntity != gHUD.m_pLocalPlayer)
				a[PITCH] *= -PITCH_CORRECTION_MULTIPLIER;
#endif
			AngleVectors(a, efw, ert, eup);
			m_vecOrigin += /*m_pLastFollowedEntity->origin +*/ m_vOffset.x*ert + m_vOffset.y*efw + m_vOffset.z*eup;
			//UTIL_DebugAngles(m_pLastFollowedEntity->origin, m_pLastFollowedEntity->angles, 1, 48);
		}

		if (!FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_NOANGLES))// XDM3037a
		{
			if (m_pLastFollowedEntity == gHUD.m_pLocalPlayer)
				gEngfuncs.GetViewAngles(m_vecAngles);// gHUD.m_vecAngles
			else
			{
				m_vecAngles = m_pLastFollowedEntity->angles;// this is interpolated, should we use curstate.angles instead?
#if defined (SV_NO_PITCH_CORRECTION)
				m_vecAngles[PITCH] *= -PITCH_CORRECTION_MULTIPLIER;
#endif
			}
		}
	}
	else
	{
		if (FBitSet(m_iFollowFlags, RENDERSYSTEM_FFLAG_USEOFFSET))// XDM3038c: we have to provide offset functionality even when not following anything
		{
			Vector efw, ert, eup;// TODO: revisit
			AngleVectors(m_vecAngles, efw, ert, eup);
			m_vecOrigin = m_vecOriginStart + m_vOffset.x*ert + m_vOffset.y*efw + m_vOffset.z*eup;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pick next texture frame if needed.
//			Internal frame counter must be a float to accumulate half-frames.
// Input  : &time - 
//			&elapsedTime - 
//-----------------------------------------------------------------------------
void CRenderSystem::UpdateFrame(const float &time, const double &elapsedTime)
{
	if (m_fFrameRate == 0.0f)// XDM3037a: single frame
		return;
	if (m_pTexture == NULL)
	{
		DBG_PRINTF("CRenderSystem[%s]::UpdateFrame(): m_pTexture == NULL!\n", GetUID());
		return;
	}
	if (m_pTexture->numframes <= 1)
		return;
	if (gHUD.m_iPaused > 0)
		return;

	//DBG_RS_PRINT("UpdateFrame() (inside)");
	/*if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)// UNDONE: display ALL frames but in RANDOM ORDER, then destroy if nescessary. Count displayed frames?
	{
		m_fFrame = (float)RANDOM_LONG(0, m_pTexture->numframes - 1);
		return;
	}*/

	if (m_fFrameAccumulator >= 1.0f)// time for the next frame
	{
		//m_iFramePrevious = m_iFrame;
		m_fFrameAccumulator = 0.0f;// reset counter
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
			m_iFrame = RANDOM_LONG(0, m_pTexture->numframes - 1);
		else
		{
			++m_iFrame;
			if (m_iFrame >= m_pTexture->numframes)
			{
				// don't remove after last frame
				if (m_fDieTime == 0.0f)
				{
					if (m_iState == RSSTATE_DISABLED || IsVirtual())// XDM3038c: virtual systems have disabled state
						SetBits(m_iFlags, RENDERSYSTEM_FLAG_NODRAW);
					else if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_LOOPFRAMES))
					{
						if (!IsShuttingDown())
						{
							DBG_RS_PRINTF("%s[%d]\"%s\": UpdateFrame() shutting down: last frame\n", GetClassName(), GetIndex(), GetUID());
							ShutdownSystem();
						}
						return;
					}
				}
				m_iFrame = 0;//m_fFrame -= (int)m_fFrame;// = 0.0f; // leave fractional part
			}
		}
	}
	if (m_fFrameRate < 0)// framerate == fps
		m_fFrameAccumulator = 1.0f;// trigger frame update next screen frame
	else
		m_fFrameAccumulator += m_fFrameRate * elapsedTime;
}

//-----------------------------------------------------------------------------
// Purpose: Set new state externally and let system react immediately
// Input  : iNewState - 
//-----------------------------------------------------------------------------
void CRenderSystem::SetState(const uint32 &iNewState)
{
	if (m_iState != iNewState)
	{
		if (IsVirtual())//m_iState == RSSTATE_VIRTUAL)
		{
			DBG_RS_PRINT("SetState() warning: tried to change RSSTATE_VIRTUAL!");
			return;// false;
		}
		if (iNewState == RSSTATE_ENABLED && m_iState == RSSTATE_DISABLED)
			ClearBits(m_iFlags, RENDERSYSTEM_FLAG_NODRAW);

		m_iState = iNewState;
	}
	//return true;
}

//-----------------------------------------------------------------------------
// Purpose: An external phusical force must be applied. Wind, shockwave, etc.
// Input  : origin - 
//			force - 
//			radius - 
//			point - 
//-----------------------------------------------------------------------------
void CRenderSystem::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
	// TODO: write force interaction code here
}

//-----------------------------------------------------------------------------
// Purpose: Check if user setting allows this system
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::CheckFxLevel(void) const
{
	return ((int)g_pCvarEffects->value >= m_iFxLevel);
}

//-----------------------------------------------------------------------------
// Purpose: Should this system be drawn?
// Note   : We don't check IsVirtual() here because ot is for Manager to decide.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::ShouldDraw(void) const
{
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NODRAW))
		return false;
	if (!CheckFxLevel())// don't draw disallowed systems
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check specified 3D point for visibility. This function decides what is to be drawn.
// Warning: Be EXTREMELY careful with this when porting! This thing may screw up your whole work!
// Input  : &point - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::PointIsVisible(const Vector &point)
{
	// TODO: make render systems render pass-dependant?
	//m_iRenderPass == gHUD.m_iRenderPass

	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_DRAWALWAYS))// may be used by 3D skybox elements
		return true;

	return UTIL_PointIsVisible(point, true);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: additional layer to insert brightness modifiers
// Note   : This code aims to emulate HL-specific renderfx modes
// Output : float calculated brightness (absolute)
//-----------------------------------------------------------------------------
float CRenderSystem::GetRenderBrightness(void)
{
	if (m_iRenderEffects > 0)
	{
		// HL compatibility (probably differs)
		short blend = 255;
		short baseamt = (short)(m_fColorCurrent[3]*255.0f);//(m_fBrightness*255);
		float offset = (m_iFollowEntity > 0)?m_iFollowEntity:0;//(float)index * 128.0f;
		switch (m_iRenderEffects)
		{
		default: break;
		case kRenderFxPulseSlow:
			blend = __min(baseamt, 255-32) + (short)(16.0f * sinf(gHUD.m_flTime * 2.0f + offset));
			break;
		case kRenderFxPulseFast:
			blend = __min(baseamt, 255-32) + (short)(16.0f * sinf(gHUD.m_flTime * 8.0f + offset));
			break;
		case kRenderFxPulseSlowWide:
			blend = __min(baseamt, 255-128) + (short)(64.0f * sinf(gHUD.m_flTime * 2.0f + offset));
			break;
		case kRenderFxPulseFastWide:
			blend = __min(baseamt, 255-128) + (short)(64.0f * sinf(gHUD.m_flTime * 8.0f + offset));
			break;
		case kRenderFxFadeSlow:
			if (baseamt > 0)
				baseamt -= 1;
			else
				baseamt = 0;
			blend = baseamt;
			break;
		case kRenderFxFadeFast:
			if (baseamt > 3)
				baseamt -= 4;
			else
				baseamt = 0;
			blend = baseamt;
			break;
		case kRenderFxSolidSlow:
			if (baseamt < 255)
				baseamt += 1;
			else
				baseamt = 255;
			blend = baseamt;
			break;
		case kRenderFxSolidFast:
			if (baseamt < 252)
				baseamt += 4;
			else
				baseamt = 255;
			blend = baseamt;
			break;
		case kRenderFxStrobeSlow:
			blend = (20.0f * sinf(gHUD.m_flTime * 4.0f + offset));
			if (blend < 0)
				blend = 0;
			else
				blend = baseamt;
			break;
		case kRenderFxStrobeFast:
			blend = (20.0f * sinf(gHUD.m_flTime * 16.0f + offset));
			if (blend < 0)
				blend = 0;
			else
				blend = baseamt;
			break;
		case kRenderFxStrobeFaster:
			blend = (20.0f * sinf(gHUD.m_flTime * 36.0f + offset));
			if (blend < 0)
				blend = 0;
			else
				blend = baseamt;
			break;
		case kRenderFxFlickerSlow:
			blend = (20.0f * (sinf(gHUD.m_flTime * 2.0f) + sinf(gHUD.m_flTime * 17.0f + offset)));
			if (blend < 0)
				blend = 0;
			else
				blend = baseamt;
			break;
		case kRenderFxFlickerFast:
			blend = (20.0f * (sinf(gHUD.m_flTime * 16.0f) + sinf(gHUD.m_flTime * 23.0f + offset)));
			if (blend < 0)
				blend = 0;
			else
				blend = baseamt;
			break;
		case kRenderFxHologram:
		case kRenderFxDistort:
			//baseamt = 180;
			//blend = (int)(baseamt * (1.0f - (dist - 100)/400)) + RANDOM_LONG(-32, 31);
			blend = RANDOM_LONG(143, 223);
			break;
		}
		return (float)clamp(blend,0,255)/255.0f;
	}// (m_iRenderEffects > 0)
	else
		return m_fColorCurrent[3];//m_fBrightness;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*void CRenderSystem::RenderOpaque(void)
{
}*/

//alight_t aLightTest;
//extern CGameStudioModelRenderer g_StudioRenderer;

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CRenderSystem::Render(void)
{
	// handled globally by manager	if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)
	//	return;

	// XDM3037a: OBSOLETE	if (!InitTexture(texindex))
	//	return;

	if (m_iDrawContents != 0 && !ContentsArrayHas(m_iDrawContents, gEngfuncs.PM_PointContents(m_vecOrigin, NULL)))
		return;

/* FAIL
	aLightTest.ambientlight = 128;
	aLightTest.shadelight = 192;
	aLightTest.color[0] = 255;
	aLightTest.color[1] = 0;
	aLightTest.color[2] = 0;
	aLightTest.plightvec = (float *)&g_vecZero;
	IEngineStudio.StudioSetupLighting(&aLightTest);
*/
	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	FollowUpdatePosition();

	Vector vecDelta(m_vecOrigin); vecDelta -= g_vecViewOrigin;
	vec_t fViewDist = vecDelta.Length();// draw when viewer is inside the system
	if (fViewDist > m_fScale && !CL_CheckVisibility(m_vecOrigin))// XDM3035a: TESTME! Should not allow systems to be rendered behind sky
		return;

	/* we can check box too, but it's not needed
	Vector halfsize(m_fScale/2,m_fScale/2,m_fScale/2);
	if (!Mod_CheckBoxInPVS(m_vecOrigin-halfsize, m_vecOrigin+halfsize))// XDM3035c: fast way to check visiblility
		return;synctype_t*/

	Vector right, up;
	AngleVectors(m_vecAngles, NULL, right, up);
	Vector rx(right);// tmp for faster code
	Vector uy(up);
	rx *= m_fSizeX*m_fScale;
	uy *= m_fSizeY*m_fScale;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);// kRenderNormal
	//if (CVAR_GET_FLOAT("test1") > 0)
		gEngfuncs.pTriAPI->Color4f(m_fColorCurrent[0],m_fColorCurrent[1],m_fColorCurrent[2],1.0f);// HL m_fColorCurrent[3]);// XDM3038c
		// old gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, 255);// ? (unsigned char)(m_fBrightness*255.0f));
	//else
	//	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));

	gEngfuncs.pTriAPI->Brightness(GetRenderBrightness());
	gEngfuncs.pTriAPI->CullFace(m_iDoubleSided?TRI_NONE:TRI_FRONT);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin-rx+uy);
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin+rx+uy);
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin+rx-uy);
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin-rx-uy);
	gEngfuncs.pTriAPI->End();

	/* TEST if (g_StudioRenderer.m_pCvarDrawEntities->value == 5)
	gEngfuncs.pTriAPI->Begin(TRI_LINES);
	gEngfuncs.pTriAPI->TexCoord2f(0,0.5);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin-rx);
	gEngfuncs.pTriAPI->TexCoord2f(1,0.5);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin);
	gEngfuncs.pTriAPI->End();*/
	//gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//-----------------------------------------------------------------------------
// Purpose: Gracefully shut down the system
// Warning: Do not delete/destroy/free anything here!
//-----------------------------------------------------------------------------
void CRenderSystem::ShutdownSystem(void)
{
	m_ShuttingDown = true;// XDM3037: tell the system to stop producing effects and shut down ASAP
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_HARDSHUTDOWN))// XDM3038c: system doesn't want to wait
		m_RemoveNow = true;

	DBG_RS_PRINTF("%s[%d]\"%s\": ShutdownSystem() mode: %s\n", GetClassName(), GetIndex(), GetUID(), m_RemoveNow?"hard":"soft");
}

//-----------------------------------------------------------------------------
// Purpose: Safely set UID
// Input  : *uid - "0123456789ABCDEF", max RENDERSYSTEM_UID_LENGTH chars
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::SetUID(const char *uid)
{
	if (g_pRenderManager)
		if (g_pRenderManager->FindSystemByUID(uid))// the main purpose of UID
			return false;

	strncpy(m_UUID, uid, RENDERSYSTEM_UID_LENGTH);//memcpy?
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add contents to the filter (bit array)
// Input  : container - m_iXContents to be modified
//			contents - CONTENTS_*
//-----------------------------------------------------------------------------
void CRenderSystem::ContentsArrayAdd(uint16 &container, signed int contents)
{
	container |= (1 << abs(contents));
}

//-----------------------------------------------------------------------------
// Purpose: Remove contents from the filter (bit array)
// Input  : container - m_iXContents to be modified
//			contents - CONTENTS_*
//-----------------------------------------------------------------------------
void CRenderSystem::ContentsArrayRemove(uint16 &container, signed int contents)
{
	container &= ~(1 << abs(contents));
}

//-----------------------------------------------------------------------------
// Purpose: Check if these world contents are allowed by the filter rules
// Warning: It is your responsibility to check container to be != 0!
// Input  : container - m_iXContents to be modified
//			contents - CONTENTS_*
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::ContentsArrayHas(uint16 &container, signed int contents)
{
	return ((container & (1 << abs(contents))) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Clear contents filter (bit array)
//-----------------------------------------------------------------------------
void CRenderSystem::ContentsArrayClear(uint16 &container)
{
	container = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Check if this point is in allowed world contents
// Input  : origin - point to probe
// Output : Returns true on success (or container == 0), false on failure.
//-----------------------------------------------------------------------------
/*bool CRenderSystem::ContentsCheck(uint16 &container, const Vector &origin)
{
	if (container != 0 && !ContentsArrayHas(container, gEngfuncs.PM_PointContents(origin, NULL)))
		return false;

	return true;// by default, everything is valid
}*/
