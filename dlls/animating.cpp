/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "saverestore.h"

TYPEDESCRIPTION	CBaseAnimating::m_SaveData[] = 
{
	DEFINE_FIELD(CBaseAnimating, m_flFrameRate, FIELD_FLOAT),
	DEFINE_FIELD(CBaseAnimating, m_flGroundSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CBaseAnimating, m_flLastEventCheck, FIELD_TIME),
	DEFINE_FIELD(CBaseAnimating, m_fSequenceFinished, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseAnimating, m_fSequenceLoops, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseAnimating, m_nFrames, FIELD_INTEGER),// XDM
};

IMPLEMENT_SAVERESTORE(CBaseAnimating, CBaseDelay);

//-----------------------------------------------------------------------------
// Purpose: Model must be set before calling this!
// Note: This function is used as inline... mostly.
// WARNING: Beware of "Cache_UnlinkLRU: NULL link" errors!
//-----------------------------------------------------------------------------
void CBaseAnimating::Spawn(void)// XDM
{
	CBaseDelay::Spawn();// <- Precache();
	if (!FStringNull(pev->model))// XDM3038c
	{
		SET_MODEL(edict(), STRING(pev->model));// XDM3038c: this modifies pev->mins/maxs!
		UTIL_SetOrigin(this, pev->origin);// XDM3038b
		ResetSequenceInfo();// XDM3037
		// ^inside: pev->animtime = gpGlobals->time + 0.1;
		if (pev->modelindex > 0)
		{
			m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM
			if (IsMonster() && /*!FStringNull(pev->model) && */UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))// prevent "Cache_UnlinkMRU: NULL link" fatal error
				::GetEyePosition(GET_MODEL_PTR(edict()), pev->view_ofs);// XDM3038c
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Precache model that was set in pev->model
//-----------------------------------------------------------------------------
void CBaseAnimating::Precache(void)// XDM3038a
{
	if (!FStringNull(pev->model))
		pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));

	if (pev->modelindex > 0)
		m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM3038c

	CBaseDelay::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: StudioFrameAdvance - advance the animation frame up to the current time
// if an flInterval is passed in, only advance animation that number of seconds
// Input  : flInterval - 
// Output : float
//-----------------------------------------------------------------------------
float CBaseAnimating::StudioFrameAdvance(float flInterval)
{
	if (flInterval == 0.0f)
	{
		flInterval = (gpGlobals->time - pev->animtime);
		if (flInterval <= 0.001f)
		{
			pev->animtime = gpGlobals->time;
			return 0.0f;
		}
	}
	if (!pev->animtime)
		flInterval = 0.0f;

	pev->frame += flInterval * m_flFrameRate * pev->framerate;
	pev->animtime = gpGlobals->time;

	if (pev->frame < 0.0f || pev->frame >= 256.0f) 
	{
		if (m_fSequenceLoops)
			pev->frame -= (int)(pev->frame / 256.0f) * 256.0f;
		else
			pev->frame = (pev->frame < 0.0) ? 0 : 255;
		m_fSequenceFinished = TRUE;	// just in case it wasn't caught in GetEvents
	}

	return flInterval;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimating::LookupActivity(int activity)
{
	ASSERT(activity != 0);
	return ::LookupActivity(GET_MODEL_PTR(edict()), pev, activity);
}

//-----------------------------------------------------------------------------
// Purpose: Get activity with highest 'weight'
// Input  : activity - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimating::LookupActivityHeaviest(int activity)
{
	return ::LookupActivityHeaviest(GET_MODEL_PTR(edict()), pev, activity);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *label - 
// Output : int - sequence or -1 on failure
//-----------------------------------------------------------------------------
int CBaseAnimating::LookupSequence(const char *label)
{
	return ::LookupSequence(GET_MODEL_PTR(edict()), label);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAnimating::ResetSequenceInfo(void)
{
	if (pev->modelindex > 0 && !FStringNull(pev->model))
	{
		if (UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))// XDM3037: prevent the infamous "Cache_UnlinkMRU: NULL link" fatal error in GET_MODEL_PTR
		{
			studiohdr_t *pModel = (studiohdr_t *)GET_MODEL_PTR(edict());// XDM3037: faster, safer
			if (pModel)
			{
				if (GetSequenceInfo(pModel, pev, &m_flFrameRate, &m_flGroundSpeed) == 0)// XDM3038a
				{
					m_flFrameRate = 0.0f;
					m_flGroundSpeed = 0.0f;
				}
				m_fSequenceLoops = ((GetSequenceFlags() & STUDIO_LOOPING) != 0);
				m_fSequenceFinished = FALSE;
				m_flLastEventCheck = gpGlobals->time;
				pev->frame = 0.0f;
				pev->animtime = gpGlobals->time;
				pev->framerate = 1.0f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseAnimating::GetSequenceFlags(void)
{
	return ::GetSequenceFlags(GET_MODEL_PTR(edict()), pev);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CBaseAnimating::DispatchAnimEvents(float flInterval)
{
	void *pmodel = GET_MODEL_PTR(edict());
	if (pmodel == NULL)
	{
		ALERT(at_aiconsole, "CBaseAnimating::DispatchAnimEvents() with no model!\n");
		return;
	}

	// HACK FIXME: I have to do this or some events get missed, and this is probably causing the problem below
	flInterval = 0.1f;

	// FIX: this still sometimes hits events twice
	float flStart = pev->frame + (m_flLastEventCheck - pev->animtime) * m_flFrameRate * pev->framerate;
	float flEnd = pev->frame + flInterval * m_flFrameRate * pev->framerate;
	m_flLastEventCheck = pev->animtime + flInterval;

	m_fSequenceFinished = FALSE;
	if (flEnd >= 256.0f || flEnd <= 0.0f)
		m_fSequenceFinished = TRUE;

	int index = 0;
	MonsterEvent_t mevent;
	while ((index = GetAnimationEvent(pmodel, pev, &mevent, flStart, flEnd, index)) != 0)
		HandleAnimEvent(&mevent);
}

//-----------------------------------------------------------------------------
// Purpose: Override this function is subclasses to implement custom event handlers
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CBaseAnimating::HandleAnimEvent(MonsterEvent_t *pEvent)
{
}

float CBaseAnimating::SetBoneController(int iController, float flValue)
{
	return SetController(GET_MODEL_PTR(edict()), pev, iController, flValue);
}

void CBaseAnimating::InitBoneControllers(void)
{
	void *pmodel = GET_MODEL_PTR(edict());
	if (pmodel)
	{
		SetController(pmodel, pev, 0, 0.0f);
		SetController(pmodel, pev, 1, 0.0f);
		SetController(pmodel, pev, 2, 0.0f);
		SetController(pmodel, pev, 3, 0.0f);
	}
}

float CBaseAnimating::SetBlending(byte iBlender, float flValue)
{
	return ::SetBlending(GET_MODEL_PTR(edict()), pev, iBlender, flValue);
}

void CBaseAnimating::GetBonePosition(int iBone, Vector &origin, Vector &angles)
{
	GET_BONE_POSITION(edict(), iBone, origin, angles);
}

void CBaseAnimating::GetAttachment(int iAttachment, Vector &origin, Vector &angles)
{
	GET_ATTACHMENT(edict(), iAttachment, origin, angles);
}

int CBaseAnimating::FindTransition(int iEndingSequence, int iGoalSequence, int *piDir)
{
	void *pmodel = GET_MODEL_PTR(edict());
	if (piDir == NULL)
	{
		int iDir;
		int sequence = ::FindTransition(pmodel, iEndingSequence, iGoalSequence, &iDir);
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}
	return ::FindTransition(pmodel, iEndingSequence, iGoalSequence, piDir);
}

/*void CBaseAnimating::GetAutomovement(Vector &origin, Vector &angles, float flInterval)
{

}*/

// Call this only after the model is set!
void CBaseAnimating::SetBodygroup(int iGroup, int iValue)
{
	::SetBodygroup(GET_MODEL_PTR(edict()), pev, iGroup, iValue);
}

int CBaseAnimating::GetBodygroup(int iGroup)
{
	return ::GetBodygroup(GET_MODEL_PTR(edict()), pev, iGroup);
}

int CBaseAnimating::ExtractBbox(int sequence, Vector &mins, Vector &maxs)
{
	return ::ExtractBbox(GET_MODEL_PTR(edict()), sequence, mins, maxs);
}

void CBaseAnimating::SetSequenceBox(void)
{
	Vector mins, maxs;
	// Get sequence bbox
	if (ExtractBbox(pev->sequence, mins, maxs))
	{
		// expand box for rotation
		// find min / max for rotations
		float yaw = pev->angles.y * (M_PI / 180.0);

		Vector xvector, yvector;
		xvector.x = cos(yaw);
		xvector.y = sin(yaw);
		yvector.x = -sin(yaw);// TODO: #if !defined (NOSQB)?
		yvector.y = cos(yaw);
		Vector bounds[2];

		bounds[0] = mins;
		bounds[1] = maxs;

		Vector rmin(9999, 9999, 9999);
		Vector rmax(-9999, -9999, -9999);
		Vector base, transformed;

		for (short i = 0; i <= 1; ++i)
		{
			base.x = bounds[i].x;
			for (short j = 0; j <= 1; ++j)
			{
				base.y = bounds[j].y;
				for (short k = 0; k <= 1; ++k)
				{
					base.z = bounds[k].z;

					// transform the point
					transformed.x = xvector.x*base.x + yvector.x*base.y;
					transformed.y = xvector.y*base.x + yvector.y*base.y;
					transformed.z = base.z;

					if (transformed.x < rmin.x)
						rmin.x = transformed.x;
					if (transformed.x > rmax.x)
						rmax.x = transformed.x;
					if (transformed.y < rmin.y)
						rmin.y = transformed.y;
					if (transformed.y > rmax.y)
						rmax.y = transformed.y;
					if (transformed.z < rmin.z)
						rmin.z = transformed.z;
					if (transformed.z > rmax.z)
						rmax.z = transformed.z;
				}
			}
		}
		rmin.z = 0;
		rmax.z = rmin.z + 1;
		UTIL_SetSize(this, rmin, rmax);
	}
}

void CBaseAnimating::UpdateFrame(void)
{
//	pev->frame = (int)(pev->frame + 1) % m_nFrames;// XDM
	if (m_nFrames > 1)// && 1/pev->framerate < gpGlobals->frametime)
	{
		if (pev->frame < m_nFrames - 1)
			++pev->frame;
		else
			pev->frame = 0;
	}
}

void CBaseAnimating::ValidateBodyGroups(bool settolast)// XDM3038
{
	int nBGroups = GetEntBodyGroupsCount(edict());
	int submodel, numsubmodels;
	for (int i=0; i<nBGroups; ++i)
	{
		submodel = GetBodygroup(i);
		numsubmodels = GetEntBodyCount(edict(), i);
		if (submodel > numsubmodels)
		{
			ALERT(at_aiconsole, "Warning: %s[%d] %s: has bad body %d set for body group %d!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), submodel, i);
			if (settolast)
				SetBodygroup(i, numsubmodels-1);
			else
				SetBodygroup(i, 0);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBaseAnimating::ReportState(int printlevel)
{
	CBaseDelay::ReportState(printlevel);
	conprintf(printlevel, "SequenceFinished: %d, SequenceLoops: %d, nFrames: %d, FrameRate: %g\n", m_fSequenceFinished, m_fSequenceLoops, m_nFrames, m_flFrameRate);
}
