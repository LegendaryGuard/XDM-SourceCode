#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "keydefs.h"


//-----------------------------------------------------------------------------
// Purpose: Read the Command Menu structure from the txt file and create the menu.
//			Returns Index of menu in m_pCommandMenus
// TODO: additionally parse maps/mapname_menu.txt to add map-specific menu items: bool bAppendMapMenu
// Output : Returns pointer to a new root menu.
//-----------------------------------------------------------------------------
/*CCommandMenu *CCommandMenu::CreateCommandMenu(char *menuFile, int direction, int yOffset, int design, int flButtonSizeX, int flButtonSizeY, int xOffset, int iMaxMenus)
{
 UNDONE
}*/


//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pParentMenu - 
//			x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
CCommandMenu::CCommandMenu(CCommandMenu *pParentMenu, int x, int y, int wide, int tall) : Panel(x,y,wide,tall)//CMenuPanel(0,x,y,wide,tall)
{
	m_iButtonSizeY = CBUTTON_SIZE_Y;
	m_iXOffset = x;
	m_iYOffset = y;
	m_pParentMenu = pParentMenu;
	memset(&m_aButtons, 0, sizeof(void*)*MAX_BUTTONS);
	m_hCommandMenuScheme = 0;
	m_iButtons = 0;
	m_iDirection = 0;
	Initialize();
	setVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pParentMenu - 
//			direction - 
//			x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
CCommandMenu::CCommandMenu(CCommandMenu *pParentMenu, int direction, int x, int y, int wide, int tall) : Panel(x,y,wide,tall)//CMenuPanel(0,x,y,wide,tall)
{
	m_iButtonSizeY = CBUTTON_SIZE_Y;
	m_iXOffset = x;
	m_iYOffset = y;
	m_pParentMenu = pParentMenu;
	memset(&m_aButtons, 0, sizeof(void*)*MAX_BUTTONS);//m_aButtons[0] = NULL;
	m_hCommandMenuScheme = 0;
	m_iButtons = 0;
	m_iDirection = direction;
	Initialize();
	setVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Initialize
//-----------------------------------------------------------------------------
void CCommandMenu::Initialize(void)
{
	m_hCommandMenuScheme = gViewPort->GetSchemeManager()->getSchemeHandle("CommandMenu Text");
	setBgColor(0,0,0,255);// don't draw menu BG, buttons do that
	setPaintBackgroundEnabled(false);
	//m_TitleIcon.setPaintEnabled(false);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Open
//-----------------------------------------------------------------------------
void CCommandMenu::Open(void)
{
	MakeVisible(NULL);
	if (m_pParentMenu == NULL)// this is a root menu
		PlaySound("vgui/menu_activate.wav", VOL_NORM);// XDM
}

//-----------------------------------------------------------------------------
// Purpose: Close
//-----------------------------------------------------------------------------
void CCommandMenu::Close(void)
{
	ClearButtonsOfArmedState();
	if (isVisible())
	{
		setVisible(false);
		if (m_pParentMenu == NULL)// this is a root menu
			PlaySound("vgui/menu_close.wav", VOL_NORM);// XDM
	}
	gViewPort->OnCommandMenuClosed(this);// XDM3038c: !
	// ^ gViewPort->UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Update
//-----------------------------------------------------------------------------
void CCommandMenu::Update(void)
{
	RecalculateVisibles(0, false);
	RecalculatePositions(0);
}

//-----------------------------------------------------------------------------
// Purpose: AddButton
// Input  : *pButton - 
//-----------------------------------------------------------------------------
void CCommandMenu::AddButton(CommandButton *pButton)
{
	if (!pButton)// XDM3038a
		return;
	if (m_iButtons >= MAX_BUTTONS)
		return;

	m_aButtons[m_iButtons] = pButton;
	++m_iButtons;
	pButton->setParent(this);
	pButton->SetScheme(gViewPort->GetSchemeManager()->getSafeScheme(m_hCommandMenuScheme));// XDM3037

	// Give the button a default key binding, but only if it already hasn't one
	if (pButton->getBoundKey() == 0)// XDM3038c
	{
		if (m_iButtons <= 9)
			pButton->setBoundKey('0' + m_iButtons);
		else
			pButton->setBoundKey('a' + m_iButtons-10);
	}
}

//-----------------------------------------------------------------------------
// Purpose: HL20130901
//-----------------------------------------------------------------------------
void CCommandMenu::RemoveAllButtons(void)
{
	/*for(int i=0;i<m_iButtons;i++)
	{
		CommandButton *pTemp = m_aButtons[i]; 
		m_aButtons[i] = NULL;
		pTemp
		if (pTemp)
		{
			delete(pTemp);
		}
	}*/
	removeAllChildren();
	m_iButtons = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Tries to find a button that has a key bound to the input, and
//			presses the button if found
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 0: handled, 1: unhandled, allow other panels to recieve key
//-----------------------------------------------------------------------------
int CCommandMenu::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down)
	{
		if (keynum == K_ESCAPE || keynum == K_SPACE)// K_ESCAPE should not get here! Must be handled by UI outside!
		{
			Close();
			return 0;
		}
		// loop through all our buttons looking for one bound to keyNum
		for (uint32 i = 0; i < m_iButtons; ++i)
		{
			if (!m_aButtons[i]->IsNotValid())
			{
				if (m_aButtons[i]->getBoundKey() == keynum)
				{
					if (m_aButtons[i]->GetSubMenu())// open the sub menu
					{
						m_aButtons[i]->setArmed(true);// XDM3038a
						gViewPort->SetCurrentCommandMenu(m_aButtons[i]->GetSubMenu());
					}
					else// run the bound command
						m_aButtons[i]->fireActionSignal();

					return 0;
				}
			}
		}
	}
	//return false;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: clears the current menus buttons of any armed (highlighted) 
//			state, and all their sub buttons
//-----------------------------------------------------------------------------
void CCommandMenu::ClearButtonsOfArmedState(void)
{
	for (uint32 i = 0; i < GetNumButtons(); ++i)
	{
		m_aButtons[i]->setArmed(false);

		if (m_aButtons[i]->GetSubMenu())
			m_aButtons[i]->GetSubMenu()->ClearButtonsOfArmedState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSubMenu - 
// Output : CommandButton
//-----------------------------------------------------------------------------
CommandButton *CCommandMenu::FindButtonWithSubmenu(CCommandMenu *pSubMenu)
{
	for (uint32 i = 0; i < GetNumButtons(); ++i)
	{
		if (m_aButtons[i]->GetSubMenu() == pSubMenu)
			return m_aButtons[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the visible buttons
// Input  : iYOffset - 
//			bHideAll - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCommandMenu::RecalculateVisibles(int iYOffset, bool bHideAll)
{
	uint32 i;
	int iCurrentY = 0;
	int iVisibleButtons = 0;

	// Cycle through all the buttons in this menu, and see which will be visible
	for (i = 0; i < m_iButtons; ++i)
	{
		if (m_aButtons[i]->IsNotValid() || bHideAll)
		{
			m_aButtons[i]->setVisible(false);
			if (m_aButtons[i]->GetSubMenu() != NULL)
				(m_aButtons[i]->GetSubMenu())->RecalculateVisibles(0, true);
		}
		else
		{
 			// If it's got a submenu, force it to check visibilities
			if (m_aButtons[i]->GetSubMenu() != NULL)
			{
				if (!(m_aButtons[i]->GetSubMenu())->RecalculateVisibles(0, false))
				{
					// The submenu had no visible buttons, so don't display this button
					m_aButtons[i]->setVisible(false);
					continue;
				}
			}
			m_aButtons[i]->setVisible(true);
			++iVisibleButtons;
		}
	}

	// Set Size
	setSize(_size[0], (iVisibleButtons * (m_iButtonSizeY-1)) + 1);

	if (iYOffset != 0)
		m_iYOffset = iYOffset;

	for (i = 0; i < m_iButtons; ++i)
	{
		if (m_aButtons[i]->isVisible())
		{
			if (m_aButtons[i]->GetSubMenu() != NULL)
				(m_aButtons[i]->GetSubMenu())->RecalculateVisibles(iCurrentY + m_iYOffset, false);

			// Make sure it's at the right Y position
			// m_aButtons[i]->getPos( iXPos, iYPos );
			if (m_iDirection)
				m_aButtons[i]->setPos(0, (iVisibleButtons-1) * (m_iButtonSizeY-1) - iCurrentY);
			else
				m_aButtons[i]->setPos(0, iCurrentY);

			iCurrentY += (m_iButtonSizeY-1);
		}
	}
	return iVisibleButtons?true:false;
}

//-----------------------------------------------------------------------------
// Purpose: Make sure all submenus can fit on the screen
// Input  : iYOffset - 
//-----------------------------------------------------------------------------
void CCommandMenu::RecalculatePositions(int iYOffset)
{
	int iTop;
	int iAdjust = 0;

	m_iYOffset += iYOffset;

	if (m_iDirection == CMENU_DIR_UP)
		iTop = ScreenHeight - (m_iYOffset + _size[1]);
	else
		iTop = m_iYOffset;

	if (iTop < 0)
		iTop = 0;

	// Calculate if this is going to fit onscreen, and shuffle it up if it won't
	int iBottom = iTop + _size[1];
	if (iBottom > ScreenHeight)
	{
		// Move in increments of button sizes
		while (iAdjust < (iBottom - ScreenHeight))
			iAdjust += m_iButtonSizeY - 1;

		iTop -= iAdjust;

		// Make sure it doesn't move off the top of the screen (the menu's too big to fit it all)
		if (iTop < 0)
		{
			iAdjust -= (0 - iTop);
			iTop = 0;
		}
	}

	setPos(_pos[0], iTop);

	// We need to force all menus below this one to update their positions now, because they
	// might have submenus riding off buttons in this menu that have just shifted.
	for (uint32 i = 0; i < m_iButtons; ++i)
		m_aButtons[i]->UpdateSubMenus(iAdjust);
}

//-----------------------------------------------------------------------------
// Purpose: Make this menu and all menus above it in the chain visible
// Input  : *pChildMenu - 
//-----------------------------------------------------------------------------
void CCommandMenu::MakeVisible(CCommandMenu *pChildMenu)
{
	// Push down the button leading to the child menu
	/*for (int i = 0; i < m_iButtons; ++i)
	{
		if ((pChildMenu != NULL) && (m_aButtons[i]->GetSubMenu() == pChildMenu))
			m_aButtons[i]->setArmed(true);
		else
			m_aButtons[i]->setArmed(false);
	}*/
	setVisible(true);
	//PlaySound("vgui/menu_activate.wav", VOL_NORM);// XDM

	if (m_pParentMenu)
		m_pParentMenu->MakeVisible(this);
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPlayersCommandMenu::CPlayersCommandMenu(CCommandMenu *pParentMenu, CMenuPanel *pParentPanel, bool bIncludeSpectators, bool bIncludeLocal, bool bIncludeEnemies, int iPlayerSignalOffset, int direction, int x, int y, int wide, int tall) : CCommandMenu(pParentMenu, direction, x, y, wide, tall), m_pParentPanel(pParentPanel), m_iPlayerSignalOffset(iPlayerSignalOffset)
{
	m_bIncludeSpectators = bIncludeSpectators;
	m_bIncludeLocal = bIncludeLocal;
	m_bIncludeEnemies = bIncludeEnemies;
	// not now setParent(pParentPanel);// ?
}

//-----------------------------------------------------------------------------
// Purpose: Initialize
//-----------------------------------------------------------------------------
void CPlayersCommandMenu::Initialize(void)
{
	CCommandMenu::Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Close
//-----------------------------------------------------------------------------
/*void CPlayersCommandMenu::Close(void)
{
	ClearButtonsOfArmedState();
	CCommandMenu::Close();
}*/

//-----------------------------------------------------------------------------
// Purpose: Client DLL player information has changed, we can rebuild player name buttons here
// Input  : player - 0=all, NOT USED NOW
//-----------------------------------------------------------------------------
void CPlayersCommandMenu::UpdateOnPlayerInfo(int player)
{
	RemoveAllButtons();
	/*int bx, by;
	m_pSpectatorPanel->m_BottomMainButton->getPos(bx, by);
	m_pSpectatorPanel->m_BottomMainButton->localToScreen(bx, by);
	//if (getParent())
	//	getParent()->screenToLocal(bx, by);
	bx = test1->value;
	by = test2->value;*/
	//int by = m_iYOffset;
	///if (m_pSpectatorPanel)
	/// bad		by = m_pSpectatorPanel->GetBorderHeight();
	///else
	//	by = YRES(SPECTATORPANEL_HEIGHT);

	//setPos(ScreenWidth/2 - getWide()/2, by);
	RecalculateVisibles(0, false);// way to set pMenu->m_iYOffset

	/* Works, but we don't need this
	CommandButton *pButton = new CommandButton(BufferedLocaliseTextString("#Menu_Close"),0,0,getWide(), BUTTON_SIZE_Y/2, false, true, false, NULL);
	pButton->setBoundKey(' ');
	pButton->setContentAlignment(vgui::Label::a_center);
	AddButton(pButton);
	pButton->setParentMenu(pMenu);
	pButton->addActionSignal(new CMenuActionHandler(pMenu, 0, true));
	pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, pMenu));	*/

	CommandButton *pButton;
	byte r,g,b;
	char szLabel[MAX_BUTTON_TEXTLEN];
	for (CLIENT_INDEX i = 1; i <= MAX_PLAYERS; ++i)
	{
		if (!IsValidPlayer(i))// IsActivePlayer was wrong
			continue;

		if (!m_bIncludeSpectators && IsSpectator(i))
			continue;

		if (!m_bIncludeLocal && gHUD.m_pLocalPlayer && i == gHUD.m_pLocalPlayer->index)
			continue;

		if (!m_bIncludeEnemies && IsTeamGame(gHUD.m_iGameType) && !(gHUD.m_iGameFlags & GAME_FLAG_ALLOW_SPECTATE_ALL))
		{
			// XDM3035a: WARNING! current team of a spectator is 0 ("spectators")! last team the player has played on is stored in "playerclass". Hack?
			// XDM3038a: OBSOLETE if (gHUD.m_pLocalPlayer->curstate.playerclass != TEAM_NONE && g_PlayerExtraInfo[i].teamnumber != gHUD.m_pLocalPlayer->curstate.playerclass)
			// XDM3038c: team is now saved
			if (gHUD.m_iTeamNumber != TEAM_NONE && g_PlayerExtraInfo[i].teamnumber != gHUD.m_iTeamNumber)
				continue;
		}

		_snprintf(szLabel, MAX_BUTTON_TEXTLEN, "[%d] %s", i, g_PlayerInfoList[i].name);
		szLabel[MAX_BUTTON_TEXTLEN-1] = '\0';
		//pButton = new CommandButton(g_PlayerInfoList[i].name,0,0,getWide(), (ScreenHeight-by)/MAX_PLAYERS, false, true, false, NULL);
		pButton = new CommandButton(szLabel, 0, i, getWide(), m_iButtonSizeY, false, true, true, NULL);
		if (pButton)
		{
			if (i <= 9)
				pButton->setBoundKey('1'+i-1);
			else
				pButton->setBoundKey('a'+i-10);

			pButton->setContentAlignment(vgui::Label::a_center);
			AddButton(pButton);
			pButton->setParentMenu(this);
			//pButton->setFont(Scheme::sf_primary2);// scoreboard font
			if (m_pParentPanel)
				pButton->addActionSignal(new CMenuPanelActionSignalHandler(m_pParentPanel, m_iPlayerSignalOffset+i));

			pButton->addActionSignal(new CMenuActionHandler(this, VGUI_IDCLOSE, true));
			// Create an input signal that'll popup the current menu
			// XDM3038a: obsolete? pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, pMenu));
			GetPlayerColor(i, r,g,b);
			pButton->m_ColorNormal.Set(r,g,b,191);
			pButton->m_ColorArmed.Set(r,g,b,255);
		}
		else
			DBG_PRINTF("CPlayersCommandMenu::UpdateOnPlayerInfo(): error creating CommandButton!\n");
	}
}
