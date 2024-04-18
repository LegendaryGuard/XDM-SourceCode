//====================================================================
//
// Purpose: player personal statistics
//
//====================================================================

#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

// Two methods: struct or
// or CBasePlayer::unsigned long m_Stats[] and enum of elements:
// SCORE, ACHIEVEMENT_SCORE, ETC.

/*
typedef struct playerstats_s
{
	unsigned long score;
	unsigned long achievement_score;// awards for capturing flag, taking control point, etc.
	unsigned long death_count;
	unsigned short suicide_count;
	unsigned short last_score_award;
	unsigned short best_score_award;
	unsigned short current_combo;
	unsigned short best_combo;
	unsigned short combo_breaker_count;
	unsigned short revenge_count;
	unsigned short fail_count;
	unsigned short teamkill_count;
	unsigned short control_point_count;// dom
	unsigned short check_point_count;// coop
	unsigned short capture_count;// ctf
	unsigned short spectator_enable_count;
} playerstats_t;
*/

typedef enum statindex_e
{
	STAT_SCORE = 0,
	STAT_ACHIEVEMENT_SCORE,
	STAT_SPAWN_COUNT,
	STAT_DEATH_COUNT,
	STAT_SUICIDE_COUNT,
	STAT_FIRST_SCORE_AWARD,// 
	STAT_CURRENT_SCORE_AWARD,
	STAT_BEST_SCORE_AWARD,
	STAT_CURRENT_COMBO,
	STAT_BEST_COMBO,
	STAT_COMBO_BREAKER_COUNT,
	STAT_REVENGE_COUNT,
	STAT_FAIL_COUNT,
	STAT_TROLL_COUNT,
	STAT_PLAYERKILL_COUNT,
	STAT_MONSTERKILL_COUNT,
	STAT_TEAMKILL_COUNT,
	STAT_HEADSHOT_COUNT,
	STAT_CONTROL_POINT_COUNT,// DOM
	STAT_CHECK_POINT_COUNT,// COOP
	STAT_FLAG_CAPTURE_COUNT,// CTF
	STAT_FLAG_RETURN_COUNT,// CTF
	STAT_SPECTATOR_ENABLE_COUNT,
	STAT_DISTANTSHOT_COUNT,
	STAT_DECALS_COUNT,
	STAT_GIB_COUNT,
	STAT_SECRETS_COUNT,
	STAT_MAPS_COUNT,
	STAT_ENDLEVEL_COUNT,
	STAT_NUMELEMENTS// LAST!
} statindex_t;


#endif // PLAYERSTATS_H
