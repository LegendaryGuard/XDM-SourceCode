#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "keydefs.h"

//-----------------------------------------------------------------------------
// Purpose: UNDONE: Panel that can easily apply scheme to controls
// Input  : *pScheme - 
//			x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
CSchemeEnabledPanel::CSchemeEnabledPanel(CScheme *pScheme, int x, int y, int wide, int tall) : Panel(x,y,wide,tall)
{
	setScheme(pScheme);
}

void CSchemeEnabledPanel::paintBackground(void)
{
	Panel::paintBackground();
}

CScheme *CSchemeEnabledPanel::getScheme(void)
{
	return m_pScheme;
}

void CSchemeEnabledPanel::setScheme(CScheme *pScheme)
{
	m_pScheme = pScheme;
}

void CSchemeEnabledPanel::ApplyScheme(CScheme *pScheme, Panel *pControl)
{
//	isArmed
	if (pScheme && pControl)
	{
		pControl->setBgColor(pScheme->BgColor.r, pScheme->BgColor.g, pScheme->BgColor.b, 255-pScheme->BgColor.a);
		pControl->setFgColor(pScheme->FgColor.r, pScheme->FgColor.g, pScheme->FgColor.b, 255-pScheme->FgColor.a);
	}
}

//-----------------------------------------------------------------------------
// Purpose: DragNDropPanel
// Input  : x y - 
//			wide tall - 
// WARNING: do not use setPos(x,y) or setBounds(x, y, wide, tall) on self in subclasses!
//-----------------------------------------------------------------------------
DragNDropPanel::DragNDropPanel(bool dragenabled, int x, int y, int wide, int tall) : Panel(x,y,wide,tall)
{
	if (x < 0)
		x = ScreenWidth/2 - wide/2;
	if (y < 0)
		y = ScreenHeight/2 - tall/2;

	setPos(x,y);
	//we only changed position	setBounds(x, y, wide, tall);

	m_OriginalPos[0] = x;
	m_OriginalPos[1] = y;
	m_bBeingDragged = false;
	m_bDragEnabled = dragenabled;
	addInputSignal(new CDragNDropHandler(this));// Create the Drag Handler

	/*m_pOriginalBorder = NULL;
	m_pDragBorder = new LineBorder();// Create the border (for dragging)

	if (gViewPort)
	{
		CSchemeManager *pSchemeMgr = gViewPort->GetSchemeManager();
		if (pSchemeMgr)
		{
			CScheme *pScheme = pSchemeMgr->getSafeScheme(pSchemeMgr->getSchemeHandle("Basic Text"));
			if (pScheme)
				m_pDragBorder->setLineColor(pScheme->FgColorArmed[0], pScheme->FgColorArmed[1], pScheme->FgColorArmed[2], pScheme->BorderColor[3]);
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Don't allow setting position outside the screen
// Input  : x y - 
//-----------------------------------------------------------------------------
void DragNDropPanel::setPos(int x, int y)
{
	if (x < 0)
	{
		x = 0;
	}
	else
	{
		int mx = ScreenWidth - getWide();
		if (x > mx)
			x = mx;
	}

	if (y < 0)
	{
		y = 0;
	}
	else
	{
		int my = ScreenHeight - getTall();
		if (y > my)
			y = my;
	}

	Panel::setPos(x, y);
}

//-----------------------------------------------------------------------------
// Purpose: Called by signal
// Input  : bState - 
//-----------------------------------------------------------------------------
void DragNDropPanel::setDragged(bool bState)
{
	if (m_bDragEnabled)
	{
		/*if (bState)// change broder disregarding _paintBorderEnabled
		{
			m_pOriginalBorder = _border;// save old border
			setBorder(m_pDragBorder);// BUGBUG: this function deletes old instance
		}	
		else
		{
			setBorder(m_pOriginalBorder);
			m_pOriginalBorder = NULL;// in case it will be deleted by the window itself
		}*/
		m_bBeingDragged = bState;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable or disable DragNDrop functionality
// Input  : enable - 
//-----------------------------------------------------------------------------
void DragNDropPanel::setDragEnabled(bool enable)
{
	m_bDragEnabled = enable;
}

//-----------------------------------------------------------------------------
// Purpose: is DragNDrop functionality enabled?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool DragNDropPanel::getDragEnabled(void)
{
	return m_bDragEnabled;
}



//-----------------------------------------------------------------------------
// Purpose: Menu Panel that supports buffering of menus and basic action signals
// Input  : iRemoveMe - delete this when closed
//			x y - position
//			wide tall - size
// Note   : All internal controls use RELATIVE coordinates, where (0,0) is panel's top-left corner!
//-----------------------------------------------------------------------------
CMenuPanel::CMenuPanel(int RemoveMe, int x, int y, int wide, int tall) : DragNDropPanel(false, x,y,wide,tall)//CTransparentPanel(BG_FILL_NORMAL, 127, x,y,wide,tall)
{
	m_pNextMenu = NULL;
	m_iMenuID = 0;
	m_iRemoveMe = RemoveMe;
	m_iIsActive = 0;
	m_iBackgroundMode = BG_FILL_NORMAL;
	m_bCaptureInput = false;
	m_bShowCursor = false;
	m_flOpenTime = 0;// XDM3038a: !!!
	m_flCloseDelay = 0;// XDM3038a: !!!
	m_TitleIcon.setParent(this);
	m_TitleIcon.LoadImage("resource/icon_xhl16.tga", true);
	m_TitleIcon.setPos(PANEL_INNER_OFFSET, PANEL_INNER_OFFSET);
	m_TitleIcon.setContentAlignment(vgui::Label::a_center);
	m_TitleIcon.setPaintBackgroundEnabled(false);

	Reset();
	//_OnCloseActionSignalDar.putElement(new CMenuPanelActionSignalHandler(this, MENUPANEL_SIGNAL_CLOSE));
}

//-----------------------------------------------------------------------------
// Purpose: Destructor. For tracing mostly.
//-----------------------------------------------------------------------------
CMenuPanel::~CMenuPanel()
{
	m_pNextMenu = NULL;
	m_iMenuID = 0;
	m_bCaptureInput = false;
	m_bShowCursor = false;
}

//-----------------------------------------------------------------------------
// Purpose: Menu panel is being opened
//-----------------------------------------------------------------------------
void CMenuPanel::Open(void)
{
	setVisible(true);
	// Note the open time, so we can delay input for a bit
	// this does fail sometimes (-9876543 seconds)?
	if (gHUD.m_iActive > 0)
		m_flOpenTime = max(0, gEngfuncs.GetClientTime());// XDM3038a: WARNING! gHUD.m_flTime may be wrong (still not updated from previous map)
	else
		m_flOpenTime = 0;

	setEnabled(true);// XDM3038

	// shall we?
	requestFocus();
	//PlaySound("vgui/window2.wav", VOL_NORM);// XDM
	//m_iIsActive = true;// XDM3038: TODO?
}

//-----------------------------------------------------------------------------
// Purpose: Close
//-----------------------------------------------------------------------------
void CMenuPanel::Close(void)
{
	if (OnClose() == false)// XDM3037a: derived class forbids me to close
		return;

	setVisible(false);
	setEnabled(false);// XDM3038
	m_iIsActive = false;

	//Reset();?
	//PlaySound("vgui/menu_close.wav", VOL_NORM);// XDM
	//gViewPort->UpdateCursorState();// XDM3038 -> OnMenuPanelClose()

	if (getParent())// XDM3037
	{
		getParent()->requestFocus();// BringToFront?
		//TODO?	getParent()->OnRemoveChild(this);

		if (ShouldBeRemoved())
		{
			removeAllChildren();// XDM3038
			if (getParent() != gViewPort)
				getParent()->removeChild(this);// important! also, do it last // WARNING! TODO: TESTME: BUGBUG: does it free the memory?
		}
	}
	/*return */gViewPort->OnMenuPanelClose(this); //removeChild(this);
	// This MenuPanel has now been deleted. Don't append code here.
}

//-----------------------------------------------------------------------------
// Purpose: Reset
//-----------------------------------------------------------------------------
void CMenuPanel::Reset(void)
{
	m_pNextMenu = NULL;
	m_iIsActive = false;
	m_flOpenTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: HACK! Overridden to identify self
// Input  : *buf - 
//			bufLen - 
//-----------------------------------------------------------------------------
void CMenuPanel::getPersistanceText(char *buf, int bufLen)
{
	if (buf)
		strncpy(buf, "CMenuPanel", bufLen);

//	Panel::getPersistanceText() returns "->SetBounds(0,0,123,123)" - WTF?!
}

//-----------------------------------------------------------------------------
// Purpose: Engine
//-----------------------------------------------------------------------------
void CMenuPanel::paintBackground(void)
{
	if (getBackgroundMode() == BG_FILL_NORMAL)
		CMenuPanel::BaseClass::paintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: ActionSignal is fired
// Input  : signal - 
//			pCaller - CAN BE NULL!
//-----------------------------------------------------------------------------
void CMenuPanel::OnActionSignal(int signal, CMenuPanelActionSignalHandler *pCaller)
{
	if (signal == VGUI_IDNOTHING)
	{
		return;
	}
	else if (signal == VGUI_IDCLOSE)
	{
		if (pCaller)// !!!
			pCaller->m_iSoundType = ACTSIGSOUND_NONE;// XDM3038a

		Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Capture use input when active and visible
// Input  : enable - 
//-----------------------------------------------------------------------------
void CMenuPanel::setCaptureInput(bool enable)
{
	m_bCaptureInput = enable;
}

//-----------------------------------------------------------------------------
// Purpose: Show mouse cursor when active and visible
// Input  : show - 
//-----------------------------------------------------------------------------
void CMenuPanel::setShowCursor(bool show)
{
	m_bShowCursor = show;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that this panel should capture all keyboard and mouse input
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMenuPanel::IsCapturingInput(void)
{
	return m_bCaptureInput;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that mouse cursor should be visible when this panel is visible
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMenuPanel::IsShowingCursor(void)
{
	return m_bShowCursor;
}

//-----------------------------------------------------------------------------
// Purpose: Read-only access
// Output : m_iBackgroundMode
//-----------------------------------------------------------------------------
int CMenuPanel::getBackgroundMode(void)
{
	return m_iBackgroundMode;
}

//-----------------------------------------------------------------------------
// Purpose: Set how the window should draw its background.
// Input  : BackgroundMode - BG_FILL_NORMAL
//-----------------------------------------------------------------------------
void CMenuPanel::setBackgroundMode(int BackgroundMode)
{
	m_iBackgroundMode = BackgroundMode;
}

//-----------------------------------------------------------------------------
// Purpose: Overloadable. Panel is going to close.
// Output : Return true to ALLOW and false to PREVENT closing
//-----------------------------------------------------------------------------
bool CMenuPanel::OnClose(void)
{
	if (m_flCloseDelay > 0 && (m_flOpenTime + m_flCloseDelay > gHUD.m_flTime))
		return false;// XDM3038a

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Catch hotkeys
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 0: handled, 1: unhandled, allow other panels to recieve key
//-----------------------------------------------------------------------------
int CMenuPanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down)
	{
		if (keynum == K_ENTER || keynum == K_KP_ENTER || keynum == K_ESCAPE || keynum == K_SPACE)
		{
			Close();
			return 0;
		}
		/*else if (keynum == K_SPACE)
		{
			int n = getChildCount();
			for (int i=0; i<n; ++i)
			{
				if (getChild(i)->hasFocus())
				{
					getChild(i)->internalMousePressed(MOUSE_LEFT);// THOSE STUPID FUCKERS AT VALVE MAKE ME PUKE!!!!!!!!!
					return 0;
				}
			}
		}*/
		else if (keynum == K_TAB)// XDM3038a
		{
			int n = getChildCount();
			for (int i=0; i<n; ++i)
			{
				if (getChild(i)->hasFocus())
				{
					if (gViewPort->isKeyDown(KEY_LSHIFT) || gViewPort->isKeyDown(KEY_RSHIFT))
					{
						while (i > 0 && getChild(i-1))
						{
							if (getChild(i-1)->isEnabled())
							{
								getChild(i-1)->requestFocus();
								break;
							}
						}
						//getChild(i)->requestFocusPrev(); BAD!!
					}
					else
					{
						while (i < n-1 && getChild(i+1))
						{
							if (getChild(i+1)->isEnabled())
							{
								getChild(i+1)->requestFocus();
								break;
							}
						}
						//getChild(i)->requestFocusNext(); BAD!!
					}
					return 0;
				}
			}
		}
	}
	/* how it should be, but it's impossible	int n = getChildCount();
	for (int i=0; i<n; ++i)// right thing to do
	{
		if (getChild(i))// was not removed
			if (getChild(i)->getBoundKey() == keynum)
				getChild(i)->KeyInput()
	}*/
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Cache pNextPanel for display once this one's finished
// CAUTION: RECURSION!!!!!!!!!!!!!!!!!
// Input  : *pNextPanel - 
//-----------------------------------------------------------------------------
void CMenuPanel::SetNextMenu(CMenuPanel *pNextPanel)
{
	if (pNextPanel == this)// this may and will happen!
		return;
	//if (pNextPanel == m_pNextMenu)
	//	return;

	// DONT FUCK WITH THIS FUCKIN FUCK!!!!!!!!!!!
	///	if (m_pNextMenu)// we already have different NEXT menu queued up
	////////////		m_pNextMenu->SetNextMenu(pNextPanel);
	/////////////////else
		m_pNextMenu = pNextPanel;
}



//-----------------------------------------------------------------------------
// Purpose: Simple integer signal handler
// Input  : *pPanel - 
//			iSignal - 
//-----------------------------------------------------------------------------
CMenuPanelActionSignalHandler::CMenuPanelActionSignalHandler(CMenuPanel *pPanel, int iSignal)
{
	ASSERT(pPanel != NULL);
	m_pPanel = pPanel;
	m_iSignal = iSignal;
	m_iSoundType = ACTSIGSOUND_DEFAULT;// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Signal is fired. Called by the engine.
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CMenuPanelActionSignalHandler::actionPerformed(Panel *panel)
{
	if (m_pPanel)
	{
		m_pPanel->OnActionSignal(m_iSignal, this);

		if (m_iSoundType == ACTSIGSOUND_DEFAULT || m_iSoundType == ACTSIGSOUND_SUCCESS)// XDM3038a
			PlaySound("vgui/button_press.wav", VOL_NORM);
		else if (m_iSoundType == ACTSIGSOUND_FAILURE)
			PlaySound("vgui/button_error.wav", VOL_NORM);
	}
}


//-----------------------------------------------------------------------------
// Purpose: XDM3038a: use to automatically close menus when they lose focus
// Input  : *panel - 
//-----------------------------------------------------------------------------
/*CFocusChangeSignalClose::CFocusChangeSignalClose(CMenuPanel *pPanel)
{
	ASSERT(pPanel != NULL);
	m_pPanel = pPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Focus changed
// Input  : lost - true if focus is lost
//			*panel - 
//-----------------------------------------------------------------------------
void CFocusChangeSignalClose::focusChanged(bool lost, Panel *panel)
{
	if (m_pPanel && m_pPanel == panel && lost && m_pPanel->isVisible())
		m_pPanel->Close();// FATAL HL BUGBUG: CRASH
		//m_pPanel->OnActionSignal(VGUI_IDCANCEL);
}*/




//-----------------------------------------------------------------------------
// Purpose: Dialog Panel with standardized sets of buttons and actions
// CDialogPanel *pDialog = (CDialogPanel *)gViewPort->ShowMenu(new CDialogPanel(1, x,y,w,t, MB_OKCANCEL));
// Input  : iRemoveMe - delete this when closed
//			x y - position
//			wide - 
//			tall - 
//			type - WinAPI-like option
// Note   : All internal controls use RELATIVE coordinates, where (0,0) is panel's top-left corner!
//-----------------------------------------------------------------------------
CDialogPanel::CDialogPanel(int RemoveMe, int x, int y, int wide, int tall, unsigned long type) : CMenuPanel(RemoveMe, x,y,wide,tall)
{
	m_iSignalRecieved = 0;
	m_iNumDialogButtons = 0;
	switch (type)
	{
		case VGUI_MB_OK:
		{
			m_iNumDialogButtons = 1;
			m_pButtons = new CommandButton[m_iNumDialogButtons];
			m_pButtons[0].m_ulID = VGUI_IDOK;
			m_pButtons[0].setArmed(true);
			m_pButtons[0].setMainText(BufferedLocaliseTextString("#Menu_OK"));
		}
		break;
		case VGUI_MB_OKCANCEL:
		{
			m_iNumDialogButtons = 2;
			m_pButtons = new CommandButton[m_iNumDialogButtons];
			m_pButtons[0].m_ulID = VGUI_IDOK;
			m_pButtons[0].setMainText(BufferedLocaliseTextString("#Menu_OK"));
			m_pButtons[1].m_ulID = VGUI_IDCANCEL;
			m_pButtons[1].setMainText(BufferedLocaliseTextString("#Menu_Cancel"));
		}
		break;
		case VGUI_MB_ABORTRETRYIGNORE:
		{
			m_iNumDialogButtons = 3;
			m_pButtons = new CommandButton[m_iNumDialogButtons];
			m_pButtons[0].m_ulID = VGUI_IDABORT;
			m_pButtons[0].setMainText(BufferedLocaliseTextString("#Menu_Abort"));
			m_pButtons[1].m_ulID = VGUI_IDRETRY;
			m_pButtons[1].setMainText(BufferedLocaliseTextString("#Menu_Retry"));
			m_pButtons[2].m_ulID = VGUI_IDIGNORE;
			m_pButtons[2].setMainText(BufferedLocaliseTextString("#Menu_Ignore"));
		}
		break;
		case VGUI_MB_YESNOCANCEL:
		{
			m_iNumDialogButtons = 3;
			m_pButtons = new CommandButton[m_iNumDialogButtons];
			m_pButtons[0].m_ulID = VGUI_IDYES;
			m_pButtons[0].setMainText(BufferedLocaliseTextString("#Menu_Yes"));
			m_pButtons[1].m_ulID = VGUI_IDNO;
			m_pButtons[1].setMainText(BufferedLocaliseTextString("#Menu_No"));
			m_pButtons[2].m_ulID = VGUI_IDCANCEL;
			m_pButtons[2].setMainText(BufferedLocaliseTextString("#Menu_Cancel"));
		}
		break;
		case VGUI_MB_YESNO:
		{
			m_iNumDialogButtons = 2;
			m_pButtons = new CommandButton[m_iNumDialogButtons];
			m_pButtons[0].m_ulID = VGUI_IDYES;
			m_pButtons[0].setMainText(BufferedLocaliseTextString("#Menu_Yes"));
			m_pButtons[1].m_ulID = VGUI_IDNO;
			m_pButtons[1].setMainText(BufferedLocaliseTextString("#Menu_No"));
		}
		break;
		case VGUI_MB_RETRYCANCEL:
		{
			m_iNumDialogButtons = 2;
			m_pButtons = new CommandButton[m_iNumDialogButtons];
			m_pButtons[0].m_ulID = VGUI_IDRETRY;
			m_pButtons[0].setMainText(BufferedLocaliseTextString("#Menu_Retry"));
			m_pButtons[1].m_ulID = VGUI_IDCANCEL;
			m_pButtons[1].setMainText(BufferedLocaliseTextString("#Menu_Cancel"));
		}
		break;
	}

	for (unsigned long i=0; i<m_iNumDialogButtons; ++i)
	{
		m_pButtons[i].setParent(this);
		m_pButtons[i].setBounds(XRES(PANEL_INNER_OFFSET) + BUTTON_SIZE_X*i,
			tall-BUTTON_SIZE_Y-YRES(PANEL_INNER_OFFSET), BUTTON_SIZE_X, BUTTON_SIZE_Y);
		m_pButtons[i].setMainText(BufferedLocaliseTextString("#Menu_OK"));
		m_pButtons[i].addActionSignal(new CMenuPanelActionSignalHandler(this, m_pButtons[i].m_ulID));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset
//-----------------------------------------------------------------------------
void CDialogPanel::Reset(void)
{
	m_iSignalRecieved = 0;
	CMenuPanel::Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Close
//-----------------------------------------------------------------------------
void CDialogPanel::Close(void)
{
	for (unsigned long i=0; i<m_iNumDialogButtons; ++i)
		removeChild(&m_pButtons[i]);

	delete [] m_pButtons;
	m_pButtons = NULL;

	CMenuPanel::Close();
}

//-----------------------------------------------------------------------------
// Purpose: Overloadable. Do cleanup here.
//-----------------------------------------------------------------------------
bool CDialogPanel::OnClose(void)
{
	return CDialogPanel::BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Overloadable. OK
//-----------------------------------------------------------------------------
void CDialogPanel::OnOK(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Overloadable. Cancel
//-----------------------------------------------------------------------------
void CDialogPanel::OnCancel(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Overloadable. User code goes here!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDialogPanel::DoExecCommand(void)
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Button pressed or something
// Undone : button policy is unclear
// Input  : signal - button ID
//-----------------------------------------------------------------------------
void CDialogPanel::OnActionSignal(int signal, CMenuPanelActionSignalHandler *pCaller)
{
	m_iSignalRecieved = signal;
	switch (signal)
	{
	case VGUI_IDNOTHING:
		{
			return;
		}
		break;
	case VGUI_IDOK:
		{
			if (DoExecCommand())
				PlaySound("vgui/button_press.wav", VOL_NORM);
			else
				PlaySound("vgui/menu_close.wav", VOL_NORM);

			OnOK();
		}
		break;
	case VGUI_IDCANCEL:
		{
			OnCancel();
			PlaySound("vgui/button_exit.wav", VOL_NORM);
		}
		break;
	case VGUI_IDABORT:
		{
			OnCancel();
			PlaySound("vgui/button_exit.wav", VOL_NORM);
		}
		break;
	case VGUI_IDRETRY:
		{
			if (DoExecCommand())
				PlaySound("vgui/button_press.wav", VOL_NORM);
			else
				PlaySound("vgui/button_enter.wav", VOL_NORM);
		}
		break;
	case VGUI_IDIGNORE:
	case VGUI_IDYES:
	case VGUI_IDNO:
	case VGUI_IDCLOSE:
	case VGUI_IDHELP:
		{
			PlaySound("vgui/button_press.wav", VOL_NORM);
		}
		break;
	}

	Close();
}
