/***
*
*	Copyright(c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc.("Id Technology").  Id Technology(c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#ifndef BASEMONSTER_H
#define BASEMONSTER_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

//#define _DEBUG_MONSTERS	1

// This works even in release builds
#if defined (_DEBUG_MONSTERS)
#define DBG_MON_PRINTF	DBG_PrintF
#else
#define DBG_MON_PRINTF
#endif


typedef enum scriptstates_e
{
	SCRIPT_PLAYING = 0,		// Playing the sequence
	SCRIPT_WAIT,			// Waiting on everyone in the script to be ready
	SCRIPT_CLEANUP,			// Cancelling the script / cleaning up
	SCRIPT_WALK_TO_MARK,
	SCRIPT_RUN_TO_MARK,
} SCRIPTSTATE;

typedef enum monsterstates_e
{
	MONSTERSTATE_NONE = 0,
	MONSTERSTATE_IDLE,
	MONSTERSTATE_COMBAT,
	MONSTERSTATE_ALERT,
	MONSTERSTATE_HUNT,
	MONSTERSTATE_PRONE,
	MONSTERSTATE_SCRIPT,
	MONSTERSTATE_PLAYDEAD,
	MONSTERSTATE_DEAD
} MONSTERSTATE;

#define	ROUTE_SIZE				8 // how many waypoints a monster can store at one time
#define MAX_OLD_ENEMIES			4 // how many old enemies to remember
#define VISIBLE_ENEMY_MAX_DIST	8192// XDM3037a

// Entity game flags, some used by monsters
#define EGF_SPAWNED				(1 << 0)
#define EGF_DIED				(1 << 1)
#define EGF_RESPAWNED			(1 << 2)
#define EGF_JOINEDGAME			(1 << 3)// Player have been playing at least once (never clear this!)
#define EGF_SPECTATED			(1 << 4)// Player have been a spectator
#define EGF_SPECTOGGLED			(1 << 5)// Player have switched to spectator mode manually
#define EGF_PRESSEDREADY		(1 << 6)// Ready buttons pressed (clearable)
#define EGF_ADDEDDEFITEMS		(1 << 7)// AddDefault() was called for this player
#define EGF_DROPPEDITEMS		(1 << 8)
#define EGF_REACHEDGOAL			(1 << 9)// for co-op mostly

// Capabilities
#define	bits_CAP_DUCK			(1 << 0 )// crouch
#define	bits_CAP_JUMP			(1 << 1 )// jump/leap
#define bits_CAP_STRAFE			(1 << 2 )// strafe ( walk/run sideways)
#define bits_CAP_SQUAD			(1 << 3 )// can form squads
#define	bits_CAP_SWIM			(1 << 4 )// proficiently navigate in water
#define bits_CAP_CLIMB			(1 << 5 )// climb ladders/ropes
#define bits_CAP_USE			(1 << 6 )// open doors/push buttons/pull levers
#define bits_CAP_HEAR			(1 << 7 )// can hear forced sounds
#define bits_CAP_AUTO_DOORS		(1 << 8 )// can trigger auto doors
#define bits_CAP_OPEN_DOORS		(1 << 9 )// can open manual doors
#define bits_CAP_TURN_HEAD		(1 << 10)// can turn head, always bone controller 0
#define bits_CAP_RANGE_ATTACK1	(1 << 11)// can do a range attack 1
#define bits_CAP_RANGE_ATTACK2	(1 << 12)// can do a range attack 2
#define bits_CAP_MELEE_ATTACK1	(1 << 13)// can do a melee attack 1
#define bits_CAP_MELEE_ATTACK2	(1 << 14)// can do a melee attack 2
#define bits_CAP_FLY			(1 << 15)// can fly, move all around

#define bits_CAP_DOORS_GROUP    (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)


class CCineMonster;
class CSound;

//
// Generic Monster class
//
// WARNING: engine often depends on FL_MONSTER flag set!
// WARNING: don't use pev->dmg for monsters/player because engine may modify it!
//
class CBaseMonster : public CBaseToggle
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Precache(void);
	virtual void Spawn(void);
	virtual CBaseEntity *StartRespawn(void);// XDM3038c

// monster use function
	void EXPORT MonsterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	//void EXPORT CorpseUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

// overrideable Monster member functions
	virtual int Classify(void) { return m_iClass; }
	virtual int BloodColor(void) { return m_bloodColor; }
	virtual int IRelationship(CBaseEntity *pTarget);// XDM3038c
	virtual Vector EyePosition(void);// XDM3038c

//	virtual int IRelationship(CBaseEntity *pTarget);
	virtual CBaseMonster *MyMonsterPointer(void) { return this; }
	virtual void Look(int iDistance);// basic sight function for monsters
	virtual void RunAI(void);// core ai function!
	void Listen(void);

	virtual bool ShouldFadeOnDeath(void) const;

// Basic Monster AI functions
	virtual void SetYawSpeed(void) {}// allows different yaw_speeds for each activity
	virtual float ChangeYaw(float yawSpeed);
	vec_t VecToYaw(const Vector &vecDir);
	float FlYawDiff(void);
//	virtual float DamageForce(float damage);

// stuff written for new state machine
	virtual void MonsterThink(void);
	void EXPORT	CallMonsterThink(void);
	virtual void MonsterInit(void);
	virtual void MonsterInitDead(void);	// Call after animation/pose is set up
	virtual void BecomeDead(void);
	virtual void RunPostDeath(void);// XDM3037

	void EXPORT CorpseFallThink(void);
	void EXPORT MonsterInitThink(void);

	virtual void StartMonster(void);
	virtual CBaseEntity* BestVisibleEnemy(void);// finds best visible enemy for attack
	virtual bool FInViewCone(CBaseEntity *pEntity);// see if pEntity is in monster's view cone
	virtual bool FInViewCone(const Vector &origin);// see if given location is in monster's view cone
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);

	virtual int CheckLocalMove(const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist);// check validity of a straight move through space
	virtual void Move(float flInterval = 0.1);
	virtual void MoveExecute(CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval);
	virtual BOOL ShouldAdvanceRoute(float flWaypointDist);

	virtual Activity GetStoppedActivity(void) { return ACT_IDLE; }
	virtual void Stop(void);

	// these functions will survey conditions and set appropriate conditions bits for attack types.
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack2(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);
	virtual float GetTaskDelay(int iTask) { return 0.5f; };// XDM3038c

	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual Schedule_t *GetSchedule(void);
	virtual void ScheduleChange(void) {}
	// virtual int CanPlaySequence(void) { return((m_pCine == NULL) &&(m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE)); }
	virtual int CanPlaySequence(BOOL fDisregardState, int interruptLevel);
	virtual int CanPlaySentence(BOOL fDisregardState) { return IsAlive(); }
	virtual void PlaySentence(const char *pszSentence, float duration, float volume, float attenuation);
	virtual void PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener);
	virtual void SentenceStop(void);

	BOOL FHaveSchedule(void);
	BOOL FScheduleValid(void);
	void ClearSchedule(void);
	BOOL FScheduleDone(void);
	void ChangeSchedule(Schedule_t *pNewSchedule);
	void NextScheduledTask(void);
	Schedule_t *ScheduleInList(const char *pName, Schedule_t **pList, int listCount);

	virtual Schedule_t *ScheduleFromName(const char *pName);
	static Schedule_t *m_scheduleList[];

	void MaintainSchedule(void);
	Task_t *GetTask(void);
	virtual MONSTERSTATE GetIdealState(void);
	virtual void SetActivity(Activity NewActivity);
	virtual void ReportState(int printlevel);// XDM3038c

	void SetSequenceByName(char *szSequence);
	void SetMonsterState(MONSTERSTATE State);// XDM3035c: name confusion
	void CheckAttacks(CBaseEntity *pTarget, const vec_t &flDist);

	virtual int CheckEnemy(CBaseEntity *pEnemy);

	void PushEnemy(CBaseEntity *pEnemy, Vector &vecLastKnownPos);
	BOOL PopEnemy(void);

	void MovementComplete(void);
	int TaskIsRunning(void);
	inline void TaskComplete(void) { if(!HasConditions(bits_COND_TASK_FAILED)) m_iTaskStatus = TASKSTATUS_COMPLETE; }
	inline void TaskFail(void) { SetConditions(bits_COND_TASK_FAILED); }
	inline void TaskBegin(void) { m_iTaskStatus = TASKSTATUS_RUNNING; }
	inline int TaskIsComplete(void) { return(m_iTaskStatus == TASKSTATUS_COMPLETE); }
	inline int MovementIsComplete(void) { return(m_movementGoal == MOVEGOAL_NONE); }
	// This will stop animation until you call ResetSequenceInfo() at some point in the future
	inline void StopAnimation(void) { pev->framerate = 0; }

	int IScheduleFlags(void);
	bool FGetNodeRoute(const Vector &vecDest);
	bool FRefreshRoute(void);
	bool FRouteClear(void);
	void RouteSimplify(CBaseEntity *pTargetEnt);
	void AdvanceRoute(float distance);
	virtual BOOL FTriangulate(const Vector &vecStart, const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex);
	void MakeIdealYaw(const Vector &vecTarget);
	bool BuildRoute(const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget);
	virtual bool BuildNearestRoute(const Vector &vecThreat, const Vector &vecViewOffset, float flMinDist, float flMaxDist);
	int RouteClassify(int iMoveFlag);
	void InsertWaypoint(const Vector &vecLocation, const int &afMoveFlags);

	BOOL FindLateralCover(const Vector &vecThreat, const Vector &vecViewOffset);
	virtual bool FindCover(const Vector &vecThreat, const Vector &vecViewOffset, float flMinDist, float flMaxDist);
	virtual bool FValidateCover(const Vector &vecCoverLocation) { return true; }
	virtual float CoverRadius(void) { return 784; } // Default cover radius

	virtual bool NoFriendlyFire(bool playerAlly = false) { return true; }// XDM
	virtual bool FCanCheckAttacks(void);
	virtual void CheckAmmo(void) {}
	virtual int IgnoreConditions(void);

	inline void	SetConditions(int iConditions) { m_afConditions |= iConditions; }
	inline void	ClearConditions(int iConditions) { m_afConditions &= ~iConditions; }
	inline bool HasConditions(int iConditions) { return FBitSet(m_afConditions, iConditions); }
	inline bool HasAllConditions(int iConditions) { return ((m_afConditions & iConditions) == iConditions); }

	virtual bool FValidateHintType(short sHint);
	int FindHintNode(void);
	virtual bool FCanActiveIdle(void);
	void SetTurnActivity(void);
	float FLSoundVolume(CSound *pSound);

	BOOL MoveToNode(Activity movementAct, float waitTime, const Vector &goal);
	BOOL MoveToTarget(Activity movementAct, float waitTime);
	BOOL MoveToLocation(Activity movementAct, float waitTime, const Vector &goal);
	BOOL MoveToEnemy(Activity movementAct, float waitTime);

	// Returns the time when the door will be open
	float OpenDoorAndWait(CBaseEntity *pDoor);

	virtual int ISoundMask(void);
	virtual CSound *PBestSound(void);
	virtual CSound *PBestScent(void);
	virtual float HearingSensitivity(void) { return 1.0f; }

	virtual float FallDamage(const float &flFallVelocity);// XDM3035c

	virtual bool FBecomeProne(void);// XDM3038: virtual
	virtual void BarnacleVictimBitten(CBaseEntity *pBarnacle);
	virtual void BarnacleVictimReleased(void);

	void SetEyePosition(void);

	bool FShouldEat(void) const;// see if a monster is 'hungry'
	void Eat(float flFullDuration);// make the monster 'full' for a while.

	CBaseEntity *CheckTraceHullAttack(const float &flDist, const float &fDamage, const int &iDmgType);
	bool FacingIdeal(void);
	bool FCheckAITrigger(void);// checks and, if necessary, fires the monster's trigger target.
	BOOL BBoxFlat(void);

	virtual void PrescheduleThink(void) {}

	// combat functions
	BOOL GetEnemy(void);

	virtual Activity GetDeathActivity(void);
	Activity GetSmallFlinchActivity(void);
	BOOL ExitScriptedSequence();
	BOOL CineCleanup();

	Vector ShootAtEnemy(const Vector &shootOrigin);

	virtual Vector BodyTarget(const Vector &posSrc);
	virtual	Vector GetGunPosition(void);

	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual float TakeHealth(const float &flHealth, const int &bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int GetScoreAward(void) { return m_iScoreAward; }// XDM3038c

	void RouteClear(void);
	void RouteNew(void);

	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// NEW
	virtual bool GibMonster(void);
	virtual bool ShouldGibMonster(int iGib) const;
	virtual void CallGibMonster(void);
	virtual bool HasHumanGibs(void);
	virtual bool HasAlienGibs(void);
	virtual void FadeMonster(void);	// Called instead of GibMonster() when gibs are disabled

	virtual void DeathSound(void);
	virtual void AlertSound(void) {}
	virtual void IdleSound(void) {}
	virtual void PainSound(void) {}
	virtual void StopFollowing(BOOL clearSchedule) {}

	inline void	Remember(int iMemory) { m_afMemory |= iMemory; }
	inline void	Forget(int iMemory) { m_afMemory &= ~iMemory; }
	inline bool HasMemory(int iMemory) { return (m_afMemory & iMemory) != 0; }
	inline bool HasAllMemories(int iMemory) { return ((m_afMemory & iMemory) == iMemory); }

	// XDM
	void StartPatrol(CBaseEntity *path);
	virtual int GetSoundPitch(void) { return m_voicePitch; }
	virtual float GetSoundVolume(void) { return VOL_NORM; }

	virtual bool IsMoving(void) const { return (m_movementGoal != MOVEGOAL_NONE); }
	virtual	bool IsMonster(void) const { return true; }
	virtual bool IsBSPModel(void) const { return false; }
	virtual bool IsPushable(void) { return true; }
	virtual bool IsHeavyDamage(float flDamage, int bitsDamageType) const;// FALSE means light damage
	virtual bool IsHuman(void);// conflicts const;
	virtual bool IsTalkMonster(void) const { return false; }// XDM3038
	virtual bool HasTarget(string_t targetname);// XDM3035c
	virtual bool ShouldRespawn(void) const;// XDM3035
	virtual bool ShowOnMap(CBasePlayer *pPlayer) const { return true; }// XDM3038c

	virtual void FrozenStart(float freezetime);
	virtual void FrozenEnd(void);
	virtual void FrozenThink(void);
	virtual bool CanAttack(void);

	virtual const char *GetDropItemName(void) { return NULL; }// XDM3038: WARNING: must always return the same classname, because it is used in Precache()!

	virtual CBaseEntity *DropItem(const char *pszItemName, const Vector &vecPos, const Vector &vecAng);// drop an item.

	Vector		m_vFrozenViewAngles;	// XDM: can't look around
	float		m_flUnfreezeTime;		// XDM: for a long time?
	string_t	m_iszGibModel;// XDM: custom gib model
	int			m_iGibModelIndex;
	uint32		m_iGibCount;// XDM3038c: uint
	BOOL		m_fFrozen;				// XDM: is the player frozen by fgrenade?
	BOOL		m_fFreezeEffect;		// XDM: used by glowshell effect.
	int			m_voicePitch;
	int			m_iClass;// XDM: custom class,
	//float		m_flRespawnTime;// XDM3035

	// these fields have been added in the process of reworking the state machine.(sjb)
	EHANDLE				m_hEnemy;		 // the entity that the monster is fighting.
	EHANDLE				m_hTargetEnt;	 // the entity that the monster is trying to reach
	EHANDLE				m_hOldEnemy[MAX_OLD_ENEMIES];
	Vector				m_vecOldEnemy[MAX_OLD_ENEMIES];

	float				m_flFieldOfView;// width of monster's field of view(dot product)
	float				m_flWaitFinished;// if we're told to wait, this is the time that the wait will be over.
	float				m_flMoveWaitFinished;

	Activity			m_Activity;// what the monster is doing(animation)
	Activity			m_IdealActivity;// monster should switch to this activity

	int					m_LastHitGroup; // the last body region that took damage

	MONSTERSTATE		m_MonsterState;// monster's current state
	MONSTERSTATE		m_IdealMonsterState;// monster should change to this state

	int					m_iTaskStatus;
	Schedule_t			*m_pSchedule;
	int					m_iScheduleIndex;

	WayPoint_t			m_Route[ROUTE_SIZE];	// Positions of movement
	int					m_movementGoal;			// Goal that defines route
	int					m_iRouteIndex;			// index into m_Route[]
	float				m_moveWaitTime;			// How long I should wait for something to move

	Vector				m_vecMoveGoal; // kept around for node graph moves, so we know our ultimate goal
	Activity			m_movementActivity;	// When moving, set this activity

	int					m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
	int					m_afSoundTypes;

	Vector				m_vecLastPosition;// monster sometimes wants to return to where it started after an operation.

	int					m_iHintNode; // this is the hint node that the monster is moving towards or performing active idle on.
	int					m_afMemory;
	//int					m_iMaxHealth;// keeps track of monster's maximum health value(for re-healing, etc)
	Vector				m_vecEnemyLKP;// last known position of enemy.(enemy's origin)
	int					m_cAmmoLoaded;		// how much ammo is in the weapon(used to trigger reload anim sequences)
	int					m_afCapability;// tells us what a monster can/can't do.
	float				m_flNextAttack;		// cannot attack again until this time
	int					m_bitsDamageType;	// what types of damage has monster(player) taken
	byte				m_rgbTimeBasedDamage[CDMG_TIMEBASED];
	int					m_lastDamageAmount;// how much damage did monster(player) last take
											// time based damage counters, decr. 1 per 2 seconds
	int					m_bloodColor;		// color of blood particless
	int					m_failSchedule;				// Schedule type to choose if current schedule fails
	float				m_flHungryTime;// set this is a future time to stop the monster from eating for a while.
	float				m_flDistTooFar;	// if enemy farther away than this, bits_COND_ENEMY_TOOFAR set in CheckEnemy
	float				m_flDistLook;	// distance monster sees(Default 2048)
	int					m_iTriggerCondition;// for scripted AI, this is the condition that will cause the activation of the monster's TriggerTarget
	string_t			m_iszTriggerTarget;// name of target that should be fired.
	Vector				m_HackedGunPos;	// HACK until we can query end of gun
// Scripted sequence Info
	SCRIPTSTATE			m_scriptState;		// internal cinematic state
	CCineMonster		*m_pCine;

	char				m_szSoundDir[MAX_ENTITY_STRING_LENGTH];// XDM3038c: custom sounds, set in Precache() because Spawn() is not always called.

	float				m_flFallVelocity;// XDM3035c: from player
	float				m_flDamage1;// XDM3038c
	float				m_flDamage2;// XDM3038c
	int					m_iScoreAward;// XDM3038c

	int					m_iGameFlags;// XDM3038c: centralized waay of storing game-related properties
	EHANDLE				m_hLastKiller;// XDM30308: don't save. EHANDLE because we need to invalidate disconnected players
	EHANDLE				m_hLastVictim;// same here

protected:
	int m_afConditions;
};

#endif // BASEMONSTER_H
