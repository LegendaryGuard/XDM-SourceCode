#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSDelayed.h"

//-----------------------------------------------------------------------------
// Purpose: Default constructor. Should not be used for this system.
//-----------------------------------------------------------------------------
CRSDelayed::CRSDelayed(void) : CRSDelayed::BaseClass()
{
	DBG_RS_PRINT("CRSDelayed()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: This system is used to delay creation of another render system
// Input  : *pSystem - a newly created, NOT ADDED to the render manager list RS
//			delay - time period in which pSystem will be activated
//-----------------------------------------------------------------------------
CRSDelayed::CRSDelayed(CRenderSystem *pSystem, float delay, int flags, int followentindex, int followflags)
{
	DBG_RS_PRINT("CRSDelayed(...)");
	if (pSystem)
	{
		ResetParameters();
		m_pSystem = pSystem;
		m_pSystem->m_iFlags |= flags;// add! system may contain some custom flags inside already!
		m_pSystem->m_iFollowEntity = followentindex;
		m_pSystem->m_iFollowFlags |= followflags;// add!
		m_fDieTime = gEngfuncs.GetClientTime() + delay;
		//InitializeSystem();// is this an exception among all systems?
	}
	else
	{
		conprintf(1, "CRSDelayed() error: no pSystem!\n");
		m_RemoveNow = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSDelayed::~CRSDelayed(void)
{
	DBG_RS_PRINT("~CRSDelayed()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSDelayed::ResetAllParameters(void)
{
	CRSDelayed::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSDelayed::ResetParameters(void)
{
	SetBits(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_NODRAW|RENDERSYSTEM_FLAG_UPDATEOUTSIDEPVS);
	m_pSystem = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSDelayed::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSDelayed::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSDelayed::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	if (m_pSystem)// somehow we didn't activate this system, so just delete it
	{
		delete m_pSystem;// it was not added to the manager's list
		m_pSystem = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSDelayed::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "szSystem") == 0)// UNDONE: this still does not work because we cannot create system from script and NOT add it to manager!
	{
		if (m_pSystem == NULL)
		{
			if (szValue && szValue[0] != '\0')
			{
				CRenderSystem *pSystem = g_pRenderManager->FindSystemByName(szValue);
				if (pSystem)
				{
					if (pSystem->GetState() == RSSTATE_DISABLED)
						m_pSystem = pSystem;
					else
						conprintf(0, " Error: %s %u: %s system \"%s\" is already active!\n", GetClassName(), GetIndex(), szKey, szValue);
				}
				else
					conprintf(0, " Error: %s %u: %s system \"%s\" not found!\n", GetClassName(), GetIndex(), szKey, szValue);
			}
		}
		else
			conprintf(0, " Error: %s %u: already has \"%s\" set!\n", GetClassName(), GetIndex(), szKey);
	}
	else
		return CRSDelayed::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check if it is time to activate our system
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRSDelayed::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime <= time)// do not call parent updates
	{
		if (m_pSystem)// WARNING! Very tricky code here!
		{
			if (m_pSystem->m_fStartTime <= 0.0f && m_pSystem->m_fDieTime > 0.0f)// m_pSystem->m_fDieTime was set at MY start time, so reschedule it according to NOW
			{
				float lifetime = (m_pSystem->m_fDieTime - m_fStartTime);
				DBG_RS_PRINTF("%s[%d]\"%s\": Update() rescheduling system lifetime (%gs)\n", GetClassName(), GetIndex(), GetUID(), lifetime);
				m_pSystem->m_fDieTime = gEngfuncs.GetClientTime() + lifetime;
			}
			g_pRenderManager->AddSystem(m_pSystem, m_pSystem->m_iFlags, m_pSystem->m_iFollowEntity, m_pSystem->m_iFollowFlags, true, true);
			m_pSystem = NULL;// the new system MODIFIED the render manager's linked list order! DO NOT remove ourself right now! Will do on next update.
		}
		else// Remove self only on NEXT update! This is probably a hack...
			return 1;// ready for removal
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing
//-----------------------------------------------------------------------------
void CRSDelayed::Render(void)
{
}
