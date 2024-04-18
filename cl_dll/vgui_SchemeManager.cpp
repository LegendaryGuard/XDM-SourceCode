#include "hud.h"
#include "vgui_SchemeManager.h"
#include "cvardef.h"
#include "cl_util.h"


typedef struct tgaheader_s
{
	unsigned char	IdLength;
	unsigned char	ColorMap;
	unsigned char	DataType;
	unsigned char	unused[5];
	unsigned short	OriginX;
	unsigned short	OriginY;
	unsigned short	Width;
	unsigned short	Height;
	unsigned char	BPP;
	unsigned char	Description;
} tgaheader_t;

// XDM: HACK to return proper width of every non-latin character
class ProxyFont : public vgui::Font
{
public:
	ProxyFont(const char *name, void *pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol) : Font(name, pFileData, fileDataLen, tall, wide, rotation, weight, italic, underline, strikeout, symbol)
	{
		if (pFileData)
		{
			tgaheader_t *pHeader = (tgaheader_t *)pFileData;
			m_iCharWidth = pHeader->Width / 256;// we assume this NUMCHARS per bitmap
		}
		else if (wide > 0)
			m_iCharWidth = wide;
		else
			m_iCharWidth = (tall*3)/4;
	}

	virtual void getCharABCwide(int ch, int &a, int &b, int &c)
	{
		/*if (ch >= 0x80)// Cyrillic: 0xC0 - 0xFF
		{
			a = 0;
			b = m_iCharWidth;
			c = 0;
		}
		else*/
			Font::getCharABCwide(ch, a,b,c);
		if (b == 0 && ch != 0)
			b = m_iCharWidth;
	}

protected:
	short	m_iCharWidth;
};




cvar_t *g_pCvarBitmapFonts = NULL;

void Scheme_Init(void)
{
	DBG_PRINTF("Scheme_Init()\n");
	g_pCvarBitmapFonts = CVAR_CREATE("bitmapfonts",
#if defined (_WINDOWS)
		"0",
#else// there's no TTF support under linux
		"1",
#endif
		FCVAR_CLIENTDLL);
}

//-----------------------------------------------------------------------------
// Purpose: Scheme managers data container
//-----------------------------------------------------------------------------
CScheme::CScheme()
{
	schemeName[0] = 0;
	fontName[0] = 0;
	fontSize = 0;
	fontWeight = 0;
	font = NULL;
	ownFontPointer = false;
	FgColor.Set(0xFFAFAFAF);
	//BgColor.Set(0x00000000);
	FgColorArmed.SetWhite();
	//BgColorArmed.Set(0x00000000); 
	FgColorClicked.Set(0xFF7F7F7F);
	//BgColorClicked.Set(0x00000000);
	BorderColor.SetWhite();
	BorderColorArmed.SetWhite();
	BrightColor.SetWhite();
	DarkColor.SetBlack();
}

CScheme::~CScheme()
{
	if (ownFontPointer)// only delete our font pointer if we own it
		delete font;

	font = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: resolution information
//			!! needs to be shared out
//-----------------------------------------------------------------------------
static int g_ResArray[] =
{
	320,
	400,
	512,
	640,
	800,
	1024,
	1152,
	1280,
	1600,
	1920,
	2048,
	3072,
	4096
};

static size_t g_NumReses = sizeof(g_ResArray) / sizeof(int);

static byte *LoadFileByResolution(const char *filePrefix, int xRes, const char *filePostfix)
{
	if (xRes <= 0)
		xRes = ScreenWidth;// XDM3038: I've fixed this error globally, but still make a double check

	ASSERT(g_NumReses > 0);
	// find our resolution in the res array
	int resNum = g_NumReses - 1;
	while (g_ResArray[resNum] > xRes)
	{
		resNum--;
		if (resNum < 0)
			return NULL;
	}

	// try open the file
	byte *pFile = NULL;

	while (resNum >= 0)// XDM3037
	{
		// try load
		char fname[128];// XDM: 256 is too much!
		_snprintf(fname, 128, "%s%d%s", filePrefix, g_ResArray[resNum], filePostfix);
		pFile = gEngfuncs.COM_LoadFile(fname, 5, NULL);

		if (pFile)
			break;
		else
			gEngfuncs.Con_DPrintf("Unable to load \"%s\"\n", fname);// XDM

		if (resNum == 0)
			return NULL;

		resNum--;
	}
	return pFile;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes the scheme manager
//			loading the scheme files for the current resolution
// Input  : xRes, yRes - dimensions of output window
//-----------------------------------------------------------------------------
CSchemeManager::CSchemeManager(int xRes, int yRes)
{
	// basic setup
	m_pSchemeList = NULL;
	m_iNumSchemes = 0;
	m_xRes = xRes;
	m_yRes = yRes;

	LoadScheme();// XDM

	ASSERT(m_iNumSchemes > 0);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees all the memory used by the scheme manager
//-----------------------------------------------------------------------------
CSchemeManager::~CSchemeManager()
{
	delete [] m_pSchemeList;
	m_iNumSchemes = 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM: Load scheme from file
//-----------------------------------------------------------------------------
void CSchemeManager::LoadScheme(void)
{
	// find the closest matching scheme file to our resolution
	char str[80] = "schemes/";

	if (g_pCvarScheme && g_pCvarScheme->string != NULL && strlen(g_pCvarScheme->string) > 0)
	{
		strcat(str, g_pCvarScheme->string);
		conprintf(0, "CL: Loading color scheme from %s\n", str);// XDM
		strcat(str, "/");
	}

	char *pFile = (char*)LoadFileByResolution(str, m_xRes, ".txt");
	char *pFileStart = pFile;

	int fontFileLength;
	char fontFilename[256];// XDM3038
	byte *pFontData;

	//
	// Read the scheme descriptions from the text file, into a temporary array
	// format is simply:
	// <paramName name> = <paramValue>
	//
	// a <paramName name> of "SchemeName" signals a new scheme is being described
	//
	const static int numTmpSchemes = 64;
	static CScheme tmpSchemes[numTmpSchemes];
	memset(tmpSchemes, 0, sizeof(tmpSchemes));
	int currentScheme = -1;
	CScheme *pScheme = NULL;

	if (pFile)
	{
		char token[1024];
		// record what has been entered so we can create defaults from the different values
		bool hasFgColor = false, hasBgColor = false, hasFgColorArmed = false, hasBgColorArmed = false, hasFgColorClicked = false, hasBgColorClicked = false;
		pFile = gEngfuncs.COM_ParseFile(pFile, token);
		while (strlen(token) > 0 && (currentScheme < numTmpSchemes))
		{
			// get the paramName name
			static const size_t tokenSize = 64;
			char paramName[tokenSize], paramValue[tokenSize];

			strncpy(paramName, token, tokenSize);
			paramName[tokenSize-1] = 0; // ensure null termination

			// get the '=' character
			pFile = gEngfuncs.COM_ParseFile(pFile, token);
			if (_stricmp(token, "="))
			{
				if (currentScheme < 0)
					conprintf(0, "Error parsing font scheme text file at file start - expected '=', found '%s''\n", token);
				else
					conprintf(0, "Error parsing font scheme text file at scheme '%s' - expected '=', found '%s''\n", tmpSchemes[currentScheme].schemeName, token);

				break;
			}

			// get paramValue
			pFile = gEngfuncs.COM_ParseFile( pFile, token );
			strncpy( paramValue, token, tokenSize );
			paramValue[tokenSize-1] = 0; // ensure null termination

			// is this a new scheme?
			if (!_stricmp(paramName, "SchemeName"))
			{
				// setup the defaults for the current scheme
				if ( pScheme )
				{
					// foreground color defaults (normal -> armed -> mouse down)
					if ( !hasFgColor )
						pScheme->FgColor.SetWhite();

					if ( !hasFgColorArmed )
						pScheme->FgColorArmed = pScheme->FgColor;
						//memcpy( pScheme->FgColorArmed, pScheme->FgColor, sizeof(pScheme->FgColorArmed) );

					if ( !hasFgColorClicked )
						pScheme->FgColorClicked = pScheme->FgColorArmed;
						//memcpy( pScheme->FgColorClicked, pScheme->FgColorArmed, sizeof(pScheme->FgColorClicked) );

					// background color (normal -> armed -> mouse down)
					if ( !hasBgColor )
						pScheme->BgColor.Set(0,0,0,0);

					if ( !hasBgColorArmed )
						pScheme->BgColorArmed = pScheme->BgColor;
						//memcpy( pScheme->BgColorArmed, pScheme->BgColor, sizeof(pScheme->BgColorArmed) );

					if ( !hasBgColorClicked )
						pScheme->BgColorClicked = pScheme->BgColorArmed;
						//memcpy( pScheme->BgColorClicked, pScheme->BgColorArmed, sizeof(pScheme->BgColorClicked) );

					// font size
					if ( !pScheme->fontSize )
						pScheme->fontSize = 17;

					if ( !pScheme->fontName[0] )
						strcpy( pScheme->fontName, "Arial" );
				}

				// create the new scheme
				currentScheme++;
				pScheme = &tmpSchemes[currentScheme];
				hasFgColor = hasBgColor = hasFgColorArmed = hasBgColorArmed = hasFgColorClicked = hasBgColorClicked = false;

				strncpy(pScheme->schemeName, paramValue, SCHEME_NAME_LENGTH);
				pScheme->schemeName[SCHEME_NAME_LENGTH-1] = '\0'; // ensure null termination of string
			}

			if (!pScheme)
			{
				conprintf(0, "Font scheme text file MUST start with a 'SchemeName'\n");
				break;
			}

			// pull the data out into the scheme
			if (!_stricmp(paramName, "SchemeName"))
			{
				// WTF? we get here, skip and don't show any errors
			}
			else if (!_stricmp(paramName, "FontName"))
			{
				strncpy( pScheme->fontName, paramValue, FONT_NAME_LENGTH );
				pScheme->fontName[FONT_NAME_LENGTH-1] = 0;
			}
			else if (!_stricmp(paramName, "FontSize"))
			{
				pScheme->fontSize = atoi( paramValue );
			}
			else if (!_stricmp(paramName, "FontWeight"))
			{
				pScheme->fontWeight = atoi( paramValue );
			}
			else if (!_stricmp(paramName, "FgColor"))
			{
				StringToColor(paramValue, pScheme->FgColor);
				hasFgColor = true;
			}
			else if (!_stricmp(paramName, "BgColor"))
			{
				StringToColor(paramValue, pScheme->BgColor);
				hasBgColor = true;
			}
			else if (!_stricmp(paramName, "FgColorArmed"))
			{
				StringToColor(paramValue, pScheme->FgColorArmed);
				hasFgColorArmed = true;
			}	
			else if (!_stricmp(paramName, "BgColorArmed"))
			{
				StringToColor(paramValue, pScheme->BgColorArmed);
				hasBgColorArmed = true;
			}
			else if (!_stricmp(paramName, "FgColorMousedown"))
			{
				StringToColor(paramValue, pScheme->FgColorClicked);
				hasFgColorClicked = true;
			}
			else if (!_stricmp(paramName, "BgColorMousedown"))
			{
				StringToColor(paramValue, pScheme->BgColorClicked);
				hasBgColorClicked = true;
			}
			else if (!_stricmp(paramName, "BorderColor"))
			{
				StringToColor(paramValue, pScheme->BorderColor);
			}
			else if (!_stricmp(paramName, "BorderColorArmed"))
			{
				StringToColor(paramValue, pScheme->BorderColorArmed);
			}
			else if (!_stricmp(paramName, "Border"))
			{
				pScheme->BorderThickness = atoi(paramValue);
			}
			else if (!_stricmp(paramName, "BrightColor"))
			{
				StringToColor(paramValue, pScheme->BrightColor);
			}
			else if (!_stricmp(paramName, "DarkColor"))
			{
				StringToColor(paramValue, pScheme->DarkColor);
			}
			else
				CON_DPRINTF("CSchemeManager::LoadScheme(): unknown parameter: \"%s\" (%s) in scheme \"%s\"\n", paramName, paramValue, pScheme->schemeName);

			//CON_DPRINTF("CSchemeManager::LoadScheme(): parsed: %s = %s\n", paramName, paramValue);
			// get the new token last, so we now if the loop needs to be continued or not
			pFile = gEngfuncs.COM_ParseFile(pFile, token);
		}
		// free the file
		gEngfuncs.COM_FreeFile(pFileStart);
	}// XDM: buildDefaultFont:
	else
		return;

	// make sure we have at least 1 valid font
	if (currentScheme < 0)
	{
		currentScheme = 0;
		strcpy(tmpSchemes[0].schemeName, "Default Scheme");
		strcpy(tmpSchemes[0].fontName, "Arial");
		tmpSchemes[0].fontSize = 0;
		tmpSchemes[0].FgColor[0] = tmpSchemes[0].FgColor[1] = tmpSchemes[0].FgColor[2] = tmpSchemes[0].FgColor[3] = 255;
		tmpSchemes[0].FgColorArmed[0] = tmpSchemes[0].FgColorArmed[1] = tmpSchemes[0].FgColorArmed[2] = tmpSchemes[0].FgColorArmed[3] = 255;
		tmpSchemes[0].FgColorClicked[0] = tmpSchemes[0].FgColorClicked[1] = tmpSchemes[0].FgColorClicked[2] = tmpSchemes[0].FgColorClicked[3] = 255;
		CON_DPRINTF("Errors encountered while loading scheme \"%s\", defaults used.\n", str);
	}

	// we have the full list of schemes in the tmpSchemes array
	// now allocate the correct sized list
	m_iNumSchemes = currentScheme + 1; // 0-based index
	m_pSchemeList = new CScheme[m_iNumSchemes];

	// copy in the data
	memcpy(m_pSchemeList, tmpSchemes, sizeof(CScheme) * m_iNumSchemes);

	// create the fonts
	for (size_t i = 0; i < m_iNumSchemes; ++i)
	{
		m_pSchemeList[i].font = NULL;

		// see if the current font values exist in a previously loaded font
		for (size_t j = 0; j < i; ++j)
		{
			// check if the font name, size, and weight are the same
			if (!_stricmp(m_pSchemeList[i].fontName, m_pSchemeList[j].fontName)
				&& m_pSchemeList[i].fontSize == m_pSchemeList[j].fontSize
				&& m_pSchemeList[i].fontWeight == m_pSchemeList[j].fontWeight)
			{
				// copy the pointer, but mark i as not owning it
				m_pSchemeList[i].font = m_pSchemeList[j].font;
				m_pSchemeList[i].ownFontPointer = false;
			}
		}

		// if we haven't found the font already, load it ourselves
		if (!m_pSchemeList[i].font)
		{
			fontFileLength = -1;
			pFontData = NULL;

			if (g_pCvarBitmapFonts && g_pCvarBitmapFonts->value > 0)
			{
				int fontRes = 640;// HL20130901 HACK
				if ( m_xRes >= 1600 )
					fontRes = 1600;
				else if ( m_xRes >= 1280 )
					fontRes = 1280;
				else if ( m_xRes >= 1152 )
					fontRes = 1152;
				else if ( m_xRes >= 1024 )
					fontRes = 1024;
				else if ( m_xRes >= 800 )
					fontRes = 800;

				_snprintf(fontFilename, 256, "gfx\\vgui\\fonts\\%d_%s.tga", fontRes, m_pSchemeList[i].schemeName);
				//exact Xres	_snprintf(fontFilename, 256, "gfx\\vgui\\fonts\\%d_%s.tga", m_xRes, m_pSchemeList[i].schemeName);
				pFontData = gEngfuncs.COM_LoadFile( fontFilename, 5, &fontFileLength );
				if (!pFontData)
					conprintf(0, "CL: Warning: missing bitmap font: %s\n", fontFilename);
			}

			m_pSchemeList[i].font = new ProxyFont(//vgui::Font(
				m_pSchemeList[i].fontName,
				pFontData,
				fontFileLength,
				m_pSchemeList[i].fontSize,
				0,
				0,
				m_pSchemeList[i].fontWeight,
				false,
				false,
				false,
				false);

			m_pSchemeList[i].ownFontPointer = true;
		}

		// fix up alpha values; VGUI uses 1-A (A=0 being solid, A=255 transparent)
		/* XDM3038: store data normally, user should feed VGUI proper values
		m_pSchemeList[i].FgColor[3] = 255 - m_pSchemeList[i].FgColor[3];
		m_pSchemeList[i].BgColor[3] = 255 - m_pSchemeList[i].BgColor[3];
		m_pSchemeList[i].FgColorArmed[3] = 255 - m_pSchemeList[i].FgColorArmed[3];
		m_pSchemeList[i].BgColorArmed[3] = 255 - m_pSchemeList[i].BgColorArmed[3];
		m_pSchemeList[i].FgColorClicked[3] = 255 - m_pSchemeList[i].FgColorClicked[3];
		m_pSchemeList[i].BgColorClicked[3] = 255 - m_pSchemeList[i].BgColorClicked[3];*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finds a scheme in the list, by name
// Input  : char *schemeName - string name of the scheme
// Output : SchemeHandle_t handle to the scheme
//-----------------------------------------------------------------------------
SchemeHandle_t CSchemeManager::getSchemeHandle(const char *schemeName)
{
	// iterate through the list
	for (size_t i = 0; i < m_iNumSchemes; ++i)
	{
		if (!_stricmp(schemeName, m_pSchemeList[i].schemeName))
			return i;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a scheme in the list, by name
// Input  : char *schemeName - string name of the scheme
// Output : CScheme *
//-----------------------------------------------------------------------------
CScheme *CSchemeManager::getScheme(const char *schemeName)
{
	// iterate through the list
	for (size_t i = 0; i < m_iNumSchemes; ++i)
	{
		if (!_stricmp(schemeName, m_pSchemeList[i].schemeName))
			return &m_pSchemeList[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: always returns a valid scheme handle
// Input  : schemeHandle - 
// Output : CScheme
//-----------------------------------------------------------------------------
CScheme *CSchemeManager::getSafeScheme(SchemeHandle_t schemeHandle)
{
	if (schemeHandle < m_iNumSchemes)
		return m_pSchemeList + schemeHandle;

	return m_pSchemeList;
}
