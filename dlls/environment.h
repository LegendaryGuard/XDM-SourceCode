#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H


#define SF_FOG_ACTIVE				0x0001
#define SF_FOG_FLAG2				0x0002
#define SF_FOG_FLAG3				0x0004
#define SF_FOG_FADING				0x8000

// better match TOGGLE_STATE? or STATE?
enum fog_states_e
{
	FOG_STATE_OFF = 0,
	FOG_STATE_ON,
	FOG_STATE_FADEOUT,
	FOG_STATE_FADEIN
};

class CEnvFog : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }// !!!
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
	STATE GetState(void);

	static TYPEDESCRIPTION m_SaveData[];
	short m_iStartDist;
	short m_iEndDist;
	short m_iCurrentStartDist;
	short m_iCurrentEndDist;
	float m_fFadeTime;// fade time
	float m_fFadeStartTime;// if we're fading in/out, then when did the fade start?
};




#define SF_FOGZONE_ACTIVE			0x0001

class CEnvFogZone : public CEnvFog
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};




#define SF_RAIN_START_OFF			0x0001
#define SF_RAIN_ZROTATION			0x0002
#define SF_RAIN_NOSPLASH			0x0004
#define SF_RAIN_IGNOREMODELS		0x0008// XDM3035c

#define DEFAULT_RAIN_TEXTURE		"sprites/rain.spr"
#define DEFAULT_RAIN_SPLASH			"sprites/rain_s.spr"
#define DEFAULT_RAIN_SPLASHG		"sprites/wsplash3.spr"

class CEnvRain : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int sprTexture;// don't save
	int sprHitWater;
	int sprHitGround;
	float m_fScaleX;
	float m_fScaleY;
};




#define WARPBALL_DEFAULT_SPR_MAIN			"sprites/Fexplo1.spr"
//#define WARPBALL_DEFAULT_SPR_EFFECT			"sprites/XFlare1.spr"
#define WARPBALL_DEFAULT_SND_MAIN			"debris/beamstart2.wav"
//#define WARPBALL_DEFAULT_SND_EFFECT			"debris/beamstart7.wav"
#define WARPBALL_MIN_RADIUS			60
#define SF_WARPBALL_ONCE			0x0001

class CEnvWarpBall : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int m_iSprite1;
	int m_iSprite2;
	int m_iSpriteBeam;
};




#define SF_SHOCKWAVE_CENTERED		0x0001
#define SF_SHOCKWAVE_REPEATABLE		0x0002

class CEnvShockwave : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	void DoEffect(Vector vecPos);

	int m_iTime;
	int m_iRadius;
	int	m_iHeight;
	int m_iScrollRate;
	int m_iNoise;
	int m_iFrameRate;
	int m_iStartFrame;
	int m_iSpriteTexture;
	char m_cType;
	int m_iszPosition;
};




#define SF_DLIGHT_ONLYONCE			0x0001
#define SF_DLIGHT_STARTON			0x0002

class CEnvDLight : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void PostSpawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Think(void);
	virtual void DesiredAction(void);
	virtual void MakeLight(int iTime);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	EHANDLE m_hAttach;
	Vector m_vecPos;
	BOOL elight;
};




#define SF_FOUNTAIN_START_OFF		0x0001
#define SF_FOUNTAIN_SPARKS			0x0002
#define SF_FOUNTAIN_RANDOM_FRAME	0x0004
#define SF_FOUNTAIN_SCALE			0x0008
#define SF_FOUNTAIN_ZROTATION		0x0010
#define SF_FOUNTAIN_ONCE			0x0020
#define SF_FOUNTAIN_RANDOM_DIR		0x0040
#define SF_FOUNTAIN_FRAMES_ONCE		0x0080

class CEnvFountain : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
//	void UpdatePos(void);
//	virtual int Save(CSave &save);
//	virtual int Restore(CRestore &restore);
//	static TYPEDESCRIPTION m_SaveData[];
//	BOOL bRandVec;
//	BOOL bState;
//	BOOL bEntFound;
//	CBaseEntity *pTargetEnt;
};



/*
class CEnvSun : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
};
*/



class CLavaBall : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
};




#define SF_LAVA_START_OFF 0x0001

class CBaseLava : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	void EXPORT LavaThink(void);
	void EXPORT LavaUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};




class CEnvLightAttachment : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Think(void);
};




/*#define SF_ROTPART_START_OFF		0x0001

class CEnvRotParticles : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Think(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};*/




class CEnvSky : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
};




#define SF_ENVFLAMESPAWNER_START_OFF	0x0001

class CEnvFlameSpawner : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};



//#define SF_ENVSTATIC_UNUSED			0x000001
#define SF_ENVSTATIC_START_INVISIBLE	0x000002
#define SF_ENVSTATIC_CLIENTONLY			0x800000// 8388608 // XDM3038c: NOUSER: set/unset once per map during precache

// XDM3038: future TODO
enum ENVSTATIC_DETAILTYPE_E
{
	ENVSTATIC_DT_INTERACTIVE = 0,// almost an entity, high LOD, collisions, etc.
	ENVSTATIC_DT_DECORATION,// does nothing, but resides in the playable area
	ENVSTATIC_DT_VISUAL// demands fewest attention, safe to be omitted, distant objects, 3D sky elements, etc.
};

class CEnvStatic : public CBaseAnimating
{
public:
	virtual int	ObjectCaps(void) { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }// don't go across transitions
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool ValidateModel(void);// XDM3038c
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
	virtual bool IsTriggered(const CBaseEntity *pActivator) { return (pev->impulse > 0); }// XDM3038
	virtual bool IsAlive(void) const { return false; }
};

#endif //ENVIRONMENT_H
