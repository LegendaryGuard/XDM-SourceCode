#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"
#include "pm_shared.h"
#include "keydefs.h"
#include "screenfade.h"// specfade
#include "shake.h"
#include "demo.h"
#include "demo_api.h"
#include "voice_status.h"
#include "vgui_Int.h"
#include "vgui_Viewport.h"
#include "vgui_ServerBrowser.h"
#include "vgui_ScorePanel.h"
#include "vgui_SpectatorPanel.h"
#include "vgui_TeamMenu.h"// XDM
#include "vgui_MessageWindow.h"
#include "vgui_MusicPlayer.h"
#include <new>// XDM3038c: is this really necessary?
#ifdef USE_EXCEPTIONS
#include <exception>
#endif

// Warning: this file contains hacks. Even after rewriting all valve's stuff.

CViewport *gViewPort = NULL;

#define MAX_GRINTRO_LENGTH				512// unused: GetGameRulesIntroText
#define MAX_TEXTSEPARATOR_LENGTH		80// localized "-------"
#define MAX_MAPDESCRIPTION_LENGTH		8192// mapname.txt
#define MAX_GAMEFLAGDESC_LENGTH			80

static const int MAX_TITLE_LENGTH		= MAX_MAPNAME + MAX_SERVERNAME_LENGTH + 16;
static const int MAX_INTRO_TEXT_LENGTH	= MAX_MOTD_LENGTH + MAX_GRINTRO_LENGTH + MAX_GAMEFLAGDESC_LENGTH*MAX_GAME_FLAGS + MAX_TITLE_LENGTH + MAX_MAPDESCRIPTION_LENGTH + MAX_TEXTSEPARATOR_LENGTH*4;


using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Makes sure the memory allocated for Viewport is nulled out
// Note   : It happens once, so it can be a little slow
// Input  : stAllocateBlock - 
// Output : void *
//-----------------------------------------------------------------------------
void *CViewport::operator new(size_t stAllocateBlock)
{
	//void *mem = Panel::operator new(stAllocateBlock);
// NOT AN OPTION! #if defined (USE_EXCEPTIONS)
	try
	{
//#endif
		void *pNewViewportMemBlock = ::operator new(stAllocateBlock);
		ASSERT(pNewViewportMemBlock != NULL);
		memset(pNewViewportMemBlock, 0, stAllocateBlock);
		return pNewViewportMemBlock;
//#if defined (USE_EXCEPTIONS)
	}
	catch (std::bad_alloc &ba)// XDM3038b
	{
		DBG_PRINT_GUI(ba.what());
		conprintf(0, ba.what());
		DBG_FORCEBREAK
		return NULL;
	}
//#endif
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
CViewport::CViewport(int x,int y,int wide,int tall) : Panel(x,y,wide,tall), m_SchemeManager(wide,tall)
{
	DBG_PRINT_GUI("CViewport::CViewport(%d, %d, %d, %d)\n", x, y, wide, tall);
	gViewPort = this;
	m_iInitialized = false;
	//m_iNumberOfTeams = 0;

	m_flMenuOpenTime = m_flScoreBoardLastUpdated = m_flSpectatorPanelLastUpdated = 0.0f;
	// ! the earlier the better
	m_iNumMenus = m_iCurrentTeamNumber = m_iUser1 = m_iUser2 = m_iUser3 = 0;

	m_pScoreBoard = NULL;
	m_pMusicPlayer = NULL;
	m_pTeamMenuPanel = NULL;
	m_pSpectatorPanel = NULL;
	m_pServerBrowser = NULL;// XDM3038c

	/*m_StandardMenu = 0;
	m_SpectatorOptionsMenu = 0;
	m_SpectatorCameraMenu = 0;*/

	Initialize();
	addInputSignal(new CViewPortInputHandler);

	// VGUI MENUS
	CreateScoreBoard();

	m_pTeamMenuPanel = new CTeamMenuPanel(false, 0, 0, ScreenWidth, ScreenHeight);
	if (m_pTeamMenuPanel)
	{
		m_pTeamMenuPanel->setParent(this);
		m_pTeamMenuPanel->setVisible(false);
	}

	m_pSpectatorPanel = new SpectatorPanel(0, 0, ScreenWidth, ScreenHeight);
	if (m_pSpectatorPanel)
	{
		m_pSpectatorPanel->setParent(this);
		m_pSpectatorPanel->setVisible(false);
	}

	m_pMusicPlayer = new CMusicPlayerPanel(false, 0, 0, XRES(MPLAYER_PANEL_SIZEX), YRES(MPLAYER_PANEL_SIZEY));
	if (m_pMusicPlayer)
	{
		m_pMusicPlayer->setParent(this);
		m_pMusicPlayer->setVisible(false);
	}

	// Init command menus
	m_StandardMenu = CreateCommandMenu("scripts/commandmenu.txt", CMENU_DIR_DOWN, CMENU_TOP, 0, CMENU_SIZE_X, CBUTTON_SIZE_Y, 0);
	m_SpectatorOptionsMenu = CreateCommandMenu("scripts/spectatormenu.txt", CMENU_DIR_UP, YRES(32), 1, CMENU_SIZE_X, BUTTON_SIZE_Y/2, 0);// above bottom bar, flat design
	m_SpectatorCameraMenu = CreateCommandMenu("scripts/spectcammenu.txt", CMENU_DIR_UP, YRES(32), 1, CMENU_SIZE_X, BUTTON_SIZE_Y/2, ScreenWidth-CMENU_SIZE_X);// above bottom bar, flat design

	/*if (m_pSpectatorPanel)
	{
		m_PlayerMenu = m_iNumMenus;// HL20130901 Last menu
		m_iNumMenus++;
		m_pCommandMenus[m_PlayerMenu] = new CCommandMenu(NULL, CMENU_DIR_UP, ScreenWidth/2, ScreenHeight-CBUTTON_SIZE_Y, CMENU_SIZE_X, CBUTTON_SIZE_Y);// default values will be overridden (right now Specpanel button position is 0,0)
		if (m_pCommandMenus[m_PlayerMenu])
		{
			m_pCommandMenus[m_PlayerMenu]->setParent(this);
			m_pCommandMenus[m_PlayerMenu]->setVisible(false);
			m_pCommandMenus[m_PlayerMenu]->m_iButtonSizeY = BUTTON_SIZE_Y /2;
			//m_pCommandMenus[m_PlayerMenu]->m_iSpectCmdMenu = 1;
			UpdateCommandMenu(m_PlayerMenu);
		}
	}*/

	/* XDM3038c: free some space m_pServerBrowser = new ServerBrowser(0, 0, ScreenWidth, ScreenHeight);
	if (m_pServerBrowser)
	{
		m_pServerBrowser->setParent(this);
		m_pServerBrowser->setVisible(false);
	}*/
	// it should be here!! Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called (from VGui_Startup) everytime a new level is started. Viewport clears out it's data.
// XDM: NO it isn't!
//-----------------------------------------------------------------------------
void CViewport::Initialize(void)
{
	DBG_PRINT_GUI("CViewport::Initialize()\n");

	// reset player info
	//??? gHUD.m_iTeamNumber = TEAM_NONE;
	m_pCurrentCommandMenu = NULL;
	m_pCurrentMenuPanel = NULL;

	LoadScheme();// XDM3038: earlier

	//m_sMapName[0] = 0;
	m_szServerName[0] = 0;
	LocaliseTextString("#Spectators", g_TeamInfo[0].name, MAX_TEAMNAME_LENGTH);// XDM3035a

	// Force each menu to Initialize
	if (m_pTeamMenuPanel)
		m_pTeamMenuPanel->Initialize();

	if (m_pScoreBoard)
		m_pScoreBoard->Initialize();

	if (m_pSpectatorPanel)
		m_pSpectatorPanel->Initialize();
		//m_pSpectatorPanel->setVisible(false);

	if (m_pMusicPlayer)// XDM
		m_pMusicPlayer->Initialize();

	// Make sure all menus are hidden
	HideVGUIMenu();
	HideCommandMenu();

	// Clear out some data
	m_flMenuOpenTime = m_flScoreBoardLastUpdated = m_flSpectatorPanelLastUpdated = 0.0f;

	App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_none));// was Scheme::SchemeCursor::scu_none ..?
	m_iGotAllMOTD = true;
	m_iInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: LoadScheme
//-----------------------------------------------------------------------------
void CViewport::LoadScheme(void)
{
	DBG_PRINT_GUI("CViewport::LoadScheme()\n");
	Scheme *pScheme = App::getInstance()->getScheme();
	if (pScheme == NULL)
	{
		conprintf(0, "CViewport::LoadScheme() error: pScheme is NULL!\n");
		return;
	}
	// Upload colors from local scheme into vgui scheme
	//!! two different types of scheme here, need to integrate
	CScheme *pLocalScheme = m_SchemeManager.getScheme("Basic Text");
	ASSERT(pLocalScheme != NULL);
	if (pLocalScheme)
	{
		m_iBorderThickness = pLocalScheme->BorderThickness;
		m_BorderColor = pLocalScheme->BorderColor;
		m_BorderColorArmed = pLocalScheme->BorderColorArmed;
		// TODO: remove these when you get rid of all controls that don't use XDM CSchemes
		// font
		pScheme->setFont(Scheme::sf_primary1, pLocalScheme->font);
		// 3d border colors
		pScheme->setColor(Scheme::sc_black, pLocalScheme->DarkColor[0], pLocalScheme->DarkColor[1], pLocalScheme->DarkColor[2], 255-pLocalScheme->DarkColor[3]);
		pScheme->setColor(Scheme::sc_white, pLocalScheme->BrightColor[0], pLocalScheme->BrightColor[1], pLocalScheme->BrightColor[2], 255-pLocalScheme->BrightColor[3]);
		// text color
		pScheme->setColor(Scheme::sc_primary1, pLocalScheme->FgColor[0], pLocalScheme->FgColor[1], pLocalScheme->FgColor[2], 255-pLocalScheme->FgColor[3]);
		pScheme->setColor(Scheme::sc_secondary1, pLocalScheme->FgColorArmed[0], pLocalScheme->FgColorArmed[1], pLocalScheme->FgColorArmed[2], 255-pLocalScheme->FgColorArmed[3]);
		// background color
		pScheme->setColor(Scheme::sc_primary2, pLocalScheme->BgColor[0], pLocalScheme->BgColor[1], pLocalScheme->BgColor[2], 255-pLocalScheme->BgColor[3]);
		pScheme->setColor(Scheme::sc_secondary2, pLocalScheme->BgColorArmed[0], pLocalScheme->BgColorArmed[1], pLocalScheme->BgColorArmed[2], 255-pLocalScheme->BgColorArmed[3]);
		// borders around buttons
		pScheme->setColor(Scheme::sc_primary3, pLocalScheme->BorderColor[0], pLocalScheme->BorderColor[1], pLocalScheme->BorderColor[2], 255-pLocalScheme->BorderColor[3]);
		pScheme->setColor(Scheme::sc_secondary3, pLocalScheme->BorderColorArmed[0], pLocalScheme->BorderColorArmed[1], pLocalScheme->BorderColorArmed[2], 255-pLocalScheme->BorderColorArmed[3]);
		// mouse down foreground
		pScheme->setColor(Scheme::sc_user, pLocalScheme->FgColorClicked[0], pLocalScheme->FgColorClicked[1], pLocalScheme->FgColorClicked[2], 255-pLocalScheme->FgColorClicked[3]);
	}

	// Change the second primary font (used in the scoreboard)
	CScheme *pScoreboardScheme = m_SchemeManager.getScheme("Scoreboard Text");
	ASSERT(pScoreboardScheme != NULL);
	if (pScoreboardScheme)
		pScheme->setFont(Scheme::sf_primary2, pScoreboardScheme->font);

	// Change the third primary font (used in command menu)
	CScheme *pCommandMenuScheme = m_SchemeManager.getScheme("CommandMenu Text");
	ASSERT(pCommandMenuScheme != NULL);
	if (pCommandMenuScheme)
		pScheme->setFont(Scheme::sf_primary3, pCommandMenuScheme->font);

	App::getInstance()->setScheme(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: Read the Command Menu structure from the txt file and create the menu.
//			Returns Index of menu in m_pCommandMenus
// TODO: additionally parse maps/mapname_menu.txt to add map-specific menu items
// bool bAppendMapMenu
//-----------------------------------------------------------------------------
int CViewport::CreateCommandMenu(char *menuFile, int direction, int yOffset, int design, int flButtonSizeX, int flButtonSizeY, int xOffset)
{
	DBG_PRINT_GUI("CViewport::CreateCommandMenu(%s, %d, %d, %d, %d, %d, %d)\n", menuFile, direction, yOffset, design, flButtonSizeX, flButtonSizeY, xOffset);
	// Create the root of this new Command Menu
	int newIndex = m_iNumMenus;
	m_pCommandMenus[newIndex] = new CCommandMenu(NULL, direction, xOffset, yOffset, flButtonSizeX, 256);	// This will be resized once we know how many items are in it
	if (m_pCommandMenus[newIndex] == NULL)
	{
		conprintf(0, "CreateCommandMenu(): error creating CCommandMenu!\n");
		return 0;
	}
	m_pCurrentCommandMenu = m_pCommandMenus[newIndex];
	m_pCurrentCommandMenu->setParent(this);
	m_pCurrentCommandMenu->setVisible(false);
	m_pCurrentCommandMenu->m_iButtonSizeY = (int)flButtonSizeY;
	++m_iNumMenus;

	// Read Command Menu from the txt file
	char token[1024];
	char *pfile = (char *)gEngfuncs.COM_LoadFile(menuFile, 5, NULL);
	if (!pfile)
	{
		conprintf(0, "CreateCommandMenu() error: unable to open \"%s\"\n", menuFile);
		SetCurrentCommandMenu(NULL);
		return newIndex;
	}
	char *pFileStart = pfile;// XDM3035c

#if defined (USE_EXCEPTIONS)
try
{
#endif
	// First, read in the localisation strings
	// Now start parsing the menu structure
	static const size_t cCommandLength = 128;
	static const size_t iKeyLength = 16;
	static const size_t iItemTypeLength = 16;
	static const size_t iItemTextLength = 32;
	char cCommand[cCommandLength] = "";
	char szLastButtonText[32] = "";
	char cBoundKey[iKeyLength] = "";// UNDONE: should be key NAME, in plain text, like in kb_keys.lst, but we don't have access to key names
	char cItemType[iItemTypeLength] = "";
	char cItemText[iItemTextLength] = "";
	char szMap[MAX_MAPNAME] = "";
	int iCustom = false;
	int iTeamOnly = -1;
	int iToggle = 0;
	int iButtonY;
	CommandButton *pButton = NULL;
	bool bGetExtraToken = true;
	pfile = gEngfuncs.COM_ParseFile(pfile, token);
	while ((strlen(token) > 0) && (m_iNumMenus < MAX_MENUS))
	{
		// Keep looping until we hit the end of this menu
		while (token[0] != '}' && (strlen(token) > 0))
		{
			// We should never be here without a Command Menu
			if (!m_pCurrentCommandMenu)
			{
				conprintf(0, "Error in \"%s\" after \"%s\".\n", menuFile, szLastButtonText);
				m_iInitialized = false;
				return newIndex;
			}

			iCustom = false;
			iTeamOnly = -1;
			iToggle = 0;
			bGetExtraToken = true;
			// token should already be the bound key, or the custom name
			strncpy(cItemType, token, iItemTypeLength);
			cItemType[iItemTypeLength-1] = '\0';

			if (strcmp(cItemType, "CUSTOM") == 0)// See if it's a custom button
			{
				iCustom = true;
				pfile = gEngfuncs.COM_ParseFile(pfile, token);// get next token
			}
			else if (strcmp(cItemType, "MAP") == 0)// See if it's a map
			{
				pfile = gEngfuncs.COM_ParseFile(pfile, token);// Get the mapname
				strncpy(szMap, token, MAX_MAPNAME);
				szMap[MAX_MAPNAME-1] = '\0';
				pfile = gEngfuncs.COM_ParseFile(pfile, token);// get next token
			}
			else if (strbegin(cItemType, "TEAM") > 0)// "TEAM1", "TEAM%d"
			{
				// make it a team only button
				iTeamOnly = atoi(cItemType + 4);
				pfile = gEngfuncs.COM_ParseFile(pfile, token);// get next token
			}
			else if (strbegin(cItemType, "TOGGLE") > 0) 
			{
				iToggle = true;
				pfile = gEngfuncs.COM_ParseFile(pfile, token);// get next token
			}
			//else don't get token, because current token IS a key

			// Get the button bound key
			strncpy(cBoundKey, token, iKeyLength);
			cBoundKey[iKeyLength-1] = '\0';

			// Get the button text
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strncpy(cItemText, token, iItemTextLength);
			cItemText[iItemTextLength-1] = '\0';
			// save off the last button text we've come across (for error reporting)
			strncpy(szLastButtonText, cItemText, iItemTextLength);

			// Get the button command
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strncpy(cCommand, token, cCommandLength);
			cCommand[cCommandLength-1] = '\0';
			// replace single quotes with double quotes?
			//for (size_t ci=0; ci<cCommandLength, cCommand[ci] != '\0'; ++ci)
			//	if (cCommand[ci] == '\'')
			//		cCommand[ci] = '\"';

			iButtonY = (BUTTON_SIZE_Y-1) * m_pCurrentCommandMenu->GetNumButtons();

			// Custom button handling
			if (szMap[0] != '\0')// create a map button
				pButton = new MapButton(szMap, cItemText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY);
			else if (iTeamOnly != -1)// button that only shows up if the player is on team iTeamOnly
				pButton = new TeamOnlyCommandButton(iTeamOnly, cItemText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, design==1, true);
			else if (iToggle)// && direction == 0)
				pButton = new ToggleCommandButton(CVAR_GET_FLOAT(cCommand) > 0, cItemText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, design==1, true);
			else// normal button
				pButton = new CommandButton(cItemText, xOffset, iButtonY, flButtonSizeX, flButtonSizeY, false, design==1, true);

			// add the button into the command menu
			if (pButton)
			{
				m_pCurrentCommandMenu->AddButton(pButton);
				pButton->setBoundKey(cBoundKey[0]);
				pButton->setParentMenu(m_pCurrentCommandMenu);

				// Find out if it's a submenu or a button we're dealing with
				if (cCommand[0] == '{')
				{
					if (m_iNumMenus < MAX_MENUS)
					{
						// Create the menu
						m_pCommandMenus[m_iNumMenus] = CreateSubMenu(pButton, m_pCurrentCommandMenu, iButtonY);
						m_pCurrentCommandMenu = m_pCommandMenus[m_iNumMenus];
						m_iNumMenus++;
					}
					else
						conprintf(0, "Too many menus in \"%s\" past \"%s\"\n", menuFile, szLastButtonText);
				}
				else if (!iCustom)
				{
					// Create the button and attach it to the current menu
					if (iToggle)
						pButton->addActionSignal(new CMenuHandler_ToggleCvar(m_pCurrentCommandMenu, cCommand));
					else
						pButton->addActionSignal(new CMenuHandler_StringCommand(m_pCurrentCommandMenu, cCommand, true));

					// Create an input signal that'll keep the current menu AND hide inactive submenus
					pButton->addInputSignal(new CMenuHandler_PopupSubMenuInput(pButton, m_pCurrentCommandMenu));

					// XDM3038c: Get the tooltip from a comment
					/*pfile = COM_Parse(pfile, token, true);
					if (token)
					{
						if (token[0] == '/' && token[1] == '/')
							pButton->setToolTip(token+2);
						else
							bGetExtraToken = false;// this was not a comment, must be a new entry
					}
					else
						break;*/
				}
			}

			// Get the next token
			if (bGetExtraToken)
				pfile = gEngfuncs.COM_ParseFile(pfile, token);
		}
		// Move back up a menu
		m_pCurrentCommandMenu = m_pCurrentCommandMenu->GetParentMenu();

		pfile = gEngfuncs.COM_ParseFile(pfile, token);
	}
#if defined (USE_EXCEPTIONS)
}
catch(...)
{
	conprintf(0, "CViewport::CreateCommandMenu() exception!\n");
	// WTF!! m_iInitialized = false;
	return newIndex;
}
#endif

	SetCurrentMenu(NULL);
	SetCurrentCommandMenu(NULL);
	if (pFileStart)
		gEngfuncs.COM_FreeFile(pFileStart);

	// WTF!!!! m_iInitialized = true;
	return newIndex;
}

//-----------------------------------------------------------------------------
// Purpose: CreateSubMenu
// Input  : *pButton - 
//			*pParentMenu - 
//			iYOffset - 
//			iXOffset - 
// Output : CCommandMenu
//-----------------------------------------------------------------------------
CCommandMenu *CViewport::CreateSubMenu(CommandButton *pButton, CCommandMenu *pParentMenu, int iYOffset, int iXOffset)
{
	int iXPos = 0;
	int iYPos = 0;
	int iWide = CMENU_SIZE_X;
	int iTall = 0;
	int iDirection = 0;

	if (pParentMenu && m_pCurrentCommandMenu)
	{
		iXPos = m_pCurrentCommandMenu->GetXOffset() + (CMENU_SIZE_X - 1) + iXOffset;
		iYPos = m_pCurrentCommandMenu->GetYOffset() + iYOffset;
		iDirection = pParentMenu->GetDirection();
	}

	CCommandMenu *pMenu = new CCommandMenu(pParentMenu, iDirection, iXPos, iYPos, iWide, iTall);
	if (pMenu)
	{
		pMenu->setParent(this);
		//pMenu->m_TitleIcon.setPaintEnabled(false);// XDM3038a
		pButton->AddSubMenu(pMenu);
		pButton->setFont(Scheme::sf_primary3);
		pMenu->m_iButtonSizeY = m_pCurrentCommandMenu?m_pCurrentCommandMenu->m_iButtonSizeY:BUTTON_SIZE_Y;
		// Create the Submenu-open signal
		InputSignal *pISignal = new CMenuHandler_PopupSubMenuInput(pButton, pMenu);
		if (pISignal)
			pButton->addInputSignal(pISignal);

		// Put a > to show it's a submenu
		CImageLabel *pLabel = new CImageLabel("arrowright", CMENU_SIZE_X - SUBMENU_SIZE_X, SUBMENU_SIZE_Y);
		if (pLabel)
		{
			pLabel->setParent(pButton);
			if (pISignal)
				pLabel->addInputSignal(pISignal);
			// Reposition
			pLabel->getPos(iXPos, iYPos);
			int iw,it;
			pLabel->getImageSize(iw,it);
			//pLabel->setPos(CMENU_SIZE_X - iw, (BUTTON_SIZE_Y - it) / 2);
			pLabel->setPos(pButton->getWide() - iw - XRES(PANEL_INNER_OFFSET), (pButton->getTall() - it) / 2);// XDM3037

			// Create the mouse off signal for the Label too
			if (!pButton->m_bNoHighlight)
				pLabel->addInputSignal(new CHandler_CommandButtonHighlight(pButton));
		}
		else
			DBG_PRINTF("CreateSubMenu(): error creating CImageLabel!\n");
	}
	else
		DBG_PRINTF("CreateSubMenu(): error creating CCommandMenu!\n");

	return pMenu;
}

//-----------------------------------------------------------------------------
// Purpose: IsCommandMenuOpened
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewport::IsCommandMenuOpened(int menuindex)//CCommandMenu *pMenu)
{
	if (m_pCommandMenus[menuindex])
		return m_pCommandMenus[menuindex]->IsOpened();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide ServerBrowser
//-----------------------------------------------------------------------------
void CViewport::ToggleServerBrowser(void)
{
	if (!m_iInitialized)
		return;
	if (!m_pServerBrowser)
		return;

	if (IsMenuPanelOpened(m_pServerBrowser))
		m_pServerBrowser->setVisible(false);
	else
		m_pServerBrowser->setVisible(true);

	UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide MusicPlayer
//-----------------------------------------------------------------------------
void CViewport::ToggleMusicPlayer(void)// XDM
{
	if (!m_iInitialized)
		return;
	if (!m_pMusicPlayer)
		return;

	if (IsMenuPanelOpened(m_pMusicPlayer))
		CloseMenuPanel(m_pMusicPlayer);
	else
		ShowMenuPanel(m_pMusicPlayer, false);

	UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Command menu, a real one. NOT A WINDOW.
// Input  : menuIndex - m_pCommandMenus[]
//			bParseUserCmdArgs - may be called by console command, parse menu items to quickly activate them
//-----------------------------------------------------------------------------
void CViewport::ShowCommandMenu(int menuIndex, bool bParseUserCmdArgs)
{
	DBG_PRINTF("CViewport::ShowCommandMenu(%d)\n", menuIndex);
	if (!m_iInitialized)
		return;

	if (menuIndex > MAX_MENUS || m_pCommandMenus[menuIndex] == NULL)// no such menu index
	{
		conprintf(1, "CViewport::ShowCommandMenu(%d): bad index!\n", menuIndex);
		return;
	}
	ShowCommandMenu(m_pCommandMenus[menuIndex], bParseUserCmdArgs);
}

//-----------------------------------------------------------------------------
// Purpose: Command menu, a real one. NOT A WINDOW.
// Input  : pNewMenu - a new menu
//			bParseUserCmdArgs - may be called by console command, parse menu items to quickly activate them
//-----------------------------------------------------------------------------
void CViewport::ShowCommandMenu(CCommandMenu *pNewMenu, bool bParseUserCmdArgs)
{
	DBG_PRINTF("CViewport::ShowCommandMenu()\n");
	if (!m_iInitialized)
		return;
	if (!pNewMenu)
		return;

	if (m_pCurrentMenuPanel)// there's a window
	{
		if (!m_pCurrentMenuPanel->AllowConcurrentWindows())// XDM3038: current window does not allow anything else to be shown
			return;
	}

	if (m_pCurrentCommandMenu == pNewMenu)// is the command menu already open?
	{
		m_pCurrentCommandMenu->Close();
		// XDM3038: ???		HideCommandMenu();
		return;
	}

	// Recalculate visible menus
	pNewMenu->UpdateOnPlayerInfo(CLIENT_INDEX_INVALID);// XDM3038c: required for player menu
	pNewMenu->Update();// XDM3038c: was UpdateCommandMenu(menuIndex);
	// XDM3038: ???	HideVGUIMenu();

	SetCurrentCommandMenu(pNewMenu);
	m_flMenuOpenTime = gEngfuncs.GetClientTime();

	// get command menu parameters
	if (bParseUserCmdArgs)
	{
		for (int i = 2; i < CMD_ARGC(); ++i)
		{
			const char *param = CMD_ARGV(i-1);
			if (param)
			{
				if (m_pCurrentCommandMenu->KeyInput(1, param[0], "") == 0)
					HideCommandMenu();// kill the menu open time, since the key input is final
			}
		}
	}
	if (m_pCurrentCommandMenu->IsOpened())// XDM3038: may be closed by keys above
		UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Handles the key input of "-commandmenu"
//-----------------------------------------------------------------------------
void CViewport::InputSignalHideCommandMenu(void)
{
	if (!m_iInitialized)
		return;

	// if they've just tapped the command menu key, leave it open
	if ((m_flMenuOpenTime + 0.3) > gHUD.m_flTime)
		return;

	HideCommandMenu();
}

//-----------------------------------------------------------------------------
// Purpose: Hides all command menus
//-----------------------------------------------------------------------------
void CViewport::HideCommandMenu(void)
{
	if (!m_iInitialized)
		return;

	for (uint32 menuIndex = 0; menuIndex<MAX_MENUS; ++menuIndex)
	{
		if (m_pCommandMenus[menuIndex])
			m_pCommandMenus[menuIndex]->Close();
	}
	if (m_pCurrentCommandMenu)// not required to be in array right now
		m_pCurrentCommandMenu->Close();

	m_flMenuOpenTime = 0.0f;
	//SetCurrentCommandMenu(NULL);
	//UpdateCursorState();
}

//-----------------------------------------------------------------------------
// Purpose: Bring up the scoreboard (from key binding)
//-----------------------------------------------------------------------------
CMenuPanel *CViewport::ShowScoreBoard(void)
{
	return ShowMenuPanel(m_pScoreBoard, true);
}

//-----------------------------------------------------------------------------
// Purpose: Hide the scoreboard (from key binding)
//-----------------------------------------------------------------------------
void CViewport::HideScoreBoard(void)
{
	CloseMenuPanel(m_pScoreBoard);
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the scoreboard is up
//-----------------------------------------------------------------------------
bool CViewport::IsScoreBoardVisible(void)
{
	return IsMenuPanelOpened(m_pScoreBoard);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMenu - 
//-----------------------------------------------------------------------------
void CViewport::OnCommandMenuClosed(CCommandMenu *pMenu)
{
	if (pMenu)
	{
		if (m_pCurrentCommandMenu == pMenu)
			SetCurrentCommandMenu(NULL);

		UpdateCursorState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: SetCurrentCommandMenu
// Set the submenu of the Command Menu
// Old comment: Unfortunately, this is called for the root menu and for all submenus
// Input  : *pNewMenu - can be NULL!
//-----------------------------------------------------------------------------
void CViewport::SetCurrentCommandMenu(CCommandMenu *pNewMenu)
{
	//DBG_PRINT_GUI("CViewport::SetCurrentCommandMenu(%d)\n", menuIndex);
	for (int i = 0; i < m_iNumMenus; ++i)
	{
		if (m_pCommandMenus[i] && m_pCommandMenus[i] != pNewMenu)// && m_pCommandMenus[i] != pNewMenu->GetParentMenu())
			m_pCommandMenus[i]->setVisible(false);
	}
	m_pCurrentCommandMenu = pNewMenu;

	if (m_pCurrentCommandMenu)// can be NULL
	{
		CCommandMenu *pMenu = m_pCurrentCommandMenu->GetParentMenu();
		while (pMenu)// XDM3038a: restore visibility of all parent menus, don't call Open() for them
		{
			pMenu->setVisible(true);
			pMenu = pMenu->GetParentMenu();
		}
		if (!m_pCurrentCommandMenu->IsOpened())
			m_pCurrentCommandMenu->Open();
	}
}

//-----------------------------------------------------------------------------
// Purpose: UpdateCommandMenu
// Input  : menuIndex - 
//-----------------------------------------------------------------------------
/*void CViewport::UpdateCommandMenu(int menuIndex)
{
	//DBG_PRINT_GUI("CViewport::UpdateCommandMenu(%d)\n", menuIndex);
	if (menuIndex < 0 || menuIndex >= MAX_MENUS)
		return;

	if (m_pCommandMenus[menuIndex])
	{
		if (menuIndex == m_PlayerMenu)// HL20130901 hack
			UpdatePlayerMenu(m_PlayerMenu);

		m_pCommandMenus[menuIndex]->RecalculateVisibles(0, false);
		m_pCommandMenus[menuIndex]->RecalculatePositions(0);
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: Update Spectator Panel
//-----------------------------------------------------------------------------
void CViewport::UpdateSpectatorPanel(void)
{
	m_iUser1 = g_iUser1;
	m_iUser2 = g_iUser2;
	m_iUser3 = g_iUser3;

	if (!m_pSpectatorPanel)
		return;

	if (gHUD.IsSpectator() && gHUD.m_pCvarDraw->value && !gHUD.m_iIntermission)	// don't draw in dev_overview mode
	{
		// check if spectator combinations are still valid
		gHUD.m_Spectator.CheckSettings();

		if (!IsMenuPanelOpened(m_pSpectatorPanel))
		{
			m_pSpectatorPanel->Open();//setVisible(true);	// show spectator panel, but
			m_pSpectatorPanel->ShowMenu(false);	// dsiable all menus/buttons
			// done inside UpdateCursorState();
		}
		/*gViewPort->*/m_pSpectatorPanel->Update();
	}
	else
	{
		if (IsMenuPanelOpened(m_pSpectatorPanel))
		{
			m_pSpectatorPanel->setVisible(false);
			m_pSpectatorPanel->ShowMenu(false);// dsiable all menus/buttons
		}
	}

	m_flSpectatorPanelLastUpdated = gEngfuncs.GetClientTime() + 1.0f; // update every seconds
}

//-----------------------------------------------------------------------------
// Purpose: Create Score Board
//-----------------------------------------------------------------------------
void CViewport::CreateScoreBoard(void)
{
	int xdent = SBOARD_INDENT_X, ydent = SBOARD_INDENT_Y;
	if (ScreenWidth <= 400)
	{
		xdent = SBOARD_INDENT_X_400; 
		ydent = SBOARD_INDENT_Y_400;
	}
	else if (ScreenWidth <= 512)
	{
		xdent = SBOARD_INDENT_X_512; 
		ydent = SBOARD_INDENT_Y_512;
	}

	m_pScoreBoard = new ScorePanel(xdent, ydent, ScreenWidth - xdent - xdent, ScreenHeight - ydent - ydent);
	if (m_pScoreBoard)
	{
		m_pScoreBoard->setParent(this);
		m_pScoreBoard->setVisible(false);
	}
}

#define SZTEMP_LENGTH		128
//-----------------------------------------------------------------------------
// Purpose: GenerateFullIntroText into a static buffer
// WARNING: Always check for overflows!!! If memory overflows, it corrupts g_VoiceStatus. Test on large .txt files (>12K)
// Input  : bShowIntro - server intro (rules+flags)
//			bShowMOTD - server MOTD
//			bShowMapText - mapname.txt
// Output : const char * - temporary buffer
//-----------------------------------------------------------------------------
const char *CViewport::GenerateFullIntroText(bool bShowIntro, bool bShowMOTD, bool bShowMapText)
{
	static char cText[MAX_INTRO_TEXT_LENGTH];// XDM: !!!

	if (!bShowIntro && !bShowMOTD && !bShowMapText)// return previously generated text
		return cText;

	char szSeparator[MAX_TEXTSEPARATOR_LENGTH] = "\0\0\0\0";
	char szTemp[SZTEMP_LENGTH] = "\0\0\0\0";
	size_t offset = 0;
	int wlen = 0;
	bool bAddSeparator = false;
	cText[0] = '\0';
	//memset(cText, 0, sizeof(char)*MAX_INTRO_TEXT_LENGTH);
	// WARNING: you can NOT use multiple BufferedLocaliseTextString() in single call or on a single string!
	LocaliseTextString("#TextSeparatorLine", szSeparator, MAX_TEXTSEPARATOR_LENGTH);
	//szSeparator[MAX_TEXTSEPARATOR_LENGTH-1] = '\0';
	if (bShowMOTD && GetMOTD() && GetMOTD()[0])
	{
		wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "%s\n", GetMOTD());
		ASSERT(wlen > 0);
		cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
		offset += wlen;
		//strncat(cText, GetMOTD(), MAX_INTRO_TEXT_LENGTH);// strncat deals with null-terminators
		//strncat(cText, "\n\0", 2);
		bAddSeparator = true;
		DBG_PRINT_GUI(" - GenerateFullIntroText(): after bShowMOTD offset = %u\n", offset);
	}
	if (bShowIntro)
	{
		if (gEngfuncs.GetMaxClients() > 1 && gHUD.m_iGameType > GT_SINGLE)// XDM3037a: only in multiplayer
		{
			bool bWritten = false;
			// UNDONE: check each _snprintf return value to not to be negative
			if (gHUD.m_iScoreLimit > 0)
			{
				LocaliseTextString("#Score_limit", szTemp, SZTEMP_LENGTH);// temporary
				szTemp[SZTEMP_LENGTH-1] = '\0';
				wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "%s: %u\n", szTemp, gHUD.m_iScoreLimit);
				cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
				ASSERT(wlen > 0);
				offset += wlen;
				ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
				bWritten = true;
			}
			if (gHUD.m_iTimeLimit > 0)
			{
				LocaliseTextString("#Time_limit", szTemp, SZTEMP_LENGTH);// temporary
				szTemp[SZTEMP_LENGTH-1] = '\0';
				wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "%s: %g\n", szTemp, (float)gHUD.m_iTimeLimit/60.0f);// XDM3038: UNDONE: float or integer after all?
				cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
				ASSERT(wlen > 0);
				offset += wlen;
				ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
				bWritten = true;
			}
			if (gHUD.m_iDeathLimit > 0)
			{
				LocaliseTextString("#Death_limit", szTemp, SZTEMP_LENGTH);// temporary
				szTemp[SZTEMP_LENGTH-1] = '\0';
				wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "%s: %u\n", szTemp, gHUD.m_iDeathLimit);
				cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
				ASSERT(wlen > 0);
				offset += wlen;
				ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
				bWritten = true;
			}
			if (bWritten)
			{
				wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "\n");
				cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
				ASSERT(wlen > 0);
				offset += wlen;
				ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
			}
			DBG_PRINT_GUI(" - GenerateFullIntroText(): after bShowIntro offset = %u\n", offset);
		}
		/*strncat(cText, szSeparator, MAX_INTRO_TEXT_LENGTH);
		strncat(cText, "\n\n\0", 3);
		strncat(cText, GetGameRulesIntroText(gHUD.m_iGameType, gHUD.m_iGameMode), MAX_INTRO_TEXT_LENGTH);
		strncat(cText, "\n\0", 2);
		strncat(cText, szSeparator, MAX_INTRO_TEXT_LENGTH);
		strncat(cText, "\n\n\0", 3);
		strncat(cText, BufferedLocaliseTextString("#Game_sv_flags"), MAX_INTRO_TEXT_LENGTH);*/
		LocaliseTextString("#Game_sv_flags", szTemp, SZTEMP_LENGTH);// temporary
		/*wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH, "%s\n%s\n\n%s\n%s\n\n%s\n", GetMOTD(), szSeparator,
							GetGameRulesIntroText(gHUD.m_iGameType, gHUD.m_iGameMode), szSeparator,
							szTemp);*/

		if (bAddSeparator)
		{
			wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "\n%s", szSeparator);
			ASSERT(wlen > 0);
			offset += wlen;
		}

		szTemp[SZTEMP_LENGTH-1] = '\0';
		wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "\n%s\n%s\n\n%s\n", GetGameRulesIntroText(gHUD.m_iGameType, gHUD.m_iGameMode), szSeparator, szTemp);
		cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
		ASSERT(wlen > 0);
		offset += wlen;
		ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
		// XDM3037: server flags info
		for (uint32 sfbit = 0; sfbit < (CHAR_BIT*sizeof(gHUD.m_iGameFlags)); ++sfbit)
		{
			if ((gHUD.m_iGameFlags & (1<<sfbit)) != 0)// GAME_FLAG_NOSHOOTING, etc
			{
				_snprintf(szTemp, SZTEMP_LENGTH, "#GAMEFLAG%u\0", sfbit);
				szTemp[SZTEMP_LENGTH-1] = '\0';
				wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, " - %s\n", BufferedLocaliseTextString(szTemp));
				ASSERT(wlen > 0);
				offset += wlen;
			}
		}
		ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
		//strncat(cText, "\n\0", 2);
		bAddSeparator = true;
		DBG_PRINT_GUI(" - GenerateFullIntroText(): after bShowIntro offset = %u\n", offset);
	}
	if (bShowMapText)
	{
		char szFileName[_MAX_FNAME];
		// Get the current mapname, and open it's map briefing text
		_snprintf(szFileName, _MAX_FNAME, "maps/%s.txt", GetMapName(true));
		szFileName[_MAX_FNAME-1] = '\0';
		FILE *pFile = LoadFile(szFileName, "rt");
		if (pFile)
		{
			if (bAddSeparator)
			{
				wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "\n%s", szSeparator);
				ASSERT(wlen > 0);
				offset += wlen;
			}
			wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "%s\n\n", BufferedLocaliseTextString("#Title_mapinfo"));
			ASSERT(wlen > 0);
			offset += wlen;
			//DBG_PRINT_GUI(" - GenerateFullIntroText(): bShowMapText (1) offset = %u\n", offset);
			/*size_t blocksread;
			while (!feof(pFile) && (MAX_INTRO_TEXT_LENGTH-offset > SZTEMP_LENGTH))// remaining space is > read buffer size
			{
				blocksread = fread(szTemp, sizeof(char), SZTEMP_LENGTH-1, pFile);
				szTemp[SZTEMP_LENGTH-1] = '\0';
				if (ferror(pFile))
				{
					//strncat(cText, "Read error!\n", MAX_MAPDESCRIPTION_LENGTH);
					wlen = _snprintf(cText+offset, MAX_INTRO_TEXT_LENGTH-offset, "\nRead error!\n");
					ASSERT(wlen > 0);
					offset += wlen;
					DBG_PRINT_GUI(" - GenerateFullIntroText(): bShowMapText (2) offset = %u\n", offset);
					conprintf(1, "CViewport::GenerateFullIntroText() Warning: error %d encountered while reading \"%s\"!\n", ferror(pFile), szFileName);
					break;
				}
				//strncat(cText, szTemp, blocksread);
				//offset += blocksread;
				//ASSERT(offset < MAX_INTRO_TEXT_LENGTH-1);
				wlen = _snprintf(cText+offset, blocksread, "%s", szTemp);// if +1, then '\0' is appended
				ASSERT(wlen > 0);
				offset += wlen;
				DBG_PRINT_GUI(" - GenerateFullIntroText(): bShowMapText (3) offset = %u\n", offset);
			}*/
			size_t readsize = MAX_INTRO_TEXT_LENGTH-offset-1;
			offset += fread(cText+offset, sizeof(char), readsize, pFile);
			ASSERT(offset < MAX_INTRO_TEXT_LENGTH);
			if (offset < MAX_INTRO_TEXT_LENGTH)
				cText[offset] = '\0';
			else
				cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';

			if (!feof(pFile))
				conprintf(1, "CViewport::GenerateFullIntroText() Warning: \"%s\" is larger than buffer can hold!\n", szFileName);

			if (ferror(pFile))
				conprintf(1, "CViewport::GenerateFullIntroText() Warning: error %d encountered while reading \"%s\"!\n", ferror(pFile), szFileName);

			fclose(pFile);
		}
		else
		{
			//strncat(cText+offset, "\n", MAX_INTRO_TEXT_LENGTH);
			strncat(cText+offset, BufferedLocaliseTextString("\n#Map_DescNA"), MAX_INTRO_TEXT_LENGTH-offset);
		}
		//DBG_PRINT_GUI(" - GenerateFullIntroText(): after bShowMapText offset = %u\n", offset);
	}
	ASSERT(offset < MAX_INTRO_TEXT_LENGTH);
	// neigh strncat(cText+offset, "\n\0", 2);
	if (offset < MAX_INTRO_TEXT_LENGTH)
		cText[offset] = '\0';
	else
		cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';

	return cText;
}


static const int SZ_LEN = 256;
//-----------------------------------------------------------------------------
// Purpose: Create Text Window
// WARNING: Always check for overflows!!!
// Input  : iTextToShow - SHOW_MAPBRIEFING
// Output : CMenuPanel *
//-----------------------------------------------------------------------------
CMenuPanel *CViewport::CreateTextWindow(int iTextToShow)
{
	DBG_PRINT_GUI("CViewport::CreateTextWindow(%d)\n", iTextToShow);
	char cTitle[MAX_TITLE_LENGTH] = "\0\0\0\0";
	char cText[MAX_INTRO_TEXT_LENGTH] = "\0\0\0\0";// XDM: !!!
	bool fullscreen = true;

	if (iTextToShow == SHOW_MAPBRIEFING || iTextToShow == SHOW_MOTD)// XDM
	{
		if (iTextToShow == SHOW_MOTD)// XDM: append text from file to the MOTD
		{
			_snprintf(cTitle, MAX_TITLE_LENGTH, "%s - %s %s\0", m_szServerName[0]?m_szServerName:"XHL Server", GetMapName(), GetGameDescription(gHUD.m_iGameType));
			strncpy(cText, GenerateFullIntroText(true, true, true), MAX_INTRO_TEXT_LENGTH);
		}
		else// SHOW_MAPBRIEFING
		{
			strncpy(cText, GenerateFullIntroText(false, false, true), MAX_INTRO_TEXT_LENGTH);
			_snprintf(cTitle, MAX_TITLE_LENGTH, "%s %s\0", GetMapName(), GetGameDescription(gHUD.m_iGameType));
		}
	}
	else if (iTextToShow == SHOW_SPECHELP)
	{
		LocaliseTextString("#Spec_Help_Title", cTitle, MAX_TITLE_LENGTH);
		//cTitle[MAX_TITLE_LENGTH-1] = '\0';
		LocaliseTextString("#Spec_Help_Text", cText, MAX_INTRO_TEXT_LENGTH);
		//cText[MAX_INTRO_TEXT_LENGTH-1] = '\0';
		fullscreen = false;
	}
	// terminate all strings
	cText[MAX_INTRO_TEXT_LENGTH-1] = 0;
	cTitle[MAX_TITLE_LENGTH-1] = 0;

	if (IsMenuPanelOpened(m_pSpectatorPanel))// XDM
		fullscreen = false;

	// if we're in the game, flag the menu to be only grayed in the dialog box, instead of full screen // XDM3038: RemoveMe = TRUE
	CMenuPanel *pMessageWindowPanel = new CMessageWindowPanel(cText, cTitle, fullscreen, TRUE, -1, -1, XRES(MOTD_WINDOW_SIZE_X), YRES(MOTD_WINDOW_SIZE_Y));
	if (pMessageWindowPanel)
		pMessageWindowPanel->setParent(this);

	return pMessageWindowPanel;
}

//-----------------------------------------------------------------------------
// Purpose: IsMenuPanelOpened
// Warning: Must return false when menu is invalid
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewport::IsMenuPanelOpened(CMenuPanel *pMenuPanel)
{
	if (pMenuPanel)
	{
		if (pMenuPanel->isVisible())// && pMenuPanel->getParent() == this)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: FindMenu by ID
// Input  : iMenu - 
//-----------------------------------------------------------------------------
CMenuPanel *CViewport::FindMenuPanel(int iMenu)
{
	CMenuPanel *pMenu = m_pCurrentMenuPanel;
	while (pMenu != NULL)
	{
		if (pMenu->GetMenuID() == iMenu)// found existing menu
			break;//return pMenu;

		pMenu = pMenu->GetNextMenu();
	}
	return pMenu;
}

//-----------------------------------------------------------------------------
// Purpose: ShowMenuPanel by ID
// Input  : iMenu - 
//-----------------------------------------------------------------------------
CMenuPanel *CViewport::ShowMenuPanel(int iMenu)
{
	DBG_PRINT_GUI("CViewport::ShowMenuPanel(%d)\n", iMenu);
	CMenuPanel *pNewMenu = NULL;

	if (gEngfuncs.pDemoAPI->IsPlayingback())// Don't open menus during demo playback
		return pNewMenu;

	//if ( gHUD.m_iIntermission && iMenu != MENU_INTRO )// XDM3037a: we need all menus sent from the server!
	//	return pNewMenu;

	// Don't create one if it's already in the list
	CMenuPanel *pMenu = FindMenuPanel(iMenu);
	if (pMenu)
		return pMenu;

	bool bForceOnTop = false;
	switch (iMenu)
	{
	case MENU_CLOSEALL:
		{
			DBG_PRINT_GUI("ShowVGUIMenu(): MENU_CLOSEALL\n");
			HideCommandMenu();// XDM3038
			HideVGUIMenu();// XDM3038
			return NULL;
		}
		break;
	case MENU_TEAM:
		pNewMenu = m_pTeamMenuPanel;// XDM3038
		bForceOnTop = true;
		break;

	case MENU_MAPBRIEFING:
		pNewMenu = CreateTextWindow(SHOW_MAPBRIEFING);
		break;

	case MENU_INTRO:
		pNewMenu = CreateTextWindow(SHOW_MOTD);
		break;

	case MENU_SPECHELP:
		pNewMenu = CreateTextWindow(SHOW_SPECHELP);
		break;

	case MENU_SCOREBOARD:
		pNewMenu = m_pScoreBoard;// XDM3038
		bForceOnTop = true;
		break;

	case MENU_CONSOLE:
		CLIENT_COMMAND("toggleconsole");// lol
		//pNewMenu = new ConsolePanel(0,0, getWide(), getTall()/2);
		break;

	//case MENU_: STUB
	//	pNewMenu = new ControlConfigPanel(0,0, getWide(), getTall()/2);
	//	break;
	//case MENU_ENTITYCREATE:
	//	pNewMenu = new CEntityEntryPanel(ScreenWidth/2 - EEP_WIDTH/2, ScreenHeight/2 - EEP_HEIGHT/2, EEP_WIDTH, EEP_HEIGHT);
	//	break;
/*#if defined (_DEBUG)// too risky
	case MENU_VOICETWEAK:
		pNewMenu = GetVoiceTweakDlg();//CVoiceVGUITweakDlg();
		break;
#endif*/
	default:
		conprintf(0, "ShowVGUIMenu(): Cannot open menu %d!\n", iMenu);
		break;
	}

	if (pNewMenu)
	{
		pNewMenu->SetMenuID(iMenu);
		return ShowMenuPanel(pNewMenu, bForceOnTop);
	}
	return pNewMenu;
}

//-----------------------------------------------------------------------------
// Purpose: ShowMenuPanel directly
// Input  : *pMenu - 
// Input  : bForceOnTop - don't queue, show now
// Output : CMenuPanel * == pNewMenu on success, == NULL on failure, == menu after which pNewMenu was queued
//-----------------------------------------------------------------------------
CMenuPanel *CViewport::ShowMenuPanel(CMenuPanel *pNewMenu, bool bForceOnTop)
{
	if (pNewMenu == NULL)
		return NULL;

	DBG_PRINT_GUI("CViewport::ShowMenuPanel(%d, %s)\n", pNewMenu->GetMenuID(), bForceOnTop?"true":"false");

/* decision criteria:
	IsPersistent()
	AllowConcurrentWindows()
	IsCapturingInput()
	IsShowingCursor()
*/
	if (m_pCurrentMenuPanel && m_pCurrentMenuPanel != pNewMenu)
	{
		DBG_PRINT_GUI("ShowMenuPanel(): 1 different CURRENT menu exists\n");
		if (m_pCurrentMenuPanel->IsPersistent())// cannot close current window
		{
			DBG_PRINT_GUI("ShowMenuPanel(): 2 CURRENT menu IS persistent\n");
			if (m_pCurrentMenuPanel->AllowConcurrentWindows())// this window allows other to be opened
			{
				if (m_pCurrentMenuPanel->IsCapturingInput())// user is using this window
				{
					m_pCurrentMenuPanel->SetNextMenu(pNewMenu);
					DBG_PRINT_GUI("ShowMenuPanel(): 3 CURRENT menu is capturing input, NEW queued\n");
					return m_pCurrentMenuPanel;
				}
				else
				{
					pNewMenu->SetNextMenu(m_pCurrentMenuPanel);// bring it back after
					m_pCurrentMenuPanel = NULL;// show NEW menu
					DBG_PRINT_GUI("ShowMenuPanel(): 3 CURRENT menu queued after the NEW menu\n");
				}
			}
			else // current menu is demanding all input
			{
				DBG_PRINT_GUI("ShowMenuPanel(): 4 CURRENT menu does NOT allow other menus\n");
				if (bForceOnTop || pNewMenu->IsPersistent())// ASKING for it
				{
					m_pCurrentMenuPanel->SetNextMenu(pNewMenu);
					DBG_PRINT_GUI("ShowMenuPanel(): 5 NEW menu queued after CURRENT menu\n");
					return m_pCurrentMenuPanel;
				}
				else// nope, occupied, sorry.
				{
					DBG_PRINT_GUI("ShowMenuPanel(): 6 NEW menu denied\n");
					return NULL;
				}
			}
		}
		else// current window can be closed
		{
			DBG_PRINT_GUI("ShowMenuPanel(): 7 CURRENT menu is NOT persistent\n");
			if (m_pCurrentMenuPanel->AllowConcurrentWindows())// this window allows other to be opened, no need to close it
			{
				pNewMenu->SetNextMenu(m_pCurrentMenuPanel);// bring it back after
				m_pCurrentMenuPanel = NULL;// show NEW menu
				DBG_PRINT_GUI("ShowMenuPanel(): 8 CURRENT menu queued after the NEW menu\n");
			}
			else// this window doesnt's share input, but is not persistent
			{
				DBG_PRINT_GUI("ShowMenuPanel(): 9 CURRENT menu does NOT allow other menus\n");
				if (!CloseMenuPanel(m_pCurrentMenuPanel))// new window has priority over the old one
				{
					DBG_PRINT_GUI("ShowMenuPanel(): 10 CURRENT menu fails to close!\n");
					if (bForceOnTop || pNewMenu->IsPersistent())// ASKING for it
					{
						pNewMenu->SetNextMenu(m_pCurrentMenuPanel);// queue CURRENT to be shown again after NEW closes
						m_pCurrentMenuPanel = NULL;// show NEW menu
						DBG_PRINT_GUI("ShowMenuPanel(): 11 CURRENT menu queued after the NEW menu\n");
					}
					else// but we still may fail
					{
						DBG_PRINT_GUI("ShowMenuPanel(): 12 CURRENT menu fails to close and blocks NEW menu!\n");
						return NULL;
					}
				}
				//else// ok m_pCurrentMenuPanel = pNewMenu;
			}
		}
		ASSERT(m_pCurrentMenuPanel == NULL);
	}
	DBG_PRINT_GUI("ShowMenuPanel(): opening NEW menu\n");

	// Close the Command Menu if it's open
	HideCommandMenu();

	//pNewMenu->SetMenuID(iMenu);
	SetCurrentMenu(pNewMenu);
	m_pCurrentMenuPanel->Open();
	/*m_pCurrentMenuPanel = pNewMenu;
	m_pCurrentMenuPanel->SetActive(true);
	m_pCurrentMenuPanel->setParent(this);
	m_pCurrentMenuPanel->Open();*/
	UpdateCursorState();
	return m_pCurrentMenuPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Set m_pCurrentMenuPanel to a new CMenuPanel
// Input  : *pMenu - 
//-----------------------------------------------------------------------------
void CViewport::SetCurrentMenu(CMenuPanel *pMenu)
{
	m_pCurrentMenuPanel = pMenu;
	if (m_pCurrentMenuPanel)
	{
		DBG_PRINT_GUI("CViewport::SetCurrentMenu(%d)\n", pMenu->GetMenuID());
		// Don't open menus in demo playback
		if (gEngfuncs.pDemoAPI->IsPlayingback())
			return;

		m_pCurrentMenuPanel->SetActive(true);// XDM3038
		m_pCurrentMenuPanel->setParent(this);// XDM3038
		//m_pCurrentMenuPanel->Open(); no..?
		UpdateCursorState();// XDM3038
	}
	else
	{
		gEngfuncs.pfnClientCmd("closemenus;");// HL20130901 ?
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all VGUI Menu's onscreen
//-----------------------------------------------------------------------------
void CViewport::HideVGUIMenu(void)
{
	DBG_PRINT_GUI("CViewport::HideVGUIMenu()\n");
	size_t iLoopCounter = 0;
	while (m_pCurrentMenuPanel)
	{
		CloseMenuPanel(m_pCurrentMenuPanel);
		++iLoopCounter;
		//if (!ASSERTSZ(iLoopCounter < 256, "Possible infinite loop detected in HideVGUIMenu()!\n"))
		if (iLoopCounter >= 256)
		{
			conprintf(1, "WARNING: Possible infinite loop detected in HideVGUIMenu()!\n");
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: CloseMenu
// Input  : *pMenuPanel - 
// Output : Returns true on success, false on failure (menu left open).
//-----------------------------------------------------------------------------
bool CViewport::CloseMenuPanel(CMenuPanel *pMenuPanel)
{
	if (pMenuPanel)
	{
		DBG_PRINT_GUI("CViewport::CloseMenuPanel(%d)\n", pMenuPanel->GetMenuID());
		if (IsMenuPanelOpened(pMenuPanel))
		{
			pMenuPanel->Close();
			//if (m_pCurrentMenuPanel == pMenuPanel)
			//	m_pCurrentMenuPanel = NULL;// DONE in OnMenuPanelClose

			if (pMenuPanel->isVisible())
				return false;
			/* not always true
			int n = getChildCount();
			for (int i=0; i<n; ++i)// right thing to do
			{
				if (getChild(i) == pMenuPanel)// was not removed
					return false;
			}*/
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: last update: menu tells us it is closing
// Input  : *pMenu - 
//-----------------------------------------------------------------------------
void CViewport::OnMenuPanelClose(CMenuPanel *pMenu)
{
	if (pMenu)
	{
		DBG_PRINT_GUI("CViewport::OnMenuPanelClose(%d)\n", pMenu->GetMenuID());
		if (m_pCurrentMenuPanel == pMenu)
		{
			m_pCurrentMenuPanel = NULL;
			SetCurrentMenu(pMenu->GetNextMenu());
			pMenu->SetNextMenu(NULL);// clear it now
		}
		if (pMenu->ShouldBeRemoved())
		{
			removeChild(pMenu);
			// who can free this?	delete pMenu;
		}
		UpdateCursorState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: Loading starts
// Input  : active - 1/0
//-----------------------------------------------------------------------------
void CViewport::OnGameActivated(const int &active)
{
	if (active == 0)
		HideVGUIMenu();
}

//-----------------------------------------------------------------------------
// Purpose: Remove the top VGUI menu, and bring up the next one
//-----------------------------------------------------------------------------
void CViewport::HideTopMenu(void)
{
	DBG_PRINT_GUI("CViewport::HideTopMenu()\n");
	CloseMenuPanel(m_pCurrentMenuPanel);
	//UpdateCursorState(); <- OnMenuPanelClose
}

//-----------------------------------------------------------------------------
// Purpose:  AllowedToPrintText
// Output : Returns true if the HUD's allowed to print text messages
//-----------------------------------------------------------------------------
bool CViewport::AllowedToPrintText(void)
{
	// Prevent text messages when fullscreen menus are up
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Bring up the Team selection Menu
// Output : CMenuPanel *
// Old hack
//-----------------------------------------------------------------------------
#if 0
CMenuPanel *CViewport::ShowTeamMenu(void)
{
	// Don't open menus in demo playback
	if (gEngfuncs.pDemoAPI->IsPlayingback())
		return NULL;

	//if (m_pTeamMenuPanel)
	//	m_pTeamMenuPanel->Reset();

	// XDM3037a: Create the panel every time?
	/*m_pTeamMenuPanel = new CTeamMenuPanel(true/remove when closed/, 0, 0, ScreenWidth, ScreenHeight);
	if (m_pTeamMenuPanel)
	{
		m_pTeamMenuPanel->setParent(this);
		m_pTeamMenuPanel->setVisible(false);
		return ShowMenuPanel(m_pTeamMenuPanel);
	}*/
	//return m_pTeamMenuPanel;
	return ShowMenuPanel(m_pTeamMenuPanel);// XDM3038
}
#endif

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Called every frame
//-----------------------------------------------------------------------------
void CViewport::StartFrame(const double &time)
{
	//if (!App::getInstance()->_running)// VALVe, SCREW YOU!!!!!!!!!
	if (IsMenuPanelOpened(m_pTeamMenuPanel))
		m_pTeamMenuPanel->Update();

	// BAD. Crashes when quitting the game. UpdateSpectatorPanel();
}

//-----------------------------------------------------------------------------
// Purpose: UPDATE HUD SECTIONS
// We've got an update on player info
// Recalculate any menus that use it.
// DON'T use this for score and other frequent updates!
//-----------------------------------------------------------------------------
void CViewport::UpdateOnPlayerInfo(void)
{
	GetAllPlayersInfo();
	for (uint32 menuIndex = 0; menuIndex<MAX_MENUS; ++menuIndex)
	{
		if (m_pCommandMenus[menuIndex] && m_pCommandMenus[menuIndex] != m_pCurrentCommandMenu)
			m_pCommandMenus[menuIndex]->UpdateOnPlayerInfo(CLIENT_INDEX_INVALID);
	}
	if (m_pCurrentCommandMenu)
		m_pCurrentCommandMenu->UpdateOnPlayerInfo(CLIENT_INDEX_INVALID);// this may not be in the array
	//UpdateCommandMenu(m_StandardMenu);
	//UpdateCommandMenu(m_PlayerMenu);

	if (m_pTeamMenuPanel)
		m_pTeamMenuPanel->Update();

	if (m_pScoreBoard)
		m_pScoreBoard->Update();
}

//-----------------------------------------------------------------------------
// Purpose: UpdateCursorState
//-----------------------------------------------------------------------------
void CViewport::UpdateCursorState(void)
{
	// Need cursor if any VGUI window is up
	if (IsMenuPanelOpened(m_pCurrentMenuPanel) && m_pCurrentMenuPanel->IsShowingCursor() ||
		IsMenuPanelOpened(m_pSpectatorPanel) && m_pSpectatorPanel->IsShowingCursor() ||
		IsMenuPanelOpened(m_pScoreBoard) && m_pScoreBoard->IsShowingCursor() ||// XDM3037a
		IsMenuPanelOpened(m_pTeamMenuPanel) && m_pTeamMenuPanel->IsShowingCursor() ||
		IsMenuPanelOpened(m_pServerBrowser) && m_pServerBrowser && m_pServerBrowser->IsShowingCursor() ||
		IsMenuPanelOpened(m_pMusicPlayer) && m_pMusicPlayer->IsShowingCursor() ||
		(m_pCurrentCommandMenu && (gHUD.m_pCvarStealMouse->value > 0)) ||// XDM3037a
		(g_iMouseManipulationMode > 0) ||// XDM3035a: test
		GetClientVoiceMgr()->IsInSquelchMode())// XDM
	{
		g_iVisibleMouse = true;
		App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_arrow));
		return;
	}

	// Don't reset mouse in demo playback
	if (!gEngfuncs.pDemoAPI->IsPlayingback())
	{
		IN_ResetMouse();
	}

	g_iVisibleMouse = false;
	App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_none));
}

//-----------------------------------------------------------------------------
// Purpose: UpdateHighlights
//-----------------------------------------------------------------------------
void CViewport::UpdateHighlights(void)
{
	if (m_pCurrentCommandMenu)
		m_pCurrentCommandMenu->Open();
}

//-----------------------------------------------------------------------------
// Purpose: Get MOTD text
// Output : char *
//-----------------------------------------------------------------------------
const char *CViewport::GetMOTD(void)
{
	//if (m_iGotAllMOTD)
		return m_szMOTD;
}

//-----------------------------------------------------------------------------
// Purpose: GetAllowSpectators
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewport::GetAllowSpectators(void)
{
	return (gHUD.m_iGameFlags & GAME_FLAG_ALLOW_SPECTATORS) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: draw "fullscreen" background for menus that require it
//-----------------------------------------------------------------------------
void CViewport::paintBackground(void)
{
	if (m_pCurrentMenuPanel)
	{
		if (m_pCurrentMenuPanel->getBackgroundMode() == BG_FILL_SCREEN)//> BG_FILL_NONE)// TODO: revisit this when fill bug is fixed
		{
			vgui::Color color;
			m_pCurrentMenuPanel->getBgColor(color);
			drawSetColor(color[0],color[1],color[2], color[3]);// m_pCurrentMenuPanel->m_iTransparency);
			drawFilledRect(0,0,_size[0],_size[1]);//ScreenWidth,ScreenHeight);
			//FillRGBA(0,0, ScreenWidth,ScreenHeight, color[0],color[1],color[2], color[3]);// this shit does not work
		}
	}

	if (m_pScoreBoard)
	{
		int x, y;
		getApp()->getCursorPos(x, y);
		m_pScoreBoard->cursorMoved(x, y, m_pScoreBoard);
	}

	// See if the command menu is visible and needs recalculating due to some external change
	if (gHUD.m_iTeamNumber != m_iCurrentTeamNumber)
	{
		m_pCommandMenus[m_StandardMenu]->Update();// XDM3038c: was UpdateCommandMenu(m_StandardMenu);
		m_iCurrentTeamNumber = gHUD.m_iTeamNumber;
	}

	// See if the Spectator Menu needs to be updated
	if ((g_iUser1 != m_iUser1 || g_iUser2 != m_iUser2) || (m_flSpectatorPanelLastUpdated < gHUD.m_flTime))
	{
		UpdateSpectatorPanel();
	}

	// Update the Scoreboard, if it's visible
	if (IsMenuPanelOpened(m_pScoreBoard) && (m_flScoreBoardLastUpdated < gHUD.m_flTime))
	{
		m_pScoreBoard->Update();
		m_flScoreBoardLastUpdated = gHUD.m_flTime + 0.5f;
	}

	if (IsMenuPanelOpened(m_pMusicPlayer))// XDM
		m_pMusicPlayer->Update();

	int extents[4];
	getAbsExtents(extents[0],extents[1],extents[2],extents[3]);
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
	VGui_ViewportPaintBackground(extents);
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		conprintf(0, "CViewport: VGui_ViewportPaintBackground() exception!\n");
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: FROM ENGINE: Direct Key Input
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 1 to allow engine to process the key
//-----------------------------------------------------------------------------
int CViewport::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (gEngfuncs.Con_IsVisible() == 1)// XDM3037a: TESTME
		return 1;

	int passnext = 1;
	// if we're in a command menu, try hit one of it's buttons
	if (passnext)
	{
		if (m_pCurrentCommandMenu)
		{
			if (down && (keynum == K_ESCAPE || keynum == K_SPACE))// XDM3037a: escape is used by Steam UI
			{
				HideCommandMenu();
				return 0;
			}
			else
				passnext = m_pCurrentCommandMenu->KeyInput(down, keynum, pszCurrentBinding);
		}
	}
	// check current active window
	if (passnext)
	{
		if (IsMenuPanelOpened(m_pCurrentMenuPanel))
		{
			if (m_pCurrentMenuPanel->IsCapturingInput())//hasFocus())
				passnext = m_pCurrentMenuPanel->KeyInput(down, keynum, pszCurrentBinding);
		}
	}
	// little hacks follow
	if (passnext)
	{
		if (IsMenuPanelOpened(m_pScoreBoard))// always exists
		{
			if (m_pScoreBoard->IsCapturingInput())
				passnext = m_pScoreBoard->KeyInput(down, keynum, pszCurrentBinding);
		}
	}
	if (passnext)
	{
		if (IsMenuPanelOpened(m_pTeamMenuPanel))
		{
			if (m_pTeamMenuPanel->IsCapturingInput())
				passnext = m_pTeamMenuPanel->KeyInput(down, keynum, pszCurrentBinding);
		}
	}
	if (passnext)
	{
		if (IsMenuPanelOpened(m_pServerBrowser))// this checks for NULL
		{
			if (m_pServerBrowser->IsCapturingInput())
				passnext = m_pServerBrowser->KeyInput(down, keynum, pszCurrentBinding);
		}
	}
	if (passnext)
	{
		if (IsMenuPanelOpened(m_pSpectatorPanel))
		{
			if (m_pSpectatorPanel->IsCapturingInput())
				passnext = m_pSpectatorPanel->KeyInput(down, keynum, pszCurrentBinding);
		}
	}

	/*for (int i=0; passnext && i<getChildCount(); ++i)// right thing to do
	{
		Panel *pPanel = getChild(i);
		if (IsMenuPanelOpened(pPanel))
		{
			char str[16];
			pPanel->getPersistanceText(str, 16);// HACK
			//conprintf(1, " VPPT: %s\n", str);
			//if (IsMenuPanel(pPanel))
			if (strcmp(str, "CMenuPanel") == 0)
			{
				passnext = ((CMenuPanel *)pPanel)->KeyInput(down, keynum, pszCurrentBinding);
			}
		}
	}*/
	return passnext;
}

//-----------------------------------------------------------------------------
// Purpose: DeathMsg incoming from the server
// Input  : killer - 
//			victim - 
//-----------------------------------------------------------------------------
void CViewport::DeathMsg(short killer, short victim)
{
	if (m_pScoreBoard)
		m_pScoreBoard->DeathMsg(killer, victim);
}

//-----------------------------------------------------------------------------
// Purpose: MOTD
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	if (m_iGotAllMOTD)
		m_szMOTD[0] = 0;

	BEGIN_READ(pbuf, iSize);
	m_iGotAllMOTD = READ_BYTE();
	int roomInArray = MAX_MOTD_LENGTH - strlen(m_szMOTD) - 1;
	strncat(m_szMOTD, READ_STRING(), roomInArray >= 0 ? roomInArray : 0);
	m_szMOTD[sizeof(m_szMOTD)-1] = '\0';
	END_READ();

	// don't show MOTD for HLTV spectators
	/* XDM3038a	if (m_iGotAllMOTD && !gEngfuncs.IsSpectateOnly() && !(IsTeamGame(gHUD.m_iGameType) && FindMenu(MENU_TEAM)))// XDM3038a: don't interrupt team menu
	{
		CMenuPanel *pPanel = ShowMenuPanel(MENU_INTRO);
		if (pPanel)
			pPanel->SetCloseDelay(3);// XDM3038a
	}*/
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: ServerName
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	strncpy(m_szServerName, READ_STRING(), MAX_SERVERNAME_LENGTH);
	END_READ();

	m_szServerName[MAX_SERVERNAME_LENGTH-1] = '\0';// XDM: overflow!!

	if (m_pScoreBoard)
		m_pScoreBoard->Update();//XDM3035c://UpdateTitle();

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: ScoreInfo
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	if (m_pScoreBoard)
		m_pScoreBoard->Update();

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Show GUI menu
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_ShowMenu(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int menu = READ_BYTE();
	byte flags = READ_BYTE();
	END_READ();

	CMenuPanel *pMenu;
	if (flags & MENUFLAG_CLOSE)// close it
	{
		pMenu = FindMenuPanel(menu);
		if (pMenu)
			CloseMenuPanel(pMenu);// pMenu->IsPersistent() will override this :-/
	}
	else// show it
	{
		pMenu = ShowMenuPanel(menu);
		if (pMenu)
		{
			if (flags & MENUFLAG_BG_FILL_NONE)
				pMenu->setBackgroundMode(BG_FILL_NONE);
			else if (flags & MENUFLAG_BG_FILL_SCREEN)
				pMenu->setBackgroundMode(BG_FILL_SCREEN);
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: "player is on team" notification
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	if (IsMenuPanelOpened(m_pScoreBoard))
		m_pScoreBoard->Update();// XDM3038a: avoid calling this too often!

	UpdateOnPlayerInfo();// XDM3038a
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Reset player's screen immediately (used by CHudSpectator)
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_ResetFade(const char *pszName, int iSize, void *pbuf)
{
	screenfade_t sf;
	gEngfuncs.pfnGetScreenFade(&sf);
	sf.fader = 0;
	sf.fadeg = 0;
	sf.fadeb = 0;
	sf.fadealpha = 0;
	sf.fadeEnd = 0.1;
	sf.fadeReset = 0.0;
	sf.fadeSpeed = 0.0;
	sf.fadeFlags = FFADE_IN;
	sf.fadeReset += gHUD.m_flTime;
	sf.fadeEnd += sf.fadeReset;
	gEngfuncs.pfnSetScreenFade(&sf);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Fade a player's screen out/in when they're spectating someone who is teleported
// Output : int
//-----------------------------------------------------------------------------
int CViewport::MsgFunc_SpecFade(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();
	if (g_iUser1 == OBS_IN_EYE)// we're in first-person spectator mode (...not first-person in the PIP)
	{
		if (g_iUser2 == iIndex)// this is the person we're watching
		{
			screenfade_t sf;
			int iFade = READ_BYTE();
			TEAM_ID iTeam = READ_BYTE();
			float flTime = ((float)READ_SHORT() / 100.0);
			sf.fadealpha = READ_BYTE();
			GetTeamColor(iTeam, sf.fader, sf.fadeg, sf.fadeb);
			gEngfuncs.pfnGetScreenFade(&sf);
			sf.fadeEnd = flTime;
			sf.fadeReset = 0.0;
			sf.fadeSpeed = 0.0;
			if (iFade == 1)// XDM3037: UNDONE? BUILD_TELEPORTER_FADE_OUT )
			{
				sf.fadeFlags = FFADE_OUT;
				sf.fadeReset = flTime;

				if (sf.fadeEnd)
					sf.fadeSpeed = -(float)sf.fadealpha / sf.fadeEnd;

				sf.fadeTotalEnd = sf.fadeEnd += gEngfuncs.GetClientTime();
				sf.fadeReset += sf.fadeEnd;
			}
			else
			{
				sf.fadeFlags = FFADE_IN;

				if (sf.fadeEnd)
					sf.fadeSpeed = (float)sf.fadealpha / sf.fadeEnd;

				sf.fadeReset += gEngfuncs.GetClientTime();
				sf.fadeEnd += sf.fadeReset;
			}
			gEngfuncs.pfnSetScreenFade(&sf);
		}
	}
	return 1;
}





//-----------------------------------------------------------------------------
// Purpose: InputSignal handler for the main viewport
// Everything you see here will be called ALWAYS disregarding which panels
// are currently shown e.g. if you click outside windows too.
//-----------------------------------------------------------------------------
void CViewPortInputHandler::mousePressed(MouseCode code, Panel *panel) 
{
	if (code != MOUSE_LEFT)
	{
		// send a message to close the command menu
		// this needs to be a message, since a direct call screws the timing
		CLIENT_COMMAND("ForceCloseCommandMenu\n");
	}
	else if (gViewPort)
	{
		gViewPort->GetScoreBoard()->SetClickMode(false);
		//if (IsMenuPanelOpened(gViewPort->GetScoreBoard())// XDM3037a
		//	gViewPort->GetScoreBoard()->mousePressed(code, panel);
	}
}

/*void CViewPortInputHandler::mouseDoublePressed(MouseCode code, Panel *panel)
{
	if (panel && panel != gViewPort && panel->isEnabled() && panel->isVisible())
		PlaySound("vgui/button_press.wav", VOL_NORM);
}*/
