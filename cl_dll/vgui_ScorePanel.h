//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SCOREPANEL_H
#define SCOREPANEL_H

#include <ctype.h>
#include <VGUI_Panel.h>
#include <VGUI_TablePanel.h>
#include <VGUI_HeaderPanel.h>
#include <VGUI_TextGrid.h>
#include <VGUI_Label.h>
#include <VGUI_TextImage.h>
#include "../game_shared/vgui_listbox.h"
#include "../game_shared/vgui_grid.h"
#include "../game_shared/vgui_defaultinputsignal.h"
#include "vgui_CustomObjects.h"

// Scoreboard positions
// space to leave between scoreboard borders and screen borders
#define SBOARD_INDENT_X				XRES(104)
#define SBOARD_INDENT_Y				YRES(40)
// low-res scoreboard indents
#define SBOARD_INDENT_X_512			30
#define SBOARD_INDENT_Y_512			30
// ultra-low-res
#define SBOARD_INDENT_X_400			0
#define SBOARD_INDENT_Y_400			20

#define SBOARD_MAX_TEAMS			MAX_TEAMS+1

#define SBOARD_NUM_COLUMNS			8
#define SBOARD_NUM_ROWS				(MAX_PLAYERS + (4*SBOARD_MAX_TEAMS))

#define SBOARD_BOTTOM_TEXT_LEN		128

// Scoreboard dimensions
#define SBOARD_TITLE_SIZE_Y			YRES(22)
#define SBOARD_BOTTOM_LABEL_Y		YRES(8)
#define SBOARD_X_BORDER				XRES(4)

#define SB_LAST_KILLER_HIGHLIGHT_TIME	20

#define SBOARD_PANEL_ALPHA			95// in VGUI units
/*
#define SBOARD_COLOR_KILLER_BG_R		255
#define SBOARD_COLOR_KILLER_BG_G		0
#define SBOARD_COLOR_KILLER_BG_B		0
*/
#define SBOARD_COLOR_SPECTARGET_BG_R	127
#define SBOARD_COLOR_SPECTARGET_BG_G	127
#define SBOARD_COLOR_SPECTARGET_BG_B	207
#define SBOARD_COLOR_SPECTARGET_BG_A	191

// Scoreboard cells
typedef enum
{
	COLUMN_TRACKER = 0,
	COLUMN_NAME,
	COLUMN_TSCORE,
	COLUMN_KILLS,
	COLUMN_DEATHS,
	COLUMN_LATENCY,
	COLUMN_VOICE,
	COLUMN_BLANK
} sboard_column_types;

typedef enum
{
	SBOARD_ROW_PLAYER = 0,
	SBOARD_ROW_TEAM,
	//SBOARD_ROW_SPECTATORS,
	SBOARD_ROW_BLANK
} sboard_row_types;

enum
{
	SCORE_PANEL_CMD_PLAYERSTAT = VGUI_IDLAST,
};


typedef struct columninfo_s
{
	char				*m_pTitle;		// If null, ignore, if starts with #, it's localized, otherwise use the string directly.
	int					m_Width;		// Based on 640 width. Scaled to fit other resolutions.
	Label::Alignment	m_Alignment;	
} columninfo_t;



using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Custom label for cells in the Scoreboard's Table Header
//-----------------------------------------------------------------------------
class CTextImage2 : public Image
{
	typedef Image BaseClass;
public:
	CTextImage2()
	{
		_image[0] = new TextImage("");
		_image[1] = new TextImage("");
	}

	~CTextImage2()
	{
		if (_image[0])
			delete _image[0];

		if (_image[1])
			delete _image[1];
	}

	TextImage *GetImage(int image)
	{
		return _image[image];
	}

	virtual void getSize(int &wide, int &tall)
	{
		int w1, w2, t1, t2;
		if (_image[0])
			_image[0]->getTextSize(w1, t1);
		else
		{
			w1 = 0; t1 = 0;
		}
		if (_image[1])
			_image[1]->getTextSize(w2, t2);
		else
		{
			w2 = 0; t2 = 0;
		}
		wide = w1 + w2;
		tall = max(t1, t2);
		setSize(wide, tall);
	}

	virtual void doPaint(Panel *panel)
	{
		if (_image[0])
			_image[0]->doPaint(panel);
		if (_image[1])
			_image[1]->doPaint(panel);
	}

	virtual void setPos(int x, int y)
	{
		int swide, stall = 0;
		if (_image[0])
		{
			_image[0]->setPos(x, y);
			_image[0]->getSize(swide, stall);
		}
		if (_image[1])
		{
			int wide, tall;
			_image[1]->getSize(wide, tall);
			_image[1]->setPos(x + wide, y + (int)((float)stall * 0.9f) - tall);// XDM: TODO: revisit
		}
	}

	virtual void setColor(vgui::Color color)
	{
		if (_image[0])
			_image[0]->setColor(color);
	}

	virtual void setColor2(vgui::Color color)
	{
		if (_image[1])
			_image[1]->setColor(color);
	}

private:
	TextImage *_image[2];

};

//-----------------------------------------------------------------------------
// Purpose: Custom label for cells in the Scoreboard's Table Header
//-----------------------------------------------------------------------------
class CLabelHeader : public Label
{
	typedef Label BaseClass;
public:
	CLabelHeader() : Label("")
	{
		_dualImage = new CTextImage2();
		if (_dualImage)
		{
			_dualImage->setColor2(vgui::Color(0, 255, 0, 0));
			_row = -2;
			_useFgColorAsImageColor = true;
		}
		_offset[0] = 0;
		_offset[1] = 0;
	}

	~CLabelHeader()
	{
		if (_dualImage)
			delete _dualImage;
	}

	inline void setRow(int row)
	{
		_row = row;
	}

	inline void setFgColorAsImageColor(bool state)
	{
		_useFgColorAsImageColor = state;
	}

	// name warning: recursion from base classes!
	virtual void setText1(const char *text)
	{
		if (_dualImage)
		{
			_dualImage->GetImage(0)->setText(text);
			// calculate the text size
			Font *font = _dualImage->GetImage(0)->getFont();
			_gap = 0;
			if (font)
			{
				int a, b, c;
				for (const char *ch = text; *ch != 0; ++ch)
				{
					font->getCharABCwide(*ch, a, b, c);
					_gap += (a + b + c);
				}
			}
			_gap += XRES(5);
		}
	}

	virtual void setTextSimple(const char *text)
	{
		// strip any non-alnum characters from the end
		char buf[512];
		strncpy(buf, text, 512);//strcpy(buf, text);
		buf[511] = '\0';
		size_t len = strlen(buf);
		while (len > 0 && isspace((int)buf[--len]))
			buf[len] = 0;

		setText1(buf);
	}

	/* recursion from base classes! virtual void setText(const char* format,...)
	{
		char buf[512];
		va_list argptr;
		va_start(argptr, format);
		int len = _vsnprintf(buf, 512, format, argptr);
		va_end(argptr);
		while (len > 0 && isspace((int)buf[--len]))
			buf[len] = 0;
		setText(512, buf);
	}*/

	virtual void setText2(const char *text)
	{
		if (_dualImage)
			_dualImage->GetImage(1)->setText(text);
	}

	virtual void getTextSize(int &wide, int &tall)
	{
		if (_dualImage)
			_dualImage->getSize(wide, tall);
	}

	virtual void setFgColor(int r,int g,int b,int a)
	{
		Label::setFgColor(r,g,b,a);
		vgui::Color color(r,g,b,a);
		if (_dualImage)
		{
			_dualImage->setColor(color);
			_dualImage->setColor2(color);
		}
		repaint();
	}

	virtual void setFgColor(Scheme::SchemeColor sc)
	{
		int r, g, b, a;
		Label::setFgColor(sc);
		Label::getFgColor(r, g, b, a);
		// Call the r,g,b,a version so it sets the color in the dualImage..
		setFgColor(r, g, b, a);
	}

	virtual void setFont(Font *font)
	{
		if (_dualImage)
			_dualImage->GetImage(0)->setFont(font);
	}

	virtual void setFont2(Font *font)
	{
		if (_dualImage)
			_dualImage->GetImage(1)->setFont(font);
	}

	// this adjust the absolute position of the text after alignment is calculated
	virtual void setTextOffset(int x, int y)
	{
		_offset[0] = x;
		_offset[1] = y;
	}

	virtual void paint();
	virtual void paintBackground();
	virtual void calcAlignment(const int &iwide, const int &itall, int &x, int &y);

private:
	//ScorePanel *m_pScorePanel;
	CTextImage2 *_dualImage;
	int _row;
	int _gap;
	int _offset[2];
	bool _useFgColorAsImageColor;
};



//-----------------------------------------------------------------------------
// Purpose: Scoreboard back panel
//-----------------------------------------------------------------------------
class ScorePanel : public CMenuPanel, public vgui::CDefaultInputSignal
{
	friend class CLabelHeader;
	typedef CMenuPanel BaseClass;
private:

	CLabelHeader *GetPlayerEntry(const int &x, const int &y)	{return &m_PlayerEntries[x][y];}

public:
	ScorePanel(int x,int y,int wide,int tall);
	virtual void Open(void);
	virtual bool OnClose(void);

	// InputSignal overrides.
	//virtual void cursorExited(Panel *panel);
	virtual void mousePressed(MouseCode code, Panel *panel);
	virtual void cursorMoved(int x, int y, Panel *panel);

	virtual void OnActionSignal(int signal, class CMenuPanelActionSignalHandler *pCaller);
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);// XDM3037a

	virtual bool IsPersistent(void);// XDM3038
	virtual bool AllowConcurrentWindows(void);// XDM3038

	void Initialize(void);
	void Update(void);
	void UpdateCounters(void);// update time/frags left counters

	void SortTeams(void);
	void SortPlayers(/*const int &iRowType, */const TEAM_ID &teamindex, bool bIncludeSpectators);
	void FillGrid(void);

	void SetClickMode(bool bOn);
	void SetTitle(const char *pText, const float &duration);// XDM3038a

	void DeathMsg(short killer, short victim);
	void MouseOverCell(const int &row, const int &col);
	void ShowStats(int cl_index);
	void ShowStatsMenu(void);
	void DumpInfo(void);// XDM: debug

	CLIENT_INDEX GetBestPlayer(void);
	TEAM_ID GetBestTeam(void);

	void AddRow(int iRowData, int iRowType);// XDM3035c
	void RecievePlayerStats(int cl_index);// XDM3037
	void CheckStatsPanel(void);// XDM3037

	//int				m_iShowscoresHeld;
	size_t			m_iRows;
	int				m_iSortedRows[SBOARD_NUM_ROWS];// contains player or team indexes
	//bool			m_bHasBeenSorted[MAX_PLAYERS+1];// XDM3035
	//bool			m_bHasBeenSortedTeam[MAX_TEAMS+1];// XDM3035a
	CLIENT_INDEX	m_iBestPlayer[MAX_TEAMS+1];// XDM3037: can be implemented without modifying sorting algoritm
	CLIENT_INDEX	m_iWinner;// XDM3037: explicit
	CLIENT_INDEX	m_iServerBestPlayer;// XDM3037
	CLIENT_INDEX	m_iLastKilledBy;
	float			m_fLastKillDisplayStopTime;// XDM: was int
	float			m_fTitleNextUpdateTime;// XDM3038a: use to freeze some text
	float			m_flWaitForStatsTime;// XDM3038c
	TEAM_ID			m_SortedTeams[MAX_TEAMS+1];// +one for spectators
	short			m_SortedPlayers[MAX_PLAYERS];// max of 32 players
	unsigned short	m_iRowType[SBOARD_NUM_ROWS];// XDM3035a
	TEAM_ID			m_iServerBestTeam;// XDM3037

protected:
	char			m_szScoreLimitLabelFmt[SBOARD_BOTTOM_TEXT_LEN];// XDM3035: localized string
	Label			m_TitleLabel;
	Label			m_BottomLabel;// XDM3035
	Label			m_CurrentTimeLabel;

	// Here is how these controls are arranged hierarchically.
	// m_HeaderGrid
	//     m_HeaderLabels
	CGrid			m_HeaderGrid;
	CLabelHeader	m_HeaderLabels[SBOARD_NUM_COLUMNS];// Labels above the grid
	CLabelHeader	*m_pCurrentHighlightLabel;// just a pointer to a highlighted label
	int				m_iHighlightRow;

	// m_PlayerList
	//     m_PlayerGrids
	//         m_PlayerEntries 
	CListBox		m_PlayerList;
	CGrid			m_PlayerGrids[SBOARD_NUM_ROWS];// The grid with player and team info. 
	CLabelHeader	m_PlayerEntries[SBOARD_NUM_COLUMNS][SBOARD_NUM_ROWS];// Labels for the grid entries.

	CHitTestPanel	m_HitTestPanel;
	CommandButton	m_ButtonClose;
	CommandButton	m_ButtonStatsMode;// XDM3037

	CScheme			*m_pTextScheme;// XDM3037
	Font			*m_pFontScore;
	Font			*m_pFontTitle;
	Font			*m_pFontSmall;

	CPlayersCommandMenu	*m_pPlayersMenu;// XDM3038c
	CMenuPanel		*m_pStatsPanel;// XDM3037
	char			m_StatsWindowTextBuffer[1024];// XDM3037
	bool			m_bHasStats;// XDM3037
};

#endif // SCOREPANEL_H
