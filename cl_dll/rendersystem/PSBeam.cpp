#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSBeam.h"
#include "pm_shared.h"
#include "event_api.h"

CPSBeam::CPSBeam(void) : CPSBeam::BaseClass()
{
	DBG_RS_PRINT("CPSBeam()");
	ResetParameters();// non-recursive
}

CPSBeam::CPSBeam(uint32 maxParticles, const Vector &origin, const Vector &end, struct model_s *pTexture, int r_mode, float timetolive)
{
	DBG_RS_PRINT("CPSBeam(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_vecEnd = end;
	m_fEnergyStart = 1.0f;// XDM3037a: to start drawing

	m_iRenderMode = r_mode;
	if (timetolive <= 0.0f)// XDM3038a: was <
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSBeam::ResetAllParameters(void)
{
	CPSBeam::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSBeam::ResetParameters(void)
{
	m_vecEnd.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Very old code, functionality is unknown
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CPSBeam::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0 && m_fDieTime <= time)
		ShutdownSystem();

	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;
	if (!CheckFxLevel())// XDM3038c
		return 0;

	//UpdateFrame();
	FollowEntity();
	Emit();

	pmtrace_t pmtrace;
	CParticle *pCurPart = NULL;

	if (!(m_iFlags & RENDERSYSTEM_FLAG_NOCLIP))
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
	}
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
			RemoveParticle(pCurPart);// XDM3037
			continue;
		}
		pCurPart->m_vPosPrev = pCurPart->m_vPos;
		pCurPart->m_vVel.MultiplyAdd(elapsedTime, pCurPart->m_vAccel);//pCurPart->m_vVel += elapsedTime*pCurPart->m_vAccel;
		pCurPart->m_vPos.MultiplyAdd(elapsedTime, pCurPart->m_vVel);//pCurPart->m_vPos += elapsedTime*pCurPart->m_vVel;

		if (!(m_iFlags & RENDERSYSTEM_FLAG_NOCLIP))
		{
			gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, -1, &pmtrace);
			if (pmtrace.fraction != 1.0f)
			{
				RemoveParticle(pCurPart);// XDM3037
				continue;
			}
		}

		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			pCurPart->FrameRandomize();
		else if (m_fFrameRate != 0)
			pCurPart->FrameIncrease();

		pCurPart->m_fEnergy -= (float)(1.5 * elapsedTime);
		pCurPart->UpdateColor(elapsedTime);
		pCurPart->UpdateSize(elapsedTime);
		pCurPart->m_fColor[3] = pCurPart->m_fEnergy;
	}
	if (!(m_iFlags & RENDERSYSTEM_FLAG_NOCLIP))
		gEngfuncs.pEventAPI->EV_PopPMStates();
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : pParticle - particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSBeam::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	float dist = RANDOM_FLOAT(0,1);
	pParticle->m_vPos = m_vecOrigin;
	pParticle->m_vPosPrev = m_vecOrigin;
	pParticle->m_vAccel.Clear();
	pParticle->m_vVel = m_vecEnd - m_vecOrigin;
	VectorMA(pParticle->m_vPos, dist, pParticle->m_vVel, pParticle->m_vPos);// pos += vel+dist
	pParticle->m_vVel.NormalizeSelf();
//???	pParticle->m_vVel = pParticle->m_vVel * 0; //Scalar velocity component
	pParticle->m_fEnergy = m_fEnergyStart;// XDM3037a: to start drawing
	pParticle->m_fSizeX = 1.0f;
	pParticle->m_fSizeY = 1.0f;
	pParticle->m_fSizeDelta = 0.0f;
	pParticle->m_pTexture = m_pTexture;
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))
		pParticle->FrameRandomize();

	pParticle->SetDefaultColor();
}
