#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#if !defined (CLIENT_DLL)
#include "squeakgrenade.h"
#endif
#include "pm_shared.h"

// XDM3038c: w_sqknest.mdl is no longer used by the code

enum vanim_squeak_e
{
	SQUEAK_IDLE1 = 0,
	SQUEAK_FIDGETFIT,
	SQUEAK_FIDGETNIP,
	SQUEAK_DOWN,
	SQUEAK_UP,
	SQUEAK_THROW
};

// 3 groups, 2 submodels each
//#define VSQUEAK_BODY_BOX		0
//#define VSQUEAK_BODY_RAT		2

// IMPORTANT: these must match in p_ and v_ models!
// NOTE: these are redefined with different names only to be available to client code that does not include "squeakgrenade.h"
enum vsqueak_bodygroups_e
{
	VSQUEAK_BODYGROUP_BOX = 0,
	VSQUEAK_BODYGROUP_BODY,
	VSQUEAK_BODYGROUP_HANDS
};

enum vsqueak_body_box_e
{
	VSQUEAK_BODY_BOX_ON = 0,
	VSQUEAK_BODY_BOX_OFF
};

enum vsqueak_body_body_e
{
	VSQUEAK_BODY_BODY_OFF = 0,
	VSQUEAK_BODY_BODY_ON
};

/* hands are just always black in p_model
enum vsqueak_body_hands_e
{
	VSQUEAK_BODY_HANDS_OFF = 0,
	VSQUEAK_BODY_HANDS_ON
};*/

enum wanim_squeak_e
{
	WSQUEAK_BOX_IDLE = 0,
	WSQUEAK_BOX_STAYPUT,
	WSQUEAK_BOX_OPEN,
	WSQUEAK_BOX_OPENED,
	WSQUEAK_HAND_IDLE
};

LINK_ENTITY_TO_CLASS(weapon_snark, CWeaponSqueak);

TYPEDESCRIPTION	CWeaponSqueak::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponSqueak, m_fJustThrown, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponSqueak, CBasePlayerWeapon);

void CWeaponSqueak::Spawn(void)
{
	CBasePlayerWeapon::Spawn();// XDM3038a
	//pev->scale = 1.0;// XDM: overridden for this w_ model
	pev->sequence = WSQUEAK_BOX_IDLE;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	//pev->body = VSQUEAK_BODY_BOX;// XDM3038c
	SetBodygroup(VSQUEAK_BODYGROUP_BOX, VSQUEAK_BODY_BOX_ON);
	SetBodygroup(VSQUEAK_BODYGROUP_BODY, VSQUEAK_BODY_BODY_OFF);
}

void CWeaponSqueak::Precache(void)
{
	m_iId = WEAPON_SNARK;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = SNARK_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a: when customizing, use only one model
		pev->model = MAKE_STRING("models/p_squeak.mdl");
	/*{
		pev->model = MAKE_STRING("models/w_sqknest.mdl");// goes to pev->modelindex AND m_iModelIndexWorld
		pev->weaponmodel = MAKE_STRING("models/p_squeak.mdl");// HACK: to use separate p_model
		PRECACHE_MODEL(STRINGV(pev->weaponmodel));
	}
	else
		pev->weaponmodel = pev->model;*/

	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_squeak.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.snarkDmgBite;

	PRECACHE_SOUND("weapons/sqk_select.wav");
	//UTIL_PrecacheOther(SQUEAK_CLASSNAME);
	//UTIL_PrecacheOther("squeakbox");// XDM
	//m_usFire = PRECACHE_EVENT(1, "events/weapons/snarkfire.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponSqueak::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_SNARK;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iMaxClip = SNARK_MAX_CLIP;
	p->iWeight = SNARK_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_SNARK;
	p->iPosition = POSITION_SNARK;
#endif
	p->pszAmmo1 = "Snarks";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = SNARK_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponSqueak::Deploy(void)
{
	//old pev->model = pev->weaponmodel;
	//pev->body = VSQUEAK_BODY_RAT;
	SetBodygroup(VSQUEAK_BODYGROUP_BOX, VSQUEAK_BODY_BOX_OFF);
	SetBodygroup(VSQUEAK_BODYGROUP_BODY, VSQUEAK_BODY_BODY_ON);
	if (DefaultDeploy(SQUEAK_UP, "squeak", "weapons/sqk_select.wav"))
	{
		//old m_iModelIndexWorld = MODEL_INDEX("models/w_sqknest.mdl");
		// this is 0	m_iModelIndexWorld = pev->modelindex;// XDM3037: override model
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		pev->sequence = WSQUEAK_HAND_IDLE;
		pev->frame = 0;
		return true;
	}
	return false;
}

void CWeaponSqueak::Holster(int skiplocal /* = 0 */)
{
	SetBodygroup(VSQUEAK_BODYGROUP_BOX, VSQUEAK_BODY_BOX_ON);
	SetBodygroup(VSQUEAK_BODYGROUP_BODY, VSQUEAK_BODY_BODY_OFF);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (HasAmmo(AMMO_PRIMARY))
		SendWeaponAnim(SQUEAK_DOWN);

	//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponSqueak::PrimaryAttack(void)
{
	if (!HasAmmo(AMMO_PRIMARY))
		return;

	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)// XDM3035: fixed! Weapons spawned under water
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "squeek/sqk_die1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	TraceResult tr;
	Vector trace_origin(m_pPlayer->pev->origin);
	// HACK HACK: Ugly hacks to handle change in origin based on new physics code for players
	// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
	if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		trace_origin -= (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
	// find place to toss monster
	UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 20.0f, trace_origin + gpGlobals->v_forward * 64.0f, dont_ignore_monsters, NULL, &tr);
	if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25f)
	{
		UseAmmo(AMMO_PRIMARY, 1);
		SendWeaponAnim(SQUEAK_THROW);
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#if !defined (CLIENT_DLL)
		CBaseEntity *pSnark = CBaseEntity::Create(SQUEAK_CLASSNAME, tr.vecEndPos, vecAng, gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity, GetDamageAttacker()->edict());
		if (pSnark)
		{
			pSnark->pev->dmg = pev->dmg;// XDM3038c
			pSnark->pev->owner = GetDamageAttacker()->edict();// XDM3037: SetIgnoreEnt
		}
#endif
		switch (RANDOM_LONG(0,2))
		{
		case 0: EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "squeek/sqk_hunt1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM); break;
		case 1: EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "squeek/sqk_hunt2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM); break;
		case 2: EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "squeek/sqk_hunt3.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM); break;
		}
		m_fJustThrown = 1;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SQUEAK_ATTACK_INTERVAL1);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	}
}

void CWeaponSqueak::SecondaryAttack(void)
{
	if (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) >= SNARK_DEFAULT_GIVE)
	{
		SendWeaponAnim(SQUEAK_THROW);
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#if !defined (CLIENT_DLL)
		Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		CSqueakBox *pBox = (CSqueakBox *)CSqueakBox::Create("squeakbox", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 2.0f, vecAng, m_pPlayer->pev->velocity + gpGlobals->v_forward * 240.0f, GetDamageAttacker()->edict());
		if (pBox)
		{
			pBox->pev->owner = GetDamageAttacker()->edict();
			pBox->Open();
		}
#endif
		UseAmmo(AMMO_PRIMARY, SNARK_DEFAULT_GIVE);
		m_pPlayer->m_iWeaponVolume = 100;
		m_fJustThrown = 1;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SQUEAK_ATTACK_INTERVAL2);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	}
}

void CWeaponSqueak::WeaponIdle(void)
{
	if (m_fJustThrown)
	{
		m_fJustThrown = 0;
		if (HasAmmo(AMMO_PRIMARY))
		{
			SendWeaponAnim(SQUEAK_UP);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
			RetireWeapon();

		return;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 2;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 2)
		SendWeaponAnim(SQUEAK_FIDGETNIP, pev->body);
	else if (r == 1)
		SendWeaponAnim(SQUEAK_FIDGETFIT, pev->body);
	else
		SendWeaponAnim(SQUEAK_IDLE1, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 4, 8);
}
