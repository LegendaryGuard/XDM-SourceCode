#ifndef PSDRIPS_H
#define PSDRIPS_H

#define PSD_WATERCIRCLE_SIZE_DELTA		96.0f

#define PSD_PARTICLE_FWATERCIRCLE		PARTICLE_FLAG1
#define PSD_PARTICLE_FSPLASH			PARTICLE_FLAG2

//-----------------------------------------------------------------------------
// Spawns rain particles at specified origin and in volume of mins and maxs
// Legacy system. Obsolete because CPSCustom can do the same thing
//-----------------------------------------------------------------------------
class CPSDrips : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSDrips"; }// XDM3038b
	CPSDrips(void);
	CPSDrips(uint32 maxParticles, const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &direction, struct model_s *pTexture, struct model_s *pTextureHitWater, struct model_s *pTextureHitGround, int r_mode, float sizex, float sizey, float scaledelta, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
	//virtual void ApplyForce(const Vector &origin, const Vector &force, float radius, bool point);
	virtual void Render(void);

private:
	//uint32 m_iNumEmit;// XDM3037: auto-adjusted number of particles to emit next time
	model_t *m_pTextureHitWater;
	model_t *m_pTextureHitGround;
};

#endif // PSDRIPS_H
