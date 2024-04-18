#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"
#include "skill.h"
#include "crossbowbolt.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"

LINK_ENTITY_TO_CLASS(bolt, CCrossbowBolt);

CCrossbowBolt *CCrossbowBolt::BoltCreate(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, bool sniper)
{
	CCrossbowBolt *pNew = GetClassPtr((CCrossbowBolt *)NULL, "bolt");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;
		pNew->pev->impulse = sniper;
		pNew->pev->dmg = fDamage;// XDM3038c
		if (pOwner)
		{
			pNew->pev->team = pOwner->pev->team;// XDM3037
			pNew->m_hOwner = pOwner;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		/*pNew->pev->basevelocity = */pNew->pev->velocity = vecDir; pNew->pev->velocity *= BOLT_AIR_VELOCITY;
	}
	return pNew;
}


void CCrossbowBolt::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_SLIDEBOX;
	pev->flags |= FL_FLY;
	//pev->framerate = 1.0f;

	if (sv_overdrive.value > 0.0f)
		pev->scale = 2.0f;// LOL

	pev->mins.Set(-1,-1,-1);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(1,1,1);
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgBolt;

	m_fEntIgnoreTimeInterval = 0.25f;// fast projectile
	CBaseProjectile::Spawn();// XDM3037

	SetThink(&CCrossbowBolt::BoltThink);

	if (pev->impulse > 0)
		SetTouch(&CCrossbowBolt::BoltTouch);
	else
		SetTouch(&CCrossbowBolt::BlowTouch);

	SetNextThink(0);
}

void CCrossbowBolt::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/crossbow_bolt.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	// XDM3038	m_iFireball = PRECACHE_MODEL("sprites/xbow_exp.spr");
	//m_iSpark = PRECACHE_MODEL("sprites/xbow_ric.spr");
	m_usBoltHit = PRECACHE_EVENT(1, "events/fx/bolthit.sc");
	// XDM3038: client precache	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	//PRECACHE_SOUND("weapons/xbow_hit2.wav");
	//PRECACHE_SOUND("weapons/xbow_exp.wav");
}

void CCrossbowBolt::BoltThink(void)
{
	if (pev->waterlevel > WATERLEVEL_WAIST && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
	{
		FX_BubblesLine(pev->origin - pev->velocity * 0.1f, pev->origin, 4);
		SetNextThink(0.1f);
	}
	else
		SetNextThink(0.05);// XDM3038a
}

void CCrossbowBolt::BoltTouch(CBaseEntity *pOther)
{
	int iSoundIndex;
	int iDecalIndex;
	int iHitType = 3;
	SetTouchNull();
	SetThink(&CBaseEntity::SUB_Remove);
	pev->owner = NULL;// XDM3037
	Vector vecDir(pev->velocity);
	vecDir.NormalizeSelf();
	Vector vecForward(vecDir);
	vecForward *= 32.0f;
	//AngleVectors(pev->angles, vecForward, NULL, NULL);// XDM3037: fixes many bugs
	Vector end(pev->origin + vecForward);
	//SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	TraceResult tr;
	UTIL_TraceLine(pev->origin, end, dont_ignore_monsters, edict(), &tr);
	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		ClearMultiDamage();
		CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037
		pOther->TraceAttack(pAttacker, pev->dmg, pev->velocity.Normalize(), &tr, pOther->IsPlayer()?DMG_NEVERGIB:(DMG_BULLET|DMG_NEVERGIB));
		ApplyMultiDamage(this, pAttacker);
		float force = 0.5f*pOther->DamageForce(pev->dmg);// XDM3037: it was quite unfair. XDM3038a: Or too much.
		pOther->pev->velocity += vecForward * force;
		iSoundIndex = RANDOM_LONG(0,3);// XDM3038
		iDecalIndex = 0;
		if (tr.iHitgroup == HITGROUP_ARMOR)
			iHitType = 1;
		else
			iHitType = 0;

		SetNextThink(0);
		//Killed(pev, GIB_NEVER);
	}
	else
	{
		const char *tex = NULL;
		if (pOther->IsBSPModel())
			tex = TRACE_TEXTURE(tr.pHit, pev->origin, end);

		if (tex && _stricmp(tex, "SKY") == 0)
		{
			pev->velocity.Clear();
			SetNextThink(0.01);// XDM3038a
			// already	SetTouchNull();
			// already	SetThink(&CBaseEntity::SUB_Remove);
			// XDM3038: unsafe	Destroy();
			return;
		}
		iSoundIndex = RANDOM_LONG(0,1);// XDM3038
		iDecalIndex = DECAL_LARGESHOT1 + RANDOM_LONG(0,4);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, WEAPON_ACTIVITY_VOLUME, 1.0f);
		if (pOther->IsBSPModel() && !pOther->IsMovingBSP())
		{
			// if what we hit is static architecture, can stay around for a while.
			UTIL_SetOrigin(this, pev->origin - vecDir * 12.0f);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity.Clear();
			pev->avelocity.Clear();
			pev->framerate = 0.0f;
			//not now pev->dmg = 0;// XDM3037: so bots won't react on it
			//?pev->angles.z = RANDOM_LONG(0,360);
			SetNextThink(4.0);// XDM3038a
			iHitType = 2;
		}
		else
		{
			Disintegrate();// XDM3035
			SetNextThink(0);
			iHitType = 3;
		}
		SetBits(pev->effects, EF_MUZZLEFLASH);// XDM3034
	}
	pev->velocity.Clear();

	PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usBoltHit, 0.0, pev->origin, pev->angles, pev->dmg, 0/**/, iDecalIndex, iHitType, iSoundIndex, BOLT_EV_HIT);
	//if (pOwner)
	//	pev->owner = pOwner->edict();
	if (iHitType == 2)
		pev->dmg = 0;// XDM3037: so bots won't react on it
}

void CCrossbowBolt::BlowTouch(CBaseEntity *pOther)
{
/*	Vector vecForward, vecOrigin(pev->origin);
	AngleVectors(pev->angles, vecForward, NULL, NULL);// XDM3037: fixes many bugs
	vecOrigin -= vecForward*test1.value;// pull out of the wall*/
	Vector vecOrigin(pev->origin);
	vecOrigin -= pev->velocity.Normalize()*8;// pull out of the wall

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;// !
	//STOP_SOUND(edict(), CHAN_BODY, "weapons/xbow_fly1.wav");
	SetTouchNull();
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0);
	const char *tex = NULL;
	if (pOther->IsBSPModel())// XDM3038b: don't hit the sky
	{
		tex = TRACE_TEXTURE(pOther->edict(), vecOrigin, pev->origin+pev->velocity);
		if (tex && _stricmp(tex, "SKY") == 0)
		{
			pev->velocity.Clear();
			return;
		}
	}
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usBoltHit, 0.0, vecOrigin, pev->angles, gSkillData.DmgBoltExplode, 0/**/, DECAL_MDSCORCH1 + RANDOM_LONG(0,2), (pev->waterlevel == WATERLEVEL_NONE)?0:1/*iHitType*/, 0/*iSoundIndex*/, BOLT_EV_EXPLODE);
	//CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	pev->owner = NULL; // can't traceline attack owner if this is set
	::RadiusDamage(vecOrigin, this, GetDamageAttacker(), gSkillData.DmgBoltExplode, 80, CLASS_NONE, DMG_BLAST);
	//UTIL_DecalPoints(vecOrigin, pev->origin+pev->velocity.Normalize()*8, edict(), DECAL_MDSCORCH1 + RANDOM_LONG(0,2));
	//if (pOwner)
	//	pev->owner = pOwner->edict(); // can't traceline attack owner if this is set
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, vecOrigin, SMALL_EXPLOSION_VOLUME, 1.0f);
}
