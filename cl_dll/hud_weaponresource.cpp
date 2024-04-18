//====================================================================
//
// Purpose: WeaponsResource class implementation
// It holds actual client inventory data and the global weapon type database
//
//====================================================================
#include "hud.h"
#include "cl_util.h"
#include "ammohistory.h"

// XDM: we don't use ammo sprites from weapon_*.txt! Ammo types are loaded from a separate file and associate by name (uranium, grenades, etc.)
// XDM3037: replaces (HUD_WEAPON *)1
HUD_WEAPON hwNoSelection =
{
	WEAPON_NONE,// iId;
	0,		// iFlags;
	WEAPON_NOCLIP,// iMaxClip;
	0,		// iSlot;
	0,		// iSlotPos;
	-1,		// iAmmoType
	-1,		// iAmmo2Type
#if defined (OLD_WEAPON_AMMO_INFO)
	-1,		// iMax1
	-1,		// iMax2
#endif
	0,		// iClip
	0,		// iState
	0,		// iZoomMode
	0,		// iPriority
	"",		// szName
	0, {0,0,0,0},// hActive, rcActive
	0, {0,0,0,0},// hInactive, rcInactive
#if defined (OLD_WEAPON_AMMO_INFO)
	0, {0,0,0,0},
	0, {0,0,0,0},
#endif
	0, {0,0,0,0},// hCrosshair, rcCrosshair
	0, {0,0,0,0},// hAutoaim, rcAutoaim
	0, {0,0,0,0},// hZoomedCrosshair, rcZoomedCrosshair
	0, {0,0,0,0}// hZoomedAutoaim, rcZoomedAutoaim
};

HUD_WEAPON *g_pwNoSelection = &hwNoSelection;// A valid pointer!
//WeaponsResource gWR;// XDM3038a: moved to CHUDAmmo

//-----------------------------------------------------------------------------
// Purpose: Allocate memory and invalidate data
//-----------------------------------------------------------------------------
void WeaponsResource::Init(void)
{
	memset(m_rgWeapons, 0, sizeof(m_rgWeapons));
	Reset();
	for (size_t i=0; i<MAX_WEAPONS; ++i)// XDM
	{
		m_rgWeapons[i].iPriority = MAX_WEAPONS;
		m_rgWeapons[i].iAmmoType = -1;
		m_rgWeapons[i].iAmmo2Type = -1;
#if defined (OLD_WEAPON_AMMO_INFO)
		m_rgWeapons[i].iMax1 = -1;
		m_rgWeapons[i].iMax2 = -1;
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset data to defaults
//-----------------------------------------------------------------------------
void WeaponsResource::Reset(void)
{
	// don't reset m_rgWeapons database!
	memset(m_rgSlots, 0, sizeof(m_rgSlots));
	memset(m_riAmmo, 0, sizeof(m_riAmmo));
}

void WeaponsResource::LoadAllWeaponSprites(void)
{
	DBG_PRINTF("WeaponsResource::LoadAllWeaponSprites()\n");
	for (int i = 0; i < MAX_WEAPONS; ++i)// i != ID
	{
		if (m_rgWeapons[i].iId != WEAPON_NONE)// this weapon actually exists
			LoadWeaponSprites(&m_rgWeapons[i]);
	}
}

void WeaponsResource::LoadWeaponSprites(HUD_WEAPON *pWeapon)
{
	ASSERT(pWeapon != NULL);
	if (pWeapon == NULL)
		return;

	DBG_PRINTF("WeaponsResource::LoadWeaponSprites(%d)\n", pWeapon->iId);
	int i;
	char sz[64];
	memset(&pWeapon->rcActive, 0, sizeof(wrect_t));
	memset(&pWeapon->rcInactive, 0, sizeof(wrect_t));
#if defined (OLD_WEAPON_AMMO_INFO)
	memset(&pWeapon->rcAmmo, 0, sizeof(wrect_t));
	memset(&pWeapon->rcAmmo2, 0, sizeof(wrect_t));
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;
#endif
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;

	_snprintf(sz, 64, "sprites/%s.txt", pWeapon->szName);
	client_sprite_t *pList = SPR_GetList(sz, &i);

	if (pList == NULL)// XDM
	{
		conprintf(1, "WeaponsResource: loading default sprite set for %s %d\n", pWeapon->szName, pWeapon->iId);
		pList = SPR_GetList("sprites/weapon_unknown.txt", &i);
	}

	if (pList == NULL)
		return;

	client_sprite_t *p;
	p = GetSpriteList(pList, "crosshair", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hCrosshair = SPR_Load(sz);
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = NULL;

	p = GetSpriteList(pList, "autoaim", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hAutoaim = SPR_Load(sz);
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList(pList, "zoom", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedCrosshair = SPR_Load(sz);
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; //default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList(pList, "zoom_autoaim", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedAutoaim = SPR_Load(sz);
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair;  //default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList(pList, "weapon", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hInactive = SPR_Load(sz);
		pWeapon->rcInactive = p->rc;
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

#if defined (OLD_WEAPON_AMMO_INFO)
	p = GetSpriteList(pList, "ammo", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo = SPR_Load(sz);
		pWeapon->rcAmmo = p->rc;
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", gHUD.m_iRes, i);
	if (p)
	{
		_snprintf(sz, 64, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo2 = SPR_Load(sz);
		pWeapon->rcAmmo2 = p->rc;
	}
	else
		pWeapon->hAmmo2 = 0;
#endif
}

// Add weapon INFO into database
void WeaponsResource::AddWeapon(HUD_WEAPON *pWeapon)
{ 
	ASSERT(pWeapon != NULL);
	if (pWeapon)
	{
		DBG_PRINTF("WeaponsResource::AddWeapon(%d)\n", pWeapon->iId);
		m_rgWeapons[pWeapon->iId] = *pWeapon;//memcpy(&m_rgWeapons[pWeapon->iId], pWeapon, sizeof(HUD_WEAPON));// XDM3038a: ?
		LoadWeaponSprites(&m_rgWeapons[pWeapon->iId]);
	}
}

// Pickup an instance of a weapon
void WeaponsResource::PickupWeapon(HUD_WEAPON *pWeapon)
{
	ASSERT(pWeapon != NULL);
	if (pWeapon)
	{
		DBG_PRINTF("WeaponsResource::PickupWeapon(%d)\n", pWeapon->iId);
		m_rgSlots[pWeapon->iSlot][pWeapon->iSlotPos] = pWeapon;
	}
}

void WeaponsResource::DropWeapon(HUD_WEAPON *pWeapon)
{
	ASSERT(pWeapon != NULL);
	if (pWeapon)
	{
		DBG_PRINTF("WeaponsResource::DropWeapon(%d)\n", pWeapon->iId);
		m_rgSlots[pWeapon->iSlot][pWeapon->iSlotPos] = NULL;
	}
}

// Returns place even for NON-EXISTING weapons!
// NOT an inventory check!
HUD_WEAPON *WeaponsResource::GetWeaponStruct(const int iId)
{
	if (iId > WEAPON_NONE && ASSERT(iId < MAX_WEAPONS))// only check array bounds
	{
		// DO NOT CHECK HERE ASSERT(m_rgWeapons[iId].iId == iId);
		return &m_rgWeapons[iId];
	}
	return NULL;
}

// Returns the first weapon for a given slot.
HUD_WEAPON *WeaponsResource::GetFirstPos(uint16 iSlot)
{
	HUD_WEAPON *pret = NULL;
	for (uint16 i = 0; i < MAX_WEAPON_POSITIONS; ++i)
	{
		if (m_rgSlots[iSlot][i] && IsSelectable(m_rgSlots[iSlot][i]))// was HasAmmo
		{
			pret = m_rgSlots[iSlot][i];
			break;
		}
	}
	return pret;
}

HUD_WEAPON *WeaponsResource::GetNextActivePos(uint16 iSlot, uint16 iSlotPos)
{
	if (iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS)
		return NULL;

	HUD_WEAPON *p = m_rgSlots[iSlot][iSlotPos+1];
	if (p == NULL || !IsSelectable(p))// was !gWR.HasAmmo(p))
		return GetNextActivePos(iSlot, iSlotPos+1);

	return p;
}


#if defined (OLD_WEAPON_AMMO_INFO)
// Helper function to return a Ammo pointer from id
HLSPRITE *WeaponsResource::GetAmmoPicFromWeapon(const int &iAmmoId, wrect_t &rect)
{
	for (short i = 1; i < MAX_WEAPONS; ++i)// XDM: start from 1 to ignore WEAPON_NONE
	{
		//conprintf(1, "m_rgWeapons[%d].iAmmoType = %d\n", i, m_rgWeapons[i].iAmmoType);
		if (m_rgWeapons[i].iAmmoType == iAmmoId)
		{
			//conprintf(1, "GetAmmoPicFromWeapon found m_rgWeapons[%d].iAmmoType = %d\n", i, m_rgWeapons[i].iAmmoType);
			rect = m_rgWeapons[i].rcAmmo;
			return &m_rgWeapons[i].hAmmo;
		}
		else if (m_rgWeapons[i].iAmmo2Type == iAmmoId)
		{
			rect = m_rgWeapons[i].rcAmmo2;
			return &m_rgWeapons[i].hAmmo2;
		}
	}
	//conprintf(1, "GetAmmoPicFromWeapon failed!\n");
	return NULL;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Update ammo count
// Input  : iId - 
//			iCount -
//-----------------------------------------------------------------------------
void WeaponsResource::SetAmmo(const int iId, short iCount)
{
//	DBG_PRINTF("WeaponsResource::SetAmmo(%d, %d)\n", iId, iCount);
	if (iId >= 0 && iId < MAX_AMMO_SLOTS)
	{
		if (iCount == 0 && m_riAmmo[iId] > 0)// XDM3038a: the real thing
		{
			if (g_pCvarAmmoVoice->value > 0.0f && gHUD.PlayerIsAlive())
				PlaySoundSuit("!HEV_AMO0");
		}
		m_riAmmo[iId] = iCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inventory check
// Input  : iID - ammo ID
// Output : short - quantity
//-----------------------------------------------------------------------------
short WeaponsResource::CountAmmo(const int iId) 
{ 
	if (iId >= 0 && iId < MAX_AMMO_SLOTS)
		return m_riAmmo[iId];

	return 0;// -1 ?
}

//-----------------------------------------------------------------------------
// Purpose: Reliably check if there is at least one usable weapon
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WeaponsResource::HasUsableWeapons(void)
{
	HUD_WEAPON *pHUDWeapon;
	for (size_t iItemID = WEAPON_NONE+1; iItemID < MAX_WEAPONS; ++iItemID)
	{
		pHUDWeapon = GetWeaponStruct(iItemID);
		if (pHUDWeapon)//if (HasWeapon(iItemID)) too slow
		{
			if (pHUDWeapon->iState != wstate_undefined && pHUDWeapon->iState != wstate_unusable)
			{
				if (pHUDWeapon->iState != wstate_error)
				{
					return true;
					break;
				}
				else
					conprintf(1, "CL: Weapon %d has wstate_error!\n", iItemID);
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Inventory check by server weapon bits (fast)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WeaponsResource::HasWeapons(void)
{
/*#ifdef USE_WEAPONBITS
	return FBitExclude(gHUD.m_iWeaponBits, (1<<WEAPON_SUIT));
#else*/
	for (size_t iItemID = WEAPON_NONE+1; iItemID < MAX_WEAPONS; ++iItemID)
	{
		// WRONG! if (iItemID != WEAPON_SUIT && m_rgWeapons[iItemID].iId == iItemID)// exists
		if (HasWeapon(iItemID))
		{
			return true;
			break;
		}
	}
	return false;
//#endif
}

//-----------------------------------------------------------------------------
// Purpose: Inventory check by real client inventory data
// Input  : iItemID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WeaponsResource::HasWeapon(const int &iItemID)
{
	if (iItemID > WEAPON_NONE && iItemID < MAX_WEAPONS)
	{
		if (m_rgWeapons[iItemID].iId == iItemID)
			return (m_rgSlots[m_rgWeapons[iItemID].iSlot][m_rgWeapons[iItemID].iSlotPos] == &m_rgWeapons[iItemID]);
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Does this weapon have any ammo (or does it not use ammo at all)
// Input  : pWeapon - HUD_WEAPON
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WeaponsResource::HasAmmo(HUD_WEAPON *pWeapon)
{
	if (pWeapon == NULL)
		return false;

#if defined (OLD_WEAPON_AMMO_INFO)
	if (p->iMax1 == -1)// weapons with no max ammo can always be selected
#else
	if (gHUD.m_Ammo.MaxAmmoCarry(pWeapon->iAmmoType) == -1)
#endif
		return true;

	return ((pWeapon->iAmmoType == -1) || (pWeapon->iClip > 0) || CountAmmo(pWeapon->iAmmoType) || CountAmmo(pWeapon->iAmmo2Type));// NOT HERE! || (pWeapon->iFlags & WEAPON_FLAGS_SELECTONEMPTY);
}

//-----------------------------------------------------------------------------
// Purpose: Decides if this weapon is selectable by any means possible
// Input  : pWeapon - HUD_WEAPON
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WeaponsResource::IsSelectable(HUD_WEAPON *pWeapon)
{
	if (pWeapon == NULL)
		return false;

	if (pWeapon->iState == wstate_unusable)// XDM3038a
	{
		//DBG_PRINTF("WeaponsResource::IsSelectable(%d): wstate_unusable\n", pWeapon->iId);
		return false;
	}
	else if (pWeapon->iState == wstate_error)// XDM3038c
	{
		DBG_PRINTF("WeaponsResource::IsSelectable(%d): wstate_error!\n", pWeapon->iId);
		return false;
	}
	if (pWeapon->iFlags & WEAPON_FLAGS_SELECTONEMPTY)
	{
		//DBG_PRINTF("WeaponsResource::IsSelectable(%d): WEAPON_FLAGS_SELECTONEMPTY\n", pWeapon->iId);
		return true;
	}
	return HasAmmo(pWeapon);
}
