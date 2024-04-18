//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// studio_model.cpp
// routines for setting up to draw 3DStudio models

#include "hud.h"
#include "vector.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "activity.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "pm_shared.h"
#include "triangleapi.h"
//#include "studio_util.h"
#include "r_studioint.h"
#include "r_efx.h"
#include "StudioModelRenderer.h"

alight_t aLightFullBright;// = { 128, 192, vec3_255, g_vecZero };// XDM3035: replace plightvec with current model origin
alight_t aLightEntityHighlight;
Vector g_vecLight(0,0,0);// XDM3038: to protect the g_vecZero
Vector g_vecLightTmp(0,0,0);

// Global engine <-> studio model rendering code interface
engine_studio_api_t IEngineStudio;
extern float g_fEntityHighlightEffect;

#define BLEND_MIN	0
#define BLEND_MID	127
#define BLEND_MAX	255

/////////////////////
// Implementation of CStudioModelRenderer.h

/* TODO? #define LEGS_BONES_COUNT	8

// enumerate all the bones that used for gait animation
const char *legs_bones[] =
{
{ "Bip01" },
{ "Bip01 Pelvis" },
{ "Bip01 L Leg" },
{ "Bip01 L Leg1" },
{ "Bip01 L Foot" },
{ "Bip01 R Leg" },
{ "Bip01 R Leg1" },
{ "Bip01 R Foot" },
};*/

//-----------------------------------------------------------------------------
// Weird, weird HL code
//int iCurrent = (pev->body / pbodypart->base) % pbodypart->nummodels;
//pev->body = (pev->body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
//-----------------------------------------------------------------------------
/*int R_StudioBodyVariations(model_t *pModel)
{
	studiohdr_t	*pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(pModel);
	if (!pstudiohdr)
		return 0;

	int count = 1;
	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex);

	for (int i = 0; i < pstudiohdr->numbodyparts; i++)
		count *= pbodypart[i].nummodels;

	return count;
}*/

/*
pbodypart[i].
i	base	nummodels
0	def.1	2
1	1*2=2	3
2	2*3=6	1
3	6*1=6	1
4	6*1=6	1
*/
/*
// bodypart[i].base = bodypart[i-1].base * bodypart[i-1].nummodels;

pw_npc22f.mdl

groups:
i	name	base	nummodels
0	body	1		1
1	wings	1		3
2	detail	3		2

body	g0	g1	g2
0		0	0	0
1		0	1	0
2		0	2	0
3		0	0	1
4		0	1	1
5		0	2	1
*/
//-----------------------------------------------------------------------------
// Purpose: Calculate pev->body value to be set to draw an entity
// Warning: We assume that in each body group the last model is most detailed
// Input  : *pModel - model
//			iLOD - lower LOD index == higher detail level
// Output : curstate.body value to be used by the engine
//-----------------------------------------------------------------------------
int R_StudioBodyLOD(model_t *pModel, int iLOD)
{
	studiohdr_t	*pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(pModel);
	if (!pstudiohdr)
		return 0;

	int body = 0;
	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex);
	for (int i = 0; i < pstudiohdr->numbodyparts; i++)
		body += pbodypart[i].base * max(0, pbodypart[i].nummodels-1 - iLOD);/// UNDONE TODO TESTME BUGBUG FIXME this is just an assumption!!

//	int iCurrent = (pev->body / pbodypart[i].base) % pbodypart[i].nummodels;
//	pev->body = (pev->body - (iCurrent * pbodypart[i].base) + (iValue * pbodypart[i].base));
//	pev->body = pbodypart[i].base * bodyvariant;

	/*
	variants = n1 * n2 * n3 = 6
	0 += 1 + 0
	1 += 1 + 2
	4 += 3 + 1
	= 8
	*/
	/*lod0
	0 += 1 * 0
	0 += 1 * 2
	2 += 3 * 1
	 = 5

	lod1
	0 += 1 * 0
	0 += 1 * 1
	1 += 3 * 1
	4
	*/
	return body;
}

/*
====================
Init

====================
*/
void CStudioModelRenderer::Init( void )
{
	aLightFullBright.ambientlight = 128;
	aLightFullBright.shadelight = 192;
	aLightFullBright.color[0] = 1;
	aLightFullBright.color[1] = 1;
	aLightFullBright.color[2] = 1;
	aLightFullBright.plightvec = (float *)&g_vecLight;

	aLightEntityHighlight.ambientlight = 64;
	aLightEntityHighlight.shadelight = 96;
	aLightEntityHighlight.color[0] = 1;
	aLightEntityHighlight.color[1] = 1;
	aLightEntityHighlight.color[2] = 1;
	aLightEntityHighlight.plightvec = (float *)&g_vecLight;

	// Set up some variables shared with engine
	m_pCvarHiModels			= IEngineStudio.GetCvar("cl_himodels");
	ASSERT(m_pCvarHiModels != NULL);
	m_pCvarDrawEntities		= IEngineStudio.GetCvar("r_drawentities");
	ASSERT(m_pCvarDrawEntities != NULL);
	m_pChromeSprite			= IEngineStudio.GetChromeSprite();
	ASSERT(m_pChromeSprite != NULL);
	IEngineStudio.GetModelCounters(&m_pStudioModelCount, &m_pModelsDrawn);

	// Get pointers to engine data structures
	m_pbonetransform		= (float (*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetBoneTransform();
	m_plighttransform		= (float (*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetLightTransform();
	m_paliastransform		= (float (*)[3][4])IEngineStudio.StudioGetAliasTransform();
	m_protationmatrix		= (float (*)[3][4])IEngineStudio.StudioGetRotationMatrix();
}

/*
====================
CStudioModelRenderer

====================
*/
CStudioModelRenderer::CStudioModelRenderer( void )
{
	m_fDoInterp			= 1;
	m_fGaitEstimation	= 1;
	m_pCurrentEntity	= NULL;
//	m_pCvarHiModels		= NULL;
	m_pCvarDrawEntities	= NULL;
	m_pChromeSprite		= NULL;
	m_pStudioModelCount	= NULL;
	m_pModelsDrawn		= NULL;
	m_protationmatrix	= NULL;
	m_paliastransform	= NULL;
	m_pbonetransform	= NULL;
	m_plighttransform	= NULL;
	m_pStudioHeader		= NULL;
	m_pBodyPart			= NULL;
	m_pSubModel			= NULL;
	m_pPlayerInfo		= NULL;
	m_pRenderModel		= NULL;
}

/*
====================
~CStudioModelRenderer

====================
*/
CStudioModelRenderer::~CStudioModelRenderer( void )
{
}

/*
====================
StudioCalcBoneAdj

====================
*/
void CStudioModelRenderer::StudioCalcBoneAdj( float dadt, float *adj, const byte *pcontroller1, const byte *pcontroller2, byte mouthopen )
{
	int					i, j;
	float				value;
	mstudiobonecontroller_t *pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bonecontrollerindex);

	for (j = 0; j < m_pStudioHeader->numbonecontrollers; ++j)
	{
		i = pbonecontroller[j].index;
		if (i <= 3)
		{
			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				if (abs(pcontroller1[i] - pcontroller2[i]) > 128)
				{
					int a, b;
					a = (pcontroller1[j] + 128) % 256;
					b = (pcontroller2[j] + 128) % 256;
					value = ((a * dadt) + (b * (1 - dadt)) - 128) * (360.0/256.0) + pbonecontroller[j].start;
				}
				else 
				{
					value = ((pcontroller1[i] * dadt + (pcontroller2[i]) * (1.0 - dadt))) * (360.0/256.0) + pbonecontroller[j].start;
				}
			}
			else 
			{
				value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0 - dadt)) / 255.0;
				if (value < 0) value = 0;
				else if (value > 1.0) value = 1.0;
				value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
			// Con_DPrintf( "%d %d %f : %f\n", m_pCurrentEntity->curstate.controller[j], m_pCurrentEntity->latched.prevcontroller[j], value, dadt );
		}
		else
		{
			value = mouthopen / 64.0;
			if (value > 1.0) value = 1.0;				
			value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			// Con_DPrintf("%d %f\n", mouthopen, value );
		}
		switch(pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			adj[j] = value * (M_PI / 180.0);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			adj[j] = value;
			break;
		}
	}
}


/*
====================
StudioCalcBoneQuaterion

====================
*/
void CStudioModelRenderer::StudioCalcBoneQuaterion( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q )
{
	int					j, k;
	vec4_t				q1, q2;
	vec3_t				angle1, angle2;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j+3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}

		if (pbone->bonecontroller[j+3] != -1)
		{
			angle1[j] += adj[pbone->bonecontroller[j+3]];
			angle2[j] += adj[pbone->bonecontroller[j+3]];
		}
	}

//	if (!VectorCompare( angle1, angle2 ))
	if (angle1 != angle2)
	{
		AngleQuaternion( angle1, q1 );
		AngleQuaternion( angle2, q2 );
		QuaternionSlerp( q1, q2, s, q );
	}
	else
	{
		AngleQuaternion( angle1, q );
	}
}

/*
====================
StudioCalcBonePosition

====================
*/
void CStudioModelRenderer::StudioCalcBonePosition( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos )
{
	int					j, k;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			/*
			if (i == 0 && j == 0)
				Con_DPrintf("%d  %d:%d  %f\n", frame, panimvalue->num.valid, panimvalue->num.total, s );
			*/
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
  				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0f - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if ( pbone->bonecontroller[j] != -1 && adj )
		{
			pos[j] += adj[pbone->bonecontroller[j]];
		}
	}
}

/*
====================
StudioSlerpBones

====================
*/
//void CStudioModelRenderer::StudioSlerpBones(Vector4D q1[], float pos1[][3], Vector4D q2[], float pos2[][3], float s)
void CStudioModelRenderer::StudioSlerpBones( vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s )
{
	int			i;
	vec4_t		q3;
	float		s1;

	if (s < 0) s = 0;
	else if (s > 1.0) s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < m_pStudioHeader->numbones; ++i)
	{
		QuaternionSlerp( q1[i], q2[i], s, q3 );
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}

/*
====================
StudioGetAnim

====================
*/
mstudioanim_t *CStudioModelRenderer::StudioGetAnim( model_t *m_pSubModel, mstudioseqdesc_t *pseqdesc )
{
	mstudioseqgroup_t *pseqgroup = (mstudioseqgroup_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqgroupindex) + pseqdesc->seqgroup;
	if (pseqdesc->seqgroup == 0)
	{
		return (mstudioanim_t *)((byte *)m_pStudioHeader + pseqdesc->animindex);// HL20130901
	}

	cache_user_t *paSequences = (cache_user_t *)m_pSubModel->submodels;
	if (paSequences == NULL)
	{
		gEngfuncs.Con_DPrintf("CL: allocating dangerous block!\n");// XDM3037a
		paSequences = (cache_user_t *)IEngineStudio.Mem_Calloc( 16, sizeof( cache_user_t ) ); // UNDONE: leak!
		m_pSubModel->submodels = (dmodel_t *)paSequences;
	}

	if (!IEngineStudio.Cache_Check( (struct cache_user_s *)&(paSequences[pseqdesc->seqgroup])))
	{
		// TODO? G-Cont suggests this against lazy users who make corrupted models
		//Q_snprintf( filepath, sizeof( filepath ), "%s/%s%i%i.mdl", modelpath, modelname, pseqdesc->seqgroup / 10, pseqdesc->seqgroup % 10 );
		gEngfuncs.Con_DPrintf("Loading sequence group %s for model %s\n", pseqgroup->name, m_pSubModel->name);// XDM3037a: more informative
		IEngineStudio.LoadCacheFile( pseqgroup->name, (struct cache_user_s *)&paSequences[pseqdesc->seqgroup] );
	}
	return (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}

/*
====================
StudioPlayerBlend

BUGBUG: this works wrong for local player!
Warning: blendstart / blendend are in degrees, commonly used as blend XR -45 45
		pBlend is: 0 - looking down, 255 - up
		pPitch is IN DEGREES!!!
Warning: modifies pPitch!
Note : can only be tested on player aim animations (which have blendings)
====================
*/
void CStudioModelRenderer::StudioPlayerBlend( mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch )
{
	// calc up/down pointing
	*pBlend = (int)(*pPitch);// XDM3038c
	if (*pBlend < pseqdesc->blendstart[0])
	{
		*pPitch -= pseqdesc->blendstart[0];// let model rotation compensate the narrow range of blendings (-90...blendstart...0...blendend...90)
		*pBlend = BLEND_MIN;
	}
	else if (*pBlend > pseqdesc->blendend[0])
	{
		*pPitch -= pseqdesc->blendend[0];
		*pBlend = BLEND_MAX;
	}
	else
	{
		if (pseqdesc->blendend[0] - pseqdesc->blendstart[0] < 0.1f)// catch qc error
			*pBlend = BLEND_MID;
		else
			*pBlend = (int)(255.0f * (*pBlend - pseqdesc->blendstart[0]) / (pseqdesc->blendend[0] - pseqdesc->blendstart[0]));// XDM3035c: float

		*pPitch = 0.0f;
	}
}

/*
====================
StudioSetUpTransform

====================
*/
void CStudioModelRenderer::StudioSetUpTransform (int trivial_accept)
{
	int		i;
	vec3_t	angles(m_pCurrentEntity->curstate.angles);// This affects overall model rotation, but not blendings!
	vec3_t	modelpos(m_pCurrentEntity->origin);

	// TODO: should really be stored with the entity instead of being reconstructed
	// TODO: should use a look-up table
	// TODO: could cache lazily, stored in the entity

	//Con_DPrintf("Angles %4.2f prev %4.2f for %i\n", angles[PITCH], m_pCurrentEntity->index);
	//Con_DPrintf("movetype %d %d\n", m_pCurrentEntity->movetype, m_pCurrentEntity->aiment );
	if (m_pCurrentEntity->curstate.movetype == MOVETYPE_STEP) 
	{
		float f;
		// don't do it if the goalstarttime hasn't updated in a while.

		// NOTE: Because we need to interpolate multiplayer characters, the interpolation time limit was increased to 1.0 s., which is 2x the max lag we are accounting for.
		if ((m_clTime < m_pCurrentEntity->curstate.animtime + 1.0f) && m_pCurrentEntity->curstate.animtime != m_pCurrentEntity->latched.prevanimtime)
		{
			f = (m_clTime - m_pCurrentEntity->curstate.animtime) / (m_pCurrentEntity->curstate.animtime - m_pCurrentEntity->latched.prevanimtime);
			//Con_DPrintf("%4.2f %.2f %.2f\n", f, m_pCurrentEntity->curstate.animtime, m_clTime);
		}
		else
			f = 0;

		if (m_fDoInterp)// ugly hack to interpolate angle, position. current is reached 0.1 seconds after being set
			f = f - 1.0f;
		else
			f = 0;

		//acess to studio flags
		//mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

		// XDM: interpolation fails on trains
		//if (pseqdesc && (pseqdesc->motiontype & STUDIO_LX))//enable interpolation only for walk\run
		{
			// AMD Optimization Guide 40546
			modelpos[0] += (m_pCurrentEntity->origin[0] - m_pCurrentEntity->latched.prevorigin[0]) * f;
			modelpos[1] += (m_pCurrentEntity->origin[1] - m_pCurrentEntity->latched.prevorigin[1]) * f;
			modelpos[2] += (m_pCurrentEntity->origin[2] - m_pCurrentEntity->latched.prevorigin[2]) * f;
		}

		// NOTE:  Because multiplayer lag can be relatively large, we don't want to cap f at 1.5 anymore.
		//if (f > -1.0 && f < 1.5) {}

		//Con_DPrintf("%.0f %.0f\n",m_pCurrentEntity->msg_angles[0][YAW], m_pCurrentEntity->msg_angles[1][YAW] );
		// InterpolateAngles(m_pCurrentEntity->angles, m_pCurrentEntity->latched.prevangles, angles, f);
		vec_t ang1, ang2, d;
		for (i = 0; i < 3; ++i)
		{
			ang1 = m_pCurrentEntity->angles[i];
			ang2 = m_pCurrentEntity->latched.prevangles[i];
			d = ang1 - ang2;
			NormalizeAngle180(&d);
			angles[i] += d * f;
		}
		//Con_DPrintf("%.3f \n", f );
	}
	else if (m_pCurrentEntity->curstate.movetype != MOVETYPE_NONE)
		angles = m_pCurrentEntity->angles;// curstate.angles is bad

	//Con_DPrintf("%.0f %0.f %0.f\n%.0f %0.f %0.f\n", modelpos[0], modelpos[1], modelpos[2], angles[0], angles[1], angles[2]);
	DBG_ANGLES_DRAW(10, m_pCurrentEntity->origin, angles, m_pCurrentEntity->index, "StudioSetUpTransform()");

#if defined (CORRECT_PITCH)
	//if (m_pCurrentEntity == NULL || gHUD.m_pLocalPlayer == NULL || m_pCurrentEntity->index != gHUD.m_pLocalPlayer->index)// XDM3038c: took years to find out how this shit gets corrupted in the engine
	//	angles[PITCH] = -angles[PITCH];
#else
	angles[PITCH] = -angles[PITCH];
#endif

	AngleMatrix(g_vecZero, angles, (*m_protationmatrix));

	if ( !IEngineStudio.IsHardware() )
	{
		static float viewmatrix[3][4];

		VectorCopy (m_vRight, viewmatrix[0]);
		VectorCopy (m_vUp, viewmatrix[1]);
		VectorInverse (viewmatrix[1]);
		VectorCopy (m_vNormal, viewmatrix[2]);

		(*m_protationmatrix)[0][3] = modelpos[0] - m_vRenderOrigin[0];
		(*m_protationmatrix)[1][3] = modelpos[1] - m_vRenderOrigin[1];
		(*m_protationmatrix)[2][3] = modelpos[2] - m_vRenderOrigin[2];

		ConcatTransforms (viewmatrix, (*m_protationmatrix), (*m_paliastransform));

		// do the scaling up of x and y to screen coordinates as part of the transform
		// for the unclipped case (it would mess up clipping in the clipped case).
		// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
		// correspondingly so the projected x and y come out right
		// FIXME: make this work for clipped case too?
		if (trivial_accept)
		{
			for (i=0 ; i<4 ; ++i)
			{
				(*m_paliastransform)[0][i] *= m_fSoftwareXScale * (1.0f / (ZISCALE * 0x10000));
				(*m_paliastransform)[1][i] *= m_fSoftwareYScale * (1.0f / (ZISCALE * 0x10000));
				(*m_paliastransform)[2][i] *= 1.0f / (ZISCALE * 0x10000);
			}
		}
	}

	(*m_protationmatrix)[0][3] = modelpos[0];
	(*m_protationmatrix)[1][3] = modelpos[1];
	(*m_protationmatrix)[2][3] = modelpos[2];

	// XDM: update model scale factor
	if (m_pCurrentEntity->curstate.scale != 0.0f)// backward compatibility
	{
		// XDM3038a: AMD Optimization Guide 40546
		(*m_protationmatrix)[0][0] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[0][1] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[0][2] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[1][0] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[1][1] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[1][2] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[2][0] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[2][1] *= m_pCurrentEntity->curstate.scale;
		(*m_protationmatrix)[2][2] *= m_pCurrentEntity->curstate.scale;
	}
}


/*
====================
StudioEstimateInterpolant

====================
*/
float CStudioModelRenderer::StudioEstimateInterpolant( void )
{
	float dadt = 1.0;
	if (m_fDoInterp && (m_pCurrentEntity->curstate.animtime >= m_pCurrentEntity->latched.prevanimtime + 0.01f))
	{
		dadt = (m_clTime - m_pCurrentEntity->curstate.animtime) / 0.1f;
		if (dadt > 2.0f)
			dadt = 2.0f;
	}
	return dadt;
}

/*
====================
StudioCalcRotations

====================
*/
void CStudioModelRenderer::StudioCalcRotations ( float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f )
//void CStudioModelRenderer::StudioCalcRotations(float pos[][3], Vector4D q[], mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f)
{
	int					i;
	int					frame;
	float				s;
	float				adj[MAXSTUDIOCONTROLLERS];
	float				dadt;

	if (f > pseqdesc->numframes - 1)
		f = 0;	// bah, fix this bug with changing sequences too fast

	// BUG ( somewhere else ) but this code should validate this data.
	// This could cause a crash if the frame # is negative, so we'll go ahead and clamp it here
	else if (f < -0.01)
	{
		f = -0.01;
	}

	frame = (int)f;

	// Con_DPrintf("%d %.4f %.4f %.4f %.4f %d\n", m_pCurrentEntity->curstate.sequence, m_clTime, m_pCurrentEntity->animtime, m_pCurrentEntity->frame, f, frame );
	// Con_DPrintf( "%f %f %f\n", m_pCurrentEntity->angles[ROLL], m_pCurrentEntity->angles[PITCH], m_pCurrentEntity->angles[YAW] );
	// Con_DPrintf("frame %d %d\n", frame1, frame2 );

	dadt = StudioEstimateInterpolant();
	s = (f - frame);

	// add in programtic controllers
	mstudiobone_t *pbone = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	StudioCalcBoneAdj(dadt, adj, m_pCurrentEntity->curstate.controller, m_pCurrentEntity->latched.prevcontroller, m_pCurrentEntity->mouth.mouthopen);

	for (i = 0; i < m_pStudioHeader->numbones; ++i, ++pbone, ++panim)
	{
		StudioCalcBoneQuaterion( frame, s, pbone, panim, adj, q[i] );
		StudioCalcBonePosition( frame, s, pbone, panim, adj, pos[i] );
		// if (0 && i == 0)
		//	Con_DPrintf("%d %d %d %d\n", m_pCurrentEntity->curstate.sequence, frame, j, k );
	}

	if (pseqdesc->motiontype & STUDIO_X)
		pos[pseqdesc->motionbone][0] = 0.0;

	if (pseqdesc->motiontype & STUDIO_Y)
		pos[pseqdesc->motionbone][1] = 0.0;

	if (pseqdesc->motiontype & STUDIO_Z)
		pos[pseqdesc->motionbone][2] = 0.0;

	// XDM: unneeded trash? non-zero values make bad movement
	/*s = 0 * ((1.0 - (f - frame)) / (pseqdesc->numframes)) * m_pCurrentEntity->curstate.framerate;

	if (pseqdesc->motiontype & STUDIO_LX)
		pos[pseqdesc->motionbone][0] += s * pseqdesc->linearmovement[0];

	if (pseqdesc->motiontype & STUDIO_LY)
		pos[pseqdesc->motionbone][1] += s * pseqdesc->linearmovement[1];

	if (pseqdesc->motiontype & STUDIO_LZ)
		pos[pseqdesc->motionbone][2] += s * pseqdesc->linearmovement[2];
	*/
}

/*
====================
Studio_FxTransform

====================
*/
void CStudioModelRenderer::StudioFxTransform( cl_entity_t *ent, float transform[3][4] )
{
	switch( ent->curstate.renderfx )
	{
	case kRenderFxDistort:
	case kRenderFxHologram:
		if ( gEngfuncs.pfnRandomLong(0,49) == 0 )
		{
			int axis = gEngfuncs.pfnRandomLong(0,1);
			if (axis == 1) // Choose between x & z
				axis = 2;
			VectorScale(transform[axis], gEngfuncs.pfnRandomFloat(1,1.484), transform[axis]);
		}
		else if ( gEngfuncs.pfnRandomLong(0,49) == 0 )
		{
			int axis = gEngfuncs.pfnRandomLong(0,1);
			if (axis == 1) // Choose between x & z
				axis = 2;
			transform[gEngfuncs.pfnRandomLong(0,2)][3] += gEngfuncs.pfnRandomFloat(-10,10);
		}
	break;
	case kRenderFxExplode:
		{
			float scale = (float)(1.0 + (m_clTime - ent->curstate.animtime) * 10.0);
			if (scale > 3.0)	// Don't blow up more than 200%
				scale = 3.0;

			transform[0][1] *= scale;
			transform[1][1] *= scale;
			transform[2][1] *= scale;
		}
	break;
	}
}

/*
====================
StudioEstimateFrame

====================
*/
double CStudioModelRenderer::StudioEstimateFrame(mstudioseqdesc_t *pseqdesc)
{
	double dfdt, f;

	if (pseqdesc->numframes <= 1)
		f = 0;
	else
		f = (m_pCurrentEntity->curstate.frame * (pseqdesc->numframes - 1)) / 256.0;

	if (gHUD.m_iPaused || gHUD.m_iIntermission)// XDM3037: stops current frame
		return f;

	if (m_fDoInterp)
	{
		if (m_clTime < m_pCurrentEntity->curstate.animtime)
			dfdt = 0;
		else
			dfdt = (m_clTime - m_pCurrentEntity->curstate.animtime) * m_pCurrentEntity->curstate.framerate * pseqdesc->fps;
	}
	else
		dfdt = 0;

	f += dfdt;

	if (pseqdesc->flags & STUDIO_LOOPING) 
	{
		if (pseqdesc->numframes > 1)
			f -= (int)(f / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);

		if (f < 0.0) 
			f += (pseqdesc->numframes - 1);
	}
	else 
	{
		if (f >= pseqdesc->numframes - 1.001) 
			f = pseqdesc->numframes - 1.001;

		if (f < 0.0) 
			f = 0.0;
	}
	return f;
}

/*
====================
StudioSetupBones

====================
*/
void CStudioModelRenderer::StudioSetupBones ( void )
{
	int					i;
	double				f;

	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t		*panim;

	static float		pos[MAXSTUDIOBONES][3];
	static vec4_t		q[MAXSTUDIOBONES];
	float				bonematrix[3][4];

	static float		pos2[MAXSTUDIOBONES][3];
	static vec4_t		q2[MAXSTUDIOBONES];
	static float		pos3[MAXSTUDIOBONES][3];
	static vec4_t		q3[MAXSTUDIOBONES];
	static float		pos4[MAXSTUDIOBONES][3];
	static vec4_t		q4[MAXSTUDIOBONES];

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq) 
		m_pCurrentEntity->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	f = StudioEstimateFrame( pseqdesc );

	// always want new gait sequences to start on frame zero
	if (m_pCurrentEntity->player && m_pPlayerInfo)// XDM3038b
	{
		// new jump gaitsequence?  start from frame zero
		if (m_pPlayerInfo->gaitsequence == GAITSEQUENCE_DISABLED)// XDM3037: TESTME allow gaitsequence 0
		{
			m_pPlayerInfo->gaitframe = 0.0;
		}
		else if (m_pCurrentEntity->prevstate.gaitsequence != m_pPlayerInfo->gaitsequence)// XDM3037: much more elegant way
		{
			if (m_pPlayerInfo->gaitsequence == m_pCurrentEntity->curstate.sequence)
				m_pPlayerInfo->gaitframe = f;// synchronize with the main sequence
			else
				m_pPlayerInfo->gaitframe = 0.0;
//			gEngfuncs.Con_Printf( "Setting gaitframe to 0\n" );
		}
/* TEST ONLY fixes non-looping animations, but makes animaitons much slower because gaitsequences use different framerate calculation algorithm
		else if (m_pPlayerInfo->gaitsequence != m_pCurrentEntity->curstate.sequence)// stand-alone gaitsequence is being played
		{
			m_pPlayerInfo->gaitframe = StudioEstimateFrame((mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pPlayerInfo->gaitsequence);
		}*/
	}

	panim = StudioGetAnim( m_pRenderModel, pseqdesc );
	StudioCalcRotations( pos, q, pseqdesc, panim, f );

	if (pseqdesc->numblends > 1)
	{
		float				s;
		float				dadt;

		panim += m_pStudioHeader->numbones;
		StudioCalcRotations( pos2, q2, pseqdesc, panim, f );

		dadt = StudioEstimateInterpolant();
		s = (m_pCurrentEntity->curstate.blending[0] * dadt + m_pCurrentEntity->latched.prevblending[0] * (1.0f - dadt)) / 255.0f;

		StudioSlerpBones( q, pos, q2, pos2, s );

		if (pseqdesc->numblends == 4)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations( pos3, q3, pseqdesc, panim, f );

			panim += m_pStudioHeader->numbones;
			StudioCalcRotations( pos4, q4, pseqdesc, panim, f );

			s = (m_pCurrentEntity->curstate.blending[0] * dadt + m_pCurrentEntity->latched.prevblending[0] * (1.0f - dadt)) / 255.0f;
			StudioSlerpBones( q3, pos3, q4, pos4, s );

			s = (m_pCurrentEntity->curstate.blending[1] * dadt + m_pCurrentEntity->latched.prevblending[1] * (1.0f - dadt)) / 255.0f;
			StudioSlerpBones( q, pos, q3, pos3, s );
		}
	}
	
	if (m_fDoInterp &&
		m_pCurrentEntity->latched.sequencetime &&
		(m_pCurrentEntity->latched.sequencetime + 0.2 > m_clTime ) && 
		(m_pCurrentEntity->latched.prevsequence < m_pStudioHeader->numseq ))
	{
		// blend from last sequence
		static float		pos1b[MAXSTUDIOBONES][3];
		static vec4_t		q1b[MAXSTUDIOBONES];
		float				s;

		if (m_pCurrentEntity->latched.prevsequence >=  m_pStudioHeader->numseq) 
		{
			m_pCurrentEntity->latched.prevsequence = 0;
		}

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->latched.prevsequence;
		panim = StudioGetAnim( m_pRenderModel, pseqdesc );
		// clip prevframe
		StudioCalcRotations( pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

		if (pseqdesc->numblends > 1)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations( pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

			s = (m_pCurrentEntity->latched.prevseqblending[0]) / 255.0f;
			StudioSlerpBones( q1b, pos1b, q2, pos2, s );

			if (pseqdesc->numblends == 4)
			{
				panim += m_pStudioHeader->numbones;
				StudioCalcRotations( pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

				panim += m_pStudioHeader->numbones;
				StudioCalcRotations( pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

				s = (m_pCurrentEntity->latched.prevseqblending[0]) / 255.0f;
				StudioSlerpBones( q3, pos3, q4, pos4, s );

				s = (m_pCurrentEntity->latched.prevseqblending[1]) / 255.0f;
				StudioSlerpBones( q1b, pos1b, q3, pos3, s );
			}
		}

		s = 1.0 - (m_clTime - m_pCurrentEntity->latched.sequencetime) / 0.2;
		StudioSlerpBones( q, pos, q1b, pos1b, s );
	}
	else
	{
		//Con_DPrintf("prevframe = %4.2f\n", f);
		m_pCurrentEntity->latched.prevframe = f;
	}

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	// calc gait animation
	if (m_pCurrentEntity->player && m_pPlayerInfo && m_pPlayerInfo->gaitsequence != GAITSEQUENCE_DISABLED)// XDM3037: allow gaitsequence 0
	{
		// bounds checking
		if (m_pPlayerInfo->gaitsequence >= m_pStudioHeader->numseq) 
			m_pPlayerInfo->gaitsequence = 0;

		bool copy = 1;

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pPlayerInfo->gaitsequence;

		panim = StudioGetAnim( m_pRenderModel, pseqdesc );
		StudioCalcRotations( pos2, q2, pseqdesc, panim, m_pPlayerInfo->gaitframe );

		for (i = 0; i < m_pStudioHeader->numbones; ++i)
		{
			if ( !strcmp( pbones[i].name, "Bip01 Spine" ) )// HL20130901
				copy = 0;
			else if ( !strcmp( pbones[ pbones[i].parent ].name, "Bip01 Pelvis" ) )
				copy = 1;

			if ( copy )
			{
				memcpy( pos[i], pos2[i], sizeof( pos[i] ) );
				memcpy( q[i], q2[i], sizeof( q[i] ) );
			}
		}
	}

	for (i = 0; i < m_pStudioHeader->numbones; ++i) 
	{
		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) 
		{
			if ( IEngineStudio.IsHardware() )
			{
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
				// MatrixCopy should be faster...
				//ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
			}
			else
			{
				ConcatTransforms((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}

			// Apply client-side effects to the transformation matrix
			StudioFxTransform(m_pCurrentEntity, (*m_pbonetransform)[i]);
		} 
		else 
		{
			ConcatTransforms((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
			ConcatTransforms((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
		}
	}
}


/*
====================
StudioSaveBones

====================
*/
void CStudioModelRenderer::StudioSaveBones( void )
{
	int		i;

	mstudiobone_t		*pbones;
	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	m_nCachedBones = m_pStudioHeader->numbones;

	for (i = 0; i < m_pStudioHeader->numbones; i++) 
	{
		strcpy( m_nCachedBoneNames[i], pbones[i].name );
		MatrixCopy( (*m_pbonetransform)[i], m_rgCachedBoneTransform[i] );
		MatrixCopy( (*m_plighttransform)[i], m_rgCachedLightTransform[i] );
	}
}


/*
====================
StudioMergeBones

====================
*/
void CStudioModelRenderer::StudioMergeBones ( model_t *m_pSubModel )
{
	int					i, j;
	double				f;

	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t		*panim;

	static float		pos[MAXSTUDIOBONES][3];
	float				bonematrix[3][4];
	static vec4_t		q[MAXSTUDIOBONES];

	if (m_pCurrentEntity->curstate.sequence >=  m_pStudioHeader->numseq) 
	{
		m_pCurrentEntity->curstate.sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	f = StudioEstimateFrame( pseqdesc );

	//if (m_pCurrentEntity->latched.prevframe > f)
		//Con_DPrintf("%f %f\n", m_pCurrentEntity->prevframe, f );

	panim = StudioGetAnim( m_pSubModel, pseqdesc );
	StudioCalcRotations( pos, q, pseqdesc, panim, f );

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	for (i = 0; i < m_pStudioHeader->numbones; ++i) 
	{
		for (j = 0; j < m_nCachedBones; ++j)
		{
			if (_stricmp(pbones[i].name, m_nCachedBoneNames[j]) == 0)
			{
				MatrixCopy( m_rgCachedBoneTransform[j], (*m_pbonetransform)[i] );
				MatrixCopy( m_rgCachedLightTransform[j], (*m_plighttransform)[i] );
				break;
			}
		}
		if (j >= m_nCachedBones)
		{
			QuaternionMatrix( q[i], bonematrix );

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1) 
			{
				if ( IEngineStudio.IsHardware() )
				{
					ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
					// MatrixCopy should be faster...
					//ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
					MatrixCopy( (*m_pbonetransform)[i], (*m_plighttransform)[i] );
				}
				else
				{
					ConcatTransforms ((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
					ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				}

				// Apply client-side effects to the transformation matrix
				StudioFxTransform( m_pCurrentEntity, (*m_pbonetransform)[i] );
			} 
			else 
			{
				ConcatTransforms ((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms ((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
			}
		}
	}
}


/*
====================
StudioEstimateGait

====================
*/
void CStudioModelRenderer::StudioEstimateGait( entity_state_t *pplayer )
{
	ASSERT(m_pPlayerInfo != NULL);
	double dt = (m_clTime - m_clOldTime);
	if (dt < 0)
		dt = 0;
	else if (dt > 1.0)
		dt = 1;

	if (dt == 0 || m_pPlayerInfo->renderframe == m_nFrameCount)
	{
		m_flGaitMovement = 0;
		return;
	}

	vec3_t est_velocity;
	// VectorAdd( pplayer->velocity, pplayer->prediction_error, est_velocity );
	if ( m_fGaitEstimation )
	{
		VectorSubtract( m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin, est_velocity );
		VectorCopy( m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin );
		m_flGaitMovement = Length( est_velocity );
		if (dt <= 0 || m_flGaitMovement / dt < 5)
		{
			est_velocity[0] = 0;
			est_velocity[1] = 0;
			m_flGaitMovement = 0;
		}
	}
	else
	{
		VectorCopy(pplayer->velocity, est_velocity);
		m_flGaitMovement = Length(est_velocity) * dt;
	}

	if (est_velocity[1] == 0 && est_velocity[0] == 0)
	{
		vec_t flYawDiff = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360.0f) * 360.0f;
		NormalizeAngle180(&flYawDiff);

		if (dt < 0.25)
			flYawDiff *= dt * 4;
		else
			flYawDiff *= dt;

		m_pPlayerInfo->gaityaw += flYawDiff;
		m_pPlayerInfo->gaityaw -= (int)(m_pPlayerInfo->gaityaw / 360.0f) * 360.0f;
		m_flGaitMovement = 0;
	}
	else
	{
		m_pPlayerInfo->gaityaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);
		//NormalizeAngle180(&m_pPlayerInfo->gaityaw);// XDM3038a: TESTME?
		if (m_pPlayerInfo->gaityaw > 180.0f)
			m_pPlayerInfo->gaityaw = 180.0f;
		else if (m_pPlayerInfo->gaityaw < -180.0f)
			m_pPlayerInfo->gaityaw = -180.0f;
	}
}

/*
====================
StudioProcessGait
Warning: modifies angles!
====================
*/
void CStudioModelRenderer::StudioProcessGait( entity_state_t *pplayer )
{
	ASSERT(m_pPlayerInfo != NULL);
	int iBlend;
	vec_t flYaw;	 // view direction relative to movement

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq) 
		m_pCurrentEntity->curstate.sequence = 0;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

/*#if !defined (CORRECT_PITCH)
	m_pCurrentEntity->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;
#endif*/
	StudioPlayerBlend(pseqdesc, &iBlend, &m_pCurrentEntity->angles[PITCH]);// INPUTS and OUTPUTS angles in DEGREES
	if (gHUD.m_pLocalPlayer == m_pCurrentEntity)
		m_pCurrentEntity->angles[PITCH] *= -1.0f;
/*#if !defined (CORRECT_PITCH)
	m_pCurrentEntity->angles[PITCH] /= PITCH_CORRECTION_MULTIPLIER;
#endif*/

	m_pCurrentEntity->curstate.blending[0] = iBlend;
	m_pCurrentEntity->latched.prevangles[PITCH] = m_pCurrentEntity->angles[PITCH];
	m_pCurrentEntity->latched.prevblending[0] = m_pCurrentEntity->curstate.blending[0];
	m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];

	// Con_DPrintf("%f %d\n", m_pCurrentEntity->angles[PITCH], m_pCurrentEntity->blending[0] );

	double dt = (m_clTime - m_clOldTime);// XDM3037a: was float
	if (dt < 0)
		dt = 0;
	else if (dt > 1.0)
		dt = 1;

	StudioEstimateGait(pplayer);
	// Con_DPrintf("%f %f\n", m_pCurrentEntity->angles[YAW], m_pPlayerInfo->gaityaw );

	// calc side to side turning
	flYaw = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
	flYaw = flYaw - (int)(flYaw / 360.0f) * 360.0f;
	NormalizeAngle180(&flYaw);// XDM3038a

	if (flYaw > 120)
	{
		m_pPlayerInfo->gaityaw -= 180.0f;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw -= 180.0f;
	}
	else if (flYaw < -120)
	{
		m_pPlayerInfo->gaityaw += 180.0f;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw += 180.0f;
	}

	// adjust torso
	m_pCurrentEntity->curstate.controller[1] = m_pCurrentEntity->curstate.controller[2] = m_pCurrentEntity->curstate.controller[3] = m_pCurrentEntity->curstate.controller[0] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);// XDM3037: why same value for all?
	m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
	m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
	m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
	m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];
	m_pCurrentEntity->angles[YAW] = m_pPlayerInfo->gaityaw;
	NormalizeAngle360(&m_pCurrentEntity->angles[YAW]);// XDM3038a
	m_pCurrentEntity->latched.prevangles[YAW] = m_pCurrentEntity->angles[YAW];

	if (pplayer->gaitsequence >= m_pStudioHeader->numseq)// GAITSEQUENCE_DISABLED should be already checked beforehand!!!
		pplayer->gaitsequence = 0;

	if (gHUD.m_iPaused || gHUD.m_iIntermission)// XDM3037: stops current frame
		return;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + pplayer->gaitsequence;

	// calc gait frame
	if (pseqdesc->linearmovement[0] > 0)
		m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	else
		m_pPlayerInfo->gaitframe += pseqdesc->fps * dt;

	// do modulo
	m_pPlayerInfo->gaitframe = m_pPlayerInfo->gaitframe - (int)(m_pPlayerInfo->gaitframe / pseqdesc->numframes) * pseqdesc->numframes;
	if (m_pPlayerInfo->gaitframe < 0)
	{
		//if (pseqdesc->flags & STUDIO_LOOPING)// XDM3037: UNDONE
			m_pPlayerInfo->gaitframe += pseqdesc->numframes;
		//else
		//	m_pPlayerInfo->gaitframe = pseqdesc->numframes-1;// keep last? = 0.0f;// reset?
	}
}

/*
====================
StudioDrawModel

====================
*/
int CStudioModelRenderer::StudioDrawModel( int flags )
{
	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();
	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
	IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

	m_pRenderModel = m_pCurrentEntity->model;
	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pRenderModel);
	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	StudioSetUpTransform(0);

	if (flags & STUDIO_RENDER)
	{
		// see if the bounding box lets us trivially reject, also sets
		if (!(m_pCurrentEntity->baseline.eflags & EFLAG_DRAW_ALWAYS))// XDM3035: some env_static tempentities are stuck in brushes
		{
			if (!IEngineStudio.StudioCheckBBox())
				return 0;
		}

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++; // render data cache cookie

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	if (m_pCurrentEntity->curstate.movetype == MOVETYPE_FOLLOW)
		StudioMergeBones(m_pRenderModel);
	else
		StudioSetupBones();

	StudioSaveBones();

	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments();
		IEngineStudio.StudioClientEvents();
		// copy attachments into global entity array
		if (m_pCurrentEntity->index > 0)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);
			if (ent)
				memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * MAXSTUDIOATTACHMENTS);
		}
	}

	if (flags & STUDIO_RENDER)
	{
// doesn't save a single FPS		if (!UTIL_PointIsFar(m_pCurrentEntity->origin))// XDM3035c
//		{
		if (m_pCurrentEntity->curstate.eflags & EFLAG_HIGHLIGHT)
		{
			//aLightEntityHighlight.ambientlight = test1->value;// + g_fEntityHighlightEffect*test1->value;
			//aLightEntityHighlight.shadelight = test2->value;// + g_fEntityHighlightEffect*test2->value;
			aLightEntityHighlight.color[0] = 0.75f + 0.25f*g_fEntityHighlightEffect;// 1/4 range
			aLightEntityHighlight.color[1] = aLightEntityHighlight.color[2] = aLightEntityHighlight.color[0];
			IEngineStudio.StudioSetupLighting(&aLightEntityHighlight);// use special lighting
		}
		else if (m_pCurrentEntity->curstate.renderfx == kRenderFxFullBright)// XDM3035: ignore environment lighting
		{
			IEngineStudio.StudioSetupLighting(&aLightFullBright);// use fixed maximal lighting
		}
		else
		{
			alight_t lighting;
			//vec3_t dir;
			lighting.plightvec = (float *)&g_vecLightTmp;
			IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting);
			IEngineStudio.StudioEntityLight(&lighting);
			// model and frame independant
			IEngineStudio.StudioSetupLighting(&lighting);
		}
//		}// far
		// get remap colors
		IEngineStudio.StudioSetRemapColors(m_pCurrentEntity->curstate.colormap & 0xFF, (m_pCurrentEntity->curstate.colormap & 0xFF00) >> 8);
		StudioRenderModel();
	}
	return 1;
}

//const float g_fPlayerHighlightMaxDist = 512.0f;// XDM3035
/*
====================
StudioDrawPlayer

====================
*/
int CStudioModelRenderer::StudioDrawPlayer(int flags, entity_state_t *pplayer)
{
	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();
	//ASSERT(gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index) == m_pCurrentEntity); should be true

	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
	IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

	// Con_DPrintf("DrawPlayer %d\n", m_pCurrentEntity->blending[0] );
	// Con_DPrintf("DrawPlayer %d %d (%d)\n", m_nFrameCount, pplayer->player_index, m_pCurrentEntity->curstate.sequence );
	// Con_DPrintf("Player %.2f %.2f %.2f\n", pplayer->velocity[0], pplayer->velocity[1], pplayer->velocity[2] );

	m_nPlayerIndex = pplayer->number - 1;

	//ASSERT(m_nPlayerIndex >= 0 && m_nPlayerIndex < gEngfuncs.GetMaxClients());
	//if (m_nPlayerIndex < 0 || m_nPlayerIndex >= gEngfuncs.GetMaxClients())
	//	return 0;

	m_pRenderModel = IEngineStudio.SetupPlayerModel(m_nPlayerIndex);
	if (m_pRenderModel == NULL)
		return 0;

	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pRenderModel);
	if (m_pStudioHeader == NULL)// XDM3037a
		return 0;

	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

	if (gHUD.m_pLocalPlayer && pplayer->number == gHUD.m_pLocalPlayer->index)
		m_pCurrentEntity->angles[PITCH] = -m_pCurrentEntity->angles[PITCH];
	else
		m_pCurrentEntity->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;

/*#if !defined (CORRECT_PITCH)
	if (gHUD.m_pLocalPlayer && pplayer->number == gHUD.m_pLocalPlayer->index)// XDM3038c: took years to find out how this shit gets corrupted in the engine
		m_pCurrentEntity->angles[PITCH] *= -PITCH_CORRECTION_MULTIPLIER;
	//else
		//m_pCurrentEntity->angles[PITCH] *= PITCH_CORRECTION_MULTIPLIER;
#endif*/
	// nothere m_pCurrentEntity->angles[PITCH] *= cl_test1->value;
	DBG_ANGLES_DRAW(9, m_pCurrentEntity->origin, m_pCurrentEntity->angles, pplayer->number, "DrawPlayer()");

	if (pplayer->gaitsequence != GAITSEQUENCE_DISABLED/* && pplayer->health > 0 && (UTIL_PointViewDist(m_pCurrentEntity->origin) <= 2.0f)*/)// XDM3037: allow gaitsequence 0
	{
		Vector orig_angles(m_pCurrentEntity->angles);
		StudioProcessGait(pplayer);// this modifies angles
		m_pCurrentEntity->angles[PITCH] *= 0.1f;// decrease player model rotation even more for better looks
		m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		m_pPlayerInfo = NULL;// important
		StudioSetUpTransform(0);
		VectorCopy(orig_angles, m_pCurrentEntity->angles);
	}
	else
	{
		/*int iBlend = BLEND_MID;// XDM3035b: why isn't local blending working??
		mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;
		StudioPlayerBlend(pseqdesc, &iBlend, &m_pCurrentEntity->angles[PITCH]);// XDM3035b
		m_pCurrentEntity->latched.prevangles[PITCH] = m_pCurrentEntity->angles[PITCH];
		m_pCurrentEntity->curstate.blending[0] = iBlend;
		m_pCurrentEntity->latched.prevblending[0] = m_pCurrentEntity->curstate.blending[0];
		m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];*/
		m_pCurrentEntity->curstate.blending[0] = BLEND_MID;// XDM3035b reset?
		m_pCurrentEntity->curstate.blending[1] = BLEND_MID;
		m_pCurrentEntity->curstate.blending[2] = BLEND_MID;
		m_pCurrentEntity->curstate.blending[3] = BLEND_MID;
		m_pCurrentEntity->curstate.controller[0] = 127;
		m_pCurrentEntity->curstate.controller[1] = 127;
		m_pCurrentEntity->curstate.controller[2] = 127;
		m_pCurrentEntity->curstate.controller[3] = 127;
		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];
		//m_pPlayerInfo = IEngineStudio.PlayerInfo( m_nPlayerIndex );
		m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		StudioSetUpTransform(0);
	}

	if (flags & STUDIO_RENDER)
	{
		// see if the bounding box lets us trivially reject, also sets
		if (!IEngineStudio.StudioCheckBBox())
			return 0;

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++; // render data cache cookie

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	// HACK
	m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);
	StudioSetupBones();
	StudioSaveBones();

	m_pPlayerInfo->renderframe = m_nFrameCount;
	m_pPlayerInfo = NULL;// old hack

	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments();
		IEngineStudio.StudioClientEvents();
		// copy attachments into global entity array
		if (m_pCurrentEntity->index > 0 && pplayer->weaponmodel == 0)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);
			// XDM: WTF? ent == m_pCurrentEntity?
			memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t)*MAXSTUDIOATTACHMENTS);
		}
	}

	if (flags & STUDIO_RENDER)
	{
		// XDM3035: brought back
		if (m_pRenderModel != m_pCurrentEntity->model)// m_pCurrentEntity->model == player.mdl, m_pRenderModel == player/model/model.mdl
		{
			int iLOD;
			//if (m_pCvarHiModels->value > 0)// XDM3038b: disable LOD, use highest body in every group
				//m_pCurrentEntity->curstate.body = R_StudioBodyVariations(m_pRenderModel)-1;//(int)m_pCvarHiModels->value;// XDM3035a: TESTME! show highest resolution multiplayer model
			if (m_pCvarHiModels->value <= 0 && g_pCvarLODDist->value > 0)
				iLOD = (int)(((m_pCurrentEntity->origin - g_vecViewOrigin).Length()/g_pCvarLODDist->value) - gHUD.GetUpdatedDefaultFOV()/gHUD.GetCurrentFOV()+1.0f);// zoom 10x means decrease LOD by 9 levels, because when zoom is 1x we don't change LOD.
			else
				iLOD = 0;// default
				//vec_t fDistance = (m_pCurrentEntity->origin - g_vecViewOrigin).Length();
				//int iLOD = (int)(fDistance/g_pCvarLODDist->value);
				//int iLOD = (m_pCvarHiModels->value > 0)?0:(int)(fDistance/g_pCvarLODDist->value);

			m_pCurrentEntity->curstate.body = R_StudioBodyLOD(m_pRenderModel, max(0,iLOD));// XDM3038b: finally, LoD!
		}
		alight_t lighting;
		//vec3_t dir;
		lighting.plightvec = (float *)&g_vecLightTmp;// somebody tell me wtf is this?!

		//XDM3035: works but do we need it?	if (!UTIL_PointIsVisible(m_pCurrentEntity->origin, true))
		//	return 0;

		if (!UTIL_PointIsFar(m_pCurrentEntity->origin))// XDM3035c
		{
			IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting);
			IEngineStudio.StudioEntityLight(&lighting);
			// model and frame independant
			IEngineStudio.StudioSetupLighting(&lighting);
		}// far

		// XDM3037a: hack: get info once again
		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);
		IEngineStudio.StudioSetRemapColors(m_pPlayerInfo->topcolor, m_pPlayerInfo->bottomcolor);
		// FAIL: no data! IEngineStudio.StudioSetRemapColors(colormap2topcolor(m_pCurrentEntity->curstate.colormap), colormap2bottomcolor(m_pCurrentEntity->curstate.colormap));
		StudioRenderModel();
		m_pPlayerInfo = NULL;

		if (pplayer->weaponmodel && !UTIL_PointIsFar(m_pCurrentEntity->origin))// XDM3035c: I personally hate all of this recursive not-so-OOP bullshit
		{
			cl_entity_t SavedEnt = *m_pCurrentEntity;// WTF: will be overwritten with current weapon model "entity"
			model_t *pWeaponModel = IEngineStudio.GetModelByIndex(pplayer->weaponmodel);
			if (pWeaponModel)
			{
				m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(pWeaponModel);
				IEngineStudio.StudioSetHeader(m_pStudioHeader);
				StudioMergeBones(pWeaponModel);
				IEngineStudio.SetRenderModel(pWeaponModel);// XDM3037a: is this right thing to do?
				//IEngineStudio.StudioSetRemapColors(m_nTopColor, m_nBottomColor);// XDM3034: same colors?
				if (gHUD.m_pLocalPlayer && SavedEnt.index == gHUD.m_pLocalPlayer->index)// copy body and skin from view model to weapon model
				{
					cl_entity_t *pViewModel = gEngfuncs.GetViewModel();
					if (pViewModel)
					{
						m_pCurrentEntity->curstate.skin = pViewModel->curstate.skin;
						m_pCurrentEntity->curstate.body = pViewModel->curstate.body;
						//mstudiotexture_t *pTextureHeader = (mstudiotexture_t *)((byte *)m_pStudioHeader + m_pStudioHeader->textureindex + sizeof(mstudiotexture_t) * (int)test1->value);
						//mstudiotexture_t *pTextureHeader = m_pStudioHeader + m_pStudioHeader->texturedataindex + sizeof(mstudiotexture_t) * (int)test1->value;
						//if (pTextureHeader)
						//	IEngineStudio.StudioSetupSkin(m_pStudioHeader, test1->value);//viewmodel->curstate.skin);// XDM3035c: Why doesn't this work?!
					}
				}
				IEngineStudio.StudioSetupLighting(&lighting);
				StudioRenderModel();// or skip to StudioRenderFinal();?
				//memcpy(SavedEnt.attachment, m_pCurrentEntity->attachment, sizeof(vec3_t)*MAXSTUDIOATTACHMENTS);// somehow this gets cleared!
				*m_pCurrentEntity = SavedEnt;// WHAT THE SHIT WAS THAT?!!!!!!!
				StudioCalcAttachments();
			}
		}
#if defined (CLDLL_FIX_PLAYER_ATTACHMENTS)
		m_pCurrentEntity->baseline.vuser1 = m_pCurrentEntity->attachment[0];// XDM3038c: HACKFIX to override bullshit that valve wrote
		m_pCurrentEntity->baseline.vuser2 = m_pCurrentEntity->attachment[1];
		m_pCurrentEntity->baseline.vuser3 = m_pCurrentEntity->attachment[2];
		m_pCurrentEntity->baseline.vuser4 = m_pCurrentEntity->attachment[3];
#endif
	}
	return 1;
}

/*
====================
StudioCalcAttachments

====================
*/
void CStudioModelRenderer::StudioCalcAttachments( void )
{
	if (m_pStudioHeader->numattachments > MAXSTUDIOATTACHMENTS)
	{
		CON_DPRINTF("WARNING! Too many attachments on %s\n", m_pStudioHeader->name);
		m_pStudioHeader->numattachments = MAXSTUDIOATTACHMENTS;
		// XDM	exit(-1);
	}

	// calculate attachment points
	mstudioattachment_t *pAttachments = (mstudioattachment_t *)((byte *)m_pStudioHeader + m_pStudioHeader->attachmentindex);
	for (int i = 0; i < m_pStudioHeader->numattachments; ++i)
		VectorTransform(pAttachments[i].org, (*m_plighttransform)[pAttachments[i].bone], m_pCurrentEntity->attachment[i]);
}

/*
====================
StudioRenderModel

====================
*/
#include "gl_dynamic.h"

static GLfloat fog_color_reset[4] = {0.0f, 0.0f, 0.0f, 0.0f};// 4 real floats, 0.0 - 1.0
extern GLfloat fog_color_gl[4];

void CStudioModelRenderer::StudioRenderModel(void)
{
	IEngineStudio.SetChromeOrigin();
	IEngineStudio.SetForceFaceFlags(0);

	if (m_pCurrentEntity->curstate.renderfx == kRenderFxGlowShell)
	{
		//int savedmode = m_pCurrentEntity->curstate.rendermode;
		m_pCurrentEntity->curstate.renderfx = kRenderFxNone;
		StudioRenderFinal();

#if !defined (CLDLL_NOFOG)
		if (gHUD.m_iHardwareMode == 1)// XDM3035 OpenGL: disable fog
		{
			if (gHUD.m_iFogMode > 0)
				GL_glFogfv(GL_FOG_COLOR, fog_color_reset);// GL_glDisable(GL_FOG); does not work
		}
#endif

// causes random white flicker		m_pCurrentEntity->curstate.rendermode = kRenderTransAdd;
		IEngineStudio.SetForceFaceFlags(STUDIO_NF_ADDITIVE|STUDIO_NF_CHROME);
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		gEngfuncs.pTriAPI->SpriteTexture(m_pChromeSprite, 0);
		m_pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;

		StudioRenderFinal();

		if (gHUD.m_iHardwareMode > 0)//if (!IEngineStudio.IsHardware())
		{
#if !defined (CLDLL_NOFOG)
			if (gHUD.m_iHardwareMode == 1)// XDM3035 OpenGL: re-enable fog
			{
				if (gHUD.m_iFogMode > 0)
					GL_glFogfv(GL_FOG_COLOR, fog_color_gl);
			}
#endif
		}
		else
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

		//m_pCurrentEntity->curstate.rendermode = savedmode;
	}
	else
	{
		if (m_pCurrentEntity->curstate.renderfx == kRenderFxFullBright)// XDM
			IEngineStudio.SetForceFaceFlags(STUDIO_NF_FULLBRIGHT);
		else if (m_pCurrentEntity->curstate.renderfx == kRenderFxDisintegrate)
		{
			// XDM: right now it's just a flag. do nothing?
			/*BUGBUG! bone origins are zeroes?
			if (RANDOM_LONG(0,10) == 0)
			{
				//if (m_pStudioHeader->numhitboxes > 0)
				//mstudiobbox_t *pHitBoxes = (mstudiobbox_t *)((byte *)m_pStudioHeader + m_pStudioHeader->hitboxindex);

				if (m_pStudioHeader->numbones > 0)
				{
					mstudiobone_t *pBones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);
					//Matrix3x4_OriginFromMatrix(m_pbonetransform[i], point);
					int j = RANDOM_LONG(0, m_pStudioHeader->numbones);
					Vector pos(*m_pbonetransform[j][0][3], *m_pbonetransform[j][1][3], *m_pbonetransform[j][2][3]);
					//for (j = 0; j < m_pStudioHeader->numhitboxes; ++j)
					gEngfuncs.pEfxAPI->R_SparkEffect(pos, RANDOM_LONG(8,16), -RANDOM_LONG(100,200), RANDOM_LONG(100,200));
				}
			}*/
		}
		//else if (m_pCurrentEntity->curstate.renderfx == kRenderFxFlatShade)
		//	IEngineStudio.SetForceFaceFlags(STUDIO_NF_FLATSHADE);

		StudioRenderFinal();
	}
}

/*
====================
StudioRenderFinal_Software

====================
*/
void CStudioModelRenderer::StudioRenderFinal_Software( void )
{
	// Note, rendermode set here has effect in SW
	int faceflags = IEngineStudio.GetForceFaceFlags();// XDM3035c: lasts from any model in the frame that had ANY faces with flags
	int rendermode = (faceflags & STUDIO_NF_ADDITIVE) ? kRenderTransAdd : m_pCurrentEntity->curstate.rendermode;// global render mode :(
	IEngineStudio.SetupRenderer(rendermode);
//?	IEngineStudio.SetupRenderer(0);//m_pCurrentEntity->curstate.rendermode); 

	if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else
	{
		for (int i=0; i < m_pStudioHeader->numbodyparts; ++i)
		{
			IEngineStudio.StudioSetupModel(i, (void **)&m_pBodyPart, (void **)&m_pSubModel);
			IEngineStudio.StudioDrawPoints();
		}
		if (m_pCvarDrawEntities->value == 4)
		{
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
			IEngineStudio.StudioDrawHulls();
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		}
		else if (m_pCvarDrawEntities->value == 5)
		{
			IEngineStudio.StudioDrawAbsBBox();
		}
		else if (m_pCvarDrawEntities->value == 6 && gHUD.m_iPaused == 0)
		{
			for (int i = 0; i < m_pStudioHeader->numattachments; ++i)
				gEngfuncs.pEfxAPI->R_MuzzleFlash(m_pCurrentEntity->attachment[i], 10+i);
		}
	}
	IEngineStudio.RestoreRenderer();
}

/*
====================
StudioRenderFinal_Hardware

====================
*/
void CStudioModelRenderer::StudioRenderFinal_Hardware( void )
{
	int faceflags = IEngineStudio.GetForceFaceFlags();// XDM3035c: lasts from any model in the frame that had ANY faces with flags
	int rendermode = (faceflags & STUDIO_NF_ADDITIVE) ? kRenderTransAdd : m_pCurrentEntity->curstate.rendermode;// global render mode :(
	IEngineStudio.SetupRenderer(rendermode);

	if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else
	{
		for (int i=0; i < m_pStudioHeader->numbodyparts; ++i)
		{
			IEngineStudio.StudioSetupModel(i, (void **)&m_pBodyPart, (void **)&m_pSubModel);// BUGBUG: this crashes with models that use external textures
			if (m_fDoInterp)
			{
				// interpolation messes up bounding boxes.
				m_pCurrentEntity->trivial_accept = 0; 
			}
			IEngineStudio.GL_SetRenderMode(rendermode);
			IEngineStudio.StudioDrawPoints();
			IEngineStudio.GL_StudioDrawShadow();
		}

		if (m_pCvarDrawEntities->value == 4)
		{
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
			IEngineStudio.StudioDrawHulls();
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		}
		else if (m_pCvarDrawEntities->value == 5)
		{
			IEngineStudio.StudioDrawAbsBBox();
		}
		else if (m_pCvarDrawEntities->value == 6 && gHUD.m_iPaused == 0)
		{
			//mstudioattachment_t *pAttachments = (mstudioattachment_t *)((byte *)m_pStudioHeader + m_pStudioHeader->attachmentindex);
			//mstudiobone_t *pBones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);
			for (int i = 0; i < m_pStudioHeader->numattachments; ++i)
			{
				//gEngfuncs.pTriAPI->Begin(TRI_LINES);
				//gEngfuncs.pTriAPI->Vertex3fv(pBones[pAttachments[i].bone].???
				//gEngfuncs.pTriAPI->Vertex3fv(m_pCurrentEntity->attachment[i]);
				gEngfuncs.pEfxAPI->R_MuzzleFlash(m_pCurrentEntity->attachment[i], 10+i);
			}
		}
	}
	IEngineStudio.RestoreRenderer();
}

/*
====================
StudioRenderFinal

====================
*/
void CStudioModelRenderer::StudioRenderFinal(void)
{
	if (IEngineStudio.IsHardware() > 0)
		StudioRenderFinal_Hardware();
	else
		StudioRenderFinal_Software();
}
