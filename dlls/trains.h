/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef TRAINS_H
#define TRAINS_H

// Macros to make entvars more user-friendly
#define noiseMovement noise
#define noiseStopMoving noise1
//#define noiseStartMoving noise2 // XDM

// Spawnflags of CPathCorner
#define SF_CORNER_WAITFORTRIG		0x00000001
#define SF_CORNER_TELEPORT			0x00000002
#define SF_CORNER_FIREONCE			0x00000004
#define SF_CORNER_INTERPOLATE		0x00000008
#define SF_CORNER_AVELOCITY			0x00800000// XDM

class CPathCorner : public CBaseDelay// XDM3038c
{
public:
	virtual void Spawn(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual float GetDelay(void) { return m_flWait; }
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// XDM3038c
	void FireOnPass(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// XDM3038c
	//virtual void Touch(CBaseEntity *pOther);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

private:
	float	m_flWait;
};


// Spawnflag for CPathTrack
#define SF_PATH_DISABLED			0x00000001
#define SF_PATH_FIREONCE			0x00000002
#define SF_PATH_ALTREVERSE			0x00000004
#define SF_PATH_DISABLE_TRAIN		0x00000008
#define SF_PATH_DONTROTATE			0x00000010// XDM
#define SF_PATH_INTERPOLATE			0x00000020// XDM
#define SF_PATH_ALTERNATE			0x00008000
#define SF_PATH_AVELOCITY			0x00080000// XDM

// SHL - values in 'armortype'
#define PATHSPEED_SET			0
#define PATHSPEED_ACCEL			1
#define PATHSPEED_TIME			2
#define PATHSPEED_SET_MASTER	3

// SHL - values in 'frags'
#define PATHTURN_SET			0
#define PATHTURN_SET_MASTER		1
#define PATHTURN_RESET			2

// SHL - values in 'armorvalue'
#define PATHMATCH_NO			0
#define PATHMATCH_YES			1
#define PATHMATCH_TRACK			2

//#define PATH_SPARKLE_DEBUG		1	// This makes a particle effect around path_track entities for debugging
class CPathTrack : public CBaseDelay//CPointEntity // XDM3038c: not CPathCorner because of methods conflicts and pev->targetname requirements
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Activate(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void FireOnPass(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// XDM3038c

	void Link(void);
	void Project(CPathTrack *pStart, CPathTrack *pEnd, Vector *origin, float dist);
	CPathTrack *ValidPath(CPathTrack *pPath, int testFlag);		// Returns ppath if enabled, NULL otherwise
	CPathTrack *LookAhead(Vector *origin, float dist, int move);
	CPathTrack *Nearest(const Vector &origin);
	CPathTrack *GetNext(void);
	CPathTrack *GetPrevious(void);
	void SetPrevious(CPathTrack *pprevious);

	Vector GetOrientation(bool bForwardDir);
	CPathTrack *GetNextInDir(bool bForward);
	static CPathTrack *Instance(edict_t *pent);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	
	static	TYPEDESCRIPTION m_SaveData[];
#if defined (PATH_SPARKLE_DEBUG)
	void EXPORT Sparkle(void);
#endif

	float		m_length;
	string_t	m_altName;
	CPathTrack	*m_pnext;
	CPathTrack	*m_pprevious;
	CPathTrack	*m_paltpath;
};



#define TRAIN_STARTPITCH	60
#define TRAIN_MAXPITCH		200
#define TRAIN_MAXSPEED		1000// approx max speed for sound pitch calculation


#define SF_PLAT_TOGGLE		0x0001
#define SF_PLAT_PASSABLE	0x0008// XDM3037a

class CBasePlatTrain : public CBaseToggle
{
public:
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Precache(void);
	// This is done to fix spawn flag collisions between this class and a derived class
	virtual bool IsTogglePlat(void) const { return FBitSet(pev->spawnflags, SF_PLAT_TOGGLE); }
	virtual bool IsBSPModel(void) const { return true; }// XDM
	virtual bool IsMovingBSP(void) const { return true; }// XDM
	virtual void StartSound(void);// XDM
	virtual void StopSound(void);// XDM
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	static const char *StartSounds[];// XDM
	static const char *MoveSounds[];// XDM
	static const char *StopSounds[];// XDM
	byte m_bMoveSnd;			// sound a plat makes while moving
	byte m_bStopSnd;			// sound a plat makes when it stops
	float m_volume;			// Sound volume
	float m_flAttenuation;// attenuation value
	BOOL m_soundPlaying;
};


class CFuncPlat : public CBasePlatTrain
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Blocked(CBaseEntity *pOther);
	virtual void GoUp(void);
	virtual void GoDown(void);
	virtual void HitTop(void);
	virtual void HitBottom(void);
	void Setup(void);
	void EXPORT PlatUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT CallGoDown(void) { GoDown(); }
	void EXPORT CallHitTop(void) { HitTop(); }
	void EXPORT CallHitBottom(void) { HitBottom(); }
};

// UNDONE: Need to save this!!! It needs class & linkage
class CPlatTrigger : public CBaseEntity
{
public:
	virtual int	ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
	virtual void Touch(CBaseEntity *pOther);
	void SetPlatform(CFuncPlat *pPlatform);
	CFuncPlat *m_pPlatform;
};

class CFuncPlatRot : public CFuncPlat
{
public:
	virtual void Spawn(void);
	virtual void SetupRotation(void);
	virtual void GoUp(void);
	virtual void GoDown(void);
	virtual void HitTop(void);
	virtual void HitBottom(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	void RotMove(const Vector &destAngle, float time);
	static TYPEDESCRIPTION m_SaveData[];
	Vector m_end, m_start;
};


// Trains
#define	SF_TRAIN_WAIT_RETRIGGER	0x0001// Internal use only!
//#define	SF_TRAIN_			0x0002
#define SF_TRAIN_START_ON		0x0004// Train is initially moving
#define SF_TRAIN_PASSABLE		0x0008// Train is not solid -- used to make water trains
//#define	SF_TRAIN_			0x0010
#define SF_TRAIN_BLOCKEDONCE	0x0020// XDM3038


class CFuncTrain : public CBasePlatTrain
{
public:
	virtual void Spawn(void);
	//virtual void Precache(void);
	virtual void Activate(void);
	virtual void OverrideReset(void);
	virtual void Blocked(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void KeyValue(KeyValueData *pkvd);
	//virtual Vector Center(void) { return (pev->mins + pev->maxs)* 0.5f; }// XDM: wrong
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	void EXPORT Wait(void);
	void EXPORT Next(void);
	static TYPEDESCRIPTION m_SaveData[];
	entvars_t *m_pevCurrentTarget;
	int m_sounds;
	BOOL m_activated;
	string_t m_iszLastTarget;
};


// Tracktrain spawn flags
#define SF_TRACKTRAIN_NOPITCH				0x000001
#define SF_TRACKTRAIN_NOCONTROL				0x000002
#define SF_TRACKTRAIN_FORWARDONLY			0x000004
#define SF_TRACKTRAIN_PASSABLE				0x000008
#define SF_TRACKTRAIN_NOYAW					0x000010	// SHL
//#define SF_TRACKTRAIN_BLOCKEDONCE			0x000020	// XDM3038
#define SF_TRACKTRAIN_STOPUNCONTROLLED		0x000040	// XDM3038
#define SF_TRACKTRAIN_AVELOCITY				0x800000	// SHL - avelocity has been set manually, don't turn.
#define SF_TRACKTRAIN_AVEL_GEARS			0x400000	// SHL - avelocity should be scaled up/down when the train changes gear.

class CFuncTrackTrain : public CBaseDelay// CFuncTrain?
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int ObjectCaps(void) { return (CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DIRECTIONAL_USE; }// XDM3038
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Blocked(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Think(void);// XDM3038a
	virtual bool IsMovingBSP(void) const { return true; }// XDM
	virtual bool OnControls(entvars_t *pevTest);
	virtual void StopSound(void);
	virtual void UpdateSound(void);
	virtual void OverrideReset(void);
	virtual void PrepareForTransfer(void);// XDM3037
	virtual void DesiredAction(void);// XDM3037: SHL
	void NextThink(float thinkTime, BOOL alwaysThink);
	void SetControls(entvars_t *pevControls);
	void SetTrack(CPathTrack *track) { m_ppath = track->Nearest(pev->origin); }

	//void EXPORT TrackNext(void);// XDM3035: ambiguity prevention
	void EXPORT PostponeNext(void);
	void EXPORT Find(void);
	void EXPORT NearestPath(void);
	void EXPORT DeadEnd(void);

	static CFuncTrackTrain *Instance(edict_t *pent);

	static const char *MoveSounds[];// XDM
	static	TYPEDESCRIPTION m_SaveData[];
	CPathTrack	*m_ppath;
	float		m_length;
	float		m_height;
// XDM3038: OBSOLETE	float		m_speed;
	float		m_dir;
	float		m_startSpeed;
	Vector		m_controlMins;
	Vector		m_controlMaxs;
	int			m_soundPlaying;
	int			m_sounds;
	float		m_flVolume;
	float		m_flBank;
	float		m_oldSpeed;

protected:
	unsigned short m_usAdjustPitch;
};



// ----------------------------------------------------------------------------
//
// Track changer / Train elevator
//
// ----------------------------------------------------------------------------
#define SF_TRACK_ACTIVATETRAIN		0x00000001
#define SF_TRACK_RELINK				0x00000002
#define SF_TRACK_ROTMOVE			0x00000004
#define SF_TRACK_STARTBOTTOM		0x00000008
#define SF_TRACK_DONT_MOVE			0x00000010

//
// This entity is a rotating/moving platform that will carry a train to a new track.
// It must be larger in X-Y planar area than the train, since it must contain the
// train within these dimensions in order to operate when the train is near it.
//
typedef enum train_codes_e
{
	TRAIN_SAFE,
	TRAIN_BLOCKING,
	TRAIN_FOLLOWING
} TRAIN_CODE;

class CFuncTrackChange : public CFuncPlatRot
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	//virtual void Blocked(void);
	virtual void KeyValue(KeyValueData* pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Touch(CBaseEntity *pOther);
	virtual void EXPORT GoUp(void);
	virtual void EXPORT GoDown(void);
	virtual void HitBottom(void);
	virtual void HitTop(void);
	virtual void UpdateAutoTargets( int toggleState );
	virtual	bool IsTogglePlat(void) const { return true; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
	virtual void OverrideReset(void);

	TRAIN_CODE EvaluateTrain( CPathTrack *pcurrent );
	void UpdateTrain( Vector &dest );
	void EXPORT Find(void);
	void DisableUse(void) { m_use = 0; }
	void EnableUse(void) { m_use = 1; }
	int UseEnabled(void) { return m_use; }

	CPathTrack		*m_trackTop;
	CPathTrack		*m_trackBottom;
	CFuncTrackTrain	*m_train;
	int				m_trackTopName;
	int				m_trackBottomName;
	int				m_trainName;
	TRAIN_CODE		m_code;
	int				m_targetState;
	int				m_use;
};

#endif // TRAINS_H
