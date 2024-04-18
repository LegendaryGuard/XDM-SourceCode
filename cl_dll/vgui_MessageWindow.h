#ifndef VGUI_MESSAGEWINDOW_H
#define VGUI_MESSAGEWINDOW_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#define MOTD_WINDOW_X			112
#define MOTD_WINDOW_Y			80
#define MOTD_WINDOW_SIZE_X		480// in 640x480 space XRES
#define MOTD_WINDOW_SIZE_Y		360

#define MOTD_BORDER				16

//-----------------------------------------------------------------------------
// Basic text window with close button
//-----------------------------------------------------------------------------
class CMessageWindowPanel : public CMenuPanel
{
private:
	typedef CMenuPanel BaseClass;
public:
	CMessageWindowPanel(const char *szMessage, const char *szTitle, bool ShadeFullScreen, int iRemoveMe, int x, int y, int wide, int tall);
	virtual ~CMessageWindowPanel();
	virtual void Open(void);// XDM
	virtual bool OnClose(void);
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);// XDM3038c
	virtual bool IsPersistent(void) { return false; }// XDM3038
	virtual bool AllowConcurrentWindows(void) { return true; }// XDM3038

private:
	//CTransparentPanel	*m_pBackgroundPanel;
	Label				m_LabelTitle;// XDM3038
	CDefaultTextPanel	m_TextPanel;
	CCustomScrollPanel	m_ScrollPanel;
	CommandButton		m_ButtonClose;
};

#endif // VGUI_MESSAGEWINDOW_H
