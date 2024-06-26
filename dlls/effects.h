/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef EFFECTS_H
#define EFFECTS_H


#define SF_SPRITE_STARTON		0x000001// another bad move by valve
#define SF_SPRITE_ONCE			0x000002
#define SF_SPRITE_TEMPORARY		0x008000// 32768 // NOUSER: only set from code
#define SF_SPRITE_CLIENTONLY	0x800000// 8388608 // XDM3038c: NOUSER: set/unset once per map during precache

// Warning: now it uses almost the same technique as env_static, but not entirely
class CSprite : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);// XDM
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int ObjectCaps(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool IsTriggered(const CBaseEntity *pActivator) { return (pev->impulse > 0); }// XDM3038
	virtual bool ValidateModel(void);// XDM3037a
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);// XDM3035a
	virtual bool StartOn(void);// XDM3038c

	virtual void TurnOff(void);
	virtual void TurnOn(void);

	void Animate(float frames);
	void AnimateAndDie(const float &framerate);
	void Expand(float scaleSpeed, float fadeSpeed);

	void EXPORT AnimateThink(void);
	void EXPORT ExpandThink(void);
	void EXPORT AnimateUntilDead(void);

	void SetAttachment(edict_t *pEntity, int attachment);
	float Frames(void) { return m_maxFrame; }
	inline void SetTransparency(int rendermode, int r, int g, int b, int a, int fx)
	{
		pev->rendermode = rendermode;
		pev->rendercolor.x = (float)r;
		pev->rendercolor.y = (float)g;
		pev->rendercolor.z = (float)b;
		pev->renderamt = (float)a;
		pev->renderfx = fx;
	}
	inline void SetTexture(int spriteIndex) { pev->modelindex = spriteIndex; }
	inline void SetScale(const float &scale) { pev->scale = scale; }
	inline void SetColor(int r, int g, int b) { pev->rendercolor.x = (float)r; pev->rendercolor.y = (float)g; pev->rendercolor.z = (float)b; }
	inline void SetBrightness(int brightness) { pev->renderamt = (float)brightness; }

	static CSprite *SpriteCreate(const char *pSpriteName, const Vector &origin, bool animate);
	static TYPEDESCRIPTION m_SaveData[];

protected:
	float		m_lastTime;
	float		m_maxFrame;
};



#define SF_GLOW_START_OFF			0x0001// XDM: the only difference between this and base class

// XDM: UNDONE: cannot replace glow with static RenderSystem because there's no way to render glow using TriAPI
class CGlow : public CSprite
{
public:
	virtual bool StartOn(void);// XDM3038c
};




#define SF_BEAM_STARTON			0x0001
#define SF_BEAM_TOGGLE			0x0002
#define SF_BEAM_RANDOM			0x0004
#define SF_BEAM_RING			0x0008
#define SF_BEAM_SPARKSTART		0x0010
#define SF_BEAM_SPARKEND		0x0020
#define SF_BEAM_DECALS			0x0040
#define SF_BEAM_SHADEIN			0x0080
#define SF_BEAM_SHADEOUT		0x0100
#define SF_BEAM_SOLID			0x0200// XDM

#define SF_BEAM_TEMPORARY		0x8000

class CBeam : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int ObjectCaps(void);
	virtual Vector Center(void) { return (GetStartPos() + GetEndPos()) * 0.5f; } // center point of beam

	//void EXPORT TriggerTouch(CBaseEntity *pOther);

	// These functions are here to show the way beams are encoded as entities.
	// Encoding beams as entities simplifies their management in the client/server architecture
	inline void	SetType(int type) { pev->rendermode = (pev->rendermode & 0xF0) | (type&0x0F); }
	inline void	SetFlags(int flags) { pev->rendermode = (pev->rendermode & 0x0F) | (flags&0xF0); }
	inline void SetStartPos(const Vector &pos) { pev->origin = pos; }
	inline void SetEndPos(const Vector &pos) { pev->angles = pos; }
	void SetStartEntity(int entityIndex);
	void SetEndEntity(int entityIndex);

	inline void SetStartAttachment(int attachment) { pev->sequence = (pev->sequence & 0x0FFF) | ((attachment&0xF)<<12); }
	inline void SetEndAttachment(int attachment) { pev->skin = (pev->skin & 0x0FFF) | ((attachment&0xF)<<12); }

	inline void SetTexture(int spriteIndex) { pev->modelindex = spriteIndex; }
	inline void SetWidth(int width) { pev->scale = (float)width; }
	inline void SetNoise(int amplitude) { pev->body = amplitude; }
	inline void SetColor(int r, int g, int b) { pev->rendercolor.x = (float)r; pev->rendercolor.y = (float)g; pev->rendercolor.z = (float)b; }
	inline void SetBrightness(int brightness) { pev->renderamt = (float)brightness; }
	inline void SetFrame(float frame) { pev->frame = frame; }
	inline void SetScrollRate(int speed) { pev->animtime = (float)speed; }

	inline int	GetType(void) { return pev->rendermode & 0x0F; }
	inline int	GetFlags(void) { return pev->rendermode & 0xF0; }
	inline int	GetStartEntity(void) { return pev->sequence & 0xFFF; }
	inline int	GetEndEntity(void) { return pev->skin & 0xFFF; }

	const Vector &GetStartPos(void);
	const Vector &GetEndPos(void);

	inline int GetTexture(void) { return pev->modelindex; }
	inline int GetWidth(void) { return (int)pev->scale; }
	inline int GetNoise(void) { return pev->body; }
	// inline void GetColor( int r, int g, int b ) { pev->rendercolor.x = r; pev->rendercolor.y = g; pev->rendercolor.z = b; }
	inline int GetBrightness(void) { return (int)pev->renderamt; }
	inline int GetFrame(void) { return (int)pev->frame; }
	inline int GetScrollRate(void) { return (int)pev->animtime; }

	// Call after you change start/end positions
	void RelinkBeam(void);
	//virtual void SetObjectCollisionBox(void);
	void DoSparks(const Vector &start, const Vector &end);
	CBaseEntity *RandomTargetname(const char *szName);
	void BeamDamage(TraceResult *ptr);// XDM3037
	// Init after BeamCreate()
	void BeamInit(const char *pSpriteName, int width);
	void PointsInit(const Vector &start, const Vector &end);
	void PointEntInit(const Vector &start, int endIndex);
	void EntPointInit(int endIndex, const Vector &end);// XDM
	void EntsInit(int startIndex, int endIndex);
	void HoseInit(const Vector &start, const Vector &direction);

	inline void LiveForTime(float time) { SetThink(&CBaseEntity::SUB_Remove); SetNextThink(time); }
	/* OBSOLETE inline void	BeamDamageInstant(TraceResult *ptr, float damage)
	{
		pev->dmg = damage;
		pev->dmgtime = gpGlobals->time - 1.0f;
		BeamDamage(ptr);// XDM3037
	}*/
	void Expand(float scaleSpeed, float fadeSpeed); // XDM
	void EXPORT ExpandThink(void);
	//CBaseEntity *GetTripEntity(TraceResult *ptr);

	static CBeam *BeamCreate(const char *pSpriteName, int width);
};




class CLightning : public CBeam
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Activate(void);
	//virtual int ObjectCaps(void) { return (CBeam::ObjectCaps() | FCAP_MUST_SPAWN); }// XDM3038: doesn't fix save/restore rings bug :(
	virtual STATE GetState(void) { return m_active?STATE_OFF:STATE_ON; }// XDM
	virtual int SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);// XDM3038

	void EXPORT StrikeThink(void);
	void EXPORT UpdateThink(void);
	void EXPORT DamageThink(void);// XDM3038c: more inline than a real think

	bool ServerSide(void);
	void RandomArea(void);
	void RandomPoint(Vector &vecSrc);
	void Zap(const Vector &vecSrc, const Vector &vecDest);

	void EXPORT StrikeUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	void BeamUpdateVars(void);

	int			m_active;
	string_t	m_iszStartEntity;
	string_t	m_iszEndEntity;
	float		m_life;
	int			m_boltWidth;
	int			m_noiseAmplitude;
	int			m_brightness;
	int			m_speed;
	float		m_restrike;
	int			m_spriteTexture;
	string_t	m_iszSpriteName;
	int			m_frameStart;
	float		m_radius;
};




class CLaser : public CBeam
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void UpdateOnRemove(void);
	virtual void TurnOn(void);
	virtual void TurnOff(void);
	virtual int IsOn(void);

	void FireAtPoint(TraceResult &point);// XDM3037

	void EXPORT TurnOffThink(void);// XDM3035
	void EXPORT StrikeThink(void);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CSprite	*m_pSprite;
	int		m_iszSpriteName;
	Vector  m_firePosition;
};




#define	SF_GIBSHOOTER_REPEATABLE	0x0001 // allows a gibshooter to be refired

class CGib;
// TODO: re-check this on existing maps!
class CGibShooter : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void KeyValue(KeyValueData *pkvd);
	void EXPORT ShootThink(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual CGib *CreateGib(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	int	m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
	float m_flGibLife;
};




#define SF_BLOOD_RANDOM			0x0001
#define SF_BLOOD_STREAM			0x0002
#define SF_BLOOD_PLAYER			0x0004
#define SF_BLOOD_DECAL			0x0008

class CEnvBlood : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	inline int Color(void) { return pev->impulse; }
	inline float BloodAmount(void) { return pev->dmg; }
	inline void SetColor(int color) { pev->impulse = color; }
	inline void SetBloodAmount(float amount) { pev->dmg = amount; }

	Vector Direction(void);
	Vector BloodPosition(CBaseEntity *pActivator);
};




#define SF_SHAKE_EVERYONE		0x0001		// Don't check radius
#define SF_SHAKE_DISRUPT		0x0002		// Disrupt controls
#define SF_SHAKE_INAIR			0x0004		// Shake players in air
#define SF_SHAKE_STARTON		0x0008		// XDM3038c: for constant effect

class CEnvShake : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void EXPORT ShakeThink(void);
	void SetDuration(const float &duration);

	inline float Amplitude(void) { return pev->scale; }
	inline float Frequency(void) { return pev->dmg_save; }
	inline float Duration(void) { return pev->dmg_take; }
	inline float Radius(void) { return pev->dmg; }

	inline void SetAmplitude( float amplitude ) { pev->scale = amplitude; }
	inline void SetFrequency( float frequency ) { pev->dmg_save = frequency; }
	inline void SetRadius( float radius ) { pev->dmg = radius; }
};




#define SF_FADE_IN				0x0001		// Fade in, not out
#define SF_FADE_MODULATE		0x0002		// Modulate, don't blend
#define SF_FADE_ONLYONE			0x0004
#define SF_FADE_STAYOUT			0x0008		// XDM
#define SF_FADE_DIRECTVISIBLE	0x0010		// XDM3035a: affect directly visible clients
#define SF_FADE_FACING			0x0020		// XDM3035a: affect only clients looking at this entity

class CEnvFade : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	inline float Duration(void) { return pev->dmg_take; }
	inline float HoldTime(void) { return pev->dmg_save; }

	inline void	SetDuration(float duration) { pev->dmg_take = duration; }
	inline void	SetHoldTime(float hold) { pev->dmg_save = hold; }
};




#define SF_MESSAGE_ONCE			0x0001		// Fade in, not out
#define SF_MESSAGE_ALL			0x0002		// Send to all clients

class CEnvMessage : public CPointEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void KeyValue(KeyValueData *pkvd);
};




#define SF_FUNNEL_REVERSE		0x0001 // funnel effect repels particles instead of attracting them.
#define SF_FUNNEL_REPEATABLE	0x0002 // allows a funnel to be refired

class CEnvFunnel : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	int m_iSprite;	// Don't save, precache
};




#define SF_ENVSPARK_TOGGLE		0x0020
#define SF_ENVSPARK_STARTON		0x0040
#define SF_ENVSPARK_LIGHT		0x0080

class CEnvSpark : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void EXPORT SparkThink(void);
};




// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX		(1<<0)
#define SF_RENDER_MASKAMT		(1<<1)
#define SF_RENDER_MASKMODE		(1<<2)
#define SF_RENDER_MASKCOLOR		(1<<3)

class CRenderFxManager : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Apply(entvars_t *pevTarget);
};

#endif		//EFFECTS_H
