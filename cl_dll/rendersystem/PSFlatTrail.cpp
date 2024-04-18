#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSFlatTrail.h"
#include "triangleapi.h"

CPSFlatTrail::CPSFlatTrail(void) : CPSFlatTrail::BaseClass()
{
	DBG_RS_PRINT("CPSFlatTrail()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : &start - 
//			&end - 
//			pTexture - 
//			r_mode - 
//			r g b a - 
//			adelta - 
//			scale - 
//			scaledelta - 
//			dist_delta - 
//			timetolive - 
//-----------------------------------------------------------------------------
CPSFlatTrail::CPSFlatTrail(const Vector &start, const Vector &end, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float dist_delta, float timetolive)
{
	DBG_RS_PRINT("CPSFlatTrail(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_vecOrigin = start;// XDM3035c
	m_vDirection = end;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;

	m_vDelta = end;
	m_vDelta -= m_vecOrigin;
	vec_t fDist = m_vDelta.Length();
	if (fDist < dist_delta)
	{
		//conprintf(2, "CPSFlatTrail: fDist < dist_delta\n");
		m_RemoveNow = true;
		return;
	}
	m_fEnergyStart = 1.0f;// XDM3037a: to start drawing
	m_iMaxParticles = (uint32)fabs(fDist/dist_delta);

	if (m_iMaxParticles <= 0)
	{
		//conprintf(2, "CPSFlatTrail: m_iMaxParticles == 0\n");
		m_RemoveNow = true;
		return;
	}
	VectorAngles(m_vDelta, m_vecAngles);// SQB correction done inside

	m_vDelta /= m_iMaxParticles;

	if (timetolive <= 0.0f)// if 0, just fade
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	/* XDM3038c: This may be faster, but we need to reuse as much common code as possible (and it's called once)
	InitializeSystem();
	m_iNumParticles = m_iMaxParticles;

	for (uint32 i = 0; i < m_iNumParticles; ++i)// a little hack?
	{
		m_pParticleList[i].index = i;
		InitializeParticle(&m_pParticleList[i]);
	}*/
	//conprintf(2, "CPSFlatTrail: fDist = %f, m_iMaxParticles = %d, Length(m_vecDelta) = %f\n", fDist, m_iMaxParticles, Length(m_vecDelta));
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSFlatTrail::ResetAllParameters(void)
{
	CPSFlatTrail::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSFlatTrail::ResetParameters(void)
{
	m_fBounceFactor = 0.0f;
	m_iDoubleSided = true;
	SetBits(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP|RENDERSYSTEM_FLAG_SIMULTANEOUS);
	m_iTraceFlags = PM_WORLD_ONLY|PM_GLASS_IGNORE|PM_STUDIO_IGNORE;
	m_iMovementType = PSMOVTYPE_DIRECTED;
	m_fParticleWeight = 0.0f;
	m_vDelta.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : pParticle - particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSFlatTrail::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	//pParticle->m_vPos = m_vecStart + (float)pParticle->index * m_vecDelta;
	//VectorMA(m_vecStart, (float)pParticle->index, m_vecDelta, pParticle->m_vPos);
	// XDM3035c: reverse order
	//pParticle->m_vPos = m_vDirection - (float)pParticle->index * m_vecDelta;
	//VectorMA(m_vDirection, -(float)pParticle->index, m_vecDelta, pParticle->m_vPos);
	pParticle->m_vPos = m_vDelta;
	pParticle->m_vPos *= -(float)pParticle->index;
	pParticle->m_vPos += m_vDirection;
	pParticle->m_vPosPrev = pParticle->m_vPos;
	pParticle->m_vVel.Clear();
	pParticle->m_vAccel.Clear();
	pParticle->m_fEnergy = m_fEnergyStart;// XDM3037a: to start drawing
	pParticle->m_iFlags |= PARTICLE_ORIENTED;
	pParticle->m_pTexture = m_pTexture;

	if (m_OnParticleInitialize == NULL || m_OnParticleInitialize(this, pParticle, m_pOnParticleInitializeData, fInterpolaitonMult) == true)
	{
		pParticle->m_fSizeX = 32.0f*m_fScale;
		pParticle->m_fSizeY = 32.0f*m_fScale;
		pParticle->m_fSizeDelta = m_fScaleDelta;
		pParticle->SetColor(m_fColorCurrent);//pParticle->SetColor(m_color, m_fBrightness);
		pParticle->m_fColorDelta[3] = m_fColorDelta[3];
		// done in constructor	pParticle->m_iFrame = 0;

		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))
			pParticle->FrameRandomize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
/* XDM3038c: This may be a little faster, but we need to reuse as much common code as possible
bool CPSFlatTrail::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		m_ShuttingDown = true;
	else if (m_iNumParticles <= 0)
		m_ShuttingDown = true;

	m_fBrightness = max(0.0f, m_fBrightness + m_fColorDelta[3]*elapsedTime);// update before checking!

	if (!IsShuttingDown() && m_fBrightness <= 0.0f && m_fColorDelta[3] < 0.0f)
		m_ShuttingDown = true;

	if (IsShuttingDown())
		return 1;

	uint32 iLastFrame = m_iFrame;
	UpdateFrame(time, elapsedTime);// update system's frame according to desired frame rate, required to detect proper time to update frames in particles
	CParticle *pCurPart = NULL;
	// special code for this system
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
		if (iLastFrame != m_iFrame)// time to change the frame
		{
			if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
				pCurPart->FrameRandomize();
			else if (m_fFrameRate != 0)
				pCurPart->FrameIncrease();
		}
		// causes particles to disappear TOO early, spoils smoothness of effect pCurPart->m_fEnergy -= elapsedTime;
		pCurPart->m_fColor[3] = m_fBrightness;
		pCurPart->UpdateSize(elapsedTime);
		pCurPart->UpdateEnergyByBrightness();// XDM3035c: HOW DID THIS WORK BEFORE?!
	}
	return 0;
}*/

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CPSFlatTrail::Render(void)
{
	if (m_pTexture == NULL)
		return;

	//if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, frame))
	//	return;
	Vector up, rt;
	AngleVectors(m_vecAngles, NULL, rt, up);// system rotation, same for all particles
	CParticle *p = NULL;
	/* TEST: draw order
	bool bDrawEndFirst = true;
	if (!PointIsVisible(m_vDirection) || ((m_vDirection - g_vecViewOrigin).Length() < (m_vecOrigin - g_vecViewOrigin).Length()))// m_vDirection is actually vecEnd
		bDrawEndFirst = false;

	if (bDrawEndFirst)
	{*/
		for (uint32 i = 0; i < m_iMaxParticles; ++i)
		{
			p = &m_pParticleList[i];
			if (p->IsExpired())
				continue;
			//if (m_iDrawContents != 0 && !ContentsArrayHas(m_iDrawContents, gEngfuncs.PM_PointContents(p->m_vPos, NULL)))
			//	continue;
			p->Render(rt, up, m_iRenderMode, true);// always // m_iDoubleSided
		}
	/*}
	else
	{
		for (uint32 i = m_iMaxParticles-1; i >= 0; --i)
		{
			p = &m_pParticleList[i];
			if (p->IsExpired())
				continue;
			//if (m_iDrawContents != 0 && !ContentsArrayHas(m_iDrawContents, gEngfuncs.PM_PointContents(p->m_vPos, NULL)))
			//	continue;
			p->Render(rt, up, m_iRenderMode, true);// always // m_iDoubleSided
		}
	}*/
	//gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
