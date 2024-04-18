//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef VGUI_SCHEMEMANAGER_H
#define VGUI_SCHEMEMANAGER_H

#include "color.h"
#include <VGUI_Font.h>
#include <VGUI_Color.h>


// handle to an individual scheme
typedef uint32 SchemeHandle_t;// XDM3037: probably obsolete


// Register console variables, etc..
void Scheme_Init();

#define SCHEME_NAME_LENGTH		32
#define FONT_NAME_LENGTH		48
#define FONT_FILENAME_LENGTH	64

class CScheme
{
public:
	// construction/destruction
	CScheme();
	~CScheme();

	// name
	char schemeName[SCHEME_NAME_LENGTH];
	// font
	char fontName[FONT_NAME_LENGTH];

	int fontSize;
	int fontWeight;

	vgui::Font *font;

	// scheme
	::Color FgColor;// not vgui::Color
	::Color BgColor;
	::Color FgColorArmed;
	::Color BgColorArmed; 
	::Color FgColorClicked;
	::Color BgColorClicked;
	::Color BorderColor;
	::Color BorderColorArmed;
	::Color BrightColor;
	::Color DarkColor;

	int BorderThickness;
	bool ownFontPointer; // true if the font is ours to delete
};


//-----------------------------------------------------------------------------
// Purpose: Handles the loading of text scheme description from disk
//			supports different font/color/size schemes at different resolutions 
//-----------------------------------------------------------------------------
class CSchemeManager
{
public:
	// initialization
	CSchemeManager(int xRes, int yRes);
	virtual ~CSchemeManager();

	// scheme handling
	SchemeHandle_t getSchemeHandle( const char *schemeName );
	CScheme *getScheme(const char *schemeName);
	CScheme *getSafeScheme( SchemeHandle_t schemeHandle );

	void LoadScheme(void);// XDM

private:
	CScheme *m_pSchemeList;
	size_t m_iNumSchemes;

	// Resolution we were initted at.
	int		m_xRes;
	int		m_yRes;
};


#endif // VGUI_SCHEMEMANAGER_H
