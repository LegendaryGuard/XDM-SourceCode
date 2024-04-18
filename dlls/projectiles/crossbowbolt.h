#ifndef CROSSBOWBOLT_H
#define CROSSBOWBOLT_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

#define BOLT_AIR_VELOCITY	8192
#define BOLT_WATER_VELOCITY	4096

class CCrossbowBolt : public CBaseProjectile// XDM3037
{
public:
	static CCrossbowBolt *BoltCreate(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, bool sniper);

	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void) { return CLASS_NONE; }

	void EXPORT BoltThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT BlowTouch(CBaseEntity *pOther);// XDM

protected:
	//int m_iFireball;// XDM3038
	//int m_iSpark;
	unsigned short m_usBoltHit;
};

#endif // CROSSBOWBOLT_H
