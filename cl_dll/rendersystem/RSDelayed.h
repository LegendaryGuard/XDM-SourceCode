#ifndef RSDELAYED_H
#define RSDELAYED_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// A RenderSystem delayerm purely virtual system, not visual.
//-----------------------------------------------------------------------------
class CRSDelayed : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSDelayed"; }// XDM3038b
	CRSDelayed(void);
	CRSDelayed(CRenderSystem *pSystem, float delay, int flags = 0, int followentindex = -1, int followflags = 0);
	virtual ~CRSDelayed(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

private:
	CRenderSystem *m_pSystem;// RenderSystem to be activated
};

#endif // RSDELAYED_H
