#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSDrips.h"
#include "pm_shared.h"
#include "event_api.h"
#include "triangleapi.h"
#include "studio_util.h"
#include "bsputil.h"

#include "cl_fx.h"

// UNDONE: ParseKeyValue()

CPSDrips::CPSDrips(void) : CPSDrips::BaseClass()
{
	DBG_RS_PRINT("CPSDrips()");
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Spawn drips from specified volume and send them in specified direction
// Input  : maxParticles - 
//			&origin - center of system (mins/maxs will be applied)
//			&mins &maxs - volume in which the particles will be created (relative)
//			&dir - direction AND speed as a vector length
//			sprindex - main particle texture
//			sprindex_splash - (optional) impact particle texture
//			r_mode - render mode
//			sizex sizey - particle size
//			scaledelta - 
//			timetolive - 
//-----------------------------------------------------------------------------
CPSDrips::CPSDrips(uint32 maxParticles, const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &direction, struct model_s *pTexture, struct model_s *pTextureHitWater, struct model_s *pTextureHitGround, int r_mode, float sizex, float sizey, float scaledelta, float timetolive)
{
	DBG_RS_PRINT("CPSDrips(...)");
	ResetParameters();
	if (!InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	if (pTextureHitWater && pTextureHitWater->type == mod_sprite)
		m_pTextureHitWater = pTextureHitWater;
	else
		m_pTextureHitWater = NULL;

	if (pTextureHitGround && pTextureHitGround->type == mod_sprite)
		m_pTextureHitGround = pTextureHitGround;
	else
		m_pTextureHitGround = NULL;

	m_iMaxParticles = maxParticles;
	//m_iNumEmit = max(1, maxParticles/4);// XDM3037a
	m_iEmitRate = max(1, maxParticles/4);// XDM3037a COMPATIBILITY
	m_fEnergyStart = 1.0f;// XDM3037a: to start drawing
	m_vStartMins = mins; m_vStartMins.z += 1.0f;//m_vMinS = mins; m_vMinS[2] += 1.0f;
	m_vStartMaxs = maxs;//m_vMaxS = maxs;
	m_vecOrigin = origin;
	m_vDirection = direction;
	m_fParticleSpeedMax = m_fParticleSpeedMin = m_vDirection.NormalizeSelf();// XDM3038b
	m_fParticleSpeedMin *= 0.9f;// old code compatibility
	m_fParticleSpeedMax *= 1.1f;
	m_iRenderMode = r_mode;
	m_fScale = -0.1f;// XDM3035: this prevents CRenderSystem::InitializeSystem() from modifying sizes
	m_fScaleDelta = scaledelta;
	//m_OnParticleUpdate = OnParticleUpdateWhirl;// TEST ONLY

	if (sizex > 0)
		m_fSizeX = sizex;
	else
		m_fSizeX = 1.0f;

	if (sizey > 0)
		m_fSizeY = sizey;
	else
		m_fSizeY = 1.0f;
	//conprintf(1, "CPSDrips: size: %f %f\n", m_fSizeX, m_fSizeY);

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSDrips::ResetAllParameters(void)
{
	CPSDrips::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSDrips::ResetParameters(void)
{
	m_iFlags |= RENDERSYSTEM_FLAG_LOOPFRAMES;// XDM3038b: compatibility HACK
	//not available yet m_iTraceFlags = FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS)?PM_STUDIO_BOX:PM_STUDIO_IGNORE;// COMPATIBILITY
	m_iStartType = PSSTARTTYPE_BOX;
	//m_iNumEmit = 1;
	m_pTextureHitWater = NULL;
	m_pTextureHitGround = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CPSDrips::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		ShutdownSystem();

	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;
	if (!CheckFxLevel())// XDM3038c
		return 0;

	FollowEntity();
	Emit();

	if (GetState() != RSSTATE_ENABLED && m_iNumParticles == 0)// XDM3038b: optimization: don't iterate
		return 0;

	int c = 0;
	uint32 iLastFrame = m_iFrame;
	UpdateFrame(time, elapsedTime);// update system's frame according to desired frame rate, required to detect proper time to update frames in particles
	pmtrace_t pmtrace;
	CParticle *pCurPart = NULL;

	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// && gHUD.m_iIntermission == 0)// XDM3035b: prevent exceptions during level change
	{
		m_iTraceFlags = FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS)?PM_STUDIO_BOX:PM_STUDIO_IGNORE;// COMPATIBILITY
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
		gEngfuncs.pEventAPI->EV_PushPMStates();// BUGBUG: do not call this during map loading process
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

		if (FBitSet(pCurPart->m_iFlags, PSD_PARTICLE_FWATERCIRCLE|PSD_PARTICLE_FSPLASH))// splash/water circle
		{
			pCurPart->m_fEnergy -= (float)(1.5 * elapsedTime);
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
			{
				pCurPart->FrameRandomize();
			}
			else if (m_fFrameRate != 0)
			{
				pCurPart->FrameIncrease();
				if (FBitSet(pCurPart->m_iFlags, PSD_PARTICLE_FSPLASH))// splash sprites play only once
				{
					if (pCurPart->m_iFrame == 0)// the frame is zero AFTER it should've been increased, it means loop
						RemoveParticle(pCurPart);// XDM3037
				}
			}
		}
		else// falling drips
		{
			pCurPart->m_vPosPrev = pCurPart->m_vPos;
			// BUGBUG TODO FIXME T_T  do something with this!!!
			pCurPart->m_vVel.MultiplyAdd(elapsedTime, pCurPart->m_vAccel);//pCurPart->m_vVel += elapsedTime*pCurPart->m_vAccel;
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDGRAVITY))
				pCurPart->m_vVel.z -= elapsedTime*g_cl_gravity*m_fParticleWeight;

			pCurPart->m_vPos.MultiplyAdd(elapsedTime, pCurPart->m_vVel);//pCurPart->m_vPos += elapsedTime*pCurPart->m_vVel;

//XDM3038b: old			gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos, ((m_iFlags & RENDERSYSTEM_FLAG_ADDPHYSICS)?PM_STUDIO_BOX:PM_STUDIO_IGNORE), PHYSENT_NONE, &pmtrace);// TODO: merge with m_iTraceFlags
			//if (!pmtrace.inwater)
			//	Vector vNext = pCurPart->m_vPos + pCurPart->m_vVel*elapsedTime;
				c = gEngfuncs.PM_PointContents(pCurPart->m_vPos, NULL);

			if (/*pmtrace.inwater || */(c < CONTENTS_SOLID && c > CONTENTS_SKY))
			{
				//conprintf(1, "in water\n");
				pCurPart->m_vVel.Clear();
				if (m_pTextureHitWater/* bad! && PointIsVisible(pCurPart->m_vPos)*/)// slow?
				{
					pCurPart->m_pTexture = m_pTextureHitWater;//IEngineStudio.GetModelByIndex(m_iSplashTexture);
					pCurPart->m_fEnergy = m_fEnergyStart;// restart
					pCurPart->SetSizeFromTexture(m_fScale, m_fScale);// slow?
					pCurPart->m_fSizeDelta = PSD_WATERCIRCLE_SIZE_DELTA;
					pCurPart->m_iFlags |= PSD_PARTICLE_FWATERCIRCLE|PARTICLE_ORIENTED;
					pCurPart->m_iFrame = 0;
/*#if defined (DRIPSPARALLELTEST)// testing: circles parallel to surface
					pCurPart->m_vVel = -pmtrace.plane.normal.Normalize();
#endif// unused since there is only horizontal water in Half-Life*/
				}
				else// just remove
				{
					RemoveParticle(pCurPart);// XDM3037
					continue;
				}
			}
			else// XDM3038b
			{
				gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, PHYSENT_NONE, &pmtrace);// TODO: merge with m_iTraceFlags
				if (pmtrace.fraction < 1.0f || c == CONTENTS_SOLID)// && !pmtrace.startsolid)// XDM3035c: allow to start in solid area?.. XDM3037: don't.
				{
					pCurPart->m_vVel.Clear();
					pCurPart->m_fSizeDelta = 0.0f;
					if (!pmtrace.allsolid && !pmtrace.inwater && c != CONTENTS_EMPTY && c != CONTENTS_SKY && m_pTextureHitGround)// bad && PointIsVisible(pCurPart->m_vPos))// slow?
					{
						pCurPart->m_pTexture = m_pTextureHitGround;//IEngineStudio.GetModelByIndex(m_iSplashTexture);
						pCurPart->m_fEnergy = 1.0f;
						pCurPart->SetSizeFromTexture(m_fScale, m_fScale);
						pCurPart->m_iFlags |= PSD_PARTICLE_FSPLASH;
						pCurPart->m_iFrame = 0;
						pCurPart->m_vPos = pCurPart->m_vPosPrev;// revert back a little
					}
					else if (pmtrace.startsolid && !pmtrace.allsolid)// just remove
					{
						// test
					}
					else
						RemoveParticle(pCurPart);// XDM3037

					continue;
				}
			}

			if (iLastFrame != m_iFrame)// time to change the frame
			{
				if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
					pCurPart->FrameRandomize();
				else if (m_fFrameRate != 0)
					pCurPart->FrameIncrease();
			}
		}// drips
		if (m_OnParticleUpdate == NULL || m_OnParticleUpdate(this, pCurPart, m_pOnParticleUpdateData, time, elapsedTime) == true)
		{
			pCurPart->UpdateColor(elapsedTime);
			pCurPart->UpdateSize(elapsedTime);
			pCurPart->m_fColor[3] = pCurPart->m_fEnergy;
		}
	}
	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// && gHUD.m_iIntermission == 0)
		gEngfuncs.pEventAPI->EV_PopPMStates();

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : pParticle - particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSDrips::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	CPSDrips::BaseClass::InitializeParticle(pParticle, fInterpolaitonMult);
	pParticle->m_fSizeX *= m_fSizeX;
	pParticle->m_fSizeY *= m_fSizeY;
}

/*void CPSDrips::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
	Vector delta;
	float d, f;
	uint32 i;
	// particles closer to origin gets more velocity (radius-l)
	// partivles outside radius are not affected
	if (point)// random direction from origin
	{
		f = Length(force)*0.001;//*CVAR_GET_FLOAT("test1");
		//conprintf(1, "CPSDrips: f = %f\n", f);
		for (i = 0; i < m_iMaxParticles; ++i)
		{
			if (m_pParticleList[i].IsExpired())
				continue;

			VectorSubtract(m_pParticleList[i].m_vPos, origin, delta);
			d = Length(delta);
			if (d <= radius)
			{
				// k = radius/d; this should normalize delta-vectors up to radius (I could use Normalize(), but it is SLOWER
				// ADD to previous	VectorScale(delta, (radius/d)*(radius-d)*f, m_pParticleList[i].m_vVelAdd);
				VectorMA(m_pParticleList[i].m_vVelAdd, (radius/d)*(radius-d)*f, delta, m_pParticleList[i].m_vVelAdd);
				//debug	m_pParticleList[i].m_fColor[0]=1.0;
				//m_pParticleList[i].m_fColor[1]=0.1;
				//m_pParticleList[i].m_fColor[2]=0.1;
				//m_pParticleList[i].m_fColor[3]=1.0;
			}
		}
	}
	else
	{
		for (i = 0; i < m_iMaxParticles; ++i)
		{
			if (m_pParticleList[i].m_fEnergy <= 0.0f)
				continue;

			VectorSubtract(m_pParticleList[i].m_vPos, origin, delta);
			d = Length(delta);
			if (d <= radius)
			{
				VectorMA(m_pParticleList[i].m_vVelAdd, (radius/d)*(radius-d), force, m_pParticleList[i].m_vVelAdd);
				//VectorMA(m_pParticleList[i].m_vVelAdd, (radius-l)/radius, force, m_pParticleList[i].m_vVelAdd);
				//VectorAdd(m_pParticleList[i].m_vVelAdd, force, m_pParticleList[i].m_vVelAdd);
				//VectorCopy(m_pParticleList[i].m_vVelAdd, m_pParticleList[i].m_vAccel);
			}
		}
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: Render
// Warning: Particles may have different textures and drawing methods
//-----------------------------------------------------------------------------
void CPSDrips::Render(void)
{
	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_DRAWALWAYS))
		if (!Mod_CheckBoxInPVS(m_vecOrigin+m_vStartMins, m_vecOrigin+m_vStartMaxs))//if (!Mod_CheckBoxInPVS(m_vecOrigin+m_vMinS, m_vecOrigin+m_vMaxS))// XDM3035c: fast way to check visiblility
			return;

	//if (gHUD.m_iPaused <= 0)
	//	m_vecAngles = g_vecViewAngles;

	Vector v_up, v_right;
	/*if (m_iFlags & RENDERSYSTEM_FLAG_ZROTATION)// UNDONE: TODO: rotate around direction vector, not just Z axis
	{
		//m_vecAngles[0] = 0.0f;
		//m_vecAngles[2] = 0.0f;
		v_up = -m_vecDirection;// FIXME: reversed rain particles drawn upside down and facing wrong direction (temporarily fixed with TRI_NONE
	}
	else
		v_up = g_vecViewUp;// FIXME: reversed rain particles drawn upside down and facing wrong direction (temporarily fixed with TRI_NONE*/

	v_up = -m_vDirection;// FIXME: reversed rain particles drawn upside down and facing wrong direction (temporarily fixed with TRI_NONE
	v_right = g_vecViewRight;
	//AngleVectors(m_vecAngles, NULL, v_right, NULL);// TODO: this is common angles for all particles which is fast but not as nice as individual sprite-like rotation
	Vector v1, v2;// up, right for circle
	// UNDONE: right now - horizontal. Otherwise I have no idea of how to get water surface normal. :-/
	v1[0] = 1.0f;
	v1[1] = 0.0f;
	v1[2] = 0.0f;
	v2[0] = 0.0f;
	v2[1] = 1.0f;
	v2[2] = 0.0f;

	Vector rx;// tmp for faster code
	Vector uy;
	Vector delta;
	CParticle *p = NULL;

	// We should draw rain as a single mesh (which is faster) but we can't do that because of different texture frames

	if (m_pTextureHitWater)// water splash: parallel to water surface
	{
		for (uint32 i = 0; i < m_iMaxParticles; ++i)
		{
			p = &m_pParticleList[i];
			if (p->m_iFlags & PSD_PARTICLE_FWATERCIRCLE)
			{
				if (p->IsExpired())
					continue;
				//if (!PointIsVisible(p->m_vPos))// faster? Can't perform check on system origin because it's a large brush entity
				if (UTIL_PointIsFar(p->m_vPos))
					continue;

//#if defined (DRIPSPARALLELTEST)// testing: circles parallel to surface
//				AngleVectors(p->m_vVel, NULL, v1, v2);
//#endif
				if (p->m_iFlags & PARTICLE_ORIENTED)
					p->Render(v1, v2, m_iRenderMode, true);
				else
					p->Render(g_vecViewRight, g_vecViewUp, m_iRenderMode, false);
			}
		}
	}

	if (m_pTextureHitGround)// ground splash: normal sprite
	{
		for (uint32 i = 0; i < m_iMaxParticles; ++i)
		{
			p = &m_pParticleList[i];
			if (p->m_iFlags & PSD_PARTICLE_FSPLASH)
			{
				if (p->IsExpired())
					continue;
				//if (!PointIsVisible(p->m_vPos))// faster? Can't perform check on system origin because it's a large brush entity
				if (UTIL_PointIsFar(p->m_vPos))
					continue;

				if (p->m_iFlags & PARTICLE_ORIENTED)
					p->Render(v1, v2, m_iRenderMode, true);
				else
					p->Render(g_vecViewRight, g_vecViewUp, m_iRenderMode, false);
			}
		}
	}

	if (m_pTexture)// drips
	{
		for (uint32 i = 0; i < m_iMaxParticles; ++i)
		{
			p = &m_pParticleList[i];
			if (!(p->m_iFlags & (PSD_PARTICLE_FWATERCIRCLE|PSD_PARTICLE_FSPLASH)))
			{
				if (p->IsExpired())
					continue;
				if (!PointIsVisible(p->m_vPos))// faster? Can't perform check on system origin because it's a large brush entity
					continue;
				if (m_iDrawContents != 0 && !ContentsArrayHas(m_iDrawContents, gEngfuncs.PM_PointContents(p->m_vPos, NULL)))
					continue;

				if (gEngfuncs.pTriAPI->SpriteTexture(p->m_pTexture, p->m_iFrame))
				{
					// We draw drips the way they are always rotated perpendicular to vector between particle and camera
					delta = (p->m_vPos - g_vecViewOrigin).Normalize();
					v_right = CrossProduct(delta, m_vDirection).Normalize();
					//float adiff = AngleBetweenVectors(g_vecViewForward, delta);
					//AngleVectors(m_vecAngles + Vector(0.0f,0.0f,adiff), NULL, v_right, v_up);
					if (m_iFlags & RENDERSYSTEM_FLAG_ZROTATION)
					{
						uy = v_up * p->m_fSizeY;
					}
					else// rotate
					{
//XDM3037a: don't modify v_up!						v_up = CrossProduct(delta, v_right).Normalize();
						uy = CrossProduct(delta, v_right).Normalize() * p->m_fSizeY;
					}
					rx = v_right * p->m_fSizeX;
//XDM3037a					uy = v_up * p->m_fSizeY;
/* TEST
					gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
					gEngfuncs.pTriAPI->CullFace(TRI_NONE);
					gEngfuncs.pTriAPI->Begin(TRI_LINES);
					gEngfuncs.pTriAPI->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
					gEngfuncs.pTriAPI->Brightness(1.0f);
					gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
					gEngfuncs.pTriAPI->Vertex3fv(p->m_vPosPrev);
					gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
					gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos);
*/
					gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
					gEngfuncs.pTriAPI->CullFace(TRI_NONE);
					gEngfuncs.pTriAPI->Begin(TRI_QUADS);
					gEngfuncs.pTriAPI->Color4f(p->m_fColor[0], p->m_fColor[1], p->m_fColor[2], 255);//p->m_fColor[3]);
					gEngfuncs.pTriAPI->Brightness(p->m_fColor[3]);

					gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
					gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos + rx + uy);// XDM3037: was m_vPosPrev
					gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
					gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos + rx - uy);
					gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
					gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos - rx - uy);
					gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
					gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos - rx + uy);// was m_vPosPrev

					gEngfuncs.pTriAPI->End();
				}
				//p->Render(v_right, v_up, rendermode, true);
			}
		}
	}
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//-----------------------------------------------------------------------------
// Purpose: Set relative particle size (texture must be set!)
// Input  : &sizex sizey - X Y
//-----------------------------------------------------------------------------
/*OBSOLETE void CPSDrips::SetParticleSize(const float &sizex, const float &sizey)
{
	if (m_pTexture)
	{
		m_fSizeX = m_pTexture->maxs[1] - m_pTexture->mins[1];
		m_fSizeY = m_pTexture->maxs[2] - m_pTexture->mins[2];
		if (sizex > 0.0f)
			m_fSizeX *= sizex;
		if (sizey > 0.0f)
			m_fSizeY *= sizey;
	}
	else
	{
		m_fSizeX = sizex;
		m_fSizeY = sizey;
	}
}*/

/* TEST bool OnParticleUpdateWhirl(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime)
{
	float s, c;
//	float r = time*1.5f;
	SinCos(time + pParticle->index, &s, &c);// 4
// haha! A TORNADO!!	pParticle->m_vPos.x = pSystem->m_vecOrigin.x + 512 * s;
//	pParticle->m_vPos.y = pSystem->m_vecOrigin.y + 512 * c;

	float k = cl_test1->value;//4.0f;
	if (cl_test2->value == 0.0f)
	{
		pParticle->m_vVel.x = pSystem->m_vDirection.x + k * s;
		pParticle->m_vVel.y = pSystem->m_vDirection.y + k * c;
	}
	else if (cl_test2->value == 1.0f)
	{
		pParticle->m_vVel.x = pSystem->m_vDirection.x*RANDOM_FLOAT(pSystem->m_fParticleSpeedMin, pSystem->m_fParticleSpeedMax) + k * s;
		pParticle->m_vVel.y = pSystem->m_vDirection.y*RANDOM_FLOAT(pSystem->m_fParticleSpeedMin, pSystem->m_fParticleSpeedMax) + k * c;
	}
	if (cl_test2->value == 2.0f)// k=4 swarm of flies!
	{
		pParticle->m_vVel.x += k * s;
		pParticle->m_vVel.y += k * c;
	}
	else if (cl_test2->value == 3.0f)
	{
		pParticle->m_vVel.x = (pSystem->m_vDirection.x + k * s)*RANDOM_FLOAT(pSystem->m_fParticleSpeedMin, pSystem->m_fParticleSpeedMax);
		pParticle->m_vVel.y = (pSystem->m_vDirection.y + k * c)*RANDOM_FLOAT(pSystem->m_fParticleSpeedMin, pSystem->m_fParticleSpeedMax);
	}
//snow
//	float k = 2000;
//	pParticle->m_vAccel.x = s * k;
//	pParticle->m_vAccel.y = c * k;
//	pParticle->m_vAccel.z = -500;
	return true;
}*/
