// Player code. Very far from Half-Life, but still hacky.
#include "extdll.h"
#include "util.h"
#include "pm_shared.h"
#include "cbase.h"
#include "player.h"
#include "animation.h"
#include "weapons.h"
#include "trains.h"
#include "nodes.h"
#include "sound.h"
#include "soundent.h"
#include "monsters.h"
#include "maprules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "ctf_gamerules.h"
#include "game.h"
#include "hltv.h"
#include "items.h"
#include "globals.h"
#include "shake.h"
#include "decals.h"
#include "explode.h"
#include "client.h"

// XDM3038c: renamed back to normal
TYPEDESCRIPTION	CBasePlayer::m_SaveData[] =
{
	//DEFINE_FIELD(CBasePlayer, random_seed, FIELD_INTEGER),			// Sent by usercmd

	DEFINE_FIELD(CBasePlayer, m_iTargetVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponFlash, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_flFlashLightTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_fFlashBattery, FIELD_FLOAT),

	DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

	//DEFINE_FIELD(CBasePlayer, m_hEntSndLast, FIELD_EHANDLE),			// Don't restore, client needs reset
	//DEFINE_FIELD(CBasePlayer, m_flSndRoomtype, FIELD_FLOAT),			// Don't restore, client needs reset
	//DEFINE_FIELD(CBasePlayer, m_flSndRange, FIELD_FLOAT),				// Don't restore, client needs reset

	DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
	DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
	DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
	DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
	//DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
	//DEFINE_FIELD(CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER),	// Only in multiplayer
	//DEFINE_FIELD(CBasePlayer, m_iInitEntities, FIELD_INTEGER),		// Don't restore, client needs reset
	//DEFINE_FIELD(CBasePlayer, m_iInitEntity, FIELD_INTEGER),			// Don't restore, client needs reset
	//DEFINE_FIELD(CBasePlayer, m_flInitEntitiesTime, FIELD_TIME),		// Don't restore, client needs reset

	DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_pTrain, FIELD_EHANDLE),// XDM3035a
	DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),

	//DEFINE_FIELD(CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN),		// Don't need to restore, debug
	DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
	//DEFINE_FIELD(CBasePlayer, m_fWaterCircleTime, FIELD_TIME),
	//DEFINE_FIELD(CBasePlayer, m_fGaitAnimFinishTime, FIELD_TIME),
	//DEFINE_FIELD(CBasePlayer, m_iUpdateTime, FIELD_INTEGER),			// Don't need to restore
	//DEFINE_FIELD(CBasePlayer, m_iClientBattery, FIELD_INTEGER),		// Don't restore, client needs reset
	DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayer, m_iClientHideHUD, FIELD_INTEGER),		// Don't restore, client needs reset

	DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayer, m_pLastItem, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayer, m_pNextItem, FIELD_CLASSPTR),// XDM

	DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
	//DEFINE_ARRAY(CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS),	// Don't restore, client needs reset
	DEFINE_ARRAY(CBasePlayer, m_rgpWeapons, FIELD_EHANDLE, PLAYER_INVENTORY_SIZE),// XDM3035b: ID=32
	DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_UINT32, MAX_ITEMS),

	//DEFINE_FIELD(CBasePlayer, m_fInventoryChanged, FIELD_INTEGER),	// Don't restore, client needs reset

	//DEFINE_FIELD(CBasePlayer, m_vecAutoAim, FIELD_VECTOR),			// Don't save/restore - this is recomputed
	//DEFINE_FIELD(CBasePlayer, m_vecAutoAimPrev, FIELD_VECTOR),		// Don't save/restore - this is recomputed
	//DEFINE_FIELD(CBasePlayer, m_hAutoaimTarget, FIELD_EHANDLE),		// Don't save/restore - this is recomputed
	//DEFINE_FIELD(CBasePlayer, m_fOnTarget, FIELD_BOOLEAN),			// Don't save/restore - this is recomputed

	DEFINE_FIELD(CBasePlayer, m_iDeaths, FIELD_UINT32),// XDM3038c
	//DEFINE_FIELD(CBasePlayer, m_iRespawnFrames, FIELD_INTEGER),		// Don't restore, client needs reset
	//DEFINE_FIELD(CBasePlayer, m_fDiedTime, FIELD_FLOAT),				// Only in multiplayer
	//DEFINE_FIELD(CBasePlayer, m_fNextSuicideTime, FIELD_TIME),		// Don't need to restore

	//DEFINE_FIELD(CBasePlayer, m_flNextObserverInput, FIELD_TIME),		// Don't need to restore
	//DEFINE_FIELD(CBasePlayer, m_iObserverLastMode, FIELD_INTEGER),	// Don't need to restore
	//DEFINE_FIELD(CBasePlayer, m_hObserverTarget, FIELD_EHANDLE),		// Don't need to restore

	DEFINE_FIELD(CBasePlayer, m_pCarryingObject, FIELD_CLASSPTR),// XDM3038c

	//DEFINE_FIELD(CBasePlayer, m_flgeigerRange, FIELD_FLOAT),			// Don't restore, reset in Precache()
	//DEFINE_FIELD(CBasePlayer, m_flgeigerDelay, FIELD_FLOAT),			// Don't restore, reset in Precache()
	//DEFINE_FIELD(CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT),		// Don't restore, reset in Precache()

	DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),

	DEFINE_FIELD(CBasePlayer, m_flIgnoreLadderStopTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flNextDecalTime, FIELD_TIME),
	//DEFINE_FIELD(CBasePlayer, m_flNextChatTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flThrowNDDTime, FIELD_TIME),
	//DEFINE_FIELD(CBasePlayer, m_flLastSpawnTime, FIELD_TIME),

	DEFINE_FIELD(CBasePlayer, m_vecLastSpawnOrigin, FIELD_VECTOR),
	DEFINE_FIELD(CBasePlayer, m_vecLastSpawnAngles, FIELD_VECTOR),

	//DEFINE_FIELD(CBasePlayer, m_iLastSpawnFlags, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayer, m_iNumTeamChanges, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayer, m_iScoreCombo, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayer, m_iLastScoreAward, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayer, m_fNextScoreTime, FIELD_TIME),
	DEFINE_ARRAY(CBasePlayer, m_hszCheckPoints, FIELD_EHANDLE, MAX_CHECKPOINTS),
	DEFINE_ARRAY(CBasePlayer, m_Stats, FIELD_INTEGER, STAT_NUMELEMENTS),

	DEFINE_ARRAY(CBasePlayer, m_szCurrentMusicTrack, FIELD_CHARACTER, MAX_ENTITY_STRING_LENGTH),
	DEFINE_FIELD(CBasePlayer, m_iCurrentMusicTrackCommand, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iCurrentMusicTrackStartTime, FIELD_TIME),
	//DEFINE_FIELD(CBasePlayer, m_iCurrentMusicTrackCommandIssued, FIELD_SHORT),	// Don't save/restore - this is recomputed

	//DEFINE_FIELD(CBasePlayer, m_iSpawnState, FIELD_SHORT),

	DEFINE_FIELD(CBasePlayer, m_MemOrigin, FIELD_VECTOR),
	DEFINE_FIELD(CBasePlayer, m_MemAngles, FIELD_VECTOR),
	//DEFINE_FIELD(CBasePlayer, m_PickTrace, FIELD_),// cannot save TraceResult because of pointer inside! Also, don't need to save
	//DEFINE_FIELD(CBasePlayer, m_pPickEntity, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayer, m_CoordsRemembered, FIELD_BOOLEAN),
	
	DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),// TODO: FIELD_UINT32 ???
	//DEFINE_FIELD(CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER),	// Don't restore, depends on server message after spawning and only matters in multiplayer

	DEFINE_FIELD(CBasePlayer, m_iUserRights, FIELD_UINT32),// XDM3038c
	//DEFINE_FIELD(CBasePlayer, m_pClientActiveItem, FIELD_CLASSPTR),	// Don't restore, client needs reset

	//DEFINE_FIELD(CBasePlayer, m_iSequenceDeepIdle, FIELD_INTEGER),	// Don't restore, reset in Precache()
	DEFINE_ARRAY(CBasePlayer, m_szAnimExtention, FIELD_CHARACTER, 32),

	//DEFINE_FIELD(CBasePlayer, m_flNextSBarUpdateTime, FIELD_TIME),
	//DEFINE_FIELD(CBasePlayer, m_flStatusBarDisappearDelay, FIELD_TIME),
	//DEFINE_ARRAY(CBasePlayer, m_StatusBarString, FIELD_CHARACTER, SBAR_STRING_SIZE),
	//DEFINE_ARRAY(CBasePlayer, m_iStatusBarValues, FIELD_SHORT, SBAR_NUMVALUES),

	//DEFINE_FIELD(CBasePlayer, m_iHUDDistortUpdate, FIELD_SHORT),		// Don't restore, client needs reset
	DEFINE_FIELD(CBasePlayer, m_iHUDDistortMode, FIELD_SHORT),
	DEFINE_FIELD(CBasePlayer, m_iHUDDistortValue, FIELD_SHORT),
};

LINK_ENTITY_TO_CLASS(player, CBasePlayer);


//-----------------------------------------------------------------------------
// Purpose: ID's player as such.
// Warning: May be called before Spawn()!
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::Classify(void)
{
	return m_iClass?m_iClass:CLASS_PLAYER;// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: Spawn. Called by ClientPutInServer().
// Warning: Not called on level change!
// Input  : restore - true if restoring from a saved game with FCAP_MUST_SPAWN
//-----------------------------------------------------------------------------
void CBasePlayer::Spawn(byte restore)
{
	if (restore == FALSE)
	{
		size_t i = 0;
		for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
			m_rgpWeapons[i] = NULL;

		for (i = 0; i < MAX_ITEMS; ++i)// XDM3034
			m_rgItems[i] = 0;

		m_rgItems[ITEM_ANTIDOTE] = DEFAULT_ANTIDOTE_GIVE;
		m_rgItems[ITEM_FLARE] = DEFAULT_FLARE_GIVE;

		m_flBurnTime = 0.0f;// XDM3037
		m_hTBDAttacker = NULL;// XDM3037

		m_fFrozen = FALSE;// XDM3035
		m_voicePitch = PITCH_NORM;

		m_afButtonLast = 0;// XDM3038a
		m_afButtonPressed = 0;// XDM3038a
		m_afButtonReleased = 0;// XDM3038a
		m_fInitHUD = TRUE;// XDM3038c: only init if not restoring or changing map

		if (m_iSpawnState <= SPAWNSTATE_CONNECTED)// XDM3038c: player is conecting, start playing default map list
		{
			m_szCurrentMusicTrack[0] = '\0';// play map list
			m_iCurrentMusicTrackStartTime = gpGlobals->time + 4.0;// should be enough to let the world override command before it gets sent
			m_iCurrentMusicTrackCommand = MPSVCMD_PLAYFILELOOP;
			m_iCurrentMusicTrackCommandIssued = 0;
		}

		m_iHUDDistortMode = 0;// XDM3038
		m_iHUDDistortValue = 0;// XDM3038

		UTIL_SetView(edict(), edict());// XDM3035b: restore after possible SET_VIEW (like trigger_camera on previous level)

		pev->velocity.Clear();// XDM3038c
		pev->basevelocity.Clear();// XDM3038c
		pev->angles.Clear();// XDM3038c
		pev->avelocity.Clear();// XDM3038c
		pev->scale = 1.0;// XDM3038a
		pev->weapons = 0;// XDM3038a
		pev->button = 0;// XDM3038a
		//FAIL! pev->groupinfo = (1 << entindex());// XDM3038c: to disable collisions with this player, set entity's groupinfo to this player index and UTIL_SetGroupTrace(, GROUP_OP_AND). DO NOT use just entindex because of bitwise operators

		if (g_pWorld)// XDM3038c
		{
			if (FBitSet(g_pWorld->pev->spawnflags, SF_WORLD_STARTSUIT))
				pev->weapons |= (1<<WEAPON_SUIT);
		}
	}
	Spawn();
	//FlashlightUpdate(FlashlightIsOn());
}

//-----------------------------------------------------------------------------
// Purpose: Spawn, called by Spawn(bool restore)
// Note   : Arrange variables in cache-friendly manner
//-----------------------------------------------------------------------------
void CBasePlayer::Spawn(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::Spawn()\n", entindex());
	SetClassName("player");
	ASSERT(edict()->serialnumber != 0);// XDM3038c: !!!
	pev->targetname = pev->classname;// XDM3037: TESTME: is this ok?
	pev->max_health = g_pGameRules?g_pGameRules->GetPlayerMaxHealth():MAX_PLAYER_HEALTH;//100.0f;
	if (IsMultiplayer())// XDM3038a: setup initial values
	{
		pev->health = min(MAX_ABS_PLAYER_HEALTH, mp_playerstarthealth.value>0?mp_playerstarthealth.value:pev->max_health);
		pev->armorvalue = min(MAX_ABS_PLAYER_ARMOR, mp_playerstartarmor.value);
	}
	else
	{
		if (pev->health <= 0.0f)// player can't spawn dead
			pev->health = pev->max_health;

		//pev->armorvalue = 0;
	}
	pev->deadflag = DEAD_NO;
	pev->owner = NULL;// XDM3035
	SetBits(pev->flags, FL_CLIENT);
	pev->dmg_take = 0.0f;
	pev->dmg_save = 0.0f;
	pev->maxspeed = g_psv_maxspeed->value;// XDM3035c
	pev->fov = 0.0f;// for client side this sould mean "reset to default"

	m_flFieldOfView		= 0.5f;// some monsters use this to determine whether or not the player is looking at them.
	m_bitsDamageType	= 0;
	m_iScoreAward		= gSkillData.plrScore;// XDM3038c: for CoOp, used for friendly fire penalty

	m_afPhysicsFlags	= 0;
	m_fLongJump			= FALSE;// no longjump module.
	m_flLastSpawnTime	= gpGlobals->time;

	if (m_iSpawnState <= SPAWNSTATE_CONNECTED)// XDM3038: is this reliable enough?
		CheckPointsClear();

	bool startfailed = false;

	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f && g_psv_cheats && g_psv_cheats->value > 0.0f)
		RightsAssign(USER_RIGHTS_DEVELOPER);// XDM3038c

	if (g_pGameRules)// XDM3038a: early initialization
	{
		if (!g_pGameRules->PlayerSpawnStart(this))// allowed to spawn? assign team, required to use spawn spot later
			startfailed = true;

		if (!startfailed)
		{
			if (!g_pGameRules->PlayerUseSpawnSpot(this, false))// team must be set by now!
			{
				ClientPrint(pev, HUD_PRINTCENTER, "#PL_NOSSTARTSPOT");
				startfailed = true;
			}
		}

		if (startfailed)
		{
			g_pGameRules->PlayerUseSpawnSpot(this, true);// try using spectator spot
			if (g_pGameRules->IsMultiplayer())// continue anyway in SP
			{
				ObserverStart(pev->origin, pev->angles, OBS_ROAMING, NULL);
				goto spawn_end;
			}
		}
	}
	RightsAssign(USER_RIGHTS_PLAYER);// XDM3038c: not for spectators

	pev->movetype		= MOVETYPE_WALK;
	pev->solid			= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;
	pev->dmg			= 2;// initial water damage
	pev->air_finished	= gpGlobals->time + 12.0f;

	{
	char *infobuffer = GET_INFO_KEY_BUFFER(edict());
	if (pev->skin == 0 && infobuffer)
		pev->skin = atoi(GET_INFO_KEY_VALUE(infobuffer, "skin"));// XDM
	}

	// XDM3037: players aren't created by DispatchSpawn(), so handle these separately
	if (g_pWorld)// HACK: this global multiplier should be used by physics code, but we don't have access to entity physics
	{
		pev->gravity	= g_pWorld->pev->gravity;
		pev->friction	= g_pWorld->pev->friction;// XDM3035c: use map global multipliers
	}
	pev->rendermode		= kRenderNormal;// XDM3035
	pev->renderamt		= 255;// XDM3037a
	pev->rendercolor.Set(255,255,255);// XDM3037a
	pev->renderfx		= kRenderFxNone;// XDM

	m_bitsHUDDamage		= -1;
	if (FBitSet(m_iGameFlags, EGF_DIED))
		SetBits(m_iGameFlags, EGF_RESPAWNED);// XDM3038c

	SetBits(m_iGameFlags, EGF_SPAWNED);// XDM3038c
	ClearBits(m_iGameFlags, EGF_DROPPEDITEMS);// XDM3038c
	ENGINE_SETPHYSKV(edict(), PHYSKEY_LONGJUMP, "0");
	ENGINE_SETPHYSKV(edict(), PHYSKEY_DEFAULT, "1");
	ENGINE_SETPHYSKV(edict(), PHYSKEY_IGNORELADDER, "0");// XDM3037: ignore ladder: off
	//XDM3037a	m_iClientFOV		= -1; // make sure fov reset is sent
	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.
	m_flgeigerDelay		= gpGlobals->time + 2.0; // wait a few seconds until user-defined message registrations are recieved by all clients
	//m_flTimeStepSound	= 0;
	//m_iStepLeft			= 0;
	m_Activity			= ACT_RESET;// XDM3037a
	m_IdealActivity		= ACT_IDLE;// XDM3037a
	m_LastHitGroup		= HITGROUP_GENERIC;// XDM3037
	m_afCapability		= bits_CAP_DUCK | bits_CAP_JUMP | bits_CAP_STRAFE | bits_CAP_SWIM | bits_CAP_CLIMB | bits_CAP_USE | bits_CAP_HEAR | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS | bits_CAP_TURN_HEAD;

	if (m_bloodColor == 0)
		m_bloodColor = BLOOD_COLOR_RED;

	if (m_iGibCount == 0)
		m_iGibCount = GIB_COUNT_HUMAN;// XDM

	m_afMemory			= MEMORY_CLEAR;// XDM3037a: forget everything, like bits_MEMORY_KILLED
	m_flNextAttack		= UTIL_WeaponTimeBase();
	m_flFallVelocity	= 0;// dont let uninitialized value here hurt the player

	m_flFlashLightTime	= gpGlobals->time + FLASHLIGHT_UPDATE_INTERVAL;// force first message
	m_fFlashBattery		= MAX_FLASHLIGHT_CHARGE;//-1; XDM3037: TESTME?
	m_fDiedTime			= 0.0f;// XDM3035
	m_flIgnoreLadderStopTime = 0.0f;// XDM3037
	m_flNextChatTime	= gpGlobals->time;
	m_flThrowNDDTime	= 0.0f;// XDM3035
	m_pClientActiveItem	= NULL;
	m_iHUDDistortUpdate	= 1;// XDM3038

	Precache();

	/*if (sv_modelhitboxes.value > 0.0f)// XDM3035: UNDONE: use custom hitboxes it not afraid of cheaters. Error no precache
	{
		char *bufmdl = GET_INFO_KEY_VALUE(infobuffer, "model");
		char modelpath[32];
		_snprintf(modelpath, 32, "models/player/%s/%s.mdl", bufmdl, bufmdl);
		SET_MODEL(edict(), modelpath);
	}
	else*/
		//SET_MODEL(edict(), "models/player.mdl");
	SET_MODEL(edict(), STRING(pev->model));// XDM3038a

    g_ulModelIndexPlayer= pev->modelindex;
	pev->sequence		= LookupActivity(m_IdealActivity);

	m_HackedGunPos.Set(0,0,VEC_VIEW);// XDM3037: probably doesn't change a thing anyway

	// XDM3038c: m_fInitHUD			= TRUE;
	//m_pTrain			= NULL;// XDM3035
	m_pTank				= NULL;
	m_fNoPlayerSound	= FALSE;// normal sound behavior.
	m_iClientBattery	= -1;
	m_iClientHideHUD	= -1;  // force this to be recalculated
	m_pActiveItem		= NULL;// XDM: TESTED
	m_pLastItem			= NULL;
	m_pNextItem			= NULL;// XDM3038
	m_vecAutoAimPrev.Clear();
	m_pCarryingObject	= NULL;// XDM3033 TESTED
	//m_fWeapon			= FALSE;

	m_iScoreCombo		= 0;// XDM3035
	m_iLastScoreAward	= 0;
	m_fNextScoreTime	= 0.0f;

	// reset all ammo values to 0
	size_t i;
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset (the death hud clear messages does on the client side)
	}
	m_iSequenceDeepIdle	= LookupSequence("deep_idle");// XDM3035b: fasterizer
	m_fGaitAnimFinishTime = 0;// XDM3037

	if (g_pGameRules)
		g_pGameRules->PlayerSpawn(this);

	Materialize();// XDM3038c

	UpdateSoundEnvironment(NULL, 0, 0);// XDM3038a: very old bug many players complained about

	// XDM3038: moved after PlayerSpawn() so it may affect these flags
	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);

	//pev->oldorigin = pev->origin;// XDM3035: possibly reduces interpolation during respawn
	if (!IsObserver())
	{
		pev->view_ofs = VEC_VIEW_OFFSET;
		pev->effects = 0;// stay invisible during respawn
	}
spawn_end:
	m_iSpawnState = SPAWNSTATE_SPAWNED;// MUST be set AFTER calling PlayerSpawn()
}

//-----------------------------------------------------------------------------
// Purpose: Precache resources and reset unsaved data
// Warning: Sounds and models are precached in ClientPrecache() because clients are not created as ordinary entities
//-----------------------------------------------------------------------------
void CBasePlayer::Precache(void)
{
	// XDM: some stuff moved to CWorld

	m_bitsDamageType = 0;
	// init geiger counter vars during spawn and each time we cross a level transition
	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;
	m_bitsHUDDamage = -1;
	m_iTrain = TRAIN_NEW;
	m_iClientBattery = -1;
	//m_iUpdateTime = 5;  // won't update for 1/2 a second
	// XDM3038c: TESTME if (g_ClientShouldInitialize[entindex()])// XDM3037
	//	m_fInitHUD = TRUE;

	if (FStringNull(pev->model))// XDM3038a: test?
		pev->model = MAKE_STRING("models/player.mdl");

	//if (IsOnTrain())// XDM3035c: restore
	//already	m_iTrain |= TRAIN_NEW;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: Player can be used when alive
//-----------------------------------------------------------------------------
int	CBasePlayer::ObjectCaps(void)
{
	if (IsAlive())
		return (CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;// players can use players
	else
		return (CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
}

//-----------------------------------------------------------------------------
// Purpose: Save player data (hl-compatible override)
// Input  : &save - 
// Output : int - 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
int CBasePlayer::Save(CSave &save)
{
	if (!CBaseMonster::Save(save))
		return 0;

	return save.WriteFields("PLAYER", this, m_SaveData, ARRAYSIZE(m_SaveData));
}

//-----------------------------------------------------------------------------
// Purpose: Restore from saved game or after map transition
// Input  : &restore - 
// Output : int - 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
int CBasePlayer::Restore(CRestore &restore)
{
	if (!CBaseMonster::Restore(restore))
		return 0;

	int status = restore.ReadFields("PLAYER", this, m_SaveData, ARRAYSIZE(m_SaveData));
	if (status == 0)
	{
		conprintf(0, "CBasePlayer::Restore() ERROR: ReadFields(\"PLAYER\") failed!\n");
		return 0;
	}
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	ASSERT(pSaveData != NULL);
	// landmark isn't present.
	if (pSaveData && !pSaveData->fUseLandmark)
	{
		conprintf(1, "CBasePlayer::Restore(): error! No landmark: \"%s\"!\n", pSaveData->szLandmarkName);
		// default to normal spawn
		CBaseEntity *pSpawnSpot = SpawnPointEntSelect(this, false);
		if (pSpawnSpot)
		{
			pev->origin = pSpawnSpot->pev->origin;
			pev->origin.z += 1.0f;
			pev->angles = pSpawnSpot->pev->angles;
		}
	}
	pev->v_angle.z = 0;// Clear out roll
	pev->angles = pev->v_angle;
	pev->fixangle = TRUE;// turn this way immediately

	// still no help	m_fInitHUD = TRUE;// XDM3035b: CL_Precache needs this

	// Copied from spawn() for now
	//?	m_bloodColor	= BLOOD_COLOR_RED;
    g_ulModelIndexPlayer = pev->modelindex;

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);

	edict_t *pPlayerEdict = edict();
	ASSERT(pPlayerEdict != NULL);
	ClientAssignSerialNumber(pPlayerEdict);// XDM3038c: this gets reset on map change in sp!

	ENGINE_SETPHYSKV(pPlayerEdict, PHYSKEY_DEFAULT, "1");

	if (m_fLongJump)
		ENGINE_SETPHYSKV(pPlayerEdict, PHYSKEY_LONGJUMP, "1");
	else
		ENGINE_SETPHYSKV(pPlayerEdict, PHYSKEY_LONGJUMP, "0");

#if defined(CLIENT_WEAPONS)
	m_flNextAttack = UTIL_WeaponTimeBase();// in CBaseMonster it is server time
#endif

	m_iCurrentMusicTrackCommandIssued = 0;// XDM3038c: resend last music player command
	//if (IsOnTrain())// XDM3035c: doesn't help - too early
	//	m_iTrain |= TRAIN_NEW;

	return status;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c in case player is deleted, should never be called!
//-----------------------------------------------------------------------------
void CBasePlayer::UpdateOnRemove(void)
{
	DBG_PRINT_ENT("UpdateOnRemove()");
	if (g_ServerActive)
		ClientDisconnect(edict());

	DBG_FORCEBREAK
}

//-----------------------------------------------------------------------------
// Purpose: Returns an integer that describes the relationship between this monster and target.
// Warning: May be called before Spawn()!
// Input  : *pTarget - 
// Output : int R_NO
//-----------------------------------------------------------------------------
int CBasePlayer::IRelationship(CBaseEntity *pTarget)
{
	if (g_pGameRules)
	{
		int relation = g_pGameRules->PlayerRelationship(this, pTarget);
		if (relation == GR_NOTTEAMMATE)
			return R_DL;
		else if (relation == GR_ENEMY)
			return R_HT;
		else if (relation == GR_TEAMMATE || relation == GR_ALLY)
			return R_AL;
		//else // otherwise look into relationship table
	}
	return CBaseMonster::IRelationship(pTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Can be pushed by friendly players
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBasePlayer::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (sv_playerpushable.value <= 0.0f)
		return;
	if (!pOther->IsPlayer())// || !pOther->IsAlive())// XDM3035c: allow players to push each other
		return;
	if (pOther->edict() == pev->groundentity)// Don't send velocity to an entity I'm standing on
		return;

	int relation = g_pGameRules?g_pGameRules->PlayerRelationship(this, pOther):GR_NEUTRAL;
	if (sv_playerpushable.value < 2.0f && (relation == GR_NOTTEAMMATE || relation == GR_ENEMY))// XDM3038
	{
		if (!(/*FBitSet(pOther->pev->flags, FL_ONGROUND) && */pOther->pev->groundentity && pOther->pev->groundentity == edict()))// if someone's standing on me, allow to push him anyway
			return;
	}

	vec_t fLocalSpeed = pev->velocity.Length();
	vec_t fOtherSpeed = pOther->pev->velocity.Length();
	float kEnergy;// how much enegry is transfered
	if (FBitSet(pev->flags, FL_ONGROUND))// I'm on ground
	{
		if (FBitSet(pOther->pev->flags, FL_ONGROUND))
		{
			if (pOther->pev->friction > 0)
				kEnergy = pev->friction/pOther->pev->friction;
			else
				kEnergy = 0.5;// ?
		}
		else// he's in the air
			kEnergy = 1;
	}
	else// I'm not on ground
	{
		if (FBitSet(pOther->pev->flags, FL_ONGROUND))// Bumped into a standing player
			kEnergy = 0.25;
		else
			kEnergy = 0.5;// Two flying players
	}
	pOther->pev->velocity += pev->velocity*kEnergy;// Inverse logic. Not scientific, but works fine with HL
	pev->velocity *= (1-kEnergy);// lose energy
	if (pOther->pev->velocity.Length() > max(fLocalSpeed, fOtherSpeed))
		pOther->pev->velocity.SetLength(max(fLocalSpeed, fOtherSpeed));
}

//-----------------------------------------------------------------------------
// Purpose: Players can 'use' each other. Works for bots and gets attention of players!
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBasePlayer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator->IsPlayer() && pActivator->pev != pev)// using self is really possible
	{
		if (IsBot())// Don't mess with real players entvars
			pev->owner = pActivator->edict();// XBM: bot will react to this change
		else
		{
			// Show direction from which the player was called
			MESSAGE_BEGIN(MSG_ONE, gmsgDamage, NULL, edict());
				WRITE_BYTE(0);// hithgoup
				//WRITE_BYTE(0);// impossible damageTaken
				WRITE_LONG(DMG_NOHITPOINT);// impossible bitsDamage
				WRITE_COORD(pActivator->pev->origin.x);
				WRITE_COORD(pActivator->pev->origin.y);
				WRITE_COORD(pActivator->pev->origin.z);
			MESSAGE_END();
			ClientPrint(pev, HUD_PRINTCENTER, "%s\n", STRING(pActivator->pev->netname));// Print caller's name (or use some localized string maybe)
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For everybody to hear
// UNDONE: TODO: character voice here
//-----------------------------------------------------------------------------
void CBasePlayer::DeathSound(void)
{
	EMIT_GROUPNAME_SUIT(edict(), "HEV_DEAD");
}

//-----------------------------------------------------------------------------
// Purpose: GetGunPosition
// Output : Vector
//-----------------------------------------------------------------------------
Vector CBasePlayer::GetGunPosition(void)
{
	//Vector org, ang;
	//GetAttachment(0, org, ang);
	//UTIL_MakeVectors(pev->v_angle);
	//m_HackedGunPos = pev->view_ofs;
	return pev->origin + pev->view_ofs;// + myForward * 12? undone
}

//-----------------------------------------------------------------------------
// Purpose: TraceAttack
// Input  : *pAttacker - 
//			flDamage - 
//			&vecDir - 
//			*ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CBasePlayer::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	//conprintf(1, "%s(%d)::TraceAttack(%g, tr.hg %d, bits %d)\n", STRING(pev->classname), entindex(), flDamage, ptr->iHitgroup, bitsDamageType);
	if (pev->takedamage == DAMAGE_NO)
		return;

	/* CBaseMonster if (FBitSet(bitsDamageType, DMG_NOHITPOINT))// XDM3037
		m_LastHitGroup = HITGROUP_GENERIC;
	else
		m_LastHitGroup = ptr->iHitgroup;*/

	switch (ptr->iHitgroup)
	{
	case HITGROUP_HEAD:
		{
			flDamage *= gSkillData.plrHead;
			//	if (!m_fFrozen)// XDM: undone: move to client
			//		UTIL_ScreenFade(this, Vector(255,20,0), 1.0f, 0.1f, clamp((int)flDamage, 10, 255), FFADE_IN);
			if (gSkillData.iSkillLevel == SKILL_HARD && m_flNextAttack != 0.0f)// XDM3037
			{
				if (m_flNextAttack > UTIL_WeaponTimeBase())
					m_flNextAttack += 0.5f;
				else
					m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
			}
		}
		break;
	case HITGROUP_CHEST:
		{
			flDamage *= gSkillData.plrChest;
			//if (!m_fFrozen)// XDM
			//	UTIL_ScreenFade(this, Vector(255,80,0), 1.0f, 0.1f, clamp((int)flDamage, 10, 255), FFADE_IN);
		}
		break;
	case HITGROUP_STOMACH:
		{
			flDamage *= gSkillData.plrStomach;
			if (FBitSet(bitsDamageType, DMG_BULLET|DMG_ENERGYBEAM))// XDM3038a
				PunchPitchAxis(-6.0f);
		}
		break;
	case HITGROUP_LEFTARM:// flDamage *= gSkillData.Arm; break;
	case HITGROUP_RIGHTARM:
		{
			flDamage *= gSkillData.plrArm;
			if (FBitSet(bitsDamageType, DMGM_PUNCHVIEW))// XDM3035: punch player backwards
			{
				pev->punchangle.y = clamp(flDamage*0.5f, 0.0f, 10.0f);
				if (ptr->iHitgroup == HITGROUP_RIGHTARM)
					pev->punchangle.y *= -1.0f;
			}
			// idea does not work :( if (gSkillData.iSkillLevel == SKILL_HARD && m_flNextAttack != 0.0f)// XDM3037: you can't fire if shot in hand
			//	m_flNextAttack += 0.5f;
		}
		break;
	case HITGROUP_RIGHTLEG:
	case HITGROUP_LEFTLEG:
		{
			flDamage *= gSkillData.plrLeg;
			if (FBitSet(bitsDamageType, DMGM_PUNCHVIEW))
			{
				PunchPitchAxis(-4.0f);// punch down
				pev->punchangle.z = clamp(flDamage*0.5f, 0.0f, 10.0f);
				if (ptr->iHitgroup == HITGROUP_LEFTLEG)
					pev->punchangle.z *= -1.0f;
			}
		}
		break;
	case HITGROUP_ARMOR:
		{
			if (!FBitSet(bitsDamageType, DMG_IGNOREARMOR))
				flDamage *= (1-ARMOR_TAKE_GENERIC);// XDM3037: not a proper way to calculate damage, but better than nothing
		}
		break;
	}

	/* CBaseMonster if (ptr->iHitgroup == HITGROUP_ARMOR)// XDM3035
	{
		if (g_pGameRules && g_pGameRules->FAllowEffects())
			UTIL_Ricochet(ptr->vecEndPos, flDamage);
	}
	else if (BloodColor() != DONT_BLEED && !FBitSet(bitsDamageType, DMG_DONT_BLEED))// XDM
	{
		UTIL_BloodDrips(ptr->vecEndPos, -vecDir, BloodColor(), (int)flDamage);// a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
	}*/
	CBaseMonster::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);// XDM3038c AddMultiDamage(pAttacker, this, flDamage, bitsDamageType);

	// XDM3038
	int iMaxArmor = (g_pGameRules?g_pGameRules->GetPlayerMaxArmor():MAX_NORMAL_BATTERY);// XDM3038
	if (FBitSet(bitsDamageType, DMGM_HUD_DISTORT) && (pev->armorvalue < (iMaxArmor/2)) && (ptr->iHitgroup == HITGROUP_GENERIC ||
		ptr->iHitgroup == HITGROUP_HEAD || ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH))
	{
		if (FBitSet(bitsDamageType, DMGM_HUDFAIL_COLOR))
			SetBits(m_iHUDDistortMode, HUD_DISTORT_COLOR);

		if (FBitSet(bitsDamageType, DMGM_HUDFAIL_SPR))
			SetBits(m_iHUDDistortMode, HUD_DISTORT_SPRITE);

		if (FBitSet(bitsDamageType, DMGM_HUDFAIL_POS))
			SetBits(m_iHUDDistortMode, HUD_DISTORT_POS);

		if (m_iHUDDistortValue < UCHAR_MAX)
			m_iHUDDistortValue += (short)(flDamage/10.0f);
		else
			m_iHUDDistortValue = UCHAR_MAX;

		m_iHUDDistortUpdate = 1;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Take some damage.
// Warning: Wach call to TakeDamage with bitsDamageType set to a time-based damage type will cause the damage time countdown to be reset.
// Warning: Thus the ongoing effects of poison, radiation etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	DBG_PRINT_ENT_TAKEDAMAGE;
	if (pev->movetype == MOVETYPE_NOCLIP && pev->solid == SOLID_NOT)// XDM3035: disintegration
		return 0;

	if (FBitSet(pev->effects, EF_NODRAW))// XDM3035: gibbed or respawning or smth
		return 0;

	DBG_PLR_PRINT("CBasePlayer(%d)::TakeDamage(%g): health was %g of %g\n", entindex(), flDamage, pev->health, pev->max_health);

	// Warning: this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	// XDM: moved inside
	int fTookDamage = CBaseMonster::TakeDamage(pInflictor, pAttacker, /*(int)*/flDamage, bitsDamageType);
	if (fTookDamage > 0)
		m_lastDamageAmount = (int)flDamage;

	DBG_PLR_PRINT("CBasePlayer()::TakeDamage(): exit CBaseMonster::TakeDamage()\n");
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	float flHealthPrev = pev->health;

	// Reset damage time countdown for each type of time based damage player just sustained
	{
		for (uint32 i = 0; i < CDMG_TIMEBASED; ++i)
			if (FBitSet(bitsDamageType, DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	if (fTookDamage == 0 || !IsAlive())// XDM3037
		return fTookDamage;

	// Tell director about it
	if (pInflictor)
	{
		if (IsMultiplayer())// XDM3035a: causes buffer overflow in SP
		{
		MESSAGE_BEGIN(MSG_SPEC, svc_director);
			WRITE_BYTE(9);// command length in bytes
			WRITE_BYTE(DRC_CMD_EVENT);// take damage event
			WRITE_SHORT(entindex());// index number of primary entity
			WRITE_SHORT(pInflictor->entindex());	// index number of secondary entity
			WRITE_LONG(5);// eventflags (priority and flags)
		MESSAGE_END();
		}
	}

	bool ffound = true;
	bool ftrivial = (pev->health > (pev->max_health * HEALTH_PERCENT_TRIVIAL) || m_lastDamageAmount < 5);// XDM3037: use pev->max_health
	bool fmajor = (m_lastDamageAmount > (pev->max_health * HEAVY_DAMAGE_PERCENT));
	bool fcritical = (pev->health <= (pev->max_health * HEALTH_PERCENT_CRITICAL));

	// Handle all bits set in this damage message, let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )
	// UNDONE: still need to record damage and heal messages for the following types: DMG_BURN DMG_FREEZE DMG_BLAST DMG_SHOCK

	SetBits(m_bitsDamageType, bitsDamage); // Save this so we can report it to the client
	m_bitsHUDDamage = -1;// make sure the damage bits get resent

	if (FBitSet(bitsDamageType, DMGM_LADDERFALL) && (flDamage > 10) && IsOnLadder())// && fTookDamage)// XDM3037: make the player fall from ladder
		DisableLadder(1.0f);

	//if (FBitSet(bitsDamageType, DMG_IGNITE) && g_pGameRules && g_pGameRules->FAllowEffects())// XDM: client
	//	UTIL_ScreenFade(this, Vector(255,120,10), 1.0, 0.1, 160, FFADE_MODULATE);

	DBG_PLR_PRINT("CBasePlayer()::TakeDamage(): start sorting damage types\n");
	while (fTookDamage && (!ftrivial || FBitSet(bitsDamage, DMGM_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = false;
		if (FBitSet(bitsDamage, DMG_CLUB))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture

			if (gSkillData.iSkillLevel == SKILL_HARD)
				pev->punchangle += RandomVector(2.0f);// XDM3035c

			ClearBits(bitsDamage, DMG_CLUB);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_FALL|DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC);// major fracture
			else
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);// minor fracture

			ClearBits(bitsDamage, DMG_FALL | DMG_CRUSH);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_BULLET))
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC);// blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);// minor laceration

			ClearBits(bitsDamage, DMG_BULLET);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_SLASH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC);	// major laceration
			else
				SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration

			ClearBits(bitsDamage, DMG_SLASH);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_SONIC))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN);	// internal bleeding

			ClearBits(bitsDamage, DMG_SONIC);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_POISON | DMG_PARALYZE))
		{
			/* XDM3037: client	if (!m_fFrozen)
				UTIL_ScreenFade(this, Vector(0,255,0), 1.0, 0.2, (int)flDamage + 20, FFADE_IN);*/
			SetSuitUpdate("!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN);	// blood toxins detected
			ClearBits(bitsDamage, DMG_POISON | DMG_PARALYZE);
			ffound = TRUE;
		}

		if (FBitSet(bitsDamage, DMG_ACID))
		{
			SetSuitUpdate("!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN);	// hazardous chemicals detected
			ClearBits(bitsDamage, DMG_ACID);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_NERVEGAS))
		{
			SetSuitUpdate("!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN);	// biohazard detected
			ClearBits(bitsDamage, DMG_NERVEGAS);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_RADIATION))
		{
			SetSuitUpdate("!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN);	// radiation detected
			ClearBits(bitsDamage, DMG_RADIATION);
			ffound = true;
		}

		/* XDM3037: client	if (FBitSet(bitsDamage, DMG_DROWN))
		{
			if (!m_fFrozen)
				UTIL_ScreenFade(this, Vector(200,200,200), 1.0, 0.2, 120, FFADE_IN);
		}*/

		if (FBitSet(bitsDamage, DMG_SHOCK))
		{
			SetSuitUpdate("!HEV_SHOCK", FALSE, SUIT_NEXT_IN_30SEC);	// XDM: electric
			ClearBits(bitsDamage, DMG_SHOCK);
			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_BURN))// XDM: fire
		{
			SetSuitUpdate("!HEV_FIRE", FALSE, SUIT_NEXT_IN_1MIN);
			ClearBits(bitsDamage, DMG_BURN);
			if (m_fFrozen)
				FrozenEnd();

			ffound = true;
		}

		if (FBitSet(bitsDamage, DMG_SLOWBURN))// XDM: fire
		{
			ClearBits(bitsDamage, DMG_SLOWBURN);
			if (m_fFrozen)
				FrozenEnd();

			ffound = true;
		}
	}

	if (fTookDamage)
	{
		if (flHealthPrev >= (HEALTH_PERCENT_TRIVIAL*pev->max_health))
		{
			if (!ftrivial)
			{
				// first time we take major damage...
				SetSuitUpdate("!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN);// automedic on
				// give morphine shot if not given recently
				if (fmajor)
					SetSuitUpdate("!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN);// morphine shot
			}
		}
		else// if (!ftrivial && fcritical && flHealthPrev < 75)
		{
			if (!ftrivial && fcritical)
			{
				// already took major damage, now it's critical...
				if (pev->health < (pev->max_health * HEALTH_PERCENT_NEARDEATH))
					SetSuitUpdate("!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN);	// near death
				else if (pev->health < (pev->max_health * HEALTH_PERCENT_CRITICAL))
					SetSuitUpdate("!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN);	// health critical

				// give critical health warnings
				if (!RANDOM_LONG(0,3) && (flHealthPrev < pev->max_health/2))
					SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
			}
			// if we're taking time based damage, warn about its continuing effects
			if (FBitSet(bitsDamageType, DMGM_TIMEBASED))
			{
				if (flHealthPrev < pev->max_health/2)
					SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
				else
					SetSuitUpdate("!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN);	// health dropping
			}
		}
	}
	return fTookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Killed
// Input  : *pInflictor - 
//			*pAttacker - 
//			iGib - 
//-----------------------------------------------------------------------------
void CBasePlayer::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::Killed(%d %s, %d %s, %d)\n", entindex(), pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"", pAttacker?pAttacker->entindex():0, pAttacker?STRING(pAttacker->pev->netname):"", iGib);
	SetBits(m_iGameFlags, EGF_DIED);// XDM3038c
	if (pev->deadflag == DEAD_NO)// XDM3035: player may get 'killed' second time like dead monsters
	{
		if (pev->health > 0)// XDM3037a: ?!
		{
			//DBG_FORCEBREAK// XDM3037a: TEST
			pev->health = 0;
		}
		SetAnimation(PLAYER_DIE);// Call this while pev->deadflag is 0!
		pev->gaitsequence = GAITSEQUENCE_DISABLED;// XDM3037: don't move legs
		pev->deadflag = DEAD_DYING;// set here so weapons may detect it
		pev->button = 0;// XDM3037a
		//m_bReadyPressed = false;// XDM3037a
		m_fDiedTime = gpGlobals->time;// XDM3038c

		// Holster weapon immediately, to allow it to cleanup
		if (m_pActiveItem)// && m_pActiveItem->GetHost())// XDM3035a: somehow glock with m_pPlayer == NULL got here
		{
			m_pActiveItem->Holster();// some weapons may trigger fire when put away
			m_pActiveItem->SetThinkNull();// XDM3035b
			m_pActiveItem->DontThink();// XDM3038a
			// NO! PackDeadPlayerItems needs it:	m_pActiveItem = NULL;
			m_pLastItem = NULL;
		}
		m_pNextItem = NULL;// XDM
		m_iDeaths++;// XDM3038c: before GameRules()!

//#if defined (_DEBUG)// now
		//if (pAttacker == this)
		//		DBG_FORCEBREAK;
//#endif
		// MUST be called BEORE removing any items!
		if (g_pGameRules)
			g_pGameRules->PlayerKilled(this, pAttacker, pInflictor);

		if (m_pTank != NULL)
		{
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}

		TrainDetach();

		if (m_pCarryingObject)
		{
			m_pCarryingObject->Use(this, this, USE_TOGGLE/*COU_DROP*/, 0.0);
			m_pCarryingObject = NULL;
		}

		// this client isn't going to be thinking for a while, so reset the sound until they respawn
		CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
		{
			if (pSound)
				pSound->Reset();
		}
		m_iRespawnFrames = 0;
		pev->modelindex = g_ulModelIndexPlayer;// don't use eyes
		pev->movetype = MOVETYPE_TOSS;
		pev->friction *= 0.5f;// XDM3038a
		ClearBits(pev->flags, FL_ONGROUND);
		Remember(bits_MEMORY_KILLED);// XDM3037
		if (pev->velocity.z < 10)
			pev->velocity.z += min(10+fabs(pev->health)*2,300);// XDM3038a: was RANDOM_FLOAT(0, 300);

		SetSuitUpdate(NULL, FALSE, 0);// clear out the suit message cache so we don't keep chattering

		/* XDM3037: UpdateClientData(clientdata_s) takes care of it now
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, edict());
			WRITE_BYTE(0);
			WRITE_BYTE(0XFF);
			WRITE_BYTE(0xFF);
		MESSAGE_END();*/
		/* XDM3037a: obsolete	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, edict());
			WRITE_BYTE(0);
		MESSAGE_END();*/
		pev->fov/*//XDM3037a = m_iFOV*/ = 0.0f;// reset FOV

		EnableControl(true);// XDM3037: FIX: players died with controls disabled can not respawn!
	}//pev->deadflag == DEAD_NO

	if (FBitSet(pev->effects, EF_NODRAW))// XDM3035: gibbed or respawning or smth (should never get here)
		return;

	// *** ONLY EFFECTS AFTER THIS LINE! No logic and deadflags! ***
	CSoundEnt::InsertSound(bits_SOUND_DEATH, pev->origin, 256, 1.0f);// XDM3035c

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );
	if (iGib == GIB_DISINTEGRATE)
	{
		pev->movetype = MOVETYPE_NOCLIP;
		pev->solid = SOLID_NOT;
		SetBits(pev->flags, FL_FLY);
		pev->velocity.Set(0.0f,0.0f,4.0f);
		pev->origin.z += 0.5f;// HACK
		pev->avelocity.Set(RANDOM_FLOAT(-2.0, 2.0), RANDOM_FLOAT(-2.0, 2.0), RANDOM_FLOAT(-4.0, 4.0));
		SetBits(pev->effects, EF_MUZZLEFLASH);
		pev->rendermode = kRenderTransTexture;
		pev->rendercolor.Set(127,127,127);
		pev->renderamt = 160;
		pev->framerate *= 0.25f;
		pev->gravity = 0.0f;
		pev->takedamage = DAMAGE_NO;
		if (g_pGameRules && g_pGameRules->FAllowEffects())// Disintegration effect allowed?
		{
			ParticleBurst(pev->origin, 20, 5, 10);// Center() is not necessary for players
			UTIL_ScreenShakeOne(this, pev->origin, 10.0f, 0.5f, 3.0f);
			pev->punchangle = pev->avelocity * 20.0f;//Set(RANDOM_FLOAT(-48,48), RANDOM_FLOAT(-48,48), RANDOM_FLOAT(-48,48));
		}
		UTIL_ScreenFade(this, g_vecZero, 1.5f, mp_respawntime.value + 1.0f, 255, FFADE_OUT | FFADE_MODULATE | FFADE_STAYOUT);// XDM3035 :3

		// this shit does nothing SetThink(&CBasePlayer::PlayerDeathThink);
		// DON'T Disintegrate(); players!! Emulate!
	}
	else if (iGib == GIB_FADE)// XDM3037: UNDONE
	{
		pev->rendermode = kRenderTransTexture;
		pev->rendercolor.Set(127,127,127);
		pev->renderamt = 127;
	}
	else if (iGib == GIB_REMOVE)// XDM3037
	{
		SetBits(pev->effects, EF_NODRAW|EF_NOINTERP);
		pev->renderamt = 0;
	}
	else if (ShouldGibMonster(iGib))// XDM3034
	{
		//pev->modelindex = 0;// XDM3038a: don't mess with model indexes
		pev->avelocity = RandomVector(RANDOM_FLOAT(-320,320), RANDOM_FLOAT(-320,320), RANDOM_FLOAT(-160,160));// XDM
		pev->solid = SOLID_NOT;
		if (pAttacker && pAttacker->IsPlayer())
			((CBasePlayer *)pAttacker)->m_Stats[STAT_GIB_COUNT]++;// XDM3038c

		if (GibMonster())// This clears pev->model
		{
			CGib *pHeadGib = CGib::SpawnHeadGib(this);
			if (pHeadGib)
			{
				pHeadGib->pev->owner = edict();
				UTIL_SetView(edict(), pHeadGib->edict());// this is potentially dangerous, the gib must return view if removed
				pev->view_ofs.Clear();// Set(0,0,8);
			}
		}
		//SET_MODEL(edict(), g_szDefaultGibsHuman);// this doesn't work
		//pev->movetype = MOVETYPE_TOSS;
		SetBits(pev->effects, EF_NODRAW|EF_NOINTERP);

		if (mp_playerexplode.value > 0)// XDM3034: players carrying fuel tanks will explode! :D
		{
			if (m_pActiveItem && m_pActiveItem->pszAmmo1())// check if pszAmmo1 exists (!= NULL) before comparing!
			{
				if (strcmp(m_pActiveItem->pszAmmo1(), "fuel") == 0)// XDM3038a: full check to prevent name collisions!
				{
					CBasePlayerWeapon *pWeapon = m_pActiveItem->GetWeaponPtr();
					if (pWeapon && pWeapon->PrimaryAmmoIndex() >= 0 && pWeapon->HasAmmo(AMMO_PRIMARY))
					{
						pev->takedamage = DAMAGE_NO;// ! stack overflow prevention
						ExplosionCreate(pev->origin, pev->angles, NULL, this, 2.5f*AmmoInventory(pWeapon->PrimaryAmmoIndex()), SF_NOSPARKS|SF_NOPARTICLES, 0.0f);
						m_rgAmmo[pWeapon->PrimaryAmmoIndex()] = 0;// XDM3038c
					}
				}
			}
		}
	}
	else if (!HasMemory(bits_MEMORY_KILLED))// XDM3037: don't play DeathSound() everytime this dead body is hit
	{
		DeathSound();
		SetBits(pev->flags, FL_FLOAT);// XDM3037a
		// XDM3035	pev->angles.x = 0;
		//pev->angles.z = 0;
		// this shit does nothing SetThink(&CBasePlayer::PlayerDeathThink);
		if (FlashlightIsOn() && IsMultiplayer())// XDM3037a: in singleplayer it's fine
			FlashlightTurnOff();
	}
	SetNextThink(0.1f);// does nothing on players
}

//-----------------------------------------------------------------------------
// Purpose: Get position where to shoot at (for enemies)
// Note   : Called by PostThink() and also may get called by some events
// Input  : posSrc - 
// Output : 
//-----------------------------------------------------------------------------
Vector CBasePlayer::BodyTarget(const Vector &posSrc)
{
	Vector vecTarget(Center());// don't static this
	vecTarget += pev->view_ofs * RANDOM_FLOAT(0.5f, 1.0f);
	return vecTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Set the activity based on an event or current state
// Note   : Called by PostThink() and also may get called by some events
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetAnimation(const int &iNewPlayerAnim)
{
	if (pev->deadflag != DEAD_NO)
		return;

	float speed2D;
	int playerAnim = iNewPlayerAnim;
	int animDesired = 0;
	char szAnim[32];// XDM3037: mstudioseqdesc_t has 32 anyway
	bool bIsJumping;
	if (FBitSet(pev->flags, FL_FROZEN))
	{
		speed2D = 0;
		bIsJumping = false;
		playerAnim = PLAYER_IDLE;
	}
	else
	{
		speed2D = pev->velocity.Length2D();// 2D is required!
		bIsJumping = !FBitSet(pev->flags, FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) && (m_fGaitAnimFinishTime > gpGlobals->time);
	}
	//else if (m_Activity == ACT_RANGE_ATTACK1)
	//	goto skipplayeranim;
	//conprintf(1, "CBasePlayer::SetAnimation(%d) BEGIN speed %g, m_Activity %d, bIsJumping %d\n", playerAnim, speed2D, m_Activity, bIsJumping?1:0);

	if (m_fSequenceFinished && m_Activity == ACT_RANGE_ATTACK1)
		m_Activity = ACT_RESET;// finished shooting, unblock other animaitons

	switch (playerAnim)
	{
	case PLAYER_JUMP:
		{
			m_IdealActivity = ACT_HOP;
			m_movementActivity = m_IdealActivity;
			bIsJumping = true;
		}
		break;
	case PLAYER_SUPERJUMP:
		{
			m_IdealActivity = ACT_LEAP;
			m_movementActivity = m_IdealActivity;
			bIsJumping = true;
		}
		break;
	case PLAYER_DIE:
		{
			//no!	m_IdealActivity = ACT_DIESIMPLE;
			m_IdealActivity = GetDeathActivity();
			m_movementActivity = ACT_RESET;
		}
		break;
	case PLAYER_ATTACK1:
		{
			switch (m_Activity)// don't interrupt some of higher priority animations
			{
			//case ACT_HOVER:
			case ACT_SWIM:
			//case ACT_HOP:
			case ACT_LEAP:
			//this may be a new attack with a different weapon		case ACT_RANGE_ATTACK1:
			case ACT_DIESIMPLE:
			case ACT_DIEBACKWARD:
			case ACT_DIEFORWARD:
			case ACT_DIEVIOLENT:
			case ACT_DIE_HEADSHOT:
			case ACT_DIE_CHESTSHOT:
			case ACT_DIE_GUTSHOT:
			case ACT_DIE_BACKSHOT:
				m_IdealActivity = m_Activity;
				//conprintf(1, "              SetAnimation(PLAYER_ATTACK1) IGNORE %d\n", RANDOM_LONG(1000,9999));
				break;
			default:
				m_IdealActivity = ACT_RANGE_ATTACK1;
				//conprintf(1, "              SetAnimation(PLAYER_ATTACK1) ACCEPT %d\n", RANDOM_LONG(1000,9999));
				break;
			}
			// this is the place where we do not touch m_movementActivity
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		{
			if (bIsJumping)// Still jumping
			{
				m_IdealActivity = m_Activity;
			}
			else if (!FBitSet(pev->flags, FL_ONGROUND) && pev->waterlevel > WATERLEVEL_NONE)
			{
				if (speed2D <= PLAYER_MAX_WALK_SPEED/20)// XDM3037: must be synchronized with PM_WaterMove()!
					m_IdealActivity = ACT_HOVER;
				else
					m_IdealActivity = ACT_SWIM;
			}
			else
			{
				if (speed2D > PLAYER_MAX_STAND_SPEED)
					m_IdealActivity = ACT_WALK;
				else
					m_IdealActivity = ACT_IDLE;
			}
			m_movementActivity = m_IdealActivity;
		}
		break;
	case PLAYER_ARM:
		m_IdealActivity = ACT_ARM;// XDM: draw animations
		break;
	case PLAYER_DISARM:
		m_IdealActivity = ACT_DISARM;// XDM: holster animations
		break;
	case PLAYER_RELOAD:
		m_IdealActivity = ACT_RELOAD;// XDM: reload animations
		break;
	case PLAYER_CLIMB:
		{
			m_IdealActivity = ACT_WALK;// XDM: ladder climb animations replacement
			m_movementActivity = m_IdealActivity;
		}
		break;
	case PLAYER_FALL:// XDM: fall animations
		{
			if (pev->waterlevel > WATERLEVEL_FEET)
				m_movementActivity = ACT_HOVER;
			else if (bIsJumping)// Still jumping
				m_movementActivity = m_Activity;
			else if (FBitSet(m_afButtonPressed, IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT))
				m_movementActivity = ACT_RUN;
			else
				m_IdealActivity = ACT_HOVER;// UNDONE: ACT_FALL; there's no such animaiton :(

			//m_movementActivity = m_IdealActivity;
		}
		break;
	}

	//conprintf(1, "              SetAnimation(%d) m_IdealActivity %d\n", playerAnim, m_IdealActivity);
//skipplayeranim:

	bool bForceRestart = false;
	switch (m_IdealActivity)
	{
	case ACT_RANGE_ATTACK1:// player fires a weapon
		{
			if (FBitSet(pev->flags, FL_DUCKING))
				strcpy(szAnim, "crouch_shoot_");
			else
				strcpy(szAnim, "ref_shoot_");

			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);
			m_Activity = m_IdealActivity;// even if animation fails?
			bForceRestart = true;
		}
		break;
	case ACT_DIESIMPLE:
	case ACT_DIEBACKWARD:
	case ACT_DIEFORWARD:
	case ACT_DIEVIOLENT:
	case ACT_DIE_HEADSHOT:
	case ACT_DIE_CHESTSHOT:
	case ACT_DIE_GUTSHOT:
	case ACT_DIE_BACKSHOT:
		{
			if (m_Activity != m_IdealActivity)
			{
				animDesired = LookupActivity(m_IdealActivity);
				m_Activity = m_IdealActivity;
			}
		}
		break;
	default:
		/*{
			if (m_Activity != m_IdealActivity || m_fSequenceFinished)
				animDesired = LookupActivity(m_Activity);
			else
				animDesired = pev->sequence;// don't re-pick sequence for same activity!

			m_Activity = m_IdealActivity;
		}
		break;
	case ACT_IDLE:
	case ACT_WALK:*/
		{
			if (m_Activity == ACT_RESET || ((m_fSequenceFinished || bIsJumping) && m_Activity != ACT_RANGE_ATTACK1))// if we're NOT shooting and previous sequence is finished
			{
				if (pev->weaponmodel > 0 || (m_pActiveItem && !m_pActiveItem->IsHolstered()) || m_pNextItem)// this decides. Don't start idle animations while switching weapon
				{
					if (FBitSet(pev->flags, FL_DUCKING))
						strcpy(szAnim, "crouch_aim_");
					else
						strcpy(szAnim, "ref_aim_");

					strcat(szAnim, m_szAnimExtention);
					animDesired = LookupSequence(szAnim);// we don't use LookupActivity() here
				}
				else if (m_IdealActivity != m_Activity || m_fSequenceFinished)// activity changed OR last activity just finished playing
				{
					if (m_IdealActivity == ACT_IDLE && (m_pActiveItem && m_pActiveItem->GetWeaponPtr() && (UTIL_WeaponTimeBase() - m_pActiveItem->GetWeaponPtr()->m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)))//&& RANDOM_LONG(0,3) == 0)
						animDesired = m_iSequenceDeepIdle;
					else
						animDesired = LookupActivity(m_IdealActivity);// update or renew animaiton
				}

				m_Activity = m_IdealActivity;
			}
			else
				animDesired = pev->sequence;// keep old upper body animation
		}
		break;
	}

	//conprintf(1, "              SetAnimation(%d) animDesired %d\n", playerAnim, animDesired);

	//ASSERT(animDesired != ACTIVITY_NOT_AVAILABLE);
	if (animDesired == ACTIVITY_NOT_AVAILABLE)
		animDesired = 0;

	if (pev->sequence != animDesired)// Already using the desired animation?
	{
		if (!m_fSequenceLoops)
			SetBits(pev->effects, EF_NOINTERP);

		pev->sequence = animDesired;
		pev->frame = 0;// Reset to first frame of desired animation
		ResetSequenceInfo();
		//conprintf(1, "              SetAnimation(%d) animDesired %d ResetSequenceInfo()\n", playerAnim, animDesired);
	}

	if (!m_fSequenceLoops && (bForceRestart || m_fSequenceFinished))
		pev->frame = 0;

//skipsequence:
	//conprintf(1, "           ::SetAnimation(%d) pev->sequence %d pev->gaitsequence %d END\n", playerAnim, pev->sequence, pev->gaitsequence);
}

//-----------------------------------------------------------------------------
// Purpose: Updates gaitsequence according to current situation
// Note   : Called "Set" to match SetAnimation() and not to conflict with Update functions
// Warning: gaitsequence can ONLY be toggled ON/OFF! No control on server side. It is always looped.
// Warning: we can only ASSUME gaitsequence length, so it's a barely reliable HACK
// Warning: XDM3037 allows gaitsequence 0! OFF is -1!
//-----------------------------------------------------------------------------
void CBasePlayer::SetGaitAnimation(void)
{
	if (pev->deadflag != DEAD_NO)
		return;

	// A little info: gaitsequence is played on client side, it is always looped (forced), it cannot be controlled on server (on/off only).
	float speed2D;
	bool bIsJumping;
	bool bMoveControls = FBitSet(pev->button, (IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT));
	//bool bDuckControls = FBitSet(pev->button, IN_DUCK);
	int oldgaitsequence = pev->gaitsequence;
	if (FBitSet(pev->flags, FL_FROZEN))
	{
		speed2D = 0;
		bIsJumping = false;
		pev->gaitsequence = m_iSequenceDeepIdle;
	}
	else
	{
		speed2D = pev->velocity.Length2D();// 2D is required!
		//bIsJumping = !FBitSet(pev->flags, FL_ONGROUND) && (m_movementActivity == ACT_HOP || m_movementActivity == ACT_LEAP);
		bIsJumping = !FBitSet(pev->flags, FL_ONGROUND) && (m_movementActivity == ACT_HOP || m_movementActivity == ACT_LEAP) && (m_fGaitAnimFinishTime > gpGlobals->time);

		if (FBitSet(pev->flags, FL_DUCKING))
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST)
				pev->gaitsequence = LookupActivity(ACT_SWIM);// animation with horizontal body orientation
			else if (bIsJumping && m_flFallVelocity < 20)// lots of hacks here
				pev->gaitsequence = LookupActivity(m_movementActivity);// lower body will do jumping
			else if (speed2D > PLAYER_MAX_STAND_SPEED || bMoveControls)// even in the air
				pev->gaitsequence = LookupActivity(ACT_CROUCH);// moving while crouching
			else
				pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);// crouching position
		}
		/* wrong!	else if (bDuckControls)// player just pressed CROUCH, but crouching not finished
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST)
				pev->gaitsequence = LookupActivity(ACT_SWIM);
			else
				pev->gaitsequence = LookupActivity(ACT_CROUCH);// move legs
		}*/
		else if (bIsJumping)// HACK: there's NO !m_fSequenceFinished for GAITsequences and will never be, so we just check FL_ONGROUND :(
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST)
				pev->gaitsequence = LookupActivity(ACT_HOVER);
			else
				pev->gaitsequence = LookupActivity(m_movementActivity);//ACT_HOP/ACT_LEAP);
		}
		else if (speed2D > PLAYER_MAX_WALK_SPEED)// run detected
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST)
				pev->gaitsequence = LookupActivity(ACT_SWIM);
			else if (FBitSet(pev->flags, FL_ONGROUND) || bMoveControls)// move legs while onground or indicate attempts to move in the air
				pev->gaitsequence = LookupActivity(ACT_RUN);
			else
				pev->gaitsequence = GAITSEQUENCE_DISABLED;// falling. better ideas?
		}
		else if (speed2D > PLAYER_MAX_STAND_SPEED || bMoveControls)// walk detected
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST)
				pev->gaitsequence = LookupActivity(ACT_HOVER);
			else if (FBitSet(pev->flags, FL_ONGROUND) || bMoveControls)// move legs while onground or indicate attempts to move in the air
				pev->gaitsequence = LookupActivity(ACT_WALK);
			else
				pev->gaitsequence = GAITSEQUENCE_DISABLED;
		}
		else// not moving
		{
			if (pev->waterlevel >= WATERLEVEL_WAIST && !FBitSet(pev->flags, FL_ONGROUND))
				pev->gaitsequence = LookupActivity(ACT_HOVER);
			else
				pev->gaitsequence = pev->sequence;// TESTME: use upper body animaiton
			// acceptable: m_iSequenceDeepIdle;
			// not acceptable: LookupActivity(ACT_IDLE); will pick random animaiton EVERY GAME FRAME!
		}

		if (pev->gaitsequence == ACTIVITY_NOT_AVAILABLE)
			pev->gaitsequence = GAITSEQUENCE_DISABLED;
	}

	if (pev->gaitsequence != oldgaitsequence)
	{
		//conprintf(1, "pev->gaitsequence = %d CHANGED\n", pev->gaitsequence);
		if (pev->gaitsequence != GAITSEQUENCE_DISABLED)// XDM3037: HACK! Assume the gaitsequence length (time when it finishes).
		{
			studiohdr_t *pStudioHdr = (studiohdr_t *)GET_MODEL_PTR(edict());
			if (pStudioHdr)
			{
				mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pStudioHdr + pStudioHdr->seqindex) + pev->gaitsequence;
				if (pseqdesc)// WARNING! gaitsequence fps is not constant!! we can't calculate it here!
					m_fGaitAnimFinishTime = gpGlobals->time + (pseqdesc->numframes / pseqdesc->fps);// +animaiton length
				else
					m_fGaitAnimFinishTime = 0;
			}
			else
				m_fGaitAnimFinishTime = 0;
		}
		else
			m_fGaitAnimFinishTime = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Safe method to set animation extnsion
// Input  : *szExtention - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetWeaponAnimType(const char *szExtention)
{
	if (szExtention)
	{
		strncpy(m_szAnimExtention, szExtention, 31);
		m_szAnimExtention[31] = '\0';
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called at the start of every frame regardless of waterlevel
//-----------------------------------------------------------------------------
void CBasePlayer::WaterMove(void)
{
	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->waterlevel >= WATERLEVEL_WAIST)// XDM3038a
	{
		m_flBurnTime = 0.0f;
		ClearBits(m_bitsDamageType, DMGM_FIRE|DMG_IGNITE);

		if (pev->waterlevel == WATERLEVEL_WAIST)// water circles
		{
			if (g_pGameRules && g_pGameRules->FAllowEffects() && m_fWaterCircleTime < gpGlobals->time)
			{
				BeamEffect(TE_BEAMDISK, pev->origin + Vector(0,0,4), pev->origin + Vector(0,0,56), g_iModelIndexShockWave, 0, 10, 10, 1, 64/*noise*/, Vector(95,95,95), 64, 0);
				m_fWaterCircleTime = gpGlobals->time + 1.0f;
			}
		}
		if (!FBitSet(pev->flags, FL_INWATER))
			SetBits(pev->flags, FL_INWATER);
	}

	if (pev->waterlevel < WATERLEVEL_HEAD)
	{
		// 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(edict(), CHAN_VOICE, "player/pl_wade1.wav", VOL_NORM, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + gSkillData.PlrAirTime*0.5f)
			EMIT_SOUND(edict(), CHAN_VOICE, "player/pl_wade2.wav", VOL_NORM, ATTN_NORM);

		pev->air_finished = gpGlobals->time + gSkillData.PlrAirTime;
		pev->dmg = 2.0f;

		if (m_idrowndmg > m_idrownrestored)// if we took drowning damage, give it back slowly
		{
			// Set drowning damage bit. HACK: dmg_drownrecover actually makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.
			// NOTE: this actually causes the count to continue restarting until all drowning damage is healed.
			ClearBits(m_bitsDamageType, DMG_DROWN);
			SetBits(m_bitsDamageType, DMG_DROWNRECOVER);
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}

		if (pev->waterlevel == WATERLEVEL_NONE)
		{
			if (FBitSet(pev->flags, FL_INWATER))
				ClearBits(pev->flags, FL_INWATER);

			return;
		}
	}
	else// fully under water
	{
		// stop restoring damage while underwater
		ClearBits(m_bitsDamageType, DMG_DROWNRECOVER);
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time)// drown
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;

				TakeDamage(g_pWorld, g_pWorld, pev->dmg, DMG_DROWN);// XDM3034
				pev->pain_finished = gpGlobals->time + 1.0;
				m_idrowndmg += pev->dmg;// track drowning damage to give it back when player finally takes a breath
			}
		}
		else
		{
			ClearBits(m_bitsDamageType, DMG_DROWN);

			if (RANDOM_LONG(0,31) == 0 && RANDOM_LONG(0,gSkillData.PlrAirTime-1) >= (int)(pev->air_finished - gpGlobals->time))
			{
				//FX_BubblesPoint(GetGunPosition(), VECTOR_CONE_45DEGREES, 16); bubbles are in CBaseMonster::TakeDamage()
				switch (RANDOM_LONG(0,3))
				{
					case 0:	EMIT_SOUND(edict(), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
					case 1:	EMIT_SOUND(edict(), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
					case 2:	EMIT_SOUND(edict(), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
					case 3:	EMIT_SOUND(edict(), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles +USE keypress, not for spectator
//-----------------------------------------------------------------------------
void CBasePlayer::PlayerUse(void)
{
	if (!FBitSet(pev->button | m_afButtonPressed | m_afButtonReleased, IN_USE))// was USE pressed, released, changed?
		return;

	if (FBitSet(m_afButtonPressed, IN_USE))
	{
		if (m_pTank != NULL)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}
		else
		{
			if (IsOnTrain())// XDM3035b: +USE on a train
			{
				TrainDetach();// don't stop the train, just disable controls
				return;
			}
			else// Start controlling the train!
			{
				if (!FBitSet(pev->button, IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND))
				{
					CBaseEntity *pTrain = CBaseEntity::Instance(pev->groundentity);
					if (pTrain && FBitSet(pTrain->ObjectCaps(), FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev))
					{
						if (TrainAttach(pTrain))// XDM3035: HACKHACKHACK!!!!!!!!
							EMIT_SOUND(edict(), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);

						return;// we're done here
					}
				}
			}
		}
	}

	int caps = 0;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;
	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector vecLOS;
	Vector vecSrc(GetGunPosition());
	UTIL_MakeVectors(pev->v_angle);// so we know which way we are facing
	// Don't filter out invisible entities!
	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_USE_SEARCH_RADIUS)) != NULL)
	{
		if (pObject == this)// XDM3035
			continue;
		//if (pObject->IsProjectile())// XDM3037: NO! Satchels and mines are usable!
		//	continue;
		if (pObject->IsPlayerItem() && FBitSet(pObject->pev->effects, EF_NODRAW))// XDM3037
			continue;

		caps = pObject->ObjectCaps();
		if (FBitSet(caps, FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE))
		{
			// XDM3037: TODO: move this to a separate PlayerUseCheck() function
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_USE_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (pObject->Center() - vecSrc);// old: VecBModelOrigin

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5f);
			// can't see what is this vector anyway UTIL_DebugBeam(pObject->Center(), pObject->Center() + vecLOS, 5.0);

			// XDM3037: traceline here to prevent USEing buttons through walls
			// XDM3038c: update: some mappers like to use non-solid buttons overlapped by solid entities in which case the trace always fails
			if (FBitSet(caps, FCAP_ONLYDIRECT_USE) && (pObject->pev->solid != SOLID_NOT  && pObject->pev->solid != SOLID_TRIGGER))
			{
				TraceResult tr;
				UTIL_TraceLine(vecSrc, pObject->Center(), dont_ignore_monsters, dont_ignore_glass, edict(), &tr);// UNDONE: trace to object's closest plane, not center // XDM3038a: new: dont_ignore_monsters TESTME!
				//UTIL_DebugBeam(vecSrc, pObject->Center(), 4.0, 0,255,255);
				//UTIL_DebugBeam(vecSrc, tr.vecEndPos, 4.0, 255,0,0);
				if (tr.flFraction < 1.0f && tr.pHit != pObject->edict())
				{
					//CBaseEntity *pEnt = CBaseEntity::Instance(tr.pHit);UTIL_PrintEntInfo(pEnt); for debugging/breakpoints
					continue;
				}
			}

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)// only if the item is in front of the user
			{
				pClosest = pObject;
				flMaxDot = flDot;
			}
		}
	}
	//pObject = pClosest;
	float value;
	if (FBitSet(m_afPhysicsFlags, PFLAG_USING) || pClosest && pClosest->GetState() == STATE_OFF)
		value = 1.0f;
	else
		value = 0.0f;

	int iButtonState = 0;
	if (FBitSet(m_afButtonPressed, IN_USE))
		iButtonState = 1;
	else if (FBitSet(pev->button, IN_USE))
		iButtonState = 2;
	else if (FBitSet(m_afButtonReleased, IN_USE))
		iButtonState = 3;

	if (iButtonState > 0)// only when pressed, held or released
		PlayerUseObject(iButtonState, pClosest, value);// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: may be called externally
// Input  : iButtonState - 0-nothing; 1-pressed once; 2-held; 3-released
//			*pObject - can be NULL!
//			value - Use() value
//-----------------------------------------------------------------------------
void CBasePlayer::PlayerUseObject(const short iButtonState, CBaseEntity *pObject, const float value)
{
	if (pObject)
	{
		if (FBitSet(m_afButtonPressed, IN_USE))// only play sound if called on server
			EMIT_SOUND(edict(), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_IDLE);

		int caps = pObject->ObjectCaps();
		if (iButtonState == 2 && FBitSet(caps, FCAP_CONTINUOUS_USE))
		{
			SetBits(m_afPhysicsFlags, PFLAG_USING);
			pObject->Use(this, this, USE_SET, value);
		}
		else if (iButtonState == 1 && FBitSet(caps, FCAP_IMPULSE_USE|FCAP_ONOFF_USE))
			pObject->Use(this, this, USE_SET, value);
		else if (iButtonState == 3 && FBitSet(caps, FCAP_ONOFF_USE))// This is an "off" use
			pObject->Use(this, this, USE_SET, value);
	}
	else
	{
		if (iButtonState == 1)
			EMIT_SOUND(edict(), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_IDLE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Jump
//-----------------------------------------------------------------------------
void CBasePlayer::Jump(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::Jump()\n", entindex());
	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= WATERLEVEL_WAIST)
		return;

	// jump velocity is sqrt( height * gravity * 2)
	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return;         // don't pogo stick

	if (!FBitSet(pev->flags, FL_ONGROUND))// XDM3038: FAIL on world! || FNullEnt(pev->groundentity))
		return;

	if (UTIL_IsValidEntity(pev->groundentity))// XDM3037
	{
	// many features in this function use v_forward, so makevectors now.
//XDM3038:???	UTIL_MakeVectors(pev->angles);
	// ClearBits(pev->flags, FL_ONGROUND);			// don't stairwalk

	if (sv_jumpaccuracy.value <= 0.0f)// XDM
		PunchPitchAxis(4.0f);

	if (m_fLongJump &&
		FBitSet(pev->button, IN_DUCK) &&
		(pev->flDuckTime > 0) &&
		pev->velocity.Length() > 50)
	{
		SetAnimation(PLAYER_SUPERJUMP);
	}
	else
		SetAnimation(PLAYER_JUMP);

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	if (FBitSet(pev->groundentity->v.flags, FL_CONVEYOR))// XDM3038: I forgot what was that
		pev->velocity += pev->basevelocity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when ducking
//-----------------------------------------------------------------------------
void CBasePlayer::Duck(void)
{
	if (m_IdealActivity != ACT_LEAP)
		SetAnimation(PLAYER_WALK);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: separate function
// Check to see if player landed hard enough to make a sound or do damage.
// Falling farther than half of the maximum safe distance, but not as far a max safe distance will play a bootscrape sound, and no damage will be inflicted.
// Falling a distance shorter than half of maximum safe distance will make no sound.
// Falling farther than max safe distance will play a fallpain sound, and damage will be inflicted based on how far the player fell.
//-----------------------------------------------------------------------------
void CBasePlayer::Land(void)
{
	if (m_movementActivity == ACT_HOP || m_movementActivity == ACT_LEAP)
		m_fGaitAnimFinishTime = gpGlobals->time;

	if (m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// conprintf(1, "%f\n", m_flFallVelocity );
		if (pev->watertype == CONTENTS_WATER && pev->waterlevel >= WATERLEVEL_WAIST)// XDM3035a: player still get hit on shallow areas
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(edict(), CHAN_BODY, "player/pl_wade1.wav", VOL_NORM, ATTN_NORM);
			// see PM_CheckFalling g_usPM_Fall
			m_fWaterCircleTime = gpGlobals->time;
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)// after this point, we start doing damage
		{
			float flFallDamage = FallDamage(m_flFallVelocity);
			if (flFallDamage > 0.0f)
			{
				if (pev->groundentity)// XDM3035b: TESTME! ooops! We've landed on someone's head!
				{
					CBaseEntity *pLandedOn = CBaseEntity::Instance(pev->groundentity);
					if (pLandedOn && pLandedOn->pev->takedamage != DAMAGE_NO)
					{
						// XDM3038c: TESTME: experimental! if (pLandedOn->IsPlayer() || pLandedOn->IsMonster())
						pLandedOn->TakeDamage(/*this confuses DeathNotice()*/g_pWorld, this, flFallDamage, DMG_CRUSH/* | DMG_NEVERGIB*/);// we can do headshots without DMG_NOHITPOINT =)
						flFallDamage *= 0.5f;// XDM3038c: cushion!
					}
				}
				if (flFallDamage > pev->health)// XDM3035// NOTE: play on item/voice channel because we play footstep landing on body channel
				{
					if (m_fFrozen)
						EMIT_SOUND_DYN(edict(), CHAN_VOICE, gBreakSoundsGlass[RANDOM_LONG(0,NUM_BREAK_SOUNDS-1)], VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96, 104));
					else
						EMIT_SOUND_DYN(edict(), CHAN_VOICE, "common/bodysplat.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(96, 104));

					UTIL_DecalPoints(pev->origin, pev->origin-Vector(0.0f,0.0f,m_flFallVelocity), edict(), DECAL_BLOODSMEARR1);// the biggest spot // XDM3035a
					pev->punchangle.x = 0.0f;
				}
				else if (flFallDamage > pev->health/2)
				{
					UTIL_DecalPoints(pev->origin, pev->origin-Vector(0.0f,0.0f,m_flFallVelocity), edict(), DECAL_BLOODSMEARR2 + RANDOM_LONG(0,1));// XDM3035a
				}
				TakeDamage(g_pWorld, g_pWorld, flFallDamage, DMG_FALL | DMG_NEVERGIB | DMG_NOHITPOINT);// XDM: never gib! XDM3034 VARS(eoNullEntity)
			}
		}
		m_flFallVelocity = 0;

		if (IsAlive())// XDM3037: don't interrupt possible death
		{
			SetAnimation(PLAYER_WALK);
			if (m_flFallVelocity >= PLAYER_MIN_BOUNCE_SPEED)// XDM && !IsMultiplayer())
				CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, (int)m_flFallVelocity, 0.2);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override
// Output : float - final fall damage value
//-----------------------------------------------------------------------------
float CBasePlayer::FallDamage(const float &flFallVelocity)
{
	if (flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		return g_pGameRules?g_pGameRules->GetPlayerFallDamage(this):(flFallVelocity*DAMAGE_FOR_FALL_SPEED);

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: PlayerPreThink - called at the beginning of each frame
// Warning: Keep as clean as possible!!!
//-----------------------------------------------------------------------------
void CBasePlayer::PreThink(void)
{
	//DBG_PRINT_ENT_THINK(PreThink);
//useless	if (FBitSet(pev->button, IN_SCORE) || FBitSet(m_afButtonLast, IN_SCORE))// XDM3037a: disable accidental shots/respawn when player is interacting with the score panel
//		pev->button &= ~BUTTONS_READY;//(IN_ATTACK|IN_ATTACK2)

	int buttonsChanged;
	if (FBitSet(pev->flags, FL_FROZEN))// XDM3035b: disable everyting
		buttonsChanged = 0;
	else
		buttonsChanged = (m_afButtonLast ^ pev->button);	// These buttons have changed this frame

	bool bWasAlive = IsAlive();
	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);	// The ones not down are "released"
	//m_vecLastPosition = pev->origin;// for future use

	/* DEBUG if (FBitSet(m_afButtonPressed, IN_ATTACK))
		UTIL_DebugAngles(pev->origin, pev->v_angle, 1.0, 64.0f);
	if (FBitSet(m_afButtonPressed, IN_ATTACK2))
		UTIL_DebugAngles(pev->origin, pev->angles, 1.0, 64.0f);*/

	if (g_pGameRules)
	{
		g_pGameRules->PlayerThink(this);
		//DBG_PRINTF("CBasePlayer::PreThink() 1\n");
		if (g_pGameRules->IsGameOver())
		{
			UpdateStatusBar();// XDM3037: display winner name
			goto prethink_end;//?
			//return;// intermission or finale
		}
	}

	if (IsObserver())// XDM3037: allow spectators to change mode?
	{
		if (HasWeapons())// XDM3038a: shoud've been delayed from ObserverStart
		{
			PackImportantItems();// XDM3038c
			RemoveAllItems(true, true);
		}
		ObserverHandleButtons();
		//ObserverCheckTarget();
		//ObserverCheckProperties();
		// XDM3038b	pev->impulse = 0;
		goto prethink_end;// XDM3037a: do UpdateClientData() and InitHUD()!
		//return;
	}

	CheckEnvironment();// XDM3035b: this may cause death

	UTIL_MakeVectors(pev->v_angle);             // is this still used?

	// BUGBUG access violations, priveleged instructions always appear here. WTF!?

	if (IsAlive())// XDM3034: don't send useless data
	{
		CheckTimeBasedDamage();// this may cause death
		CheckSuitUpdate();
	}

	if (!IsAlive())// XDM3037
	{
		if (!bWasAlive)
			PlayerDeathThink();// DON'T CALL THIS ON THE SAME FRAME AS PLAYER DIED!!!

		goto prethink_end;//return;
	}

	////==== ! Only updates that are made when the player is alive are allowed after this point! ====////
	if (m_fFrozen)
		FrozenThink();

	if (m_flBurnTime > 0 && m_flBurnTime <= gpGlobals->time)// XDM3038a
		ClearBits(m_bitsDamageType, DMGM_FIRE|DMG_IGNITE);

//	if (!IsObserver())// XDM: BUGBUG: this prevents crash when entering spectator mode without crowbar
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		ItemPreFrame();
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		DBG_PRINTF("*** CBasePlayer(%d)::PreThink() ItemPreFrame() exception!\n", entindex());
		DBG_FORCEBREAK
	}
#endif

	if (FBitSet(m_afPhysicsFlags, PFLAG_ONBARNACLE))
	{
		if (m_hEnemy.Get())
		{
			//SetBits(pev->effects, EF_INVLIGHT);
			pev->velocity.Clear();
			TrainDetach();// XDM3035b: don't stop the train!
			//ClearBits(m_afPhysicsFlags, PFLAG_ONTRAIN);
			m_flFallVelocity = 0.0f;
		}
		else
		{
			conprintf(2, "CBasePlayer::PreThink() error: PFLAG_ONBARNACLE with no m_hEnemy (barnacle)! Releasing.\n");
			BarnacleVictimReleased();
		}
	}
	else
	{
		// If trying to duck, already ducked, or in the process of ducking
		if (FBitSet(pev->button, IN_DUCK) || FBitSet(pev->flags, FL_DUCKING) || FBitSet(m_afPhysicsFlags, PFLAG_DUCKING) )
			Duck();

		if (FBitSet(pev->button, IN_JUMP))
			Jump();

		if (!FBitSet(pev->flags, FL_ONGROUND))
			m_flFallVelocity = -pev->velocity.z;
	}

	if (m_iHUDDistortMode != 0)// XDM3038: someone has restored the power!
	{
		int iMaxArmor = (g_pGameRules?g_pGameRules->GetPlayerMaxArmor():MAX_NORMAL_BATTERY);// XDM3038
		if (pev->armorvalue >= iMaxArmor/10)
		{
			m_iHUDDistortUpdate = 1;
			m_iHUDDistortMode = 0;
			m_iHUDDistortValue = 0;
		}
	}

	TrainPreFrame();

	// Semi-secret weapon code here
	if (!m_fFrozen && !FBitSet(m_afPhysicsFlags, PFLAG_ONBARNACLE))// XDM3035
	{
		if (m_flThrowNDDTime > 0.0f && m_flThrowNDDTime <= gpGlobals->time)// XDM3035
		{
			int aiUranium = GetAmmoIndexFromRegistry("uranium");
			int aiSatchelCharge = GetAmmoIndexFromRegistry("Satchel Charge");
			int evflags = FEV_HOSTONLY|FEV_UPDATE;
			if (!GetInventoryItem(WEAPON_EGON) || AmmoInventory(aiUranium) < NUKE_AMMO_USE_URANIUM || AmmoInventory(aiSatchelCharge) < NUKE_AMMO_USE_SATCHEL)
			{
				PLAYBACK_EVENT_FULL(evflags, edict(), g_usNuclearDevice, 0.0f, pev->origin, pev->angles, 0.0f, 0.0f, 0, 0, 0, 0);
			}
			else
			{
				UTIL_MakeVectors(pev->v_angle);
				CBaseEntity *pNDD = Create("nucdevice", pev->origin + gpGlobals->v_forward * 32.0f, pev->angles, pev->velocity + gpGlobals->v_forward * 240.0f, edict(), SF_NORESPAWN);
				if (pNDD)
				{
					pNDD->pev->owner = edict();// XDM3037: hack! Must call SetIgnoreEnt()!
					m_rgAmmo[aiUranium] -= NUKE_AMMO_USE_URANIUM;
					m_rgAmmo[aiSatchelCharge] -= NUKE_AMMO_USE_SATCHEL;
					CBasePlayerItem *pEgon = GetInventoryItem(WEAPON_EGON);
					if (pEgon)
					{
						if (RemovePlayerItem(pEgon))// XDM3038a: if
							pEgon->Killed(this, this, GIB_NEVER);// XDM3038a
					}
					PLAYBACK_EVENT_FULL(evflags, edict(), g_usNuclearDevice, 0.0f, pev->origin, pev->angles, 0.0f, 0.0f, 1, 0, 0, 0);
				}
			}
			m_flThrowNDDTime = 0.0f;
			m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
			if (m_pActiveItem)
				DeployActiveItem();
		}
	}
	// StudioFrameAdvance();//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	WaterMove();

prethink_end:
	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: PlayerPostThink
// Warning: DON'T return;!!
//-----------------------------------------------------------------------------
void CBasePlayer::PostThink(void)
{
	//DBG_PRINT_ENT_THINK(PostThink);
	if (IsObserver())// XDM3035
	{
		// XDM3038b: TODO: ObserverImpulseCommands()?
		if (pev->impulse == 100)
		{
			if (FBitSet(pev->effects, EF_DIMLIGHT))
				ClearBits(pev->effects, EF_DIMLIGHT);
			else
				SetBits(pev->effects, EF_DIMLIGHT);

			pev->impulse = 0;
		}
		goto postthink_end;
	}

	if (!IsAlive())
		goto postthink_end;

	if (g_pGameRules && g_pGameRules->IsGameOver())
		goto postthink_end;

	// Handle Tank controlling
	if (m_pTank != NULL)// if player moved too far from the gun, or selected a weapon, unuse the tank
	{
		if (m_pTank->OnControls(pev))// XDM3037 && !pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2);	// try fire the gun
		}
		else// moved off controls
		{
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}

	if (m_flIgnoreLadderStopTime != 0.0f && m_flIgnoreLadderStopTime < gpGlobals->time)
	{
		ENGINE_SETPHYSKV(edict(), PHYSKEY_IGNORELADDER, "0");
		m_flIgnoreLadderStopTime = 0.0f;
	}

	if ((FBitSet(pev->flags, FL_ONGROUND) || IsOnLadder()) && (pev->health > 0) && m_flFallVelocity > 0)
	{
		Land();// may cause death!

		if (!IsAlive())// XDM3037: don't interrupt possible death
			goto postthink_end;// XDM3037: otherwise we would need to check below
	}

	// select the proper animation for the player character
	if (gpGlobals->frametime > 0.0)// XDM3037: don't spam while game is paused
	{
		//---- top animation: sequence ----
		if (pev->velocity.IsZero())
			SetAnimation(PLAYER_IDLE);
		else if (!FBitSet(pev->flags, FL_ONGROUND)/* && pev->waterlevel == 0 */&& m_flFallVelocity > 2.0f)// && (m_Activity != ACT_HOP && m_Activity != ACT_LEAP))
			SetAnimation(PLAYER_FALL);
		else
			SetAnimation(PLAYER_WALK);// swim animations will be used automatically

		//---- bottom animation: gaitsequence ----
		SetGaitAnimation();// XDM3037

		StudioFrameAdvance();
		//pev->modelindex = g_ulModelIndexPlayer;// XDM: ? was CheckPowerups() // XDM3037: since we don't use StartDeathCam(), it's probably obsolete
		UpdatePlayerSound();
	}

	// redundant if (!IsObserver())
		PlayerUse();// Handle +use commands

	ImpulseCommands();// Handle inmpulse commands
	ItemPostFrame();// Do weapon stuff (can affect animation)

postthink_end:

#if defined(CLIENT_WEAPONS)
#error UNDONE: HACK: fix this code when implementing local weapons
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for (size_t i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		CBasePlayerItem *pInvItem = GetInventoryItem(i);
		if (pInvItem)
		{
			CBasePlayerWeapon *pWeapon = pInvItem->GetWeaponPtr();
			if (pWeapon && pWeapon->UseDecrement())
			{
				pWeapon->m_flNextPrimaryAttack = max(pWeapon->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0);
				pWeapon->m_flNextSecondaryAttack = max(pWeapon->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001);

				if (pWeapon->m_flTimeWeaponIdle != 1000)
					pWeapon->m_flTimeWeaponIdle = max(pWeapon->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001);

				if (pWeapon->pev->fuser1 != 1000)
					pWeapon->pev->fuser1 = max(pWeapon->pev->fuser1 - gpGlobals->frametime, -0.001);

				// Only decrement if not flagged as NO_DECREMENT
				if (pWeapon->m_flPumpTime != 1000)
					pWeapon->m_flPumpTime = max(pWeapon->m_flPumpTime - gpGlobals->frametime, -0.001);

				if (pWeapon->m_flNextAmmoBurn != 1000)// XDM: unhack
					pWeapon->m_flNextAmmoBurn = max(pWeapon->m_flNextAmmoBurn - gpGlobals->frametime, -0.001);
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;
#endif

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

	if (IsOnLadder())// XDM3038a: we need this flag to track changes. Must be last in the frame!
		SetBits(m_afPhysicsFlags, PFLAG_ONLADDER);
	else
		ClearBits(m_afPhysicsFlags, PFLAG_ONLADDER);
}

//-----------------------------------------------------------------------------
// Purpose: Update/think function after player is Killed() (from PreThink())
// Warning: DON'T CALL THIS ON THE SAME FRAME AS PLAYER DIED!!!
// Warning: It will remove weapons that are still working (stack execution) and nullify their m_pPlayer!
//-----------------------------------------------------------------------------
void CBasePlayer::PlayerDeathThink(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::PlayerDeathThink()\n", entindex());
	if (pev->movetype == MOVETYPE_NOCLIP)// XDM3035 Disintegrate
	{
		if (HasWeapons())// XDM3038a: when disintegrating, don't drop anything
		{
			PackImportantItems();// XDM3038c
			RemoveAllItems(true, true);
		}

		ClearBits(pev->flags, FL_ONGROUND);
		if (pev->renderamt > 1)
		{
			pev->renderamt -= 1;
			//pev->origin.z += 0.5f;// HACK
			if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
			{
				if (RANDOM_LONG(0,5) == 0)
				{
					SetBits(pev->effects, EF_MUZZLEFLASH);
					UTIL_Sparks(pev->origin);
				}
			}
		}
		else
			pev->renderamt = 0;// add EF_NODRAW?
	}
	else// if (pev->movetype != MOVETYPE_NOCLIP)
	{
		if (HasWeapons())
		{
			// we drop the guns here because weapons that have an area effect and can kill their user
			// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
			// player class sometimes is freed. It's safer to manipulate the weapons once we know
			// we aren't calling into any of their code anymore through the player pointer.
#if defined (USE_EXCEPTIONS)
			try
			{
#endif
				PackDeadPlayerItems();
#if defined (USE_EXCEPTIONS)
			}
			catch(...) {conprintf(1, "ERROR: PlayerDeathThink: PackDeadPlayerItems() exception!\n");}
#endif
		}
		if (FBitSet(pev->flags, FL_ONGROUND))
		{
			vec_t flForward = pev->velocity.Length() - 20.0f;
			if (flForward <= 0)
				pev->velocity.Clear();
			else
				pev->velocity.SetLength(flForward);// = flForward * pev->velocity.Normalize();

			pev->avelocity.Clear();// XDM3038c
		}
	}

	m_flNextSBarUpdateTime = gpGlobals->time;
	UpdateStatusBar();// XDM3037: display killer's name

	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))// XDM3038a: TODO: check EF_NODRAW instead of modelindex?
	{
		StudioFrameAdvance();
		++m_iRespawnFrames;// Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if (m_iRespawnFrames < 120)// Animations should be no longer than this
			return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	//if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
	//	if (pev->movetype != MOVETYPE_NOCLIP && pev->waterlevel < 3)// XDM3035
	//		pev->movetype = MOVETYPE_NONE;// XDM3037a: TODO: remove this, but there might be some MOVETYPE_NONE checks elsewhere!
	// XDM3038: keep MOVETYPE_TOSS and react to conveyors

	// wait for all buttons released
	if (pev->deadflag == DEAD_DYING)// XDM3034
	{
		pev->deadflag = DEAD_DEAD;
		//?if (fAnyButtonDown)
			return;
	}
	else if (pev->deadflag == DEAD_DEAD)// XDM3034
	{
		ASSERT(m_fDiedTime != 0);// XDM3038c: moved to Killed() m_fDiedTime = gpGlobals->time;
		SetBits(pev->effects, EF_NOINTERP);
		pev->deadflag = DEAD_RESPAWNABLE;
		pev->button = 0;// XDM3037a: stop, have a break;
		StopAnimation();// pev->framerate = 0.0;
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);// XDM3038c: HACK: become smaller, block less (but still do!)
	}
	else if (pev->deadflag == DEAD_RESPAWNABLE)
	{
		if (IsObserver())// HL20130901
			return;

		bool fAnyButtonDown = !FBitSet(pev->button, IN_SCORE) && FBitSet(pev->button, BUTTONS_READY);// XDM3037a: never attempt anything while scoreboard is open // we could use m_bReadyPressed, but it's set in MP
		// wait for any button down, or mp_forcerespawn is set and the respawn time is up
		if (fAnyButtonDown || g_pGameRules && g_pGameRules->FForceRespawnPlayer(this))// XDM3034
		{
			if (g_pGameRules && g_pGameRules->FPlayerCanRespawn(this))// XDM: this checks respawn time
			{
				//conprintf(1, "Respawn\n");// RESPAWN CODE GOES HERE
				m_iRespawnFrames = 0;
				m_pLastItem = NULL;// XDM3038: TESTME
				m_pNextItem = NULL;// XDM3038
				pev->button = 0;
				pev->impulse = 0;// XDM3035a: clear all pending impulse commands (flashlight, flares, etc.)
				SetBits(pev->effects, EF_NODRAW|EF_NOINTERP);// XDM
				ClearBits(pev->flags, FL_FLOAT);// XDM3037a
				DontThink();// XDM3038c pev->nextthink = -1;
				Spawn(FALSE);// XDM3037: IMPORTANT: call with this explicit "not restoring" flag!
			}
			else if (g_pGameRules == NULL || !g_pGameRules->IsMultiplayer())// restart the entire server
			{
				if ((gpGlobals->time - m_fDiedTime) > SINGLEPLAYER_RESTART_DELAY)// XDM3037a
					SERVER_COMMAND("reload\n");
				else
					pev->button = 0;
			}
			// NO CODE AFTER THESE LINES!!!
		}
	}
	else
		ASSERTSZ(false, "PlayerDeathThink: BAD DEADFLAG!!!\n");
}

//-----------------------------------------------------------------------------
// Purpose: Apply long-lasting damage here
//-----------------------------------------------------------------------------
void CBasePlayer::CheckTimeBasedDamage(void)
{
	if (!FBitSet(m_bitsDamageType, DMGM_TIMEBASED))
		return;
	if (abs(gpGlobals->time - m_tbdPrev) < TIMEBASED_DAMAGE_INTERVAL)
		return;

	size_t i;
	byte bDuration = 0;
	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; ++i)
	{
		// Do not use DMG_types here to prevent positive feedback and infinite damage
		if (FBitSet(m_bitsDamageType, (DMG_PARALYZE << i)))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = TD_PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//TakeDamage(this, this, TD_NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = TD_NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(this, g_pWorld, TD_POISON_DAMAGE, DMG_GENERIC);// XDM3035: world
				bDuration = TD_POISON_DURATION;
				break;
			case itbd_Radiation:
				//TakeDamage(this, this, TD_RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = TD_RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);
					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;// get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//TakeDamage(pev, pev, TD_ACID_DAMAGE, DMG_GENERIC);
				bDuration = TD_ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//TakeDamage(pev, pev, TD_SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = TD_SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//TakeDamage(pev, pev, TD_SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = TD_SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				if (m_rgItems[ITEM_ANTIDOTE])// use up an antitoxin on poison or nervegas after a few seconds of damage
				{
					if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < TD_NERVEGAS_DURATION)) ||
						((i == itbd_Poison)   && (m_rgbTimeBasedDamage[i] < TD_POISON_DURATION)))
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}
				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					ClearBits(m_bitsDamageType, DMG_PARALYZE << i);// if we're done, clear damage bits
				}
			}
			else// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update Geiger counter (regardless of damage)
//-----------------------------------------------------------------------------
void CBasePlayer::UpdateGeigerCounter(void)
{
	//should be checked	if (!HasSuit())// XDM3038: don't flood the net
	//		return;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client
	byte range = (byte)(m_flgeigerRange / 4.0f);
	if (range != m_igeigerRangePrev)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, edict());
			WRITE_BYTE(range);
		MESSAGE_END();
		m_igeigerRangePrev = range;
	}
	// reset counter and semaphore
	if (RANDOM_LONG(0,3) == 0)
		m_flgeigerRange = 1000;
}

//-----------------------------------------------------------------------------
// Purpose: Play queued suit messages
//-----------------------------------------------------------------------------
void CBasePlayer::CheckSuitUpdate(void)
{
	if (!HasSuit())
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (IsMultiplayer())
		return;// don't bother updating HEV voice in multiplayer.

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		int i;
		int isentence = 0;
		int isearch = m_iSuitPlayNext;
		ASSERT(isearch >= 0 && isearch < CSUITPLAYLIST);
		if (isearch >= CSUITPLAYLIST)
		{
			conprintf(2, "CBasePlayer::CheckSuitUpdate() bad index: m_iSuitPlayNext = %d!\n", m_iSuitPlayNext);
			return;
		}

		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; ++i)
		{
			isentence = m_rgSuitPlayList[isearch];
			if (isentence)
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}
		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)// play sentence number
			{
				char sentence[CBSENTENCENAME_MAX+1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(edict(), sentence);
			}
			else
				EMIT_GROUPID_SUIT(edict(), -isentence);// play sentence group

			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: add sentence to suit playlist queue. if fgroup is true, then name is a sentence group (HEV_AA), otherwise name is a specific sentence name ie: !HEV_AA0.
// If iNoRepeat is specified in seconds, then we won't repeat playback of this word or sentence for at least that number of seconds.
// Input  : *name - 
//			fgroup - 
//			iNoRepeatTime - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetSuitUpdate(const char *name, int fgroup, int iNoRepeatTime)
{
	if (!HasSuit())
		return;

	if (IsMultiplayer())
		return;// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.

	int i;
	if (name == NULL)// if name == NULL, then clear out the queue
	{
		for (i = 0; i < CSUITPLAYLIST; ++i)
			m_rgSuitPlayList[i] = 0;
		return;
	}

	int isentence;
	if (fgroup)// get sentence or group number
		isentence = -SENTENCEG_GetIndex(name);// mark group number as negative
	else
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}

	int iempty = -1;
	// check norepeat list - this list lets us cancel the playback of words or sentences that have already been played within a certain time.
	for (i = 0; i < CSUITNOREPEAT; ++i)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in the norepeat list
			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given
	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT-1); // pick random slot to take over

		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext >= CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Small ID text shown when aiming at someone
//-----------------------------------------------------------------------------
void CBasePlayer::InitStatusBar(void)
{
	m_StatusBarString[0] = 0;
	for (size_t i = 0; i < SBAR_NUMVALUES; ++i)
		m_iStatusBarValues[i] = 0;

	m_flStatusBarDisappearDelay = 0;
	m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
}

//-----------------------------------------------------------------------------
// Purpose: StatusBar - small text label shown when player is facing someone
// Warning: Protocol is XDM-specific
//-----------------------------------------------------------------------------
void CBasePlayer::UpdateStatusBar(void)
{
	if (m_flNextSBarUpdateTime > gpGlobals->time)
		return;

	if (IsBot())
		return;

	size_t i;
	short newSBarValues[SBAR_NUMVALUES] = {0,0,0,0};
	char newSBarString[SBAR_STRING_SIZE];
	// restore old values to compare to
	strcpy(newSBarString, m_StatusBarString);// <-

	// Select status bar target
	CBaseEntity *pEntity = NULL;
	if (g_pGameRules && g_pGameRules->IsGameOver() && g_pGameRules->GetIntermissionActor1())// XDM3037: display winner name during intermission
	{
		pEntity = g_pGameRules->GetIntermissionActor1();// same m_hObserverTarget
		m_flStatusBarDisappearDelay = g_pGameRules->GetIntermissionEndTime();
	}
	else if (IsAlive())
	{
		if (IsMultiplayer() && m_hAutoaimTarget.Get())// XDM3037: display autoaim ID string
		{
			pEntity = m_hAutoaimTarget;
			m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
		}
		else// Find an ID Target infront of me
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->v_angle + pev->punchangle);
			Vector vecSrc(EyePosition());
			Vector vecEnd(vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE));
			SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// XDM3038b: improves performance
			UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);
			ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
			if (tr.flFraction < 1.0)
			{
				if (!FNullEnt(tr.pHit))// FNullEnt invalidates the world too!
				{
					pEntity = CBaseEntity::Instance(tr.pHit);
					m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
				}
			}
		}
	}
	else// XDM3037: if player is dead, display killer's name
	{
		pEntity = m_hLastKiller;// EHANDLE may get invalidated while getting value, which is good
	}

	// Fill the data to send
	int numlines = 0;
	bool bSendString = false;
	bool bSendData = false;
	if (pEntity)
	{
		if (pEntity->IsPlayer() || (pEntity->IsMonster() && !pEntity->IsProjectile()))// XDM
		{
			newSBarValues[SBAR_ID_TARGETINDEX] = ENTINDEX(pEntity->edict());
			// allies and medics get to see the targets health
			int rs = g_pGameRules?g_pGameRules->PlayerRelationship(this, pEntity):R_NO;
			if (rs == GR_TEAMMATE || rs == GR_ALLY)
			{
				strcpy(newSBarString, "#SB_AL\0");// %c0%i0: (ALLY) %n0\n%c0 Health: %d1\n%c0 Armor: %d2
				newSBarValues[SBAR_ID_TARGETHEALTH] = (short)pEntity->pev->health;// XDM
				newSBarValues[SBAR_ID_TARGETARMOR] = (short)pEntity->pev->armorvalue;
			}
			else if (/*g_pGameRules && g_pGameRules->IsCoOp() && */pEntity->IsMonster())// XDM3038b: uncomment to use special status for monsters
			{
				strcpy(newSBarString, "#SB_MO\0");// %i0: %n0\n%c0 Health: %d1\n%c0 Armor: %d2
				newSBarValues[SBAR_ID_TARGETHEALTH] = (short)pEntity->pev->health;// XDM
				newSBarValues[SBAR_ID_TARGETARMOR] = (short)pEntity->pev->armorvalue;
			}
			else
			{
				strcpy(newSBarString, "#SB_EN\0");// %c0%i0: %n0
				newSBarValues[SBAR_ID_TARGETHEALTH] = 0;
				newSBarValues[SBAR_ID_TARGETARMOR] = 0;
			}
			numlines = 1;
			//m_flStatusBarDisappearDelay = gpGlobals->time + 1.0f;
			// check for changes
			if (strcmp(newSBarString, m_StatusBarString) != 0)
				bSendString = true;
			// check for changes
			for (i = 0; i < SBAR_NUMVALUES; ++i)// XDM3038a
			{
				if (newSBarValues[i] != m_iStatusBarValues[i])
				{
					bSendData = true;
					break;// one is enough
				}
			}
		}
	}
	else// no entity in sight
	{
		if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			//for (i = 0; i < SBAR_NUMVALUES; ++i)
			//	newSBarValues[i] = m_iStatusBarValues[i];
			return;// XDM3038a
		}
		else// timeout reached
		{
			numlines = SBAR_MSG_CLEAR;// clear
			m_StatusBarString[0] = 0;// clear so this entity may be redetected next time
			bSendString = true;
		}
	}

	if (bSendString || bSendData)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusData, NULL, edict());
			WRITE_BYTE(numlines);// num lines (255 means clear all)
		if (numlines > 0 && numlines < 255)// there is actual string to send
		{
			WRITE_STRING(newSBarString);// string
			strcpy(m_StatusBarString, newSBarString);
		}
		//conprintf(1, "UpdateStatusBar(%d): +string \"%s\"\n", entindex(), sbuf0);
		if (bSendData)
		{
			for (i = 0; i < SBAR_NUMVALUES; ++i)// XDM3038a
			{
				if (newSBarValues[i] != m_iStatusBarValues[i])
				{
					WRITE_BYTE(i);
					WRITE_SHORT(newSBarValues[i]);
					m_iStatusBarValues[i] = newSBarValues[i];
					//conprintf(1, "UpdateStatusBar(%d): +value %d %d\n", entindex(), i, newSBarValues[i]);
				}
			}
		}
		MESSAGE_END();
	}
	m_flNextSBarUpdateTime = gpGlobals->time + 0.25;
}

//-----------------------------------------------------------------------------
// Purpose: updates the position of the player's reserved sound slot in the sound list.
//-----------------------------------------------------------------------------
void CBasePlayer::UpdatePlayerSound(void)
{
	int iBodyVolume = 0;
	int iVolume = 0;
	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));// XDM: may return null if monsters are not allowed
	if (pSound)
		pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon is louder than his body/movement, use the weapon volume, else, use the body volume.
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();
		// clamp the noise that can be made by the body, in case a push trigger, weapon recoil, or anything shoves the player abnormally fast.
		if (iBodyVolume > HUMAN_MAX_BODY_VOLUME)
			iBodyVolume = HUMAN_MAX_BODY_VOLUME;
	}
	else
		iBodyVolume = 0;

	if (FBitSet(pev->button, IN_JUMP))
		iBodyVolume += HUMAN_JUMP_VOLUME;

	// convert player move speed and actions into sound audible by monsters
	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;
		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		if (pSound)
			SetBits(pSound->m_iType, bits_SOUND_COMBAT);
	}
	else
		m_iTargetVolume = iBodyVolume;

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= (int)(250.0f * gpGlobals->frametime);
	if (m_iWeaponVolume < 0)
		iVolume = 0;

	if (m_fNoPlayerSound)// debugging only
		iVolume = 0;
	else
	{
		// if target volume is greater than the player sound's current volume, we paste the new volume in
		// immediately. If target is less than the current volume, current volume is not set immediately to the
		// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
		// to hear a sound, especially if they don't listen every frame.
		if (pSound)
			iVolume = pSound->m_iVolume;

		if (m_iTargetVolume > iVolume)
		{
			iVolume = m_iTargetVolume;
		}
		else if (iVolume > m_iTargetVolume)
		{
			iVolume -= (int)(250.0f * gpGlobals->frametime);
			if (iVolume < m_iTargetVolume)
				iVolume = 0;
		}
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)// time to "stop" extra sounds
		m_iExtraSoundTypes = 0;

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		SetBits(pSound->m_iType, bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= (int)(256.0f * gpGlobals->frametime);// XDM3038c: changed order of type casting
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the player is making.
	//UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//conprintf(1, "%d/%d\n", iVolume, m_iTargetVolume );
}

//-----------------------------------------------------------------------------
// Purpose: "impulse %d"
//-----------------------------------------------------------------------------
void CBasePlayer::ImpulseCommands(void)
{
	if (pev->impulse == 204)// Demo recording, update client dll specific data again.
	{
		ForceClientDllUpdate();
		return;
	}

	if (m_fFrozen)// XDM3035: fgrenade affects all other commands
	{
		pev->impulse = 0;
		return;
	}

	switch (pev->impulse)
	{
	case 100:
		{
			if (FlashlightIsOn())
				FlashlightTurnOff();
			else
				FlashlightTurnOn();
		}
		break;
	case 120:
		{
			if (m_rgItems[ITEM_FLARE] > 0)// XDM3035
			{
				UTIL_MakeVectors(pev->v_angle);
				Vector vecSpot(gpGlobals->v_forward); vecSpot *= 32.0f; vecSpot += pev->origin;//pev->origin + gpGlobals->v_forward * 32.0f;
				if (UTIL_PointContents(vecSpot) != CONTENTS_SOLID)
				{
					CBaseEntity *pFlare = Create("flare", vecSpot, g_vecZero, pev->velocity + gpGlobals->v_forward * 300.0f, edict(), SF_NORESPAWN);
					if (pFlare && !FBitSet(pFlare->pev->flags, FL_KILLME))
					{
						m_rgItems[ITEM_FLARE]--;
						if (m_rgItems[ITEM_FLARE] > 0)
							ClientPrint(pev, HUD_PRINTTALK, UTIL_VarArgs("#ITEMSLEFT%d %d\n", ITEM_FLARE, m_rgItems[ITEM_FLARE]));
						else
							ClientPrint(pev, HUD_PRINTTALK, UTIL_VarArgs("#ITEMSLEFTNONE%d", ITEM_FLARE));
					}
				}
			}
		}
		break;
	case 121:
		{
			ThrowNuclearDevice();// XDM3035
		}
		break;
	case 201:// paint decal
		if (m_flNextDecalTime <= gpGlobals->time)
		{
			TraceResult	tr;
			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(GetGunPosition(), GetGunPosition() + gpGlobals->v_forward * 128.0f, ignore_monsters, edict(), &tr);
			if (tr.flFraction != 1.0)// line hit something, so paint a decal
			{
				int nFrames = GetCustomDecalFrames();
				if (nFrames == -1)// No customization present.
					UTIL_DecalTrace(&tr, DECAL_XHL);
				else
					UTIL_PlayerDecalTrace(&tr, entindex(), nFrames, true);

				EMIT_AMBIENT_SOUND(edict(), tr.vecEndPos, "player/sprayer.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM+RANDOM_LONG(-4,4));// XDM3038
				m_flNextDecalTime = gpGlobals->time + sv_decalfrequency.value;
				m_Stats[STAT_DECALS_COUNT]++;// XDM3037
			}
		}
		break;
	default:
		CheatImpulseCommands(pev->impulse);
		break;
	}
	pev->impulse = 0;
}

//-----------------------------------------------------------------------------
// Purpose: ImpulseCommands considered cheats
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CBasePlayer::CheatImpulseCommands(const int &iImpulse)
{
	if (g_psv_cheats->value <= 0.0f)
		return;

	switch (iImpulse)
	{
	case 101:
		ClientPrint(pev, HUD_PRINTCONSOLE, "This impulse is obsolete, use 'giveall' instead.\n");
		break;
	case 102:
		CGib::SpawnRandomGibs(this, 4, true, gpGlobals->v_forward*40.0f);
		break;
	case 103:
	case 106:
		{
			CBaseEntity *pEntity = UTIL_FindEntityForward(this);
			if (pEntity)
				pEntity->ReportState(1);
		}
		break;
	case 104:
		gGlobalState.DumpGlobals();// Dump all of the global state varaibles (and global entity names)
		break;
	case 105:
		{
			if (m_fNoPlayerSound)
			{
				m_fNoPlayerSound = false;
				ClientPrint(pev, HUD_PRINTCONSOLE, "You are audible for AI\n");
			}
			else
			{
				m_fNoPlayerSound = true;
				ClientPrint(pev, HUD_PRINTCONSOLE, "You are silent for AI\n");
			}
		}
		break;
	case 107:
		{
			TraceResult tr;
			Vector start(GetGunPosition());
			Vector end(gpGlobals->v_forward); end *= 1024.0f; end += start;
			UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
			if (tr.pHit)
			{
				const char *pTextureName = TRACE_TEXTURE(tr.pHit, start, end);
				if (pTextureName)
					ClientPrintF(pev, HUD_PRINTCONSOLE, 0, "Texture: %s, type: %c\n", pTextureName, PM_FindTextureTypeFull(pTextureName));
			}
		}
		break;
	case 108:
		{
			ClientPrintF(pev, HUD_PRINTCONSOLE, 0, "Showing pev->angles (%g, %g, %g)\n", pev->angles.x, pev->angles.y, pev->angles.z);
			UTIL_DebugAngles(pev->origin, pev->angles, 3.0, 24);
		}
		break;
	case 109:
		{
			ClientPrintF(pev, HUD_PRINTCONSOLE, 0, "Showing pev->v_angle (%g, %g, %g)\n", pev->v_angle.x, pev->v_angle.y, pev->v_angle.z);
			UTIL_DebugAngles(pev->origin, pev->v_angle, 3.0, 24);
		}
		break;
	case 195:
		{
			conprintf(0, "Showing shortest paths for entire level to nearest node: node_viewer_fly\n");
			Create("node_viewer_fly", pev->origin, pev->angles);
		}
		break;
	case 196:
		{
			conprintf(0, "Showing shortest paths for entire level to nearest node: node_viewer_large\n");
			Create("node_viewer_large", pev->origin, pev->angles);
		}
		break;
	case 197:
		{
			conprintf(0, "Showing shortest paths for entire level to nearest node: node_viewer_human\n");
			Create("node_viewer_human", pev->origin, pev->angles);
		}
		break;
	case 199:// show nearest node and all connections
		{
			int node = WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM);
			conprintf(0, "Showing nearest node (%d) and all connections\n", node);
			WorldGraph.ShowNodeConnections(node);
		}
		break;
	case 202:// Random blood splatter
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(GetGunPosition(), GetGunPosition() + gpGlobals->v_forward * 128.0f, ignore_monsters, edict(), &tr);
			if (tr.flFraction < 1.0f)// line hit something, so paint a decal
				UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
		}
		break;
	case 203:
		{
			CBaseEntity *pEntity = UTIL_FindEntityForward(this);
			if (pEntity)// && pEntity->pev->takedamage)
			{
				conprintf(0, "Removing entity %s[%d]\n", STRING(pEntity->pev->classname), pEntity->entindex());// XDM
				UTIL_Remove(pEntity);// Dangerous! // pEntity->SetThink(&CBaseEntity::SUB_Remove);? // SetBits(pEntity->pev->flags, FL_KILLME);?
			}
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true/false
//-----------------------------------------------------------------------------
bool CBasePlayer::FlashlightIsOn(void)
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::FlashlightTurnOn(void)
{
	if (g_pGameRules && !g_pGameRules->FAllowFlashlight())
		return;

	if (HasSuit())
	{
		EMIT_SOUND_DYN(edict(), CHAN_ITEM, "items/flashlight1.wav", VOL_NORM, ATTN_IDLE, 0, PITCH_NORM);
		SetBits(pev->effects, EF_DIMLIGHT);
		FlashlightUpdate(1);
		m_flFlashLightTime = gpGlobals->time + FLASHLIGHT_UPDATE_INTERVAL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::FlashlightTurnOff(void)
{
	if (IsAlive() && HasSuit())
		EMIT_SOUND_DYN(edict(), CHAN_ITEM, "items/flashlight1.wav", VOL_NORM, ATTN_IDLE, 0, PITCH_NORM);

    ClearBits(pev->effects, EF_DIMLIGHT);
	FlashlightUpdate(0);
	m_flFlashLightTime = gpGlobals->time + FLASHLIGHT_UPDATE_INTERVAL;
}

//-----------------------------------------------------------------------------
// Purpose: Send flashlight state update to client side
// Input  : state - 1/0
//-----------------------------------------------------------------------------
void CBasePlayer::FlashlightUpdate(const int &state)
{
	MESSAGE_BEGIN(((sv_reliability.value > 0)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgFlashlight, NULL, edict());
		WRITE_BYTE(state);
		WRITE_BYTE((int)m_fFlashBattery);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: When recording a demo, we need to have the server tell us the entire client state
// so that the client side .dll can behave correctly.
// Reset stuff so that the state is transmitted.
//-----------------------------------------------------------------------------
void CBasePlayer::ForceClientDllUpdate(void)
{
	// XDM3037a	m_iClientHealth  = -1;
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;// Force new train message.
	//m_fWeapon = FALSE;// Force weapon send
	m_fInventoryChanged = FALSE;// Force weaponinit messages.
	m_fInitHUD = TRUE;// Force HUD gmsgResetHUD message
	// Now force all the necessary messages to be sent.
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: How much ammo of this type? (index)
// Input  : &iAmmoIndex - 
// Output : int - count
//-----------------------------------------------------------------------------
int CBasePlayer::AmmoInventory(const int &iAmmoIndex)
{
	if (iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS)
		return -1;// TODO: remove this and rewrite with uint32

	return m_rgAmmo[iAmmoIndex];
}

//-----------------------------------------------------------------------------
// Purpose: This function is used to find and store all the ammo we have into the ammo vars.
//-----------------------------------------------------------------------------
void CBasePlayer::TabulateAmmo(void)
{
/*#if defined(CLIENT_WEAPONS)// big valve hack
	ammo_9mm = AmmoInventory(GetAmmoIndexFromRegistry("9mm"));
	ammo_357 = AmmoInventory(GetAmmoIndexFromRegistry("357"));
	ammo_argrens = AmmoInventory(GetAmmoIndexFromRegistry("ARgrenades"));
	ammo_bolts = AmmoInventory(GetAmmoIndexFromRegistry("bolts"));
	ammo_buckshot = AmmoInventory(GetAmmoIndexFromRegistry("buckshot"));
	ammo_rockets = AmmoInventory(GetAmmoIndexFromRegistry("rockets"));
	ammo_uranium = AmmoInventory(GetAmmoIndexFromRegistry("uranium"));
	ammo_hornets = AmmoInventory(GetAmmoIndexFromRegistry("Hornets"));
#endif*/
	if (m_pActiveItem && m_pActiveItem->GetWeaponPtr())
	{
		if (m_pActiveItem->GetWeaponPtr()->m_fInReload > 0)
			SetAnimation(PLAYER_RELOAD);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Give Ammo by ID
// Input  : iAmount - how much do we try to add
//			&iIndex - 
//			iMax - ??? to allow custom maximum value?
// Output : Returns the amount of ammo actually added
//-----------------------------------------------------------------------------
#if defined(OLD_WEAPON_AMMO_INFO)
int CBasePlayer::GiveAmmo(const int &iAmount, const int &iIndex, const int &iMax)
#else
int CBasePlayer::GiveAmmo(const int &iAmount, const int &iIndex)
#endif
{
	DBG_ITM_PRINT("CBasePlayer(%d)::GiveAmmo(%d, %d)\n", entindex(), iAmount, iIndex);//, iMax);

	if (iIndex < 0 || iIndex >= MAX_AMMO_SLOTS)
		return 0;

#if !defined(OLD_WEAPON_AMMO_INFO)
	int iMax = g_AmmoInfoArray[iIndex].iMaxCarry;
#endif

	int iAmmoInInventory = AmmoInventory(iIndex);
#if defined (_DEBUG_ITEMS)
	ASSERT(iAmmoInInventory <= iMax);// WTF
#endif

	if (iAmmoInInventory >= iMax)// XDM3035b: check here!
		return 0;

	//never used	if (g_pGameRules && !g_pGameRules->CanHaveAmmo(this, iIndex))
	//	return 0;// game rules say I can't have any more of this ammo type.

	int iAdd = min(iMax - iAmmoInInventory, iAmount);// fill inventory to MAX or take what iAmount can offer
	if (iAdd <= 0)
		return 0;

	//conprintf(1, "CBasePlayer::GiveAmmo(%d %d %d max)\n", iAmount, iIndex, iMax);

	m_rgAmmo[iIndex] += iAdd;
	// BAD, leaves old values on client side	if (g_SilentItemPickup && m_rgAmmoLast[iIndex] > 0)// XDM3038b: don't notify when asked, but don't leave client thinking he has NO AMMO or weapon selection will not work!!
	//	m_rgAmmoLast[iIndex] = m_rgAmmo[iIndex];

	/* XDM3038a: client detects difference automatically
	if (gmsgAmmoPickup)// make sure the ammo messages have been linked first
	{
		//conprintf(2, "SV: gmsgAmmoPickup: %d (%s) +%d\n", iIndex, g_AmmoInfoArray[iIndex].name, iAdd);// XDM
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, NULL, edict());
			WRITE_BYTE(iIndex);// ammo ID
			WRITE_BYTE(iAdd);// amount
		MESSAGE_END();
	}*/

	TabulateAmmo();
	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: Add a weapon to the player (Item == Weapon == Selectable Object)
// XDM3035a: NOTE: add lots of checks here because we may get corrupted pointers/objects
// Input  : *pItem - 
// Output : ITEM_ADDRESULT
//-----------------------------------------------------------------------------
int CBasePlayer::AddPlayerItem(CBasePlayerItem *pItem)
{
	DBG_ITM_PRINT("CBasePlayer(%d)::AddPlayerItem(%d)(id %d) START\n", entindex(), pItem?pItem->entindex():0, pItem?pItem->GetID():0);

	if (pItem == NULL)
		return ITEM_ADDRESULT_NONE;

	if (pItem->pev == NULL)
	{
		DBG_FORCEBREAK
		conprintf(0, "CBasePlayer(%d)::AddPlayerItem(%d) corrupted item!\n", entindex(), pItem->GetID());
		return ITEM_ADDRESULT_NONE;
	}
	int id = pItem->GetID();
	if (id == WEAPON_NONE)
	{
		conprintf(1, "CBasePlayer(%d)::AddPlayerItem(%d) tried to add WEAPON_NONE!\n", entindex(), id);
		DBG_FORCEBREAK
		return ITEM_ADDRESULT_NONE;
	}

	//something's not right here! recheck step-by-step!
	//somehow after removing pItem it appears in our inventory!
	CBasePlayerItem *pExisting = GetInventoryItem(id);// XDM: !!! we may already have item of this kind!
	if (pExisting)// XDM3035b
	{
		if (pExisting == pItem)// Why? Why does this keep happening?
		{
			conprintf(1, "CBasePlayer(%d)::AddPlayerItem(%d)(id %d): Error detected: pExisting == pItem!\n", entindex(), pItem->entindex(), id);
#if defined (_DEBUG_ITEMS)
			DBG_FORCEBREAK
#endif
			pItem->AttachTo(this);// prevent further touching!!
			return ITEM_ADDRESULT_NONE;// pretend we did not touch it, so DefaultTouch() won't kill it! right?
		}

		if (pItem->AddDuplicate(pExisting))
		{
			if (HasPlayerItem(pItem))
			{
				conprintf(1, "CBasePlayer(%d)::AddPlayerItem(%d)(id %d) duplicate pItem got into inventory!!\n", entindex(), pItem->entindex(), id);
				DBG_FORCEBREAK
			}
			return ITEM_ADDRESULT_EXTRACTED;// EXTRACTED AMMO
		}
#if defined (_DEBUG)
		if (GetInventoryItem(id) == NULL)
		{
			conprintf(1, "CBasePlayer(%d)::AddPlayerItem(%d)(id %d) KERNEL PANIC!!!!!!!!\n", entindex(), pItem->entindex(), pItem->GetID());
			DBG_FORCEBREAK
		}
#endif
		DBG_ITM_PRINT("CBasePlayer(%d)::AddPlayerItem(%d)(id %d) step 1 ITEM_ADDRESULT_NONE\n", entindex(), pItem->entindex(), pItem->GetID());
		return ITEM_ADDRESULT_NONE;// NOT EXTRACTED
	}
	else if (pItem->AddToPlayer(this))
	{
		// Don't disable this!
		SetBits(pev->weapons, 1<<id);// XDM3035a: NOTE: External bots use this!
		m_rgpWeapons[id] = pItem;// XDM3038: pItem->GetID()] = pItem;
//#if defined (_DEBUG_ITEMS)
//		m_rgpWeapons[id].m_debuglevel = 2;
//#endif
		//useless	if (g_pGameRules) g_pGameRules->PlayerGotWeapon(this, pItem);
		//pItem->CheckRespawn();
		/* XDM3038a: OBSOLETE		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, edict());
			WRITE_BYTE(id);// XDM3038: pItem->GetID());
		MESSAGE_END();*/
#if defined(SERVER_WEAPON_SLOTS)
		// Switch only if we're not attacking or have no weapon equipped at all
		if (!FBitSet(pev->button, IN_ATTACK|IN_ATTACK2)) || m_pActiveItem == NULL)
		{
			if (g_pGameRules && g_pGameRules->FShouldSwitchWeapon(this, pItem))// should we switch to this item?
				SelectItem(pItem);
		}
#endif
		DBG_ITM_PRINT("CBasePlayer(%d)::AddPlayerItem(%d)(id %d) step 2 ITEM_ADDRESULT_PICKED\n", entindex(), pItem->entindex(), pItem->GetID());
		return ITEM_ADDRESULT_PICKED;// PICKED UP
	}
	DBG_ITM_PRINT("CBasePlayer(%d)::AddPlayerItem(%d)(id %d) step 3 ITEM_ADDRESULT_NONE!\n", entindex(), pItem->entindex(), pItem->GetID());
	return ITEM_ADDRESULT_NONE;// NOT PICKED
}

//-----------------------------------------------------------------------------
// Purpose: Clears item from player, but does not destroy it!
// Warning: Don't corrupt the item! Also, modify only player data here.
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::RemovePlayerItem(CBasePlayerItem *pItem)
{
	DBG_ITM_PRINT("CBasePlayer(%d)::RemovePlayerItem(%d)(id %d)\n", entindex(), pItem?pItem->entindex():0, pItem?pItem->GetID():0);
	if (pItem == NULL)
		return false;

	if (pItem->GetID() == WEAPON_NONE)
	{
		conprintf(1, "RemovePlayerItem(%s) ERROR: tried to remove item with ID WEAPON_NONE!\n", STRING(pItem->pev->classname));
		DBG_FORCEBREAK
		return false;
	}

	if (m_pActiveItem == pItem)
	{
		ResetAutoaim();
		//if (pItem->CanHolster())// XDM3038c if (AmmoInventory(pItem->PrimaryAmmoIndex()) != 0)// CAN be -1!! Prevent recursion when removing item
			pItem->Holster();

		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}

	if (m_pLastItem == pItem)
		m_pLastItem = NULL;

	if (m_pNextItem == pItem)// XDM
		m_pNextItem = NULL;

	int ID = pItem->GetID();
	if (m_rgpWeapons[ID] == pItem || m_rgpWeapons[ID].Get() == NULL)// EXACTLY the same pointer! Not just same weapon ID XDM3035b: wtf?!?!
	{
		m_rgpWeapons[ID] = NULL;
		ClearBits(pev->weapons, 1<<ID);// XDM: moved from weapons like CWeaponHandGrenade // take item off hud
		pItem->DetachFromHost();
		return true;
	}
	else
	{
		DBG_FORCEBREAK
		conprintf(1, "CBasePlayer(%d)::RemovePlayerItem(%d)(id %d) MISMATCH!! ei %d\n", entindex(), pItem->entindex(), ID, m_rgpWeapons[ID]->entindex());
		pItem->DetachFromHost();
		//pItem->DestroyItem();
		return true;// HACK: to act as normal
	}
	//return false;
}

//-----------------------------------------------------------------------------
// Purpose: Drop specified item onto floor
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::DropPlayerItem(CBasePlayerItem *pItem)
{
	DBG_ITM_PRINT("CBasePlayer(%d)::DropPlayerItem(%d)(id %d)\n", entindex(), pItem?pItem->entindex():0, pItem?pItem->GetID():0);
	if (pItem == NULL)
		return false;
	if (IsMultiplayer() && (mp_weaponstay.value > 0.0f))
		return false;// no dropping allowed
	if (!CanDropItem(pItem))// XDM3038a
		return false;

	int	iAmmoIndex;
	bool bDroppedActiveItem;// XDM3038
	if (pItem == m_pActiveItem)
		bDroppedActiveItem = true;
	else
		bDroppedActiveItem = false;

	if (!RemovePlayerItem(pItem))// m_pActiveItem == NULL after this!
		return false;

	CBasePlayerWeapon *pDroppedWeapon = pItem->GetWeaponPtr();
	if (pDroppedWeapon)// here we decide how much ammunition should be dropped with this weapon
	{
		pDroppedWeapon->m_iDefaultAmmo = 0;// important
		iAmmoIndex = pDroppedWeapon->PrimaryAmmoIndex();//GetAmmoIndexFromRegistry(pItem->pszAmmo1());// safety?
		if (iAmmoIndex >= 0)// XDM3037: fixed. this weapon weapon uses ammo, so pack an appropriate amount.
		{
			int iPack;
			if (FBitSet(pDroppedWeapon->iFlags(), ITEM_FLAG_EXHAUSTIBLE))// pack up all the ammo, this weapon is its own ammo type
				iPack = m_rgAmmo[iAmmoIndex];
			else// pack half of the ammo
				iPack = (int)(m_rgAmmo[iAmmoIndex]/2);// now we won't lose any ammo due to type conversion

			pDroppedWeapon->PackAmmo(iPack, false);
			m_rgAmmo[iAmmoIndex] -= iPack;
		}
		iAmmoIndex = pDroppedWeapon->SecondaryAmmoIndex();//GetAmmoIndexFromRegistry(pItem->pszAmmo2());// safety?
		if (iAmmoIndex >= 0)// XDM3038c: secondary ammo too
		{
			int iPack;
			if (FBitSet(pDroppedWeapon->iFlags(), ITEM_FLAG_EXHAUSTIBLE))// pack up all the ammo
				iPack = m_rgAmmo[iAmmoIndex];
			else// pack half of the ammo
				iPack = (int)(m_rgAmmo[iAmmoIndex]/2);

			pDroppedWeapon->PackAmmo(iPack, true);
			m_rgAmmo[iAmmoIndex] -= iPack;
		}
	}
	// XDM3037a: even if it's not a CBasePlayerWeapon
	UTIL_MakeVectors(pev->v_angle);//ANGLE_VECTORS
	//ClearBits(pDroppedItem->pev->spawnflags, SF_ITEM_NOFALL);
	//SetBits(pDroppedItem->pev->spawnflags, SF_NORESPAWN);// should be already set
	pItem->pev->gravity = g_pWorld?g_pWorld->pev->gravity:1.0f;// XDM3038: HACK until global gravity policy is established
	pItem->pev->origin = GetGunPosition() + gpGlobals->v_forward * 36.0f;
	pItem->pev->movetype = MOVETYPE_TOSS;//pItem->FallInit();
	pItem->Materialize();
	pItem->pev->angles.y = pev->angles.y;
	pItem->pev->angles.z = 0.0f;
	pItem->pev->velocity = gpGlobals->v_forward * 30.0f + pev->velocity;// weaponbox has player's velocity, then some.
	//FAIL! pItem->pev->groupinfo = pev->groupinfo;// XDM3038c: to disable collisions with this player
	//UTIL_SetGroupTrace(pev->groupinfo, GROUP_OP_AND);// XDM3038c: TESTME!
	pItem->pev->owner = edict();// don't touch me!
	if (bDroppedActiveItem && IsAlive())// XDM3038c
		SelectNextBestItem(pItem);

	m_flNextAttack = UTIL_WeaponTimeBase() + 0.2f;// XDM3038a
	return true;// since RemovePlayerItem() worked
}

//-----------------------------------------------------------------------------
// Purpose: drop the named item, or if no name, the active item.
// Input  : *pszItemName - can be NULL
//-----------------------------------------------------------------------------
bool CBasePlayer::DropPlayerItem(const char *pszItemName)
{
	DBG_ITM_PRINT("CBasePlayer(%d)::DropPlayerItem(%s)\n", entindex(), pszItemName);

	if (pszItemName && strlen(pszItemName) == 0)// XDM3038b: NULL
		pszItemName = NULL;

	CBasePlayerItem *pWeapon = NULL;
	if (pszItemName)
	{
		CBasePlayerItem *pItem = NULL;
		for (size_t i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
		{
			pItem = GetInventoryItem(i);
			if (pItem)
			{
				if (strcmp(pItem->GetWeaponName(), pszItemName) == 0)// XDM3038a: this should NOT use g_ItemInfoArray!
				{
					pWeapon = pItem;
					break;
				}
			}
		}
	}
	else
		pWeapon = m_pActiveItem;// trying to drop active item

	return DropPlayerItem(pWeapon);// pWeapon may still be NULL
}

//-----------------------------------------------------------------------------
// Purpose: Pack up items flagged as quest/important only (not really pack, just drop)
//-----------------------------------------------------------------------------
uint32 CBasePlayer::PackImportantItems(void)
{
	SetBits(m_iGameFlags, EGF_DROPPEDITEMS);// XDM3038c
	CBasePlayerItem *pInvItem;
	uint32 n = 0;
	for (uint32 i = 0; i < PLAYER_INVENTORY_SIZE; ++i)// search for items that should never disappear!
	{
		pInvItem = GetInventoryItem(i);
		if (pInvItem)
		{
			if (pInvItem->iFlags() & ITEM_FLAG_IMPORTANT)
			{
				// Things may go wrong there: DropPlayerItem(pInvItem);
				ASSERT(RemovePlayerItem(pInvItem));// -> DetachFromHost
#if defined (_DEBUG)
				ASSERT(pInvItem->pev->impulse == ITEM_STATE_DROPPED);// XDM3038
				ASSERT(FBitSet(pInvItem->pev->spawnflags, SF_NORESPAWN));
#endif
				pInvItem->pev->movetype = MOVETYPE_TOSS;//pInvItem->FallInit();
				pInvItem->Materialize();
				pInvItem->pev->velocity = pev->velocity; pInvItem->pev->velocity *= 1.4f;
				pInvItem->pev->angles.y = pev->angles.y;
				pInvItem->pev->angles.z = 0.0f;
				pInvItem->pev->avelocity = pev->avelocity;// XDM3038
				pInvItem->m_flRemoveTime = 0;// don't remove
				++n;
			}
		}
	}
	return n;
}

//-----------------------------------------------------------------------------
// Purpose: Pack up the appropriate weapons and ammo items, and destroy anything that shouldn't be packed.
// UNDONE: this sould really be someting like this->TransferInventory(pWeaponBox);
// BUGBUG: satchels are packed AS AMMO when dropped through GR_PLR_DROP_AMMO_ALL (when not packed as weapon) so player who picks up the box won't get satchel weapon!
//-----------------------------------------------------------------------------
void CBasePlayer::PackDeadPlayerItems(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::PackDeadPlayerItems()\n", entindex());
	if (FBitSet(m_iGameFlags, EGF_DROPPEDITEMS))// XDM3038c
		return;

	// get the game rules
	int i, j;
	uint32 iNumImportantItems = 0;
	CBasePlayerItem *pInvItem;
	short iWeaponRules = g_pGameRules?g_pGameRules->DeadPlayerWeapons(this):GR_PLR_DROP_GUN_ACTIVE;
 	short iAmmoRules = g_pGameRules?g_pGameRules->DeadPlayerAmmo(this):GR_PLR_DROP_AMMO_ACTIVE;

	SetBits(m_iGameFlags, EGF_DROPPEDITEMS);// XDM3038c
	// XDM3038c: search for items that should never disappear!
	for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		pInvItem = GetInventoryItem(i);
		if (pInvItem)
		{
			if (pInvItem->iFlags() & ITEM_FLAG_IMPORTANT)
			{
				++iNumImportantItems;
				if (iWeaponRules != GR_PLR_DROP_GUN_ALL)// will not be packed into box, so drop it now! All of them!
				{
					// Things may go wrong there: DropPlayerItem(pInvItem);
					ASSERT(RemovePlayerItem(pInvItem));// -> DetachFromHost
#if defined (_DEBUG)
					ASSERT(pInvItem->pev->impulse == ITEM_STATE_DROPPED);// XDM3038
					ASSERT(FBitSet(pInvItem->pev->spawnflags, SF_NORESPAWN));
#endif
					pInvItem->pev->movetype = MOVETYPE_TOSS;//pInvItem->FallInit();
					pInvItem->Materialize();
					pInvItem->pev->velocity = pev->velocity; pInvItem->pev->velocity *= 1.4f;
					pInvItem->pev->angles.y = pev->angles.y;
					pInvItem->pev->angles.z = 0.0f;
					pInvItem->pev->avelocity = pev->avelocity;// XDM3038
					pInvItem->m_flRemoveTime = 0;// don't remove
				}
			}
		}
	}

	// nothing to pack. Remove the weapons and return. Don't call create on the box!
	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
		goto finish;// don't drop, just cleanup

	//CBasePlayerItem *pItemToDrop; TODO: make a cvar to decide which weapon to drop: current or best
	if (m_pActiveItem == NULL)// player was killed while changing weapons
		m_pActiveItem = m_pLastItem;

	// XDM3035b: drop real weapon, pack ammo into its clip. TODO: Move to DropPlayerItem()?
	if (iWeaponRules == GR_PLR_DROP_GUN_ACTIVE && iAmmoRules == GR_PLR_DROP_AMMO_ACTIVE)
	{
		if (m_pActiveItem)
		{
			CBasePlayerItem *pDroppedItem = m_pActiveItem;
			if (!CanDropItem(pDroppedItem))// XDM3037a: player is holding a weapon he cannot drop
			{
				pDroppedItem = g_pGameRules?g_pGameRules->GetNextBestWeapon(this, pDroppedItem):NULL;// try dropping other best weapon
				if (pDroppedItem)// found something
				{
					if (!CanDropItem(pDroppedItem))// cannot drop best item
					{
						pDroppedItem = NULL;
						CBasePlayerItem *pTempItem;
						for (i = WEAPON_SUIT-1; i > WEAPON_NONE; --i)// XDM3038: desperation could probably lead to endless NextBestWeapon loop, so just iterate numbers to zero
						{
							pTempItem = GetInventoryItem(i);
							if (pTempItem == NULL)// since we surely know that active item is not droppable
								continue;
							if (CanDropItem(pTempItem))
							{
								pDroppedItem = pTempItem;
								break;// found one!
							}
						}
					}
				}
			}
			if (pDroppedItem == NULL)// XDM3037a: nothing, at all
				goto finish;// give up safely

			if (RemovePlayerItem(pDroppedItem))// m_pActiveItem == NULL after this!
			{
				CBasePlayerWeapon *pDroppedWeapon = pDroppedItem->GetWeaponPtr();
				if (pDroppedWeapon)// actually a weapon
				{
					int	iAmmoIndex = GetAmmoIndexFromRegistry(pDroppedItem->pszAmmo1());
					if (iAmmoIndex >= 0)// this weapon uses ammo
					{
						pDroppedWeapon->m_iDefaultAmmo = 0;// important
						pDroppedWeapon->PackAmmo(m_rgAmmo[iAmmoIndex], false);//, true); don't lose the clip itself here
						m_rgAmmo[iAmmoIndex] = 0;
					}
					iAmmoIndex = GetAmmoIndexFromRegistry(pDroppedItem->pszAmmo2());
					if (iAmmoIndex >= 0)// this weapon uses secondary ammo
					{
						pDroppedWeapon->PackAmmo(m_rgAmmo[iAmmoIndex], true);
						m_rgAmmo[iAmmoIndex] = 0;
					}
				}
				// XDM3038a: this was a terrible mistake...
#if defined (_DEBUG)
				ASSERT(pDroppedItem->pev->impulse == ITEM_STATE_DROPPED);// XDM3038
				ASSERT(FBitSet(pDroppedItem->pev->spawnflags, SF_NORESPAWN));
#endif
				pDroppedItem->pev->movetype = MOVETYPE_TOSS;//pDroppedItem->FallInit();
				pDroppedItem->Materialize();
				pDroppedItem->pev->velocity = pev->velocity; pDroppedItem->pev->velocity *= 1.4f;
				pDroppedItem->pev->angles.y = pev->angles.y;
				pDroppedItem->pev->angles.z = 0.0f;
				pDroppedItem->pev->avelocity = pev->avelocity;// XDM3038
				if (IsMultiplayer())
				{
					pDroppedItem->pev->owner = edict();// XDM3038c: TESME fall through
					pDroppedItem->m_hOwner = this;// XDM3038c: TESME pickup policy

					if ((pDroppedItem->iFlags() & ITEM_FLAG_IMPORTANT) || (mp_droppeditemstaytime.value <= 0.0f) || (g_pGameRules && g_pGameRules->IsCoOp()))// XDM3038c
					{
						pDroppedItem->m_flRemoveTime = 0.0;// prevent
						//pDroppedItem->pev->takedamage = DAMAGE_NO;
					}
					else
						pDroppedItem->m_flRemoveTime = gpGlobals->time + mp_droppeditemstaytime.value;
				}
				++g_iWeaponBoxCount;// hack
				goto finish;// since RemovePlayerItem() worked, we can't go back
			}
		}
	}// (iWeaponRules == GR_PLR_DROP_GUN_ACTIVE && iAmmoRules == GR_PLR_DROP_AMMO_ACTIVE)

	{// <- prevent Compiler Error C2362: initialization of 'identifier' is skipped by 'goto label'
	int iPackAmmo[MAX_AMMO_SLOTS];// holds ammo indexes to pack
	CBasePlayerWeapon *rgpPackWeapons[PLAYER_INVENTORY_SIZE];// XDM3034: 32 hardcoded for now. How to determine exactly how many weapons we have?
	unsigned short iPW = 0;// number of weapons packed
	unsigned short iPA;// number of ammo packed

	memset(rgpPackWeapons, NULL, sizeof(rgpPackWeapons));
	for (iPA=0; iPA<MAX_AMMO_SLOTS; ++iPA)
		iPackAmmo[iPA] = AMMOINDEX_NONE;

	iPA = 0;// !
	// Go through ammo and make a list of which types to pack
	if (iAmmoRules == GR_PLR_DROP_AMMO_ACTIVE)
	{
		if (m_pActiveItem)// NOTE: ammo amount mey be 0 here
		{
			CBasePlayerWeapon *pWeapon = m_pActiveItem->GetWeaponPtr();
			if (pWeapon)
			{
				if (pWeapon->PrimaryAmmoIndex() >= 0) // this is the primary ammo type for the active weapon
					iPackAmmo[iPA++] = pWeapon->PrimaryAmmoIndex();

				if (pWeapon->SecondaryAmmoIndex() >= 0)// this is the secondary ammo type for the active weapon
					iPackAmmo[iPA++] = pWeapon->SecondaryAmmoIndex();
			}
		}
	}
	else if (iAmmoRules == GR_PLR_DROP_AMMO_ALL)// NOTE: if this "ammo" is useless because of EXHAUSTIBLE item, it will be removed from this list later
	{
		for (i = 0; i < MAX_AMMO_SLOTS; ++i)
		{
			if (m_rgAmmo[i] > 0)
				iPackAmmo[iPA++] = i;
		}
	}

	// Go through all of the weapons and make a list of the ones to pack
	if (iWeaponRules != GR_PLR_DROP_GUN_NO && !(iWeaponRules == GR_PLR_DROP_GUN_ACTIVE && m_pActiveItem == NULL))
	{
		for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
		{
			pInvItem = GetInventoryItem(i);
			if (pInvItem)
			{
				CBasePlayerWeapon *pItem = pInvItem->GetWeaponPtr();
				if (pItem == NULL)
					continue;
				if (pItem->GetID() == WEAPON_NONE)// XDM3035
				{
					DBG_PRINTF("CBasePlayer::PackDeadPlayerItems() ERROR! Got WEAPON_NONE for i = %d!\n", i);
					continue;
				}
				if (pItem->IsRemoving())
					continue;

				// Special case: exhausted weapon, we don't pack its ammo separately, but inside the weapon itself
				if (FBitSet(pItem->iFlags(), ITEM_FLAG_EXHAUSTIBLE))// DeactivateSatchels() must be called before this point!
				{
					for (j = 0; j < MAX_AMMO_SLOTS; ++j)// XDM3038c: exhaustible weapon, DO NOT pack any of its ammo! UNDONE: what if some non-exhaustible weapons share ammo with this one?!
					{
						if (iPackAmmo[j] != AMMOINDEX_NONE && (iPackAmmo[j] == pItem->PrimaryAmmoIndex() || iPackAmmo[j] == pItem->SecondaryAmmoIndex()))
						{
							iPackAmmo[j] = AMMOINDEX_NONE;
							--iPA;
						}
					}
					if (!pItem->IsUseable())
						continue;

					if (!pItem->HasAmmo(AMMO_ANYTYPE))
					{
						conprintf(1, "CBasePlayer(%d)::PackDeadPlayerItems(): error: ITEM_FLAG_EXHAUSTIBLE %s IsUseable() with no ammo!\n", pItem->GetWeaponName());
						// UNDONE: TESTME: deployed satchels? continue;
					}
					pItem->PackAmmo(AmmoInventory(pItem->PrimaryAmmoIndex()), false);
					pItem->PackAmmo(AmmoInventory(pItem->SecondaryAmmoIndex()), true);
					// don't have to if (PrimaryAmmoIndex() >= ?) m_rgAmmo[pItem->PrimaryAmmoIndex()] = 0;
				}

				if (!CanDropItem(pItem) || (iWeaponRules == GR_PLR_DROP_GUN_ACTIVE && pItem != m_pActiveItem))// firewalled by game rules, etc. OR it's not the only weapon we're dropping
					continue;// XDM3035

				rgpPackWeapons[iPW++] = pItem;
			}
		}
	}

	if (iPW > 0 || iPA > 0)// TODO: UNDONE: XDM3035: I don't really want to flood world with ammo-only weapon boxes! But others may want.
	{
		ASSERT(iPA < MAX_AMMO_SLOTS);
		ASSERT(iPW < PLAYER_INVENTORY_SIZE);
		CWeaponBox *pWeaponBox = CWeaponBox::CreateBox(this, pev->origin, pev->angles);
		if (pWeaponBox)
		{
			DBG_PRINTF("CBasePlayer(%d)::PackDeadPlayerItems(): packing %d weapons and %d ammo types\n", entindex(), iPW, iPA);
			int iAmmoAmount;
			for (i = 0; i < MAX_AMMO_SLOTS; ++i)// IMPORTANT: pack the AMMO first! // IMPORTANT: non-continuous list!
			{
				if (iPackAmmo[i] != AMMOINDEX_NONE)
				{
					iAmmoAmount = AmmoInventory(iPackAmmo[i]);
					if (!pWeaponBox->PackAmmo(g_AmmoInfoArray[iPackAmmo[i]].name, iAmmoAmount))// when dropping ammo of the active weapon, amount may be 0
					{
#if defined (_DEBUG)
						conprintf(1, "CBasePlayer(%d)::PackDeadPlayerItems() failed to pack ammo id %d (%d rounds) into box!\n", entindex(), iPackAmmo[i], iAmmoAmount);
#endif
					}
				}
			}
			for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)// now pack all of the WEAPONS in the list
			{
				if (rgpPackWeapons[i])
				{
					if (RemovePlayerItem(rgpPackWeapons[i]))// clear out from player's inventory, but don't destroy it!
					{
						if (!pWeaponBox->PackWeapon(rgpPackWeapons[i]))
						{
#if defined (_DEBUG)
							conprintf(1, "CBasePlayer(%d)::PackDeadPlayerItems() failed to pack weapon[%d] id %d into box!\n", entindex(), rgpPackWeapons[i]->entindex(), rgpPackWeapons[i]->GetID());
#endif
							rgpPackWeapons[i]->Killed(this, this, GIB_NEVER);// XDM3038a: IMPORTANT! Box may fail to store some items!
							rgpPackWeapons[i] = NULL;
						}
					}
					else {DBG_PRINTF("CBasePlayer(%d)::PackDeadPlayerItems(): error: weapon id %d failed to detach from player!\n", entindex(), rgpPackWeapons[i]->GetID());}
				}
			}
			if (IsMultiplayer())
			{
				if ((iNumImportantItems > 0) || (mp_droppeditemstaytime.value <= 0.0f) || (g_pGameRules && g_pGameRules->IsCoOp()))// XDM3038c
				{
					pWeaponBox->m_flRemoveTime = 0.0;// NEVER remove: contains important items!
					pWeaponBox->pev->takedamage = DAMAGE_NO;
				}
				else
					pWeaponBox->m_flRemoveTime = gpGlobals->time + mp_droppeditemstaytime.value;
			}
#if defined (_DEBUG)
			pWeaponBox->ValidateContents();
#endif
		}
		else
			conprintf(1, "CBasePlayer(%d)::PackDeadPlayerItems() ERROR! Unable to create a weaponbox!\n", entindex());
	}
	}// <- prevent Compiler Error C2362
finish:
	//m_pActiveItem = NULL;// don't holster, just remove
	RemoveAllItems(true, true);// now strip off everything that wasn't handled by the code above.
}

//-----------------------------------------------------------------------------
// Purpose: Clear inventory array and other containers
// Input  : bRemoveSuit - 
//			bRemoveImportant - 
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveAllItems(bool bRemoveSuit, bool bRemoveImportant)
{
	DBG_ITM_PRINT("CBasePlayer(%d)::RemoveAllItems()\n", entindex());

	m_pLastItem = NULL;
	m_pNextItem = NULL;

	if (m_pTank != NULL)// HL20130901
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	short i = 0;
	for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		CBasePlayerItem *pItem = GetInventoryItem(i);
		if (pItem && (bRemoveImportant || !(pItem->iFlags() & ITEM_FLAG_IMPORTANT)))// XDM3038c
		{
			if (pItem == m_pActiveItem)// XDM3037a: now we make sure active item is in the inventory and gets removed
			{
				ResetAutoaim();
				m_pActiveItem = NULL;
			}
			pItem->DetachFromHost();// testme
			pItem->Killed(this, this, GIB_NEVER);// XDM3038a: changed from 'Drop()', this will call RemovePlayerItem() here
			m_rgpWeapons[i] = NULL;
		}
	}
	ASSERT(m_pActiveItem == NULL);

	pev->viewmodel = iStringNull;
	pev->weaponmodel = iStringNull;

	if (bRemoveSuit)
		pev->weapons = 0;// in case we add some other non-weapon entries: ClearBits(pev->weapons, WEAPON_ALLWEAPONS|WEAPON_SUIT);
	else
		ClearBits(pev->weapons, WEAPON_ALLWEAPONS);

	for (i = 0; i < MAX_AMMO_SLOTS; ++i)
		m_rgAmmo[i] = 0;

	// XDM3037a: TESTME! A bad place for this!	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame by the player PreThink
//-----------------------------------------------------------------------------
void CBasePlayer::ItemPreFrame(void)
{
	CalcShootSpreadFactor();// do it once per frame
	// this prevents player from shooting while changing weapons
	if (m_flNextAttack > UTIL_WeaponTimeBase())
		return;

	if (m_pActiveItem == NULL)// XDM
	{
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (m_pNextItem)// we have some item queued for selection
		{
			if (m_pNextItem->GetID() > WEAPON_NONE)// XDM3035a: HACKFIX: somehow bad items get here
			{
				if (CanDeployItem(m_pNextItem))// XDM3038a
				{
					m_pActiveItem = m_pNextItem;
					//m_pActiveItem->SetOwner(this);// HACKFIX?
					DeployActiveItem();// XDM
					m_pNextItem = NULL;
				}
			}
			else
				conprintf(1, "CBasePlayer::ItemPreFrame(): Bad next item!\n");
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		DBG_PRINTF("*** CBasePlayer::ItemPreFrame() exception!\n");
		DBG_FORCEBREAK
	}
#endif
	}
	else
		m_pActiveItem->ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame by the player PostThink
//-----------------------------------------------------------------------------
void CBasePlayer::ItemPostFrame(void)
{
	if (m_pTank)// XDM3037: prevents any weapon-related code and unwanted un-holstering
		return;

	//pev->fuser1 = 0;// XDM3038c
	//bool bHaveSpread = false;
	if (m_pActiveItem)
	{
		if (g_pGameRules && !g_pGameRules->FAllowShootingOnLadder(this))
		{
			if (IsOnLadder())
			{
				if (!m_pActiveItem->IsHolstered())
				{
#if defined (_DEBUG_ITEMS)
					conprintf(2, "Client %d: %s Holster()\n", entindex(), STRING(m_pActiveItem->pev->classname));
#endif
					m_pActiveItem->Holster();
				}
				else// Holster() sets m_flNextAttack
					m_flNextAttack = UTIL_WeaponTimeBase() + 0.4f;
			}
			else
			{
				if (/*already checked m_pTank == NULL && */m_pTrain.Get() == NULL)// not just IsOnTrain()
				{
					// we already know that IsOnLadder() == false; bool changed_ladder = (IsOnLadder() != FBitSet(m_afPhysicsFlags, PFLAG_ONLADDER));// XDM3038a
					bool changed_ladder = FBitSet(m_afPhysicsFlags, PFLAG_ONLADDER);
					if (changed_ladder && m_pActiveItem->IsHolstered())
					{
#if defined (_DEBUG_ITEMS)
						conprintf(2, "Client %d: %s Deploy()\n", entindex(), STRING(m_pActiveItem->pev->classname));
#endif
						DeployActiveItem();// XDM3038a: don't call Deploy() directly!
/*#if defined (_DEBUG_ITEMS)
						if (!m_pActiveItem->Deploy())
							conprintf(2, "Client %d: %s fails to Deploy() after ladder!\n", entindex(), STRING(m_pActiveItem->pev->classname));
#else
						m_pActiveItem->Deploy();
#endif*/
					}
				}
			}
		}
		if (m_pActiveItem)// XDM3038a: somehow it may change
		{
			m_pActiveItem->ItemPostFrame();
			//bHaveSpread = true;
			if (fabs(m_fShootSpreadFactor - pev->fuser1) > 0.05f)// don't send updates too frequently
				pev->fuser1 = m_fShootSpreadFactor;// send to client side
		}
		else
			pev->fuser1 = 0;// XDM3038c
	}
	else if (m_pNextItem == NULL)// XDM3038a: no current and no queued items
	{
		if (!IsOnLadder() || g_pGameRules == NULL || g_pGameRules->FAllowShootingOnLadder(this))// XDM3038
		{
			if (m_flNextAttack <= gpGlobals->time)// XDM3035: monitor empty hands, but try to not to flood the net
			{
				size_t i = 0;
				bool bFoundWeapon = false;
				for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)// XDM3037a: flood prevention
				{
					CBasePlayerItem *pInvItem = GetInventoryItem(i);// XDM3038a
					if (pInvItem && pInvItem->CanDeploy() && CanDeployItem(pInvItem))// && pInvItem != m_pActiveItem) <- that's NULL // XDM3038: prevents unnecessary attempts
					{
						bFoundWeapon = true;// found something usable, doesn't matter how good it is
						break;
					}
				}
				if (bFoundWeapon)// have something to select (no matter what)
					SelectNextBestItem(NULL);// call this AFTER sending weapon data! (client message)

				m_flNextAttack = UTIL_WeaponTimeBase() + 4.0f;
			}
		}
		pev->fuser1 = 0;// XDM3038c
	}
	//if (!bHaveSpread)
	//	pev->fuser1 = 0;// XDM3038c
	if (pev->fov == 0.0f || pev->fov == DEFAULT_FOV)
		ResetAutoaim();// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Tracks changes of ammo inventory and sends changed data to client side.
// XDM3035: combine into one packet/message
// Called from UpdateClientData
//-----------------------------------------------------------------------------
void CBasePlayer::SendAmmoUpdate(void)
{
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		size_t i = 0;
		size_t iSize = 0;
#ifdef DOUBLE_MAX_AMMO
		uint16 value;
#else
		byte value;
#endif
		char pBuffer[MAX_AMMO_SLOTS*(1+sizeof(value)) + 2];// XDM3038a
		memset(pBuffer, NULL, sizeof(pBuffer));

		for (i=0; i < MAX_AMMO_SLOTS; ++i)
		{
			if (m_rgAmmo[i] != m_rgAmmoLast[i])
			{
				ASSERT(m_rgAmmo[i] >= 0);// && m_rgAmmo[i] < 255);
				//if (m_rgAmmo[i] < 0) conprintf(1, "* error: Player %s has %d ammo of type %s!\n", STRING(pev->netname), m_rgAmmo[i], g_AmmoInfoArray[i].pszName);
				// ammo index
				value = i;
				memcpy(pBuffer+iSize, &value, sizeof(byte)); ++iSize;
				// ammo amount
				value = max(min(m_rgAmmo[i], MAX_AMMO), 0);
				memcpy(pBuffer+iSize, &value, sizeof(value)); iSize+=sizeof(value);
				m_rgAmmoLast[i] = m_rgAmmo[i];
				//conprintf(2, "CBasePlayer::SendAmmoUpdate(): sending: %d %d\n", i, m_rgAmmo[i]);
			}
		}

		if (iSize > 0)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgUpdAmmo, NULL, edict());
				//WRITE_STRING(pBuffer); nobody trusts valve :( Also, bot code relies on separate WRITE_BYTEs
			for (i = 0; i < iSize; ++i)
				WRITE_BYTE(pBuffer[i]);// write buffer byte by byte even if the data size differ

			MESSAGE_END();
			//conprintf(2, "CBasePlayer::SendAmmoUpdate(): sent %d bytes\n", iSize);
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		DBG_PRINTF("*** CBasePlayer::SendAmmoUpdate() exception!\n");
		DBG_FORCEBREAK
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Send base weapon information (name, ammo type, etc.) and the inventory
//-----------------------------------------------------------------------------
void CBasePlayer::SendWeaponsUpdate(void)
{
	if (!m_fInventoryChanged)// sends weapon factory data, NOT THE INVENTORY DATA!!!
	{
		SendWeaponsRegistry(this);
		m_fInventoryChanged = TRUE;
	}

	// XDM3035c: this part was moved here
	//if (m_iLocalWeapons == 0)// || sv_oldweaponupdate.value > 0)// XDM3038a: otherwise GetWeaponData() comes into play
	{
		int i = 0;
		size_t bufferlen = 0;
#if defined (USE_EXCEPTIONS)
		try
		{
#endif // USE_EXCEPTIONS
			// Update all the items
			//int sent = 0;
			CBasePlayerItem *pItem = (CBasePlayerItem *)BADPOINTER;
			char pBuffer[PLAYER_INVENTORY_SIZE*3 + 2];// WARNING! If protocol ever changes, change this buffer size!
			memset(pBuffer, 0, sizeof(pBuffer));
			for (i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
			{
#if defined (USE_EXCEPTIONS)
				try
				{
#endif // USE_EXCEPTIONS
					pItem = GetInventoryItem(i);
					if (pItem)
						bufferlen += pItem->UpdateClientData((char *)(pBuffer + bufferlen));// XDM3035: combined multiple mesages into single
#if defined (USE_EXCEPTIONS)
				}
				catch (...)
				{
					DBG_PRINTF("*** CBasePlayer::SendWeaponsUpdate() exception in UpdateClientData()! (i = %d, bufferlen = %u)\n", i, bufferlen);
					DBG_FORCEBREAK
				}
#endif // USE_EXCEPTIONS
			}
			if (bufferlen > 0)
			{
				ASSERT(bufferlen <= MAX_USER_MSG_DATA);
				MESSAGE_BEGIN(MSG_ONE, gmsgUpdWeapons, NULL, edict());// XDM3035: pack all updates into a single packet!
				for (size_t j = 0; j < bufferlen; ++j)
					WRITE_BYTE(pBuffer[j]);

				MESSAGE_END();
				DBG_ITM_PRINT("CBasePlayer(%d)::SendWeaponsUpdate(): sent %d bytes\n", entindex(), bufferlen);
				m_pClientActiveItem = m_pActiveItem;// XDM3037
			}
#if defined (USE_EXCEPTIONS)
		}
		catch (...)
		{
			DBG_PRINTF("*** CBasePlayer::SendWeaponsUpdate() exception! (i = %d, bufferlen = %u)\n", i, bufferlen);
			DBG_FORCEBREAK
		}
#endif // USE_EXCEPTIONS
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: cycle through all entities that need to SendClientData()
// WARNING: This may cause datagram overflow, so don't send all data in one frame!
// m_flInitEntitiesTime slows down data transmission
// NOTE: this system does not consider PVS so all players recieve packets from all entities,
// but this happens rarely and keeps data well synchronized.
//-----------------------------------------------------------------------------
void CBasePlayer::SendEntitiesData(void)
{
	if (m_iInitEntities > 0 && m_flInitEntitiesTime <= gpGlobals->time)
	{
		edict_t *pEdict = (edict_t *)BADPOINTER;//INDEXENT(m_iInitEntityLast);
		short sendcase;// XDM3038b: decide clearly for what reason we're requesting an update
		if (FBitSet(gpGlobals->serverflags, FSERVER_RESTORE))
			sendcase = SCD_CLIENTRESTORE;
		else if (m_iInitEntities == 1)//(m_iSpawnState < SPAWNSTATE_INITIALIZED)
			sendcase = SCD_CLIENTCONNECT;
		else
			sendcase = SCD_CLIENTUPDATEREQUEST;

		do// some entindexes may be free or some entities send no data, so we may do many iterations here to save time
		{
			pEdict = INDEXENT(m_iInitEntity);
			++m_iInitEntity;// we don't need this anywhere else here, so increase immediately
			if (UTIL_IsValidEntity(pEdict))
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
				if (pEntity && !pEntity->IsPlayer())// XDM3035c: not players
				{
					if (pEntity->SendClientData(this, MSG_ONE, sendcase) > 0)
					{
						//DBG_PRINTF("CBasePlayer(%d)::SendClientData(%d) %s %s sent\n", entindex(), m_iInitEntity-1, STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname));
						break;// data was sent, exit and wait for next UpdateClientData()
					}// otherwise entity doesn't need to send anything
				}
			}
		} while (m_iInitEntity < gpGlobals->maxEntities);

		if (m_iInitEntity >= gpGlobals->maxEntities)// last index, finished.
		{
			m_iInitEntities = 0;// disable
			m_iInitEntity = 0;
		}
		else
			m_flInitEntitiesTime = gpGlobals->time + sv_entinitinterval.value;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resends any changed player HUD info to the client. Called every frame by PlayerPreThink
// Also called at start of demo recording and playback by ForceClientDllUpdate to ensure the demo gets messages reflecting all of the HUD state info.
//-----------------------------------------------------------------------------
void CBasePlayer::UpdateClientData(void)
{
	int myindex = entindex();

	if (g_ClientShouldInitialize[myindex] > 0)// XDM3035a
	{
		m_fInitHUD = TRUE;// XDM3037: instead of gInitHUD
		m_iInitEntities = 1;// this should NEVER happen twice per map!
		m_iInitEntity = 0;// XDM3035c: include world
		m_flInitEntitiesTime = gpGlobals->time + gpGlobals->frametime + 0.5f;// XDM: IMPORTANT!!! HL will crash if we send too much in one frame
		g_ClientShouldInitialize[myindex] = 0;
	}

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
	if (m_fInitHUD)// once per spawn
	{
		m_fInitHUD = FALSE;
		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, edict());
		MESSAGE_END();

		if (!m_fGameHUDInitialized)// this section is executed only once per map
		{
			SendAmmoRegistry(this);
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, edict());// XDM: this also clears client fog
			MESSAGE_END();

			FlashlightUpdate(FlashlightIsOn());// XDM3034: update both state and battery

			if (g_pGameRules)
				g_pGameRules->InitHUD(this);

			m_iObserverLastMode = OBS_ROAMING;// HL20130901
			m_iSpawnState = SPAWNSTATE_INITIALIZED;
			m_iHUDDistortUpdate	= 1;// XDM3038a
			m_fGameHUDInitialized = TRUE;
		}
		InitStatusBar();
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		if (g_pGameRules && g_pGameRules->FAllowFlashlight())
			ClearBits(m_iHideHUD, HIDEHUD_FLASHLIGHT);
		else
			SetBits(m_iHideHUD, HIDEHUD_FLASHLIGHT);

		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, edict());
			WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();
		m_iClientHideHUD = m_iHideHUD;
	}

	if ((int)pev->armorvalue != m_iClientBattery)// update armor
	{
		m_iClientBattery = (int)pev->armorvalue;
		//ASSERT(gmsgBattery > 0);
		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, NULL, edict());
			WRITE_BYTE(m_iClientBattery);// XDM3038: was WRITE_SHORT, changed to byte because max.battery is sent via gmsgGameMode.
		MESSAGE_END();
	}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		DBG_PRINTF("*** CBasePlayer::UpdateClientData() exception 1!\n");
		DBG_FORCEBREAK
	}
#endif

	if (!IsObserver())// XDM
	{
		// update damage
		if (pev->dmg_take/* || pev->dmg_save*/ || m_bitsHUDDamage != m_bitsDamageType)
		{
			Vector damageOrigin(pev->origin);// when direction undetermined
			if (pev->dmg_inflictor)
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pev->dmg_inflictor);
				if (pEntity)
					damageOrigin = pEntity->Center();
			}
			int visibleDamageBits = m_bitsDamageType & (DMGM_SHOWNHUD | DMGM_VISIBLE);
			if (FBitSet(m_bitsDamageType, DMGM_DISARM))// XDM3038a: 20150713
				SetBits(visibleDamageBits, DMG_CRUSH);// hack to display "disarmed" icon

			if (visibleDamageBits != 0)// XDM3038a: only send types that have visible effects or HUD indication
			{
				byte a = m_LastHitGroup & 0x000000FF;
				if (m_fFrozen)// add frozen bit
					a |= 0x80;

				MESSAGE_BEGIN(MSG_ONE, gmsgDamage, NULL, edict());
					WRITE_BYTE(a);//pev->dmg_save);// XDM3037: hitgroup
					//WRITE_BYTE(pev->dmg_take);// damageTaken
					WRITE_LONG(visibleDamageBits);// bitsDamage
					WRITE_COORD(damageOrigin.x);// vecFrom
					WRITE_COORD(damageOrigin.y);
					WRITE_COORD(damageOrigin.z);
				MESSAGE_END();
			}
			pev->dmg_take = 0;
			pev->dmg_save = 0;
			// Clear off non-time-based damage indicators
			m_bitsDamageType &= DMGM_TIMEBASED;
			m_bitsHUDDamage = m_bitsDamageType;// XDM3037: moved here to avoid redundant messages
		}

		// Update Flashlight
		if ((m_flFlashLightTime > 0.0f) && (m_flFlashLightTime <= gpGlobals->time))
		{
			if (FlashlightIsOn())
			{
				if (m_fFlashBattery > 0.0f)
				{
					m_fFlashBattery -= FLASHLIGHT_UPDATE_INTERVAL * gSkillData.FlashlightDrain;
					if (m_fFlashBattery <= 0.0f)
					{
						m_fFlashBattery = 0.0f;
						FlashlightTurnOff();
					}
				}
				m_flFlashLightTime = gpGlobals->time + FLASHLIGHT_UPDATE_INTERVAL;// update in any case
			}
			else
			{
				if (m_fFlashBattery < MAX_FLASHLIGHT_CHARGE)
				{
					m_fFlashBattery += FLASHLIGHT_UPDATE_INTERVAL * gSkillData.FlashlightCharge;
					if (m_fFlashBattery > MAX_FLASHLIGHT_CHARGE)
						m_fFlashBattery = MAX_FLASHLIGHT_CHARGE;

					m_flFlashLightTime = gpGlobals->time + FLASHLIGHT_UPDATE_INTERVAL;
				}
				else
					m_flFlashLightTime = 0.0f;// fully charged, disable updates
			}
			FlashlightUpdate(FlashlightIsOn());// send the message
		}

		if (FBitSet(m_iTrain, TRAIN_NEW))
		{
			ASSERT(gmsgTrain > 0);
			MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, edict());
				WRITE_BYTE(m_iTrain);// & 0xFF);// XDM3038a: anything beyond 1 byte is not sent anyway
			MESSAGE_END();
			ClearBits(m_iTrain, TRAIN_NEW);
		}

		// XDM3038: HUD effects
		if (m_iHUDDistortUpdate > 0)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgHUDEffects, NULL, edict());
				WRITE_BYTE(m_iHUDDistortMode);
				WRITE_BYTE(m_iHUDDistortValue);
			MESSAGE_END();
			m_iHUDDistortUpdate = 0;
		}
		
		SendAmmoUpdate();// XDM3035c: combined update for all ammo
		SendWeaponsUpdate();// Send weapon registry (if needed) and inventory
	}// !IsObserver()

	if (m_iCurrentMusicTrackCommandIssued == 0 && m_iCurrentMusicTrackStartTime <= gpGlobals->time)// XDM3038c: can be delayed
	{
		if (m_iCurrentMusicTrackCommand != MPSVCMD_RESET)// server-only
		{
		MESSAGE_BEGIN(MSG_ONE, gmsgAudioTrack, NULL, edict());
			WRITE_BYTE(m_iCurrentMusicTrackCommand);
			WRITE_SHORT((int)(gpGlobals->time - m_iCurrentMusicTrackStartTime));// +-32767sec = 546mins ~= 9h
			WRITE_STRING(m_szCurrentMusicTrack);// must allow empty string
		MESSAGE_END();
		}
		// command, track and time must stay preserved to be saved
		m_iCurrentMusicTrackCommandIssued = 1;
	}

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		SendEntitiesData();
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		DBG_PRINTF("*** CBasePlayer::UpdateClientData() exception in SendEntitiesData()!\n");
		DBG_FORCEBREAK
	}
#endif

	//XDM3037a	m_iClientFOV = m_iFOV;

	if (g_pGameRules && g_pGameRules->IsMultiplayer())// XDM3035: SAVE CPU! Not for bots
		UpdateStatusBar();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: update room type from local point of view
// Input  : *pEmitter - 
//			iRoomtype - 
//			flRange - 
// Output : Returns true if room type was changed.
//-----------------------------------------------------------------------------
bool CBasePlayer::UpdateSoundEnvironment(CBaseEntity *pEmitter, const int iRoomtype, const float flRange)
{
	//conprintf(1, "CBasePlayer(%d)::UpdateSoundEnvironment(%d %d %g)\n", entindex(), pEmitter?pEmitter->entindex():0, iRoomtype, flRange);
	if (pEmitter == NULL)
		pEmitter = g_pWorld;// by default, emitter is the world

	int iNewRoomType = iRoomtype;
	if (pEmitter)
	{
		m_hEntSndLast = pEmitter;
		if (g_pWorld && pEmitter == g_pWorld && g_pWorld->pev != NULL)// copy world's custom room type
		{
			iNewRoomType = g_pWorld->m_iRoomType;
			m_flSndRange = 0.0f;
		}
		else
			m_flSndRange = flRange;
	}
	else// wtf? world is invalid?
	{
		m_hEntSndLast = NULL;
		m_flSndRange = 0.0f;// reset
	}

	if (m_flSndRoomtype != iNewRoomType)
	{
		MESSAGE_BEGIN(MSG_ONE, svc_roomtype, NULL, edict());
			WRITE_SHORT(iNewRoomType);
		MESSAGE_END();
		//conprintf(1, "CBasePlayer(%d)::UpdateSoundEnvironment() CHANGED from %d to %d\n", entindex(), m_flSndRoomtype, iNewRoomType);
		m_flSndRoomtype = iNewRoomType;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Overridden for the player to set the proper physics flags when a barnacle grabs player.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::FBecomeProne(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::FBecomeProne()\n", entindex());
	if (CBaseMonster::FBecomeProne())
	{
		m_afPhysicsFlags |= PFLAG_ONBARNACLE;
		m_flFallVelocity = 0.0f;
		TrainDetach();// XDM3035
		DisableLadder(2.0);// XDM3038c
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called by Barnacle victims when the barnacle pulls their head into its mouth.
// Input  : *pBarnacle - 
//-----------------------------------------------------------------------------
void CBasePlayer::BarnacleVictimBitten(CBaseEntity *pBarnacle)
{
	if (!IsAlive())// Or the player will stay frozen and unable to respawn/load game!
	{
		Killed(pBarnacle, pBarnacle->GetDamageAttacker(), GIB_ALWAYS);
		return;
	}

	if (FBitSet(pev->flags, FL_DUCKING))
		ClearBits(pev->flags, FL_DUCKING);

	TrainDetach();// XDM3035: Double check. Just in case.
	DisableLadder(1.0);// XDM3038c
	m_hEnemy = pBarnacle;// XDM3038c: a little hack
	UTIL_ScreenFade(this, Vector(191,0,0), 1.0, 0.1, 160, FFADE_IN);
}

//-----------------------------------------------------------------------------
// Purpose: Overridden for player who has physics flags concerns.
//-----------------------------------------------------------------------------
void CBasePlayer::BarnacleVictimReleased(void)
{
	//UTIL_ScreenFade(this, Vector(100,0,0), 1.0, 0.5, 80, FFADE_IN);// XDM
	CBaseMonster::BarnacleVictimReleased();// XDM3038c
	ClearBits(m_afPhysicsFlags, PFLAG_ONBARNACLE);
	pev->movetype = MOVETYPE_WALK;
	pev->fixangle = FALSE;
	m_flNextAttack = UTIL_WeaponTimeBase();
	DisableLadder(0.0);// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Return player light level plus virtual muzzle flash
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::Illumination(void)
{
	int iIllum = CBaseMonster::Illumination() + m_iWeaponFlash;// XDM3035c: parent class
	if (iIllum > 255)
		return 255;
	return iIllum;
}

//-----------------------------------------------------------------------------
// Purpose: Ignore dropped items
// Output : int 1
//-----------------------------------------------------------------------------
int CBasePlayer::ShouldCollide(CBaseEntity *pOther)
{
	if (pOther->IsPlayerItem())
	{
		if (pOther->pev->owner == edict())
			return 0;
	}
	return CBaseMonster::ShouldCollide(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: Network decision
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::ShouldBeSentTo(CBasePlayer *pClient)
{
	if (m_iSpawnState < SPAWNSTATE_SPAWNED)// XDM3038c: prevent connecting players from showing at (0,0,0)
		return false;

	return CBaseMonster::ShouldBeSentTo(pClient);
}

//-----------------------------------------------------------------------------
// Purpose: Calculate dynamic shooting spread based on various factors
//-----------------------------------------------------------------------------
void CBasePlayer::CalcShootSpreadFactor(void)
{
	if (gSkillData.iSkillLevel == SKILL_EASY)
		m_fShootSpreadFactor = SPREADFACTOR_DISABLED;
	else
	{
		vec_t speed = pev->velocity.Length();
		if (FBitSet(pev->flags, FL_DUCKING))
		{
			if (speed == 0)
				m_fShootSpreadFactor = SPREADFACTOR_CROUCH_IDLE;
			else
				m_fShootSpreadFactor = SPREADFACTOR_CROUCH_MOVING;
		}
		else if (speed > PLAYER_MAX_WALK_SPEED)// running
			m_fShootSpreadFactor = SPREADFACTOR_RUNNING;
		else if (speed > 0.0f)// walking
			m_fShootSpreadFactor = SPREADFACTOR_WALKING;// Good, but generates lots of traffic: m_fShootSpreadFactor = SPREADFACTOR_STANDING + (SPREADFACTOR_WALKING-SPREADFACTOR_STANDING) * (speed/PLAYER_MAX_WALK_SPEED);
		else// standing
			m_fShootSpreadFactor = SPREADFACTOR_STANDING;
	}
	if (!FBitSet(pev->flags, FL_ONGROUND))
		m_fShootSpreadFactor += SPREADFACTOR_INAIR;

	if (m_pActiveItem && m_pActiveItem->IsPlayerWeapon())// GetWeaponPtr())
	{
		float fTimeSinceAttack = UTIL_WeaponTimeBase() - m_pActiveItem->GetLastUseTime();
		if (fTimeSinceAttack > 0.0f && fTimeSinceAttack < SPREAD_ATTACK_DECAY_TIME)
			m_fShootSpreadFactor += (1.0f - fTimeSinceAttack/SPREAD_ATTACK_DECAY_TIME) * SPREADFACTOR_AFTERSHOT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set crosshair position to point to enemey
// Warning: SUPER SLOW!!!!!!!!!!
// Input  : flDelta - 
// Output : direction, not angles!
//-----------------------------------------------------------------------------
Vector CBasePlayer::GetAutoaimVector(const float &flDelta)
{
	if (!m_pActiveItem || !m_pActiveItem->ForceAutoaim())// XDM: always check m_pActiveItem!!
	{
		UTIL_MakeVectors(pev->v_angle + pev->punchangle);
		return gpGlobals->v_forward;
	}

	Vector vecSrc(GetGunPosition());
	//bool m_fOldTargeting = m_fOnTarget;
	Vector angles(AutoaimDeflection(vecSrc, g_psv_zmax->value, flDelta));

	//if (m_fOldTargeting != m_fOnTarget)
	// XDM3038	m_pActiveItem->UpdateItemInfo();

	NormalizeAngle180(&angles.x);// XDM3037a
	NormalizeAngle180(&angles.y);

	if (angles.x > 25)
		angles.x = 25;
	else if (angles.x < -25)// XDM3038a: else
		angles.x = -25;

	if (angles.y > 12)
		angles.y = 12;
	else if (angles.y < -12)
		angles.y = -12;

	m_vecAutoAim = angles * 0.9f;

	// Don't send across network if sv_aim is 0
	//if (m_pActiveItem->ForceAutoaim() /*|| g_psv_aim->value > 0 */)
	//if (m_vecAutoAim.x != m_lastx || m_vecAutoAim.y != m_lasty)
	if (m_vecAutoAim != m_vecAutoAimPrev)
	{
		SET_CROSSHAIRANGLE(edict(), -m_vecAutoAim.x, m_vecAutoAim.y);
		m_vecAutoAimPrev = m_vecAutoAim;
		//m_lastx = m_vecAutoAim.x;
		//m_lasty = m_vecAutoAim.y;
	}
	//conprintf(1, "%f %f\n", angles.x, angles.y);
	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);
	return gpGlobals->v_forward;
}

//-----------------------------------------------------------------------------
// Purpose: Main aiming code
// Note   : When maxEntities went over 4000 old code became useless.
// Input  : &vecSrc - 
//			flDist - 
//			flDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CBasePlayer::AutoaimDeflection(const Vector &vecSrc, const float &flDist, const float &flDelta)
{
	CBaseEntity	*pEntity = NULL;
	Vector bestdir(gpGlobals->v_forward);
	TraceResult tr;

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);

	//conprintf(1, "AutoaimDeflection\n");
	// try all possible entities
	m_fOnTarget = false;

	// We have acquired target last frame, check it
	if (m_hAutoaimTarget.Get())
	{
		pEntity = m_hAutoaimTarget;
		Vector vecCenter(pEntity->BodyTarget(vecSrc));
		Vector dir((vecCenter - vecSrc).Normalize());
		vec_t dot = fabs(DotProduct(dir, gpGlobals->v_right)) + fabs(DotProduct(dir, gpGlobals->v_up)) * 0.5f;
		dot *= 1.0f + 0.2f * ((vecCenter - vecSrc).Length() / flDist);
		if (dot <= flDelta)
		{
			m_fOnTarget = true;
			return dir;
		}
		else// went out of required angle
		{
			m_hAutoaimTarget = NULL;
			//m_fOnTarget = false;
		}
	}
	// find a new target (direct trace only)
	UTIL_TraceLine(vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, ignore_glass, edict(), &tr);
	if (tr.pHit)
	{
		pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity != NULL)
		{
			if (pEntity->pev->takedamage == DAMAGE_AIM && pEntity->IsAlive())// basically aimable
			{
				// don't look through water
				if (!((pev->waterlevel < WATERLEVEL_HEAD && tr.pHit->v.waterlevel >= WATERLEVEL_HEAD) || (pev->waterlevel >= WATERLEVEL_HEAD && tr.pHit->v.waterlevel == WATERLEVEL_NONE)))
				{
					if (g_pGameRules == NULL || g_pGameRules->FShouldAutoAim(this, pEntity))// don't shoot at friends
					{
						m_hAutoaimTarget = pEntity;
						m_fOnTarget = true;
						return bestdir;// No autoaim for now!
					}
				}
			}
		}
	}
	//conprintf(1, "AutoaimDeflection: NO BEST ENT!\n");
	m_hAutoaimTarget = NULL;
	return g_vecZero;
}

//-----------------------------------------------------------------------------
// Purpose: Reset autoaim
//-----------------------------------------------------------------------------
void CBasePlayer::ResetAutoaim(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::ResetAutoaim()\n", entindex());
	if (m_vecAutoAim.x != 0.0f || m_vecAutoAim.y != 0.0f)
	{
		m_vecAutoAim.Clear();
		SET_CROSSHAIRANGLE(edict(), 0, 0);// this is a client message, don't flood with it
	}
	m_fOnTarget = false;
	m_hAutoaimTarget = NULL;// XDM
}

//-----------------------------------------------------------------------------
// Purpose: UNDONE: Determine real frame limit, 8 is a placeholder.
// Note   : -1 means no custom frames present.
// Input  : nFrames - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 && nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the # of custom frames this player's custom clan logo contains.
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::GetCustomDecalFrames(void)
{
	return m_nCustomSprayFrames;
}

//-----------------------------------------------------------------------------
// Purpose: Does the player already have EXACTLY THIS INSTANCE of item?
// Input  : *pCheckItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasPlayerItem(CBasePlayerItem *pCheckItem)
{
	if (pCheckItem == NULL)
		return false;

	if (GetInventoryItem(pCheckItem->GetID()) == pCheckItem)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Safe way to access player's inventory
// Input  : &iID - 
// Output : CBasePlayerItem *
//-----------------------------------------------------------------------------
CBasePlayerItem *CBasePlayer::GetInventoryItem(const int &iItemId)
{
#if defined (_DEBUG)
	ASSERT(pev != NULL);
#endif
	if (iItemId >= WEAPON_NONE && iItemId < PLAYER_INVENTORY_SIZE)
	{
		if (m_rgpWeapons[iItemId].Get())
		{
			CBasePlayerItem *pItem = (CBasePlayerItem *)(CBaseEntity *)m_rgpWeapons[iItemId];
			if (pItem->GetHost() != this)
			{
				conprintf(1, "CBasePlayer(%d)::GetInventoryItem(%d)(ei %d id %d) bad item owner!\n", entindex(), iItemId, pItem->entindex(), pItem->GetID());
				DBG_FORCEBREAK
				pItem->SetOwner(this);
			}

#if defined (_DEBUG)
			if (UTIL_IsValidEntity(pItem) && (pItem->GetID() > WEAPON_NONE))
#endif
				return pItem;
#if defined(_DEBUG) && defined(_DEBUG_ITEMS)
			else
				conprintf(1, "CBasePlayer(%d)::GetInventoryItem(%d)(ei %d id %d) got bad item!\n", entindex(), iItemId, m_rgpWeapons[iItemId]->entindex(), pItem->GetID());
#endif
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player attack in any way now?
// WARNING: Do not use CBaseMonster version as it uses m_flNextAttack in a different way!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::CanAttack(void)
{
	if (m_fFrozen)
	{
		//ALERT(at_debug, "CBasePlayer(%d %s)::CanAttack() Frozen\n", entindex(), STRING(pev->netname));
		return false;
	}

	if (IsOnLadder() && g_pGameRules && !g_pGameRules->FAllowShootingOnLadder(this))// XDM3035: TESTME
	{
		//ALERT(at_debug, "CBasePlayer(%d %s)::CanAttack() OnLadder\n", entindex(), STRING(pev->netname));
		if (m_pActiveItem && !FBitSet(m_pActiveItem->iFlags(), ITEM_FLAG_USEONLADDER))
			return false;
	}

	if (m_flNextAttack > UTIL_WeaponTimeBase())// this prevents player from shooting while changing weapons
	{
		//ALERT(at_debug, "CBasePlayer(%d %s)::CanAttack() NextAttack\n", entindex(), STRING(pev->netname));
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: player-specific layer
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::CanDeployItem(CBasePlayerItem *pItem)
{
	if (pItem == NULL)
		return false;

	// XDM3038a: temporary fix of bad viewmodels!	if (m_fFrozen)
	//		return false;
	if (m_pTank != NULL)
		return false;

	if (m_flNextAttack > UTIL_WeaponTimeBase())// XDM3038a: prevent client cmd from selecting server-holstered weapon
		return false;

	if (m_fFrozen)
	{
		//ALERT(at_debug, "CBasePlayer(%d %s)::CanDeployItem() Frozen\n", entindex(), STRING(pev->netname));
		return false;
	}

	if (m_pCarryingObject)
	{
		if (FBitSet(m_pCarryingObject->pev->spawnflags, SF_CAPTUREOBJECT_HOLSTERWEAPON))
		{
			//ALERT(at_debug, "CBasePlayer(%d %s)::CanDeployItem() SF_CAPTUREOBJECT_HOLSTERWEAPON\n", entindex(), STRING(pev->netname));
			return false;
		}
	}

	if (g_pGameRules && !g_pGameRules->FAllowShootingOnLadder(this))// XDM3035: TESTME
	{
		if (IsOnLadder() && !FBitSet(pItem->iFlags(), ITEM_FLAG_USEONLADDER))
			return false;
	}
	return IsAlive();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: player-specific layer
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::CanDropItem(CBasePlayerItem *pItem)
{
	if (pItem == NULL)
		return false;

	return g_pGameRules?g_pGameRules->CanDropPlayerItem(this, pItem):true;
}

//-----------------------------------------------------------------------------
// Purpose: DeployActiveItem
//-----------------------------------------------------------------------------
void CBasePlayer::DeployActiveItem(void)// XDM
{
	if (m_pActiveItem)
	{
#if defined (_DEBUG)
		DBG_ITM_PRINT("CBasePlayer(%d)::DeployActiveItem()\n", entindex());
		if (m_pActiveItem->GetHost() != this)
		{
			conprintf(1, "CBasePlayer(%d)::DeployActiveItem(ei %d id %d) bad item owner!\n", entindex(), m_pActiveItem->entindex(), m_pActiveItem->GetID());
			DBG_FORCEBREAK
		}
#endif
		if (!CanDeployItem(m_pActiveItem))// XDM3038
			return;

		//	m_pActiveItem->SetOwner(this);// WTF? safety
		// bad	if (m_pLastItem && m_pLastItem->IsHolstered())// XDM3038
		//		m_pActiveItem->Holster();
		//	else
			m_pActiveItem->Deploy();

		// XDM3038	m_pActiveItem->UpdateItemInfo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used by SelectItem to delay weapon selection
// Input  : *pItem - 
//-----------------------------------------------------------------------------
void CBasePlayer::QueueItem(CBasePlayerItem *pItem)// XDM
{
	DBG_ITM_PRINT("CBasePlayer(%d)::QueueItem(%d)(id %d)\n", entindex(), pItem?pItem->entindex():0, pItem?pItem->GetID():0);
	if (pItem == NULL)
		return;

	ASSERT(pItem->GetID() != WEAPON_NONE);
#if defined (_DEBUG)
	if (pItem->GetHost() != this)
	{
		conprintf(1, "CBasePlayer(%d)::QueueItem(ei %d id %d) bad item owner!\n", entindex(), pItem->entindex(), pItem->GetID());
		DBG_FORCEBREAK
	}
#endif
	/* XDM3038: fixes fast last/next selection bug	if (m_pActiveItem == NULL)// no active weapon
	{
		m_pActiveItem = pItem;
		//return;// just set this item as active
	}
    else*/

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = NULL;// clear current
	m_pNextItem = pItem;// add item to queue

	if (m_pLastItem == NULL)// XDM3038: there was no weapon
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.05f;// XDM3038: pick up immediately
}

//-----------------------------------------------------------------------------
// Purpose: Normal method to switch to specified item
// Input  : *pItem - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::SelectItem(CBasePlayerItem *pItem)
{
	DBG_ITM_PRINT("CBasePlayer(%d)::SelectItem(%d)(id %d)\n", entindex(), pItem?pItem->entindex():0, pItem?pItem->GetID():0);
	if (pItem == NULL)
		return false;
	if (!IsAlive())// XDM3038a
		return false;
	if (pItem->GetID() == WEAPON_NONE)// invalid pointer was passed!!
	{
		conprintf(1, "CBasePlayer(%d)::SelectItem() tried to select WEAPON_NONE!\n", entindex());
		DBG_FORCEBREAK;
		return false;
	}
	if (pItem == m_pNextItem)// XDM3038: already selecting this item, don't interrupt
		return true;// IMPORTANT

#if defined (_DEBUG)
	if (pItem->GetHost() != this)
	{
		conprintf(1, "CBasePlayer(%d)::SelectItem(ei %d id %d) bad item owner!\n", entindex(), pItem->entindex(), pItem->GetID());
		DBG_FORCEBREAK
		pItem->SetOwner(this);// HACKFIX
	}
#endif
	if (!pItem->CanDeploy())// XDM
		return false;

	if (m_pActiveItem)
	{
		ASSERT(m_pActiveItem->GetHost() == this);
		//m_pActiveItem->SetOwner(this);// HACKFIX

		if (pItem == m_pActiveItem)// already selected
			return true;

		if (!m_pActiveItem->IsHolstered() && !m_pActiveItem->CanHolster())// XDM3038
			return false;
	}

	ResetAutoaim();

	if (m_pActiveItem)
	{
		if (m_pActiveItem->IsHolstered())// XDM3038
			pItem->pev->impulse = ITEM_STATE_HOLSTERED;// No, we don't want any animations! pItem->Holster();// XDM3038a: this item must start in holstered state
		else
			m_pActiveItem->Holster();
	}
	QueueItem(pItem);// XDM
	DeployActiveItem();// XDM: QueueItem() sets m_pActiveItem if we have no current weapon
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Select item by ID
// Input  : iID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::SelectItem(const int &iID)
{
//SPAM	DBG_PLR_PRINT("CBasePlayer(%d)::SelectItem(%d)\n", entindex(), iID);
	if (iID <= WEAPON_NONE || iID >= PLAYER_INVENTORY_SIZE)// ??? allow WEAPON_NONE?
		return false;

	CBasePlayerItem *pItem = GetInventoryItem(iID);
	if (pItem)// do we actually have item with this ID?
		return SelectItem(pItem);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Select item by name
// Input  : *pstr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::SelectItem(const char *pstr)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::SelectItem(\"%s\")\n", entindex(), pstr);
	if (pstr == NULL)
		return false;
	if (!IsAlive())// XDM3038a
		return false;

	CBasePlayerItem *pItem = NULL;
	for (int i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		pItem = GetInventoryItem(i);
		if (pItem)
		{
			if (strcmp(pItem->GetWeaponName(), pstr) == 0)
			{
				return SelectItem(pItem);
				break;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Select Next Best Item
// Warning: XDM3037: moved to client, a command is set back to the server
// Input  : *pItem - 
// Output : CBasePlayerItem
//-----------------------------------------------------------------------------
CBasePlayerItem *CBasePlayer::SelectNextBestItem(CBasePlayerItem *pItem)
{
	if (m_pActiveItem && !m_pActiveItem->CanHolster())
		return NULL;// can't put this gun away right now, so can't switch.
	if (!IsAlive())// XDM3038a: dont't flood
		return NULL;
#if defined(SERVER_WEAPON_SLOTS)
	CBasePlayerItem *pBestItem = g_pGameRules?g_pGameRules->GetNextBestWeapon(this, pItem):NULL;
	if (pBestItem && pBestItem != pItem)
		SelectItem(pBestItem);

	return pBestItem;
#else// client-side logic
	DBG_ITM_PRINT("CBasePlayer(%d)::SelectNextBestItem(%d)\n", entindex(), pItem?pItem->GetID():WEAPON_NONE);

	if (HasWeapons())// XDM3038a
	{
	MESSAGE_BEGIN(MSG_ONE, gmsgSelBestItem, NULL, edict());
		WRITE_BYTE(pItem?pItem->GetID():WEAPON_NONE);
	MESSAGE_END();
	}
	return pItem;//NULL;?
#endif
}

//-----------------------------------------------------------------------------
// Purpose: HasWeapons - do I have any weapons at all?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasWeapons(void)
{
	for (int i = 0; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		if (GetInventoryItem(i))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: allows future flexibility
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasSuit(void)
{
	return FBitSet(pev->weapons, (1<<WEAPON_SUIT));// TODO: move suit to m_rgItems ?
}

//-----------------------------------------------------------------------------
// Purpose: GiveNamedItem - gives player an item and DOES NOT let it drop into world
// Warning: BUGBUG UNDONE string allocation problem! Does ALLOC_STRING() have any overflow-proof mechanism?? UPD: no.
// Warning: BUGBUG UNDONE shittiest mechanism ever! We cannot track items like this!! We don't know if anything was really picked!!!!!!!!!!
// XDM3035: modified to return a pointer
// XDM3037: changed creation method and string allocation, but may it cause string overflow?? TODO TESTME
// XDM3038: modified to return boolean *sigh*
// NEW: SF_NOREFERENCE protects the game from being spammed with items added by game rules
// Input  : *pszName - must reside in memory! Temporary variables are not allowed!
// Output : NULL if item was not created or accepted
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayer::GiveNamedItem(const char *pszName)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::GiveNamedItem(%s)\n", entindex(), pszName);
	CBaseEntity *pEnt = Create(pszName, pev->origin, pev->angles, g_vecZero, NULL, SF_NOREFERENCE | SF_NORESPAWN | SF_ITEM_NOFALL);// XDM3037
	if (pEnt)
	{
		if (pEnt->IsPickup() && (g_pGameRules == NULL || g_pGameRules->IsAllowedToSpawn(pEnt)))// XDM3038a: sanity/security check
		{
			SetBits(pEnt->pev->effects, EF_NODRAW);
			pEnt->m_flRemoveTime = gpGlobals->time + 0.002f;// XDM3038a: DANGEROUS! We rely upon that fact that internal code will reset this on pickup!
			pEnt->Touch(this);// UNDONE: TODO: make something that would track item existence!

			//if (/*pEnt->pev->owner != edict() && */pEnt->m_hOwner != this)// XDM3038: is not owned/carried!
			if (pEnt->GetExistenceState() != ENTITY_EXSTATE_CARRIED)// XDM3038c: the most logical way to track entity existence
			{
				DBG_ITM_PRINT("CBasePlayer(%d)::GiveNamedItem(%s): exstate != CARRIED, removing.\n", entindex(), pszName);
				/*if (pEnt->pev->health > 0.0f)// Not marked for removal
				{
					conprintf(1, "GiveNamedItem(%s) ERROR: pEnt->m_hOwner != this but health > 0!\n", pszName);
					DBG_FORCEBREAK
				}*/
				// Force these flags to be 200% sure
				SetBits(pEnt->pev->effects, EF_NODRAW);
				SetBits(pEnt->pev->spawnflags, SF_NORESPAWN);
				SetBits(pEnt->pev->flags, FL_KILLME);// may be already removed by this moment, so use safe method!
				pEnt->SetTouchNull();
				pEnt->m_flRemoveTime = gpGlobals->time;// let's see who gets to it first
				//pEnt->Destroy();// XDM3038: unsafe?
			}
			else
				return pEnt;
		}
		else// no illegal stuff allowed
		{
			DBG_ITM_PRINT("CBasePlayer(%d)::GiveNamedItem(%s): not allowed, removing.\n", entindex(), pszName);
			SetBits(pEnt->pev->effects, EF_NODRAW);
			SetBits(pEnt->pev->spawnflags, SF_NORESPAWN);
			SetBits(pEnt->pev->flags, FL_KILLME);// use safe method!
			pEnt->SetTouchNull();
			pEnt->m_flRemoveTime = gpGlobals->time;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Freeze grenade
// Input  : freezetime - 
//-----------------------------------------------------------------------------
void CBasePlayer::FrozenStart(float freezetime)
{
	if (!IsAlive())
		return;
	DBG_PLR_PRINT("CBasePlayer(%d)::FrozenStart(%g)\n", entindex(), freezetime);

	pev->movetype = MOVETYPE_TOSS;
	pev->fixangle = 1;

	CBaseMonster::FrozenStart(freezetime);// this sets rendercolor and stops burning

	m_flNextAttack = UTIL_WeaponTimeBase() + freezetime;// override for player
	if (m_pActiveItem)// XDM: crash prevention
	{
		CBasePlayerWeapon *pWeapon = m_pActiveItem->GetWeaponPtr();
		if (pWeapon)
		{
			ASSERT(pWeapon->GetHost() != NULL);
			pWeapon->m_flNextPrimaryAttack = pWeapon->m_flNextSecondaryAttack = m_flNextAttack;
			pWeapon->m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
			pWeapon->WeaponIdle();// Force TESTME!!
		}
	}

	//TODO: reduce pev->maxspeed?
	DisableLadder(freezetime);// XDM3037

	//if (!m_fFrozen)// UNDONE: if already frozen
	//CL	UTIL_ScreenFade(this, pev->rendercolor, 1.0, 1.0, 127, FFADE_OUT | FFADE_STAYOUT);
}

//-----------------------------------------------------------------------------
// Purpose: Frozen period is ending
//-----------------------------------------------------------------------------
void CBasePlayer::FrozenEnd(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::FrozenEnd()\n", entindex());
	pev->movetype = MOVETYPE_WALK;
	pev->fixangle = FALSE;
	CBaseMonster::FrozenEnd();
	m_flNextAttack = UTIL_WeaponTimeBase();
	DisableLadder(0.0);// XDM3038: re-enable

	if (m_pActiveItem)// XDM: crash prevention
	{
		CBasePlayerWeapon *pWeapon = m_pActiveItem->GetWeaponPtr();
		if (pWeapon != NULL)
			pWeapon->m_flNextPrimaryAttack = pWeapon->m_flNextSecondaryAttack = m_flNextAttack;
	}
	UTIL_ScreenFade(this, Vector(63,143,255)/*pev->rendercolor*/, 4, 0, 128, FFADE_IN);// XDM3037: FIXME
}

//-----------------------------------------------------------------------------
// Purpose: called every frame while frozen (freeze grenade)
//-----------------------------------------------------------------------------
void CBasePlayer::FrozenThink(void)
{
	// should be already checked	if (!m_fFrozen)
	//	return;

	if (m_flUnfreezeTime > 0.0f)
	{
		if (m_flUnfreezeTime <= gpGlobals->time)
			FrozenEnd();// call this before CBaseMonster
		else
			CBaseMonster::FrozenThink();
	}
}

//-----------------------------------------------------------------------------
// Purpose: What force should be applied to this entity upon taking damage
// Input  : damage - 
// Output : float
//-----------------------------------------------------------------------------
float CBasePlayer::DamageForce(const float &damage)
{
	return PLAYER_DAMAGE_FORCE_MULTIPLIER*CBaseEntity::DamageForce(damage);
}

//-----------------------------------------------------------------------------
// Purpose: Enable client controls (movement, etc.)
// Input  : fControl - 
//-----------------------------------------------------------------------------
void CBasePlayer::EnableControl(bool fControl)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::EnableControl(%d)\n", entindex(), fControl);
	if (fControl)
	{
		ClearBits(pev->flags, FL_FROZEN);
		pev->view_ofs = VEC_VIEW_OFFSET;// XDM3038: HACK! view_ofs is 0 after SET_VIEW!
	}
	else
		SetBits(pev->flags, FL_FROZEN);
}

//-----------------------------------------------------------------------------
// Purpose: Is this player on ladder?
// Output : Returns true if the player is attached to a ladder
//-----------------------------------------------------------------------------
bool CBasePlayer::IsOnLadder(void)
{
	return (pev->movetype == MOVETYPE_FLY);
}

//-----------------------------------------------------------------------------
// Purpose: Disable use of ladders (just fall)
// Input  : time - for this amount of time (0 to reset)
//-----------------------------------------------------------------------------
void CBasePlayer::DisableLadder(const float &time)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::DisableLadder(%g)\n", entindex(), time);
	if (time > 0.0f)
	{
		pev->movetype = MOVETYPE_WALK;
		ENGINE_SETPHYSKV(edict(), PHYSKEY_IGNORELADDER, "1");
		m_flIgnoreLadderStopTime = max(m_flIgnoreLadderStopTime, gpGlobals->time + time);// XDM3037: don't add up, repalce with longest interval
	}
	else
	{
		m_flIgnoreLadderStopTime = gpGlobals->time;// just let it end normally
		//ENGINE_SETPHYSKV(edict(), PHYSKEY_IGNORELADDER, "0");
		//m_flIgnoreLadderStopTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is this player on train?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::IsOnTrain(void)// XDM3035
{
	if (FBitSet(m_afPhysicsFlags, PFLAG_ONTRAIN))
		return true;
	//if (m_pTrain)// OR or AND?
	//	return TRUE;
	//if (m_iTrain & TRAIN_ACTIVE)
	//	return TRUE;
	// THIS DESTROYS LOGIC!	if (pev->flags & FL_ONTRAIN)
	//	return TRUE;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Attach player to a train controls
// Input  : *pTrain - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::TrainAttach(CBaseEntity *pTrain)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::TrainAttach()\n", entindex());
	if (pTrain == NULL)
		return false;

	CBaseDelay *pRealTrain = (CBaseDelay *)pTrain;// XDM3035: HACKHACKHACK!!!!!!

	if (pRealTrain && pRealTrain->IsLockedByMaster(this))// XDM3037: disabled train
		return false;

	// old if ((pTrain->pev->euser1 != NULL) ||
	if (pRealTrain->m_hActivator != NULL && pRealTrain->m_hActivator != this)
	{
		ClientPrint(pev, HUD_PRINTCENTER, "#TRAIN_OCC", STRING(pRealTrain->m_hActivator->pev->netname));
		if (sv_allowstealvehicles.value == 0.0f)// XDM3038: :)
			return false;
	}

	/* XDM3038: just update anyway	if (IsOnTrain())// already using
	{
		//if (pTrain->pev != m_pTrain->pev)// ANOTHER train
		//	if (!TrainDetach())// which I failed to detach from
				return false;// so don't bother
	}*/

	SetBits(m_afPhysicsFlags, PFLAG_ONTRAIN);
	m_iTrain = TrainSpeed((int)pTrain->pev->speed, pTrain->pev->maxspeed);// XDM3038a
	SetBits(m_iTrain, TRAIN_NEW);
	// old pTrain->pev->euser1 = edict();// become attacker in multiplayer
	pRealTrain->m_hActivator = this;// XDM3035: HACKHACKHACK!!!!!!
	m_pTrain = pTrain;
	//SetBits(pev->flags, FL_ONTRAIN);
#if defined (_DEBUG)
	conprintf(2, "CBasePlayer::TrainAttach() successful\n");
#endif
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Detach player from train controls
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::TrainDetach(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::TrainDetach()\n", entindex());
	if (m_pTrain)
	{
		CBaseDelay *pRealTrain = GetControlledTrain();// get this FIRST, while it is valid!
		if (pRealTrain)// May get NULL!
		{
			if (pRealTrain->m_hActivator == this)// new
			{
				pRealTrain->Use(this, this, USE_OFF, 0);// XDM3038: A MUST!
				//pRealTrain->m_hActivator = NULL;
			}
		}
		//if (m_pTrain->pev->euser1 == edict())
		//	m_pTrain->pev->euser1 = NULL;

		m_pTrain = NULL;
#if defined (_DEBUG)
	conprintf(2, "CBasePlayer::TrainDetach() successful\n");
#endif
	}
	ClearBits(m_afPhysicsFlags, PFLAG_ONTRAIN);
	m_iTrain = TRAIN_OFF|TRAIN_NEW;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: For external use
// Output : CBaseDelay
//-----------------------------------------------------------------------------
CBaseDelay *CBasePlayer::GetControlledTrain(void)
{
	if (m_pTrain.Get())
	{
		CBaseDelay *pTrain = (CBaseDelay *)(CBaseEntity *)m_pTrain;
		// in CoOp this is not true	ASSERT(pTrain->m_hActivator == this);
		return pTrain;// XDM3037: ...
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: PreThink OnTrain part
//-----------------------------------------------------------------------------
void CBasePlayer::TrainPreFrame(void)
{
	// Train speed control
	if (IsOnTrain())//m_afPhysicsFlags & PFLAG_ONTRAIN)
	{
		// So the correct flags get sent to client asap.
		SetBits(pev->flags, FL_ONTRAIN);
		CBaseEntity *pTrain = (CBaseEntity *)m_pTrain;// XDM3035b
		if (pTrain == NULL)
		{
			conprintf(2, "TrainPreFrame(): client %d: acquiring train from ground entity.\n", entindex());
			pTrain = CBaseEntity::Instance(pev->groundentity);
			TrainAttach(pTrain);// somehow it works
		}
		if (pTrain == NULL)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0.0f,0.0f,HULL_MIN-2.0f), ignore_monsters, edict(), &trainTrace);
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);

			if (pTrain == NULL || !FBitSet(pTrain->ObjectCaps(), FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev))
			{
				conprintf(2, "TrainPreFrame() error: client %d: in train mode with no train!\n", entindex());
				TrainDetach();// XDM3035b
			}
		}
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || FBitSet(pev->button, IN_MOVELEFT|IN_MOVERIGHT))
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			TrainDetach();// XDM3035b
		}

		//if (IsOnTrain())// still on train?
		if (FBitSet(m_afPhysicsFlags, PFLAG_ONTRAIN) && pTrain)// XDM3035: removed returns;
		{
			float vel = 0.0f;
			pev->velocity.Clear();
			if (FBitSet(m_afButtonPressed, IN_FORWARD))
			{
				vel = 1.0f;
				pTrain->Use(this, this, USE_SET, vel);
			}
			else if (FBitSet(m_afButtonPressed, IN_BACK))
			{
				vel = -1.0f;
				pTrain->Use(this, this, USE_SET, vel);
			}

			if (vel != 0.0f || m_fInitHUD)// XDM3038: update train when updating hud, too
			{
				m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->maxspeed);
				SetBits(m_iTrain, TRAIN_ACTIVE|TRAIN_NEW);
			}
		}
	}
	else
	{
		ClearBits(pev->flags, FL_ONTRAIN);
		if (FBitSet(m_iTrain, TRAIN_ACTIVE))
			m_iTrain = TRAIN_OFF|TRAIN_NEW; // turn off train
	}
}

//-----------------------------------------------------------------------------
// Purpose: new special weapon
//-----------------------------------------------------------------------------
void CBasePlayer::ThrowNuclearDevice(void)// XDM3035
{
	if (m_flNextAttack <= UTIL_WeaponTimeBase())
	{
		if (m_pActiveItem)
		{
			if (!m_pActiveItem->CanHolster())// XDM
				return;

			m_pActiveItem->Holster();
		}
		EMIT_SOUND_DYN(edict(), CHAN_ITEM, DEFAULT_PICKUP_SOUND_CONTAINER, VOL_NORM, ATTN_STATIC, 0, PITCH_NORM);
		m_flThrowNDDTime = gpGlobals->time + 2.0f;
		m_flNextAttack = UTIL_WeaponTimeBase() + 2.5f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get team player belonged to before becoming spectator
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CBasePlayer::GetLastTeam(void)
{
	return pev->team;// playerclass; XDM3038a: OBSOLETE
}

//-----------------------------------------------------------------------------
// Purpose: CoOp: player reached and touched some checkpoint (trigger_autosave)
// Warning: Order is important! Last touched must be last in the list!
// Input  : *pCheckPoint - 
//-----------------------------------------------------------------------------
void CBasePlayer::OnCheckPoint(CBaseEntity *pCheckPoint)
{
	if (!ASSERT(pCheckPoint != NULL))
		return;

	DBG_PLR_PRINT("CBasePlayer(%d)::OnCheckPoint()\n", entindex());
	//if (m_iLastCheckPointIndex != pCheckPoint->entindex())
	//if (!PassedCheckPoint(pCheckPoint))
	size_t i;
	size_t iEmpty = MAX_CHECKPOINTS;
	for (i=0; i<MAX_CHECKPOINTS; ++i)
	{
		if (m_hszCheckPoints[i] == pCheckPoint)// already touched
			return;
		else if (m_hszCheckPoints[i].Get() == NULL && iEmpty == MAX_CHECKPOINTS)// empty slot and we need it
			iEmpty = i;
	}
	if (iEmpty >= MAX_CHECKPOINTS)
	{
		conprintf(2, "WARNING: Client %d: has too many registered checkpoints!\n", entindex());
		iEmpty = 0;// TESTME: register anyway...
	}
	m_hszCheckPoints[iEmpty] = pCheckPoint;
	m_Stats[STAT_CHECK_POINT_COUNT]++;// XDM3037
	if (g_pGameRules)
		g_pGameRules->OnPlayerCheckPoint(this, pCheckPoint);
}

//-----------------------------------------------------------------------------
// Purpose: CoOp: has this player ever touched this checkpoint? (trigger_autosave)
// Input  : *pCheckPoint - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::PassedCheckPoint(CBaseEntity *pCheckPoint)
{
	if (!ASSERT(pCheckPoint != NULL))
		return false;

	for (size_t i=0; i<MAX_CHECKPOINTS; ++i)// array MUST BE CLEARED OUT before using this!
	{
		if (m_hszCheckPoints[i].Get() && m_hszCheckPoints[i] == pCheckPoint)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: CoOp: get last checkpoint this player touched (trigger_autosave)
// Warning: No checks on touch time is done! We rely solely on element order!!!
// Output : CBaseEntity *pCheckPoint
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayer::GetLastCheckPoint(void)
{
	size_t i = MAX_CHECKPOINTS;
	do// 'for' won't do any good with unsigned values
	{
		--i;
		if (m_hszCheckPoints[i].Get())
			return m_hszCheckPoints[i];
	} while (i > 0);
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: safely clear out array
//-----------------------------------------------------------------------------
void CBasePlayer::CheckPointsClear(void)
{
	for (size_t i=0; i<MAX_CHECKPOINTS; ++i)
		m_hszCheckPoints[i] = NULL;

	m_vecLastSpawnOrigin.Clear();
	m_vecLastSpawnAngles.Clear();
	m_iLastSpawnFlags = 0;
}

//-----------------------------------------------------------------------------
// Purpose: User rights
// Input  : iRightIndex - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::RightsCheck(uint32 iRightIndex)
{
	return FBitSet(m_iUserRights, 1 << iRightIndex);
}

//-----------------------------------------------------------------------------
// Purpose: User rights - to check multiple rights at once
// Input  : iRightBits - == (1 << iRightIndex)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::RightsCheckBits(uint32 iRightBits)
{
	return FBitSet(m_iUserRights, iRightBits);
}

//-----------------------------------------------------------------------------
// Purpose: Add user rights
// Input  : iRightIndex - 
//-----------------------------------------------------------------------------
void CBasePlayer::RightsAssign(uint32 iRightIndex)
{
	if (!RightsCheck(iRightIndex))// don't inform server if nothing changes
	{
		SetBits(m_iUserRights, 1 << iRightIndex);
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" gained rights: \"%s\"\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()), GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(edict()), "team"), g_UserRightsNames[iRightIndex]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove user rights
// Input  : iRightIndex - 
//-----------------------------------------------------------------------------
void CBasePlayer::RightsRemove(uint32 iRightIndex)
{
	if (RightsCheck(iRightIndex))// don't inform server if nothing changes
	{
		ClearBits(m_iUserRights, 1 << iRightIndex);
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" lost rights: \"%s\"\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()), GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(edict()), "team"), g_UserRightsNames[iRightIndex]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: UNDONE TODO: we ONLY need to restore player's CURRENT stats in case of sudden disconnect & rejoin
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::StatsLoad(void)
{
	//const char *pID = GETPLAYERAUTHID(edict());
	//start_read_file()
	//if (file.stattime_and_matchID_and_mapname == current)
	//load_from_file("stats/%s.txt", pID)
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: TODO: write stats with by Unique ID to a file so it can be restored on reconnect
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::StatsSave(void)
{
	// merge stats with player's GLOBAL stats collected over years on this server?
	//const char *pID = GETPLAYERAUTHID(edict());
	//merge_from_file("stats/%s.txt", pID)
	//save_to_file("stats/%s.txt", pID)
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: completely wipe out statistics
//-----------------------------------------------------------------------------
void CBasePlayer::StatsClear(void)
{
	memset(m_Stats, 0, sizeof(m_Stats));
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBasePlayer::ReportState(int printlevel)
{
	CBaseMonster::ReportState(printlevel);
	conprintf(printlevel, "Name: %s, SpawnState: %hd, GameFlags: %d, UserRights: %u, Deaths %u, PhysicsFlags %u, AnimExtention: %s\n", STRING(pev->netname), m_iSpawnState, m_iGameFlags, m_iUserRights, m_iDeaths, m_afPhysicsFlags, m_szAnimExtention);
	/*  Dumping inventory:\n
	for (int i = WEAPON_NONE; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		CBasePlayerItem *pItem = pTargetPlayer->GetInventoryItem(i);
		if (pItem)
			pItem->ReportState(printlevel);
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Universal, for internal and external APIs
// Input  : *pTrackName - track/playlist
//			iCommand - MUSICPLAYER_CMD enum
//			fTimeOffset - offset in seconds
//-----------------------------------------------------------------------------
void CBasePlayer::PlayAudioTrack(const char *pTrackName, int iCommand, const float &fTimeOffset)
{
	DBG_PRINTF("PlayAudioTrack(\"%s\", cmd %d, ofs %g)\n", pTrackName, iCommand, fTimeOffset);
	if ((iCommand == MPSVCMD_PLAYTRACK || iCommand == MPSVCMD_PLAYTRACKLOOP) && (pTrackName == NULL || pTrackName[0] == 0))// some commands are allowed without name
	{
		ClientPrintF(pev, HUD_PRINTCONSOLE, 2, "SV: PlayAudioTrack(\"%s\", cmd %d, ofs %g) error: no track specified!\n", pTrackName, iCommand, fTimeOffset);
		return;
	}
	else
		ClientPrintF(pev, HUD_PRINTCONSOLE, 2, "SV: PlayAudioTrack(\"%s\", cmd %d, ofs %g)\n", pTrackName, iCommand, fTimeOffset);

	if (iCommand == MPSVCMD_RESET)// normally this should not be used
	{
		m_szCurrentMusicTrack[0] = '\0';
		m_iCurrentMusicTrackStartTime = gpGlobals->time;
	}
	else// a real sendable command
	{
		if ((pTrackName == NULL || pTrackName[0] == '\0'))// perfectly acceptable!
			m_szCurrentMusicTrack[0] = '\0';
		else
		{
			strcpy(m_szCurrentMusicTrack, pTrackName);
			m_szCurrentMusicTrack[MAX_ENTITY_STRING_LENGTH-1] = '\0';
		}
		m_iCurrentMusicTrackCommand = iCommand;
		m_iCurrentMusicTrackStartTime = gpGlobals->time - fTimeOffset;// pretend we started playing somewhen in the past :)
	}
	if (fTimeOffset == 0.0f)
		m_iCurrentMusicTrackStartTime += 0.05f;// delay a little bit

	m_iCurrentMusicTrackCommandIssued = 0;
	// ASYNC: message will be sent in UpdateClientData
}
