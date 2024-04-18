#ifndef ITEMS_H
#define ITEMS_H

enum ITEMS
{
	ITEM_NONE = 0,
	ITEM_HEALTHKIT,
	ITEM_ANTIDOTE,
	ITEM_SECURITY,
	ITEM_BATTERY,
	ITEM_FLARE,// XDM
	ITEM_CUSTOM1
};

#define DEFAULT_ANTIDOTE_GIVE	3
#define DEFAULT_FLARE_GIVE		5
#define DEFAULT_AIRTANK_GIVE	50

#define FLARE_LIGHT_TIME		20

#define SF_ITEM_REQUIRESUIT		(1<<0)// !!! conflicts with item_suit flag, but that's OK because suit cannot require itself :)
#define SF_ITEM_FL2				(1<<1)
//#define SF_ITEM_FL3			(1<<2)
#define SF_ITEM_QUEST			(1<<3)// TODO: right now prevents respawning
// !!!#define SF_ITEM_NOFALL		0x0200

// TODO
//class CBasePickup : public CBaseToggle
//class CBasePlayerItemItem : public CBasePickup
//class CItem : public CBasePickup or do we have to have it?
//class CAmmo : public CBasePickup
//class CWeapon : public CBasePickup

class CItem : public CBaseAnimating
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void Materialize(void);// XDM3038c
	virtual void OnRespawn(void);// XDM3038c
	virtual CBaseEntity *StartRespawn(void);// XDM3038c

	virtual bool ShouldRespawn(void) const;// XDM3038c
	virtual bool IsPickup(void) const { return true; }// XDM3037
	//virtual bool IsPushable(void) { return !FBitSet(pev->spawnflags, SF_ITEM_QUEST); }// if we need it in the future

	//virtual int GetCapacity(void);// XDM3038c: initial, maximum amount!
	virtual int MyTouch(CBasePlayer *pPlayer);

	void EXPORT ItemTouch(CBaseEntity *pOther);
	void EXPORT FallThink(void);
};

#endif // ITEMS_H
