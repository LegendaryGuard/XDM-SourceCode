#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"
#include "squeakgrenade.h"
#include "globals.h"
#include "game.h"


LINK_ENTITY_TO_CLASS(monster_snark, CSqueakGrenade);

TYPEDESCRIPTION	CSqueakGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CSqueakGrenade, m_flDie, FIELD_TIME),
	DEFINE_FIELD(CSqueakGrenade, m_vecTarget, FIELD_VECTOR),
	DEFINE_FIELD(CSqueakGrenade, m_flNextHunt, FIELD_TIME),
	DEFINE_FIELD(CSqueakGrenade, m_flNextHit, FIELD_TIME),
	DEFINE_FIELD(CSqueakGrenade, m_posPrev, FIELD_POSITION_VECTOR),
};

IMPLEMENT_SAVERESTORE(CSqueakGrenade, CBaseMonster);//CGrenade);

int CSqueakGrenade::Classify(void)
{
	if (m_iClass != CLASS_NONE)
		return m_iClass;

	// should never get here
	if (m_hOwner != NULL && m_hOwner->IsPlayer())
		return CLASS_PLAYER_BIOWEAPON;
	else
		return CLASS_ALIEN_BIOWEAPON;
}

int CSqueakGrenade::IRelationship(CBaseEntity *pTarget)
{
	if (!pTarget->IsProjectile())
	{
		/* done in R table	if (pTarget->IsMonster())
		{
			if (pTarget->Classify() == CLASS_ALIEN_MILITARY)
				return R_NM;// XDM3037
			else
				return R_HT;// XDM3034
		}
		else */if (pTarget->IsPlayer())// XDM3034
		{
			if (m_hOwner != NULL)
			{
				if (mp_friendlyfire.value <= 0.0f)
				{
					if (m_hOwner == pTarget)
						return R_NO;

					if (g_pGameRules && g_pGameRules->IsTeamplay() && m_hOwner->pev->team == pTarget->pev->team)
						return R_NO;
				}
			}
			return R_HT;
		}
	}
	return /*CGrenade*/CBaseMonster::IRelationship(pTarget);
}

void CSqueakGrenade::Spawn(void)
{
	if (pev->health == 0)
		pev->health = gSkillData.snarkHealth;

	pev->max_health = pev->health;
	pev->gravity *= 0.5;// XDM3038
	pev->friction *= 0.5;// XDM3038
	if (pev->dmg == 0)
		pev->dmg = gSkillData.snarkDmgBite;

	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	SET_MODEL(edict(), STRING(pev->model));// XDM3037
	UTIL_SetSize(this, Vector(-4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin(this, pev->origin);
	SetTouch(&CSqueakGrenade::SuperBounceTouch);
	SetThink(&CSqueakGrenade::HuntThink);
	SetNextThink(0.1);// XDM3038a
	m_bloodColor = BLOOD_COLOR_YELLOW;
	m_iGibCount = 0;
	m_flFieldOfView = 0; // 180 degrees
	m_flDie = gpGlobals->time + SQUEAK_DETONATE_DELAY;
	m_flNextHunt = gpGlobals->time + 1E6;
	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.
	if (m_hOwner)
	{
		if (m_hOwner->IsPlayer())
			m_iClass = CLASS_PLAYER_BIOWEAPON;
		else if (m_hOwner->IsMonster())
			m_iClass = CLASS_ALIEN_BIOWEAPON;
		else if (m_iClass == CLASS_NONE)
			m_iClass = CLASS_ALIEN_PREY;// XDM3034 =)))))))))) CLASS_ALIEN_MILITARY;
	}
	else if (m_iClass == CLASS_NONE)
		m_iClass = CLASS_ALIEN_PREY;// XDM3037

	ResetSequenceInfo();
	pev->sequence = max(0,LookupSequence("run"));// XDM
}

void CSqueakGrenade::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/w_squeak.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "squeek");

	CBaseMonster::Precache();
	PRECACHE_SOUND("squeek/sqk_blast1.wav");
	PRECACHE_SOUND("squeek/sqk_die1.wav");
	PRECACHE_SOUND("squeek/sqk_hunt1.wav");
	PRECACHE_SOUND("squeek/sqk_hunt2.wav");
	PRECACHE_SOUND("squeek/sqk_hunt3.wav");
	PRECACHE_SOUND("squeek/sqk_deploy1.wav");
}

// XDM3037a: ignore poison damage (but accept mixed damage)
int CSqueakGrenade::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if ((bitsDamageType & ~DMGM_POISON) != 0)// not poison-only bits
		return CBaseMonster::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);

	return 0;
}

void CSqueakGrenade::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->model = iStringNull;// make invisible
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	SetThink(&CBaseEntity::SUB_Remove);
	SetTouchNull();
	SetNextThink(0.1);// XDM3038a
	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	RadiusDamage(pev->origin, this, GetDamageAttacker(), gSkillData.snarkDmgPop, gSkillData.snarkDmgPop*DAMAGE_TO_RADIUS_DEFAULT, CLASS_NONE, DMG_POISON|DMG_DONT_BLEED);// XDM3037
	EMIT_SOUND_DYN(edict(), CHAN_ITEM, "squeek/sqk_blast1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0);
	Vector vecDir = UTIL_RandomBloodVector();
	UTIL_BloodDrips(pev->origin, vecDir, BloodColor(), 60);
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		StreakSplash(pev->origin, vecDir, 5, 16, 50, 200);

	//Destroy();// XDM3037: simpler, faster, not counted as monsterkill
}

bool CSqueakGrenade::GibMonster(void)
{
	//EMIT_SOUND_DYN(edict(), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);// XDM: too small to make this sound!
	return false;
}

void CSqueakGrenade::HuntThink(void)
{
	if (!IsInWorld())
	{
		SetTouchNull();
		Destroy();
		return;
	}

	// explode when ready
	if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity;
		g_vecAttackDir.NormalizeSelf();
		pev->health = -1;
		Killed(this, this, GIB_NEVER);// XDM3034
		return;
	}

	StudioFrameAdvance();
	SetNextThink(0.1);// XDM3038a

	// float
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
			pev->movetype = MOVETYPE_FLY;

		pev->velocity *= 0.9f;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype == MOVETYPE_FLY)
		pev->movetype = MOVETYPE_BOUNCE;

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;
	//CBaseEntity *pOther = NULL;
	//Vector vecDir;
	TraceResult tr;
	Vector vecFlat(pev->velocity);
	vecFlat.z = 0;
	vecFlat.NormalizeSelf();

	UTIL_MakeVectors(pev->angles);

	if (m_hEnemy.Get() == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look(SQUEAK_LOOK_RADIUS);
		m_hEnemy = BestVisibleEnemy();
	}

	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5) && (m_flDie - gpGlobals->time >= 0.3))
	{
		pev->scale = 2.0;// XDM :)
		pev->renderfx = kRenderFxExplode;// XDM3034 :)))
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "squeek/sqk_die1.wav", VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(0,0x3F));
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 256, 0.25);
	}

	if (sv_overdrive.value > 0.0f)
		pev->scale = 0.5f + 1.0f-((m_flDie - gpGlobals->time) / SQUEAK_DETONATE_DELAY);

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0f - 60.0f * ((m_flDie - gpGlobals->time) / SQUEAK_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != NULL)
	{
		if (FVisible(m_hEnemy))
		{
			//vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = (m_hEnemy->EyePosition() - pev->origin).Normalize();//vecDir.Normalize();
		}

		vec_t flVel = pev->velocity.Length();
		vec_t flAdj = 50.0f / (flVel + 10.0f);
		if (flAdj > 1.2f)
			flAdj = 1.2f;

		//conprintf(1, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );
		pev->velocity *= flAdj;
		pev->velocity += m_vecTarget * 300.0f;
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		pev->avelocity.Clear();
	}
	else
	{
		if (pev->avelocity.IsZero())
		{
			pev->avelocity.x = RANDOM_FLOAT(-100, 100);
			pev->avelocity.z = RANDOM_FLOAT(-100, 100);
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0f)
	{
		pev->velocity.x = RANDOM_FLOAT(-100, 100);
		pev->velocity.y = RANDOM_FLOAT(-100, 100);
	}
	m_posPrev = pev->origin;

	VectorAngles(pev->velocity, pev->angles);
	pev->angles.z = 0;
	pev->angles.x = 0;
}

void CSqueakGrenade::SuperBounceTouch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this grenade
	if (m_hOwner.Get() && pOther == m_hOwner)
		return;

	// at least until we've bounced once
	pev->owner = NULL;
	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too often
	if (m_flNextHit > gpGlobals->time)
		return;

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0f - 60.0f * ((m_flDie - gpGlobals->time) / SQUEAK_DETONATE_DELAY);
	TraceResult tr;
	UTIL_GetGlobalTrace(&tr);// ??? is it set for us??
	if (pOther->pev->takedamage && m_flNextAttack < gpGlobals->time)
	{
		if (tr.pHit == pOther->edict())// make sure it's me who has touched them
		{
			if (tr.pHit->v.modelindex != pev->modelindex)// and it's not another squeakgrenade
			{
				//conprintf( 1, "hit enemy\n");
				ClearMultiDamage();
				pOther->TraceAttack(this, pev->dmg, gpGlobals->v_forward, &tr, DMG_SLASH);
				if (m_hOwner != NULL)
					ApplyMultiDamage(this, m_hOwner);
				else
					ApplyMultiDamage(this, this);

				//pev->dmg += gSkillData.snarkDmgPop; // add more explosion damage
				// m_flDie += 2.0; // add more life
				EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "squeek/sqk_deploy1.wav", VOL_NORM, ATTN_NORM, 0, (int)flpitch);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		//else
		//	conprintf(1, "been hit\n");
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
	if (gpGlobals->time < m_flNextBounceSoundTime)
		return;

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		switch (RANDOM_LONG(0,2))
		{
			case 0:	EMIT_SOUND_DYN(edict(), CHAN_VOICE, "squeek/sqk_hunt1.wav", VOL_NORM, ATTN_NORM, 0, (int)flpitch); break;
			case 1:	EMIT_SOUND_DYN(edict(), CHAN_VOICE, "squeek/sqk_hunt2.wav", VOL_NORM, ATTN_NORM, 0, (int)flpitch); break;
			case 2:	EMIT_SOUND_DYN(edict(), CHAN_VOICE, "squeek/sqk_hunt3.wav", VOL_NORM, ATTN_NORM, 0, (int)flpitch); break;
		}
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 256, 0.25);
		if (sv_overdrive.value > 0.0f)
		{
			if (RANDOM_LONG(0,2) == 0)//if (pev->teleport_time <= gpGlobals->time)
			{
				CAGrenade::ShootTimed(pev->origin + UTIL_RandomBloodVector()*4.0f, g_vecZero,
					(/*m_vecNormal + */RandomVector(VECTOR_CONE_45DEGREES)) * RANDOM_FLOAT(10, 20),
					m_hOwner, this, m_flDie - gpGlobals->time + RANDOM_FLOAT(1.0, 2.0), gSkillData.DmgAcidGrenade, 0);// wath out for overload!

				//pev->teleport_time = gpGlobals->time + 0.4f;
			}
		}
	}
	else// skittering sound
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 100, 0.1);

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}



LINK_ENTITY_TO_CLASS(squeakbox, CSqueakBox);

void CSqueakBox::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(this, Vector(-4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin(this, pev->origin);
	pev->gravity = 0.75;
	pev->friction = 0.8;
	pev->impulse = SQUEAKBOX_NUM_SQUEAKS;
	pev->dmg = 0;// OFF
	pev->sequence = SQUEAK_BOX_IDLE;
	SetBodygroup(PSQUEAK_BODYGROUP_BOX, PSQUEAK_BODY_BOX_ON);
	SetBodygroup(PSQUEAK_BODYGROUP_BODY, PSQUEAK_BODY_BODY_OFF);
}

void CSqueakBox::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/w_sqknest.mdl");

	CBaseAnimating::Precache();
}

void CSqueakBox::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, pev->dmg > 0))
		Open();
}

void CSqueakBox::Open(void)
{
	pev->dmg = pev->impulse;// ON: XDM3037: DMG so bots can react to it
	pev->body = 1;// XDM3037: open
	pev->sequence = SQUEAK_BOX_OPEN;//pev->sequence = max(0,LookupSequence("open"));// XDM3037: don't let it be -1
	pev->skin = 0;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;
	SetNextThink(0.2);// XDM3038a
	pev->dmgtime = gpGlobals->time + SQUEAKBOX_RELEASE_DELAY;
	SetThink(&CSqueakBox::SqueakBoxThink);// XDM3037: otherwise disintegration won't work
}

void CSqueakBox::SqueakBoxThink(void)
{
	if (pev->waterlevel == WATERLEVEL_HEAD)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity *= 0.8f;
		pev->avelocity *= 0.9f;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == WATERLEVEL_NONE)
		pev->movetype = MOVETYPE_TOSS;
	else
		pev->velocity.z -= 8.0f;

	if (pev->dmgtime < gpGlobals->time)
	{
		if (pev->impulse == SQUEAKBOX_NUM_SQUEAKS)// XDM3037: start releasing
			CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 200, 2.0);

		if (pev->impulse > 0)
		{
			Vector vecDir(UTIL_RandomBloodVector());
			Vector vecAng;
			VectorAngles(vecDir, vecAng);
			if (Create(SQUEAK_CLASSNAME, pev->origin + vecDir * RANDOM_FLOAT(3,5), vecAng, vecDir * 100, m_hOwner.Get()) != NULL)
				pev->impulse--;
		}
		if (pev->impulse <= 0)
		{
			pev->dmg = 0;
			Disintegrate();// XDM3037: not just remove
			return;
		}
	}
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (pev->skin == 0)
		{
			AlignToFloor();
			pev->skin = 1;
		}
		SetNextThink(0.2);// XDM3038a
	}
	else
		SetNextThink(0.1);// XDM3038a
}
