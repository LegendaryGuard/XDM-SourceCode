#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "hornet.h"
#include "gamerules.h"
#include "game.h"

// TODO: rewrite as projectile

LINK_ENTITY_TO_CLASS(hornet, CHornet);

CHornet *CHornet::CreateHornet(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, bool track_target)
{
	CHornet *pHornet = GetClassPtr((CHornet *)NULL, "hornet");
	if (pHornet)
	{
		pHornet->pev->origin = vecSrc;
		pHornet->pev->angles = vecAng;
		pHornet->pev->velocity = vecVel;
		if (pOwner)
		{
			pHornet->m_hOwner = pOwner;
			pHornet->pev->team = pOwner->pev->team;// XDM3037
			if (pOwner->IsPlayer())
				pHornet->m_iClass = CLASS_PLAYER_BIOWEAPON;
			else if (pOwner->IsMonster())// XDM3038a
			{
				int iOwnerClass = pOwner->Classify();
				if (iOwnerClass == CLASS_NONE || iOwnerClass == CLASS_MACHINE)
					pHornet->m_iClass = CLASS_GRENADE;
				else
					pHornet->m_iClass = CLASS_ALIEN_BIOWEAPON;
			}
		}
		if (pEntIgnore)
			pHornet->pev->owner = pEntIgnore->edict();// XDM3037: since it's not CBaseProjectile

		pHornet->pev->dmg = fDamage;
		pHornet->Spawn();

		if (track_target)
			pHornet->SetThink(&CHornet::StartTrack);
		else
			pHornet->SetThink(&CHornet::StartDart);
	}
	return pHornet;
}

void CHornet::Spawn(void)
{
	pev->health = 1;// weak!
	pev->max_health = pev->health;
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	//pev->flags |= FL_MONSTER;
	pev->flags |= FL_FLY; // XDM
	SET_MODEL(edict(), STRING(pev->model));// XDM3037
	UTIL_SetSize(this, Vector(-4,-4,-4), Vector(4,4,4));
	UTIL_SetOrigin(this, pev->origin);
	SetTouch(&CHornet::DieTouch);
	//SetThink(&CHornet::StartTrack);
	SetThink(&CBaseEntity::SUB_DoNothing);// XDM
// entities are still bugged #if !defined (SV_NO_PITCH_CORRECTION)
	pev->angles[PITCH] = -pev->angles[PITCH];
//#endif
	pev->dmgtime = gpGlobals->time + HORNET_LIFE;
	m_flFieldOfView = 0.9;//+- 25 degrees
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 8;// XDM: move the glowshell farther from model
	}
	int c = RANDOM_LONG(0,127);
	if (m_hOwner.Get() && m_hOwner->IsPlayer())
	{
		if (pev->dmg == 0)
			pev->dmg = gSkillData.DmgHornet;

		pev->impulse = 1;
		if (RANDOM_LONG(0,1) == 0)
		{
			pev->rendercolor.Set(c,255,c);
			m_bloodColor = BLOOD_COLOR_GREEN;
			pev->skin = HORNET_TYPE_GREEN;
			//m_iHornetPuff = m_iHornetPuffG;
		}
		else
		{
			pev->rendercolor.Set(c,c,255);
			m_bloodColor = BLOOD_COLOR_BLUE;
			pev->skin = HORNET_TYPE_BLUE;
			//m_iHornetPuff = m_iHornetPuffB;
		}
	}
	else// no real owner, or owner isn't a client.
	{
		if (pev->dmg == 0)
			pev->dmg = gSkillData.monDmgHornet;

		pev->impulse = 0;
		if (RANDOM_LONG(0,1) == 0)
		{
			pev->rendercolor.Set(255,255,c);
			m_bloodColor = BLOOD_COLOR_YELLOW;
			pev->skin = HORNET_TYPE_YELLOW;
			//m_iHornetPuff = m_iHornetPuffY;
		}
		else
		{
			pev->rendercolor.Set(255,c,c);
			m_bloodColor = BLOOD_COLOR_RED;
			pev->skin = HORNET_TYPE_RED;
			//m_iHornetPuff = m_iHornetPuffR;
		}
	}
	if (g_pGameRules && !g_pGameRules->FAllowEffects())
		m_bloodColor = DONT_BLEED;

	pev->speed = RANDOM_LONG(800, 900);
	SetNextThink(0.1);// XDM3038a // trail creation will be delayed a bit
	CBaseAnimating::Spawn();// XDM3037
	pev->owner = m_hOwner.Get();// XDM3037: since it's not a CBaseProjectile
}

void CHornet::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/hornet.mdl");// XDM3037
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "hornet");

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	PRECACHE_SOUND("hornet/ag_puff.wav");
	PRECACHE_SOUND("hornet/ag_buzz1.wav");
	PRECACHE_SOUND("hornet/ag_buzz2.wav");
	// economy PRECACHE_SOUND("hornet/ag_buzz3.wav");
	// XDM3038b: economy :(	PRECACHE_SOUND("hornet/ag_hornethit1.wav");
	//PRECACHE_SOUND("hornet/ag_hornethit2.wav");
	PRECACHE_SOUND("hornet/ag_hornethit3.wav");
	/*m_iHornetPuffG = PRECACHE_MODEL("sprites/exp_green.spr");
	m_iHornetPuffB = PRECACHE_MODEL("sprites/exp_blue.spr");
	m_iHornetPuffY = PRECACHE_MODEL("sprites/exp_yellow.spr");
	m_iHornetPuffR = PRECACHE_MODEL("sprites/exp_red.spr");*/
	m_iHornetTrail = PRECACHE_MODEL("sprites/hornetbeam.spr");
	m_usHornet = PRECACHE_EVENT(1, "events/fx/hornet.sc");
}

// hornets will never get mad at each other, no matter who the owner is.
int CHornet::IRelationship(CBaseEntity *pTarget)
{
	if (pTarget->pev->modelindex == pev->modelindex)// TODO: RTTI
		return R_NO;

	if (g_pGameRules && g_pGameRules->IsTeamplay())
	{
		if (m_hOwner.Get() && (m_hOwner->pev->team == pTarget->pev->team))
			return R_NO;
	}

	return CBaseMonster::IRelationship(pTarget);
}

// Starts a hornet out tracking its target
void CHornet::StartTrack(void)
{
	IgniteTrail();
	SetTouch(&CHornet::TrackTouch);
	SetThink(&CHornet::TrackTarget);
	SetNextThink(0.1);// XDM3038a
}

// Starts a hornet out just flying straight.
void CHornet::StartDart(void)
{
	IgniteTrail();
	SetTouch(&CHornet::DartTouch);
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(4);// XDM3038a
}

void CHornet::IgniteTrail(void)
{
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BEAMFOLLOW);
		WRITE_SHORT(entindex());// entity
		WRITE_SHORT(m_iHornetTrail);// model
		WRITE_BYTE(RANDOM_LONG(8,10));// life
		WRITE_BYTE(2);// width
		WRITE_BYTE(pev->rendercolor.x);// XDM: use the rendercolor, defined in Spawn()
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_BYTE(RANDOM_LONG(191,223));// brightness
	MESSAGE_END();
	//PLAYBACK_EVENT_FULL(FEV_UPDATE, edict(), m_usHornet, 0.0, pev->origin, pev->angles, pev->dmg, 0, 0, m_bloodColor, pev->skin, HORNET_EV_START);// larger
}

// Hornet is flying, gently tracking target
void CHornet::TrackTarget(void)
{
	if (gpGlobals->time >= pev->dmgtime)
	{
		pev->health = 0.0;
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}

	StudioFrameAdvance();

	// UNDONE: The player pointer should come back after returning from another level
	if (m_hEnemy.Get() == NULL)
	{// enemy is dead.
		Look(HORNET_DETECT_DIST);
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != NULL && FVisible(m_hEnemy))
		m_vecEnemyLKP = m_hEnemy->BodyTarget(pev->origin);
	else
		m_vecEnemyLKP += pev->velocity * pev->speed * 0.1f;

	Vector vecDirToEnemy((m_vecEnemyLKP - pev->origin).Normalize());
	Vector vecFlightDir;

	if (pev->velocity.Length() < 0.1)
		vecFlightDir = vecDirToEnemy;
	else
	{
		vecFlightDir = pev->velocity; vecFlightDir.NormalizeSelf();
	}

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	vec_t flDelta = DotProduct(vecFlightDir, vecDirToEnemy);

	if (flDelta < 0.5)// hafta turn wide again. play sound
	{
		switch (RANDOM_LONG(0,1))
		{
		case 0:	EMIT_SOUND(edict(), CHAN_VOICE, "hornet/ag_buzz1.wav", VOL_NORM, ATTN_IDLE); break;
		case 1:	EMIT_SOUND(edict(), CHAN_VOICE, "hornet/ag_buzz2.wav", VOL_NORM, ATTN_IDLE); break;
		//case 2:	EMIT_SOUND(edict(), CHAN_VOICE, "hornet/ag_buzz3.wav", VOL_NORM, ATTN_IDLE); break;
		}
	}

	if (flDelta <= 0 && (pev->skin == HORNET_TYPE_GREEN || pev->skin == HORNET_TYPE_YELLOW))
		flDelta = 0.25f;// no flying backwards, but we don't want to invert this, cause we'd go fast when we have to turn REAL far.

	pev->velocity = vecFlightDir; pev->velocity += vecDirToEnemy; pev->velocity.NormalizeSelf();

	if ((sv_overdrive.value > 0.0f) || m_hOwner.Get() && m_hOwner->IsMonster())
	{
		// random pattern only applies to hornets fired by monsters, not players.
		pev->velocity.x += RANDOM_FLOAT(-0.10, 0.10);// scramble the flight dir a bit.
		pev->velocity.y += RANDOM_FLOAT(-0.10, 0.10);
		pev->velocity.z += RANDOM_FLOAT(-0.10, 0.10);
	}

	if (pev->skin == HORNET_TYPE_GREEN)
	{
		pev->velocity *= (pev->speed * flDelta);// scale the dir by the ( speed * width of turn )
		SetNextThink(RANDOM_FLOAT(0.1, 0.3));// XDM3038a
	}
	else if (pev->skin == HORNET_TYPE_BLUE)
	{
		pev->velocity *= pev->speed;// do not have to slow down to turn.
		SetNextThink(0.1);// XDM3038a // fixed think time
	}
	else if (pev->skin == HORNET_TYPE_YELLOW)
	{
		pev->velocity *= (pev->speed * flDelta);
		SetNextThink(RANDOM_FLOAT(0.1, 0.3));// XDM3038a
	}
	else if (pev->skin == HORNET_TYPE_RED)
	{
		pev->velocity *= pev->speed;
		SetNextThink(0.1);// XDM3038a
	}

	VectorAngles(pev->velocity, pev->angles);
//#if !defined (SV_NO_PITCH_CORRECTION)
	pev->angles.x = -pev->angles.x;
//#endif
	pev->solid = SOLID_BBOX;

	// if hornet is close to the enemy, jet in a straight line for a half second.
	if (m_hEnemy != NULL && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
	{
		if (flDelta >= 0.4 && (pev->origin - m_vecEnemyLKP).Length() <= 300)
		{
			/*MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
				WRITE_BYTE(TE_SPRITE);
				WRITE_COORD(pev->origin.x);// pos
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z);
				WRITE_SHORT(m_iHornetPuff);
				WRITE_BYTE(2);// size * 10
				WRITE_BYTE(128);//brightness
			MESSAGE_END();
			switch (RANDOM_LONG(0,2))
			{
			case 0:	EMIT_SOUND(edict(), CHAN_VOICE, "hornet/ag_buzz1.wav", VOL_NORM, ATTN_IDLE); break;
			case 1:	EMIT_SOUND(edict(), CHAN_VOICE, "hornet/ag_buzz2.wav", VOL_NORM, ATTN_IDLE); break;
			//case 2:	EMIT_SOUND(edict(), CHAN_VOICE, "hornet/ag_buzz3.wav", VOL_NORM, ATTN_IDLE); break;
			}*/
			PLAYBACK_EVENT_FULL(FEV_UPDATE, edict(), m_usHornet, 0.0, pev->origin, pev->angles, pev->dmg, 0, 0, m_bloodColor, pev->skin, HORNET_EV_BUZZ);
			pev->velocity *= 2.0f;
			SetNextThink(1.0);// XDM3038a
			// don't attack again
			pev->dmgtime = gpGlobals->time;
		}
	}
}

// Tracking Hornet hit something
void CHornet::TrackTouch(CBaseEntity *pOther)
{
	/* XDM3037: BAD	if (pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex)
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}*/
	if (IRelationship(pOther) <= R_NO)
	{
		// hit something we don't want to hurt, so turn around.
		pev->velocity.NormalizeSelf();
		pev->velocity.x *= -1;// SQB? no?
		pev->velocity.y *= -1;
		pev->origin += pev->velocity * 4;// bounce the hornet off a bit. XDM3037a: optimization
		pev->velocity *= pev->speed;
		return;
	}

	DieTouch(pOther);
}

void CHornet::DartTouch(CBaseEntity *pOther)
{
	DieTouch(pOther);
}

void CHornet::DieTouch(CBaseEntity *pOther)
{
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	if (pOther)
	{
		if (pOther->pev->takedamage != DAMAGE_NO)
		{
			pev->effects |= EF_MUZZLEFLASH;
			PLAYBACK_EVENT_FULL(FEV_UPDATE, edict(), m_usHornet, 0.0, pev->origin, pev->angles, pev->dmg, 0, 0, m_bloodColor, pev->skin, HORNET_EV_HIT);
			pev->owner = NULL;// XDM3037
			pOther->TakeDamage(this, m_hOwner, pev->dmg, DMG_BULLET);
		}
		if (pOther->IsPushable())// XDM3038
			pOther->pev->velocity += pev->velocity.Normalize() * pOther->DamageForce(pev->dmg);// XDM3038
	}
	Killed(pOther, pOther, GIB_NEVER);
}

int CHornet::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType != 0 && (bitsDamageType & ~DMGM_COLD) == 0)// XDM3038a: no other damage than cold
		return 0;

	PLAYBACK_EVENT_FULL(FEV_UPDATE, edict(), m_usHornet, 0.0, pev->origin, pev->angles, pev->dmg, 0, 0, m_bloodColor, pev->skin, HORNET_EV_DESTROY);
	/*EMIT_SOUND_DYN(edict(), CHAN_BODY, "hornet/ag_puff.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(m_iHornetPuff);
		WRITE_BYTE(5);
		WRITE_BYTE(200);
	MESSAGE_END();
	}*/
	Killed(pInflictor, pAttacker, GIB_NEVER);
	return 1;
}

void CHornet::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	//pev->modelindex = 0;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	//pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	pev->renderfx = kRenderFxNone;
	pev->health = 0.0;
	pev->takedamage = DAMAGE_NO;
	pev->owner = NULL;
	m_hOwner = NULL;
	SetTouchNull();
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(1.0f);// XDM3038a // stick around long enough for the event/sound to finish
}

int CHornet::ShouldCollide(CBaseEntity *pOther)// XDM3035
{
	if (pOther->IsProjectile() && pOther->pev->modelindex == pev->modelindex)// fast enough, TODO: RTTI
		return 0;

	return CBaseMonster::ShouldCollide(pOther);
}
