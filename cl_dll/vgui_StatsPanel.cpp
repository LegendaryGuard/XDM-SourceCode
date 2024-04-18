#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "vgui_StatsPanel.h"
#include "keydefs.h"

// UNDONE
#if 0
//-----------------------------------------------------------------------------
// Purpose: Constructs a message panel
//-----------------------------------------------------------------------------
CStatsPanel::CStatsPanel(bool ShadeFullscreen, int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iRemoveMe, x, y, wide, tall)
{
	setCaptureInput(true);
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	ASSERT(pSchemes != NULL);
	// Schemes
	CScheme *pTitleScheme = pSchemes->getScheme("Title Font");
	CScheme *pTextScheme = pSchemes->getScheme("Briefing Text");
	//CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");

	//if (ShadeFullscreen)
	//	setBackgroundMode(BG_FILL_SCREEN);
	//else
		setBackgroundMode(BG_FILL_NORMAL);

	setBgColor(0,0,0, ShadeFullscreen ? 95:PANEL_DEFAULT_ALPHA);// reversed alpha!
	//setCaptureInput(true);
	setShowCursor(true);

	int iXPos = 0,iYPos = 0;
	int iXSize = wide,iYSize = tall;

	m_pLabelTitle = new Label(BufferedLocaliseTextString("#Stats_Title"), iXPos+XRES(STATSPANEL_BORDER), iYPos+YRES(STATSPANEL_BORDER));
	m_pLabelTitle->setParent(this);
	m_pLabelTitle->setFont(pTitleScheme->font);
	m_pLabelTitle->setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_pLabelTitle->setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_pLabelTitle->setContentAlignment(vgui::Label::a_west);
	//m_pLabelTitle->setText(szTitle);

	iYPos = (iYPos + iYSize)-YRES(STATSPANEL_BORDER)-BUTTON_SIZE_Y;
	m_pCheckButtonSave = new ToggleCommandButton(g_pCvarLogStats->value > 0, "@", iXPos+XRES(STATSPANEL_BORDER), iYPos, XRES(BUTTON_SIZE_X), YRES(BUTTON_SIZE_Y), false, false);
	//m_pCheckButtonSave->setFont(pButtonScheme->font);
	//m_pCheckButtonSave->setContentAlignment(Label::a_center);
	m_pCheckButtonSave->setParent(this);
	//m_pCheckButtonSave->m_bShowHotKey = false;
	m_pCheckButtonSave->setBoundKey('s');
	m_pCheckButtonSave->addActionSignal(new CMenuHandler_ToggleCvar(NULL, g_pCvarLogStats));

	m_pButtonClose = new CommandButton(BufferedLocaliseTextString("#Menu_OK"), (iXPos + iXSize)-XRES(STATSPANEL_BORDER)-BUTTON_SIZE_X, iYPos, BUTTON_SIZE_X, BUTTON_SIZE_Y);
	m_pButtonClose->setParent(this);
	m_pButtonClose->addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));

	// -------------------
	// Set the width of the last column to be the remaining space.
	iYPos = m_pLabelTitle->getTall()+YRES(STATSPANEL_BORDER*2);

	m_DataList.setBounds(STATSPANEL_BORDER, iYPos, wide - XRES(STATSPANEL_BORDER*2), m_pButtonClose->GetY() - iYPos - YRES(STATSPANEL_BORDER));
	m_DataList.setParent(this);
	m_DataList.setBgColor(0,0,0,255);
	//m_DataList.setBgColor(191,255,255,127);// TEST
	m_DataList.setPaintBackgroundEnabled(false);

	char sz[128];
	for (size_t row=0; row < STATSPANEL_NUM_ROWS; ++row)
	{
		CGrid *pGridRow = &m_DataGrids[col];
		pGridRow->SetDimensions(STATSPANEL_NUM_COLUMNS, 1);
		pGridRow->SetSpacing(1, 0);
		pGridRow->setBgColor(0,0,0,255);
		for(int col=0; col < STATSPANEL_NUM_COLUMNS; ++col)
		{
			m_DataEntries[col][row].setContentFitted(false);
			//m_DataEntries[col][row].setRow(row);
			//m_DataEntries[col][row].addInputSignal(this);
			Label *pDataLabel = &m_DataEntries[col][row];
			pDataLabel->setImage(NULL);
			pDataLabel->setFont(pTextScheme->font);
			//pDataLabel->setTextOffset(0, 0);
			if (col <= MAX_PLAYERS && row <= STAT_NUMELEMENTS)// test
				_snprintf(sz, 128, "val1 %d", g_PlayerStats[col][row]);

			pDataLabel->setText(sz);
			pGridRow->SetEntry(col, 0, pDataLabel);
		}
		//pGridRow->CopyColumnWidths(&m_HeaderGrid);
		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(pGridRow->getWide(), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();
		m_DataList.AddItem(pGridRow);
	}

	// -------------------
	setBorder(new CDefaultLineBorder());
	setPaintBorderEnabled(true);
	//setPos(iXPos,iYPos);
}

void CStatsPanel::Open(void)
{
	PlaySound("vgui/window2.wav", VOL_NORM);// XDM
	CMenuPanel::Open();
	//m_DataList.requestFocus();// ?
	//m_pButtonClose->requestFocus();// ?
}

void CStatsPanel::Close(void)
{
	PlaySound("vgui/menu_close.wav", VOL_NORM);// XDM
	CMenuPanel::Close();
}
#endif
