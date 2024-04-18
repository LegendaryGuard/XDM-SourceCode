#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "vgui_MusicPlayer.h"
#include "musicplayer.h"
#include "keydefs.h"


CMusicPlayerPanel::CMusicPlayerPanel(int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iRemoveMe, x,y,wide,tall)
{
	m_iLoopMode = 0;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	ASSERT(pSchemes != NULL);
	if (!pSchemes)// STFU analyzer
		return;
	CScheme *pTitleScheme = pSchemes->getScheme("Title Font");
	CScheme *pButtonScheme = pSchemes->getScheme("Primary Button Text");

	setBgColor(0,0,0,PANEL_DEFAULT_ALPHA);
	setBorder(new CDefaultLineBorder());
	setPaintBorderEnabled(true);
	setCaptureInput(true);
	setShowCursor(true);

	m_pTrackTitle = new Label("----", XRES(MPLAYER_WINDOW_MARGIN), YRES(MPLAYER_WINDOW_MARGIN), wide-XRES(MPLAYER_WINDOW_MARGIN*2 + MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_TITLE_HEIGHT));
	m_pTrackTitle->setParent(this);
	m_pTrackTitle->setFont(pButtonScheme->font);// lesser font for long names
	m_pTrackTitle->setContentAlignment(Label::a_east);
	m_pTrackTitle->setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_pTrackTitle->setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-pTitleScheme->BgColor.a);
	//m_pTrackTitle->setAsMouseArena(false);
	//m_pTrackTitle->setAsMouseCapture(false);
	//m_pTrackTitle->addInputSignal(new CDragNDropHandler(this));// XDM3038a: TODO: UNDONE: find out where it fails

	m_pTrackTime = new Label("--:--", XRES(MPLAYER_WINDOW_MARGIN), m_pTrackTitle->getTall()+YRES(MPLAYER_WINDOW_MARGIN+2), XRES(MPLAYER_TIMEWINDOW_WIDTH), YRES(MPLAYER_TIMEWINDOW_HEIGHT));
	m_pTrackTime->setParent(this);
	m_pTrackTime->setFont(pTitleScheme->font);
	m_pTrackTime->setContentAlignment(Label::a_west);
	m_pTrackTime->setFgColor(pTitleScheme->FgColor.r, pTitleScheme->FgColor.g, pTitleScheme->FgColor.b, 255-pTitleScheme->FgColor.a);
	m_pTrackTime->setBgColor(pTitleScheme->BgColor.r, pTitleScheme->BgColor.g, pTitleScheme->BgColor.b, 255-clamp(pTitleScheme->BgColor.a+63, 0,255));

	m_pSliderVolume = new Slider2(m_pTrackTime->getWide()+XRES(MPLAYER_WINDOW_MARGIN*2), m_pTrackTitle->getTall()+m_pTrackTime->getTall()+YRES(MPLAYER_WINDOW_MARGIN+2-MPLAYER_VOLSLIDER_HEIGHT), XRES(MPLAYER_VOLSLIDER_WIDTH), YRES(MPLAYER_VOLSLIDER_HEIGHT), false);
	m_pSliderVolume->setParent(this);
	m_pSliderVolume->setRange(0, BGM_VOLUME_MAX);
	m_pSliderVolume->setRangeWindow(8);
	m_pSliderVolume->setRangeWindowEnabled(true);
	m_pSliderVolume->setValue(0);
	m_pSliderVolume->setPaintBorderEnabled(true);
	m_pSliderVolume->setPaintBackgroundEnabled(true);
	//m_pSliderVolume->setPaintEnabled(true);
	//m_pSliderVolume->setBgColor(255,0,0, 127);
	//m_pSliderVolume->setFgColor(255,0,255, 127);
	m_pSliderVolume->setMinimumSize(XRES(MPLAYER_VOLSLIDER_WIDTH), YRES(MPLAYER_VOLSLIDER_HEIGHT));
	m_pSliderVolume->addIntChangeSignal(new CMusicPlayerHandler_Slider(this, MPSI_VOLUME));

	m_pSeekBar = new ScrollBar2(XRES(MPLAYER_WINDOW_MARGIN), tall/2, wide-XRES(MPLAYER_WINDOW_MARGIN)*2, YRES(MPLAYER_SEEKBAR_HEIGHT), false);
	m_pSeekBar->setParent(this);
	m_pSeekBar->setRange(MPLAYER_SEEKBAR_VALMIN, MPLAYER_SEEKBAR_VALMAX);
	m_pSeekBar->setRangeWindow(10);
	m_pSeekBar->setRangeWindowEnabled(false);
	Slider2 *pSlider = m_pSeekBar->getSlider();
	if (pSlider)
	{
		pSlider->setValue(0);
		//pSlider->setRange(MPLAYER_SEEKBAR_VALMIN, MPLAYER_SEEKBAR_VALMAX);
		pSlider->setMinimumSize(XRES(4), m_pSeekBar->getTall());
		pSlider->addInputSignal(new CMusicPlayerInputHandler(this, MPCMD_SEEK));
	}
	m_pSeekBar->setButtonPressedScrollValue(10);// +- 10
	m_pSeekBar->setPaintBorderEnabled(true);
	//m_pSeekBar->setPaintBackgroundEnabled(true);
	//m_pSeekBar->setPaintEnabled(true);
	m_pSeekChangeHandler = new CMusicPlayerHandler_Slider(this, MPSI_SEEK);
	m_pSeekBar->addIntChangeSignal(m_pSeekChangeHandler);
	m_pSeekBar->validate();
/* UNDONE
	m_pSeekBar = new ScrollBar(XRES(MPLAYER_WINDOW_MARGIN), tall/2, wide-XRES(MPLAYER_WINDOW_MARGIN)*2, YRES(MPLAYER_SEEKBAR_HEIGHT), false);
	m_pSeekBar->setButton(new CCustomScrollButton(ARROW_LEFT, "", 0,0,16,16), 0);
	m_pSeekBar->setButton(new CCustomScrollButton(ARROW_RIGHT, "", 0,0,16,16), 1);
	m_pSeekBar->setSlider(new CCustomSlider(0,0,XRES(32),m_pSeekBar->getTall(), false));
	m_pSeekBar->setRange(MPLAYER_SEEKBAR_VALMIN, MPLAYER_SEEKBAR_VALMAX);
	m_pSeekBar->setRangeWindow(10);
	m_pSeekBar->setRangeWindowEnabled(true);
	m_pSeekBar->setButtonPressedScrollValue(10);// +- 10
	m_pSeekBar->setPaintBorderEnabled(true);
	m_pSeekChangeHandler = new CMusicPlayerHandler_Slider(this, MPSI_SEEK);
	m_pSeekBar->addIntChangeSignal(m_pSeekChangeHandler);
	m_pSeekBar->validate();*/

	//m_pButtonClose = new CommandButton("X", wide - XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_WINDOW_MARGIN), YRES(MPLAYER_WINDOW_MARGIN), XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, NULL, pButtonScheme);
	m_ButtonClose.setParent(this);
	m_ButtonClose.setBounds(wide - XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_WINDOW_MARGIN), YRES(MPLAYER_WINDOW_MARGIN), XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonClose.setFont(pButtonScheme->font);
	m_ButtonClose.setContentAlignment(Label::a_center);
	m_ButtonClose.m_bShowHotKey = false;
	m_ButtonClose.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_CLOSE));
	m_ButtonClose.m_bShowHotKey = false;
	m_ButtonClose.setBoundKey((char)255);
	m_ButtonClose.setMainText("X");

	int posX = XRES(16);
	int posY = tall - YRES(MPLAYER_BUTTON_SIZEY + MPLAYER_BUTTON_BOTTOM_MARGIN + MPLAYER_WINDOW_MARGIN);

	//m_pButtonPrev = new CommandButton("<|", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_start.tga", pButtonScheme);
	m_ButtonPrev.setParent(this);
	m_ButtonPrev.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonPrev.setFont(pButtonScheme->font);
	m_ButtonPrev.setContentAlignment(Label::a_center);
	m_ButtonPrev.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_PREV));
	m_ButtonPrev.m_bShowHotKey = false;
	m_ButtonPrev.setBoundKey('z');// these do nothing by themselves
	m_ButtonPrev.setMainText("<|");
	m_ButtonPrev.LoadIcon("resource/icon_start.tga", true);
	posX += XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_BUTTON_INTERVAL);

	//m_pButtonPlay = new CommandButton("|>", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_play.tga", pButtonScheme);
	m_ButtonPlay.setParent(this);
	m_ButtonPlay.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonPlay.setFont(pButtonScheme->font);
	m_ButtonPlay.setContentAlignment(Label::a_center);
	m_ButtonPlay.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_PLAY));
	m_ButtonPlay.m_bShowHotKey = false;
	m_ButtonPlay.setBoundKey('x');
	m_ButtonPlay.setMainText("|>");
	m_ButtonPlay.LoadIcon("resource/icon_play.tga", true);
	posX += XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_BUTTON_INTERVAL);

	//m_pButtonPause = new CommandButton("||", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_pause.tga", pButtonScheme);
	m_ButtonPause.setParent(this);
	m_ButtonPause.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonPause.setFont(pButtonScheme->font);
	m_ButtonPause.setContentAlignment(Label::a_center);
	m_ButtonPause.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_PAUSE));
	m_ButtonPause.m_bShowHotKey = false;
	m_ButtonPause.setBoundKey('c');
	m_ButtonPause.setMainText("||");
	m_ButtonPause.LoadIcon("resource/icon_pause.tga", true);
	posX += XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_BUTTON_INTERVAL);

	//m_pButtonStop = new CommandButton("[]", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_stop.tga", pButtonScheme);
	m_ButtonStop.setParent(this);
	m_ButtonStop.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonStop.setFont(pButtonScheme->font);
	m_ButtonStop.setContentAlignment(Label::a_center);
	m_ButtonStop.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_STOP));
	m_ButtonStop.m_bShowHotKey = false;
	m_ButtonStop.setBoundKey('v');
	m_ButtonStop.setMainText("[]");
	m_ButtonStop.LoadIcon("resource/icon_stop.tga", true);
	posX += XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_BUTTON_INTERVAL);

	//m_pButtonNext = new CommandButton("|>", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_end.tga", pButtonScheme);
	m_ButtonNext.setParent(this);
	m_ButtonNext.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonNext.setFont(pButtonScheme->font);
	m_ButtonNext.setContentAlignment(Label::a_center);
	m_ButtonNext.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_NEXT));
	m_ButtonNext.m_bShowHotKey = false;
	m_ButtonNext.setBoundKey('b');
	m_ButtonNext.setMainText("|>");
	m_ButtonNext.LoadIcon("resource/icon_end.tga", true);
	posX += XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_BUTTON_INTERVAL*2);

	//m_pButtonLoad = new CommandButton("^", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_load.tga", pButtonScheme);
	m_ButtonLoad.setParent(this);
	m_ButtonLoad.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonLoad.setFont(pButtonScheme->font);
	m_ButtonLoad.setContentAlignment(Label::a_center);
	m_ButtonLoad.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_LOAD));
	m_ButtonLoad.m_bShowHotKey = false;
	m_ButtonLoad.setBoundKey('l');
	m_ButtonLoad.setMainText("^");
	m_ButtonLoad.LoadIcon("resource/icon_load.tga", true);
	posX += XRES(MPLAYER_BUTTON_SIZEX + MPLAYER_BUTTON_INTERVAL);

	// XDM3037a: "load map playlist"
	//m_pButtonLoadMap = new CommandButton("^m", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, false, "resource/icon_load.tga", pButtonScheme);
	m_ButtonLoadMap.setParent(this);
	m_ButtonLoadMap.setBounds(posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY));
	m_ButtonLoadMap.setFont(pButtonScheme->font);
	m_ButtonLoadMap.setContentAlignment(Label::a_center);
	m_ButtonLoadMap.addActionSignal(new CMenuPanelActionSignalHandler(this, MPCMD_LOADMAP));
	m_ButtonLoadMap.m_bShowHotKey = false;
	m_ButtonLoadMap.setBoundKey('m');
	m_ButtonLoadMap.setMainText("^m");
	m_ButtonLoadMap.LoadIcon("resource/icon_load.tga", true);

// Toggle buttons
	posX = wide-XRES(16)-XRES(MPLAYER_BUTTON_SIZEX);// start from the right border
	m_pButtonRepeat = new ToggleCommandButton(bgm_pls_loop?(bgm_pls_loop->value > 0):0, "@", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, pButtonScheme);
	m_pButtonRepeat->setParent(this);
	m_pButtonRepeat->setFont(pButtonScheme->font);
	m_pButtonRepeat->setContentAlignment(Label::a_center);
	m_pButtonRepeat->m_bShowHotKey = false;
	m_pButtonRepeat->setBoundKey('r');
	if (bgm_pls_loop)// XDM3037a: may be NULL if DLL is absent
		m_pButtonRepeat->addActionSignal(new CMenuHandler_ToggleCvar(NULL, bgm_pls_loop));

	// XDM3037a: "auto play at map start" checkbox
	posX -= XRES(MPLAYER_BUTTON_INTERVAL + MPLAYER_BUTTON_SIZEX);// continue to the left
	m_pButtonAutoPlay = new ToggleCommandButton(bgm_playmaplist?(bgm_playmaplist->value > 0):0, "AP", posX, posY, XRES(MPLAYER_BUTTON_SIZEX), YRES(MPLAYER_BUTTON_SIZEY), false, false, pButtonScheme);
	m_pButtonAutoPlay->setParent(this);
	m_pButtonAutoPlay->setFont(pButtonScheme->font);
	m_pButtonAutoPlay->setContentAlignment(Label::a_center);
	m_pButtonAutoPlay->m_bShowHotKey = false;
	m_pButtonAutoPlay->setBoundKey('p');
	if (bgm_playmaplist)
		m_pButtonAutoPlay->addActionSignal(new CMenuHandler_ToggleCvar(NULL, bgm_playmaplist));

	setDragEnabled(true);
	//m_bDisableSeekAutoUpdate = false;
}

void CMusicPlayerPanel::Initialize(void)
{
	setVisible(false);
	if (m_pSeekBar)
		m_pSeekBar->setValue(0);
	if (m_pSliderVolume)
		m_pSliderVolume->setValue(BGM_VOLUME_MAX);

	m_iLoopMode = 0;
	// TODO: Clear out all the values here
}

void CMusicPlayerPanel::Update(void)
{
	//if (m_bDisableSeekAutoUpdate == false)
	if (!m_pSeekBar->isMouseDown(MOUSE_LEFT) && BGM_IsPlaying())
	{
		m_pSeekChangeHandler->SetEnabled(false);// HACK!?
		m_pSeekBar->setValue(BGM_GetPosition());
		m_pTrackTitle->setText(BGM_PLS_GetTrackName());
		int ms = BGM_GetTimeMs();
		int min = ms/60000;
		float sec = (float)ms/1000.0f - min*60;
		m_pTrackTime->setText("%d:%.2f", min, sec);
		m_pSeekChangeHandler->SetEnabled(true);
	}
	m_pSliderVolume->setValue(BGM_GetVolume());

	if (isVisible())
	{
		m_pTrackTime->repaint();
		m_pSeekBar->validate();
		m_pSeekBar->repaint();
		m_pTrackTitle->repaint();
	}
}

void CMusicPlayerPanel::SetTrackInfo(const char *name)
{
	m_pTrackTitle->setText(name);
}

void CMusicPlayerPanel::Open(void)
{
	if (!BGM_Initialized())
	{
		PlaySound("vgui/button_error.wav", VOL_NORM);
		m_pSeekBar->setEnabled(false);
		return;
	}
	PlaySound("vgui/menu_activate.wav", VOL_NORM);
	m_pSeekBar->setEnabled(true);

	if (bgm_pls_loop)
	{
		m_pButtonRepeat->SetState(bgm_pls_loop->value > 0);
		m_pButtonRepeat->setEnabled(true);
	}
	else
	{
		m_pButtonRepeat->SetState(false);
		m_pButtonRepeat->setEnabled(false);
	}
	if (bgm_playmaplist)
	{
		m_pButtonAutoPlay->SetState(bgm_playmaplist->value > 0);
		m_pButtonAutoPlay->setEnabled(true);
	}
	else
	{
		m_pButtonAutoPlay->SetState(false);
		m_pButtonAutoPlay->setEnabled(false);
	}
	CMusicPlayerPanel::BaseClass::Open();
	Update();
	//m_pTrackTitle->repaint();
}

// XDM3038
bool CMusicPlayerPanel::OnClose(void)
{
	bool result = CMusicPlayerPanel::BaseClass::OnClose();
	if (result)// OK to close
	{
		m_pSeekBar->setEnabled(false);
		PlaySound("vgui/menu_close.wav", VOL_NORM);
	}
	return result;
}

void CMusicPlayerPanel::OnActionSignal(int signal, CMenuPanelActionSignalHandler *pCaller)
{
	bool success = false;
	switch (signal)
	{
	case MPCMD_CLOSE:
		{
			Close();// don't execute any code after this
			return;
		}
		break;
	case MPCMD_PREV:	success = BGM_PLS_Prev(); break;
	case MPCMD_PLAY:	success = BGM_PLS_Play(-1, -1); break;// XDM3035
	case MPCMD_PAUSE:	success = BGM_Pause(); break;
	case MPCMD_STOP:
		{
			success = BGM_Stop();
			m_pSeekBar->setValue(0);// XDM3037a
			m_pTrackTime->setText("");// XDM3038a
		}
		break;
	case MPCMD_NEXT:	success = BGM_PLS_Next(); break;
	case MPCMD_SEEK:
		{
			success = BGM_SetPosition((float)m_pSeekBar->getValue());
			//m_bDisableSeekAutoUpdate = false;
		}
		break;
	case MPCMD_LOAD:
		{
			if (bgm_defplaylist && bgm_defplaylist->string)
			{
				if (BGM_PLS_Load(bgm_defplaylist->string) > 0)// XDM3035
				{
					//BGM_PLS_SetLoopMode(m_iLoopMode);
					success = BGM_PLS_Play(0, m_iLoopMode);// is autoplay necessary?
				}
			}
		}
		break;
	case MPCMD_LOADMAP:
		{
			if (BGM_PLS_LoadForMap() > 0)// XDM3037a
				success = BGM_PLS_Play(0, m_iLoopMode);
		}
		break;// XDM3037a
	}
	if (success)
	{
		if (pCaller)// !!!
			pCaller->m_iSoundType = ACTSIGSOUND_SUCCESS;

		m_pTrackTitle->setText(BGM_PLS_GetTrackName());
		Update();
	}
	else
	{
		if (pCaller)// !!!
			pCaller->m_iSoundType = ACTSIGSOUND_FAILURE;
		//PlaySound("vgui/button_error.wav", VOL_NORM);
	}
}

void CMusicPlayerPanel::SliderSignal(mplayer_sliders slider, int value)
{
	switch (slider)
	{
	case MPSI_SEEK:
		{
			//m_bDisableSeekAutoUpdate = true;
			// TODO: if (mousereleased)
			//NO!	BGM_SetPosition(value);// 0-100
			//m_pTrackTime->setText("%.2f", ((float)BGM_GetLengthMs()/1000.0f)*(value/(MPLAYER_SEEKBAR_VALMAX-MPLAYER_SEEKBAR_VALMIN)));
			m_pTrackTime->setText("%d%%", value);
			PlaySound("vgui/slider_move.wav", VOL_NORM);
		}
		break;
	case MPSI_VOLUME:
		{
			m_pTrackTime->setText("< %d", value);
			BGM_SetVolume(value);// 0-255
		}
		break;
	case MPSI_BALANCE:
		{
			m_pTrackTime->setText(">< %d%%", (value*100)/255);
			BGM_SetBalance(value);
		}
		break;// 0-255
	}
}

//-----------------------------------------------------------------------------
// Purpose: Catch hotkeys
// Input  : &down - 
//			&keynum - 
//			*pszCurrentBinding - 
// Output : int - 0: handled, 1: unhandled, allow other panels to recieve key
//-----------------------------------------------------------------------------
int CMusicPlayerPanel::KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding)
{
	if (down && keynum != K_TAB)
	{
		/*if (/ *keynum == K_ENTER || keynum == K_KP_ENTER || * /keynum == K_SPACE || keynum == K_ESCAPE)
		{
			//m_pButtonClose->fireActionSignal();
			Close();
			return 0;
		}
		else */if (keynum == m_ButtonPrev.getBoundKey())
		{
			m_ButtonPrev.fireActionSignal();
			return 0;
		}
		else if (keynum == m_ButtonPlay.getBoundKey())
		{
			m_ButtonPlay.fireActionSignal();
			return 0;
		}
		else if (keynum == m_ButtonPause.getBoundKey())
		{
			m_ButtonPause.fireActionSignal();
			return 0;
		}
		else if (keynum == m_ButtonStop.getBoundKey())
		{
			m_ButtonStop.fireActionSignal();
			return 0;
		}
		else if (keynum == m_ButtonNext.getBoundKey())
		{
			m_ButtonNext.fireActionSignal();
			return 0;
		}
		else if (keynum == m_ButtonLoad.getBoundKey())
		{
			m_ButtonLoad.fireActionSignal();
			return 0;
		}
		else if (keynum == m_ButtonLoadMap.getBoundKey())
		{
			m_ButtonLoadMap.fireActionSignal();
			return 0;
		}
		else if (keynum == m_pButtonRepeat->getBoundKey())
		{
			m_pButtonRepeat->fireActionSignal();
			return 0;
		}
		else if (keynum == m_pButtonAutoPlay->getBoundKey())
		{
			m_pButtonAutoPlay->fireActionSignal();
			return 0;
		}
	}
	return CMusicPlayerPanel::BaseClass::KeyInput(down, keynum, pszCurrentBinding);
}




/* OBSOLETE CMusicPlayerHandler_Command::CMusicPlayerHandler_Command(CMusicPlayerPanel *panel, mplayer_cmd cmd) : CMenuPanelActionSignalHandler(panel, cmd)
{
	m_pPanel = panel;
	m_iCmd = cmd;
}

void CMusicPlayerHandler_Command::actionPerformed(Panel *panel)
{
	if (m_pPanel)
		m_pPanel->ActionSignal(m_iCmd, this);

	CMenuPanelActionSignalHandler::actionPerformed(panel);
}*/


CMusicPlayerHandler_Slider::CMusicPlayerHandler_Slider(CMusicPlayerPanel *panel, mplayer_sliders slider)
{
	m_pPanel = panel;
	m_usSlider = slider;
	SetEnabled(true);
}

void CMusicPlayerHandler_Slider::intChanged(int value, Panel *panel)
{
	if (m_pPanel && m_bEnabled)
		m_pPanel->SliderSignal(m_usSlider, value);
}

void CMusicPlayerHandler_Slider::SetEnabled(bool enable)
{
	m_bEnabled = enable;
}


CMusicPlayerInputHandler::CMusicPlayerInputHandler(CMusicPlayerPanel *panel, mplayer_cmd cmd) : CDefaultInputSignal()
{
	m_pPanel = panel;
	m_iCmd = cmd;
}

void CMusicPlayerInputHandler::mousePressed(MouseCode code, Panel *panel)
{
	m_bMousePressed = true;
}

void CMusicPlayerInputHandler::mouseReleased(MouseCode code, Panel *panel)
{
	if (m_pPanel && m_bMousePressed)
	{
		m_pPanel->OnActionSignal(m_iCmd, NULL);// handler is of incompatible type
	}
	m_bMousePressed = false;
}
