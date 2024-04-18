#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "game.h"

LINK_ENTITY_TO_CLASS(monster_rat, CRat);

const char *CRat::pIdleSounds[] =
{
	"rat/rat_idle1.wav",
	"rat/rat_idle2.wav",
};
const char *CRat::pAlertSounds[] =
{
	"rat/rat_alert1.wav",
	"rat/rat_alert2.wav",
};
const char *CRat::pAttackSounds[] =
{
	"rat/rat_attack.wav",
};
const char *CRat::pDeathSounds[] =
{
	"rat/rat_die1.wav",
	"rat/rat_die2.wav",
	"rat/rat_die3.wav",
	"rat/rat_die4.wav",
};
const char *CRat::pAttackHitSounds[] =
{
	"rat/rat_bite.wav",
};

void CRat::SetYawSpeed(void)
{
	pev->yaw_speed = 100;
}

void CRat::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.ratHealth;
	if (pev->dmg == 0)// XDM3037
		pev->dmg = gSkillData.ratDmgBite;
	//if (m_voicePitch == 0)// XDM3038b: before MonsterInit()!
	//	m_voicePitch = PITCH_NORM;// sounds should be high themselves
	if (m_iClass == 0)// XDM3038b
		m_iClass = CLASS_INSECT;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iGibCount == 0)
		m_iGibCount = 1;// XDM: very small one
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.ratScore;

	CBaseMonster::Spawn();// XDM3038b: Precache( );
	UTIL_SetSize(this, Vector(-2, -2, 0), Vector(2, 2, 4));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->takedamage		= DAMAGE_YES;
	pev->view_ofs.Set(0, 0, 4);// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	MonsterInit();
}

void CRat::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/bigrat.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "rat");

	CBabyCrab::Precache();// XDM3038a

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
}

void CRat::IdleSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch());
}
void CRat::AlertSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch());
}
// XDM3038c
void CRat::AttackSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), GetSoundVolume(), ATTN_STATIC, 0, GetSoundPitch());
}
// XDM3038c
void CRat::AttackHitSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackHitSounds), GetSoundVolume(), ATTN_STATIC, 0, GetSoundPitch());
}
void CRat::PainSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch());
}
void CRat::DeathSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolume(), ATTN_IDLE, 0, GetSoundPitch());
}

BOOL CRat::CheckRangeAttack1(float flDot, float flDist)
{
	if (pev->flags & FL_ONGROUND)
	{
		if (pev->groundentity && (pev->groundentity->v.flags & (FL_CLIENT|FL_MONSTER)))
			return TRUE;

		// A little less accurate, but jump from closer
		if (flDist <= 180 && flDot >= 0.55)
			return TRUE;
	}
	return FALSE;
}

// headcrabs (base class) do not take acid damage, but rats do.
int CRat::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType == DMG_RADIATION && flDamage < (pev->health * 10.0f))// == ONLY radiation
		flDamage = 0;

	return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}
