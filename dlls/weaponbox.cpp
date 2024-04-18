#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "pm_shared.h"
#include "player.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"

DLL_GLOBAL int g_iWeaponBoxCount = 0;// XDM3035: limit number of dropped boxes and weapons in world! WARNING: not only boxes!

// TODO: this should really be transformed into something like CBaseContainer

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

TYPEDESCRIPTION	CWeaponBox::m_SaveData[] =
{
	DEFINE_ARRAY(CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
	DEFINE_ARRAY(CWeaponBox, m_rgpWeapons, FIELD_EHANDLE, MAX_WEAPONS)
};

IMPLEMENT_SAVERESTORE(CWeaponBox, CBaseEntity);

const char g_szCWeaponBoxClassName[] = "weaponbox";

//-----------------------------------------------------------------------------
// Purpose: Remove some dropped weapon boxes from the world to prevent overflows
// Warning: BOXES ONLY!!!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CleanupTemporaryWeaponBoxes(void)
{
	// remove some other box or just temporary entity to prevent overflow
	if (IsMultiplayer())
	{
		int maxboxes = max(32, gpGlobals->maxClients*2);// 2 dropped boxes per player should be enough
		if (g_iWeaponBoxCount > maxboxes)// || count_all_entities >= gpGlobals->maxEntities-256// XDM3035: remove some other box :P UNDONE: remove the oldest one!
		{
			CBaseEntity *pFound = NULL;
			CBaseEntity *pEntity = NULL;
			edict_t *pEdict = INDEXENT(1);
			//while ((pEntity = (CWeaponBox *)UTIL_FindEntityByClassname(pEntity, szClassName)) != NULL)
			for (int i = 1; i < gpGlobals->maxEntities; ++i, ++pEdict)
			{
				if (!UTIL_IsValidEntity(pEdict))
					continue;

				if (FBitSet(pEdict->v.spawnflags, SF_NOREFERENCE|SF_NORESPAWN))// OR // WARNING! This cycle WILL find attached/carried/packed weapons which are still marked like this! DO NOT DESTROY THEM!!
				{
					if (pEdict->v.movetype != MOVETYPE_FOLLOW && pEdict->v.aiment == NULL)// XDM3038: more safety
					{
						pEntity = CBaseEntity::Instance(pEdict);
						if (pEntity)
						{
							if (/* WEAPON BOXES ONLY!!!(pEntity->IsPlayerItem() && pEdict->v.impulse == ITEM_STATE_DROPPED) || */FClassnameIs(&pEdict->v, g_szCWeaponBoxClassName))// XDM3038: more safety
							{
								pFound = /*WTF?! (CWeaponBox *)*/pEntity;// XDM3038: fix
								break;
							}
						}
					}
				}
			}
			if (pFound)
			{
//#if defined (_DEBUG)
//				conprintf(1, "CleanupTemporaryWeaponBoxes(): Removing %s %d to prevent overflow.\n", STRING(pEntity->pev->classname), pEntity->entindex());
//#endif
				pFound->pev->health = 1.0f;// don't disintegrate
				pFound->Killed(g_pWorld, g_pWorld, (g_pGameRules == NULL || g_pGameRules->FAllowEffects())?GIB_DISINTEGRATE:GIB_REMOVE);// common method for safe removal
				pFound = NULL;
				return true;
			}
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Create a weapon box properly
// Input  : *pOwner - optional owner to fall through
//			vecOrigin -
//			vecAngles -
// Output : new instance of a box
//-----------------------------------------------------------------------------
CWeaponBox *CWeaponBox::CreateBox(CBaseEntity *pOwner, const Vector &vecOrigin, const Vector &vecAngles)
{
	CleanupTemporaryWeaponBoxes();
	DBG_ITM_PRINT("CWeaponBox::CreateBox(owner: %d)\n", pOwner?pOwner->entindex():0);
	CWeaponBox *pBox = (CWeaponBox *)CBaseEntity::Create(g_szCWeaponBoxClassName, vecOrigin, vecAngles, pOwner?pOwner->edict():NULL);//GetClassPtr((CWeaponBox *)NULL, g_szCWeaponBoxClassName);
	if (pBox)
	{
		SetBits(pBox->pev->spawnflags, SF_NOREFERENCE|SF_NORESPAWN);
		pBox->Clear();// XDM3035b the whole thing for the sake of this
		pBox->pev->origin = vecOrigin;
		pBox->pev->oldorigin = vecOrigin;
		pBox->pev->angles = vecAngles;
		pBox->pev->angles.x = 0.0f;// don't let weaponbox tilt.
		if (pOwner)
		{
			pBox->pev->velocity = pOwner->pev->velocity; pBox->pev->velocity *= 1.4f;// weaponbox has player's velocity, then some.
			pBox->pev->avelocity = pOwner->pev->avelocity;// XDM3038
			// ^::Create pBox->m_hOwner = pOwner;// IMPORTANT for pickup policy
			pBox->pev->owner = pOwner->edict();// don't collide with owner
			pBox->pev->colormap = pOwner->pev->colormap;// XDM3038c
		}
		pBox->pev->impulse = 1;// not placed by mapper
		pBox->Spawn();
		pBox->SetThink(&CWeaponBox::FallThink);// XDM3038c
		pBox->SetNextThink(0.5);
		++g_iWeaponBoxCount;// XDM: register me!
	}
	return pBox;
}

//-----------------------------------------------------------------------------
// Purpose: legacy: ammo placed by level designer
//-----------------------------------------------------------------------------
void CWeaponBox::KeyValue(KeyValueData *pkvd)
{
	CBaseEntity::KeyValue(pkvd);// XDM3038c
	if (pkvd->fHandled)
		return;

	int count = atoi(pkvd->szValue);
	if (count > 0)
	{
		if (!PackAmmo(pkvd->szKeyName, count))
			conprintf(1, "Warning: WeaponBox could not pack \"%s\"!\n", pkvd->szKeyName);
	}
	else
		conprintf(1, "Warning: WeaponBox has bad amount of \"%s\"!\n", pkvd->szKeyName);

	pkvd->fHandled = TRUE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBox::Spawn(void)
{
	pev->movetype = MOVETYPE_TOSS;

	if (IsMultiplayer() && mp_weaponboxbreak.value > 0.0f)// XDM
		pev->takedamage = DAMAGE_YES;
	else
		pev->takedamage = DAMAGE_NO;

	if (pev->health == 0)
		pev->health = 10.0f;// most triggers should be able to destroy it (because of the touching troubles)

	CBaseEntity::Spawn();

	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model));// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBox::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/w_weaponbox.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	CBaseEntity::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Called during [re]spawning
//-----------------------------------------------------------------------------
void CWeaponBox::Materialize(void)
{
	pev->solid = SOLID_TRIGGER;
	//pev->owner = NULL;
	UTIL_SetSize(this, VEC_HULL_ITEM_MIN, VEC_HULL_ITEM_MAX);
	SetTouch(&CWeaponBox::PickupTouch);
	CBaseEntity::Materialize();
}

//-----------------------------------------------------------------------------
// Purpose: override: removes the box from the world
//-----------------------------------------------------------------------------
void CWeaponBox::Destroy(void)
{
	DBG_ITM_PRINT("%s[%d]::Destroy()\n", STRING(pev->classname), entindex());
	SetTouchNull();
	SetThinkNull();
	Clear();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->aiment = NULL;
	CBaseEntity::Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: Try to regain collision abilities
//-----------------------------------------------------------------------------
void CWeaponBox::FallThink(void)
{
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (pev->owner)
		{
			UTIL_SetOrigin(this, pev->origin);
			pev->owner = NULL;// XDM3038c: TESTME! enable collisions
		}
		pev->avelocity.Clear();
		AlignToFloor();
	}
	else
	{
		UTIL_SetOrigin(this, pev->origin);
		SetNextThink(0.5);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Think function that waits until owner is able to pick up this box again
//-----------------------------------------------------------------------------
void CWeaponBox::WaitForOwner(void)
{
	CBasePlayer *pPlayer = (CBasePlayer *)(CBaseEntity *)m_hOwner;
	if (pPlayer == NULL)
	{
		Destroy();
		return;
	}
	if (pPlayer->IsAlive())
	{
		conprintf(1, "CWeaponBox[%d]::WaitForOwner() returning weapons to owner %s[%d].\n", entindex(), STRING(pPlayer->pev->classname), pPlayer->entindex());
		if (Unpack(pPlayer))
			ClientPrint(VARS(m_hOwner.Get()), HUD_PRINTTALK, "#WPNBOX_BREAKRETURN");

		Destroy();
		return;
	}
	SetNextThink(0.25);
}

//-----------------------------------------------------------------------------
// Purpose: Try to add my contents to the toucher if the toucher is a player.
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CWeaponBox::PickupTouch(CBaseEntity *pOther)
{
	if (!FBitSet(pev->flags, FL_ONGROUND))// ENT_IS_ON_FLOOR()?
	{
		if (pev->owner && (pOther->edict() == pev->owner))// XDM
			return;// Don't touch the player that has just dropped this box
	}

	if (!pOther->IsPlayer())// only players may touch a weaponbox.
	{
		// XDM3035 FL_ONGROUND is not set when touching sky
		//conprintf(0, "CWeaponBox::Touch() !pOther->IsPlayer()\n");
		Vector vecEnd(pev->velocity); vecEnd.SetLength(4.0); vecEnd += pev->origin;//Vector end = pev->origin + pev->velocity.Normalize() * 4.0f;// don't trace too far!
		int pc = POINT_CONTENTS(vecEnd);// XDM
		if (pc == CONTENTS_SLIME || pc == CONTENTS_LAVA || pc == CONTENTS_SKY)
		{
			SetTouchNull();
			SetBits(pev->effects, EF_NODRAW);
			SetThink(&CBaseEntity::SUB_Remove);
			SetNextThink(0.001f);// XDM3038a
			return;
		}
		/*if (pOther->IsBSPModel() && pOther->pev->solid != SOLID_NOT)// FL_ONGROUND and pev->groundentity are not set yet
		{
			if (pev->origin != pev->oldorigin)//(!pev->velocity.IsZero())
			{
				EMIT_SOUND_DYN(edict(), CHAN_VOICE, DEFAULT_DROP_SOUND_WEAPON, VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(90,100));
				//pev->velocity = g_vecZero;
			}
		}*/
		return;
	}

	if (!pOther->IsAlive())// no dead guys.
		return;

	Use(pOther, pOther, USE_ON, 1.0f);// redirect to unpack method
}

//-----------------------------------------------------------------------------
// Purpose: Add this ammo to the box
// Input  : *szName - ammo name, WARNING!!! only safe to provide pointer to g_AmmoInfoArray[i].name!!!
//			iCount -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::PackAmmo(const char *szName, const int &iCount)
{
	if (szName == NULL)
	{
		// error here
		conprintf(1, "CWeaponBox::PackAmmo(): szName is NULL!\n");
		return false;
	}
	DBG_ITM_PRINT("%s[%d]::PackAmmo(\"%s\", %d)\n", STRING(pev->classname), entindex(), szName, iCount);
	if (iCount == 0)
		return false;

	int iAmmoID = GetAmmoIndexFromRegistry(szName);
	if (iAmmoID == AMMOINDEX_NONE)
		return false;

	int iMaxCarry = MaxAmmoCarry(iAmmoID);
	if (iMaxCarry == 0)
		return false;

	// this ammo type may already be in the box, add value
	int iAdd = min(iCount, iMaxCarry - m_rgAmmo[iAmmoID]);
	if (iAdd > 0)
	{
		m_rgAmmo[iAmmoID] += iAdd;
		return true;// added some ammo
	}
	else
		return false;// couldn't pack - full
}

//-----------------------------------------------------------------------------
// Purpose: CWeaponBox - PackWeapon: Add this weapon to the box
// Input  : *pWeapon - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::PackWeapon(CBasePlayerItem *pWeapon)
{
	if (!UTIL_IsValidEntity(pWeapon))//if (pWeapon == NULL || pWeapon->pev == NULL)
	{
		conprintf(1, "CWeaponBox::PackWeapon() error: invalid pWeapon!\n");
		DBG_FORCEBREAK
		return false;
	}
	DBG_ITM_PRINT("%s[%d]::PackWeapon(%s[%d])\n", STRING(pev->classname), entindex(), pWeapon->GetWeaponName(), pWeapon->entindex());
	// is one of these weapons already packed in this box?
	if (HasWeapon(pWeapon->GetID()))
	{
		//DBG_PRINTF("CWeaponBox::PackWeapon(%s[%d]): refused: already have one.\n", pWeapon->GetWeaponName(), pWeapon->entindex());
		return false;// box can only hold one of each weapon type
	}

	if (pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE)// XDM: don't add 'fake' weapons
	{
		bool bFound = false;
		if (pWeapon->GetWeaponPtr())
		{
			if (pWeapon->GetWeaponPtr()->m_iAmmoContained1 > 0 ||
				pWeapon->GetWeaponPtr()->m_iAmmoContained2 > 0)
				bFound = true;
		}
#if defined (_DEBUG)// this is somewhat slow
		if (!bFound)
		{
			int iAmmoID = GetAmmoIndexFromRegistry(pWeapon->pszAmmo1());
			if (iAmmoID != AMMOINDEX_NONE)
				if (m_rgAmmo[iAmmoID] > 0)
					bFound = true;
		}
		if (!bFound)
		{
			int iAmmoID = GetAmmoIndexFromRegistry(pWeapon->pszAmmo2());
			if (iAmmoID != AMMOINDEX_NONE)
				if (m_rgAmmo[iAmmoID] > 0)
					bFound = true;
		}
#endif
		if (!bFound)// quite possible with GR_PLR_DROP_GUN_ALL && GR_PLR_DROP_AMMO_ACTIVE
		{
			DBG_PRINTF("CWeaponBox::PackWeapon(%s[%d]): refusing empty exhaustible weapon.\n", pWeapon->GetWeaponName(), pWeapon->entindex());
			return false;
		}
	}

	pWeapon->AttachTo(this);// origin, non-solid, MOVETYPE_FOLLOW, owner, non-touchable, etc.
	m_rgpWeapons[pWeapon->GetID()] = pWeapon;
	//?SetBits(pWeapon->pev->spawnflags, SF_NORESPAWN);
	//?pWeapon->pev->modelindex = 0;
	//?pWeapon->pev->model = iStringNull;
	pWeapon->SetOwner(NULL);// don't set to 'this'!
	pWeapon->pev->owner = edict();// override

	if (sv_overdrive.value > 0.0f)// lulz
		pev->scale += (2.0f/(float)MAX_WEAPONS);

	//DBG_PRINTF("CWeaponBox::PackWeapon(\"%s\"): Packed weapon.\n", szName);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Unpack contents into player's inventory
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::Unpack(CBasePlayer *pPlayer)
{
	if (pPlayer)
	{
		DBG_ITM_PRINT("%s[%d]::Unpack(%s[%d])\n", STRING(pev->classname), entindex(), STRING(pPlayer->pev->classname), pPlayer->entindex());
		size_t n = 0;
		n += UnpackAmmo(pPlayer);// unpack ammo first!
		n += UnpackWeapons(pPlayer);
		if (n > 0)
		{
			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, DEFAULT_PICKUP_SOUND_CONTAINER, VOL_NORM, ATTN_IDLE);
			// don't have to pPlayer->SendWeaponsUpdate();
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Unpack ammo into player's inventory
// Warning: Player must be given ammo first, so the weapons code may deploy a better weapon that the player may pick up because he has no ammo for it.
// Input  : *pPlayer - 
// Output : Number of weapons added
//-----------------------------------------------------------------------------
size_t CWeaponBox::UnpackAmmo(CBasePlayer *pPlayer)
{
	DBG_ITM_PRINT("%s[%d]::UnpackAmmo(%s[%d])\n", STRING(pev->classname), entindex(), STRING(pPlayer->pev->classname), pPlayer->entindex());
	int iAmmoIndex, iAmount;
	size_t i, n = 0;
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)
	{
#if defined(OLD_WEAPON_AMMO_INFO)
		iAmount = pPlayer->GiveAmmo(m_rgAmmo[i], i, MaxAmmoCarry(i));
#else
		iAmount = pPlayer->GiveAmmo(m_rgAmmo[i], i);
#endif
		if (iAmount > 0)
		{
			++n;
			m_rgAmmo[i] -= iAmount;
			//conprintf(2, "CWeaponBox: Unpacked %d rounds of \"%s\".\n", iAmount, g_AmmoInfoArray[i].name));
		}
	}
	DBG_ITM_PRINT("%s[%d]::UnpackAmmo(%s[%d]): Unpacked %u ammo types.\n", STRING(pev->classname), entindex(), STRING(pPlayer->pev->classname), pPlayer->entindex(), n);
	return n;
}

//-----------------------------------------------------------------------------
// Purpose: Unpack weapons into player's inventory
// Input  : *pPlayer - 
// Output : Number of weapons added
//-----------------------------------------------------------------------------
size_t CWeaponBox::UnpackWeapons(CBasePlayer *pPlayer)
{
	DBG_ITM_PRINT("%s[%d]::UnpackWeapons(%s[%d])\n", STRING(pev->classname), entindex(), STRING(pPlayer->pev->classname), pPlayer->entindex());
	size_t i, n = 0;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (m_rgpWeapons[i])
		{
			CBasePlayerItem *pItem = (CBasePlayerItem *)(CBaseEntity *)m_rgpWeapons[i];
			if (g_pGameRules && g_pGameRules->CanHavePlayerItem(pPlayer, pItem))
			{
				if (pPlayer->AddPlayerItem(pItem))
				{
					pItem->AttachTo(pPlayer);
					//conprintf(2, "CWeaponBox: Unpacked \"%s\".\n", STRING(pItem->pev->classname));
					m_rgpWeapons[i] = NULL;
					++n;
				}
				else
				{
					DBG_PRINTF("CWeaponBox: Failed to give \"%s\".\n", STRING(pItem->pev->classname));
				}
			}
			/*else
			{
				DBG_PRINTF("CWeaponBox(%d)::Touch(): Player cannot have \"%s\"\n", entindex(), STRING(pItem->pev->classname));
			}*/
			// Not here! m_rgpWeapons[i] = NULL;
		}
	}
	DBG_ITM_PRINT("%s[%d]::UnpackAmmo(%s[%d]): Unpacked %u weapons.\n", STRING(pev->classname), entindex(), STRING(pPlayer->pev->classname), pPlayer->entindex(), n);
	return n;
}

//-----------------------------------------------------------------------------
// Purpose: Remove boxes that fell into lava, etc.
//-----------------------------------------------------------------------------
int CWeaponBox::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)
		return 0;

	if (FBitSet(pev->spawnflags, SF_NORESPAWN) && pev->impulse > 0)// dropped by somebody
	{
		pev->health -= flDamage;
		if (pev->health <= 0.0f)
			Killed(pInflictor, pAttacker, GIB_NORMAL);

		return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destroyed by external forces
//-----------------------------------------------------------------------------
void CWeaponBox::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)// XDM3035c
{
	DBG_ITM_PRINT("%s[%d]::Killed()\n", STRING(pev->classname), entindex());
	if (IsMultiplayer() && mp_weaponboxbreak.value > 1.0f)// XDM3038c
	{
		pev->takedamage = DAMAGE_NO;
		if (m_hOwner.IsValid() && m_hOwner->IsPlayer())// return weapons to their owner
		{
			CBasePlayer *pPlayer = (CBasePlayer *)(CBaseEntity *)m_hOwner;
			if (pPlayer->IsAlive())
			{
#if defined (_DEBUG)
				conprintf(1, "CWeaponBox::Killed() returning weapons to owner.\n");
#endif
				if (Unpack(pPlayer))
					ClientPrint(VARS(m_hOwner.Get()), HUD_PRINTTALK, "#WPNBOX_BREAKRETURN");
			}
			else if (g_pGameRules && g_pGameRules->FPlayerCanRespawn(pPlayer))// player must be waiting to respawn
			{
#if defined (_DEBUG)
				conprintf(1, "CWeaponBox::Killed() following respawning player.\n");
#endif
				pev->movetype = MOVETYPE_FOLLOW;
				pev->effects |= EF_NODRAW;
				pev->aiment = pPlayer->edict();
				UTIL_SetOrigin(this, pPlayer->pev->origin);
				SetThink(&CWeaponBox::WaitForOwner);
				SetNextThink(0.25f);
				return;// don't remove
			}
		}
	}
	if (g_pGameRules && g_pGameRules->FAllowEffects())
	{
		pev->mins.Set(-2,-2,-2);// XDM3035: for Disintegrate()
		pev->maxs.Set(2,2,2);
		Disintegrate();// XDM3035: allow box to be removed by common code ONLY HERE!!
	}
	else
	{
		pev->solid = SOLID_NOT;
		SetBits(pev->effects, EF_NODRAW);
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.001f);// XDM3038a
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overload
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::ShouldRespawn(void) const
{
	if (g_SilentItemPickup)
		return false;
	if (FBitSet(pev->spawnflags, SF_NORESPAWN))
		return false;
	if (g_pGameRules)
	{
		if (!g_pGameRules->FItemShouldRespawn(this))
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: allow player to grab it (in case item is stuck somewhere visually reachable)
// Input  : *pActivator - ...
//-----------------------------------------------------------------------------
void CWeaponBox::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_ITM_PRINT("%s[%d]::Use()\n", STRING(pev->classname), entindex());
	if (pActivator && pActivator->IsPlayer() && pCaller == pActivator)
	{
		if (FVisible(pActivator))
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			if (g_pGameRules)
			{
				if (/*m_hOwner.Get() && */!g_pGameRules->CanPickUpOwnedThing(pPlayer, this))// XDM3038c
					return;
			}
			Unpack(pPlayer);
			if (sv_partialpickup.value <= 0.0f || IsEmpty())
			{
				//pev->mins.Clear();
				//pev->maxs.Clear();
				SetBits(pev->effects, EF_NODRAW);
				SetThink(&CBaseEntity::SUB_Remove);
				SetNextThink(0.001f);// XDM3038a
				// XDM3038: unsafe!	Destroy(this);
			}
			//else cannot print because may be called every frame and cause overflow
			//	ClientPrint(pOther->pev, HUD_PRINTCENTER, "#CONTAINER_REMAIN", UTIL_dtos1(n));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is a weapon of this type already packed in this box?
// Input  : iID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::HasWeapon(const int &iID) const// XDM3035b: explicitly check by ID and NOT by pointer!
{
	return (m_rgpWeapons[iID].IsValid() != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Is there anything in this box?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::IsEmpty(void) const
{
	size_t i;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (m_rgpWeapons[i].IsValid())
			return false;
	}
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)
	{
		if (m_rgAmmo[i] > 0)
			return false;// still have a bit of this type of ammo
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035b: more safety?
//-----------------------------------------------------------------------------
void CWeaponBox::Clear(void)
{
	DBG_ITM_PRINT("%s[%d]::Clear()\n", STRING(pev->classname), entindex());
	size_t i;
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)
		m_rgAmmo[i] = 0;

	CBasePlayerItem *pWeapon = NULL;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		pWeapon = (CBasePlayerItem *)(CBaseEntity *)m_rgpWeapons[i];
		if (pWeapon && /*pWeapon->IsPlayerItem() && */pWeapon->pev)// XDM3035b: catch all possible memory violations??
		{
			SetBits(pWeapon->pev->effects, EF_NODRAW);
			pWeapon->Destroy();
			//REMOVE_ENTITY(pWeapon->edict());// CRASH
		}
		m_rgpWeapons[i] = NULL;// XDM3035b: !
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035b: Memory cleanup
//-----------------------------------------------------------------------------
void CWeaponBox::OnFreePrivateData(void)
{
	// destroy the weapons
	//try this commented
	//	if (g_pGameRules && !g_pGameRules->IsGameOver())
	// UNSAFE: when the engine is shutting down, attempting to shutdown... NO! Is't not shutting down! It's not! Aaaarrrrgh!		Clear();

	--g_iWeaponBoxCount;// XDM3035: unregister me (safe to use global variable here)
	CBaseEntity::OnFreePrivateData();// XDM3038c
}


//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Show it on map in CoOp
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::ShowOnMap(CBasePlayer *pPlayer) const
{
	if (g_pGameRules && g_pGameRules->IsCoOp())//IsMultiplayer())
	{
		if (m_hOwner.IsValid())
		{
			if (m_hOwner == pPlayer)//if (g_pGameRules->CanPickUpOwnedThing(pPlayer, this))
				return true;
		}
	}
	return CBaseEntity::ShowOnMap(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CWeaponBox::ReportState(int printlevel)
{
	CBaseEntity::ReportState(printlevel);
	char szString[256] = "Contents: weapons: ";
	char szNumber[8];
	uint i;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (m_rgpWeapons[i])
		{
			//CBasePlayerItem *pItem = (CBasePlayerItem *)(CBaseEntity *)m_rgpWeapons[i];
			//_snprintf(szNumber, 7, "%d, ", pItem->GetID());// print numbers because they are shorter?
			//szNumber[7] = '\0';
			//strcat(szString, szNumber);
			strcat(szString, STRING(m_rgpWeapons[i]->pev->classname));
			strcat(szString, ", ");
			//szString[255] = '\0';
		}
	}
	strcat(szString, "ammo: ");
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)
	{
		if (m_rgAmmo[i] > 0)
		{
			strcat(szString, g_AmmoInfoArray[i].name);// name
			_snprintf(szNumber, 7, ": %d, ", m_rgAmmo[i]);// amount
			szNumber[7] = '\0';
			strcat(szString, szNumber);
			//szString[255] = '\0';
		}
	}
	strcat(szString, "\n");
	szString[255] = '\0';
	conprintf(printlevel, szString);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: General contents sanity checks
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBox::ValidateContents(void)
{
	CBasePlayerItem *pItem;
	uint i, j, numerrors = 0;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (m_rgpWeapons[i])
		{
			pItem = (CBasePlayerItem *)(CBaseEntity *)m_rgpWeapons[i];
			if (pItem->iFlags() & ITEM_FLAG_EXHAUSTIBLE)
			{
				if (pItem->GetWeaponPtr())
					if (pItem->GetWeaponPtr()->m_iAmmoContained1 > 0 ||
						pItem->GetWeaponPtr()->m_iAmmoContained2 > 0)
						continue;

				for (j=0; j<MAX_AMMO_SLOTS; ++j)
				{
					if (m_rgAmmo[j] > 0 && FStrEq(g_AmmoInfoArray[j].name, pItem->pszAmmo1()) ||
											FStrEq(g_AmmoInfoArray[j].name, pItem->pszAmmo2()))
						break;// found
				}
				if (j >= MAX_AMMO_SLOTS)
				{
					conprintf(2, "CWeaponBox[%d]::ValidateContents() error: weapon %s and no ammo!\n", entindex(), pItem->GetWeaponName());
					numerrors++;
				}
			}
			if (pItem->GetHost())
			{
				conprintf(2, "CWeaponBox[%d]::ValidateContents() error: weapon %s has player host!\n", entindex(), pItem->GetWeaponName());
				numerrors++;
			}
			if (pItem->GetExistenceState() != ENTITY_EXSTATE_CONTAINER)
			{
				conprintf(2, "CWeaponBox[%d]::ValidateContents() error: weapon %s has bad ExistenceState %u!\n", entindex(), pItem->GetWeaponName(), pItem->GetExistenceState());
				numerrors++;
			}
			if (pItem->m_flRemoveTime > 0.0)
			{
				conprintf(2, "CWeaponBox[%d]::ValidateContents() error: weapon %s has remove time set!\n", entindex(), pItem->GetWeaponName());
				numerrors++;
			}
		}
	}
	return numerrors == 0;
}
