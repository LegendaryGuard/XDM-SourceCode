#ifndef RSDISK_H
#define RSDISK_H

//-----------------------------------------------------------------------------
// Renders disk, replacement for TE_BEAMDISK
//-----------------------------------------------------------------------------
class CRSDisk : public CRotatingSystem
{
	typedef CRotatingSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSDisk"; }// XDM3038b
	CRSDisk(void);
	CRSDisk(const Vector &origin, const Vector &angles, const Vector &anglesdelta, float radius, float radiusdelta, size_t segments, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeSystem(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual void Render(void);

private:
	float m_fAngleDelta;// for segments
	float m_fTexDelta;
	size_t m_iSegments;
};

#endif // RSDISK_H
