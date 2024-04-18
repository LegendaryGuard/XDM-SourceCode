//=========================================================
// Shock Trooper
// This piece of shame is written by Ghoul.
//=========================================================
#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
//#include "talkmonster.h"
#include "soundent.h"
#include "globals.h"
//#include "customentity.h"
//#include "effects.h"
#include "sound.h"
//#include "gamerules.h"
//#include "pm_materials.h"
#include "weapons.h"
#include "lightp.h"
#include "hornet.h"


#define bits_COND_SHOCKTROOPER_NOFIRE	bits_COND_SPECIAL1

#define	SHOCKTROOPER_LIMP_HEALTH		30

#define	MELEE_DIST						70
#define	SHOCKRIFLE_DIST					4096
#define	SPORELAUNCHER_DIST				3072
#define	HORNETGUN_DIST					2048

#define	SPORE_GREN_SPEED				800
#define	SPORE_BALL_SPEED				1125

#define WPN_SHOCKRIFLE					(1 << 0)
#define WPN_SPOREGRENADE				(1 << 1)
#define WPN_SHOCKBALL_LAUNCHER			(1 << 2)
#define WPN_SPORELAUNCHER				(1 << 3)
#define WPN_HORNETGUN					(1 << 4)

enum
{
	BODY_GROUP = 0,
	GUN_GROUP
};

enum
{
	GUN_SHOCKRIFLE = 0,
	GUN_SPORELAUNCHER,
	GUN_HORNETGUN,
	GUN_NONE
};

enum
{
	BODY_NORMAL = 0,
	BODY_RED
};

#define SHOCKTROOPER_EYE_FRAMES 4

enum
{
	AE_KICK = 1,
	AE_BURST1,
	AE_GREN_THROW,
	AE_GREN_LAUNCH,
	AE_DROP_GUN,
	AE_CAUGHT_ENEMY
};

enum
{
	SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE = LAST_COMMON_SCHEDULE + 1,
	SCHED_SHOCKTROOPER_SWEEP,
	SCHED_SHOCKTROOPER_FOUND_ENEMY,
	SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY,
	SCHED_SHOCKTROOPER_TAKECOVER_FAILED,
	SCHED_SHOCKTROOPER_ELOF_FAIL,
	SCHED_SHOCKTROOPER_CHECK_SOUND
};

enum
{
	TASK_SHOCKTROOPER_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_SHOCKTROOPER_CHECK_FIRE
};

class CShockTrooper : public CSquadMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int	Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void SetYawSpeed(void);
	virtual int Classify(void);
	virtual int ISoundMask(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual bool FCanCheckAttacks(void);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack2(float flDot, float flDist);
	virtual void SetActivity(Activity NewActivity);
	virtual void StartTask(Task_t *pTask);
	virtual void RunTask(Task_t *pTask);

	virtual void AttackHitSound(void);
	virtual void AttackMissSound(void);
	virtual void AttackSound(void);
	virtual void AlertSound(void);
	virtual void DeathSound(void);
	virtual void PainSound(void);
	virtual void IdleSound (void);

	virtual Vector GetGunPosition(void);

	virtual void ShootShock(void);
	virtual void ShootShockBall(void);
	virtual void ShootSpore(void);
	virtual void ShootHornet(void);

	virtual void PrescheduleThink(void);
	virtual Schedule_t *GetSchedule(void);
	virtual Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int IRelationship(CBaseEntity *pTarget);

	void DropItems(void);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;

	static const char *pAlertSounds[];
	static const char *pIdleSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pSentenceSounds[];

	static const char *pAttackSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS( monster_shocktrooper, CShockTrooper );

TYPEDESCRIPTION	CShockTrooper::m_SaveData[] =
{
	DEFINE_FIELD( CShockTrooper, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CShockTrooper, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CShockTrooper, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CShockTrooper, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CShockTrooper, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CShockTrooper, m_fFirstEncounter, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CShockTrooper, CSquadMonster );

const char *CShockTrooper::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CShockTrooper::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CShockTrooper::pAttackSounds[] = 
{
	"shocktrooper/st_attack.wav",
};

const char *CShockTrooper::pSentenceSounds[] = 
{
	"shocktrooper/st_speak1.wav",
	"shocktrooper/st_speak2.wav",
	"shocktrooper/st_speak3.wav",
	"shocktrooper/st_speak4.wav",
	"shocktrooper/st_speak5.wav",
	"shocktrooper/st_speak6.wav",
	"shocktrooper/st_speak7.wav",
	"shocktrooper/st_speak8.wav",
};

const char *CShockTrooper::pAlertSounds[] = 
{
	"shocktrooper/st_alert1.wav",
	"shocktrooper/st_alert2.wav",
	"shocktrooper/st_alert3.wav",
};

const char *CShockTrooper::pIdleSounds[] = 
{
	"shocktrooper/st_idle1.wav",
	"shocktrooper/st_idle2.wav",
	"shocktrooper/st_idle3.wav",
};


const char *CShockTrooper::pPainSounds[] = 
{
	"shocktrooper/st_pain1.wav",
	"shocktrooper/st_pain2.wav",
	"shocktrooper/st_pain3.wav",
	"shocktrooper/st_pain4.wav",
};

const char *CShockTrooper::pDeathSounds[] = 
{
	"shocktrooper/st_die1.wav",
	"shocktrooper/st_die2.wav",
	"shocktrooper/st_die3.wav",
	"shocktrooper/st_die4.wav",
};

void CShockTrooper::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.shocktrooperHealth;
	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_YELLOW;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.shocktrooperScore;
	if (m_iGibCount == 0)
		m_iGibCount = 24;

	CSquadMonster::Spawn();
	UTIL_SetSize(this, Vector(-32, -32, 0), Vector(32, 32, 75));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_flFieldOfView		= 0.1;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck= gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD;
	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;

	if (pev->weapons == 0)
		pev->weapons = WPN_SHOCKRIFLE | WPN_SPOREGRENADE;

	if (FBitSet(pev->weapons, WPN_SPORELAUNCHER))
		SetBodygroup(GUN_GROUP, GUN_SPORELAUNCHER);
	else if (FBitSet(pev->weapons, WPN_HORNETGUN))
		SetBodygroup(GUN_GROUP, GUN_HORNETGUN);
	else
		SetBodygroup(GUN_GROUP, GUN_SHOCKRIFLE);

	SetBodygroup(BODY_GROUP, RANDOM_LONG(BODY_NORMAL, BODY_RED));
	MonsterInit();
}

void CShockTrooper::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/strooper.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "shocktrooper");

	CSquadMonster::Precache();

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pSentenceSounds);

	/*UTIL_PrecacheOther("st_hornet");
	UTIL_PrecacheOther("st_shock");
	UTIL_PrecacheOther("st_shockball");
	UTIL_PrecacheOther("sporegrenade");
	UTIL_PrecacheOther("sporeball");

	if (FBitSet(pev->weapons, WPN_SHOCKRIFLE))
		UTIL_PrecacheOther("monster_shockroach");*/
}

int CShockTrooper::IRelationship(CBaseEntity *pTarget)
{
	if (pTarget == NULL)
		return R_NO;

	if (pTarget->IsMonster())
	{
		if (FClassnameIs(pTarget->pev, "monster_human_grunt") || (FClassnameIs(pTarget->pev, "monster_apache")))
			return R_NM;
	}
	return CSquadMonster::IRelationship(pTarget);
}

int CShockTrooper::ISoundMask(void)
{
	return	bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

void CShockTrooper::PrescheduleThink(void)
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
				MySquadLeader()->m_fEnemyEluded = TRUE;
		}
	}

	if ( ( pev->skin == 0 ) && RANDOM_LONG(0,0x7F) == 0 )
	{
		pev->skin = SHOCKTROOPER_EYE_FRAMES - 1;
	}
	else if ( pev->skin != 0 )
	{
		pev->skin--;
	}
}

bool CShockTrooper::FCanCheckAttacks (void)
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
		return true;
	else
		return false;
}

BOOL CShockTrooper::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster *pEnemy = NULL;
	if (m_hEnemy != NULL)
		pEnemy = m_hEnemy->MyMonsterPointer();

	if (pEnemy && flDist <= MELEE_DIST && flDot >= 0.7 && pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON && pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
		return TRUE;

	return FALSE;
}

BOOL CShockTrooper::CheckRangeAttack1 ( float flDot, float flDist )
{
	int WpnRange;

	if (FBitSet(pev->weapons, WPN_SPORELAUNCHER))
		WpnRange = SPORELAUNCHER_DIST;
	else if (FBitSet(pev->weapons, WPN_HORNETGUN))
		WpnRange = HORNETGUN_DIST;
	else
		WpnRange = SHOCKRIFLE_DIST;

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= WpnRange && flDist > MELEE_DIST && flDot >= 0.7 && NoFriendlyFire() )
	{
		if (FBitSet(pev->weapons, WPN_SPORELAUNCHER))
		{
			Vector vecTarget;
			vecTarget = m_hEnemy->Center();
			Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, SPORE_BALL_SPEED, g_pWorld->pev->gravity);
			if (!vecToss.IsZero())
				m_vecTossVelocity = vecToss;

			if ( InSquad() )
			{
				if (SquadMemberInRange( vecTarget, 128))
				return FALSE;
			}

			if ( ( vecTarget - pev->origin ).Length2D() <= 128)
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CShockTrooper::CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (WPN_SPOREGRENADE | WPN_SHOCKBALL_LAUNCHER)))
		return FALSE;

	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	if (gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;

	if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z)
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet( pev->weapons, WPN_SPOREGRENADE))
	{
		if (RANDOM_LONG(0,1))
			vecTarget.Set( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		else
			vecTarget = m_vecEnemyLKP;
	}
	else
		vecTarget = m_hEnemy->Center();

	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 128))
		{
			m_flNextGrenadeCheck = gpGlobals->time + 1; 
			m_fThrowGrenade = FALSE;
		}
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 128)
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	if (FBitSet( pev->weapons, WPN_SPOREGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, g_pWorld->pev->gravity);
		if (!vecToss.IsZero())
		{
			m_vecTossVelocity = vecToss;
			m_fThrowGrenade = TRUE;
			m_flNextGrenadeCheck = gpGlobals->time;
		}
		else
		{
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 1; 
		}
	}
	else if (FBitSet( pev->weapons, WPN_SHOCKBALL_LAUNCHER))
	{
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}

	return m_fThrowGrenade;
}

int CShockTrooper::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	Forget( bits_MEMORY_INCOVER );

	if (pInflictor && m_hEnemy.Get() == NULL)
	{
		if (pInflictor->IsMonster() && CBaseMonster::IRelationship(pInflictor) >= R_DL)
			m_hEnemy = pInflictor;
	}

	if (bitsDamageType & (DMG_SHOCK))
		flDamage *= 0.33;
	return CSquadMonster::TakeDamage ( pInflictor, pAttacker, flDamage, bitsDamageType );
}

void CShockTrooper::SetYawSpeed (void)
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}
	pev->yaw_speed = ys;
}

int	CShockTrooper::Classify(void)
{
	return m_iClass?m_iClass:CLASS_ALIEN_MILITARY;
}

void CShockTrooper::ShootShock(void)
{
	if (m_hEnemy.Get() == NULL)
		return;

	UTIL_MakeVectors(pev->angles);
	Vector vecShootOrigin(GetGunPosition());
	Vector vecShootDir(ShootAtEnemy(vecShootOrigin));

	//CSTShock::Shoot(vecShootOrigin, vecShootDir, GetDamageAttacker(), this);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, pev->origin, pev->angles, 0.0, 0.0, MONFX_SHOCKTROOPER_SHOOT_EFFECT, 0, 0, 0);
	CLightProjectile::CreateLP(vecShootOrigin, pev->angles, vecShootDir, this, this, gSkillData.shocktrooperDmgShock, 1);// STUB

	/*Vector angDir;
	VectorAngles(vecShootDir, angDir);
#if defined (NOSQB)
	SetBlending(0, -angDir.x);
#else
	SetBlending(0, angDir.x);
#endif*/
}

void CShockTrooper::ShootShockBall(void)
{
	Vector vecShootOrigin(GetGunPosition());
	Vector vecShootDir(ShootAtEnemy(vecShootOrigin));
	UTIL_MakeVectors(pev->angles);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, pev->origin, pev->angles, 0.0, 0.0, MONFX_SHOCKTROOPER_SHOOT_EFFECT, 1, 0, 0);
	CLightProjectile::CreateLP(vecShootOrigin, pev->angles, vecShootDir, this, this, gSkillData.shocktrooperDmgShock, 0);// STUB
	//CSTShockBall::Shoot(GetGunPosition(), vecShootDir, this, this);

	Vector angDir;
	VectorAngles(vecShootDir, angDir);
#if defined (NOSQB)
	SetBlending(0, -angDir.x);
#else
	SetBlending(0, angDir.x);
#endif
}

void CShockTrooper::ShootSpore(void)
{
	//CSporeBall::Shoot(GetGunPosition(), m_vecTossVelocity, 5, GetDamageAttacker(), this);
	//PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, pev->origin, pev->angles, 0.0, 0.0, MONFX_SHOCKTROOPER_SHOOT_EFFECT, 1, 0, 0);
	CAGrenade::ShootTimed(GetGunPosition(), pev->angles, ShootAtEnemy(GetGunPosition())*256.0f, this, this, 4.0, gSkillData.shocktrooperDmgGrenade, 0);
}

void CShockTrooper::ShootHornet(void)
{
	CHornet *pHornet = CHornet::CreateHornet(GetGunPosition(), pev->angles, ShootAtEnemy(GetGunPosition())*300.0f, this, this, gSkillData.monDmgHornet, true);
	if (pHornet)
		pHornet->m_hEnemy = m_hEnemy;

	//CSTHornet::Shoot(vecShootOrigin, vecShootDir*500.0f, GetDamageAttacker(), this, m_hEnemy, gSkillData.monDmgHornet);
	/*Vector angDir;
	VectorAngles(vecShootDir, angDir);
#if defined (NOSQB)
	SetBlending(0, -angDir.x);
#else
	SetBlending(0, angDir.x);
#endif*/
}

Vector CShockTrooper::GetGunPosition(void)
{
	Vector	vecStart, angleGun;
	UTIL_MakeVectors(pev->angles);
	GetAttachment(0, vecStart, angleGun);
	return vecStart;
}

void CShockTrooper::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
		case AE_DROP_GUN:
		{
			DropItems();
		}
		break;

		case AE_GREN_THROW:
		{
			UTIL_MakeVectors( pev->angles );
			//CSporeGrenade::Shoot(GetGunPosition(), m_vecTossVelocity, 3.5, this, this);
			EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[0], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 5;
		}
		break;

		case AE_GREN_LAUNCH:
		{
			ShootShockBall();
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 5;
		}
		break;

		case AE_BURST1:
		{
			if (m_hEnemy.Get() == NULL)
			{
				if ( FBitSet( pev->weapons, WPN_SPORELAUNCHER ))
					ShootSpore();
				else if ( FBitSet( pev->weapons, WPN_HORNETGUN ))
					ShootHornet();
				else
					ShootShock();

				CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
			}
		}
		break;

		case AE_KICK:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack(MELEE_DIST, gSkillData.shocktrooperDmgKick, DMG_CLUB);
			if (pHurt)
			{
				if (pHurt->pev->flags & (FL_MONSTER|FL_CLIENT))
				{
					pHurt->pev->punchangle.z = -8;
					pHurt->PunchPitchAxis(-5);
					pHurt->pev->velocity -= gpGlobals->v_right * 100;
				}
				AttackHitSound();
			}
			else
				AttackMissSound();

			AttackSound();
		}
		break;

		case AE_CAUGHT_ENEMY:
		{
			EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[RANDOM_LONG(1,3)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CShockTrooper::StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_SHOCKTROOPER_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_SHOCKTROOPER_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_SHOCKTROOPER_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster::StartTask( pTask );
		break;

	default:
		CSquadMonster::StartTask( pTask );
		break;
	}
}

void CShockTrooper::RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SHOCKTROOPER_FACE_TOSS_DIR:
		{
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster::RunTask( pTask );
			break;
		}
	}
}

void CShockTrooper::AttackHitSound(void)
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CShockTrooper::AttackMissSound(void)
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CShockTrooper::PainSound (void)
{
	if (IsAlive() && m_flNextPainTime < gpGlobals->time)
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);// XDM3038c
		m_flNextPainTime = gpGlobals->time + RANDOM_FLOAT(1, 3);
	}
}

void CShockTrooper::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);// XDM3038c
}

void CShockTrooper::IdleSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);// XDM3038c
}

void CShockTrooper::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);// XDM3038c
}

void CShockTrooper::AlertSound (void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);// XDM3038c
}

Task_t	tlShockTrooperFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slShockTrooperFail[] =
{
	{
		tlShockTrooperFail,
		ARRAYSIZE ( tlShockTrooperFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Fail"
	},
};

Task_t	tlShockTrooperCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slShockTrooperCombatFail[] =
{
	{tlShockTrooperCombatFail,	ARRAYSIZE(tlShockTrooperCombatFail), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,	0, "Combat Fail"},
};

Task_t	tlShockTrooperVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slShockTrooperVictoryDance[] =
{
	{
		tlShockTrooperVictoryDance,
		ARRAYSIZE ( tlShockTrooperVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"VictoryDance"
	},
};

Task_t tlShockTrooperEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_SHOCKTROOPER_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slShockTrooperEstablishLineOfFire[] =
{
	{
		tlShockTrooperEstablishLineOfFire,
		ARRAYSIZE ( tlShockTrooperEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

Task_t	tlShockTrooperFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slShockTrooperFoundEnemy[] =
{
	{tlShockTrooperFoundEnemy,	ARRAYSIZE(tlShockTrooperFoundEnemy), bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "FoundEnemy"},
};

Task_t	tlShockTrooperCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SHOCKTROOPER_SWEEP	},
};

Schedule_t	slShockTrooperCombatFace[] =
{
	{
		tlShockTrooperCombatFace1,
		ARRAYSIZE ( tlShockTrooperCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

Task_t	tlShockTrooperWaitInCover[] =
{
	{ TASK_STOP_MOVING,		(float)0		},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},
	{ TASK_WAIT_FACE_ENEMY,	(float)1		},
};

Schedule_t	slShockTrooperWaitInCover[] =
{
	{
		tlShockTrooperWaitInCover,
		ARRAYSIZE ( tlShockTrooperWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"WaitInCover"
	},
};

Task_t	tlShockTrooperTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_SHOCKTROOPER_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY	},
};

Schedule_t	slShockTrooperTakeCover[] =
{
	{tlShockTrooperTakeCover1,	ARRAYSIZE(tlShockTrooperTakeCover1), 0, 0,	"TakeCover"},
};

Task_t	tlShockTrooperTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slShockTrooperTakeCoverFromBestSound[] =
{
	{tlShockTrooperTakeCoverFromBestSound,	ARRAYSIZE(tlShockTrooperTakeCoverFromBestSound), 0, 0,	"TakeCoverFromBestSound"},
};

Task_t	tlShockTrooperSweep[] =
{
	{ TASK_TURN_LEFT,	(float)179	},
	{ TASK_WAIT,		(float)1	},
	{ TASK_TURN_LEFT,	(float)179	},
	{ TASK_WAIT,		(float)1	},
};

Schedule_t	slShockTrooperSweep[] =
{
	{
		tlShockTrooperSweep,
		ARRAYSIZE ( tlShockTrooperSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Sweep"
	},
};

Task_t	tlShockTrooperRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_CROUCH },
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slShockTrooperRangeAttack1A[] =
{
	{
		tlShockTrooperRangeAttack1A,
		ARRAYSIZE ( tlShockTrooperRangeAttack1A ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_SHOCKTROOPER_NOFIRE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};

Task_t	tlShockTrooperRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_SHOCKTROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slShockTrooperRangeAttack1B[] =
{
	{
		tlShockTrooperRangeAttack1B,
		ARRAYSIZE ( tlShockTrooperRangeAttack1B ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_SHOCKTROOPER_NOFIRE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

Task_t	tlShockTrooperRangeAttack2[] =
{
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_SHOCKTROOPER_FACE_TOSS_DIR,	(float)0					},
	{ TASK_PLAY_SEQUENCE,				(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,				(float)SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY	},
};

Schedule_t	slShockTrooperRangeAttack2[] =
{
	{tlShockTrooperRangeAttack2, ARRAYSIZE(tlShockTrooperRangeAttack2), 0, 0, "RangeAttack2"},
};

Task_t	tlShockTrooperCheckSound[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSOUND,	(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE_ANGRY	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SHOCKTROOPER_SWEEP	},
	{ TASK_WAIT,					(float)4				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slShockTrooperCheckSound[] =
{
	{
		tlShockTrooperCheckSound,
		ARRAYSIZE ( tlShockTrooperCheckSound ),

		bits_COND_SEE_ENEMY			|
		bits_COND_PROVOKED			|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_ENEMY_FACING_ME	|
		bits_COND_SEE_NEMESIS,

		bits_SOUND_DEATH		|
		bits_SOUND_COMBAT		|
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Check Sound"
	},
};

DEFINE_CUSTOM_SCHEDULES( CShockTrooper )
{
	slShockTrooperFail,
	slShockTrooperCombatFail,
	slShockTrooperVictoryDance,
	slShockTrooperEstablishLineOfFire,
	slShockTrooperFoundEnemy,
	slShockTrooperCombatFace,
	slShockTrooperWaitInCover,
	slShockTrooperTakeCover,
	slShockTrooperTakeCoverFromBestSound,
	slShockTrooperSweep,
	slShockTrooperRangeAttack1A,
	slShockTrooperRangeAttack1B,
	slShockTrooperRangeAttack2,
	slShockTrooperCheckSound,
};

IMPLEMENT_CUSTOM_SCHEDULES( CShockTrooper, CSquadMonster );

void CShockTrooper::SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		if (FBitSet( pev->weapons, WPN_SPORELAUNCHER))
		{
			if ( m_fStanding )
				iSequence = LookupSequence( "standing_sporelauncher" );
			else
				iSequence = LookupSequence( "crouching_sporelauncher" );
		}
		else if (FBitSet( pev->weapons, WPN_HORNETGUN))
		{
			if ( m_fStanding )
				iSequence = LookupSequence( "standing_hornetgun" );
			else
				iSequence = LookupSequence( "crouching_hornetgun" );
		}
		else
		{
			if ( m_fStanding )
				iSequence = LookupSequence( "standing_shockrifle" );
			else
				iSequence = LookupSequence( "crouching_shockrifle" );
		}
		break;
	case ACT_RANGE_ATTACK2:
		if ( pev->weapons & WPN_SPOREGRENADE )
			iSequence = LookupSequence( "throwgrenade" );
		else
			iSequence = LookupSequence( "launchgrenade" );
		break;

	case ACT_RUN:
		if ( pev->health <= SHOCKTROOPER_LIMP_HEALTH )
			iSequence = LookupActivity ( ACT_RUN_HURT );
		else
			iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
		if ( pev->health <= SHOCKTROOPER_LIMP_HEALTH )
			iSequence = LookupActivity ( ACT_WALK_HURT );
		else
			iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}

	m_Activity = NewActivity; 
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		ALERT(at_aiconsole, "%s[%d] \"%s\" has no sequence for act: %d (%s) \n", STRING(pev->classname), entindex(), STRING(pev->targetname), NewActivity, activity_map[ActivityMapFind(NewActivity)].name);
		pev->sequence		= 0;
	}
}

Schedule_t *CShockTrooper::GetSchedule(void)
{
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound = PBestSound();
		if (pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[2], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			if (m_MonsterState == MONSTERSTATE_IDLE && ((pSound->m_iType & bits_SOUND_COMBAT) || (pSound->m_iType & bits_SOUND_DEATH)))
			{
				EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[1], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
				return GetScheduleOfType( SCHED_SHOCKTROOPER_CHECK_SOUND );
			}
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
				return CBaseMonster::GetSchedule();

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					else
					{
						AlertSound();
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
							return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
						else
							return GetScheduleOfType ( SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
					}
				}
			}
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 60 && m_hEnemy != NULL )
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				else
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );

			else if ( FBitSet( pev->weapons, WPN_SHOCKBALL_LAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );

			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_SHOCKTROOPER_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );

				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );

				else
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[4], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}

				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[5], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
					return GetScheduleOfType( SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[6], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				return GetScheduleOfType ( SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
		}
	}
	return CSquadMonster::GetSchedule();
}

Schedule_t* CShockTrooper::GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( RANDOM_LONG(0,1) )
				return &slShockTrooperTakeCover[ 0 ];
			else
			{
				EMIT_SOUND_DYN(edict(), CHAN_BODY, pSentenceSounds[7], VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
				return &slShockTrooperRangeAttack2[ 0 ];
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
			return &slShockTrooperTakeCoverFromBestSound[ 0 ];

	case SCHED_SHOCKTROOPER_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_SHOCKTROOPER_ELOF_FAIL:
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		break;

	case SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE:
		return &slShockTrooperEstablishLineOfFire[ 0 ];
		break;

	case SCHED_RANGE_ATTACK1:
		{
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,5);

			if (m_fStanding)
				return &slShockTrooperRangeAttack1B[ 0 ];
			else
				return &slShockTrooperRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
			return &slShockTrooperRangeAttack2[ 0 ];

	case SCHED_COMBAT_FACE:
			return &slShockTrooperCombatFace[ 0 ];

	case SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY:
			return &slShockTrooperWaitInCover[ 0 ];

	case SCHED_SHOCKTROOPER_SWEEP:
			return &slShockTrooperSweep[ 0 ];

	case SCHED_SHOCKTROOPER_FOUND_ENEMY:
			return &slShockTrooperFoundEnemy[ 0 ];

	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
					return &slShockTrooperFail[ 0 ];
			}
			return &slShockTrooperVictoryDance[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
				return &slShockTrooperCombatFail[ 0 ];

			return &slShockTrooperFail[ 0 ];
		}
	case SCHED_SHOCKTROOPER_CHECK_SOUND:
			return &slShockTrooperCheckSound[0];

	default:
			return CSquadMonster::GetScheduleOfType ( Type );
	}
}

void CShockTrooper::DropItems(void)
{
	if (GetBodygroup(GUN_GROUP) == GUN_NONE)
		return;

	if (GetBodygroup(GUN_GROUP) != GUN_SHOCKRIFLE)
		return;

	SetBodygroup(GUN_GROUP, GUN_NONE);
	//Create("monster_shockroach", GetGunPosition(), pev->angles, gpGlobals->v_forward * 512, edict(), SF_MONSTER_FALL_TO_GROUND | SF_NOREFERENCE | SF_NORESPAWN);
}
