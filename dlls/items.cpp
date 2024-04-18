#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"
#include "skill.h"
#include "items.h"
#include "pm_shared.h"
#include "sound.h"
#include "soundent.h"

// CItem is not really added to host's inventory, just does something upon touching

LINK_ENTITY_TO_CLASS(item, CItem);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "capacity"))// XDM3038c: abstraction
	{
		pev->max_health = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: Precache resources used by this entity
//-----------------------------------------------------------------------------
void CItem::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038
	{
		conprintf(0, "Design error: %s[%d] %s at (%g %g %g) without model!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->model = MAKE_STRING(g_szDefaultStudioModel);
		//pev->modelindex = g_iModelIndexTestSphere;
	}

	CBaseAnimating::Precache();

	if (FStringNull(pev->noise))// XDM3038a
		pev->noise = MAKE_STRING(DEFAULT_PICKUP_SOUND_ITEM);

	PRECACHE_SOUND(STRINGV(pev->noise));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem::Spawn(void)
{
	if (FBitSet(pev->spawnflags, SF_ITEM_QUEST))
		SetBits(pev->spawnflags, SF_NORESPAWN);

	if (GetExistenceState() != ENTITY_EXSTATE_CARRIED)
	{
		if (FBitSet(pev->spawnflags, SF_ITEM_NOFALL))// XDM3038c
			pev->movetype = MOVETYPE_NONE;
		else
			pev->movetype = MOVETYPE_TOSS;

		if (pev->scale <= 0.0f)
			pev->scale = UTIL_GetWeaponWorldScale();// XDM3035b
	}

	CBaseAnimating::Spawn();// CBaseDelay::Spawn(); -> CMyItem::Precache() -> CItem::Precache() -> CBaseAnimating::Precache()
	//SET_MODEL(edict(), STRING(pev->model));// XDM3038: this modifies pev->mins/maxs!

	pev->health = pev->max_health;// XDM3038c: capacity

	if (IsMultiplayer())
		SetBits(pev->flags, FL_HIGHLIGHT);// XDM3037

	if (GetExistenceState() != ENTITY_EXSTATE_CARRIED)
	{
		// keep pev->sequence set in the editor
		if (pev->movetype == MOVETYPE_TOSS && !FBitSet(pev->effects, EF_NODRAW))// falling
		{
			pev->solid = SOLID_BBOX;
			ClearBits(pev->flags, FL_ONGROUND);
			UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
			if (!FBitSet(pev->spawnflags, SF_NOREFERENCE))
			{
				if (DROP_TO_FLOOR(edict()) == 0)
				{
					conprintf(0, "Design error: %s[%d] fell out of level at %g %g %g!\n", STRING(pev->classname), entindex(), pev->origin.x, pev->origin.y, pev->origin.z);
					Destroy();
					return;
				}
			}
			SetThink(&CItem::FallThink);// XDM3035c: reuse more code
			SetNextThink(0.25);
			//SetTouch(&CItem::ItemTouch);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called during [re]spawning
//-----------------------------------------------------------------------------
void CItem::Materialize(void)
{
	if (GetExistenceState() == ENTITY_EXSTATE_CARRIED)
		return;

	//if (pev->movetype == MOVETYPE_NONE)// not falling
	//{
		pev->solid = SOLID_TRIGGER;
		pev->owner = NULL;// XDM3038c: TESTME! enable collisions
		UTIL_SetSize(this, VEC_HULL_ITEM_MIN, VEC_HULL_ITEM_MAX);// XDM3038c
		SetTouch(&CItem::ItemTouch);
		//pev->groupinfo = 0;// XDM3038c: TESTME! enable collisions
		//UTIL_UnsetGroupTrace();// XDM3038c: TESTME!
		/*SetThinkNull();
		DontThink();
	}
	else if (pev->movetype == MOVETYPE_TOSS)
	{
		ClearBits(pev->flags, FL_ONGROUND);
		pev->solid = SOLID_BBOX;
		UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
	}*/
	CBaseAnimating::Materialize();// 	UTIL_SetOrigin(this, pev->origin);
}

//-----------------------------------------------------------------------------
// Purpose: THINK Items check landing and materialize here
// Note   : Item should not stop thinking in case it gets pushed or falls from breakable, etc.
//-----------------------------------------------------------------------------
void CItem::FallThink(void)
{
#if defined (_DEBUG)
	if (GetExistenceState() == ENTITY_EXSTATE_CARRIED)
	{
		conprintf(1, "%s[%d]::FallThink() called while IsCarried()!\n", STRING(pev->classname), entindex());
		SetThinkNull();
		return;
	}
#endif

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (pev->solid != SOLID_TRIGGER)// m_pfnTouch != &CItem::ItemTouch // if (!pev->vuser3.IsZero())// is not materialized yet
		{
			if (/*pev->impulse == ITEM_STATE_DROPPED && */FBitSet(pev->spawnflags, SF_NORESPAWN) && !FBitSet(pev->spawnflags, SF_ITEM_QUEST))
			{
				int pc = POINT_CONTENTS(pev->origin);// Remove items that fall into unreachable locations
				if (pc == CONTENTS_SLIME || pc == CONTENTS_LAVA || pc == CONTENTS_SKY)
				{
					Destroy();
					return;
				}
				else
				{
					EMIT_SOUND_DYN(edict(), CHAN_VOICE, DEFAULT_DROP_SOUND_ITEM, VOL_NORM, ATTN_STATIC, 0, RANDOM_LONG(110,114));// since the sound is the same, use different pitch
					//SetModelCollisionBox();// XDM3035b: TODO
					AlignToFloor();// XDM3036
				}
			}
			Materialize();
			pev->vuser3.Clear();
		}
		SetNextThink(0.25);
	}
	else// not on ground
	{
		if (pev->vuser3.IsZero())
		{
			pev->solid = SOLID_BBOX;
			UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
		}
		pev->vuser3 = pev->velocity;// like m_flFallVelocity
		SetNextThink(0.1);// check faster
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overload: XDM3038c
// Warning: CItem does not create duplicates or gets destroyed, just hides!
// Output : CBaseEntity * - entity that will appear later
//-----------------------------------------------------------------------------
CBaseEntity *CItem::StartRespawn(void)
{
	m_vecSpawnSpot = g_pGameRules->GetItemRespawnSpot(this);
	if (pev->health <= 0.0f)// don't hide partially picked up item
	{
		SetBits(pev->effects, EF_NODRAW);// BAD! m_iExistenceState = ENTITY_EXSTATE_VOID;
		SetTouchNull();
		UTIL_SetSize(this, g_vecZero, g_vecZero);
	}
	ScheduleRespawn(g_pGameRules->GetItemRespawnDelay(this));
	return this;
}

//-----------------------------------------------------------------------------
// Purpose: Overload: XDM3038c: Called after respawning successfully
//-----------------------------------------------------------------------------
void CItem::OnRespawn(void)
{
	SetBits(pev->effects, EF_MUZZLEFLASH);
	ClearBits(pev->flags, FL_NOTARGET);// clear "don't pickup" flags for AI
	if (IsMultiplayer())
		SetBits(pev->flags, FL_HIGHLIGHT);

#if !defined(CLIENT_DLL)
	MESSAGE_BEGIN(MSG_PVS, gmsgItemSpawn, pev->origin+Vector(0,0,4));
		WRITE_BYTE(EV_ITEMSPAWN_ITEM);
		WRITE_SHORT(entindex());
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_ANGLE(pev->angles.x);
		WRITE_ANGLE(pev->angles.y);
		WRITE_ANGLE(pev->angles.z);
		WRITE_SHORT(pev->modelindex);
		WRITE_BYTE((int)(pev->scale*10.0f));
		WRITE_BYTE(pev->body);
		WRITE_BYTE(pev->skin);
		WRITE_BYTE(pev->sequence);
	MESSAGE_END();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Overload
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CItem::ShouldRespawn(void) const
{
	if (FBitSet(pev->spawnflags, /*SF_NOREFERENCE|*/SF_NORESPAWN))
		return false;
	if (g_pGameRules)
	{
		if (!g_pGameRules->FItemShouldRespawn(this))
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Warning: These items are PURELY VIRTUAL (not stored in player's inventory) and are DESTROYED after touching!
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CItem::ItemTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;
	if (!pOther->IsAlive())// XDM3038: why check inside overloads?
		return;
	if (g_pGameRules && g_pGameRules->IsGameOver())// XDM3037
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (g_pGameRules && !g_pGameRules->CanHaveItem(pPlayer, this))
		return;

	if (FBitSet(pev->spawnflags, SF_ITEM_REQUIRESUIT))
	{
		if (!pPlayer->HasSuit())
			return;
	}

	int iAdded = MyTouch(pPlayer);
	if (iAdded > 0)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->edict());// XDM3038: centralized
			WRITE_SHORT(iAdded);
			WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		if (!g_SilentItemPickup)// XDM3038c
		{
			if (!FBitSet(pev->spawnflags, SF_NOREFERENCE))// XDM3038c
				CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, ITEM_PICKUP_VOLUME, 1.0f);// XDM3038a

			if (!FStringNull(pev->noise))// XDM3038a
				EMIT_SOUND(edict(), CHAN_ITEM, STRING(pev->noise), VOL_NORM, ATTN_NORM);
		}
		pev->health -= min(pev->health, (float)iAdded);// don't bo below 0

		if (sv_partialpickup.value > 0.0f && pev->health > 0.0f)
		{
			if (ShouldRespawn())
				StartRespawn();// don't disappear

			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#CONTAINER_REMAIN", UTIL_dtos1((int)ceilf(pev->health)));
		}
		else// remove or respawn normally
		{
			SetTouchNull();
			SetThink(&CBaseEntity::SUB_Remove);// XDM3038: unsafe! Destroy();
			SetNextThink(0.001);// XDM3038a
		}
	}
	else if (g_SilentItemPickup)
	{
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.001);// XDM3038a
		// XDM3038: unsafe! Destroy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overload: XDM3038c
// Output : int - initial, maximum amount!
//-----------------------------------------------------------------------------
//int CItem::GetCapacity(void)
//{
//	return (int)pev->max_health;// WARNING: unsafe type conversion!
//}

//-----------------------------------------------------------------------------
// Purpose: Overload: custom touch function
// Input  : *pPlayer - 
// Output : int - amount actually added
//-----------------------------------------------------------------------------
int CItem::MyTouch(CBasePlayer *pPlayer)
{
	return 1;// XDM3038c: we now use this class as a generic pickup
}



//=========================================================
// item_suit
//=========================================================
#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
	virtual void Spawn(void)
	{
#if (SF_SUIT_SHORTLOGON == SF_ITEM_REQUIRESUIT)
		if (FBitSet(pev->spawnflags, SF_SUIT_SHORTLOGON))
		{
			ClearBits(pev->spawnflags, SF_ITEM_REQUIRESUIT);
			pev->button = 1;
		}
		else
#else
#error Bad flag definition: SF_SUIT_SHORTLOGON conflicts with SF_ITEM_REQUIRESUIT!
#endif
			pev->button = 0;

		if (FStringNull(pev->model))
			pev->model = MAKE_STRING("models/w_suit.mdl");

		if (pev->max_health <= 0)// XDM3038c: allow multiple suits? For multiplayer?
			pev->max_health = 1;

		CItem::Spawn();
		if (pev->colormap == 0)// XDM3038c: allow customization
			pev->colormap = 0x00001E1E;// XDM3035: Half-Life default orange
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->HasSuit())
			return 0;

		if (!g_SilentItemPickup)// This should be called from something like OnPicked()...
		{
			if (pev->button > 0)//if (FBitSet(pev->spawnflags, SF_SUIT_SHORTLOGON))
				EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A0");// short version of suit logon
			else
				EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_AAx");// long version of suit logon
		}
		SetBits(pPlayer->pev->weapons, 1<<WEAPON_SUIT);
		pPlayer->m_fFlashBattery = MAX_FLASHLIGHT_CHARGE;
		return 1;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);


//=========================================================
// item_healthkit
//=========================================================
class CItemHealthKit : public CItem
{
	virtual void Precache(void)
	{
		if (pev->max_health <= 0)
			pev->max_health = gSkillData.healthkitCapacity;

		if (FStringNull(pev->model))// XDM3038
			pev->model = MAKE_STRING("models/w_medkit.mdl");

		if (FStringNull(pev->noise))// XDM3038a
			pev->noise = MAKE_STRING("items/smallmedkit1.wav");

		CItem::Precache();
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		return ceilf(pPlayer->TakeHealth(pev->health, DMG_GENERIC));
	}
};

LINK_ENTITY_TO_CLASS(item_healthkit, CItemHealthKit);


//=========================================================
// item_battery
//=========================================================
class CItemBattery : public CItem
{
	virtual void Precache(void)
	{
		SetBits(pev->spawnflags, SF_ITEM_REQUIRESUIT);// XDM3038
		if (pev->max_health == 0)
		{
			if (pev->armorvalue > 0)// COMPATIBILITY
				pev->max_health = pev->armorvalue;
			else
				pev->max_health = gSkillData.batteryCapacity;
		}
		if (FStringNull(pev->model))// XDM3038
			pev->model = MAKE_STRING("models/w_battery.mdl");

		//if (FStringNull(pev->noise))// XDM3038a
		//	pev->noise = MAKE_STRING(DEFAULT_PICKUP_SOUND_ITEM);

		CItem::Precache();
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		int iMaxArmor = (g_pGameRules?g_pGameRules->GetPlayerMaxArmor():MAX_NORMAL_BATTERY);// XDM3038
		if (pPlayer->pev->armorvalue < iMaxArmor)// && pPlayer->HasSuit())
		{
			float added;// points actually added
			pPlayer->pev->armorvalue += pev->health;

			if (pPlayer->pev->armorvalue > iMaxArmor)
			{
				added = pev->health - (pPlayer->pev->armorvalue - iMaxArmor);
				pPlayer->pev->armorvalue = iMaxArmor;
			}
			else
				added = pev->health;

			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			int pct = (int)((float)(pPlayer->pev->armorvalue * 100.0f) * (1.0/iMaxArmor) + 0.5f);
			pct /= 5;
			if (pct > 0)
				--pct;

			char szcharge[CBSENTENCENAME_MAX];
			_snprintf(szcharge, CBSENTENCENAME_MAX, "!HEV_%1dP", pct);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);//EMIT_SOUND_SUIT(edict(), szcharge);
			return added;
		}
		return 0;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);


//=========================================================
// item_antidote
//=========================================================
class CItemAntidote : public CItem
{
	virtual void Precache(void)
	{
		if (FStringNull(pev->model))// XDM3038
			pev->model = MAKE_STRING("models/w_antidote.mdl");

		if (pev->max_health <= 0)// XDM3038c
			pev->max_health = DEFAULT_ANTIDOTE_GIVE;

		CItem::Precache();
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->m_rgItems[ITEM_ANTIDOTE] == 0)
		{
			pPlayer->m_rgItems[ITEM_ANTIDOTE]++;
			if (!g_SilentItemPickup)
				pPlayer->SetSuitUpdate("!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN);

			return 1;
		}
		return 0;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


//=========================================================
// item_security
//=========================================================
class CItemSecurity : public CItem
{
	virtual void Precache(void)
	{
		SetBits(pev->spawnflags, SF_NORESPAWN);// XDM3038c: this is a unique item. We don't set SF_ITEM_QUEST because in the future that flag may have more meaning.
		if (FStringNull(pev->model))// XDM3038
			pev->model = MAKE_STRING("models/w_security.mdl");
		if (FStringNull(pev->noise))// XDM3038a
			pev->noise = MAKE_STRING("buttons/blip2.wav");

		if (pev->max_health <= 0)
			pev->max_health = 1;// XDM3038c: there cannot be many??

		CItem::Precache();
		PRECACHE_SOUND("buttons/blip2.wav");
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		pPlayer->m_rgItems[ITEM_SECURITY]++;
		return 1;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);


//=========================================================
// item_longjump
//=========================================================
class CItemLongJump : public CItem
{
	virtual void Precache(void)
	{
		SetBits(pev->spawnflags, SF_ITEM_REQUIRESUIT);// XDM3038
		if (FStringNull(pev->model))// XDM3038
			pev->model = MAKE_STRING("models/w_longjump.mdl");

		if (pev->max_health <= 0)// XDM3038c
			pev->max_health = 1;

		CItem::Precache();
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->m_fLongJump == FALSE)
		{
			ENGINE_SETPHYSKV(pPlayer->edict(), PHYSKEY_LONGJUMP, "1");
			pPlayer->m_fLongJump = TRUE;// player now has longjump module
			//pPlayer->m_rgItems[ITEM_LONGJUMP]++;
			return 1;
		}
		return 0;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);


//=========================================================
// Flare pickup
//=========================================================
class CItemFlare : public CItem
{
public:
	virtual void Precache(void)
	{
		if (pev->max_health <= 0)// XDM3038c
			pev->max_health = DEFAULT_FLARE_GIVE;
		//if (pev->impulse <= 0)// XDM3038a: fixed endless loop
		//	pev->impulse = 1;// must return non-zero!
		if (FStringNull(pev->model))// XDM3037
			pev->model = MAKE_STRING("models/w_flare.mdl");
		if (FStringNull(pev->noise))// XDM3038a
			pev->noise = MAKE_STRING("items/9mmclip1.wav");

		CItem::Precache();
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		pPlayer->m_rgItems[ITEM_FLARE] += (int)pev->health;
		return (int)pev->health;// number of flares
	}
};

LINK_ENTITY_TO_CLASS(item_flare, CItemFlare);


//=========================================================
// The light emitter itself
//=========================================================
class CFlare : public CBaseAnimating
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsPushable(void) { return true; }// XDM3038c
};

LINK_ENTITY_TO_CLASS(flare, CFlare);

void CFlare::Spawn(void)
{
	//Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_SLIDEBOX;// XDM3038c: to get pushed
	CBaseAnimating::Spawn();// XDM3038c
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	//FallInit();
	pev->effects = EF_BRIGHTLIGHT;
	pev->sequence = 1;
	pev->frame = 0;
	pev->framerate = 1.0;
	pev->renderamt = 255;
	pev->rendercolor.Set(255,255,255);
	EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/flare.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(PITCH_NORM,PITCH_NORM+5));
	//GetAttachment(0, pev->origin, pev->angles);
	//conprintf(1, ">>>>>>>> spr: %d MODEL_INDEX: %d\n", spr, MODEL_INDEX("sprites/flare_glow.spr"));
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
		SetBits(pev->effects, EF_LIGHT);// XDM3035
		SetThink(&CFlare::SUB_FadeOut);// don't SUB_StartFadeOut!
	}
	else
		SetThink(&CFlare::SUB_Remove);

	SetNextThink(FLARE_LIGHT_TIME);// XDM3038a
}

void CFlare::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/w_flare.mdl");

	CBaseAnimating::Precache();
	PRECACHE_SOUND("weapons/flare.wav");
}


//=========================================================
// Soda can pickup
//=========================================================
class CItemSoda : public CItem
{
	virtual void Spawn(void)
	{
		if (pev->max_health <= 0)
			pev->max_health = gSkillData.foodHeal;

		CItem::Spawn();
		//pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_TOSS;
		pev->impulse = GetEntTextureGroupsCount(edict());// CEnvBeverage depends on it
	}
	virtual void Precache(void)
	{
		if (FStringNull(pev->model))// XDM3038
			pev->model = MAKE_STRING("models/can.mdl");

		if (FStringNull(pev->noise))// XDM3038a
			pev->noise = MAKE_STRING("items/can_drink.wav");

		CItem::Precache();
		//PRECACHE_SOUND("weapons/g_bounce3.wav");
	}
	virtual int MyTouch(CBasePlayer *pPlayer)
	{
		int iRet = (int)ceilf(pPlayer->TakeHealth(pev->health, DMG_GENERIC));
		// tell the machine the can was taken
		if (iRet > 0 && FBitSet(pev->spawnflags, SF_NORESPAWN))
		{
			if (m_hOwner.Get())// XDM3037: owner mechanism has changed // if ( !FNullEnt( pev->owner ) )
				m_hOwner->DeathNotice(this);//m_hOwner->pev->frags = 0;//pev->owner->v.frags = 0;
		}
		return iRet;
	}
};

LINK_ENTITY_TO_CLASS(item_sodacan, CItemSoda);

/*old
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	SetThink(&CItemSoda::SodaCanThink);
	SetNextThink(0.5);// XDM3038a

void CItemSoda::SodaCanThink(void)
{
	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/g_bounce3.wav", VOL_NORM, ATTN_NORM);
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(this, Vector(-8, -8, 0), Vector(8, 8, 8));
	SetThinkNull();
	SetTouch(&CItemSoda::SodaCanTouch);
}*/


//=========================================================
// Airtank pickup (explodable), HACK
// TODO: rewrite this as a proper item
//=========================================================
class CAirtank : public CGrenade
{
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool ShouldRespawn(void) const;// XDM3035
	virtual bool IsPickup(void) const { return true; }// XDM3037
	virtual bool IsProjectile(void) const { return false; }// XDM3038a
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);// XDM3038c
	void EXPORT TankThink(void);
	void EXPORT TankTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(item_airtank, CAirtank);

bool CAirtank::ShouldRespawn(void) const
{
	if (IsMultiplayer())
	{
		if (FBitSet(pev->spawnflags, SF_NORESPAWN))
			return false;

		//g_pGameRules->GetItemRespawnTime() > 0
		return true;
	}
	return false;
}

void CAirtank::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "capacity"))
	{
		pev->max_health = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CGrenade::KeyValue(pkvd);
}

void CAirtank::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	Precache();
	// motor
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model));// XDM3038a

	UTIL_SetOrigin(this, pev->origin);

	SetTouch(&CAirtank::TankTouch);
	SetThink(&CAirtank::TankThink);

	pev->takedamage = DAMAGE_YES;

	if (pev->health <= 0)// actual health because airtank is destructible
		pev->health = 20;

	if (pev->max_health <= 0)// XDM3038c
	{
		if (pev->dmg <= 0)// COMPATIBILITY
			pev->max_health = DEFAULT_AIRTANK_GIVE;
		else
			pev->max_health = pev->dmg;
	}

	ClearBits(pev->flags, FL_ONGROUND);
	UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground

	if (DROP_TO_FLOOR(edict()) == 0)
	{
		conprintf(0, "Design error: %s[%d] fell out of level at %g %g %g!\n", STRING(pev->classname), entindex(), pev->origin.x, pev->origin.y, pev->origin.z);
		Destroy();
		return;
	}

	UTIL_SetSize(this, VEC_HULL_ITEM_MIN, VEC_HULL_ITEM_TALL_MAX);// XDM3038c
	UTIL_SetOrigin(this, pev->origin);
	pev->impulse = 1;
	pev->skin = 0;
}

void CAirtank::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/w_oxygen.mdl");

	CGrenade::Precache();
	if (FStringNull(pev->noise))// XDM3038a
		pev->noise = MAKE_STRING("items/airtank1.wav");

	PRECACHE_SOUND(STRINGV(pev->noise));
}

int CAirtank::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->takedamage != DAMAGE_NO)
	{
		if (FBitSet(bitsDamageType, DMGM_BREAK|DMGM_FIRE))
		{
			pev->health -= flDamage;
			if (pev->health <= 0.0)
			{
				pev->dmg = pev->max_health;
				Killed(pInflictor, pAttacker, GIB_NORMAL);
			}
			return 1;
		}
	}
	return 0;
}

void CAirtank::TankThink(void)
{
	pev->impulse = 1;
	pev->skin = 0;
	SUB_UseTargets(this, USE_TOGGLE, 0);
}

void CAirtank::TankTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	if (pev->impulse <= 0)
		return;

	if (pev->teleport_time <= gpGlobals->time)// XDM3035: retouch time
	{
		pOther->pev->air_finished = gpGlobals->time + pev->max_health;// XDM3034
		if (!FStringNull(pev->noise))// XDM3038a
			EMIT_SOUND(edict(), CHAN_ITEM, STRING(pev->noise), VOL_NORM, ATTN_NORM);

		pev->impulse = 0;
		pev->skin = 1;
		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pOther->edict());// XDM3038c
			WRITE_SHORT((int)pev->max_health);
			WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();
		SUB_UseTargets(this, USE_TOGGLE, 1);
		SetNextThink(g_pGameRules?g_pGameRules->GetChargerRechargeDelay():60.0f);// XDM3038a // recharge airtank // XDM3034
		pev->teleport_time = gpGlobals->time + 1.0f;// next sound/effect time
	}
}




//=========================================================
// world_items HACK, OBSOLETE, made by VALVe
//=========================================================
class CWorldItem : public CBaseEntity
{
public:
	int m_iType;
	virtual void KeyValue(KeyValueData *pkvd)
	{
		if (FStrEq(pkvd->szKeyName, "type"))
		{
			m_iType = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CBaseEntity::KeyValue(pkvd);
	}
	virtual void Spawn(void)
	{
		CBaseEntity *pEntity = NULL;
		switch (m_iType)
		{
		case 44: pEntity = Create("item_battery", pev->origin, pev->angles); break;
		case 42: pEntity = Create("item_antidote", pev->origin, pev->angles); break;
		case 43: pEntity = Create("item_security", pev->origin, pev->angles); break;
		case 45: pEntity = Create("item_suit", pev->origin, pev->angles); break;
		}
		if (pEntity)
		{
			pEntity->pev->target = pev->target;
			pEntity->pev->targetname = pev->targetname;
			pEntity->pev->spawnflags = pev->spawnflags;
			//pEntity->pev->scale = UTIL_GetWeaponWorldScale();// XDM3035b
		}
		else
			conprintf(1, "%s unable to create world_item %d\n", STRING(pev->classname), m_iType);

		REMOVE_ENTITY(edict());
	}
};
LINK_ENTITY_TO_CLASS(world_items, CWorldItem);
