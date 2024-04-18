#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "player.h"// XDM
#include "decals.h"
#include "soundent.h"
#include "game.h"
#include "gamerules.h"
#include "pm_materials.h"

#define	BARNACLE_BODY_HEIGHT		40// barnacle's model height
#define BARNACLE_PULL_SPEED			8
#define BARNACLE_CHECK_SPACING		8
#define BARNACLE_MAX_LENGTH			1024
// XDM3038a: flags
#define BARNACLE_EATEN_HUMAN		0x01
#define BARNACLE_EATEN_ALIEN		0x02
#define BARNACLE_EATEN_CUSTOMGIBS	0x04
// Custom animation event
#define	BARNACLE_AE_PUKEGIB			2

// XDM3038c: changed victim storage from m_hEnemy to m_hTargetEnt to avoid base class conflict
class CBarnacle : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void);
	virtual int IRelationship(CBaseEntity *pTarget);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual bool IsPushable(void) { return pev->deadflag == DEAD_DEAD; }// XDM: detached
	virtual bool HasAlienGibs(void);
	virtual bool HasHumanGibs(void);
	virtual void AlertSound(void) {}// in case this ever gets called, disable sound
	virtual void AttackSound(void);
	virtual void DeathSound(void);
	virtual const char *GetDropItemName(void) { return "item_sodacan";}// XDM3038
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	CBaseEntity *TongueTouchEnt(float *pflLength);
	void EXPORT BarnacleThink(void);
	void EXPORT WaitTillDead(void);
	void EXPORT BodyFallThink(void);// XDM
	void ThrowGibs(int count);// XDM3038c

	float m_flAltitude;
	float m_flTongueAdj;
	int m_cGibs;// barnacle loads up on gibs each time it kills something
	int m_iEatenFlags;// XDM3038a
	BOOL m_fTongueExtended;
	BOOL m_fLiftingPrey;

	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS(monster_barnacle, CBarnacle);

TYPEDESCRIPTION	CBarnacle::m_SaveData[] =
{
	DEFINE_FIELD(CBarnacle, m_flAltitude, FIELD_FLOAT),
	DEFINE_FIELD(CBarnacle, m_cGibs, FIELD_INTEGER),
	DEFINE_FIELD(CBarnacle, m_iEatenFlags, FIELD_INTEGER),
	DEFINE_FIELD(CBarnacle, m_fTongueExtended, FIELD_BOOLEAN),
	DEFINE_FIELD(CBarnacle, m_fLiftingPrey, FIELD_BOOLEAN),
	DEFINE_FIELD(CBarnacle, m_flTongueAdj, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CBarnacle, CBaseMonster);

const char *CBarnacle::pAlertSounds[] = 
{
	//"barnacle/bcl_alert1.wav",
	"barnacle/bcl_alert2.wav",
};

const char *CBarnacle::pAttackSounds[] = 
{
	"barnacle/bcl_chew1.wav",
	"barnacle/bcl_chew2.wav",
	"barnacle/bcl_chew3.wav",
};

const char *CBarnacle::pDeathSounds[] = 
{
	"barnacle/bcl_die1.wav",
	//"barnacle/bcl_die2.wav",
	"barnacle/bcl_die3.wav",
};

void CBarnacle::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.barnacleHealth;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.barnacleScore;
	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iGibCount == 0)
		m_iGibCount = 10;// XDM: medium one
	if (m_flDamage1 == 0)// XDM3038c
		m_flDamage1 = gSkillData.barnacleDmgBite;

	CBaseMonster::Spawn();// XDM3038b: Precache();
	UTIL_SetSize(this, Vector(-16, -16, -BARNACLE_BODY_HEIGHT), Vector(16, 16, 0));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_AIM;
	pev->view_ofs.Set(0,0,-BARNACLE_BODY_HEIGHT);
	pev->effects			= EF_INVLIGHT; // take light from the ceiling
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	MonsterInit();// IMPORTANT! Resets common vars like enemy!
	m_flNextAttack		= -1;//m_flKillVictimTime	= -1;
	m_cGibs				= RANDOM_LONG(1,3);
	m_fLiftingPrey		= FALSE;
	m_flTongueAdj		= -100;
	// Done in MonsterInit InitBoneControllers();
	SetActivity(ACT_IDLE);
	SetThink(&CBarnacle::BarnacleThink);
	SetNextThink(0.5);
	UTIL_SetOrigin(this, pev->origin);
}

void CBarnacle::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/barnacle.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "barnacle");

	CBaseMonster::Precache();// XDM3038

	// XDM3038c: arrays and macros
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND("barnacle/bcl_bite3.wav");
}

int	CBarnacle::Classify(void)
{
	return m_iClass?m_iClass:CLASS_BARNACLE;// XDM3038a
}

// XDM: special
int CBarnacle::IRelationship(CBaseEntity *pTarget)
{
	if (pTarget)
	{
		switch (pTarget->Classify())
		{
		case CLASS_PLAYER:
		case CLASS_HUMAN_PASSIVE:
		case CLASS_ALIEN_PREY:
		case CLASS_PLAYER_ALLY:
		case CLASS_GRENADE:
		case CLASS_GIB:
			{
				return R_HT;
				break;
			}
		case CLASS_MACHINE:
		case CLASS_HUMAN_MILITARY:
			{
				return R_DL;
				break;
			}
		}
		if (pTarget->IsPushable() && !pTarget->IsBSPModel())// XDM3038c: TESTME: pull everything laying around
		{
			if (!pTarget->IsPickup())// bad if weapon, ammo or item gets destroyed
				return R_DL;
		}
	}
	return CBaseMonster::IRelationship(pTarget);
}

void CBarnacle::DeathSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

void CBarnacle::AttackSound(void)
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

// XDM3038a
bool CBarnacle::HasAlienGibs(void)
{
	return false;//return FBitSet(m_iEatenFlags, BARNACLE_EATEN_ALIEN);
}

// XDM3038c NO! We need this during precache to set m_iGibModelIndex!
bool CBarnacle::HasHumanGibs(void)
{
	if (pev->deadflag == DEAD_NO)// Little hack: during spawn/precache we need to tell the monster code to precache human gibs by default
		return true;
	else// otherwise, during GibMonster() we need to use "custom gibs"
		return false;
	//return !FBitSet(m_iEatenFlags, BARNACLE_EATEN_ALIEN|BARNACLE_EATEN_CUSTOMGIBS);
	// BAD return FBitSet(m_iEatenFlags, BARNACLE_EATEN_HUMAN);// XDM3038a
}

void CBarnacle::ThrowGibs(int count)// XDM3038c
{
	if (count > 0)
	{
		int gcolor;
		int gflags;
		if (FBitSet(m_iEatenFlags, BARNACLE_EATEN_HUMAN))
		{
			gcolor = BLOOD_COLOR_RED;
			gflags = BREAK_FLESH;
		}
		else if (FBitSet(m_iEatenFlags, BARNACLE_EATEN_ALIEN))
		{
			gcolor = BLOOD_COLOR_RED;
			gflags = BREAK_FLESH;
		}
		else
		{
			gcolor = DONT_BLEED;
			gflags = 0;
		}
		CGib::SpawnModelGibs(this, EyePosition(), pev->mins, pev->maxs, Vector(0,0,-1), 10, m_iGibModelIndex, count, GIB_LIFE_DEFAULT, matFlesh, gcolor, gflags);
		if (gcolor != DONT_BLEED && g_pGameRules->FAllowEffects())
			UTIL_BloodDrips(EyePosition(), Vector(0.0f,0.0f,-1.0f), gcolor, count*10);
	}
}

void CBarnacle::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case BARNACLE_AE_PUKEGIB:
		ThrowGibs(m_cGibs+1);//CGib::SpawnRandomGibs(this, 1, true, Vector(0,0,-1));
		// this is a rare event (during death), also leave gibs for GibMonster()! m_cGibs = 0;
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

int CBarnacle::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if ((bitsDamageType & DMG_CLUB) && pev->health > (pev->max_health * 0.1f))// XDM3035: this may be a dead monster with 0 health. Kill instantly if alive and hit with crowbar.
		flDamage = pev->health;

	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

void CBarnacle::BarnacleThink(void)
{
	SetNextThink(0.1);

	if (m_hTargetEnt.Get())// barnacle has prey.
	{
		CBaseEntity *pEntity = m_hTargetEnt;
		if ((pEntity->IsMonster() || pEntity->IsPlayer()) && (pEntity->pev->solid == SOLID_NOT || FBitSet(pEntity->pev->effects, EF_NODRAW)))// XDM3038c: fail! Eat bodies! !pEntity->IsAlive())// XDM3038a: projectiles are not "alive"
		{
			// someone (maybe even the barnacle) killed the prey. Reset barnacle.
			m_fLiftingPrey = FALSE;// indicate that we're not lifting prey.
			m_hTargetEnt = NULL;
			return;
		}

		if (m_fLiftingPrey)// still pulling prey.
		{
			if (fabs(pev->origin.z - (pEntity->pev->origin.z + pEntity->pev->view_ofs.z - 8)) < BARNACLE_BODY_HEIGHT)
			{
				// prey has just been lifted into position (if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body)
				m_fLiftingPrey = FALSE;
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "barnacle/bcl_bite3.wav", GetSoundVolume(), ATTN_NORM);
				// XDM: barnacle now will bite prey every once in a while :)
				pEntity->BarnacleVictimBitten(this);
				SetActivity(ACT_EAT);
				// XDM3037a: even if it's not a monster :)
				m_flNextAttack = gpGlobals->time;// now that the victim is in place, the killing bite will be administered in 10 seconds.
			}
			else
			{
				Vector vecNewEnemyOrigin(pev->origin);// = pEntity->pev->origin;
				// guess as to where their neck is
				vecNewEnemyOrigin.x -= 6.0f * cos(pEntity->pev->angles.y * M_PI/180.0f);
				vecNewEnemyOrigin.y -= 6.0f * sin(pEntity->pev->angles.y * M_PI/180.0f);
				vecNewEnemyOrigin.z = pEntity->pev->origin.z + BARNACLE_PULL_SPEED;// XDM3037a
				m_flAltitude -= BARNACLE_PULL_SPEED;
				UTIL_SetOrigin(pEntity, vecNewEnemyOrigin);
				pEntity->pev->origin = vecNewEnemyOrigin;
			}
		}
		else// prey is lifted fully into feeding position and is dangling there.
		{
			pEntity->pev->velocity.Clear();
			if (m_flNextAttack > 0 && gpGlobals->time >= m_flNextAttack)
			{
				// kill!
				if (!pEntity->IsRemoving() && pEntity->pev->solid != SOLID_NOT && !FBitSet(pEntity->pev->effects, EF_NODRAW))// XDM3038c: was (pEntity->IsAlive())
				{
					int iNewGibModelIndex = m_iGibModelIndex;// XDM3038c: in case it changes after death
					CBaseMonster *pVictim = pEntity->MyMonsterPointer();// can be NULL!
					if (pVictim)
					{
						if (pVictim->HasAlienGibs())
							SetBits(m_iEatenFlags, BARNACLE_EATEN_ALIEN);
						else if (pVictim->HasHumanGibs())
							SetBits(m_iEatenFlags, BARNACLE_EATEN_HUMAN);
						else if (pVictim->m_iGibModelIndex > 0)// XDM3038c
							SetBits(m_iEatenFlags, BARNACLE_EATEN_CUSTOMGIBS);

						if (m_iEatenFlags != 0 && pVictim->m_iGibCount > 0)// XDM3038c
							m_cGibs += pVictim->m_iGibCount/2;

						iNewGibModelIndex = pVictim->m_iGibModelIndex;// XDM3038c: in case it changes after death
						if (sv_overdrive.value > 0)
							pev->scale += 0.01;
					}
					else
						++m_cGibs;

					bool bMachine = (pEntity->Classify() == CLASS_MACHINE);
					pEntity->BarnacleVictimBitten(this);
					pEntity->TakeDamage(this, this, m_flDamage1, DMG_ALWAYSGIB);// don't include any of DMGM_PUSH!!!

					if (!pEntity->IsRemoving() && pEntity->pev->health > 0)// IsAlive())// XDM3035b: check NOW!
					{
						AttackSound();
						m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(1.0, 2.0);// XDM
					}
					else// we destroyed it
					{
						m_hTargetEnt = NULL;
						m_flNextAttack = -1;
						if (iNewGibModelIndex > 0)
							m_iGibModelIndex = iNewGibModelIndex;// throw up what it has eaten, rely on the victim to have that model precached
					}

					// XDM3035: oops! moved
					if (bMachine)// XDM3034 IsEdible()?
					{
						m_hTargetEnt = NULL;
						if (!pEntity->IsRemoving())
						{
							pEntity->pev->origin = pev->origin - pEntity->pev->view_ofs;// XDM3037a
							pEntity->pev->origin.z -= BARNACLE_BODY_HEIGHT;
							pEntity->BarnacleVictimReleased();
							if (pEntity->IsAlive())
							{
								if (pVictim)
								{
									//pVictim->m_hEnemy = this;
									pVictim->SetMonsterState(MONSTERSTATE_ALERT);
									pVictim->SetConditions(bits_COND_SEE_FEAR | bits_COND_LIGHT_DAMAGE);
									pVictim->ChangeSchedule(pVictim->GetScheduleOfType(SCHED_TAKE_COVER_FROM_ORIGIN));
									//pVictim->RunTask(TASK_FIND_COVER_FROM_ORIGIN);
								}
							}
							pEntity->pev->velocity.Set(RANDOM_FLOAT(-4,4), RANDOM_FLOAT(-4,4), -g_psv_gravity->value*0.05f);
						}
						SetNextThink(RANDOM_FLOAT(2.0, 3.0));
						return;
					}
				}
				else
					m_flNextAttack = -1;// XDM
			}
		}
	}
	else// m_hTargetEnt !null
	{
		// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		vec_t fPlayerSearchRadius = 640;
		bool bPlayersNearby = false;
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
			if (pPlayer)
			{
				Vector delta(pev->origin - pPlayer->pev->origin);
				if (delta.Length() <= fPlayerSearchRadius)
				{
					bPlayersNearby = true;// XDM3038c: a quick hack to detect presence of players
					break;
				}
			}
		}
		if (!bPlayersNearby)//if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
		{
			SetNextThink(RANDOM_FLOAT(1.0, 1.5));// Stagger a bit to keep barnacles from thinking on the same frame
			return;
		}

		if (m_fSequenceFinished)// this is done so barnacle will fidget.
		{
			SetActivity(ACT_IDLE);
			m_flTongueAdj = -100;
		}

		if (m_cGibs > 0 && RANDOM_LONG(0,50) == 1)
		{
			// cough up a gib.
			ThrowGibs(1);//CGib::SpawnRandomGibs(this, 1, true, Vector(0,0,-1));
			m_cGibs--;
			AttackSound();
		}

		vec_t flLength = 0.0f;
		CBaseEntity *pTouchEnt = TongueTouchEnt(&flLength);
		if (pTouchEnt != NULL && m_fTongueExtended)// tongue is fully extended, and is touching someone.
		{
			if (pTouchEnt->FBecomeProne())
			{
				EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);// XDM3038c: we don't call AlertSound() here because it may eventually make barnacles make undesired sounds
				SetSequenceByName("attack1");
				m_flTongueAdj = -20;
				m_hTargetEnt = pTouchEnt;
				pTouchEnt->pev->movetype = MOVETYPE_FLY;
				pTouchEnt->pev->velocity.Clear();
				pTouchEnt->pev->basevelocity.Clear();
				pTouchEnt->pev->origin.x = pev->origin.x;
				pTouchEnt->pev->origin.y = pev->origin.y;
				if (pTouchEnt->IsPlayer())
					pTouchEnt->MyMonsterPointer()->m_hEnemy = this;// XDM3038c: a little hack for players to track barnacle existence

				m_fLiftingPrey = TRUE;// indicate that we should be lifting prey.
				m_flNextAttack = -1;// set this to a bogus time while the victim is lifted.
				m_flAltitude = (pev->origin.z - pTouchEnt->EyePosition().z);
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it.
			if (m_flAltitude < flLength)
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += BARNACLE_PULL_SPEED;
				m_fTongueExtended = FALSE;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = TRUE;
			}
		}
	}
	// ALERT( at_console, "tounge %f\n", m_flAltitude + m_flTongueAdj );
	SetBoneController(0, -(m_flAltitude + m_flTongueAdj));
	StudioFrameAdvance(0.1);
	SetNextThink(0.1);
}

void CBarnacle::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	if (m_hTargetEnt.Get())
	{
		m_hTargetEnt->BarnacleVictimReleased();
		m_hTargetEnt = NULL;
	}

	if (HasMemory(bits_MEMORY_KILLED))// XDM3037: animation finished, detached: falling or on ground
	{
		CBaseMonster::Killed(pInflictor, pAttacker, iGib);
		return;
	}

	if (pev->deadflag == DEAD_DYING)// XDM3037: playing death animation. don't restart it.
		return;

	// XDM3037: never played animation yet.
	pev->solid = SOLID_NOT;// so the prey may fall freely

	if (ShouldGibMonster(iGib))// in may be an explosion, so no detaching
	{
		CallGibMonster();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}

	TraceResult	tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0,0,BARNACLE_BODY_HEIGHT), ignore_monsters, ENT(pev), &tr);
	UTIL_DecalTrace(&tr, DECAL_BLOODSMEARR1);// big spot
	DeathSound();
	pev->deadflag = DEAD_DYING;
	SetActivity(ACT_DIESIMPLE);
	SetBoneController(0, 0);
	StudioFrameAdvance(0.1);
	SetThink(&CBarnacle::WaitTillDead);
	SetNextThink(0.1);
}

void CBarnacle::WaitTillDead(void)
{
	SetNextThink(0.1);
	float flInterval = StudioFrameAdvance(0.1);
	DispatchAnimEvents(flInterval);
	if (m_fSequenceFinished)
	{
		// death anim finished.
		StopAnimation();
		BecomeDead();// XDM \V/
		Remember(bits_MEMORY_KILLED);
		pev->solid = SOLID_BBOX;
		pev->deadflag = DEAD_DEAD;
		pev->movetype = MOVETYPE_TOSS;// XDM3035: fix
		pev->takedamage = DAMAGE_YES;// XDM3035: not aim
		SetThink(&CBarnacle::BodyFallThink);// XDM
		SetNextThink(0.1);
	}
}

void CBarnacle::BodyFallThink(void)// XDM
{
	if (pev->flags & FL_ONGROUND)
	{
		CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 200, 10);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "common/bodydrop2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
		if (IsMultiplayer())
		{
			SetThink(&CBaseEntity::SUB_Remove);
			SetNextThink(5.0f);
		}
		else
			SetThinkNull();
	}
	else
		SetNextThink(0.1);
}

// Does a trace along the barnacle's tongue to see if any entity is touching it.
// Also stores the length of the trace in the int pointer provided.
CBaseEntity *CBarnacle::TongueTouchEnt(float *pflLength)
{
	TraceResult	tr;
	vec_t length;
	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, BARNACLE_MAX_LENGTH), ignore_monsters, ENT(pev), &tr);
	length = fabs(pev->origin.z - tr.vecEndPos.z);
	if (pflLength)
		*pflLength = length;

	Vector delta(BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0);
	Vector mins(pev->origin - delta);
	Vector maxs(pev->origin + delta);
	maxs.z = pev->origin.z;
	mins.z -= (length+1.0f);// XDM3035: +1 TESTME!
	CBaseEntity *pList[10];
	size_t count = UTIL_EntitiesInBox(pList, 10, mins, maxs, 0);// XDM3037a (FL_CLIENT|FL_MONSTER));
	if (count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			if (pList[i] != this && pList[i]->IsPushable() && IRelationship(pList[i]) > R_NO/* XDM3038c: :D && pList[i]->pev->deadflag == DEAD_NO*/ && !FBitSet(pList[i]->pev->flags, FL_NOTARGET))
				return pList[i];// this ent is one of our enemies. Barnacle tries to eat it.
		}
	}
	return NULL;
}
