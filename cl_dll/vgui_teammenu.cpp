#include "hud.h"
#include "cl_util.h"
#include "vgui_Int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include "vgui_Viewport.h"
#include "vgui_TeamMenu.h"
#include "keydefs.h"

//-----------------------------------------------------------------------------
// Purpose: Creation
//-----------------------------------------------------------------------------
CTeamMenuPanel::CTeamMenuPanel(int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	// Schemes
	CScheme *pTitleScheme = pSchemes->getScheme("Title Font");
	CScheme *pTextScheme = pSchemes->getScheme("Briefing Text");
	CScheme *pTeamInfoText = pSchemes->getScheme("Team Info Text");
	CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");

	setBgColor(0,0,0,PANEL_DEFAULT_ALPHA);
	if (IsPersistent())// XDM3038
		setBackgroundMode(BG_FILL_SCREEN);
	else
		setBackgroundMode(BG_FILL_NORMAL);

	setCaptureInput(true);
	setShowCursor(true);

	// Create the title
	//m_pLabelTitle = new Label(BufferedLocaliseTextString("#Title_TeamMenu"), TEAMMENU_TITLE_X, TEAMMENU_TITLE_Y, wide-(x+TEAMMENU_TITLE_X), pTitleScheme->font->getTall()+YRES(1));
	m_LabelTitle.setParent(this);
	m_LabelTitle.setBounds(TEAMMENU_TITLE_X, TEAMMENU_TITLE_Y, wide-(x+TEAMMENU_TITLE_X), pTitleScheme->font->getTall()+YRES(1));
	m_LabelTitle.setFont(pTitleScheme->font);
	m_LabelTitle.setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_LabelTitle.setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_LabelTitle.setContentAlignment(vgui::Label::a_west);
	m_LabelTitle.setText("%s\0", BufferedLocaliseTextString("#Title_TeamMenu"));

	// Create the Info Window
	//m_pTeamWindow = new CTransparentPanel(BG_FILL_NORMAL, 255, TEAMMENU_WINDOW_X, TEAMMENU_WINDOW_Y, TEAMMENU_WINDOW_SIZE_X, TEAMMENU_WINDOW_SIZE_Y);
	//m_pTeamWindow = new Panel(TEAMMENU_WINDOW_X, TEAMMENU_WINDOW_Y, TEAMMENU_WINDOW_SIZE_X, TEAMMENU_WINDOW_SIZE_Y);
	m_TeamWindow.setParent(this);
	m_TeamWindow.setBounds(TEAMMENU_WINDOW_X, TEAMMENU_WINDOW_Y, TEAMMENU_WINDOW_SIZE_X, TEAMMENU_WINDOW_SIZE_Y);
	m_TeamWindow.setBgColor(0,0,0,63);
	m_TeamWindow.setBorder(new CDefaultLineBorder());

	// Create the Map Name Label
	//m_pMapTitle = new Label("", TEAMMENU_WINDOW_TITLE_X, TEAMMENU_WINDOW_TITLE_Y);
	m_MapTitle.setParent(this);
	m_MapTitle.setPos(TEAMMENU_WINDOW_TITLE_X, TEAMMENU_WINDOW_TITLE_Y);
	m_MapTitle.setFont(pTitleScheme->font); 
	m_MapTitle.setParent(&m_TeamWindow);
	m_MapTitle.setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_MapTitle.setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_MapTitle.setContentAlignment(vgui::Label::a_west);

	// Create the Scroll panel
	//m_pScrollPanel = new CCustomScrollPanel(TEAMMENU_WINDOW_TEXT_X, TEAMMENU_WINDOW_TEXT_Y, TEAMMENU_WINDOW_SIZE_X - (TEAMMENU_WINDOW_TEXT_X * 2), TEAMMENU_WINDOW_TEXT_SIZE_Y);
	m_ScrollPanel.setParent(&m_TeamWindow);
	m_ScrollPanel.setBounds(TEAMMENU_WINDOW_TEXT_X, TEAMMENU_WINDOW_TEXT_Y, TEAMMENU_WINDOW_SIZE_X - (TEAMMENU_WINDOW_TEXT_X * 2), TEAMMENU_WINDOW_TEXT_SIZE_Y);
	m_ScrollPanel.setScrollBarAutoVisible(false, false);
	//m_ScrollPanel.setScrollBarVisible(false, false);
	m_ScrollPanel.setScrollBarVisible(true, true);
	m_ScrollPanel.setBorder(new CDefaultLineBorder());
	m_ScrollPanel.setPaintBorderEnabled(true);
	addInputSignal(new CMenuHandler_ScrollInput(&m_ScrollPanel));// XDM3038

	// Create the Map Briefing panel
	//m_pBriefing = new TextPanel(BufferedLocaliseTextString("#Map_DescNA"), 0,0, TEAMMENU_WINDOW_SIZE_X - TEAMMENU_WINDOW_TEXT_X, TEAMMENU_WINDOW_TEXT_SIZE_Y);
	m_Briefing.setText(BufferedLocaliseTextString("#Map_DescNA"));
	m_Briefing.setParent(m_ScrollPanel.getClient());
	m_Briefing.setBounds(0,0, TEAMMENU_WINDOW_SIZE_X - TEAMMENU_WINDOW_TEXT_X, TEAMMENU_WINDOW_TEXT_SIZE_Y);
	m_Briefing.setFont(pTextScheme->font);
	m_Briefing.setFgColor(pTextScheme->FgColor.r, pTextScheme->FgColor.g, pTextScheme->FgColor.b, 255-pTextScheme->FgColor.a);
	m_Briefing.setBgColor(pTextScheme->BgColor.r, pTextScheme->BgColor.g, pTextScheme->BgColor.b, 255-pTextScheme->BgColor.a);

	// Team Menu buttons
	char sz[16];
	int i;
	int iYPos = TEAMMENU_TOPLEFT_BUTTON_Y;
	byte r = 255, g = 255, b = 255;
	for (i = 0; i < NUM_TEAM_BUTTONS; ++i)
	{
		//GetTeamColor(i, r,g,b);// XDM: right now game rules are not set!
		// Team button
		//m_pButtons[i] = new CommandButton("null", TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true/*, false, "resource/icon_xhl16.tga" looks bad*/);
		m_pButtons[i] = new CommandButton("null", TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, false, false, true, NULL, pButtonScheme);
		m_pButtons[i]->setParent(this);
		m_pButtons[i]->setContentAlignment(vgui::Label::a_west);
		m_pButtons[i]->setBoundKey('0'+(char)i);

		// AutoAssign button uses special case
		if (i == TBUTTON_AUTO)
		{
			m_pButtons[i]->setMainText(BufferedLocaliseTextString("#Team_Auto"));
			//m_pButtons[i]->setVisible(true);
		}
		//else
			m_pButtons[i]->setVisible(false);

		_snprintf(sz, 16, "jointeam \"%d\"\n", i);// !!! XDM3034 UPDATE FIX TODO: use PlayerInfo_SetValueForKey("team", teamname); but it requires writing a new ActionSignal handler
		// Create the Signals
		m_pButtons[i]->addActionSignal(new CMenuHandler_StringCommand(NULL, sz, true));// XDM: was CMenuHandler_StringCommandWatch
		m_pButtons[i]->addInputSignal(new CHandler_MenuButtonOver(this, i));

		// Create the Team Info panel
		m_pTeamInfoPanel[i] = new TextPanel("", TEAMMENU_WINDOW_INFO_X, TEAMMENU_WINDOW_INFO_Y, TEAMMENU_WINDOW_SIZE_X - TEAMMENU_WINDOW_INFO_X, TEAMMENU_WINDOW_SIZE_X - TEAMMENU_WINDOW_INFO_Y);
		m_pTeamInfoPanel[i]->setParent(&m_TeamWindow);
		m_pTeamInfoPanel[i]->setFont(pTeamInfoText->font);
		m_pTeamInfoPanel[i]->setFgColor(r,g,b, 0);
		m_pTeamInfoPanel[i]->setBgColor(0,0,0, 255);
		m_pTeamInfoPanel[i]->setPaintBackgroundEnabled(false);

		iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;
	}

	GetTeamColor(TEAM_NONE, r,g,b);// XDM
	// Create the Spectate button
	m_pSpectateButton = new CommandButton(BufferedLocaliseTextString("#Menu_Spectate"), TEAMMENU_TOPLEFT_BUTTON_X, 0, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, false, false, true, NULL, pButtonScheme);
	m_pSpectateButton->setParent(this);
	//m_pSpectateButton->addInputSignal(new CHandler_MenuButtonOver(this, TBUTTON_SPECTATE));
	m_pSpectateButton->setFont(pButtonScheme->font);
	m_pSpectateButton->setFgColor(r,g,b, 0);
	m_pSpectateButton->addActionSignal(new CMenuHandler_StringCommand(NULL, "spectate", true));
	m_pSpectateButton->setBoundKey('0'+i);// XDM3037a: i should be NUM_TEAM_BUTTONS +NUM_TEAM_BUTTONS+1);// magic number?

	// Create the Cancel button
	m_pCancelButton = new CommandButton(BufferedLocaliseTextString("#Menu_Cancel"), TEAMMENU_TOPLEFT_BUTTON_X, 0, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, false, false, true, NULL, pButtonScheme);
	m_pCancelButton->setParent(this);
	//m_pCancelButton->addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));
	m_pCancelButton->addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDCLOSE));
	//m_pCancelButton->setBoundKey(' ');

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Initialize(void)
{
	m_bUpdatedMapName = false;
	m_ScrollPanel.setScrollValue(0, 0);
	Reset();// XDM3037a
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the Team Menu is displayed
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Update(void)
{
	int	iYPos = TEAMMENU_TOPLEFT_BUTTON_Y;
	// Set the team buttons
	if (IsTeamGame(gHUD.m_iGameType))
	{
		for (short i = 0; i < NUM_TEAM_BUTTONS; ++i)// XDM: start from TBUTTON_AUTO
		{
			if (m_pButtons[i] == NULL)
				continue;

			if (i == TBUTTON_AUTO || IsActiveTeam(i))// XDM3037a: i represents real playable team
			{
				m_pButtons[i]->setVisible(true);
				m_pButtons[i]->setPos(TEAMMENU_TOPLEFT_BUTTON_X, iYPos);
				iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;

				if (IsActiveTeam(i))// XDM3037a: i represents real playable team
					m_pButtons[i]->setMainText(GetTeamName(i));
				else
					continue;// must be auto-button

				// Start with the first option up
				if (m_iCurrentInfo == 0)
					continue;// XDM3035c

				if (m_iCurrentInfo != i)
					m_pButtons[i]->setSelected(false);// a little trick to unstuck bugged buttons
			}
			else// if (i != TBUTTON_AUTO)// XDM3038: hide only team buttons, not the AUTO button
				m_pButtons[i]->setVisible(false);// Hide the button (may be visible from previous maps)
		}
	}// IsTeamGame(gHUD.m_iGameType)
	// Move the AutoAssign button into place
// XDM3037a: bad	m_pButtons[TBUTTON_AUTO]->setPos(TEAMMENU_TOPLEFT_BUTTON_X, iYPos);
//	iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;

	// Spectate button
	if (!gViewPort->GetAllowSpectators() || gHUD.IsSpectator())// XDM3037
	{
		//m_pSpectateButton->DON'T ADD! addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));
		m_pSpectateButton->setVisible(false);
	}
	else
	{
		m_pSpectateButton->setPos(TEAMMENU_TOPLEFT_BUTTON_X, iYPos);
		m_pSpectateButton->setVisible(true);
		iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;
	}

	// Set the Map Title
	if (!m_bUpdatedMapName)
	{
		const char *level = GetMapName();
		if (level && level[0])
		{
			m_MapTitle.setText("%s\0", level);
			m_bUpdatedMapName = true;
		}
	}
	m_ScrollPanel.validate();

	// If the player is already in a team, make the cancel button visible
	//if (!IsTeamGame(gHUD.m_iGameType) || (gHUD.m_iTeamNumber > TEAM_NONE) || gHUD.IsSpectator())
	if (!IsPersistent())// XDM3038
	{
		m_pCancelButton->setPos(TEAMMENU_TOPLEFT_BUTTON_X, iYPos);
		iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;
		//if (!m_pCancelButton->isVisible())
		//{
			m_pCancelButton->setVisible(true);
			//Close();
		//}
	}
	else
		m_pCancelButton->setVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 1 pass the key to the next window, 0 - handled, stop
//-----------------------------------------------------------------------------
int CTeamMenuPanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down && keynum != K_TAB)// XDM3038a
	{
		// Spectate
		if (keynum == m_pSpectateButton->getBoundKey())//(iSlot == NUM_TEAM_BUTTONS+1)
		{
			if (m_pSpectateButton->isVisible())// XDM3037
			{
				m_pSpectateButton->fireActionSignal();
				return 0;
			}
		}
		else
		{
			int iSlot = keynum - '0';// Extract number from character. More simple than atoi()
			if (iSlot >= TBUTTON_AUTO && iSlot <= g_iNumberOfTeams)// Otherwise, see if a particular team is selectable
			{
				if (m_pButtons[iSlot])// we could also cycle through getBoundKey(), but that's slow
				{
					if (m_pButtons[iSlot]->isVisible())
					{
						//conprintf(1, "iSlot %d fireActionSignal\n", iSlot);
						m_pButtons[iSlot]->fireActionSignal();
						return 0;
					}
				}
			}
		}
	}
	return CTeamMenuPanel::BaseClass::KeyInput(down, keynum, pszCurrentBinding);
}

//-----------------------------------------------------------------------------
// Purpose: Update the Team menu before opening it
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Open(void)
{
	Reset();// XDM3038
	byte r,g,b,i;// XDM
	// Set the team buttons
	for (i = 1; i < NUM_TEAM_BUTTONS; ++i)// XDM: start from TBUTTON_AUTO
	{
		GetTeamColor(i, r,g,b);// XDM
		if (m_pButtons[i])
		{
			m_pButtons[i]->m_ColorNormal.Set(r,g,b,127);
			m_pButtons[i]->m_ColorArmed.Set(r,g,b,255);// XDM3038: ::Color uses normal alpha
			m_pButtons[i]->m_ColorBorderNormal.Set(r,g,b,127);// XDM3037a
			m_pButtons[i]->m_ColorBorderArmed.Set(r,g,b,255);// XDM3037a Y?
			m_pButtons[i]->m_ColorBgNormal.Set(r,g,b,63);// XDM3038
			m_pButtons[i]->setSelected(false);
			m_pButtons[i]->setArmed(false);// XDM3038
		}
		if (m_pTeamInfoPanel[i])
		{
			m_pTeamInfoPanel[i]->setVisible(false);// XDM3038
			m_pTeamInfoPanel[i]->setFgColor(min(255,r+32),min(255,g+32),min(255,b+32), 0);// VGUI reversed alpha
			m_pTeamInfoPanel[i]->setBgColor(0,0,0, 255);
		}
	}
	m_bUpdatedMapName = false;// force
	CTeamMenuPanel::BaseClass::Open();
	Update();

	// XDM3038a: this was updated too often, moved from Update() because CViewport::UpdateOnPlayerInfo() calls it too early
	m_Briefing.setText(gViewPort->GenerateFullIntroText(true, true, true));// XDM3038a m_Briefing.setText(gViewPort->GetMOTD());
	// Copied from MessageWindow
	int iScrollSizeX, iScrollSizeY;
	m_Briefing.getTextImage()->setSize(m_ScrollPanel.getClientClip()->getWide(), m_ScrollPanel.getClientClip()->getTall());
	m_Briefing.getTextImage()->getTextSize(iScrollSizeX, iScrollSizeY);// no wrapping plz
	// Now resize the textpanel to fit the scrolled size
	m_Briefing.setSize(iScrollSizeX , iScrollSizeY);
	m_ScrollPanel.setScrollBarAutoVisible(true, true);
	m_ScrollPanel.setScrollBarVisible(false, false);

	//setCaptureInput(true);
	//setShowCursor(true);
}

//-----------------------------------------------------------------------------
// Purpose: paintBackground
//-----------------------------------------------------------------------------
void CTeamMenuPanel::paintBackground(void)
{
	// make sure we get the map briefing up
	if (!m_bUpdatedMapName)
		Update();

	CTeamMenuPanel::BaseClass::paintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: Mouse is over a team button, bring up the class info
//-----------------------------------------------------------------------------
void CTeamMenuPanel::SetActiveInfo(int iInput)
{
	// Remove all the Info panels and bring up the specified one
	m_pSpectateButton->setArmed(false);
	for (int i = 0; i < NUM_TEAM_BUTTONS; ++i)
	{
		if (i != iInput)
		{
			m_pButtons[i]->setArmed(false);
			m_pTeamInfoPanel[i]->setVisible(false);
		}
	}

	m_iCurrentInfo = iInput;

	if (m_iCurrentInfo == TBUTTON_SPECTATE)// Spectate
	{
		m_pSpectateButton->setArmed(true);
	}
	else
	{
		m_pButtons[m_iCurrentInfo]->setArmed(true);

		// XDM3038a
		if (IsActiveTeam(m_iCurrentInfo))
		{
			const size_t iPlayerListLen = (MAX_PLAYER_NAME_LENGTH + 4) * MAX_PLAYERS;
			char szPlayerList[iPlayerListLen];  // name + ", "
			strcpy(szPlayerList, "\n");
			// Now count the number of teammembers of this class
			uint32 iTotal = 0;
			uint32 iWrapCounter = 0;
			for (CLIENT_INDEX j = 1; j <= MAX_PLAYERS; ++j)
			{
				if (g_PlayerInfoList[j].name == NULL)
					continue; // empty player slot, skip
				if (g_PlayerExtraInfo[j].teamnumber != m_iCurrentInfo)
					continue; // skip over players in other teams

				iTotal++;
				iWrapCounter++;
				if (iTotal > 1)
					strncat(szPlayerList, ", ", iPlayerListLen - strlen(szPlayerList));
				if (iWrapCounter >= MAX_PLAYERS/4)// better than nothing
				{
					strncat(szPlayerList, "\n", iPlayerListLen);
					iWrapCounter = 0;
				}
				strncat(szPlayerList, g_PlayerInfoList[j].name, iPlayerListLen - strlen(szPlayerList));
				szPlayerList[iPlayerListLen - 1] = '\0';
			}

			if (iTotal > 0)
			{
				// Set the text of the info Panel
				const size_t iTextLen = MAX_TEAMNAME_LENGTH + iPlayerListLen + 256;
				char szText[iTextLen]; 
				_snprintf(szText, iTextLen, "%s: %u %s (%d points)", GetTeamName(m_iCurrentInfo), iTotal, BufferedLocaliseTextString(iTotal == 1?"#Player":"#Player_plural"), g_TeamInfo[m_iCurrentInfo].score);
				strncat(szText, szPlayerList, iTextLen - strlen(szText));
				szText[iTextLen - 1] = '\0';
				m_pTeamInfoPanel[m_iCurrentInfo]->setText(szText);
				//m_pTeamInfoPanel[m_iCurrentInfo]->setFgColor(r,g,b, 0);
			}
			else
				m_pTeamInfoPanel[m_iCurrentInfo]->setText(GetTeamName(m_iCurrentInfo));

			m_pTeamInfoPanel[m_iCurrentInfo]->setVisible(true);
		}
	}
	m_ScrollPanel.validate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Reset(void)
{
	CTeamMenuPanel::BaseClass::Reset();
	m_iCurrentInfo = 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: radical way to force player to select a team
// Output : Return true to ALLOW and false to PREVENT closing
//-----------------------------------------------------------------------------
bool CTeamMenuPanel::OnClose(void)
{
	if (IsPersistent())
	{
		CenterPrint(BufferedLocaliseTextString("#Team_MustSelect"));
		return false;
	}
	else
		return CTeamMenuPanel::BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: don't allow this window to be closed if user must choose
// Output : true or false
//-----------------------------------------------------------------------------
bool CTeamMenuPanel::IsPersistent(void)
{
	if (!IsTeamGame(gHUD.m_iGameType) || (gHUD.m_iTeamNumber > TEAM_NONE) || gHUD.IsSpectator())// XDM3033: is this safe thing to do?
		return false;// CTeamMenuPanel::BaseClass::IsPersistent();?

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: don't allow other window to be opened
// Output : Return true to ALLOW
//-----------------------------------------------------------------------------
bool CTeamMenuPanel::AllowConcurrentWindows(void)
{
	return !IsPersistent();
}
