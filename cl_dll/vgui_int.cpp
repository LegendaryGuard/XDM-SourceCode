//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vgui_Int.h"
#include <VGUI_Label.h>
#include <VGUI_BorderLayout.h>
#include <VGUI_LineBorder.h>
#include <VGUI_SurfaceBase.h>
#include <VGUI_TextEntry.h>
#include <VGUI_ActionSignal.h>
#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "vgui_Viewport.h"

/*
namespace
{

class TexturePanel : public Panel , public ActionSignal
{
private:
	int _bindIndex;
	TextEntry* _textEntry;
public:
	TexturePanel() : Panel(0,0,256,276)
	{
		_bindIndex=2700;
		_textEntry=new TextEntry("2700",0,0,128,20);
		_textEntry->setParent(this);
		_textEntry->addActionSignal(this);
	}
public:
	virtual bool isWithin(int x,int y)
	{
		return _textEntry->isWithin(x,y);
	}
public:
	virtual void actionPerformed(Panel* panel)
	{
		char buf[256];
		_textEntry->getText(0,buf,256);
		sscanf(buf,"%d",&_bindIndex);
	}
protected:
	virtual void paintBackground()
	{
		Panel::paintBackground();
		int wide,tall;
		getPaintSize(wide,tall);
		drawSetColor(0,0,255,0);
		drawSetTexture(_bindIndex);
		drawTexturedRect(0,19,257,257);
	}
};

}
*/

using namespace vgui;

extern "C"// XDM3038a
{

void VGui_ViewportPaintBackground(int extents[4])
{
	gEngfuncs.VGui_ViewportPaintBackground(extents);// BUGBUG: sometimes engine hangs within. Probably nvoglv32.dll fault.
}

void VGui_Startup(void)
{
	DBG_PRINTF("VGui_Startup()\n");
	Panel *root = (Panel *)gEngfuncs.VGui_GetPanel();
	if (root == NULL)
	{
		conprintf(0, "CL: error attaching VGUI API!\n");
		return;
	}
	root->setBgColor(127,127,127,0);
	root->setPaintBackgroundEnabled(false);// XDM3037: ?
	//root->setNonPainted(false);
	//root->setBorder(new LineBorder());
	root->setLayout(new BorderLayout(0));
	//root->getSurfaceBase()->setEmulatedCursorVisible(true);

	// XDM3038: ridiculous bug!! VGUI viewport reports 0,0 if game window is not properly focused!!!
	if (root->getWide() <= 0 || root->getTall() <= 0)
	{
		conprintf(0, "CL: warning: detected wrong viewport dimensions! Resetting!\n");
		root->setSize(ScreenWidth, ScreenHeight);
		root->setPos(0,0);// these usually get stuck around -3200
	}

	if (gViewPort != NULL)
	{
		//root->removeChild(gViewPort);
		// free the memory
		//delete gViewPort;
		//gViewPort = NULL;
		gViewPort->Initialize();
	}
	else
	{
		gViewPort = new CViewport(0,0,root->getWide(),root->getTall());
		gViewPort->setParent(root);
	}
}

void VGui_Shutdown()
{
	DBG_PRINTF("VGui_Shutdown()\n");
	if (gViewPort)
	{
		delete gViewPort;
		gViewPort = NULL;
	}
}

}// extern "C"
