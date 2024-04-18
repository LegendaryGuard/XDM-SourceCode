#ifndef RSLIGHT_H
#define RSLIGHT_H

// Half-Life specific
#include "dlight.h"

#define LIGHT_INDEX_TE_RSLIGHT 0// 0 allows multiple dlight instances

//-----------------------------------------------------------------------------
// Controls a dynamic light, makes it possible to move, change color, etc.
//-----------------------------------------------------------------------------
class CRSLight : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSLight"; }// XDM3038b
	CRSLight(void);
	CRSLight(const Vector &origin, byte r, byte g, byte b, float radius, float (*RadiusFn)(CRSLight *pSystem, const float &time), float decay, float timetolive, bool elight = false);
	virtual ~CRSLight(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);
	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

	float (*m_RadiusCallback)(CRSLight *pSystem, const float &time);// radius = f(time);

private:
	dlight_t *m_pLight;
	short m_iLightType;// ELight affects studio models only
};

#endif // RSLIGHT_H
