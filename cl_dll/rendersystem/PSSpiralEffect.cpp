#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSSpiralEffect.h"
#include "triangleapi.h"

CPSSpiralEffect::CPSSpiralEffect(void) : CPSSpiralEffect::BaseClass()
{
	DBG_RS_PRINT("CPSSpiralEffect()");
	ResetParameters();// non-recursive
}

CPSSpiralEffect::CPSSpiralEffect(uint32 maxParticles, const Vector &origin, float radius, float radiusdelta, model_t *pSprite, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive)
{
	DBG_RS_PRINT("CPSSpiralEffect(...)");
	ResetParameters();
	if (!InitTexture(pSprite))
	{
		m_RemoveNow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_fScale = radius;
	m_fScaleDelta = radiusdelta;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[0] = 0.1f;// default
	m_fColorDelta[1] = 0.1f;
	m_fColorDelta[2] = 0.1f;
	m_fColorDelta[3] = adelta;
	m_fSizeX = 0.015625f;//1.0f; //COMPATIBILITY: 1/64th for 64px texture
	m_fSizeY = 0.015625f;//1.0f;
	m_iRenderMode = r_mode;
	//conprintf(1, "CPSSpiralEffect(r %f, dr %f, rm %d, c %d %d %d, a %f, da %f)\n", radius, radiusdelta, r_mode, r, g, b, a, adelta);

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSSpiralEffect::ResetAllParameters(void)
{
	CPSSpiralEffect::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSSpiralEffect::ResetParameters(void)
{
	m_fBounceFactor = 0.0f;
	SetBits(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP);//|RENDERSYSTEM_FLAG_SIMULTANEOUS);
	m_iTraceFlags = PM_WORLD_ONLY|PM_GLASS_IGNORE|PM_STUDIO_IGNORE;
	m_iMovementType = PSMOVTYPE_DIRECTED;
	m_fParticleWeight = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : pParticle - particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSSpiralEffect::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	pParticle->m_vPos.x = m_vecOrigin.x + m_fScale*sinf((float)(pParticle->index*2));
	pParticle->m_vPos.y = m_vecOrigin.y + m_fScale*cosf((float)(pParticle->index*2));
	pParticle->m_vPos.z = m_vecOrigin.z + ((float)pParticle->index)*(48.0f/(float)m_iMaxParticles) - 24.0f;// 48 means player height, 24 is half.
	pParticle->m_vPosPrev = pParticle->m_vPos;
	pParticle->m_vAccel.Set(0,0,4);
	pParticle->m_vVel.Set(0,0,16);
	pParticle->m_fEnergy = m_fEnergyStart;// XDM3037a: to start drawing
	pParticle->m_fSizeDelta = (float)(pParticle->index+1)*0.25f;
	pParticle->m_pTexture = m_pTexture;
	if (m_OnParticleInitialize == NULL || m_OnParticleInitialize(this, pParticle, m_pOnParticleInitializeData, fInterpolaitonMult) == true)
	{
		pParticle->SetSizeFromTexture(m_fSizeX, m_fSizeY);
		// done in constructor	pParticle->m_frame = 0;
		pParticle->SetColor(m_fColorCurrent);//pParticle->SetColor(m_color, m_fBrightness);
		pParticle->SetColorDelta(m_fColorDelta);
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))
			pParticle->FrameRandomize();
	}
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_SIMULTANEOUS) && pParticle->index >= m_iMaxParticles-1)// && m_fDieTime <= 0.0f)// XDM3038c: initialized last particle
		m_ShuttingDown = true;
}

//-----------------------------------------------------------------------------
// Purpose: Legacy: Update system parameters along with time
// Warning: DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CPSSpiralEffect::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		m_ShuttingDown = true;
	//no else if (m_iNumParticles == 0)
	//	m_ShuttingDown = true;

	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;
	if (!CheckFxLevel())// XDM3038c
		return 0;

	int iNumCreated = Emit();
	if (GetState() != RSSTATE_ENABLED && m_iNumParticles == 0 && iNumCreated == 0)// XDM3038b: optimization: don't iterate
		return 0;

	FollowEntity();

	uint32 iLastFrame = m_iFrame;
	UpdateFrame(time, elapsedTime);// update system's frame according to desired frame rate, required to detect proper time to update frames in particles
	float s,c;
	CParticle *pCurPart = NULL;
	m_iNumParticles = m_iMaxParticles;// XDM3037: new allocation mechanism: counter
	for (uint32 i = 0; i < m_iMaxParticles; ++i)// XDM3037: unstable m_iNumParticles; ++i)
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
		SinCos(-6.0f*time + (float)(i*2), &s, &c);
		pCurPart->m_vPos.x = m_vecOrigin.x + m_fScale*s;
		pCurPart->m_vPos.y = m_vecOrigin.y + m_fScale*c;
		pCurPart->m_vPos.z += pCurPart->m_vVel.z*elapsedTime;
		//pCurPart->m_vPos[0] = m_vecOrigin[0] + m_fScale*sinf(-6.0f*time + (float)(i*2));
		//pCurPart->m_vPos[1] = m_vecOrigin[1] + m_fScale*cosf(-6.0f*time + (float)(i*2));
		if (iLastFrame != m_iFrame)// time to change the frame
		{
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
				pCurPart->FrameRandomize();
			else if (m_fFrameRate != 0.0f)
				pCurPart->FrameIncrease();
		}
		pCurPart->UpdateColor(elapsedTime);
		pCurPart->UpdateSize(elapsedTime);
		pCurPart->UpdateEnergyByBrightness();
	}
	m_fScale += m_fScaleDelta*elapsedTime;// effect radius is increasing
	if (IsShuttingDown())// XDM3037
	{
		if (m_iNumParticles == 0)
			return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Optimized version without unnecessary checks
//-----------------------------------------------------------------------------
void CPSSpiralEffect::Render(void)
{
	Vector vecDelta(m_vecOrigin); vecDelta -= g_vecViewOrigin;
	vec_t fViewDist = vecDelta.Length2D();// WARNING: right now this system is vertical, so 2D is a must
	if (fViewDist <= m_fScale || PointIsVisible(m_vecOrigin))// draw when viewer is inside the circle of particles
	{
		//BaseClass::Render();
		Vector v_up, v_right;
		AngleVectors(g_vecViewAngles, NULL, v_right, v_up);// + m_vecAngles?
		CParticle *p = NULL;
		for (uint32 i = 0; i < m_iMaxParticles; ++i)
		{
			p = &m_pParticleList[i];
			if (p->IsExpired())
				continue;

			p->Render(v_right, v_up, m_iRenderMode);
		}
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);// ?
	}
}
