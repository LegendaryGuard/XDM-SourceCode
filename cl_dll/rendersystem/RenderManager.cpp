/*----------------------------------------------------------------
*	Render System
*
*	Copyright © 2001-2017, Xawari. All rights reserved.
*	Created for X-Half-Life a Half-Life modification.
*	http://x.netheaven.ru
*
*	This code partially depends on software technology
*	created by Valve LLC, but not required to.
*
*	Licensed under the Mozilla Public License version 2.0.
*
*	If this code to be used along with Valve Gold Source(tm) Engine,
*	the developer must also accept and follow its license agreement.
*
*	Provided terms are aplicable to all RenderSystem-related files
*	which contain code directly or indirectly derived from
*	CRenderSystem, CRenderManager and CParticle classes.
*
* This source code contains no secret or confidential information.
*---------------------------------------------------------------*/
#include "hud.h"
#include "cl_util.h"
#include "shared_resources.h"
#include "triangleapi.h"
//#ifdef USE_EXCEPTIONS
//#include <exception>
//#endif
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RotatingSystem.h"
// Include all systems for dynamic creation
#include "RSBeam.h"
#include "RSBeamStar.h"
#include "RSCylinder.h"
#include "RSDelayed.h"
#include "RSDisk.h"
#include "RSLight.h"
#include "RSModel.h"
#include "RSRadialBeams.h"
#include "RSSphere.h"
#include "RSSprite.h"
#include "RSTeleparts.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "PSBeam.h"
#include "PSBubbles.h"
#include "PSCustom.h"
#include "PSDrips.h"
#include "PSFlameCone.h"
#include "PSFlatTrail.h"
#include "PSSparks.h"
#include "PSSpiralEffect.h"

CRenderManager *g_pRenderManager = NULL;

//-----------------------------------------------------------------------------
// Purpose: Default constructor
//-----------------------------------------------------------------------------
CRenderManager::CRenderManager()
{
	m_pFirstSystem = NULL;
	m_iAllocatedSystems = 0;
	if (gEngfuncs.pTriAPI)
		if (gEngfuncs.pTriAPI->version != TRI_API_VERSION)
			conprintf(0, " WARNING: unexpected triangle API version: %d!\n", gEngfuncs.pTriAPI->version);

	conprintf(1, "CRenderManager created\n");
}

//-----------------------------------------------------------------------------
// Purpose: Destroy all systems now
//-----------------------------------------------------------------------------
CRenderManager::~CRenderManager()
{
	if (g_pRenderManager != this)
	{
		conprintf(0, "CL DLL ERROR! CRenderManager global pointer is lost or invalid! DLL may crash any time now!\n");
		DBG_FORCEBREAK
	}
	DeleteAllSystems();
	//m_pFirstSystem = NULL;
	if (m_iAllocatedSystems != 0)
	{
		conprintf(0, "CL DLL WARNING! CRenderManager has %u allocated systems!\n", m_iAllocatedSystems);
		//DBG_FORCEBREAK
	}
	conprintf(1, "CRenderManager destroyed\n");
}

//-----------------------------------------------------------------------------
// Purpose: Add render system to the BEGINNING of the manager's list (FILO)
// Warning: Will automatically DELETE defective systems from memory, so call it in the last place!
// Input  : *pSystem - newly created RS
//			flags - render system custom flags RENDERSYSTEM_FLAG_RANDOMFRAME
//			followentindex - entity to follow (if any)
//			followflags - follow flags RENDERSYSTEM_FFLAG_ICNF_REMOVE
//			autoinit - InitializeSystem()
// Output : int RS unique index or 0 on error, DO NOT delete system after this! It is already deleted here!
//-----------------------------------------------------------------------------
RS_INDEX CRenderManager::AddSystem(CRenderSystem *pSystem, uint32 flags, int followentindex, int followflags, bool autoinit, bool enable)
{
	if (pSystem == NULL)
		return RS_INDEX_INVALID;

	if (pSystem->IsRemoving())
	{
		conprintf(1, "AddSystem() ERROR: %s \"%s\" is removing itself! Deleting.\n", pSystem->GetClassName(), pSystem->m_pTexture?pSystem->m_pTexture->name:"");
		delete pSystem;
		return RS_INDEX_INVALID;
	}
	if (g_pCvarRenderSystem->value <= 0)
	{
		conprintf(2, "AddSystem(): %s \"%s\": adding not allowed. Deleting\n", pSystem->GetClassName(), pSystem->m_pTexture?pSystem->m_pTexture->name:"");
		pSystem->ShutdownSystem();
		delete pSystem;
		return RS_INDEX_INVALID;
	}
	if (!pSystem->CheckFxLevel() && pSystem->m_fDieTime > 0.0f)// make sure system has limited lifetime
	{
		conprintf(2, "AddSystem(): %s \"%s\": discarded by required fx level. Deleting\n", pSystem->GetClassName(), pSystem->m_pTexture?pSystem->m_pTexture->name:"");
		pSystem->ShutdownSystem();
		delete pSystem;
		return RS_INDEX_INVALID;
	}

	// TODO: if (delay > 0.0f)// add system to a temporary queue and then pick it in Update() and activate
	pSystem->m_iFlags |= flags;// add, do not replace
	pSystem->m_iFollowFlags |= followflags;// add, do not replace
	//pSystem->m_fStartTime = gEngfuncs.GetClientTime() + delay;

	if (followentindex > 0)
		pSystem->m_iFollowEntity = followentindex;

	if (enable && !pSystem->IsVirtual())// check here to produce less warnings
		pSystem->SetState(RSSTATE_ENABLED);// XDM3038a: by default, system is enabled, but user may disable it right away

	pSystem->index = (RS_INDEX)pSystem;//GetFirstFreeRSUID();// HACK: use real ID instead of memory address! At least it guarantees unique number...

	if (enable && autoinit && pSystem->m_fStartTime == 0)// XDM3038c: in case the system was not initialized // TESTME: should we really check "enable" here?
	{
		pSystem->InitializeSystem();
		if (pSystem->IsRemoving())// can happen!
		{
			conprintf(2, "AddSystem() ERROR: %s[%u] \"%s\" failed to initialize! Deleting.\n", pSystem->GetClassName(), pSystem->GetIndex(), pSystem->m_pTexture?pSystem->m_pTexture->name:"");
			delete pSystem;
			return RS_INDEX_INVALID;
		}
	}
	// Assign pointer only after all successful operations, when the system is not deleted
	pSystem->m_pNext = m_pFirstSystem;
	m_pFirstSystem = pSystem;
	DBG_RS_PRINTF("CRenderManager::AddSystem(): added %s[%u]\n", pSystem->GetClassName(), pSystem->index);
	return pSystem->index;
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system, remove it from manager's list
// Warning: Does NOT free the memory!
//-----------------------------------------------------------------------------
bool CRenderManager::RemoveSystem(CRenderSystem *pSystem)
{
	if (pSystem == NULL)
		return false;

	DBG_RS_PRINTF("CRenderManager::RemoveSystem(%u)\n", pSystem->GetIndex());
	CRenderSystem *pSys = m_pFirstSystem;
	CRenderSystem *pPrevSys = NULL;
	while (pSys)
	{
		if (pSys == pSystem)
		{
			pSys->ShutdownSystem();
			pSys->index = RS_INDEX_INVALID;// XDM3038c
			//pSys->m_fDieTime = gEngfuncs.GetClientTime();// ?
			if (pPrevSys)
				pPrevSys->m_pNext = pSys->m_pNext;
			else
				m_pFirstSystem = pSys->m_pNext;

			return true;// or pSys = NULL;
		}
		else
		{
			pPrevSys = pSys;
			pSys = pSys->m_pNext;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Notify all other systems of this event.
// Warning: Must be called by system's delete operator.
// WARNING: DO NOT DELETE SYSTEMS HERE! ONLY FORCE SOFT-SHUTDOWN!
// Input  : *pSystem - 
//-----------------------------------------------------------------------------
void CRenderManager::OnSystemRemoved(const CRenderSystem *pSystem)
{
	DBG_RS_PRINTF("CRenderManager::OnSystemRemoved(%u)\n", pSystem->GetIndex());
	ASSERT(m_iAllocatedSystems > 0);
	uint32 n = 0;
	for (CRenderSystem *pSys = m_pFirstSystem; pSys != NULL; pSys = pSys->m_pNext)
	{
		if (pSys != pSystem)
			pSys->OnSystemRemoved(pSystem);
		++n;
		if (n > 4096)
		{
			conprintf(0, "CRenderManager::OnSystemRemoved(%s[%u]) Warning: infinite loop detected (> %u calls)!\n", pSystem->GetClassName(), pSystem->GetIndex(), n);
			DBG_FORCEBREAK
			if (n > 0)// a chance to edit this variable during debug session to skip listing
				ListSystems();
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system, remove it from manager's list and DELETE from memory
// Warning: Frees memory! (delete[])
//-----------------------------------------------------------------------------
bool CRenderManager::DeleteSystem(CRenderSystem *pSystem)
{
	if (pSystem == NULL)
		return false;

	DBG_RS_PRINTF("CRenderManager::DeleteSystem(%u)\n", pSystem->GetIndex());
	if (RemoveSystem(pSystem))
	{
		RS_INDEX index = pSystem->GetIndex();
		try
		{
			delete pSystem;
		}
		catch (...)
		{
			conprintf(0, "CRenderManager::DeleteSystem() exception during deletion of a system[%u]!\n", index);
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Delete all systems
//-----------------------------------------------------------------------------
void CRenderManager::DeleteAllSystems(void)
{
	DBG_RS_PRINTF("CRenderManager::DeleteAllSystems()\n");
	CRenderSystem *pNext = NULL;
	while (m_pFirstSystem)// operate on this pointer because during deletion we may start reiterating all remaining systems
	{
		pNext = m_pFirstSystem->m_pNext;
		m_pFirstSystem->m_pNext = NULL;// cut it off now
		RS_INDEX index = m_pFirstSystem->GetIndex();
		try
		{
			delete m_pFirstSystem;// while deleting a system, the chain must be valid!
		}
		catch (...)
		{
			conprintf(0, "CRenderManager::DeleteAllSystems() exception during deletion of a system[%d]!\n", index);
			m_pFirstSystem = pNext;
		}
		m_pFirstSystem = pNext;
	}
	m_pFirstSystem = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Delete all systems attached to specified entity
// Input  : entindex - entity index
//			bSoftRemove - just mark for removal, RS should remove itself
//-----------------------------------------------------------------------------
void CRenderManager::DeleteEntitySystems(int entindex, bool bSoftRemove)
{
	DBG_RS_PRINTF("CRenderManager::DeleteEntitySystems(entindex: %d, soft: %d)\n", entindex, bSoftRemove?1:0);
	CRenderSystem *pSystem = NULL;
	CRenderSystem *pNext = NULL;
	while ((pSystem = FindSystemByFollowEntity(entindex, pSystem)) != NULL)
	{
		pNext = pSystem->m_pNext;// the chain will be broken
		if (bSoftRemove)
			pSystem->ShutdownSystem();// mark for removal
		else
			DeleteSystem(pSystem);// immediately

		if (pNext)
			pSystem = pNext;
		else
			break;
	}
	// return iNumDeletedSystems;?
}

//-----------------------------------------------------------------------------
// Purpose: Delete all systems attached to specified entity
// Input  : entindex - entity index (if != -1)
//			classname - if not empty
//			uid - if not empty UNDONE: wildcard/regexp
//			bSoftRemove - just mark for removal, RS should remove itself
//-----------------------------------------------------------------------------
void CRenderManager::DeleteSystemsBy(int entindex, const char *classname, const char *uid, bool bSoftRemove)
{
	DBG_RS_PRINTF("CRenderManager::DeleteEntitySystems(entindex: %d, soft: %d)\n", entindex, bSoftRemove?1:0);

	CRenderSystem *pNext = NULL;
	for (CRenderSystem *pSystem = m_pFirstSystem; pSystem; pSystem = pSystem->m_pNext)
	{
		if ((entindex >= 0 && pSystem->m_iFollowEntity == entindex) ||
			(classname != NULL && classname[0] != 0 && strcmp(pSystem->GetClassName(), classname) == 0) ||
			(uid != NULL && uid[0] != 0 && strcmp(pSystem->GetUID(), uid) == 0))
		{
			pNext = pSystem->m_pNext;// the chain will be broken
			if (bSoftRemove)
				pSystem->ShutdownSystem();// mark for removal
			else
				DeleteSystem(pSystem);// immediately

			if (pNext)
				pSystem = pNext;
			else
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update all systems. Called from HUD_Redraw()
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
//-----------------------------------------------------------------------------
void CRenderManager::Update(const float &time, const double &elapsedTime)
{
	if (gHUD.m_iPaused > 0)// && (gHUD.m_iGameType == GT_SINGLE))// XDM3035: update in multiplayer games
		return;
	if (gHUD.m_iActive == 0)
		return;
	if (time <= 0)
		return;
	if (elapsedTime <= 0)
		return;
	if (gHUD.m_flTimeDelta <= 0.0)
		return;

	//DBG_RS_PRINTF("CRenderManager::Update()\n");
	CRenderSystem *pSys = m_pFirstSystem;
	CRenderSystem *pPrevSys = NULL;
	bool remove = 0;

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
	while (pSys)
	{
		if (pSys->IsVirtual())// || !pSys->CheckFxLevel())// don't update disallowed systems, save CPU. WARNING: this will hold systems in memory!!
		{
			pSys = pSys->m_pNext;
			if (!pSys)
				break;
		}
		remove = pSys->IsRemoving();

		//if (pSys->m_fStartTime <= time)// XDM3035: only update if started
		if (remove == 0)// NO! && pSys->GetState() != RSSTATE_ENABLED)// XDM3038b: must be soft-disable/enable mechanism, update must be called for remaining particles to fly away
			remove |= pSys->Update(time, elapsedTime);// don't use arithmetical addition to prevent overflow/invalid values

		// WARNING! This code DOES NOT handle situations where systems modify our list (m_pFirstSystem) during this loop!
		if (remove)// deletion requested
		{
			RS_INDEX index = pSys->GetIndex();
			try
			{
				if (pPrevSys)
				{
					ASSERT(pPrevSys != pSys);
					pPrevSys->m_pNext = pSys->m_pNext;// exclude pSys from the chain
					delete pSys;// Warning: this calls lots of stuff including another iteration of systems!
					pSys = pPrevSys->m_pNext;
				}
				else// first system
				{
					ASSERT(m_pFirstSystem == pSys);
					m_pFirstSystem = pSys->m_pNext;
					delete pSys;// Warning: this calls lots of stuff including another iteration of systems!
					pSys = m_pFirstSystem;
				}
			}
			catch (...)
			{
				conprintf(0, "CRenderManager::Update() exception during deletion of a system[%u]!\n", index);

				if (pPrevSys)
					pSys = pPrevSys->m_pNext;
				else
					pSys = m_pFirstSystem;
			}
		}
		else
		{
			pSys->m_vecOriginPrev = pSys->m_vecOrigin;// XDM3038c: for interpolation. The only place where we are sure origin is finalized.
#if defined (_DEBUG)
			if (pSys->m_fColorCurrent[3] < 0.0f)
				conprintf(1, "CRenderManager::Update(%s) detected system with brightness < 0!\n", pSys->GetUID());
#endif
			pPrevSys = pSys;
			pSys = pSys->m_pNext;
		}
	}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		conprintf(1, "CRenderManager::Update() exception!\n");
		if (pSys && (g_pCvarDeveloper == NULL || g_pCvarDeveloper->value > 1.0f))
		{
			conprintf(1, " Current system: %s[%d] \"%s\": @(%g %g %g), entity: %d, tex: %s, shutting down: %d. Deleting.\n", pSys->GetClassName(), pSys->GetIndex(), pSys->GetUID(), pSys->m_vecOrigin.x, pSys->m_vecOrigin.y, pSys->m_vecOrigin.z, pSys->m_iFollowEntity, pSys->m_pTexture?pSys->m_pTexture->name:NULL, pSys->IsShuttingDown()?1:0);
			DeleteSystem(pSys);// XDM3037: testme!
		}
		//DBG_FORCEBREAK
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Render all active systems with depth-dependant render modes
//-----------------------------------------------------------------------------
void CRenderManager::RenderOpaque(void)
{
	// UNDONE: this makes smoke appear as one-color surfaces
	/*if (m_pFirstSystem == NULL)
		return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		//if (pSys->m_Enabled == 0)
		//	continue;
		if (pSys->IsVirtual())
			continue;
		if (!pSys->ShouldDraw())
			continue;

		if (pSys->m_iRenderMode == kRenderNormal
			|| pSys->m_iRenderMode == kRenderTransAlpha)
				pSys->Render();
				//pSys->RenderOpaque();
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Render all active systems with transparent render modes
//-----------------------------------------------------------------------------
void CRenderManager::RenderTransparent(void)
{
	if (m_pFirstSystem == NULL)
		return;

	/*cl_entity_t *pClient = gEngfuncs.GetLocalPlayer();
	if (pClient == NULL)
		return;*/
	//if (gHUD.m_Spectator.m_iDrawCycle == ???)
	//	return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		if (pSys->IsRemoving())// XDM3038c: TESTME
			continue;
		//if (pSys->m_Enabled == 0)
		//	continue;
		if (pSys->IsVirtual())
			continue;
		if (!pSys->ShouldDraw())
			continue;
			// UNDONE if (pSys->m_iRenderMode != kRenderNormal
			//		&& pSys->m_iRenderMode != kRenderTransAlpha)
			pSys->Render();
	}

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);//? reset here, after all systems
}

//-----------------------------------------------------------------------------
// Purpose: Render all active systems   TEST
//-----------------------------------------------------------------------------
/*id CRenderManager::Render(void)
{
	if (m_pFirstSystem == NULL)
		return;

	//if (gHUD.m_Spectator.m_iDrawCycle == ???)
	//	return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
		pSys->Render();
}*/

//-----------------------------------------------------------------------------
// Purpose: Called by the engine to add special entities to render
//-----------------------------------------------------------------------------
void CRenderManager::CreateEntities(void)
{
	if (m_pFirstSystem == NULL)
		return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		if (pSys->IsVirtual())
			continue;
		if (!pSys->ShouldDraw())
			continue;

		pSys->CreateEntities();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by its index
// Input  : pStartSystem - continue searching from this system (XDM3038c), NULL == start from the beginning
//			*pfnMatch - pointer to a custom match function
//			*pDataN - optional arguments to be passed to match function
// Output : CRenderSystem pointer or NULL if not found
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystems(CRenderSystem *pStartSystem, bool (*pfnMatch)(CRenderSystem *pSystem, void *pData1, void *pData2, void *pData3, void *pData4), void *pData1, void *pData2, void *pData3, void *pData4)
{
	if (pStartSystem == NULL)
		pStartSystem = m_pFirstSystem;
	else
		pStartSystem = pStartSystem;// XDM3038c: NO! //->m_pNext;// XDM3037: is this a right thing to do?

	//CRenderSystem *pNext = NULL;
	for (CRenderSystem *pSystem = pStartSystem; pSystem; pSystem = pSystem->m_pNext)
	{
		if ((*pfnMatch)(pSystem, pData1, pData2, pData3, pData4))
			return pSystem;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by its index
// Input  : index - RS UID
// Output : CRenderSystem pointer or NULL if not found
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystem(RS_INDEX index)
{
	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		if (pSys->index == index)
			return pSys;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by its unique ID
// Input  : *uid - not really a string, may contain unprintable characters
//			pStartSystem - continue searching from this system (skipped), should be NULL for the first time
// Output : CRenderSystem * - system or NULL when reached end of chain
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystemByUID(const char *uid, CRenderSystem *pStartSystem)
{
	if (pStartSystem == NULL)
		pStartSystem = m_pFirstSystem;
	else
		pStartSystem = pStartSystem->m_pNext;// don't immediately return start system since user already knew about it and probably processed it

	for (CRenderSystem *pSys = pStartSystem; pSys; pSys = pSys->m_pNext)
	{
		if (strncmp(pSys->GetUID(), uid, RENDERSYSTEM_UID_LENGTH) == 0)//if (memcmp(pSys->m_UUID, uid, RENDERSYSTEM_UID_LENGTH) == 0)
			return pSys;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by its custom name
// WARNING! There may be more than one RS!
// Input  : name - 
//			pStartSystem - continue searching from this system (skipped), should be NULL for the first time
// Output : CRenderSystem * - system or NULL when reached end of chain
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystemByName(const char *name, CRenderSystem *pStartSystem)
{
	if (pStartSystem == NULL)
		pStartSystem = m_pFirstSystem;
	else
		pStartSystem = pStartSystem->m_pNext;// don't immediately return start system since user already knew about it and probably processed it

	for (CRenderSystem *pSys = pStartSystem; pSys; pSys = pSys->m_pNext)
	{
		if (strcmp(pSys->GetName(), name) == 0)
			return pSys;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by follow entity index
// WARNING! There may be more than one RS following this entity!
// Input  : entindex - entity index
//			pStartSystem - continue searching from this system (skipped), should be NULL for the first time
// Output : CRenderSystem pointer or NULL if not found
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystemByFollowEntity(int entindex, CRenderSystem *pStartSystem)
{
	if (pStartSystem == NULL)
		pStartSystem = m_pFirstSystem;
	else
		pStartSystem = pStartSystem->m_pNext;// XDM3037: is this a right thing to do?

	for (CRenderSystem *pSys = pStartSystem; pSys; pSys = pSys->m_pNext)
	{
		if (pSys->m_iFollowEntity == entindex)
			return pSys;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Apply physical force to render systems (for wind+rain effects)
// Input  : origin - origin from which the force will be applied
//			force - force vector
//			radius - affecto only systems in this radius
//			point - if true, force goes from origin, value = Length(force)
//-----------------------------------------------------------------------------
void CRenderManager::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
//	DBG_RS_PRINTF("CRenderManager::ApplyForce(%g %g %g, %g, %d)\n", force[0], force[1], force[2], radius, point);
	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		// DO NOT check radius HERE! Some systems like rain or snow MUST be affected regardless to its 'origin'
		pSys->ApplyForce(origin, force, radius, point);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get first available unique index for render system
// Output : int UID
//-----------------------------------------------------------------------------
//
// test pattern:
// 1 3 7 6 4 9 12 2
//
unsigned int CRenderManager::GetFirstFreeRSUID(void)
{
	// TODO: generate sequental RS indexes
	/*unsigned int previous_lesser_index = 0xFFFFFFFF;
	unsigned int system_index = 0xFFFFFFFF;
	unsigned int parsed = 0;

	unsigned int first_less_index = 0xFFFFFFFF;
	unsigned int second_less_index = 0xFFFFFFFF;


	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		system_index = pSys->GetIndex();
		++parsed;

		if (system_index < first_less_index)
		{

			if (system_index - second_less_index > 1)
		}
		wtf
		
		if (system_index < previous_lesser_index)
		{
			//if (system_index <= 1)// there is no index lesser than 1
			//	return previous_lesser_index;

			if (previous_lesser_index - system_index > 1)// we skipped at least one index which is probably free
				previous_lesser_index = system_index + 1;
		}
	}
	if (parsed < 1)
		previous_lesser_index = 1;

	return previous_lesser_index;*/
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Print all systems information to console
//-----------------------------------------------------------------------------
void CRenderManager::ListSystems(void)
{
	size_t n = 0;
	float fTime = gEngfuncs.GetClientTime();
	for (CRenderSystem *pSystem = m_pFirstSystem; pSystem; pSystem = pSystem->m_pNext, ++n)
	{
		conprintf(0, "%s[%u] \"%s\" \"%s\", ent: %d, tex: %s, %s, state: %u, uptime: %gs, @(%g %g %g)\n", pSystem->GetClassName(), pSystem->GetIndex(), pSystem->GetUID(), pSystem->GetName(), pSystem->m_iFollowEntity, pSystem->m_pTexture?pSystem->m_pTexture->name:NULL, pSystem->IsShuttingDown()?"shutting down":"", pSystem->GetState(), fTime-pSystem->m_fStartTime, pSystem->m_vecOrigin.x,pSystem->m_vecOrigin.y,pSystem->m_vecOrigin.z);
		if (pSystem->m_fDieTime > 0 && pSystem->m_fDieTime < fTime)
			conprintf(0, "^ warning: system lifetime ended %gs ago!\n", fTime-pSystem->m_fDieTime);
	}
	conprintf(0, " %u total, %u systems allocated memory\n", n, m_iAllocatedSystems);
}

// For fast access, because comparing strings on every system is bad
enum search_rs_types_e
{
	RSS_ST_INDEX = 0,
	RSS_ST_UID,
	RSS_ST_CLASS,
	RSS_ST_NAME,
	RSS_ST_TEXTURE,
	RSS_ST_STATE,
	RSS_ST_ALL,
};

//-----------------------------------------------------------------------------
// Purpose: Perform some actions on a RS (CALL FROM CLIENT_COMMAND ONLY!)
// Input  : start_argument - start reading commands from this one
//			*pSystem - target
//			*pPlayer - user
//-----------------------------------------------------------------------------
void Cmd_RSAction(int start_argument, CRenderSystem *pSystem, cl_entity_t *pPlayer)
{
	if (pSystem == NULL || pPlayer == NULL)
		return;

	int option = start_argument;
	int argc = CMD_ARGC();// includes the command itself
	conprintf(0, "Got: %s[%u] \"%s\" @(%g %g %g), entity: %d, tex: %s, %s, state: %u\n", pSystem->GetClassName(), pSystem->GetIndex(), pSystem->GetUID(), pSystem->m_vecOrigin.x,pSystem->m_vecOrigin.y,pSystem->m_vecOrigin.z, pSystem->m_iFollowEntity, pSystem->m_pTexture?pSystem->m_pTexture->name:NULL, pSystem->IsShuttingDown()?"shutting down":"running", pSystem->GetState());

	while (option < argc)
	{
		const char *act = CMD_ARGV(option);//CMD_ARGV(4);
		if (act == NULL || *act == 0)
		{
			option = argc;
			//break;
		}
		else if (strcmp(act, "del") == 0)
		{
			g_pRenderManager->DeleteSystem(pSystem);
			conprintf(0, " - deleted;\n");
			return;// SKIP ALL SUBSEQUENT ACTIONS!
		}
		else if (strcmp(act, "shutdown") == 0)
		{
			if (!pSystem->IsShuttingDown())
			{
				pSystem->ShutdownSystem();
				conprintf(0, " - shut down;\n");
			}
			else
				conprintf(0, " - skipped (already shutting down);\n");
		}
		else if (strcmp(act, "show") == 0)
		{
			//if (!pSystem->IsShuttingDown())
			gEngfuncs.pEfxAPI->R_BeamEntPoint(pPlayer->index, pSystem->m_vecOrigin, g_iModelIndexLaser, 10.0f, 8,0, 1.0f, 1.0, 0,16, 1.0f, pSystem->IsShuttingDown()?0.0f:1.0f, pSystem->IsShuttingDown()?0.0f:1.0f);
		}
		/*else if (strcmp(act, "hl") == 0)
		{
			if (argc > option + 1)
			{
				if (atoi(CMD_ARGV(option+1)) > 0)
				{
					SetBits(pEntity->pev->effects, EF_BRIGHTFIELD);
					conprintf(0, " - highlighted;\n");
				}
				else
				{
					ClearBits(pEntity->pev->effects, EF_BRIGHTFIELD);
					conprintf(0, " - unhighlighted;\n");
				}
				option++;// skip argument
			}
			else
				conprintf(0, " - %s: bad argument\n", act);
		}*/
		else if (strcmp(act, "info") == 0)
		{
			conprintf(0, " flags: %u, followflags: %u, attachment: %d, rendermode: %hd;\n", pSystem->m_iFlags, pSystem->m_iFollowFlags, pSystem->m_iFollowAttachment, pSystem->m_iRenderMode);
		}
		else if (strcmp(act, "use") == 0)
		{
			if (argc > option + 1)
			{
				//int val;
				int utype = atoi(CMD_ARGV(option+1));
				/*if (argc > option + 2)
				{
					val = atoi(CMD_ARGV(option+2));
					option += 2;// skip two arguments
				}
				else*/
				{
					//val = 0;
					option++;// skip single argument
				}
				if (utype == 0)
					pSystem->SetState(RSSTATE_DISABLED);
				else if (utype == 1)
					pSystem->SetState(RSSTATE_ENABLED);
				else// toggle
				{
					if (pSystem->GetState() == RSSTATE_DISABLED)
						pSystem->SetState(RSSTATE_ENABLED);
					else
						pSystem->SetState(RSSTATE_DISABLED);
				}
				conprintf(0, " - used (state set to %u);\n", pSystem->GetState());
			}
			else
				conprintf(0, " - %s: bad arguments! <0=off,1=on,2=toggle>\n", act);
		}
		else if (strcmp(act, "set") == 0)
		{
			if (argc > option + 2)
			{
				if (pSystem->ParseKeyValue(CMD_ARGV(option+1), CMD_ARGV(option+2)))
					conprintf(0, " - set value;\n");
				else
					conprintf(1, " - failed to parse key \"%s\" \"%s\"!\n", CMD_ARGV(option+1), CMD_ARGV(option+2));

				option += 2;
			}
			else
				conprintf(0, " - %s: bad arguments! <key> <value>\n", act);
		}
		else if (strcmp(act, "debug") == 0)
		{
			conprintf(0, " - breaking execution for system %u;\n", pSystem->GetIndex());
			DBG_FORCEBREAK
		}
		else
			conprintf(0, " - unknown option: %s\n Possible commands: del,shutdown,show,info,use,set,debug\n", act);

		++option;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Console front-end
//-----------------------------------------------------------------------------
void CRenderManager::Cmd_SearchRS(void)
{
	if (CMD_ARGC() >= 2)// cmd+type
	{
		int st = 0;
		int start_argument = 3;
		const char *searchtype = CMD_ARGV(1);
		if (strcmp(searchtype, "index") == 0)
			st = RSS_ST_INDEX;
		else if (strcmp(searchtype, "uid") == 0)
			st = RSS_ST_UID;
		else if (strcmp(searchtype, "class") == 0)
			st = RSS_ST_CLASS;
		else if (strcmp(searchtype, "name") == 0)
			st = RSS_ST_NAME;
		else if (strcmp(searchtype, "texture") == 0)
			st = RSS_ST_TEXTURE;
		else if (strcmp(searchtype, "state") == 0)
			st = RSS_ST_STATE;
		else if (strcmp(searchtype, "*") == 0)
		{
			st = RSS_ST_ALL;
			start_argument = 2;// no search string required
		}
		else
		{
			conprintf(0, "%s error: unknown search type: \"%s\".\n", CMD_ARGV(0), searchtype);
			return;
		}
		RS_INDEX searchindex = 0;
		const char *searchstring = NULL;
		if (start_argument >= 3)// only if argument required
		{
			searchstring = CMD_ARGV(2);
			if (searchstring == NULL || *searchstring == 0)
			{
				conprintf(0, "%s error: bad search string.\n", CMD_ARGV(0));
				return;
			}
			else
				searchindex = strtoul(searchstring, NULL, 10);
		}
		size_t n = 0;
		// WARNING! Simple "for" iteration won't work (will crash) on deletion!
		CRenderSystem *pSystem = m_pFirstSystem;
		CRenderSystem *pNext = NULL;
		while (pSystem)
		{
			pNext = pSystem->m_pNext;// the chain may become broken
			if ((st == RSS_ST_ALL) ||
				(st == RSS_ST_INDEX && pSystem->GetIndex() == searchindex) ||
				(st == RSS_ST_UID && strncmp(pSystem->GetUID(), searchstring, RENDERSYSTEM_UID_LENGTH) == 0) ||
				(st == RSS_ST_CLASS && strcmp(pSystem->GetClassName(), searchstring) == 0) ||
				(st == RSS_ST_NAME && strcmp(pSystem->GetName(), searchstring) == 0) ||
				(st == RSS_ST_STATE && pSystem->GetState() == searchindex) ||
				(st == RSS_ST_TEXTURE && pSystem->m_pTexture != NULL && strncmp(pSystem->m_pTexture->name, searchstring, MAX_MODEL_NAME) == 0))
			{
				Cmd_RSAction(start_argument, pSystem, gHUD.m_pLocalPlayer);
				++n;
			}

			if (pNext)
				pSystem = pNext;
			else
				break;
		}
		conprintf(0, " %u systems total\n", n);
	}
	else
		conprintf(0, "Usage: %s <index|uid|class|name|texture|state|*> <string> [action [arguments]] [...]\n", CMD_ARGV(0));
}

//-----------------------------------------------------------------------------
// Purpose: Really simple fabric
// WARNING: CONSTRUCTORS USED HERE MUST _NOT_ CALL InitializeSystem()!!!
// Warning: UNDONE: arguments are mostly used by CPSCustom!!! Otherwise empty default constructor is used!
// Note   : Better sort by decreasing probability of use (most common systems at the top)
//-----------------------------------------------------------------------------
CRenderSystem *CreateSystemByClassName(const char *szClassName, const Vector &origin, int emitterindex, int attachment, float timetolive)
{
//	DBG_RS_PRINTF("CreateSystemByClassName(%s, %d, %d, %g)\n", szClassName, emitterindex, attachment, timetolive);
	CRenderSystem *pSystem = NULL;
//#if defined (USE_EXCEPTIONS)
//	try
//	{
//#endif
	if (strcmp(szClassName, "CRSBeam") == 0)
		pSystem = new CRSBeam();
	else if (strcmp(szClassName, "CRSBeamStar") == 0)
		pSystem = new CRSBeamStar();
	else if (strcmp(szClassName, "CRSCylinder") == 0)
		pSystem = new CRSCylinder();
	// no, don't else if (strcmp(szClassName, "CRSDelayed") == 0)
	//	pSystem = new CRSDelayed();
	else if (strcmp(szClassName, "CRSDisk") == 0)
		pSystem = new CRSDisk();
	else if (strcmp(szClassName, "CRSLight") == 0)
		pSystem = new CRSLight();
	else if (strcmp(szClassName, "CRSModel") == 0)
		pSystem = new CRSModel();
	else if (strcmp(szClassName, "CRSRadialBeams") == 0)
		pSystem = new CRSRadialBeams();
	else if (strcmp(szClassName, "CRSSphere") == 0)
		pSystem = new CRSSphere();
	else if (strcmp(szClassName, "CRSSprite") == 0)
		pSystem = new CRSSprite();
	else if (strcmp(szClassName, "CRSTeleparts") == 0)
		pSystem = new CRSTeleparts();
	else if (strcmp(szClassName, "CRenderSystem") == 0)
		pSystem = new CRenderSystem();
	else if (strcmp(szClassName, "CParticleSystem") == 0)
		pSystem = new CParticleSystem();
	else if (strcmp(szClassName, "CPSBeam") == 0)
		pSystem = new CPSBeam();
	else if (strcmp(szClassName, "CPSBubbles") == 0)
		pSystem = new CPSBubbles();
	else if (strcmp(szClassName, "CPSCustom") == 0)
	{
		CPSCustom *pPSCustom = new CPSCustom();//origin, emitterindex, attachment, timetolive);
		if (pPSCustom)
		{
			pPSCustom->m_iEmitterIndex = emitterindex;
			pSystem = pPSCustom;
		}
	}
	else if (strcmp(szClassName, "CPSDrips") == 0)
		pSystem = new CPSDrips();
	else if (strcmp(szClassName, "CPSFlameCone") == 0)
		pSystem = new CPSFlameCone();
	else if (strcmp(szClassName, "CPSFlatTrail") == 0)
		pSystem = new CPSFlatTrail();
	else if (strcmp(szClassName, "CPSSparks") == 0)
		pSystem = new CPSSparks();
	else if (strcmp(szClassName, "CPSSpiralEffect") == 0)
		pSystem = new CPSSpiralEffect();
//#if defined (USE_EXCEPTIONS)
//	}
//	catch (std::bad_alloc &ba)// XDM3038c: !!!
//	{
//		conprintf(0, "CreateSystemByClassName(%s, %d, %d, %g) ERROR: unable to allocate memory! %s\n", szClassName, emitterindex, attachment, timetolive, ba.what());
//		DBG_FORCEBREAK
//		pSystem = NULL;
//	}
//#endif

	if (pSystem)
	{
		pSystem->m_vecOrigin = origin;
		pSystem->m_iFollowAttachment = attachment;
		if (timetolive <= 0.0f)
			pSystem->m_fDieTime = 0.0f;
		else
			pSystem->m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

		ASSERT(pSystem->m_fStartTime == 0.0f);// check that InitializeSystem() was not called
		conprintf(2, "CreateSystemByClassName(%s, %d, %d, %g): created: %s[%u] \"%s\" @(%g %g %g), entity: %d, tex: %s, %s, state: %u\n", szClassName, emitterindex, attachment, timetolive, pSystem->GetClassName(), pSystem->GetIndex(), pSystem->GetUID(), pSystem->m_vecOrigin.x,pSystem->m_vecOrigin.y,pSystem->m_vecOrigin.z, pSystem->m_iFollowEntity, pSystem->m_pTexture?pSystem->m_pTexture->name:NULL, pSystem->IsShuttingDown()?"shutting down":"running", pSystem->GetState());
	}
	return pSystem;
}

//-----------------------------------------------------------------------------
// Purpose: Comparison callback function
// Warning: compares the beginning of the string (because actual UID has index at the end)
//-----------------------------------------------------------------------------
bool MatchRSUID(CRenderSystem *pSystem, void *pData1, void *pData2, void *pData3, void *pData4)
{
	return (strncmp(pSystem->GetUID(), (char *)pData1, *(size_t *)pData2) == 0);
}

//-----------------------------------------------------------------------------
// Purpose: Find previously loaded custom systems
// WARNING: Many systems may be attached to one target entity!!!
// Input  : *filename - "name.ext"
//			emitterindex - my emitter (env_ entity)
//			followentity - entity to follow
//			actionflags - FINDRS_UPDATE_STATE
// Output : Returns number of systems found
//-----------------------------------------------------------------------------
uint32 FindLoadedSystems(const char *filename, int emitterindex, int followentity, int attachment, int state, uint32 actionflags, void (*pfnOnFoundSystem)(CRenderSystem *pSystem, void *pData1, void *pData2), void *pOnFoundSystemData1, void *pOnFoundSystemData2)
{
	DBG_RS_PRINTF("FindLoadedSystems(\"%s\", %d, %d, %d, %u)\n", filename, emitterindex, attachment, state, actionflags);
	char szUID[RENDERSYSTEM_UID_LENGTH];
	// Actual system name has its index at the end, so compare only the beginning of the sting
	// BAD! Someone may change the follow entity! int result = _snprintf(szUID, RENDERSYSTEM_UID_LENGTH, "%s|%d|%d|", filename, emitterindex, followentity);
	int result = _snprintf(szUID, RENDERSYSTEM_UID_LENGTH, "%s|%d|", filename, emitterindex);
	size_t compare_len;
	if (result > 0 && result < RENDERSYSTEM_UID_LENGTH)
	{
		compare_len = result;
	}
	else// not only if (result <= 0)
	{
		conprintf(1, "FindLoadedSystems(%s): error creating system UID!\n", filename);
		return 0;
	}
	/* FAIL! else if (result >= RENDERSYSTEM_UID_LENGTH)
	{
		szUID[RENDERSYSTEM_UID_LENGTH-1] = 0;
		compare_len = result-1;
	}*/
	uint32 numfound = 0;
	CRenderSystem *pCurrentSystem = NULL;
	CRenderSystem *pSystem = NULL;
	while ((pSystem = g_pRenderManager->FindSystems(pSystem, MatchRSUID, (void *)szUID, (void *)&compare_len, NULL, NULL)) != NULL)
	{
		pCurrentSystem = pSystem;
		pSystem = pSystem->GetNext();// system may be destroyed in the process, so remember next system now
		++numfound;
		if (actionflags & FINDRS_UPDATE_STATE)
			pCurrentSystem->SetState(state);
		if (actionflags & FINDRS_UPDATE_FOLLOWENT)
			pCurrentSystem->m_iFollowEntity = followentity;
		if (actionflags & FINDRS_UPDATE_ATTACHMENT)
			pCurrentSystem->m_iFollowAttachment = attachment;
		//if (actionflags & FINDRS_UPDATE_EVERYTHING)
		if (pfnOnFoundSystem)
			pfnOnFoundSystem(pCurrentSystem, pOnFoundSystemData1, pOnFoundSystemData2);
	}
	return numfound;
}

const char g_ParseRSSCT[] = "{}',";// exclude ()

// A little callback for the script
bool ParseSystemKeyword(const char *szToken, float &fDelayed, bool &bSystemIsVirtual, bool &bSystemIsActive)
{
	if (strcmp(szToken, "virtual") == 0)
	{
		bSystemIsVirtual = true;
	}
	else if (strcmp(szToken, "inactive") == 0)
	{
		bSystemIsActive = false;
	}
	else if (strbegin(szToken, "delayed") > 0)
	{
		if (sscanf(szToken, "delayed(%f)", &fDelayed) != 1)
		{
			fDelayed = 0.0f;
			conprintf(1, "ParseSystemKeyword(): Error parsing keyword \"%s\"!\n", szToken);
		}
	}
	else
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Load render systems from a file
// Note   : One file may contain many systems. Format: "Class\n{\nkey value\n}\n"
// WARNING: Many systems may be attached to one target entity!!!
// Input  : *filename - "name.ext"
//			&origin - 
//			emitterindex - my emitter (env_ entity)
//			followentity - entity to follow
//			attachment - for studio models
//			fxlevel - required effects level setting
//			timetolive - 0 means forever
//			uid_postfix - used to generate unique UIDs across multiple calls to LoadRenderSystems()
//			pfnOnAddSystem - callback, called before InitializeSystem()
//			*pOnAddSystemData1, *pOnAddSystemData2 - custom parameters for that callback
// Output : Returns number of systems created
//-----------------------------------------------------------------------------
uint32 LoadRenderSystems(const char *filename, const Vector &origin, int emitterindex, int followentity, int followmodelindex, int attachment, int fxlevel, uint32 systemflags, float timetolive, uint32 uid_postfix, void (*pfnOnAddSystem)(CRenderSystem *pSystem, void *pData1, void *pData2), void *pOnAddSystemData1, void *pOnAddSystemData2)
{
	DBG_RS_PRINTF("LoadRenderSystems(\"%s\", %g %g %g, %d, %d, %d, %g, %u)\n", filename, origin.x, origin.y, origin.z, emitterindex, followentity, attachment, timetolive, uid_postfix);
	uint32 numsystems = 0;
	char *pFileStart;
	char *pFile;
	char szToken[1024];// is this a standard HL engine token size???
	char szTokenValue[1024];
	char relpath[MAX_PATH];// = "scripts/";// XDM: keep root directory clean! Put your stuff into /scripts/
	char szUID[RENDERSYSTEM_UID_LENGTH];// XDM3038b: now we find system reliably by UID
	char szClassName[RENDERSYSTEM_CLASSNAME_LENGTH];// for the fabric
	CRenderSystem *pSystem = NULL;
	float fDelayed = 0.0f;
	bool bParsingClassName = true;// we consider first token a class name
	bool bParsingSystem = false;
	bool bParsingComment = false;
	bool bSystemIsActive = true;
	bool bSystemIsVirtual = false;
	bool error = false;

	//if (_snprintf(szUID, RENDERSYSTEM_UID_LENGTH, "%s|%d|%d|%d", filename, emitterindex, followentity, 100) < 0)// failed to generate UID!
	if (_snprintf(szUID, RENDERSYSTEM_UID_LENGTH, "%s|%d|%d", filename, emitterindex, 100) < 0)// failed to generate UID!
	{
		szUID[RENDERSYSTEM_UID_LENGTH-1] = '\0';
		conprintf(0, "LoadRenderSystems(): error loading \"%s\": name is too long! (max UID length: %d)\n", relpath, RENDERSYSTEM_UID_LENGTH);
		error = true;
		return 0;
	}

	_snprintf(relpath, MAX_PATH, "scripts/rendersystem/%s", filename);// scripts/rendersystem/test.res
	pFileStart = (char *)gEngfuncs.COM_LoadFile((char *)relpath, 5, NULL);
	pFile = pFileStart;
	if (pFile == NULL)
	{
		conprintf(0, "LoadRenderSystems(): ERROR: couldn't load %s!\n", relpath);
		return numsystems;
	}

	conprintf(0, "LoadRenderSystems(): loading %s\n", relpath);
	//strncpy(m_szName, filename, PARTSYSTEM_NAME_MAX);// some functions use it during initialization
	pFile = COM_Parse(pFile, szToken, false, g_ParseRSSCT);
	if (pFile == NULL)
	{
		conprintf(1, "LoadRenderSystems(\"%s\"): unable to parse file.\n", filename);
		return numsystems;
	}

	while (pFile)
	{
		if (*szToken == '/' && szToken[1] == '*')
		{
			bParsingComment = true;
		}
		else if (*szToken == '*' && szToken[1] == '/')
		{
			bParsingComment = false;
		}
		else if (!bParsingComment)
		{
			if (*szToken == '{')//(strcmp(szToken, "{") == 0)
			{
				if (bParsingSystem)
				{
					conprintf(1, "LoadRenderSystems(\"%s\"): error parsing structure \"%s\": subsections are not allowed!\n", filename, szToken);
					error = true;
				}
				else
				{
					ASSERT(pSystem != NULL);//pSystem = new CPSCustom(origin, emitterindex, attachment, timetolive);
					bParsingSystem = true;
				}
			}
			else if (*szToken == '}')//(strcmp(szToken, "}") == 0)
			{
				if (bParsingSystem)
				{
					bParsingSystem = false;
					DBG_RS_PRINTF("LoadRenderSystems(): finished reading system number %u\n", numsystems);
					if (pSystem != NULL)
					{
						//snprintf(pSystem->m_szName, PARTSYSTEM_NAME_MAX, "%s#%d\0", filename, numsystems);// filename.ext#0, filename.ext#1...
						//if (_snprintf(szUID, RENDERSYSTEM_UID_LENGTH, "%s|%d|%d|%d", filename, emitterindex, followentity, numsystems) < 0)// delimiter MUST be forbidden to use in path names!
						if (_snprintf(szUID, RENDERSYSTEM_UID_LENGTH, "%s|%d|%u|%u", filename, emitterindex, uid_postfix, numsystems) >= 0)// uid_postfix is used to get unique UIDs, but multiple calls to LoadRenderSystems() may generate overlapping UIDs, it's up to user.
						{
							if (pSystem->SetUID(szUID))
							{
								if (bSystemIsVirtual)
									pSystem->SetState(RSSTATE_VIRTUAL);

								pSystem->m_iFxLevel = fxlevel;// will be firewalled by manager
								pSystem->m_iFollowModelIndex = followmodelindex;
								DBG_RS_PRINTF("LoadRenderSystems(): adding system: %s (delayed: %g)\n", pSystem->GetClassName(), fDelayed);
								if (fDelayed > 0.0f)
								{
									if (bSystemIsVirtual)
										conprintf(1, "LoadRenderSystems(\"%s\"): warning: virtual system #%u (%s) cannot be delayed!\n", filename, numsystems, pSystem->GetClassName());
									else
									{
										CRenderSystem *pSystemFromScript = pSystem;
										pSystem = new CRSDelayed(pSystemFromScript, fDelayed, systemflags, followentity, pSystem->m_iFollowFlags);// wrap into container
										if (pSystem)
										{
											pSystem->SetUID(szUID);// so this delay system can be found by the UID of its contained system
											if (pfnOnAddSystem)// call custom function
												pfnOnAddSystem(pSystemFromScript, pOnAddSystemData1, pOnAddSystemData2);
										}
									}
								}
								if (g_pRenderManager->AddSystem(pSystem, systemflags, followentity, pSystem->m_iFollowFlags, false, (bSystemIsActive && !bSystemIsVirtual)) != RS_INDEX_INVALID)
								{
									if (pfnOnAddSystem)// call custom function
										pfnOnAddSystem(pSystem, pOnAddSystemData1, pOnAddSystemData2);

									pSystem->InitializeSystem();// XDM3038c: only after all callbacks are finished! // NOW, when it knows how much resources to allocate!
									pSystem = NULL;
									++numsystems;
								}
								else// WARNING: pSystem is deleted now!!
								{
									pSystem = NULL;
									conprintf(1, "LoadRenderSystems(\"%s\"): error adding system %u: \"%s\" UID \"%s\"!\n", filename, numsystems, szClassName, szUID);
									// done inside AddSystem() delete pSystem;
									error = true;
								}
							}
							else
							{
								conprintf(1, "LoadRenderSystems(\"%s\"): fatal error adding system %u: \"%s\" UID \"%s\": duplicate UID!!\n", filename, numsystems, szClassName, szUID);
								delete pSystem;
								pSystem = NULL;
								error = true;
							}
						}
						else
						{
							conprintf(1, "ERROR ADDING %s FROM \"%s\": UID is too long!\n", szClassName, filename);
							delete pSystem;
							pSystem = NULL;
							error = true;
						}
					}
					else
					{
						conprintf(1, "LoadRenderSystems(\"%s\"): error closing section: system is NULL!\n", filename);
						error = true;
					}
					// reset system properties
					fDelayed = 0.0f;
					bSystemIsActive = true;
					bSystemIsVirtual = false;
				}
				else
				{
					conprintf(1, "LoadRenderSystems(\"%s\"): error parsing structure \"%s\": section was not started!\n", filename, szToken);
					error = true;
				}
				if (error)
				{
					conprintf(1, "LoadRenderSystems(\"%s\"): error encountered while adding a system, deleting.\n", filename);
					if (pSystem)
					{
						g_pRenderManager->DeleteSystem(pSystem);//pSystem->ShutdownSystem();
						pSystem = NULL;
					}
					//error = false;
				}
				bParsingClassName = true;// start looking for another class name
			}
			else if (bParsingClassName)
			{
				ASSERT(pSystem == NULL);
				while (ParseSystemKeyword(szToken, fDelayed, bSystemIsVirtual, bSystemIsActive))
					pFile = COM_Parse(pFile, szToken, false, g_ParseRSSCT);

				pSystem = CreateSystemByClassName(szToken, origin, emitterindex, attachment, timetolive);
				strncpy(szClassName, szToken, RENDERSYSTEM_CLASSNAME_LENGTH-1);
				szClassName[RENDERSYSTEM_CLASSNAME_LENGTH-1] = '\0';
				if (pSystem == NULL)
				{
					conprintf(1, "LoadRenderSystems(\"%s\"): error parsing structure \"%s\": system cannot be created!\n", filename, szToken);
					//error = true;
				}
				bParsingClassName = false;// we finished reading class name, now we expect '{' or we fail
			}
			else if (bParsingSystem)// we're inside system body {}
			{
				if (pSystem == NULL)
				{
					conprintf(1, "LoadRenderSystems(\"%s\"): error parsing \"%s\": NULL system!\n", filename, szToken);
					error = true;
				}
				else
				{
					pFile = COM_Parse(pFile, szTokenValue, false, g_ParseRSSCT);//pFile = gEngfuncs.COM_ParseFile(pFile, szTokenValue);
					if (pFile)
					{
						if (!pSystem->ParseKeyValue(szToken, szTokenValue))
							conprintf(1, "LoadRenderSystems(\"%s\"): failed to parse key \"%s\" \"%s\"!\n", filename, szToken, szTokenValue);
					}
					else
					{
						conprintf(1, "LoadRenderSystems(\"%s\"): failed to read value for key \"%s\"!\n", filename, szToken);
						break;
					}
				}
			}
			else// not a class, not inside - probably a name
			{
				if (pSystem)
				{
					const char *pOldName = pSystem->GetName();
					if (pOldName && pOldName[0])
					{
						conprintf(1, "LoadRenderSystems(\"%s\"): error parsing \"%s\": system already has a name: \"%s\"!\n", filename, szToken, pOldName);
						//error = true;
					}
					else
					{
						strncpy(pSystem->m_szName, szToken, RENDERSYSTEM_USERNAME_LENGTH-1);
						pSystem->m_szName[RENDERSYSTEM_USERNAME_LENGTH-1] = '\0';
					}
				}
				else// ???
				{
					conprintf(1, "LoadRenderSystems(\"%s\"): error parsing \"%s\" outside section!\n", filename, szToken);
					error = true;
				}
			}
		}// !bParsingComment
		if (error) break;
		pFile = COM_Parse(pFile, szToken, false, g_ParseRSSCT);
	}
	gEngfuncs.COM_FreeFile(pFileStart);
	//pFileStart = NULL;

	//pSystem->m_szName[0] = 0;// TODO: Clear names of all freshly created systems here. Make sure names are only valid across one script!

	if (error)
		conprintf(0, "LoadRenderSystems(\"%s\"): parsed %u systems, encountered errors!\n", filename, numsystems);
	else
		conprintf(0, "LoadRenderSystems(\"%s\"): added %u systems.\n", filename, numsystems);

	return numsystems;
}
