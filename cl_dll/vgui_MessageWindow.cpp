#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "vgui_Viewport.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include "vgui_CustomObjects.h"
#include "vgui_MessageWindow.h"
#include "keydefs.h"
#include "in_defs.h"


//-----------------------------------------------------------------------------
// Purpose: Constructs a message panel
//-----------------------------------------------------------------------------
CMessageWindowPanel::CMessageWindowPanel(const char *szMessage, const char *szTitle, bool ShadeFullscreen, int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iRemoveMe, x, y, wide, tall)
{
	setCaptureInput(true);
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	ASSERT(pSchemes != NULL);
	if (!pSchemes)// STFU analyzer
		return;
	// Schemes
	CScheme *pTitleScheme = pSchemes->getScheme("Title Font");
	CScheme *pTextScheme = pSchemes->getScheme("Briefing Text");
	//default	CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");

	//if (ShadeFullscreen)
	//	setBackgroundMode(BG_FILL_SCREEN);
	//else
		setBackgroundMode(BG_FILL_NORMAL);

	// color schemes
	setBgColor(0,0,0, ShadeFullscreen ? 100:PANEL_DEFAULT_ALPHA);// reversed alpha!
	setShowCursor(true);

	int iXPos = 0,iYPos = 0;
	int iXSize = wide,iYSize = tall;

	// Create the title
	//m_pLabelTitle = new Label(szTitle, iXPos + XRES(MOTD_BORDER), iYPos + YRES(MOTD_BORDER));
	m_LabelTitle.setParent(this);
	m_LabelTitle.setBounds(iXPos+XRES(MOTD_BORDER), iYPos+YRES(MOTD_BORDER), wide-XRES(MOTD_BORDER), pTitleScheme->font->getTall()+YRES(2));
	m_LabelTitle.setFont(pTitleScheme->font);
	m_LabelTitle.setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_LabelTitle.setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_LabelTitle.setContentAlignment(vgui::Label::a_west);
	m_LabelTitle.setPaintBackgroundEnabled(false);
	m_LabelTitle.setText("%s", szTitle);// HL20130901 suggests this

	// Create the Scroll panel
	//m_pScrollPanel = new CCustomScrollPanel(iXPos + XRES(MOTD_BORDER), iYPos + (YRES(MOTD_BORDER)*2 + m_LabelTitle.getTall()),
	//										iXSize - XRES(MOTD_BORDER*2), iYSize - (YRES(48) + BUTTON_SIZE_Y*2));
	m_ScrollPanel.setParent(this);
	m_ScrollPanel.setBounds(iXPos + XRES(MOTD_BORDER), iYPos + (YRES(MOTD_BORDER)*2 + m_LabelTitle.getTall()), iXSize - XRES(MOTD_BORDER*2), iYSize - (YRES(48) + BUTTON_SIZE_Y*2));
	// force the scrollbars on so clientClip will take them in account after the validate
	m_ScrollPanel.setScrollBarAutoVisible(false, false);
	m_ScrollPanel.setScrollBarVisible(true, true);
	// XDM3038a	m_ScrollPanel.getVerticalScrollBar()->getSlider()->setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_ScrollPanel.validate();
	addInputSignal(new CMenuHandler_ScrollInput(&m_ScrollPanel));// XDM3038a

	// Create the text panel
	//m_pTextPanel = new TextPanel(szMessage, iXPos,iYPos, 64,64);
	//m_TextPanel.setBounds(iXPos,iYPos, 128,128);
	m_TextPanel.setText(szMessage);// XDM3038
	m_TextPanel.setParent(m_ScrollPanel.getClient());
	m_TextPanel.setFont(pTextScheme->font);
	m_TextPanel.setFgColor(pTextScheme->FgColor.r, pTextScheme->FgColor.g, pTextScheme->FgColor.b, 255-pTextScheme->FgColor.a);
	m_TextPanel.setBgColor(pTextScheme->BgColor.r, pTextScheme->BgColor.g, pTextScheme->BgColor.b, 255-pTextScheme->BgColor.a);
	// Get the total size of the MOTD text and resize the text panel
	int iScrollSizeX, iScrollSizeY;
	// First, set the size so that the client's wdith is correct at least because the
	//  width is critical for getting the "wrapped" size right.
	// You'll see a horizontal scroll bar if there is a single word that won't wrap in the
	//  specified width.
	m_TextPanel.getTextImage()->setSize(m_ScrollPanel.getClientClip()->getWide(), m_ScrollPanel.getClientClip()->getTall());
	m_TextPanel.getTextImage()->getTextSizeWrapped(iScrollSizeX, iScrollSizeY);
	// Now resize the textpanel to fit the scrolled size
	m_TextPanel.setSize(iScrollSizeX , iScrollSizeY);

	// turn the scrollbars back into automode
	m_ScrollPanel.setScrollBarAutoVisible(true, true);
	m_ScrollPanel.setScrollBarVisible(false, false);
	m_ScrollPanel.validate();

	//m_pButtonClose = new CommandButton(BufferedLocaliseTextString("#Menu_OK"),
	//	(iXPos + iXSize)-XRES(MOTD_BORDER)-BUTTON_SIZE_X, (iYPos + iYSize)-YRES(MOTD_BORDER)-BUTTON_SIZE_Y, BUTTON_SIZE_X, BUTTON_SIZE_Y, false, false, true, NULL, pButtonScheme);
			//iXPos + XRES(MOTD_BORDER), (iYPos + iYSize)-YRES(MOTD_BORDER)-BUTTON_SIZE_Y, BUTTON_SIZE_X, BUTTON_SIZE_Y);
	m_ButtonClose.setParent(this);
	m_ButtonClose.setBounds((iXPos + iXSize)-XRES(MOTD_BORDER)-BUTTON_SIZE_X, (iYPos + iYSize)-YRES(MOTD_BORDER)-BUTTON_SIZE_Y, BUTTON_SIZE_X, BUTTON_SIZE_Y);
	m_ButtonClose.addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDCLOSE));
	m_ButtonClose.m_bShowHotKey = false;
	m_ButtonClose.setContentAlignment(Label::a_center);
	m_ButtonClose.setMainText(BufferedLocaliseTextString("#Menu_OK"));
	//m_ButtonClose.addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));
	// default	m_ButtonClose.SetScheme(pButtonScheme);

	setBorder(new CDefaultLineBorder());
	setPaintBorderEnabled(true);
	//setPos(iXPos,iYPos);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMessageWindowPanel::~CMessageWindowPanel()
{
	// don't we need to delete allocated controls??
}

//-----------------------------------------------------------------------------
// Purpose: Menu panel is being opened
//-----------------------------------------------------------------------------
void CMessageWindowPanel::Open(void)
{
	PlaySound("vgui/window2.wav", VOL_NORM);// XDM
	CMessageWindowPanel::BaseClass::Open();
	//requestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Menu panel is being closed
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMessageWindowPanel::OnClose(void)
{
	if (m_flCloseDelay > 0 && (m_flOpenTime + m_flCloseDelay > gHUD.m_flTime))
	{
		char buf[32];
		_snprintf(buf, 32, "%s (%d)\0", BufferedLocaliseTextString("#Menu_OK"), (int)(m_flOpenTime + m_flCloseDelay - gHUD.m_flTime));
		m_ButtonClose.setMainText(buf);
		PlaySound("vgui/button_error.wav", VOL_NORM);
		return false;// XDM3038a
	}
	bool result = CMessageWindowPanel::BaseClass::OnClose();
	if (result)// OK to close
	{
		m_ButtonClose.setArmed(false);// XDM3037
		PlaySound("vgui/menu_close.wav", VOL_NORM);
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Catch hotkeys XDM3038c: since VGUI has troubles focusing on controls...
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 0: handled, 1: unhandled, allow other panels to recieve key
//-----------------------------------------------------------------------------
int CMessageWindowPanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down)
	{
		if (keynum == K_MWHEELDOWN || keynum == K_MWHEELUP)
		{
			int hv, vv, delta = 1;
			if (keynum == K_MWHEELDOWN)
				delta *= -1;

			m_ScrollPanel.getScrollValue(hv, vv);
			if (gViewPort->isKeyDown(KEY_LSHIFT) || gViewPort->isKeyDown(KEY_RSHIFT))// && m_pScrollPanel->getHorizontalScrollBar())
				hv -= delta * sensitivity->value * SCROLLINPUT_WHEEL_DELTA;
			else
				vv -= delta * sensitivity->value * SCROLLINPUT_WHEEL_DELTA;

			m_ScrollPanel.setScrollValue(hv, vv);
			return 0;
		}
	}
	return CMenuPanel::KeyInput(down, keynum, pszCurrentBinding);
}
