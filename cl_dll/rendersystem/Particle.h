#ifndef PARTICLE_H
#define PARTICLE_H

// custom flags for systems to use
#define PARTICLE_FLAG1			(1<<0)
#define PARTICLE_FLAG2			(1<<1)
#define PARTICLE_FLAG3			(1<<2)
#define PARTICLE_FLAG4			(1<<3)
#define PARTICLE_FLAG5			(1<<4)
#define PARTICLE_FLAG6			(1<<5)
#define PARTICLE_FLAG7			(1<<6)
#define PARTICLE_FLAG8			(1<<7)
#define PARTICLE_ORIENTED		(1<<8)

// The particle class
// Keep this as compact and cache-friendly as possible!
class CParticle
{
public:
	CParticle();
	virtual ~CParticle();
	//virtual const char *ClassDescription(void) { return "CParticle"; }

	bool IsFree(void) const;
	bool IsExpired(void) const;
	virtual void UpdateColor(const float &elapsed_time);
	virtual void UpdateSize(const float &elapsed_time);
	virtual void Render(const Vector &rt, const Vector &up, const int &rendermode, const bool &doubleside = false);
#if defined (RS_SORTED_PARTICLES_RENDER)
	virtual bool RenderBegin(const int &rendermode, const bool &doubleside);// internal
	virtual void RenderWrite(const Vector &rt, const Vector &up, const int &rendermode);// internal
	virtual void RenderEnd(void);// internal
#endif
	void UpdateEnergyByBrightness(void);
	bool FrameIncrease(void);
	void FrameRandomize(void);
	void SetDefaultColor(void);
	void SetColor(const float rgba[4]);
	void SetColor(const ::Color &rgba);
	void SetColor(const ::Color &rgb, const float &a);
	void SetColor(const color24 &rgb, const float &a);
	void SetColorDelta(const ::Color &rgb, const float &a);
	void SetColorDelta(const color24 &rgb, const float &a);
	void SetColorDelta(const float *rgb, const float &a);
	void SetColorDelta(const float *rgba);
	void SetSizeFromTexture(const float &multipl_x = 1.0f, const float &multipl_y = 1.0f);

	Vector m_vPos;// current position
	Vector m_vPosPrev;// previous position
	Vector m_vVel;// velocity
	Vector m_vAccel;// acceleration (gravity, etc.)
	//Vector m_vAngles;?
	//Vector m_vDirection/Destination;// TODO? randomly change direction from time to time; modify velocity to slowly approach it
	//float m_fNextDirChangeTime;

	//float m_fTime;// time passed since start
	float m_fEnergy;// should change from 1 to 0 during particle lifetime (linear)  // do we really need this? // TODO: replace this with proper timeline and invent envelopes for colors
	float m_fSizeX;// particle size in world units
	float m_fSizeY;
	float m_fSizeDelta;
	//float m_fWeight;// particle weight, modifies acceleration (default is 1.0)
	//float m_fWeightDelta;
	float m_fColor[4];// color in RGBA (0...1) format
	float m_fColorDelta[4];

	uint32 m_iFlags;
	uint32 m_iFrame;
	uint32 index;// index in my system, assigned in AllocateParticle()

//protected:
	struct model_s *m_pTexture;
	//CParticleSystem *m_pContainingSystem;// particle system I am member of
	//void *pData;// some user data?
};

#endif // PARTICLE_H
