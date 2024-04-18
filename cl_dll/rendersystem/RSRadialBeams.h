#ifndef RSRADIALBEAMS_H
#define RSRADIALBEAMS_H

#include "beamdef.h"

// These are only flags allowed to be provided by user
#define BEAM_FLAGS_EFFECTS_MASK		(FBEAM_SINENOISE | FBEAM_SOLID | FBEAM_SHADEIN | FBEAM_SHADEOUT)

//-----------------------------------------------------------------------------
// Controls beams created by the engine
//-----------------------------------------------------------------------------
class CRSRadialBeams : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSRadialBeams"; }
	CRSRadialBeams(void);
	CRSRadialBeams(int modelIndex, const Vector &origin, float radius, size_t numbeams, uint32 color4b, float beamlifemin, float beamlifemax, float widthmin, float widthmax, float amplitude, float speed, float framerate, int beamflags, float timetolive);
	virtual ~CRSRadialBeams(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeSystem(void);
	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

private:
	float m_fBeamLifeMin;
	float m_fBeamLifeMax;
	float m_fBeamWidthMin;
	float m_fBeamWidthMax;
	float m_fBeamAmplitude;
	float m_fBeamSpeed;
	size_t m_iNumBeams;
	int m_iBeamFlags;
	int m_iBeamTextureIndex;
	Vector *m_vszEndPositions;// local (not world) coordinates
	BEAM **m_pszBeams;
};

#endif // RSRADIALBEAMS_H
