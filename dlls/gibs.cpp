#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "pm_materials.h"
#include "func_break.h"
#include "weapons.h"
#include "sound.h"
#include "soundent.h"
#include "gamerules.h"
#include "globals.h"
#include "game.h"

#define GIB_MAX_VELOCITY		1600

LINK_ENTITY_TO_CLASS(gib, CGib);// XDM: world gibs

void CGib::KeyValue(KeyValueData *pkvd)// XDM
{
	if (FStrEq(pkvd->szKeyName, "bloodcolor"))
	{
		m_bloodColor = atoi(pkvd->szValue);// BLOOD_COLOR_RED
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "blooddecals"))
	{
		m_cBloodDecals = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "material"))
	{
		m_material = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "life"))
	{
		m_lifeTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

// XDM: world 'constant' gibs
// Explosions/breakables use this through SpawnGib()
void CGib::Spawn(void)
{
	if (FStringNull(pev->model))
	{
		pev->flags = FL_KILLME;//Destroy();// FL_KILLME must be set here!
		return;
	}
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_SLIDEBOX;
	if (pev->health > 0)
		pev->takedamage = DAMAGE_YES;
	else
		pev->takedamage = DAMAGE_NO;// XDM: don't die right after spawning (explosion), delay a bit

	//SetClassName("gib");//	pev->classname = MAKE_STRING("gib");

	int kvbody = pev->body;
	pev->body = 0;// temp hack to avoid crash
	SET_MODEL(edict(), STRING(pev->model));

	if (FBitSet(pev->spawnflags, SF_GIB_RANDOMBODY) || kvbody < 0)// XDM3038c: fix to support older maps
	{
		int ngroups = GetEntBodyGroupsCount(edict());
		int nsubmodels = 0;
		for (int g = 0; g < ngroups; ++g)
			nsubmodels += GetEntBodyCount(edict(), g);

		pev->body = RANDOM_LONG(0, nsubmodels-1);
	}
	else
		pev->body = kvbody;

	if (FBitSet(pev->spawnflags, SF_GIB_RANDOMSKIN))
		pev->skin = RANDOM_LONG(0, GetEntTextureGroupsCount(edict())-1);

	if (FBitSet(pev->spawnflags, SF_GIB_SOLID))
		SetModelCollisionBox();
	else
		UTIL_SetSize(this, g_vecZero, g_vecZero);

	if (FBitSet(pev->spawnflags, SF_GIB_DROPTOFLOOR))
		DROP_TO_FLOOR(edict());

	SetThinkNull();
	SetTouch(&CGib::BounceGibTouch);
	DontThink();// XDM3038a
}

// Explosions/breakables-specific overrides are here
void CGib::SpawnGib(const char *szGibModel)
{
	pev->model = MAKE_STRING(szGibModel);
	Spawn();
	if (FBitSet(pev->flags, FL_KILLME))// XDM3038a
		return;

	pev->movetype = MOVETYPE_BOUNCE;
	pev->takedamage = DAMAGE_NO;// XDM: don't die right after spawning (explosion), delay a bit
	pev->friction = 0.55; // deading the bounce a bit
	// sometimes an entity inherits the edict from a former piece of glass, and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;// don't override in Spawn()!
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	m_material = matNone;
	m_cBloodDecals = 0;// how many blood decals this gib can place (1 per bounce until none remain). XDM: set in WaitTillLand()

	if (IsMultiplayer())// XDM
		m_lifeTime = GIB_LIFE_DEFAULT;
	else
		m_lifeTime = GIB_LIFE_DEFAULT*5.0f;

	SetThink(&CGib::WaitTillLand);
	//SetTouch(&CGib::BounceGibTouch);
	pev->teleport_time = gpGlobals->time + m_lifeTime;
	SetNextThink(0.4);// XDM
}

void CGib::Precache(void)// XDM
{
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

int CGib::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (FBitSet(bitsDamageType, DMG_ALWAYSGIB|DMGM_BREAK))// XDM3033: a little hack to make gibs immune to teleporter grenade | UNHACKED
		Killed(pInflictor, pAttacker, FBitSet(bitsDamageType, DMGM_GIB_CORPSE)?GIB_ALWAYS:GIB_NORMAL);

	return 1;
}

/*void CGib::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (FBitSet(bitsDamageType, DMGM_PUSH))
		pev->velocity += vecDir*flDamage;
}*/

void CGib::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->takedamage = DAMAGE_NO;// XDM3038b
	TraceResult tr;
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0,0,8), ignore_monsters, edict(), & tr);
	UTIL_BloodDecalTrace(&tr, m_bloodColor);
	if (iGib == GIB_ALWAYS && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
		UTIL_BloodDrips(pev->origin, UTIL_RandomBloodVector(), m_bloodColor, 12);

	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	MaterialSoundRandom(edict(), (Materials)m_material, 1.0);
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: Override
// Output : float - final fall damage value
//-----------------------------------------------------------------------------
float CGib::FallDamage(const float &flFallVelocity)
{
	if (flFallVelocity > GIB_MAX_VELOCITY)
		return pev->health;// destroy

	return 0.0f;
}

// HACKHACK -- The gib velocity equations don't work
void CGib::LimitVelocity(void)
{
	vec_t length = pev->velocity.Length();
	// The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if (length > GIB_MAX_VELOCITY)
		pev->velocity *= GIB_MAX_VELOCITY/length;// This should really be sv_maxvelocity * 0.75 or something
}

//-----------------------------------------------------------------------------
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop
// bouncing to emit their scent.
//-----------------------------------------------------------------------------
void CGib::WaitTillLand(void)
{
	if (!IsInWorld() || pev->teleport_time <= gpGlobals->time)
	{
		Destroy();
		return;
	}

	if (pev->velocity.IsZero())// may never be true
	{
		if (m_lifeTime >= 0.0f)
		{
			pev->solid = SOLID_NOT;
			pev->avelocity.Clear();

			if (IsMultiplayer())
				SetThink(&CBaseEntity::SUB_Remove);
			else
				SUB_StartFadeOut();// XDM3038a

			SetNextThink(m_lifeTime);
		}
		else
			SetThinkNull();

		// If you bleed, you stink!// ok, start stinkin!
		if (m_bloodColor != DONT_BLEED && (g_pGameRules == NULL || g_pGameRules->FAllowMonsters()))// XDM: LAGLAG!
			CSoundEnt::InsertSound(bits_SOUND_MEAT, pev->origin, 384, 1.0);// XDM 25 );
	}
	else
	{
		// wait and check again in another half second.
		if (IsMultiplayer())
			SetNextThink(0.5);
		else
			SetNextThink(0.25);

		if (m_material == matFlesh || m_material == matWood)// XDM3034
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST)
			{
				pev->movetype = MOVETYPE_FLY;
				pev->velocity *= 0.5;
				pev->avelocity *= 0.9;
				pev->gravity = 0.01;
				pev->velocity.z += 8.0;
			}
			else// if (pev->waterlevel == WATERLEVEL_NONE)
			{
				if (pev->movetype == MOVETYPE_FLY)// just exited water
				{
					pev->movetype = MOVETYPE_BOUNCE;// default
					//pev->velocity.z *= 0.5;
					pev->velocity.z = 0;
					pev->gravity = 0.1;
				}
				else
					pev->gravity = 1.0;
			}
		}
	}
	// XDM: some post-initialization
	if (pev->takedamage == DAMAGE_NO)
		pev->takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Gib bounces on the ground or wall, sponges some blood down, too!
//-----------------------------------------------------------------------------
void CGib::BounceGibTouch(CBaseEntity *pOther)
{
	if (pev->flags & FL_ONGROUND)
	{
		pev->velocity *= 0.95;
		pev->angles.x = 0;
		//pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z *= 0.5;
		pev->view_ofs.Set(0,0,8);// XDM3037: if someone's viewing through it
		if (m_lifeTime == 0.0f && pOther->IsPlayer())// XDM3038c: permanent gib
		{
			pev->velocity += pOther->pev->velocity;
			MaterialSoundRandom(edict(), (Materials)m_material, VOL_NORM/2);
		}
	}
	else
	{
		vec_t vlen = pev->velocity.Length();
		if (vlen > 100)
		{
			TraceResult	tr;
			vec_t volume = clamp(vlen * 0.002f, 0.1f, 1.0f);
			MaterialSoundRandom(edict(), (Materials)m_material, volume);
			if (m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED)
			{
				if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM
				{
					SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
					UTIL_TraceLine(pev->origin + Vector(0,0,8), pev->origin + Vector(0,0,-16), ignore_monsters, edict(), &tr);
					UTIL_BloodDecalTrace(&tr, m_bloodColor);
					UTIL_BloodDrips(tr.vecEndPos, pev->velocity.Normalize() + tr.vecPlaneNormal, m_bloodColor, 10);
					ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
				}
				--m_cBloodDecals;
			}
			if (pOther->IsMonster() || pOther->IsPlayer())// XDM: only do damage if we're moving fairly fast
			{
				if (m_material != matNone && m_material != matFlesh && pev->dmgtime < gpGlobals->time)
				{
					SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
					UTIL_GetGlobalTrace(&tr);
					ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
					ClearMultiDamage();
					pOther->TraceAttack(this, vlen * 0.005f, gpGlobals->v_forward, &tr, DMG_CLUB);
					ApplyMultiDamage(this, this);
					pev->dmgtime = gpGlobals->time + 1.0f; // debounce
				}
			}
		}
	}
}

// Sticky gib puts blood on the wall and stays put.
void CGib::StickyGibTouch(CBaseEntity *pOther)
{
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(10.0f);

	if (pOther->IsBSPModel() && !pOther->IsMovingBSP())// XDM
	{
		TraceResult	tr;
		SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 32.0f,  ignore_monsters, edict(), & tr);
		UTIL_BloodDecalTrace(&tr, m_bloodColor);
		ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
		pev->velocity = tr.vecPlaneNormal * -1.0f;
		VectorAngles(pev->velocity, pev->angles);//pev->angles = UTIL_VecToAngles(pev->velocity);
		pev->velocity.Clear();
		pev->avelocity.Clear();
		pev->movetype = MOVETYPE_NONE;
	}
}

void CGib::SpawnStickyGibs(CBaseEntity *pVictim, const Vector &vecOrigin, size_t cGibs)
{
	for (size_t i = 0; i < cGibs; ++i)
	{
		CGib *pGib = GetClassPtr((CGib *)NULL, "gib");
		if (pGib == NULL)
			continue;

		pGib->SpawnGib("models/stickygib.mdl");
		if (FBitSet(pGib->pev->flags, FL_KILLME))// XDM3038a
			continue;

		pGib->pev->body = RANDOM_LONG(0,2);

		if (pVictim)
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT(-3, 3);
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT(-3, 3);
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT(-3, 3);
			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir;// XDM3038c: * -1.0f;
			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity *= 900.0f;
			pGib->pev->avelocity.x = RANDOM_FLOAT(250.0f, 400.0f);
			pGib->pev->avelocity.y = RANDOM_FLOAT(250.0f, 400.0f);
			pGib->pev->avelocity.z = RANDOM_FLOAT(0, 100);
			// copy owner's blood color
			pGib->m_bloodColor = pVictim->BloodColor();

			if (pVictim->pev->health > -50.0f)
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			else if ( pVictim->pev->health > -200)
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			else
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;

			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			pGib->SetTouch(&CGib::StickyGibTouch);
			pGib->SetThinkNull();
		}
		pGib->LimitVelocity();
	}
}

CGib *CGib::SpawnHeadGib(CBaseEntity *pVictim)
{
	// not a good idea probably	Create("gib", pVictim->pev->origin + pVictim->pev->view_ofs, RandomVector(180,180,180), pVictim);
	CGib *pGib = GetClassPtr((CGib *)NULL, "gib");
	if (pGib == NULL)
		return NULL;

	pGib->SpawnGib(g_szDefaultGibsHuman);// throw one head
	if (FBitSet(pGib->pev->flags, FL_KILLME))// XDM3038a
		return NULL;
	// XDM3035c: UNDONE: how to prevent camera from looking through walls when this gib is on ground?
	//	UTIL_SetSize(pGib, 2.0f);
	//:(	pGib->pev->solid = SOLID_BBOX;
	pGib->pev->body = 0;
	pGib->m_material = matFlesh;// XDM

	if (pVictim)
	{
		pGib->pev->origin = pVictim->pev->origin + pVictim->pev->view_ofs;
		pGib->pev->velocity.Set(RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
		pGib->pev->angles = RandomVector(180,180,180);// XDM3038a
		pGib->pev->avelocity = RandomVector()*(pVictim->pev->health * 0.5f);
		// copy owner's blood color
		pGib->m_bloodColor = pVictim->BloodColor();

		if (pVictim->pev->health > -50)
			pGib->pev->velocity *= 0.7f;
		else if (pVictim->pev->health > -200)
			pGib->pev->velocity *= 2.0f;
		else
			pGib->pev->velocity *= 4.0f;
	}
	pGib->LimitVelocity();
	return pGib;
}

// WARNING: this function is only useful for monsters, don't be fooled by its name
void CGib::SpawnRandomGibs(CBaseEntity *pVictim, size_t cGibs, bool human, Vector velocity)
{
	if (NUMBER_OF_ENTITIES() > (gpGlobals->maxEntities - ENTITY_INTOLERANCE))// XDM3035a: in case server uses 'real' gibs, preserve its stability
		return;

	if (!pVictim)
		return;

	if (human)// let's keep this for some anal people
	{
		if (g_pviolence_hgibs && g_pviolence_hgibs->value <= 0)
			return;
	}
	else
	{
		if (g_pviolence_agibs && g_pviolence_agibs->value <= 0)
			return;
	}

	float vk = 80.0f;// XDM3034
	Vector vecCenter = pVictim->Center();// XDM3038

	if (UTIL_LiquidContents(vecCenter))
		vk = 40.0f;

	if (sv_clientgibs.value > 0.0f)// XDM3035
	{
		int modelindex;
		if (human)
			modelindex = g_iModelIndexHGibs;
		else
			modelindex = g_iModelIndexAGibs;

		int gibflags = BREAK_FLESH;
		if (pVictim->m_flBurnTime > gpGlobals->time)
			gibflags |= BREAK_SMOKE;

		velocity *= vk;
		// TODO: create replacement for this
		MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, vecCenter);
			WRITE_BYTE(TE_BREAKMODEL);
			WRITE_COORD(vecCenter.x);
			WRITE_COORD(vecCenter.y);
			WRITE_COORD(vecCenter.z);
			WRITE_COORD(pVictim->pev->size.x);//size.x
			WRITE_COORD(pVictim->pev->size.y);//size.y
			WRITE_COORD(pVictim->pev->size.z);//size.z
			WRITE_COORD(velocity.x);//velocity.x
			WRITE_COORD(velocity.y);//velocity.y
			WRITE_COORD(velocity.z);//velocity.z
			WRITE_BYTE(10);//random velocity 0.1
			WRITE_SHORT(modelindex);
			WRITE_BYTE(cGibs);//count
			if (IsMultiplayer())// XDM
				WRITE_BYTE(GIB_LIFE_DEFAULT*10);//life 0.1
			else
				WRITE_BYTE(GIB_LIFE_DEFAULT*50);//life 0.1

			WRITE_BYTE(gibflags);//flags
		MESSAGE_END();
		//DBG_PRINTF("TE_BREAKMODEL +%u gibs\n", cGibs);
	}
	else
	{
		for (size_t i = 0 ; i < cGibs; ++i)
		{
			CGib *pGib = GetClassPtr((CGib *)NULL, "gib");
			if (pGib == NULL)
				continue;

			if (human)
			{
				pGib->SpawnGib(g_szDefaultGibsHuman);
				if (pGib->pev->body == 0)// XDM3038: fast and ugly. disallow #0
					pGib->pev->body = 1;
				// done in Spawn()	pGib->pev->body = RANDOM_LONG(1, nsubmodels-1);// start at one to avoid throwing random amounts of skulls (0th gib)
			}
			else// aliens
			{
				pGib->SpawnGib(g_szDefaultGibsAlien);
				// done in Spawn()	pGib->pev->body = RANDOM_LONG(0, nsubmodels-1);
			}
			if (FBitSet(pGib->pev->flags, FL_KILLME))// XDM3038a
				continue;

			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pVictim->pev->absmin.x + pVictim->pev->size.x * (RANDOM_FLOAT(0.0f, 1.0f));
			pGib->pev->origin.y = pVictim->pev->absmin.y + pVictim->pev->size.y * (RANDOM_FLOAT(0.0f, 1.0f));
			pGib->pev->origin.z = pVictim->pev->absmin.z + pVictim->pev->size.z * (RANDOM_FLOAT(0.0f, 1.0f)) + 1.0f;// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			// mix in some noise
			pGib->pev->velocity = (g_vecAttackDir*4.0f + RandomVector(1,1,1))*vk;// XDM3038c: reversed attdir
			pGib->pev->avelocity = RandomVector(vk*2.0f);

			// copy owner's blood color
			pGib->m_bloodColor = pVictim->BloodColor();// XDM3037: Why?? (CBaseEntity::Instance(pVictim->pev))->BloodColor();

			if (pVictim->pev->health < 0.0f)
				pGib->pev->velocity *= 2 - pVictim->pev->health*0.01f;

			pGib->m_material = matFlesh;// XDM
			pGib->pev->solid = SOLID_BBOX;
			pGib->LimitVelocity();
		}
	}
}

//-----------------------------------------------------------------------------
// XDM: full customization
// if count is <= 0, this function uses gib model body count
//-----------------------------------------------------------------------------
int CGib::SpawnModelGibs(CBaseEntity *pVictim, const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &velocity,
						int rndVel, int modelIndex, size_t count, float life, int material, int bloodcolor, int flags/* = 0*/)
{
	if (NUMBER_OF_ENTITIES() > (gpGlobals->maxEntities - ENTITY_INTOLERANCE))// XDM3035a: in case server uses 'real' gibs, preserve its stability
		return 0;

	int bodynum = 1;
	size_t i = 0;
	bool getcount = false;
	if (count == 0)// XDM3038c: HACK: nonzero, also for client gibs
	{
		count = 8;// just to enter 'for'
		getcount = true;
	}
	float vk = 80.0f;// XDM3034
	if (UTIL_LiquidContents(origin))
		vk = 40.0f;

	//velocity *= vk;

	if (sv_clientgibs.value > 0.0f)// XDM3035
	{
		// TODO: create replacement for this
		MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, origin);
			WRITE_BYTE(TE_BREAKMODEL);
			WRITE_COORD(origin.x);
			WRITE_COORD(origin.y);
			WRITE_COORD(origin.z);
			WRITE_COORD(maxs.x - mins.x);//size.x
			WRITE_COORD(maxs.y - mins.y);//size.y
			WRITE_COORD(maxs.z - mins.z);//size.z
			WRITE_COORD(velocity.x * vk);//velocity.x
			WRITE_COORD(velocity.y * vk);//velocity.y
			WRITE_COORD(velocity.z * vk);//velocity.z
			WRITE_BYTE(rndVel);//random velocity 0.1
			WRITE_SHORT(modelIndex);
			WRITE_BYTE(count);//count
			WRITE_BYTE((int)(life*10.0f));//life 0.1
			WRITE_BYTE(flags);//flags BREAK_FLESH
		MESSAGE_END();
//DBG_PRINTF("TE_BREAKMODEL +%u gibs\n", count);
		/*MESSAGE_BEGIN(MSG_PVS, gmsgSpawnGibs, vecPos);
			WRITE_COORD(vecPos.x);// coord coord coord (mins)
			WRITE_COORD(vecPos.y);
			WRITE_COORD(vecPos.z);
			WRITE_COORD(vecVel.x);// coord coord coord (maxs)
			WRITE_COORD(vecVel.y);
			WRITE_COORD(vecVel.z);
			WRITE_SHORT(modelindex);// short (model/sprite index)
			WRITE_SHORT(life);// life in 0.1's
			WRITE_BYTE(count);
			WRITE_BYTE(body);
			WRITE_BYTE(material);
			WRITE_BYTE();
			WRITE_BYTE();
			WRITE_BYTE(pev->rendermode);
			WRITE_BYTE(pev->rendercolor.x);// byte,byte,byte (color)
			WRITE_BYTE(pev->rendercolor.y);
			WRITE_BYTE(pev->rendercolor.z);
			WRITE_BYTE(pev->renderamt);
			WRITE_BYTE(pev->renderfx);
			WRITE_BYTE(flags);
		MESSAGE_END();*/
	}
	else
	{
	//count = (pVictim->pev->size.x + pVictim->pev->size.y + pVictim->pev->size.z) * 0.3;
	for (i = 0 ; i < count ; ++i)
	{
		CGib *pGib = GetClassPtr((CGib *)NULL, "gib");// GetClassPtr(NULL) creates a new entity
		if (pGib == NULL)
			continue;

		pGib->pev->solid = SOLID_SLIDEBOX;
		pGib->pev->takedamage = DAMAGE_NO;// XDM
		pGib->pev->movetype = MOVETYPE_BOUNCE;
		pGib->pev->modelindex = modelIndex;
		pGib->pev->friction = 0.55;
		bodynum = GetEntBodyCount(pGib->edict(), 0);// XDM3038: UNDONE: BAD! count through all body groups
		if (getcount)
		{
			count = bodynum;
			getcount = false;
		}
		pGib->SetNextThink(4);
		pGib->m_lifeTime = life;
		pGib->m_cBloodDecals = 0;
		pGib->m_bloodColor = bloodcolor;
		if (pVictim)
		{
			pGib->pev->rendermode = pVictim->pev->rendermode;
			pGib->pev->renderfx = pVictim->pev->renderfx;
			pGib->pev->renderamt = pVictim->pev->renderamt;
		}
		else
		{
			pGib->pev->rendermode = kRenderNormal;
			pGib->pev->renderfx = kRenderFxNone;
			pGib->pev->renderamt = 0;
		}
		if (material < 0)
		{
			if (FBitSet(flags, BREAK_GLASS))
				material = matGlass;
			else if (FBitSet(flags, BREAK_METAL))
				material = matMetal;
			else if (FBitSet(flags, BREAK_FLESH))
			{
				if (IsMultiplayer())
					pGib->m_cBloodDecals = 1;
				else
					pGib->m_cBloodDecals = 4;

				material = matFlesh;
			}
			else if (FBitSet(flags, BREAK_WOOD))
				material = matWood;
			else if (FBitSet(flags, BREAK_CONCRETE))
				material = matCinderBlock;

			//if (FBitSet(flags, BREAK_SMOKE))
			//	PLAYBACK_EVENT_FULL(FEV_GLOBAL, ENT(pGib->pev), g_usTrail, 0.0, pGib->pev->origin, pGib->pev->angles, 0.7, 0.0, entindex(), 1, 0, 0);

			/*if (FBitSet(flags, BREAK_TRANS))
			{
				pGib->pev->rendermode = kRenderTransTexture;
				pGib->pev->renderamt = RANDOM_LONG(100, 200);
			}*/
		}
		pGib->m_material = material;
		pGib->pev->body = RANDOM_LONG(0, bodynum-1);
		pGib->pev->origin = origin + RandomVector(mins, maxs);
		pGib->pev->velocity = velocity + RandomVector(rndVel);
		pGib->pev->avelocity = RandomVector(200);
		pGib->SetThink(&CGib::WaitTillLand);
		pGib->SetTouch(&CGib::BounceGibTouch);

		if (pGib->m_bloodColor != DONT_BLEED)
			pGib->pev->takedamage = DAMAGE_YES;// XDM

		pGib->LimitVelocity();
	}
	}
	return i;
}
