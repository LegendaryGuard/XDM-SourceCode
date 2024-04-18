#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSFlameCone.h"
#include "pm_shared.h"
#include "event_api.h"

CPSFlameCone::CPSFlameCone(void) : CPSFlameCone::BaseClass()
{
	DBG_RS_PRINT("CPSFlameCone()");
	ResetParameters();// non-recursive
}

CPSFlameCone::CPSFlameCone(uint32 maxParticles, const Vector &origin, const Vector &direction, const Vector &spread, float velocity, struct model_s *pTexture, int r_mode, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	DBG_RS_PRINT("CPSFlameCone(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = origin;
	if (origin == direction)
	{
		m_vDirection.Clear();
		m_iMovementType = PSMOVTYPE_OUTWARDS;//m_flRandomDir = true;
	}
	else
	{
		m_vDirection = direction;
		m_iMovementType = PSMOVTYPE_DIRECTED;//m_flRandomDir = false;
	}
	m_fScale = scale*0.1f;//cl_test2->value;// COMPATIBILITY HACK
	m_fScaleDelta = scaledelta;
	m_color.a = a*255.0f;// compatibility
	m_fColorDelta[3] = adelta;// 0.0f
	// OLD m_fBrightness = a;// 1.0f
	m_iRenderMode = r_mode;
	m_fParticleSpeedMin = m_fParticleSpeedMax = velocity;
	m_iTraceFlags = PM_STUDIO_IGNORE;
	m_fEnergyStart = 1.0f;// XDM3037a: to start drawing
	m_vSpread = spread;
	m_iMaxParticles = maxParticles;

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSFlameCone::ResetAllParameters(void)
{
	CPSFlameCone::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSFlameCone::ResetParameters(void)
{
	m_iTraceFlags = PM_WORLD_ONLY;//PM_STUDIO_IGNORE;//PM_STUDIO_BOX
	m_iMovementType = PSMOVTYPE_OUTWARDS;
	m_fParticleSpeedMin = 280.0f;// XDM3038b
	m_fParticleSpeedMax = 300.0f;
	m_fColorDelta[3] = -1.5f;
	m_fBounceFactor = -1.0f;// don't bounce, move along
}

//-----------------------------------------------------------------------------
// TODO: remove this in favor of customized CParticleSystem
// Purpose: Update system parameters along with time
// Warning: DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
/* OBSOLETE bool CPSFlameCone::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		m_ShuttingDown = true;// when true, Emit() will stop producing particles, and remaining ones will (hopefully) disappear

	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;

	//
	FollowEntity();
	Emit();

	if (GetState() != RSSTATE_ENABLED && m_iNumParticles == 0)// XDM3038b: optimization: don't iterate
		return 0;

	if (IsShuttingDown() && (m_iFlags & RENDERSYSTEM_FLAG_HARDSHUTDOWN))// XDM3038c
		return 1;

	//iLastFrame
	//UpdateFrame
	pmtrace_t pmtrace;
	int contents;
	int ignore_pe;
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS) && !FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// XDM3038c
	{
		ignore_pe = m_pLastFollowedEntity?GetPhysent(m_pLastFollowedEntity->index):-1;
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
	}
	else
		ignore_pe = -1;

	CParticle *pCurPart;
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
			RemoveParticle(pCurPart);
			continue;
		}
		pCurPart->m_vPosPrev = pCurPart->m_vPos;
		pCurPart->m_vPos.MultiplyAdd(elapsedTime, pCurPart->m_vVel);//pCurPart->m_vPos += elapsedTime*pCurPart->m_vVel;
		// check contents right after updating origin, because we may not need anything else
		contents = gEngfuncs.PM_PointContents(pCurPart->m_vPos, NULL);
		if (contents == CONTENTS_SOLID && (m_iFlags & RENDERSYSTEM_FLAG_CLIPREMOVE))// XDM3038c: hack?
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
			gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, ignore_pe, &pmtrace);
			if (pmtrace.fraction != 1.0f)
			{
				if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))
				{
					RemoveParticle(pCurPart);
					continue;
				}
				else if (m_OnParticleCollide)// custom callback was provided
				{
					m_OnParticleCollide(this, pCurPart, m_pOnParticleCollideData, &pmtrace);
				}
				else if (m_fBounceFactor > 0.0f)// reflect velocity, bounce
				{
					pCurPart->m_vVel.MirrorByVector(pmtrace.plane.normal);
					pCurPart->m_vVel *= m_fBounceFactor;
				}
				else if (m_fBounceFactor == 0.0f)// stick // this system does not reflect
					pCurPart->m_vVel.Clear();
				else// slide
				{
					PM_ClipVelocity(pCurPart->m_vVel, pmtrace.plane.normal, pCurPart->m_vVel, 1.0f);
					vec_t fSpeed = pCurPart->m_vVel.NormalizeSelf();
					fSpeed -= (float)(m_fFriction*elapsedTime);// friction is applied constantly every frame, so make it time-based
					pCurPart->m_vVel *= fSpeed;
				}
			}
			//
			//
		}

		pCurPart->m_vVel.MultiplyAdd(elapsedTime, pCurPart->m_vAccel);//pCurPart->m_vVel += elapsedTime*pCurPart->m_vAccel;
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDGRAVITY))// XDM3038a
			pCurPart->m_vVel.z -= elapsedTime*g_cl_gravity*m_fParticleWeight;

		// this system uses old frame switching
		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			pCurPart->FrameRandomize();
		else if (m_fFrameRate != 0)
			pCurPart->FrameIncrease();

		pCurPart->UpdateColor(elapsedTime);
		pCurPart->UpdateEnergyByBrightness();// XDM3035c
		pCurPart->UpdateSize(elapsedTime);

		if (m_OnParticleUpdate)
			m_OnParticleUpdate(this, pCurPart, m_pOnParticleUpdateData, time, elapsedTime);
	}
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS) && !FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// XDM3038c
		gEngfuncs.pEventAPI->EV_PopPMStates();// we were tracing

	if (m_iFlags & RENDERSYSTEM_FLAG_SIMULTANEOUS)// XDM3035c: TESTME
		if (m_iNumParticles == 0)
			return 1;

	if (m_iNumParticles == 0 && IsShuttingDown())// XDM3037: there's nothing to draw anyway
		return 1;

	return 0;
}*/

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : index - particle index in array
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
/*void CPSFlameCone::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	if (cl_test1->value > 0)
		CPSFlameCone::BaseClass::InitializeParticle(pParticle, fInterpolaitonMult);
	else
	{
	if (fInterpolaitonMult > 0.0f)// interpolate position between CURRENT and PREVIOUS origins
	{
		Vector vDelta(m_vecOrigin); vDelta -= m_vecOriginPrev; vDelta *= fInterpolaitonMult;// we're moving point backwards
		pParticle->m_vPos -= vDelta;
	}
	pParticle->m_vPos = m_vecOrigin;
	pParticle->m_vPosPrev = m_vecOrigin;
	pParticle->m_vAccel.Clear();
	VectorRandom(pParticle->m_vVel);
	Vector rnd2;
	VectorRandom(rnd2);
	pParticle->m_vVel += rnd2;//VectorAdd(VectorRandom(), VectorRandom(), pParticle->m_vVel);

	if (m_iMovementType == PSMOVTYPE_OUTWARDS)// explosion
	{
		pParticle->m_vVel.NormalizeSelf();// v = rnd
	}
	else// fountain
	{
		if (!m_vSpread.IsZero())
			pParticle->m_vVel *= m_vSpread;

		pParticle->m_vVel += m_vDirection;// v = dir + rnd*spread
	}
	pParticle->m_pTexture = m_pTexture;
	if (m_OnParticleInitialize == NULL || m_OnParticleInitialize(this, pParticle, m_pOnParticleInitializeData, fInterpolaitonMult) == true)
	{
		pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);// XDM3038b
		pParticle->m_fEnergy = m_fEnergyStart;// XDM3037a: to start drawing
		pParticle->m_fSizeX = m_fScale*7.0f;// XDM: we have to keep ugly constants for compatibility :(
		pParticle->m_fSizeY = m_fScale*7.0f;
		pParticle->m_fSizeDelta = m_fScaleDelta;
		//pParticle->SetDefaultColor();
		pParticle->SetColor(m_color, m_fBrightness);
		pParticle->SetColorDelta(m_fColorDelta);

		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))
			pParticle->FrameRandomize();
	}
	}
}*/
