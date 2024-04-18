#ifndef VGUI_STATSPANEL_H
#define VGUI_STATSPANEL_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "../game_shared/vgui_listbox.h"
#include "../game_shared/vgui_grid.h"
//#include "playerstats.h"// XDM3037

#define STATSPANEL_WINDOW_X			112
#define STATSPANEL_WINDOW_Y			80
#define STATSPANEL_WINDOW_SIZE_X	480// in 640x480 space XRES
#define STATSPANEL_WINDOW_SIZE_Y	360

#define STATSPANEL_BORDER			16
#define STATSPANEL_NUM_COLUMNS		3// description+players
#define STATSPANEL_NUM_ROWS			STAT_NUMELEMENTS+1// +title

//-----------------------------------------------------------------------------
// Basic text window with close button
//-----------------------------------------------------------------------------
class CStatsPanel : public CMenuPanel
{
public:
	CStatsPanel(bool ShadeFullScreen, int iRemoveMe, int x, int y, int wide, int tall);
	virtual void Open(void);// XDM
	virtual void Close(void);
//	virtual bool IsPersistent(void) { return false; }// XDM3038
//	virtual bool AllowConcurrentWindows(void) { return true; }// XDM3038

private:
	Label			*m_pLabelTitle;
	CommandButton	*m_pButtonClose;
	CommandButton	*m_pCheckButtonSave;

	CListBox		m_DataList;
	CGrid			m_DataGrids[STATSPANEL_NUM_COLUMNS];
	Label			m_DataEntries[STATSPANEL_NUM_COLUMNS][STATSPANEL_NUM_ROWS];
};

extern struct cvar_s *g_pCvarLogStats;

#endif // VGUI_STATSPANEL_H
