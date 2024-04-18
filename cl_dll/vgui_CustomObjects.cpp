#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "cvardef.h"
#include "in_defs.h"
#include "vgui_Int.h"
#include "VGUI_Font.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "../game_shared/vgui_loadtga.h"


/* TODO
int ReadBMP(const char *szFile, BYTE **ppbBits, RGBQUAD *pRGBAPalette256, int &width, int &height)
{
	int i, rc = 0;
	FILE *pfile = NULL;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD rgrgbPalette[256];
	ULONG cbBmpBits = 0;
	BYTE *pbBmpBits = NULL;
	BYTE *pbReadBits = NULL;
	ULONG cbPalBytes = 0;
	ULONG biTrueWidth = 0;

	if (szFile == NULL || *ppbBits != NULL || pRGBAPalette256 == NULL)
		{ printf("invalid BMP file\n"); rc = -1000; goto GetOut; }

	if ((pfile = fopen(szFile, "rb")) == NULL)
		{ printf("unable to open BMP file\n"); rc = -1; goto GetOut; }

	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	memset(rgrgbPalette, 0, sizeof(RGBQUAD)*256);

	if (fread(&bmfh, sizeof bmfh, 1, pfile) != 1)
		{ rc = -2; goto GetOut; }

	if (!(bmfh.bfReserved1 == 0 && bmfh.bfReserved2 == 0))// Bogus file header check
		{ rc = -2000; goto GetOut; }

	if (fread(&bmih, sizeof bmih, 1, pfile) != 1)// Read info header
		{ rc = -3; goto GetOut; }

	if (!(bmih.biSize == sizeof bmih && bmih.biPlanes == 1))// Bogus info header check
	{
		printf("invalid BMP file header\n");
		rc = -4;
		goto GetOut;
	}

	if (bmih.biBitCount != 8)// Bogus bit depth?  Only 8-bit supported.
	{
		printf("BMP file not 8 bit\n");
		rc = -5;
		goto GetOut;
	}

	if (bmih.biCompression != BI_RGB)// Bogus compression?  Only non-compressed supported.
		{ printf("invalid BMP compression type\n"); rc = -6; goto GetOut; }

	if (bmih.biClrUsed == 0)// Figure out how many entires are actually in the table
	{
		bmih.biClrUsed = 256;
		cbPalBytes = (1 << bmih.biBitCount) * sizeof( RGBQUAD );
	}
	else 
		cbPalBytes = bmih.biClrUsed * sizeof( RGBQUAD );

	// Read palette (bmih.biClrUsed entries)
	if (fread(rgrgbPalette, cbPalBytes, 1, pfile) != 1)
		{ rc = -6; goto GetOut; }

	for (i = 0; i < (int)bmih.biClrUsed; i++)// Copy over used entries
	{
		pRGBAPalette256[i].rgbRed = rgrgbPalette[i].rgbRed;
		pRGBAPalette256[i].rgbGreen = rgrgbPalette[i].rgbGreen;
		pRGBAPalette256[i].rgbBlue = rgrgbPalette[i].rgbBlue;
	}
	for (i = bmih.biClrUsed; i < 256; i++)// unused entires 0,0,0
	{
		pRGBAPalette256[i].rgbRed = 0;
		pRGBAPalette256[i].rgbGreen = 0;
		pRGBAPalette256[i].rgbBlue = 0;
	}

	// Read bitmap bits (remainder of file)
	cbBmpBits = bmfh.bfSize - ftell(pfile);
	pbReadBits = (unsigned char *)malloc(cbBmpBits);
	memset(pbReadBits, 0, cbBmpBits);
	if (fread(pbReadBits, cbBmpBits, 1, pfile) != 1)
	{
		rc = -7;
		goto GetOut;
	}

	pbBmpBits = new byte[cbBmpBits];
	memset(pbBmpBits, 0, cbBmpBits);

	biTrueWidth = (bmih.biWidth + 3) & ~3;// data is actually stored with the width being rounded up to a multiple of 4

	// reverse the order of the data
	// (vertical flip & copy)
	for(i = 0; i < bmih.biHeight; i++)
	{
		// when copying, we need to ignore redundant pixels!
		memcpy(&pbBmpBits[i*bmih.biWidth], &pbReadBits[biTrueWidth*(bmih.biHeight-1-i)], bmih.biWidth);
	}

	width = (WORD)bmih.biWidth;
	height = (WORD)bmih.biHeight;
	*ppbBits = pbBmpBits;

GetOut:
	if (pfile)
	{
		fclose(pfile);
		pfile = NULL;
	}
	if (pbReadBits)
	{
		free(pbReadBits);
		pbReadBits = NULL;
	}
	return rc;
}*/

namespace vgui
{

class Panel;
class InputStream;

class BitmapBMP : public Bitmap
{
public:
	BitmapBMP(byte *pData);
	~BitmapBMP();
	virtual void paint(Panel *panel);

private:
	byte *m_pData;
	size_t m_Width;
	size_t m_Height;
	//_size[0] is private :(
};

// pData is raw file data whish is needed to be parsed
BitmapBMP::BitmapBMP(byte *pData)
{
	m_Width = 4;
	m_Height = 4;
	/*int r = 0;
	BYTE *bitmap = NULL;
	RGBQUAD rgbpal256[256];
	memset(rgbpal256, 0, sizeof(RGBQUAD)*256);
	ReadBMP(infilename, m_pData, rgbpal256, m_Width, m_Height);*/
	m_pData = new byte[m_Width*m_Height];
	strcpy((char *)m_pData, "CANNOT LOAD BITMAPS YET, SORRY :(");// TEST
}

BitmapBMP::~BitmapBMP()
{
	if (m_pData)
	{
		delete [] m_pData;
		m_pData = NULL;
	}
}

void BitmapBMP::paint(Panel *panel)
{
	if (panel && m_pData)
	{
		drawSetTextureRGBA(0, (char *)m_pData, m_Width, m_Height);
		int x, y;
		panel->getPos(x, y);
		drawTexturedRect(x, y, panel->getWide(), panel->getTall());
	}
}

}

vgui::BitmapBMP *vgui_LoadBMP(char const *pFilename)
{
	if (pFilename == NULL)
		return NULL;

	if (gEngfuncs.COM_LoadFile == NULL)// function may not exist
		return NULL;

	int iDataLen = 0;
	byte *pData = gEngfuncs.COM_LoadFile((char*)pFilename, 5, &iDataLen);
	if (!pData)
		return NULL;

	vgui::BitmapBMP *pRet = new vgui::BitmapBMP(pData);
	gEngfuncs.COM_FreeFile(pData);
	return pRet;
}




// Arrow filenames XDM3035a: updated for new HL engine
char *sArrowFilenames[] =
{
	"arrowup",
	"arrowdown", 
	"arrowleft",
	"arrowright", 
};

//-----------------------------------------------------------------------------
// Purpose: Loads a .tga file and returns a pointer to the VGUI tga object
// Path is gfx/vgui/
//-----------------------------------------------------------------------------
BitmapTGA *LoadTGAForRes(const char *pImageName)
{
	if (pImageName == NULL)
		return NULL;

	char sz[256];
	BitmapTGA *pTGA = NULL;

	// XDM3038: try loading images by screen resolution
	int d = 0;
	for (short i=0; i<2; ++i)
	{
		if (i == 0)
			d = ScreenWidth;
		else if (i == 1)// fall back to old 320/640 mechanism
			d = gHUD.m_iRes;
		//else
		//	vgui_LoadTGA("gfx/vgui/"pImageName);

		_snprintf(sz, 256, "gfx/vgui/%d_%s.tga", d, pImageName);
		pTGA = vgui_LoadTGA(sz);
		if (pTGA)// success
			break;
	}
	/*_snprintf(sz, 256, "gfx/vgui/%d_%s.tga", ScreenWidth, pImageName);
	pTGA = vgui_LoadTGA(sz);
	if (pTGA == NULL)// fall back to old 320/640 mechanism
	{
		_snprintf(sz, 256, "gfx/vgui/%d_%s.tga", gHUD.m_iRes, pImageName);
		pTGA = vgui_LoadTGA(sz);
	}*/
	if (pTGA == NULL)
		conprintf(1, "Error: LoadTGAForRes(%s) failed!\n", pImageName);

	return pTGA;
}





//-----------------------------------------------------------------------------
// Purpose: forward mouse signals
// Input  : code - 
//-----------------------------------------------------------------------------
void CHitTestPanel::internalMousePressed(MouseCode code)
{
	for (int i=0;i<_inputSignalDar.getCount();++i)
		_inputSignalDar[i]->mousePressed(code,this);
}



//-----------------------------------------------------------------------------
// Purpose: defcon
//-----------------------------------------------------------------------------
CommandButton::CommandButton(void) : Button("",0,0,1,1)
{
	setPaintBackgroundEnabled(true);// call paintBackground()
	m_bNoHighlight = false;
	m_bShowHotKey = true;
	m_bFlat = false;
	m_pIcon = NULL;// XDM3035a
	m_sMainText[0] = NULL;
	//m_sTipText[0] = NULL;
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	if (pSchemes)
		SetScheme(pSchemes->getScheme("Primary Button Text"));

	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Base class for all HUD buttons
// Input  : *text - 
//			x y - px
//			wide tall - px
//			bNoHighlight - don't add CHandler_CommandButtonHighlight input signal
//			bFlat - no border
//			bShowHotKey - 
//			*iconfile - [optional] icon full path
//			*pScheme - [optional] custom scheme
//-----------------------------------------------------------------------------
CommandButton::CommandButton(const char *text, int x, int y, int wide, int tall, bool bNoHighlight, bool bFlat, bool bShowHotKey, const char *iconfile, CScheme *pScheme) : Button("",x,y,wide,tall)
{
	m_bFlat = bFlat;
	m_bNoHighlight = bNoHighlight;
	m_pIcon = NULL;
	//m_sTipText[0] = NULL;

	if (iconfile)// XDM3035a
		LoadIcon(iconfile, true);

	if (pScheme)
	{
		SetScheme(pScheme);
	}
	else
	{
		CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
		SetScheme(pSchemes->getScheme("Primary Button Text"));
	}
	setPaintBackgroundEnabled(true);// call paintBackground()
	Init();
	m_bShowHotKey = bShowHotKey;
	//if (m_pIcon == NULL)
		setMainText(text);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CommandButton::Init(void)
{
	m_ulID = 0;
	m_pSubMenu = NULL;
	m_pSubLabel = NULL;
	m_pParentMenu = NULL;

	// left align
	setContentAlignment(vgui::Label::a_west);

	if (m_pIcon)// XDM3035a
		setImage(m_pIcon);

	// Add the Highlight signal
	if (!m_bNoHighlight)
		addInputSignal(new CHandler_CommandButtonHighlight(this));

	// not bound to any button yet
	m_iBoundKey = 0;
	//m_bShowHotKey = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pLocalScheme - 
//-----------------------------------------------------------------------------
void CommandButton::SetScheme(CScheme *pLocalScheme)// XDM3037
{
	if (pLocalScheme)
	{
		m_ColorNormal = pLocalScheme->FgColor;
		m_ColorArmed = pLocalScheme->FgColorArmed;
		m_ColorSelected = pLocalScheme->FgColorClicked;
		m_ColorBgNormal = pLocalScheme->BgColor;
		m_ColorBgArmed = pLocalScheme->BgColorArmed;
		m_ColorBgSelected = pLocalScheme->BgColorClicked;
		m_ColorBorderNormal = pLocalScheme->BorderColor;
		m_ColorBorderArmed = pLocalScheme->BorderColorArmed;
		setFont(pLocalScheme->font);
	}
	setBgColor(m_ColorBgNormal.r, m_ColorBgNormal.g, m_ColorBgNormal.b, 255-m_ColorBgNormal.a);
	setFgColor(m_ColorNormal.r, m_ColorNormal.g, m_ColorNormal.b, 255-m_ColorNormal.a);
}

//-----------------------------------------------------------------------------
// Purpose: Prepends the button text with the current bound key
//			if no bound key, then a clear space ' ' instead
//-----------------------------------------------------------------------------
void CommandButton::RecalculateText(void)
{
	if (m_bShowHotKey)// draw blank space to distinguish hotkeyed buttons
	{
		char szBuf[128];
		char ch;
		if (m_iBoundKey != 0 && isprint(m_iBoundKey))//(_isctype(m_cBoundKey, _UPPER|_LOWER|_DIGIT))// XDM3038a: don't show unprintable stuff anyway
			ch = (char)m_iBoundKey;
		else
			ch = ' ';

		_snprintf(szBuf, 128, " %c %s", ch, m_sMainText);
		//char *s = strchr(szBuf, '\%');// while?
		//if(s)*s = 0;
		szBuf[127] = 0;
 		Button::setText(128, szBuf);// no need for "%s" here
	}
	else
		Button::setText("%s", m_sMainText);
}

//-----------------------------------------------------------------------------
// Purpose: Override for vgui::Label
// Input  : *textBufferLen - 
//			*text - 
//-----------------------------------------------------------------------------
/*void CommandButton::setText(int textBufferLen, const char* text)
{
	strncpy(m_sMainText, text, min(textBufferLen,MAX_BUTTON_TEXTLEN));
	m_sMainText[MAX_BUTTON_TEXTLEN-1] = 0;
	RecalculateText();
}*/

//-----------------------------------------------------------------------------
// Purpose: Override for vgui::Label
// Input  : *format
//-----------------------------------------------------------------------------
/*void CommandButton::setText(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	_vsnprintf(m_sMainText, MAX_BUTTON_TEXTLEN, format, argptr);
	va_end(argptr);
	m_sMainText[MAX_BUTTON_TEXTLEN-1] = 0;
	RecalculateText();
}*/

//-----------------------------------------------------------------------------
// Purpose: Set user text, hotkey and number will be added automatically
// Input  : *text
//-----------------------------------------------------------------------------
void CommandButton::setMainText(const char *text)
{
	strncpy(m_sMainText, text, MAX_BUTTON_TEXTLEN);
	m_sMainText[MAX_BUTTON_TEXTLEN-1] = 0;
	RecalculateText();
}

//-----------------------------------------------------------------------------
// Purpose: Set tool top text
// Input  : *text
//-----------------------------------------------------------------------------
void CommandButton::setToolTip(const char *text)
{
	//strncpy(m_sTipText, text, MAX_BUTTON_TEXTLEN);
	//m_sTipText[MAX_BUTTON_TEXTLEN-1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Custom bound key
// Input  : boundKey - char
//-----------------------------------------------------------------------------
void CommandButton::setBoundKey(int boundKey)
{
	m_iBoundKey = boundKey;
	RecalculateText();
}

//-----------------------------------------------------------------------------
// Purpose: Get custom bound key
//-----------------------------------------------------------------------------
int CommandButton::getBoundKey(void)
{
	return m_iBoundKey;
}

//-----------------------------------------------------------------------------
// Purpose: Overlay icon
// Input  : *pIcon - set from a valid pointer
//-----------------------------------------------------------------------------
void CommandButton::SetIcon(BitmapTGA *pIcon)
{
	if (m_pIcon)
		delete m_pIcon;

	m_pIcon = pIcon;
	//no	int w,t;
	//	m_pIcon->getSize(w, t);
	//	setSize(XRES(w), YRES(t));
	//?	setImage(m_pIcon);
}

//-----------------------------------------------------------------------------
// Purpose: Button icon
// Input  : *pFileName - load from file
//			fullpath - if false, default vgui path (+ resolution) will be constructed
//-----------------------------------------------------------------------------
void CommandButton::LoadIcon(const char *pFileName, bool fullpath)
{
	if (pFileName)
	{
		BitmapTGA *pImage;
		// Load the Image
		if (fullpath)// don't make path, it's already specified
			pImage = vgui_LoadTGA(pFileName);
		else
			pImage = LoadTGAForRes(pFileName);

		if (pImage)
			SetIcon(pImage);
	}
}

//-----------------------------------------------------------------------------
// Purpose: AddSubMenu
//-----------------------------------------------------------------------------
void CommandButton::AddSubMenu(CCommandMenu *pNewMenu)
{
	m_pSubMenu = pNewMenu;
	// Prevent this button from being pushed
	setMouseClickEnabled(MOUSE_LEFT, false);
}

//-----------------------------------------------------------------------------
// Purpose: AddSubLabel
//-----------------------------------------------------------------------------
void CommandButton::AddSubLabel(CommandLabel *pSubLabel)
{
	m_pSubLabel = pSubLabel;
}

//-----------------------------------------------------------------------------
// Purpose: UpdateSubMenus
// Input  : iAdjustment - iYOffset
//-----------------------------------------------------------------------------
void CommandButton::UpdateSubMenus(int iAdjustment)
{
	if (m_pSubMenu)
		m_pSubMenu->RecalculatePositions(iAdjustment);
}

//-----------------------------------------------------------------------------
// Purpose: Custom paint to draw sublabel and set proper colors
//-----------------------------------------------------------------------------
void CommandButton::paint(void)
{
	if (m_pSubLabel)// Make the sub label paint the same as the button
	{
		if (isSelected())
			m_pSubLabel->PushDown();
		else
			m_pSubLabel->PushUp();
	}

	if (isSelected())
		setFgColor(m_ColorSelected.r, m_ColorSelected.g, m_ColorSelected.b, 255-m_ColorSelected.a);
	else if (isArmed())
		setFgColor(m_ColorArmed.r, m_ColorArmed.g, m_ColorArmed.b, 255-m_ColorArmed.a);
	else
		setFgColor(m_ColorNormal.r, m_ColorNormal.g, m_ColorNormal.b, 255-m_ColorNormal.a);

	Button::paint();

	//if (m_pIcon)
	//	m_pIcon->doPaint(this);

	/* UNDONE if (m_bTipVisible && m_sTipText[0])
	{
		int x,y, w,h;
		getBounds(x,y,w,h);
		drawSetColor(m_ColorBgNormal.r, m_ColorBgNormal.g, m_ColorBgNormal.b, 255-m_ColorBgNormal.a+32);
		//drawFilledRect(x+h,y-h/2, w,h);
		drawOutlinedRect(x+h,y-h/2, w,h);
		drawPrintText(x+h,y-h/2, m_sTipText, strlen(m_sTipText));
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Custom paintBackground to draw the icon
//-----------------------------------------------------------------------------
void CommandButton::paintBackground(void)
{
	//no access	if (_paintBackgroundEnabled)// already checked
	{
		if (isSelected())
			drawSetColor(m_ColorBgSelected.r, m_ColorBgSelected.g, m_ColorBgSelected.b, 255-m_ColorBgSelected.a);
		else if (isArmed())
			drawSetColor(m_ColorBgArmed.r, m_ColorBgArmed.g, m_ColorBgArmed.b, 255-m_ColorBgArmed.a);
		else
			drawSetColor(m_ColorBgNormal.r, m_ColorBgNormal.g, m_ColorBgNormal.b, 255-m_ColorBgNormal.a);

	//	Button::paintBackground();// unfortunately, it's a mess

		drawFilledRect(0,0,_size[0],_size[1]);
	}
	if (/*_paintBorderEnabled && */m_bFlat == false && _border == NULL)// XDM: flat means no border! Don't override custom border if set
	{
		if (isArmed())
			drawSetColor(m_ColorBorderArmed.r, m_ColorBorderArmed.g, m_ColorBorderArmed.b, 255-m_ColorBorderArmed.a);
		else
			drawSetColor(m_ColorBorderNormal.r, m_ColorBorderNormal.g, m_ColorBorderNormal.b, 255-m_ColorBorderNormal.a);

		drawOutlinedRect(0,0,_size[0],_size[1]);
	}

	if (m_pIcon)
		m_pIcon->doPaint(this);
}

//-----------------------------------------------------------------------------
// Purpose: Highlights the current button, and all it's parent menus
//-----------------------------------------------------------------------------
void CommandButton::cursorEntered(void)
{
	// unarm all the other buttons in this menu
	CCommandMenu *containingMenu = getParentMenu();
	if (containingMenu)
	{
		containingMenu->ClearButtonsOfArmedState();
		// make all our higher buttons armed
		CCommandMenu *pCParent = containingMenu->GetParentMenu();
		if (pCParent)
		{
			CommandButton *pParentButton = pCParent->FindButtonWithSubmenu(containingMenu);
			pParentButton->cursorEntered();
		}
	}

	PlaySound("vgui/button_enter.wav", VOL_NORM);// XDM

	// UNDONE if (m_sTipText[0])
	//	m_bTipVisible = true;

	// arm ourselves
	setArmed(true);
}

//-----------------------------------------------------------------------------
// Purpose: cursorExited
//-----------------------------------------------------------------------------
void CommandButton::cursorExited(void)
{
	// only clear ourselves if we have do not have a containing menu
	// only stay armed if we have a sub menu
	// the buttons only unarm themselves when another button is armed instead
	if (!getParentMenu() || !GetSubMenu())
	{
		m_bTipVisible = false;
		// TODO: this MAY be the way to skip this sound when a child window opens
		//if (hasFocus() || getParent() && getParent()->hasFocus())
		if (isArmed())
			PlaySound("vgui/button_exit.wav", VOL_NORM);// XDM

		setArmed(false);
		setSelected(false);// XDM3035c: un-stuck hack
	}
}

//-----------------------------------------------------------------------------
// Purpose: custom callback, used by CMenuActionHandler::actionPerformed
//-----------------------------------------------------------------------------
void CommandButton::OnActionPerformed(class CMenuActionHandler *pCaller)
{
	setArmed(false);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the command menu that the button is part of, if any
// Output : CCommandMenu *
//-----------------------------------------------------------------------------
CCommandMenu *CommandButton::getParentMenu(void)
{ 
	return m_pParentMenu; 
}

//-----------------------------------------------------------------------------
// Purpose: Sets the menu that contains this button
// Input  : *pParentMenu - 
//-----------------------------------------------------------------------------
void CommandButton::setParentMenu(CCommandMenu *pParentMenu)
{
	m_pParentMenu = pParentMenu;
}




//-----------------------------------------------------------------------------
// Purpose: ToggleCommandButton constructor
// Input  : state - 
//			*text - 
//			x y - 
//			wide tall - 
//			flat - 
//			bShowHotKey - 
//-----------------------------------------------------------------------------
ToggleCommandButton::ToggleCommandButton(bool state, const char *text, int x, int y, int wide, int tall, bool flat, bool bShowHotKey, CScheme *pScheme) : CommandButton(text, x, y, wide, tall, false, flat, bShowHotKey, NULL, pScheme)
{
	m_bState = state;

	// Put a > to show it's a submenu
	pLabelOn = new CImageLabel("checked", 0, 0);
	pLabelOn->setParent(this);
	//pLabelOn->setEnabled(false);
	pLabelOn->addInputSignal(this);

	pLabelOff = new CImageLabel("unchecked", 0, 0);
	pLabelOff->setParent(this);
	pLabelOff->setEnabled(true);
	pLabelOff->addInputSignal(this);

	int margin = (tall - pLabelOn->getTall())/2;
	pLabelOn->setPos(wide - margin - pLabelOn->getWide(), margin);
	pLabelOff->setPos(wide - margin - pLabelOff->getWide(), margin);

	/*int textwide, texttall;
	getTextSize(textwide, texttall);
	// Reposition
	pLabelOn->setPos(textwide, (tall - pLabelOn->getTall()) / 2);
	pLabelOff->setPos(textwide, (tall - pLabelOff->getTall()) / 2);*/
}

/*void ToggleCommandButton::cursorEntered(Panel *panel)
{
	//if (m_pCVar == NULL)// try to find it again? XDM: this helps if cvar is created after this button
	//	m_pCVar = CVAR_GET_POINTER(m_szCVarName);

	CommandButton::cursorEntered();
}

void ToggleCommandButton::cursorExited(Panel *panel)
{
	CommandButton::cursorExited();
}*/

//-----------------------------------------------------------------------------
// Purpose: from CDefaultInputSignal - makes images clickable
// Input  : code - 
//			*panel - 
//-----------------------------------------------------------------------------
void ToggleCommandButton::mousePressed(MouseCode code, Panel *panel)
{
	//CommandButton::mousePressed(code, panel);// XDM
	doClick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ToggleCommandButton::paint(void)
{
	//int textwide, texttall;
	//getTextSize(textwide, texttall);
	if (m_bState)
	{
		pLabelOff->setVisible(false);
		//doesn't help		pLabelOn->setPos(textwide, (getTall() - pLabelOn->getTall()) / 2);
		pLabelOn->setVisible(true);
	}
	else
	{
		pLabelOn->setVisible(false);
		//pLabelOff->setPos(textwide, (getTall() - pLabelOff->getTall()) / 2);
		pLabelOff->setVisible(true);
	}
	CommandButton::paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pCaller - 
//-----------------------------------------------------------------------------
void ToggleCommandButton::OnActionPerformed(CMenuActionHandler *pCaller)
{
	if (pCaller && pCaller->m_iSignal == 1)
		SetState(true);
	else
		SetState(false);

	CommandButton::OnActionPerformed(pCaller);
}


//-----------------------------------------------------------------------------
// Purpose: Set state from outside. Do not misuse!!
// Input  : *bState - 
//-----------------------------------------------------------------------------
bool ToggleCommandButton::GetState(void)
{
	return m_bState;
}

//-----------------------------------------------------------------------------
// Purpose: Set state from outside. Do not misuse!!
// Input  : *bState - 
//-----------------------------------------------------------------------------
void ToggleCommandButton::SetState(bool bState)
{
	m_bState = bState;
}




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMapName - 
//			text - 
//			x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
MapButton::MapButton(const char *pMapName, const char* text,int x,int y,int wide,int tall) : CommandButton(text,x,y,wide,tall)
{
	_snprintf(m_szMapName, MAX_MAPPATH, "maps/%s.bsp", pMapName);
}

int MapButton::IsNotValid(void)
{
	const char *level = GET_LEVEL_NAME();
	if (!level)
		return true;

	// Does it match the current map name?
	if (_stricmp(m_szMapName, level))// XDM: case insensitive!
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Label with image
//-----------------------------------------------------------------------------
CImageLabel::CImageLabel() : Label()
{
	m_pImage = NULL;
	setPaintBackgroundEnabled(false);
}

CImageLabel::CImageLabel(const char *pImageName, int x, int y) : Label("", x,y)
{
	m_pImage = NULL;// !!!
	setPaintBackgroundEnabled(false);
	setContentFitted(true);
	LoadImage(pImageName);
	if (m_pImage == NULL)
		setText(pImageName);
}

CImageLabel::CImageLabel(const char *pImageName, int x, int y, int wide, int tall) : Label("", x, y, wide, tall)
{
	m_pImage = NULL;// !!!
	setPaintBackgroundEnabled(false);
	setContentFitted(true);
	LoadImage(pImageName);
	if (m_pImage)
		setSize(wide, tall);// the size was reset by LoadImage()
	else
		setText(pImageName);
}

bool CImageLabel::LoadImage(const char *pImageName, bool fullpath)
{
	if (m_pImage)
		delete m_pImage;

	if (UTIL_FileExtensionIs(pImageName, ".bmp"))// XDM3038
	{
		//if (fullpath)
		m_pImage = vgui_LoadBMP(pImageName);
	}
	else// tga or no extension
	{
		// Load the Image
		if (fullpath)// don't make path, it's already specified
			m_pImage = vgui_LoadTGA(pImageName);
		else
			m_pImage = LoadTGAForRes(pImageName);
	}

	if (m_pImage == NULL)
		return false;	// unable to load image

	int w,t;
	m_pImage->getSize(w, t);
	setSize(XRES(w), YRES(t));
	setImage(m_pImage);
	return true;
}

void CImageLabel::getImageSize(int &wide, int &tall)
{
	if (m_pImage)
		m_pImage->getSize(wide, tall);
}

/*void CImageLabel::getImageCenter(int &x, int &y)
{
	if (m_pImage)
	{
		m_pImage->getSize(x, y);
		x /= 2;
		y /= 2;
	}
}*/



//-----------------------------------------------------------------------------
// Purpose: CCustomScrollButton
// Input  : iArrow - 
//			text - 
//			x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
CCustomScrollButton::CCustomScrollButton(int iArrow, const char *text, int x, int y, int wide, int tall) : CommandButton(text,x,y,wide,tall)
{
	// Load in the arrow
	m_pImage = LoadTGAForRes(sArrowFilenames[iArrow]);
	if (m_pImage)
		setImage(m_pImage);

	//setFgColor(Scheme::sc_primary1);
	setContentAlignment(Label::a_center);
	setContentFitted(true);
	// Highlight signal
	addInputSignal(new CHandler_CommandButtonHighlight(this));
}

void CCustomScrollButton::paint(void)
{
	if (m_pImage)
	{
		if (isArmed())
			m_pImage->setColor(vgui::Color(255,255,255, 0));
		else
			m_pImage->setColor(vgui::Color(255,255,255, 127));

		m_pImage->doPaint(this);
	}
}

/*void CCustomScrollButton::paintBackground(void)
{
	if (isArmed())
	{
		// Orange highlight background
		drawSetColor(Scheme::sc_primary2);
		drawFilledRect(0,0,_size[0],_size[1]);
	}

	// Orange Border
	drawSetColor(Scheme::sc_secondary1);
	drawOutlinedRect(0,0,_size[0]-1,_size[1]);
}*/


//-----------------------------------------------------------------------------
// Purpose: CCustomSlider: does this thing work?
// Input  : x y - 
//			wide tall - 
//			vertical - 
//-----------------------------------------------------------------------------
CCustomSlider::CCustomSlider(int x, int y, int wide, int tall, bool vertical) : Slider(x,y,wide,tall,vertical)
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	if (pSchemes)
		SetScheme(pSchemes->getScheme("Primary Button Text"));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pLocalScheme - 
//-----------------------------------------------------------------------------
void CCustomSlider::SetScheme(CScheme *pLocalScheme)// XDM3037
{
	if (pLocalScheme)
	{
		m_ColorBgNormal = pLocalScheme->BgColor;
		m_ColorBgArmed = pLocalScheme->BgColorArmed;
		m_ColorBgSelected = pLocalScheme->BgColorClicked;
		m_ColorBorderNormal = pLocalScheme->BorderColor;
		m_ColorBorderArmed = pLocalScheme->BorderColorArmed;
	}
	setBgColor(m_ColorBgNormal.r, m_ColorBgNormal.g, m_ColorBgNormal.b, 255-m_ColorBgNormal.a);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomSlider::paintBackground(void)
{
	int wide,tall,nobx,noby;
	getPaintSize(wide,tall);
	getNobPos(nobx,noby);

	int mx,my;
	App::getInstance()->getCursorPos(mx,my);

	if (isMouseDown(MOUSE_LEFT) && (mx > _clipRect[0] && mx < _clipRect[2] && my > _clipRect[1] && my < _clipRect[3]))
		drawSetColor(m_ColorBgSelected.r, m_ColorBgSelected.g, m_ColorBgSelected.b, 255-m_ColorBgSelected.a);
	else if (hasFocus())
		drawSetColor(m_ColorBgArmed.r, m_ColorBgArmed.g, m_ColorBgArmed.b, 255-m_ColorBgArmed.a);
	else
		drawSetColor(m_ColorBgNormal.r, m_ColorBgNormal.g, m_ColorBgNormal.b, 255-m_ColorBgNormal.a);

	// Nob Fill
	if (isVertical())
		drawFilledRect(0,nobx,wide,noby);
	else
		drawFilledRect(nobx,0,noby,tall);

	if (hasFocus())
		drawSetColor(m_ColorBorderArmed.r, m_ColorBorderArmed.g, m_ColorBorderArmed.b, 255-m_ColorBorderArmed.a);
	else
		drawSetColor(m_ColorBorderNormal.r, m_ColorBorderNormal.g, m_ColorBorderNormal.b, 255-m_ColorBorderNormal.a);

	// Border
	//if (_paintBorderEnabled)
		drawOutlinedRect(0,0,wide,tall);

	// Nob Outline
	if (isVertical())
		drawOutlinedRect(0,nobx,wide,noby);
	else		
		drawOutlinedRect(nobx,0,noby,tall);

	// Border
	/*drawSetColor(Scheme::sc_primary3);
	drawOutlinedRect(0,0,wide,tall);

	if (isVertical())
	{
		// Nob Fill
		drawSetColor(Scheme::sc_primary1);
		drawFilledRect(0,nobx,wide,noby);
		// Nob Outline
		//if (_dragging)
		//	drawSetColor(Scheme::sc_secondary3);
		//else
			drawSetColor(Scheme::sc_primary3);

		drawOutlinedRect(0,nobx,wide,noby);
	}
	else
	{
		// Nob Fill
		drawSetColor(Scheme::sc_primary1);
		drawFilledRect(nobx,0,noby,tall);
		// Nob Outline
		drawSetColor(Scheme::sc_primary3);
		drawOutlinedRect(nobx,0,noby,tall);
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: CCustomScrollPanel default constructor // XDM3038: no time to write
//-----------------------------------------------------------------------------
CCustomScrollPanel::CCustomScrollPanel(void) : ScrollPanel(0,0,2,2)
{
	ScrollBar *pScrollBar = getVerticalScrollBar();
	ASSERT(pScrollBar != NULL);
	pScrollBar->setButton(new CCustomScrollButton(ARROW_UP, "", 0,0,16,16), 0);
	pScrollBar->setButton(new CCustomScrollButton(ARROW_DOWN, "", 0,0,16,16), 1);
	pScrollBar->setSlider(new CCustomSlider(0,1,2,(2-(2*2))+2,true)); 
	pScrollBar->setPaintBorderEnabled(false);
	pScrollBar->setPaintBackgroundEnabled(false);
	pScrollBar->setPaintEnabled(false);
	pScrollBar = getHorizontalScrollBar();
	ASSERT(pScrollBar != NULL);
	pScrollBar->setButton(new CCustomScrollButton(ARROW_LEFT, "", 0,0,16,16), 0);
	pScrollBar->setButton(new CCustomScrollButton(ARROW_RIGHT, "", 0,0,16,16), 1);
	pScrollBar->setSlider(new CCustomSlider(2,0,-2,2,false));
	pScrollBar->setPaintBorderEnabled(false);
	pScrollBar->setPaintBackgroundEnabled(false);
	pScrollBar->setPaintEnabled(false);
	// does not work here, assign to parent panel	addInputSignal(new CMenuHandler_ScrollInput(this));
}

//-----------------------------------------------------------------------------
// Purpose: CCustomScrollPanel
// Input  : x y - 
//			wide tall - 
//-----------------------------------------------------------------------------
CCustomScrollPanel::CCustomScrollPanel(int x, int y, int wide, int tall) : ScrollPanel(x,y,wide,tall)
{
	ScrollBar *pScrollBar = getVerticalScrollBar();
	ASSERT(pScrollBar != NULL);
	if (pScrollBar)
	{
		pScrollBar->setButton(new CCustomScrollButton(ARROW_UP, "", 0,0,16,16), 0);
		pScrollBar->setButton(new CCustomScrollButton(ARROW_DOWN, "", 0,0,16,16), 1);
		pScrollBar->setSlider(new CCustomSlider(0,wide-1,wide,(tall-(wide*2))+2,true)); 
		pScrollBar->setPaintBorderEnabled(false);
		pScrollBar->setPaintBackgroundEnabled(false);
		pScrollBar->setPaintEnabled(false);
	}
	pScrollBar = getHorizontalScrollBar();
	ASSERT(pScrollBar != NULL);
	if (pScrollBar)
	{
		pScrollBar->setButton(new CCustomScrollButton(ARROW_LEFT, "", 0,0,16,16), 0);
		pScrollBar->setButton(new CCustomScrollButton(ARROW_RIGHT, "", 0,0,16,16), 1);
		pScrollBar->setSlider(new CCustomSlider(tall,0,wide-(tall*2),tall,false));
		pScrollBar->setPaintBorderEnabled(false);
		pScrollBar->setPaintBackgroundEnabled(false);
		pScrollBar->setPaintEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: CMenuHandler_ScrollInput
//-----------------------------------------------------------------------------
CMenuHandler_ScrollInput::CMenuHandler_ScrollInput(void):CDefaultInputSignal()
{
	m_pScrollPanel = NULL;
}

CMenuHandler_ScrollInput::CMenuHandler_ScrollInput(ScrollPanel *pScrollPanel):CDefaultInputSignal()
{
	m_pScrollPanel = pScrollPanel;
}

void CMenuHandler_ScrollInput::mouseWheeled(int delta, Panel *panel)
{
	if (m_pScrollPanel)
	{
		int hv, vv;
		m_pScrollPanel->getScrollValue(hv, vv);
		if (gViewPort->isKeyDown(KEY_LSHIFT) || gViewPort->isKeyDown(KEY_RSHIFT))// && m_pScrollPanel->getHorizontalScrollBar())
			hv -= delta * sensitivity->value * SCROLLINPUT_WHEEL_DELTA;
		else
			vv -= delta * sensitivity->value * SCROLLINPUT_WHEEL_DELTA;

		m_pScrollPanel->setScrollValue(hv, vv);
	}
}




//============================================================
// ActionSignals
//============================================================

//-----------------------------------------------------------------------------
// Purpose: for CCommandMenu buttons
//-----------------------------------------------------------------------------
CMenuActionHandler::CMenuActionHandler(void)
{
	m_pParentMenu = NULL;
	m_bCloseMenu = false;
}

CMenuActionHandler::CMenuActionHandler(CCommandMenu	*pParentMenu, int iSignal, bool bCloseMenu)
{
	m_pParentMenu = pParentMenu;
	m_iSignal = iSignal;
	m_bCloseMenu = bCloseMenu;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - is usually a command button that issued this signal
//-----------------------------------------------------------------------------
void CMenuActionHandler::actionPerformed(Panel *panel)
{
	//if (panel->getParent == (Panel *)m_pParentMenu)
	{
		CommandButton *pButton = (CommandButton *)panel;// TODO: revisit!
		if (pButton)
			pButton->OnActionPerformed(this);

		//if (m_iSoundType == ACTSIGSOUND_DEFAULT || m_iSoundType == ACTSIGSOUND_SUCCESS)// XDM3038a
			PlaySound("vgui/button_press.wav", VOL_NORM);
		//else if (m_iSoundType == ACTSIGSOUND_FAILURE)
		//	PlaySound("vgui/button_error.wav", VOL_NORM);
	}

	if (m_pParentMenu)// XDM3035a
	{
		//TODO?		m_pParentMenu->OnActionPerformed();
		if (m_bCloseMenu)
			m_pParentMenu->Close();
	}
	else// WTF? TODO: REVISIT close any menu???
	{
		if (m_bCloseMenu)
			gViewPort->HideTopMenu();
		else
			gViewPort->HideCommandMenu();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Simple command menu handler
// Input  : CCommandMenu	*pParentMenu - 
//			*pszCommand - 
//			bCloseMenu - 
//-----------------------------------------------------------------------------
CMenuHandler_StringCommand::CMenuHandler_StringCommand(CCommandMenu	*pParentMenu, const char *pszCommand, bool bCloseMenu) : CMenuActionHandler(pParentMenu, 0, bCloseMenu)
{
	strncpy(m_pszCommand, pszCommand, MAX_COMMAND_SIZE);
	m_pszCommand[MAX_COMMAND_SIZE-1] = '\0';
	//m_iCloseVGUIMenu = iClose;
}

// TODO: don't use this argument!
void CMenuHandler_StringCommand::actionPerformed(Panel *panel)
{
	/*if (strbegin(m_pszCommand, "jointeam") > 0)
	{
		DBG_FORCEBREAK// XDM3037a: TEST
	}*/
	CLIENT_COMMAND(m_pszCommand);
	CMenuActionHandler::actionPerformed(panel);// call BEFORE, so it will not interrupt the menu sound
}


//-----------------------------------------------------------------------------
// Purpose: Special signal which toggles client console variables
// Input  : *pParentMenu - 
//			*pCVar - 
//-----------------------------------------------------------------------------
CMenuHandler_ToggleCvar::CMenuHandler_ToggleCvar(CCommandMenu *pParentMenu, struct cvar_s *pCVar) : CMenuActionHandler(pParentMenu, 0, false)
{
	m_pCVar = pCVar;
}

CMenuHandler_ToggleCvar::CMenuHandler_ToggleCvar(CCommandMenu *pParentMenu, const char *pCVarName) : CMenuActionHandler(pParentMenu, 0, false)
{
	m_pCVar = CVAR_GET_POINTER(pCVarName);
}

void CMenuHandler_ToggleCvar::actionPerformed(Panel *panel)
{
	if (m_pCVar != NULL)// XDM
	{
		if (m_pCVar->value > 0.0f)
		{
			m_pCVar->value = 0.0f;
			m_iSignal = 0;
		}
		else
		{
			m_pCVar->value = 1.0f;
			m_iSignal = 1;
		}
	}
	//hack	gViewPort->UpdateSpectatorPanel();
	CMenuActionHandler::actionPerformed(panel);
}




//-----------------------------------------------------------------------------
// Purpose: Special signal for closing text windows
// Input  : iState - 
//-----------------------------------------------------------------------------
CMenuHandler_TextWindow::CMenuHandler_TextWindow(int iState)
{
	m_iState = iState;
}

void CMenuHandler_TextWindow::actionPerformed(Panel *panel)
{
	if (m_iState == HIDE_TEXTWINDOW)
	{
		gViewPort->HideTopMenu();
	}
	else 
	{
		gViewPort->HideCommandMenu();
		gViewPort->ShowMenuPanel(m_iState);
	}
	CMenuActionHandler::actionPerformed(panel);
}



//============================================================
// InputSignals
//============================================================

//-----------------------------------------------------------------------------
// Purpose: Show submenu
// Input  : *pButton - 
//			*pSubMenu - 
//-----------------------------------------------------------------------------
CMenuHandler_PopupSubMenuInput::CMenuHandler_PopupSubMenuInput(Button *pButton, CCommandMenu *pSubMenu)
{
	m_pSubMenu = pSubMenu;
	m_pButton = pButton;
}

void CMenuHandler_PopupSubMenuInput::cursorEntered(Panel *panel) 
{
	gViewPort->SetCurrentCommandMenu(m_pSubMenu);

	if (m_pButton)
		m_pButton->setArmed(true);
}

void CMenuHandler_PopupSubMenuInput::cursorExited(Panel *panel) 
{
	//gViewPort->SetCurrentCommandMenu(m_pSubMenu);
	// XDM3038: cursorExited() is called for this button	if (m_pButton)
	//		m_pButton->setArmed(false);// TEST
}


//-----------------------------------------------------------------------------
// Purpose: Input Handler for Drag N Drop panels
// Input  : *pPanel - 
//-----------------------------------------------------------------------------
CDragNDropHandler::CDragNDropHandler(DragNDropPanel *pPanel)
{
	m_pPanel = pPanel;
	ASSERT(m_pPanel != NULL);
	m_bDragging = false;
	m_bMouseInside = false;
	m_PanelStartPos[0] = 0;
	m_PanelStartPos[1] = 0;
	m_LastMousePos[0] = 0;
	m_LastMousePos[1] = 0;
}

void CDragNDropHandler::cursorMoved(int x, int y, Panel *panel)
{
	if (m_bDragging)
	{
		App::getInstance()->getCursorPos(x,y);// sorry, we need these
		if (m_pPanel)
		{
			int wx, wy;
			m_pPanel->getPos(wx, wy);
			m_pPanel->setPos(wx+(x-m_LastMousePos[0]), wy+(y-m_LastMousePos[1]));// currentpos + lastmousedelta
			//m_pPanel->setPos(m_iaDragOrgPos[0]+(x-m_iaDragStart[0]), m_iaDragOrgPos[1]+(y-m_iaDragStart[1]));
			if (m_pPanel->getParent() != NULL)
				m_pPanel->getParent()->repaint();

			//PlaySound("slider_move.wav", VOL_NORM);
		}
		m_LastMousePos[0] = x;
		m_LastMousePos[1] = y;
	}
}

void CDragNDropHandler::cursorEntered(Panel *panel)
{
	//conprintf(1, "cursorEntered\n");
	m_bMouseInside = 1;

	if (m_pPanel && m_pPanel->getDragEnabled())
		App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_sizeall));
	//does not work	if (m_pPanel)
	//m_pPanel->setCursor(Scheme::SchemeCursor::scu_sizeall);
}

void CDragNDropHandler::cursorExited(Panel *panel)
{
//	conprintf(1, "cursorExited\n");
	m_bMouseInside = 0;

	if (m_pPanel)
		App::getInstance()->setCursorOveride(App::getInstance()->getScheme()->getCursor(Scheme::scu_arrow));
	//if (m_pPanel)
	//	m_pPanel->setCursor(Scheme::SchemeCursor::scu_last);

	//ASSERT(m_bDragging == false);
	if (m_bDragging)// XDM3038a: should never be true!
		Drop();
}

void CDragNDropHandler::mousePressed(MouseCode code, Panel *panel)
{
//	conprintf(1, ("mousePressed\n");
	if (code == MOUSE_LEFT)
	{
		if (m_bMouseInside)
		{
			//m_bMousePressed = 1;
			Drag();
		}
	}
	else if (code == MOUSE_RIGHT)// cancel
	{
		if (m_bDragging)
		{
			m_pPanel->setPos(m_PanelStartPos[0], m_PanelStartPos[1]);
			//Drop(m_PanelStartPos[0], m_PanelStartPos[1]);
			Drop();
		}
	}
} 

void CDragNDropHandler::mouseReleased(MouseCode code, Panel *panel)
{
	//conprintf(1, "mouseReleased\n");
	//m_bMousePressed = 0;
	//Drop(m_LastMousePos[0]+m_PanelPosMouseDelta[0], m_LastMousePos[1]+m_PanelPosMouseDelta[1]);
	Drop();
}

void CDragNDropHandler::Drag(void)
{
	//conprintf(1, "CDragNDropHandler::Drag()\n");
	if (m_pPanel)
	{
		if (m_pPanel->getDragEnabled())
		{
			int x,y;
			App::getInstance()->getCursorPos(x,y);
			m_bDragging = true;
			m_LastMousePos[0] = x;
			m_LastMousePos[1] = y;
			m_pPanel->getPos(m_PanelStartPos[0], m_PanelStartPos[1]);
			//m_PanelPosMouseDelta[0] = m_PanelStartPos[0] - x;
			//m_PanelPosMouseDelta[1] = m_PanelStartPos[1] - y;
			App::getInstance()->setMouseCapture(m_pPanel);
			m_pPanel->setDragged(m_bDragging);
			m_pPanel->requestFocus();
			PlaySound("vgui/slider_move.wav", VOL_NORM);// XDM
		}
	}
}

void CDragNDropHandler::Drop(void)//(int x, int y)
{
	//conprintf(1, "CDragNDropHandler::Drop(%d, %d)\n", x, y);
	m_bDragging = false;
	if (m_pPanel)
	{
		// allow always	if (m_pPanel->getDragEnabled())
		m_pPanel->setDragged(m_bDragging);
		//m_pPanel->setPos(x,y);
		PlaySound("vgui/button_press.wav", VOL_NORM);// XDM
		App::getInstance()->setMouseCapture(null);
	}
}


//-----------------------------------------------------------------------------
// Purpose: CHandler_MenuButtonOver
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CHandler_MenuButtonOver::cursorEntered(Panel *panel)
{
	if (gViewPort && m_pMenuPanel)
		m_pMenuPanel->SetActiveInfo(m_iButton);
}


//-----------------------------------------------------------------------------
// Purpose: Default TextPanel
//-----------------------------------------------------------------------------
CDefaultTextPanel::CDefaultTextPanel(void) : TextPanel("", 0,0, 8,8)
{
	//setFgColor(Scheme::sc_primary1);
	//setBgColor(Scheme::sc_primary2);
}

CDefaultTextPanel::CDefaultTextPanel(const char *text, int x, int y, int wide, int tall) : TextPanel(text,x,y,wide,tall)
{
	//setFgColor(Scheme::sc_primary1);
	//setBgColor(Scheme::sc_primary2);
}


//-----------------------------------------------------------------------------
// Purpose: Default TextEntry
//-----------------------------------------------------------------------------
CDefaultTextEntry::CDefaultTextEntry(void) : TextEntry("", 0,0, 8,8)
{
	//setFgColor(Scheme::sc_primary1);
	//setBgColor(Scheme::sc_primary2);
}

CDefaultTextEntry::CDefaultTextEntry(const char *text, int x, int y, int wide, int tall) : TextEntry(text,x,y,wide,tall)
{
	//setFgColor(Scheme::sc_primary1);
	//setBgColor(Scheme::sc_primary2);
}


//-----------------------------------------------------------------------------
// Purpose: Default ScrollPanel
//-----------------------------------------------------------------------------
CDefaultScrollPanel::CDefaultScrollPanel(void) : ScrollPanel(0,0, 8,8)
{
	//setFgColor(Scheme::sc_primary1);
	//setBgColor(Scheme::sc_primary2);
}

CDefaultScrollPanel::CDefaultScrollPanel(int x, int y, int wide, int tall) : ScrollPanel(x,y,wide,tall)
{
	//setFgColor(Scheme::sc_primary1);
	//setBgColor(Scheme::sc_primary2);
}


//-----------------------------------------------------------------------------
// Purpose: LineBorder with default color and thickness
//-----------------------------------------------------------------------------
CDefaultLineBorder::CDefaultLineBorder() : LineBorder(gViewPort?gViewPort->m_iBorderThickness:1,
													  gViewPort?vgui::Color(gViewPort->m_BorderColor.r,gViewPort->m_BorderColor.g,gViewPort->m_BorderColor.b,gViewPort->m_BorderColor.a):vgui::Color(Scheme::sc_primary3))
{
}
