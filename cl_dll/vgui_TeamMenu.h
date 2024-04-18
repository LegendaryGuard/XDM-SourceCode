#ifndef VGUI_TEAMMENU_H
#define VGUI_TEAMMENU_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "vgui_CustomObjects.h"

// Team Menu
/* old
#define TMENU_INDENT_X			(30 * ((float)ScreenHeight / 640))
#define TMENU_HEADER			100
#define TMENU_SIZE_X			(ScreenWidth - (TMENU_INDENT_X * 2))
#define TMENU_SIZE_Y			(TMENU_HEADER + BUTTON_SIZE_Y * 7)
#define TMENU_PLAYER_INDENT		(((float)TMENU_SIZE_X / 3) * 2)
#define TMENU_INDENT_Y			(((float)ScreenHeight - TMENU_SIZE_Y) / 2)
*/

// Team Menu Dimensions
#define TEAMMENU_TITLE_X				XRES(40)
#define TEAMMENU_TITLE_Y				YRES(32)
#define TEAMMENU_TOPLEFT_BUTTON_X		XRES(40)
#define TEAMMENU_TOPLEFT_BUTTON_Y		YRES(80)
#define TEAMMENU_BUTTON_SIZE_X			XRES(124)
#define TEAMMENU_BUTTON_SIZE_Y			YRES(24)
#define TEAMMENU_BUTTON_SPACER_Y		YRES(8)
#define TEAMMENU_WINDOW_X				XRES(176)
#define TEAMMENU_WINDOW_Y				YRES(80)
#define TEAMMENU_WINDOW_SIZE_X			XRES(424)
#define TEAMMENU_WINDOW_SIZE_Y			YRES(312)
#define TEAMMENU_WINDOW_TITLE_X			XRES(16)
#define TEAMMENU_WINDOW_TITLE_Y			YRES(16)
#define TEAMMENU_WINDOW_TEXT_X			XRES(16)
#define TEAMMENU_WINDOW_TEXT_Y			YRES(40)
#define TEAMMENU_WINDOW_TEXT_SIZE_Y		YRES(191)
#define TEAMMENU_WINDOW_INFO_X			XRES(16)
#define TEAMMENU_WINDOW_INFO_Y			YRES(240)

#define NUM_TEAM_BUTTONS				MAX_TEAMS + 1// XDM: +auto
#define	TBUTTON_AUTO					0// XDM: TEAM_NONE
#define	TBUTTON_SPECTATE				NUM_TEAM_BUTTONS// last one

class CTeamMenuPanel : public CMenuPanel
{
	typedef CMenuPanel BaseClass;
public:
	CTeamMenuPanel(int iRemoveMe, int x, int y, int wide, int tall);

//old shit	virtual bool SlotInput(int iSlot);
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);
	virtual void Open(void);
	virtual void Update(void);
	virtual void SetActiveInfo(int iInput);
	virtual void paintBackground(void);
	virtual void Initialize(void);
	virtual void Reset(void);
	virtual bool OnClose(void);// XDM3038
	virtual bool IsPersistent(void);// XDM3038
	virtual bool AllowConcurrentWindows(void);// XDM3038

protected:
	CCustomScrollPanel	m_ScrollPanel;// XDM3038a
	Panel				m_TeamWindow;
	Label				m_LabelTitle;// XDM3038
	Label				m_MapTitle;// XDM3038
	CDefaultTextPanel	m_Briefing;
	TextPanel			*m_pTeamInfoPanel[NUM_TEAM_BUTTONS];
	CommandButton		*m_pButtons[NUM_TEAM_BUTTONS];
	CommandButton		*m_pCancelButton;
	CommandButton		*m_pSpectateButton;

	int					m_iCurrentInfo;
	bool				m_bUpdatedMapName;
};

#endif // VGUI_TEAMMENU_H
