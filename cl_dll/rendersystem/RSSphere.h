#ifndef RSSPHERE_H
#define RSSPHERE_H

//-----------------------------------------------------------------------------
// A sphere
//-----------------------------------------------------------------------------
class CRSSphere : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSSphere"; }// XDM3038b
	CRSSphere(void);
	CRSSphere(const Vector &origin, float radius, float radiusdelta, size_t nhorz, size_t nvert, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual void Render(void);

private:
	size_t m_iSegmentsH;
	size_t m_iSegmentsV;
};

#endif // RSSPHERE_H
