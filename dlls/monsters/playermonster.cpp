#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "gamerules.h"
#include "game.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_MONSTERPLAYER_NOTSOLID					4 

// for scripted sequence use.
class CPlayerMonster : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual int Classify (void);
	virtual int ISoundMask (void);
};
LINK_ENTITY_TO_CLASS(monster_player, CPlayerMonster);

int	CPlayerMonster::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

void CPlayerMonster::SetYawSpeed(void)
{
	pev->yaw_speed = 90.0f;
}

int CPlayerMonster::ISoundMask(void)
{
	return	NULL;
}

void CPlayerMonster::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = g_pGameRules->GetPlayerMaxHealth();// XDM3038a
	if (pev->armorvalue <= 0)
		pev->armorvalue = g_pGameRules->GetPlayerMaxArmor()/10;// XDM3038a
	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;

	CBaseMonster::Spawn();// XDM3038b: Precache( );

	//SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->health			= MAX_PLAYER_HEALTH;// XDM3038c
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	MonsterInit();
	if (FBitSet(pev->spawnflags, SF_MONSTERPLAYER_NOTSOLID))
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
}

void CPlayerMonster :: Precache()
{
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/player.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "player");

	CBaseMonster::Precache();// XDM3038a
}	


class CDeadHEV : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);// XDM3037
	virtual int	Classify(void) { return CLASS_GIB; }// XDM3035b
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual bool IsAlive(void) const { return false; }// XDM3038c
	virtual void KeyValue(KeyValueData *pkvd);
	virtual const char *GetDropItemName(void) { return "item_battery";}// XDM3038
	int	m_iPose;// don't save
	static char *m_szPoses[4];
};

char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

void CDeadHEV::Spawn(void)
{
	CBaseMonster::Spawn();// XDM3038b: Precache();
	//pev->effects = 0;// clear any yellow files
	pev->yaw_speed = 0;
	pev->body = 1;// COMPATIBILITY: helmet
	if (pev->colormap == 0)// XDM3038c
		pev->colormap = RANDOM_LONG(0x00000000,0x0000FFFF);// XDM3037a
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;// XDM: medium one

	const char *pose = m_szPoses[clamp(m_iPose,0,3)];
	pev->sequence = LookupSequence(pose);
	if (pev->sequence == -1)
	{
		conprintf(0, "Design error: %s[%d] with bad pose: %s!\n", STRING(pev->classname), entindex(), pose);
		pev->sequence = 0;
		if (g_pCvarDeveloper && g_pCvarDeveloper->value >= 2.0f)
			SetBits(pev->effects, EF_BRIGHTFIELD);
	}
	// Corpses have less health
	pev->health = MAX_PLAYER_HEALTH*0.2f;// XDM
	MonsterInitDead();
}

void CDeadHEV::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/player.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "player");

	CBaseMonster::Precache();// XDM3038
}
