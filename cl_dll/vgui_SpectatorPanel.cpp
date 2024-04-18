#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "pm_shared.h"
#include "vgui_Viewport.h"
#include "vgui_SpectatorPanel.h"
#include "vgui_ScorePanel.h"
#include "Exports.h"
#include "keydefs.h"


//-----------------------------------------------------------------------------
// Purpose: Sets the location of the input for chat text. XDM: he-he, I guessed it right :)
// Input  : *x, *y - coordinates output
//-----------------------------------------------------------------------------
void CL_DLLEXPORT HUD_ChatInputPosition(int *x, int *y)
{
//	RecClChatInputPosition( x, y );
	if (g_iUser1 != 0 || gEngfuncs.IsSpectateOnly())
	{
		if (gHUD.m_Spectator.m_iInsetMode == INSET_OFF)
		{
			if (gViewPort && gViewPort->GetSpectatorPanel())
				*y = gViewPort->GetSpectatorPanel()->GetBorderHeight();
			else
				*y = YRES(SPECTATORPANEL_HEIGHT);
		}
		else
		{
			int wh;
			gHUD.m_Spectator.GetInsetWindowBounds(NULL, NULL, NULL, &wh);
			*y = YRES(wh + 5);
		}
	}
}


const char *gSpectatorLabels[] =
{
	"#OBS_NONE",
	"#OBS_CHASE_LOCKED",
	"#OBS_CHASE_FREE",
	"#OBS_ROAMING",
	"#OBS_IN_EYE",
	"#OBS_MAP_FREE",
	"#OBS_MAP_CHASE",
	"#OBS_INTERMISSION"
};

const char *GetSpectatorModeLabel(int iMode)
{
	if (iMode >= OBS_NONE && iMode <= OBS_INTERMISSION)
		return gSpectatorLabels[iMode];

	return "";
}

/*int SpectateButton::IsNotValid(void)
{
	// Only visible if the server allows it
	if (gViewPort->GetAllowSpectators() != 0)
		return false;

	// XDM: we're already in spectator mode
	// XDM3035a: allow toggle?
	//if (gViewPort->m_pSpectatorPanel->isVisible())
	//	return false;

	return true;
}*/


//-----------------------------------------------------------------------------
// Purpose: 
// Note   :Look into CHudSpectator::HandleButtonsDown() for hotkeys
//-----------------------------------------------------------------------------
SpectatorPanel::SpectatorPanel(int x, int y, int wide, int tall) : CMenuPanel(FALSE, x,y, wide,tall)//Panel(x,y,wide,tall)
{
	// XDM3038: everything moved here
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	ASSERT(pSchemes != NULL);
	if (!pSchemes)// STFU analyzer
		return;
	// Schemes
	CScheme *pTextScheme = pSchemes->getScheme("Basic Text");
	CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");
	m_BottomMainButtonFg = pButtonScheme->FgColor;

	//m_TopBorder = new Panel(0, 0, ScreenWidth, (pTextScheme->font->getTall()+1)*MAX_TEAMS);// XDM3038 YRES(SPECTATORPANEL_HEIGHT));
	m_TopBorder.setBounds(0, 0, ScreenWidth, (pTextScheme->font->getTall()+1)*MAX_TEAMS);
	m_TopBorder.setParent(this);
	m_TopBorder.setBgColor(0,0,0,PANEL_DEFAULT_ALPHA);
	//m_BottomBorder = new Panel(0, ScreenHeight - YRES(32), ScreenWidth, m_TopBorder->getTall());//YRES(SPECTATORPANEL_HEIGHT));
	m_BottomBorder.setBounds(0, ScreenHeight - YRES(32), ScreenWidth, m_TopBorder.getTall());
	m_BottomBorder.setParent(this);
	m_BottomBorder.setBgColor(0,0,0,PANEL_DEFAULT_ALPHA);

	setCaptureInput(true);
	setShowCursor(false);// for now
	setPaintBackgroundEnabled(false);

	//m_ExtraInfo = new Label("----------------", 0, 0, wide, pTextScheme->font->getTall()+2);//YRES(SPECTATORPANEL_HEIGHT));
	m_ExtraInfo.setParent(&m_TopBorder);
	m_ExtraInfo.setBounds(0, 0, m_TopBorder.getWide(), pTextScheme->font->getTall()*2+YRES(2));
	m_ExtraInfo.setFont(pTextScheme->font);
	m_ExtraInfo.setPaintBackgroundEnabled(false);
	m_ExtraInfo.setFgColor(pTextScheme->FgColor.r, pTextScheme->FgColor.g, pTextScheme->FgColor.b, 255-pTextScheme->FgColor.a);
	m_ExtraInfo.setBgColor(pTextScheme->BgColor.r, pTextScheme->BgColor.g, pTextScheme->BgColor.b, 255-pTextScheme->BgColor.a);
	m_ExtraInfo.setContentAlignment(vgui::Label::a_west);

	//m_CurrentTime = new Label("00:00", 0, 0, wide, pTextScheme->font->getTall()+2);//YRES(SPECTATORPANEL_HEIGHT));
	m_CurrentTime.setParent(&m_TopBorder);
	m_CurrentTime.setBounds(0, 0, m_TopBorder.getWide(), pTextScheme->font->getTall()+2);
	m_CurrentTime.setFont(pTextScheme->font);
	m_CurrentTime.setPaintBackgroundEnabled(false);
	m_CurrentTime.setFgColor(pTextScheme->FgColor.r, pTextScheme->FgColor.g, pTextScheme->FgColor.b, 255-pTextScheme->FgColor.a);
	m_CurrentTime.setBgColor(pTextScheme->BgColor.r, pTextScheme->BgColor.g, pTextScheme->BgColor.b, 255-pTextScheme->BgColor.a);
	m_CurrentTime.setContentAlignment(vgui::Label::a_west);

	m_TimerImage = new CImageLabel("timer", 0, 0);
	if (m_TimerImage)
		m_TimerImage->setParent(&m_TopBorder);

	m_TopBanner = new CImageLabel("banner", 0, 0, XRES(SPECTATORPANEL_BANNER_WIDTH), YRES(SPECTATORPANEL_BANNER_HEIGHT));
	if (m_TopBanner)
		m_TopBanner->setParent(this);

	/*
	m_Separator = new Panel(0, 0, XRES(6), YRES(96));
	m_Separator->setParent(m_TopBorder);
	m_Separator->setFgColor(fg_r1, fg_g1, fg_b1, fg_a1);
	m_Separator->setBgColor(bg_r1, bg_g1, bg_b1, bg_a1);// XDM
	*/

	for (size_t j=0; j < MAX_TEAMS; ++j)// label index != team index!
	{
		//m_TeamScores[j] = new Label("   ", 0, 0, m_TopBorder.getWide(), pTextScheme->font->getTall()+1);
		m_TeamScores[j].setParent(&m_TopBorder);
		m_TeamScores[j].setBounds(0, 0, m_TopBorder.getWide(), pTextScheme->font->getTall()+1);
		m_TeamScores[j].setFont(pTextScheme->font);
		m_TeamScores[j].setPaintBackgroundEnabled(false);
		m_TeamScores[j].setFgColor(pTextScheme->FgColor.r, pTextScheme->FgColor.g, pTextScheme->FgColor.b, 255-pTextScheme->FgColor.a);
		m_TeamScores[j].setBgColor(pTextScheme->BgColor.r, pTextScheme->BgColor.g, pTextScheme->BgColor.b, 255-pTextScheme->BgColor.a);
		m_TeamScores[j].setContentAlignment(vgui::Label::a_west);
		m_TeamScores[j].setVisible(false);
	}

	// XDM: just a title.
	//m_TopMainLabel = new Label(BufferedLocaliseTextString("#SPECT_TOP"), XRES(0), YRES(0), wide, YRES(32));
	m_TopMainLabel.setParent(&m_TopBorder);
	m_TopMainLabel.setBounds(XRES(0), YRES(0), wide, YRES(32));
	m_TopMainLabel.setFont(pTextScheme->font);
	m_TopMainLabel.setPaintBackgroundEnabled(false);
	m_TopMainLabel.setFgColor(pTextScheme->FgColor.r, pTextScheme->FgColor.g, pTextScheme->FgColor.b, 255-pTextScheme->FgColor.a);
	m_TopMainLabel.setBgColor(pTextScheme->BgColor.r, pTextScheme->BgColor.g, pTextScheme->BgColor.b, 255-pTextScheme->BgColor.a);
	m_TopMainLabel.setContentAlignment(vgui::Label::a_center);
	m_TopMainLabel.setText(BufferedLocaliseTextString("#SPECT_TOP"));

	// Initialize command buttons.
	//m_OptionButton = new CommandButton(BufferedLocaliseTextString("#SPECT_OPTIONS"), XRES(SPECTATORPANEL_MARGIN), YRES(6), XRES(SPECTATORPANEL_OPTIONS_BUTTON_X), YRES(SPECTATORPANEL_BUTTON_Y), false, false, false, NULL, pButtonScheme);
	m_OptionButton.setParent(&m_BottomBorder);
	m_OptionButton.setBounds(XRES(SPECTATORPANEL_MARGIN), YRES(6), XRES(SPECTATORPANEL_OPTIONS_BUTTON_X), YRES(SPECTATORPANEL_BUTTON_Y));
	m_OptionButton.setContentAlignment(vgui::Label::a_center);
	m_OptionButton.setBoundKey('o');
	m_OptionButton.addActionSignal(new CMenuPanelActionSignalHandler(this, SPECTATOR_PANEL_CMD_OPTIONS));
	m_OptionButton.setMainText(BufferedLocaliseTextString("#SPECT_OPTIONS"));
	//m_OptionButton.SetScheme(pButtonScheme);

	//m_CamButton = new CommandButton(BufferedLocaliseTextString("#CAM_OPTIONS"),  ScreenWidth - XRES(SPECTATORPANEL_OPTIONS_BUTTON_X+SPECTATORPANEL_MARGIN), YRES(6), XRES(SPECTATORPANEL_OPTIONS_BUTTON_X), YRES(SPECTATORPANEL_BUTTON_Y), false, false, false, NULL, pButtonScheme);
	m_CamButton.setBounds(ScreenWidth - XRES(SPECTATORPANEL_OPTIONS_BUTTON_X+SPECTATORPANEL_MARGIN), YRES(6), XRES(SPECTATORPANEL_OPTIONS_BUTTON_X), YRES(SPECTATORPANEL_BUTTON_Y));
	m_CamButton.setParent(&m_BottomBorder);
	m_CamButton.setContentAlignment(vgui::Label::a_center);
	m_CamButton.setBoundKey('c');
	m_CamButton.addActionSignal(new CMenuPanelActionSignalHandler(this, SPECTATOR_PANEL_CMD_CAMERA));
	m_CamButton.setMainText(BufferedLocaliseTextString("#CAM_OPTIONS"));
	//m_CamButton.SetScheme(pButtonScheme);

	int bx = m_OptionButton.GetX() + m_OptionButton.getWide() + XRES(SPECTATORPANEL_MARGIN);
	int bw = XRES(SPECTATORPANEL_SMALL_BUTTON_X);
	//m_PrevPlayerButton= new CommandButton("<", bx, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y), false, false, false, NULL, pButtonScheme);
	m_PrevPlayerButton.setParent(&m_BottomBorder);
	m_PrevPlayerButton.setBounds(bx, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y));
	m_PrevPlayerButton.setContentAlignment(vgui::Label::a_center);
	m_PrevPlayerButton.setMainText("<");
	m_PrevPlayerButton.LoadIcon("arrowleft");
	m_PrevPlayerButton.m_bShowHotKey = false;
	m_PrevPlayerButton.setBoundKey(K_LEFTARROW);
	m_PrevPlayerButton.addActionSignal(new CMenuPanelActionSignalHandler(this, SPECTATOR_PANEL_CMD_PREVPLAYER));
	//m_PrevPlayerButton.SetScheme(pButtonScheme);

	bx = m_CamButton.GetX() - XRES(SPECTATORPANEL_MARGIN) - bw;
	//m_NextPlayerButton= new CommandButton(">", bx, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y), false, false, false, NULL, pButtonScheme);
	m_NextPlayerButton.setParent(&m_BottomBorder);
	m_NextPlayerButton.setBounds(bx, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y));
	m_NextPlayerButton.setContentAlignment(vgui::Label::a_center);
	m_NextPlayerButton.setMainText(">");
	m_NextPlayerButton.LoadIcon("arrowright");
	m_NextPlayerButton.m_bShowHotKey = false;
	m_NextPlayerButton.setBoundKey(K_RIGHTARROW);
	m_NextPlayerButton.addActionSignal(new CMenuPanelActionSignalHandler(this, SPECTATOR_PANEL_CMD_NEXTPLAYER));
	//m_NextPlayerButton.SetScheme(pButtonScheme);

	// Initialize the bottom title.
	bx = m_PrevPlayerButton.GetX() + m_PrevPlayerButton.getWide() + XRES(SPECTATORPANEL_MARGIN);
	bw = m_NextPlayerButton.GetX() - XRES(SPECTATORPANEL_MARGIN) - bx;
	//m_BottomMainButton = new CommandButton(BufferedLocaliseTextString("#SPECT_BOTTOM"), x, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y), false, false, false, NULL, pButtonScheme);
	m_BottomMainButton.setParent(&m_BottomBorder);
	m_BottomMainButton.setBounds(bx, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y));
	m_BottomMainButton.setPaintBackgroundEnabled(false);
	m_BottomMainButton.setContentAlignment(vgui::Label::a_center);
	m_BottomMainButton.setBorder(new CDefaultLineBorder());
	m_BottomMainButton.setBoundKey(K_UPARROW);
	m_BottomMainButton.addActionSignal(new CMenuPanelActionSignalHandler(this, SPECTATOR_PANEL_CMD_PLAYERS));
	m_BottomMainButton.setMainText(BufferedLocaliseTextString("#SPECT_BOTTOM"));
	m_BottomMainButton.LoadIcon("arrowup");
	//m_BottomMainButton.SetScheme(pButtonScheme);

	m_pPlayersMenu = new CPlayersCommandMenu(NULL, this, false, false, false, SPECTATOR_PANEL_CMD_PLAYERS, CMENU_DIR_UP, ScreenWidth/2-CMENU_SIZE_X/2, m_BottomMainButton.GetY(), CMENU_SIZE_X, CBUTTON_SIZE_Y);
	if (m_pPlayersMenu)
	{
		m_pPlayersMenu->setParent(this);
		m_pPlayersMenu->setVisible(false);
		m_pPlayersMenu->m_iButtonSizeY = CBUTTON_SIZE_Y;
	}
	// make the inset view window clickable
	//m_InsetViewButton = new CommandButton("", XRES(2), YRES(2), XRES(240), YRES(180), false, false, false, NULL, pButtonScheme);
	m_InsetViewButton.setParent(this);
	m_InsetViewButton.setBounds(XRES(2), YRES(2), XRES(240), YRES(180));
	m_InsetViewButton.m_bShowHotKey = false;
	m_InsetViewButton.setBoundKey(K_INS);
	m_InsetViewButton.addActionSignal(new CMenuPanelActionSignalHandler(this, SPECTATOR_PANEL_CMD_TOGGLE_INSET));
	m_InsetViewButton.m_ColorBgNormal.a = 0;// fully transparent
	m_InsetViewButton.m_ColorBgArmed.a = 127;
	//m_InsetViewButton.SetScheme(pButtonScheme);

	m_menuVisible = false;
	m_insetVisible = false;
	m_CamButton.setVisible(false);
	m_OptionButton.setVisible(false);
	m_NextPlayerButton.setVisible(false);
	m_PrevPlayerButton.setVisible(false);
	m_ExtraInfo.setVisible(false);
	//m_Separator->setVisible(false);
	if (m_TimerImage)
		m_TimerImage->setVisible(false);

	m_TopBanner->setVisible(false);

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
SpectatorPanel::~SpectatorPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize
//-----------------------------------------------------------------------------
void SpectatorPanel::Initialize(void)
{
	//int x,y,wide,tall;
	//getBounds(x,y,wide,tall);
	m_fHideHelpTime = gEngfuncs.GetClientTime() + SPECTATORPANEL_HELP_DISPLAY_TIME;
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: panel is asked to close
//-----------------------------------------------------------------------------
bool SpectatorPanel::OnClose(void)
{
	bool result = SpectatorPanel::BaseClass::OnClose();
	if (result)// OK to close
	{
		m_menuVisible = false;
		m_insetVisible = false;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: CMenuPanelActionSignalHandler calls this
//-----------------------------------------------------------------------------
void SpectatorPanel::OnActionSignal(int signal, CMenuPanelActionSignalHandler *pCaller)
{
	if (signal == SPECTATOR_PANEL_CMD_NONE)
	{
	}
	else if (signal == SPECTATOR_PANEL_CMD_OPTIONS)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_SpectatorOptionsMenu, false);
	}
	else if (signal == SPECTATOR_PANEL_CMD_NEXTPLAYER)
	{
		gHUD.m_Spectator.FindNextPlayer(false);
	}
	else if (signal == SPECTATOR_PANEL_CMD_PREVPLAYER)
	{
		gHUD.m_Spectator.FindNextPlayer(true);
	}
	else if (signal == SPECTATOR_PANEL_CMD_PLAYERS)// HL20130901
	{
		//gViewPort->ShowCommandMenu(gViewPort->m_PlayerMenu, false);
		if (m_pPlayersMenu)
		{
			/*int a = m_BottomMainButton.getWide()/2;
			a += m_BottomMainButton.GetX();
			a -= m_pPlayersMenu->getWide()/2;*/
			m_pPlayersMenu->setPos(m_BottomMainButton.getWide()/2 + m_BottomMainButton.GetX() - m_pPlayersMenu->getWide()/2, getTall()-m_BottomMainButton.getTall());// button size is changing
			gViewPort->ShowCommandMenu(m_pPlayersMenu, false);
		}
	}
	else if (signal == SPECTATOR_PANEL_CMD_HIDEMENU)
	{
		ShowMenu(false);
	}
	else if (signal == SPECTATOR_PANEL_CMD_CAMERA)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_SpectatorCameraMenu, false);
	}
	else if (signal == SPECTATOR_PANEL_CMD_TOGGLE_INSET)
	{
		gHUD.m_Spectator.SetModes(-1, gHUD.m_Spectator.ToggleInset(gHUD.m_Spectator.m_iInsetMode + 1, false));
	}
	else if (signal > SPECTATOR_PANEL_CMD_PLAYERS)// XDM3038c
	{
		gHUD.m_Spectator.FindPlayer(signal - SPECTATOR_PANEL_CMD_PLAYERS);
	}
	else
	{
		CON_DPRINTF("Unknown SpectatorPanel ActionSingal %d!\n", signal);
		if (pCaller)
			pCaller->m_iSoundType = ACTSIGSOUND_FAILURE;// XDM3038a
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable/disable bottom menus
//-----------------------------------------------------------------------------
void SpectatorPanel::ShowMenu(bool bVisible)
{
	m_OptionButton.setVisible(bVisible);		m_OptionButton.setArmed(false);
	m_CamButton.setVisible(bVisible);			m_CamButton.setArmed(false);
	m_NextPlayerButton.setVisible(bVisible);	m_NextPlayerButton.setArmed(false);
	m_PrevPlayerButton.setVisible(bVisible);	m_PrevPlayerButton.setArmed(false);
	m_BottomMainButton.setPaintBorderEnabled(bVisible);
	m_TopMainLabel.setVisible(bVisible);

	if (bVisible)
	{
		//m_BottomMainButton.setPos(XRES((15 + SPECTATORPANEL_OPTIONS_BUTTON_X + 15) + 31), YRES(6));
		/*bx = m_PrevPlayerButton.GetX() + m_PrevPlayerButton.getWide() + XRES(SPECTATORPANEL_MARGIN);
		bw = m_NextPlayerButton.GetX() - XRES(SPECTATORPANEL_MARGIN) - bx;
		m_BottomMainButton.setBounds(bx, YRES(6), bw, YRES(SPECTATORPANEL_BUTTON_Y));*/
		m_fHideHelpTime = gEngfuncs.GetClientTime();
	}
	else
	{
		/*int iLabelSizeX, iLabelSizeY;
		m_BottomMainButton.getSize(iLabelSizeX, iLabelSizeY);
		m_BottomMainButton.setPos((ScreenWidth / 2) - (iLabelSizeX/2), YRES(6));*/
		m_InsetViewButton.setArmed(false);// XDM3038
		if (m_pPlayersMenu)
			m_pPlayersMenu->Close();

		gViewPort->HideCommandMenu();
		if (m_menuVisible && this->isVisible())// if switching from visible menu to invisible menu, show help text
		{
			m_BottomMainButton.setMainText(BufferedLocaliseTextString("#Spec_Duck"));// XDM
			m_fHideHelpTime = gEngfuncs.GetClientTime() + SPECTATORPANEL_HELP_DISPLAY_TIME;
		}
	}
	m_menuVisible = bVisible;
	setShowCursor(bVisible);
	gViewPort->UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Enable/disable overview window
//-----------------------------------------------------------------------------
void SpectatorPanel::EnableInsetView(bool isEnabled)
{
	int topbartall = m_TopBorder.getTall();
	int offset;
	if (isEnabled)
	{
		int x,y,wide,tall;
		gHUD.m_Spectator.GetInsetWindowBounds(&x,&y,&wide,&tall);// XDM3038a
		m_InsetViewButton.setBounds(x, y, wide, tall);
		offset = x + wide + 2;
		// short black bar to see full inset
		//m_TopBorder.setBounds(offset, 0, ScreenWidth - offset, topbartall);
		//m_TopMainLabel->setBounds(offset, 0, ScreenWidth - (x+wide+2), topbartall);
		//m_InsetViewButton.setVisible(true);
	}
	else
	{
		offset = 0;
		// full black bar, no inset border
		//m_TopBorder.setBounds(offset, 0, ScreenWidth - offset, topbartall);
		//m_TopMainLabel->setBounds(offset, 0, ScreenWidth - offset, topbartall);
		//m_InsetViewButton.setVisible(false);
	}

	m_TopBorder.setBounds(offset, 0, ScreenWidth - offset, topbartall);
	m_ExtraInfo.setSize(ScreenWidth - offset, topbartall);
	//m_TopMainLabel->setBounds(offset, 0, ScreenWidth - offset, topbartall);
	m_InsetViewButton.setVisible(isEnabled);
	m_insetVisible = isEnabled;

	// show banner only in real HLTV mode
	if (gEngfuncs.IsSpectateOnly())
	{
		m_TopBanner->setVisible(true);
		m_TopBanner->setPos(offset, 0);
	}
	else
		m_TopBanner->setVisible(false);

	Update();
	m_CamButton.setMainText(BufferedLocaliseTextString(GetSpectatorModeLabel(g_iUser1)));
}

//-----------------------------------------------------------------------------
// Purpose: Called very often, optimize as much as possible!
//-----------------------------------------------------------------------------
void SpectatorPanel::Update(void)
{
	int j, wx,wy,ww;//,wh;
	bool visible = gHUD.m_Spectator.GetShowStatus();

	m_ExtraInfo.setVisible(visible);
	//m_CurrentTime.setVisible(visible);
	//if (m_TimerImage)
	//	m_TimerImage->setVisible(visible);

	for (j=0; j < MAX_TEAMS; ++j)
	{
		if (visible && IsActiveTeam(j+1))// XDM3038: enable drawing of active teams only
			m_TeamScores[j].setVisible(visible);
		else
			m_TeamScores[j].setVisible(false);
	}

	if (!visible)
	{
		m_CurrentTime.setVisible(false);
		if (m_TimerImage)
			m_TimerImage->setVisible(false);
		return;
	}

	/* wrong! we need to display number of HLTV spectators only!
	if (!gEngfuncs.IsSpectateOnly())// && !gEngfuncs.pDemoAPI->IsPlayingback())
	{
		gHUD.m_Spectator.m_iSpectatorNumber = 0;
		for (j=1; j <= MAX_PLAYERS; ++j)
		{
			if (IsSpectator(j))
				gHUD.m_Spectator.m_iSpectatorNumber++;
		}
	}*/
	gHUD.m_Spectator.GetInsetWindowBounds(&wx,&wy,&ww,NULL);// XDM3038a

	int iTextWidth, iTextHeight;

	// DEBUG
	//m_ExtraInfo.setBgColor(255,255,0,127);
	//m_ExtraInfo.setPaintBackgroundEnabled(true);

	// update extra info field
	char szText[64];
	_snprintf(szText, 63, "%s: %s\n%s: %d\0", BufferedLocaliseTextString("#Spec_Map"), GetMapName(), LookupString("#Spectators"), gHUD.m_Spectator.m_iSpectatorNumber);
	szText[63] = '\0';
	m_ExtraInfo.setText(szText);
	m_ExtraInfo.getTextSize(iTextWidth, iTextHeight);
	int xPos = m_TopBorder.getWide() - max(XRES(64),iTextWidth) - XRES(4);// map names have different length
	m_ExtraInfo.setPos(xPos, YRES(1));
	if (m_TimerImage)
	{
		m_TimerImage->setPos(xPos, YRES(2) + iTextHeight);
		m_CurrentTime.setPos(xPos+m_TimerImage->getWide(), YRES(2) + iTextHeight);
	}
	char str[32];
	int iwidth, iheight;
	int r,g,b;// XDM
	xPos -= XRES(8);
	for (j=0; j < MAX_TEAMS; ++j)
	{
		GetTeamColor(j+1, r,g,b);
		m_TeamScores[j].setFgColor(r,g,b,0);// alpha is reversed
		_snprintf(str, 32, "%d | %d\0", g_TeamInfo[j+1].score, g_TeamInfo[j+1].scores_overriden);
		m_TeamScores[j].setText(str);
		m_TeamScores[j].getTextSize(iwidth, iheight);
		m_TeamScores[j].setBounds(xPos-iwidth, YRES(1) + (iheight * j), iwidth, iheight);
	}
	/* ^ ScreenWidth - (iTextWidth + XRES(4+2+4+2+offset) + iwidth)*/

	if (m_fHideHelpTime <= gHUD.m_flTime/* && m_fHideHelpTime != 0.0f*/)// if not, keep showing help text
	{
		// MOVED FROM UpdateSpectatorPanel()
		const char *name = NULL;
		int player = 0;
		char bottomText[128];

		// check if we're locked onto a target, show the player's name
		if ((g_iUser1 != OBS_ROAMING) && IsActivePlayer(g_iUser2))
			player = g_iUser2;

		// special case in free map and inset off, don't show names
		if ((g_iUser1 == OBS_MAP_FREE) && (gHUD.m_Spectator.m_iInsetMode == INSET_OFF))
			name = NULL;
		else if (player > 0)
		{
			GetPlayerInfo(player, &g_PlayerInfoList[player]);
			name = g_PlayerInfoList[player].name;
		}

		// fill in bottomText: player or mode
		if (player > 0 && g_iUser1 != OBS_ROAMING && g_iUser1 != OBS_MAP_FREE)
		{
			m_BottomMainButton.m_ColorNormal = GetTeamColor(g_PlayerExtraInfo[player].teamnumber);// looks better GetPlayerColor(player);

			if (gHUD.m_Spectator.GetAutoDirector())
				_snprintf(bottomText, 128, "#Spec_Auto %s\0", name);
			else if (name)
				strncpy(bottomText, name, 128);//_snprintf(bottomText, 128, "%s", name);
			else
				strncpy(bottomText, "ERROR!\0", 128);
		}
		else// restore GUI color
		{
			//old	m_BottomMainButton->setFgColor(Scheme::sc_primary1);
			m_BottomMainButton.m_ColorNormal = m_BottomMainButtonFg;
			_snprintf(bottomText, 128, "#Spec_Mode%d\0", g_iUser1);
		}
		if (gEngfuncs.IsSpectateOnly())
		{
			bottomText[127] = '\0';
			strncat(bottomText, " - HLTV", 8);
			bottomText[127] = '\0';
		}
		m_BottomMainButton.setMainText(BufferedLocaliseTextString(bottomText));
		//m_fHideHelpTime = 0.0f;// XDM3035c: don't do this every frame
	}

	/*if (gHUD.m_iTimeLimit != 0)// XDM3038a
	{
		char szTimeLeftText[16];
		_snprintf(szTimeLeftText, 16, "%d:%02d\n", (int)(gHUD.m_flTimeLeft/60), ((int)gHUD.m_flTimeLeft % 60));
		szTimeLeftText[15] = 0;
		m_CurrentTime.setText(szTimeLeftText);
	}*/
	// WARNING: changes to this code must be also applied to ScorePanel::UpdateCounters() timer section
	float displaynumber;
	char szTimeText[16];
	if (gHUD.m_iGameState == GAME_STATE_ACTIVE)
	{
		szTimeText[0] = 0;
		if (gHUD.m_iTimeLimit != 0)// time limit
			displaynumber = gHUD.m_flTimeLeft;
		else// when no limit set, display elapsed time
			displaynumber = gHUD.m_flTime - gHUD.m_flGameStartTime;
	}
	else if (gHUD.m_iGameState == GAME_STATE_WAITING || gHUD.m_iIntermission > 0)// intermission/join remaining time
	{
		szTimeText[0] = 0;
		displaynumber = gHUD.m_flTimeLeft;
	}
	else
	{
		szTimeText[0] = 1;
		displaynumber = 0;// dat warning...
	}
	if (szTimeText[0] == 0)// use as a flag
	{
		_snprintf(szTimeText, 16, "%d:%02d\0", (int)(displaynumber/60), ((int)abs(displaynumber) % 60));
		szTimeText[15] = 0;
		m_CurrentTime.setText(szTimeText);
		m_CurrentTime.setVisible(true);
		if (m_TimerImage)
			m_TimerImage->setVisible(true);
	}
	else
	{
		m_CurrentTime.setVisible(false);
		if (m_TimerImage)
			m_TimerImage->setVisible(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 1 pass the key to the next window, 0 - handled, stop
//-----------------------------------------------------------------------------
int SpectatorPanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down)
	{
		if (/*keynum == K_CTRL || */keynum == K_ESCAPE)// override these keys
		{
			ShowMenu(!m_menuVisible);
			return 0;
		}
		else if (m_menuVisible)// XDM3038: some hotheys
		{
			if (keynum == m_BottomMainButton.getBoundKey())// these checks are more proper, but m_menuVisible is faster. && m_BottomMainButton.isVisible())
			{
				m_BottomMainButton.fireActionSignal();
				return 0;
			}
			else if (keynum == m_OptionButton.getBoundKey())// && m_OptionButton.isVisible())
			{
				m_OptionButton.fireActionSignal();
				return 0;
			}
			else if (keynum == m_PrevPlayerButton.getBoundKey())// && m_PrevPlayerButton.isVisible())
			{
				m_PrevPlayerButton.fireActionSignal();
				return 0;
			}
			else if (keynum == m_NextPlayerButton.getBoundKey())// && m_NextPlayerButton.isVisible())
			{
				m_NextPlayerButton.fireActionSignal();
				return 0;
			}
			else if (keynum == m_CamButton.getBoundKey())// && m_CamButton.isVisible())
			{
				m_CamButton.fireActionSignal();
				return 0;
			}
			else if (keynum == m_InsetViewButton.getBoundKey())// && m_InsetViewButton.isVisible())
			{
				m_InsetViewButton.fireActionSignal();
				return 0;
			}
		}
		/*else if (keynum == K_ENTER || keynum == K_KP_ENTER)
			ToggleInset()
		else if (K_LEFTARROW)*/
	}
	// DON'T!!	return BaseClass::KeyInput(down, keynum, pszCurrentBinding);// NEVER allow to close this menu by hotkeys!
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *image - 
//-----------------------------------------------------------------------------
void SpectatorPanel::SetBanner(const char *image)
{
	if (m_TopBanner)
		m_TopBanner->LoadImage(image);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a
//-----------------------------------------------------------------------------
int SpectatorPanel::GetBorderHeight(void)
{
	return m_TopBorder.getTall();
}
