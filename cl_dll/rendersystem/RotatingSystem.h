#ifndef ROTATINGSYSTEM_H
#define ROTATINGSYSTEM_H

//-----------------------------------------------------------------------------
// A RenderSystem with rotation matrix, can rotate in any way
// UNDONE: integrate into CRenderSystem
//-----------------------------------------------------------------------------
class CRotatingSystem : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRotatingSystem"; }// XDM3038b
	CRotatingSystem(void);
	CRotatingSystem(const Vector &origin, const Vector &velocity, const Vector &angles, const Vector &anglesdelta, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeSystem(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

	void UpdateAngles(const double &timedelta, bool updatematrix/* = true*/);
	void UpdateAngleMatrix(void);
	Vector LocalToWorld(const Vector &local);
	Vector LocalToWorld(float localx, float localy, float localz);

	Vector m_vecAnglesDelta;

private:
	float m_fMatrix[3][4];
};

#endif // ROTATINGSYSTEM_H
