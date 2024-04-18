#ifndef RSTELEPARTS_H
#define RSTELEPARTS_H

#include "particledef.h"
#include "dlight.h"

#define NUMVERTEXNORMALS	162

static const float vdirs[NUMVERTEXNORMALS][3] =
{
#include "anorms.h"
};

//-----------------------------------------------------------------------------
// Controls particles created by the engine
//-----------------------------------------------------------------------------
class CRSTeleparts : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSTeleparts"; }// XDM3038b
	CRSTeleparts(void);
	CRSTeleparts(const Vector &origin, float radius, byte pal_color, byte type, float timetolive, byte r, byte g, byte b);
	virtual ~CRSTeleparts(void);

	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

private:
	dlight_t *m_pLight;
	particle_t *m_pParticles[NUMVERTEXNORMALS];

	float m_vecAng[NUMVERTEXNORMALS][2];//3 for vectors

	byte m_colorpal;
	byte m_iType;
};

#endif // RSTELEPARTS_H
