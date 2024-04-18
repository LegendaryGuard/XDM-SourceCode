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
//=========================================================
// GMan - misunderstood servant of the people
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "talkmonster.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGMan : public CTalkMonster// XDM3038c: was CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int Classify (void);
	virtual int ISoundMask (void);

	virtual int	Save(CSave &save); 
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	virtual void StartTask( Task_t *pTask );
	virtual void RunTask( Task_t *pTask );
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);

	virtual void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );
	virtual void MonsterThink(void);// XDM
	virtual bool IsPushable(void) { return false; }// XDM3038a: prevents glitches in scripted scenes

	void ShowDamage(void);

	EHANDLE m_hPlayer;
//	EHANDLE m_hTalkTarget;// now in CTalkMonster
	float m_flTalkTime;
};
LINK_ENTITY_TO_CLASS( monster_gman, CGMan );


TYPEDESCRIPTION	CGMan::m_SaveData[] = 
{
//	DEFINE_FIELD( CGMan, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CGMan, m_flTalkTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CGMan, CTalkMonster );


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGMan :: Classify (void)
{
	return CLASS_NONE;// CLASS_HUMAN_MONSTER? :D
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGMan :: SetYawSpeed (void)
{
	pev->yaw_speed = 90;
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGMan :: ISoundMask (void)
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGMan :: Spawn()
{
	if (pev->health <= 0)
		pev->health = 100;
	if (pev->armorvalue <= 0)
		pev->armorvalue = 100;
	if (m_iScoreAward == 0)
		m_iScoreAward = 10000;// WTF :)
	if (m_bloodColor == 0)
		m_bloodColor = DONT_BLEED;
	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;

	CTalkMonster::Spawn();// XDM3038b: Precache();

	UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGMan :: Precache()
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/gman.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "gman");

	CTalkMonster::Precache();// XDM3038a
}	


//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CGMan :: StartTask( Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_WAIT:
		if (m_hPlayer.Get() == NULL)
		{
			m_hPlayer = (CBaseEntity *)FindNearestPlayer(EyePosition());// XDM3038c UTIL_FindEntityByClassname( NULL, "player" );
		}
		break;
	}
	CTalkMonster::StartTask( pTask );
}

void CGMan :: RunTask( Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_WAIT:
		// look at who I'm talking to
		if (m_flTalkTime > gpGlobals->time && m_hTalkTarget != NULL)
		{
			vec_t yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;
			NormalizeAngle180(&yaw);// XDM3038
			// turn towards vector
			SetBoneController( 0, yaw );
		}
		// look at player, but only if playing a "safe" idle animation
		else if (m_hPlayer != NULL && pev->sequence == 0)
		{
			vec_t yaw = VecToYaw(m_hPlayer->pev->origin - pev->origin) - pev->angles.y;
			NormalizeAngle180(&yaw);// XDM3038
			// turn towards vector
			SetBoneController( 0, yaw );
		}
		else
		{
			SetBoneController( 0, 0 );
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		SetBoneController( 0, 0 );
		CTalkMonster::RunTask( pTask );
		break;
	}
}


//=========================================================
// Override all damage
//=========================================================
int CGMan::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	pev->health = pev->max_health / 2; // always trigger the 50% damage aitrigger

	if (flDamage >= 20)// XDM: fixed
		SetConditions(bits_COND_HEAVY_DAMAGE);
	else if (flDamage > 0)
		SetConditions(bits_COND_LIGHT_DAMAGE);

	if (flDamage > 0.0f)
		ShowDamage();

	return 1;
}


void CGMan::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (/*ptr->iHitgroup == HITGROUP_ARMOR && */FBitSet(bitsDamageType, DMGM_RICOCHET) && !FBitSet(bitsDamageType, DMG_NOHITPOINT|DMG_IGNOREARMOR))// XDM3038c
		UTIL_Ricochet(ptr->vecEndPos, 1.0);

	// XDM: purpose?	AddMultiDamage(pAttacker, this, flDamage, bitsDamageType);
	CTalkMonster::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);// XDM3038c
}


void CGMan::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	CTalkMonster::PlayScriptedSentence( pszSentence, duration, volume, attenuation, bConcurrent, pListener );

	m_flTalkTime = gpGlobals->time + duration;
	m_hTalkTarget = pListener;
}

void CGMan::MonsterThink(void)// XDM
{
	if (pev->renderfx != 0 && pev->dmgtime < gpGlobals->time)
	{
		pev->renderfx = 0;
		pev->rendercolor.Set(255,255,255);// XDM3037a
	}

	CTalkMonster::MonsterThink();
}	

void CGMan::ShowDamage(void)
{
	pev->renderfx = kRenderFxGlowShell;// XDM
	pev->rendercolor.Set(143, 0, 255);
	pev->renderamt = 0;
	pev->dmgtime = gpGlobals->time + 2.0;
}
