//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//
// Since implementing real teleporter requires special maps and entities,
// we'll just stick with "teleport to nowhere" concept.
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
//#include "monsters.h"
#include "weapons.h"
#include "gamerules.h"
#include "decals.h"
#include "explode.h"
#include "teleporter.h"
#include "game.h"
#include "skill.h"


LINK_ENTITY_TO_CLASS(teleporter, CTeleporter);

int CTeleporter::Restore(CRestore &restore)
{
	int status = CBaseProjectile::Restore(restore);
	/* uncomment if any savedata is added
	if (status == 0)
		return 0;

	status = restore.ReadFields("CTeleporter", this, m_SaveData, ARRAYSIZE(m_SaveData));*/
	if (status != 0)
	{
		m_nFrames = MODEL_FRAMES(pev->modelindex);
		PLAYBACK_EVENT_FULL(FEV_RELIABLE | FEV_GLOBAL, edict(), m_usTeleporter, 0, pev->origin, pev->angles, pev->armortype, pev->teleport_time - gpGlobals->time, pev->skin > 0?BLOOD_COLOR_BLUE:BLOOD_COLOR_GREEN, 0, pev->skin, 0);
	}
	return status;
}

CTeleporter *CTeleporter::CreateTeleporter(const Vector &vecOrigin, const Vector &vecVelocity, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, CBaseEntity *pEnemy, float fDamage, float fLife, int iType)
{
	CTeleporter *pNew = GetClassPtr((CTeleporter *)NULL, "teleporter");
	if (pNew)
	{
		pNew->pev->origin = vecOrigin;
		pNew->pev->skin = iType;
		pNew->pev->dmg = fDamage;// XDM3038c
		pNew->pev->teleport_time = gpGlobals->time + fLife;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		if (iType == 0)
			pNew->pev->model = MAKE_STRING(TELEPORTER_SPRITE_BALL);// XDM3037
		else
			pNew->pev->model = MAKE_STRING(TELEPORTER_SPRITE_FLASHBALL);// XDM3037

		pNew->Spawn();
		// XDM3037	UTIL_SetOrigin(pNew->pev, vecSrc);
		pNew->pev->velocity = vecVelocity;
		pNew->m_hActivator = pEnemy;
	}
	return pNew;
}

void CTeleporter::Spawn(void)
{
	Precache();
	//pev->movetype = MOVETYPE_FLY;
	//pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_SLIDEBOX;// XDM3038a: crash prevention// XDM3037: relies on MOVETYPE_FLYMISSILE. It's purely magic.
	pev->flags |= FL_FLY;
	pev->takedamage = DAMAGE_YES;
	pev->gravity = 0;
	pev->friction = 0;
	pev->mins.Set(-TELEPORTER_RADIUS,-TELEPORTER_RADIUS,-TELEPORTER_RADIUS);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(TELEPORTER_RADIUS,TELEPORTER_RADIUS,TELEPORTER_RADIUS);
	pev->framerate = 1.0;
	pev->scale = 0.6;
	if (pev->skin == 0)
		pev->armortype = TELEPORTER_EFFECT_RADIUS1;
	else
		pev->armortype = TELEPORTER_EFFECT_RADIUS2;

	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgDisplacerBlast;

	CBaseProjectile::Spawn();// XDM3037: initialize a projectile
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->rendercolor.Set(255,255,255);
	pev->health = 1;// so TeleportThink() will not filter me out with IsAlive()
	if (pev->teleport_time <= 0)
		pev->teleport_time = gpGlobals->time + TELEPORTER_LIFE;

	//EMIT_SOUND(edict(), CHAN_BODY, "weapons/teleporter_fly.wav", VOL_NORM, ATTN_NORM);
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
		pev->renderfx = kRenderFxPulseFast;
		PLAYBACK_EVENT_FULL(/*FEV_GLOBAL hack to stop effects/etc.*/0, edict(), m_usTeleporter, 0, pev->origin, pev->angles, pev->armortype, pev->teleport_time - gpGlobals->time, g_iModelIndexLaser, g_iModelIndexColdball2, pev->skin, pev->skin > 0?BLOOD_COLOR_BLUE:BLOOD_COLOR_GREEN);
	}
	m_hTBDAttacker = NULL;// XDM3038: we use this to track something we touched
	SetThink(&CTeleporter::TeleportThink);
	SetTouch(&CTeleporter::TeleportTouch);
	//pev->animtime = gpGlobals->time;
	pev->dmgtime = gpGlobals->time + 0.25f;
	SetNextThink(0.1);
}

// XDM3038c: pev->model should be set beforehand
void CTeleporter::Precache(void)
{
	PRECACHE_MODEL(TELEPORTER_SPRITE_BALL);// XDM: precache both!
	PRECACHE_MODEL(TELEPORTER_SPRITE_FLASHBALL);

	if (FStringNull(pev->model))
		pev->model = MAKE_STRING(TELEPORTER_SPRITE_BALL);// default

	pev->modelindex = MODEL_INDEX(STRING(pev->model));//PRECACHE_MODEL(STRINGV(pev->model));
	m_iFlash = PRECACHE_MODEL(TELEPORTER_SPRITE_FLASHBEAM);

	PRECACHE_SOUND("weapons/teleporter_blast.wav");
	PRECACHE_SOUND("weapons/teleporter_blast2.wav");// XDM3037
	//PRECACHE_SOUND("weapons/teleporter_fly.wav");
	PRECACHE_SOUND("weapons/teleporter_zap.wav");

	m_usTeleporter = PRECACHE_EVENT(1, "events/fx/teleporter.sc");
	m_usTeleporterHit = PRECACHE_EVENT(1, "events/fx/teleporterhit.sc");
}

/*void CTeleporter::CollideThink(void)
{
	if (pev->skin == 0)
	{
		pev->takedamage = DAMAGE_NO;
		::RadiusDamage(pev->origin, pev, VARS(pev->owner), gSkillData.DmgDisplacerBlast, 320, CLASS_NONE, DMG_BLAST);
		MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_IMPLOSION);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_BYTE(255);
			WRITE_BYTE(128);
			WRITE_BYTE(20);
		MESSAGE_END();
	}
	Destroy();
}*/

void CTeleporter::TeleportThink(void)
{
	if (pev->teleport_time <= gpGlobals->time)
	{
		Blast(TELEPORTER_BT_NORMAL);
		return;
	}
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		Blast(TELEPORTER_BT_COL_WATER);// XDM3038c
		return;
	}
	CBaseEntity *pAttacker = GetDamageAttacker();// XDM3035
	//if (pev->owner)
	//	pAttacker = CBaseEntity::Instance(pev->owner);

	if (pev->skin > 0)
	{
		if (pev->dmgtime <= gpGlobals->time)
		{
			CBaseEntity *pEntity = NULL;
			while((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, TELEPORTER_SEARCH_RADIUS)) != NULL)
			{
				if (pEntity->pev->takedamage == DAMAGE_NO)
					continue;

				if (pEntity == this || pEntity->pev == pev)
					continue;
				//if (pEntity->IsProjectile())
				//	continue;

				if (!pEntity->IsAlive())
					continue;

				if (pEntity == m_hOwner)// don't hit my owner
					continue;

				if (pEntity->pev->waterlevel > WATERLEVEL_FEET)// UPD hit even when feet in water
					continue;

				if (pEntity->IsMonster() || pEntity->IsPlayer())
				{
					TraceResult tr;
					UTIL_TraceLine(pev->origin, pEntity->Center(), ignore_monsters, ignore_glass, edict(), &tr);
					if (tr.flFraction == 1.0)// visible
					{
						//Vector mid = (pev->origin + tr.vecEndPos) * 0.5f;// more people should get this message?
						//MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);// XDM3035c
						MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, tr.vecEndPos);
							WRITE_BYTE(TE_BEAMENTPOINT);// TE_BEAMENTS? but that would require attachments
							WRITE_SHORT(entindex());
							WRITE_COORD(tr.vecEndPos.x);
							WRITE_COORD(tr.vecEndPos.y);
							WRITE_COORD(tr.vecEndPos.z);
							WRITE_SHORT(m_iFlash);
							WRITE_BYTE(0);// framestart
							WRITE_BYTE(20);// framerate*10
							WRITE_BYTE(5);// life*10
							WRITE_BYTE(20);// width*10
							WRITE_BYTE(RANDOM_LONG(15,20));// noise*10
							WRITE_BYTE(255);// r
							WRITE_BYTE(255);// g
							WRITE_BYTE(255);// b
							WRITE_BYTE(207);// a
							WRITE_BYTE(30);// speed
						MESSAGE_END();
						UTIL_EmitAmbientSound(edict(), tr.vecEndPos, "weapons/teleporter_zap.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
						if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
						{
							UTIL_Sparks(tr.vecEndPos);
							if (pEntity->IsPlayer())
								UTIL_ScreenShakeOne(pEntity, tr.vecEndPos, 10.0f, 2.0f, 1.0f);
						}
						//EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/teleporter_zap.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
						ClearMultiDamage();
						gMultiDamage.type = DMG_ENERGYBEAM|DMG_SHOCK;
						pEntity->TraceAttack(pAttacker, gSkillData.DmgDisplacerBeam, pev->velocity.Normalize(), &tr, gMultiDamage.type);
						ApplyMultiDamage(this, pAttacker);
					}
				}
				else if (pEntity->IsProjectile())
				{
					if (FStrEq(pEntity->pev->classname, pev->classname))
					{
						if (pEntity->m_hOwner.Get() == m_hOwner.Get())// another teleporter with the same owner
							continue;

						//if (pOther->pev->modelindex == pev->modelindex)// same type
						// idea	Vector vecMegaExplosion = 0.5*(pev->origin + pOther->pev->origin);
					}
					pEntity->pev->effects |= EF_MUZZLEFLASH;
					pEntity->TakeDamage(this, pAttacker, gSkillData.DmgDisplacerBeam, DMG_BLAST|DMG_ENERGYBEAM);
				}
			}
			if (sv_overdrive.value > 0.0f)
				pev->dmgtime = gpGlobals->time + 0.25f;
			else
				pev->dmgtime = gpGlobals->time + 0.5f;
		}
	}
	else
	{
		if (m_hActivator.Get())
		{
			if (pev->dmgtime <= gpGlobals->time)
			{
				if (m_hActivator->IsAlive())
				{
					if (FVisible(m_hActivator))
						MovetoTarget(m_hActivator->Center());
				}
				else
					m_hActivator = NULL;// don't move to respawned players too

				pev->dmgtime = gpGlobals->time + 0.1;
			}
		}
	}
	UpdateFrame();
	SetNextThink(0.05);// frame time
}

void CTeleporter::TeleportTouch(CBaseEntity *pOther)
{
	if (pOther->IsProjectile() && !pOther->IsMonster())// XDM3038a: avoid squeaks
	{
		// can differ	if (pOther->pev->modelindex == pev->modelindex)
		if (pOther != m_hTBDAttacker)
		{
			m_hTBDAttacker = pOther;// XDM3038: prevent recursion
			pOther->Touch(this);// XDM3038: this causes really funny things
		}
		if (!IsRemoving())// Just in case something destroyed me
		{
			if (FStrEq(STRING(pOther->pev->classname), STRING(pev->classname)))
				Blast(TELEPORTER_BT_COL_TELEPORTER);
			else if (FClassnameIs(pOther->pev, "plasmaball"))
				Blast(TELEPORTER_BT_COL_PLASMA);
			else
				Blast(TELEPORTER_BT_COL_PROJECTILE);
		}
		return;
	}

	if (POINT_CONTENTS(pev->origin) == CONTENTS_SKY)
	{
		//STOP_SOUND(edict(), CHAN_BODY, "weapons/teleporter_fly.wav");
		pev->health = 0;
		PLAYBACK_EVENT_FULL(FEV_GLOBAL, edict(), m_usTeleporterHit, 0.0, pev->origin, pev->angles, 0, 0, 0, 0, 0, 1);
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}
	/*else
	{
		CBaseEntity *pEnemy = m_hActivator;
		if (pOther == pEnemy)
			pOther->Killed(m_hOwner, GIB_ALWAYS);
	}*/
	if ((pOther->IsPlayer() || pOther->IsMonster()) && !(pOther->pev->flags & FL_GODMODE))// XDM3037: touch is always deadly
	{
		// Here we could teleport this entity...
		if (pOther->pev->takedamage != DAMAGE_NO)
			pOther->TakeDamage(this, GetDamageAttacker(), 100+pOther->pev->max_health*2.0f, DMG_NEVERGIB | DMG_NOHITPOINT | DMG_DONT_BLEED | DMG_IGNOREARMOR | DMG_DISINTEGRATING);
	}
	Blast(TELEPORTER_BT_COL_ENTITY);
}

/*void CTeleporter::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (pev->deadflag == DEAD_DEAD)
		return;

	if (FBitSet(bitsDamageType, DMG_ENERGYBEAM))
		Blast(TELEPORTER_BT_BEAM);
}*/

int CTeleporter::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->deadflag == DEAD_DEAD)
		return 0;

	if (pInflictor && pInflictor->IsProjectile())// XDM3037: attack by projectile detection
	{
		// can differ	if (pOther->pev->modelindex == pev->modelindex)
		if (FStrEq(STRING(pInflictor->pev->classname), STRING(pev->classname)))
		{
			Blast(TELEPORTER_BT_COL_TELEPORTER);
			return 1;
		}
		/* these are detected only by collision		else if (FClassnameIs(pInflictor->pev, "plasmaball"))
		{
			Blast(TELEPORTER_BT_COL_PLASMA);
		}
		else
		{
			Blast(TELEPORTER_BT_COL_PROJECTILE);
		}
		return 1;*/
	}

	if (FBitSet(bitsDamageType, DMG_ENERGYBEAM))
		Blast(TELEPORTER_BT_BEAM);

	return 1;// override the damage
}

int CTeleporter::ShouldCollide(CBaseEntity *pOther)// XDM3035
{
	//if (pOther->pev->modelindex == pev->modelindex)// BAD
	//	return 0;

	if (pOther->IsPlayer() && !pOther->IsAlive())// some disintegrating players block entities
		return 0;

	return CBaseProjectile::ShouldCollide(pOther);
}

// velocity += |(vecTarget - origin)| * speed;
void CTeleporter::MovetoTarget(const Vector &vecTarget)
{
	vec_t flSpeed = pev->velocity.Length();
	if (flSpeed > TELEPORTER_FLY_SPEED1)// accelerate, limit speed
		pev->velocity.SetLength(TELEPORTER_FLY_SPEED1);// use TELEPORTER_FLY_SPEED1 because type 2 teleporter never tracks a target

	Vector vecToTarget(vecTarget); vecToTarget -= pev->origin;
	vecToTarget.SetLength(TELEPORTER_FLY_SPEED1);
	pev->velocity += vecToTarget;//(vecTarget - pev->origin).Normalize() * TELEPORTER_FLY_SPEED1;
}

void CTeleporter::Blast(short type)
{
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->deadflag = DEAD_DEAD;
	//pev->dmg = gSkillData.DmgDisplacerBlast;
	//STOP_SOUND(edict(), CHAN_BODY, "weapons/teleporter_fly.wav");

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity.Normalize() * 2.0f, ignore_monsters, edict(), &tr);
	//UTIL_DecalTrace(&tr, DECAL_MDSCORCH1 + RANDOM_LONG(0,2));
	Vector vecSpot = pev->origin + tr.vecPlaneNormal * (TELEPORTER_BLAST_RADIUS * 0.1f);// XDM3037: explosion origin
	int dmgbits = 0;// XDM3037: more variants of energy blast
	float radius = TELEPORTER_BLAST_RADIUS;
	if (pev->skin == 0)
		dmgbits = DMG_BLAST | DMG_DISINTEGRATING;
	else
		dmgbits = DMG_BLAST | DMG_SHOCK;

	if (type == TELEPORTER_BT_COL_PROJECTILE)
	{
		UTIL_ScreenShake(pev->origin, pev->dmg*0.5f, 1.25f, 2.5f, radius*1.5f);
	}
	else if (type == TELEPORTER_BT_COL_TELEPORTER || type == TELEPORTER_BT_COL_PLASMA)
	{
		dmgbits |= DMG_ENERGYBEAM;
		radius *= 1.25f;
		UTIL_ScreenShake(pev->origin, pev->dmg*0.5f, 1.0f, 3.0f, radius*2.0f);
	}

	// TODO: PushPolicy = POLICY_DENY
	::RadiusDamage(vecSpot, this, GetDamageAttacker(), pev->dmg, radius, CLASS_NONE, dmgbits);

	PLAYBACK_EVENT_FULL(FEV_GLOBAL, edict(), m_usTeleporterHit, 0, pev->origin, pev->angles, pev->dmg, radius, pev->modelindex, type, pev->skin, 0);
	pev->dmg = 0.0f;
	SetTouchNull();
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0);
}
