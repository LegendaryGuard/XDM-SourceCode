#ifndef VGUI_CUSTOMOBJECTS_H
#define VGUI_CUSTOMOBJECTS_H

#include <VGUI_Panel.h>
#include <VGUI_Frame.h>
#include <VGUI_TextPanel.h>
#include <VGUI_TextEntry.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_InputSignal.h>
#include <VGUI_Image.h>
#include <VGUI_BitmapTGA.h>
#include <VGUI_LineBorder.h>
#include <VGUI_ScrollPanel.h>
#include <VGUI_ScrollBar.h>
#include <VGUI_Slider.h>
#include "vgui_SchemeManager.h"
#include "vgui_defaultinputsignal.h"

using namespace vgui;

class CViewport;
class CCommandMenu;
class CMenuPanel;
class DragNDropPanel;
//class ActionSignal;

// Arrows
enum
{
	ARROW_UP,
	ARROW_DOWN,
	ARROW_LEFT,
	ARROW_RIGHT,
};

#define MAX_BUTTON_TEXTLEN		48
#define MAX_BUTTON_TIPLEN		64// XDM3037a: TODO
#define MAX_BUTTONS				100



// Default panel implementation doesn't forward mouse messages when there is no cursor and we need them.
class CHitTestPanel : public Panel
{
public:
	virtual void internalMousePressed(MouseCode code);
};


//============================================================
// Labels
//============================================================


//-----------------------------------------------------------------------------
// Purpose: Overridden label so we can darken it when submenus open
//-----------------------------------------------------------------------------
class CommandLabel : public Label
{
public:
	CommandLabel(const char *text, int x,int y, int wide,int tall) : Label(text,x,y,wide,tall)
	{
		m_iState = false;
	}

	void PushUp(void)
	{
		m_iState = false;
		repaint();
	}
	void PushDown(void)
	{
		m_iState = true;
		repaint();
	}

private:
	int		m_iState;
};




//============================================================
// Command Buttons
// Concept: these only DISPLAY some possible actions
//============================================================

//-----------------------------------------------------------------------------
// Purpose: Special button for command menu
//-----------------------------------------------------------------------------
class CommandButton : public Button
{
public:
	CommandButton(void);
	//CommandButton(const char *text, int x, int y, int wide, int tall, bool bNoHighlight = false);
	CommandButton(const char *text, int x, int y, int wide, int tall, bool bNoHighlight = false, bool bFlat = false, bool bShowHotKey = true, const char *iconfile = NULL, CScheme *pScheme = NULL);

	virtual void Init(void);
	virtual void SetScheme(CScheme *pLocalScheme);// XDM3037
	virtual int IsNotValid(void) { return false; }

	// Overloaded vgui functions
	virtual void paint(void);
	// RECURSION! VGUI class overload will call itself! virtual void setText(int textBufferLen,const char* text);// exactly same as in vgui::Label
	// RECURSION! virtual void setText(const char* format,...);
	// TODO: override these so hard that nobody will ever get to Label or Panel::*
	//virtual void setFgColor(int r,int g,int b,int a) { m_ColorNormal.SetColor(r,g,b,a); }
	//virtual void setBgColor(int r,int g,int b,int a) {etc.}
	virtual void paintBackground(void);

	virtual void cursorEntered(void);
	virtual void cursorExited(void);
	virtual void OnActionPerformed(class CMenuActionHandler *pCaller);

	const int &GetX(void) { return _pos[0]; }
	const int &GetY(void) { return _pos[1]; }
	const int &GetLocX(void) { return _loc[0]; }
	const int &GetLocY(void) { return _loc[1]; }

	// Menu Handling
	void AddSubMenu(CCommandMenu *pNewMenu);
	void AddSubLabel(CommandLabel *pSubLabel);
	void UpdateSubMenus(int iAdjustment);
	CCommandMenu *GetSubMenu(void) { return m_pSubMenu; }
	CCommandMenu *getParentMenu(void);
	void setParentMenu(CCommandMenu *pParentMenu);
	void setBoundKey(int boundKey);
	int getBoundKey(void);
	void SetIcon(BitmapTGA *pIcon);
	void LoadIcon(const char *pFileName, bool fullpath = false);
	void setMainText(const char *text);
	void setToolTip(const char *text);// XDM3037a
	void RecalculateText(void);

public:
	::Color m_ColorNormal;
	::Color m_ColorArmed;
	::Color m_ColorSelected;
	::Color m_ColorBgNormal;
	::Color m_ColorBgArmed;
	::Color m_ColorBgSelected;
	::Color m_ColorBorderNormal;
	::Color m_ColorBorderArmed;
	unsigned long m_ulID;// for dialogs
	bool m_bNoHighlight;
	bool m_bShowHotKey;

protected:
	// Submenus under this button
	CCommandMenu	*m_pSubMenu;
	CCommandMenu	*m_pParentMenu;
	CommandLabel	*m_pSubLabel;
	BitmapTGA		*m_pIcon;

	int m_iBoundKey;// XDM3038c: to avoid CRT conflicts
	char m_sMainText[MAX_BUTTON_TEXTLEN];// this is the text provided by the user, the internal Label::text will additionally include hotkey and number
	//char m_sTipText[MAX_BUTTON_TIPLEN];// XDM3037a: TODO: add ToolTipHandler, that will display this text when mouse is over this button
	bool m_bFlat;
	bool m_bTipVisible;
};


//-----------------------------------------------------------------------------
// Purpose: CommandButton that has on/off state
//-----------------------------------------------------------------------------
class ToggleCommandButton : public CommandButton, public CDefaultInputSignal
{
public:
	//ToggleCommandButton(struct cvar_s *pCVar, const char *text, int x, int y, int wide, int tall, bool flat, bool bShowHotKey);
	//ToggleCommandButton(const char *cvarname, const char *text, int x, int y, int wide, int tall, bool flat, bool bShowHotKey);
	ToggleCommandButton(bool state, const char *text, int x, int y, int wide, int tall, bool flat, bool bShowHotKey, CScheme *pScheme = NULL);
	//virtual void cursorEntered(Panel *panel);
	//virtual void cursorExited(Panel *panel);
	virtual void mousePressed(MouseCode code, Panel *panel);
	virtual void paint(void);
	virtual void OnActionPerformed(class CMenuActionHandler *pCaller);
	virtual bool GetState(void);
	virtual void SetState(bool bState);

private:
//	struct cvar_s *m_pCVar;
	CImageLabel *pLabelOn;
	CImageLabel *pLabelOff;
	bool m_bState;
};


//-----------------------------------------------------------------------------
// Purpose: Special button for spectator menu
//-----------------------------------------------------------------------------
/* OBSOLETE class SpectateButton : public CommandButton
{
public:
	SpectateButton(const char *text,int x,int y,int wide,int tall, bool bNoHighlight) : CommandButton(text,x,y,wide,tall, bNoHighlight)
	{
	}
	virtual int IsNotValid();
};*/


//-----------------------------------------------------------------------------
// Purpose: Special button which is only valid for specified map
//-----------------------------------------------------------------------------
class MapButton : public CommandButton
{
public:
	MapButton(const char *pMapName, const char* text,int x,int y,int wide,int tall);
	virtual int IsNotValid(void);

private:
	char m_szMapName[MAX_MAPPATH];
};


//-----------------------------------------------------------------------------
// Purpose: CommandButton which is only displayed if the player is on team X
//-----------------------------------------------------------------------------
class TeamOnlyCommandButton : public CommandButton
{
public:
	TeamOnlyCommandButton(int iTeamNum, const char *text, int x, int y, int wide, int tall, bool flat, bool bShowHotKey) : CommandButton(text, x, y, wide, tall, false, flat, bShowHotKey)
	{
		m_iTeamNum = iTeamNum;
	}

	virtual int IsNotValid(void)
	{
		if (gHUD.m_iTeamNumber != m_iTeamNum)
			return true;

		return CommandButton::IsNotValid();
	}

private:
	int m_iTeamNum;
};



//============================================================
// Command Menus
//============================================================

//-----------------------------------------------------------------------------
// Purpose: menu
//-----------------------------------------------------------------------------
class CCommandMenu : public Panel//CMenuPanel: this class is too huge
{
public:
	//static CCommandMenu *CreateCommandMenu(char *menuFile, int direction, int yOffset, bool flatDesign, int flButtonSizeX, int flButtonSizeY, int xOffset, int iMaxMenus);
	CCommandMenu(CCommandMenu *pParentMenu, int x, int y, int wide, int tall);
	CCommandMenu(CCommandMenu *pParentMenu, int direction, int x, int y, int wide, int tall);
	virtual void Initialize(void);
	virtual void Open(void);
	virtual void Close(void);
	virtual void Update(void);
	virtual void UpdateOnPlayerInfo(int player) {}// 0 == all
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);

	void AddButton(CommandButton *pButton);
	void RemoveAllButtons(void);// HL20130901
	bool RecalculateVisibles(int iNewYPos, bool bHideAll);
	void RecalculatePositions(int iYOffset);
	void MakeVisible(CCommandMenu *pChildMenu);
	void ClearButtonsOfArmedState(void);

	CCommandMenu *GetParentMenu(void) { return m_pParentMenu; }
	bool IsOpened(void) { return isVisible(); }
	int GetX(void) { return _pos[0]; }
	int GetY(void) { return _pos[1]; }
	int GetXOffset(void) { return m_iXOffset; }
	int GetYOffset(void) { return m_iYOffset; }
	int GetDirection(void) { return m_iDirection; }
	uint32 GetNumButtons(void) { return m_iButtons; }
	CommandButton *FindButtonWithSubmenu(CCommandMenu *pSubMenu);

	int m_iButtonSizeY;
protected:
	int				m_iXOffset;
	int				m_iYOffset;
	CCommandMenu	*m_pParentMenu;
	CommandButton	*m_aButtons[MAX_BUTTONS];
	SchemeHandle_t	m_hCommandMenuScheme;
	uint32			m_iButtons;
	int				m_iDirection; // opens menu from top to bottom (0 = default), or from bottom to top (1)?
};


//-----------------------------------------------------------------------------
// Purpose: Special menu with buttons representing players
// pParentPanel recieves ActionSignal with value == iPlayerSignalOffset + player index
//-----------------------------------------------------------------------------
class CPlayersCommandMenu : public CCommandMenu
{
public:
	CPlayersCommandMenu(CCommandMenu *pParentMenu, CMenuPanel *pParentPanel, bool bIncludeSpectators, bool bIncludeLocal, bool bIncludeEnemies, int iPlayerSignalOffset, int direction, int x, int y, int wide, int tall);
	virtual void Initialize(void);
	virtual void UpdateOnPlayerInfo(int player);
protected:
	CMenuPanel *m_pParentPanel;
	int m_iPlayerSignalOffset;// recieving CMenuPanel should expect player indexes start from this value+i
	bool m_bIncludeSpectators;
	bool m_bIncludeLocal;
	bool m_bIncludeEnemies;
};



//============================================================
// ActionSignals
// Concept: these really PERFORM actions and signal their parents
//============================================================

// Command Menu Button Handlers
#define MAX_COMMAND_SIZE	256

//-----------------------------------------------------------------------------
// Purpose: Base action handler for all menus
//-----------------------------------------------------------------------------
class CMenuActionHandler : public ActionSignal
{
public:
	CMenuActionHandler(void);
	CMenuActionHandler(CCommandMenu	*pParentMenu, int iSignal, bool bCloseMenu);
	virtual void actionPerformed(Panel *panel);

	int				m_iSignal;
protected:
	CCommandMenu	*m_pParentMenu;// close on fire
	bool			m_bCloseMenu;
};

//-----------------------------------------------------------------------------
// Purpose: Special action handler for console command menu
//-----------------------------------------------------------------------------
class CMenuHandler_StringCommand : public CMenuActionHandler
{
public:
	CMenuHandler_StringCommand(CCommandMenu *pParentMenu, const char *pszCommand, bool bCloseMenu = false);
	virtual void actionPerformed(Panel *panel);

protected:
	char m_pszCommand[MAX_COMMAND_SIZE];
};


//-----------------------------------------------------------------------------
// Purpose: Special action handler for cvar switching
//-----------------------------------------------------------------------------
class CMenuHandler_ToggleCvar : public CMenuActionHandler
{
public:
	CMenuHandler_ToggleCvar(CCommandMenu *pParentMenu, struct cvar_s *pCVar);
	CMenuHandler_ToggleCvar(CCommandMenu *pParentMenu, const char *pCVarName);
	virtual void actionPerformed(Panel *panel);

protected:
	struct cvar_s *m_pCVar;
};




#define HIDE_TEXTWINDOW		0

//-----------------------------------------------------------------------------
// Purpose: Special action handler for activating windows
//-----------------------------------------------------------------------------
class CMenuHandler_TextWindow : public CMenuActionHandler
{
public:
	CMenuHandler_TextWindow(int iState);
	virtual void actionPerformed(Panel *panel);

protected:
	int	m_iState;
};




//============================================================
// InputSignals
//============================================================

//-----------------------------------------------------------------------------
// Purpose: Special handler for CCommandMenu
//-----------------------------------------------------------------------------
class CMenuHandler_PopupSubMenuInput : public CDefaultInputSignal
{
public:
	CMenuHandler_PopupSubMenuInput(Button *pButton, CCommandMenu *pSubMenu);
//	virtual void cursorMoved(int x,int y,Panel *panel)
	virtual void cursorEntered(Panel *panel);
	virtual void cursorExited(Panel *panel);

protected:
	CCommandMenu *m_pSubMenu;
	Button		 *m_pButton;
};


//-----------------------------------------------------------------------------
// Purpose: Special handler for drag and drop
//-----------------------------------------------------------------------------
class CDragNDropHandler : public CDefaultInputSignal
{
public:
	CDragNDropHandler(DragNDropPanel *pPanel);
	virtual void cursorMoved(int x, int y, Panel *panel);
	virtual void cursorEntered(Panel *panel);
	virtual void cursorExited(Panel *panel);
	virtual void mousePressed(MouseCode code, Panel *panel);
	virtual void mouseReleased(MouseCode code, Panel *panel);
	virtual void Drag(void);
	virtual void Drop(void);

protected:
	DragNDropPanel	*m_pPanel;
	int				m_PanelStartPos[2];// panel position when it was grabbed
//	int				m_PanelPosMouseDelta[2];// vecMouseClick - vecPanelPos
	int				m_LastMousePos[2];
	bool			m_bDragging;
	bool			m_bMouseInside;
};


//-----------------------------------------------------------------------------
// Purpose: Special handler for CMenuPanel buttons
//-----------------------------------------------------------------------------
class CHandler_MenuButtonOver : public CDefaultInputSignal
{
public:
	CHandler_MenuButtonOver(CMenuPanel *pPanel, int iButton)
	{
		m_iButton = iButton;
		m_pMenuPanel = pPanel;
	}
	virtual void cursorEntered(Panel *panel);

protected:
	int			m_iButton;
	CMenuPanel	*m_pMenuPanel;
};


//-----------------------------------------------------------------------------
// Purpose: Special handler for highlighting of ordinary buttons
//-----------------------------------------------------------------------------
class CHandler_ButtonHighlight : public CDefaultInputSignal
{
public:
	CHandler_ButtonHighlight(Button *pButton)
	{
		m_pButton = pButton;
	}
	virtual void cursorEntered(Panel *panel) 
	{ 
		m_pButton->setArmed(true);
	}
	virtual void cursorExited(Panel *Panel) 
	{
		m_pButton->setArmed(false);
	}

protected:
	Button *m_pButton;
};

//-----------------------------------------------------------------------------
// Purpose: Special handler for highlighting of command menu buttons
//-----------------------------------------------------------------------------
class CHandler_CommandButtonHighlight : public CHandler_ButtonHighlight
{
public:
	CHandler_CommandButtonHighlight(CommandButton *pButton) : CHandler_ButtonHighlight(pButton)
	{
		m_pCommandButton = pButton;
	}
	virtual void cursorEntered(Panel *panel)
	{
		m_pCommandButton->cursorEntered();
	}
	virtual void cursorExited(Panel *panel)
	{
		m_pCommandButton->cursorExited();
	}

protected:
	CommandButton *m_pCommandButton;
};




//============================================================
// Custom scroll panel
//============================================================

// Custom drawn scroll bars
class CCustomScrollButton : public CommandButton
{
public:
	CCustomScrollButton(int iArrow, const char *text, int x, int y, int wide, int tall);
	virtual void paint(void);
//	virtual void paintBackground(void);

protected:
	BitmapTGA	*m_pImage;
};

// Custom drawn slider bar
class CCustomSlider : public Slider
{
public:
	CCustomSlider(int x, int y, int wide, int tall, bool vertical);
	virtual void paintBackground(void);
	virtual void SetScheme(CScheme *pLocalScheme);// XDM3037
	::Color m_ColorBgNormal;
	::Color m_ColorBgArmed;
	::Color m_ColorBgSelected;
	::Color m_ColorBorderNormal;
	::Color m_ColorBorderArmed;
};

//-----------------------------------------------------------------------------
// Purpose: Custom drawn scrollpanel
//-----------------------------------------------------------------------------
class CCustomScrollPanel : public ScrollPanel
{
public:
	CCustomScrollPanel(void);// XDM3038
	CCustomScrollPanel(int x, int y, int wide, int tall);
};

#define SCROLLINPUT_WHEEL_DELTA		10

//-----------------------------------------------------------------------------
// Purpose: Allows to scroll ScrollPanel with a mouse wheel
//-----------------------------------------------------------------------------
class CMenuHandler_ScrollInput : public CDefaultInputSignal
{
public:
	CMenuHandler_ScrollInput(void);
	CMenuHandler_ScrollInput(ScrollPanel *pScrollPanel);
	virtual void mouseWheeled(int delta, Panel *panel);

protected:
	ScrollPanel *m_pScrollPanel;
};




//-----------------------------------------------------------------------------
// Purpose: TextPanel with default constructor for easier instantiation
//-----------------------------------------------------------------------------
class CDefaultTextPanel : public TextPanel
{
public:
	CDefaultTextPanel(void);
	CDefaultTextPanel(const char *text, int x, int y, int wide, int tall);
	int GetX(void) { return _pos[0]; }
	int GetY(void) { return _pos[1]; }
};


//-----------------------------------------------------------------------------
// Purpose: TextEntry with default constructor for easier instantiation
//-----------------------------------------------------------------------------
class CDefaultTextEntry : public TextEntry
{
public:
	CDefaultTextEntry(void);
	CDefaultTextEntry(const char *text, int x, int y, int wide, int tall);
	int GetX(void) { return _pos[0]; }
	int GetY(void) { return _pos[1]; }
};


//-----------------------------------------------------------------------------
// Purpose: ScrollPanel with default constructor for easier instantiation
//-----------------------------------------------------------------------------
class CDefaultScrollPanel : public ScrollPanel
{
public:
	CDefaultScrollPanel(void);
	CDefaultScrollPanel(int x, int y, int wide, int tall);
	int GetX(void) { return _pos[0]; }
	int GetY(void) { return _pos[1]; }
};


//-----------------------------------------------------------------------------
// Purpose: LineBorder with default color and thickness
// TODO: active/inactive colors
//-----------------------------------------------------------------------------
class CDefaultLineBorder : public LineBorder
{
public:
	CDefaultLineBorder();
};

#endif // VGUI_CUSTOMOBJECTS_H
