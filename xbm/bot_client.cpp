//====================================================================
// Purpose: Server-to-client message handlers
//====================================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "bot.h"
#include "bot_func.h"
#include "bot_client.h"
#include "bot_cvar.h"
#include "bot_weapons.h"

extern int recent_bot_taunt[];
extern bot_chat_t bot_taunt[];

int g_iGameType = 0;
int g_iGameMode = 0;
int g_iGameState = GAME_STATE_WAITING;// XDM3038b
int g_iSkillLevel = 0;
int g_iPlayerMaxHealth = MAX_PLAYER_HEALTH;// XDM3038
int g_iPlayerMaxArmor = MAX_NORMAL_BATTERY;

//DLL_GLOBAL char weapon_classnames[MAX_WEAPONS][64];
//bad idea - ammo IDs and max DLL_GLOBAL ItemInfo weapon_defs[MAX_WEAPONS];// array of weapon definitions
DLL_GLOBAL bot_weapon_t weapon_defs[MAX_WEAPONS];// array of weapon definitions

// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
// Called many times for each WRITE_BYTE()/etc.
void BotClient_Valve_WeaponList(void *p, int bot_index)
{
	static short state = 0;// current state machine state
	static bot_weapon_t bot_weapon;

	if (state == 0)
	{
		state++;
		strncpy(bot_weapon.szClassname, (char *)p, MAX_WEAPON_NAME_LEN);
	}
	else if (state == 1)
	{
		state++;
		bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
#ifndef OLD_WEAPON_AMMO_INFO
		++state;// pretend we're skipping one WRITE
#endif
	}
#ifdef OLD_WEAPON_AMMO_INFO
	else if (state == 2)
	{
		state++;
		bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
	}
#endif
	else if (state == 3)
	{
		state++;
		bot_weapon.iAmmo2 = *(int *)p;  // ammo index 2
#ifndef OLD_WEAPON_AMMO_INFO
		++state;// pretend we're skipping one WRITE
#endif
	}
#ifdef OLD_WEAPON_AMMO_INFO
	else if (state == 4)
	{
		state++;
		bot_weapon.iAmmo2Max = *(int *)p;  // max ammo2
	}
#endif
	else if (state == 5)
	{
		state++;
		bot_weapon.iSlot = *(int *)p;  // slot for this weapon
	}
	else if (state == 6)
	{
		state++;
		bot_weapon.iPosition = *(int *)p;  // position in slot
	}
	else if (state == 7)
	{
		state++;
		bot_weapon.iId = *(byte *)p;  // weapon ID
	}
	else if (state == 8)
	{
		state = 0;
		bot_weapon.iFlags = *(byte *)p;  // flags for weapon (WTF???)
		// store away this weapon with it's ammo information...
		weapon_defs[bot_weapon.iId] = bot_weapon;
	}
}

// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_Valve_CurrentWeapon(void *p, int bot_index)
{
	static short state = 0;// current state machine state
	static byte iState;
	static byte iId;
	static byte iClip;

	if (state == 0)
	{
		state++;
		iState = *(byte *)p;  // state of the current weapon
	}
	else if (state == 1)
	{
		state++;
		iId = *(byte *)p;  // weapon ID of current weapon
	}
	else if (state == 2)
	{
		state = 0;
		iClip = *(byte *)p;  // ammo currently in the clip for this weapon
		if (iId < MAX_WEAPONS)
		{
			bot_t *pBot = &bots[bot_index];
			//if (pBot && g_pWeaponSelectTable)
			//pBot->bot_weapons |= (1<<iId);  // set this weapon bit
			if (pBot)
				pBot->pEdict->v.weapons |= (1<<iId);// XDM3037: TESTME: failproof?

			if (iState == 1)
			{
				pBot->current_weapon.iId = iId;
				pBot->current_weapon.iClip = iClip;
				pBot->current_weapon.weapon_select_table_index = GetWeaponSelectIndex(iId);
			}
			//conprintf(1, "BotClient_Valve_CurrentWeapon: set %d clip %d state %d\n", iId, iClip, iState);
			iId = WEAPON_NONE;
		}
	}
}

// This message is sent whenever ammo amounts are adjusted (up or down).
void BotClient_Valve_AmmoX(void *p, int bot_index)
{
	static short state = 0;// current state machine state
	static byte index;
//	static byte amount;
//	int ammo_index;
	
	if (state == 0)
	{
		state++;
		index = *(int *)p;  // ammo index (for type of ammo) // XDM3038: argument is passed as int to the engine
	}
	else if (state == 1)
	{
		bot_t *pBot = &bots[bot_index];
//		amount = *(byte *)p;  // the amount of ammo currently available
		pBot->m_rgAmmo[index] = *(int *)p;  // store it away // XDM3038: argument is passed as int to the engine
//		ammo_index = pBot->current_weapon.iId;
		// update the ammo counts for this weapon...
// XDM3037		pBot->current_weapon.iAmmo1 = pBot->m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
// XDM3037		pBot->current_weapon.iAmmo2 = pBot->m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
		state = 0;

//		conprintf(1, "BotClient_Valve_AmmoX: set %d amount %d\n", index, pBot->m_rgAmmo[index]);
	}
}

// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_Valve_AmmoPickup(void *p, int bot_index)
{
	static short state = 0;// current state machine state
	static byte ammoindex;

	if (state == 0)
	{
		state++;
		ammoindex = *(byte *)p;
	}
	else if (state == 1)
	{
		bot_t *pBot = &bots[bot_index];
		if (pBot)
			pBot->m_rgAmmo[ammoindex] = *(byte *)p;

//		ammo_index = pBot->current_weapon.iId;
		// update the ammo counts for this weapon...
//		pBot->current_weapon.iAmmo1 = pBot->m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
//		pBot->current_weapon.iAmmo2 = pBot->m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
		state = 0;
	}
}

int g_iValve_WeaponPickupID = WEAPON_NONE;
// This message gets sent when the bot picks up a weapon.
void BotClient_Valve_WeaponPickup(void *p, int bot_index)
{
	g_iValve_WeaponPickupID = *(int *)p;// HPB40
	// no calls here!
}

void BotClient_Valve_WeaponPickupEnd(void *p, int bot_index)
{
	if (g_iValve_WeaponPickupID == WEAPON_NONE)
		return;

	// set this weapon bit to indicate that we are carrying this weapon
	//bots[bot_index].bot_weapons |= (1<<index);// HPB40
	bot_t *pBot = &bots[bot_index];
	pBot->pEdict->v.weapons |= (1<<g_iValve_WeaponPickupID);

	// XDM3037: check if we picked up a better weapon
	int iBestID = BotGetNextBestWeapon(pBot, pBot->current_weapon.iId);
	if (iBestID != WEAPON_NONE)
	{
		//conprintf(1, "XBM: bot %s picked %d and has better weapon (%d) than (%d) UTIL_SelectItem()\n", STRING(pBot->pEdict->v.netname), g_iValve_WeaponPickupID, iBestID, pBot->current_weapon_id);
		//UTIL_SelectItem(pBot->pEdict, iBestID);// this creates another message, so it's ONLY safe to call from End function
		//UTIL_SelectWeapon(pBot->pEdict, iBestID);// XDM3037a
		pBot->m_iQueuedSelectWeaponID = iBestID;// XDM3038
		pBot->current_weapon.iId = iBestID;// hack: just try to avoid checking this
	}
	g_iValve_WeaponPickupID = WEAPON_NONE;
}

// This message gets sent when the bot picks up an item (like a battery or a healthkit)
void BotClient_Valve_ItemPickup(void *p, int bot_index)
{
//	if (mod_id == XDM_DLL)// XDM3035a: first byte indicates quantity
}

// This message gets sent when the bots health changes.
// XDM: just use pev->health, ok?
/*void BotClient_Valve_Health(void *p, int bot_index)
{
//	bots[bot_index].bot_health = *(int *)p;// HPB40
}*/

// This message gets sent when the bots armor changes.
// XDM: just use pev->armorvalue, ok?
/*void BotClient_Valve_Battery(void *p, int bot_index)
{
//	bots[bot_index].bot_armor = *(int *)p;// HPB40
}*/

// This message gets sent when the bots are getting damaged.
void BotClient_Valve_Damage(void *p, int bot_index)
{
	static short state = 0;// current state machine state
	static byte damage_armor;
	static byte damage_taken;
	static long damage_bits;  // type of damage being done
	static Vector damage_origin;
	bot_t *pBot = &bots[bot_index];

	if (state == 0)
	{
		state++;
		damage_armor = *(byte *)p;
	}
	else if (state == 1)
	{
		state++;
		damage_taken = *(byte *)p;
	}
	else if (state == 2)
	{
		state++;
		damage_bits = *(long *)p;
	}
	else if (state == 3)
	{
		state++;
		damage_origin.x = *(float *)p;
	}
	else if (state == 4)
	{
		state++;
		damage_origin.y = *(float *)p;
	}
	else if (state == 5)
	{
		state = 0;
		damage_origin.z = *(float *)p;
		if ((damage_armor > 0) || (damage_taken > 0))
		{
			if (damage_bits & DMG_DROWN)// XDM3035: drowning, swim up!
			{
				//BotUnderWater(pBot);
				if (pBot->pEdict->v.waterlevel >= 3)
				{
					pBot->f_exit_water_time = gpGlobals->time;
					pBot->pEdict->v.idealpitch = PITCH_LOOK_UP;
					// swim up towards the surface
					pBot->pEdict->v.v_angle.x = PITCH_LOOK_UPWD;// look upwards
					// move forward (i.e. in the direction the bot is looking, up or down)
					pBot->pEdict->v.button |= (IN_FORWARD | IN_JUMP | IN_MOVEUP);// XDM3038a
					if (g_psv_maxspeed)
						pBot->cmd_upmove = g_psv_maxspeed->value;

					return;
				}
			}

			// if the bot doesn't have an enemy and someone is shooting at it then turn in the attacker's direction...
			if (pBot->pBotEnemy == NULL && FBitSet(damage_bits, BOT_DMGM_REACT))// XDM3038c
			{
				// face the attacker...
				Vector v_enemy = damage_origin - bots[bot_index].pEdict->v.origin;
				//Vector bot_angles = UTIL_VecToAngles(v_enemy);
				//pBot->pEdict->v.ideal_yaw = bot_angles.y;
				pBot->pEdict->v.ideal_yaw = VecToYaw(v_enemy);
				BotFixIdealYaw(pBot->pEdict);
				// stop using health or HEV stations...
				pBot->b_use_health_station = FALSE;
				pBot->b_use_HEV_station = FALSE;
			}
		}
	}
}

void BotClient_Valve_DeathMsg(void *p, int bot_index)// This message gets sent when the bots get killed
{
	static short state = 0;// current state machine state
	static CLIENT_INDEX killer_index;
	static CLIENT_INDEX victim_index;
	static edict_t *killer_edict;
	static edict_t *victim_edict;
	static short victimbotindex;// index in bot array
	static short killerbotindex;

	if (state == 0)
	{
		state++;
		killer_index = *(int *)p;  // ENTINDEX() of killer
	}
	else if (state == 1)
	{
		state++;
		victim_index = *(int *)p;  // ENTINDEX() of victim
	}
	else if (state == 2)
	{
		state = 0;
		killer_edict = INDEXENT(killer_index);
		victim_edict = INDEXENT(victim_index);
		// get the bot index of the killer...
		killerbotindex = UTIL_GetBotIndex(killer_edict);
		// is this message about a bot killing someone?
		if (killerbotindex != -1)
		{
			if (killer_index != victim_index)  // didn't kill self...
			{
				if ((RANDOM_LONG(1, 100) <= bots[killerbotindex].logo_percent) && (num_logos))
				{
					bots[killerbotindex].b_spray_logo = TRUE;  // this bot should spray logo now
					bots[killerbotindex].f_spray_logo_time = gpGlobals->time;
				}
			}

			if (victim_edict != NULL)
			{
				if (bots[killerbotindex].pBotKiller == victim_edict)// I killed my killer
					bots[killerbotindex].need_to_avenge = false;

				if ((g_bot_chat_enable.value > 0.0f) && (RANDOM_LONG(1,100) <= bots[killerbotindex].taunt_percent))
					BotSpeakTaunt(&bots[killerbotindex], victim_edict);
			}
		}
		// get the bot index of the victim...
		victimbotindex = UTIL_GetBotIndex(victim_edict);
		// is this message about a bot being killed?
		if (victimbotindex != -1)
		{
			if ((killer_index == 0) || (killer_index == victim_index))
			{
				bots[victimbotindex].pBotKiller = NULL;// bot killed by world (worldspawn) or bot killed self...
				bots[victimbotindex].need_to_avenge = false;
			}
			else
			{
				bots[victimbotindex].pBotKiller = killer_edict;//INDEXENT(killer_index);// store edict of player that killed this bot...
				bots[victimbotindex].need_to_avenge = true;
			}
		}
	}
}

void BotClient_Valve_ScreenFade(void *p, int bot_index)
{
	static short state = 0;// current state machine state
	static int duration;
	static int hold_time;
	static int fade_flags;
	
	if (state == 0)
	{
		state++;
		duration = *(int *)p;
	}
	else if (state == 1)
	{
		state++;
		hold_time = *(int *)p;
	}
	else if (state == 2)
	{
		state++;
		fade_flags = *(int *)p;
	}
	else if (state == 6)
	{
		state = 0;
		
		float length = (float)(duration + hold_time) / 4096.0f;
		bots[bot_index].blinded_time = gpGlobals->time + length - 2.0f;
	}
	else
		state++;
}

void BotClient_CS_HLTV(void *p, int bot_index)// HPB40
{
	static short state = 0;// current state machine state
	static int players;
	short index;

	if (state == 0)
		players = *(int *) p;
	else if (state == 1)
	{
		// new round in CS 1.6
		if ((players == 0) && (*(int *) p == 0))
		{
			for (index = 0; index < MAX_PLAYERS; ++index)
			{
				if (IsActiveBot(index))
					BotSpawnInit(&bots[index]); // reset bots for new round
			}
		}
	}
}

static int g_iGameModeArgument = 0;

void BotClient_XDM_GameMode(void *p, int bot_index)
{
	if (g_iGameModeArgument == 0)
		g_iGameType = *(byte *)p;
	else if (g_iGameModeArgument == 1)
		g_iGameMode = *(byte *)p;
	else if (g_iGameModeArgument == 2)// XDM3038b: FIX
		g_iGameState = *(byte *)p;
	else if (g_iGameModeArgument == 3)
		g_iSkillLevel = *(byte *)p;// we can safely skip unused values as long as we increas the counter
	else if (g_iGameModeArgument == 11)// XDM3038a: new protocol
		g_iPlayerMaxHealth = *(byte *)p;
	else if (g_iGameModeArgument == 12)// XDM3038a: new protocol
		g_iPlayerMaxArmor = *(byte *)p;

	++g_iGameModeArgument;
}

// XDM3037: sizeless, reliable method
void BotClient_XDM_GameModeEnd(void *p, int bot_index)
{
	g_iGameModeArgument = 0;

	// XDM3038b: changed to match message order in DLL gamerules code
	if (g_iGameState == GAME_STATE_FINISHED && g_bot_chat_enable.value > 0.0f)
	{
		bot_t *pBot;
		int i = 0;
		for (i=0; i < MAX_PLAYERS; ++i)
		{
			if (!IsActiveBot(i))
				continue;

			pBot = &bots[i];
			pBot->f_move_speed = 0.0f;
			pBot->f_bot_chat_time += 10.0f;// delay all normal chatting
			// in BotThink()	pBot->pEdict->v.button |= (IN_ATTACK|IN_JUMP);// XDM3037: press ready buttons
			//NO YET!	pBot->autospawn_state = RESPAWN_NEED_TO_SPAWN;// XDM3037: need to spawn after level change

			// Speak something towards winners/losers
			if (g_iGameType != GT_COOP)// in CoOp this looks stupid
			{
				CLIENT_INDEX winner = pBot->pEdict->v.iuser2;// should be same for all
				edict_t *pWinner = UTIL_ClientByIndex(winner);
				if (pWinner)
				{
					if (pBot->pEdict == pWinner)//if (ENTINDEX(pBot->pEdict) == winner)// I am the winner
					{
						edict_t *pRandomEnemy = NULL;
						CLIENT_INDEX enemy = RANDOM_LONG(1, gpGlobals->maxClients);
						for (int j=0; j < gpGlobals->maxClients; ++j)// counter, not index
						{
							pRandomEnemy = UTIL_ClientByIndex(enemy);
							if (pRandomEnemy == NULL)// crash prevention
								continue;
							if (pRandomEnemy == pBot->pEdict)
								continue;// don't talk to self
							if (pRandomEnemy == pBot->pBotUser)
								continue;// don't insult frients
							if (pRandomEnemy && (pWinner->v.team == TEAM_NONE || pRandomEnemy->v.team != pWinner->v.team))
								break;

							if (enemy < gpGlobals->maxClients)
								++enemy;
							else
								enemy = 1;// loop
						}
						BotSpeakTaunt(pBot, pRandomEnemy);// don't randomize, winner speaks always
						pBot->f_bot_say = gpGlobals->time + 1.0f;// override the timing
					}
					else// this bot is not the winner
					{
						if (IsTeamplay() && pBot->pEdict->v.team == pWinner->v.team)// this bot is from the winning team
						{
							if ((RANDOM_LONG(1,100) <= pBot->taunt_percent))// don't ALWAYS speak so the chat won't get overflowed
							{
								edict_t *pLoser = NULL;
								CLIENT_INDEX lid = RANDOM_LONG(1, gpGlobals->maxClients);
								for (int j=0; j < gpGlobals->maxClients; ++j)// don't loop more than once through all clients
								{
									pLoser = UTIL_ClientByIndex(lid);
									if (pLoser == NULL)// crash prevention
										continue;
									if (pLoser->v.team != pWinner->v.team && pLoser != pBot->pEdict)// found some player from non-winning team
										break;

									++lid;
									if (lid > gpGlobals->maxClients)
										lid = 1;
								}
								BotSpeakTaunt(pBot, pLoser);
								//pBot->f_bot_say = gpGlobals->time + RANDOM_FLOAT(1,2);// override the timing
							}
						}
						else// this bot is NOT from the winning team
						{
							if (pWinner != pBot->pBotUser && (RANDOM_LONG(1,100) <= pBot->whine_percent))// don't ALWAYS speak so the chat won't get overflowed
								BotSpeakWhine(pBot, pWinner);
						}
						pBot->f_bot_say = gpGlobals->time + 1.0f + (float)i*0.5f;// let everyone get talkative, but not at the same time
					}
				}
			}// !COOP
		}
	}
}


static byte g_iSelBestItemCurrentID = WEAPON_NONE;

void BotClient_XDM_SelBestItem(void *p, int bot_index)
{
	g_iSelBestItemCurrentID = *(byte *)p;// the only parameter
}

void BotClient_XDM_SelBestItemEnd(void *p, int bot_index)
{
//	DBG_BOT_PRINT("BotClient_XDM_SelBestItemEnd()\n");
	bot_t *pBot = &bots[bot_index];
	ASSERT(pBot != NULL);
	int iBestID = BotGetNextBestWeapon(pBot, g_iSelBestItemCurrentID);
	g_iSelBestItemCurrentID = WEAPON_NONE;
	if (iBestID != WEAPON_NONE)
		pBot->m_iQueuedSelectWeaponID = iBestID;
// NO! Stack overflow!		UTIL_SelectItem(pBot->pEdict, iBestID);
}


static int g_iWriteIndexWeaponList = 0;
static bot_weapon_t g_WeaponListData;
// XDM3035b
void BotClient_XDM_WeaponList(void *p, int bot_index)
{
	DBG_BOT_PRINT("BotClient_XDM_WeaponList()\n");
	if (g_iWriteIndexWeaponList == 0)
	{
		memset(&g_WeaponListData, 0, sizeof(bot_weapon_t));
		g_iWriteIndexWeaponList++;
		strncpy(g_WeaponListData.szClassname, (char *)p, MAX_WEAPON_NAME_LEN);
		g_WeaponListData.iSlot = 0;
		g_WeaponListData.iPosition = 0;
	}
	else if (g_iWriteIndexWeaponList == 1)
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iAmmo1 = *(signed char *)p;  // ammo index can be -1!
#ifndef OLD_WEAPON_AMMO_INFO
		g_iWriteIndexWeaponList++;// pretend we're skipping one WRITE so we catch next index
#endif // OLD_WEAPON_AMMO_INFO
	}
#ifdef OLD_WEAPON_AMMO_INFO
	else if (g_iWriteIndexWeaponList == 2)
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iAmmo1Max = *(byte *)p;  // max ammo1
	}
#endif // OLD_WEAPON_AMMO_INFO
	else if (g_iWriteIndexWeaponList == 3)
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iAmmo2 = *(signed char *)p;  // ammo index
#ifndef OLD_WEAPON_AMMO_INFO
		g_iWriteIndexWeaponList++;// pretend we're skipping one WRITE
#endif // !OLD_WEAPON_AMMO_INFO
#ifndef SERVER_WEAPON_SLOTS
		g_iWriteIndexWeaponList += 2;// skip 2
#endif // SERVER_WEAPON_SLOTS
	}
#ifdef OLD_WEAPON_AMMO_INFO
	else if (g_iWriteIndexWeaponList == 4)
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iAmmo2Max = *(byte *)p;// max ammo2
		startafterskip = g_iWriteIndexWeaponList;
	}
#endif // OLD_WEAPON_AMMO_INFO
#ifdef SERVER_WEAPON_SLOTS// skip 2
	else if (g_iWriteIndexWeaponList == 5)
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iSlot = *(byte *)p;// slot for this weapon
	}
	else if (g_iWriteIndexWeaponList == 6)
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iPosition = *(byte *)p;// position in slot
	}
#endif // SERVER_WEAPON_SLOTS
	else if (g_iWriteIndexWeaponList == 7)// 7 or 5
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iId = *(byte *)p;// weapon ID
	}
	else if (g_iWriteIndexWeaponList == 8)// XDM3037A: 8 or 6
	{
		g_iWriteIndexWeaponList++;
		g_WeaponListData.iMaxClip = *(byte *)p;// iMaxClip
	}
	else if (g_iWriteIndexWeaponList == 9)// 9 or 7
	{
		g_iWriteIndexWeaponList = 0;
		g_WeaponListData.iFlags = *(byte *)p;// iFlags
		// store away this weapon with it's ammo information...
		weapon_defs[g_WeaponListData.iId] = g_WeaponListData;
	}
}


// XDM3035 multiple updates combined into one packet
static int g_iWriteIndexAmmoList = 0;

void BotClient_XDM_AmmoList(void *p, int bot_index)
{
	DBG_BOT_PRINT("BotClient_XDM_AmmoList()\n");
	// fill g_AmmoInfoArray
	if (g_iWriteIndexAmmoList == 0)// byte ID
	{
		giAmmoIndex = *(byte *)p;
		if (giAmmoIndex == 0)// reset
		{
			giAmmoIndex = 0;
			memset(g_AmmoInfoArray, 0, sizeof(AmmoInfo)*MAX_AMMO_SLOTS);// XDM3037
		}
		//g_AmmoInfoArray[*(byte *)p].
		++g_iWriteIndexAmmoList;
	}
	else if (g_iWriteIndexAmmoList == 1)// byte/short MAX
	{
		g_AmmoInfoArray[giAmmoIndex].iMaxCarry = *(int *)p;
		++g_iWriteIndexAmmoList;
	}
	else if (g_iWriteIndexAmmoList == 2)// string NAME
	{
		strcpy(g_AmmoInfoArray[giAmmoIndex].name, (char *)p);
		g_iWriteIndexAmmoList = 0;// pretend we're reading from the beginning
	}
}

void BotClient_XDM_AmmoListEnd(void *p, int bot_index)
{
	DBG_BOT_PRINT("BotClient_XDM_AmmoListEnd()\n");
	g_iWriteIndexAmmoList = 0;
}


// XDM3035 multiple updates combined into one packet
static int g_iWriteIndexUpdWeapons = 0;
static int g_iWriteIndexUpdWeaponsSubsection = 0;

void BotClient_XDM_UpdWeapons(void *p, int bot_index)
{
	static byte wstate = 0;
	static byte wid = 0;
	static byte wclip = 0;
	if (g_iWriteIndexUpdWeaponsSubsection == 0)
	{
		wstate = *(byte *)p;// state of the current weapon
		g_iWriteIndexUpdWeaponsSubsection++;
	}
	else if (g_iWriteIndexUpdWeaponsSubsection == 1)
	{
		wid = *(byte *)p;// weapon ID of current weapon
		g_iWriteIndexUpdWeaponsSubsection++;
	}
	else if (g_iWriteIndexUpdWeaponsSubsection == 2)
	{
#if defined(DOUBLE_MAX_AMMO)
		wclip = *(short *)p;  // ammo currently in the clip for this weapon
#else
		wclip = *(byte *)p;
#endif
		if (wid > WEAPON_NONE && wid < MAX_WEAPONS)
		{
			bot_t *pBot = &bots[bot_index];
			if (pBot)
				pBot->pEdict->v.weapons |= (1<<wid);

			if (wstate == wstate_current || wstate == wstate_current_ontarget)// XDM3038b
			{
				pBot->current_weapon.iId = wid;
				pBot->current_weapon.iClip = wclip;
				pBot->current_weapon.weapon_select_table_index = GetWeaponSelectIndex(wid);
			}
			//conprintf(1, "BotClient_Valve_CurrentWeapon: set %d clip %d state %d\n", iId, iClip, iState);
		}
		wid = WEAPON_NONE;
		g_iWriteIndexUpdWeaponsSubsection = 0;
	}

	++g_iWriteIndexUpdWeapons;
}

void BotClient_XDM_UpdWeaponsEnd(void *p, int bot_index)
{
	g_iWriteIndexUpdWeapons = 0;
/*#ifndef SERVER_WEAPON_SLOTS
	if (g_iByteIndexUpdWeapons > 0 && iLastUpdatedWeaponId != WEAPON_NONE && bots[bot_index].current_weapon.iId == WEAPON_NONE)// XDM3037
		UTIL_SelectItem(iLastUpdatedWeaponId);// well, it's a little bit of a hack - because we still have no updates from weapon bits and thus HaveWeapon() fails, we have to explicitly select ANY acquired item
#endif*/
}

// XDM3035 multiple updates combined into one packet
static int g_iWriteIndexUpdAmmo = 0;

void BotClient_XDM_UpdAmmo(void *p, int bot_index)
{
	static byte ammoindex;

	if (g_iWriteIndexUpdAmmo & 1)// 2nd byte - value
	{
		bot_t *pBot = &bots[bot_index];
		if (pBot)
		{
#ifdef DOUBLE_MAX_AMMO
			pBot->m_rgAmmo[ammoindex] = *(uint16 *)p;
#else
			pBot->m_rgAmmo[ammoindex] = *(byte *)p;
#endif
			//old pBot->current_weapon.iAmmo1 = pBot->m_rgAmmo[weapon_defs[pBot->current_weapon.iId].iAmmo1];
			//old pBot->current_weapon.iAmmo2 = pBot->m_rgAmmo[weapon_defs[pBot->current_weapon.iId].iAmmo2];
		}
	}
	else// if (!(g_iWriteIndexUpdAmmo & 1))// 1st byte - ammo index
	{
		ammoindex = *(byte *)p;  // the amount of ammo currently available
	}
	++g_iWriteIndexUpdAmmo;
}

void BotClient_XDM_UpdAmmoEnd(void *p, int bot_index)
{
	g_iWriteIndexUpdAmmo = 0;
}

/*void BotClient_XDM_TeamNames(void *p, int bot_index)
{
}*/

static int g_iByteIndexShowMenu = 0;

// byte ID, byte flags
void BotClient_XDM_ShowMenu(void *p, int bot_index)
{
	if (g_iByteIndexShowMenu == 0)
	{
		bot_t *pBot = &bots[bot_index];
		if (pBot)
			pBot->m_iCurrentMenu = *(byte *)p;
	}
	++g_iByteIndexShowMenu;
}

void BotClient_XDM_ShowMenuEnd(void *p, int bot_index)
{
	g_iByteIndexShowMenu = 0;
}
