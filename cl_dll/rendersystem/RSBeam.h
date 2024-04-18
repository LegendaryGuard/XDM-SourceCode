#ifndef RSBEAM_H
#define RSBEAM_H


//-----------------------------------------------------------------------------
// Simple beam made of two crossed quads (like those in Unreal)
//-----------------------------------------------------------------------------
class CRSBeam : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSBeam"; }// XDM3038b
	CRSBeam(void);
	CRSBeam(const Vector &start, const Vector &end, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual void Render(void);

	Vector	m_vEnd;
	float	m_fTextureTile;
};

#endif // RSBEAM_H
