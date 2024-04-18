#ifndef RSCYLINDER_H
#define RSCYLINDER_H

//-----------------------------------------------------------------------------
// Renders cylinder, replacement for TE_BEAMCYLINDER
//-----------------------------------------------------------------------------
class CRSCylinder : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSCylinder"; }// XDM3038b
	CRSCylinder(void);
	CRSCylinder(const Vector &origin, float radius, float radiusdelta, float width, size_t segments, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive);
	virtual ~CRSCylinder(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeSystem(void);
	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual bool Update(const float &time, const double &elapsedTime);// XDM3037
	virtual void Render(void);

	float m_fWidth;
	float m_fWidthDelta;// XDM3037
	float m_fTextureStart;// XDM3037a
	float m_fTextureScrollRate;// XDM3037a
	float m_fTextureStep;// XDM3037a: texture V coordinate width per edge
private:
	size_t m_iSegments;
	Vector2D *m_pv2dPoints;
};

#endif // RSCYLINDER_H
