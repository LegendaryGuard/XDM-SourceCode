#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSSparks.h"
#include "pm_shared.h"
#include "event_api.h"
#include "triangleapi.h"
#include "studio_util.h"

CPSSparks::CPSSparks(void) : CPSSparks::BaseClass()
{
	DBG_RS_PRINT("CPSSparks()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : maxParticles - 
//			origin - 
//			scalex, scaley - Use negative scale values to get real sprite dimensions multiplied by fabs(scale)
//			scaledelta - was always 0.5f
//			velocity - 
//			startenergy - constantly decreases (particles are removed when their energy reaches zero), was always 2.0f
//			r,g,b - rendercolor
//			a - alpha/brightness
//			adelta - alpha/brightness delta
//			pTexture - sprite
//			r_mode - render mode
//			timetolive - 
//-----------------------------------------------------------------------------
CPSSparks::CPSSparks(uint32 maxParticles, const Vector &origin, float scalex, float scaley, float scaledelta, float velocity, float startenergy, byte r, byte g, byte b, float a, float adelta, struct model_s *pTexture, int r_mode, float timetolive)
{
	DBG_RS_PRINT("CPSSparks(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_fEnergyStart = startenergy;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = 0.125f;//1/8;cl_test1->value;// XDM3038b: hate magic numbers, but COMPATIBILITY
	m_fScaleDelta = scaledelta*0.01f;// HACK? COMPATIBILITY

	if (scalex > 0.0f)// otherwise we have default value for these
		m_fSizeX = scalex;

	if (scaley > 0.0f)
		m_fSizeY = scaley;

	if (velocity < 0.0f)
	{
		m_iStartType = PSSTARTTYPE_SPHERE;
		m_iMovementType = PSMOVTYPE_INWARDS;//m_bReversed = true;
		m_fParticleSpeedMin = m_fParticleSpeedMax = -velocity;
	}
	else
	{
		m_iStartType = PSSTARTTYPE_POINT;
		m_iMovementType = PSMOVTYPE_OUTWARDS;
		m_fParticleSpeedMin = m_fParticleSpeedMax = velocity;
	}
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
/*void CPSSparks::ResetAllParameters(void)
{
	CPSSparks::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSSparks::ResetParameters(void)
{
	m_iTraceFlags = PM_STUDIO_IGNORE;//PM_STUDIO_BOX
	m_iStartType = PSSTARTTYPE_POINT;
	m_iMovementType = PSMOVTYPE_OUTWARDS;
	m_fSizeX = 0.1f;
	m_fSizeY = 0.05f;
	m_fBounceFactor = 0.8f;// bounce with energy loss
	//m_fParticleSpeedMin = m_fParticleSpeedMax = 200.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPSSparks::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)// XDM3035b: how did I managed to miss this?!
		m_ShuttingDown = true;// when true, Emit() will stop producing particles, and remaining ones will (hopefully) disappear

	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;

/*#if defined (_DEBUG)
	if ((time - m_fStartTime) > 20.0f && (m_iNumParticles/m_iMaxParticles < 0.1f))// > 20sec and < 10% particles
		conprintf(1, "CPSSparks HUNG SYSTEM %d WITH %d PARTICLES PROBABLY DETECTED\n", GetIndex(), m_iNumParticles);
#endif*/
	//UpdateFrame();
	int iNumCreated = Emit();
	if (GetState() != RSSTATE_ENABLED && m_iNumParticles == 0 && iNumCreated == 0)// XDM3038b: optimization: don't iterate
		return 0;

	FollowEntity();

	pmtrace_t pmtrace;
	int contents;
	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
	}
	CParticle *pCurPart = NULL;
	m_iNumParticles = m_iMaxParticles;
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
		pCurPart->m_vVel.MultiplyAdd(elapsedTime, pCurPart->m_vAccel);//pCurPart->m_vVel += elapsedTime*pCurPart->m_vAccel;
		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDGRAVITY))
			pCurPart->m_vVel.z -= elapsedTime*g_cl_gravity*m_fParticleWeight;

		pCurPart->m_vPos.MultiplyAdd(elapsedTime, pCurPart->m_vVel);//pCurPart->m_vPos += elapsedTime*pCurPart->m_vVel;

		if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))
		{
			// TODO: don't collide with owner
			gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, -1, &pmtrace);
			if (pmtrace.fraction != 1.0f)
			{
				if (m_iFlags & RENDERSYSTEM_FLAG_CLIPREMOVE)// remove particle
				{
					RemoveParticle(pCurPart);// XDM3037
					continue;
				}
				else if (m_iFlags & RENDERSYSTEM_FLAG_ADDPHYSICS)// reflect particle velocity
				{
					vec_t p = DotProduct(pCurPart->m_vVel, pmtrace.plane.normal);
					VectorMA(pCurPart->m_vVel, -2.0f*p, pmtrace.plane.normal, pCurPart->m_vVel);
					pCurPart->m_vVel *= 0.8f;
					pCurPart->m_fColor[3] *= 0.8f;// XDM3035b: lose some energy
				}
			}
		}

		if (m_OnParticleUpdate == NULL || m_OnParticleUpdate(this, pCurPart, m_pOnParticleUpdateData, time, elapsedTime) == true)
		{
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
				pCurPart->FrameRandomize();
			else if (m_fFrameRate != 0.0f)
				pCurPart->FrameIncrease();

			pCurPart->UpdateColor(elapsedTime);
			pCurPart->UpdateSize(elapsedTime);
			pCurPart->UpdateEnergyByBrightness();// this is so wrong!
			//conprintf(1, "P %d E %f\n", i, pCurPart->m_fEnergy);
		}
	}
	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))
		gEngfuncs.pEventAPI->EV_PopPMStates();

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : pParticle - particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSSparks::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	Vector rnd;// = VectorRandom() + VectorRandom();
	VectorRandom(rnd);
	rnd += VectorRandom();//VectorNormalize(rnd);// too ideal
	if (m_iMovementType == PSMOVTYPE_INWARDS)
	{
		pParticle->m_vPos = rnd;// pos = org + speed*dir
		pParticle->m_vPos *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);// XDM3038b
		rnd *= -1.0f;
	}
	else
		pParticle->m_vPos.Clear();

	if (fInterpolaitonMult > 0.0f)// interpolate position between CURRENT and PREVIOUS origins
	{
		Vector vDelta(m_vecOrigin); vDelta -= m_vecOriginPrev; vDelta *= fInterpolaitonMult;// we're moving point backwards
		pParticle->m_vPos -= vDelta;
	}
	pParticle->m_vPos += m_vecOrigin;

	pParticle->m_vVel = rnd;
	pParticle->m_vVel *= RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax);// XDM3038b

	pParticle->m_vPosPrev = pParticle->m_vPos;
	pParticle->m_vPosPrev -= pParticle->m_vVel;// just to start drawing in proper direction
	pParticle->m_vAccel.Clear();
	pParticle->m_fEnergy = m_fEnergyStart;
	pParticle->m_pTexture = m_pTexture;

	if (m_OnParticleInitialize == NULL || m_OnParticleInitialize(this, pParticle, m_pOnParticleInitializeData, fInterpolaitonMult) == true)
	{
		pParticle->m_fSizeX = m_fSizeX * m_fScale;///cl_test2->value;//fabs(m_fSizeX); already checked and is > 0
		pParticle->m_fSizeY = m_fSizeY * m_fScale;///cl_test2->value;//fabs(m_fSizeY);
		//?pParticle->SetSizeFromTexture(m_fSizeX*m_fScale, m_fSizeY*m_fScale);
		pParticle->m_fSizeDelta = m_fScaleDelta;// 0.5f
		pParticle->SetColor(m_fColorCurrent);//pParticle->SetColor(m_color, m_fBrightness);
		pParticle->SetColorDelta(m_fColorDelta);

		if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME|RENDERSYSTEM_FLAG_STARTRANDOMFRAME))
			pParticle->FrameRandomize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CPSSparks::Render(void)
{
	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
		return;

	CParticle *pCurPart = NULL;
	Vector v_fwd, crossvel, backpoint, vx, cy;
	AngleVectors(g_vecViewAngles, v_fwd, NULL, NULL);

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	for (uint32 i = 0; i < m_iMaxParticles; ++i)
	{
		pCurPart = &m_pParticleList[i];

		if (pCurPart->IsExpired())
			continue;
		//if (!PointIsVisible(pCurPart->m_vPos))// slower on modern computers
		//	continue;
		if (m_iDrawContents != 0 && !ContentsArrayHas(m_iDrawContents, gEngfuncs.PM_PointContents(pCurPart->m_vPos, NULL)))
			continue;

		pCurPart->m_iFrame = m_iFrame;
		//velocity = pCurPart->m_vVel;
		CrossProduct(pCurPart->m_vVel, v_fwd, crossvel);

		vx = pCurPart->m_vVel*pCurPart->m_fSizeX;
		cy = crossvel*pCurPart->m_fSizeY;
		gEngfuncs.pTriAPI->Color4f(pCurPart->m_fColor[0], pCurPart->m_fColor[1], pCurPart->m_fColor[2], pCurPart->m_fColor[3]);
		gEngfuncs.pTriAPI->Brightness(pCurPart->m_fColor[3]);
		//gEngfuncs.pTriAPI->Brightness(pCurPart->m_fEnergy);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pCurPart->m_vPos - vx + cy);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pCurPart->m_vPos/*+ vx*/+ cy);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pCurPart->m_vPos/*+ vx*/- cy);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pCurPart->m_vPos - vx - cy);
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
