//=========================================================
// Male Assasin
// Ripoff from hgrunt
// This code is a total shit. It is not part of XHL. It should be rewritten.
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
#include "talkmonster.h"
#include "soundent.h"
#include "globals.h"
#include "customentity.h"
#include "effects.h"
#include "sound.h"
#include "gamerules.h"
#include "pm_materials.h"
//#include "projectiles_mon.h"

#define bits_COND_MASSASSIN_NOFIRE	( bits_COND_SPECIAL1 )

#define	MASSASSIN_CLIP_SIZE	30 
#define	MASSASSIN_LIMP_HEALTH	20

#define	MELEE_DIST		75
#define	SNIPER_DIST		16384
#define	MP5_DIST		3072

#define MASSASSIN_9MMAR				(1 << 0)
#define MASSASSIN_HANDGRENADE		(1 << 1)
#define MASSASSIN_GRENADELAUNCHER	(1 << 2)
#define MASSASSIN_SNIPERRIFLE		(1 << 3)

/*
//Ghoul: There are different dmg types, so MASSASSIN's armor has different protection classes against these damages.
//very rude and simple formula. In a good way, we need more "dmg type groups", but... for now it's ok
#define MASSASSIN_ARMOR_TAKE_NORMAL		0.4
#define MASSASSIN_ARMOR_TAKE_SPECIAL		0.8
#define MASSASSIN_ARMOR_TAKE_LESS		0.2

#define MASSASSIN_ARMOR_BONUS_NORMAL		0.5
#define MASSASSIN_ARMOR_BONUS_SPECIAL	0.2
#define MASSASSIN_ARMOR_BONUS_LESS		0.8
*/
enum
{
	MASSN_BODY_GROUP = 0,
	MASSN_HEAD_GROUP,
	MASSN_GUN_GROUP
};

enum
{
	HEAD_00 = 0,
	HEAD_01,
	HEAD_02,
	HEAD_03,
};

enum
{
	GUN_MP5 = 0,
	GUN_SNIPERRIFLE,
	GUN_NONE
};

#define	AE_RELOAD		( 2 )
#define	AE_KICK			( 3 )
#define	AE_BURST1		( 4 )
#define	AE_BURST2		( 5 )
#define	AE_BURST3		( 6 )
#define	AE_GREN_TOSS	( 7 )
#define	AE_GREN_LAUNCH	( 8 )
#define	AE_GREN_DROP	( 9 )

#define	AE_DROP_GUN		( 11)

enum massassin_schedules_e
{
	SCHED_MASSASSIN_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE,
	SCHED_MASSASSIN_COVER_AND_RELOAD,
	SCHED_MASSASSIN_SWEEP,
	SCHED_MASSASSIN_FOUND_ENEMY,
	SCHED_MASSASSIN_REPEL,
	SCHED_MASSASSIN_REPEL_ATTACK,
	SCHED_MASSASSIN_REPEL_LAND,
	SCHED_MASSASSIN_WAIT_FACE_ENEMY,
	SCHED_MASSASSIN_TAKECOVER_FAILED,
	SCHED_MASSASSIN_ELOF_FAIL,
	SCHED_MASSASSIN_CHECK_SOUND
};

enum massassin_tasks_e
{
	TASK_MASSASSIN_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_MASSASSIN_CHECK_FIRE
};

class CMassassin : public CSquadMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int	Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void SetYawSpeed (void);
	virtual int Classify (void);
	virtual int ISoundMask (void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual bool FCanCheckAttacks (void);
	virtual BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	virtual BOOL CheckRangeAttack1 ( float flDot, float flDist );
	virtual BOOL CheckRangeAttack2 ( float flDot, float flDist );
	virtual void CheckAmmo (void);
	virtual void SetActivity ( Activity NewActivity );
	virtual void StartTask ( Task_t *pTask );
	virtual void RunTask ( Task_t *pTask );
	virtual void DeathSound(void);
	virtual void PainSound(void);
	virtual void AttackHitSound(void);
	virtual void AttackMissSound(void);
	virtual Vector GetGunPosition(void);
	virtual void ShootMP5 (void);
	virtual void ShootSniper (void);

	virtual void PrescheduleThink (void);
	virtual Schedule_t *GetSchedule(void);
	virtual Schedule_t *GetScheduleOfType ( int Type );
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int IRelationship ( CBaseEntity *pTarget );

	void DropItems(void);
	CBaseEntity	*Kick(void);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;
	int		m_cClipSize;

	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS( monster_male_assassin, CMassassin );

TYPEDESCRIPTION	CMassassin::m_SaveData[] =
{
	DEFINE_FIELD( CMassassin, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CMassassin, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CMassassin, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CMassassin, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMassassin, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMassassin, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMassassin, m_cClipSize, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMassassin, CSquadMonster );

const char *CMassassin::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CMassassin::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CMassassin::pPainSounds[] = 
{
	"hgrunt/gr_pain1.wav",
	"hgrunt/gr_pain2.wav",
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
	"hgrunt/gr_pain5.wav",
};

const char *CMassassin::pDeathSounds[] = 
{
	"hgrunt/gr_die1.wav",
	"hgrunt/gr_die2.wav",
	"hgrunt/gr_die3.wav",
};

void CMassassin :: Spawn()
{
	if (pev->health <= 0)
		pev->health = gSkillData.massassinHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = gSkillData.batteryCapacity * (gSkillData.iSkillLevel+1);
	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.massassinScore;
	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;

	CSquadMonster::Spawn();// XDM3038b: Precache( );
	//SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck= gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;

	if (pev->weapons == 0)
		pev->weapons = MASSASSIN_9MMAR | MASSASSIN_HANDGRENADE;

	if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE ))
	{
		m_cClipSize		= SHOTGUN_MAX_CLIP;
		SetBodygroup(MASSN_GUN_GROUP, GUN_SNIPERRIFLE);
	}
	else
	{
		SetBodygroup(MASSN_GUN_GROUP, GUN_MP5);
		m_cClipSize		= MP5_MAX_CLIP;
	}

	SetBodygroup(MASSN_HEAD_GROUP, RANDOM_LONG(HEAD_00, HEAD_03));
	m_cAmmoLoaded		= m_cClipSize;

	MonsterInit();
}

void CMassassin :: Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/monsters/massn.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "hgrunt");

	CSquadMonster::Precache();

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	PRECACHE_MODEL("models/w_shells.mdl");

	//Ghoul: do we really need this? Weapons precached by player anyway
	if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
		UTIL_PrecacheWeapon("weapon_357");
	else 
		UTIL_PrecacheWeapon("weapon_9mmAR");

	if (FBitSet(pev->weapons, MASSASSIN_GRENADELAUNCHER))
		UTIL_PrecacheAmmo("ammo_ARgrenades");

	if (FBitSet(pev->weapons, MASSASSIN_HANDGRENADE))
		UTIL_PrecacheWeapon("weapon_handgrenade");
}

int CMassassin::IRelationship ( CBaseEntity *pTarget )
{
	if (pTarget == NULL)
		return R_NO;

	int targetclass = pTarget->Classify();
	if (pTarget->IsProjectile())
	{
		if (pTarget->IsMonster())
		{
			if (targetclass == CLASS_PLAYER_BIOWEAPON)
				return R_DL;
			else if (targetclass == CLASS_ALIEN_BIOWEAPON)
				return R_FR;
		}
		return R_FR;
	}
	return CSquadMonster::IRelationship( pTarget );
}

int CMassassin :: ISoundMask (void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

void CMassassin :: PrescheduleThink (void)
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
}

bool CMassassin :: FCanCheckAttacks (void)
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
		return true;
	else
		return false;
}

BOOL CMassassin :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy = NULL;
	if (m_hEnemy != NULL)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();
	}

	if (pEnemy && flDist <= MELEE_DIST && flDot >= 0.7 &&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CMassassin :: CheckRangeAttack1 ( float flDot, float flDist )
{
	int WpnRange;

	if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
		WpnRange = SNIPER_DIST;
	else
		WpnRange = MP5_DIST;

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= WpnRange && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= MELEE_DIST )
			return FALSE;

		Vector vecSrc = GetGunPosition();
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
			return TRUE;
	}
	return FALSE;
}

BOOL CMassassin :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (MASSASSIN_HANDGRENADE | MASSASSIN_GRENADELAUNCHER)))
		return FALSE;

	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	if (gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet( pev->weapons, MASSASSIN_HANDGRENADE))
	{
		if (RANDOM_LONG(0,1))
			vecTarget.Set( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		else
			vecTarget = m_vecEnemyLKP;
	}
	else
	{
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			m_flNextGrenadeCheck = gpGlobals->time + 1; 
			m_fThrowGrenade = FALSE;
		}
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	if (FBitSet( pev->weapons, MASSASSIN_HANDGRENADE))
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
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, g_pWorld->pev->gravity);
		if (!vecToss.IsZero())
		{
			m_vecTossVelocity = vecToss;
			m_fThrowGrenade = TRUE;
			m_flNextGrenadeCheck = gpGlobals->time + 0.3;
		}
		else
		{
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 1;
		}
	}
	return m_fThrowGrenade;
}

void CMassassin::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == HITGROUP_ARMOR || ptr->iHitgroup == 11)// Warning: HLBUG: HITGROUP_ARMOR differs in this model (HL compatibility)
	{
		if (GetBodygroup(MASSN_HEAD_GROUP) == HEAD_00 && FBitSet(bitsDamageType, DMGM_RICOCHET) && !FBitSet(bitsDamageType, DMG_IGNOREARMOR))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				//UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		//WRONG! ptr->iHitgroup = HITGROUP_HEAD;
	}

	//NO! Double protection. We need to owerride combat.cpp code....
	/*
	//Ghoul. New dmg formula: armor works ONLY, if hit chest or stomach hitgroup
	float flTake = flDamage;

	if (ptr->iHitgroup == HITGROUP_ARMOR)
	{
		float flArmorTake;
		if (pev->armorvalue > 0 && !FBitSet(bitsDamageType, DMG_IGNOREARMOR))
		{
			if (FBitSet(bitsDamageType, DMG_BULLET | DMG_SLASH | DMG_CLUB))
			{
				flArmorTake = flTake * MASSASSIN_ARMOR_TAKE_SPECIAL;
				pev->armorvalue -= flArmorTake * MASSASSIN_ARMOR_BONUS_SPECIAL;
			}
			else if (FBitSet(bitsDamageType, DMG_SONIC | DMG_ENERGYBEAM | DMG_MICROWAVE | DMG_BLAST))
			{
				flArmorTake = flTake * MASSASSIN_ARMOR_TAKE_LESS;
				pev->armorvalue -= flArmorTake * MASSASSIN_ARMOR_BONUS_LESS;
			}
			else
			{
				flArmorTake = flTake * MASSASSIN_ARMOR_TAKE_NORMAL;
				pev->armorvalue -= flArmorTake * MASSASSIN_ARMOR_BONUS_NORMAL;
			}
			pev->armorvalue -= flArmorTake;
			flTake -= flArmorTake;

			if (pev->armorvalue < 0.0f)
			{
				flTake -= pev->armorvalue;
				pev->armorvalue = 0.0f;
			}
		}
		pev->health -= flTake;
	}
	*/
	CSquadMonster::TraceAttack( pAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CMassassin::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	Forget( bits_MEMORY_INCOVER );

	if (pInflictor && m_hEnemy.Get() == NULL)
	{
		if (pInflictor->IsMonster() && CBaseMonster::IRelationship(pInflictor) >= R_DL)
			m_hEnemy = pInflictor;
	}
	return CSquadMonster :: TakeDamage ( pInflictor, pAttacker, flDamage, bitsDamageType );
}

void CMassassin :: SetYawSpeed (void)
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
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CMassassin :: CheckAmmo (void)
{
	if ( m_cAmmoLoaded <= 0 )
		SetConditions(bits_COND_NO_AMMO_LOADED);
}

int	CMassassin :: Classify (void)
{
	return m_iClass?m_iClass:CLASS_HUMAN_MILITARY;
}

CBaseEntity *CMassassin :: Kick(void)
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * MELEE_DIST);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

Vector CMassassin :: GetGunPosition( )
{
	Vector	vecStart, angleGun;
	UTIL_MakeVectors(pev->angles);
	GetAttachment(0, vecStart, angleGun );

	return vecStart;
}

void CMassassin::ShootMP5(void)
{
	if (m_hEnemy.Get() == NULL)
		return;

	Vector vecShootOrigin(GetGunPosition());
	Vector vecShootDir(ShootAtEnemy(vecShootOrigin));
	UTIL_MakeVectors(pev->angles);

	FireBullets(1, vecShootOrigin, vecShootDir, m_fStanding?VECTOR_CONE_5DEGREES:VECTOR_CONE_4DEGREES, NULL, BULLET_DEFAULT_DIST, BULLET_MP5, 0, this, this, 0);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, vecShootOrigin, pev->angles, 0.0, 0.0, MONFX_HGRUNT_FIREGUN, 0, 0, 0);

	m_cAmmoLoaded--;

	Vector angDir;
	VectorAngles(vecShootDir, angDir);
#if defined (NOSQB)
	SetBlending(0, -angDir.x);
#else
	SetBlending(0, angDir.x);
#endif
}

void CMassassin::ShootSniper(void)
{
	if (m_hEnemy.Get() == NULL)
		return;

	Vector vecShootOrigin(GetGunPosition());
	Vector vecShootDir(ShootAtEnemy(vecShootOrigin));
	UTIL_MakeVectors(pev->angles);
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0,0,0), NULL, SNIPER_DIST, BULLET_357, 0, this, this, 0);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, vecShootOrigin, pev->angles, 0.0, 0.0, MONFX_HGRUNT_FIREGUN, 4, 0, 0);
	m_cAmmoLoaded--;

	Vector angDir;
	VectorAngles(vecShootDir, angDir);
#if defined (NOSQB)
	SetBlending(0, -angDir.x);
#else
	SetBlending(0, angDir.x);
#endif
}

void CMassassin :: HandleAnimEvent(MonsterEvent_t *pEvent)
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

		case AE_RELOAD:
		{
			if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/357_reload1.wav", 0.8, ATTN_NORM);
			else
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/mp5_reload.wav", 0.8, ATTN_NORM);

			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
		}
		break;

		case AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed(GetGunPosition(), m_vecTossVelocity, 2.5, 0, this, this);
			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;
		}
		break;

		case AE_GREN_LAUNCH:
		{
			PLAYBACK_EVENT_FULL(FEV_RELIABLE, ENT(pev), g_usMonFx, 0, pev->origin, pev->angles, 0.0, 0.0, MONFX_HGRUNT_FIREGUN, 2, 0, 0);
			CARGrenade::CreateGrenade(GetGunPosition(), pev->angles, m_vecTossVelocity, this, this, 0);
			m_fThrowGrenade = FALSE;
			if (gSkillData.iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT(1, 3);
			else
				m_flNextGrenadeCheck = gpGlobals->time + 4;
		}
		break;

		case AE_GREN_DROP://Ghoul TODO: drop something powerfull may be tripmine?
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed(pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3, 0, this, this);
		}
		break;

		case AE_BURST1:
		{
			if ( FBitSet( pev->weapons, MASSASSIN_9MMAR ))
				ShootMP5();
			else
				ShootSniper();

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case AE_BURST2:
		case AE_BURST3:
			ShootMP5();
			break;

		case AE_KICK:
		{
			CBaseEntity *pHurt = Kick();
			if ( pHurt )
			{
				AttackHitSound();
				pHurt->PunchPitchAxis(-15.0f);
				pHurt->TakeDamage(this, this, gSkillData.hgruntDmgKick, DMG_CLUB);
				if (pHurt->IsPushable())
				{
					UTIL_MakeVectors( pev->angles );
					pHurt->pev->velocity += (gpGlobals->v_forward*2 + gpGlobals->v_up)*DamageForce(gSkillData.hgruntDmgKick);
				}
			}
			else
				AttackMissSound();
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CMassassin :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MASSASSIN_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_MASSASSIN_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_MASSASSIN_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

void CMassassin :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MASSASSIN_FACE_TOSS_DIR:
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
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

void CMassassin :: AttackHitSound(void)
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CMassassin :: AttackMissSound(void)
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CMassassin :: PainSound (void)
{
	if (IsAlive() && m_flNextPainTime < gpGlobals->time)
	{
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);// XDM3038c
		m_flNextPainTime = gpGlobals->time + RANDOM_FLOAT(1, 3);
	}
}

void CMassassin :: DeathSound (void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);// XDM3038c
}

Task_t	tlMassnFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slMassnFail[] =
{
	{
		tlMassnFail,
		ARRAYSIZE ( tlMassnFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Fail"
	},
};

Task_t	tlMassnCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slMassnCombatFail[] =
{
	{tlMassnCombatFail,	ARRAYSIZE(tlMassnCombatFail), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,	0, "Combat Fail"},
};

Task_t	tlMassnVictoryDance[] =
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

Schedule_t	slMassnVictoryDance[] =
{
	{
		tlMassnVictoryDance,
		ARRAYSIZE ( tlMassnVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"VictoryDance"
	},
};

Task_t tlMassnEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_MASSASSIN_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slMassnEstablishLineOfFire[] =
{
	{
		tlMassnEstablishLineOfFire,
		ARRAYSIZE ( tlMassnEstablishLineOfFire ),
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

Task_t	tlMassnFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slMassnFoundEnemy[] =
{
	{tlMassnFoundEnemy,	ARRAYSIZE(tlMassnFoundEnemy), bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "FoundEnemy"},
};

Task_t	tlMassnCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MASSASSIN_SWEEP	},
};

Schedule_t	slMassnCombatFace[] =
{
	{
		tlMassnCombatFace1,
		ARRAYSIZE ( tlMassnCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

Task_t	tlMassnSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MASSASSIN_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MASSASSIN_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MASSASSIN_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MASSASSIN_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_MASSASSIN_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slMassnSignalSuppress[] =
{
	{
		tlMassnSignalSuppress,
		ARRAYSIZE ( tlMassnSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_MASSASSIN_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlMassnSuppress[] =
{
	{ TASK_STOP_MOVING,			0			},
	{ TASK_FACE_ENEMY,			(float)0	},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0	},
	{ TASK_RANGE_ATTACK1,		(float)0	},
	{ TASK_FACE_ENEMY,			(float)0	},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0	},
	{ TASK_RANGE_ATTACK1,		(float)0	},
	{ TASK_FACE_ENEMY,			(float)0	},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0	},
	{ TASK_RANGE_ATTACK1,		(float)0	},
	{ TASK_FACE_ENEMY,			(float)0	},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0	},
	{ TASK_RANGE_ATTACK1,		(float)0	},
	{ TASK_FACE_ENEMY,			(float)0	},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0	},
	{ TASK_RANGE_ATTACK1,		(float)0	},
};

Schedule_t	slMassnSuppress[] =
{
	{
		tlMassnSuppress,
		ARRAYSIZE ( tlMassnSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_MASSASSIN_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};

Task_t	tlMassnWaitInCover[] =
{
	{ TASK_STOP_MOVING,		(float)0		},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},
	{ TASK_WAIT_FACE_ENEMY,	(float)1		},
};

Schedule_t	slMassnWaitInCover[] =
{
	{
		tlMassnWaitInCover,
		ARRAYSIZE ( tlMassnWaitInCover ),
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

Task_t	tlMassnTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MASSASSIN_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MASSASSIN_WAIT_FACE_ENEMY	},
};

Schedule_t	slMassnTakeCover[] =
{
	{tlMassnTakeCover1,	ARRAYSIZE(tlMassnTakeCover1), 0, 0,	"TakeCover"},
};

Task_t	tlMassnGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_MASSASSIN_WAIT_FACE_ENEMY	},
};

Schedule_t	slMassnGrenadeCover[] =
{
	{tlMassnGrenadeCover1, ARRAYSIZE(tlMassnGrenadeCover1),	0, 0, "GrenadeCover"},
};

Task_t	tlMassnTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,		(float)0							},
	{ TASK_RANGE_ATTACK2, 	(float)0							},
	{ TASK_SET_SCHEDULE,	(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slMassnTossGrenadeCover[] =
{
	{tlMassnTossGrenadeCover1, ARRAYSIZE(tlMassnTossGrenadeCover1), 0,0, "TossGrenadeCover"},
};

Task_t	tlMassnTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slMassnTakeCoverFromBestSound[] =
{
	{tlMassnTakeCoverFromBestSound,	ARRAYSIZE(tlMassnTakeCoverFromBestSound), 0, 0,	"TakeCoverFromBestSound"},
};

Task_t	tlMassnHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slMassnHideReload[] =
{
	{
		tlMassnHideReload,
		ARRAYSIZE ( tlMassnHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"HideReload"
	}
};

Task_t	tlMassnSweep[] =
{
	{ TASK_TURN_LEFT,	(float)179	},
	{ TASK_WAIT,		(float)1	},
	{ TASK_TURN_LEFT,	(float)179	},
	{ TASK_WAIT,		(float)1	},
};

Schedule_t	slMassnSweep[] =
{
	{
		tlMassnSweep,
		ARRAYSIZE ( tlMassnSweep ),

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

Task_t	tlMassnRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slMassnRangeAttack1A[] =
{
	{
		tlMassnRangeAttack1A,
		ARRAYSIZE ( tlMassnRangeAttack1A ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_MASSASSIN_NOFIRE		|
		bits_COND_SEE_FEAR			|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};

Task_t	tlMassnRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MASSASSIN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slMassnRangeAttack1B[] =
{
	{
		tlMassnRangeAttack1B,
		ARRAYSIZE ( tlMassnRangeAttack1B ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_MASSASSIN_NOFIRE		|
		bits_COND_SEE_FEAR			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

Task_t	tlMassnRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_MASSASSIN_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MASSASSIN_WAIT_FACE_ENEMY	},
};

Schedule_t	slMassnRangeAttack2[] =
{
	{tlMassnRangeAttack2, ARRAYSIZE(tlMassnRangeAttack2), 0, 0, "RangeAttack2"},
};

Task_t	tlMassnRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slMassnRepel[] =
{
	{
		tlMassnRepel,
		ARRAYSIZE ( tlMassnRepel ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel"
	},
};

Task_t	tlMassnRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slMassnRepelAttack[] =
{
	{tlMassnRepelAttack, ARRAYSIZE(tlMassnRepelAttack),	bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack"},
};

Task_t	tlMassnRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0	},
	{ TASK_RUN_PATH,				(float)0	},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0	},
	{ TASK_CLEAR_LASTPOSITION,		(float)0	},
};

Schedule_t	slMassnRepelLand[] =
{
	{
		tlMassnRepelLand,
		ARRAYSIZE ( tlMassnRepelLand ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel Land"
	},
};

Task_t	tlMassnCheckSound[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSOUND,	(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE_ANGRY	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_MASSASSIN_SWEEP	},
	{ TASK_WAIT,					(float)4				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slMassnCheckSound[] =
{
	{
		tlMassnCheckSound,
		ARRAYSIZE ( tlMassnCheckSound ),

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

DEFINE_CUSTOM_SCHEDULES( CMassassin )
{
	slMassnFail,
	slMassnCombatFail,
	slMassnVictoryDance,
	slMassnEstablishLineOfFire,
	slMassnFoundEnemy,
	slMassnCombatFace,
	slMassnSignalSuppress,
	slMassnSuppress,
	slMassnWaitInCover,
	slMassnTakeCover,
	slMassnGrenadeCover,
	slMassnTossGrenadeCover,
	slMassnTakeCoverFromBestSound,
	slMassnHideReload,
	slMassnSweep,
	slMassnRangeAttack1A,
	slMassnRangeAttack1B,
	slMassnRangeAttack2,
	slMassnRepel,
	slMassnRepelAttack,
	slMassnRepelLand,
	slMassnCheckSound,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMassassin, CSquadMonster );

void CMassassin :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		if (FBitSet( pev->weapons, MASSASSIN_9MMAR))
		{
			if ( m_fStanding )
				iSequence = LookupSequence( "standing_mp5" );
			else
				iSequence = LookupSequence( "crouching_mp5" );
		}
		else
		{
			if ( m_fStanding )
				iSequence = LookupSequence( "standing_sniper" );
			else
				iSequence = LookupSequence( "crouching_sniper" );
		}
		break;
	case ACT_RANGE_ATTACK2:
		if ( pev->weapons & MASSASSIN_HANDGRENADE )
			iSequence = LookupSequence( "throwgrenade" );
		else
			iSequence = LookupSequence( "launchgrenade" );
		break;

	case ACT_RUN:
		if ( pev->health <= MASSASSIN_LIMP_HEALTH )
			iSequence = LookupActivity ( ACT_RUN_HURT );
		else
			iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
		if ( pev->health <= MASSASSIN_LIMP_HEALTH )
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
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;
	}
}

Schedule_t *CMassassin :: GetSchedule(void)
{
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_MASSASSIN_REPEL_LAND );
		}
		else
		{
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_MASSASSIN_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_MASSASSIN_REPEL );
		}
	}
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound = PBestSound();
		if (pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	
			if (m_MonsterState == MONSTERSTATE_IDLE && ((pSound->m_iType & bits_SOUND_COMBAT) || (pSound->m_iType & bits_SOUND_DEATH)))
				return GetScheduleOfType( SCHED_MASSASSIN_CHECK_SOUND );
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
				return CBaseMonster :: GetSchedule();

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					else
					{
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
							return GetScheduleOfType ( SCHED_MASSASSIN_SUPPRESS );
						else
							return GetScheduleOfType ( SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
					}
				}
			}
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
				return GetScheduleOfType ( SCHED_MASSASSIN_COVER_AND_RELOAD );

			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				else
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );

			else if ( FBitSet( pev->weapons, MASSASSIN_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );

			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_MASSASSIN_FOUND_ENEMY );
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
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );

				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
					return GetScheduleOfType( SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
				else
					return GetScheduleOfType( SCHED_STANDOFF );
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				return GetScheduleOfType ( SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
		}
	default:
		{
			if (HasConditions(bits_COND_SEE_FEAR))
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
	}
	return CSquadMonster :: GetSchedule();
}

Schedule_t* CMassassin :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( gSkillData.iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
					return slMassnTossGrenadeCover;
				else
					return &slMassnTakeCover[ 0 ];
			}
			else
			{
				if ( RANDOM_LONG(0,1) )
					return &slMassnTakeCover[ 0 ];
				else
					return &slMassnGrenadeCover[ 0 ];
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
			return &slMassnTakeCoverFromBestSound[ 0 ];

	case SCHED_MASSASSIN_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_MASSASSIN_ELOF_FAIL:
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		break;

	case SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE:
		return &slMassnEstablishLineOfFire[ 0 ];
		break;

	case SCHED_RANGE_ATTACK1:
		{
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);

			if (m_fStanding)
				return &slMassnRangeAttack1B[ 0 ];
			else
				return &slMassnRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
			return &slMassnRangeAttack2[ 0 ];

	case SCHED_COMBAT_FACE:
			return &slMassnCombatFace[ 0 ];

	case SCHED_MASSASSIN_WAIT_FACE_ENEMY:
			return &slMassnWaitInCover[ 0 ];

	case SCHED_MASSASSIN_SWEEP:
			return &slMassnSweep[ 0 ];

	case SCHED_MASSASSIN_COVER_AND_RELOAD:
			return &slMassnHideReload[ 0 ];

	case SCHED_MASSASSIN_FOUND_ENEMY:
			return &slMassnFoundEnemy[ 0 ];

	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
					return &slMassnFail[ 0 ];
			}
			return &slMassnVictoryDance[ 0 ];
		}
	case SCHED_MASSASSIN_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;
				return &slMassnSignalSuppress[ 0 ];
			}
			else
				return &slMassnSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
				return &slMassnCombatFail[ 0 ];

			return &slMassnFail[ 0 ];
		}
	case SCHED_MASSASSIN_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slMassnRepel[ 0 ];
		}
	case SCHED_MASSASSIN_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slMassnRepelAttack[ 0 ];
		}
	case SCHED_MASSASSIN_REPEL_LAND:
			return &slMassnRepelLand[ 0 ];

	case SCHED_MASSASSIN_CHECK_SOUND:
			return &slMassnCheckSound[0];

	default:
			return CSquadMonster :: GetScheduleOfType ( Type );
	}
}

void CMassassin::DropItems(void)
{
	if (GetBodygroup(MASSN_GUN_GROUP) == GUN_NONE)
		return;

	Vector vecGunPos;
	Vector vecGunAngles;
	GetAttachment(0, vecGunPos, vecGunAngles);
	SetBodygroup(MASSN_GUN_GROUP, GUN_NONE);

	CBaseEntity *pGun = NULL;
	if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
		pGun = DropItem("weapon_357", vecGunPos, vecGunAngles);
	else
		pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);

	if (pGun)
	{
		pGun->pev->velocity[0] += RANDOM_FLOAT(-100,100);
		pGun->pev->velocity[1] += RANDOM_FLOAT(-100,100);
		pGun->pev->velocity[2] += RANDOM_FLOAT(-100,100);
	}

	if (FBitSet(pev->weapons, MASSASSIN_GRENADELAUNCHER))
	{
		pGun = DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
		if (pGun)
		{
			pGun->pev->velocity[0] += RANDOM_FLOAT(-100,100);
			pGun->pev->velocity[1] += RANDOM_FLOAT(-100,100);
			pGun->pev->velocity[2] += RANDOM_FLOAT(-100,100);
		}
	}

	if (FBitSet(pev->weapons, MASSASSIN_HANDGRENADE))
	{
		CBasePlayerItem *pItem = (CBasePlayerItem *)DropItem("weapon_handgrenade", BodyTarget(pev->origin)+UTIL_RandomBloodVector()*2.0f, RandomVector(90.0f));
		if (pItem && pItem->GetWeaponPtr())
		{
			pItem->pev->velocity *= 0.5f;
			pItem->pev->velocity += RandomVector(80.0f);
			pItem->GetWeaponPtr()->m_iDefaultAmmo = RANDOM_LONG(1, HANDGRENADE_DEFAULT_GIVE);
		}
	}
}

/*
class CMassassinRepel : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;
};

LINK_ENTITY_TO_CLASS( monster_assassin_repel, CMassassinRepel );

void CMassassinRepel::Spawn(void)
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse(&CMassassinRepel::RepelUse );
}

void CMassassinRepel::Precache(void)
{
	UTIL_PrecacheOther( "monster_male_assassin" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CMassassinRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0.0f, 0.0f, -4096.0f), dont_ignore_monsters, ENT(pev), &tr);

	CBaseEntity *pEntity = Create( "monster_male_assassin", pev->origin, pev->angles );
	if (pEntity)
	{
		CBaseMonster *pGrunt = pEntity->MyMonsterPointer( );
		pGrunt->pev->movetype = MOVETYPE_FLY;
		pGrunt->pev->velocity.Set( 0.0f, 0.0f, RANDOM_FLOAT( -196, -128 ) );
		pGrunt->SetActivity( ACT_GLIDE );

		pGrunt->m_vecLastPosition = tr.vecEndPos;

		CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
		if (pBeam)
		{
			pBeam->PointEntInit( pev->origin + Vector(0,0,112), pGrunt->entindex() );
			pBeam->SetFlags( BEAM_FSOLID );
			pBeam->SetColor( 255, 255, 255 );
			pBeam->SetNoise(RANDOM_LONG(0,2));
			pBeam->SetThink(&CBaseEntity::SUB_Remove);
			pBeam->SetNextThink(-4096.0f * tr.flFraction / pGrunt->pev->velocity.z + 0.5f);
		}
	}
	else
		ALERT(at_aiconsole, "Warning: %s %s: failed to create!\n", STRING(pev->classname), STRING(pev->targetname));

	UTIL_Remove( this );
}
*/
