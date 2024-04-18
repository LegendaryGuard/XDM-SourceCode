#include "hud.h"
#include "cl_util.h"
#include "kbutton.h"
#include "cvardef.h"
#include "vgui_Int.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "vgui_PSEditorPanel.h"

// UNDONE

// XDM: in order to inherit updated coordinates from base class,
// this one should NOT use x y arguments (only getPos() if really nescessasry),
CPSEditorPanel::CPSEditorPanel(int x, int y, int wide, int tall, CParticleSystem *pSystem) : CDialogPanel(1/*remove*/, x, y, wide, tall, VGUI_MB_OKCANCEL)
{
	setBgColor(0,0,0,PANEL_DEFAULT_ALPHA);

	char nullstring[32];
	memset(nullstring, 0, 32);
	int offsetY = PANEL_INNER_OFFSET + m_TitleIcon.getTall() + PSEP_MARGIN;
	m_Label.setParent(this);
	m_Label.setBounds(PSEP_MARGIN,offsetY,wide-PSEP_MARGIN*2, TEXTENTRY_SIZE_Y);
	m_Label.setText("label", 0);

	offsetY += PSEP_MARGIN + m_Label.getTall();
	m_TargetName.setParent(this);
	m_TargetName.setBounds(PSEP_MARGIN,offsetY, wide-PSEP_MARGIN*2, TEXTENTRY_SIZE_Y);
	m_TargetName.addActionSignal(new CMenuPanelActionSignalHandler(this, VGUI_IDOK));
	m_TargetName.setText(nullstring, 0);

	setBorder(new CDefaultLineBorder());
	setPaintBorderEnabled(true);
	setDragEnabled(true);
	setCaptureInput(true);

	m_TargetName.requestFocus();
}

bool CPSEditorPanel::OnClose(void)
{
	if (m_pSystem)
	{
		g_pRenderManager->DeleteSystem(m_pSystem);
		m_pSystem = NULL;
	}
	return true;
}

bool CPSEditorPanel::DoExecCommand(void)
{
	char targetname[80];
	m_TargetName.getText(0,targetname,80);
//	m_pTextEntry.setText(null,0);
	if (targetname[0])
	{
			return true;
	}
	return false;
}
