#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "game.h"
#include "globals.h"
#include "skill.h"

LINK_ENTITY_TO_CLASS(rpg_rocket, CRpgRocket);

TYPEDESCRIPTION	CRpgRocket::m_SaveData[] =
{
	DEFINE_FIELD(CRpgRocket, m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(CRpgRocket, m_hLauncher, FIELD_EHANDLE),// XDM3038c
};
IMPLEMENT_SAVERESTORE(CRpgRocket, CGrenade);

CRpgRocket *CRpgRocket::CreateRpgRocket(const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, CWeaponRPG *pLauncher, float fDamage)
{
	CRpgRocket *pNew = GetClassPtr((CRpgRocket *)NULL, "rpg_rocket");
	if (pNew)
	{
		pNew->pev->origin = vecOrigin;
		pNew->pev->angles = vecAngles;
		pNew->pev->velocity = vecDir; pNew->pev->velocity *= ROCKET_SPEED_START;
		if (pOwner)
		{
			pNew->pev->team = pOwner->pev->team;// XDM3037
			pNew->m_hOwner = pOwner;// XDM3037
		}
		pNew->pev->dmg = fDamage;// XDM3038c
		pNew->m_hLauncher = pLauncher;// remember what RPG fired me.
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
	}
	return pNew;
}

void CRpgRocket::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/rpgrocket.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));

	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	// XDM3038: does not work right now	m_iGlow = PRECACHE_MODEL("sprites/rocketflare1.spr");
	//PRECACHE_MODEL("sprites/rocketflare2.spr");

	pev->noise1 = MAKE_STRING("weapons/rocket_explode1.wav");
	pev->noise2 = MAKE_STRING("weapons/rocket_explode2.wav");
	// economy :( pev->noise3 = MAKE_STRING("weapons/rocket_explode3.wav");
	PRECACHE_SOUND("weapons/rocket1.wav");
	PRECACHE_SOUND(STRINGV(pev->noise1));
	PRECACHE_SOUND(STRINGV(pev->noise2));
	//PRECACHE_SOUND(STRINGV(pev->noise3));

	m_usRocket = PRECACHE_EVENT(1, "events/fx/rocket.sc");
}

void CRpgRocket::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_NO;
	pev->health = 4;
	pev->sequence = 0;// open
	pev->mins.Set(-2,-2,-2);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(2,2,2);
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgRPG;

	CBaseProjectile::Spawn();// XDM3037
	SetTouchNull();
	SetThink(&CRpgRocket::IgniteThink);
	SetNextThink(ROCKET_IGNITION_TIME);// XDM3038a
}

void CRpgRocket::IgniteThink(void)
{
	pev->movetype = MOVETYPE_FLY;
	if (sv_overdrive.value > 0.0f)
		pev->takedamage = DAMAGE_NO;
	else
		pev->takedamage = DAMAGE_YES;

	pev->effects |= EF_LIGHT;
	UTIL_SetSize(this, Vector(-2,-2,-2), Vector(2,2,2));
	UTIL_SetOrigin(this, pev->origin);

	//EMIT_SOUND(edict(), CHAN_BODY, "weapons/rocket1.wav", VOL_NORM, ATTN_NORM);
	PLAYBACK_EVENT_FULL(/*FEV_RELIABLE*/FEV_GLOBAL, edict(), m_usRocket, 0.0, pev->origin, pev->angles, ROCKET_LIFE_TIME, 0.0, m_iTrail, 0/*m_iGlow*/, 0, 0);

	m_flIgniteTime = gpGlobals->time;
	m_fEntIgnoreTimeInterval += (m_flIgniteTime - m_fLaunchTime);// XDM3037: increase ignore period..?
	pev->sequence = 1;// spin
	ResetSequenceInfo();

	if (sv_overdrive.value > 0.0f)
		pev->renderfx = kRenderFxExplode;

	//if (m_hLauncher) TEST
	//	SET_VIEW(m_hLauncher->GetOwner(), edict());

	//if (m_hLauncher)
		SetThink(&CRpgRocket::FollowThink);

	SetTouch(&CRpgRocket::RocketTouch);
	SetNextThink(0.1);// XDM3038a
	//conprintf(1, "CRpgRocket(%p)::IgniteThink(): m_pfnThink = %p\n", this, m_pfnThink);
}

void CRpgRocket::FollowThink(void)
{
	if (gpGlobals->time - m_flIgniteTime >= ROCKET_LIFE_TIME)
	{
		Detonate();
		return;
	}
	SetNextThink(0.05);// XDM3038a // datagram overflow

	Vector vecTarget;
	vec_t flSpeed = pev->velocity.NormalizeSelf();//Length();
	if (flSpeed < 100.0f)//fabs(pev->velocity.Volume()) < 100)// safety measure
	{
		Detonate();
		return;
	}
	bool bFollowing = false;
	if (!FNullEnt(pev->enemy) && gpGlobals->frametime > 0.0)// XDM: when player disables the spot, it is removed. When spot hits the sky, it has EF_NODRAW
	{
		CBaseEntity *pOther = CBaseEntity::Instance(pev->enemy);
		if (pOther)// let's pretend the rocket isn't folloing the DOT, but sees the BEAM. && !(gSkillData.iSkillLevel == SKILL_HARD && FBitSet(pev->enemy->v.effects, EF_NODRAW)))// XDM3037: don't follow invisible spot on hard level
		{
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pOther->Center(), dont_ignore_monsters, ignore_glass, edict(), &tr);
			if (tr.flFraction >= 0.90f)
			{
				Vector vecTargetDir(pOther->Center()); vecTargetDir -= pev->origin;
				vec_t flDist = vecTargetDir.NormalizeSelf();
				//Vector vecForward(pev->velocity);
				// velocity was already normalized vecForward.NormalizeSelf();
				vec_t flDot = DotProduct(pev->velocity, vecTargetDir);//DotProduct(vecForward, vecTargetDir);
				if ((flDot > 0.0f) && (flDist * (1.0f - flDot) < 4096.0f))//flMax))
				{
					//flMax = flDist * (1.0f - flDot);
					vecTarget = vecTargetDir;
					VectorAngles(vecTarget, pev->angles);// XDM3038c: prevent angle oscillaiton due to SQB
#if defined (SV_NO_PITCH_CORRECTION)
					pev->angles.x = -pev->angles.x;
#endif
					bFollowing = true;
				}
			}
		}
	}// pev->enemy

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	if (bFollowing)
	{
		//pev->velocity *= 0.2f; pev->velocity += vecTarget * (flSpeed * 0.8f + 400.0f);//pev->velocity = pev->velocity * 0.2f + vecTarget * (flSpeed * 0.8f + 400.0f);
		pev->velocity *= 0.125f; pev->velocity += vecTarget;// new direction to old direction = 10:1
		flSpeed *= 0.8f; flSpeed += 400.0f;
	}
	else
		flSpeed *= 1.25f;// just accelerate // TODO: insert frametime into formula??

	if (pev->waterlevel >= WATERLEVEL_HEAD)
	{
		if (flSpeed > ROCKET_SPEED_WATER)
			flSpeed = ROCKET_SPEED_WATER;
	}
	else
	{
		if (flSpeed > ROCKET_SPEED_AIR)
			flSpeed = ROCKET_SPEED_AIR;
	}
	pev->velocity.SetLength(flSpeed);

	if (sv_overdrive.value > 0.0f)
	{
		if (pev->teleport_time <= gpGlobals->time)
		{
			CLGrenade *pGrenade = CLGrenade::CreateGrenade(pev->origin + UTIL_RandomBloodVector()*4.0f, pev->angles, pev->velocity*0.5f + RandomVector(10.0f), m_hOwner, this, gSkillData.DmgGrenadeLauncher*0.8f, RANDOM_FLOAT(1.5f, 2.5f), false);
			if (pGrenade)
				pGrenade->pev->impulse = 0;// disable subgrenades

			pev->teleport_time = gpGlobals->time + 0.4f + pev->waterlevel*0.25f;
		}
	}
}

int CRpgRocket::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (FBitSet(bitsDamageType, DMGM_BREAK))// XDM3038a: don't explode because of poison clouds or radiation
		Detonate();

	return 1;
}

void CRpgRocket::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	Detonate();// reset launcher
}

void CRpgRocket::RocketTouch(CBaseEntity *pOther)
{
	if (sv_overdrive.value > 0.0f && pOther->IsProjectile())
		return;

	Detonate();
}

// return; after calling this!
void CRpgRocket::Detonate(void)
{
	if (m_hLauncher.IsValid())
		m_hLauncher->DeathNotice(this);

	m_hLauncher = NULL;
	pev->effects = EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	SetTouchNull();
	if (sv_reliability.value > 0)
	{
		STOP_SOUND(edict(), CHAN_BODY, "weapons/rocket1.wav");
		if (sv_reliability.value > 1)
		{
		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(TE_KILLBEAM);
			WRITE_SHORT(entindex());
		MESSAGE_END();
		}
	}

	if (sv_rpg_nuclear.value > 0)
	{
		SetThink(&CGrenade::NuclearExplodeThink);
		SetNextThink(0.0001);
	}
	else
	{
		switch (RANDOM_LONG(0,1))// 2 Explode() custom sound
		{
		case 0: pev->noise = pev->noise1; break;
		case 1: pev->noise = pev->noise2; break;
		//case 2: pev->noise = pev->noise3; break;
		}
		TraceResult tr;
		SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3035c: decorative, simplify
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity.Normalize()*64.0f, ignore_monsters, ignore_glass, edict(), &tr);
		ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
		if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
			pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * max(8.0f,pev->dmg*0.0625f));

		// XDM3038a	tr.vecPlaneNormal.x *= -1.0f;
		//	tr.vecPlaneNormal.y *= -1.0f;
		VectorAngles(-tr.vecPlaneNormal, pev->angles);// XDM3037a: explosion code calculates decal direction
		//UTIL_DebugAngles(pev->origin, pev->angles, 10.0f, 64.0f);
		//?	SetNextThink(0.25);
	}
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usRocket, 0, pev->origin, pev->angles, 0, 0, 0, 0, 1, 0);

	if (sv_rpg_nuclear.value <= 0.0f)
		Explode(pev->origin, DMG_BLAST, g_iModelIndexFireball, 0, g_iModelIndexWExplosion, 0, 0, STRING(pev->noise));
}
