#ifndef RSSPRITE_H
#define RSSPRITE_H

//-----------------------------------------------------------------------------
// Ordinary sprite, faces viewport, displays all frames and removes itself
// UNDONE: looks like it is impossible to make a glow sprite like this
//-----------------------------------------------------------------------------
class CRSSprite : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSSprite"; }// XDM3038b
	CRSSprite(void);
	CRSSprite(const Vector &origin, const Vector &velocity, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive);
//	virtual ~CRSSprite(void);

	virtual void Render(void);
};

#endif // RSSPRITE_H
