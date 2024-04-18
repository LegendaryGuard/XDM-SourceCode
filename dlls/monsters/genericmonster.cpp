#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "talkmonster.h"

#define	SF_GENERICMONSTER_NOTSOLID			4// For holograms mostly
#define	SF_HEAD_CONTROLLER					8

// For scripted scenes mostly
class CGenericMonster : public CTalkMonster
{
public:
	virtual void Spawn(void);
	virtual void SetYawSpeed(void);
	virtual int Classify(void);
	virtual int ISoundMask(void);
	virtual bool IsPushable(void) { return false; }// XDM3035: holograms use this
};
LINK_ENTITY_TO_CLASS(monster_generic, CGenericMonster);

int	CGenericMonster::Classify(void)
{
	return m_iClass?m_iClass:CLASS_NONE;// XDM3038: nobody gets pissed off if this monster gets shot
}

void CGenericMonster::SetYawSpeed(void)
{
	switch (m_Activity)
	{
	case ACT_RUN:
	case ACT_GUARD:
		pev->yaw_speed = 120;
		break;
	case ACT_IDLE:
		pev->yaw_speed = 90;
		break;
	case ACT_EAT:
		pev->yaw_speed = 80;
		break;
	default:
		CTalkMonster::SetYawSpeed();// XDM3038
		break;
	}
}

int CGenericMonster::ISoundMask(void)
{
	return	NULL;
}

void CGenericMonster::Spawn(void)
{
	if (m_bloodColor <= 0)
		m_bloodColor = BLOOD_COLOR_RED;
	if (m_iGibCount <= 0)
		m_iGibCount = 16;// XDM: medium one
	if (pev->health <= 0)
		pev->health = 8;

	CTalkMonster::Spawn();// XDM3038b: Precache();

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetModelCollisionBox();// XDM3038

	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	if (FBitSet(pev->spawnflags, SF_GENERICMONSTER_NOTSOLID))// XDM3038: moved before MonsterInit()
	{
		pev->solid = SOLID_NOT;
		//pev->takedamage = DAMAGE_NO; hack mainly used by holograms in HC
	}

	MonsterInit();

	if (FBitSet(pev->spawnflags, SF_GENERICMONSTER_NOTSOLID))// XDM3038a: :(
		pev->takedamage = DAMAGE_NO;

	if (FBitSet(pev->spawnflags, SF_HEAD_CONTROLLER))
		m_afCapability = bits_CAP_TURN_HEAD;

	m_iTriggerCondition = AITRIGGER_NONE;// XDM3038: BUGBUG: UNDONE: these tend to cause unnecessary chaos
}
