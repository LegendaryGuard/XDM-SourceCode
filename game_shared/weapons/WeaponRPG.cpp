#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"

#if !defined (CLIENT_DLL)

LINK_ENTITY_TO_CLASS(laser_spot, CLaserSpot);

CLaserSpot *CLaserSpot::CreateSpot(void)
{
	CLaserSpot *pSpot = GetClassPtr((CLaserSpot *)NULL, "laser_spot");
	if (pSpot)
		pSpot->Spawn();

	return pSpot;
}

void CLaserSpot::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->rendercolor.Set(255,0,0);
	pev->scale = 0.1;
	pev->takedamage = DAMAGE_NO;
	SetBits(pev->flags, FL_NOTARGET);
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetOrigin(this, pev->origin);
}

void CLaserSpot::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("sprites/laserdot.spr");// XDM3037a

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

void CLaserSpot::Suspend(const float &flSuspendTime)
{
	SetBits(pev->effects, EF_NODRAW);
	SetThink(&CLaserSpot::Revive);
	SetNextThink(flSuspendTime);// XDM3038a
}

void CLaserSpot::Revive(void)
{
	ClearBits(pev->effects, EF_NODRAW);
	SetThinkNull();
}

#endif // CLIENT_DLL






enum vanim_rpg_e
{
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,
	RPG_FIRE2,
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL,	// unloaded fidget
};

LINK_ENTITY_TO_CLASS(weapon_rpg, CWeaponRPG);

TYPEDESCRIPTION	CWeaponRPG::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponRPG, m_fSpotActive, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponRPG, m_cActiveRockets, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponRPG, CBasePlayerWeapon);

void CWeaponRPG::Spawn(void)
{
	CBasePlayerWeapon::Spawn();// XDM3038a
	m_fSpotActive = 1;
}

void CWeaponRPG::Precache(void)
{
	m_iId = WEAPON_RPG;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = RPG_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_rpg.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_rpg.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgRPG;

	PRECACHE_SOUND("weapons/rpg_aim_off.wav");
	PRECACHE_SOUND("weapons/rpg_aim_on.wav");
	PRECACHE_SOUND("weapons/rpg_fire.wav");
	//PRECACHE_SOUND("weapons/rpg_lock.wav");
	PRECACHE_SOUND("weapons/rpg_reload.wav");
	PRECACHE_SOUND("weapons/rpg_select.wav");
	//PRECACHE_SOUND("weapons/rpg_track.wav");

	//UTIL_PrecacheOther("laser_spot");
	//UTIL_PrecacheOther("rpg_rocket");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/rpg.sc");

	m_hLaserSpot = NULL;
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponRPG::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();
	p->iFlags = ITEM_FLAG_SUPERWEAPON;// XDM3035
	p->iMaxClip = RPG_MAX_CLIP;
	p->iWeight = RPG_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_RPG;
	p->iPosition = POSITION_RPG;
#endif
	p->pszAmmo1 = "rockets";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponRPG::Deploy(void)
{
	m_cActiveRockets = 0;// XDM3035: some dropped RPGs may have this non-zero!
	//if (HasAmmo(AMMO_PRIMARY|AMMO_CLIP)) EMIT_SOUND(edict(), CHAN_WEAPON, , VOL_NORM, ATTN_NORM);
	m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;// XDM: !!!
	return DefaultDeploy((m_iClip == 0?RPG_DRAW_UL:RPG_DRAW1), "rpg", "weapons/rpg_select.wav");
}

bool CWeaponRPG::CanHolster(void) const
{
	if (m_fSpotActive && m_cActiveRockets)
		return false;// can't put away while guiding a missile.

	return true;
}

void CWeaponRPG::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.
	m_cActiveRockets = 0;// XDM: we won't be able to control any rocket
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;

	if (m_iClip > 0)
		SendWeaponAnim(RPG_HOLSTER1);
	else
		SendWeaponAnim(RPG_HOLSTER2);

#if !defined (CLIENT_DLL)
	if (m_hLaserSpot.Get())
	{
		m_hLaserSpot->Killed(NULL, NULL, GIB_NEVER);
		m_hLaserSpot = NULL;
	}
#endif
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponRPG::PrimaryAttack(void)
{
	if (m_fInReload)
		return;

	if (m_iClip > 0)
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#if !defined (CLIENT_DLL)
		Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward*GetBarrelLength() + gpGlobals->v_right*8.0f + gpGlobals->v_up*-4.0f;
		CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket(vecSrc, vecAng, gpGlobals->v_forward, GetDamageAttacker(), GetDamageAttacker(), this, pev->dmg);
		if (pRocket)
		{
			++m_cActiveRockets;
			if (m_fSpotActive && m_hLaserSpot.Get())// XDM: don't store target in local pev->enemy!
				pRocket->pev->enemy = m_hLaserSpot->edict();
			else if (m_pPlayer->m_hAutoaimTarget != NULL && m_pPlayer->FVisible(m_pPlayer->m_hAutoaimTarget))
			{
				pRocket->pev->enemy = m_pPlayer->m_hAutoaimTarget->edict();
				m_pPlayer->ResetAutoaim();
			}
			else
				pRocket->pev->enemy = NULL;
		}
#endif
		int flags;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, RPG_FIRE2, 0, 0, 0);
		m_iClip--;
		m_flNextPrimaryAttack = m_flTimeWeaponIdle = GetNextAttackDelay(RPG_ATTACK_INTERVAL1);// fast reload
	}
	else
	{
#ifdef CLIENT_DLL
		if (!m_fSpotActive)
#else
		if (!(m_fSpotActive && m_hLaserSpot.Get()))// XDM3035: when laser is not active
#endif
		{
			if (!HasAmmo(AMMO_PRIMARY))// trrrrrrrrr prevention
			{
				PlayEmptySound();
				m_flNextPrimaryAttack = m_flTimeWeaponIdle = GetNextAttackDelay(RPG_ATTACK_INTERVAL1);
			}
			else
			{
				m_flNextPrimaryAttack = -1.0f;// ignore pressed fire buttons
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RPG_RELOAD_TIME;
				Reload();// force reload
			}
		}
	}
	UpdateSpot();
}

void CWeaponRPG::SecondaryAttack(void)
{
	m_fSpotActive = !m_fSpotActive;
/*#if !defined (CLIENT_DLL)
	if (!m_fSpotActive && m_pSpot)
	{
		EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/rpg_aim_off.wav", VOL_NORM, ATTN_NORM);
		m_pSpot->Killed(NULL, NULL, 0);
		m_pSpot = NULL;
		pev->skin = 0;
	}
#endif*/
	UpdateSpot();
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RPG_ATTACK_INTERVAL2;
}

void CWeaponRPG::WeaponIdle(void)
{
	if (m_fInReload)
		return;

	UpdateSpot();

	//if (!m_fSpotActive)// XDM
	//	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	/*if (!m_fSpotActive && pev->teleport_time < UTIL_WeaponTimeBase())
	{
		if (m_pPlayer->m_hAutoaimTarget != NULL && m_pPlayer->FVisible(m_pPlayer->m_hAutoaimTarget))
		{
			if (pev->enemy == m_pPlayer->m_hAutoaimTarget->edict() && pev->button > 3)
				EMIT_SOUND(edict(), CHAN_BODY, "weapons/rpg_lock.wav", VOL_NORM, ATTN_NORM);
			else
			{
				EMIT_SOUND(edict(), CHAN_BODY, "weapons/rpg_track.wav", VOL_NORM, ATTN_NORM);
				pev->button++;
			}
		}
		else
			pev->button = 0;

		pev->teleport_time = UTIL_WeaponTimeBase() + 0.5;
	}*/

	/*if (m_cActiveRockets > 1)
	{
		conprintf(1, "ERROR: RPG has more than one active rocket!\n");
		m_cActiveRockets = 1;
	}*/

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (HasAmmo(AMMO_PRIMARY))//if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 1;
		else
			rmax = 0;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
		if (r == 1)
		{
			if (m_iClip == 0)
				SendWeaponAnim(RPG_FIDGET_UL);
			else
				SendWeaponAnim(RPG_FIDGET);
		}
		else// if (r == 0)
		{
			if (m_iClip == 0)
				SendWeaponAnim(RPG_IDLE_UL);
			else
				SendWeaponAnim(RPG_IDLE);
		}
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0f;
	}
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
}

void CWeaponRPG::Reload(void)
{
	if (m_iClip > 0)
		return;

	if (!HasAmmo(AMMO_ANYTYPE))
		return;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

	if (m_cActiveRockets && m_fSpotActive)
		return;

	if (DefaultReload(RPG_RELOAD, RPG_RELOAD_TIME, "weapons/rpg_reload.wav"))
	{
#if !defined (CLIENT_DLL)
		if (m_fSpotActive && m_hLaserSpot.Get())
		{
			CLaserSpot *pSpot = (CLaserSpot *)(CBaseEntity *)m_hLaserSpot;
			if (pSpot)
				pSpot->Suspend(RPG_RELOAD_TIME);

			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RPG_RELOAD_TIME;
		}
#endif
		m_pPlayer->ResetAutoaim();// XDM
	}
}

void CWeaponRPG::UpdateSpot(void)
{
#if !defined (CLIENT_DLL)
	if (m_fSpotActive)
	{
		if (m_hLaserSpot.Get() == NULL)
		{
			m_hLaserSpot = CLaserSpot::CreateSpot();
			m_pPlayer->ResetAutoaim();// XDM
			EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/rpg_aim_on.wav", VOL_NORM, ATTN_NORM);
			pev->skin = 1;
		}

		ASSERT(m_hLaserSpot.Get() != NULL);
		Vector vecForward;
		AngleVectors(m_pPlayer->pev->v_angle, vecForward, NULL, NULL);
		Vector vecSrc(m_pPlayer->GetGunPosition());
		Vector vecEnd(vecForward); vecEnd *= g_psv_zmax->value; vecEnd += vecSrc;
		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ignore_glass, m_pPlayer->edict(), &tr);
		if (tr.fAllSolid)// XDM3038: just in case
		{
			SetBits(m_hLaserSpot->pev->effects, EF_NODRAW);
		}
		else
		{
			UTIL_SetOrigin(m_hLaserSpot, tr.vecEndPos + tr.vecPlaneNormal * 0.1f);// XDM: pull out of the wall

			const char *tex = NULL;// runtime libraries fault protection
			if (tr.pHit && tr.pHit->v.solid == SOLID_BSP)// XDM3038: extra check
				tex = TRACE_TEXTURE(tr.pHit, m_hLaserSpot->pev->origin, vecEnd);

			if (tex != NULL && _stricmp(tex, "sky") == 0)// Texture name can be 'sky' or 'SKY' so use strIcmp!
				SetBits(m_hLaserSpot->pev->effects, EF_NODRAW);// always hide the spot
			else
				ClearBits(m_hLaserSpot->pev->effects, EF_NODRAW);

			if (!FBitSet(m_hLaserSpot->pev->effects, EF_NODRAW))
			{
#if defined (NOSQB)
				VectorAngles(-tr.vecPlaneNormal, m_hLaserSpot->pev->angles);// XDM3038
#else
				/*Vector n = tr.vecPlaneNormal;// XDM: for 'oriented' sprite
				n.x *= -1.0f;
				n.y *= -1.0f;*/
				tr.vecPlaneNormal.x *= -1.0f;
				tr.vecPlaneNormal.y *= -1.0f;
				//m_hLaserSpot->pev->angles = UTIL_VecToAngles(tr.vecPlaneNormal);
				VectorAngles(tr.vecPlaneNormal, m_hLaserSpot->pev->angles);// XDM3038
#endif
				/*if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM3035: this creates additional traffic
				{
					m_hLaserSpot->pev->renderamt = min(255, 31 + 255*(1.0-tr.flFraction));
					m_hLaserSpot->pev->scale = 0.1f + tr.flFraction*1.5f;
				}*/
			}
		}
	}
	else// if (!m_fSpotActive && m_hLaserSpot.Get())
	{
		if (m_hLaserSpot.Get())
		{
			EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/rpg_aim_off.wav", VOL_NORM, ATTN_NORM);
			m_hLaserSpot->Killed(NULL, NULL, 0);
			m_hLaserSpot = NULL;
			pev->skin = 0;
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: rocket calls this
// Input  : *pChild - rocket
//-----------------------------------------------------------------------------
void CWeaponRPG::DeathNotice(CBaseEntity *pChild)
{
	ASSERT(pChild->m_hOwner == GetDamageAttacker());
	if (m_cActiveRockets > 0)
		m_cActiveRockets--;
	else
		conprintf(1, "%s[%d]::DeathNotice(%s[%d]) error: there were no active rockets!\n", STRING(pev->classname), entindex(), STRING(pChild->pev->classname), pChild->entindex());
}

//-----------------------------------------------------------------------------
// Purpose: entity is detached from its former owner
//-----------------------------------------------------------------------------
void CWeaponRPG::OnDetached(void)// XDM3038c
{
	CBasePlayerWeapon::OnDetached();// XDM3038c
#if !defined (CLIENT_DLL)
	if (m_hLaserSpot.Get())
	{
		m_hLaserSpot->Killed(NULL, NULL, GIB_NEVER);
		m_hLaserSpot = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: entity is about to be erased from world and server memory
//-----------------------------------------------------------------------------
void CWeaponRPG::OnFreePrivateData(void)// XDM3037
{
#if !defined (CLIENT_DLL)
	if (m_hLaserSpot.Get())
	{
		CBaseEntity *pSpotEnt = (CBaseEntity *)m_hLaserSpot;
		if (pSpotEnt)
			m_hLaserSpot->Killed(NULL, NULL, GIB_NEVER);
	}
	m_hLaserSpot = NULL;
#endif
	CBasePlayerWeapon::OnFreePrivateData();
}

//-----------------------------------------------------------------------------
// Purpose: Server-side only. Pack data.
// Warning: Derived weapon classes MUST CALL this!
// Input  : *player - receiver
//			*weapondata - pack data into this structure
//-----------------------------------------------------------------------------
void CWeaponRPG::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->m_fInZoom = m_fSpotActive;
}
