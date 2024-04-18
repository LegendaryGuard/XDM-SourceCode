// XDM: No need to implement CBasePlayerItem/Weapon classes here anymore
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "weapons/weapons.h"
//#include "event_api.h"
//#include "r_efx.h"
#include "hud.h"
#include "com_weapons.h"

// XDM: right now this code is mostly outdated and useless

//extern globalvars_t *gpGlobals;
//extern int g_iUser1;
// Pool of client side entities/entvars_t
static entvars_t ev[MAX_WEAPONS];
static int num_ents = 0;
// The entity we'll use to represent the local client
CBasePlayer g_ClientPlayer;
// Local version of game .dll global variables ( time, etc. )
static globalvars_t	Globals; 
//static CBasePlayerWeapon *g_pWpns[MAX_WEAPONS];

//float g_flApplyVel = 0.0;
//int g_irunninggausspred = 0;
//static float previousorigin[3];

int giAmmoIndex = 0;
DLL_GLOBAL AmmoInfo g_AmmoInfoArray[MAX_AMMO_SLOTS];

// Weapon placeholder entities
CWeaponCrowbar g_Crowbar;
CWeaponGlock g_Glock;
CWeaponPython g_Python;
CWeaponMP5 g_Mp5;
CWeaponChemGun g_ChemGun;// XDM: was unused slot
CWeaponCrossbow g_Crossbow;
CWeaponShotgun g_Shotgun;
CWeaponRPG g_Rpg;
CWeaponGauss g_Gauss;
CWeaponEgon g_Egon;
CWeaponHornetGun g_HGun;
CWeaponHandGrenade g_HandGren;
CWeaponTripmine g_Tripmine;
CWeaponSatchel g_Satchel;
CWeaponSqueak g_Snark;
CWeaponDiskLauncher g_DLauncher;
CWeaponGrenadeLauncher g_GLauncher;
CWeaponAcidLauncher g_ALauncher;
CWeaponSword g_Sword;
CWeaponSniperRifle g_SniperRifle;
CWeaponStrikeTarget g_StrikeTarget;
CWeaponPlasma g_Plasma;
CWeaponFlame g_Flame;
CWeaponDisplacer g_Displacer;
CWeaponBeamRifle g_BeamRifle;
CWeaponBHG g_BHG;
// (unused slot)
// (unused slot)
CWeaponCustom g_CustomWeapon1;
CWeaponCustom g_CustomWeapon2;
// END

// MUST BE IN ORDER OF WEAPON ID!
CBasePlayerWeapon *g_pClientWeapons[PLAYER_INVENTORY_SIZE] = {
	NULL,// 0 - WEAPON_NONE
	&g_Crowbar,
	&g_Glock,
	&g_Python,
	&g_Mp5,
	&g_ChemGun,// XDM: was unused slot
	&g_Crossbow,
	&g_Shotgun,
	&g_Rpg,
	&g_Gauss,
	&g_Egon,// 10
	&g_HGun,
	&g_HandGren,
	&g_Tripmine,
	&g_Satchel,
	&g_Snark,
	&g_DLauncher,
	&g_GLauncher,// XDM weapons start here
	&g_ALauncher,
	&g_Sword,
	&g_SniperRifle,// 20
	&g_StrikeTarget,
	&g_Plasma,
	&g_Flame,
	&g_Displacer,
	&g_BeamRifle,
	&g_BHG,
	NULL,// (unused slot)
	NULL,// (unused slot)
	&g_CustomWeapon1,
	&g_CustomWeapon2,// 30
	NULL// (suit)
};


//-----------------------------------------------------------------------------
// Purpose: HACK: Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then we set up the m_pPlayer field.
// Input  : 
//-----------------------------------------------------------------------------
void HUD_PrepEntity(CBaseEntity *pEntity)
{
	if (pEntity)
	{
		memset(&ev[num_ents], 0, sizeof(entvars_t));
		pEntity->pev = &ev[num_ents++];
		pEntity->Precache();
		pEntity->Spawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: HACK:
// Input  : 
//-----------------------------------------------------------------------------
void HUD_PrepWeapon(CBasePlayerItem *pItem, CBasePlayer *pWeaponOwner)
{
	if (pItem)
	{
		HUD_PrepEntity(pItem);
		//g_pWpns[pWeapon->GetID()] = pWeapon;
		pItem->SetOwner(pWeaponOwner);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server stub
// Input  : szAmmoName - 
//			iMaxCarry
// Output : int
//-----------------------------------------------------------------------------
int AddAmmoToRegistry(const char *szAmmoName, int iMaxCarry)
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: maximum amount of that type of ammunition that a player can carry.
// Input  : ammoID - 
// Output : int
//-----------------------------------------------------------------------------
short MaxAmmoCarry(int ammoID)
{
	if (ammoID >= 0)
		return gHUD.m_Ammo.MaxAmmoCarry(ammoID);

	return 0;// XDM3035c: TESTME!! was -1
}





//-----------------------------------------------------------------------------
// Purpose: Does the player already have EXACTLY THIS INSTANCE of item?
// Input  : *pCheckItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasPlayerItem(CBasePlayerItem *pCheckItem)
{
	if (pCheckItem == NULL)
		return false;

	if (GetInventoryItem(pCheckItem->GetID()) == pCheckItem)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Safe way to access player's inventory
// Input  : &iID - 
// Output : CBasePlayerItem *
//-----------------------------------------------------------------------------
CBasePlayerItem *CBasePlayer::GetInventoryItem(const int &iItemId)
{
#if defined (_DEBUG)
	ASSERT(pev != NULL);
#endif
	if (iItemId >= WEAPON_NONE && iItemId < PLAYER_INVENTORY_SIZE)
	{
		if (m_rgpWeapons[iItemId])
		{
			CBasePlayerItem *pItem = (CBasePlayerItem *)(CBaseEntity *)m_rgpWeapons[iItemId];
			if (pItem->GetHost() != this)
			{
				ALERT(at_console, "CBasePlayer(%d)::GetInventoryItem(%d)(ei %d id %d) bad item owner!\n", entindex(), iItemId, pItem->entindex(), pItem->GetID());
#if defined(_DEBUG_ITEMS)
				DBG_FORCEBREAK
#endif
				pItem->SetOwner(this);
			}

#if defined (_DEBUG)
			if (m_rgpWeapons[iItemId]->pev && pItem->GetID() > WEAPON_NONE)
#endif
				return pItem;
#if defined(_DEBUG) && defined(_DEBUG_ITEMS)
			else
				ALERT(at_console, "CBasePlayer(%d)::GetInventoryItem(%d)(ei %d id %d) got bad item!\n", entindex(), iItemId, m_rgpWeapons[iItemId]->entindex(), pItem->GetID());
#endif
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::DeployActiveItem(void)// XDM
{
	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
		// XDM3035b: obsolete	m_pActiveItem->UpdateItemInfo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pItem - 
//-----------------------------------------------------------------------------
void CBasePlayer::QueueItem(CBasePlayerItem *pItem)// XDM
{
//	if (pItem && pItem->m_iId == WEAPON_NONE)
	if (pItem == NULL || pItem->GetID() == WEAPON_NONE)// XDM3035: wtf?
		return;

	if (!m_pActiveItem)// no active weapon
	{
		m_pActiveItem = pItem;
		return;// just set this item as active
	}
    else// remember active item
	{
		m_pLastItem = m_pActiveItem;
		m_pActiveItem = NULL;// clear current
	}
	m_pNextItem = pItem;// add item to queue
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::SelectItem(CBasePlayerItem *pItem)
{
	if (pItem == NULL)
		return false;

	if (!pItem->CanDeploy())// XDM: TESTME!!
		return false;

	if (m_pActiveItem)
	{
		if (pItem == m_pActiveItem)
			return true;

		if (!m_pActiveItem->CanHolster())// XDM
			return false;
	}

	ResetAutoaim();

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	QueueItem(pItem);// XDM
	DeployActiveItem();// XDM: QueueItem() sets m_pActiveItem if we have no current weapon
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Select item by ID
// Input  : iID - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::SelectItem(const int &iID)
{
	if (iID <= WEAPON_NONE || iID >= PLAYER_INVENTORY_SIZE)// ??? allow WEAPON_NONE?
		return false;

	if (GetInventoryItem(iID))
		return SelectItem(m_rgpWeapons[iID]);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Select item by name
// Input  : *pstr - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::SelectItem(const char *pstr)
{
	if (pstr == NULL)
		return false;

	CBasePlayerItem *pItem = NULL;
	for (int i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		pItem = GetInventoryItem(i);
		if (pItem)
		{
			if (strcmp(pItem->GetWeaponName(), pstr) == 0)// WARNING: may not work on client side!!
			{
				return SelectItem(pItem);
				break;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Select Next Best Item
// Warning: XDM3037: moved to client, a command is set back to the server
// Input  : *pItem - 
// Output : CBasePlayerItem
//-----------------------------------------------------------------------------
CBasePlayerItem *CBasePlayer::SelectNextBestItem(CBasePlayerItem *pItem)
{
#ifdef CLIENT_DLL
	int iBestID = gHUD.m_Ammo.GetNextBestItem(pItem->GetID());
	gHUD.m_Ammo.SelectItem(iBestID);
	return GetInventoryItem(iBestID);
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/* XDM3038: OBSOLETE void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem || !m_pLastItem->CanDeploy())// XDM
		return;

	if (m_pActiveItem && !m_pActiveItem->CanHolster())
		return;

	ResetAutoaim();

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	QueueItem(m_pLastItem);// XDM
	DeployActiveItem();// XDM
}*/

//-----------------------------------------------------------------------------
// Purpose: HasWeapons - do I have any weapons at all?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasWeapons(void)
{
	for (short i = WEAPON_NONE; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		if (GetInventoryItem(i))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: allows future flexibility
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasSuit(void)
{
	return FBitSet(pev->weapons, (1<<WEAPON_SUIT));// TODO: move suit to m_rgItems ?
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			iGib - 
//-----------------------------------------------------------------------------
void CBasePlayer::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	//g_irunninggausspred = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : restore - 
//-----------------------------------------------------------------------------
void CBasePlayer::Spawn(byte restore)
{
	Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::Spawn(void)
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy();

	//g_irunninggausspred = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : damage - 
// Output : float
//-----------------------------------------------------------------------------
float CBasePlayer::DamageForce(const float &damage)
{
	return PLAYER_DAMAGE_FORCE_MULTIPLIER*CBaseEntity::DamageForce(damage);
}

//-----------------------------------------------------------------------------
// Purpose: Enable client controls (movement, etc.)
// Input  : fControl - 
//-----------------------------------------------------------------------------
void CBasePlayer::EnableControl(bool fControl)
{
	if (fControl)
		ClearBits(pev->flags, FL_FROZEN);
	else
		SetBits(pev->flags, FL_FROZEN);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE if the player is attached to a ladder
//-----------------------------------------------------------------------------
bool CBasePlayer::IsOnLadder(void)
{
	return (pev->movetype == MOVETYPE_FLY);
}

bool CBasePlayer::IsObserver(void) const
{
//#ifdef CLIENT_DLL
//	return IsSpectator(entindex());
//#else
	if (pev->iuser1 != OBS_NONE && pev->iuser1 != OBS_INTERMISSION)
		return true;

	return false;
//#endif
}




/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons(void)
{
	static bool initialized = 0;
	if (initialized)
		return;

	initialized = 1;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;
	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();
	// Fake functions
	g_engfuncs.pfnPrecacheModel		= stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound		= stub_PrecacheSound;
	g_engfuncs.pfnSetModel			= stub_SetModel;
	g_engfuncs.pfnModelIndex		= stub_ModelIndex;// XDM3037a
	g_engfuncs.pfnIndexOfEdict		= stub_IndexOfEdict;// XDM3037a
	g_engfuncs.pfnFindEntityByVars	= stub_FindEntityByVars;// XDM3037a
	g_engfuncs.pfnNameForFunction	= stub_NameForFunction;
	g_engfuncs.pfnRandomLong		= gEngfuncs.pfnRandomLong;//stub_RandomLong;
	g_engfuncs.pfnRandomFloat		= gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;
	//g_engfuncs.pfnPrecacheEvent	= stub_PrecacheEvent;
	g_engfuncs.pfnPrecacheEvent		= gEngfuncs.pfnPrecacheEvent;
	// Handled locally
	g_engfuncs.pfnPlaybackEvent		= HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage		= AlertMessage;
	// Pass through to engine
	g_engfuncs.pfnAllocString		= stub_AllocString;// XDM3037a

	// Allocate a slot for the local player
	HUD_PrepEntity(&g_ClientPlayer);

	// Allocate slot(s) for each weapon that we are going to be predicting
	for (size_t i=0; i<PLAYER_INVENTORY_SIZE; ++i)
		HUD_PrepWeapon(g_pClientWeapons[i], &g_ClientPlayer);// XDM3037a
}

//-----------------------------------------------------------------------------
// Purpose: HACK: Retruns the last position that we stored for egon beam endpoint.
// Input  : 
//-----------------------------------------------------------------------------
/*void HUD_GetLastOrg(float *org)
{
	// Return last origin
	for (short i = 0; i < 3; ++i)
		org[i] = previousorigin[i];
}*/

//-----------------------------------------------------------------------------
// Purpose: HACK: Remember our exact predicted origin so we can draw the egon to the right position.
//-----------------------------------------------------------------------------
/*void HUD_SetLastOrg(void)
{
	// Offset final origin by view_offset
	for (short i = 0; i < 3; ++i)
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[i];
}*/

//-----------------------------------------------------------------------------
// Purpose: HACK: Client weapons run here
// Input  : from - 
//			to - 
//			cmd - 
//			time -
//			random_seed - 
//-----------------------------------------------------------------------------
void HUD_WeaponsPostThink(local_state_s *from, local_state_s *to, usercmd_t *cmd, const double &time, const unsigned int &random_seed)
{
	static int lasthealth;
	CBasePlayerWeapon *pWeapon = NULL;
	CBasePlayerWeapon *pCurrent = NULL;
	//weapon_data_t nulldata, *pto;
	//memset( &nulldata, 0, sizeof( nulldata ) );
	HUD_InitClientWeapons();	

	// Get current clock
	gpGlobals->time = time;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	pWeapon = g_pClientWeapons[from->client.m_iId];// XDM3037a

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if (g_runfuncs)
	{
		if (to->client.health <= 0 && lasthealth > 0)
			g_ClientPlayer.Killed(NULL, NULL, 0);
		else if (to->client.health > 0 && lasthealth <= 0)
			g_ClientPlayer.Spawn();

		lasthealth = (int)to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if (pWeapon == NULL)
		return;

	short i;
	//pWeapon->ClientPostFrame(from, to, cmd, time, random_seed);// XDM3035c

#if defined(CLIENT_WEAPONS)
	weapon_data_t *pfrom;

	for (i = 0; i < PLAYER_INVENTORY_SIZE; i++)
	{
		pCurrent = g_pClientWeapons[i];
		if ( !pCurrent )
			continue;

		pfrom = &from->weapondata[i];
		pCurrent->ClientPostFrame(from, to, cmd, time, random_seed);// XDM3035c
	}
#endif

	// For random weapon events, use this seed to seed random # generator
	g_ClientPlayer.random_seed = random_seed;

	// ---- fill from entity_state_t playerstate
	// Get old buttons from previous state.
	g_ClientPlayer.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	int buttonsChanged = (g_ClientPlayer.m_afButtonLast ^ cmd->buttons);	// These buttons have changed this frame
	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	g_ClientPlayer.m_afButtonPressed = buttonsChanged & cmd->buttons;	
	// The ones not down are "released"
	g_ClientPlayer.m_afButtonReleased = buttonsChanged & (~cmd->buttons);

	// ---- fill from usercmd_t cmd
	// Set player variables that weapons code might check/alter
	g_ClientPlayer.pev->button = cmd->buttons;
	g_ClientPlayer.pev->impulse = cmd->impulse;// XDM3037a

	// ---- fill from clientdata_t client
	g_ClientPlayer.pev->velocity = from->client.velocity;
	//g_ClientPlayer.pev->viewmodel = from->client.viewmodel;// XDM3037a: CHECKME!!
	g_ClientPlayer.pev->punchangle = from->client.punchangle;// XDM3037a
	g_ClientPlayer.pev->flags = from->client.flags;
	g_ClientPlayer.pev->waterlevel = from->client.waterlevel;
	g_ClientPlayer.pev->watertype = from->client.watertype;
	g_ClientPlayer.pev->view_ofs = from->client.view_ofs;
	g_ClientPlayer.pev->health = from->client.health;
	g_ClientPlayer.pev->bInDuck = from->client.bInDuck;
	g_ClientPlayer.pev->weapons = from->client.weapons;
	g_ClientPlayer.pev->flTimeStepSound = from->client.flTimeStepSound;
	g_ClientPlayer.pev->flDuckTime = from->client.flDuckTime;
	g_ClientPlayer.pev->flSwimTime = from->client.flSwimTime;
	// = from->client.waterjumptime;
	g_ClientPlayer.pev->maxspeed = from->client.maxspeed;
	g_ClientPlayer.pev->fov = from->client.fov;
	g_ClientPlayer.pev->weaponanim = from->client.weaponanim;
	//g_ClientPlayer.m_pActiveItem = from->client.m_iId;
	/*g_ClientPlayer.ammo_buckshot = from->client.ammo_shells;
	g_ClientPlayer.ammo_bolts = from->client.ammo_nails;
	g_ClientPlayer.ammo_uranium = from->client.ammo_cells;
	g_ClientPlayer.ammo_rockets = from->client.ammo_rockets;*/
	g_ClientPlayer.m_flNextAttack = from->client.m_flNextAttack;
	//NO!	g_ClientPlayer.pev->gamestate = from->client.tfstate;
	g_ClientPlayer.pev->pushmsec = from->client.pushmsec;
	g_ClientPlayer.pev->deadflag = from->client.deadflag;
	//	physinfo?
	g_ClientPlayer.pev->iuser1 = from->client.iuser1;
	g_ClientPlayer.pev->iuser2 = from->client.iuser2;
	g_ClientPlayer.pev->iuser3 = from->client.iuser3;
	g_ClientPlayer.pev->iuser4 = from->client.iuser4;
	g_ClientPlayer.pev->fuser1 = from->client.fuser1;
	g_ClientPlayer.pev->fuser2 = from->client.fuser2;
	g_ClientPlayer.pev->fuser3 = from->client.fuser3;
	g_ClientPlayer.pev->fuser4 = from->client.fuser4;
	g_ClientPlayer.pev->vuser1 = from->client.vuser1;
	g_ClientPlayer.pev->vuser2 = from->client.vuser2;
	g_ClientPlayer.pev->vuser3 = from->client.vuser3;
	g_ClientPlayer.pev->vuser4 = from->client.vuser4;
	//Stores all our ammo info, so the client side weapons can use them.
	// XDM: HACKHACKHACK! SUX!
/*#if defined(CLIENT_WEAPONS)
	g_ClientPlayer.ammo_9mm			= (int)from->client.vuser1[0];
	g_ClientPlayer.ammo_357			= (int)from->client.vuser1[1];
	g_ClientPlayer.ammo_argrens		= (int)from->client.vuser1[2];
	g_ClientPlayer.ammo_bolts		= (int)from->client.ammo_nails; //is an int anyways...
	g_ClientPlayer.ammo_buckshot	= (int)from->client.ammo_shells; 
	g_ClientPlayer.ammo_uranium		= (int)from->client.ammo_cells;
	g_ClientPlayer.ammo_hornets		= (int)from->client.vuser2[0];
	g_ClientPlayer.ammo_rockets		= from->client.ammo_rockets;
#endif*/

	// Point to current weapon object
	if (from->client.m_iId > WEAPON_NONE && from->client.m_iId < MAX_WEAPONS)
		g_ClientPlayer.m_pActiveItem = g_pClientWeapons[from->client.m_iId];

	// HACK!
	if (g_ClientPlayer.m_pActiveItem && g_ClientPlayer.m_pActiveItem->GetID() == WEAPON_RPG)
	{
		((CWeaponRPG *)g_ClientPlayer.m_pActiveItem)->m_fSpotActive = (int)from->client.vuser2[1];
		((CWeaponRPG *)g_ClientPlayer.m_pActiveItem)->m_cActiveRockets = (int)from->client.vuser2[2];
	}
	
	// Don't go firing anything if we have died or are spectating
	// Or if we don't have a weapon model deployed
	if ((g_ClientPlayer.pev->deadflag != (DEAD_DISCARDBODY + 1)) && !CL_IsDead() && g_ClientPlayer.pev->viewmodel && g_iUser1 == 0)//!gHUD.IsSpectator())//
	{
		if (g_ClientPlayer.m_flNextAttack <= 0)
			pWeapon->ItemPostFrame();
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId	 = from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if (cmd->weaponselect && (g_ClientPlayer.pev->deadflag != (DEAD_DISCARDBODY + 1)))
	{
		// Switched to a different weapon?
		if (from->weapondata[cmd->weaponselect].m_iId == cmd->weaponselect)
		{
			CBasePlayerWeapon *pNew = g_pClientWeapons[cmd->weaponselect];
			if (pNew && (pNew != pWeapon))
			{
				// Put away old weapon
				if (g_ClientPlayer.m_pActiveItem)
					g_ClientPlayer.m_pActiveItem->Holster();

				g_ClientPlayer.m_pLastItem = g_ClientPlayer.m_pActiveItem;
				g_ClientPlayer.m_pActiveItem = pNew;

				// Deploy new weapon
				if (g_ClientPlayer.m_pActiveItem)
					g_ClientPlayer.m_pActiveItem->Deploy();

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel					= g_ClientPlayer.pev->viewmodel;
	to->client.fov						= g_ClientPlayer.pev->fov;
	to->client.weaponanim				= g_ClientPlayer.pev->weaponanim;
	to->client.m_flNextAttack			= g_ClientPlayer.m_flNextAttack;
	to->client.fuser2					= pWeapon->m_flNextAmmoBurn;
	//to->client.fuser3					= pWeapon->m_flStartCharge;
	to->client.maxspeed					= g_ClientPlayer.pev->maxspeed;
	//HL Weapons
	// XDM: HACKHACKHACK! SUX!
/*#if defined(CLIENT_WEAPONS)
	to->client.vuser1[0]				= g_ClientPlayer.ammo_9mm;
	to->client.vuser1[1]				= g_ClientPlayer.ammo_357;
	to->client.vuser1[2]				= g_ClientPlayer.ammo_argrens;
	to->client.ammo_nails			= g_ClientPlayer.ammo_bolts;
	to->client.ammo_shells			= g_ClientPlayer.ammo_buckshot;
	to->client.ammo_cells			= g_ClientPlayer.ammo_uranium;
	to->client.vuser2[0]				= g_ClientPlayer.ammo_hornets;
	to->client.ammo_rockets			= g_ClientPlayer.ammo_rockets;
#endif*/
	// HACK!
	/* Must be in CWeaponRPG::ClientPostFrame()!
	if (g_ClientPlayer.m_pActiveItem->GetID() == WEAPON_RPG)
	{
		from->client.vuser2[1] = ((CWeaponRPG *)g_ClientPlayer.m_pActiveItem)->m_fSpotActive;
		from->client.vuser2[2] = ((CWeaponRPG *)g_ClientPlayer.m_pActiveItem)->m_cActiveRockets;
	}*/

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if (g_runfuncs && (HUD_GetWeaponAnim() != to->client.weaponanim))
	{
		// HACK! OBSOLETE
		/*int body = 2;
		//Pop the model to body 0.
		if (pWeapon == &g_Tripmine)
			 body = 0;

		//Show laser sight/scope combo
		if ( pWeapon == &g_Python && IsMultiplayer() )
			 body = 1;*/
		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim(to->client.weaponanim, g_ClientPlayer.m_pActiveItem->pev->body, 1);
	}

	weapon_data_t *pto;
	for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		pCurrent = g_pClientWeapons[i];
		pto = &to->weapondata[i];
		if (!pCurrent)
		{
			memset(pto, 0, sizeof(weapon_data_t));
			continue;
		}

		pto->m_fInReload					= pCurrent->m_fInReload;
		pto->m_fInSpecialReload			= pCurrent->m_fInSpecialReload;
		pto->m_flPumpTime				= pCurrent->m_flPumpTime;
		pto->m_iClip						= pCurrent->m_iClip; 
		pto->m_flNextPrimaryAttack		= pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack		= pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle			= pCurrent->m_flTimeWeaponIdle;
		pto->fuser1						= pCurrent->pev->fuser1;
		/* XDM pto->fuser2						= pCurrent->m_flStartThrow;
		pto->fuser3						= pCurrent->m_flReleaseThrow;
		pto->iuser1						= pCurrent->m_chargeReady;
		pto->iuser2						= pCurrent->m_fInAttack;
		pto->iuser3						= pCurrent->m_fireState;*/
		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload				-= cmd->msec / 1000.0f;
		pto->m_fNextAimBonus				-= cmd->msec / 1000.0f;
		pto->m_flNextPrimaryAttack		-= cmd->msec / 1000.0f;
		pto->m_flNextSecondaryAttack		-= cmd->msec / 1000.0f;
		pto->m_flTimeWeaponIdle			-= cmd->msec / 1000.0f;
		pto->fuser1						-= cmd->msec / 1000.0f;

		to->client.vuser3[2]			= pCurrent->PrimaryAmmoIndex();
		to->client.vuser4[0]			= pCurrent->SecondaryAmmoIndex();
		to->client.vuser4[1]			= g_ClientPlayer.AmmoInventory(pCurrent->PrimaryAmmoIndex());
		to->client.vuser4[2]			= g_ClientPlayer.AmmoInventory(pCurrent->SecondaryAmmoIndex());

		if (pto->m_flPumpTime != -9999)
		{
			pto->m_flPumpTime -= cmd->msec / 1000.0f;
			if (pto->m_flPumpTime < -0.001)
				pto->m_flPumpTime = -0.001;
		}

		if (pto->m_fNextAimBonus < -1.0)
			pto->m_fNextAimBonus = -1.0;

		if (pto->m_flNextPrimaryAttack < -1.0)
			pto->m_flNextPrimaryAttack = -1.0;

		if (pto->m_flNextSecondaryAttack < -0.001)
			pto->m_flNextSecondaryAttack = -0.001;

		if (pto->m_flTimeWeaponIdle < -0.001)
			pto->m_flTimeWeaponIdle = -0.001;

		if (pto->m_flNextReload < -0.001)
			pto->m_flNextReload = -0.001;

		if (pto->fuser1 < -0.001)
			pto->fuser1 = -0.001;
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0f;
	if (to->client.m_flNextAttack < -0.001)
		to->client.m_flNextAttack = -0.001;

	to->client.fuser2 -= cmd->msec / 1000.0f;
	if (to->client.fuser2 < -0.001)
		to->client.fuser2 = -0.001;

	to->client.fuser3 -= cmd->msec / 1000.0f;
	if (to->client.fuser3 < -0.001)
		to->client.fuser3 = -0.001;

	// Store off the last position from the predicted state.
	//HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void CL_DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
//	RecClPostRunCmd(from, to, cmd, runfuncs, time, random_seed);
	g_runfuncs = (runfuncs > 0);

	//int nWeapons = UpdateLocalInventory(to->weapondata); DO NOT USE
	/*weapon_data_t *pData = &from->weapondata[MAX_WEAPONS-1];
	if (nWeapons != pData->iuser1)//ASSERT(dbg_nwpns == nWeapons);
	{
/*#if defined(_DEBUG)
		uint32 sv_wpnbits = (uint32)pData->iuser2;
		conprintf(0, "HUD_PostRunCmd() ERROR: weapons out of sync! Server sent %d weapons while client got %d! Bits: sv from: %X, sv to: %X, sv ck: %X, cl: %X\n", pData->iuser1, nWeapons, from->client.weapons & ~(1<<WEAPON_SUIT), to->client.weapons & ~(1<<WEAPON_SUIT), sv_wpnbits, cl_wpnbits);
		for (iID = WEAPON_NONE+1; iID < MAX_WEAPONS; ++iID)
		{
			if (((sv_wpnbits & (1 << iID)) != 0) && ((cl_wpnbits & (1 << iID)) == 0))
				conprintf(0, "missing %d, ", iID);
		}
		conprintf(0, "\n");
#else* /
		conprintf(0, "HUD_PostRunCmd() ERROR: weapons out of sync! Server sent %d weapons while client got %d! Bits: sv from: %X, sv to: %X, cl: %X\n", pData->iuser1, nWeapons, from->client.weapons & ~(1<<WEAPON_SUIT), to->client.weapons & ~(1<<WEAPON_SUIT), cl_wpnbits);
//#endif
	}*/

	//gHUD.m_Ammo.UpdateCurrentWeapon(from->client.m_iId);? we use entity_state_s::m_iId

	/*if (g_irunninggausspred == 1)
	{
		Vector forward;
		gEngfuncs.pfnAngleVectors(v_angles, forward, NULL, NULL);
		to->client.velocity = to->client.velocity - forward * g_flApplyVel * 5; 
		g_irunninggausspred = false;
	}*/

#if defined(CLIENT_WEAPONS)
	if (g_pCvarLW && g_pCvarLW->value > 0)
	{
		HUD_WeaponsPostThink(from, to, cmd, time, random_seed);
	}
	else
#endif
	{
		to->client.fov = g_lastFOV;
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
	gHUD.m_iRandomSeed = random_seed;// XDM3037
#if defined (_DEBUG_ANGLES)
	DBG_ANGLES_DRAW(12, from->playerstate.origin, from->playerstate.angles, from->playerstate.number, "PostRunCmd() from");
	DBG_ANGLES_DRAW(13, to->playerstate.origin, to->playerstate.angles, to->playerstate.number, "PostRunCmd() to");
#endif
}
