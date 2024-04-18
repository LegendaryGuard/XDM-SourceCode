#include "hud.h"
#include "cl_util.h"
#include "in_defs.h"
#include "bench.h"

cl_entity_t *g_pWorld = NULL;

//-----------------------------------------------------------------------------
// Purpose: called every time shared client dll/engine data gets changed,
// Input  : *cdata - 
//			time - current time
// Output : return 1 if in anything in the client_data struct has been changed, 0 otherwise
//-----------------------------------------------------------------------------
int CHud::UpdateClientData(client_data_t *cdata, const float &time)
{
	//m_vecOrigin = cdata->origin;// XDM: why here?
	//m_vecAngles = cdata->viewangles;

	m_iKeyBits = CL_ButtonBits(0);
	// XDM3038: wrong, this is INPUT.	m_iWeaponBits = cdata->iWeaponBits;

	//in_fov = cdata->fov;

	Think();

	// XDM3037a: Server-set FOV has highest priority
	cdata->fov = GetCurrentFOV();

	//v_idlescale = m_iConcussionEffect;

	CL_ResetButtonBits(m_iKeyBits);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: update data, no drawing
//-----------------------------------------------------------------------------
void CHud::Think(void)
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	if (m_iActive == 0 || m_flTimeDelta == 0.0)// XDM3037: a little hack to avoid xash3d crash
		return;

	HUDLIST *pList = m_pHudList;
	while (pList)
	{
		if (pList->p->IsActive())
			pList->p->Think();
		pList = pList->pNext;
	}

	if (gEngfuncs.IsSpectateOnly())
	{
		m_fFOV = gHUD.m_Spectator.GetFOV();	// default_fov->value;
	}
	else
	{
		float fNewFOV = HUD_GetFOV();
		/*if (fNewFOV == 0)
		{
			m_fFOV = GetUpdatedDefaultFOV();// XDM3037a
		}
		else
		{*/
			if (m_fFOV != fNewFOV)
			{
				m_fFOV = fNewFOV;
				m_flMouseSensitivity = GetSensitivityModByFOV(m_fFOV);// XDM
			}
		//}
	}

	if (gEngfuncs.GetMaxClients() > 1)// XDM3037: multiplayer
		CheckRemainingTimeAnnouncements();

	//test	gHUD.m_iDistortMode = test1->value;
	//	gHUD.m_fDistortValue = test2->value;

#if defined (ENABLE_BENCKMARK)
	Bench_CheckStart();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: called by CHud::Init() once on game start, MsgFunc_ResetHUD() on level start
// Input  : bForce - ignore HUD_NORESET flags (e.g. during initialization)
//-----------------------------------------------------------------------------
void CHud::Reset(bool bForce)
{
	//precaution?	if (m_iActive)
	g_pWorld = gEngfuncs.GetEntityByIndex(0);

	if (m_iGameType != GT_SINGLE)// XDM3037: change levels without resetting this data
	{
		DistortionReset();
		m_bFrozen = 0;
	}
	m_flNextSuitSoundTime = 0.0f;// XDM3038

	// clear all hud data
	HUDLIST *pList = m_pHudList;
	while (pList)
	{
		if (pList->p && (bForce || !FBitSet(pList->p->m_iFlags, HUD_NORESET)))
			pList->p->Reset();
		pList = pList->pNext;
	}

	byte r,g,b;
	if (StringToRGB(m_pCvarColorMain->string, r,g,b))
		m_iDrawColorMain = RGB2INT(r,g,b);
	else
		m_iDrawColorMain = 0x7F7F7F7F;//RGB2INT(127,127,127);// 0xFF7F7F7F?

	if (StringToRGB(m_pCvarColorRed->string, r,g,b))
		m_iDrawColorRed = RGB2INT(r,g,b);
	else
		m_iDrawColorRed = 0x7F7F7F7F;

	if (StringToRGB(m_pCvarColorBlue->string, r,g,b))
		m_iDrawColorBlue = RGB2INT(r,g,b);
	else
		m_iDrawColorBlue = 0x7F7F7F7F;

	if (StringToRGB(m_pCvarColorCyan->string, r,g,b))
		m_iDrawColorCyan = RGB2INT(r,g,b);
	else
		m_iDrawColorCyan = 0x7F7F7F7F;

	if (StringToRGB(m_pCvarColorYellow->string, r,g,b))
		m_iDrawColorYellow = RGB2INT(r,g,b);
	else
		m_iDrawColorYellow = 0x7F7F7F7F;

	// NONONO!	m_iTimeLeftLast = 0;
	//NO!			m_iScoreLeftLast = 0;
	//NO!	m_flNextAnnounceTime = 0.0f;
	//NO!!	m_flMouseSensitivity = 0.0f;
	//	m_iConcussionEffect = 0;
	//	m_iFogMode = 0;
	//	ResetFog();// XDM: don't clear the fog here.
	//	if (m_iGameType == GT_SINGLE)// XDM3035b: single in trouble
	//		CL_Precache();
}
