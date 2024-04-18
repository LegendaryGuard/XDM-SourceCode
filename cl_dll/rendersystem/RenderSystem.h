#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

//-----------------------------------------------------------------------------
// Render System
// by Xawari
//
// Licensed under the Mozilla Public License version 2.0.
//-----------------------------------------------------------------------------

//#if defined(_MSC_VER)
//#pragma warning(disable: 4097)// typedef-name 'BaseClass' used as synonym for class-name '%s'
//#endif

//#include "color.h"
#include "com_model.h"
#include "r_studioint.h"
#include "studio_util.h"

//#define _DEBUG_RENDERSYSTEM

#if defined (_DEBUG_RENDERSYSTEM)
#define DBG_RS_PRINTF			DBG_PrintF
#define DBG_RS_PRINT(text)		DBG_PrintF("%s[%d]\"%s\": %s\n", GetClassName(), GetIndex(), GetUID(), text)
#else
#define DBG_RS_PRINTF
#define DBG_RS_PRINT
#endif

extern vec3_t ev_punchangle;

/*typedef */enum rendersystemstate_e
{
	RSSTATE_DISABLED = 0,// must be 0
	RSSTATE_ENABLED = 1,// must be 1
	RSSTATE_ENABLING,
	RSSTATE_DISABLING,
	RSSTATE_VIRTUAL// should never change to any other state!
};

#ifndef RS_INDEX
typedef uint32 RS_INDEX;
#endif

#define RENDERSYSTEM_UID_LENGTH				64
#define RENDERSYSTEM_CLASSNAME_LENGTH		32
#define RENDERSYSTEM_USERNAME_LENGTH		32
#define RENDERSYSTEM_REFERENCE_FPS			60.0f// used for lifetime calculation

//-----------------------------------------------------------------------------
// Basic Render System, root class for all systems.
// This is a simple square non-rotating surface (sprite).
//-----------------------------------------------------------------------------
class CRenderSystem
{
	friend class CRenderManager;// allow manager to access protected data

public:
	void *operator new(size_t stAllocateBlock);// XDM3038c: custom fail-safe allocation mechanism
	void operator delete(void *pMem);

	virtual const char *GetClassName(void) const { return "CRenderSystem"; }// XDM3038b
	CRenderSystem(void);
	CRenderSystem(const Vector &origin, const Vector &origindelta, const Vector &angles, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive);
	virtual ~CRenderSystem(void);// ALL RS DESTRUCTORS MUST BE DECLARED AS VIRTUAL

	//virtual void ResetAllParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void InitializeSystem(void);
	//virtual void KillSystem(void);// recursive for subclasses
	virtual cl_entity_t *FollowEntity(void);
	virtual void FollowUpdatePosition(void);// XDM3035c: must be fast
	virtual void UpdateFrame(const float &time, const double &elapsedTime);
	virtual float GetRenderBrightness(void);// XDM3037a
	virtual void Render(void);
	virtual void CreateEntities(void);
	virtual void OnSystemRemoved(const CRenderSystem *pSystem);// XDM3038c

	virtual void SetState(const uint32 &iNewState);// XDM3038c
	virtual void ApplyForce(const Vector &origin, const Vector &force, float radius, bool point);
	virtual bool IsShuttingDown(void) const { return m_ShuttingDown > 0; }// soft shut down process
	virtual bool IsRemoving(void) const { return m_RemoveNow > 0; }// Render System is being removed! DON'T do anything with it! Set by constructors!
	virtual bool IsVirtual(void) const { return m_iState == RSSTATE_VIRTUAL; };// XDM3038c
	virtual bool CheckFxLevel(void) const;
	virtual bool ShouldDraw(void) const;

	void FreeData(void);// XDM3038c: non-virtual on purpose! See KB00001818 somewhere around.
	void ResetParameters(void);// XDM3038c: now it's non-virtual on purpose! See KB00001818 somewhere around.
	void ShutdownSystem(void);// XDM3037: tell the system to stop producing effects and shut down when ready
	bool SetUID(const char *uid);
	const char *GetUID(void) const { return m_UUID; }// SYSTEM
	const char *GetName(void) const { return m_szName; }// SYSTEM

	void ContentsArrayAdd(uint16 &container, signed int contents);
	void ContentsArrayRemove(uint16 &container, signed int contents);
	bool ContentsArrayHas(uint16 &container, signed int contents);
	void ContentsArrayClear(uint16 &container);
	//bool ContentsCheck(uint16 &container, const Vector &origin);

	bool InitTexture(struct model_s *pTexture);// SYSTEM
	bool PointIsVisible(const Vector &point);
	// not always means the same	bool IsLooping(void) const { return (m_fDieTime <= 0.0f); }

	RS_INDEX GetIndex(void) const { return index; }// SYSTEM
	CRenderSystem *GetNext(void) const { return m_pNext; }// SYSTEM
	uint32 GetState(void) const { return m_iState; }// R/O

	Vector m_vecOrigin;
	Vector m_vecVelocity;
	Vector m_vecAngles;
	//Vector m_vecAnglesDelta;// AVelocity
	// TODO:	Vector m_vecScale;
	Vector m_vecOriginPrev;// for interpolation (linear only)
	Vector m_vecOriginStart;
	Vector m_vOffset;// (right, forward, up)

	float m_fStartTime;// XDM3035b: remember when the system was started, also it must be zero until Initialized()!
	float m_fDieTime;// exact time at which the system will be deleted

	uint32 m_iFrame;
	//uint32 m_iFramePrevious;
	float m_fFrameAccumulator;// must be float, so it can accumulate half-frames generated by elapsedTime
	float m_fFrameRate;// FPS

	float m_fScale;
	float m_fScaleDelta;// SPS

	float m_fSizeX;// system dimensions, these are actually texture half-sizes
	float m_fSizeY;

	::Color m_color;// XDM3038b: 4 bytes, no padding and lots of great tools. This is now start color.
	float m_fColorCurrent[4];// XDM3038c: had to separate that, also floats are better for math
	float m_fColorDelta[4];
	// we add a little more type conversion, but a lot less confusion float m_fBrightness;// 0...1

	float m_fBounceFactor;// 0(stick)...1(bounce); -1 = slide (with friction)
	float m_fFriction;// 0(none)...1(stop), only if m_fBounceFactor == -1

	short m_iRenderMode;// standard engine render mode
	short m_iRenderEffects;// undone

	uint16 m_iDestroyContents;
	uint16 m_iDrawContents;// draw only in specified type of environment contents (CONTENTS_WATER, etc.)
	uint16 m_iDoubleSided;// XDM3037: draw backfaces (glCullFace)
	uint16 m_iFxLevel;// level of user setting at which this system should be drawn

	uint32 m_iFlags;// RS flags, assigned by manager AFTER constructor, so don't rely on this too early!

	uint32 m_iFollowFlags;
	int m_iFollowEntity;// is it a bad idea to store entity pointer?
	int m_iFollowModelIndex;// in case we're following entity that is not sent to client
	int m_iFollowAttachment;// [0 <= x < MAXSTUDIOATTACHMENTS) is a valid index and will be processed!
	//int m_iFollowBoneIndex;//follow specified model bone?
	cl_entity_t *m_pLastFollowedEntity;

	struct model_s *m_pTexture;
	char m_szName[RENDERSYSTEM_USERNAME_LENGTH];// NON-UNIQUE, name that can be given by a user

	void *m_pOnUpdateData;// will be passed as pData argument into the callback function
	bool (*m_OnUpdate)(CRenderSystem *pSystem, void *pData, const float &time, const double &elapsedTime);
protected:
	RS_INDEX index;// UNIQUE
	char m_UUID[RENDERSYSTEM_UID_LENGTH];// UNIQUE, automatically generated in most cases
	uint32 m_iState;// enum! Set by the manager: is this system active, should it be updated by the manager?
	uint16 m_ShuttingDown;// system is going to shut down but may be waiting for all particles to emit (soft shut down)
	uint16 m_RemoveNow;// tell manager to destroy this system ASAP, use in constructors to prevent system initialization

	CRenderSystem *m_pNext;
};

#endif // RENDERSYSTEM_H
