#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSCustom.h"
#include "pm_shared.h"
#include "event_api.h"
#include "bsputil.h"

// Purpose: A real entindex to compare and ignore inside custom TraceLine
// Warning: MULTITHREAD KILLER!
static int g_CPSCustom_PhysEntIgnoreIndex = -1;// 
// Purpose: Filter callback for PlayerTraceEx
static int CPSCustom_PhysEntIgnore(physent_t *pe)
{
	if (pe->info == g_CPSCustom_PhysEntIgnoreIndex)
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Default constructor. USE THIS FOR SCRIPTS!
// Warning: All constructors must call ResetParameters()!
// WARNING: Special system that does not know m_iMaxParticles yet!!
//-----------------------------------------------------------------------------
CPSCustom::CPSCustom(void) : CPSCustom::BaseClass()
{
	DBG_RS_PRINT("CPSCustom()");
	m_iEmitterIndex = 0;
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Default particle system
// Warning: Do not allocate anything here! System must parse data beforehand!
// Input  : origin - 
//			attachment - 
//			timetolive - 0 means forever
//-----------------------------------------------------------------------------
CPSCustom::CPSCustom(uint32 maxParticles, const Vector &origin, int emitterindex, int attachment, struct model_s *pTexture, int rendermode, float timetolive)
{
	DBG_RS_PRINTF("CPSCustom(%g %g %g, %d, %d, \"%s\", %d, %g)\n", origin.x, origin.y, origin.z, emitterindex, attachment, pTexture?pTexture->name:"", rendermode, timetolive);
	ResetParameters();
	if (maxParticles == 0 || !InitTexture(pTexture))
	{
		m_RemoveNow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_iEmitterIndex = emitterindex;
	m_iFollowAttachment = attachment;
	m_iRenderMode = rendermode;

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CPSCustom::~CPSCustom(void)
{
	DBG_RS_PRINT("~CPSCustom()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CPSCustom::ResetAllParameters(void)
{
	DBG_RS_PRINT("ResetAllParameters()");
	CPSCustom::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CPSCustom::ResetParameters(void)
{
	DBG_RS_PRINT("ResetParameters()");
	// NEVER RESET! m_iEmitterIndex
	m_iParticleFlags = 0;
	m_ColorMax.SetBlack();
	m_vAccelMin.Clear();
	m_vAccelMax.Clear();
	m_vSinVel.Clear();
	m_vCosVel.Clear();
	m_vSinAccel.Clear();
	m_vCosAccel.Clear();
	m_iDefinedFlags = 0;
	m_pTextureHitLiquid = NULL;
	m_pTextureHitSurface = NULL;
	m_pSystemOnHitLiquid = NULL;
	m_pSystemOnHitSurface = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CPSCustom::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "ColorMax4b") == 0)
	{
		if (StringToColor(szValue, m_ColorMax))
		{
			m_iDefinedFlags |= PSCUSTOM_DEF_COLORMAX;
			return true;
		}
	}
	else if (strcmp(szKey, "iParticleFlags") == 0)
		m_iParticleFlags = strtoul(szValue, NULL, 10);
	else if (strcmp(szKey, "vAccelMin") == 0)
		return StringToVec(szValue, m_vAccelMin);
	else if (strcmp(szKey, "vAccelMax") == 0)
		return StringToVec(szValue, m_vAccelMax);
	else if (strcmp(szKey, "vSinVel") == 0)
		return StringToVec(szValue, m_vSinVel);
	else if (strcmp(szKey, "vCosVel") == 0)
		return StringToVec(szValue, m_vCosVel);
	else if (strcmp(szKey, "vSinAccel") == 0)
		return StringToVec(szValue, m_vSinAccel);
	else if (strcmp(szKey, "vCosAccel") == 0)
		return StringToVec(szValue, m_vCosAccel);
	else if (strcmp(szKey, "szTextureHitLiquid") == 0)
	{
		if (szValue && szValue[0] != '\0')
		{
			m_pTextureHitLiquid = (model_t *)IEngineStudio.Mod_ForName(szValue, 0);
			//m_pTextureHitLiquid = (model_t *)gEngfuncs.GetSpritePointer(SPR_Load(szValue));// client-only, no modelindex
			if (m_pTextureHitLiquid == NULL)
			{
				conprintf(0, " ERROR: %s %u has invalid %s \"%s\"!\n", GetClassName(), GetIndex(), szKey, szValue);
				//m_pTextureHitLiquid = g_pSpritePTracer;
			}
			else
			{
				if (m_pTextureHitLiquid->type != mod_sprite)
				{
					conprintf(0, " ERROR: %s %u: %s \"%s\" is not a sprite!\n", GetClassName(), GetIndex(), szKey, szValue);
					m_pTextureHitLiquid = NULL;
					return false;
				}
			}
		}
		else
			m_pTextureHitSurface = NULL;// unset
	}
	else if (strcmp(szKey, "szTextureHitSurface") == 0)
	{
		if (szValue && szValue[0] != '\0')
		{
			m_pTextureHitSurface = (model_t *)gEngfuncs.GetSpritePointer(SPR_Load(szValue));// client-only, no modelindex
			if (m_pTextureHitSurface == NULL)
			{
				conprintf(0, " ERROR: %s %u has invalid %s \"%s\"!\n", GetClassName(), GetIndex(), szKey, szValue);
				//m_pTextureHitSurface = g_pSpritePTracer;
			}
			else
			{
				if (m_pTextureHitSurface->type != mod_sprite)
				{
					conprintf(0, " ERROR: %s %u: %s \"%s\" is not a sprite!\n", GetClassName(), GetIndex(), szKey, szValue);
					m_pTextureHitSurface = NULL;
					return false;
				}
			}
		}
		else
			m_pTextureHitSurface = NULL;// unset
	}
	else if (strcmp(szKey, "szSystemOnHitLiquid") == 0)
	{
		if (szValue && szValue[0] != '\0')
		{
			CRenderSystem *pSystem = g_pRenderManager->FindSystemByName(szValue);
			if (pSystem)
			{
				if (strcmp(pSystem->GetClassName(), "CPSCustom") == 0)
					m_pSystemOnHitLiquid = (CPSCustom *)pSystem;
				else
					conprintf(0, " Error: %s %u: %s system \"%s\" is of incompatible type (%s)!\n", GetClassName(), GetIndex(), szKey, szValue, pSystem->GetClassName());
			}
			else
				conprintf(0, " Error: %s %u: %s system \"%s\" not found!\n", GetClassName(), GetIndex(), szKey, szValue);
		}
	}
	else if (strcmp(szKey, "szSystemOnHitSurface") == 0)
	{
		if (szValue && szValue[0] != '\0')
		{
			CRenderSystem *pSystem = g_pRenderManager->FindSystemByName(szValue);
			if (pSystem)
			{
				if (strcmp(pSystem->GetClassName(), "CPSCustom") == 0)
					m_pSystemOnHitSurface = (CPSCustom *)pSystem;
				else
					conprintf(0, " Error: %s %u: %s system \"%s\" is of incompatible type (%s)!\n", GetClassName(), GetIndex(), szKey, szValue, pSystem->GetClassName());
			}
			else
				conprintf(0, " Error: %s %u: %s system \"%s\" not found!\n", GetClassName(), GetIndex(), szKey, szValue);
		}
	}
	else
		return CPSCustom::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize SYSTEM (non-user) startup variables.
// Note:    Must be called from class constructor (after ResetParameters).
// Warning: Be very careful overriding this function!!
//-----------------------------------------------------------------------------
/*void CPSCustom::InitializeSystem(void)
{
	CPSCustom::BaseClass::InitializeSystem();
	if (m_pParticleList && !IsRemoving())// succeeded
	{
		// add here
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: A particle has been created, initialize system-specific start values for it.
// Input  : pParticle - particle
//			fInterpolaitonMult - reverse progress [1...0] from previous origin to current, 0 also means disabled
//-----------------------------------------------------------------------------
void CPSCustom::InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult)
{
	CPSCustom::BaseClass::InitializeParticle(pParticle, fInterpolaitonMult);

	// ACCELERATION
	VectorRandom(pParticle->m_vAccel, m_vAccelMin, m_vAccelMax);

	// FLAGS
	if (FBitSet(m_iParticleFlags, PSCUSTOM_PFLAG_ORIENTED_START))
		pParticle->m_iFlags = PARTICLE_ORIENTED;
	else
		pParticle->m_iFlags = 0;

	// COLOR
	if (FBitSet(m_iDefinedFlags, PSCUSTOM_DEF_COLORMAX))
	{
		// We may have range from (0, 255, 0, 255) to (0, 0, 255, 127)! RANDOM_FLOAT
		pParticle->m_fColor[0] = RANDOM_FLOAT(m_fColorCurrent[0], (float)m_ColorMax[0]/255.0f);
		pParticle->m_fColor[1] = RANDOM_FLOAT(m_fColorCurrent[1], (float)m_ColorMax[1]/255.0f);
		pParticle->m_fColor[2] = RANDOM_FLOAT(m_fColorCurrent[2], (float)m_ColorMax[2]/255.0f);
		pParticle->m_fColor[3] = RANDOM_FLOAT(m_fColorCurrent[3], (float)m_ColorMax[3]/255.0f);
		/*pParticle->m_fColor[0] = (float)(RANDOM_LONG(m_color.r, m_ColorMax.r))/255.0f;
		pParticle->m_fColor[1] = (float)(RANDOM_LONG(m_color.g, m_ColorMax.g))/255.0f;
		pParticle->m_fColor[2] = (float)(RANDOM_LONG(m_color.b, m_ColorMax.b))/255.0f;
		pParticle->m_fColor[3] = RANDOM_FLOAT(m_fBrightness, m_ColorMax.a/255.0f);*/
	}
	//else should already be set by parent class
		//pParticle->SetColor(m_fColorCurrent);//pParticle->SetColor(m_color, m_fBrightness);
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
// Warning: DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CPSCustom::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		ShutdownSystem();

	if (m_iNumParticles == 0 && IsShuttingDown())
		return 1;
	if (!CheckFxLevel())// XDM3038c
		return 0;

	// WARNING! If you call CRenderSystem::Update(), all starting parameters for particles will be ruined! (origin. velocity, color, brightness, etc.)
	FollowEntity();

	if (IsRemoving())// only in emergency
		return 1;
	if (GetState() != RSSTATE_ENABLED && m_iNumParticles == 0)// XDM3038b: optimization: don't iterate
		return 0;

	uint32 iLastFrame = m_iFrame;
	UpdateFrame(time, elapsedTime);// update system's frame according to desired frame rate, required to detect proper time to update frames in particles
	// Update child systems so they have valid frames and colors
	if (m_pSystemOnHitLiquid)
		m_pSystemOnHitLiquid->UpdateFrame(time, elapsedTime);
	if (m_pSystemOnHitSurface)
		m_pSystemOnHitSurface->UpdateFrame(time, elapsedTime);

	if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_UPDATEOUTSIDEPVS))// XDM3038c: updates are usually very heavy because of TraceLines
	{
		if (m_iStartType == PSSTARTTYPE_POINT || m_iStartType == PSSTARTTYPE_SPHERE)
		{
			if (!Mod_CheckBoxInPVS(m_vecOrigin, m_vecOrigin))// Bad check: any obstruction hides the effect! if (!PointIsVisible(m_vecOrigin))
				return 0;
		}
		else if (m_iStartType == PSSTARTTYPE_BOX || m_iStartType == PSSTARTTYPE_LINE || m_iStartType == PSSTARTTYPE_ENTITYBBOX || m_iStartType == PSSTARTTYPE_CYLINDER)
		{
			if (!Mod_CheckBoxInPVS(m_vecOrigin+m_vStartMins, m_vecOrigin+m_vStartMaxs))
				return 0;
		}
		else if (m_iStartType == PSSTARTTYPE_CYLINDER)// we set mins/maxs in InitializeSystem()
		{
			if (!Mod_CheckBoxInPVS(Vector(m_vecOrigin.x - m_fStartRadiusMax, m_vecOrigin.y - m_fStartRadiusMax, m_vecOrigin.z + m_vStartMins.z),
								Vector(m_vecOrigin.x + m_fStartRadiusMax, m_vecOrigin.y + m_fStartRadiusMax, m_vecOrigin.z + m_vStartMaxs.z)))
				return 0;
		}
	}
	Emit();
	pmtrace_t pmtrace;
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS) && !FBitSet(m_iFlags, RENDERSYSTEM_FLAG_NOCLIP))// XDM3038c
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, (m_pLastFollowedEntity == gHUD.m_pLocalPlayer)?false:true);// must be called before SetSolidPlayers()
		//gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);// TEST
		gEngfuncs.pEventAPI->EV_PushPMStates();
		if (m_pLastFollowedEntity)
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(m_pLastFollowedEntity->index-1);// actually "SetNONSolidPlayers", shouldn't be bad if provided index is not a player
		else
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);// index is NOT entindex! just array of players[MAX_PLAYERS];

		g_CPSCustom_PhysEntIgnoreIndex = m_pLastFollowedEntity?m_pLastFollowedEntity->index:-1;
	}

	CParticle *pCurPart;
	Vector vToDest, vEmitterToDest(m_vDestination); vEmitterToDest -= m_vecOrigin;
	int contents;
	float fPartSin, fPartCos;
	float fDistToDest, fDistEmitterToDest = vEmitterToDest.Length(); 
	bool bDoFrameUpdate;
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
		bDoFrameUpdate = true;
		// Do movement
		if (!FBitSet(pCurPart->m_iFlags, PARTICLE_FWATERCIRCLE|PARTICLE_FHITSURFACE))// normal flying particle
		{
			pCurPart->m_vPosPrev = pCurPart->m_vPos;
			if (m_iMovementType == PSMOVTYPE_TOWARDSPOINT)// check if following destination point
			{
				vToDest = m_vDestination; vToDest -= pCurPart->m_vPos;
				fDistToDest = vToDest.Length();
				if (fDistToDest < max(2.0f,m_fParticleCollisionSize))// less than 2 units
				{
					RemoveParticle(pCurPart);// XDM3037
					continue;
				}
				vToDest.SetLength(m_fParticleSpeedMax*fabs(1.0f-fDistToDest/fDistEmitterToDest));
				pCurPart->m_vVel += vToDest;
			}
			pCurPart->m_vPos.MultiplyAdd(elapsedTime, pCurPart->m_vVel);//pCurPart->m_vPos += elapsedTime*pCurPart->m_vVel;// + elapsedTime*m_vSinMax*fSysSin + elapsedTime*m_vCosMax*fSysCos;
			// check contents right after updating origin, because we may not need anything else
			contents = gEngfuncs.PM_PointContents(pCurPart->m_vPos, NULL);
			if (contents == CONTENTS_SOLID && FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))// XDM3038c: hack?
			{
				RemoveParticle(pCurPart);// XDM3037
				continue;
			}
			if (m_iDestroyContents != 0 && ContentsArrayHas(m_iDestroyContents, contents))
			{
				RemoveParticle(pCurPart);
				continue;
			}
			// Check for water
			if (contents < CONTENTS_SOLID && contents > CONTENTS_SKY)// || pmtrace.inwater
			{
				if (m_pSystemOnHitLiquid != NULL)// re-initialize particle woth parameters from another system
				{
					ASSERT(!FBitSet(pCurPart->m_iFlags, PARTICLE_FWATERCIRCLE));
					//TransferParticleTo(m_pSystemOnHitLiquid);
					m_pSystemOnHitLiquid->m_vecOrigin = pCurPart->m_vPos;// required so the particle may continue
					m_pSystemOnHitLiquid->m_vecOrigin -= (pCurPart->m_vPos - pCurPart->m_vPosPrev)*2.0f;
					m_pSystemOnHitLiquid->InitializeParticle(pCurPart, 0.0f);
					pCurPart->m_iFlags |= PARTICLE_FWATERCIRCLE;
					if (FBitSet(m_iParticleFlags, PSCUSTOM_PFLAG_ORIENTED_HIT_LIQUID))
					{
						pCurPart->m_vPosPrev = pCurPart->m_vPos;
						pCurPart->m_vPosPrev.z += 1.0f;// hack to make it horizontal
						pCurPart->m_iFlags |= PARTICLE_ORIENTED;
					}
					if (m_pSystemOnHitLiquid->m_fBounceFactor > 0.0f)// reflect velocity, bounce
					{
						vec_t p = DotProduct(pCurPart->m_vVel, pmtrace.plane.normal);
						pCurPart->m_vVel -= (2.0f*p)*pmtrace.plane.normal;
						pCurPart->m_vVel *= m_pSystemOnHitLiquid->m_fBounceFactor;
					}
					else if (m_pSystemOnHitLiquid->m_fBounceFactor == 0.0f)// stick
						pCurPart->m_vVel.Clear();
					else// slide
					{
						PM_ClipVelocity(pCurPart->m_vVel, pmtrace.plane.normal, pCurPart->m_vVel, 1.0f);
						pCurPart->m_vVel *= m_pSystemOnHitLiquid->m_fFriction;
					}
					//bDoFrameUpdate = false;
					continue;
				}
				else if (m_pTextureHitLiquid != NULL)// old method
				{
					ASSERT(!FBitSet(pCurPart->m_iFlags, PARTICLE_FWATERCIRCLE));
					pCurPart->m_pTexture = m_pTextureHitLiquid;//IEngineStudio.GetModelByIndex(m_iSplashTexture);
					pCurPart->m_fEnergy = m_fEnergyStart;// restart
					pCurPart->SetSizeFromTexture(m_fScale, m_fScale);// slow?
					pCurPart->m_fSizeDelta = WATERCIRCLE_SIZE_DELTA;
					pCurPart->m_iFlags |= PARTICLE_FWATERCIRCLE;
					pCurPart->m_iFrame = 0;
					pCurPart->m_vVel.Clear();
					if (FBitSet(m_iParticleFlags, PSCUSTOM_PFLAG_ORIENTED_HIT_LIQUID))
					{
						pCurPart->m_vPosPrev = pCurPart->m_vPos;
						pCurPart->m_vPosPrev.z += 1.0f;// hack to make it horizontal
						pCurPart->m_iFlags |= PARTICLE_ORIENTED;
					}
					//bDoFrameUpdate = false;
					continue;
				}
			}// water

			// Velocity and acceleration are only calculated for normal particles for now. Is this right?
			SinCos(time + pCurPart->index, &fPartSin, &fPartCos);
			pCurPart->m_vVel.x = m_vDirection.x*RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax) + m_vSinVel.x*fPartSin + m_vCosVel.x*fPartCos;
			pCurPart->m_vVel.y = m_vDirection.y*RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax) + m_vSinVel.y*fPartSin + m_vCosVel.y*fPartCos;
			pCurPart->m_vVel.z = m_vDirection.z*RANDOM_FLOAT(m_fParticleSpeedMin, m_fParticleSpeedMax) + m_vSinVel.z*fPartSin + m_vCosVel.z*fPartCos;

			pCurPart->m_vVel.MultiplyAdd(elapsedTime, pCurPart->m_vAccel);//pCurPart->m_vVel += elapsedTime*pCurPart->m_vAccel;
			pCurPart->m_vAccel.x += m_vSinAccel.x*fPartSin + m_vCosAccel.x*fPartCos;
			pCurPart->m_vAccel.y += m_vSinAccel.y*fPartSin + m_vCosAccel.y*fPartCos;
			pCurPart->m_vAccel.z += m_vSinAccel.z*fPartSin + m_vCosAccel.z*fPartCos;
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDGRAVITY))
				pCurPart->m_vVel.z -= elapsedTime*g_cl_gravity*m_fParticleWeight;

			// Check for collisions
			if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS))// don't apply physics to hit effects // && !(pCurPart->m_iFlags & (PARTICLE_FWATERCIRCLE|PARTICLE_FHITSURFACE))
			{
				// stuck gEngfuncs.pEventAPI->EV_PlayerTrace(pCurPart->m_vPosPrev, pCurPart->m_vPos + pCurPart->m_vVel.Normalize()*m_fParticleCollisionSize, m_iTraceFlags, ignore_pe, &pmtrace);
				if (m_fParticleCollisionSize > 0.0f)
					pmtrace = PM_PlayerTraceEx(pCurPart->m_vPosPrev, pCurPart->m_vPos + pCurPart->m_vVel.Normalize()*m_fParticleCollisionSize, m_iTraceFlags, HULL_POINT, CPSCustom_PhysEntIgnore);
				else
					pmtrace = PM_PlayerTraceEx(pCurPart->m_vPosPrev, pCurPart->m_vPos, m_iTraceFlags, HULL_POINT, CPSCustom_PhysEntIgnore);

				if (pmtrace.fraction != 1.0f)// hit surface
				{
					if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_CLIPREMOVE))
					{
						RemoveParticle(pCurPart);
						continue;// XDM3037
					}
					else if (m_pSystemOnHitSurface != NULL)// re-initialize particle woth parameters from another system
					{
						ASSERT(!FBitSet(pCurPart->m_iFlags, PARTICLE_FHITSURFACE));
						//TransferParticleTo(m_pSystemOnHitSurface);
						m_pSystemOnHitSurface->m_vecOrigin = pCurPart->m_vPos;// required so the particle may continue
						m_pSystemOnHitSurface->m_vecOrigin -= (pCurPart->m_vPos - pCurPart->m_vPosPrev)*2.0f;
						m_pSystemOnHitSurface->InitializeParticle(pCurPart, 0.0f);
						pCurPart->m_iFlags |= PARTICLE_FHITSURFACE;
						if (FBitSet(m_iParticleFlags, PSCUSTOM_PFLAG_ORIENTED_HIT_SURFACE))
							pCurPart->m_iFlags |= PARTICLE_ORIENTED;

						if (m_pSystemOnHitSurface->m_fBounceFactor > 0.0f)// reflect velocity, bounce
						{
							vec_t p = DotProduct(pCurPart->m_vVel, pmtrace.plane.normal);
							pCurPart->m_vVel -= (2.0f*p)*pmtrace.plane.normal;
							pCurPart->m_vVel *= m_pSystemOnHitSurface->m_fBounceFactor;
						}
						else if (m_pSystemOnHitSurface->m_fBounceFactor == 0.0f)// stick
							pCurPart->m_vVel.Clear();
						else// slide
						{
							PM_ClipVelocity(pCurPart->m_vVel, pmtrace.plane.normal, pCurPart->m_vVel, 1.0f);
							pCurPart->m_vVel *= m_pSystemOnHitSurface->m_fFriction;
						}
						//bDoFrameUpdate = false;
						continue;
					}
					else if (m_pTextureHitSurface)
					{
						ASSERT(!FBitSet(pCurPart->m_iFlags, PARTICLE_FHITSURFACE));
						//pCurPart->m_fSizeDelta = 0.0f;?
						pCurPart->m_fEnergy = 1.0f;
						pCurPart->m_iFlags |= PARTICLE_FHITSURFACE;
						if (FBitSet(m_iParticleFlags, PSCUSTOM_PFLAG_ORIENTED_HIT_SURFACE))
							pCurPart->m_iFlags |= PARTICLE_ORIENTED;

						pCurPart->m_iFrame = 0;// restart
						pCurPart->m_vVel.Clear();
						pCurPart->m_vPos -= (pCurPart->m_vPos - pCurPart->m_vPosPrev)*2.0f;// revert back a little // But we need valid delta! m_vPos must NOT be equal to m_vPosPrev!
						pCurPart->m_pTexture = m_pTextureHitSurface;
						pCurPart->SetSizeFromTexture(m_fScale, m_fScale);
						//bDoFrameUpdate = false;
						continue;
					}
					else if (FBitSet(m_iParticleFlags, PSCUSTOM_PFLAG_HIT_FADEOUT))// stay and fade out
					{
						pCurPart->m_iFlags |= PARTICLE_FHITSURFACE;
						pCurPart->m_vVel.Clear();
						if (pCurPart->m_fColorDelta[3] >= 0.0f)// keep old fade out rate if it was set
							pCurPart->m_fColorDelta[3] = -1.0f;
					}
					else if (m_OnParticleCollide == NULL || m_OnParticleCollide(this, pCurPart, m_pOnParticleCollideData, &pmtrace) == true)// custom callback can disallow further frocessing
					{
						if (m_fBounceFactor > 0.0f)// reflect velocity, bounce
						{
							pCurPart->m_vVel.MirrorByVector(pmtrace.plane.normal);
							pCurPart->m_vVel *= m_fBounceFactor;
						}
						else if (m_fBounceFactor == 0.0f)// stick
							pCurPart->m_vVel.Clear();
						else// slide
						{
							PM_ClipVelocity(pCurPart->m_vVel, pmtrace.plane.normal, pCurPart->m_vVel, 1.0f);
							vec_t fSpeed = pCurPart->m_vVel.NormalizeSelf();
							fSpeed -= (float)(m_fFriction*elapsedTime);// friction is applied constantly every frame, so make it time-based
							pCurPart->m_vVel *= fSpeed;
						}
					}
				}
				//already included in acceleration	else
				//	pCurPart->m_vPos.z -= elapsedTime * g_cl_gravity;
			}// RENDERSYSTEM_FLAG_ADDPHYSICS
		}// normal flying particle
		else if (FBitSet(pCurPart->m_iFlags, PARTICLE_FHITSURFACE) && m_pSystemOnHitSurface)
		{
			if (m_pSystemOnHitSurface->m_iDestroyContents != 0 && m_pSystemOnHitSurface->ContentsArrayHas(m_pSystemOnHitSurface->m_iDestroyContents, contents))
			{
				RemoveParticle(pCurPart);// consider: a pushable appears!
				continue;
			}
			if (m_pSystemOnHitSurface->m_fFrameAccumulator >= 1.0f)
			{
				if (FBitSet(m_pSystemOnHitSurface->m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
					pCurPart->FrameRandomize();
				else if (pCurPart->FrameIncrease())
				{
					if (!FBitSet(m_pSystemOnHitSurface->m_iFlags, RENDERSYSTEM_FLAG_LOOPFRAMES))
						pCurPart->m_fEnergy = 0.0f;// remove (expire) on last frame
				}
			}
			bDoFrameUpdate = false;
		}
		else if (FBitSet(pCurPart->m_iFlags, PARTICLE_FWATERCIRCLE) && m_pSystemOnHitLiquid)
		{
			if (m_pSystemOnHitLiquid->m_iDestroyContents != 0 && m_pSystemOnHitLiquid->ContentsArrayHas(m_pSystemOnHitLiquid->m_iDestroyContents, contents))
			{
				RemoveParticle(pCurPart);// consider: particle became PARTICLE_FWATERCIRCLE, but water has been drained?
				continue;
			}
			if (m_pSystemOnHitLiquid->m_fFrameAccumulator >= 1.0f)
			{
				if (FBitSet(m_pSystemOnHitLiquid->m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
					pCurPart->FrameRandomize();
				else if (pCurPart->FrameIncrease())
				{
					if (!FBitSet(m_pSystemOnHitLiquid->m_iFlags, RENDERSYSTEM_FLAG_LOOPFRAMES))
						pCurPart->m_fEnergy = 0.0f;// remove (expire) on last frame
				}
			}
			bDoFrameUpdate = false;
		}

		if (!pCurPart->IsExpired())
		{
			if (m_OnParticleUpdate == NULL || m_OnParticleUpdate(this, pCurPart, m_pOnParticleUpdateData, time, elapsedTime) == true)
			{
				// FRAME
				if (bDoFrameUpdate && iLastFrame != m_iFrame)// Now update particle's frame. Note that different particles have different textures and different frames!
				{
					if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_RANDOMFRAME))
					{
						pCurPart->FrameRandomize();
					}
					else if (m_fFrameRate != 0)
					{
						if (pCurPart->FrameIncrease())// don't copy system frame
						{
							if (!FBitSet(m_iFlags, RENDERSYSTEM_FLAG_LOOPFRAMES))
								pCurPart->m_fEnergy = 0.0f;// remove (expire) on last frame
						}
					}
				}
				// COLOR
				pCurPart->UpdateColor(elapsedTime);
				pCurPart->UpdateSize(elapsedTime);
				pCurPart->UpdateEnergyByBrightness();
			}
		}
	}
	if (FBitSet(m_iFlags, RENDERSYSTEM_FLAG_ADDPHYSICS))
		gEngfuncs.pEventAPI->EV_PopPMStates();

	if (m_iNumParticles == 0 && m_iMaxParticles > 0 && IsShuttingDown())// m_iMaxParticles == 0 means this system is virtual or disabled
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Notifies us that some system was removed, a chance to get rid of all references to it.
// Input  : *pSystem - removed, do not use its data!
//-----------------------------------------------------------------------------
void CPSCustom::OnSystemRemoved(const CRenderSystem *pSystem)
{
	if (m_pSystemOnHitLiquid == pSystem)
	{
		conprintf(0, "%s[%u]: m_pSystemOnHitLiquid was removed\n", GetClassName(), GetIndex());
		m_pSystemOnHitLiquid = NULL;
	}
	if (m_pSystemOnHitSurface == pSystem)
	{
		conprintf(0, "%s[%u]: m_pSystemOnHitSurface was removed\n", GetClassName(), GetIndex());
		m_pSystemOnHitSurface = NULL;
	}
}
