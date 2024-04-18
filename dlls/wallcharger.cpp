#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "globals.h"
#include "sound.h"
#include "game.h"
#include "weapons.h"
#include "pm_materials.h"

// UNDONE #define SF_WCHARGER_ALLOW_MULTIPLE_USERS		0x0001

//-------------------------------------------------------------
// Wall mounted universal charging device (base class)
// XDM: this code is much better
// TODO: should be public CBreakable - XDM3035a
// TODO: should use global respawn mechanism
//
// m_iJuice - current amount of juice, define after calling CBaseWallCharger::Spawn()
// pev->max_health - maximum amount of juice, define after calling CBaseWallCharger::Spawn()
// pev->noise - accept/start sound, define BEFORE calling CBaseWallCharger::Precache()
// pev->noise1 - looping sound
// pev->noise2 - deny sound
// pev->health - real for TakeDamage(), independent
//
//-------------------------------------------------------------
class CBaseWallCharger : public CBaseToggle
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);// XDM3035a
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual bool CanCharge(CBaseEntity *pOther);
	virtual int DoCharge(CBaseEntity *pOther);
	virtual int	ObjectCaps(void) { return (CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	//virtual bool ShouldRespawn(void) const;// XDM3038
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	void ChargerDisable(void);
	void EXPORT TurnOff(void);
	void EXPORT Recharge(void);
	static TYPEDESCRIPTION m_SaveData[];
	int m_iJuice;
	float m_fReactivate;// DeathMatch Delay until reactvated
	float m_flNextCharge;
	float m_flSoundTime;
	short m_iOn;// 0 = off, 1 = startup sound, 2 = looping sound
};

TYPEDESCRIPTION CBaseWallCharger::m_SaveData[] =
{
	DEFINE_FIELD(CBaseWallCharger, m_iOn, FIELD_SHORT),
	DEFINE_FIELD(CBaseWallCharger, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD(CBaseWallCharger, m_fReactivate, FIELD_FLOAT),
	DEFINE_FIELD(CBaseWallCharger, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD(CBaseWallCharger, m_flSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CBaseWallCharger, CBaseToggle);

LINK_ENTITY_TO_CLASS(func_charger, CBaseWallCharger);

// use max_health to specify capacity
void CBaseWallCharger::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "capacity"))// XDM3038c: abstraction
	{
		pev->max_health = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_fReactivate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: detect and fix initial parameters here!
//-----------------------------------------------------------------------------
void CBaseWallCharger::Spawn(void)
{
	Precache();
	if (*STRING(pev->model) == '*')
	{
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSH;// required for SOLID_BSP
	}
	else
	{
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_NONE;
	}
	UTIL_SetOrigin(this, pev->origin);// set size and link into world
	UTIL_SetSize(this, pev->mins, pev->maxs);
	SET_MODEL(edict(), STRING(pev->model));
	if (pev->max_health == 0)
	{
		pev->max_health = 100;
		conprintf(1, "CBaseWallCharger (%s): max_health not set! Setting default %d.\n", STRING(pev->targetname), pev->max_health);
	}
	if (UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))
	{
		InitBoneControllers();
		ResetSequenceInfo();
		SetBoneController(0, 0);
	}
	pev->skin = 0;// XDM3038a: studio model support
	pev->sequence = 0;// XDM3038a: studio model support
	pev->frame = 0;// set to prevent "respawn" effects
	pev->health = pev->max_health;
	pev->takedamage = DAMAGE_YES;// XDM3038
	m_iJuice = pev->max_health;
}

//-----------------------------------------------------------------------------
// Purpose: Precache custom resources (defined by subclasses)
//-----------------------------------------------------------------------------
void CBaseWallCharger::Precache(void)
{
	PRECACHE_MODEL(STRINGV(pev->model));
	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRINGV(pev->noise));
	if (!FStringNull(pev->noise1))
		PRECACHE_SOUND(STRINGV(pev->noise1));
	if (!FStringNull(pev->noise2))
		PRECACHE_SOUND(STRINGV(pev->noise2));

	size_t i = 0;// XDM: don't precache all sounds!
	for (i = 0; i < NUM_SHARD_SOUNDS; ++i)
		PRECACHE_SOUND((char *)gSoundsMetal[i]);

	for (i = 0; i < NUM_BREAK_SOUNDS; ++i)
		PRECACHE_SOUND((char *)gBreakSoundsMetal[i]);
}

//-----------------------------------------------------------------------------
// Purpose: Currently allows use only by players
//-----------------------------------------------------------------------------
void CBaseWallCharger::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator == NULL)// make sure that we have an activator
		return;

	if (!pActivator->IsPlayer())// if it's not a player, ignore
		return;

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		if (m_iOn > 0)// XDM3038
			ChargerDisable();
		else// conprintf( error!!! )
		{
			pev->frame = 1;// change texture
			pev->skin = 1;// XDM3038a: studio model support
			pev->controller[0] = 255;// XDM3038a: studio model support
		}
	}

	if (pev->deadflag != DEAD_NO)// XDM3038: don't make any sounds if destroyed
		return;

	// TESTME: prevent players from using chargers through walls? This will probably break some maps or gameplay.
	if (pCaller == NULL || pCaller == pActivator)// allow triggering by... something remote
	{
		TraceResult tr;
		UTIL_TraceLine(Center(), pActivator->EyePosition(), ignore_monsters, dont_ignore_glass, pActivator->edict(), &tr);
		if (tr.flFraction != 1.0f && tr.pHit != edict())
			return;
	}

	// if the player doesn't have the suit, or there is no juice left, or the suit is full make the deny noise
	if (!CanCharge(pActivator))
	{
		if (!FStringNull(pev->noise2))
		{
			if (m_flSoundTime <= gpGlobals->time)// nope || m_hActivator != pActivator)
			{
				EMIT_SOUND(edict(), CHAN_ITEM, STRING(pev->noise2), VOL_NORM, ATTN_NORM);
				m_flSoundTime = gpGlobals->time + 1.0f;
			}
		}
		return;
	}

	SetThink(&CBaseWallCharger::TurnOff);
	pev->nextthink = pev->ltime + 0.25f;

	// Charge portion delay
	if (m_flNextCharge >= gpGlobals->time)
		return;

	m_hActivator = pActivator;
	// Play the "on" sound
	if (m_iOn == 0)
	{
		m_iOn++;// next stage: looping sound
		if (!FStringNull(pev->noise))
			EMIT_SOUND(edict(), CHAN_ITEM, STRING(pev->noise), VOL_NORM, ATTN_NORM);

		pev->sequence = 1;// XDM3038a: studio model support
		m_flSoundTime = gpGlobals->time + 0.56f;
		SUB_UseTargets(pActivator, USE_TOGGLE, 1);// XDM3035c: fire some targets!
	}
	else if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))// Start the looping "charging" sound
	{
		m_iOn++;// next stage
		if (!FStringNull(pev->noise1))
			EMIT_SOUND(edict(), CHAN_ITEM, STRING(pev->noise1), VOL_NORM, ATTN_NORM);
	}

	// charge the player
	m_iJuice -= DoCharge(pActivator);
	pev->controller[0] = (byte)(255.0f*((float)m_iJuice)/pev->max_health);// XDM3038a: studio model support

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: One-shot function. Changes charger texture to OFF.
//-----------------------------------------------------------------------------
void CBaseWallCharger::ChargerDisable(void)
{
	TurnOff();// stop charging and initiate respawn
	pev->frame = 1;// change texture
	pev->skin = 1;// XDM3038a: studio model support
	pev->controller[0] = 255;
}

//-----------------------------------------------------------------------------
// Purpose: Instantly restore capacity
//-----------------------------------------------------------------------------
void CBaseWallCharger::Recharge(void)
{
	pev->skin = 0;// XDM3038a: studio model support
	pev->sequence = 0;
	pev->frame = 0;// restore default texture
	pev->controller[0] = 0;
	pev->health = pev->max_health;
	pev->takedamage = DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	m_iJuice = pev->max_health;
	SetThink(&CBaseEntity::SUB_DoNothing);
	EMIT_SOUND(edict(), CHAN_ITEM, DEFAULT_RESPAWN_SOUND_ITEM, VOL_NORM, ATTN_NORM);
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
	Vector vCenter(Center());
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vCenter);// stub for respawn effect // TODO: use some PS/RS
		WRITE_BYTE(TE_IMPLOSION);
		WRITE_COORD(vCenter.x);
		WRITE_COORD(vCenter.y);
		WRITE_COORD(vCenter.z);
		WRITE_BYTE(56);// radius
		WRITE_BYTE(64);// count
		WRITE_BYTE(10);// life*10
	MESSAGE_END();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stops charging process and initiates "respawn"
//-----------------------------------------------------------------------------
void CBaseWallCharger::TurnOff(void)
{
	if (m_iOn > 0)// So constant usage will not prevent respawning
	{
		if (m_iOn > 1)
		{
			pev->sequence = 0;// XDM3038a: studio model support
			if (!FStringNull(pev->noise1))
				STOP_SOUND(edict(), CHAN_ITEM, STRING(pev->noise1));// Stop looping sound.

			SUB_UseTargets(m_hActivator, USE_OFF, 0);// XDM3035c: fire some targets!
		}
		m_iOn = 0;
	}
	// Is it time to start regenerating? Even slightly used chargers will respawn after use.
	if ((m_iJuice < pev->max_health) && (m_fReactivate > 0.0f) && IsMultiplayer())
	{
		pev->nextthink = pev->ltime + m_fReactivate;
		SetThink(&CBaseWallCharger::Recharge);
	}
	else// permanently disable
	{
		DontThink();// XDM3038a
		SetThinkNull();
		//SetThink(&CBaseEntity::SUB_DoNothing);
	}
}

//-----------------------------------------------------------------------------
// Purpose: special hit effects
//-----------------------------------------------------------------------------
void CBaseWallCharger::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (FBitSet(bitsDamageType, DMGM_BREAK))
	{
		if (m_iJuice > 0)
		{
			//UTIL_Sparks(ptr->vecEndPos);
			if ((g_pGameRules == NULL || g_pGameRules->FAllowEffects()) && RANDOM_LONG(0,1))
			{
				StreakSplash(ptr->vecEndPos, ptr->vecPlaneNormal, RANDOM_INT2(0,5), RANDOM_LONG(12,16), flDamage, 400);// "r_efx.h"
				//EMIT_SOUND_DYN(edict(), CHAN_VOICE, gSoundsSparks[RANDOM_LONG(0,NUM_SPARK_SOUNDS-1)], RANDOM_FLOAT(0.7, 1.0), ATTN_NORM, 0, RANDOM_LONG(95,105));
			}
		}
	}
	CBaseToggle::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//-----------------------------------------------------------------------------
// Purpose: track damage and decrease capacity
//-----------------------------------------------------------------------------
int CBaseWallCharger::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->health > 0 && FBitSet(bitsDamageType, DMGM_BREAK))// don't get Killed() multiple times
	{
		m_iJuice -= (int)(flDamage*0.5f);// decrease remaining charge
		int iRet = CBaseToggle::TakeDamage(pInflictor, pAttacker, flDamage*0.5f, bitsDamageType);
		if (pev->health > 0)
		{
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, gSoundsMetal[RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)], VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95,105));

			if (m_iJuice <= 0)// charge depleted earlier than the charger was destroyed
				ChargerDisable();
		}
		return iRet;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: destruction effects
//-----------------------------------------------------------------------------
void CBaseWallCharger::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	m_iJuice = 0;
	if (pev->deadflag != DEAD_NO)
		return;

	pev->takedamage = DAMAGE_NO;
	ChargerDisable();
	Vector vCenter(Center());
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, gBreakSoundsMetal[RANDOM_LONG(0,NUM_BREAK_SOUNDS-1)], VOL_NORM, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(0,10));
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vCenter);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(vCenter.x + RANDOM_FLOAT(-2, 2));
		WRITE_COORD(vCenter.y + RANDOM_FLOAT(-2, 2));
		WRITE_COORD(vCenter.z + RANDOM_FLOAT(-2, 2));
		WRITE_SHORT(g_iModelIndexSmoke);
		WRITE_BYTE(15);// scale * 10
		WRITE_BYTE(10);// framerate
	MESSAGE_END();
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		PLAYBACK_EVENT_FULL(0, edict(), g_usSparkShower, 0, vCenter, pev->angles, 160.0f, 1.0f, /*g_iModelIndexZeroFlare*/0, RANDOM_LONG(3,4), 1, 0);

	if (IsMultiplayer())
		pev->deadflag = DEAD_RESPAWNABLE;
	else
		pev->deadflag = DEAD_DEAD;

	// don't let it be removed!
}

//-----------------------------------------------------------------------------
// Purpose: OVERRIDE: check if pOther can be charged
// Input  : *pOther - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseWallCharger::CanCharge(CBaseEntity *pOther)
{
	if (m_iJuice > 0)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: OVERRIDE: Do charging 
// Input  : *pOther - 
// Output : int amount that was actually accepted by pOther
//-----------------------------------------------------------------------------
int CBaseWallCharger::DoCharge(CBaseEntity *pOther)
{
	if (pOther)
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: This is completely different mechanism!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool CBaseWallCharger::ShouldRespawn(void)
{
	return false;
}*/


//-----------------------------------------------------------------------------
// Purpose: wall-mounted health charger
//-----------------------------------------------------------------------------
class CWallHealthCharger : public CBaseWallCharger
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool CanCharge(CBaseEntity *pOther);
	virtual int DoCharge(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_healthcharger, CWallHealthCharger);

void CWallHealthCharger::Spawn(void)
{
	pev->max_health = gSkillData.healthchargerCapacity;
	CBaseWallCharger::Spawn();
	//m_iJuice = pev->max_health;
	if (m_fReactivate <= 0.0f)// allow mappers to override settings
		m_fReactivate = g_pGameRules?g_pGameRules->GetChargerRechargeDelay():60.0f;
}

void CWallHealthCharger::Precache(void)
{
	if (FStringNull(pev->noise))// XDM3038c: custom sounds
		pev->noise = ALLOC_STRING("items/medshot4.wav");

	if (FStringNull(pev->noise1))
		pev->noise1 = ALLOC_STRING("items/medcharge4.wav");

	if (FStringNull(pev->noise2))
		pev->noise2 = ALLOC_STRING("items/medshotno1.wav");

	CBaseWallCharger::Precache();
}

bool CWallHealthCharger::CanCharge(CBaseEntity *pOther)
{
	if (CBaseWallCharger::CanCharge(pOther))
	{
		if (pOther->pev->health < pOther->pev->max_health)
		{
			if (pOther->IsPlayer() && ((CBasePlayer *)pOther)->HasSuit())
				return true;
		}
	}
	return false;
}

int CWallHealthCharger::DoCharge(CBaseEntity *pOther)
{
	return (pOther->TakeHealth(1, DMG_GENERIC) > 0);
}

//-----------------------------------------------------------------------------
// Purpose: wall-mounted health charger
//-----------------------------------------------------------------------------
class CWallSuitCharger : public CBaseWallCharger
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool CanCharge(CBaseEntity *pOther);
	virtual int DoCharge(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_recharge, CWallSuitCharger);

void CWallSuitCharger::Spawn(void)
{
	pev->max_health = gSkillData.suitchargerCapacity;
	CBaseWallCharger::Spawn();
	//m_iJuice = pev->max_health;
	if (m_fReactivate <= 0.0f)// allow mappers to override settings
		m_fReactivate = g_pGameRules?g_pGameRules->GetChargerRechargeDelay():60.0f;
}

void CWallSuitCharger::Precache(void)
{
	if (FStringNull(pev->noise))// XDM3038c: custom sounds
		pev->noise = ALLOC_STRING("items/suitchargeok1.wav");

	if (FStringNull(pev->noise1))
		pev->noise1 = ALLOC_STRING("items/suitcharge1.wav");

	if (FStringNull(pev->noise2))
		pev->noise2 = ALLOC_STRING("items/suitchargeno1.wav");

	CBaseWallCharger::Precache();
}

bool CWallSuitCharger::CanCharge(CBaseEntity *pOther)
{
	if (CBaseWallCharger::CanCharge(pOther))
	{
		if (pOther->pev->armorvalue < (g_pGameRules?g_pGameRules->GetPlayerMaxArmor():MAX_NORMAL_BATTERY))
		{
			if (pOther->IsPlayer() && ((CBasePlayer *)pOther)->HasSuit())
				return true;
		}
	}
	return false;
}

int CWallSuitCharger::DoCharge(CBaseEntity *pOther)
{
	int iMaxArmor = (g_pGameRules?g_pGameRules->GetPlayerMaxArmor():MAX_NORMAL_BATTERY);// XDM3038
	if (pOther->pev->armorvalue < iMaxArmor)
	{
		pOther->pev->armorvalue += 1;

		if (pOther->pev->armorvalue > iMaxArmor)
			pOther->pev->armorvalue = iMaxArmor;

		return 1;
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: wall-mounted ammo charger // XDM3037
// KV:
// ammotype - "uranium", etc.
// max_health - contained ammo amount
//-----------------------------------------------------------------------------
class CWallAmmoCharger : public CBaseWallCharger
{
public:
	//virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool CanCharge(CBaseEntity *pOther);
	virtual int DoCharge(CBaseEntity *pOther);
	int m_iAmmoIndex;
};

LINK_ENTITY_TO_CLASS(func_ammocharge, CWallAmmoCharger);

/*void CWallAmmoCharger::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "ammotype"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseWallCharger::KeyValue(pkvd);
}*/

void CWallAmmoCharger::Spawn(void)
{
	CBaseWallCharger::Spawn();
	//pev->max_health = ;
	//m_iJuice = pev->max_health;
	if (m_fReactivate <= 0.0f)// allow mappers to override settings
		m_fReactivate = g_pGameRules?g_pGameRules->GetChargerRechargeDelay():mp_itemrespawntime.value;

	if (pev->impulse <= 0)
		pev->impulse = 1;// ammo per frame

	m_iAmmoIndex = GetAmmoIndexFromRegistry(STRING(pev->message));
	if (m_iAmmoIndex < 0)
	{
		conprintf(1, "CWallAmmoCharger: bad ammo type \"%s\"! Removing.\n", STRING(pev->message));
		pev->flags = FL_KILLME;//Destroy();
	}
}

void CWallAmmoCharger::Precache(void)
{
	if (FStringNull(pev->noise))// XDM3038c: custom sounds
		pev->noise = ALLOC_STRING("hassault/hw_spinup.wav");

	if (FStringNull(pev->noise1))
		pev->noise1 = ALLOC_STRING("hassault/hw_spin.wav");

	if (FStringNull(pev->noise2))
		pev->noise2 = ALLOC_STRING("hassault/hw_spindown.wav");

	CBaseWallCharger::Precache();
}

bool CWallAmmoCharger::CanCharge(CBaseEntity *pOther)
{
	if (CBaseWallCharger::CanCharge(pOther))
	{
		if (pOther->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pOther;
			if (pPlayer->AmmoInventory(m_iAmmoIndex) < g_AmmoInfoArray[m_iAmmoIndex].iMaxCarry)
				return true;
		}
	}
	return false;
}

int CWallAmmoCharger::DoCharge(CBaseEntity *pOther)
{
	if (pOther->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		if (pPlayer->AmmoInventory(m_iAmmoIndex) < g_AmmoInfoArray[m_iAmmoIndex].iMaxCarry)
#if defined(OLD_WEAPON_AMMO_INFO)
			return pOther->GiveAmmo(pev->impulse, m_iAmmoIndex, g_AmmoInfoArray[m_iAmmoIndex].iMaxCarry);
#else
			return pOther->GiveAmmo(pev->impulse, m_iAmmoIndex);
#endif
	}
	return 0;
}
