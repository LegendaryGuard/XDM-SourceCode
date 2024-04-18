#ifndef PLAYER_H
#define PLAYER_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "basemonster.h"
#include "playerstats.h"

#if defined (_DEBUG_PLAYER)
#define DBG_PLR_PRINT	DBG_PrintF
#else
#define DBG_PLR_PRINT
#endif

#define MAX_CHECKPOINTS			16// XDM3035c: per level

#define CSUITPLAYLIST			4// max of 4 suit sentences queued up at any time

#define	SUIT_REPEAT_OK			0
#define SUIT_NEXT_IN_30SEC		30
#define SUIT_NEXT_IN_1MIN		60
#define SUIT_NEXT_IN_5MIN		300
#define SUIT_NEXT_IN_10MIN		600
#define SUIT_NEXT_IN_30MIN		1800
#define SUIT_NEXT_IN_1HOUR		3600

#define SUITUPDATETIME			3.5f
#define SUITFIRSTUPDATETIME		0.1f

#define CSUITNOREPEAT			32

// XDM3037a: values changed to fit unsigned variable
enum spawn_state_e
{
	SPAWNSTATE_UNCONNECTED = 0,
	SPAWNSTATE_CONNECTED,// FIRSTTIME, ClientConnected() was called
	SPAWNSTATE_SPAWNED,// Spawn() was called
	SPAWNSTATE_INITIALIZED// UpdateClientData() and InitHUD() were called
};

// Internal use only: player animation "type", not an index or something meaningful.
enum player_animtypes_e
{
	PLAYER_IDLE = 0,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_ARM,
	PLAYER_DISARM,
	PLAYER_RELOAD,
	PLAYER_CLIMB,
	PLAYER_FALL
};// player_animtypes_t;

// Player model sequences. Must reside in this specific order to be compatible with standard Half-Life player models
/* unused right now
enum anim_player_e
{
	// IDLE
	look_idle = 0,
	idle,
	deep_idle,
	// RUN/WALK
	run2,
	walk2handed,
	// SHOOT
	twohandshoot,
	// CROUCH
	crawl,
	crouch_idle,
	// JUMP
	jump,
	long_jump,
	// SWIM
	swim,
	treadwater,
	// RUN/WALK
	run,
	walk,
	// AIM/SHOOT
	aim_2,
	shoot_2,
	aim_1,
	shoot_1,
	// DEATH
	die_simple,
	die_backwards1,
	die_backwards,
	die_forwards,
	headshot,
	die_spin,
	gutshot,
	// WEAPON ANIMATIONS
	ref_aim_crowbar,
	ref_shoot_crowbar,
	crouch_aim_crowbar,
	crouch_shoot_crowbar,

	ref_aim_trip,
	ref_shoot_trip,
	crouch_aim_trip,
	crouch_shoot_trip,

	ref_aim_onehanded,
	ref_shoot_onehanded,
	crouch_aim_onehanded,
	crouch_shoot_onehanded,

	ref_aim_python,
	ref_shoot_python,
	crouch_aim_python,
	crouch_shoot_python,

	ref_aim_shotgun,
	ref_shoot_shotgun,
	crouch_aim_shotgun,
	crouch_shoot_shotgun,

	ref_aim_gauss,
	ref_shoot_gauss,
	crouch_aim_gauss,
	crouch_shoot_gauss,

	ref_aim_mp5,
	ref_shoot_mp5,
	crouch_aim_mp5,
	crouch_shoot_mp5,

	ref_aim_rpg,
	ref_shoot_rpg,
	crouch_aim_rpg,
	crouch_shoot_rpg,

	ref_aim_egon,
	ref_shoot_egon,
	crouch_aim_egon,
	crouch_shoot_egon,

	ref_aim_squeak,
	ref_shoot_squeak,
	crouch_aim_squeak,
	crouch_shoot_squeak,

	ref_aim_hive,
	ref_shoot_hive,
	crouch_aim_hive,
	crouch_shoot_hive,

	ref_aim_bow,
	ref_shoot_bow,
	crouch_aim_bow,
	crouch_shoot_bow,

	// DEAD POSES
	deadback,
	deadsitting,
	deadstomach,
	deadtable,
	// END onf standard sequences

	// Following are just a concept for now
	ref_reload_onehanded,
	crouch_reload_onehanded,

	ref_reload_python,
	crouch_reload_python,

	ref_reload_shotgun,
	crouch_reload_shotgun,

	ref_reload_mp5,
	crouch_reload_mp5,

	ref_reload_rpg,
	crouch_reload_rpg,

	ref_aim_bowscope,
	ref_shoot_bowscope,
	ref_reload_bow,
	crouch_aim_bowscope,
	crouch_shoot_bowscope,
	crouch_reload_bow,

	ref_cock_gren,
	crouch_cock_gren,
};*/

#define CHAT_INTERVAL				1.0f

// XDM3038a: anything beyond 1 byte is not sent to the client anyway
#define TRAIN_OFF					0x0000// disable HUD
// these bits are left for the actual speed value
#define TRAIN_ACTIVE				0x0080// enable HUD
#define TRAIN_NEW					0x0100// means value was updated and must be sent to client

//#define	FLASH_DRAIN_TIME			1.2f// 100 units/3 minutes
//#define	FLASH_CHARGE_TIME			0.2f// 100 units/20 seconds  (seconds per unit)
#define FLASHLIGHT_UPDATE_INTERVAL		0.2f// don't update too often

// if in range of radiation source, ping geiger counter
#define GEIGERDELAY					0.25f

//#define PLAYER_AIR_TIME				12.0f// lung full of air lasts this many seconds

#define	PLAYER_USE_SEARCH_RADIUS	48.0f

#define PLAYER_MAX_STAND_SPEED		2// player may slide a little
#define PLAYER_MAX_WALK_SPEED		220// otherwise player is considered running



//-----------------------------------------------------
// This is Half-Life player entity
//-----------------------------------------------------
class CBasePlayer : public CBaseMonster
{
	friend class CBasePlayerItem;
	friend class CBasePlayerWeapon;
public:
	virtual int Classify(void);
	virtual void Spawn(byte restore);// XDM
	virtual void Spawn(void);
	virtual void Precache(void);
	// Player is moved across the transition by other means
	virtual int ObjectCaps(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual void UpdateOnRemove(void);// XDM3038c
	virtual int IRelationship(CBaseEntity *pTarget);// XDM3038c

	virtual void Jump(void);
	virtual void Duck(void);
	virtual void Land(void);
	virtual void PreThink(void);
	virtual void PostThink(void);
	virtual Vector GetGunPosition(void);
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual Vector BodyTarget(const Vector &posSrc);// position to shoot at

	virtual	bool IsPlayer(void) const { return true; }// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned
	virtual bool IsNetClient(void) const { return FBitSet(pev->flags, FL_CLIENT); }// true; // Bots should return FALSE for this, they can't receive NET messages Spectators should return TRUE for this
	virtual	bool IsMonster(void) const { return false; }// XDM
	virtual bool IsHuman(void)/* const*/ { return true; }// XDM: modify if there will be non-human players
	virtual	bool IsBot(void) const { return FBitSet(pev->flags, FL_FAKECLIENT); }
	virtual bool IsObserver(void) const;
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual bool ShouldFadeOnDeath(void) const { return false; }
	virtual float DamageForce(const float &damage);

	virtual bool FBecomeProne(void);
	virtual void BarnacleVictimBitten(CBaseEntity *pBarnacle);
	virtual void BarnacleVictimReleased(void);
	virtual int Illumination(void);
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3038c
	virtual bool ShouldBeSentTo(CBasePlayer *pClient);// XDM3038c

	virtual void FrozenStart(float freezetime);// XDM
	virtual void FrozenEnd(void);
	virtual void FrozenThink(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);// XDM3035: players can 'use' each other

	void CalcShootSpreadFactor(void);// XDM3038c
	const float &GetShootSpreadFactor(void) { return m_fShootSpreadFactor; }// XDM3037
	Vector GetAutoaimVector(const float &flDelta);
	Vector AutoaimDeflection(const Vector &vecSrc, const float &flDist, const float &flDelta);

	int AmmoInventory(const int &iAmmoIndex);
	void TabulateAmmo(void);
#if defined(OLD_WEAPON_AMMO_INFO)
	virtual int GiveAmmo(const int &iAmount, const int &iIndex, const int &iMax);
#else
	virtual int GiveAmmo(const int &iAmount, const int &iIndex);
#endif

	virtual float FallDamage(const float &flFallVelocity);// XDM3035c
	virtual void ReportState(int printlevel);// XDM3038c

	// JOHN: sends custom messages if player HUD data has changed  (eg health, ammo)
	void UpdateClientData(void);
	bool UpdateSoundEnvironment(CBaseEntity *pEmitter, const int iRoomtype, const float flRange);
	void SendAmmoUpdate(void);
	void SendWeaponsUpdate(void);
	void SendEntitiesData(void);// XDM3037

	CBaseEntity *GiveNamedItem(const char *szName);// XDM3038c

	uint32 PackImportantItems(void);// XDM3038c
	void PackDeadPlayerItems(void);
	void RemoveAllItems(bool bRemoveSuit, bool bRemoveImportant);
	void ResetAutoaim(void);

	CBasePlayerItem *GetInventoryItem(const int &iItemId);
	//CBasePlayerWeapon *GetActiveWeapon(void);// XDM3037: safe way to get current weapon pointer
	//BOOL SwitchWeapon(CBasePlayerItem *pWeapon);
	void DeployActiveItem(void);
	int AddPlayerItem(CBasePlayerItem *pItem);
	bool RemovePlayerItem(CBasePlayerItem *pItem);
	bool DropPlayerItem(CBasePlayerItem *pItem);
	bool DropPlayerItem(const char *pszItemName);
	bool HasPlayerItem(CBasePlayerItem *pCheckItem);
	//use GetInventoryItem	BOOL HasPlayerItem(const int &iItemId);
	//BOOL HasNamedPlayerItem(const char *pszItemName);
	bool HasWeapons(void);// do I have ANY weapons?
	bool HasSuit(void);// XDM3037
	// OBSOLETE	void SelectLastItem(void);
	void QueueItem(CBasePlayerItem *pItem);
	bool SelectItem(CBasePlayerItem *pItem);// XDM3035
	bool SelectItem(const int &iID);// XDM3035
	bool SelectItem(const char *pstr);
	CBasePlayerItem *SelectNextBestItem(CBasePlayerItem *pItem);
	bool CanDeployItem(CBasePlayerItem *pItem);// XDM3038
	bool CanDropItem(CBasePlayerItem *pItem);// XDM3038a
	void ItemPreFrame(void);
	void ItemPostFrame(void);
	virtual bool CanAttack(void);// from CBaseMonster

	bool FlashlightIsOn(void);
	void FlashlightTurnOn(void);
	void FlashlightTurnOff(void);
	void FlashlightUpdate(const int &state);// XDM3034

	void UpdatePlayerSound(void);
	virtual void DeathSound(void);

	void SetAnimation(const int &iNewPlayerAnim);
	void SetGaitAnimation(void);// XDM3037: update gaitsequence
	void SetWeaponAnimType(const char *szExtention);

	// custom player functions
	virtual void ImpulseCommands(void);
	void CheatImpulseCommands(const int &iImpulse);

	// XDM3038a: standardized
	bool ObserverStart(const Vector &vecPosition, const Vector &vecViewAngle, int mode, CBaseEntity *pTarget);
	bool ObserverStop(void);
	void ObserverFindNextPlayer(bool bReverse);
	void ObserverHandleButtons(void);
	void ObserverSetMode(int iMode);
	bool ObserverValidateTarget(CBaseEntity *pTarget);
	bool ObserverSetTarget(CBaseEntity *pTarget);
	void ObserverCheckTarget(void);// HL20130901
	void ObserverCheckProperties(void);

	void EnableControl(bool fControl);

	bool IsOnLadder(void);
	void DisableLadder(const float &time);

	bool IsOnTrain(void);
	bool TrainAttach(CBaseEntity *pTrain);
	bool TrainDetach(void);
	CBaseDelay *GetControlledTrain(void);
	void TrainPreFrame(void);
	void WaterMove(void);
	void PlayerUse(void);
	void PlayerUseObject(const short iButtonState, CBaseEntity *pObject, const float value);

	void EXPORT PlayerDeathThink(void);

	void CheckSuitUpdate(void);
	void SetSuitUpdate(const char *name, int fgroup, int iNoRepeat);
	void UpdateGeigerCounter(void);
	void CheckTimeBasedDamage(void);
	void ForceClientDllUpdate(void);// Forces all client .dll specific data to be resent to client.

	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames(void);

	void ThrowNuclearDevice(void);

	void InitStatusBar(void);// text displayed when you aim at another object
	void UpdateStatusBar(void);

	/*TEAM_ID*/short GetLastTeam(void);// XDM3037a: allows layer of abstraction

	void OnCheckPoint(CBaseEntity *pCheckPoint);// CoOp, etc.
	bool PassedCheckPoint(CBaseEntity *pCheckPoint);
	CBaseEntity *GetLastCheckPoint(void);
	void CheckPointsClear(void);

	bool RightsCheck(uint32 iRightIndex);
	bool RightsCheckBits(uint32 iRightBits);
	//uint32 RightsGetBits(void) { return m_iUserRights; }
	void RightsAssign(uint32 iRightIndex);
	void RightsRemove(uint32 iRightIndex);

	bool StatsLoad(void);
	bool StatsSave(void);
	void StatsClear(void);

	void PlayAudioTrack(const char *pTrackName, int iCommand, const float &fTimeOffset);

public:
	static TYPEDESCRIPTION m_SaveData[];

	int				random_seed;// See that is shared between client & server for shared weapons code

	int				m_iTargetVolume;// ideal sound volume. 
	int				m_iWeaponVolume;// how loud the player's weapon is right now.
	int				m_iExtraSoundTypes;// additional classification for this weapon's sound
	int				m_iWeaponFlash;// brightness of the weapon flash
	float			m_flStopExtraSoundTime;

	float			m_flFlashLightTime;// Time until next battery draw/recharge
	float			m_fFlashBattery;// Flashlight battery level (must be float to accumulate data between updates)

	int				m_afButtonLast;// Button bits from previous frame
	int				m_afButtonPressed;// Buttons that CHANGED STATE to PRESSED during THIS FRAME
	int				m_afButtonReleased;// Buttons that CHANGED STATE to RELEASED during THIS FRAME

	EHANDLE			m_hEntSndLast;// last sound entity to modify player room type // XDM3038: EHANDLE
	int				m_flSndRoomtype;// last roomtype set by sound entity // XDM3038a: padding
	float			m_flSndRange;// dist from player to sound entity

	float			m_flSuitUpdate;// when to play next suit update
	int				m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int				m_iSuitPlayNext;// next sentence slot for queue storage;
	int				m_rgiSuitNoRepeat[CSUITNOREPEAT];// suit sentence no repeat list
	float			m_rgflSuitNoRepeatTime[CSUITNOREPEAT];// how long to wait before allowing repeat
	//int			m_lastDamageAmount;// Last damage taken
	float			m_tbdPrev;// Time-based damage timer

	int				m_bitsHUDDamage;// Damage bits for the current fame. These get sent to the hud via the DAMAGE message
	BOOL			m_fInitHUD;// True when deferred HUD restart msg needs to be sent
	BOOL			m_fGameHUDInitialized;
	int				m_iInitEntities;// XDM3035: do we need to ask entities to send client data? 1 = 1st time, 2 = request an update
	int				m_iInitEntity;// XDM3035: next entity to search for
	float			m_flInitEntitiesTime;// XDM3035: slow down updates to avoid datagram overflows

	int				m_iTrain;// Train control position
	EHANDLE			m_pTrain;// XDM3035a
	EHANDLE			m_pTank;// the tank which the player is currently controlling,  NULL if no tank

	BOOL			m_fNoPlayerSound;// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump;// does this player have the longjump module?
	float			m_fWaterCircleTime;// XDM: should really be on client side
	float			m_fGaitAnimFinishTime;// XDM3037: when gait animation will finish
	//int			m_iUpdateTime;// stores the number of frame ticks before sending HUD update messages
	int				m_iClientBattery;// the Battery currently known by the client.  If this changes, send a new
	int				m_iHideHUD;// the players hud weapon info is to be hidden
	int				m_iClientHideHUD;

	CBasePlayerItem *m_pActiveItem;// weapon in hands
	CBasePlayerItem *m_pLastItem;// previously selected weapon
	CBasePlayerItem *m_pNextItem;// selected weapon which is being drawn

	int				m_rgAmmo[MAX_AMMO_SLOTS];
	int				m_rgAmmoLast[MAX_AMMO_SLOTS];
	EHANDLE			m_rgpWeapons[PLAYER_INVENTORY_SIZE];// XDM: please do not access this directly
	uint32			m_rgItems[MAX_ITEMS];// Quantity, simple item IDs
	int				m_fInventoryChanged;// True when a new item needs to be added

	float			m_fShootSpreadFactor;// XDM3038c
	Vector			m_vecAutoAim;
	Vector			m_vecAutoAimPrev;// These are the previous update's crosshair angles, DON'T SAVE
	EHANDLE			m_hAutoaimTarget;
	BOOL			m_fOnTarget;

	uint32			m_iDeaths;// XDM3038: can't be < 0
	int				m_iRespawnFrames;// used in PlayerDeathThink() to make sure players can always respawn
	float			m_fDiedTime;// XDM3037: renamed to reflect purpose. the time at which the player died  (used in PlayerDeathThink())
	float			m_fNextSuicideTime;// the time after which the player can next use the suicide command

	float			m_flNextObserverInput;
	int				m_iObserverLastMode;// last used observer mode
	EHANDLE			m_hObserverTarget;// XDM

	CBaseEntity		*m_pCarryingObject;// not EHANDLE, because don't need to save
	//EHANDLE invalidates after each respawn!

	float			m_flgeigerRange;// range to nearest radiation source
	float			m_flgeigerDelay;// delay per update of range msg to client
	int				m_igeigerRangePrev;

	int				m_idrowndmg;// track drowning damage taken
	int				m_idrownrestored;// track drowning damage restored

	float			m_flIgnoreLadderStopTime;// XDM3037: time to enable ladder detection
	float			m_flNextDecalTime;// next time this player can spray a decal
	float			m_flNextChatTime;
	float			m_flThrowNDDTime;
	float			m_flLastSpawnTime;
	Vector			m_vecLastSpawnOrigin;// XDM3038
	Vector			m_vecLastSpawnAngles;
	int				m_iLastSpawnFlags;
	int				m_iNumTeamChanges;// XDM3037
	int				m_iScoreCombo;// XDM3035: accumulated score
	int				m_iLastScoreAward;// XDM3035: last type of award (or number of times scored)
	float			m_fNextScoreTime;// XDM3035: after this time passed the player won't get award increased
	EHANDLE			m_hszCheckPoints[MAX_CHECKPOINTS];// XDM3035c
	long			m_Stats[STAT_NUMELEMENTS];// XDM3037

	char			m_szCurrentMusicTrack[MAX_ENTITY_STRING_LENGTH];// XDM3038c: save
	float			m_iCurrentMusicTrackStartTime;// XDM3038c: time when this track start(ed) playing. Also used to delay the message.
	int				m_iCurrentMusicTrackCommand;// XDM3038c: save
	short			m_iCurrentMusicTrackCommandIssued;// XDM3038c: don't save

	short			m_iSpawnState;// XDM3037a: changed spawn_state_e

	// Developer commands use these
	Vector			m_MemOrigin;// coords
	Vector			m_MemAngles;
	TraceResult		m_PickTrace;
	CBaseEntity		*m_pPickEntity;
	BOOL			m_CoordsRemembered;

protected:
	unsigned int	m_afPhysicsFlags;// physics flags - set when 'normal' physics should be revisited or overriden
	int				m_nCustomSprayFrames;// Custom clan logo frames for this player

	uint32			m_iUserRights;// XDM3038c: user rights (to execute commands, etc.)

	CBasePlayerItem	*m_pClientActiveItem;// client version of the active item

	int				m_iSequenceDeepIdle;//LookupSequence("deep_idle");
	char			m_szAnimExtention[32];// can not be longer than name of studioseqhdr_t anyway

	float			m_flNextSBarUpdateTime;// time
	float			m_flStatusBarDisappearDelay;// time
	char			m_StatusBarString[SBAR_STRING_SIZE];
	short			m_iStatusBarValues[SBAR_NUMVALUES];

	short			m_iHUDDistortUpdate;// XDM3038: do client update this/next frame
	short			m_iHUDDistortMode;// XDM3038: flags, currently fits in 1 byte
	short			m_iHUDDistortValue;// XDM3038: actually it's 1 byte 0...255 -> client
};

#endif // PLAYER_H
