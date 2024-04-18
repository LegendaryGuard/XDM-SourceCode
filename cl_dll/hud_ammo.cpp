// Oh come on! There's too much new code to make it look valve's!
#include "hud.h"
#include <ctype.h>
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"
#include "ammohistory.h"

int g_weaponselect = WEAPON_NONE;// external variable for input.cpp

//DECLARE_MESSAGE(m_Ammo, CurWeapon);// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, WeaponList);// new weapon type
DECLARE_MESSAGE(m_Ammo, AmmoList);// XDM3037: ammo type registry
// OBSOLETE DECLARE_MESSAGE(m_Ammo, AmmoX);// update known ammo type's count
// OBSOLETE DECLARE_MESSAGE(m_Ammo, AmmoPickup);// flashes an ammo pickup record
//DECLARE_MESSAGE(m_Ammo, WeapPickup);// flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);
DECLARE_MESSAGE(m_Ammo, UpdWeapons);// XDM: All weapons and clip
DECLARE_MESSAGE(m_Ammo, UpdAmmo);// XDM: All ammo
#if !defined (SERVER_WEAPON_SLOTS)
DECLARE_MESSAGE(m_Ammo, SelBestItem);// XDM3037
#endif

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
//DECLARE_COMMAND(m_Ammo, Slot);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, SelectWeapon);// XDM3038
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);
DECLARE_COMMAND(m_Ammo, InvUp);
DECLARE_COMMAND(m_Ammo, InvDown);
DECLARE_COMMAND(m_Ammo, InvLast);// XDM3038
DECLARE_COMMAND(m_Ammo, Holster);// XDM3038
//#if defined (_DEBUG)
DECLARE_COMMAND(m_Ammo, DumpInventory);// XDM3038c
//#endif


//-----------------------------------------------------------------------------
// Purpose: Init
// Output : int
//-----------------------------------------------------------------------------
int CHudAmmo::Init(void)
{
	m_pWeapon = NULL;
	//m_pLastWeapon = NULL;// XDM3038

	gHUD.AddHudElem(this);

	//HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(WeaponList);
	HOOK_MESSAGE(AmmoList);
	// OBSOLETE	HOOK_MESSAGE(AmmoX);
	// OBSOLETE	HOOK_MESSAGE(AmmoPickup);
	//HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(UpdWeapons);// XDM3035
	HOOK_MESSAGE(UpdAmmo);
#if !defined (SERVER_WEAPON_SLOTS)
	HOOK_MESSAGE(SelBestItem);// XDM3037
#endif

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	// XDM3038 the right way that noone needs anyway :(	HOOK_COMMAND("slot", Slot);
	HOOK_COMMAND("_sw", SelectWeapon);// XDM3038
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);
	HOOK_COMMAND("invup", InvUp);// XDM
	HOOK_COMMAND("invdown", InvDown);
	HOOK_COMMAND("lastinv", InvLast);// XDM3038
	HOOK_COMMAND("holster", Holster);// XDM3038
//#if defined (_DEBUG)
	HOOK_COMMAND("dbg_dumpinventory", DumpInventory);// XDM3038c
//#endif

	m_pCvarDrawHistoryTime = CVAR_CREATE("hud_drawhistory_time", "5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarDrawAccuracy = CVAR_CREATE("hud_drawaccuracy", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3038c
	m_pCvarFastSwitch = CVAR_CREATE("hud_fastswitch", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// controls whether or not weapons can be selected in one keypress
	m_pCvarSwitchOnPickup = CVAR_CREATE("hud_switchonpickup", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// 0-no, 1-if better, 2-always
	m_pCvarWeaponSelectionTime = CVAR_CREATE("hud_weaponselection_time", "5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	if (m_pCvarWeaponSelectionTime)
		m_pCvarWeaponSelectionTime->value = HUD_WEAPON_SELECTION_TIME;

	// XDM3037: moved these two here, before .Reset(); and InitWeaponSlots() are called
	gWR.Init();
	gHR.Init();

	Reset();

#if defined (SERVER_WEAPON_SLOTS)
	m_pCvarWeaponPriority = NULL;
	m_pCvarWeaponSlots = NULL;
#else
	// Warning: InitWeaponSlots() should've been called
	m_pCvarWeaponPriority = CVAR_CREATE("hud_weapon_priority", m_szWeaponPriorityConfig, FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarWeaponSlots = CVAR_CREATE("hud_weapon_slots", m_szWeaponSlotConfig, FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
#endif

	SetActive(true);//!!!
	return 1;
};

//-----------------------------------------------------------------------------
// Purpose: VidInit
// Output : int
//-----------------------------------------------------------------------------
int CHudAmmo::VidInit(void)
{
	m_iAmmoTypes = 0;// XDM3037
	memset(m_AmmoInfoArray, 0, sizeof(HUD_AMMO)*MAX_AMMO_SLOTS);// XDM3037

	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex("bucket1");
	//ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	if (m_HUD_bucket0 != HUDSPRITEINDEX_INVALID)
	{
		m_iBucketWidth = RectWidth(gHUD.GetSpriteRect(m_HUD_bucket0));
		m_iBucketHeight = RectHeight(gHUD.GetSpriteRect(m_HUD_bucket0));
	}
	else
	{
		m_iBucketWidth = 8;
		m_iBucketHeight = 8;
	}
	m_HUD_selection = gHUD.GetSpriteIndex("selection");
	//bad	m_iABWidth = XRES(16);
	//	m_iABHeight = YRES(2);
	// 170*45; 170*x=20; x=20/170; x~=0.12;// 45*y=4; y~=0.08
	if (m_HUD_selection != HUDSPRITEINDEX_INVALID)
	{
		m_iABWidth  = (int)((float)RectWidth(gHUD.GetSpriteRect(m_HUD_selection))*0.15f);
		m_iABHeight = (int)((float)RectHeight(gHUD.GetSpriteRect(m_HUD_selection))*0.1f);
	}
	else
	{
		m_iABWidth = 8;
		m_iABHeight = 8;
	}
	m_HUD_bucket_flat = gHUD.GetSpriteIndex("bucketf");// XDM3037
	m_iHUDSpriteSpread = gHUD.GetSpriteIndex("shootspread");// XDM3038c
	if (m_iHUDSpriteSpread != HUDSPRITEINDEX_INVALID)// XDM3038c
		m_rcSpread = gHUD.GetSpriteRect(m_iHUDSpriteSpread);
	else
		m_rcSpread = nullrc;

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: InitHUDData
//-----------------------------------------------------------------------------
void CHudAmmo::InitHUDData(void)
{
	//Reset();// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Reset
//-----------------------------------------------------------------------------
void CHudAmmo::Reset(void)
{
	m_fFade = MIN_ALPHA;
	SetActive(true);//!!!

	m_pActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	m_iOldWeaponBits = 0;// XDM3038c
	m_iActiveWeapon = WEAPON_NONE;// XDM3038
	m_iPrevActiveWeapon = WEAPON_NONE;// XDM3038
	m_iRecentPickedWeapon = WEAPON_NONE;// XDM3035a
	m_fSelectorShowTime = 0.0f;// XDM3037

	//allow?	if (m_pCvarWeaponSelectionTime && m_pCvarWeaponSelectionTime->value == 0)
	//		CVAR_SET_FLOAT m_pCvarWeaponSelectionTime->value = HUD_WEAPON_SELECTION_TIME;

	gWR.Reset();
	gHR.Reset();

#if !defined (SERVER_WEAPON_SLOTS)
	InitWeaponSlots();// XDM3035b
#endif
	//VidInit();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035b: Init Weapon Slots and Priority from user cvars
// Warning: May be called before cvars are created!
// Output : int flags: 1-custom slots, 2-custom priorities
//-----------------------------------------------------------------------------
int CHudAmmo::InitWeaponSlots(void)
{
//#if (MAX_WEAPONS > 32)
//#error WARNING: This code probably needs fixing.
//#endif
	int iRet = 0;
#if !defined(SERVER_WEAPON_SLOTS)
	// Default value
	if (m_pCvarWeaponSlots == NULL || m_pCvarWeaponSlots->string == NULL || strlen(m_pCvarWeaponSlots->string) < 3)
	{
		conprintf(1, "CHudAmmo::InitWeaponSlots(): generating default slot list\n");
		_snprintf(m_szWeaponSlotConfig, 128, "%d,%d; %d,%d,%d; %d,%d,%d,%d; %d,%d,%d,%d; %d,%d,%d,%d; %d,%d,%d,%d,%d; %d,%d,%d; %d,%d,%d,%d,%d;\0",
			WEAPON_CROWBAR,		WEAPON_SWORD,
			WEAPON_GLOCK,		WEAPON_PYTHON,		WEAPON_CHEMGUN,
			WEAPON_MP5,			WEAPON_SHOTGUN,		WEAPON_CROSSBOW,	WEAPON_SNIPERRIFLE,
			WEAPON_RPG,			WEAPON_GLAUNCHER,	WEAPON_ALAUNCHER,	WEAPON_DLAUNCHER,
			WEAPON_GAUSS,		WEAPON_EGON,		WEAPON_PLASMA,		WEAPON_FLAME,
			WEAPON_HANDGRENADE,	WEAPON_SATCHEL,		WEAPON_TRIPMINE,	WEAPON_SNARK,		WEAPON_STRTARGET,
			WEAPON_DISPLACER,	WEAPON_BEAMRIFLE,	WEAPON_BHG,			WEAPON_HORNETGUN,
			WEAPON_CUSTOM1,		WEAPON_CUSTOM2,		WEAPON_UNUSED1,		WEAPON_UNUSED2);

		if (m_pCvarWeaponSlots)
		{
#if defined (CLDLL_NEWFUNCTIONS)
			if (gEngfuncs.Cvar_Set)
				gEngfuncs.Cvar_Set(m_pCvarWeaponSlots->name, m_szWeaponSlotConfig);
#else
			char cmd[1024];
			_snprintf(cmd, 1024, "%s \"%s\"\n", m_pCvarWeaponSlots->name, m_szWeaponSlotConfig);
			CLIENT_COMMAND(cmd);// XDM3037a HACK!
	//NO! Who (de)allocates this???		m_pCvarWeaponSlots->string = m_szWeaponSlotConfig;
#endif
		}
	}
	else// custom value specified
	{
		strcpy(m_szWeaponSlotConfig, m_pCvarWeaponSlots->string);
		conprintf(1, "CHudAmmo::InitWeaponSlots(): using custom slot list\n");
		iRet |= 1;
	}

	// Default value
	if (m_pCvarWeaponPriority == NULL || m_pCvarWeaponPriority->string == NULL || strlen(m_pCvarWeaponPriority->string) < 3)
	{
		conprintf(1, "CHudAmmo::InitWeaponSlots(): generating default priority list\n");
		// priority: most desired go first
		_snprintf(m_szWeaponPriorityConfig, 128, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\0",
			WEAPON_BHG,			WEAPON_DISPLACER,	WEAPON_BEAMRIFLE,
			WEAPON_GAUSS,		WEAPON_EGON,		WEAPON_PLASMA,		WEAPON_FLAME,
			WEAPON_RPG,			WEAPON_GLAUNCHER,	WEAPON_ALAUNCHER,	WEAPON_DLAUNCHER,
			WEAPON_MP5,			WEAPON_SNIPERRIFLE,	WEAPON_SHOTGUN,		WEAPON_CROSSBOW,	WEAPON_HORNETGUN,
			WEAPON_CHEMGUN,		WEAPON_PYTHON,		WEAPON_GLOCK,
			WEAPON_HANDGRENADE,	WEAPON_SNARK,		WEAPON_STRTARGET,	WEAPON_SATCHEL,		WEAPON_TRIPMINE,
			WEAPON_SWORD,		WEAPON_CROWBAR,
			WEAPON_CUSTOM1,		WEAPON_CUSTOM2,
			WEAPON_UNUSED1,		WEAPON_UNUSED2,		WEAPON_SUIT,		WEAPON_NONE);

		if (m_pCvarWeaponPriority)
		{
#if defined (CLDLL_NEWFUNCTIONS)
			if (gEngfuncs.Cvar_Set)
				gEngfuncs.Cvar_Set(m_pCvarWeaponPriority->name, m_szWeaponPriorityConfig);
#else
			char cmd[1024];
			_snprintf(cmd, 1024, "%s \"%s\"\n", m_pCvarWeaponPriority->name, m_szWeaponPriorityConfig);
			CLIENT_COMMAND(cmd);// XDM3037a HACK!
//NO! Who (de)allocates this???		m_pCvarWeaponPriority->string = m_szWeaponPriorityConfig;
#endif
		}
	}
	else// custom value specified
	{
		strcpy(m_szWeaponPriorityConfig, m_pCvarWeaponPriority->string);
		// Read the string into linked list
		HUD_WEAPON *pW;
		char *pData = m_szWeaponPriorityConfig;
		//char *token = NULL;
		short pos = 0;
		int id;
		// DON'T USE THIS SHIT!	while ((token = strtok(startstring, " ")) != NULL) it replaces delimiters with \0!
		while ((pData = gEngfuncs.COM_ParseFile(pData, m_szToken)) != NULL)
		{
			++pos;// start from 1
			//startstring = NULL;
			id = atoi(m_szToken);
			//ok conprintf(0, " # %d token %d '%s'\n", pos, id, m_szToken);

			pW = gWR.GetWeaponStruct(id);
			if (pW)
				pW->iPriority = pos;
		}
		conprintf(1, "CHudAmmo::InitWeaponSlots(): using custom priority list\n");
		iRet |= 2;
	}
#endif // !defined(SERVER_WEAPON_SLOTS)
	return iRet;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Detect inventory changes by weapon bits
// Callers: May get called by HUD_TxferLocalOverrides
// Input  : from gHUD.m_iWeaponBits
//-----------------------------------------------------------------------------
void CHudAmmo::UpdateWeaponBits(void)
{
	if (gHUD.m_iWeaponBits != m_iOldWeaponBits)// WARNING: this may not be true when you think it should!
	{
		uint32 newbit;
		for (int wid = WEAPON_NONE+1; wid < MAX_WEAPONS; ++wid)
		{
			newbit = (gHUD.m_iWeaponBits & (1<<wid));
			if (newbit != (m_iOldWeaponBits & (1<<wid)))// bit differs
			{
				if (newbit)// means oldbit was 0
					WeaponPickup(wid);
				else// means oldbit was 1
					WeaponRemove(wid);
			}
		}
		m_iOldWeaponBits = gHUD.m_iWeaponBits;
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: Assign current weapon by ID
// Callers: May get called by HUD_TxferLocalOverrides and MsgFunc_UpdWeapons
// Input  : &iId - iID 4 -> 0 -> 12 -> 0 -> 1
//-----------------------------------------------------------------------------
void CHudAmmo::UpdateCurrentWeapon(const int &iID)
{
	if (iID != m_iActiveWeapon)
		OnWeaponChanged(iID);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038b: player has selected a different weapon
// Callers: UpdateCurrentWeapon
// Input  : &iId - iID 4 -> 0 -> 12 -> 0 -> 1
//-----------------------------------------------------------------------------
void CHudAmmo::OnWeaponChanged(const int &iID)// XDM3038b
{
	if (m_iActiveWeapon != WEAPON_NONE)
		m_iPrevActiveWeapon = m_iActiveWeapon;// don't check for ammo/usability as player may pick up some soon

	m_iActiveWeapon = iID;
	m_pWeapon = gWR.GetWeaponStruct(iID);// safe

	if (iID == WEAPON_NONE)
	{
		UpdateCrosshair(CROSSHAIR_OFF);// XDM3038: nice way to do things
		m_fFade = MIN_ALPHA;
	}
	else
	{
		UpdateCrosshair(CROSSHAIR_NORMAL);
		m_fFade = MAX_ALPHA;
	}

	/*TODO: UNDONE: load v_ model by name without precaching?
	if (gHUD.m_pLocalPlayer && gHUD.m_pLocalPlayer->curstate.weaponmodel != 0)
	{
		cl_entity_t *pViewModel = gEngfuncs.GetViewModel();
		if (pViewModel)
		{
			model_t *pWeaponModel = IEngineStudio.GetModelByIndex(gHUD.m_pLocalPlayer->curstate.weaponmodel);
			if (pWeaponModel)
			{
				char szViewModel[MAX_MODEL_NAME];
				strncpy(szViewModel, pWeaponModel->name, MAX_MODEL_NAME);
				char *pStart = strrchr(szViewModel, '/')
				if (pStart == NULL)// find last, name may contain no delimiters at all
					pStart = szViewModel;

				if (pStart[1] == 'p' && pStart[2] == '_')
				{
					pStart[1] = 'v';
						pViewModel->model = IEngineStudio.Mod_ForName(szViewModel, 0);// XDM3038b
				}
			}
		}
	}*/
}

//-----------------------------------------------------------------------------
// "1,19; 2,3,5; 4,7,6; 8,17,18; 9,10,22,23; 12,14,13,15,21,20; 24,25,26,11; 29,30"
// Purpose: Parse weapon slot config string and find slot and position for ID
// Note   : Slow, do not call every frame
// Input  : &iId - WEAPON_CROWBAR
//			&wslot - 
//			&wpos - 
// Output : int 1 = success
//-----------------------------------------------------------------------------
int CHudAmmo::GetWeaponSlotPos(const int &iId, short &wslot, short &wpos)
{
	//char *string = m_pCvarWeaponSlots->string;
	size_t len = strlen(m_szWeaponSlotConfig);
	char *c = m_szWeaponSlotConfig;

	byte slot = 0;
	byte slotpos = 0;

	// 1,2,3;4,5,6;7,8,9;
	/*char *slotstring = string;
	char *slotposstring = NULL;
	while ((slotstring = strtok(slotstring, ";")) != NULL)// NO SPACES! STRTOK MODIFIES THE INPUT STING!!!!!!!!!!!!
	{
		slotpos = 0;
		conprintf(0, " # %d: slotstring: '%s'\n", slot, slotstring);
		slotposstring = strtok(slotstring, " ,");
		while (slotposstring)
		{
			conprintf(0, "## %d: slotposstring: '%s'\n", slotpos, slotposstring);
			++slotpos;
			slotposstring = strtok(NULL, " ,");
		}
		conprintf(0, "END slot %d\n", slot);
		++slot;
	}*/

	//conprintf(1, "CHudAmmo::GetWeaponSlotPos(%d)\n", iId);
	//int tokenlen;
	int current_id = 0;
	char *tokenstart = NULL;
	char swapchar;
	for (size_t i=0; i<=len; ++i)// XDM3037a: allow '\0'
	{
		if (isdigit(*c))
		{
			if (tokenstart == NULL)
			{
				tokenstart = c;
				//tokenlen = 0;
			}
		}
		else if (*c == ';' || *c == ',' || *c == '\0')// found separator, analyze previously found string
		{
			swapchar = *c;
			*c = NULL;// set as end of string
			if (tokenstart)
				current_id = atoi(tokenstart);
			else
			{
				current_id = 0;// WTF?
				conprintf(1, "CHudAmmo::GetWeaponSlotPos(%d) error: tokenstart is NULL!\n", iId);
			}
			*c = swapchar;// restore original!
			//conprintf(1, "#%d # %d: current_id: %d\n", slot, slotpos, current_id);

			if (current_id == iId)// found, write output
			{
				wslot = slot;
				wpos = slotpos;
				return 1;
			}

			if (*c == ';')// Next slot
			{
				++slot;
				slotpos = 0;
			}
			else// Next position
				++slotpos;

			tokenstart = NULL;
		}
		if (slot >= MAX_WEAPON_SLOTS)// XDM3037a: safety checks
			break;
		if (slotpos >= MAX_WEAPON_POSITIONS)
			break;
		c++;
	}
	conprintf(0, "CHudAmmo::GetWeaponSlotPos(%d) failed!\n", iId);
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: GetNextBestItem
// Input  : iCurrentID - 
// Output : int
//-----------------------------------------------------------------------------
int CHudAmmo::GetNextBestItem(const int iCurrentID)
{
	/*if (!gWR.HasWeapons())
	{
		conprintf(1, "CHudAmmo::GetNextBestItem(%d): error: no weapons!\n", iCurrentID);
		return WEAPON_NONE;
	}*/
	int iBestPriority = MAX_WEAPONS;// lower the better, set to max initially
	int iBestID = WEAPON_NONE;// output
	HUD_WEAPON *pW;
	HUD_WEAPON *pCurrent = (iCurrentID>WEAPON_NONE)?gWR.GetWeaponStruct(iCurrentID):NULL;
	for (int i=1; i<MAX_WEAPONS; ++i)
	{
		pW = gWR.GetWeaponStruct(i);
		if (pW)
		{
			// XDM3038a: There's now a substitution to server's IsUseable(): wstate_unusable
			if (/* inside pW->iId != WEAPON_NONE && */pW != pCurrent && gWR.HasWeapon(pW->iId) && (pW->iPriority < iBestPriority) && gWR.IsSelectable(pW))// don't reselect the weapon we're trying to get rid of
			{
				iBestPriority = pW->iPriority;// if this weapon is useable, flag it as the best
				iBestID = i;
			}
		}
	}
	return iBestID;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: SelectItem (sends command to server)
// Warning: To prevent various recursions, the choosing and selection must be done in different frames
// Input  : iItemID - 
//-----------------------------------------------------------------------------
void CHudAmmo::SelectItem(const int iItemID)
{
	/*old	char cmd[8];// XDM3035
	_snprintf(cmd, 8, "_sw %d", iItemID);
	SERVER_COMMAND(cmd);*/
	if (iItemID >= WEAPON_NONE && iItemID < WEAPON_SUIT)// XDM3038: safety!
		g_weaponselect = iItemID;
}

//-----------------------------------------------------------------------------
// Purpose: ShouldSwitchWeapon after picking up iNewItemID
// Input  : iNewItemID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudAmmo::ShouldSwitchWeapon(const int iNewItemID)// XDM3037
{
#if !defined (SERVER_WEAPON_SLOTS)
	if (m_pWeapon == NULL)
		return true;

	int id;
	if (m_pCvarSwitchOnPickup)
	{
		id = (int)m_pCvarSwitchOnPickup->value;
		if (id == 0)
			return false;
		else if (id == 2)
			return true;
	}

	int iCurrentID = m_pWeapon->iId;
	int pos = 0;
	int pos_current = 0;
	int pos_new = 0;
	char *pData = m_szWeaponPriorityConfig;
	//char *token = NULL;
	//while ((token = strtok(startstring, " ")) != NULL)// destroys original string
	while ((pData = gEngfuncs.COM_ParseFile(pData, m_szToken)) != NULL)
	{
		++pos;// start from 1
		//startstring = NULL;
		id = atoi(m_szToken);
		//ok conprintf(0, " # %d id %d\n", pos, id);

		if (pos_current == 0 && id == iCurrentID)
			pos_current = pos;
		else if (pos_new == 0 && id == iNewItemID)
			pos_new = pos;

		if (pos_current != 0 && pos_new != 0)
		{
			if (pos_new < pos_current && !(gHUD.m_iKeyBits & (IN_ATTACK|IN_ATTACK2)))
				return true;
			else
				return false;
		}
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame. Used for selection of weapon menu item.
//-----------------------------------------------------------------------------
void CHudAmmo::Think(void)
{
	if (!gHUD.PlayerIsAlive())
		return;

	// XDM3038a: MsgFunc_SelBestItem was cached until we get some weapons AND info on them!
	//if (FBitExclude(gHUD.m_iWeaponBits, (1<<WEAPON_SUIT)) && m_iSelectNextBestItemCache != WEAPON_NONE)// suit is not a weapon!
	if (/*slow gWR.HasWeapons() && */m_iSelectNextBestItemCache != WEAPON_NONE)// suit is not a weapon!
	{
		//DBG_PRINTF("CHudAmmo::Think(): Using m_iSelectNextBestItemCache = %d\n", m_iSelectNextBestItemCache);
		int iBestID = GetNextBestItem(m_iSelectNextBestItemCache);
		//conprintf(1, "CHudAmmo::Think() %d iBestID %d\n", m_iSelectNextBestItemCache, iBestID);
		if (iBestID != WEAPON_NONE)
			SelectItem(iBestID);
		else if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f && gWR.HasUsableWeapons())
			conprintf(2, "CHudAmmo::Think() warning: GetNextBestItem(%d) == WEAPON_NONE!\n", m_iSelectNextBestItemCache);

		m_iSelectNextBestItemCache = WEAPON_NONE;
	}

	// XDM3035a: somehow the engine refuses to play sentence from the message code!
	if (m_iRecentPickedWeapon > 0)// and this should probably solve queuing problem when multiple weapons are picked up at the same time
	{
		if (gHUD.PlayerHasSuit() && (!IsMultiplayer() || g_pCvarPickupVoice->value > 0.0f))// XDM3034 && !g_SilentItemPickup XDM3038b: always in single
		{
			HUD_WEAPON *p = gWR.GetWeaponStruct(m_iRecentPickedWeapon);
			if (p)
			{
				if (p->szName && p->szName[0])
				{
					char sentence[CBSENTENCENAME_MAX+4];
					_snprintf(sentence, CBSENTENCENAME_MAX+4, "!W%s\0", p->szName+6);// skip "weapon"
					sentence[CBSENTENCENAME_MAX+3] = '\0';
					_strupr(sentence);// XDM3038b: was "!HEV_WEAPONNAME", now "!W_WEAPONNAME".
					PlaySoundSuit(sentence);
					// This stupid shit does not work from the pickup message handler!! I've spend two hours on figuring that out!
				}
				else
					conprintf(2, "CHudAmmo::Think() ERROR: HUD_WEAPON %d without szName!\n", m_iRecentPickedWeapon);
			}
		}
		m_iRecentPickedWeapon = 0;
	}

	if (m_iActiveWeapon == WEAPON_NONE)// XDM3038: works when changing map
	{
		if (m_pWeapon)
		{
			UpdateCrosshair(CROSSHAIR_OFF);
			m_pWeapon = NULL;// WARNING! NULL always happens during weapon switching!
		}
		//WEAPON *p = gWR.GetWeaponStruct(lol?);
		//if (p)
		//	gWR.DropWeapon(p);
	}

	if (m_pActiveSel)
	{
		if (gHUD.m_iKeyBits & IN_ATTACK)// has the player selected one?
		{
			if (m_pActiveSel != g_pwNoSelection)
				SelectItem(m_pActiveSel->iId);

			//unused	m_pLastSel = m_pActiveSel;
			m_pActiveSel = NULL;
			ClearBits(gHUD.m_iKeyBits, IN_ATTACK);
			PlaySound("common/wpn_select.wav", VOL_NORM);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set crosshair spite. Additional abstraction layer to track current sprite
// Input  : hspr - HLSPRITE
//			rc - rect
//			r,g,b - 
//-----------------------------------------------------------------------------
void CHudAmmo::SetCurrentCrosshair(HLSPRITE hspr, wrect_t &rc, int r, int g, int b)
{
	m_hCurrentCrosshair = hspr;
	m_CurrentCrosshairRect = rc;
	SetCrosshair(hspr, rc, r, g, b);
}

//-----------------------------------------------------------------------------
// Purpose: Set current crosshair + mode
// Warning: Uses m_pWeapon!
// Input  : mode - CROSSHAIR_NORMAL, can be used to force autoaim crosshair
//-----------------------------------------------------------------------------
void CHudAmmo::UpdateCrosshair(int mode)// XDM3038c
{
	if (mode == CROSSHAIR_OFF)
	{
		SetCurrentCrosshair(0, nullrc, 0, 0, 0);
	}
	else if (m_pWeapon)// XDM3035b: fix
	{
		ASSERTD(m_pWeapon->iState != wstate_undefined && m_pWeapon->iState != wstate_unusable && m_pWeapon->iState != wstate_error);
		if (gHUD.m_fFOV == DEFAULT_FOV || gHUD.m_fFOV == 0.0f)// XDM3037a: (zoomed == 0)// normal crosshairs
		{
			if ((m_pWeapon->iState == wstate_current_ontarget || mode == CROSSHAIR_AUTOAIM) && m_pWeapon->hAutoaim)
				SetCurrentCrosshair(m_pWeapon->hAutoaim, m_pWeapon->rcAutoaim, 255, 255, 255);
			else if (m_pWeapon->hCrosshair)
				SetCurrentCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255);
			else
				SetCurrentCrosshair(0, nullrc, 0, 0, 0);
		}
		else// zoomed crosshairs
		{
			if ((m_pWeapon->iState == wstate_current_ontarget || mode == CROSSHAIR_AUTOAIM) && m_pWeapon->hZoomedAutoaim)
				SetCurrentCrosshair(m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim, 255, 255, 255);
			else if (m_pWeapon->hZoomedCrosshair)
				SetCurrentCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 255, 255);
			else
				SetCurrentCrosshair(0, nullrc, 0, 0, 0);
		}
	}
}

#define HUD_SPREAD_RESET_RATE		2.0

//-------------------------------------------------------------------------
// Drawing code
// Input  : flTime - client time in seconds
// Output : int - Return 1 on success, 0 on failure.
//-------------------------------------------------------------------------
int CHudAmmo::Draw(const float &flTime)
{
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return 0;

	if (!gHUD.PlayerHasSuit())
		return 0;

	// Weapon/Ammo lists depend on this
	//m_iAmmoDisplayY = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight/2;// XDM3038: what if player picks up crowbar first?
	m_iHeight = gHUD.GetHUDBottomLine();//m_iHeight = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight/2;// XDM3038a: what if player picks up crowbar first?

	DrawWList(flTime);// Draw Weapon Menu

	gHR.DrawAmmoHistory(flTime);// Draw ammo pickup history

	if (!IsActive())
		return 0;

	if (m_pWeapon == NULL)
		return 0;

	if (m_iActiveWeapon == WEAPON_NONE)// XDM3037
		return 0;

	if ((m_pWeapon->iAmmoType == AMMOINDEX_NONE) && (m_pWeapon->iAmmo2Type == AMMOINDEX_NONE))
		return 0;

	int x = 0, y;
	int iFlags = DHN_DRAWZERO;// draw 0 values
	int iAmmoWidth = RectWidth(gHUD.GetSpriteRect(gHUD.m_HUD_number_0));
	byte r, g, b, a = (int)max(MIN_ALPHA, m_fFade);

	if (m_fFade > MIN_ALPHA)
		m_fFade -= (float)(gHUD.m_flTimeDelta * HUD_FADE_SPEED);

	// Draw dynamic spread display under main crosshair
	if (m_hCurrentCrosshair != 0 && m_pCvarDrawAccuracy->value > 0.0f && (m_pWeapon->iState == wstate_current || m_pWeapon->iState == wstate_current_ontarget || m_pWeapon->iState == wstate_reloading) && m_iHUDSpriteSpread != HUDSPRITEINDEX_INVALID && gHUD.m_pLocalPlayer && gHUD.m_pLocalPlayer->curstate.fuser1 > 0.0f)
	{
		if (m_fShootSpread <= gHUD.m_pLocalPlayer->curstate.fuser1 || m_pCvarDrawAccuracy->value == 2.0f)
			m_fShootSpread = gHUD.m_pLocalPlayer->curstate.fuser1;
		else
		{
			m_fShootSpread -= gHUD.m_flTimeDelta*HUD_SPREAD_RESET_RATE*m_fShootSpread;
			if (m_fShootSpread < gHUD.m_pLocalPlayer->curstate.fuser1)// prevent flickering
				m_fShootSpread = gHUD.m_pLocalPlayer->curstate.fuser1;
		}

		//CON_NPRINTF(20, "Spread: %f", gHUD.m_pLocalPlayer->curstate.fuser1);
		int iCenterX = ScreenWidth/2;
		int iCenterY = ScreenHeight/2;
		//int iCrossLeft = iCenterX - RectWidth(m_CurrentCrosshairRect)/2;
		int iCrossBottom = iCenterY + RectHeight(m_CurrentCrosshairRect)/2;
		int iSpreadHalfWidth = RectWidth(m_rcSpread)/2;//XRES(40);
		int iSpreadHeight = RectHeight(m_rcSpread);
		float k = min(SPREAD_DISPLAY_MAX, m_fShootSpread)/SPREAD_DISPLAY_MAX;
		GetMeterColor(1.0f-k, r,g,b);
		int iSpreadCurrentWidth = iSpreadHalfWidth*k;
		int iLineH;
		if (gHUD.m_iDistortMode & (HUD_DISTORT_SPRITE|HUD_DISTORT_POS))
			iLineH = (int)(iSpreadHeight*(0.5f + gHUD.m_fDistortValue*RANDOM_FLOAT(0.0f,0.5f))*0.25f);
		else
			iLineH = iSpreadHeight/4;
		//iSpreadHeight /= 4; FillRGBA(iCenterX - iSpreadCurrentWidth, iCrossBottom+iSpreadHeight, iSpreadCurrentWidth, iSpreadHeight, r,g,b,a);
		//FillRGBA(iCenterX, iCrossBottom+iSpreadHeight, iSpreadCurrentWidth, iSpreadHeight, r,g,b,a);
		FillRGBA(iCenterX - iSpreadCurrentWidth, iCrossBottom+iSpreadHeight-iLineH, iSpreadCurrentWidth*2, iLineH, r,g,b,a);
		ScaleColors(r,g,b, (byte)(255.0f*(0.25f + 0.75f*k)));
		SPR_Set(gHUD.GetSprite(m_iHUDSpriteSpread), r,g,b);
		SPR_DrawAdditive(0, iCenterX-iSpreadHalfWidth, iCrossBottom, &m_rcSpread);// sprite: [\|_|_|_|+|_|_|_|/]
	}

	UnpackRGB(r,g,b, RGB_GREEN);
	ScaleColors(r, g, b, a);

	y = m_iHeight;
	HUD_AMMO *pAmmo;
	// Draw clip and primary ammo
	if (m_pWeapon->iAmmoType >= 0)// != AMMOINDEX_NONE)
	{
#if defined (OLD_WEAPON_AMMO_INFO)
		int iIconWidth = RectWidth(m_pWeapon->rcAmmo);
#else
		pAmmo = &m_AmmoInfoArray[m_pWeapon->iAmmoType];
		int iIconWidth = RectWidth(pAmmo->rcAmmoSprite);
#endif
		if (m_pWeapon->iClip > 0 || m_pWeapon->iMaxClip != WEAPON_NOCLIP)// XDM3038a: display "0" only if we have iMaxClip defined
		{
			// room for the number, divider '|' and the current ammo
			x = ScreenWidth - (8 * iAmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, m_pWeapon->iClip, r, g, b);
			int iBarWidth = iAmmoWidth/10;
			x += iAmmoWidth/2;
			UnpackRGB(r,g,b, RGB_GREEN);
			// draw the divider | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);
			x += iBarWidth + iAmmoWidth/2;
			ScaleColors(r, g, b, a);
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(m_pWeapon->iAmmoType), r, g, b);		
		}
		else// no clip
		{
			x = ScreenWidth - 4 * iAmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(m_pWeapon->iAmmoType), r, g, b);
		}
		// draw ammo icon
#if defined (OLD_WEAPON_AMMO_INFO)
		int iOffset = RectHeight(m_pWeapon->rcAmmo)/8;
		SPR_Set(m_pWeapon->hAmmo, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo);
#else
		int iOffset = RectHeight(pAmmo->rcAmmoSprite)/8;
		SPR_Set(pAmmo->hAmmoSprite, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset, &pAmmo->rcAmmoSprite);
#endif
	}
	// Draw seconday ammo
	if (m_pWeapon->iAmmo2Type >= 0)// XDM =
	{
#if defined (OLD_WEAPON_AMMO_INFO)
		int iIconWidth = RectWidth(m_pWeapon->rcAmmo2);
#else
		pAmmo = &m_AmmoInfoArray[m_pWeapon->iAmmo2Type];
		int iIconWidth = RectWidth(pAmmo->rcAmmoSprite);
#endif
		if (gWR.CountAmmo(m_pWeapon->iAmmo2Type) > 0)
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight/4;
			x = ScreenWidth - 4 * iAmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags|DHN_3DIGITS, gWR.CountAmmo(m_pWeapon->iAmmo2Type), r, g, b);
			// draw ammo icon
#if defined (OLD_WEAPON_AMMO_INFO)
			int iOffset = RectHeight(m_pWeapon->rcAmmo2)/8;
			SPR_Set(m_pWeapon->hAmmo2, r, g, b);
			SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo2);
#else
			int iOffset = RectHeight(pAmmo->rcAmmoSprite)/8;
			SPR_Set(pAmmo->hAmmoSprite, r, g, b);
			SPR_DrawAdditive(0, x, y - iOffset, &pAmmo->rcAmmoSprite);
#endif
		}
	}
	//m_iAmmoDisplayY = y;
	m_iWidth = x;
	m_iHeight = y;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the remaining ammo indicator bar on the selector hud
// Input  : x y - 
//			width height - 
//			f - percent to fill (0...1)
// Output : int
//-----------------------------------------------------------------------------
int DrawBar(int x, int y, int width, int height, float f)
{
	byte r, g, b;// XDM3035
	if (f < 0.0f)
		f = 0.0f;
	else if (f > 1.0f)
		f = 1.0f;

	if (f > 0.0f)
	{
		int w = (int)(f * (float)width);
		if (w <= 0)// Always show at least one pixel if we have ammo.
			w = 1;

		UnpackRGB(r, g, b, RGB_BLUE);
		FillRGBA(x, y, w, height, r, g, b, 255);
		x += w;
		width -= w;
	}
	UnpackRGB(r, g, b, RGB_YELLOW);
	FillRGBA(x, y, width, height, r, g, b, 127);
	return (x + width);
}

//-----------------------------------------------------------------------------
// Purpose: DrawAmmoBar - the thin bar inside weapon selection bucket
// Input  : *p - 
//			x y - 
//			width height - 
//-----------------------------------------------------------------------------
void CHudAmmo::DrawAmmoBar(HUD_WEAPON *p, int x, int y, int width, int height)
{
	if (p == NULL)
		return;

	float f = 0.0f;
	if (p->iAmmoType != AMMOINDEX_NONE)
	{
		//if (!gWR.CountAmmo(p->iAmmoType))
		//	return;

#if defined (OLD_WEAPON_AMMO_INFO)
		f = (float)gWR.CountAmmo(p->iAmmoType)/(float)p->iMax1;
#else
		short max = MaxAmmoCarry(p->iAmmoType);
		if (max > 0)
			f = (float)gWR.CountAmmo(p->iAmmoType)/(float)max;
#endif
		x = DrawBar(x, y, width, height, f);
	}
	// Do we have secondary ammo too?
	if (p->iAmmo2Type != AMMOINDEX_NONE)
	{
#if defined (OLD_WEAPON_AMMO_INFO)
		f = (float)gWR.CountAmmo(p->iAmmo2Type)/(float)p->iMax2;
#else
		short max = MaxAmmoCarry(p->iAmmo2Type);
		if (max > 0)
			f = (float)gWR.CountAmmo(p->iAmmo2Type)/(float)max;
#endif
		x += height;//4; XDM3037: make a square space between two indicators // don't forget about some offset !!!
		DrawBar(x, y, width, height, f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw Weapon Menu
// Input  : &flTime - 
// Output : int
//-----------------------------------------------------------------------------
int CHudAmmo::DrawWList(const float &flTime)
{
	if (m_pActiveSel == NULL)
		return 0;

	if (m_pCvarWeaponSelectionTime && (m_pCvarWeaponSelectionTime->value > 0) && (/*gHUD.m_Ammo.*/m_fSelectorShowTime + m_pCvarWeaponSelectionTime->value) <= gHUD.m_flTime)// XDM3037
	{
		SlotClose();
		return 0;
	}

	int x,y,i;
	short iActiveSlot;
	byte r,g,b;

	if (m_pActiveSel == g_pwNoSelection)
		iActiveSlot = -1;// current slot has no weapons
	else
		iActiveSlot = m_pActiveSel->iSlot;

	x = HUD_WEAPON_SELECTION_MARGIN;//!!!
	y = HUD_WEAPON_SELECTION_MARGIN;//!!!

	// Ensure that there are available choices in the active slot
	if (iActiveSlot >= 0)// XDM3037a: = 0?
	{
		if (!gWR.GetFirstPos(iActiveSlot))
		{
			m_pActiveSel = g_pwNoSelection;
			iActiveSlot = -1;
		}
	}

	// Slot numbers
	int iWidth;
	for (i = 0; i < MAX_WEAPON_SLOTS; ++i)
	{
		UnpackRGB(r, g, b, RGB_GREEN);
		ScaleColors(r, g, b, (iActiveSlot == i)?MAX_ALPHA:WEAPONLIST_ALPHA);// XDM3035c: fix?
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b);

		if (i == iActiveSlot)
		{
			HUD_WEAPON *p = gWR.GetFirstPos(iActiveSlot);
			if (p)
				iWidth = RectWidth(p->rcActive);
			else
				iWidth = m_iBucketWidth;
		}
		else
			iWidth = m_iBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket0 + i));
		x += iWidth + HUD_WEAPON_SELECTION_SPACING_X;
	}

	x = HUD_WEAPON_SELECTION_MARGIN;

	// Draw all of the buckets in the selected slot
	for (i = 0; i < MAX_WEAPON_SLOTS; ++i)
	{
		y = m_iBucketHeight + HUD_WEAPON_SELECTION_MARGIN+YRES(1);
		// If this is the active slot, draw the big pictures, otherwise draw boxes
		if (i == iActiveSlot)
		{
			HUD_WEAPON *p = gWR.GetFirstPos(i);
			iWidth = m_iBucketWidth;
			if (p)
				iWidth = RectWidth(p->rcActive);

			for (uint16 iPos = 0; iPos < MAX_WEAPON_POSITIONS; ++iPos)
			{
				p = gWR.GetWeaponSlot(i, iPos);
				if (p == NULL || p->iId == WEAPON_NONE)
					continue;

				// if active, then we must have ammo
				if (m_pActiveSel == p)
				{
					UnpackRGB(r,g,b, RGB_GREEN);
					SPR_Set(p->hActive, r, g, b);
					SPR_DrawAdditive(0, x, y, &p->rcActive);
					SPR_Set(gHUD.GetSprite(m_HUD_selection), r, g, b);
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					if (gWR.IsSelectable(p))
					{
						UnpackRGB(r,g,b, RGB_GREEN);
						ScaleColors(r, g, b, (p==m_pWeapon)?MAX_ALPHA:WEAPONLIST_ALPHA);
					}
					else// XDM: Draw Weapon in YELLOW if no ammo
					{
						UnpackRGB(r,g,b, RGB_YELLOW);
						ScaleColors(r, g, b, MIN_ALPHA);
					}
					SPR_Set(p->hInactive, r, g, b);
					SPR_DrawAdditive(0, x, y, &p->rcInactive);
				}

				// Draw ammo bar
				DrawAmmoBar(p, x + m_iABWidth/2, y+1, m_iABWidth, m_iABHeight);
				y += RectHeight(p->rcActive) + HUD_WEAPON_SELECTION_SPACING_Y;
			}

			if (m_pActiveSel)// XDM3037: draw ammo types used by selected weapon
			{
				int xoffset = 0;
				if (m_pActiveSel->iAmmoType >= 0 && AmmoSpriteHandle(m_pActiveSel->iAmmoType) != 0)// iAmmoType != AMMOINDEX_NONE
				{
					if (gWR.CountAmmo(m_pActiveSel->iAmmoType) > 0)
						UnpackRGB(r,g,b, RGB_GREEN);
					else
						UnpackRGB(r,g,b, RGB_YELLOW);

#if defined (OLD_WEAPON_AMMO_INFO)
					SPR_Set(m_pActiveSel->hAmmo, r, g, b);
					SPR_DrawAdditive(0, x, y, &m_pActiveSel->rcAmmo);
#else
					SPR_Set(m_AmmoInfoArray[m_pActiveSel->iAmmoType].hAmmoSprite, r, g, b);
					SPR_DrawAdditive(0, x, y, &m_AmmoInfoArray[m_pActiveSel->iAmmoType].rcAmmoSprite);
					xoffset += RectWidth(m_AmmoInfoArray[m_pActiveSel->iAmmoType].rcAmmoSprite);
#endif
				}
				if (m_pActiveSel->iAmmo2Type >= 0 && AmmoSpriteHandle(m_pActiveSel->iAmmo2Type) != 0)
				{
					if (gWR.CountAmmo(m_pActiveSel->iAmmo2Type) > 0)
						UnpackRGB(r,g,b, RGB_GREEN);
					else
						UnpackRGB(r,g,b, RGB_YELLOW);

#if defined (OLD_WEAPON_AMMO_INFO)
					SPR_Set(m_pActiveSel->hAmmo2, r, g, b);
					SPR_DrawAdditive(0, x+RectWidth(m_pActiveSel->rcAmmo), y, &m_pActiveSel->rcAmmo2);
#else
					SPR_Set(m_AmmoInfoArray[m_pActiveSel->iAmmo2Type].hAmmoSprite, r, g, b);
					SPR_DrawAdditive(0, x+xoffset, y, &m_AmmoInfoArray[m_pActiveSel->iAmmo2Type].rcAmmoSprite);
#endif
				}
			}
			x += iWidth + HUD_WEAPON_SELECTION_SPACING_X;
		}
		else// (i == iActiveSlot)
		{
			UnpackRGB(r,g,b, RGB_GREEN);
			HUD_WEAPON *p;
			for (uint16 iPos = 0; iPos < MAX_WEAPON_POSITIONS; ++iPos)
			{
				p = gWR.GetWeaponSlot(i, iPos);
				if (p == NULL || p->iId == WEAPON_NONE)
					continue;

				if (gWR.IsSelectable(p))
					UnpackRGB(r,g,b, RGB_GREEN);
				else
					UnpackRGB(r,g,b, RGB_YELLOW);

				SPR_Set(gHUD.GetSprite(m_HUD_bucket_flat), r, g, b);// XDM3037
				SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket_flat));
				y += m_iBucketHeight + HUD_WEAPON_SELECTION_SPACING_Y;
			}
			x += m_iBucketWidth + HUD_WEAPON_SELECTION_SPACING_X;
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Load icons for specific HUD_AMMO structure
// Input  : &iAmmoID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/* slow, we better load all sprites at once. bool CHudAmmo::LoadAmmoSprite(const byte &iAmmoID)
{
	if (m_AmmoInfoArray[iAmmoID].szName[0] == 0)
	{
		conprintf(1, "CHudAmmo::LoadAmmoSprite(%d) failed: unregistered ammo index!\n", iAmmoID);
		return false;
	}
	int count = 0;
	client_sprite_t *pList;
	client_sprite_t *pInfo = NULL;
	const char *szListFileName = "hud_ammo.txt";
	char szListFile[MAX_PATH];
	// Optional custom index file
	_snprintf(szListFile, MAX_PATH, "maps/%s_%s", GetMapName(true), szListFileName);
	szListFile[MAX_PATH-1] = '\0';
	// Load the index file
	pList = SPR_GetList(szListFile, &count);
	if (pList)
	{
		// Now find required info in the list by ammo name
		pInfo = GetSpriteList(pList, m_AmmoInfoArray[iAmmoID].szName, gHUD.m_iRes, count);// this shit cannot handle names with spaces!
		if (pInfo == NULL)
			conprintf(1, "CHudAmmo::LoadAmmoSprite(%d) failed: %s not found in list!\n", iAmmoID, m_AmmoInfoArray[iAmmoID].szName);
	}
#if defined (_DEBUG)
	else
		conprintf(1, "CHudAmmo::LoadAmmoSprite(%d) unable to open list \"%s\".\n", iAmmoID, szListFile);
#endif

	// Now try the main index file
	if (pInfo == NULL)
	{
		_snprintf(szListFile, MAX_PATH, "sprites/%s", szListFileName);
		szListFile[MAX_PATH-1] = '\0';

		// Load the index file
		pList = SPR_GetList(szListFile, &count);
		if (pList == NULL)
		{
			conprintf(1, "CHudAmmo::LoadAmmoSprite(%d) failed to open list \"%s\"!\n", iAmmoID, szListFile);
			return false;
		}
		// Now find required info in the list by ammo name
		pInfo = GetSpriteList(pList, m_AmmoInfoArray[iAmmoID].szName, gHUD.m_iRes, count);// this shit cannot handle names with spaces!
		if (pInfo == NULL)
		{
			conprintf(1, "CHudAmmo::LoadAmmoSprite(%d) failed: %s not found in list!\n", iAmmoID, m_AmmoInfoArray[iAmmoID].szName);
			m_AmmoInfoArray[iAmmoID].hAmmoSprite = 0;
			m_AmmoInfoArray[iAmmoID].rcAmmoSprite = nullrc;
			return false;
		}
	}

	char sz[64];
	_snprintf(sz, 64, "sprites/%s.spr", pInfo->szSprite);
	sz[63] = '\0';
	m_AmmoInfoArray[iAmmoID].hAmmoSprite = SPR_Load(sz);
	m_AmmoInfoArray[iAmmoID].rcAmmoSprite = pInfo->rc;
	return true;
}*/

//-----------------------------------------------------------------------------
// Purpose: Load icons for all HUD_AMMO structures
// Output : uint32 - number of entries loaded
//-----------------------------------------------------------------------------
uint32 CHudAmmo::LoadAmmoSprites(void)
{
	uint32 iResult = 0;
	uint32 iAmmoID;
	int count = 0;
	client_sprite_t *pList;
	client_sprite_t *pInfo = NULL;
	char szListFile[MAX_PATH];
	char szSprite[64];
	const char *szListFileName = "hud_ammo.txt";
	// Optional custom index file
	_snprintf(szListFile, MAX_PATH, "maps/%s_%s", GetMapName(true), szListFileName);
	szListFile[MAX_PATH-1] = '\0';
	pList = SPR_GetList(szListFile, &count);
	if (pList)
	{
		// Now find required info in the list by ammo name
		for (iAmmoID = 0; iAmmoID < m_iAmmoTypes; ++iAmmoID)
		{
			if (m_AmmoInfoArray[iAmmoID].szName[0])
			{
				pInfo = GetSpriteList(pList, m_AmmoInfoArray[iAmmoID].szName, gHUD.m_iRes, count);// this shit cannot handle names with spaces!
				if (pInfo == NULL)
				{
					conprintf(1, "CHudAmmo::LoadAmmoSprites(%d): %s not found in custom list.\n", iAmmoID, m_AmmoInfoArray[iAmmoID].szName);
					m_AmmoInfoArray[iAmmoID].hAmmoSprite = 0;
					m_AmmoInfoArray[iAmmoID].rcAmmoSprite = nullrc;
				}
				else
				{
					_snprintf(szSprite, 64, "sprites/%s.spr", pInfo->szSprite);
					szSprite[63] = '\0';
					m_AmmoInfoArray[iAmmoID].hAmmoSprite = SPR_Load(szSprite);
					m_AmmoInfoArray[iAmmoID].rcAmmoSprite = pInfo->rc;
					++iResult;
				}
			}
		}
	}
	else
		conprintf(2, "CHudAmmo::LoadAmmoSprites() custom list not present \"%s\".\n", szListFile);

	// Now load the main index file
	_snprintf(szListFile, MAX_PATH, "sprites/%s", szListFileName);
	szListFile[MAX_PATH-1] = '\0';
	pList = SPR_GetList(szListFile, &count);
	if (pList == NULL)
	{
		conprintf(0, "CHudAmmo::LoadAmmoSprites() ERROR: failed to open list \"%s\"!\n", szListFile);
		return iResult;
	}
	// Now find required info in the list by ammo name
	for (iAmmoID = 0; iAmmoID < m_iAmmoTypes; ++iAmmoID)
	{
		if (m_AmmoInfoArray[iAmmoID].szName[0] && m_AmmoInfoArray[iAmmoID].hAmmoSprite == 0)// hasn't been loaded yet
		{
			pInfo = GetSpriteList(pList, m_AmmoInfoArray[iAmmoID].szName, gHUD.m_iRes, count);// this shit cannot handle names with spaces!
			if (pInfo == NULL)
			{
				conprintf(1, "CHudAmmo::LoadAmmoSprites(%d) failed: %s not found in list!\n", iAmmoID, m_AmmoInfoArray[iAmmoID].szName);
				m_AmmoInfoArray[iAmmoID].hAmmoSprite = 0;
				m_AmmoInfoArray[iAmmoID].rcAmmoSprite = nullrc;
			}
			else
			{
				_snprintf(szSprite, 64, "sprites/%s.spr", pInfo->szSprite);
				szSprite[63] = '\0';
				m_AmmoInfoArray[iAmmoID].hAmmoSprite = SPR_Load(szSprite);
				m_AmmoInfoArray[iAmmoID].rcAmmoSprite = pInfo->rc;
				++iResult;
			}
		}
	}
	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037
// Input  : &iAmmoID - 
// Output : short - maximum amount of ammo in the inventory
//-----------------------------------------------------------------------------
short CHudAmmo::MaxAmmoCarry(const byte &iAmmoID)
{
	if (m_AmmoInfoArray[iAmmoID].szName[0] == 0)
		return 0;

	return m_AmmoInfoArray[iAmmoID].iMax;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037
// Input  : &iAmmoID - 
// Output : HLSPRITE - ammo icon
//-----------------------------------------------------------------------------
HLSPRITE CHudAmmo::AmmoSpriteHandle(const byte &iAmmoID)
{
	if (m_AmmoInfoArray[iAmmoID].szName[0] == 0)
		return 0;

	return m_AmmoInfoArray[iAmmoID].hAmmoSprite;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037
// Input  : &iAmmoID - 
// Output : wrect_t
//-----------------------------------------------------------------------------
wrect_t &CHudAmmo::AmmoSpriteRect(const byte &iAmmoID)
{
	if (m_AmmoInfoArray[iAmmoID].szName[0] == 0)
		return nullrc;

	return m_AmmoInfoArray[iAmmoID].rcAmmoSprite;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037 Abstraction layer
// Input  : *m_pWeapon - 
//			type - 
// Output : HLSPRITE
//-----------------------------------------------------------------------------
/*HLSPRITE CHudAmmo::AmmoSpriteHandle(HUD_WEAPON *m_pWeapon, byte type)
{
	if (m_pWeapon == NULL)
		return 0;
#if defined (OLD_WEAPON_AMMO_INFO)
	if (type == 0)
		return m_pWeapon->hAmmo;
	else
		return m_pWeapon->hAmmo2;
#else
	if (type == 0)
		return m_AmmoInfoArray[m_pWeapon->iAmmoType].hAmmoSprite;
	else
		return m_AmmoInfoArray[m_pWeapon->iAmmo2Type].hAmmoSprite;
#endif
}*/

//-----------------------------------------------------------------------------
// Purpose: XDM3037 Abstraction layer
// Input  : *m_pWeapon - 
//			type - 
// Output : wrect_t
//-----------------------------------------------------------------------------
/*wrect_t &CHudAmmo::AmmoSpriteRect(HUD_WEAPON *m_pWeapon, byte type)
{
	if (m_pWeapon == NULL)
		return nullrc;
#if defined (OLD_WEAPON_AMMO_INFO)
	if (type == 0)
		return m_pWeapon->rcAmmo;
	else
		return m_pWeapon->rcAmmo2;
#else
	if (type == 0)
		return m_AmmoInfoArray[m_pWeapon->iAmmoType].rcAmmoSprite;
	else
		return m_AmmoInfoArray[m_pWeapon->iAmmo2Type].rcAmmoSprite;
#endif
}*/

//-----------------------------------------------------------------------------
// Purpose: Slot button pressed
// Input  : iSlot - 
//-----------------------------------------------------------------------------
void CHudAmmo::SlotInput(short iSlot)
{
	if (iSlot > MAX_WEAPON_SLOTS)
		return;

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return;

	if (!gHUD.PlayerHasSuit() || !gHUD.PlayerIsAlive())// XDM3038a
		return;

	if (!gWR.HasWeapons())//if ((gHUD.m_iWeaponBits & WEAPON_ALLWEAPONS) == 0)// XDM3038: no weapons (suit does not count)
		return;

	HUD_WEAPON *p = NULL;
	bool fastSwitch = /*gHUD.m_Ammo.*/m_pCvarFastSwitch->value > 0.0f;

	if ((m_pActiveSel == NULL) || (m_pActiveSel == g_pwNoSelection) || (iSlot != m_pActiveSel->iSlot))
	{
		p = gWR.GetFirstPos(iSlot);
		if (p && fastSwitch) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress but only if there is only one item in the bucket
			HUD_WEAPON *p2 = gWR.GetNextActivePos(p->iSlot, p->iSlotPos);
			if (p2 == NULL)// only one active item in bucket, so change directly to weapon
			{
				SelectItem(p->iId);
				return;
			}
		}
		PlaySound("common/wpn_hudon.wav", VOL_NORM);
	}
	else
	{
		if (m_pActiveSel)
			p = gWR.GetNextActivePos(m_pActiveSel->iSlot, m_pActiveSel->iSlotPos);
		if (p == NULL)
			p = gWR.GetFirstPos(iSlot);

		PlaySound("common/wpn_moveselect.wav", VOL_NORM);
	}

	if (p == NULL)// no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if (!fastSwitch)
			m_pActiveSel = g_pwNoSelection;
		else
			m_pActiveSel = NULL;
	}
	else 
		m_pActiveSel = p;

	m_fSelectorShowTime = gHUD.m_flTime;// XDM3038c: reset timeout counter
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAmmo::SlotClose(void)
{
	if (m_pActiveSel)
	{
		//unused m_pLastSel = m_pActiveSel;
		m_pActiveSel = NULL;
		m_fSelectorShowTime = 0.0f;// XDM3037
		PlaySound("common/wpn_hudoff.wav", VOL_NORM);
	}
}




//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Safe way to add weapon to inventory
//-----------------------------------------------------------------------------
void CHudAmmo::WeaponPickup(const int &iID)
{
	if (iID == WEAPON_SUIT)
		return;
	if (iID == WEAPON_NONE)
		return;

	gWR.PickupWeapon(/*gHUD.m_Ammo.*/gWR.GetWeaponStruct(iID));

	gHR.AddToHistory(HISTSLOT_WEAP, iID);
	m_iRecentPickedWeapon = iID;// XDM3035a

	if (ShouldSwitchWeapon(iID))// should we switch to this item?
		SelectItem(iID);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Safe way to remove weapon from inventory
//-----------------------------------------------------------------------------
void CHudAmmo::WeaponRemove(const int &iID)
{
	if (iID == WEAPON_SUIT)
		return;
	if (iID == WEAPON_NONE)
		return;

	gWR.DropWeapon(/*gHUD.m_Ammo.*/gWR.GetWeaponStruct(iID));

	if (m_iRecentPickedWeapon == iID)
		m_iRecentPickedWeapon = WEAPON_NONE;

	if (g_weaponselect == iID)
		g_weaponselect = WEAPON_NONE;
}

//-----------------------------------------------------------------------------
// Weapons
//-----------------------------------------------------------------------------
/*int CHudAmmo::MsgFunc_WeapPickup(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	WeaponPickup(READ_BYTE());
	END_READ();
	return 1;
}*/

//-----------------------------------------------------------------------------
// Purpose: Item pickup notification
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_ItemPickup(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	int count = READ_SHORT();
	const char *szName = READ_STRING();
	END_READ();

	// Add the weapon to the history
	int id = gHUD.GetSpriteIndex(szName);
	if (id >= 0)
		gHR.AddToHistory(HISTSLOT_ITEM, id, count);

	if (strcmp(szName, "item_longjump") == 0)// XDM3035c
	{
		PlaySoundSuit("!HEV_A1");
		/* bad		if (gHUD.m_iGameType > GT_SINGLE)// XDM3037a: UNDONE: needs save/restore in SP
		{
			byte r,g,b;
			UnpackRGB(r,g,b, RGB_GREEN);
			gHUD.m_StatusIcons.EnableIcon(szName, r,g,b, 0, 0);
		}*/
	}
	//else if (strcmp(szName, "item_security") == 0)
	//	PlaySoundSuit("!");
	else if (strcmp(szName, "item_antidote") == 0)
		PlaySoundSuit("!HEV_HEAL4");

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Hide weapon info from HUD
// Note   : Used by the entire HUD, not just weapon selection!
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_HideWeapon(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	gHUD.m_iHideHUDDisplay = READ_BYTE();
	END_READ();
	//conprintf(1, "CHudAmmo::MsgFunc_HideWeapon() %d\n", gHUD.m_iHideHUDDisplay);

	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
	{
		m_pActiveSel = NULL;
		UpdateCrosshair(CROSSHAIR_OFF);// XDM3038: nice way to do things
	}
	else
		UpdateCrosshair(CROSSHAIR_NORMAL);// XDM3038: nice way to do things

	return 1;
}

// OBSOLETE! DO NOT USE!
// Replaced by MsgFunc_UpdWeapons
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
/*int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf)
{
	static wrect_t nullrc;
	int fOnTarget = FALSE;

	BEGIN_READ(pbuf, iSize);
	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();
	END_READ();
	//conprintf(1, "CHudAmmo::MsgFunc_CurWeapon() %d %d %d OBSOLETE!\n", iState, iId, iClip);

	// detect if we're also on target
	if (iState > 1)
		fOnTarget = TRUE;

	if (iId < 1)
	{
		UpdateCrosshair(CROSSHAIR_OFF);
		m_iFlags &= ~HUD_ACTIVE;// XDM3035c: don't draw ammo indicators too
		//WEAPON *p = gWR.GetWeapon(lol?);
		//if (p)
		//	gWR.DropWeapon(p);
		return 0;
	}

	if (g_iUser1 != OBS_IN_EYE)
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = TRUE;
			m_pActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = FALSE;
	}

	HUD_WEAPON *pWeapon = gWR.GetWeapon(iId);
	if (!pWeapon)
		return 0;

	if (iClip < -1)
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;

	if (iState == 0)	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	if (!(gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) && pWeapon)// XDM: check weapon
		UpdateCrosshair(CROSSHAIR_NORMAL);// XDM

	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
	return 1;
}*/

//-----------------------------------------------------------------------------
// Purpose: Recieves weapon registry from the server. One for each weapon.
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	// SHI!! Can't gWR.GetWeapon() until we get the ID!
	HUD_WEAPON Weapon;// create a temporary instance to read into
	memset(&Weapon, 0, sizeof(HUD_WEAPON));

	BEGIN_READ(pbuf, iSize);
	//strncpy(Weapon.szName, READ_STRING(), MAX_WEAPON_NAME_LEN);
	//ASSERT(Weapon.szName[0] != '\0');
	READ_STRING(Weapon.szName, MAX_WEAPON_NAME_LEN);
	Weapon.iAmmoType = READ_CHAR();	
#if defined (OLD_WEAPON_AMMO_INFO)
	Weapon.iMax1 = READ_BYTE();
	if (Weapon.iMax1 == 255)
		Weapon.iMax1 = -1;
#endif
	Weapon.iAmmo2Type = READ_CHAR();
#if defined (OLD_WEAPON_AMMO_INFO)
	Weapon.iMax2 = READ_BYTE();
	if (Weapon.iMax2 == 255)
		Weapon.iMax2 = -1;
#endif
#if defined (SERVER_WEAPON_SLOTS)
	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
#endif
	Weapon.iId = READ_CHAR();
	Weapon.iMaxClip = READ_BYTE();// XDM3037a
	if (Weapon.iMaxClip == UCHAR_MAX)
		Weapon.iMaxClip = WEAPON_NOCLIP;// XDM3038a: a little hack, since -1 becomes 255

	Weapon.iFlags = READ_BYTE();
	//strcpy(Weapon.szViewModel, READ_STRING());
	//ASSERT(Weapon.szViewModel[0] != '\0');
	END_READ();

	// Values not set by message data
	// IMPORTANT PART! Since AddWeapon() overwrites the entire structure, back-up these values first!
	HUD_WEAPON *pW = gWR.GetWeaponStruct(Weapon.iId);
	if (pW != NULL)
	{
		Weapon.iClip = pW->iClip;
		//Weapon.iCount = pW->iCount;
		Weapon.iPriority = pW->iPriority;
#if !defined (SERVER_WEAPON_SLOTS)
		if (GetWeaponSlotPos(Weapon.iId, Weapon.iSlot, Weapon.iSlotPos) == 0)
		{
			conprintf(0, "CHudAmmo::MsgFunc_%s() error! Cannot find slot information for weapon %d (%s)!\n", pszName, Weapon.iId, Weapon.szName);
			Weapon.iSlot = pW->iSlot;//0;
			Weapon.iSlotPos = pW->iSlotPos;//0;
		}
#endif
	}
	gWR.AddWeapon(&Weapon);// <- LoadWeaponSprites() already there
#if defined (_DEBUG)
	pW = gWR.GetWeaponStruct(Weapon.iId);
	conprintf(1, "CHudAmmo::MsgFunc_%s() registered %d %s, ammo1: %hd, ammo2: %hd\n", pszName, pW->iId, pW->szName, pW->iAmmoType, pW->iAmmo2Type);
#endif
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Recieves ammo registry from the server. Can be a sequence of messages.
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_AmmoList(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);

	//NO! It could be a split list!	m_iAmmoTypes = 0;
	uint32 iAmmoID;
	int strpos;
	while (READ_REMAINING())
	{
		iAmmoID = READ_BYTE();
		if (iAmmoID == 0)// new list incoming, reset!
		{
			m_iAmmoTypes = 0;
			memset(m_AmmoInfoArray, 0, sizeof(HUD_AMMO)*MAX_AMMO_SLOTS);// XDM3037
		}
		else if (iAmmoID >= MAX_AMMO_SLOTS)
		{
			//?m_iAmmoTypes = 0;
			conprintf(0, "MsgFunc_%s() ERROR: bad ammo ID in message (%u)!\n", pszName, iAmmoID);
			return 0;
		}
		// useless m_AmmoInfoArray[m_iAmmoTypes].iAmmoID = read_id;
#ifdef DOUBLE_MAX_AMMO
		m_AmmoInfoArray[iAmmoID].iMax = READ_SHORT();
#else
		m_AmmoInfoArray[iAmmoID].iMax = READ_BYTE();
#endif
		if (READ_REMAINING() < 1)
		{
			conprintf(0, "MsgFunc_%s() ERROR: bad message!\n", pszName);
			return 0;
		}
		strncpy(m_AmmoInfoArray[iAmmoID].szName, READ_STRING(), MAX_AMMO_NAME_LEN);
		strpos = 0;
		while (m_AmmoInfoArray[iAmmoID].szName[strpos] && strpos < MAX_AMMO_NAME_LEN)
		{
			if (isspace(m_AmmoInfoArray[iAmmoID].szName[strpos]))
				m_AmmoInfoArray[iAmmoID].szName[strpos] = '_';

			++strpos;
		}
		// BAD(slow)! LoadAmmoSprite(iAmmoID);
#if defined (_DEBUG)
		conprintf(1, "CHudAmmo::MsgFunc_%s() registered %u \"%s\"\n", pszName, iAmmoID, m_AmmoInfoArray[iAmmoID].szName);
#endif
		++m_iAmmoTypes;
	}
	END_READ();
	iAmmoID = LoadAmmoSprites();
#if defined (_DEBUG)
	conprintf(1, "CHudAmmo::MsgFunc_%s() LoadAmmoSprites() returned: %u\n", pszName, iAmmoID);
#endif
	conprintf(1, "CHudAmmo::MsgFunc_%s() total %u ammo types\n", pszName, m_iAmmoTypes);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM: replaces CurWeapon message, handles multiple weapons at once
// Note: XDM3038a: only sent if server is configured to!
// BYTE state
// CHAR iId
// CHAR iClip
// If a weapon is not sent here, it's not in our inventory
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_UpdWeapons(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	byte iState = 0;
	byte iId = WEAPON_NONE;
	short iClip;
	int num = 0;
	while (READ_REMAINING())
	{
		iState = READ_BYTE();
		iId = READ_CHAR();
		if (READ_REMAINING() < 1)
		{
			conprintf(1, "MsgFunc_UpdWeapons(): bad message!\n");
			return 0;
		}
#if defined(DOUBLE_MAX_AMMO)
		iClip = READ_SHORT();
#else
		iClip = READ_CHAR();
#endif
		++num;

		if (iId == WEAPON_NONE)
			continue;

		if (g_iUser1 != OBS_IN_EYE)
		{
			// Is player dead???
			if ((iId == -1) && (iClip == -1))
			{
				//gHUD.m_fPlayerDead = TRUE;
				m_pActiveSel = NULL;
				END_READ();
				return 1;
			}
			//gHUD.m_fPlayerDead = FALSE;
		}

		HUD_WEAPON *pWeapon = gWR.GetWeaponStruct(iId);
		if (pWeapon == NULL)
			continue;

		if (iClip == 255)// XDM3035c: -1 TODO: TESTME: if something goes wrong with clips - check this and server code!
			pWeapon->iClip = WEAPON_NOCLIP;
		else if (iClip < -1)
			pWeapon->iClip = abs(iClip);
		else
			pWeapon->iClip = iClip;

		pWeapon->iState = iState;// XDM3038a
		if (iState == wstate_current || iState == wstate_current_ontarget)// 1 = current, 2 = current and on target
		{
			//if (pWeapon != m_pWeapon)
			//{
			//	m_fFade = MAX_ALPHA; //!!!
				UpdateCurrentWeapon(pWeapon->iId);// XDM3038
			//}
			//m_pWeapon = pWeapon;// make weapon current

			if (!FBitSet(gHUD.m_iHideHUDDisplay, HIDEHUD_WEAPONS | HIDEHUD_ALL) && pWeapon)// XDM: check weapon
				UpdateCrosshair(CROSSHAIR_NORMAL);// XDM3038c

			//conprintf(1, "CHudAmmo::MsgFunc_UpdWeapons() weapon %d state is %d\n", iId, iState);
		}
		SetActive(true);
	}
	END_READ();
	//conprintf(1, "CHudAmmo::MsgFunc_UpdWeapons() updated %d weapons\n", num);

#if !defined (SERVER_WEAPON_SLOTS)
	if (m_pWeapon == NULL && num > 0 && iId != WEAPON_NONE)// XDM3037
	{
		//conprintf(1, "CHudAmmo::gogo() %d\n", iId);
		SelectItem(iId);// well, it's a little bit of a hack - because we still have no updates from weapon bits and thus HaveWeapon() fails, we have to explicitly select ANY acquired item
	}
#endif
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM: Replaces MsgFunc_AmmoX message, handles multiple weapons at once
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_UpdAmmo(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	short num = 0;
	short iIndex;
	short iCount;
	short iCountPrev;
	while (READ_REMAINING())
	{
		iIndex = READ_BYTE();
		if (READ_REMAINING() < 1)
		{
			conprintf(1, "MsgFunc_UpdAmmo(): bad message!\n");
			END_READ();
			return 0;
		}
#if defined (DOUBLE_MAX_AMMO)
		iCount = READ_SHORT();
#else
		iCount = READ_BYTE();
#endif
		++num;

		iCountPrev = gWR.CountAmmo(iIndex);
		if (iCountPrev < iCount)
			gHR.AddToHistory(HISTSLOT_AMMO, iIndex, iCount-iCountPrev);// XDM3038a: detect new ammo

		gWR.SetAmmo(iIndex, abs(iCount));
	}
	END_READ();
	//conprintf(1, "CHudAmmo::MsgFunc_UpdAmmo() updated %d ammo\n", num);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: now server sends us a signal to select next best item
//-----------------------------------------------------------------------------
int CHudAmmo::MsgFunc_SelBestItem(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("CHudAmmo::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	m_iSelectNextBestItemCache = READ_BYTE();
	END_READ();
	if (m_iSelectNextBestItemCache == WEAPON_NONE)
		m_iSelectNextBestItemCache = WEAPON_SUIT;// little hack just to make it start

	/* XDM3038a: since m_iWeaponBits is almost for sure empty, cache this call until we get some weapon info
	int iBestID = GetNextBestItem(iCurrentID);
	//conprintf(1, "CHudAmmo::MsgFunc_SelBestItem() %d %d\n", iCurrentID, iBestID);

	if (iBestID != WEAPON_NONE)
		SelectItem(iBestID);*/

	return 1;
}




//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput(0);
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput(1);
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput(2);
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput(3);
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput(4);
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput(5);
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput(6);
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput(7);
}

/*void CHudAmmo::UserCmd_Slot(void)
{
	SlotInput(atoi(CMD_ARGV(1)));
}*/

//-----------------------------------------------------------------------------
// Purpose: XDM3038: select weapon by ID
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_SelectWeapon(void)
{
	SelectItem(atoi(CMD_ARGV(1)));
}

//-----------------------------------------------------------------------------
// Purpose: Close weapon selection
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_Close(void)
{
	if (m_pActiveSel)
		SlotClose();
	else
		CLIENT_COMMAND("escape\n");// HL1120 overrides the escape key binding anyway
}

//-----------------------------------------------------------------------------
// Purpose: Selects the next item in the weapon menu
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if (!gHUD.PlayerIsAlive() || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)))
		return;

	if (!m_pActiveSel || m_pActiveSel == g_pwNoSelection)
		m_pActiveSel = m_pWeapon;

	short pos = 0;
	short slot = 0;
	if (m_pActiveSel)
	{
		pos = m_pActiveSel->iSlotPos + 1;
		slot = m_pActiveSel->iSlot;
	}

	for (short loop = 0; loop <= 1; ++loop)
	{
		for ( ; slot < MAX_WEAPON_SLOTS; ++slot)
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; ++pos)
			{
				HUD_WEAPON *wsp = gWR.GetWeaponSlot(slot, pos);
				if (wsp && gWR.IsSelectable(wsp))
				{
					m_pActiveSel = wsp;
					m_fSelectorShowTime = gHUD.m_flTime;// XDM3037: reset timeout counter
					return;
				}
			}
			pos = 0;
		}
		slot = 0; // start looking from the first slot again
	}
	m_pActiveSel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Selects the previous item in the weapon menu
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if (!gHUD.PlayerIsAlive() || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)))
		return;

	if (!m_pActiveSel || m_pActiveSel == g_pwNoSelection)
		m_pActiveSel = m_pWeapon;

	short pos = MAX_WEAPON_POSITIONS-1;
	short slot = MAX_WEAPON_SLOTS-1;
	if (m_pActiveSel)
	{
		pos = m_pActiveSel->iSlotPos - 1;
		slot = m_pActiveSel->iSlot;
	}

	for (short loop = 0; loop <= 1; ++loop)
	{
		for ( ; slot >= 0; --slot)
		{
			for ( ; pos >= 0; --pos)
			{
				HUD_WEAPON *wsp = gWR.GetWeaponSlot(slot, pos);
				if (wsp && gWR.IsSelectable(wsp))
				{
					m_pActiveSel = wsp;
					m_fSelectorShowTime = gHUD.m_flTime;// XDM3037: reset timeout counter
					return;
				}
			}
			pos = MAX_WEAPON_POSITIONS-1;
		}
		slot = MAX_WEAPON_SLOTS-1;
	}
	m_pActiveSel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Selects the upper item in current weapon menu only
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_InvUp(void)
{
	if (m_pActiveSel && m_pActiveSel != g_pwNoSelection)
	{
		byte slot = m_pActiveSel->iSlot;
		byte pos = m_pActiveSel->iSlotPos - 1;// byte cannot be < 0, it will underflow and wrap. it's ok
		while (pos >= 0 && pos < MAX_WEAPON_POSITIONS)
		{
			HUD_WEAPON *wsp = gWR.GetWeaponSlot(slot, pos);
			if (wsp && gWR.IsSelectable(wsp))// XDM3037: TODO: HasAmmo(wsp)) ?
			{
				PlaySound("common/wpn_moveselect.wav", VOL_NORM);
				m_pActiveSel = wsp;
				m_fSelectorShowTime = gHUD.m_flTime;// XDM3037: reset timeout counter
				return;
			}
			--pos;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Selects the lower item in current weapon menu only
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_InvDown(void)
{
	if (m_pActiveSel && m_pActiveSel != g_pwNoSelection)
	{
		byte slot = m_pActiveSel->iSlot;
		byte pos = m_pActiveSel->iSlotPos + 1;// overflow is ok
		while (pos >= 0 && pos < MAX_WEAPON_POSITIONS)
		{
			HUD_WEAPON *wsp = gWR.GetWeaponSlot(slot, pos);
			if (wsp && gWR.IsSelectable(wsp))// XDM3037: TODO: HasAmmo(wsp)) ?
			{
				PlaySound("common/wpn_moveselect.wav", VOL_NORM);
				m_pActiveSel = wsp;
				m_fSelectorShowTime = gHUD.m_flTime;// XDM3037: reset timeout counter
				return;
			}
			++pos;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: Selects previously used weapon (moved from server)
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_InvLast(void)
{
	if (m_iPrevActiveWeapon != WEAPON_NONE)
		SelectItem(m_iPrevActiveWeapon);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: puts current weapon away
//-----------------------------------------------------------------------------
void CHudAmmo::UserCmd_Holster(void)
{
	if (g_weaponselect == WEAPON_NONE)// XDM3038a
		SelectItem(m_iActiveWeapon);
	else
		SelectItem(WEAPON_NONE);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: puts current weapon away
//-----------------------------------------------------------------------------
//#if defined (_DEBUG)
void CHudAmmo::UserCmd_DumpInventory(void)
{
	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return;

	HUD_WEAPON *pWeapon;
	uint32 i;
	conprintf(0, "Weapons:\n");// # ID Name AmmoType1 AmmoType2 State HasWeapon\n");
	for (i=1; i<MAX_WEAPONS; ++i)
	{
		pWeapon = gWR.GetWeaponStruct(i);
		if (pWeapon)
			conprintf(0, "%0d: id: %d \"%s\", ammo1: %hd, ammo2: %hd, state: %hd, have:%s`\n", i, pWeapon->iId, pWeapon->szName, pWeapon->iAmmoType, pWeapon->iAmmo2Type, pWeapon->iState, gWR.HasWeapon(pWeapon->iId)?"+":"-");
	}
	conprintf(0, "Ammo types:\n");
	for (i=0; i<m_iAmmoTypes; ++i)
		conprintf(0, "%d: \"%s\", max: %hd, have: %hd\n", i, m_AmmoInfoArray[i].szName, m_AmmoInfoArray[i].iMax, gWR.CountAmmo(i));
}
//#endif
