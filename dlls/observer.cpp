#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "globals.h"
#include "game.h"
#include "gamerules.h"
#include "pm_shared.h"// observer modes


//-----------------------------------------------------------------------------
// Purpose: Handle buttons in observer mode
//-----------------------------------------------------------------------------
void CBasePlayer::ObserverHandleButtons(void)
{
	// Slow down mouse clicks
	if (m_flNextObserverInput > gpGlobals->time)
		return;

	// I don't know how this shit fails and I don't want to know
	//if (m_afButtonReleased + m_afButtonPressed > 0)
	//	conprintf(1, "ObserverHandleButtons() P = %d, R = %d\n", m_afButtonReleased, m_afButtonPressed);

	if (pev->iuser1 == OBS_INTERMISSION)
		return;

	// Jump changes from modes: Chase to Roaming
	if (FBitSet(m_afButtonPressed, IN_JUMP))
		ObserverSetMode(pev->iuser1 + 1);

	if (pev->iuser1 != OBS_ROAMING && pev->iuser1 != OBS_MAP_FREE)
	{
		// Attack moves to the next player
		if (FBitSet(m_afButtonPressed, IN_ATTACK))
			ObserverFindNextPlayer(false);
		// Attack2 moves to the prev player
		else if (FBitSet(m_afButtonPressed, IN_ATTACK2))
			ObserverFindNextPlayer(true);
	}
	m_flNextObserverInput = gpGlobals->time + 0.02f;
}

//-----------------------------------------------------------------------------
// Purpose: Find the next client in the game for this player to spectate
// Input  : bReverse - 
//-----------------------------------------------------------------------------
void CBasePlayer::ObserverFindNextPlayer(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.
	int iStart;
	if (m_hObserverTarget)
		iStart = ENTINDEX(m_hObserverTarget.Get());
	else
		iStart = ENTINDEX(edict());

	m_hObserverTarget = NULL;

	if (gpGlobals->maxClients <= 1)// XDM3037: when server is shutting down this creates an infinite loop
		return;

	int iCurrent = iStart;
	int iDir = bReverse ? -1 : 1; 
	do
	{
		iCurrent += iDir;
		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		else if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		//CBasePlayer *client = UTIL_ClientByIndex(iCurrent);
		// inside ObserverSetTarget		if (!ObserverValidateTarget(client))
		//	continue;

		// MOD AUTHORS: Add checks on target here.
		if (ObserverSetTarget(UTIL_ClientByIndex(iCurrent)))
			break;

	} while (iCurrent != iStart);

	// Did we find a target?
	if (m_hObserverTarget)
	{
		// Store the target in pev so the physics DLL can get to it
		/*pev->iuser2 = ENTINDEX(m_hObserverTarget.Get());
		// Move to the target
		UTIL_SetOrigin(this, m_hObserverTarget->pev->origin);*/
		//conprintf(1, "Now Tracking %s\n", STRING(m_hObserverTarget->pev->netname));

	}
	else
		ClientPrint(pev, HUD_PRINTNOTIFY, "#Spec_NoTarget\n");
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to change the observer mode
// Input  : iMode - new mode
//-----------------------------------------------------------------------------
void CBasePlayer::ObserverSetMode(int iMode)
{
	// Just abort if we're changing to the mode we're already in
	if (iMode == pev->iuser1)
		return;

	if (iMode > OBS_MAP_CHASE)// XDM: loop
		iMode = OBS_CHASE_LOCKED;

	if (iMode == OBS_CHASE_LOCKED)// 3rd person, camera follows target's angles
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if (m_hObserverTarget.Get() == NULL)
			ObserverFindNextPlayer(false);

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_LOCKED;
			pev->iuser2 = ENTINDEX(m_hObserverTarget.Get());
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_Mode1");
			pev->maxspeed = 0;
		}
		else
		{
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_NoTarget");
			ObserverSetMode(OBS_ROAMING);
		}
	}
	else if (iMode == OBS_CHASE_FREE)// 3rd person, camera moves freely
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if (m_hObserverTarget.Get() == NULL)
			ObserverFindNextPlayer(false);

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_FREE;
			pev->iuser2 = ENTINDEX(m_hObserverTarget.Get());
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_Mode2");
			pev->maxspeed = 0;
		}
		else
		{
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_NoTarget");
			ObserverSetMode(OBS_ROAMING);
		}
	}
	else if (iMode == OBS_ROAMING)// camera moves freely
	{
		// MOD AUTHORS: If you don't want to allow roaming observers at all in your mod, just abort here.
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;
		//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_Mode3");
		pev->maxspeed = CVAR_GET_FLOAT("sv_spectatormaxspeed");// g_psv_maxspeed->value;// overridden by the engine anyway
	}
	else if (iMode == OBS_IN_EYE)// 1st person, camera follows target's angles
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if (m_hObserverTarget.Get() == NULL)
			ObserverFindNextPlayer(false);

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_IN_EYE;
			pev->iuser2 = ENTINDEX(m_hObserverTarget.Get());
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_Mode4");
			pev->maxspeed = 0;
		}
		else
		{
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_NoTarget");
			ObserverSetMode(OBS_ROAMING);
		}
	}
	else if (iMode == OBS_MAP_FREE)// 2D map, camera moves freely
	{
		pev->iuser1 = OBS_MAP_FREE;
		pev->iuser2 = 0;
		//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_Mode5");
		pev->maxspeed = CVAR_GET_FLOAT("sv_spectatormaxspeed");// g_psv_maxspeed->value;// overridden by the engine anyway
	}
	else if (iMode == OBS_MAP_CHASE)// 2D map, camera follows target's angles
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if (m_hObserverTarget.Get() == NULL)
			ObserverFindNextPlayer(false);

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_MAP_CHASE;
			pev->iuser2 = ENTINDEX(m_hObserverTarget.Get());
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_Mode6");
			pev->maxspeed = 0;
		}
		else
		{
			//ClientPrint(pev, HUD_PRINTCENTER, "#Spec_NoTarget");
			ObserverSetMode(OBS_MAP_FREE);
		}
	}
	/*char str[32];
	_snprintf(str, 32, "#Spec_Mode%d\n", pev->iuser1);
	ClientPrint(pev, HUD_PRINTCENTER, str);*/
	m_iObserverLastMode = iMode;
}

//-----------------------------------------------------------------------------
// Purpose: Called by ObserverSetTarget
// Input  : *pTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::ObserverValidateTarget(CBaseEntity *pTarget)
{
	if (!pTarget)
		return false;
	if (!pTarget->pev)
		return false;
	if (pTarget == this)
		return false;
	// Don't spec observers or invisible players
	if (pTarget->IsPlayer())
	{
		if (((CBasePlayer *)pTarget)->IsObserver())
			return false;
	}
	// BAD because we don't want to immediately switch camera from gibbed players! if (FBitSet(pTarget->pev->effects, EF_NODRAW))
	//	return false;
	if (g_pGameRules && g_pGameRules->IsTeamplay() && mp_specteammates.value > 0.0f)
	{
		if (GetLastTeam() != TEAM_NONE && pTarget->pev->team != GetLastTeam())
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: The one and only way to set target for the observer
// Input  : *pTarget - can be NULL
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::ObserverSetTarget(CBaseEntity *pTarget)
{
	if (pTarget && !ObserverValidateTarget(pTarget))
		return false;

	m_hObserverTarget = pTarget;
	if (m_hObserverTarget)
	{
		// Store the target in pev so the physics DLL can get to it
		pev->iuser2 = ENTINDEX(m_hObserverTarget.Get());
		// Move to the target
		UTIL_SetOrigin(this, m_hObserverTarget->pev->origin);
		//conprintf(1, "Now Tracking %s\n", STRING(m_hObserverTarget->pev->netname));
	}
	else
	{
		pev->iuser2 = 0;
		if (pev->iuser1 != OBS_ROAMING && pev->iuser1 != OBS_MAP_FREE)
			ObserverSetMode(OBS_ROAMING);//pev->iuser1 = OBS_ROAMING;
		//conprintf(1, "Now Tracking nobody\n");
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check current target, if following. Change if necessary. Called every frame.
//-----------------------------------------------------------------------------
void CBasePlayer::ObserverCheckTarget(void)
{
	if (pev->iuser1 == OBS_ROAMING || pev->iuser1 == OBS_MAP_FREE)
		return;

	// try to find a traget if we have no current one
	CBaseEntity *pTargetEntity = m_hObserverTarget;
	if (!ObserverValidateTarget(pTargetEntity))
	{
		ObserverFindNextPlayer(false);

		if (m_hObserverTarget.Get() == NULL)// we still have no target
		{
			// no target found at all 
			int lastMode = pev->iuser1;
			ObserverSetMode(OBS_ROAMING);
			m_iObserverLastMode = lastMode;	// don't overwrite users lastmode
			return;	
		}
	}

	if (pTargetEntity->IsPlayer())
	{
		if (((CBasePlayer *)pTargetEntity)->IsObserver())
		{
			ObserverFindNextPlayer(false);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check current mode/state. Change if necessary. Called every frame.
//-----------------------------------------------------------------------------
void CBasePlayer::ObserverCheckProperties(void)
{
	// try to find a traget if we have no current one
	if (pev->iuser1 == OBS_IN_EYE && m_hObserverTarget.Get())
	{
		CBasePlayer *pTarget = UTIL_ClientByIndex(ENTINDEX(m_hObserverTarget.Get()));
		if (!pTarget)
			return;

		// use fov of tracked client
		pev->fov = pTarget->pev->fov;
		if (pTarget->m_pActiveItem)
		{
			size_t bufferlen = 0;
			char pBuffer[3 + 2];// WARNING! If protocol ever changes, change this buffer size!
			memset(pBuffer, 0, sizeof(pBuffer));
			bufferlen += pTarget->m_pActiveItem->UpdateClientData(pBuffer);
			if (bufferlen > 0)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgUpdWeapons, NULL, edict());// XDM3035: pack all updates into a single packet!
				for (size_t j = 0; j < bufferlen; ++j)
					WRITE_BYTE(pBuffer[j]);

				MESSAGE_END();
			}
		}
		/*int weaponID = (pTarget->m_pActiveItem)?pTarget->m_pActiveItem->GetID():WEAPON_NONE;
		MESSAGE_BEGIN(MSG_ONE, gmsgUpdWeapons, NULL, edict());// XDM3035: pack all updates into a single packet!
			if (pTarget->m_fOnTarget)
				WRITE_BYTE(2);
			else
				WRITE_BYTE(1);

			WRITE_BYTE(weaponID);
			WRITE_BYTE(m_iClip);
		MESSAGE_END();*/

		/*XDM3037a: OBSOLETE
		if (m_iObserverWeapon != weapon)
		{
			m_iObserverWeapon = weapon;
			//send weapon update
			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE( 1 );	// 1 = current weapon, not on target
				WRITE_BYTE( m_iObserverWeapon );	
				WRITE_BYTE( 0 );	// clip
			MESSAGE_END();
		}*/
	}
	else
	{
		pev->fov = DEFAULT_FOV;
		MESSAGE_BEGIN(MSG_ONE, gmsgUpdWeapons, NULL, edict());// XDM3038a
			WRITE_BYTE(1);
			WRITE_BYTE(WEAPON_NONE);
			WRITE_BYTE(255);
		MESSAGE_END();
		/*if (m_iObserverWeapon != 0)
		{
			m_iObserverWeapon = 0;
			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE( 1 );	// 1 = current weapon
				WRITE_BYTE( m_iObserverWeapon );	
				WRITE_BYTE( 0 );	// clip
			MESSAGE_END();
		}*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Player becomes a spectator.
// Input  : &vecPosition - 
//			&vecViewAngle - 
//			mode - OBS_ROAMING
//			*pTarget - NULL for OBS_ROAMING
//-----------------------------------------------------------------------------
bool CBasePlayer::ObserverStart(const Vector &vecPosition, const Vector &vecViewAngle, int mode, CBaseEntity *pTarget)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::ObserverStart(%d)\n", entindex(), mode);
	if (g_pGameRules && !g_pGameRules->PlayerStartObserver(this))// XDM3037a
		return false;

	// Holster weapon immediately, to allow it to cleanup
	/* done in RemoveAllItems()
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
		m_pActiveItem->SetThinkNull();
		m_pActiveItem->m_flNextPrimaryAttack = m_pActiveItem->m_flNextSecondaryAttack = -1;
		m_pActiveItem->m_flTimeWeaponIdle = -1;
		m_pActiveItem = NULL;// XDM: TESTME!
	}*/

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	TrainDetach();

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// clear any clientside entities attached to this player
	MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, pev->origin);// MSG_ALL?
		WRITE_BYTE(TE_KILLPLAYERATTACHMENTS);
		WRITE_BYTE((byte)entindex());
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	/*XDM3037: UpdateClientData(clientdata_s) takes care of it now
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, edict());
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();*/

	// reset FOV
	// XDM3037a: obsolete	m_iFOV = m_iClientFOV = 0;
	//pev->fov = m_iFOV;
	pev->fov = 0;// 
	/*XDM3037a: obsolete
	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, edict());
		WRITE_BYTE(0);
	MESSAGE_END();*/

	// Setup flags
	pev->effects = EF_NODRAW;
	pev->view_ofs.Clear();
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	SetBits(m_iHideHUD, HIDEHUD_ALL);
	ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
	SetBits(m_afPhysicsFlags, PFLAG_OBSERVER);
	ClearBits(pev->flags, FL_DUCKING);
	ClearBits(pev->flags, FL_ONGROUND);
	SetBits(pev->flags, FL_SPECTATOR); // XDM: Should we set Spectator flag? Or is it reserved for people connecting with observer 1?
	SetBits(m_iGameFlags, EGF_SPECTATED);// XDM3038c
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;
	// XDM3037a	m_iClientHealth = 1;
	// XDM3037a
	pev->button = 0;
	m_afButtonLast = 0;
	m_afButtonPressed = 0;
	m_afButtonReleased = 0;
	//m_iSpawnState = SPAWNSTATE_CONNECTED;// XDM3037a: DON'T CHANGE!
	ClearBits(m_iGameFlags, EGF_PRESSEDREADY);
	m_hLastKiller		= NULL;
	m_hLastVictim		= NULL;
	m_iScoreCombo		= 0;// XDM3035
	m_iLastScoreAward	= 0;
	m_fNextScoreTime	= 0.0f;

	// XDM3037a
	/*MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, edict());
		WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();*/
	// Clear out the status bar
	m_fInitHUD = TRUE;
	// Update Team Status
	// Remove all the player's stuff
	// DO NOT DO IT IN THE SAME FRAME AS Killed() !!! RemoveAllItems(true);// XDM3037a: why keep the suit?
	// Move them to the new position
	UTIL_SetOrigin(this, vecPosition);
	pev->oldorigin = pev->origin;
	// Find a player to watch
	//?	ObserverSetMode( m_iObserverLastMode );// HL20130901
	m_flNextObserverInput = gpGlobals->time;

	// XDM: save current team!!! ?
	//	int m_iLastTeamID = pev->team;
	// XDM3038a: OBSOLETE	pev->playerclass = pev->team;// XDM3035: save in entvars to transmit to local client side
	// XDM3038a: OBSOLETE	pev->team = TEAM_NONE;

	if (m_iSpawnState >= SPAWNSTATE_SPAWNED)// XDM3038c
		m_Stats[STAT_SPECTATOR_ENABLE_COUNT]++;// XDM3037

	if (g_pGameRules)
		g_pGameRules->ClientRemoveFromGame(this);// XDM3038c: fixed

	//UTIL_ScreenFade(this, Vector(255,255,255), 1.0f, 0.1f, 255, FFADE_IN);// XDM3035: we need this to clear out remaining screen fades

	// Tell the physics code that this player's now in observer mode
	pev->iuser1 = mode+1;// must not be equal to 'mode'
	ObserverSetTarget(pTarget);
	ObserverSetMode((mode==OBS_NONE)?m_iObserverLastMode:mode);

	// Tell all clients this player is now a spectator
	MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
		WRITE_BYTE(entindex());
		WRITE_BYTE(max(1,pev->iuser1));// XDM3037a: was 1);
	MESSAGE_END();

	if (pev->iuser1 != OBS_ROAMING && pev->iuser1 != OBS_MAP_FREE && m_hObserverTarget.Get() == NULL)
		ObserverFindNextPlayer(false);// Find a player to watch

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Leave observer mode
// Warning: If pev->team is not set (TEAM_NONE), team menu is shown and this function fails.
//-----------------------------------------------------------------------------
bool CBasePlayer::ObserverStop(void)
{
	DBG_PLR_PRINT("CBasePlayer(%d)::ObserverStop()\n", entindex());
	if (g_pGameRules && !g_pGameRules->PlayerStopObserver(this))// XDM3037a
		return false;

	// Turn off spectator
	if (pev->iuser1 || pev->iuser2)
	{
		// Tell all clients this player is not a spectator anymore
		MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
			WRITE_BYTE(ENTINDEX(edict()));
			WRITE_BYTE(0);
		MESSAGE_END();
		pev->iuser1 = 0;
		pev->iuser2 = 0;
		pev->iuser3 = 0;// XDM3037
	}

	ClearBits(pev->flags, FL_DUCKING|FL_ONGROUND|FL_INWATER|FL_SPECTATOR);
	m_iHideHUD &= ~HIDEHUD_ALL;
	m_hObserverTarget = NULL;
	m_afPhysicsFlags = 0;
	m_flNextObserverInput = -1;
	m_hLastKiller = NULL;
	m_hLastVictim = NULL;
	pev->effects = 0;
	pev->fixangle = FALSE;
	pev->maxspeed = g_psv_maxspeed->value;
	//pev->team = TEAM_NONE;// DON'T! This is called after ChangePlayerTeam()

	Spawn();
	if (g_pGameRules)
		g_pGameRules->InitHUD(this);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Reliably tell if this player is a spectator/observer
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::IsObserver(void) const
{
	if (pev->iuser1 != OBS_NONE && pev->iuser1 != OBS_INTERMISSION)
		return true;

	return false;
}
