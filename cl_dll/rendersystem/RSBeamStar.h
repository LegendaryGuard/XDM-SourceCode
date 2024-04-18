#ifndef RSBEAMSTAR_H
#define RSBEAMSTAR_H

//-----------------------------------------------------------------------------
// A very nice "sunburst" effect
//-----------------------------------------------------------------------------
class CRSBeamStar : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSBeamStar"; }// XDM3038b
	CRSBeamStar(void);
	CRSBeamStar(const Vector &origin, struct model_s *pTexture, size_t number, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive);
	virtual ~CRSBeamStar(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeSystem(void);// XDM3038b
	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

protected:
	float *m_ang1;
	float *m_ang2;
	vec3_t *m_Coords;
	size_t m_iCount;
};

#endif // RSBEAMSTAR_H
