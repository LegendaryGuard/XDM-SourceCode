#ifndef SPECTATORPANEL_H
#define SPECTATORPANEL_H

#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include "vgui_CustomObjects.h"

using namespace vgui;

enum
{
	SPECTATOR_PANEL_CMD_NONE = VGUI_IDLAST,
	SPECTATOR_PANEL_CMD_OPTIONS,
	SPECTATOR_PANEL_CMD_PREVPLAYER,
	SPECTATOR_PANEL_CMD_NEXTPLAYER,
	SPECTATOR_PANEL_CMD_HIDEMENU,
	SPECTATOR_PANEL_CMD_TOGGLE_INSET,
	SPECTATOR_PANEL_CMD_CAMERA,
	SPECTATOR_PANEL_CMD_PLAYERS
};

#define SPECTATORPANEL_HEIGHT				32
#define SPECTATORPANEL_BANNER_WIDTH			256
#define SPECTATORPANEL_BANNER_HEIGHT		64
#define SPECTATORPANEL_SMALL_BUTTON_X		24
#define SPECTATORPANEL_OPTIONS_BUTTON_X		112
#define SPECTATORPANEL_BUTTON_Y				20
#define SPECTATORPANEL_MARGIN				16

#define SPECTATORPANEL_HELP_DISPLAY_TIME	5


class SpectatorPanel : public CMenuPanel//Panel //, public vgui::CDefaultInputSignal
{
	typedef CMenuPanel BaseClass;
public:
	SpectatorPanel(int x, int y, int wide, int tall);
	virtual ~SpectatorPanel();
	virtual bool OnClose(void);
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);// XDM3037a
	virtual void OnActionSignal(int signal, class CMenuPanelActionSignalHandler *pCaller);// XDM3038a

	virtual bool IsPersistent(void) { return true; }// XDM3038
	virtual bool AllowConcurrentWindows(void) { return true; }// XDM3038

	void Initialize(void);
	void Update(void);
	void EnableInsetView(bool isEnabled);
	void ShowMenu(bool bVisible);
	void SetBanner(const char *image);
	int GetBorderHeight(void);

protected:
	CommandButton	m_OptionButton;
	CommandButton	m_PrevPlayerButton;
	CommandButton	m_NextPlayerButton;
	CommandButton	m_CamButton;	
	CommandButton	m_BottomMainButton;// HL20130901
	CommandButton	m_InsetViewButton;
	CPlayersCommandMenu	*m_pPlayersMenu;// XDM3038c

	Panel			m_TopBorder;// Top black line (letterboxing). XDM3038a: not a pointer anymore
	Panel			m_BottomBorder;// Bottom black line (menu buttons are drawn on it)
	Label			m_TopMainLabel;// XDM: title label
	Label			m_ExtraInfo;// text above timer
	Label			m_CurrentTime;// next to timer image
	Label			m_TeamScores[MAX_TEAMS];// top frame, near timer
	CImageLabel		*m_TimerImage;// @top right
	CImageLabel		*m_TopBanner;// top frame

	::Color			m_BottomMainButtonFg;
	float			m_fHideHelpTime;

public:
	bool			m_menuVisible;// buttons and menus at the bottom are currently visible
	bool			m_insetVisible;// inset view (overview window) is currently visible
};


class CMenuHandler_SpectateFollow : public ActionSignal
{
protected:
	CLIENT_INDEX m_iPlayerIndex;

public:
	CMenuHandler_SpectateFollow(CLIENT_INDEX iPlayerIndex)
	{
		//strncpy(m_szplayer, player, MAX_COMMAND_SIZE);
		//m_szplayer[MAX_COMMAND_SIZE-1] = '\0';
		m_iPlayerIndex = iPlayerIndex;
	}

	virtual void actionPerformed(Panel *panel)
	{
		gHUD.m_Spectator.FindPlayer(m_iPlayerIndex);
		gViewPort->HideCommandMenu();
	}
};

#endif // !defined SPECTATORPANEL_H
