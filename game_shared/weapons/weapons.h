#ifndef WEAPONS_H
#define WEAPONS_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

#include "weapondef.h"
#include "weaponinfo.h"
#include "basemonster.h"
#include "shared_resources.h"
#include "color.h"
#include "projectiles/projectile.h"

#if defined (_DEBUG_ITEMS)
#define DBG_ITM_PRINT	DBG_PrintF
#else
#define DBG_ITM_PRINT
#endif

// In the future it should be possible to just attach weapon to a player using pev->aiment
#define WEAPON_MODELATTACH// global flag to use old HL mechanism - pev->weaponmodel

typedef struct multidamage_s
{
	CBaseEntity		*pEntity;
	float			amount;
	int				type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

void ClearMultiDamage(void);
int ApplyMultiDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker);
int AddMultiDamage(CBaseEntity *pInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);


// XDM3035a
extern ItemInfo g_ItemInfoArray[MAX_WEAPONS];
extern AmmoInfo g_AmmoInfoArray[MAX_AMMO_SLOTS];
extern int giAmmoIndex;

void DecalGunshot(TraceResult *pTrace, const int &iBulletType);
int DamageDecal(CBaseEntity *pEntity, const int &bitsDamageType);
void RadiusDamage(const Vector &vecSrc, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType);
const float GetDefaultBulletDistance(void);// XDM3038b
Vector FireBullets(ULONG cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, Vector *endpos, const float flDistance, const int iBulletType, const float fDamage = 0.0f, CBaseEntity *pInflictor = NULL, CBaseEntity *pAttacker = NULL, int shared_rand = 0);


void DeactivateSatchels(CBasePlayer *pOwner);
void DeactivateMines(CBasePlayer *pOwner);// XDM3035
bool CleanupTemporaryWeaponBoxes(void);// XDM3038

//-----------------------------------------------------------------------------
size_t SendWeaponsRegistry(CBaseEntity *pPlayer);
size_t SendAmmoRegistry(CBaseEntity *pPlayer);

void SetViewModel(const char *szViewModel, CBasePlayer *m_pPlayer);

void W_Precache(void);

int RegisterAmmoTypes(const char *szAmmoRegistryFile);
int AddAmmoToRegistry(const char *szAmmoName, short iMaxCarry);
int GetAmmoIndexFromRegistry(const char *szAmmoName);
short MaxAmmoCarry(int ammoID);
short MaxAmmoCarry(const char *szName);

//-----------------------------------------------------------------------------
class CBasePlayerWeapon;
class CBaseMonster;
class CBasePlayer;
class CBeam;
class CSprite;


// HasAmmo() FLAGS
#define AMMO_ANYTYPE	0// For HasAmmo() only!! Same as (AMMO_PRIMARY|AMMO_SECONDARY).
#define AMMO_PRIMARY	1
#define AMMO_SECONDARY	2
#define AMMO_CLIP		4// check clip separately




//-----------------------------------------------------------------------------
// XDM: Base grenade class
// Hand grenades, ARgrenades, launcher grenades and most other projectiles
//-----------------------------------------------------------------------------
class CGrenade : public CBaseProjectile
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);

	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual bool FBecomeProne(void) { return true; }// XDM: for barnacles 8)
	virtual bool IsPushable(void) { return true; }
	virtual void BounceSound(void);
	virtual bool CheckContents(const Vector &vecOrigin);// XDM3038c
	void Explode(const Vector &vecOrigin, int bitsDamageType, int spr, int fps, int spr_uw, int fps_uw, float scale, const char *customsound = NULL);
	void DoDamageInit(const float &life, const float &updatetime, const float &damage, const float &damagedelta, const float &radius, const float &radiusdelta, int bitsDamageType);

	void EXPORT DangerSoundThink(void);
	void EXPORT Detonate(void);
	void EXPORT TumbleThink(void);
	void EXPORT DoDamageThink(void);// XDM
	void EXPORT FreezeThink(void);
	void EXPORT FreezeThinkEnd(void);
	void EXPORT PoisonThink(void);
	void EXPORT PoisonThinkEnd(void);
	void EXPORT BurnThink(void);
	//void EXPORT BurnThinkEnd(void);
	void EXPORT RadiationThink(void);
	void EXPORT RadiationThinkEnd(void);
	//void EXPORT TeleportationThink(void);
	//void EXPORT TeleportationThinkEnd(void);
	void EXPORT NuclearExplodeThink(void);

	void EXPORT BounceTouch(CBaseEntity *pOther);
	//void EXPORT SlideTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);

	//static CGrenade *ShootContact(const Vector &vecStart, const Vector &vecVelocity, CBaseEntity *pOwner, CBaseEntity *pEntIgnore);
	static CGrenade *ShootTimed(const Vector &vecStart, const Vector &vecVelocity, float time, int type, CBaseEntity *pOwner, CBaseEntity *pEntIgnore);

	BOOL m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
};


//-----------------------------------------------------------------------------
// XDM: Base item class 
// Items that can be stored in inventory as objects and be equipped and used
//-----------------------------------------------------------------------------
class CBasePlayerItem : public CBaseAnimating// UNDONE: public CBasePickup
{
public:
	//virtual void SetObjectCollisionBox(void);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Materialize(void);
	virtual void OnRespawn(void);
	virtual CBaseEntity *StartRespawn(void);
	virtual int	ObjectCaps(void);// XDM3038a
	virtual void Destroy(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);// XDM3035c
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM3035c
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3035c
	virtual void UpdateOnRemove(void);// XDM3038a
	virtual void OnFreePrivateData(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// XDM3038a
	virtual void ReportState(int printlevel);// XDM3038c

	virtual bool ShouldRespawn(void) const;// XDM3038c
	virtual bool IsPushable(void);
	virtual bool IsPickup(void) const { return true; }// XDM3037
	virtual bool IsPlayerItem(void) const { return true; }// XDM3035
	virtual bool IsPlayerWeapon(void) const { return false; }// XDM3035
	virtual bool ShowOnMap(CBasePlayer *pPlayer) const;// XDM3038c

	virtual bool AddToPlayer(CBasePlayer *pPlayer);
	virtual bool AddDuplicate(CBasePlayerItem *pOriginal);
	virtual void OnAttached(void) { }// overridable
	virtual void OnDetached(void) { }// overridable

	virtual void ItemPreFrame(void)	{ }// called at the beginning of each frame by the player's PreThink
	virtual void ItemPostFrame(void) { }// called each frame by the player's PostThink
	virtual void ClientPostFrame(struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, const double &time, const unsigned int &random_seed) { }// called by HUD_WeaponsPostThink (client-side only)
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata) { }// called by GetWeaponData (server-side only)

	virtual bool CanDeploy(void) const { return true; }// can this weapon be drawn right now?
	virtual bool CanHolster(void) const { return true; }// can this weapon be put away right now?
	virtual bool ForceAutoaim(void) const { return false; }// always enable autoaim
	virtual bool IsHolstered(void) const;// XDM3035
	virtual bool IsCarried(void) const;// is this item carried by someone

	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	//virtual void OnOwnerDied(CBasePlayer *pOwner) {}// XDM3034
	virtual const float &GetLastUseTime(void) const { return 0.0f; }// XDM3038c

	virtual uint32 UpdateClientData(char *pBuffer = NULL) { return 0; }
	//virtual void UpdateItemInfo(void);

	virtual int GetItemInfo(ItemInfo *p) { return 0; }// Used to fill global info array g_ItemInfoArray, returns 0 if struct not filled out
	virtual const char *GetWeaponName(void) const;// XDM3038c Do not save this pointer!!!
	virtual CBasePlayerWeapon *GetWeaponPtr(void) { return NULL; }// overridable for easy class separation

	void EXPORT DefaultTouch(CBaseEntity *pOther);// default weapon touch
	void EXPORT FallThink(void);
	void EXPORT AttemptToRespawn(void);// overload for SUB_Respawn

	void SetOwner(CBaseEntity *pOwner);
	void AttachTo(CBaseEntity *pHost);
	void DetachFromHost(void);
	CBasePlayer	*GetHost(void) const;// for safety checks
	inline const int &GetID(void) const { return m_iId; }// for safety checks

	// types should match ItemInfo struct
	short		iFlags(void) const			{ return g_ItemInfoArray[m_iId].iFlags; }
	short		iWeight(void) const			{ return g_ItemInfoArray[m_iId].iWeight; }
#if defined(SERVER_WEAPON_SLOTS)
	byte		iItemSlot(void) const		{ return g_ItemInfoArray[m_iId].iSlot + 1; }
	byte		iItemPosition(void) const	{ return g_ItemInfoArray[m_iId].iPosition; }
#endif
	short		iMaxClip(void) const		{ return g_ItemInfoArray[m_iId].iMaxClip; }
	const char	*pszAmmo1(void) const		{ return g_ItemInfoArray[m_iId].pszAmmo1; }
#if defined(OLD_WEAPON_AMMO_INFO)
	short		iMaxAmmo1(void) const		{ return g_ItemInfoArray[m_iId].iMaxAmmo1; }
#endif
	const char	*pszAmmo2(void) const		{ return g_ItemInfoArray[m_iId].pszAmmo2; }
#if defined(OLD_WEAPON_AMMO_INFO)
	short		iMaxAmmo2(void) const		{ return g_ItemInfoArray[m_iId].iMaxAmmo2; }
#endif

	static TYPEDESCRIPTION m_SaveData[];

protected:
	CBasePlayer	*m_pPlayer;
	EHANDLE		m_hLastOwner;// never reset this
	int			m_iId;// WEAPON_CROWBAR
	int			m_iModelIndexView;// v_
	int			m_iModelIndexWorld;// p_
	//pev->impulse	int			m_iItemState;// XDM3038: ITEM_STATE
};


//-----------------------------------------------------------------------------
// XDM: Base weapon class
// CBasePlayerItem virtual functions should go first!
//-----------------------------------------------------------------------------
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Destroy(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void ReportState(int printlevel);// XDM3038c
	virtual CBaseEntity *GetDamageAttacker(void);// XDM3037

	virtual bool AddToPlayer(CBasePlayer *pPlayer);
	virtual bool AddDuplicate(CBasePlayerItem *pOriginal);
	virtual void OnAttached(void);// XDM3038c: overridable
	virtual void OnDetached(void);// XDM3038c: overridable

	virtual void ItemPostFrame(void);
	virtual void ClientPostFrame(struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, const double &time, const unsigned int &random_seed);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	virtual uint32 UpdateClientData(char *pBuffer);

	virtual bool CanDeploy(void) const;
	virtual bool ForceAutoaim(void) const { return false; }// XDM
	virtual bool IsPlayerWeapon(void) const { return true; }// XDM3035
	virtual bool HasAmmo(short type, short count = 0) const;

	virtual bool CanAttack(const float &attack_time);// XDM3035
	virtual bool IsUseable(void) const;
	virtual bool ShouldWeaponIdle(void) const { return false; }
	virtual bool UseDecrement(void) const;

	virtual int	PrimaryAmmoIndex(void) const;// const;
	virtual int	SecondaryAmmoIndex(void) const;// const;

	virtual int ExtractAmmo(CBasePlayerWeapon *pDestWeapon);
	//virtual int ExtractClipAmmo(CBasePlayerWeapon *pDestWeapon);
	virtual bool PlayEmptySound(void);
	virtual void ResetEmptySound(void);
	virtual void SendWeaponAnim(const int &iAnim, const int &body = 0, bool skiplocal = true);

	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void) { return; }	// "+ATTACK"
	virtual void SecondaryAttack(void) { return; }	// "+ATTACK2"
	virtual void Reload(void) { return; }			// "+RELOAD"
	virtual void WeaponIdle(void) { return; }		// called when no buttons pressed
	virtual void RetireWeapon(void);
	virtual int UseAmmo(byte type, int count);// XDM3035c
	virtual const float &GetLastUseTime(void) const { return m_flLastAttackTime; }// XDM3038c
	virtual float GetBarrelLength(void) const;// XDM3038
	virtual CBasePlayerWeapon *GetWeaponPtr(void) { return this; }

	int AddPrimaryAmmo(const int &iCount, const int &iAmmoIndex);//, const int &iMaxClip, const int &iMaxCarry);
	int AddSecondaryAmmo(const int &iCount, const int &iAmmoIndex);//, const int &iMaxCarry);
	void PackAmmo(const int &iCount, bool bSecondary);//, bool bPackClip);// can only contain ammo of local types

	bool DefaultDeploy(int iViewAnim, const char *szAnimExt, const char *szSound, int skiplocal = 0, int body = 0);// XDM3038a
	bool DefaultReload(const int &iAnim, const float &fDelay, const char *szSound = NULL);// XDM3038a, const int &body = 0);

	float GetNextAttackDelay(float delay);// HL20130901
	int GetWeaponState(void) const;// XDM3038a: weapon_state for UpdWeapons message or weapon_data_t::m_iWeaponState

	static TYPEDESCRIPTION m_SaveData[];

	float	m_flNextPrimaryAttack;		// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;	// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flNextAmmoBurn;			// XDM: while charging, when to absorb another unit of player's ammo?
	float	m_flTimeWeaponIdle;			// soonest time ItemPostFrame will call WeaponIdle
	float	m_flPumpTime;
	float	m_flLastAttackTime;			// XDM3037: track last attempt to shoot, set even if *Attack() was unsuccessful.
	// hle time creep vars
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;			

	int		m_fInReload;				// are we in the middle of a reload
	int		m_fInSpecialReload;			// are we in the middle of a reload for the shotguns
	int		m_iClip;					// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;				// the last version of m_iClip sent to client dll
	int		m_iClientWeaponState;		// the last version of the weapon state sent to client dll (is current weapon, is on target)
	int		m_iDefaultAmmo;				// left for compatibility
	uint32	m_iAmmoContained1;			// XDM3038c: ammo reaining in a dropped weapon
	uint32	m_iAmmoContained2;

protected:
	int		m_iPrimaryAmmoType;			// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;		// "secondary" ammo index into players m_rgAmmo[]
	int		m_iPlayEmptySound;
	//int	m_fFireOnEmpty;				// True when the gun is empty and the player is still holding down the attack key(s)
};


//-----------------------------------------------------------------------------
// XDM: Base ammo class. Ammo mechanism is different in XHL.
// Custom name is in pev->message
//-----------------------------------------------------------------------------
class CBasePlayerAmmo : public CBaseAnimating
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void Materialize(void);// XDM3038c
	virtual void OnRespawn(void);// XDM3038c
	virtual CBaseEntity *StartRespawn(void);// XDM3038c

	virtual bool ShouldRespawn(void) const;// XDM3038c
	virtual bool IsPushable(void);
	virtual bool IsPickup(void) const { return true; }// XDM3037
	virtual int AddAmmo(CBaseEntity *pOther);// XDM3038c

	void EXPORT DefaultTouch(CBaseEntity *pOther);
	void EXPORT FallThink(void);

#if defined(OLD_WEAPON_AMMO_INFO)
	inline int GetAmmoMax(void) { return m_iAmmoMax; }
	void InitAmmo(const uint32 &ammo_give, const char *name, const uint32 &ammo_max, const char *model);
#else
	void InitAmmo(const uint32 &ammo_give, const char *name, const char *model);
#endif
	inline const uint32 &GetAmmoGive(void) { return m_iAmmoGive; }
	inline const char *GetAmmoName(void) { return STRING(pev->message); }

	static TYPEDESCRIPTION m_SaveData[];
protected:
	uint32	m_iAmmoGive;
	uint32	m_iAmmoContained;// XDM3038c
#if defined(OLD_WEAPON_AMMO_INFO)
	uint32	m_iAmmoMax;
#endif
};


//-----------------------------------------------------------------------------
// XDM: Weapon Box
// A single entity that can store weapons and ammo
//-----------------------------------------------------------------------------
class CWeaponBox : public CBaseEntity// CBasePickup : public CContainer
{
public:
	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void OnFreePrivateData(void);
	virtual void Materialize(void);
	virtual void Destroy(void);
	virtual int	ObjectCaps(void) { return (CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE); }
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM3035c
	virtual bool ShouldRespawn(void) const;// XDM3038c
	virtual bool IsPickup(void) const { return true; }// XDM3037
	virtual bool ShowOnMap(CBasePlayer *pPlayer) const;// XDM3038c
	virtual void ReportState(int printlevel);// XDM3038c
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	void Clear(void);
	bool IsEmpty(void) const;
	bool HasWeapon(const int &iID) const;
	bool PackAmmo(const char *szName, const int &iCount);
	bool PackWeapon(CBasePlayerItem *pWeapon);
	bool Unpack(CBasePlayer *pPlayer);
	size_t UnpackAmmo(CBasePlayer *pPlayer);
	size_t UnpackWeapons(CBasePlayer *pPlayer);
	bool ValidateContents(void);

	void EXPORT PickupTouch(CBaseEntity *pOther);
	void EXPORT FallThink(void);
	void EXPORT WaitForOwner(void);

	static CWeaponBox *CreateBox(CBaseEntity *pOwner, const Vector &vecOrigin, const Vector &vecAngles);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	int	m_rgAmmo[MAX_AMMO_SLOTS];// ammo quantities by ID
	EHANDLE	m_rgpWeapons[MAX_WEAPONS];// weapons by ID
};





class CWeaponCrowbar : public CBasePlayerWeapon
{
public:
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);

	void EXPORT Smack(void);
	void Swing(void);

protected:
	int m_iSwing;
	TraceResult m_Trace;
	unsigned short m_usFire;
};




class CWeaponGlock : public CBasePlayerWeapon
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	void Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

protected:
	int m_iShell;
	unsigned short m_usFire;
};




class CWeaponPython : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	virtual void OnDetached(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	BOOL m_fInZoom;
	unsigned short m_usFire;
};




class CWeaponMP5 : public CBasePlayerWeapon
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);

protected:
	int m_iShell;
	unsigned short m_usFire1;
	unsigned short m_usFire2;
};




class CWeaponCrossbow : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	virtual void OnDetached(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	void FireBolt(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	int m_iInZoom;
	int m_iZoomCrosshair;
	int m_iZoomSound;
	unsigned short m_usFire;
	unsigned short m_usZC;
};




class CWeaponShotgun : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	virtual bool ShouldWeaponIdle(void) const { return true; }

	static TYPEDESCRIPTION m_SaveData[];
protected:
	float m_flNextReload;
	int m_iShell;
	unsigned short m_usFire1;
	unsigned short m_usFire2;
};




class CWeaponSniperRifle : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	virtual void OnDetached(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	int m_iInZoom;
	int m_iShell;
	int m_iZoomCrosshair;
	int m_iZoomSound;
	unsigned short m_usFire;
	unsigned short m_usZC;
};




class CLaserSpot : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int	ObjectCaps(void) { return FCAP_DONT_SAVE; }
	virtual Vector Center(void) { return pev->origin; }// rocket uses this often, so simplify!
	static CLaserSpot *CreateSpot(void);
	void Suspend(const float &flSuspendTime);
	void EXPORT Revive(void);
};




class CWeaponRPG : public CBasePlayerWeapon
{
	friend class CRpgRocket;
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Reload(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual bool CanHolster(void) const;
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual bool ShouldWeaponIdle(void) const { return true; }
	//virtual bool ForceAutoaim(void) { return true; }
	virtual void OnDetached(void);
	virtual void OnFreePrivateData(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	virtual void DeathNotice(CBaseEntity *pChild);// XDM3038c
	void UpdateSpot(void);

	static TYPEDESCRIPTION m_SaveData[];

	int m_fSpotActive;
	int m_cActiveRockets;// how many missiles in flight from this launcher right now?
protected:
	EHANDLE m_hLaserSpot;
	unsigned short m_usFire;
};




class CWeaponGauss : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual bool CanHolster(void) const;
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	void StartFire(void);
	void Fire(const Vector &vecOrigSrc, const Vector &vecDirShooting, float flDamage);
	float GetFullChargeTime(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	float m_flStartCharge;
	float m_flPlayAftershock;
	int m_fInAttack;
	int m_iSoundState;// don't save
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iBeamAlpha;
	WEAPON_FIREMODE m_fireMode;
	Color m_BeamColor1;// XDM3037a: don't use pev->rendercolor!
	Color m_BeamColor2;
	unsigned short m_usFire;
};




class CWeaponEgon : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual bool CanHolster(void) const;
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual void OnDetached(void);
	virtual void OnFreePrivateData(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	void Attack(void);
	void Fire(const Vector &vecOrigSrc, const Vector &vecDir);
	void EndAttack(void);
	void CreateEffect(void);
	void UpdateEffect(const Vector &startPoint, const Vector &endPoint, float timeBlend);
	void DestroyEffect(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
	float timedist;
	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.
	WEAPON_FIRESTATE m_fireState;
	WEAPON_FIREMODE m_fireMode;
};




class CWeaponHornetGun : public CBasePlayerWeapon
{
public:
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool AddToPlayer(CBasePlayer *pPlayer);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual bool IsUseable(void) const;
	virtual void Reload(void);
	virtual void WeaponIdle(void);

protected:
	float m_flRechargeTime;
	unsigned short m_usHornetFire;
};




class CWeaponHandGrenade : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual bool CanHolster(void) const;
	virtual void Holster(int skiplocal = 0);
	virtual void WeaponIdle(void);
	//virtual bool ShouldWeaponIdle(void) const { return true; }
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	float m_flStartThrow;
	float m_flReleaseThrow;
	unsigned short m_usGrenMode;
};




class CWeaponSatchel : public CBasePlayerWeapon
{
	friend class CSatchelCharge;
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool AddToPlayer(CBasePlayer *pPlayer);
	virtual bool AddDuplicate(CBasePlayerItem *pOriginal);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool CanDeploy(void) const;
	virtual bool Deploy(void);
	virtual bool IsUseable(void) const;
	virtual void Holster(int skiplocal = 0);
	virtual void WeaponIdle(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	void SwitchToRadio(void);
	void SwitchToCharge(void);

	void Throw(void);
	void RadioClick(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	int m_iChargeReady;
};




class CWeaponTripmine : public CBasePlayerWeapon
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void WeaponIdle(void);
	bool PlaceMine(short pingmode, short explodetype);

protected:
	unsigned short m_usFireMode;
};




class CWeaponSqueak : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void WeaponIdle(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	int m_fJustThrown;
	unsigned short m_usFire;
};




class CWeaponAcidLauncher : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual bool CanHolster(void) const;
	virtual void Holster(int skiplocal = 0);
	virtual void WeaponIdle(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	void Fire(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	WEAPON_FIRESTATE m_fireState;
	unsigned short m_usFire;
};




class CWeaponGrenadeLauncher : public CBasePlayerWeapon
{
public:
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	virtual bool CanAttack(const float &attack_time);// XDM3035
	void Fire(int firemode);

protected:
	int m_iShell;
	unsigned short m_usFire;
};




class CWeaponSword : public CBasePlayerWeapon
{
public:
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual bool ShouldWeaponIdle(void) const { return true; }
	void Attack(WEAPON_FIREMODE mode, vec_t distance);
};




class CWeaponChemGun : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void WeaponIdle(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	WEAPON_FIREMODE m_fireMode;
	unsigned short m_usFire;
};




class CWeaponPlasma : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual bool CanHolster(void) const;
	virtual bool IsUseable(void) const;
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual void OnDetached(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	void Attack(void);
	void Fire(const Vector &vecOrigSrc, const Vector &vecDir);
	void EndAttack(void);
	void CreateEffect(void);
	void DestroyEffect(void);
	void UpdateEffect(const Vector &startPoint, const Vector &endPoint);

protected:
	CBeam *m_pBeam;// TODO: use EHANDLE to track deletion of the beam
	CBeam *m_pNoise1;
	CBeam *m_pNoise2;
	CSprite *m_pSprite;
	BOOL m_fSpriteActive;
	WEAPON_FIREMODE m_fireMode;
	WEAPON_FIRESTATE m_fireState;
	int m_iSpriteIndexEnd;
	int m_iSpriteIndexHit;
	unsigned short m_usFire;
};




class CWeaponFlame : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void ItemPreFrame(void);// XDM3038c
	virtual void WeaponIdle(void);
	virtual void OnDetached(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);

	void Attack(WEAPON_FIREMODE mode);
	void EndAttack(void);
	void Fire1(const Vector &vecOrigSrc, const Vector &vecDir);
	void Fire2(const Vector &vecOrigSrc, const Vector &vecDir);
	void CreateEffect(const Vector &vecOrigSrc, const Vector &vecDir);
	void DestroyEffect(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	float m_fFlameDist;// changes over time
	float m_fFlameDistMax;// differs between fire modes
	float m_fFlameDelta;// increase/decrease rate (not only distance)
	float m_fConeRadius;
	Vector m_vFlameAngles;// current flame angles
	Vector m_vFlameAngVelocity;// how fast flame angles catch up with player view angles
	//EHANDLE m_hPhysEntFlameEnd;
	EHANDLE m_hBeam;
	WEAPON_FIREMODE m_fireMode;
	WEAPON_FIRESTATE m_fireState;
	unsigned short m_usFire;
};




class CWeaponDisplacer : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual bool ForceAutoaim(void) const { return true; }
	virtual bool ShouldWeaponIdle(void) const { return true; }
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	virtual bool CanHolster(void) const;
	virtual bool IsUseable(void) const;

	void StartFire(void);
	void ShootBall(void);
	void KillFX(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	WEAPON_FIREMODE m_fireMode;
	WEAPON_FIRESTATE m_fireState;
	unsigned short m_usFire;
};




class CWeaponBHG : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	virtual bool CanHolster(void) const;
	virtual bool IsUseable(void) const;
	//virtual bool ShouldWeaponIdle(void) const { return true; }
	void Fire(void);
	void Fail(void);
	void ChargeStart(void);
	void ChargeUpdate(void);
	int GetCurrentChargeLimit(void);
	float GetFullChargeTime(void);

	static TYPEDESCRIPTION m_SaveData[];
protected:
	float m_flStartCharge;
	int m_iSoundState;// don't save this
	WEAPON_FIREMODE m_fireMode;
	WEAPON_FIRESTATE m_fireState;
	unsigned short m_usFire;
};




class CWeaponStrikeTarget : public CBasePlayerWeapon
{
public:
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
};




class CWeaponBeamRifle : public CBasePlayerWeapon
{
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual bool CanHolster(void) const;
	virtual bool IsUseable(void) const;
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	virtual bool ShouldWeaponIdle(void) const { return true; }
	virtual void ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata);
	void StartFire(void);
	void Fire(void);

protected:
	WEAPON_FIREMODE m_fireMode;
	WEAPON_FIRESTATE m_fireState;
	int m_iGlow;
	int m_iBeam;
	int m_iCircle;
	int m_iStar;
	unsigned short m_usFire;
	unsigned short m_usFireBeam;
	unsigned short m_usBeamBlast;
};




class CWeaponDiskLauncher : public CBasePlayerWeapon
{
public:
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual void WeaponIdle(void);
	void Fire(int diskmode, int ammouse, int animation, float delay);

protected:
	unsigned short m_usFire;
};




class CWeaponCustom : public CBasePlayerWeapon
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int GetItemInfo(ItemInfo *p);
	virtual const char *GetWeaponName(void) const;// XDM3038c
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);
	virtual bool Deploy(void);
	virtual void Holster(int skiplocal = 0);
	virtual void Reload(void);
	virtual void WeaponIdle(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	bool UseBullets(const int &fireMode);
	static TYPEDESCRIPTION m_SaveData[];

protected:
	string_t m_iszModelWorld;
	string_t m_iszModelView;
	string_t m_iszWeaponName;// weapon_myweapon
	string_t m_iszAmmoName1;
	string_t m_iszAmmoName2;
	string_t m_iszProjectile1;
	string_t m_iszProjectile2;
	string_t m_iszCustomProjectile1;
	string_t m_iszCustomProjectile2;
	string_t m_iszFireTarget1;
	string_t m_iszFireTarget2;
	string_t m_iszPlayerAnimExt;
	float m_fAttackDelay1;
	float m_fAttackDelay2;
	float m_fAmmoRechargeDelay1;
	float m_fAmmoRechargeDelay2;
	float m_flRechargeTime1;
	float m_flRechargeTime2;
	float m_fReloadDelay;
	int m_iUseAmmo1;
	int m_iUseAmmo2;
	int m_iMaxClip;
	int m_iItemFlags;
	int m_iWeight;
	float m_fProjectileSpeed1;
	float m_fProjectileSpeed2;
	float m_fDamage1;
	float m_fDamage2;
	int m_iNumBullets1;
	int m_iNumBullets2;
	short m_iAnimationDeploy;
	short m_iAnimationHolster;
	short m_iAnimationFire1;
	short m_iAnimationFire2;
	short m_iAnimationReload;
	//all other	animations: m_iAnimationIdle;
};




//=============================================================================
// Projectiles
//=============================================================================

#define ROCKET_LIFE_TIME				3.0f
#define ROCKET_IGNITION_TIME			0.4f
#define ROCKET_SPEED_START				600.0f
#define ROCKET_SPEED_AIR				2048.0f
#define ROCKET_SPEED_WATER				512.0f

class CRpgRocket : public CGrenade
{
	friend class CWeaponRPG;
public:
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION; }
	void Detonate(void);
	void EXPORT FollowThink(void);
	void EXPORT IgniteThink(void);
	void EXPORT RocketTouch(CBaseEntity *pOther);

	static CRpgRocket *CreateRpgRocket(const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, CWeaponRPG *pLauncher, float fDamage);

	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hLauncher;// XDM3038c: safer than pointer
	float m_flIgniteTime;

protected:
	int m_iTrail;
	//int m_iGlow;

	unsigned short m_usRocket;

};


class CSatchelCharge : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }
	virtual void UpdateOnRemove(void);// XDM3034
	virtual void BounceSound(void);
	virtual void Deactivate(bool disintegrate);

	void EXPORT SatchelSlide(CBaseEntity *pOther);
	void EXPORT SatchelThink(void);
	void EXPORT SatchelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	static CSatchelCharge *CreateSatchelCharge(const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fScale, float fDamage);
};




class CTripmineGrenade : public CGrenade
{
public:
	//virtual int Classify(void) { return CLASS_NONE; }// hack to make monsters ignore it
	virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE | FCAP_ONLYDIRECT_USE; }
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool IsPushable(void) { return false; }
	virtual void Deactivate(bool disintegrate);// XDM3035

	bool MakeBeam(void);
	void KillBeam(void);
	bool CheckAttachSurface(void);

	void EXPORT PowerupThink(void);
	void EXPORT BeamBreakThink(void);
	void EXPORT RadiusThink(void);
	void EXPORT DelayDeathThink(void);

	static TYPEDESCRIPTION m_SaveData[];

protected:
	float m_flPowerUp;
	float m_flBeamLength;
	Vector m_vecDir;
	// redundant Vector m_vecEnd;
	Vector m_posHost;
	Vector m_angleHost;
	EHANDLE m_hHost;
	CBeam *m_pBeam;
};


class CARGrenade : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	static CARGrenade *CreateGrenade(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage);
};


class CLGrenade : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void BounceSound(void);
	void EXPORT TimeThink(void);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT Detonate(void);// virtual ?

	static CLGrenade *CreateGrenade(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, float fTime, bool contact);
};


#define AGRENADE_NORMAL_SCALE	1.0f
#define AGRENADE_SCALE_FACTOR	0.2f

class CAGrenade : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int	BloodColor(void) { return BLOOD_COLOR_GREEN; }// XDM3037
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	bool IsAttached(void);// XDM3037: explicit
	void EXPORT AcidTouch(CBaseEntity *pOther);
	void EXPORT AcidThink(void);
	void EXPORT DissociateThink(void);
	void EXPORT Detonate(void);// virtual ?

	static CAGrenade *ShootTimed(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float time, float singledamage, int numSubgrenades);

	static TYPEDESCRIPTION m_SaveData[];

protected:
	EHANDLE m_hAiment;
	int m_iCount;
	BOOL m_fTouched;
	Vector m_vecNormal;
	unsigned short m_usAGrenade;
};

#endif // WEAPONS_H
