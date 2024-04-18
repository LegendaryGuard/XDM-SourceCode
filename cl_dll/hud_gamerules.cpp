#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
#include "pm_shared.h"

#define EVENTICON_FADE_TIME 2.0f

// Must end with 0-terminator
int g_iszScoreAnnounceValues[] =
{
	1000, 500, 400, 300, 200, 100, 50, 30, 20, 10, 5, 3, 2, 1, 0
};

int g_iszTimeAnnounceValues[] =
{
	6000, 3000, 1200, 600, 300, 180, 60, 30, 10, 0
};


//-----------------------------------------------------------------------------
// Purpose: Is local player a spectator?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHud::IsSpectator(void)
{
	if (g_iUser1 != OBS_NONE)// NO! We may check this during intermission too!- && g_iUser1 != OBS_INTERMISSION)
	{
		if (m_pLocalPlayer)
			return (g_PlayerExtraInfo[m_pLocalPlayer->index].observer > 0);//return ::IsSpectator(localplayer->index);
			//return (g_IsSpectator[localplayer->index] > 0);
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a Local player spectator mode
// Output : SPEC_NONE
//-----------------------------------------------------------------------------
int CHud::GetSpectatorMode(void)// XDM3038a
{
	return g_iUser1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a Spectator/camera target
// Output : cl_entity_t *
//-----------------------------------------------------------------------------
//cl_entity_t *CHud::GetSpectatorTarget(void)
int CHud::GetSpectatorTarget(void)
{
	if (GetSpectatorMode() == OBS_NONE || GetSpectatorMode() == OBS_ROAMING || GetSpectatorMode() == OBS_MAP_FREE)
		return 0;

	return g_iUser2;//gEngfuncs.GetEntityByIndex(g_iUser2);
}

//-----------------------------------------------------------------------------
// Purpose: IntermissionStart (end of game) // XDM3035
// Warning: This is called too late! Server messages have already arrived!
//-----------------------------------------------------------------------------
void CHud::IntermissionStart(void)
{
	DBG_PRINTF("CHud::IntermissionStart()\n");
	m_Ammo.UpdateCrosshair(CROSSHAIR_OFF);// XDM
	CenterPrint("");// hopefully this will clear any "sudden messages"

#if defined (_DEBUG)
	if (!IsExtraScoreBasedGame(m_iGameType))
	{
		gViewPort->GetScoreBoard()->DumpInfo();
		//ASSERT(g_TeamInfo[gViewPort->GetScoreBoard()->GetBestTeam()].frags >= m_iScoreLimit);
	}
#endif
	for (size_t i=0; i<=MAX_PLAYERS; ++i)// XDM3038a
		g_PlayerExtraInfo[i].ready = 0;

	//m_iTimeLimit = 0;// XDM3038a: don't reset it here!
	//?m_iJoinTime = 0;
	m_iScoreLeft = 0;
	m_iTimeLeftLast = 0;
	m_iScoreLeftLast = 0;
	m_iDistortMode = 0;// XDM3037a: at the end of game this is annoying
	m_fDistortValue = 0.0f;

	if (g_iUser1 == OBS_INTERMISSION)// XDM3035 TODO: revisit
		GameRulesEndGame();

	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
		gViewPort->HideVGUIMenu();
		//ShowScoreBoard includes this		gViewPort->GetScoreBoard()->UpdateTitle();// XDM3035a: set the end-of-the-match title
		// doesn't help gViewPort->requestFocus();
		gViewPort->UpdateSpectatorPanel();
		gViewPort->ShowScoreBoard();
		gViewPort->GetScoreBoard()->SetClickMode(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Intermission ends (does not work as expected)
// Warning: Called AFTER MsgFunc_GameMode, so DON'T reset any sensitive data like game limits!
//-----------------------------------------------------------------------------
void CHud::IntermissionEnd(void)
{
	DBG_PRINTF("CHud::IntermissionEnd()\n");
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
		gViewPort->HideScoreBoard();
		gViewPort->UpdateSpectatorPanel();
	}
	//NO!m_iTimeLimit = 0;
	m_flTimeLeft = m_iTimeLimit;
	m_iScoreLeft = m_iScoreLimit;
	m_iTimeLeftLast = 0;
	m_iScoreLeftLast = 0;
	m_iDistortMode = 0;// XDM3037a: at the end of game this is annoying
	m_fDistortValue = 0.0f;
	m_Ammo.UpdateCrosshair(CROSSHAIR_NORMAL);// XDM
	m_flNextAnnounceTime = 0.0f;// XDM3035
}

//-----------------------------------------------------------------------------
// XDM3035a
//-----------------------------------------------------------------------------
void CHud::CheckRemainingScoreAnnouncements(void)
{
#if defined (_DEBUG_GAMERULES)
	DBG_PRINTF("CHud::CheckRemainingScoreAnnouncements()\n");
#endif
	// better safe than sorry
	if (m_iIntermission > 0)
		return;
	if (gEngfuncs.GetMaxClients() <= 1)
		return;

	if (m_iScoreLeft > 0 && m_iScoreLeft != m_iScoreLeftLast)
	{
		//if (m_iScoreLeftLast != 0)
		// XDM3038: not always true	ASSERT(m_iScoreLeft <= m_iScoreLeftLast+1);

		//conprintf(1, "CHud::CheckRemainingScoreAnnouncements(%d)\n", m_iScoreLeft);
		// GRInfo messages are already filtered, but there are other sources which can trigger this more often
		if (Array_FindInt(g_iszScoreAnnounceValues, m_iScoreLeft) != -1)// XDM3037a
		{
			//if (m_flNextAnnounceTime <= m_flTime)// avoid to be triggered more than once per second
			{
				char *msgname = NULL;
				if (IsExtraScoreBasedGame(m_iGameType))
				{
					msgname = "MP_SCORELEFT_PTS";
				}
				else if (m_iGameType == GT_COOP)
				{
					if (m_iGameMode == COOP_MODE_LEVEL)
						msgname = "MP_SCORELEFT_TOUCH";
					else
						msgname = "MP_SCORELEFT_MONS";
				}
				else
				{
					msgname = "MP_SCORELEFT";
				}
				client_textmessage_t *msg = TextMessageGet(msgname);
				if (msg)
				{
					m_MessageScoreLeft = *msg;// copy localized message
					memcpy(&m_MessageScoreLeft, msg, sizeof(client_textmessage_t));// copy localized message
					//strncpy(m_szMessageScoreLeft, msg->pMessage, ANNOUNCEMENT_MSG_LENGTH);// store message TEXT
					_snprintf(m_szMessageScoreLeft, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, m_iScoreLeft);// format the string
					m_szMessageScoreLeft[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
					m_MessageScoreLeft.pMessage = m_szMessageScoreLeft;
					m_Message.MessageAdd(&m_MessageScoreLeft);
				}
				//else
					conprintf(0, "- %d score points left\n", m_iScoreLeft);

				//m_flNextAnnounceTime = m_flTime + 3.0f;
				//m_iScoreLeftLast = m_iScoreLeft;
			}
		}
		m_iScoreLeftLast = m_iScoreLeft;
	}
}

//-----------------------------------------------------------------------------
// XDM3035a
//-----------------------------------------------------------------------------
void CHud::CheckRemainingTimeAnnouncements(void)
{
//	DBG_PRINTF("CHud::CheckRemainingTimeAnnouncements()\n");
	// better safe than sorry
	if (m_iIntermission > 0)
		return;
	if (gEngfuncs.GetMaxClients() <= 1)
		return;

	uint32 timer = 0;

	if (m_flTimeLeft > 0.0)
		timer = (uint32)m_flTimeLeft;

	/*cl_entity_t *pWorld = gEngfuncs.GetEntityByIndex(0);	// get world
	if (pWorld)
	{
		timer = pWorld->curstate.impacttime;
		conprintf(0, "CHud::Think(%d) pWorld\n", timer);
	}*/
	if (timer > 0 && timer != m_iTimeLeftLast)
	{
		//conprintf(1, "CHud::CheckRemainingTimeAnnouncements(%d)\n", timer);
		if (Array_FindInt(g_iszTimeAnnounceValues, timer) != -1)// XDM3037a
		{
			if (m_flNextAnnounceTime <= m_flTime)// avoid to be triggered more than once per second
			{
				char msgname[32];// message/sound name + NULLterm
				_snprintf(msgname, 32, "MP_TIMELEFT%d\0", timer);
				client_textmessage_t *msg = TextMessageGet(msgname);
				if (msg)
				{
					m_MessageTimeLeft = *msg;// copy localized message
					//memcpy(&m_MessageTimeLeft, msg, sizeof(client_textmessage_t));// copy localized message
					strncpy(m_szMessageTimeLeft, msg->pMessage, ANNOUNCEMENT_MSG_LENGTH);// store message TEXT
					m_szMessageTimeLeft[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
					m_MessageTimeLeft.pMessage = m_szMessageTimeLeft;
					m_Message.MessageAdd(&m_MessageTimeLeft);
				}
				//else
					conprintf(0, "- %d seconds left\n", timer);

				_snprintf(msgname, 32, "!CTR%d\0", timer);//_snprintf(msgname, 32, "announcer/timeleft%d.wav\0", timer);
				PlaySoundAnnouncer(msgname, 3.0f);
			}
		}
		else if (timer <= 5)
		{
			char msgname[16];
			_snprintf(msgname, 16, "!CTR%d\0", timer);
			PlaySoundAnnouncer(msgname, 0.9f);
		}
		m_iTimeLeftLast = timer;
	}
}


//-----------------------------------------------------------------------------
// Purpose: GameRulesEndGame
//-----------------------------------------------------------------------------
void CHud::GameRulesEndGame(void)
{
	DBG_PRINTF("CHud::GameRulesEndGame()\n");
	if (!ASSERT(gViewPort->GetScoreBoard() != NULL))
		return;

	// IMPORTANT: __MsgFunc_GRInfo() should arrive BEFORE this!!!
	CLIENT_INDEX iWinner = gViewPort->GetScoreBoard()->m_iServerBestPlayer;// XDM3037
	if (iWinner > 0)// winner
	{
		cl_entity_t *pWinner = gEngfuncs.GetEntityByIndex(iWinner);
		if (pWinner && pWinner->player)
		{
			if (gViewPort->GetScoreBoard())
				gViewPort->GetScoreBoard()->m_iWinner = iWinner;// XDM3037

			char msgname[32];// + NULLterm
			cl_entity_t *localplayer = gEngfuncs.GetLocalPlayer();
			if (pWinner == localplayer || (IsTeamGame(m_iGameType) && localplayer->curstate.team != TEAM_NONE && g_PlayerExtraInfo[iWinner].teamnumber == localplayer->curstate.team))
			{
				_snprintf(msgname, 32, "MP_WIN_LP\0");
				client_textmessage_t *msg = TextMessageGet(msgname);
				if (msg)
				{
					conprintf(0, "* %s\n", msg->pMessage);
					if (g_pCvarTFX->value > 0.0f)
					{
						m_MessageAnnouncement = *msg;// copy localized message
						//memcpy(&m_MessageAnnouncement, msg, sizeof(client_textmessage_t));// copy localized message
						strncpy(m_szMessageAnnouncement, msg->pMessage, ANNOUNCEMENT_MSG_LENGTH);// store message TEXT
						m_szMessageAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
						m_MessageAnnouncement.pMessage = m_szMessageAnnouncement;
						m_Message.MessageAdd(&m_MessageAnnouncement);
					}
				}
				else
					conprintf(0, "* You are the winner!\n");

				//_snprintf(msgname, 32, "!MP_WINNER\0");
				PlaySoundAnnouncer("!MP_WINNER\0", 5);
			}
			else// winner is not me
			{
				if (IsTeamGame(m_iGameType))
					_snprintf(msgname, 32, "MP_WIN_TEAM\0");
				else
					_snprintf(msgname, 32, "MP_WIN_PLAYER\0");

				client_textmessage_t *msg = TextMessageGet(msgname);
				if (msg)
				{
					// insert player/team name
					_snprintf(m_szMessageAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, IsTeamGame(m_iGameType)?GetTeamName(g_PlayerExtraInfo[iWinner].teamnumber):g_PlayerInfoList[iWinner].name);
					m_szMessageAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';// XDM3038c
					conprintf(0, "* %s\n", m_szMessageAnnouncement);
					if (g_pCvarTFX->value > 0.0f)
					{
						m_MessageAnnouncement = *msg;// copy localized message
						//memcpy(&m_MessageAnnouncement, msg, sizeof(client_textmessage_t));// copy localized message
						//strncpy(m_szMessageAnnouncement, msg->pMessage, ANNOUNCEMENT_MSG_LENGTH);// store message TEXT
						//m_szMessageAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
						m_MessageAnnouncement.pMessage = m_szMessageAnnouncement;
						m_Message.MessageAdd(&m_MessageAnnouncement);
					}
				}
				else
					conprintf(0, "* %s is the winner!\n", IsTeamGame(m_iGameType)?GetTeamName(g_PlayerExtraInfo[iWinner].teamnumber):g_PlayerInfoList[iWinner].name);

				if (!IsSpectator() && m_iGameType != GT_COOP)//UTIL_IsSpectator(localplayer->index))// spectators can't lose :) In CoOp there are no losers
				{
					//_snprintf(msgname, 32, "!MP_LOST\0");
					PlaySoundAnnouncer("!MP_LOST\0", 5);

					// store this message in m_MessageAward/m_szMessageAward so it won't interfere
					//if (g_PlayerExtraInfo[iWinner].teamnumber != localplayer->curstate.team)// my team lost
					{
						//char msgname[32];// + NULLterm
						_snprintf(msgname, 32, "MP_LOST_LP\0");
						msg = TextMessageGet(msgname);
						if (msg)
						{
							conprintf(0, "* %s\n", msg->pMessage);
							if (g_pCvarTFX->value > 0.0f)
							{
								m_MessageAward = *msg;// copy localized message
								//memcpy(&m_MessageAward, msg, sizeof(client_textmessage_t));// copy localized message
								//ConsolePrint(msg->pMessage);
								strncpy(m_szMessageAward, msg->pMessage, DEATHNOTICE_MSG_LENGTH);// store message TEXT
								m_szMessageAward[DEATHNOTICE_MSG_LENGTH-1] = '\0';
								m_MessageAward.pMessage = m_szMessageAward;
								m_Message.MessageAdd(&m_MessageAward);
							}
						}
						else
							conprintf(0, "* You have lost the match!\n");
					}
				}
			}
		}
	}
}

#define ASTRLEN	32
//-----------------------------------------------------------------------------
// Purpose: GameRules Event handler
// Warning: For NON-CRITICAL stuff only! Game mechanics should not depend on this function!
// Warning: LOTS of memory corruption possibilities! Also, mind the compiler...
// Input  : gameevent - GAME_EVENT_UNKNOWN
//			client1 - 
//			client2 - 
//-----------------------------------------------------------------------------
void CHud::GameRulesEvent(int gameevent, short data1, short data2)
{
#if defined (_DEBUG_GAMERULES)
	DBG_PRINTF("CHud::GameRulesEvent(%d, %hd, %hd)\n", gameevent, data1, data2);
#endif
	if (g_pCvarTFX->value <= 0.0f)
		return;

	// IMPORTANT: no client index checks done on data1/2 because for now they represent ONLY PLAYER indexes, not monsters!
	char astr[ASTRLEN];// + NULLterm
	client_textmessage_t *msg = NULL;
	astr[0] = 0;

	if (gameevent == GAME_EVENT_START ||
		gameevent == GAME_EVENT_END ||
		gameevent == GAME_EVENT_ROUND_START ||
		gameevent == GAME_EVENT_ROUND_END)
	{
		// data1 - m_iRoundsCompleted, data2 - GetRoundsLimit()
		if (gameevent == GAME_EVENT_START)
		{
			msg = TextMessageGet("GAME_START");
			if (msg == NULL || msg->pMessage == NULL)
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_START %d %d\n", data1, data2);
		}
		else if (gameevent == GAME_EVENT_END)
		{
			msg = TextMessageGet("GAME_END");
			if (msg == NULL || msg->pMessage == NULL)
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_END %d %d\n", data1, data2);
		}
		else if (gameevent == GAME_EVENT_ROUND_START)
		{
			msg = TextMessageGet("ROUND_START");
			if (msg == NULL || msg->pMessage == NULL)
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_ROUND_START %d %d\n", data1, data2);

			data1++;// %d == round index
		}
		else if (gameevent == GAME_EVENT_ROUND_END)
		{
			msg = TextMessageGet("ROUND_END");
			if (msg == NULL || msg->pMessage == NULL)
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_ROUND_END %d %d\n", data1, data2);

			data1++;// %d == round index
		}
		if (msg && msg->pMessage)
		{
			m_MessageCombo = *msg;
			_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, data1);
			m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
			m_MessageCombo.pMessage = m_szMessageCombo;
			m_Message.MessageAdd(&m_MessageCombo);

			if (gameevent == GAME_EVENT_ROUND_END)// XDM3038a: print to score board title for certain cases
				gViewPort->GetScoreBoard()->SetTitle(m_szMessageCombo, 4.0f);
		}
		ConsolePrint(m_szMessageCombo);
	}
	else if (gameevent == GAME_EVENT_AWARD)
	{
		// data1 - player, data2 - award
		if (g_PlayerInfoList[data1].thisplayer)// should always be true
		{
			if (data2 > 1)
			{
				_snprintf(astr, ASTRLEN, "AWARD%d\0", data2);
				msg = TextMessageGet(astr);
				if (msg)
				{
					m_MessageAward = *msg;// copy localized message
					//memcpy(&m_MessageAward, msg, sizeof(client_textmessage_t));
					strncpy(m_szMessageAward, msg->pMessage, DEATHNOTICE_MSG_LENGTH);
					m_szMessageAward[DEATHNOTICE_MSG_LENGTH-1] = '\0';
					m_MessageAward.pMessage = m_szMessageAward;
					//m_MessageAward.holdtime = SCORE_AWARD_TIME;
					m_Message.MessageAdd(&m_MessageAward);
				}
				else
				{
					_snprintf(m_szMessageAward, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_AWARD %d %d\n", data1, data2);
					m_szMessageAward[DEATHNOTICE_MSG_LENGTH-1] = '\0';
				}
				ConsolePrint(m_szMessageAward);// log to console too

				if (g_pCvarAnnouncerEvents->value > 0)
				{
					_snprintf(astr, ASTRLEN, "announcer/score%d.wav\0", data2);
					PlaySoundAnnouncer(astr, 4);
				}
			}
		}
	}
	else if (gameevent == GAME_EVENT_COMBO)
	{
		// data1 - killer, data2 - victim
		if (g_PlayerInfoList[data1].thisplayer)
		{
			//CenterPrint("You did a combo!\n");
			//TODO	if (m_pCvarDNEcho->value > 0.0f)
			//TODO		ConsolePrint(szConsoleString);
			if (data2 > 1)// remove previous
			{
				_snprintf(astr, ASTRLEN, "COMBO_LOCAL%d\0", data2-1);
				m_StatusIcons.DisableIcon(astr);
			}
			_snprintf(astr, ASTRLEN, "COMBO_LOCAL%d\0", data2);
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
		}
		else
		{
			//CenterPrint("Someone did a combo!\n");
			_snprintf(astr, ASTRLEN, "COMBO_OTHER%d\0", data2);
		}

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageCombo = *msg;// copy localized message
				//memcpy(&m_MessageCombo, msg, sizeof(client_textmessage_t));
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name);// add player name
				//strcpy(m_szMessageCombo, msg->pMessage);
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
				m_MessageCombo.pMessage = m_szMessageCombo;
				m_Message.MessageAdd(&m_MessageCombo);
			}
			else
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_COMBO %d %d\n", data1, data2);

			ConsolePrint(m_szMessageCombo);// log to console too
		}
		if (g_PlayerInfoList[data1].thisplayer && g_pCvarAnnouncerEvents->value > 0.0f)
		{
			_snprintf(astr, ASTRLEN, "announcer/combo%d.wav\0", data2);
			PlaySoundAnnouncer(astr, 3);
		}
	}
	else if (gameevent == GAME_EVENT_COMBO_BREAKER)
	{
		// data1 - killer, data2 - victim
		if (data1 == data2)
		{
			//CenterPrint("%s completed combo with a braincrushing suicide!\n");
			_snprintf(astr, ASTRLEN, "COMBOBREAK_SELF\0");
			if (IsValidPlayerIndex(data1) && g_PlayerInfoList[data1].thisplayer)
			{
				if (m_pCvarEventIconTime->value > 0)
					m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
				if (g_pCvarAnnouncerEvents->value > 0)
					PlaySoundAnnouncer("announcer/combofail.wav\0", 2);// you fail
			}
		}
		else if (IsValidPlayerIndex(data1) && g_PlayerInfoList[data1].thisplayer)
		{
			//CenterPrint("COMBO REAKER!\n");
			_snprintf(astr, ASTRLEN, "COMBOBREAK_LOCAL\0");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("announcer/combo0.wav\0", 2);
		}
		else
		{
			//if (g_PlayerInfoList[data2].thisplayer)
			//{
			//	CenterPrint("Your combo was broken!\n");
			//	_snprintf(astr, ASTRLEN, "COMBO_THISPLAYER0\0");
			//}
			//else
			//{
				//CenterPrint("%s's precious combo was ended by %s!\n");
				_snprintf(astr, ASTRLEN, "COMBOBREAK_OTHER\0");
			//}
		}

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				char entstring1[MAX_PLAYER_NAME_LENGTH];
				if (m_iGameType == GT_COOP && !IsValidPlayerIndex(data1))
					_snprintf(entstring1, MAX_PLAYER_NAME_LENGTH, "monster %d", data1);
				else
					GetEntityPrintableName(data1, entstring1, MAX_PLAYER_NAME_LENGTH);

				m_MessageCombo = *msg;// copy localized message
				//memcpy(&m_MessageCombo, msg, sizeof(client_textmessage_t));
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data2].name, entstring1);// replace all '%s' with names// for all??
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
				//StripEndNewlineFromString(m_szMessageCombo);
				m_MessageCombo.pMessage = m_szMessageCombo;
				m_Message.MessageAdd(&m_MessageCombo);
			}
			else
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_COMBO_BREAKER %d %d\n", data1, data2);

			ConsolePrint(m_szMessageCombo);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_FIRST_SCORE)
	{
		if (!IsActivePlayer(data1))
		{
			return;
		}
		// data1 - player, data2 - player score
		if (g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "FIRSTSCORE_LOCAL\0");// CenterPrint("You made the first score!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("announcer/firstscore.wav\0", 2);
		}
		else
			_snprintf(astr, ASTRLEN, "FIRSTSCORE_OTHER\0");// CenterPrint("%s made the first score!\n");

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageCombo = *msg;// copy localized message
				//memcpy(&m_MessageCombo, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name);// replace all '%s' with names
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
				m_MessageCombo.pMessage = m_szMessageCombo;
				m_Message.MessageAdd(&m_MessageCombo);
			}
			else
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_FIRST_SCORE %d %d\n", data1, data2);

			ConsolePrint(m_szMessageCombo);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_TAKES_LEAD)
	{
		// data1 - player (NOT in teamplay), data2 - team (only in teamplay)
		if (IsActivePlayer(data1))
		{
			if (g_PlayerInfoList[data1].thisplayer)
			{
				_snprintf(astr, ASTRLEN, "TAKENLEAD_LOCAL\0");// CenterPrint("Taken the lead!\n");
				if (m_pCvarEventIconTime->value > 0)
					m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
				if (g_pCvarAnnouncerEvents->value > 0)
					PlaySoundAnnouncer("announcer/takenlead.wav\0", 2);
			}
			else
				_snprintf(astr, ASTRLEN, "TAKENLEAD_OTHER\0");// CenterPrint("%s takes the lead!\n");
		}
		else if (IsTeamGame(m_iGameType) && IsActiveTeam(data2))
			_snprintf(astr, ASTRLEN, "TAKENLEAD_TEAM\0");// CenterPrint("%s takes the lead!\n");
		else
			return;// !!!

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageAnnouncement = *msg;// copy localized message
				//memcpy(&m_MessageAnnouncement, msg, sizeof(client_textmessage_t));// store in separate place?
				if (IsTeamGame(m_iGameType))
					_snprintf(m_szMessageAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, g_TeamInfo[data2].name);// replace '%s' with team name
				else
					_snprintf(m_szMessageAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name);// replace '%s' with name

				m_szMessageAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
				m_MessageAnnouncement.pMessage = m_szMessageAnnouncement;
				m_Message.MessageAdd(&m_MessageAnnouncement);
			}
			else
				_snprintf(m_szMessageAnnouncement, ANNOUNCEMENT_MSG_LENGTH, "GAME_EVENT_TAKES_LEAD %d %d\n", data1, data2);

			ConsolePrint(m_szMessageAnnouncement);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_HEADSHOT)
	{
		// data1 - killer, data2 - victim
		if (g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "HEADSHOT_LOCAL\0");// CenterPrint("BOOM, HEADSHOT!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("announcer/headshot.wav\0", 2);
		}
		else if (g_PlayerInfoList[data2].thisplayer)
		{
			astr[0] = 0;
			//_snprintf(astr, ASTRLEN, "HEADSHOT_THISPLAYER\0");// CenterPrint("%s did you a HEADSHOT!\n");
		}
		else
			_snprintf(astr, ASTRLEN, "HEADSHOT_OTHER\0");// CenterPrint("%s -> HEADSHOT! -> %s!\n");

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageExtraAnnouncement = *msg;// copy localized message
				//memcpy(&m_MessageExtraAnnouncement, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name, GetEntityPrintableName(data2));// replace all '%s' with names
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';// XDM3038c
				strcat(m_szMessageExtraAnnouncement, "\n");// XDM3038c
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
				m_MessageExtraAnnouncement.pMessage = m_szMessageExtraAnnouncement;
				m_Message.MessageAdd(&m_MessageExtraAnnouncement);
			}
			else
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, "GAME_EVENT_HEADSHOT %d %d\n", data1, data2);

			ConsolePrint(m_szMessageExtraAnnouncement);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_REVENGE)
	{
		// data1 - killer, data2 - victim
		if (g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "REVENGE_LOCAL\0");// CenterPrint("Revenge!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("announcer/revenge.wav\0", 2);
		}
		else if (g_PlayerInfoList[data2].thisplayer)
		{
			astr[0] = 0;
			//_snprintf(astr, ASTRLEN, "REVENGE_THISPLAYER\0");// CenterPrint("%s took revenge on you!\n");
		}
		else
			_snprintf(astr, ASTRLEN, "REVENGE_OTHER\0");// CenterPrint("%s tasted sweet revenge on %s!\n");

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageCombo = *msg;// copy localized message
				//memcpy(&m_MessageCombo, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name, GetEntityPrintableName(data2));// replace all '%s' with names
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';// XDM3038c
				strcat(m_szMessageCombo, "\n");// XDM3038c
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
				m_MessageCombo.pMessage = m_szMessageCombo;
				m_Message.MessageAdd(&m_MessageCombo);
			}
			else
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_REVENGE %d %d\n", data1, data2);

			ConsolePrint(m_szMessageCombo);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_REVENGE_RESET)
	{
		// data1 - localhost, data2 - killer
		if (g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "REVENGE_RESET\0");// CenterPrint("Revenge!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("announcer/revenge.wav\0", 2);
		}
		/*else if (g_PlayerInfoList[data2].thisplayer)// should never get here!!
		{
			astr[0] = 0;
			//_snprintf(astr, ASTRLEN, "REVENGE_RESET_THISPLAYER\0");// CenterPrint("%s you failed so nobody needs to kill you!\n");
		}*/
		else// should never get here!!
		{
			astr[0] = 0;
			//_snprintf(astr, ASTRLEN, "REVENGE_RESET_OTHER\0");// CenterPrint("%s was freed from %s!\n");
		}

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageExtraAnnouncement = *msg;// copy localized message
				//memcpy(&m_MessageExtraAnnouncement, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, GetEntityPrintableName(data2));// replace all '%s' with names
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
				strcat(m_szMessageExtraAnnouncement, "\n");// XDM3038c
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
				m_MessageExtraAnnouncement.pMessage = m_szMessageExtraAnnouncement;
				m_Message.MessageAdd(&m_MessageExtraAnnouncement);
			}
			else
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, "GAME_EVENT_REVENGE_RESET %d %d\n", data1, data2);

			ConsolePrint(m_szMessageExtraAnnouncement);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_LOSECOMBO)
	{
		// data1 - killer, data2 - victim
		if (IsValidPlayerIndex(data2) && g_PlayerInfoList[data2].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "LOSECOMBO_LOCAL\0");// CenterPrint("FFFUUUUUUUUU-\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerLCombo->value > 0)
				PlaySoundAnnouncer("announcer/losecombo.wav\0", 4);
		}
		else if (IsValidPlayerIndex(data1) && g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "LOSECOMBO_THISPLAYER\0");// CenterPrint("You doublekilled %s!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerLCombo->value > 0)
				PlaySoundAnnouncer("announcer/uncombo.wav\0", 4);
		}
		else if (IsValidPlayerIndex(data1))// don't allow "NULL doublekilled %s!"
		{
			if (data1 == data2)
				_snprintf(astr, ASTRLEN, "LOSECOMBO_SELF\0");// CenterPrint("%s committed another suicide!\n");
			else
				_snprintf(astr, ASTRLEN, "LOSECOMBO_OTHER\0");// CenterPrint("%s doublekilled %s!\n");
		}

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				char entstring1[MAX_PLAYER_NAME_LENGTH];
				if (m_iGameType == GT_COOP && !IsValidPlayerIndex(data1))
					_snprintf(entstring1, MAX_PLAYER_NAME_LENGTH, "monster %d", data1);
				else
					GetEntityPrintableName(data1, entstring1, MAX_PLAYER_NAME_LENGTH);

				char entstring2[MAX_PLAYER_NAME_LENGTH];
				if (m_iGameType == GT_COOP && !IsValidPlayerIndex(data2))
					_snprintf(entstring2, MAX_PLAYER_NAME_LENGTH, "monster %d", data2);
				else
					GetEntityPrintableName(data2, entstring2, MAX_PLAYER_NAME_LENGTH);

				m_MessageCombo = *msg;// copy localized message
				//memcpy(&m_MessageCombo, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, entstring1, entstring2);// replace all '%s' with names
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';// XDM3038c
				strcat(m_szMessageCombo, "\n");// XDM3038c
				m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
				m_MessageCombo.pMessage = m_szMessageCombo;
				m_Message.MessageAdd(&m_MessageCombo);
			}
			else
				_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_LOSECOMBO %d %d\n", data1, data2);

			ConsolePrint(m_szMessageCombo);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_PLAYER_READY)
	{
		// data1 - player, data2 - m_afButtonPressed
		if (IsValidPlayerIndex(data1))
		{
			g_PlayerExtraInfo[data1].ready = data2;
			//if (gViewPort && gViewPort->IsMenuOpened(gViewPort->GetScoreBoard()))
			// TESTME	gViewPort->GetScoreBoard()->FillGrid();// show changes now

			if (data2 > 0)
			{
				char szText[MAX_CHARS_PER_LINE];
				//LocaliseTextString("#CL_READY", szText, MAX_CHARS_PER_LINE);
				_snprintf(szText, MAX_CHARS_PER_LINE, BufferedLocaliseTextString("#CL_READY\n"), g_PlayerInfoList[data1].name);
				szText[MAX_CHARS_PER_LINE-1] = '\0';
				//if (!g_PlayerInfoList[data1].thisplayer)// don't flood the chat
					m_SayText.SayTextPrint(szText, 0, 0);

				strcat(szText, "\n");// XDM3038c
				ConsolePrint(szText);
			}
		}
	}
	else if (gameevent == GAME_EVENT_PLAYER_OUT)
	{
		// data1 - player, data2 - player score
		if (IsValidPlayerIndex(data1))
		{
			g_PlayerExtraInfo[data1].score = data2;
			g_PlayerExtraInfo[data1].observer = 1;
			g_PlayerExtraInfo[data1].finished = 1;
			g_PlayerExtraInfo[data1].ready = 0;
			//	if (gViewPort && gViewPort->IsMenuOpened(gViewPort->GetScoreBoard()))
			// TESTME		gViewPort->GetScoreBoard()->FillGrid();// show changes now

			_snprintf(astr, ASTRLEN, "CL_OUT\0");

			if (astr[0])
			{
				char entstring1[MAX_PLAYER_NAME_LENGTH];
				if (m_iGameType == GT_COOP && !IsValidPlayerIndex(data1))
					_snprintf(entstring1, MAX_PLAYER_NAME_LENGTH, "monster %d", data1);
				else
					GetEntityPrintableName(data1 , entstring1, MAX_PLAYER_NAME_LENGTH);

				msg = TextMessageGet(astr);
				if (msg && msg->pMessage)
				{
					_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, entstring1);// replace all '%s' with names// for all??

					if (g_PlayerInfoList[data1].thisplayer == 0)// don't print large text about self
					{
						m_MessageCombo = *msg;// copy localized message
						m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
						//StripEndNewlineFromString(m_szMessageCombo);
						m_MessageCombo.pMessage = m_szMessageCombo;
						m_Message.MessageAdd(&m_MessageCombo);
					}
					m_SayText.SayTextPrint(m_szMessageCombo, 0, 0);// print into chat
				}
				else
				{
					_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "%s is out\n", entstring1);
					m_SayText.SayTextPrint(m_szMessageCombo, 0, 0);
					_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_PLAYER_OUT %d %d\n", data1, data2);
				}
				ConsolePrint(m_szMessageCombo);// log to console too
			}
			gViewPort->UpdateOnPlayerInfo();// XDM3038a
		}
	}
	else if (gameevent == GAME_EVENT_PLAYER_FINISH)
	{
		// data1 - player, data2 - player team
		if (IsValidPlayerIndex(data1))
		{
			if (g_PlayerInfoList[data1].thisplayer)
			{
				if (m_iGameType == GT_COOP)
				{
					_snprintf(astr, ASTRLEN, "COOP_PL_FINISH_LOCAL\0");
					if (m_iGameMode == COOP_MODE_LEVEL)
						CenterPrint(BufferedLocaliseTextString("#COOP_TRIGGER_TOUCH"));
				}
				else
					_snprintf(astr, ASTRLEN, "MP_PL_FINISH_LOCAL\0");
			}
			else
			{
				if (m_iGameType == GT_COOP)
					_snprintf(astr, ASTRLEN, "COOP_PL_FINISH_OTHER\0");
				else
					_snprintf(astr, ASTRLEN, "MP_PL_FINISH_OTHER\0");
			}
			g_PlayerExtraInfo[data1].finished = 1;
			//?	g_PlayerExtraInfo[data1].ready = 0;
			//	if (gViewPort && gViewPort->IsMenuOpened(gViewPort->GetScoreBoard()))
			// TESTME	gViewPort->GetScoreBoard()->FillGrid();// show changes now

			if (astr[0])
			{
				msg = TextMessageGet(astr);
				if (msg && msg->pMessage)
				{
					m_MessageCombo = *msg;// copy localized message
					//memcpy(&m_MessageCombo, msg, sizeof(client_textmessage_t));// store in separate place?
					_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name);// replace all '%s' with names
					m_szMessageCombo[DEATHNOTICE_MSG_LENGTH-1] = '\0';
					m_MessageCombo.pMessage = m_szMessageCombo;
					m_Message.MessageAdd(&m_MessageCombo);
				}
				else
					_snprintf(m_szMessageCombo, DEATHNOTICE_MSG_LENGTH, "GAME_EVENT_PLAYER_FINISH %d %d\n", data1, data2);

				ConsolePrint(m_szMessageCombo);// log to console too
			}
		}
	}
	else if (gameevent == GAME_EVENT_OVERTIME)
	{
		// data1 - new time limit, data2 - 
		_snprintf(astr, ASTRLEN, "MP_OVERTIME\0");
		msg = TextMessageGet(astr);
		if (msg && msg->pMessage)
		{
			m_MessageTimeLeft = *msg;// copy localized message
			//memcpy(&m_MessageTimeLeft, msg, sizeof(client_textmessage_t));// copy localized message?
			strncpy(m_szMessageTimeLeft, msg->pMessage, ANNOUNCEMENT_MSG_LENGTH);// store message TEXT
			m_szMessageTimeLeft[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
			m_MessageTimeLeft.pMessage = m_szMessageTimeLeft;
			m_Message.MessageAdd(&m_MessageTimeLeft);
		}
		//else
			conprintf(0, "- OVERTIME!\n");

		PlaySoundAnnouncer(astr, 3.0f);
	}
	else if (gameevent == GAME_EVENT_DISTANTSHOT)
	{
		// data1 - killer, data2 - victim
		if (g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "DISTANTSHOT_LOCAL\0");// CenterPrint("BOOM, DISTANTSHOT!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("announcer/distantshot.wav\0", 2);
		}
		else if (g_PlayerInfoList[data2].thisplayer)
		{
			astr[0] = 0;
			//_snprintf(astr, ASTRLEN, "DISTANTSHOT_THISPLAYER\0");// CenterPrint("%s did you a DISTANTSHOT!\n");
		}
		else
			_snprintf(astr, ASTRLEN, "DISTANTSHOT_OTHER\0");// CenterPrint("%s -> DISTANTSHOT! -> %s!\n");

		if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageExtraAnnouncement = *msg;// copy localized message
				//memcpy(&m_MessageExtraAnnouncement, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name, GetEntityPrintableName(data2));// replace all '%s' with names
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';// XDM3038c
				strcat(m_szMessageExtraAnnouncement, "\n");// XDM3038c
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
				m_MessageExtraAnnouncement.pMessage = m_szMessageExtraAnnouncement;
				m_Message.MessageAdd(&m_MessageExtraAnnouncement);
			}
			else
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, "GAME_EVENT_DISTANTSHOT %d %d\n", data1, data2);

			ConsolePrint(m_szMessageExtraAnnouncement);// log to console too
		}
	}
	else if (gameevent == GAME_EVENT_SECRET)// XDM3038c: printable messages are sent separately because there may be text coming
	{
		// data1 - player, data2 - 0
		if (g_PlayerInfoList[data1].thisplayer)
		{
			_snprintf(astr, ASTRLEN, "SECRET_LOCAL\0");// CenterPrint("You have found a secret!\n");
			if (m_pCvarEventIconTime->value > 0)
				m_StatusIcons.EnableIcon(astr, 255,255,255, (short)m_pCvarEventIconTime->value, EVENTICON_FADE_TIME);
			if (g_pCvarAnnouncerEvents->value > 0)
				PlaySoundAnnouncer("game/secret.wav\0", 2);
		}
		//else
		//	_snprintf(astr, ASTRLEN, "SECRET_OTHER\0");// CenterPrint("%s found a secret!\n");

		/*if (astr[0])
		{
			msg = TextMessageGet(astr);
			if (msg && msg->pMessage)
			{
				m_MessageExtraAnnouncement = *msg;// copy localized message
				//memcpy(&m_MessageExtraAnnouncement, msg, sizeof(client_textmessage_t));// store in separate place?
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, msg->pMessage, g_PlayerInfoList[data1].name, secret_message);// replace all '%s' with names
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';// XDM3038c
				strcat(m_szMessageExtraAnnouncement, "\n");// XDM3038c
				m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH-1] = '\0';
				m_MessageExtraAnnouncement.pMessage = m_szMessageExtraAnnouncement;
				m_Message.MessageAdd(&m_MessageExtraAnnouncement);
			}
			else
				_snprintf(m_szMessageExtraAnnouncement, ANNOUNCEMENT_MSG_LENGTH, "GAME_EVENT_DISTANTSHOT %d %d\n", data1, data2);

			ConsolePrint(m_szMessageExtraAnnouncement);// log to console too
		}*/
	}
	else
	{
		conprintf(1, "CL: Unknown game event (%d %d %d)\n", gameevent, data1, data2);
	}

	/*if (strstr(m_szMessageCombo, "was ended by"))
	{
		DBG_FORCEBREAK
	}*/
}
