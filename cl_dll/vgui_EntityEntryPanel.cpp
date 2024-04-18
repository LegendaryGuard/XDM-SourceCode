#include "hud.h"
#include "cl_util.h"
#include "kbutton.h"
#include "cvardef.h"
#include "vgui_Int.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "vgui_EntityEntryPanel.h"
#include "in_defs.h"
#include "keydefs.h"

// XDM: in order to inherit updated coordinates from base class,
// this one should NOT use x y arguments (only getPos() if really necessasry),
CEntityEntryPanel::CEntityEntryPanel(int x, int y, int wide, int tall) : CMenuPanel(1/*remove*/, x, y, wide, tall)
{
	setBgColor(0,0,0,PANEL_DEFAULT_ALPHA);

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	ASSERT(pSchemes != NULL);
	if (!pSchemes)// STFU analyzer
		return;

	m_vTargetOrigin.Clear();
	m_vTargetAngles.Clear();
	m_iRenderSystemIndex = RS_INDEX_INVALID;

	CScheme *pTitleScheme = pSchemes->getScheme("Title Font");
	CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");
	CScheme *pSmallTextScheme = pSchemes->getScheme("Briefing Text");
	int offsetY = PANEL_INNER_OFFSET + m_TitleIcon.getTall() + YRES(EEP_MARGIN);
	int offsetX = XRES(EEP_MARGIN)+m_TitleIcon.getWide();
	char nullstring[32];
	memset(nullstring, 0, 32);

	m_LabelTitle.setParent(this);
	m_LabelTitle.setBounds(offsetX, YRES(EEP_MARGIN), wide-(offsetX+XRES(EEP_MARGIN)), m_TitleIcon.getTall());
	m_LabelTitle.setFont(pTitleScheme->font);
	m_LabelTitle.setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_LabelTitle.setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	m_LabelTitle.setContentAlignment(vgui::Label::a_west);
	m_LabelTitle.setText("%s\0", BufferedLocaliseTextString("#EntityEntryPanel_Title"));

	m_LabelClassName.setParent(this);
	m_LabelClassName.setBounds(XRES(EEP_MARGIN), offsetY, TEXTENTRY_TITLE_SIZE_X, TEXTENTRY_SIZE_Y);
	m_LabelClassName.setFont(pButtonScheme->font);// lesser font for long names
	m_LabelClassName.setContentAlignment(Label::a_east);
	m_LabelClassName.setFgColor(pButtonScheme->FgColor.r, pButtonScheme->FgColor.g, pButtonScheme->FgColor.b, 255-pButtonScheme->FgColor.a);
	m_LabelClassName.setBgColor(0,0,0, 255);
	m_LabelClassName.setPaintBackgroundEnabled(false);
	m_LabelClassName.setPaintBorderEnabled(false);
	m_LabelClassName.setText("%s\0", BufferedLocaliseTextString("#EntityEntryPanel_ClassName"));

	m_ClassName.setParent(this);
	offsetX = m_LabelClassName.getWide()+XRES(EEP_MARGIN)*2;
	m_ClassName.setBounds(offsetX, offsetY, wide-offsetX-XRES(EEP_MARGIN), TEXTENTRY_SIZE_Y);
	m_ClassName.addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDOK));
	m_ClassName.setText(BufferedLocaliseTextString("#classname"), 0);
	offsetY += YRES(EEP_MARGIN) + m_ClassName.getTall();

	m_LabelTargetName.setParent(this);
	m_LabelTargetName.setBounds(XRES(EEP_MARGIN), offsetY, TEXTENTRY_TITLE_SIZE_X, TEXTENTRY_SIZE_Y);
	m_LabelTargetName.setFont(pButtonScheme->font);// lesser font for long names
	m_LabelTargetName.setContentAlignment(Label::a_east);
	m_LabelTargetName.setFgColor(pButtonScheme->FgColor.r, pButtonScheme->FgColor.g, pButtonScheme->FgColor.b, 255-pButtonScheme->FgColor.a);
	m_LabelTargetName.setBgColor(0,0,0, 255);
	m_LabelTargetName.setPaintBackgroundEnabled(false);
	m_LabelTargetName.setPaintBorderEnabled(false);
	m_LabelTargetName.setText("%s\0", BufferedLocaliseTextString("#EntityEntryPanel_TargetName"));

	m_TargetName.setParent(this);
	offsetX = m_LabelTargetName.getWide()+XRES(EEP_MARGIN)*2;
	m_TargetName.setBounds(offsetX, offsetY, wide-offsetX-XRES(EEP_MARGIN), TEXTENTRY_SIZE_Y);
	//m_TargetName.setBounds(XRES(EEP_MARGIN), offsetY, wide-XRES(EEP_MARGIN)*2, TEXTENTRY_SIZE_Y);
	m_TargetName.addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDOK));
	m_TargetName.setText(BufferedLocaliseTextString("#targetname"), 0);
	offsetY += YRES(EEP_MARGIN) + m_TargetName.getTall();

	m_ButtonAdd.setParent(this);
	m_ButtonAdd.setBounds(XRES(EEP_MARGIN), offsetY, BUTTON_SIZE_Y, BUTTON_SIZE_Y);
	m_ButtonAdd.setContentAlignment(Label::a_center);
	m_ButtonAdd.setMainText("+");
	m_ButtonAdd.addActionSignal(new CMenuPanelActionSignalHandler(this, EEP_SIGNAL_ADD));
	m_ButtonAdd.setArmed(false);
	offsetY += YRES(EEP_MARGIN) + m_ButtonAdd.getTall();

	m_ButtonRemove.setParent(this);
	m_ButtonRemove.setBounds(XRES(EEP_MARGIN), offsetY, BUTTON_SIZE_Y, BUTTON_SIZE_Y);
	m_ButtonRemove.setContentAlignment(Label::a_center);
	m_ButtonRemove.setMainText("-");
	m_ButtonRemove.addActionSignal(new CMenuPanelActionSignalHandler(this, EEP_SIGNAL_REMOVE));
	m_ButtonRemove.setArmed(false);
	//offsetY += YRES(EEP_MARGIN) + m_ButtonRemove.getTall();

	m_ButtonOK.setParent(this);
	m_ButtonOK.setBounds(XRES(EEP_MARGIN), tall-BUTTON_SIZE_Y-YRES(EEP_MARGIN), BUTTON_SIZE_X, BUTTON_SIZE_Y);
	m_ButtonOK.setMainText(BufferedLocaliseTextString("#Menu_OK"));
	m_ButtonOK.addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDOK));
	//m_ButtonOK.setArmed(true);

	m_ButtonCancel.setParent(this);
	m_ButtonCancel.setBounds(wide-XRES(EEP_MARGIN)-BUTTON_SIZE_X, tall-BUTTON_SIZE_Y-YRES(EEP_MARGIN), BUTTON_SIZE_X, BUTTON_SIZE_Y);
	m_ButtonCancel.setMainText(BufferedLocaliseTextString("#Menu_Cancel"));
	m_ButtonCancel.addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDCANCEL));
	offsetX = m_ButtonCancel.GetX() - m_ButtonOK.getWide() - XRES(EEP_MARGIN);
	m_ButtonOK.setPos(offsetX, m_ButtonCancel.GetY());

// Toggle buttons
	m_pButtonCreate = new ToggleCommandButton(false, BufferedLocaliseTextString("#EntityEntryPanel_Create"), XRES(EEP_MARGIN), tall-BUTTON_SIZE_Y-YRES(EEP_MARGIN), offsetX-XRES(EEP_MARGIN)*2, BUTTON_SIZE_Y, true, false, NULL);
	m_pButtonCreate->setParent(this);
	m_pButtonCreate->setFont(pSmallTextScheme->font);
	m_pButtonCreate->setContentAlignment(Label::a_east);
	//ctor m_pButtonCreate->m_bShowHotKey = false;
	m_pButtonCreate->setBoundKey('d');
	m_pButtonCreate->addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDABORT));

	setBorder(new CDefaultLineBorder());
	setPaintBorderEnabled(true);
	setDragEnabled(true);
	setCaptureInput(true);
	setShowCursor(true);
	//addFocusChangeSignal(new CFocusChangeSignalClose(this));

	m_ClassName.requestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a
//-----------------------------------------------------------------------------
void CEntityEntryPanel::Open(void)
{
	CMenuPanel::Open();
	m_ClassName.requestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Capture input when player is entering text
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int
//-----------------------------------------------------------------------------
int CEntityEntryPanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (keynum != K_TAB)
	{
		if (m_ClassName.hasFocus() || m_TargetName.hasFocus())
			return 0;

		for (int i=0;i<m_szKVEntries.getCount();i++)
		{
			if (m_szKVEntries[i]->hasFocus())
			{
				//if (keynum == K_TAB && i < m_szKVEntries.getCount()-1)// XDM3038a
				//	m_szKVEntries[i+1]->requestFocus();

				return 0;
			}
		}
	}
	return CEntityEntryPanel::BaseClass::KeyInput(down, keynum, pszCurrentBinding);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : signal - 
//-----------------------------------------------------------------------------
void CEntityEntryPanel::OnActionSignal(int signal, CMenuPanelActionSignalHandler *pCaller)
{
	if (signal == VGUI_IDOK)
		OnOK();
	else if (signal == VGUI_IDCANCEL)
		OnCancel();
	else if (signal == EEP_SIGNAL_ADD)
		OnAdd();
	else if (signal == EEP_SIGNAL_REMOVE)
		OnRemove();
	else if (signal == VGUI_IDABORT)
		m_pButtonCreate->SetState(!m_pButtonCreate->GetState());// XDM3038b
	else
		CEntityEntryPanel::BaseClass::OnActionSignal(signal, pCaller);
}

//-----------------------------------------------------------------------------
// Purpose: Panel is closing
// Output : Return true to ALLOW and false to PREVENT closing
//-----------------------------------------------------------------------------
bool CEntityEntryPanel::OnClose(void)
{
	if (g_iMouseManipulationMode == MMM_CREATE)
		g_iMouseManipulationMode = MMM_NONE;

	if (m_iRenderSystemIndex != RS_INDEX_INVALID)// cancel creation
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystem(m_iRenderSystemIndex));
		m_iRenderSystemIndex = RS_INDEX_INVALID;
	}
	return CEntityEntryPanel::BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Close and apply
//-----------------------------------------------------------------------------
void CEntityEntryPanel::OnOK(void)
{
	if (doExecCommand())
		PlaySound("vgui/button_press.wav", VOL_NORM);
	else
		PlaySound("vgui/button_exit.wav", VOL_NORM);

	Close();
}

//-----------------------------------------------------------------------------
// Purpose: Close and discard
//-----------------------------------------------------------------------------
void CEntityEntryPanel::OnCancel(void)
{
	PlaySound("vgui/button_exit.wav", VOL_NORM);
	Close();
}

//-----------------------------------------------------------------------------
// Purpose: Add input box
//-----------------------------------------------------------------------------
void CEntityEntryPanel::OnAdd(void)
{
	int x = 0,y = 0, i;
	int c = m_szKVEntries.getCount();
	if (c >= EEP_MAX_LINES)
	{
		PlaySound("vgui/menu_close.wav", VOL_NORM);
		return;
	}
	m_TargetName.getPos(x, y);
	y += YRES(EEP_MARGIN) + m_TargetName.getTall();
	for (i=0;i<c;++i)
		y += m_szKVEntries[i]->getTall() + YRES(EEP_MARGIN);

	x = m_ButtonAdd.getWide() + XRES(EEP_MARGIN)*2;
	CDefaultTextEntry *pInput = new CDefaultTextEntry("key:value", x,y, getWide()-x-XRES(EEP_MARGIN), TEXTENTRY_SIZE_Y);
	if (pInput)
	{
		//pInput->addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDOK));
		m_szKVEntries.putElement(pInput);
		addChild(pInput);//pInput->setParent(this);
		if (y + pInput->getTall() > m_ButtonOK.GetY())// only increase window size if there are too many
		{
			getPos(x, y);
			setBounds(x, y, getWide(), getTall() + pInput->getTall()+YRES(EEP_MARGIN));// resize the window
			/*m_ButtonOK.getPos(x, y);
			m_ButtonOK.setBounds(x, y + pInput->getTall()+YRES(EEP_MARGIN), m_ButtonOK.getWide(), m_ButtonOK.getTall());
			m_ButtonCancel.getPos(x, y);
			m_ButtonCancel.setBounds(x, y + pInput->getTall()+YRES(EEP_MARGIN), m_ButtonCancel.getWide(), m_ButtonCancel.getTall());
			m_pButtonCreate->setBounds()*/
			// reposition buttons
			m_ButtonCancel.setPos(getWide()-XRES(EEP_MARGIN)-BUTTON_SIZE_X, getTall()-BUTTON_SIZE_Y-YRES(EEP_MARGIN));
			int offsetX = m_ButtonCancel.GetX() - m_ButtonOK.getWide() - XRES(EEP_MARGIN);
			m_ButtonOK.setPos(offsetX, m_ButtonCancel.GetY());
			m_pButtonCreate->setPos(XRES(EEP_MARGIN), getTall()-BUTTON_SIZE_Y-YRES(EEP_MARGIN));
		}
		pInput->requestFocus();
	}
	PlaySound("vgui/button_press.wav", VOL_NORM);
}

//-----------------------------------------------------------------------------
// Purpose: Remove input box
//-----------------------------------------------------------------------------
void CEntityEntryPanel::OnRemove(void)
{
	CDefaultTextEntry *pInput = NULL;
	int i = 0;//, iindex = 0;
	int c = m_szKVEntries.getCount();
	//int iTotalKVInputHeight = 0;
	for (i=0;i<c;++i)// find highlighted input box
	{
		//iTotalKVInputHeight += m_szKVEntries[i]->getTall();
		if (m_szKVEntries[i]->hasFocus())
		{
			//iindex = i;
			pInput = m_szKVEntries[i];
			break;
		}
	}
	if (pInput)
	{
		int x=0,y=0;
		for (++i;i<c;++i)// move all following input boxes upwards
		{
			//iTotalKVInputHeight += m_szKVEntries[i]->getTall();
			m_szKVEntries[i]->getPos(x,y);
			y -= pInput->getTall() + YRES(EEP_MARGIN);
			m_szKVEntries[i]->setPos(x,y);
		}

		int maxY = 0;
		for (i=0;i<c;++i)
			if (m_szKVEntries[i]->GetY() > maxY)
				maxY = m_szKVEntries[i]->GetY();

		if (maxY + pInput->getTall() > m_ButtonRemove.GetY()+m_ButtonRemove.getTall())// only increase window size if there are too many
		{
			getPos(x, y);
			setBounds(x, y, getWide(), getTall() - pInput->getTall()-YRES(EEP_MARGIN));// resize the window
			/*m_ButtonOK.getPos(x, y);
			m_ButtonOK.setBounds(x, y - pInput->getTall()-YRES(EEP_MARGIN), m_ButtonOK.getWide(), m_ButtonOK.getTall());
			m_ButtonCancel.getPos(x, y);
			m_ButtonCancel.setBounds(x, y - pInput->getTall()-YRES(EEP_MARGIN), m_ButtonCancel.getWide(), m_ButtonCancel.getTall());*/
			// reposition buttons
			m_ButtonCancel.setPos(getWide()-XRES(EEP_MARGIN)-BUTTON_SIZE_X, getTall()-BUTTON_SIZE_Y-YRES(EEP_MARGIN));
			int offsetX = m_ButtonCancel.GetX() - m_ButtonOK.getWide() - XRES(EEP_MARGIN);
			m_ButtonOK.setPos(offsetX, m_ButtonCancel.GetY());
			m_pButtonCreate->setPos(XRES(EEP_MARGIN), getTall()-BUTTON_SIZE_Y-YRES(EEP_MARGIN));
		}
		m_szKVEntries.removeElement(pInput);// does not erase/deallocate anything
		removeChild(pInput);
		//delete pInput;
		m_szKVEntries[m_szKVEntries.getCount()-1]->requestFocus();
		PlaySound("vgui/menu_close.wav", VOL_NORM);
	}
	else
	{
		PlaySound("vgui/button_error.wav", VOL_NORM);
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: The real action
// Warning: XDM3038b: new format!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEntityEntryPanel::doExecCommand(void)
{
	const size_t bufsize = 256;
	char buffer[bufsize];
	//char targetname[80];
	m_ClassName.getText(0,buffer,bufsize);
	//m_TargetName.getText(0,targetname,80);

	if (buffer[0] == 0)
	{
		conprintf(0, "CEntityEntryPanel: enter classname first.\n");
		return false;
	}	
	/*if (g_iEntityCreationRS == RS_INDEX_INVALID)
	{
		conprintf(0, "CEntityEntryPanel: ERROR! No RenderSystem index!\n");
		return false;
	}*/
	//CRSModel *pSys = (CRSModel *)g_pRenderManager->FindSystem(g_iEntityCreationRS);
	//if (pSys)
	//{
		char scmd[256];
		// .c "classname" 0 "1 2 3" "1 2 3" "k:v" "k:v" "k:v"...
		_snprintf(scmd, 256, ".c \"%s\" %d \"%g %g %g\" \"%g %g %g\" \0", buffer, m_pButtonCreate->GetState()?1:0, m_vTargetOrigin.x, m_vTargetOrigin.y, m_vTargetOrigin.z, m_vTargetAngles.x, m_vTargetAngles.y, m_vTargetAngles.z);
		scmd[255] = '\0';

		m_TargetName.getText(0,buffer,bufsize);
		if (buffer[0])
		{
			strcat(scmd, " \"targetname:");
			strcat(scmd, buffer);
			strcat(scmd, "\"");
		}
		for (int i=0;i<m_szKVEntries.getCount();i++)
		{
			m_szKVEntries[i]->getText(0,buffer,bufsize);
			if (buffer[0])
			{
				strcat(scmd, " \"");
				strcat(scmd, buffer);
				strcat(scmd, "\"");
			}
		}

		/*old	if (targetname[0])
			_snprintf(scmd, 256, ".c \"%s\" \"%g %g %g\" \"%g %g %g\" \"%s\"\0", classname, m_vTargetOrigin[0], m_vTargetOrigin[1], m_vTargetOrigin[2], m_vTargetAngles[0], m_vTargetAngles[1], m_vTargetAngles[2], targetname);
		else
			_snprintf(scmd, 256, ".c \"%s\" \"%g %g %g\" \"%g %g %g\"\0", classname, m_vTargetOrigin[0], m_vTargetOrigin[1], m_vTargetOrigin[2], m_vTargetAngles[0], m_vTargetAngles[1], m_vTargetAngles[2]);*/
#if defined (_DEBUG)
		conprintf(1, "CEntityEntryPanel: -> server: %s\n", scmd);
#endif
		SERVER_COMMAND(scmd);


		//g_pRenderManager->DeleteSystem(pSys);
		//g_iEntityCreationRS = RS_INDEX_INVALID;
		return true;
	//}
	//return false;
}
