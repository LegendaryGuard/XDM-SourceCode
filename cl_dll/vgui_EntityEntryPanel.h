//====================================================================
//
// Purpose: An input box used to create an entity
//
//====================================================================

#ifndef VGUI_ENTITYENTRYPANEL_H
#define VGUI_ENTITYENTRYPANEL_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "RenderManager.h"

#define EEP_WIDTH			300
#define EEP_HEIGHT			200
#define EEP_MARGIN			4
#define EEP_MAX_LINES		8

enum
{
	EEP_SIGNAL_ADD = VGUI_IDLAST + 1,
	EEP_SIGNAL_REMOVE
};
//typedef int (*pfnOnOKCallback)(char *classname, char *targetname);

class CEntityEntryPanel : public CMenuPanel
{
	typedef CMenuPanel BaseClass;
public:
	CEntityEntryPanel(int x, int y, int wide, int tall);
	//virtual ~CEntityEntryPanel();
public:
	//virtual void setSize(int wide,int tall);
	virtual void Open(void);// XDM3037a
	virtual int KeyInput(const int &down, const int &keynum, const char *pszCurrentBinding);
	virtual void OnActionSignal(int signal, class CMenuPanelActionSignalHandler *pCaller);
	virtual bool OnClose(void);

	virtual bool IsPersistent(void) { return false; }// XDM3038
	virtual bool AllowConcurrentWindows(void) { return true; }// XDM3038

	void OnOK(void);
	void OnCancel(void);
	void OnAdd(void);
	void OnRemove(void);
	bool doExecCommand(void);

public:
	Vector				m_vTargetOrigin;// XDM3038c: explicit data
	Vector				m_vTargetAngles;
	RS_INDEX			m_iRenderSystemIndex;// XDM3038c: optional

private:
	CDefaultTextEntry	m_ClassName;
	CDefaultTextEntry	m_TargetName;
	Dar<CDefaultTextEntry*> m_szKVEntries;
	Label				m_LabelTitle;// XDM3038b
	Label				m_LabelClassName;// XDM3038b
	Label				m_LabelTargetName;// XDM3038b
	CommandButton		m_ButtonOK;
	CommandButton		m_ButtonCancel;
	CommandButton		m_ButtonAdd;
	CommandButton		m_ButtonRemove;
	ToggleCommandButton	*m_pButtonCreate;
	//TextEntry *m_pTextEntry;
};


#endif // VGUI_ENTITYENTRYPANEL_H
