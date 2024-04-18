/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "globals.h"

#define SF_WAITFORTRIGGER	0x40
#define MAX_CARRY			24
#define MAX_REPEL_GRUNTS	4

class COsprey : public CBaseMonster
{
public:
	virtual int ObjectCaps(void) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void) { return CLASS_MACHINE; }
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// NEW
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual bool IsPushable(void) { return (pev->health > 0)?FALSE:TRUE; }// XDM: for explosions
	virtual void DeathNotice(CBaseEntity *pChild);// XDM3038

	static TYPEDESCRIPTION m_SaveData[];

	void ShowDamage(void);
	void UpdateGoal(void);
	void Flight(void);
	BOOL HasDead(void);
	CBaseMonster *MakeGrunt(const Vector &vecSrc);

	void EXPORT FlyThink(void);
	void EXPORT DeployThink(void);
	void EXPORT HitTouch(CBaseEntity *pOther);
	void EXPORT FindAllThink(void);
	void EXPORT HoverThink(void);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	Vector m_vel1;
	Vector m_vel2;
	Vector m_pos1;
	Vector m_pos2;
	Vector m_ang1;
	Vector m_ang2;
	float m_startTime;
	float m_dTime;
	Vector m_velocity;
	float m_flIdealtilt;
	float m_flRotortilt;
	float m_flRightHealth;
	float m_flLeftHealth;
	int	m_iUnits;
	EHANDLE m_hGrunt[MAX_CARRY];
	Vector m_vecOrigin[MAX_CARRY];
	EHANDLE m_hRepel[MAX_REPEL_GRUNTS];
	//EHANDLE m_hBeams[MAX_REPEL_GRUNTS];// XDM3038
	int m_iSoundState;
	int m_iSpriteTexture;
	int m_iPitch;
	int m_iExplode;
	int	m_iTailGibs;
	int	m_iBodyGibs;
	int	m_iEngineGibs;
	int m_iDoLeftSmokePuff;
	int m_iDoRightSmokePuff;
};

LINK_ENTITY_TO_CLASS( monster_osprey, COsprey );

TYPEDESCRIPTION	COsprey::m_SaveData[] =
{
//	DEFINE_FIELD( COsprey, m_pGoalEnt, FIELD_CLASSPTR ),
	DEFINE_FIELD( COsprey, m_vel1, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_vel2, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_pos1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COsprey, m_pos2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COsprey, m_ang1, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_ang2, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_startTime, FIELD_TIME ),
	DEFINE_FIELD( COsprey, m_dTime, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_velocity, FIELD_VECTOR ),
	DEFINE_FIELD( COsprey, m_flIdealtilt, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_flRotortilt, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_flRightHealth, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_flLeftHealth, FIELD_FLOAT ),
	DEFINE_FIELD( COsprey, m_iUnits, FIELD_INTEGER ),
	DEFINE_ARRAY( COsprey, m_hGrunt, FIELD_EHANDLE, MAX_CARRY ),
	DEFINE_ARRAY( COsprey, m_vecOrigin, FIELD_POSITION_VECTOR, MAX_CARRY ),
	DEFINE_ARRAY( COsprey, m_hRepel, FIELD_EHANDLE, MAX_REPEL_GRUNTS ),
	// DEFINE_FIELD( COsprey, m_iSoundState, FIELD_INTEGER ),
	// DEFINE_FIELD( COsprey, m_iSpriteTexture, FIELD_INTEGER ),
	// DEFINE_FIELD( COsprey, m_iPitch, FIELD_INTEGER ),
	DEFINE_FIELD( COsprey, m_iDoLeftSmokePuff, FIELD_INTEGER ),
	DEFINE_FIELD( COsprey, m_iDoRightSmokePuff, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( COsprey, CBaseMonster );

void COsprey :: Spawn(void)
{
	if (pev->health <= 0)
		pev->health = gSkillData.ospreyHealth;
	if (pev->armorvalue <= 0)
		pev->armorvalue = 50 * gSkillData.iSkillLevel;// XDM
	if (m_iGibCount == 0)
		m_iGibCount = 128;
	if (m_iScoreAward == 0)
		m_iScoreAward = gSkillData.ospreyScore;

	m_bloodColor = DONT_BLEED;
	CBaseMonster::Spawn();// XDM3038b: Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	//SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetSize(this, Vector(-400, -400, -100), Vector(400, 400, 32));
	UTIL_SetOrigin(this, pev->origin);
	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	pev->max_health = pev->health;// XDM3038a
	m_flRightHealth = pev->max_health/2;
	m_flLeftHealth = pev->max_health/2;
	m_flFieldOfView = 0; // 180 degrees
	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0,0xFF);
	InitBoneControllers();
	SetThink(&COsprey::FindAllThink);
	SetUse(&COsprey::CommandUse);

	if (!FBitSet(pev->spawnflags, SF_WAITFORTRIGGER))
		SetNextThink(1.0);

	m_pos2 = pev->origin;
	m_ang2 = pev->angles;
	m_vel2 = pev->velocity;
	SetBits(pev->spawnflags, SF_NORESPAWN);// XDM3038b: force no respawn for this one
}

void COsprey::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/osprey.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "apache");

	CBaseMonster::Precache();// XDM3038a

	PRECACHE_SOUND("apache/ap_whine1.wav");
	PRECACHE_SOUND("apache/ap_rotor4.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");
	m_iSpriteTexture	= PRECACHE_MODEL("sprites/rope.spr");
	m_iExplode			= PRECACHE_MODEL("sprites/fexplo.spr");
	m_iTailGibs			= PRECACHE_MODEL("models/osprey_tailgibs.mdl");
	m_iBodyGibs			= PRECACHE_MODEL("models/osprey_bodygibs.mdl");
	m_iEngineGibs		= PRECACHE_MODEL("models/osprey_enginegibs.mdl");
	UTIL_PrecacheOther("monster_human_grunt");
}

void COsprey::CommandUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetNextThink(0.1);
}

void COsprey :: FindAllThink(void)
{
	m_iUnits = min(MAX_CARRY, gSkillData.iSkillLevel*2);// XDM3035a
	for (int i = 0; i < m_iUnits; ++i)
	{
		m_hGrunt[i] = NULL;
		m_vecOrigin[i] = pev->origin;
	}

/*
	CBaseEntity *pEntity = NULL;

	m_iUnits = 0;
	while (m_iUnits < MAX_CARRY && (pEntity = UTIL_FindEntityByClassname( pEntity, "monster_human_grunt" )) != NULL)
	{
		if (pEntity->IsAlive())
		{
			m_hGrunt[m_iUnits]		= pEntity;
			m_vecOrigin[m_iUnits]	= pEntity->pev->origin;
			m_iUnits++;
		}
	}
*/
	if (m_iUnits == 0)
	{
		ALERT( at_console, "osprey error: no grunts to resupply\n");
		UTIL_Remove( this );
		return;
	}
	SetThink(&COsprey::FlyThink);
	SetNextThink(0.1);
	m_startTime = gpGlobals->time;
}

void COsprey :: DeployThink(void)
{
	UTIL_MakeAimVectors( pev->angles );
	Vector vecForward = gpGlobals->v_forward;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	Vector vecSrc;
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0.0f, 0.0f, -4096.0f), ignore_monsters, ENT(pev), &tr);
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, tr.vecEndPos, 300, 0.25 );
	vecSrc = pev->origin + vecForward *  32 + vecRight *  100 + vecUp * -96;
	m_hRepel[0] = MakeGrunt( vecSrc );
	vecSrc = pev->origin + vecForward * -64 + vecRight *  100 + vecUp * -96;
	m_hRepel[1] = MakeGrunt( vecSrc );
	vecSrc = pev->origin + vecForward *  32 + vecRight * -100 + vecUp * -96;
	m_hRepel[2] = MakeGrunt( vecSrc );
	vecSrc = pev->origin + vecForward * -64 + vecRight * -100 + vecUp * -96;
	m_hRepel[3] = MakeGrunt( vecSrc );

	SetThink(&COsprey::HoverThink);
	SetNextThink(0.1);
}

BOOL COsprey :: HasDead( )
{
	for (int i = 0; i < m_iUnits; i++)
	{
		if (m_hGrunt[i].Get() == NULL || !m_hGrunt[i]->IsAlive())
		{
			return TRUE;
		}
		else
		{
			m_vecOrigin[i] = m_hGrunt[i]->pev->origin;  // send them to where they died
		}
	}
	return FALSE;
}

CBaseMonster *COsprey::MakeGrunt(const Vector &vecSrc)
{
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + Vector( 0.0f, 0.0f, -4096.0f), dont_ignore_monsters, ENT(pev), &tr);
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
		return NULL;

	CBaseEntity *pEntity;
	CBaseMonster *pGrunt;
	for (int i = 0; i < m_iUnits; ++i)
	{
		if (m_hGrunt[i].Get() == NULL || !m_hGrunt[i]->IsAlive())
		{
			if (m_hGrunt[i] != NULL && m_hGrunt[i]->pev->rendermode == kRenderNormal)
				m_hGrunt[i]->SUB_StartFadeOut();

			pEntity = Create("monster_human_grunt", vecSrc, pev->angles, Vector(0.0f, 0.0f, RANDOM_FLOAT(-192, -128)), edict(), SF_MONSTER_FALL_TO_GROUND | SF_NOREFERENCE | SF_NORESPAWN);// XDM3038
			if (pEntity)
			{
				pGrunt = pEntity->MyMonsterPointer();
				if (pGrunt == NULL)
					continue;
				//pGrunt->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND | SF_NOREFERENCE | SF_NORESPAWN;// XDM3035c
				pGrunt->pev->movetype = MOVETYPE_FLY;
				//pGrunt->pev->velocity.Set(0.0f, 0.0f, RANDOM_FLOAT(-196, -128));
				pGrunt->m_hOwner = this;
				// does this do anything?				pGrunt->m_hActivator = this;
				pGrunt->SetActivity( ACT_GLIDE );
				CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
				if (pBeam)
				{
					pBeam->PointEntInit(vecSrc + Vector(0,0,112), pGrunt->entindex());
					pBeam->SetFlags(BEAM_FSOLID);
					pBeam->SetColor(255, 255, 255);
					pBeam->SetNoise(RANDOM_LONG(0,2));// XDM3035a
					pBeam->SetThink(&CBaseEntity::SUB_Remove);
					pBeam->SetNextThink((-4096.0 * tr.flFraction / pGrunt->pev->velocity.z) + 0.5);
					//m_hBeams[?] = pBeam;
				}
				// ALERT( at_console, "%d at %.0f %.0f %.0f\n", i, m_vecOrigin[i].x, m_vecOrigin[i].y, m_vecOrigin[i].z );
				pGrunt->m_vecLastPosition = m_vecOrigin[i];
				m_hGrunt[i] = pGrunt;
				return pGrunt;
			}
		}
	}
	ALERT(at_aiconsole, "COsprey::MakeGrunt() failed\n");
	return NULL;
}

// XDM3038: VERY IMPORTANT! If a grunt was died on a rope, the engine will freak out and crash while trying to check rope's visibility!
void COsprey::DeathNotice(CBaseEntity *pChild)
{
	for (int i = 0; i < m_iUnits; ++i)
	{
		if (m_hGrunt[i] == pChild)
		{
			m_hGrunt[i] = NULL;
			break;// delete this line if searching for beams
		}
/*UNDONE: right now UTIL_IsValidEntity() in AddToFullPack() does the job
		if (m_hBeams[i]->GetEndEntity() == pChild)
		{
			UTIL_Remove(m_hBeams[i]);
			m_hBeams[i] = NULL;
		}*/
	}
}

void COsprey :: HoverThink(void)
{
	size_t i;
	for (i = 0; i < MAX_REPEL_GRUNTS; ++i)
	{
		if (m_hRepel[i] != NULL && m_hRepel[i]->pev->health > 0 && !(m_hRepel[i]->pev->flags & FL_ONGROUND))
			break;
	}

	if (i == MAX_REPEL_GRUNTS)
	{
		m_startTime = gpGlobals->time;
		SetThink(&COsprey::FlyThink);
	}

	SetNextThink(0.1);
	ShowDamage( );
}

void COsprey::UpdateGoal( )
{
	if (m_pGoalEnt)
	{
		m_pos1 = m_pos2;
		m_ang1 = m_ang2;
		m_vel1 = m_vel2;
		m_pos2 = m_pGoalEnt->pev->origin;
		m_ang2 = m_pGoalEnt->pev->angles;
		UTIL_MakeAimVectors( Vector( 0.0f, m_ang2.y, 0.0f ) );
		m_vel2 = gpGlobals->v_forward * m_pGoalEnt->pev->speed;

		m_startTime = m_startTime + m_dTime;
		m_dTime = 2.0 * (m_pos1 - m_pos2).Length() / (m_vel1.Length() + m_pGoalEnt->pev->speed);

		if (m_ang1.y - m_ang2.y < -180)
		{
			m_ang1.y += 360;
		}
		else if (m_ang1.y - m_ang2.y > 180)
		{
			m_ang1.y -= 360;
		}

		if (m_pGoalEnt->pev->speed < 400)
			m_flIdealtilt = 0;
		else
			m_flIdealtilt = -90;
	}
	else
	{
		ALERT( at_console, "Osprey missing target!\n");
	}
}

void COsprey::FlyThink(void)
{
	StudioFrameAdvance( );
	SetNextThink(0.1);

	if ( m_pGoalEnt == NULL && !FStringNull(pev->target) )// this monster has a target
	{
		m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( pev->target ) ) );
		UpdateGoal( );
	}

	if (gpGlobals->time > m_startTime + m_dTime)
	{
		if (m_pGoalEnt->pev->speed == 0)
		{
			SetThink(&COsprey::DeployThink);
		}
		do {
			m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( m_pGoalEnt->pev->target ) ) );
		} while (m_pGoalEnt->pev->speed < 400 && !HasDead());
		UpdateGoal( );
	}

	Flight( );
	ShowDamage( );
}

void COsprey::Flight( )
{
	float t = (gpGlobals->time - m_startTime);
	float scale = 1.0 / m_dTime;

	float f = UTIL_SplineFraction( t * scale, 1.0 );

	Vector pos = (m_pos1 + m_vel1 * t) * (1.0 - f) + (m_pos2 - m_vel2 * (m_dTime - t)) * f;
	Vector ang = (m_ang1) * (1.0 - f) + (m_ang2) * f;
	m_velocity = m_vel1 * (1.0 - f) + m_vel2 * f;

	UTIL_SetOrigin(this, pos);
	pev->angles = ang;
	UTIL_MakeAimVectors( pev->angles );
	float flSpeed = DotProduct( gpGlobals->v_forward, m_velocity );

	// float flSpeed = DotProduct( gpGlobals->v_forward, pev->velocity );
	float m_flIdealtilt = (160 - flSpeed) / 10.0;

	// ALERT( at_console, "%f %f\n", flSpeed, flIdealtilt );
	if (m_flRotortilt < m_flIdealtilt)
	{
		m_flRotortilt += 0.5;
		if (m_flRotortilt > 0)
			m_flRotortilt = 0;
	}
	if (m_flRotortilt > m_flIdealtilt)
	{
		m_flRotortilt -= 0.5;
		if (m_flRotortilt < -90)
			m_flRotortilt = -90;
	}
	SetBoneController( 0, m_flRotortilt );


	if (m_iSoundState == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor4.wav", 1.0, 0.15, 0, 110 );
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		// Is this a hack to emulate doppler effect?!
		if (!IsMultiplayer())// XDM3037: UNDONE: use BUILD_SOUND_MSG for every player
		{
		CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		// UNDONE: this needs to send different sounds to every player for multiplayer.
		if (pPlayer)
		{
			float pitch = DotProduct( m_velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize() );

			pitch = (int)(100 + pitch / 75.0);

			if (pitch > 250)
				pitch = 250;
			else if (pitch < 50)
				pitch = 50;
			else if (pitch == 100)
				pitch = 101;

			if (pitch != m_iPitch)
			{
				m_iPitch = pitch;
				EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor4.wav", VOL_NORM, 0.15, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
				EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.9, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
				// ALERT( at_console, "%.0f\n", pitch );
			}
		}
		}
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
	}
}

void COsprey::HitTouch(CBaseEntity *pOther)
{
	SetNextThink(2.0);
}

/*
int COsprey::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (m_flRotortilt <= -90)
	{
		m_flRotortilt = 0;
	}
	else
	{
		m_flRotortilt -= 45;
	}
	SetBoneController( 0, m_flRotortilt );
	return 0;
}
*/

void COsprey::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;
	pev->velocity = m_velocity;
	pev->avelocity.Set( RANDOM_FLOAT( -20, 20 ), 0.0f, RANDOM_FLOAT( -50, 50 ) );
	STOP_SOUND(ENT(pev), CHAN_STATIC, "apache/ap_rotor4.wav");
	STOP_SOUND(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav");
	UTIL_SetSize(this, Vector(-32, -32, -64), Vector(32, 32, 0));
	SetThink(&COsprey::DyingThink);
	SetTouch(&COsprey::CrashTouch);
	SetNextThink(0.1);
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	m_startTime = gpGlobals->time + 4.0;
}

void COsprey::CrashTouch(CBaseEntity *pOther)
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP)
	{
		SetTouchNull();
		m_startTime = gpGlobals->time;
		SetNextThink(0);
		m_velocity = pev->velocity;
	}
}

void COsprey :: DyingThink(void)
{
	StudioFrameAdvance( );
	SetNextThink(0.1);
	pev->avelocity *= 1.02;
	// still falling?
	if (m_startTime > gpGlobals->time )
	{
		ShowDamage( );
		Vector vecSpot = pev->origin + pev->velocity * 0.2;
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_EXPLOSION);				// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( vecSpot.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( vecSpot.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_iModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG(0,29) + 30  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
		if (g_pGameRules->FAllowEffects())
		{
		// lots of smoke
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( vecSpot.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( vecSpot.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_iModelIndexSmoke );
			WRITE_BYTE( 100 );			// scale * 10
			WRITE_BYTE( 10  );			// framerate
		MESSAGE_END();
		}
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);
			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );
			// size
			WRITE_COORD( 800 );
			WRITE_COORD( 800 );
			WRITE_COORD( 132 );
			// velocity
			WRITE_COORD( pev->velocity.x );
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );
			// randomization
			WRITE_BYTE( 50 );
			// Model
			WRITE_SHORT( m_iTailGibs );	//model id#
			// # of shards
			WRITE_BYTE( 8 );			// let client decide
			// duration
			WRITE_BYTE( 200 );			// 10.0 seconds
			// flags
			WRITE_BYTE( BREAK_METAL | BREAK_SMOKE );// XDM
		MESSAGE_END();
		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		SetNextThink(0.2);
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		// gibs
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 250 );			// scale * 10
			WRITE_BYTE( 255 );			// brightness
		MESSAGE_END();
		// blast circle
		MESSAGE_BEGIN( MSG_PAS, svc_temp_entity, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 2000 ); // reach damage radius over .2 seconds
			WRITE_SHORT( g_iModelIndexShockWave );
			WRITE_BYTE( 0 );	// startframe
			WRITE_BYTE( 0 );	// framerate
			WRITE_BYTE( 4 );	// life
			WRITE_BYTE( 50 );	// width
			WRITE_BYTE( 0 );	// noise
			WRITE_BYTE( 255 );	// r, g, b
			WRITE_BYTE( 255 );	// r, g, b
			WRITE_BYTE( 100 );	// r, g, b
			WRITE_BYTE( 128 );	// brightness
			WRITE_BYTE( 0 );	// speed
		MESSAGE_END();
		if (g_pGameRules->FAllowEffects())
		{
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSpot );// XDM3038
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300 );
			WRITE_SHORT( g_iModelIndexSmoke );
			WRITE_BYTE( 250 );			// scale * 10
			WRITE_BYTE( 6 );			// framerate
		MESSAGE_END();
		}
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", VOL_NORM, 0.3);
		pev->takedamage = DAMAGE_NO;
		RadiusDamage(pev->origin, this, this, pev->max_health, pev->max_health*DAMAGE_TO_RADIUS_DEFAULT, CLASS_NONE, DMG_BLAST);
		UTIL_ScreenShake(pev->origin, 5, 5, 3, pev->max_health*DAMAGE_TO_RADIUS_DEFAULT);// XDM
		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PAS, svc_temp_entity, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);
			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 64);
			// size
			WRITE_COORD( 800 );
			WRITE_COORD( 800 );
			WRITE_COORD( 128 );
			// velocity
			WRITE_COORD( m_velocity.x );
			WRITE_COORD( m_velocity.y );
			WRITE_COORD( fabs( m_velocity.z ) * 0.25 );
			// randomization
			WRITE_BYTE( 40 );
			// Model
			WRITE_SHORT( m_iBodyGibs );		// model id#
			// # of shards
			WRITE_BYTE( m_iGibCount );
			// duration
			WRITE_BYTE( 200 );
			// flags
			if (pev->waterlevel > WATERLEVEL_NONE)// XDM
				WRITE_BYTE(BREAK_METAL);
			else
				WRITE_BYTE(BREAK_METAL | BREAK_SMOKE);

		MESSAGE_END();
		SetThinkNull();
		CBaseMonster::Killed(g_pWorld, m_hTBDAttacker, GIB_REMOVE);// XDM3038a: this will call game rules logic
	}
}

void COsprey :: ShowDamage(void)
{
	UTIL_MakeVectors(pev->angles);// XDM3038a
	if (m_iDoLeftSmokePuff > 0 || RANDOM_LONG(0,99) > m_flLeftHealth)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_right * -340;
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSrc );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( g_iModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG(0,9) + 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
		if (m_iDoLeftSmokePuff > 0)
			m_iDoLeftSmokePuff--;
	}
	if (m_iDoRightSmokePuff > 0 || RANDOM_LONG(0,99) > m_flRightHealth)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_right * 340;
		MESSAGE_BEGIN( MSG_PVS, svc_temp_entity, vecSrc );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( g_iModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG(0,9) + 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
		if (m_iDoRightSmokePuff > 0)
			m_iDoRightSmokePuff--;
	}
}

int COsprey::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	int iRet = CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
	if (iRet > 0)
		m_hTBDAttacker = pAttacker;// XDM3038a: HACK!

	return iRet;
}

void COsprey::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// only so much per engine
	if (ptr->iHitgroup == 3)
	{
		if (m_flRightHealth < 0)
			return;
		else
			m_flRightHealth -= flDamage;

		m_iDoRightSmokePuff = 3 + (flDamage / 5.0);// XDM3038a: fixed
	}
	else if (ptr->iHitgroup == 2)// XDM3038a: else
	{
		if (m_flLeftHealth < 0)
			return;
		else
			m_flLeftHealth -= flDamage;

		m_iDoLeftSmokePuff = 3 + (flDamage / 5.0);// XDM3038a: fixed
	}

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == HITGROUP_HEAD || ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH || FBitSet(bitsDamageType, DMG_ENERGYBEAM|DMG_IGNOREARMOR|DMG_WALLPIERCING))// XDM3038a: certain damage types hit
	{
		//CBaseMonster::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);// XDM3038c ?
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pAttacker, this, flDamage, bitsDamageType );
	}
	else if (flDamage > 4.0f)
	{
		UTIL_Sparks( ptr->vecEndPos );
	}
}
