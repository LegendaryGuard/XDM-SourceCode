// Stubs for server-side functions used in shared code
#include <stdarg.h>
#include "hud.h"
#include "cl_util.h"
#include "archtypes.h"
#include "com_weapons.h"
#include "const.h"
#include "entity_state.h"
#include "r_efx.h"
#include "event_api.h"// XDM


// TRUE if this is the first time we've "predicated" a particular movement/firing command.
// If it is 1, then we should play events/sounds etc., otherwise, we just will be updating state info, but not firing events
bool g_runfuncs = 0;

// During our weapon prediction processing, we'll need to reference some data that is part of
//  the final state passed into the postthink functionality.  We'll set this pointer and then
//  reset it to NULL as appropriate
struct local_state_s *g_finalstate = NULL;
extern globalvars_t  *gpGlobals;


int AddAmmoNameToAmmoRegistry(const char *szAmmoname)
{
	return 0;
}

int GetAmmoIndexFromRegistry(const char *psz)
{
	return 0;
}

// XDM3035b: TODO: customizeable client version
int GetNextBestWeapon(int iCurrentWeaponID)
{
	return 0;
}

// Print alert message to console
void AlertMessage(ALERT_TYPE atype, char *szFmt, ...)
{
	if (atype == at_aiconsole && g_pCvarDeveloper && g_pCvarDeveloper->value < 2.0f)//CVAR_GET_FLOAT("developer") < 2)
		return;

	static char	string[1024];
	va_list argptr;
	va_start(argptr, szFmt);
	vsnprintf(string, 1024, szFmt, argptr);// XDM3038
	va_end(argptr);
	gEngfuncs.Con_Printf("cl:  ");
	gEngfuncs.Con_Printf(string);
}

// Log debug messages to file ( appends )
void COM_Log(char *pszFile, char *fmt, ...)
{
#if defined(_DEBUG)
	va_list		argptr;
	char		string[1024];
	FILE *fp;
	char *pfilename;
	if (pszFile == NULL)
		pfilename = "XDM.log";
	else
		pfilename = pszFile;

	va_start (argptr,fmt);
	_vsnprintf(string, 1024, fmt, argptr);// XDM3038
	va_end (argptr);
	fp = fopen(pfilename, "a+t");
	if (fp)
	{
		fprintf(fp, "%s", string);
		fclose(fp);
	}
#endif
}

// remember the current animation for the view model, in case we get out of sync with
//  server.
static int g_currentanim;

// Change weapon model animation
void HUD_SendWeaponAnim(int iAnim, int body, int force)
{
	// Don't actually change it.
	if (!g_runfuncs && !force)
		return;

	g_currentanim = iAnim;

	// Tell animation system new info
	gEngfuncs.pfnWeaponAnim(iAnim, body);
}

// Retrieve current predicted weapon animation
int HUD_GetWeaponAnim(void)
{
	return g_currentanim;
}

// Play a sound, if we are seeing this command for the first time
void HUD_PlaySound(char *sound, float volume)
{
	if (!g_runfuncs || !g_finalstate)
		return;

	gEngfuncs.pfnPlaySoundByNameAtLocation(sound, volume, (float *)&g_finalstate->playerstate.origin);
}

// Directly queue up an event on the client
void HUD_PlaybackEvent(int flags, const edict_t *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
	if (!g_runfuncs || !g_finalstate)
	     return;

	//vec3_t org;
	//vec3_t ang;
	// Weapon prediction events are assumed to occur at the player's origin
	// XDM3038a: who said that?	org			= g_finalstate->playerstate.origin;
	//	ang			= v_angles;
	gEngfuncs.pfnPlaybackEvent(flags, pInvoker, eventindex, delay, origin, angles, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}

void HUD_SetMaxSpeed(const edict_t *ed, float speed)
{
	// TODO: test	if (ed)
	//		ed->v.maxspeed = speed;
}

// XDM3037a: can get ""
int stub_PrecacheModel(char *s)
{
	return (s && *s != '\0')?gEngfuncs.pEventAPI->EV_FindModelIndex(s):0;
}

int stub_PrecacheSound(char *s)
{
	return 0;
}

void stub_SetModel(edict_t *e, const char *m)
{
	/*if (e) {e->v.model = ALLOC_STRING(m); }*/
	if (e)
		gEngfuncs.CL_LoadModel(m, &e->v.modelindex);
}

int stub_ModelIndex(const char *m)// XDM3037a: can get ""
{
	return (m && *m != '\0')?gEngfuncs.pEventAPI->EV_FindModelIndex(m):0;
}

int stub_IndexOfEdict(const edict_t *pEdict)// XDM3037a
{
	if (pEdict && pEdict->v.flags & FL_CLIENT)
		return gEngfuncs.GetLocalPlayer()->index;

	return 0;
}

edict_t *stub_FindEntityByVars(struct entvars_s *pvars)// XDM3037a
{
	return NULL;
}

const char *stub_NameForFunction(uint32 function)
{
	return "func";
}

//direct pointer
/*int32 stub_RandomLong(int32 lLow, int32 lHigh)
{
	return gEngfuncs.pfnRandomLong(lLow, lHigh);
};*/

//direct pointer unsigned short	stub_PrecacheEvent(int type, const char *s) { return gEngfuncs.pEventAPI->EV_PrecacheEvent(type, s); }// XDM

int stub_AllocString(const char *szValue)
{
	return (szValue - gpGlobals->pStringBase);
};// XDM3037a







// Update weapons from weapon_data_s[]
int UpdateLocalInventory(const struct weapon_data_s *weapondata)
{
	// XDM3038a: replacement for UpdWeapons and WeapPickup, this section only relates to HUD!
	int i;
	int nWeapons;
/* TEST #if defined(_DEBUG)
	nWeapons = 0;
	for (i = 0; i < MAX_WEAPONS; ++i)// Find weapon with this ID in the structure WARNING: i != ID!!!
		if (from->weapondata[i].m_iId != WEAPON_NONE)
			++nWeapons;

	if (nWeapons == 0)
		conprintf(2, "HUD_PostRunCmd() 0 weapons in FROM!\n");

	/* same as below
	nWeapons = 0;
	for (i = 0; i < MAX_WEAPONS; ++i)// Find weapon with this ID in the structure WARNING: i != ID!!!
		if (to->weapondata[i].m_iId != WEAPON_NONE)
			++nWeapons;

	if (nWeapons == 0)
		conprintf(2, "HUD_PostRunCmd() 0 weapons in TO!\n");* /
#endif*/

	HUD_WEAPON *pHUDWeapon;
	const weapon_data_t *pData;
	bool bHadActiveWeapon = (gHUD.m_Ammo.GetActiveWeapon() != WEAPON_NONE);// remember now, before any changes take place
//#if defined(_DEBUG)
	uint32 cl_wpnbits = 0;
//#endif

	// New algorithm: find a weapon ID in the array of incoming data
	nWeapons = 0;
	int iID;
	int iActiveWeaponID = WEAPON_NONE;
	for (iID = WEAPON_NONE+1; iID < MAX_WEAPONS; ++iID)
	{
		pData = NULL;
		for (i = 0; i < MAX_WEAPONS; ++i)// Find weapon with this ID in the structure WARNING: i != ID!!!
			if (weapondata[i].m_iId == iID)
				pData = &weapondata[i];

		if (pData)// found it
		{
			++nWeapons;
//#if defined(_DEBUG)
			SetBits(cl_wpnbits, (1<<pData->m_iId));
//#endif
			pHUDWeapon = gHUD.m_Ammo.gWR.GetWeaponStruct(pData->m_iId);// XDM3038a
			if (pHUDWeapon)
			{
				//DBG_PRINTF("HUD_PostRunCmd(): updating weapon data: %d\n", pData->m_iId);
				pHUDWeapon->iId = pData->m_iId;
				pHUDWeapon->iZoomMode = pData->m_fInZoom;
				pHUDWeapon->iClip = pData->m_iClip;
				pHUDWeapon->iState = pData->m_iWeaponState;
				if (pData->m_iWeaponState == wstate_current ||
					pData->m_iWeaponState == wstate_current_ontarget ||
					pData->m_iWeaponState == wstate_reloading ||
					pData->m_iWeaponState == wstate_holstered)
				{
					iActiveWeaponID = pData->m_iId;
					///ASSERTD(pData->m_iId == from->client.m_iId);
					//if (from->client.m_iId != WEAPON_NONE)// from->client.m_iId == 0 while changing weapons
					//	gHUD.m_Ammo.UpdateCurrentWeapon(pHUDWeapon->iId); we use entity_state_s::m_iId

					///if (pHUDWeapon->iState != wstate_holstered)
					///	bHaveCurrentActiveWeapon = true;
				}
			}
			if (!gHUD.m_Ammo.gWR.HasWeapon(pData->m_iId))// we didn't have it
			{
				DBG_PRINTF("HUD_PostRunCmd(): adding new weapon: %d\n", pData->m_iId);
				gHUD.m_Ammo.WeaponPickup(pData->m_iId);
			}
			//dbg_nwpns++; SetBits(dbg_bits, pData->m_iId);
		}
		else// this weapon was not found
		{
			pHUDWeapon = gHUD.m_Ammo.gWR.GetWeaponStruct(iID);// XDM3038a
			if (pHUDWeapon)
			{
				pHUDWeapon->iZoomMode = 0;
				pHUDWeapon->iClip = 0;
				pHUDWeapon->iState = wstate_unusable;
				if (gHUD.m_Ammo.gWR.HasWeapon(iID))// we had it
				{
					DBG_PRINTF("HUD_PostRunCmd(): removing weapon: %d\n", iID);
					gHUD.m_Ammo.gWR.DropWeapon(pHUDWeapon);
				}
			}
			//else
			//spam	DBG_PRINTF("HUD_PostRunCmd(): got null for weapon: %d\n", i);
		}
	}
	if (nWeapons == 0)// We have no weapons, disable the crosshair
	{
		if (bHadActiveWeapon)
			gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_OFF);
	}
	/* normal during switch if (iActiveWeaponID != WEAPON_NONE)// check if weapon data tells the same as client data
	{
		ASSERTD(iActiveWeaponID == from->client.m_iId);// 26 != 0
	}*/
	/* fail else if (!FBitSet(gHUD.m_iHideHUDDisplay, HIDEHUD_WEAPONS | HIDEHUD_ALL))
	{
		if (!bHadActiveWeapon)
			gHUD.m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);
	}*/
	return nWeapons;
}
