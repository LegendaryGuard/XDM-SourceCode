#ifndef VGUI_MUSICPLAYER_H
#define VGUI_MUSICPLAYER_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include <VGUI_LineBorder.h>
#include <VGUI_IntChangeSignal.h>
#include "vgui_scrollbar2.h"
#include "vgui_slider2.h"

#define MPLAYER_PANEL_SIZEX				240
#define MPLAYER_PANEL_SIZEY				100
#define MPLAYER_WINDOW_MARGIN			2
#define MPLAYER_TIMEWINDOW_WIDTH		64
#define MPLAYER_TIMEWINDOW_HEIGHT		24
#define MPLAYER_TITLE_HEIGHT			16
#define MPLAYER_VOLSLIDER_WIDTH			80
#define MPLAYER_VOLSLIDER_HEIGHT		8
#define MPLAYER_SEEKBAR_HEIGHT			12
#define MPLAYER_BUTTON_SIZEX			20
#define MPLAYER_BUTTON_SIZEY			20
#define MPLAYER_BUTTON_INTERVAL			1
#define MPLAYER_BUTTON_BOTTOM_MARGIN	2

#define MPLAYER_SEEKBAR_VALMIN			0
#define MPLAYER_SEEKBAR_VALMAX			100

typedef enum
{
	MPCMD_CLOSE = 0,
	MPCMD_PREV,
	MPCMD_PLAY,
	MPCMD_PAUSE,
	MPCMD_STOP,
	MPCMD_NEXT,
	MPCMD_SEEK,
	MPCMD_REPEAT,
	MPCMD_SHUFFLE,
	MPCMD_LOAD,
	MPCMD_LOADMAP,
} mplayer_cmd;

typedef enum
{
	MPSI_SEEK = 0,
	MPSI_VOLUME,
	MPSI_BALANCE,
} mplayer_sliders;

/* OBSOLETE class CMusicPlayerHandler_Command : public CMenuPanelActionSignalHandler
{
	typedef CMenuActionHandler BaseClass;
public:
	CMusicPlayerHandler_Command(CMusicPlayerPanel *panel, mplayer_cmd cmd);
	virtual void actionPerformed(Panel *panel);

protected:
	CMusicPlayerPanel *m_pPanel;
	mplayer_cmd m_iCmd;
};*/


class CMusicPlayerHandler_Slider : public IntChangeSignal
{
	typedef IntChangeSignal BaseClass;
public:
	CMusicPlayerHandler_Slider(CMusicPlayerPanel *panel, mplayer_sliders slider);
	virtual void intChanged(int value, Panel *panel);
	virtual void SetEnabled(bool enable);

protected:
	CMusicPlayerPanel *m_pPanel;
	mplayer_sliders m_usSlider;
	bool m_bEnabled;
};


class CMusicPlayerInputHandler : public CDefaultInputSignal
{
	typedef CDefaultInputSignal BaseClass;
public:
	CMusicPlayerInputHandler(CMusicPlayerPanel *panel, mplayer_cmd cmd);

	virtual void mousePressed(MouseCode code, Panel *panel);
	virtual void mouseReleased(MouseCode code, Panel *panel);

protected:
	CMusicPlayerPanel *m_pPanel;
	mplayer_cmd m_iCmd;
	bool m_bMousePressed;
};



class CMusicPlayerPanel : public CMenuPanel
{
	typedef CMenuPanel BaseClass;
public:
	CMusicPlayerPanel(int iRemoveMe, int x, int y, int wide, int tall);
	virtual void Open(void);
	virtual bool OnClose(void);
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);
	virtual void OnActionSignal(int signal, class CMenuPanelActionSignalHandler *pCaller);// XDM3038a

//	virtual bool IsPersistent(void) { return false; }// XDM3038
//	virtual bool AllowConcurrentWindows(void) { return true; }// XDM3038

	void Initialize(void);
	void Update(void);
//	void ActionSignal(mplayer_cmd cmd, CMenuPanelActionSignalHandler *pCaller);
	void SliderSignal(mplayer_sliders slider, int value);
//	int	KeyInput(int down, int keynum, const char *pszCurrentBinding);
//DNW	virtual void internalKeyTyped(KeyCode code);
	void SetTrackInfo(const char *name);

	int m_iLoopMode;

private:
	CommandButton		m_ButtonPrev;
	CommandButton		m_ButtonPlay;
	CommandButton		m_ButtonPause;
	CommandButton		m_ButtonStop;
	CommandButton		m_ButtonNext;
	CommandButton		m_ButtonLoad;
	CommandButton		m_ButtonLoadMap;// XDM3037a
	CommandButton		m_ButtonClose;
	ToggleCommandButton	*m_pButtonRepeat;
	ToggleCommandButton	*m_pButtonAutoPlay;// XDM3037a
	Label				*m_pTrackTitle;
	Label				*m_pTrackTime;
//	ScrollBar			*m_pSeekBar;
//	CCustomSlider		*m_pSliderVolume;
	ScrollBar2			*m_pSeekBar;
	Slider2				*m_pSliderVolume;
//	Slider2				*m_pSliderBalance;
//	bool	m_bDisableSeekAutoUpdate;

//	CMusicPlayerInputHandler *m_pSliderInputHandler;
	CMusicPlayerHandler_Slider *m_pSeekChangeHandler;
/*
	bool m_MouseInside;
	bool m_MousePressed;
	int m_iMouseLastX;
	int m_iMouseLastY;
	*/
};


#endif // VGUI_MUSICPLAYER_H
