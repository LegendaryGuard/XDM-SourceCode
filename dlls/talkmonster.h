/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#ifndef TALKMONSTER_H
#define TALKMONSTER_H

#ifndef MONSTERS_H
#include "monsters.h"
#endif

//=========================================================
// Talking monster base class
// Used for scientists and barneys
//=========================================================

#define TALKRANGE_MIN 500.0				// don't talk to anyone farther away than this

#define TLK_STARE_DIST	128				// anyone closer than this and looking at me is probably staring at me.

#define bit_saidDamageLight		(1<<0)	// bits so we don't repeat key sentences
#define bit_saidDamageMedium	(1<<1)
#define bit_saidDamageHeavy		(1<<2)
#define bit_saidHelloPlayer		(1<<3)
#define bit_saidWoundLight		(1<<4)
#define bit_saidWoundHeavy		(1<<5)
#define bit_saidHeard			(1<<6)
#define bit_saidSmelled			(1<<7)
#define bit_saidPlayerDied		(1<<8)// XDM3038a

#define TLK_CFRIENDS		3

#define DEFAULT_TLK_ANSWER		"ANSWER"
#define DEFAULT_TLK_QUESTION	"QUESTION"
#define DEFAULT_TLK_IDLE		"IDLE"
#define DEFAULT_TLK_STARE		"STARE"
#define DEFAULT_TLK_USE			"USE"
#define DEFAULT_TLK_UNUSE		"UNUSE"
#define DEFAULT_TLK_STOP		"STOP"
#define DEFAULT_TLK_NOSHOOT		"NOSHOOT"
#define DEFAULT_TLK_HELLO		"HELLO"
#define DEFAULT_TLK_PLHURT1		"CUREA"// sentence names can't end with digits
#define DEFAULT_TLK_PLHURT2		"CUREB"
#define DEFAULT_TLK_PLHURT3		"CUREC"
#define DEFAULT_TLK_PHELLO		"PHELLO"
#define DEFAULT_TLK_PIDLE		"PIDLE"
#define DEFAULT_TLK_PQUESTION	"PQUEST"
#define DEFAULT_TLK_SMELL		"SMELL"
#define DEFAULT_TLK_WOUND		"WOUND"
#define DEFAULT_TLK_MORTAL		"MORTAL"
#define DEFAULT_TLK_DECLINE		"POK"
#define DEFAULT_TLK_HEAR		"HEAR"
#define DEFAULT_TLK_KILL		"KILL"
#define DEFAULT_TLK_SHOT		"SHOT"
#define DEFAULT_TLK_FEAR		"FEAR"
#define DEFAULT_TLK_FEARPLAYER	"PLFEAR"
#define DEFAULT_TLK_PLDEAD		"PLDEAD"

enum talkgroupnames_e
{
	TLK_ANSWER = 0,
	TLK_QUESTION,
	TLK_IDLE,
	TLK_STARE,
	TLK_USE,
	TLK_UNUSE,
	TLK_STOP,
	TLK_NOSHOOT,// someone hurting my friend
	TLK_HELLO,
	TLK_PHELLO,
	TLK_PIDLE,
	TLK_PQUESTION,
	TLK_PLHURT1,
	TLK_PLHURT2,
	TLK_PLHURT3,
	TLK_SMELL,
	TLK_WOUND,
	TLK_MORTAL,
	TLK_DECLINE,// SHL: decline following (busy)
	TLK_HEAR,// XDM: heard suspicious sound
	TLK_KILL,// XDM: killed an enemy
	TLK_SHOT,// XDM: shot by a friend (player)
	TLK_FEAR,// XDM: enemy is nearby
	TLK_FEARPLAYER,// XDM: player is my enemy and is nearby
	TLK_PLDEAD,// XDM: i see player dying
	TLK_CGROUPS// MUST be last entry
};// TALKGROUPNAMES;


enum talkmonster_schedules_e
{
	SCHED_CANT_FOLLOW = LAST_COMMON_SCHEDULE + 1,
	SCHED_MOVE_AWAY,		// Try to get out of the player's way
	SCHED_MOVE_AWAY_FOLLOW,	// same, but follow afterward
	SCHED_MOVE_AWAY_FAIL,	// Turn back toward player
	SCHED_HEAR_SOUND,		// XDM3038a

	LAST_TALKMONSTER_SCHEDULE,		// MUST be last
};

enum talkmonster_tasks_e
{
	TASK_CANT_FOLLOW = LAST_COMMON_TASK + 1,
	TASK_MOVE_AWAY_PATH,
	TASK_WALK_PATH_FOR_UNITS,

	TASK_TLK_RESPOND,		// say my response
	TASK_TLK_SPEAK,			// question or remark
	TASK_TLK_HELLO,			// Try to say hello to player
	TASK_TLK_HEADRESET,		// reset head position
	TASK_TLK_STOPSHOOTING,	// tell player to stop shooting friend
	TASK_TLK_STARE,			// let the player know I know he's staring at me.
	TASK_TLK_LOOK_AT_CLIENT,// faces player if not moving and not talking and in idle.
	TASK_TLK_CLIENT_STARE,	// same as look at client, but says something if the player stares.
	TASK_TLK_EYECONTACT,	// maintain eyecontact with person who I'm talking to
	TASK_TLK_IDEALYAW,		// set ideal yaw to face who I'm talking to
	TASK_TLK_HEARSOUND,		// XDM3038a: comment on hearing something
	TASK_FACE_PLAYER,		// Face the player

	LAST_TALKMONSTER_TASK,			// MUST be last
};

class CBasePlayer;

class CTalkMonster : public CBaseMonster
{
public:
	// Base Monster functions
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Precache(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// NEW
	virtual int IRelationship(CBaseEntity *pTarget);
	virtual int CanPlaySentence(BOOL fDisregardState);
	virtual void PlaySentence(const char *pszSentence, float duration, float volume, float attenuation);
	virtual void PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener);
	virtual bool IsTalkMonster(void) const { return true; }// XDM3038

	// AI functions
	virtual void SetActivity(Activity newActivity);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void ReportState(int printlevel);// XDM3038c

	virtual void PrescheduleThink(void);

	virtual void TalkInit(void);
	virtual void StopTalking(void) { SentenceStop(); }

	CBaseMonster	*FindNearestFriend(BOOL fPlayer);
	float			TargetDistance(void);

	// Conversations / communication
	virtual int GetSoundPitch(void) { return CBaseMonster::GetSoundPitch() + RANDOM_LONG(0,3); }
	virtual int FOkToSpeak(void);

	virtual void IdleRespond(void);
	virtual int FIdleSpeak(void);
	virtual int FIdleStare(void);
	virtual int FIdleHello(void);

	void			IdleHeadTurn(const Vector &vecFriend );
	void			TrySmellTalk(void);
	CBaseEntity		*EnumFriends( CBaseEntity *pentPrevious, int listNumber, BOOL bTrace );
	void			AlertFriends(void);
	void			ShutUpFriends(void);
	BOOL			IsTalking(void);
	void			Talk( float flDuration );	
	CBasePlayer		*FindNearestPlayer(const Vector &vCenter);// XDM3038c

	// For following
	virtual BOOL CanFollow(void);
	virtual void StopFollowing(BOOL clearSchedule);
	virtual void StartFollowing(CBaseEntity *pLeader);
	virtual void DeclineFollowing(void);
	BOOL			IsFollowing(void) { return m_hTargetEnt != NULL && m_hTargetEnt->IsPlayer(); }
	void			LimitFollowers( CBaseEntity *pPlayer, int maxFollowers );

	void EXPORT		FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual void	SetAnswerQuestion( CTalkMonster *pSpeaker );
	virtual int		FriendNumber( int arrayNumber )	{ return arrayNumber; }

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	static char *m_szFriends[TLK_CFRIENDS];		// array of friend names
	static float g_talkWaitTime;
	
	int			m_bitsSaid;						// set bits for sentences we don't want repeated
	int			m_nSpeak;						// number of times initiated talking
//	int			m_voicePitch;					// pitch of voice for this head
	const char	*m_szGrp[TLK_CGROUPS];			// sentence group names
	float		m_useTime;						// Don't allow +USE until this time
	string_t	m_iszUse;						// Custom +USE sentence group (follow)
	string_t	m_iszUnUse;						// Custom +USE sentence group (stop following)
	string_t	m_iszDecline;// XDM3038a: SHL compatibility
	string_t	m_iszSentencePrefix;// XDM3038a

	float		m_flLastSaidSmelled;// last time we talked about something that stinks
	float		m_flStopTalkTime;// when in the future that I'll be done saying this sentence.

	EHANDLE		m_hTalkTarget;	// who to look at while talking
	CUSTOM_SCHEDULES;
};


// Clients can push talkmonsters out of their way
#define		bits_COND_CLIENT_PUSH		( bits_COND_SPECIAL1 )
// Don't see a client right now.
#define		bits_COND_CLIENT_UNSEEN		( bits_COND_SPECIAL2 )


#endif		//TALKMONSTER_H
