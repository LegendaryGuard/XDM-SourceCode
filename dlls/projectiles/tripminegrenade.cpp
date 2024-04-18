#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "effects.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "crossbowbolt.h"
#include "sound.h"


//-----------------------------------------------------------------------------
// Purpose: Removes all mines owned by the specified player
// Input  : *pOwner - if NULL, ALL mines are removed
//-----------------------------------------------------------------------------
void DeactivateMines(CBasePlayer *pOwner)
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname(pEntity, TRIPMINE_CLASSNAME)) != NULL)
	{
		CTripmineGrenade *pMine = (CTripmineGrenade *)pEntity;
		if (pMine)
		{
			if (pOwner == NULL || pMine->m_hOwner == pOwner)// if specified pOwner == NULL, deactivate ALL mines
				pMine->Deactivate(true);// UNDONE: delay this a bit so the charge may detonate
		}
	}
}


LINK_ENTITY_TO_CLASS(monster_tripmine, CTripmineGrenade);

TYPEDESCRIPTION	CTripmineGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CTripmineGrenade, m_flPowerUp, FIELD_TIME ),
	DEFINE_FIELD( CTripmineGrenade, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( CTripmineGrenade, m_hHost, FIELD_EHANDLE ),
	DEFINE_FIELD( CTripmineGrenade, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTripmineGrenade, m_posHost, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_angleHost, FIELD_VECTOR )
};

IMPLEMENT_SAVERESTORE(CTripmineGrenade,CGrenade);


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CTripmineGrenade::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->sequence = 2;// XDM: world

	//UTIL_SetSize(this, Vector(-8, -8, -4), Vector(8, 8, 4));
	//UTIL_SetOrigin(this, pev->origin);
	//UTIL_ShowBox(pev->origin, pev->mins, pev->maxs, 10);

	pev->mins.Set(-8,-8,-4);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(8,8,4);
	if (pev->dmg == 0)// XDM3038c
		pev->dmg = gSkillData.DmgTripmine;

	CBaseProjectile::Spawn();// XDM3037

	if (FBitSet(pev->spawnflags, SF_TRIPMINE_QUICKPOWERUP))// power up quickly
	{
		ClearBits(pev->spawnflags, SF_TRIPMINE_QUICKPOWERUP);// XDM3035: CLEAR THIS! It interferes with SF_NODAMAGE!
		pev->button = 1;// hack?
		m_flPowerUp = gpGlobals->time + TRIPMINE_POWERUP_TIME2;// 1.0;
	}
	else// power up in 2.5 seconds
	{
		pev->button = 0;
		m_flPowerUp = gpGlobals->time + TRIPMINE_POWERUP_TIME1;// 2.5
	}

	SetThink(&CTripmineGrenade::PowerupThink);
	SetNextThink(0.2);// XDM3038a

	pev->takedamage = DAMAGE_YES;
	pev->health = 1;// don't let die normally

	if (m_hOwner)
	{
		// play deploy sound
		EMIT_SOUND(edict(), CHAN_VOICE, "weapons/mine_deploy.wav", VOL_NORM, ATTN_NORM);
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM, 0, pev->button?110:PITCH_NORM); // chargeup
		// XDM3037	m_pRealOwner = pev->owner;// see CTripmineGrenade for why.
	}
#if defined (SV_NO_PITCH_CORRECTION)
	Vector vAng(pev->angles);
	vAng.x = - vAng.x;
	AngleVectors(vAng, m_vecDir, NULL, NULL);
#else
	AngleVectors(pev->angles, m_vecDir, NULL, NULL);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CTripmineGrenade::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/p_tripmine.mdl");// XDM3037

	CBaseProjectile::Precache();// XDM3038c: jump over CGrenade
	//pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	PRECACHE_MODEL("sprites/tripbeam.spr");// XDM
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/mine_activate.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
	PRECACHE_SOUND("weapons/mine_beep.wav");
	PRECACHE_SOUND("weapons/mine_disarm.wav");
	pev->noise = ALLOC_STRING("weapons/mine_explode.wav");
	PRECACHE_SOUND(STRINGV(pev->noise));
}

//-----------------------------------------------------------------------------
// Purpose: No thinking here, just the beam code
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTripmineGrenade::MakeBeam(void)
{
	//conprintf(1, "CTripmineGrenade::MakeBeam()\n");
	Vector vecEnd(m_vecDir);// vecEnd = pev->origin + m_vecDir * g_psv_zmax->value;
	vecEnd *= g_psv_zmax->value;
	vecEnd += pev->origin;
	TraceResult tr;
	UTIL_TraceLine(pev->origin, vecEnd, dont_ignore_monsters, ignore_glass, edict(), &tr);

	if (tr.flFraction == 1.0f)// beam did not hit anything, consider the laser power was not enough
		return false;

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity->IsBSPModel())//pev->solid == SOLID_BSP)// XDM3038: detect glass (can be transparent metal, bushes, etc.)
	{
		char cTextureType = TEXTURETYPE_Trace(pEntity, pev->origin, vecEnd);// vecEnd is already set to tr.vecEndPos
		if (cTextureType == CHAR_TEX_SKY)// || CHAR_TEX_NULL?
		{
			if (m_hOwner && m_hOwner->IsPlayer())
				ClientPrint(&m_hOwner.Get()->v, HUD_PRINTCENTER, "#TRIPMINE_SKY");

			return false;
		}
	}

	//m_flBeamLength = tr.flFraction;
	m_flBeamLength = (tr.vecEndPos - pev->origin).Length();
	m_pBeam = CBeam::BeamCreate("sprites/tripbeam.spr", TRIPMINE_BEAM_WIDTH);
	if (m_pBeam == NULL)
		return false;

	//m_pBeam->EntPointInit(entindex(), tr.vecEndPos);// XDM3035: 
	m_pBeam->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam->SetScrollRate(255);
	if (gSkillData.iSkillLevel == SKILL_HARD)
		m_pBeam->SetBrightness(31);
	else
		m_pBeam->SetBrightness(47);// XDM3035: tweaked

	if (!StringToVec(sv_trip_rgb.string, m_pBeam->pev->rendercolor))
		m_pBeam->pev->rendercolor.Set(0,191,191);

	//SetNextThink(0.1);// XDM3038a
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Safely remove the beam
//-----------------------------------------------------------------------------
void CTripmineGrenade::KillBeam(void)
{
	//conprintf(1, "CTripmineGrenade::KillBeam()\n");
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check existence/consistency of an object mine is attached to
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTripmineGrenade::CheckAttachSurface(void)
{
	TraceResult tr;
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	UTIL_TraceLine(pev->origin + m_vecDir*4, pev->origin - m_vecDir*32, dont_ignore_monsters, dont_ignore_glass, edict(), &tr);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	if (tr.flFraction >= 1.0)// found nothing
		return false;
	else if (m_hHost.Get() != tr.pHit)// found something different
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: THINK
//-----------------------------------------------------------------------------
void CTripmineGrenade::PowerupThink(void)
{
	TraceResult tr;
	if (m_hHost.Get() == NULL)// find out what am I attached to
	{
		SetIgnoreEnt(NULL);// XDM3037
		UTIL_TraceLine(pev->origin + m_vecDir*4, pev->origin - m_vecDir*32, dont_ignore_monsters, edict(), &tr);
		if (tr.fStartSolid)// && !FNullEnt(tr.pHit))// || (m_hHost.Get() && tr.pHit == m_hHost.Get()))
		{
			//m_hHost.Set(tr.pHit);
			m_flPowerUp += 0.1;
			SetNextThink(0.1);// XDM3038a
			return;
		}
		if (tr.flFraction < 1.0)// found something
		{
			m_hHost.Set(tr.pHit);
			m_posHost = m_hHost->pev->origin;
			m_angleHost = m_hHost->pev->angles;
		}
		else// we're in the air?
		{
			Deactivate(m_hHost.Get() != NULL);
			conprintf(1, "Tripmine at %g %g %g removed\n", pev->origin.x, pev->origin.y, pev->origin.z);
			return;
		}
	}
	else if (m_posHost != m_hHost->pev->origin || m_angleHost != m_hHost->pev->angles)// host position changed since last time. can't attach.
	{
		Deactivate(false);
		CBaseEntity *pNewMine = CBaseEntity::Create("weapon_tripmine", pev->origin + m_vecDir * 24.0f, pev->angles, g_vecZero, NULL, SF_NORESPAWN);
		if (pNewMine)
			EMIT_SOUND(pNewMine->edict(), CHAN_BODY, "weapons/mine_beep.wav", VOL_NORM, ATTN_NORM);// can't play sound from removed mine

		return;
	}
	//conprintf(1, "%d %.0f %.0f %0.f\n", pev->owner, m_pOwner->pev->origin.x, m_pOwner->pev->origin.y, m_pOwner->pev->origin.z );

	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		SetIgnoreEnt(NULL);// XDM3037: start colliding with everything
		pev->movetype = MOVETYPE_NONE;// XDM3038a: why not? also, for headcrab detection
		pev->solid = SOLID_BBOX;
		//pev->takedamage = DAMAGE_AIM;
		UTIL_SetSize(this, Vector(-8, -8, -4), Vector(8, 8, 4));
		UTIL_SetOrigin(this, pev->origin);
		//UTIL_ShowBox(pev->origin, pev->mins, pev->maxs, 10, 0,255,0);
		if (pev->impulse == TRIPMINE_PM_BEAM)
		{
			if (MakeBeam())
			{
				SetThink(&CTripmineGrenade::BeamBreakThink);
			}
			else// XDM3038a
			{
				Deactivate(false);
				CBaseEntity *pNewMine = CBaseEntity::Create("weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles, g_vecZero, NULL, SF_NORESPAWN);
				if (pNewMine)
					EMIT_SOUND(pNewMine->edict(), CHAN_BODY, "weapons/mine_beep.wav", VOL_NORM, ATTN_NORM);// can't play sound from removed mine

				return;
			}
		}
		else
		{
			SetThink(&CTripmineGrenade::RadiusThink);
		}
		// play enabled sound
		//if (pev->button > 0)
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, "weapons/mine_activate.wav", VOL_NORM/2, ATTN_NORM, 0, PITCH_NORM);// XDM3035a: something wrong was here
	}
	SetNextThink(0.1);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: THINK: check tracking beam
//-----------------------------------------------------------------------------
void CTripmineGrenade::BeamBreakThink(void)
{
	//conprintf(1, "CTripmineGrenade::BeamBreakThink()\n");
	bool bBlowup = 0;
	if (m_hHost.Get() == NULL)// we rely on EHANDLE Get() check. Hopefully, it is restored properly.
		bBlowup = 1;
	else if (m_posHost != m_hHost->pev->origin)
		bBlowup = 1;
	else if (m_angleHost != m_hHost->pev->angles)
		bBlowup = 1;
	else// if (bBlowup == 0)
	{
		TraceResult tr;
		// HACKHACK Set simple box using this really nice global!
		SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
		Vector vecEnd(m_vecDir);// vecEnd = pev->origin + m_vecDir * g_psv_zmax->value;
		vecEnd *= g_psv_zmax->value;
		vecEnd += pev->origin;
		UTIL_TraceLine(pev->origin, vecEnd, dont_ignore_monsters, ignore_glass, edict(), &tr);
		ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);

		vec_t flCurrentLength = (tr.vecEndPos - pev->origin).Length();
		//old	if (fabs(m_flBeamLength - tr.flFraction) > 0.001)
		if (fabs(m_flBeamLength - flCurrentLength) > 1)// XDM3037: 1 unit tolerance
			bBlowup = 1;
		else// nobody crossed the beam, check the thing we're attached to
			bBlowup = !CheckAttachSurface();
	}

	if (bBlowup)
	{
		//SetIgnoreEnt(NULL);
		Killed(this, GetDamageAttacker(), GIB_NORMAL);
		return;
	}
	else
		SetNextThink(0.1);// XDM3038a

	if (m_pBeam == NULL)// respawn detect.
	{
		MakeBeam();
		//if (tr.pHit)
		//	m_hOwner = pOther;	// reset owner too
	}

	if (sv_overdrive.value > 0.0f)
	{
		float r = 30.0f;
		float s,c;
		SinCos(gpGlobals->time*10.0f, &s, &c);
		Vector rt,up;
		AngleVectors(pev->angles, NULL, rt, up);
		Vector vecBeamEnd(m_pBeam->GetStartPos());
		vecBeamEnd += rt*s*r + up*c*r;
		m_pBeam->SetNoise(RANDOM_LONG(0,10));
		m_pBeam->SetBrightness(RANDOM_LONG(31,255));
		//m_pBeam->SetEndPos(vecBeamEnd);
		m_pBeam->SetStartPos(vecBeamEnd);
	}
}

//-----------------------------------------------------------------------------
// Purpose: THINK: check radius
//-----------------------------------------------------------------------------
void CTripmineGrenade::RadiusThink(void)
{
	if (!CheckAttachSurface())
	{
		//SetIgnoreEnt(NULL);
		Killed(this, GetDamageAttacker(), GIB_NORMAL);
		return;
	}
	CBaseEntity *pEnt = NULL;
	while ((pEnt = UTIL_FindEntityInSphere(pEnt, pev->origin, TRIPMINE_SEARCH_RADIUS)) != NULL)
	{
		if (pEnt->IsPlayer() || pEnt->IsMonster())
		{
			//conprintf(1, "mine(%d): found %s %d %s (%d)\n", pev->team, STRING(pEnt->pev->classname), pEnt->entindex(), STRING(pEnt->pev->netname), pEnt->pev->team);
			if (pEnt->IsAlive())
			{
				if (g_pGameRules && g_pGameRules->IsTeamplay() && (mp_teammines.value > 0))
					if (pev->team > TEAM_NONE && pEnt->pev->team == pev->team)
						continue;

				TraceResult tr;// trace from Center() ?
				UTIL_TraceLine(pev->origin, pEnt->Center(), ignore_monsters, dont_ignore_glass, edict(), &tr);// ignore_monsters saves the day
				//UTIL_DebugBeam(pev->origin, tr.vecEndPos, 2.0);
				if (tr.flFraction >= 1.0 || tr.pHit == pEnt->edict())// visible directly or hit
				{
					//STOP_SOUND(edict(), CHAN_BODY, "weapons/mine_charge.wav");
					KillBeam();// XDM
					SetThink(&CTripmineGrenade::DelayDeathThink);
					SetNextThink(clamp(sv_trip_delay.value, 0.01, 2.0));// XDM3038a
					return;
				}
				//else
				//	conprintf(1, "mine: not visible (%f)!!\n\n", tr.flFraction);
			}
		}
	}
	SetBits(pev->effects, EF_MUZZLEFLASH);
	SetNextThink(0.5);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: THINK: ???
//-----------------------------------------------------------------------------
void CTripmineGrenade::DelayDeathThink(void)
{
	//conprintf(1, "CTripmineGrenade::DelayDeathThink()\n");
	//KillBeam();
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->deadflag = DEAD_DEAD;// XDM3038c
	SetIgnoreEnt(NULL);// XDM3037
	UTIL_SetSize(this, 0);
	TraceResult tr;
	SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3035c: decorative, simplify
	UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64,  dont_ignore_monsters, edict(), &tr);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	//pev->owner = m_hOwner->Get();
	//pev->owner = m_pRealOwner;
	//BOOL explode = pev->impulse;// save, because Explode() erases data
	int explodetype = pev->skin;
	CBaseEntity *pAttacher = GetDamageAttacker();

	if (sv_overdrive.value > 0.0f)// TESTME EXPERIMENTAL
	{
		if (explodetype != TRIPMINE_EX_NORMAL && explodetype != TRIPMINE_EX_EXTERNAL)
		{
			short n = (explodetype == TRIPMINE_EX_AGRENADES)?16:10;// 16 ags and 10 other
			Vector org, dir, ang, rt,up;
			for (short i=0; i<n; ++i)
			{
				AngleVectors(pev->angles, NULL, rt, up);
				dir = tr.vecPlaneNormal + rt*RANDOM_FLOAT(-6,6) + up*RANDOM_FLOAT(-6,6);//UTIL_RandomBloodVector();
				VectorAngles(dir, ang);
				org = pev->origin + dir*4.5f;
				if (explodetype == TRIPMINE_EX_BOLTS)
					CCrossbowBolt::BoltCreate(org, ang, dir, pAttacher, this, 0.0f, true);
				else if (explodetype == TRIPMINE_EX_BOLTS_EX)
					CCrossbowBolt::BoltCreate(org, ang, dir, pAttacher, this, 0.0f, false);
				else if (explodetype == TRIPMINE_EX_LGRENADES)
					CLGrenade::CreateGrenade(org, ang, dir*RANDOM_FLOAT(300,400), pAttacher, this, 0.0f, RANDOM_FLOAT(0.5f, 1.5f), false);// grenades that detonate first will scatter remaining ones
				else if (explodetype == TRIPMINE_EX_LGRENADES_EX)
					CLGrenade::CreateGrenade(org, ang, dir*RANDOM_FLOAT(400,600), pAttacher, this, 0.0f, 0.0f, true);
				else if (explodetype == TRIPMINE_EX_AGRENADES)
					CAGrenade::ShootTimed(org, ang, dir*RANDOM_FLOAT(400,500), pAttacher, this, RANDOM_FLOAT(2.0f, 5.0f), 0.0f, 0);
			}
		}
	}
	if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * 4.0f);

	if (explodetype == TRIPMINE_EX_NORMAL)
		Explode(pev->origin, DMG_BLAST|DMG_BURN, g_iModelIndexBigExplo2, 0, g_iModelIndexWExplosion, 0, 3.0f, STRING(pev->noise));// XDM
	else
		Destroy();
}

int CTripmineGrenade::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (gpGlobals->time < m_flPowerUp && flDamage < pev->health)
	{
		pev->health = 0.0f;
		KillBeam();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.1);// XDM3038a
		return 0;
	}
	else
	{
		if (FBitSet(bitsDamageType, DMGM_FIRE))// XDM3034: detonate on fire
			SetBits(bitsDamageType, DMG_BLAST);// make sure mine detonates

		Killed(pInflictor, pAttacker, GIB_NORMAL);// XDM3037
	}
	return 1;
	//return CGrenade::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);
}

void CTripmineGrenade::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	//conprintf(1, "CTripmineGrenade::Killed(%s %s)\n", pInflictor?STRING(pInflictor->pev->classname):"", STRING(pAttacker->pev->classname));
	pev->solid = SOLID_NOT;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	KillBeam();// XDM: call this first! Teleportation grenade will setthink to fade out!!
	SetIgnoreEnt(NULL);
	STOP_SOUND(edict(), CHAN_BODY, "weapons/mine_charge.wav");
	//EMIT_SOUND(edict(), CHAN_BODY, "common/null.wav", 0.5, ATTN_NORM); // shut off chargeup

	if (pAttacker && pAttacker->IsPlayer())
		m_hOwner = pAttacker;
		//pev->owner = pAttacker->edict();// some client has destroyed this mine, he'll get credit for any kills

	SetThink(&CTripmineGrenade::DelayDeathThink);
	SetNextThink(clamp(sv_trip_delay.value, 0.01, 2.0));// XDM3038a
	if (sv_trip_delay.value > 0.5)
		EMIT_SOUND(edict(), CHAN_BODY, "weapons/mine_beep.wav", VOL_NORM, ATTN_NORM);
}

void CTripmineGrenade::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator->IsPlayer())
		return;

	if (pev->deadflag == DEAD_DYING)// XDM3035
		return;

	m_hActivator = pActivator;
	if (pActivator->edict() == m_hOwner.Get())
	{
		Deactivate(false);// XDM3035
		EMIT_SOUND(edict(), CHAN_BODY, "weapons/mine_disarm.wav", VOL_NORM, ATTN_NORM);
		CBaseEntity::Create("weapon_tripmine", pev->origin + m_vecDir*3.0f, pev->angles, g_vecZero, NULL, SF_NORESPAWN);// XDM3038a
	}
	else
	{
		if (sv_overdrive.value > 0.0f)// XDM3035b
		{
			Killed(this, GetDamageAttacker(), GIB_NORMAL);
			//SetThink(&CGrenade::Detonate);
			//SetNextThink(0);
		}
		else
			EMIT_SOUND(edict(), CHAN_BODY, "weapons/mine_beep.wav", VOL_NORM, ATTN_NORM);
	}
}

void CTripmineGrenade::Deactivate(bool disintegrate)// XDM3035
{
	//conprintf(1, "CTripmineGrenade::Deactivate(%s)\n", disintegrate);
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	pev->renderamt = 0;
	pev->renderfx = kRenderFxNone;
	pev->health = 0.0;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;// XDM3035
	pev->owner = NULL;

	STOP_SOUND(edict(), CHAN_VOICE, "weapons/mine_deploy.wav");
	if (m_flPowerUp > gpGlobals->time)
	{
		// it's not that long	STOP_SOUND( edict(), CHAN_VOICE, "weapons/mine_deploy.wav" );
		STOP_SOUND(edict(), CHAN_BODY, "weapons/mine_charge.wav");
	}
	KillBeam();
	SetThinkNull();

	if (disintegrate)
		Disintegrate();
	else
		Destroy();
}
