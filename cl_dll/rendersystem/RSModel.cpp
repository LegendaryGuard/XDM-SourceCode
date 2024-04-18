#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSModel.h"
#include "entity_types.h"
#include "bsputil.h"

CRSModel::CRSModel(void) : CRSModel::BaseClass()
{
	DBG_RS_PRINT("CRSModel()");
	m_pTexture = NULL;// MUST be before InitTexture(), InitModel() and ResetParameters()
	m_pModel = NULL;
	m_pEntity = NULL;
	ResetParameters();// non-recursive
}

//-----------------------------------------------------------------------------
// Purpose: Static model
// Input  : origin - 
//			framerate - 
//			timetolive - 0 means the system removes itself after the last frame
//-----------------------------------------------------------------------------
CRSModel::CRSModel(const Vector &origin, const Vector &angles, const Vector &velocity, int entindex, struct model_s *pModel, int body, int skin, int sequence, int r_mode, int r_fx, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive)
{
	DBG_RS_PRINT("CRSModel(...)");
	m_pTexture = NULL;// MUST be before InitTexture(), InitModel() and ResetParameters()
	m_pModel = NULL;
	m_pEntity = NULL;
	ResetParameters();
	if (!InitModel(pModel))
	{
		m_RemoveNow = true;
		return;
	}
	m_iModelBody = body;
	m_iModelTextureGroup = skin;
	m_iModelSequence = sequence;
	m_vecOrigin = origin;
	m_vecAngles = angles;
	m_vecVelocity = velocity;
	m_color.Set(r,g,b,a*255.0f);
	//old m_fBrightness = a;
	m_fColorDelta[3] = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;
	m_iRenderEffects = r_fx;
	m_fFrameRate = framerate;
	m_iFollowEntity = entindex;

	if (timetolive <= 0.0f)// if 0, just display all frames
		m_fDieTime = 0.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Warning: Destructors are called sequentially in the chain of inheritance so don't call recursive functions from here!
//-----------------------------------------------------------------------------
CRSModel::~CRSModel(void)
{
	DBG_RS_PRINT("~CRSModel()");
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: Virtual wrapper for ResetParameters() for sub-to-superclass calls.
// Warning: Each derived class must call its BaseClass::ResetAllParameters()!
//-----------------------------------------------------------------------------
/*void CRSModel::ResetAllParameters(void)
{
	CRSModel::BaseClass::ResetAllParameters();
	ResetParameters();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set default (including public, non-system) values for all class variables
// Warning: Do not call any functions from here!
// Warning: Do not call BaseClass::ResetParameters()!
// Note   : Must be called from class constructor (before InitializeSystem).
//-----------------------------------------------------------------------------
void CRSModel::ResetParameters(void)
{
	m_iBSPLeaf = 0;// -1 ??
	if (m_pEntity)
	{
		m_pEntity->baseline.startpos.Clear();
		m_pEntity->baseline.endpos.Clear();
		m_pEntity->baseline.gravity = 0.0f;
		m_pEntity->baseline.solid = SOLID_NOT;
		m_pEntity->baseline.movetype = MOVETYPE_NONE;
	}
	SetBits(m_iFlags, RENDERSYSTEM_FLAG_LOOPFRAMES);// compatibility HACK
}

//-----------------------------------------------------------------------------
// Purpose: Initialize SYSTEM (non-user) startup variables.
// Warning: this function may only use variables set by constructor
// as other parameters may be set AFTER this instance has been created!
// Must be called from/after class constructor.
//-----------------------------------------------------------------------------
void CRSModel::InitializeSystem(void)
{
	ClearBits(m_iFlags, RENDERSYSTEM_FLAG_HARDSHUTDOWN);// !!!
	ASSERT(m_pEntity == NULL);
	CRSModel::BaseClass::InitializeSystem();// <- follow ent, color, etc.
	if (IsRemoving())
		return;

	m_pEntity = new cl_entity_t;
	memset(m_pEntity, 0, sizeof(cl_entity_t));

	//m_pEntity->index = g_pRefParams?g_pRefParams->max_entities:4096;// XDM3038c: fake index so it won't be discarded by Mod_CheckEntityPVS // +m_iFollowEntity ?
	if (m_iFollowEntity <= 0)
		m_pEntity->index = MAX_EDICTS;// XDM3038c: HACK
	else
		m_pEntity->index = m_iFollowEntity;// MUST BE SET!

	m_pEntity->model = m_pModel;
	m_pEntity->player = false;
	m_pEntity->current_position = 0;
	m_pEntity->efrag = NULL;
	m_pEntity->topnode = NULL;
	m_pEntity->origin = m_vecOrigin;
	m_pEntity->angles = m_vecAngles;
	/* memset is enough m_pEntity->baseline.startpos.Clear();
	m_pEntity->baseline.endpos.Clear();
	m_pEntity->baseline.velocity.Clear();*/
	m_pEntity->curstate.entityType		= m_pEntity->baseline.entityType = ET_NORMAL;
	m_pEntity->curstate.number			= m_pEntity->baseline.number = m_pEntity->index;
	m_pEntity->curstate.gravity			= m_pEntity->baseline.gravity = 0.0f;// THIS ENTITY IS ONLY FOR DRAWING, NOT FOR PHYSICS!
	m_pEntity->curstate.origin			= m_pEntity->baseline.origin = m_vecOrigin;
	m_pEntity->curstate.angles			= m_pEntity->baseline.angles = m_vecAngles;
	//m_pEntity->curstate.modelindex	= m_pEntity->baseline.modelindex = ???;
	m_pEntity->curstate.sequence		= m_pEntity->baseline.sequence = m_iModelSequence;
	m_pEntity->curstate.frame			= m_pEntity->baseline.frame = 0;
	//NO!m_pEntity->curstate.colormap	= RGB2colormap(m_color.r, m_color.g, m_color.b);
	m_pEntity->curstate.skin			= m_pEntity->baseline.skin = m_iModelTextureGroup;
	m_pEntity->curstate.solid			= m_pEntity->baseline.solid = SOLID_NOT;
	//m_pEntity->curstate.effects		= m_pEntity->baseline.effects = EF_INVLIGHT;//effects;
	m_pEntity->curstate.scale			= m_pEntity->baseline.scale = m_fScale;
	m_pEntity->curstate.eflags			= m_pEntity->baseline.eflags = EFLAG_DRAW_ALWAYS;
	m_pEntity->curstate.rendermode		= m_pEntity->baseline.rendermode = m_iRenderMode;
	m_pEntity->curstate.renderamt		= m_pEntity->baseline.renderamt = m_color.a;//(int)(m_fBrightness*255.0f);
	m_pEntity->curstate.rendercolor.r	= m_pEntity->baseline.rendercolor.r = m_color.r;
	m_pEntity->curstate.rendercolor.g	= m_pEntity->baseline.rendercolor.g = m_color.g;
	m_pEntity->curstate.rendercolor.b	= m_pEntity->baseline.rendercolor.b = m_color.b;
	m_pEntity->curstate.renderfx		= m_pEntity->baseline.renderfx = m_iRenderEffects;
	m_pEntity->curstate.movetype		= m_pEntity->baseline.movetype = MOVETYPE_NONE;
	m_pEntity->curstate.framerate		= m_pEntity->baseline.framerate = m_fFrameRate;
	m_pEntity->curstate.body			= m_pEntity->baseline.body = m_iModelBody;
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
/*void CRSModel::KillSystem(void)
{
	DBG_RS_PRINT("KillSystem()");
	CRSModel::BaseClass::KillSystem();
	FreeData();
}*/

//-----------------------------------------------------------------------------
// Purpose: Free allocated dynamic memory
// WARNING: Special case! Don't delete system/call this AFTER CL_CreateVisibleEntity() has been used this frame!!! Or the engine will step into freed memory!
// Warning: Do not call any functions from here! Do not call BaseClass::FreeData()!
// Note   : Must be called from class destructor.
//-----------------------------------------------------------------------------
void CRSModel::FreeData(void)
{
	DBG_RS_PRINT("FreeData()");
	m_pModel = NULL;
	if (m_pEntity)
	{
		//m_pEntity->curstate.number = m_pEntity->baseline.number = m_pEntity->index = -1;
		memset(m_pEntity, 0, sizeof(cl_entity_t));
		delete m_pEntity;
		m_pEntity = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse known values into variables
// Input  : *szKey - 
//			*szValue - 
// Output : Returns true if key was accepted.
//-----------------------------------------------------------------------------
bool CRSModel::ParseKeyValue(const char *szKey, const char *szValue)
{
	if (strcmp(szKey, "szModel") == 0)// MODEL MUST BE PRECACHED ON THE SERVER BECAUSE HL DOES NOT ALLOW LOADING OF A STUDIO MODEL
	{
		m_pModel = IEngineStudio.Mod_ForName(szValue, 0);
		if (m_pModel == NULL || m_pModel->type != mod_studio)
		{
			conprintf(1, "CRSModel::Mod_ForName(%s) failed! Model was not precached or is not a studio model.\n", szValue);
			return false;
		}
	}
	else
		return CRSModel::BaseClass::ParseKeyValue(szKey, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRSModel::Update(const float &time, const double &elapsedTime)
{
	CRSModel::BaseClass::Update(time, elapsedTime);

	if (IsShuttingDown())// IMPORTANT: prevent deletion of this system until m_pEntity is not sent to the engine
	{
		if (m_pEntity == NULL || m_pEntity->index <= 0)// || m_pEntity->model == NULL)// emergency || normal shutdown
			return 1;// safe to remove and free memory

		m_RemoveNow = false;
		m_pEntity->index = -m_pEntity->index;// crash m_pEntity->model = NULL;// signal to shutdown
		m_pEntity->curstate.effects = EF_NODRAW;
		m_pEntity->curstate.rendermode = kRenderTransTexture;
		m_pEntity->curstate.renderamt = 0;
	}
	else// update entity normally
	{
		if (m_pEntity)
		{
			//UpdateFrame(time, elapsedTime);// for texture
			//m_fFrame += m_fFrameRate * elapsedTime;
			/*m_color.r += (int)(m_fColorDelta[0] * elapsedTime);
			m_color.g += (int)(m_fColorDelta[1] * elapsedTime);
			m_color.b += (int)(m_fColorDelta[2] * elapsedTime);
			m_fScale += m_fScaleDelta * (float)elapsedTime;*/
			m_pEntity->curstate.origin = m_vecOrigin;
			m_pEntity->curstate.angles = m_vecAngles;
			//NO!m_pEntity->curstate.colormap = RGB2colormap(m_color.r, m_color.g, m_color.b);
			m_pEntity->curstate.rendermode = m_iRenderMode;
			m_pEntity->curstate.renderamt = (int)(m_fColorCurrent[3]*255.0f);//(int)(m_fBrightness*255.0f);
			m_pEntity->curstate.rendercolor.r = (int)(m_fColorCurrent[0]*255.0f);//m_color.r;
			m_pEntity->curstate.rendercolor.g = (int)(m_fColorCurrent[1]*255.0f);//m_color.g;
			m_pEntity->curstate.rendercolor.b = (int)(m_fColorCurrent[2]*255.0f);//m_color.b;
			m_pEntity->curstate.scale = m_fScale;
			//no! m_pEntity->curstate.frame = m_fFrame;
			m_pEntity->curstate.framerate = m_fFrameRate;
			m_pEntity->curstate.body = m_pEntity->baseline.body;
			m_pEntity->curstate.skin = m_pEntity->baseline.skin;
			m_pEntity->curstate.sequence = m_pEntity->baseline.sequence;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Special version for HL entities
// Input  : &time - 
//			&elapsedTime - 
//-----------------------------------------------------------------------------
void CRSModel::UpdateFrame(const float &time, const double &elapsedTime)
{
	/* engine does this stuff for entities
	if (m_fFrameRate == 0.0f)// XDM3037a: single frame
		return;
	if (m_pEntity == NULL)
	{
		DBG_PRINTF("CRSModel[%s]::UpdateFrame(): m_pEntity == NULL!\n", GetUID());
		return;
	}
	if (m_pModel == NULL)
	{
		DBG_PRINTF("CRSModel[%s]::UpdateFrame(): m_pModel == NULL!\n", GetUID());
		return;
	}
	if (m_pModel->numframes <= 1)
		return;
	if (gHUD.m_iPaused > 0)
		return;

	if (m_pEntity->curstate.frame >= 255.0f)
	{
		// don't remove after last frame
		if (m_fDieTime == 0.0f)
		{
			if (!(m_iFlags & RENDERSYSTEM_FLAG_LOOPFRAMES))
			{
				ShutdownSystem();
				return;
			}
		}
		m_pEntity->curstate.frame -= 255.0f;
	}

	if (m_fFrameRate < 0)// framerate == fps
		m_pEntity->curstate.frame += 1.0f;
	else
		m_pEntity->curstate.frame += m_fFrameRate * elapsedTime;*/
}


//-----------------------------------------------------------------------------
// Purpose: Do nothing
//-----------------------------------------------------------------------------
void CRSModel::Render(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine, allows to add user entities to render list
// Warning: Static models are usually very huge! Sometimes entire interior of a room is a single model.
//-----------------------------------------------------------------------------
void CRSModel::CreateEntities(void)
{
	if (IsRemoving() || m_iState != RSSTATE_ENABLED)
		return;

	if (m_pEntity)
	{
		if (m_pEntity->model == NULL)// the system is shutting down
			return;
		if (m_pEntity->index <= 0)
			return;
		if (FBitSet(m_pEntity->curstate.effects, EF_NODRAW))
			return;

		float l;// = 2048.0f;
		//Vector vecNearest = GetNearestPointOfABoxMinsMaxs(g_vecViewOrigin, m_vecOrigin+m_pModel->mins, m_vecOrigin+m_pModel->maxs);
		//Vector dir(vecNearest);
		Vector dir(m_vecOrigin);
		dir -= g_vecViewOrigin;
		l = dir.NormalizeSelf();

		/* useless when bbox is 0
		float r;
		if (m_pModel)
			r = (m_pModel->maxs - m_pModel->mins).Length()*0.5f;// XDM3037a: we're inside of a really big model (ex.: skybox)
		else
			r = 0;

		if (l > r)*/
		{
			if (g_pCvarServerZMax && l >= g_pCvarServerZMax->value*0.9f)// clipped by sv_zmax
				return;

			if ((gHUD.m_iFogMode > 0) && (gHUD.m_flFogEnd > 32.0f) && (l >= gHUD.m_flFogEnd))// clipped by fog
				return;
		}

		// we could IEngineStudio.StudioCheckBBox(), but it requires StudioSetHeader and SetRenderModel
		// UNDONE: TODO: don't draw what shouldn't be drawn
		// Don't simply check origin because it may be underground, in walls, etc. or the model may be large enough to show even when player is not facing it.
		//	if (PointIsVisible(m_vecOrigin + (m_pModel->maxs - m_pModel->mins)*0.5f))// somehow all mins/maxs are 0 :(
		// LAME! origin is not enough! Need to check BSP!		if (CL_CheckVisibility(m_pEntity->curstate.origin))
		//	if (CL_CheckLeafVisibility(m_iBSPLeaf))

		if (gEngfuncs.PM_PointContents(m_vecOrigin, NULL) != CONTENTS_SOLID)
		{
			if (!Mod_CheckEntityPVS(m_pEntity))// XDM3035c: fast way to check visiblility
				return;// TODO: still does not work as desired (especially on stuck-in-ground objects)
		}
		//0 0if (!Mod_CheckBoxInPVS(m_pModel->mins, m_pModel->maxs))
		//	return;
		gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, m_pEntity);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Load model by index
// Input  : texture_index - precached model index
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSModel::InitModel(model_t *pModel)
{
	if (pModel == NULL)// if (model_index <= 0)
		return false;

	if (m_pModel == NULL || m_pModel != pModel)
	{
		//model_t *pModel = IEngineStudio.GetModelByIndex(model_index);
		if (/*pModel == NULL || */pModel->type != mod_studio)
		{
			conprintf(1, "CRSModel::InitModel(%s) failed: not a mod_studio!\n", pModel->name);
			return false;
		}

		m_pModel = pModel;
		//m_iModelIndex = model_index;

		// Works, but still all zeroes
		/*studiohdr_t *m_pModelData = (studiohdr_t *)IEngineStudio.Mod_Extradata(pModel);
		if (m_pModelData)
		{
			m_pModel->mins = m_pModelData->bbmin;
			m_pModel->maxs = m_pModelData->bbmax;
		}*/
		/*mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)m_pModelData + m_pModelData->seqindex) + m_pEntity->curstate.sequence;
		if (pseqdesc)
		{
			m_pModel->mins = pseqdesc->bbmin;
			m_pModel->maxs = pseqdesc->bbmax;
		}*/
	}
	return true;
}
