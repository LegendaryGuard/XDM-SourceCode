#ifndef PLAYER_INFO_H
#define PLAYER_INFO_H
#ifdef _WIN32
#pragma once
#endif

#include "gamedefs.h"
#include "color.h"
#include "../engine/cdll_int.h"// hud_player_info_t

typedef struct extra_player_info_s
{
	int score;
	int deaths;
	float lastscoretime;
	TEAM_ID teamnumber;
	short observer;// XDM3035c: distinct name
	short finished;// XDM3037: coop special mark
	short ready;// player have pressed ready button (intermission)
} extra_player_info_t;

typedef struct team_info_s 
{
	int score;
	int deaths;
	int scores_overriden;
	Color color;// XDM3038
	short colormap;// XDM3035
	short ping;
	short packetloss;
	short players;
//	TEAM_ID id;// XDM3035
	char name[MAX_TEAMNAME_LENGTH];
} team_info_t;

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];	// player info from the engine
extern extra_player_info_t	g_PlayerExtraInfo[MAX_PLAYERS+1];	// additional player info sent directly to the client dll
extern int					g_IsSpectator[MAX_PLAYERS+1];
extern team_info_t			g_TeamInfo[MAX_TEAMS+1];
extern long					g_PlayerStats[MAX_PLAYERS+1][STAT_NUMELEMENTS];// XDM3037
extern short				g_iNumberOfTeams;// XDM3037

#endif // PLAYER_INFO_H
