#include "hud.h"
#include "cl_util.h"
#include "pm_shared.h"// XDM: spectator modes
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
#include "vgui_SpectatorPanel.h"
// undone #include "vgui_StatsPanel.h"
#include "vgui_MessageWindow.h"
#include "voice_status.h"
#include "keydefs.h"

// UNDONE: properly initialize and remove stats panel (it doesn't signal when it's closed!)

// grid size is marked out for 640x480 screen
columninfo_t g_ColumnInfo[SBOARD_NUM_COLUMNS] =
{
	{NULL,			24,			Label::a_east},		// tracker column
	{"#NAME",		140,		Label::a_west},
	{"#TEAMSCORE",	56,			Label::a_west},// UNDINE: don't draw this in teamplay?
	{"#SCORE",		40,			Label::a_east},
	{"#DEATHS",		46,			Label::a_east},
	{"#LATENCY",	46,			Label::a_east},
	{"#VOICE",		40,			Label::a_east},
	{NULL,			2,			Label::a_east},		// blank column to take up the slack
};

//-----------------------------------------------------------------------------
// Purpose: Sort array of TEAM IDs in DECREASING order
// Note   : The array is sorted in increasing order, as defined by the comparison function.
// Input  : *elem1 - TEAM_ID*
//			*elem2 - TEAM_ID*
// Output : int <0, 0, >0 : elem1 less than, equivalent to, greater than elem2
//-----------------------------------------------------------------------------
int teaminfocmp(const void *elem1, const void *elem2)
{
	TEAM_ID tid1 = *(TEAM_ID *)elem1;
	TEAM_ID tid2 = *(TEAM_ID *)elem2;
	// push invalid/unused teams to the end of list
	if (!IsActiveTeam(tid1))// TEAM_NONE must be invalid here!
	{
		if (IsActiveTeam(tid2))// second is valid, push team1 to the bottom
			return 1;
		else if (tid1 < tid2)// both invalid, sort by indexes
			return -1;
		else if (tid1 > tid2)
			return 1;
		else// in case someone fills the array with bogus values
			return 0;
	}
	else if (!IsActiveTeam(tid2))
		return -1;

	team_info_t *team1 = &g_TeamInfo[tid1];
	team_info_t *team2 = &g_TeamInfo[tid2];

	//if (gHUD.m_iGameType > GT_TEAMPLAY)// these game types use extra team score
	if (IsExtraScoreBasedGame(gHUD.m_iGameType))// XDM3037a
	{
		if (team1->scores_overriden > team2->scores_overriden)
			return -1;
		else if (team1->scores_overriden < team2->scores_overriden)
			return 1;
		// else sort by frags
	}

	// now, sort by 1st priority criteria
	if (team1->score > team2->score)
		return -1;
	else if (team1->score < team2->score)
		return 1;
	// else ==, sort by deaths

	// sort by 2nd priority criteria
	if (team1->deaths < team2->deaths)
		return -1;
	else if (team1->deaths > team2->deaths)
		return 1;

	// sort by 3rd priority criteria
	if (team1->players < team2->players)
		return -1;
	else if (team1->players > team2->players)
		return 1;
	// else sort by ID?

	if (tid1 < tid2)// sort by index (only to mimic server)
		return -1;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Sort array of player IDs. GT_DEATHMATCH version, sorts by score.
// Note   : The array is sorted in increasing order, as defined by the comparison function.
// Note   : So return 1 to push unwanted players to the end of the list
// Input  : *elem1 - int* CLIENT_INDEX
//			*elem2 - int*
// Output : int <0, 0, >0 : elem1 less than, equivalent to, greater than elem2
//-----------------------------------------------------------------------------
int playerinfocmp(const void *elem1, const void *elem2)
{
	short player1 = *(short *)elem1;
	short player2 = *(short *)elem2;

	// push invalid/unused player slots to the end of list
	if (!IsValidPlayerIndex(player1))// XDM3038a
	{
		if (IsValidPlayerIndex(player2))// second is valid, push player1 to the bottom
			return 1;
		else if (player1 < player2)// both invalid, sort by indexes
			return -1;
		else if (player1 > player2)
			return 1;
		else// in case someone fills the array with bogus values
			return 0;
	}
	else if (!IsValidPlayerIndex(player2))// player1 is valid by now
		return -1;

	// first, rule out all spectators
	if (!IsSpectator(player1) && IsSpectator(player2))
		return -1;
	else if (IsSpectator(player1) && !IsSpectator(player2))
		return 1;

	// now, sort by 1st priority criteria
	if (g_PlayerExtraInfo[player1].score > g_PlayerExtraInfo[player2].score)
		return -1;
	else if (g_PlayerExtraInfo[player1].score < g_PlayerExtraInfo[player2].score)
		return 1;
	// else sort by deaths

	// sort by 2nd priority criteria
	if (g_PlayerExtraInfo[player1].deaths < g_PlayerExtraInfo[player2].deaths)
		return -1;
	else if (g_PlayerExtraInfo[player1].deaths > g_PlayerExtraInfo[player2].deaths)
		return 1;

	// sort by 3rd priority criteria
	// don't have to check for != 0 here
	if (g_PlayerExtraInfo[player1].lastscoretime < g_PlayerExtraInfo[player2].lastscoretime)
		return -1;
	else if (g_PlayerExtraInfo[player1].lastscoretime > g_PlayerExtraInfo[player2].lastscoretime)
		return 1;

	if (player1 < player2)// sort by index (TODO: sort by play time, which is unavailable)
		return -1;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Sort array of player IDs. GT_LMS version, sorts by remaining lives.
// Note   : The array is sorted in increasing order, as defined by the comparison function.
// Note   : So return 1 to push unwanted players to the end of the list
// Input  : *elem1 - int* CLIENT_INDEX
//			*elem2 - int*
// Output : int <0, 0, >0 : elem1 less than, equivalent to, greater than elem2
//-----------------------------------------------------------------------------
int playerinfocmp_lms(const void *elem1, const void *elem2)
{
	short player1 = *(short *)elem1;
	short player2 = *(short *)elem2;

	// push invalid/unused player slots to the end of list
	if (!IsValidPlayerIndex(player1))// XDM3038a
	{
		if (IsValidPlayerIndex(player2))// second is valid, push player1 to the bottom
			return 1;
		else if (player1 < player2)// both invalid, sort by indexes
			return -1;
		else if (player1 > player2)
			return 1;
		else// in case someone fills the array with bogus values
			return 0;
	}
	else if (!IsValidPlayerIndex(player2))// player1 is valid by now
		return -1;

	if (!IsSpectator(player1) && IsSpectator(player2))
		return -1;
	else if (IsSpectator(player1) && !IsSpectator(player2))
		return 1;

	if (g_PlayerExtraInfo[player1].deaths < g_PlayerExtraInfo[player2].deaths)
		return -1;
	else if (g_PlayerExtraInfo[player1].deaths > g_PlayerExtraInfo[player2].deaths)
		return 1;

	if (g_PlayerExtraInfo[player1].score > g_PlayerExtraInfo[player2].score)
		return -1;
	else if (g_PlayerExtraInfo[player1].score < g_PlayerExtraInfo[player2].score)
		return 1;

	// don't have to check for != 0 here
	if (g_PlayerExtraInfo[player1].lastscoretime < g_PlayerExtraInfo[player2].lastscoretime)
		return -1;
	else if (g_PlayerExtraInfo[player1].lastscoretime > g_PlayerExtraInfo[player2].lastscoretime)
		return 1;

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Create the ScoreBoard panel
// Warning: ScorePanel should never be removed (m_iRemoveMe = 0)!
// Input  : x y - position
//			wide tall - size
//-----------------------------------------------------------------------------
ScorePanel::ScorePanel(int x, int y, int wide, int tall) : CMenuPanel(0, x,y,wide,tall)
{
	DBG_PRINTF("ScorePanel::ScorePanel()\n");
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	ASSERT(pSchemes != NULL);
	if (!pSchemes)
		return;

	CScheme *pTitleScheme = pSchemes->getScheme("Scoreboard Title Text");
	CScheme *pSmallTextScheme = pSchemes->getScheme("Scoreboard Small Text");
	// default	CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");
	m_pTextScheme = pSchemes->getScheme("Scoreboard Text");

	m_pFontTitle = pTitleScheme->font;
	m_pFontScore = m_pTextScheme->font;
	m_pFontSmall = pSmallTextScheme->font;

	ASSERTSZ(m_pFontSmall != NULL, "Font failed to load possibly because you minimized main window or pressed something wrong (like Ctrl+F4) during initialization!\n");
	ASSERT(m_pFontTitle != NULL);
	ASSERT(m_pFontSmall != NULL);

	m_pCurrentHighlightLabel = NULL;
	m_iHighlightRow = -1;
	m_StatsWindowTextBuffer[0] = '\0';
	m_fTitleNextUpdateTime = 0;

	//m_pTrackerIcon = vgui_LoadTGANoInvertAlpha("gfx/vgui/640_scoreboardtracker.tga");

	setBgColor(0,0,0, SBOARD_PANEL_ALPHA);
	setBorder(new CDefaultLineBorder());
	setPaintBorderEnabled(true);
	setBackgroundMode(BG_FILL_NORMAL);

	//int xpos = g_ColumnInfo[0].m_Width + 3;
	//if (ScreenWidth >= 640)
	//	xpos = XRES(xpos);// only expand column size for res greater than 640

	const int title_border_offset = 2;
	int lw = 16, lh = 16;
	m_TitleIcon.getImageSize(lw, lh);

	//int xpos = XRES(title_border_offset*2 + lw);
	// Initialize the top title.
	m_TitleLabel.setParent(this);
	m_TitleLabel.setFont(m_pFontTitle);
	m_TitleLabel.setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_TitleLabel.setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_TitleLabel.setContentAlignment(vgui::Label::a_west);
	m_TitleLabel.setBounds(XRES(title_border_offset*2) + lw, YRES(title_border_offset), wide-1, SBOARD_TITLE_SIZE_Y);// XDM3035: YRES(4), wide-1 to not overlap the border
	m_TitleLabel.setContentFitted(false);

	// Setup the header (labels like "name", "class", etc..).
	m_HeaderGrid.setParent(this);
	m_HeaderGrid.SetDimensions(SBOARD_NUM_COLUMNS, 1);
	m_HeaderGrid.SetSpacing(0, 0);

	for (int i=0; i < SBOARD_NUM_COLUMNS; ++i)
	{
		if (g_ColumnInfo[i].m_pTitle)
		{
			if (g_ColumnInfo[i].m_pTitle[0] == '#')
				m_HeaderLabels[i].setTextSimple(BufferedLocaliseTextString(g_ColumnInfo[i].m_pTitle));
			else
				m_HeaderLabels[i].setTextSimple(g_ColumnInfo[i].m_pTitle);
		}
		int xwide = g_ColumnInfo[i].m_Width;
		if (ScreenWidth >= 640)
		{
			xwide = XRES(xwide);
		}
		else// if (ScreenWidth == 400)
		{
			// hack to make 400x300 resolution scoreboard fit
			if (i == 1)// reduces size of player name cell
				xwide -= 28;
			else if (i == 0)// tracker icon cell
				xwide -= 8;
		}

		m_HeaderGrid.SetColumnWidth(i, xwide);
		m_HeaderGrid.SetEntry(i, 0, &m_HeaderLabels[i]);

		m_HeaderLabels[i].setBgColor(0,0,0,255);
		//m_HeaderLabels[i].setBgColor(255,255,0,127);// TEST
		m_HeaderLabels[i].setFgColor(m_pTextScheme->FgColor.r, m_pTextScheme->FgColor.g, m_pTextScheme->FgColor.b, 255-m_pTextScheme->FgColor.a);
		m_HeaderLabels[i].setFont(m_pFontSmall);
		m_HeaderLabels[i].setContentAlignment(g_ColumnInfo[i].m_Alignment);

		int yres = 12;
		if (ScreenHeight >= 480)
			yres = YRES(yres);

		m_HeaderLabels[i].setSize(50, yres);
	}
	// Set the width of the last column to be the remaining space.
	int ex, ey, ew, eh;
	m_HeaderGrid.GetEntryBox(SBOARD_NUM_COLUMNS - 2, 0, ex, ey, ew, eh);
	m_HeaderGrid.SetColumnWidth(SBOARD_NUM_COLUMNS - 1, (wide - SBOARD_X_BORDER) - (ex + ew));
	m_HeaderGrid.AutoSetRowHeights();
	m_HeaderGrid.setBounds(SBOARD_X_BORDER, SBOARD_TITLE_SIZE_Y, wide - SBOARD_X_BORDER*2, m_HeaderGrid.GetRowHeight(0));
	m_HeaderGrid.setBgColor(0,0,0,255);
	m_HeaderGrid.setPaintBackgroundEnabled(false);

	// Now setup the listbox with the actual player data in it.
	int headerX, headerY, headerWidth, headerHeight;
	m_HeaderGrid.getBounds(headerX, headerY, headerWidth, headerHeight);
	m_PlayerList.setBounds(headerX, headerY+headerHeight, headerWidth, tall - headerY - headerHeight - YRES(SBOARD_BOTTOM_LABEL_Y));
	m_PlayerList.setParent(this);
	m_PlayerList.setBgColor(0,0,0,255);
	//m_PlayerList.setBgColor(191,255,255,127);// TEST
	m_PlayerList.setPaintBackgroundEnabled(false);
	addInputSignal(new CMenuHandler_ScrollInput2(m_PlayerList.GetScrollBar()));// XDM3038a

	for (size_t row=0; row < SBOARD_NUM_ROWS; ++row)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];
		pGridRow->SetDimensions(SBOARD_NUM_COLUMNS, 1);
		for(size_t col=0; col < SBOARD_NUM_COLUMNS; ++col)
		{
			m_PlayerEntries[col][row].setContentFitted(false);
			m_PlayerEntries[col][row].setRow(row);
			m_PlayerEntries[col][row].addInputSignal(this);
			pGridRow->SetEntry(col, 0, &m_PlayerEntries[col][row]);
		}
		pGridRow->setBgColor(0,0,0,255);
		//pGridRow->SetSpacing(2, 0);
		pGridRow->SetSpacing(0, 0);
		pGridRow->CopyColumnWidths(&m_HeaderGrid);
		pGridRow->AutoSetRowHeights();
		//pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->setSize(pGridRow->getWide(), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();
		m_PlayerList.AddItem(pGridRow);
	}

	// Add the hit test panel. It is invisible and traps mouse clicks so we can go into squelch mode.
	m_HitTestPanel.setBounds(0, 0, wide, tall);
	m_HitTestPanel.setParent(this);
	m_HitTestPanel.setBgColor(0,0,0,255);
	m_HitTestPanel.setFgColor(0,0,0,255);
	m_HitTestPanel.setPaintBorderEnabled(false);
	m_HitTestPanel.setPaintBackgroundEnabled(false);
	m_HitTestPanel.setPaintEnabled(false);
	m_HitTestPanel.addInputSignal(this);

	m_pStatsPanel = NULL;

	const char *label = "00:00";
	if (m_pFontSmall)
		m_pFontSmall->getTextSize(label, lw, lh);

	// XDM3035: status bar indicated game limits: score and frags
	//LocaliseTextString("%s @ %s, #Score_limit: #TEAMSCORE %d, #SCORE %d\0", m_szScoreLimitLabelFmt, SBOARD_BOTTOM_TEXT_LEN/*sizeof(char)*/);
	m_BottomLabel.setParent(this);
	m_BottomLabel.setBounds(SBOARD_X_BORDER, tall-m_pFontSmall->getTall()-YRES(PANEL_INNER_OFFSET), wide-m_CurrentTimeLabel.getWide()-SBOARD_X_BORDER*2, SBOARD_BOTTOM_LABEL_Y);
	m_BottomLabel.setFont(m_pFontSmall);
	//m_BottomLabel.setFgColor(Scheme::sc_primary1);
	//test	m_BottomLabel.setBgColor(0,255,255,127);
	m_BottomLabel.setFgColor(pSmallTextScheme->FgColor.r, pSmallTextScheme->FgColor.g, pSmallTextScheme->FgColor.b, 255-pSmallTextScheme->FgColor.a);
	m_BottomLabel.setBgColor(pSmallTextScheme->BgColor.r, pSmallTextScheme->BgColor.g, pSmallTextScheme->BgColor.b, 255-pSmallTextScheme->BgColor.a);
	m_BottomLabel.setContentAlignment(vgui::Label::a_west);
	//m_BottomLabel.setContentFitted(true);
	m_BottomLabel.setPaintBackgroundEnabled(false);
	//m_BottomLabel.setText(m_szScoreLimitLabelFmt);

	m_CurrentTimeLabel.setParent(this);
	//m_CurrentTimeLabel.setPos(wide-XRES(lw+2), tall-YRES(lh+1));// XDM
	m_CurrentTimeLabel.setBounds(wide-lw-XRES(2)-SBOARD_X_BORDER, tall-SBOARD_BOTTOM_LABEL_Y-YRES(PANEL_INNER_OFFSET)/*YRES(lh+1)*/, lw+2, SBOARD_BOTTOM_LABEL_Y);
	m_CurrentTimeLabel.setFont(m_pFontSmall);
	//m_CurrentTimeLabel.setFgColor(Scheme::sc_primary1);
	m_CurrentTimeLabel.setFgColor(pSmallTextScheme->FgColor.r, pSmallTextScheme->FgColor.g, pSmallTextScheme->FgColor.b, 255-pSmallTextScheme->FgColor.a);
	m_CurrentTimeLabel.setBgColor(pSmallTextScheme->BgColor.r, pSmallTextScheme->BgColor.g, pSmallTextScheme->BgColor.b, 255-pSmallTextScheme->BgColor.a);
	m_CurrentTimeLabel.setContentAlignment(vgui::Label::a_east);
	//m_CurrentTimeLabel.setContentFitted(true);
	m_CurrentTimeLabel.setPaintBackgroundEnabled(false);
	//m_CurrentTimeLabel.setText(label);

	// Initialize these last so they remain on top
	m_ButtonClose.setParent(this);
	//m_pCloseButton = new CommandButton("x", wide-XRES(12 + 4), YRES(2), XRES(12), YRES(12), false, false, false, NULL, pButtonScheme);
	m_ButtonClose.setParent(this);
	m_ButtonClose.setBounds(wide-XRES(12 + 4), YRES(2), XRES(12), YRES(12));
	m_ButtonClose.m_bShowHotKey = false;
	m_ButtonClose.addActionSignal(new CMenuHandler_StringCommand(NULL, "-showscores", true));
	m_ButtonClose.setFont(m_pFontTitle);// custom large font
	m_ButtonClose.setBoundKey((char)255);
	m_ButtonClose.setContentAlignment(Label::a_center);
	m_ButtonClose.setMainText("x");
	//TODO?	m_pCloseButton->addActionSignal(new CMenuPanelActionSignalHandler(this, IDCLOSE));

	//m_pStatsModeButton = new CommandButton(BufferedLocaliseTextString("#Show_Stats"), wide/2-XRES(56)/2, tall-m_pFontSmall->getTall()-YRES(2), XRES(56), m_pFontSmall->getTall()+2, false, false, false, NULL, pButtonScheme);
	m_ButtonStatsMode.setParent(this);
	m_ButtonStatsMode.setBounds(wide/2-XRES(56)/2, tall-m_pFontSmall->getTall()-YRES(2), XRES(56), m_pFontSmall->getTall()+2);
	m_ButtonStatsMode.addActionSignal(new CMenuPanelActionSignalHandler(this, SCORE_PANEL_CMD_PLAYERSTAT));
	m_ButtonStatsMode.setVisible(false);
	m_ButtonStatsMode.setFont(m_pFontSmall);// custom small font
	m_ButtonStatsMode.m_bShowHotKey = false;
	m_ButtonStatsMode.setBoundKey('s');
	m_ButtonStatsMode.setContentAlignment(Label::a_center);
	m_ButtonStatsMode.setMainText(BufferedLocaliseTextString("#Show_Stats"));

	// +m_ButtonStatsMode.getWide()/2-CMENU_SIZE_X/2
	/*int x3, y3, w3, t3;
	m_ButtonStatsMode.getBounds(x3, y3, w3, t3);
	ASSERT(x3 == m_ButtonStatsMode.GetX());
	ASSERT(y3 == m_ButtonStatsMode.GetY());
	ASSERT(w3 == m_ButtonStatsMode.getWide());
	ASSERT(t3 == m_ButtonStatsMode.getTall());*/
	m_pPlayersMenu = new CPlayersCommandMenu(NULL, this, true, true, true, SCORE_PANEL_CMD_PLAYERSTAT, CMENU_DIR_UP, (m_ButtonStatsMode.getWide()/2) + m_ButtonStatsMode.GetX() - (CMENU_SIZE_X/2), getTall()-m_ButtonStatsMode.getTall(), CMENU_SIZE_X, 1);
	if (m_pPlayersMenu)
	{
		m_pPlayersMenu->setParent(this);
		m_pPlayersMenu->setVisible(false);
		m_pPlayersMenu->m_iButtonSizeY = CBUTTON_SIZE_Y*0.75;
	}

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Open
//-----------------------------------------------------------------------------
void ScorePanel::Open(void)
{
	DBG_PRINTF("ScorePanel::Open()\n");
	if (gEngfuncs.GetMaxClients() <= 1)
		return;

	// XDM3038a	RebuildTeams();
	//setVisible(true);
	setShowCursor(false);// XDM3037a
	CheckStatsPanel();// XDM3037
	if (m_pStatsPanel)// fix if stats panel stuck after map change
		m_pStatsPanel->Close();

	CMenuPanel::Open();//	requestFocus();// XDM3037: so cursorMoved() may work
	//if (isVisible())
	Update();// XDM3038a
	m_HitTestPanel.setVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Panel is closing
// Output : Return true to ALLOW and false to PREVENT closing
//-----------------------------------------------------------------------------
bool ScorePanel::OnClose(void)
{
	DBG_PRINTF("ScorePanel::OnClose()\n");
	if (m_pPlayersMenu)
		m_pPlayersMenu->Close();

	if (m_pStatsPanel)// close sats panel if requested (even if IsPersistent)
	{
		if (m_pStatsPanel->isVisible())
			m_pStatsPanel->Close();// XDM3038a
	}
	//if (gHUD.m_iIntermission > 0 && gHUD.m_iActive)// XDM3038: prevent panel from closing during intermission
	if (IsPersistent())// additional barrier
		return false;

	bool result = ScorePanel::BaseClass::OnClose();
	if (result)// OK to close
	{
		m_HitTestPanel.setVisible(false);

		CheckStatsPanel();
		if (m_pStatsPanel)
		{
#if defined (USE_EXCEPTIONS)
			try
			{
#endif
				m_pStatsPanel->Close();
				// done inside	removeChild(m_pStatsPanel);
				m_pStatsPanel = NULL;
#if defined (USE_EXCEPTIONS)
			}
			catch (...)
			{
				conprintf(1, "CL: WARNING: ScorePanel::OnClose(): m_pStatsPanel exception!\n");
			}
#endif
		}
		GetClientVoiceMgr()->StopSquelchMode();
		setShowCursor(false);// XDM3037a
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: don't allow this window to be closed during intermission
// Output : true or false
//-----------------------------------------------------------------------------
bool ScorePanel::IsPersistent(void)
{
	if (gHUD.m_iIntermission > 0 && gHUD.m_iActive)// prevent panel from closing during intermission
		return true;

	return false;// ScorePanel::BaseClass::IsPersistent();?
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038
// Output : Return true to ALLOW
//-----------------------------------------------------------------------------
bool ScorePanel::AllowConcurrentWindows(void)
{
	//if (IsPersistent()) - allow other windows (like music player) only during intermission?
	return true;
}




//-----------------------------------------------------------------------------
// Purpose: XDM3038: SetClickMode
// Input  : bOn - 
//-----------------------------------------------------------------------------
void ScorePanel::SetClickMode(bool bOn)
{
	if (bOn && !GetClientVoiceMgr()->IsInSquelchMode())
	{
		m_HitTestPanel.setVisible(false);
		GetClientVoiceMgr()->StartSquelchMode();
	}
	else if (!bOn && GetClientVoiceMgr()->IsInSquelchMode())
	{
		m_iHighlightRow = -1;
		m_HitTestPanel.setVisible(true);
		GetClientVoiceMgr()->StopSquelchMode();
	}
	setShowCursor(bOn);
	setCaptureInput(bOn);
	gViewPort->UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Temporarily set title
// Input  : *pText - 
//			duration - seconds
//-----------------------------------------------------------------------------
void ScorePanel::SetTitle(const char *pText, const float &duration)// XDM3038a
{
	if (m_fTitleNextUpdateTime > gHUD.m_flTime)
		return;

	m_TitleLabel.setText(pText);
	m_fTitleNextUpdateTime = gHUD.m_flTime + duration;
}

//-----------------------------------------------------------------------------
// Purpose: mousePressed
// Input  : code - MOUSE_LEFT
//			*panel - 
//-----------------------------------------------------------------------------
void ScorePanel::mousePressed(MouseCode code, Panel *panel)
{
	if (panel == NULL)
		return;
	if (!hasFocus())// XDM3038c
		return;

	if (!GetClientVoiceMgr()->IsInSquelchMode())
	{
		SetClickMode(true);
	}
	else if (m_iHighlightRow >= 0)
	{
		// mouse has been pressed, toggle mute state
		CLIENT_INDEX iPlayer = m_iSortedRows[m_iHighlightRow];
		if (iPlayer > 0)
		{
			// print text message
			hud_player_info_t *pl_info = &g_PlayerInfoList[iPlayer];
			if (pl_info && pl_info->name && pl_info->name[0])
			{
				char string[128];
				string[0] = 0;
				if (GetClientVoiceMgr()->IsPlayerBlocked(iPlayer))
				{
					if (GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, false))// remove mute
						_snprintf(string, 128, BufferedLocaliseTextString("#Unmuted\n"), pl_info->name);
				}
				else
				{
					if (GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, true))// mute the player
						_snprintf(string, 128, BufferedLocaliseTextString("#Muted\n"), pl_info->name);
					
				}
				if (string[0] == 0)
					LocaliseTextString("#ERROR\n", string, 128);

				gHUD.m_SayText.SayTextPrint(string, 0, false);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Lower level of mouse movement handling
// Input  : x y - 
//			*panel - 
//-----------------------------------------------------------------------------
void ScorePanel::cursorMoved(int x, int y, Panel *panel)
{
	if (hasFocus() && GetClientVoiceMgr()->IsInSquelchMode())// XDM3037: focus
	{
		// look for which cell the mouse is currently over
		for (int i = 0; i < SBOARD_NUM_ROWS; ++i)
		{
			int row, col;
			if (m_PlayerGrids[i].getCellAtPoint(x, y, row, col))
			{
				MouseOverCell(i, col);
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void ScorePanel::Initialize(void)
{
	DBG_PRINTF("ScorePanel::Initialize()\n");
	// Clear out scoreboard data
	m_iLastKilledBy = 0;
	m_fLastKillDisplayStopTime = 0.0f;
	//m_iShowscoresHeld = 0;
	m_iHighlightRow = 0;
	m_iWinner = 0;// XDM3037
	m_flWaitForStatsTime = 0.0f;// XDM3038c

	unsigned short i = 0;
	for (i = 0; i < SBOARD_NUM_ROWS; ++i)
	{
		m_iSortedRows[i] = 0;
		m_iRowType[i] = 0;
	}

	for (i = 0; i <= MAX_PLAYERS; ++i)
	{
		//m_SortedPlayers[i] = 0;
		// XDM3037a	m_bHasBeenSorted[i] = false;
	}

	//for (i = 0; i < MAX_TEAMS; ++i)
	//	m_bHasBeenSortedTeam[i] = false;

	for (i = 0; i <= MAX_TEAMS; ++i)// 4+1
	{
		m_SortedTeams[i] = TEAM_NONE;
		m_iBestPlayer[i] = 0;// XDM3037
	}
	m_bHasStats = false;// XDM3037

	Reset();// XDM3038

	if (IsRoundBasedGame(gHUD.m_iGameType))
		LocaliseTextString("%s @ %s (%d #Player_plural), #ROUND %d/%d, #Score_limit: #TEAMSCORE %u", m_szScoreLimitLabelFmt, SBOARD_BOTTOM_TEXT_LEN/**sizeof(char)*/);
	else if (IsTeamGame(gHUD.m_iGameType))// XDM3037a: WARNING: keep an eye on number of %parameters!
		LocaliseTextString("%s @ %s (%d #Player_plural), #Score_limit: #TEAMSCORE %u", m_szScoreLimitLabelFmt, SBOARD_BOTTOM_TEXT_LEN/**sizeof(char)*/);
	else
		LocaliseTextString("%s @ %s (%d #Player_plural), #Score_limit: #SCORE %u", m_szScoreLimitLabelFmt, SBOARD_BOTTOM_TEXT_LEN/**sizeof(char)*/);

	if (gHUD.m_iDeathLimit > 0)
	{
		strcat(m_szScoreLimitLabelFmt, BufferedLocaliseTextString(", #Death_limit: %u\0"));// XDM3038c
		m_szScoreLimitLabelFmt[SBOARD_BOTTOM_TEXT_LEN-1] = '\0';
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
// Warning: XDM3038a: avoid calling this too often!
//-----------------------------------------------------------------------------
void ScorePanel::Update(void)
{
	if (GetMapName())
	{
		// Fill score limit text at the bottom
		char szScoreLimitLabelText[128];
		if (IsRoundBasedGame(gHUD.m_iGameType))// XDM3037: print round info
			_snprintf(szScoreLimitLabelText, 128, m_szScoreLimitLabelFmt, GetGameDescription(gHUD.m_iGameType), GetMapName(), gEngfuncs.GetMaxClients(), gHUD.m_iRoundsPlayed, gHUD.m_iRoundsLimit, gHUD.m_iScoreLimit, gHUD.m_iDeathLimit);// XDM3037a
		else
			_snprintf(szScoreLimitLabelText, 128, m_szScoreLimitLabelFmt, GetGameDescription(gHUD.m_iGameType), GetMapName(), gEngfuncs.GetMaxClients(), gHUD.m_iScoreLimit, gHUD.m_iDeathLimit);

		szScoreLimitLabelText[127] = '\0';
		m_BottomLabel.setText(szScoreLimitLabelText);
	}

	// Fill out the title label
	if (m_fTitleNextUpdateTime <= gHUD.m_flTime)// XDM3038a: custom title was set for a while
	{
		m_fTitleNextUpdateTime = 0;
		if (g_iUser1 == OBS_INTERMISSION && IsActivePlayer(m_iWinner))// XDM3038: m_iWinner set externally (from the server)
		{
			if (!IsTeamGame(gHUD.m_iGameType))// single winner
			{
				if (g_PlayerInfoList[m_iWinner].name != NULL)// get winner's name
				{
					char label[48];
					client_textmessage_t *msg = TextMessageGet("MP_WIN_PLAYER");
					if (msg)
						_snprintf(label, 48, msg->pMessage, g_PlayerInfoList[m_iWinner].name); 
					else
						_snprintf(label, 48, "%s is the winner!\0", g_PlayerInfoList[m_iWinner].name); 

					label[47] = '\0';
					m_TitleLabel.setText(label);
				}
			}
			else// team wins, iuser2 is the best player
			{
				TEAM_ID team = g_PlayerExtraInfo[m_iWinner].teamnumber;// get winner's team ID
				if (IsActiveTeam(team))
				{
					char label[48];
					client_textmessage_t *msg = TextMessageGet("MP_WIN_TEAM");
					if (msg)
						_snprintf(label, 48, msg->pMessage, g_TeamInfo[team].name); 
					else
						_snprintf(label, 48, "Team %s wins!\0", g_TeamInfo[team].name); 

					m_TitleLabel.setText(label);
				}
			}
		}
		else// game continues, display normal title
		{
			if (gViewPort->m_szServerName && GetMapName())
			{
				char szTitleText[MAX_SERVERNAME_LENGTH + MAX_MAPNAME];
				_snprintf(szTitleText, MAX_SERVERNAME_LENGTH + MAX_MAPNAME, "%s - %s\0", gViewPort->m_szServerName, GetMapName());// XDM
				m_TitleLabel.setText(szTitleText);
			}
			else
				m_TitleLabel.setText("%s", BufferedLocaliseTextString("#SCORE"));// XDM3038a: this isn't a normal situation
		}
	}

	m_iRows = 0;
	GetAllPlayersInfo();// XDM3038a: this should make things a lot more stable

	// Clear out sorts
	unsigned short i = 0;
	for (i = 0; i < SBOARD_NUM_ROWS; ++i)
	{
		m_iSortedRows[i] = 0;
		m_iRowType[i] = SBOARD_ROW_BLANK;
	}

	// XDM3037a	for (i = 0; i <= MAX_PLAYERS; ++i)// XDM3037: fixed incl 32
	// XDM3037a		m_bHasBeenSorted[i] = false;

	size_t arrayindex;
	for (arrayindex=0; arrayindex<MAX_PLAYERS; ++arrayindex)
		m_SortedPlayers[arrayindex] = arrayindex+1;

	if (gHUD.m_iGameType == GT_LMS)
		qsort((void *)m_SortedPlayers, MAX_PLAYERS, sizeof(m_SortedPlayers[0]), playerinfocmp_lms);
	else
		qsort((void *)m_SortedPlayers, MAX_PLAYERS, sizeof(m_SortedPlayers[0]), playerinfocmp);

	// If it's not teamplay, sort all the players. Otherwise, sort the teams.
	if (IsTeamGame(gHUD.m_iGameType))
	{
		SortTeams();
		//SortPlayers(TEAM_NONE, true);// XDM3035a
	}
	else
	{
		SortPlayers(TEAM_NONE, false);
		AddRow(TEAM_NONE, SBOARD_ROW_TEAM);// XDM3035c: spectators
		SortPlayers(TEAM_NONE, true);// XDM3035a
	}

	// set scrollbar range
	m_PlayerList.SetScrollRange(m_iRows);

	FillGrid();
	UpdateCounters();

	if (gViewPort->GetSpectatorPanel()->m_menuVisible)
		 m_ButtonClose.setVisible(true);
	else 
		 m_ButtonClose.setVisible(false);

	CheckStatsPanel();// XDM3037
	m_ButtonStatsMode.setVisible(m_bHasStats);// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Update time left label
//-----------------------------------------------------------------------------
void ScorePanel::UpdateCounters(void)
{
	if (isVisible() == false)
		return;

	// WARNING: changes to this code must be also applied to SpectatorPanel::Update() timer section or any other game timer
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
		m_CurrentTimeLabel.setText(szTimeText);
		m_CurrentTimeLabel.setVisible(true);
	}
	else
		m_CurrentTimeLabel.setVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void ScorePanel::SortTeams(void)
{
	short i = 0;
	//for (i = 0; i < MAX_TEAMS; ++i)
	//	m_bHasBeenSortedTeam[i] = false;
	// clear out team scores
	for (i = TEAM_NONE; i <= g_iNumberOfTeams; ++i)// XDM: 0 <=
	{
		g_TeamInfo[i].score = 0;
		// don't touch g_TeamInfo[i].scores_overriden;// XDM: extra score
		g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = 0;
		g_TeamInfo[i].packetloss = 0;
		//m_bHasBeenSortedTeam[i] = false;
	}

	TEAM_ID team_id;
	// recalc the team scores, then draw them
	for (i = 1; i <= MAX_PLAYERS; ++i)
	{
		//m_bHasBeenSorted[i] = false;
		if (!IsValidPlayer(i))// if (g_PlayerInfoList[i].name == NULL)
			continue;// empty player slot, skip

		//TEAM_ID playerteam = g_PlayerExtraInfo[i].teamnumber;
		//ASSERT(IsValidTeam(playerteam));// TEAM_NONE must be OK!
		if (IsActivePlayer(i))// IsActivePlayer filters out spectators
		{
			team_id = g_PlayerExtraInfo[i].teamnumber;
		}
		else if (IsSpectator(i))
		{
			//if (team_id != TEAM_NONE)// XDM3038a: players now keep previous team when bacoming spectators
				team_id = TEAM_NONE;
		}
		else
		{
			conprintf(1, "ScorePanel::SortTeams() Warning: bad player info: %d is not active and not a spectator!\n", i);
			team_id = TEAM_NONE;
		}
		g_TeamInfo[team_id].score += g_PlayerExtraInfo[i].score;
		g_TeamInfo[team_id].deaths += g_PlayerExtraInfo[i].deaths;
		g_TeamInfo[team_id].ping += g_PlayerInfoList[i].ping;
		g_TeamInfo[team_id].packetloss += g_PlayerInfoList[i].packetloss;
	}

	memset(m_SortedTeams, 0, sizeof(m_SortedTeams));

	// find team ping/packetloss averages
	for (i = 0; i <= g_iNumberOfTeams; ++i)// XDM3035c: 0 <=
	{
		//m_bHasBeenSortedTeam[i] = false;
		if (g_TeamInfo[i].players > 0)
		{
			g_TeamInfo[i].ping /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
			g_TeamInfo[i].packetloss /= g_TeamInfo[i].players;
		}
		m_SortedTeams[i] = i;
	}

	// XDM: speedup???
	qsort((void *)m_SortedTeams, MAX_TEAMS+1, sizeof(m_SortedTeams[0]), teaminfocmp);

	if (gHUD.m_iIntermission > 0)// XDM3037a: HACK :)
	{
		if (m_iServerBestTeam != TEAM_NONE && m_iServerBestTeam != m_SortedTeams[0])
		{
			TEAM_ID clientbestteam = m_SortedTeams[0];
			m_SortedTeams[0] = m_iServerBestTeam;
			for (i = 1; i <= g_iNumberOfTeams; ++i)// UNDONE: this just swaps server best team and client best team, which is usually enough. But what we really should do is cut or move (but it's harder to write)
			{
				if (m_SortedTeams[i] == m_iServerBestTeam)// old place of the server best team
				{
					m_SortedTeams[i] = clientbestteam;
					conprintf(1, "CL: WARNING! Fixing best team (%d -> %d) on score panel!\n", clientbestteam, m_iServerBestTeam);
					break;
				}
			}
		}
	}

	// Iterate from the beginning (if some teams are inactive, they're left at the end of list)
	// 'i' is the index in the m_SortedTeams array
	// Spectators is somewhere at the end of list
	for (i = 0; i <= g_iNumberOfTeams; ++i)// XDM3037: g_iNumberOfTeams represents only real teams(4), but we need to include spectators here(5th)
	{
		team_id = m_SortedTeams[i];// use team from the sorted list
		//m_bHasBeenSortedTeam[team_id] = true;// set to TRUE, so this team won't get sorted again
		AddRow(team_id, SBOARD_ROW_TEAM);
		// Now sort all the players on this team
		SortPlayers(/*SBOARD_ROW_PLAYER, */team_id, (team_id == TEAM_NONE));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills rows with some filtering appiled, must be called with proper sequence in mind (e.g. after writing each team header)
//-----------------------------------------------------------------------------
void ScorePanel::SortPlayers(/*const int &iRowType, */const TEAM_ID &teamindex, bool bIncludeSpectators)
{
	//m_iBestPlayer[teamindex] = 0;
	// no need to SORT everytime, just filter
	CLIENT_INDEX ci;
	size_t arrayindex;
	bool bFirst = true;
	for (arrayindex=0; arrayindex<MAX_PLAYERS; ++arrayindex)
	{
		ci = m_SortedPlayers[arrayindex];
		// XDM3037a	m_bHasBeenSorted[ci] = true;

		if (!IsValidPlayerIndex(ci))
			continue;
		if (g_PlayerInfoList[ci].name == NULL)
			continue;
		if (IsSpectator(ci))
		{
			if (!bIncludeSpectators)// allow spectators to be added to any team list
				continue;
		}
		else
		{
			if (g_PlayerExtraInfo[ci].teamnumber != teamindex)
				continue;
			if (bIncludeSpectators)// right now it's not "include", but "only"
				continue;
		}
		AddRow(ci, SBOARD_ROW_PLAYER);

		if (bFirst)
		{
			m_iBestPlayer[teamindex] = ci;
			bFirst = false;
		}
	}
	AddRow(0, SBOARD_ROW_BLANK);
}

//-----------------------------------------------------------------------------
// Purpose: FillGrid
//-----------------------------------------------------------------------------
void ScorePanel::FillGrid(void)
{
	// update highlight position
	int x, y;
	getApp()->getCursorPos(x, y);
	cursorMoved(x, y, this);

	// remove highlight row if we're not in squelch mode
	if (!GetClientVoiceMgr()->IsInSquelchMode())
		m_iHighlightRow = -1;

	size_t row = 0;
	size_t col = 0;

	/* concept only
	if (m_bHasStats)
	{
		int pl1 = m_iWinner;
		int pl2 = gHUD.m_pLocalPlayer->index;
		CLabelHeader *pLabel;
		for (int row=0; row < SBOARD_NUM_ROWS; ++row)// row 0-title, 1-space, 2+ - data
		{
			CGrid *pGridRow = &m_PlayerGrids[row];
			if (row == 0)// title
			{
				pGridRow->SetRowUnderline(0, true, YRES(3), 191,191,191,0);
				if (IsValidPlayerIndex(pl1))
				{
					pLabel = &m_PlayerEntries[1][row];// col 0 - desc, 1-winner, 2-space, 3-you
					pLabel->setImage(NULL);
					pLabel->setFont(m_pFontTitle);
					pLabel->setTextOffset(0,0);
					pLabel->setText("Playername1");
					pLabel->setText2("winner");
					pLabel->setVisible(true);
				}
				if (IsValidPlayerIndex(pl2))
				{
					pLabel = &m_PlayerEntries[3][row];
					pLabel->setImage(NULL);
					pLabel->setFont(m_pFontTitle);
					pLabel->setTextOffset(0,0);
					pLabel->setText("Playername2");
					pLabel->setText2("you");
					pLabel->setVisible(true);
				}
			}
			else if (row == 1)// spacer
			{
				pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);
			}
			else if (row >= (STAT_NUMELEMENTS+2))// unused rows
			{
				for (int col=0; col < SBOARD_NUM_COLUMNS; ++col)
					m_PlayerEntries[col][row].setVisible(false);

				continue;
			}
			else// data lines
			{
				int statindex = row-2;
				byte r,g,b;
				char sz[128];
				pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);
				for (int col=0; col < SBOARD_NUM_COLUMNS; ++col)
				{
					CLabelHeader *pLabel = &m_PlayerEntries[col][row];
					pLabel->setImage(NULL);
					pLabel->setFont(m_pFontScore);
					pLabel->setTextOffset(0, 0);
					if (col == 0)
					{
						r = 191; g = 191; b = 191;
						_snprintf(sz, "Data %d", statindex);// localize stat desc
						pLabel->setText(sz);
						pLabel->setText2("");
					}
					else if (col == 1 && IsValidPlayerIndex(pl1))
					{
						GetPlayerColor(pl1, r,g,b);
						_snprintf(sz, "val1 %d", g_PlayerStats[pl1][statindex]);
						pLabel->setText(sz);
						pLabel->setText2("");
					}
					else if (col == 2)
					{
						r = 0; g = 0; b = 0;
						//_snprintf(sz, "Data %d", statindex);
						pLabel->setText("-spc-");
						pLabel->setText2("");
					}
					else if (col == 3 && IsValidPlayerIndex(pl2))
					{
						GetPlayerColor(pl2, r,g,b);
						_snprintf(sz, "val2 %d", g_PlayerStats[pl2][statindex]);
						pLabel->setText(sz);
						pLabel->setText2("");
					}
					else
					{
						r = 0; g = 0; b = 0;
						pLabel->setVisible(false);
						continue;
					}

					pLabel->setFgColor(r,g,b,0);
					pLabel->setVisible(true);
				}
			}
		}
		return;
	}*/
	//bool bNextRowIsGap = false;

	for (row=0; row < SBOARD_NUM_ROWS; ++row)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];
		pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);

		if (row >= m_iRows)
		{
			for (col=0; col < SBOARD_NUM_COLUMNS; ++col)
				m_PlayerEntries[col][row].setVisible(false);

			continue;
		}

		/*bool bRowIsGap = false;
		if (bNextRowIsGap)
		{
			bNextRowIsGap = false;
			bRowIsGap = true;
		}*/

		char sz[128];
		hud_player_info_t *pl_info;
		team_info_t *team_info;
		byte r,g,b;// XDM
		for (col=0; col < SBOARD_NUM_COLUMNS; ++col)
		{
			pl_info = NULL;
			team_info = NULL;
			CLabelHeader *pLabel = &m_PlayerEntries[col][row];

			pLabel->setVisible(true);
			pLabel->setText2("");
			pLabel->setImage(NULL);
			pLabel->setFont(m_pFontScore);
			pLabel->setTextOffset(0, 0);
			
			int rowheight = 13;
			if (ScreenHeight > 480)
				rowheight = YRES(rowheight);
			else
				rowheight = 15;// more tweaking, make sure icons fit at low res

			pLabel->setSize(pLabel->getWide(), rowheight);
			pLabel->setPaintBackgroundEnabled(true);
			//pLabel->setBgColor(0,0,0, 255);

			// Set color
			if (m_iRowType[row] == SBOARD_ROW_PLAYER)// XDM: fastest check should go first
			{
				if (IsValidPlayerIndex(m_iSortedRows[row]))
				{
					// Get the player's data
					pl_info = &g_PlayerInfoList[m_iSortedRows[row]];
					// special text color for player names
					if (IsSpectator(m_iSortedRows[row]))//XDM: if (pl_info->spectator) useless :(
					{
						if (IsTeamGame(gHUD.m_iGameType))
						{
							GetTeamColor(g_PlayerExtraInfo[m_iSortedRows[row]].teamnumber, r,g,b);
							float h=1,s=1,l=1;// set to 1 to get answer
							RGB2HSL(r,g,b, h,s,l);
							s *= 0.5f;// desaturate a little
							HSL2RGB(h,s,l, r,g,b);
						}
						else
							GetTeamColor(TEAM_NONE, r,g,b);
					}
					else
					{
						if (gHUD.m_pCvarUsePlayerColor->value > 0.0f)
							GetPlayerColor(m_iSortedRows[row], r,g,b);// XDM3035: TESTME!! May be slow!
						else
							GetTeamColor(g_PlayerExtraInfo[m_iSortedRows[row]].teamnumber, r,g,b);
					}
					pLabel->setFgColor(r,g,b,0);
					// Set background color
					if (pl_info->thisplayer) // if it is their name, draw it a different color
					{
						//pLabel->setFgColor(Scheme::sc_white);// Highlight LOCAL player
						pLabel->setFgColor(m_pTextScheme->FgColorClicked.r, m_pTextScheme->FgColorClicked.g, m_pTextScheme->FgColorClicked.b, 255-m_pTextScheme->FgColorClicked.a);
						pLabel->setBgColor(r,g,b, 191);// XDM
					}
					else if (m_iLastKilledBy > 0 && m_iSortedRows[row] == m_iLastKilledBy && ((gHUD.m_iRevengeMode > 0) || m_fLastKillDisplayStopTime > gHUD.m_flTime))// check time here too, so bg color will stay default
					{
						// This function (luckily) doesn't get called often, so don't update fade effect here, just set the color.
						UnpackRGB(r,g,b, RGB_RED);
						pLabel->setBgColor(r,g,b,127);// just set color, but don't do any calculations
					}
					else if (m_iWinner == m_iSortedRows[row] || (g_iUser2 == m_iSortedRows[row] && (g_iUser1 > OBS_NONE/* == OBS_CHASE_LOCKED || g_iUser1 == OBS_CHASE_FREE || g_iUser1 == OBS_IN_EYE)*/)))// XDM: spectator's target // XDM3038: ||, was &&
					{
						UnpackRGB(r,g,b, RGB_CYAN);
						pLabel->setBgColor(r,g,b,191);
						//pLabel->setBgColor(SBOARD_COLOR_SPECTARGET_BG_R,SBOARD_COLOR_SPECTARGET_BG_G,SBOARD_COLOR_SPECTARGET_BG_B,SBOARD_COLOR_SPECTARGET_BG_A);
						//setBgColor(Scheme::sc_secondary2);
					}
					/* useless because user may move mouse between FillFrid() calls				else if (row == m_iHighlightRow)
					{
						pLabel->setBgColor(Scheme::sc_secondary2);
					}*/
					else
					{
						pLabel->setBgColor(0,0,0, 255);
						pLabel->setPaintBackgroundEnabled(false);// don't bother
						// but this makes m_iHighlightRow transparent too. I consider this a harmless side-effect.
					}
				}
			}
			else if (m_iRowType[row] == SBOARD_ROW_TEAM)// || m_iRowType[row] == SBOARD_ROW_SPECTATORS)
			{
				pLabel->setBgColor(0,0,0, 255);
				if (IsValidTeam(m_iSortedRows[row]))
				{
					team_info = &g_TeamInfo[m_iSortedRows[row]];
					// different height for team header rows
					rowheight = 20;
					if (ScreenHeight >= 480)// HACK
						rowheight = YRES(rowheight);
					// team color text for team names
					GetTeamColor(m_iSortedRows[row], r,g,b);// XDM
					pLabel->setFgColor(r,g,b,0);
					pLabel->setSize(pLabel->getWide(), rowheight);
					pLabel->setFont(m_pFontTitle);
					pGridRow->SetRowUnderline(0, true, YRES(3), r,g,b,0);// XDM
				}
				else
					pLabel->setTextSimple("ERROR");
			}
			else if (m_iRowType[row] == SBOARD_ROW_BLANK)
			{
				pLabel->setBgColor(0,0,0, 255);
				pLabel->setTextSimple(" ");
				continue;
			}

			// Align 
			if (col == COLUMN_NAME || col == COLUMN_TSCORE)
				pLabel->setContentAlignment(vgui::Label::a_west);
			else if (col == COLUMN_TRACKER)
				pLabel->setContentAlignment(vgui::Label::a_center);
			else
				pLabel->setContentAlignment(vgui::Label::a_east);

			// Fill out with the correct data
			strcpy(sz, "");

			if (m_iRowType[row] == SBOARD_ROW_PLAYER)
			{
			if (pl_info)// XDM3035a: can be null?
			{
				if (col == COLUMN_NAME)// XDM: some optimization
				{
					byte l = 0;
					if (g_PlayerExtraInfo[m_iSortedRows[row]].ready &&
						(gHUD.m_iGameState == GAME_STATE_WAITING || gHUD.m_iGameState == GAME_STATE_FINISHED || gHUD.m_iIntermission > 0))// XDM3037a
					{
						sz[l] = '+';
						sz[++l] = 0;
					}
					if (g_PlayerExtraInfo[m_iSortedRows[row]].finished > 0)
					{
						sz[l] = '*';
						sz[++l] = 0;
					}
					if (pl_info->name)
						strcat(sz, pl_info->name);
					//_snprintf(sz, 128, "%s  ", pl_info->name);
				}
				//column for teams only	else if (col == COLUMN_TSCORE)
				//{
				//	_snprintf(sz, 128, "");
				//}
				else if (col == COLUMN_KILLS)
				{
					_snprintf(sz, 128, "%d",  g_PlayerExtraInfo[m_iSortedRows[row]].score);
				}
				else if (col == COLUMN_DEATHS)
				{
					_snprintf(sz, 128, "%d",  g_PlayerExtraInfo[m_iSortedRows[row]].deaths);
				}
				else if (col == COLUMN_LATENCY)
				{
					_snprintf(sz, 128, "%hd", g_PlayerInfoList[m_iSortedRows[row]].ping);
				}
				else if (col == COLUMN_VOICE)
				{
					sz[0] = 0;
					// in HLTV mode allow spectator to turn on/off commentator voice
					// HL20130901	if (!pl_info->thisplayer || gEngfuncs.IsSpectateOnly())
						GetClientVoiceMgr()->UpdateSpeakerImage(pLabel, m_iSortedRows[row]);
				}
			}
			}
			else if (m_iRowType[row] == SBOARD_ROW_TEAM)// || m_iRowType[row] == SBOARD_ROW_SPECTATORS)
			{
				if (col == COLUMN_NAME)// XDM: some optimization
				{
					//if (m_iRowType[row] == SBOARD_ROW_SPECTATORS)// XDM3035a: server sends an empty string
					if (m_iSortedRows[row] == TEAM_NONE)
						_snprintf(sz, 128, BufferedLocaliseTextString("#Spectators"));
					else
						_snprintf(sz, 128, GetTeamName(m_iSortedRows[row]));//team_info->teamnumber));

					// Append the number of players
					if (IsTeamGame(gHUD.m_iGameType) && team_info)// were used for spectators && m_iRowType[row] != SBOARD_ROW_PLAYER)// 3033 == SBOARD_ROW_TEAM)
					{
						char sz2[128];
						_snprintf(sz2, 128, "(%hd %s)\0", team_info->players, BufferedLocaliseTextString((team_info->players == 1)?"#Player":"#Player_plural"));
						pLabel->setText2(sz2);
						pLabel->setFont2(m_pFontSmall);
					}
				}
				else
				{
					if (m_iRowType[row] == SBOARD_ROW_TEAM)
					{
						//if (m_iSortedRows[row] != TEAM_NONE)// not for spectators
						if (IsActiveTeam(m_iSortedRows[row]))// not for spectators
						{
						if (col == COLUMN_TSCORE)
							sprintf(sz, "%d",  team_info->scores_overriden);
						else if (col == COLUMN_KILLS)
							sprintf(sz, "%d",  team_info->score);
						else if (col == COLUMN_DEATHS)
							sprintf(sz, "%d",  team_info->deaths);
						else if (col == COLUMN_LATENCY)
							sprintf(sz, "%hd", team_info->ping);
						}
					}
				}
			}
			pLabel->setTextSimple(sz);
		}
	}

	for (row=0; row < SBOARD_NUM_ROWS; ++row)
	{
		//CGrid *pGridRow = &m_PlayerGrids[row];
		m_PlayerGrids[row].AutoSetRowHeights();
		m_PlayerGrids[row].setSize(m_PlayerGrids[row].getWide(), m_PlayerGrids[row].CalcDrawHeight());
		m_PlayerGrids[row].RepositionContents();
	}

	// hack, for the thing to resize
	m_PlayerList.getSize(x, y);
	m_PlayerList.setSize(x, y);
}

//-----------------------------------------------------------------------------
// Purpose: Setup highlights for player names in scoreboard
//-----------------------------------------------------------------------------
void ScorePanel::DeathMsg(short killer, short victim)
{
	if (gHUD.m_pLocalPlayer == NULL)
		return;
	if (victim == gHUD.m_pLocalPlayer->index)
	{
		// if we were the one killed, or the world killed us, set the scoreboard to indicate suicide
		if (IsValidPlayerIndex(killer))
		{
			m_iLastKilledBy = killer;// ? killer : m_iPlayerNum;
			m_fLastKillDisplayStopTime = gHUD.m_flTime + SB_LAST_KILLER_HIGHLIGHT_TIME;	// display who we were killed by for 10 seconds
		}
		else// XDM3038a
		{
			// test it with revenge mode 2			m_iLastKilledBy = 0;
			m_fLastKillDisplayStopTime = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles mouse movement over a cell
// Input  : row - 
//			col - 
//-----------------------------------------------------------------------------
void ScorePanel::MouseOverCell(const int &row, const int &col)
{
	CLabelHeader *label = &m_PlayerEntries[col][row];
	// clear previously highlighted label
	if (m_pCurrentHighlightLabel != label)
	{
		m_pCurrentHighlightLabel = NULL;
		m_iHighlightRow = -1;
	}
	if (!label)
		return;

	if (m_iRowType[row] != SBOARD_ROW_PLAYER)// only highlight player rows
		return;

	if (!hasFocus())// XDM3038c
		return;

	if (m_pPlayersMenu)// XDM3038c
		if (m_pPlayersMenu->IsOpened())
			return;

	if (m_pStatsPanel)// XDM3038c
		if (m_pStatsPanel->isVisible())
			return;

	// don't act on disconnected players or ourselves
	hud_player_info_t *pl_info = &g_PlayerInfoList[m_iSortedRows[row]];
	if (!pl_info)
		return;

	if (!pl_info->name || !pl_info->name[0])
		return;

	if (pl_info->thisplayer && !gEngfuncs.IsSpectateOnly())
		return;

	// setup the new highlight
	m_pCurrentHighlightLabel = label;
	m_iHighlightRow = row;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: show statistics panel
//-----------------------------------------------------------------------------
void ScorePanel::ShowStats(int cl_index)
{
	DBG_PRINTF("ScorePanel::ShowStats(%d)\n", cl_index);
	if (m_bHasStats && cl_index > CLIENT_INDEX_INVALID && cl_index <= MAX_PLAYERS)
	{
		int statindex = 0;
		int offset = 0;
		char statname[64];
		offset += _snprintf(m_StatsWindowTextBuffer/*+offset*/, 1024, "\n%s: %s\n\n", BufferedLocaliseTextString("#Stats_Title"), g_PlayerInfoList[cl_index].name);
		while (statindex < STAT_NUMELEMENTS)
		{
			_snprintf(statname, 64, "#StatParamName%d", statindex);
			statname[63] = '\0';
			offset += _snprintf(m_StatsWindowTextBuffer+offset, 1024-offset, "%d\t- %s", g_PlayerStats[cl_index][statindex], BufferedLocaliseTextString(statname));
			if (statindex == STAT_CURRENT_SCORE_AWARD || statindex == STAT_BEST_SCORE_AWARD)// XDM3038a: add a title
			{
				_snprintf(statname, 64, "#AWARD%d", g_PlayerStats[cl_index][statindex]);
				statname[63] = '\0';
				offset += _snprintf(m_StatsWindowTextBuffer+offset, 1024-offset, ": %s\n", BufferedLocaliseTextString(statname));
			}
			else if (statindex == STAT_CURRENT_COMBO || statindex == STAT_BEST_COMBO)
			{
				_snprintf(statname, 64, "#COMBO_LOCAL%d", g_PlayerStats[cl_index][statindex]);
				statname[63] = '\0';
				offset += _snprintf(m_StatsWindowTextBuffer+offset, 1024-offset, ": %s\n", BufferedLocaliseTextString(statname));
			}
			else
				offset += _snprintf(m_StatsWindowTextBuffer+offset, 1024-offset, "\n");

			++statindex;
		}
		CheckStatsPanel();
		// TODO: child panel should send "I am closing" signal to its parent...
		if (m_pStatsPanel == NULL)// WARNING! TESTME! Potential memory leak?? How does engine deallocate it??
		{
			//_snprintf(statname, 64, "%s: %s", BufferedLocaliseTextString("#Stats_Title"), g_PlayerInfoList[cl_index].name);
			m_pStatsPanel = new CMessageWindowPanel(m_StatsWindowTextBuffer, BufferedLocaliseTextString("#Stats_Title"), false, TRUE, 0, 0, getWide(), getTall());
			m_pStatsPanel->setParent(this);// this is mandatory
			m_pStatsPanel->setBgColor(0,0,0,31);// make this more visible
			m_pStatsPanel->Open();
		}// don't use m_pStatsPanel anywhere! It may become invalid before Update() gets it.
	}
}

//-----------------------------------------------------------------------------
// Purpose: dump debug information to console. Use to verify data.
//-----------------------------------------------------------------------------
void ScorePanel::DumpInfo(void)
{
//#if defined (_DEBUG)// smaller, faster
	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 1)
	{
		int i = 0;
		//conprintf(1, " -- m_iSortedRows\tm_iRowType --\n");
		//for (i = 0; i < SBOARD_NUM_ROWS; i++)
		//	conprintf(1, " %d  %d\t%d\n", i, m_iSortedRows[i], m_iRowType[i]);

		conprintf(1, " CL:DumpInfo()\nidx\tteam\tfrg\tdth\tname\tlst\n");
		for (i = 1; i <= gEngfuncs.GetMaxClients(); ++i)
		{
			conprintf(1, "%d\t%d\t%d\t%d\t(%s)\t%g\n", i,
				g_PlayerExtraInfo[i].teamnumber,
				g_PlayerExtraInfo[i].score,
				g_PlayerExtraInfo[i].deaths,
				g_PlayerInfoList[i].name,
				g_PlayerExtraInfo[i].lastscoretime);
		}
		CLIENT_INDEX bestplayer = GetBestPlayer();
		if (bestplayer > 0)
			conprintf(1, " Best Player: %d (%s)\n", bestplayer, g_PlayerInfoList[bestplayer].name);
		else
			conprintf(1, " Best Player: none\n");

		if (IsTeamGame(gHUD.m_iGameType))
		{
			conprintf(1, " CL:teams %d\nid\tfrg\tdth\tplr\t\tsc\tnam--\n", g_iNumberOfTeams);
			for (i = 0; i <= g_iNumberOfTeams; ++i)// XDM: 0 <
			{
				conprintf(1, "%d  %d %d %hd %d (%s)\n", i,
				g_TeamInfo[i].score,
				g_TeamInfo[i].deaths,
				//g_TeamInfo[i].ping,
				//g_TeamInfo[i].packetloss,
				g_TeamInfo[i].players,
				g_TeamInfo[i].scores_overriden,
				g_TeamInfo[i].name);
			}
			TEAM_ID bestteam = GetBestTeam();
			conprintf(1, " Best Team: %d (%s)\n", bestteam, g_TeamInfo[bestteam].name);
		}
	conprintf(1, "-------- ScorePanel::DumpInfo end --------\n");
	}
//#endif
}

//-----------------------------------------------------------------------------
// Purpose: A clone of CGameRulesMultiplay::GetBestPlayer
// Output : int player index
//-----------------------------------------------------------------------------
CLIENT_INDEX ScorePanel::GetBestPlayer(void)
{
#if defined (_DEBUG)
	/*if (m_iServerBestPlayer != 0)
		if (!ASSERT(m_SortedPlayers[0] == m_iServerBestPlayer))
		{
			short BestPlayers[2];
			BestPlayers[0] = m_SortedPlayers[0];
			BestPlayers[1] = m_iServerBestPlayer;
			qsort((void *)BestPlayers, 2, sizeof(BestPlayers[0]), playerinfocmp);
			conprintf(1, "ScorePanel::GetBestPlayer(): TEST: %d, %d\n", BestPlayers[0], BestPlayers[1]);
		}*/
	conprintf(1, "ScorePanel::GetBestPlayer(): client: %d, server: %d\n", m_SortedPlayers[0], m_iServerBestPlayer);
#endif
	return m_iServerBestPlayer;// XDM3037: m_SortedPlayers[0] is unreliable
}

//-----------------------------------------------------------------------------
// Purpose: GetBestTeam
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID ScorePanel::GetBestTeam(void)
{
	if (!IsTeamGame(gHUD.m_iGameType))// XDM3037a
		return TEAM_NONE;

#if defined (_DEBUG)
	if (m_iServerBestTeam != TEAM_NONE)
		ASSERT(m_SortedTeams[0] == m_iServerBestTeam);

	conprintf(1, "ScorePanel::GetBestTeam(): cl: %d, sv: %d\n", m_SortedTeams[0], m_iServerBestTeam);
#endif
	return m_iServerBestTeam;// XDM3037: m_SortedTeams[0] is unreliable
}

//-----------------------------------------------------------------------------
// Purpose: Auto
// Input  : iRowData - 
//			iRowType - 
//-----------------------------------------------------------------------------
void ScorePanel::AddRow(int iRowData, int iRowType)
{
	m_iSortedRows[m_iRows] = iRowData;
	m_iRowType[m_iRows] = iRowType;
	++m_iRows;
}

//-----------------------------------------------------------------------------
// Purpose: server message has arrived
//-----------------------------------------------------------------------------
void ScorePanel::RecievePlayerStats(int cl_index)
{
	DBG_PRINTF("ScorePanel::RecievePlayerStats(%d)\n", cl_index);
	m_bHasStats = true;
	m_ButtonStatsMode.setVisible(true);

	if (m_flWaitForStatsTime != 0.0f)
	{
		if (m_flWaitForStatsTime >= gEngfuncs.GetClientTime())// made in time
		{
			Open();// UNDONE: does not open right now
			ShowStats(gHUD.m_pLocalPlayer->index);
		}
		else
			conprintf(2, "RecievePlayerStats(%d): arrived after timeout.\n", cl_index);

		m_flWaitForStatsTime = 0.0f;// don't wait
	}

	//int pl1 = m_iWinner;
	/*int pl2 = gHUD.m_pLocalPlayer?gHUD.m_pLocalPlayer->index:0;
	for (int statindex=0; statindex < STAT_NUMELEMENTS; ++statindex)
	{
		_snprintf(sz, 128, "val: %d\n", g_PlayerStats[pl2][statindex]);
	}*/
	//if (isVisible())
	//	FillGrid();
}

//-----------------------------------------------------------------------------
// Purpose: Check if the stats panel is among our child panels
//-----------------------------------------------------------------------------
void ScorePanel::CheckStatsPanel(void)
{
	if (m_pStatsPanel)// XDM3037: sort of a hack to keep in touch with stats panel, since it can't notify us when it's closing
	{
		int i;
		// XDM3038: faster		bool bFound = false;
		for (i=0; i<getChildCount(); ++i)
		{
			if (getChild(i) == m_pStatsPanel)//pPanel->isVisible())
			{
				return;// XDM3038 bFound = true;
				break;
			}
		}
		// XDM3038	if (bFound == false)
		{
			// viewport should've done it:	delete m_pStatsPanel;
			m_pStatsPanel = NULL;// looks like the stats panel was closed
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: catch button signals
// Input  : signal - 
//			pCaller - can be NULL!
//-----------------------------------------------------------------------------
void ScorePanel::OnActionSignal(int signal, CMenuPanelActionSignalHandler *pCaller)
{
	if (signal == SCORE_PANEL_CMD_PLAYERSTAT)// show stats
	{
		//conprintf(0, "Statistics are not implemented yet. Please view the file manually.\n");
		//gViewPort->ShowMenu(new CStatsPanel(false, true, -1,-1, XRES(STATSPANEL_WINDOW_SIZE_X), YRES(STATSPANEL_WINDOW_SIZE_Y)));
		
		//if (cl_test1->value > 0)
		//	m_pPlayersMenu->setPos(m_ButtonStatsMode.GetX() - CMENU_SIZE_X/2, m_ButtonStatsMode.GetY());
		if (m_bHasStats)
			gViewPort->ShowCommandMenu(m_pPlayersMenu, false);
		else
			PlaySound("vgui/button_error.wav", VOL_NORM);

		// if (gHUD.m_pLocalPlayer) ShowStats(gHUD.m_pLocalPlayer->index);
	}
	else if (signal > SCORE_PANEL_CMD_PLAYERSTAT)
	{
		if (m_bHasStats)
			ShowStats(signal-SCORE_PANEL_CMD_PLAYERSTAT);
		else
			PlaySound("vgui/button_error.wav", VOL_NORM);
	}
	//else if (signal == IDIGNORE)// hack? closed stats
	//	m_pStatsPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a
// Input  : &down - 
//			&keynum - always lowercase :(
//			*pszCurrentBinding - 
// Output : int - 1 pass the key to the next window, 0 - handled, stop
//-----------------------------------------------------------------------------
int ScorePanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down)
	{
		if (gHUD.m_iIntermission > 0)
		{
			if (keynum == K_ENTER || keynum == K_KP_ENTER || keynum == K_SPACE || keynum == K_ESCAPE)
			{
				CheckStatsPanel();
				if (m_pStatsPanel && m_pStatsPanel->isVisible())
					return m_pStatsPanel->KeyInput(down, keynum, pszCurrentBinding);
				else
					setShowCursor(!IsShowingCursor());

				return 0;
			}
			else if (keynum == 's')
			{
				//if (gViewPort->isKeyDown(KEY_LSHIFT) || gViewPort->isKeyDown(KEY_RSHIFT))// show stats menu
					OnActionSignal(SCORE_PANEL_CMD_PLAYERSTAT, NULL);
				//else if (gHUD.m_pLocalPlayer)
				//	ShowStats(gHUD.m_pLocalPlayer->index);// show my stats instantly

				return 0;
			}
			/*else if (keynum == 'w')
			{
				OnActionSignal(SCORE_PANEL_CMD_PLAYERSTAT, NULL);
				return 0;
			}*/
		}
	}
	return ScorePanel::BaseClass::KeyInput(down, keynum, pszCurrentBinding);
}








//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highlight status
//-----------------------------------------------------------------------------
void CLabelHeader::paintBackground(void)
{
	if (_row < 0)// XDM3035c
		return;

	ScorePanel *pScorePanel = gViewPort->GetScoreBoard();// = (ScorePanel *)(getParent()->getParent());// FAIL
	if (pScorePanel)
	{
		vgui::Color oldBg;
		getBgColor(oldBg);// save old color in case cell gets un-highlighted

		if (pScorePanel->m_iRowType[_row] == SBOARD_ROW_PLAYER)
		{
			if (pScorePanel->m_iHighlightRow == _row)// XDM3035c: background drawing is disabled anyway
			{
				if (pScorePanel->m_pTextScheme)
					setBgColor(pScorePanel->m_pTextScheme->BgColorArmed.r, pScorePanel->m_pTextScheme->BgColorArmed.g, pScorePanel->m_pTextScheme->BgColorArmed.b, 255-pScorePanel->m_pTextScheme->BgColorArmed.a);
				//setBgColor(Scheme::sc_secondary2);// XDM: mouse over
			}
			else if (pScorePanel->m_iLastKilledBy > 0 && pScorePanel->m_iSortedRows[_row] == pScorePanel->m_iLastKilledBy)
			{
				//byte r,g,b;
				//UnpackRGB(r,g,b, RGB_RED);
				// FillGrid() sets color, here we just paint
				if (gHUD.m_iRevengeMode > 0)
				{
					//setBgColor(r,g,b,191);
					setBgColor(oldBg[0],oldBg[1],oldBg[2], 191);// BUGBUG: oldBg relies on pLabel->setBgColor() in FillGrid, which sometimes fails
				}
				else if (pScorePanel->m_fLastKillDisplayStopTime > gHUD.m_flTime)// fade out
				{
					float k = 1.0f - (pScorePanel->m_fLastKillDisplayStopTime - gHUD.m_flTime)/SB_LAST_KILLER_HIGHLIGHT_TIME;
					//setBgColor(r,g,b, max(0,(int)(k*255.0f)));
					setBgColor(oldBg[0],oldBg[1],oldBg[2], max(0,(int)(k*255.0f)));// redraw last killer's name background every time nescessary
				}
				//else
			}
		}
		Panel::paintBackground();
		setBgColor(oldBg);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highlight status
//-----------------------------------------------------------------------------
void CLabelHeader::paint(void)
{
	vgui::Color oldFg;
	getFgColor(oldFg);

	//ASSERT(((ScorePanel *)((CGrid *)getParent())->getParent()) == gViewPort->GetScoreBoard());
	//if (((ScorePanel *)((CGrid *)getParent())->getParent())->m_iHighlightRow == _row)// XDM: FAIL. I hate hacks!
	if (gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
		setFgColor(255, 255, 255, 0);

	// draw text
	int x, y, iwide, itall;
	getTextSize(iwide, itall);
	calcAlignment(iwide, itall, x, y);
	_dualImage->setPos(x, y);

	int x1, y1;
	_dualImage->GetImage(1)->getPos(x1, y1);
	_dualImage->GetImage(1)->setPos(_gap, y1);
	_dualImage->doPaint(this);

	if (_image)// get size of the panel and the image
	{
		vgui::Color imgColor;
		getFgColor(imgColor);
		if (_useFgColorAsImageColor)
			_image->setColor(imgColor);

		_image->getSize(iwide, itall);
		calcAlignment(iwide, itall, x, y);
		_image->setPos(x, y);
		_image->doPaint(this);
	}

	setFgColor(oldFg[0], oldFg[1], oldFg[2], oldFg[3]);
}

//-----------------------------------------------------------------------------
// Purpose: calcAlignment
//-----------------------------------------------------------------------------
void CLabelHeader::calcAlignment(const int &iwide, const int &itall, int &x, int &y)
{
	// calculate alignment ourselves, since vgui is so broken
	int wide, tall;
	getSize(wide, tall);
// XDM3037a	x = 0, y = 0;
	// align left/right
	switch (_contentAlignment)
	{
		default:// XDM3037a
		// left
		case Label::a_northwest:
		case Label::a_west:
		case Label::a_southwest:
		{
			x = 0;
			break;
		}
		// center
		case Label::a_north:
		case Label::a_center:
		case Label::a_south:
		{
			x = (wide - iwide) / 2;
			break;
		}
		// right
		case Label::a_northeast:
		case Label::a_east:
		case Label::a_southeast:
		{
			x = wide - iwide;
			break;
		}
	}
	// top/down
	switch (_contentAlignment)
	{
		default:// XDM3037a
		// top
		case Label::a_northwest:
		case Label::a_north:
		case Label::a_northeast:
		{
			y = 0;
			break;
		}
		// center
		case Label::a_west:
		case Label::a_center:
		case Label::a_east:
		{
			y = (tall - itall) / 2;
			break;
		}
		// south
		case Label::a_southwest:
		case Label::a_south:
		case Label::a_southeast:
		{
			y = tall - itall;
			break;
		}
	}
	// don't clip to Y
	//if (y < 0)
	//	y = 0;

	if (x < 0)
		x = 0;

	x += _offset[0];
	y += _offset[1];
}
