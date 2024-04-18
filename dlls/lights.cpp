#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define NUM_STANDARD_LIGHTSTYLES		32

// WARNING!! LAST READABLE INDEX MUST BE 12! MUST BE NULL-TERMINATED!
const char *g_szStandardLightStyles[] =
{
	"m",// 0 normal
	"mmnmmommommnonmmonqnmmo",// 1 FLICKER (first variety)
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",// 2 SLOW STRONG PULSE
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",// 3 CANDLE (first variety)
	"mamamamamama",// 4 FAST STROBE
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",// 5 GENTLE PULSE 1
	"nmonqnmomnmomomno",// 6 FLICKER (second variety)
	"mmmaaaabcdefgmmmmaaaammmaamm",// 7 CANDLE (second variety)
	"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",// 8 CANDLE (third variety)
	"aaaaaaaazzzzzzzz",// 9 SLOW STROBE (fourth variety)
	"mmamammmmammamamaaamammma",// 10 FLUORESCENT FLICKER
	"abcdefghijklmnopqrrqponmlkjihgfedcba",// 11 SLOW PULSE NOT FADE TO BLACK
	"mmnnmmnnnmmnn",// 12 UNDERWATER LIGHT MUTATION - this light only distorts the lightmap - no contribution is made to the brightness of affected surfaces
	NULL// -------- DO NOT ADD ANYTHING HERE!!! --------
};


class CLight : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd); 
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	//virtual int ObjectCaps(void) { return (CPointEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_NOT_MASTER | FCAP_IGNORE_PARENT; }
	void EXPORT LightThink(void);

	virtual STATE GetState(void) { return m_iState; }// XDM!!!
	string_t GetStyle(void) { return m_iszCurrentStyle; };
	void SetStyle(int iszPattern);
	void SetCorrectStyle(void);

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int			m_iStyle;
	string_t	m_iszPattern;
	string_t	m_iszCurrentStyle;	// current style string
	STATE		m_iState;// current state
	int			m_iOnStyle;// style to use while on
	int			m_iOffStyle;// style to use while off
	int			m_iTurnOnStyle;// style to use while turning on
	int			m_iTurnOffStyle;// style to use while turning off
	int			m_iTurnOnTime;// time taken to turn on
	int			m_iTurnOffTime;// time taken to turn off
};

LINK_ENTITY_TO_CLASS(light, CLight);

TYPEDESCRIPTION	CLight::m_SaveData[] = 
{
	DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iszPattern, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CLight, CPointEntity);

void CLight::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "_light"))// XDM: don't show error in the console!
	{
		int r,g,b,a, n;
		n = sscanf(pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &a);
		if (n >= 3)// XDM3037: fixed
		{
			if (n == 4)
				pev->renderamt = a;
			else
			{
				conprintf(1, "Warning: fixing bad %s in entity %s: \"%s\"\n", pkvd->szKeyName, pkvd->szClassName, pkvd->szValue);
				pev->renderamt = 255;// ?
			}

			pev->rendercolor.Set(r,g,b);
			pkvd->fHandled = TRUE;
		}
		else
			pkvd->fHandled = FALSE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iOnStyle"))
	{
		m_iOnStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iOffStyle"))
	{
		m_iOffStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOnStyle"))
	{
		m_iTurnOnStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOffStyle"))
	{
		m_iTurnOffStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOnTime"))
	{
		m_iTurnOnTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOffTime"))
	{
		m_iTurnOffTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "_cone"))// [VXZ]HLT
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "_cone2"))// [VXZ]HLT
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "_fade"))// [VXZ]HLT
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "_sky"))// [VXZ]HLT
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "_falloff"))// [VXZ]HLT
		pkvd->fHandled = TRUE;
	else
		CPointEntity::KeyValue(pkvd);
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/
void CLight::Spawn(void)
{
	if (FStringNull(pev->targetname))// inert light
	{
		REMOVE_ENTITY(edict());
		return;
	}
	CPointEntity::Spawn();
	if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
		m_iState = STATE_OFF;
	else
		m_iState = STATE_ON;

	SetCorrectStyle();
	SetThink(&CLight::LightThink);
}

void CLight::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (m_iStyle >= NUM_STANDARD_LIGHTSTYLES)
	{
		if (!ShouldToggle(useType, GetState() == STATE_ON))
			return;

		switch (GetState())
		{
		case STATE_ON:
		case STATE_TURN_ON:
			if (m_iTurnOffTime)
			{
				m_iState = STATE_TURN_OFF;
				SetNextThink(m_iTurnOffTime);
			}
			else
				m_iState = STATE_OFF;
			break;
		case STATE_OFF:
		case STATE_TURN_OFF:
			if (m_iTurnOnTime)
			{
				m_iState = STATE_TURN_ON;
				SetNextThink(m_iTurnOnTime);
			}
			else
				m_iState = STATE_ON;
			break;
		}
		SetCorrectStyle();
	}
}

void CLight::SetStyle(int iszPattern)
{
	if (m_iStyle < NUM_STANDARD_LIGHTSTYLES) // if it's using a global style, don't change it
		return;

	m_iszCurrentStyle = iszPattern;
	LIGHT_STYLE(m_iStyle, STRINGV(iszPattern));
}

// regardless of what's been set by trigger_lightstyle ents, set the style I think I need
void CLight::SetCorrectStyle(void)
{
	if (m_iStyle >= NUM_STANDARD_LIGHTSTYLES)
	{
		switch (m_iState)
		{
		case STATE_ON:
			if (m_iszPattern) // custom styles have priority over standard ones
				SetStyle(m_iszPattern);
			else if (m_iOnStyle)
				SetStyle(MAKE_STRING(g_szStandardLightStyles[m_iOnStyle]));
			else
				SetStyle(MAKE_STRING("m"));
			break;
		case STATE_OFF:
			if (m_iOffStyle)
				SetStyle(MAKE_STRING(g_szStandardLightStyles[m_iOffStyle]));
			else
				SetStyle(MAKE_STRING("a"));
			break;
		case STATE_TURN_ON:
			if (m_iTurnOnStyle)
				SetStyle(MAKE_STRING(g_szStandardLightStyles[m_iTurnOnStyle]));
			else
				SetStyle(MAKE_STRING("a"));
			break;
		case STATE_TURN_OFF:
			if (m_iTurnOffStyle)
				SetStyle(MAKE_STRING(g_szStandardLightStyles[m_iTurnOffStyle]));
			else
				SetStyle(MAKE_STRING("m"));
			break;
		}
	}
	else
		m_iszCurrentStyle = MAKE_STRING(g_szStandardLightStyles[m_iStyle]);
}

void CLight::LightThink(void)
{
	switch (GetState())
	{
	case STATE_TURN_ON:
		m_iState = STATE_ON;
		FireTargets(STRING(pev->target), this, this, USE_ON, 0);
		DontThink();// XDM3038c
		break;
	case STATE_TURN_OFF:
		m_iState = STATE_OFF;
		FireTargets(STRING(pev->target), this, this, USE_OFF, 0);
		DontThink();
		break;
	}
	SetCorrectStyle();
}



//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );


class CEnvLight : public CLight
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

void CEnvLight::KeyValue( KeyValueData* pkvd )
{
	if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r, g, b, v, j;
		char szColor[16];
		j = sscanf(pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v);
		if (j == 1)
		{
			g = b = r;
		}
		else if (j == 4)
		{
			r *= ((float)v / 255.0f);
			g *= ((float)v / 255.0f);
			b *= ((float)v / 255.0f);
		}
		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		r = pow( r / 114.0, 0.6 ) * 264;
		g = pow( g / 114.0, 0.6 ) * 264;
		b = pow( b / 114.0, 0.6 ) * 264;

		_snprintf(szColor, 16, "%d", r);// XDM3038b
		CVAR_SET_STRING( "sv_skycolor_r", szColor );
		_snprintf(szColor, 16, "%d", g);
		CVAR_SET_STRING( "sv_skycolor_g", szColor );
		_snprintf(szColor, 16, "%d", b);
		CVAR_SET_STRING( "sv_skycolor_b", szColor );
		// XDM: TODO: why not CVAR_SET_FLOAT??
		pkvd->fHandled = TRUE;
	}
	else
		CLight::KeyValue(pkvd);
}

void CEnvLight::Spawn(void)
{
	UTIL_MakeAimVectors( pev->angles );
	/*char szVector[16];
	_snprintf(szVector, 16, "%g", gpGlobals->v_forward.x);// XDM3038b: WARNING! %g will not do because MAKE_VECTORS may produce values like "1234e-8"!
	CVAR_SET_STRING( "sv_skyvec_x", szVector );
	_snprintf(szVector, 16, "%g", gpGlobals->v_forward.y);
	CVAR_SET_STRING( "sv_skyvec_y", szVector );
	_snprintf(szVector, 16, "%g", gpGlobals->v_forward.z);
	CVAR_SET_STRING( "sv_skyvec_z", szVector );*/
	CVAR_SET_FLOAT("sv_skyvec_x", gpGlobals->v_forward.x);
	CVAR_SET_FLOAT("sv_skyvec_y", gpGlobals->v_forward.y);
	CVAR_SET_FLOAT("sv_skyvec_z", gpGlobals->v_forward.z);
	CLight::Spawn();
}


#if defined(SHL_LIGHTS)
// SHL compatibility: changes the style of a light temporarily
class CLightFader : public CPointEntity
{
public:
	void EXPORT FadeThink(void);
	void EXPORT WaitThink(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

protected:
	CLight		*m_pLight;
	char		m_cFrom;
	char		m_cTo;
	char		m_szCurStyle[2];
	float		m_fEndTime;
	string_t	m_iszPattern;
	float		m_fStep;
	int			m_iWait;
};

LINK_ENTITY_TO_CLASS(lightfader, CLightFader);

TYPEDESCRIPTION	CLightFader::m_SaveData[] = 
{
	DEFINE_FIELD( CLightFader, m_pLight, FIELD_CLASSPTR ),
	DEFINE_FIELD( CLightFader, m_cFrom, FIELD_CHARACTER ),
	DEFINE_FIELD( CLightFader, m_cTo, FIELD_CHARACTER ),
	DEFINE_ARRAY( CLightFader, m_szCurStyle, FIELD_CHARACTER, 2 ),
	DEFINE_FIELD( CLightFader, m_fEndTime, FIELD_FLOAT ),
	DEFINE_FIELD( CLightFader, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CLightFader, m_fStep, FIELD_FLOAT ),
	DEFINE_FIELD( CLightFader, m_iWait, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CLightFader, CPointEntity);

void CLightFader::FadeThink(void)
{
	if (m_fEndTime > gpGlobals->time)
	{
		m_szCurStyle[0] = m_cTo + (char)((m_cFrom - m_cTo) * (m_fEndTime - gpGlobals->time) * m_fStep);
		m_szCurStyle[1] = 0; // null terminator
		m_pLight->SetStyle(MAKE_STRING(m_szCurStyle));
		SetNextThink(0.1);
	}
	else
	{
		// fade is finished
		m_pLight->SetStyle(m_iszPattern);
		if (m_iWait > -1)
		{
			// wait until it's time to switch off
			SetThink(&CLightFader::WaitThink);
			SetNextThink(m_iWait);
		}
		else
		{
			// we've finished, kill the fader
			SetThink(&CLightFader::SUB_Remove);
			SetNextThink(0.1);
		}
	}
}

// Revert the light and kill the fader
void CLightFader::WaitThink(void)
{
	if (m_iszPattern)
		m_pLight->SetStyle(m_iszPattern);
	else
		m_pLight->SetCorrectStyle();

	SetThink(&CLightFader::SUB_Remove);
	SetNextThink(0.1);
}



class CTriggerLightstyle : public CPointEntity
{
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
private:
	char		m_szOldPattern[256];
	string_t	m_iszPattern;
	int			m_iFade;
	int			m_iWait;
};

LINK_ENTITY_TO_CLASS(trigger_lightstyle, CTriggerLightstyle);

TYPEDESCRIPTION	CTriggerLightstyle::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerLightstyle, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CTriggerLightstyle, m_iFade, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerLightstyle, m_iWait, FIELD_INTEGER ),
	DEFINE_ARRAY( CTriggerLightstyle, m_szOldPattern, FIELD_CHARACTER, 256 ),
};

IMPLEMENT_SAVERESTORE(CTriggerLightstyle, CPointEntity);

void CTriggerLightstyle::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFade"))
	{
		m_iFade = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iWait"))
	{
		m_iWait = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CTriggerLightstyle::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (FStringNull(pev->target))
		return;

	//conprintf(1, "Lightstyle change for: (%s)\n", STRING(pev->target) );
	CBaseEntity *pTarget = NULL;
	int iszPattern;
	while ((pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(pev->target), pActivator)) != NULL)
	{
		if (m_iszPattern)
			iszPattern = m_iszPattern;
		else
			iszPattern = MAKE_STRING(g_szStandardLightStyles[m_iStyle]);

		// not a light entity?
		if (!FClassnameIs(pTarget->pev, "light") && !FClassnameIs(pTarget->pev, "light_spot") && !FClassnameIs(pTarget->pev, "light_environment"))
		{
			if (pTarget->m_iStyle >= NUM_STANDARD_LIGHTSTYLES)
				LIGHT_STYLE(pTarget->m_iStyle, STRINGV(iszPattern));
		}
		else
		{
			CLight *pLight = (CLight *)pTarget;
			if (m_iFade)
			{
				CLightFader *pFader = GetClassPtr((CLightFader *)NULL);
				if (pFader)
				{
					pFader->SetClassName("lightfader");
					pFader->m_pLight = pLight;
					pFader->m_cFrom = (STRINGV(pLight->GetStyle()))[0];
					pFader->m_cTo = (STRINGV(iszPattern))[0];
					pFader->m_iszPattern = iszPattern;
					pFader->m_fEndTime = gpGlobals->time + m_iFade;
					pFader->m_fStep = 1.0f / m_iFade;
					pFader->m_iWait = m_iWait;
					pFader->SetThink(&CLightFader::FadeThink);
					pFader->SetNextThink(0.1);
				}
			}
			else
			{
				/* xash compatibility if (g_iXashEngineBuildNumber >= 2000)
				{
					// save old pattern in case we needs to be restore it
					Q_strncpy( m_szOldPattern, GET_LIGHT_STYLE(pLight->m_iStyle), 256); 
				}*/
				pLight->SetStyle(iszPattern);

				if (m_iWait != -1)
				{
					CLightFader *pFader = GetClassPtr( (CLightFader*)NULL );
					pFader->pev->classname = MAKE_STRING( "lightfader" );
					pFader->m_pLight = pLight;
					/* xash compatibility if (g_iXashEngineBuildNumber >= 2000)
					{
						// i'm hope somebody won't delete this entity from map :-)
						pFader->m_iszPattern = MAKE_STRING(m_szOldPattern);
					}*/
					pFader->SetThink( &CLightFader::WaitThink );
					pFader->SetNextThink( m_iWait );
				}
			}
		}
	}
}
#endif
