#ifndef XDMVIEWPORT_H
#define XDMVIEWPORT_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

// XDM: OMFD! This all code is so dirty... Normal people should rewrite all of this from scratch.

#include <VGUI_App.h>
#include <VGUI_MiniApp.h>

#include <VGUI_Panel.h>
#include <VGUI_Frame.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>

#include <VGUI_Image.h>
#include <VGUI_BitmapTGA.h>
//#include <VGUI_DesktopIcon.h>
#include <VGUI_Cursor.h>

#include <VGUI_Scheme.h>
//#include <VGUI_FileInputStream.h>

#include <VGUI_ActionSignal.h>
#include <VGUI_FocusChangeSignal.h>
#include "vgui_defaultinputsignal.h"

//#include <VGUI_String.h>
#include <VGUI_TextPanel.h>
#include <VGUI_ScrollPanel.h>
#include <VGUI_ScrollBar.h>
#include <VGUI_Slider.h>
#include <VGUI_LineBorder.h>

#include "vgui_SchemeManager.h"// custom scheme handling
//#include "vgui_CustomObjects.h"

#if defined(_DEBUG_GUI)
#define DBG_PRINT_GUI						DBG_PrintF
#else
#define DBG_PRINT_GUI
#endif

using namespace vgui;

class Cursor;
class ScorePanel;
class SpectatorPanel;
//class CCommandMenu;
//class CommandLabel;
//class CommandButton;
class CMenuPanel;
class ServerBrowser;
//class DragNDropPanel;
class CTeamMenuPanel;
class CMusicPlayerPanel;// XDM
class CImageLabel;
class CommandButton;
class CCommandMenu;

//char* GetVGUITGAName(const char *pszName);
BitmapTGA *LoadTGAForRes(const char *pImageName);

#define MAX_SERVERNAME_LENGTH	32
//#define MAX_MAP_NAME				256

// Command Menu positions 
#define MAX_MENUS				80

#define PANEL_DEFAULT_ALPHA		127

#define PANEL_INNER_OFFSET		2
//#define PANEL_TITLE_POS_X		XRES(PANEL_INNER_OFFSET)
//#define PANEL_TITLE_POS_Y		XRES(PANEL_INNER_OFFSET)

#define BUTTON_SIZE_X			XRES(112)
#define BUTTON_SIZE_Y			YRES(24)

#define TEXTENTRY_TITLE_SIZE_X	XRES(100)
#define TEXTENTRY_SIZE_Y		YRES(20)

#define CBUTTON_SIZE_Y			YRES(16)
#define CMENU_SIZE_X			XRES(160)

#define SUBMENU_SIZE_X			(CMENU_SIZE_X / 8)
#define SUBMENU_SIZE_Y			(BUTTON_SIZE_Y / 6)

#define CMENU_TOP				(BUTTON_SIZE_Y * 4)

enum
{
	CMENU_DIR_DOWN = 0,
	CMENU_DIR_UP,
	CMENU_DIR_AUTO//5// undone
};

// CreateTextWindow
enum
{
	SHOW_NOTHING = 0,
	SHOW_MAPBRIEFING,
	SHOW_MOTD,
	SHOW_MOTD_MANUAL,
	SHOW_SPECHELP
};

//-----------------------------------------------------------------------------
// Purpose: Wrapper for an Image Label without a background
//-----------------------------------------------------------------------------
class CImageLabel : public Label
{
public:
	CImageLabel();
	CImageLabel(const char *pImageName, int x, int y);
	CImageLabel(const char *pImageName, int x, int y, int wide, int tall);

	bool LoadImage(const char *pImageName, bool fullpath = false);
	virtual void getImageSize(int &wide, int &tall);
//	virtual void getImageCenter(int &x, int &y);

public:
	Bitmap	*m_pImage;
};



//-----------------------------------------------------------------------------
// Purpose: InputSignal handler for the main viewport
// Everything you see here will be called ALWAYS disregarding which panels
// are currently shown e.g. if you click outside windows too.
//-----------------------------------------------------------------------------
class CViewPortInputHandler : public CDefaultInputSignal
{
public:
	virtual void mousePressed(MouseCode code, Panel *panel);
//	virtual void mouseDoublePressed(MouseCode code, Panel *panel);
};


//-----------------------------------------------------------------------------
// Purpose: The main UI class
// Virtually persists all the time the client process is running
//-----------------------------------------------------------------------------
class CViewport : public Panel
{
	typedef Panel BaseClass;
public:
	void *operator new(size_t stAllocateBlock);

	CViewport(int x,int y,int wide,int tall);
	void Initialize(void);
	void LoadScheme(void);

	void StartFrame(const double &time);// XDM3038a
	void UpdateCursorState(void);
	//void UpdateCommandMenu(int menuIndex);
	//void UpdatePlayerMenu(int menuIndex);// HL20130901
	void UpdateOnPlayerInfo(void);
	void UpdateHighlights(void);
	void UpdateSpectatorPanel(void);

	int	KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);
	//void InputPlayerSpecial(void);
	// XDM3037a: moved to util	void GetAllPlayersInfo(void);
	void DeathMsg(short killer, short victim);

	int CreateCommandMenu(char *menuFile, int direction, int yOffset, int design, int flButtonSizeX, int flButtonSizeY, int xOffset);
	void ShowCommandMenu(int menuIndex, bool bParseUserCmdArgs);
	void ShowCommandMenu(CCommandMenu *pNewMenu, bool bParseUserCmdArgs);
	void InputSignalHideCommandMenu(void);
	void HideCommandMenu(void);
	void OnCommandMenuClosed(CCommandMenu *pMenu);
	void SetCurrentCommandMenu(CCommandMenu *pNewMenu);
	void SetCurrentMenu(CMenuPanel *pMenu);

	CMenuPanel *ShowScoreBoard(void);
	void HideScoreBoard(void);
	bool IsScoreBoardVisible(void);
	//void ShowInventoryPanel(void);// XDM

	bool AllowedToPrintText(void);

	bool IsMenuPanelOpened(CMenuPanel *pMenuPanel);
	bool CloseMenuPanel(CMenuPanel *pMenuPanel);
	CMenuPanel *FindMenuPanel(int iMenu);
	CMenuPanel *ShowMenuPanel(int iMenu);
	CMenuPanel *ShowMenuPanel(CMenuPanel *pNewMenu, bool bForceOnTop);

	void OnMenuPanelClose(CMenuPanel *pMenu);
	void OnGameActivated(const int &active);

	void HideVGUIMenu(void);
	void HideTopMenu(void);

	void ToggleServerBrowser(void);
	//void ToggleInventoryPanel(void);// XDM
	void ToggleMusicPlayer(void);

	CMenuPanel *CreateTextWindow(int iTextToShow);// SHOW_MAPBRIEFING
	CCommandMenu *CreateSubMenu(CommandButton *pButton, CCommandMenu *pParentMenu, int iYOffset, int iXOffset = 0);
	bool IsCommandMenuOpened(int menuindex);//CCommandMenu *pMenu);

	// Data Handlers
	//const byte GetNumberOfTeams(void) const { return m_iNumberOfTeams; }
	const char *GetMOTD(void);// XDM
	const char *GenerateFullIntroText(bool bShowIntro, bool bShowMOTD, bool bShowMapText);// XDM3038a

	bool GetAllowSpectators(void);

	// Message Handlers
	//int MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ShowMenu(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf);
	//int MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf);
	//int MsgFunc_Spectator(const char *pszName, int iSize, void *pbuf);
	// OBSOLETE	int MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ResetFade( const char *pszName, int iSize, void *pbuf );// HL20130901
	int MsgFunc_SpecFade( const char *pszName, int iSize, void *pbuf );

	// Input
	virtual void paintBackground();

	CSchemeManager *GetSchemeManager(void) { return &m_SchemeManager; }
	ScorePanel *GetScoreBoard(void) { return m_pScoreBoard; }
	CMusicPlayerPanel *GetMusicPlayer(void)  { return m_pMusicPlayer; }
	SpectatorPanel *GetSpectatorPanel(void)  { return m_pSpectatorPanel; }

protected:
	void CreateScoreBoard(void);

public:
	int			m_StandardMenu;	// indexes in m_pCommandMenus
	int			m_SpectatorOptionsMenu;
	int			m_SpectatorCameraMenu;
	//int			m_PlayerMenu; // HL20130901 a list of current players

	int			m_iBorderThickness;
	::Color		m_BorderColor;
	::Color		m_BorderColorArmed;
	char		m_szServerName[MAX_SERVERNAME_LENGTH];

protected:
	int			m_iInitialized;

	int			m_iGotAllMOTD;
	char		m_szMOTD[MAX_MOTD_LENGTH];

	float		m_flMenuOpenTime;
	float		m_flScoreBoardLastUpdated;
	float		m_flSpectatorPanelLastUpdated;

	int			m_iNumMenus;
	int			m_iCurrentTeamNumber;
	int			m_iUser1;
	int			m_iUser2;
	int			m_iUser3;

	// Scheme handler
	CSchemeManager m_SchemeManager;

	vgui::Cursor* _cursorNone;
	vgui::Cursor* _cursorArrow;
	CCommandMenu *m_pCommandMenus[MAX_MENUS];
	CCommandMenu *m_pCurrentCommandMenu;

	// VGUI Menus
//?	Dar<CMenuPanel *>	m_Panels;
	CMenuPanel			*m_pCurrentMenuPanel;
	ScorePanel			*m_pScoreBoard;
	CMusicPlayerPanel	*m_pMusicPlayer;
	CTeamMenuPanel		*m_pTeamMenuPanel;
	SpectatorPanel		*m_pSpectatorPanel;
	ServerBrowser		*m_pServerBrowser;
};












//============================================================
// Panel that uses XDM (non-vgui) schemes
class CSchemeEnabledPanel : public Panel
{
	typedef Panel BaseClass;
public:
	CSchemeEnabledPanel(CScheme *pScheme, int x, int y, int wide, int tall);
	virtual void paintBackground(void);

	virtual CScheme *getScheme(void);
	virtual void setScheme(CScheme *pScheme);

	void ApplyScheme(CScheme *pScheme, Panel *pControl);

protected:
	CScheme		*m_pScheme;
};


//============================================================
// Panel that can be dragged around
class DragNDropPanel : public Panel// TODO: public CSchemeEnabledPanel
{
private:
	typedef Panel BaseClass;
public:
	DragNDropPanel(bool dragenabled, int x, int y, int wide, int tall);
	virtual void setPos(int x, int y);// safe version
	virtual void setDragged(bool bState);
	virtual void setDragEnabled(bool enable);
	virtual bool getDragEnabled(void);
	int GetX(void) { return _pos[0]; }
	int GetY(void) { return _pos[1]; }

protected:
//	LineBorder	*m_pDragBorder;
//	Border		*m_pOriginalBorder;
	int			m_OriginalPos[2];// set on creation
	bool		m_bBeingDragged;
	bool		m_bDragEnabled;
};


enum
{
	BG_FILL_NONE = 0,
	BG_FILL_NORMAL,
	BG_FILL_SCREEN
};


//============================================================
// Menu Panel that supports buffering of menus
// main class for all windows in XHL
class CMenuPanel : public DragNDropPanel
{
public:
	typedef DragNDropPanel BaseClass;

	CMenuPanel(int RemoveMe, int x, int y, int wide, int tall);
	virtual ~CMenuPanel();
	virtual void Open(void);
	virtual void Close(void);
	virtual void Reset(void);

	virtual bool OnClose(void);
	virtual void OnActionSignal(int signal, class CMenuPanelActionSignalHandler *pCaller);

	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);

	virtual void getPersistanceText(char *buf, int bufLen);// HACK: kind of RTTI replacement
	virtual void paintBackground(void);
	virtual void setCaptureInput(bool enable);
	virtual void setShowCursor(bool show);
	virtual int getBackgroundMode(void);// BG_FILL_NONE, etc.
	virtual void setBackgroundMode(int BackgroundMode);

	virtual bool IsCapturingInput(void);
	virtual bool IsShowingCursor(void);

	virtual bool IsPersistent(void) { return false; }// XDM3038: don't close when other windows pop up
	virtual bool AllowConcurrentWindows(void) { return true; }// XDM3038: allow new windows to open (even in background), or close all other windows when opening
//	virtual bool AllowOthers(void) { return true; }// XDM3038: allow other menus to open without closing this one

	void SetNextMenu(CMenuPanel *pNextPanel);// TODO: bool ?
	inline int ShouldBeRemoved(void) { return m_iRemoveMe; }
	inline void SetMenuID(int iID) { m_iMenuID = iID; }
	inline void SetActive(int iState) { m_iIsActive = iState; }
	inline void SetCloseDelay(float fCloseDelay) { m_flCloseDelay = fCloseDelay; }// XDM3038a
	inline CMenuPanel *GetNextMenu(void) { return m_pNextMenu; }
	inline int GetMenuID(void) { return m_iMenuID; }
	inline int IsActive(void) { return m_iIsActive; }
	inline float GetOpenTime(void) { return m_flOpenTime; }

	// Numeric input
	virtual void SetActiveInfo(int iInput) {}

	CImageLabel		m_TitleIcon;// TODO: button
protected:
//	Label			m_LabelTitle;
	CMenuPanel		*m_pNextMenu;
	int				m_iMenuID;
	int				m_iRemoveMe;
	int				m_iIsActive;
	int				m_iBackgroundMode;// BG_FILL_NONE 0 - none, 1 - normal, 2 - fullscreen
	float			m_flOpenTime;
	float			m_flCloseDelay;// XDM3038a
	bool			m_bCaptureInput;
	bool			m_bShowCursor;
//	Dar<ActionSignal *>	_OnCloseActionSignalDar;
};


enum ActionSignalSound_e
{
	ACTSIGSOUND_DEFAULT = 0,
	ACTSIGSOUND_SUCCESS,
	ACTSIGSOUND_FAILURE,
	ACTSIGSOUND_NONE
};

// Simple int signal handler
class CMenuPanelActionSignalHandler : public ActionSignal
{
	typedef ActionSignal BaseClass;
public:
	CMenuPanelActionSignalHandler(CMenuPanel *pPanel, int iSignal);
	virtual void actionPerformed(Panel *panel);

	int			m_iSoundType;
private:
	CMenuPanel	*m_pPanel;
	int			m_iSignal;
};

// XDM3038a: use to automatically close menus when they lose focus
/* HL VGUI BUGS make this unusable :( class CFocusChangeSignalClose : public FocusChangeSignal
{
public:
	CFocusChangeSignalClose(CMenuPanel *pPanel);
	virtual void focusChanged(bool lost, Panel *panel);

private:
	CMenuPanel *m_pPanel;
};*/

enum
{
	VGUI_MB_OK = 0,
	VGUI_MB_OKCANCEL,
	VGUI_MB_ABORTRETRYIGNORE,
	VGUI_MB_YESNOCANCEL,
	VGUI_MB_YESNO,
	VGUI_MB_RETRYCANCEL
};

// Do not change!
enum
{
	VGUI_IDNOTHING = 0,
	VGUI_IDOK,
	VGUI_IDCANCEL,
	VGUI_IDABORT,
	VGUI_IDRETRY,
	VGUI_IDIGNORE,
	VGUI_IDYES,
	VGUI_IDNO,
	VGUI_IDCLOSE,
	VGUI_IDHELP,
	VGUI_IDLAST
};

class CDialogPanel : public CMenuPanel
{
	typedef CMenuPanel BaseClass;
public:
	CDialogPanel(int RemoveMe, int x, int y, int wide, int tall, unsigned long type);
	virtual void Reset(void);
	virtual void Close(void);

	virtual bool OnClose(void);
	virtual void OnActionSignal(int signal, class CMenuPanelActionSignalHandler *pCaller);// XDM3038a
	virtual void OnOK(void);
	virtual void OnCancel(void);
	virtual bool DoExecCommand(void);

	unsigned long GetResult(void) { return m_iSignalRecieved; }

	CommandButton *m_pButtons;
	unsigned long m_iNumDialogButtons;
	unsigned long m_iSignalRecieved;
};




extern CViewport *gViewPort;

#endif // XDMVIEWPORT_H
