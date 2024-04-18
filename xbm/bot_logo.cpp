#include "extdll.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "util.h"
#include "bot.h"
#include "bot_cvar.h"
#include "bot_func.h"


int num_logos = 0;
char bot_logos[MAX_BOT_LOGOS][16];


void BotLogoInit(void)
{
	FILE *bot_logo_fp;
	char bot_logo_filename[256];
	char logo_buffer[80];
	int length, index;

#ifndef __linux__
	HANDLE h_logo;
	char dir_name[32];
	char decal_filename[256];
	DWORD dwDummy;

	struct stat stat_str;
	wadinfo_t wad_header;
	lumpinfo_t lump_info;
	GET_GAME_DIR(dir_name);
	_snprintf(decal_filename, 256, "%s/decals.wad", dir_name);

	if (stat(decal_filename, &stat_str) != 0)
	{
		strcpy(decal_filename, "valve/decals.wad");
//		UTIL_Pathname_Convert(decal_filename);
		if (stat(decal_filename, &stat_str) != 0)
			return;  // file not found
	}

	h_logo = CreateFile(decal_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (h_logo == INVALID_HANDLE_VALUE)
		return;  // can't open file

	if (!ReadFile(h_logo, &wad_header, sizeof(wadinfo_t), &dwDummy, NULL))
	{
		CloseHandle(h_logo);  // can't read wad header
		return;
	}

	if (SetFilePointer(h_logo, wad_header.infotableofs, NULL, FILE_BEGIN) == 0)
	{
		CloseHandle(h_logo);
		return;  // can't seek to lump info table
	}

	for (index=0; index < wad_header.numlumps; ++index)
	{
		if (!ReadFile(h_logo, &lump_info, sizeof(lumpinfo_t), &dwDummy, NULL))
		{
			CloseHandle(h_logo);  // can't read lump info
			return;
		}

		if (strncmp(lump_info.name, "{__", 3) == 0)
		{
			strcpy(bot_logos[num_logos], lump_info.name);
			++num_logos;
		}

		if (strncmp(lump_info.name, "__", 2) == 0)
		{
			strcpy(bot_logos[num_logos], lump_info.name);
			++num_logos;
		}
	}

	CloseHandle(h_logo);

#endif
	const char *logocfgfile;// XDM
	if (mod_id == GAME_XDM_DLL)// XDM
		logocfgfile = "botlogo.lst";
	else
		logocfgfile = "HPB_bot_logo.cfg";

	UTIL_BuildFileName(bot_logo_filename, logocfgfile, NULL);
	bot_logo_fp = fopen(bot_logo_filename, "r");
	if (bot_logo_fp != NULL)
	{
		while ((num_logos < MAX_BOT_LOGOS) && (fgets(logo_buffer, 80, bot_logo_fp) != NULL))
		{
			length = strlen(logo_buffer);

			if (logo_buffer[length-1] == '\n')
			{
				logo_buffer[length-1] = 0;  // remove '\n'
				--length;
			}

			if (logo_buffer[0] != 0)
			{
				strncpy(bot_logos[num_logos], logo_buffer, 16);
				++num_logos;
			}
		}
		fclose(bot_logo_fp);
	}
}

void BotPickLogo(bot_t *pBot)
{
	if (num_logos == 0)
		return;

	pBot->logo_name[0] = 0;
	int logo_index = RANDOM_LONG(1, num_logos) - 1;  // zero based
	// check make sure this name isn't used
	bool used = true;
	int check_count = 0;
	int index;
	while ((used) && (check_count < MAX_BOT_LOGOS))
	{
		used = false;
		for (index=0; index < MAX_PLAYERS; ++index)
		{
			if (bots[index].is_used)
			{
				if (strcmp(bots[index].logo_name, bot_logos[logo_index]) == 0)
					used = true;
			}
		}
		if (used)
			++logo_index;

		if (logo_index == MAX_BOT_LOGOS)
			logo_index = 0;

		++check_count;
	}
	strcpy(pBot->logo_name, bot_logos[logo_index]);
}

void BotTrySprayLogo(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;
	if (pBot->b_spray_logo && ((pBot->f_spray_logo_time + 3.0) < gpGlobals->time))// took too long trying to spray logo?...
	{
		pBot->b_spray_logo = false;
		pEdict->v.idealpitch = 0.0f;
	}
	if (pBot->b_spray_logo)  // trying to spray a logo?
	{
		Vector v_src, v_dest, angle;
		TraceResult tr;
		// find the nearest wall to spray logo on (or floor)...
		angle = pEdict->v.v_angle;
		angle.x = 0;  // pitch is level horizontally
		UTIL_MakeVectors( angle );
		v_src = pEdict->v.origin + pEdict->v.view_ofs;
		v_dest = v_src + gpGlobals->v_forward * 100.0f;
		UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
		if (tr.flFraction < 1.0)
		{
			// already facing the correct yaw, just set pitch...
			pEdict->v.idealpitch = RANDOM_FLOAT(0.0f, 30.0f) - 15.0f;
		}
		else
		{
			v_dest = v_src + gpGlobals->v_right * 100.0f;  // to the right
			UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
			if (tr.flFraction < 1.0)
			{
				// set the ideal yaw and pitch...
				//Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);
				//pEdict->v.ideal_yaw = bot_angles.y;
				pEdict->v.ideal_yaw = VecToYaw(v_dest - v_src);
				BotFixIdealYaw(pEdict);
				pEdict->v.idealpitch = RANDOM_FLOAT(0.0, 30.0) - 15.0f;
			}
			else
			{
				v_dest = v_src + gpGlobals->v_right * -100;  // to the left
				UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
				if (tr.flFraction < 1.0)
				{
					// set the ideal yaw and pitch...
					//Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);
					//pEdict->v.ideal_yaw = bot_angles.y;
					pEdict->v.ideal_yaw = VecToYaw(v_dest - v_src);
					BotFixIdealYaw(pEdict);
					pEdict->v.idealpitch = RANDOM_FLOAT(0.0, 30.0) - 15.0f;
				}
				else
				{
					v_dest = v_src + gpGlobals->v_forward * -100;  // behind
					UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
					if (tr.flFraction < 1.0)
					{
						// set the ideal yaw and pitch...
						//Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);
						//pEdict->v.ideal_yaw = bot_angles.y;
						pEdict->v.ideal_yaw = VecToYaw(v_dest - v_src);
						BotFixIdealYaw(pEdict);
						pEdict->v.idealpitch = RANDOM_FLOAT(0.0, 30.0) - 15.0f;
					}
					else
					{
						// on the ground...
						angle = pEdict->v.v_angle;
						angle.x = 85.0f;  // 85 degrees is downward
						UTIL_MakeVectors( angle );
						v_src = pEdict->v.origin + pEdict->v.view_ofs;
						v_dest = v_src + gpGlobals->v_forward * 80;
						UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

						if (tr.flFraction < 1.0)
						{
							// set the pitch...
							pEdict->v.idealpitch = 85.0f;
							BotFixIdealPitch(pEdict);
						}
					}
				}
			}
		}
		pBot->f_dont_avoid_wall_time = gpGlobals->time + 5.0f;
		// is there a wall close to us?
		UTIL_MakeVectors( pEdict->v.v_angle );
		v_src = pEdict->v.origin + pEdict->v.view_ofs;
		v_dest = v_src + gpGlobals->v_forward * 80;
		UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
		if (tr.flFraction < 1.0)
		{
			BotSprayLogo(pEdict, pBot->logo_name);
			pBot->b_spray_logo = false;
			pEdict->v.idealpitch = 0.0f;
		}
	}
}

void BotSprayLogo(edict_t *pEntity, const char *logo_name)
{
	int index = DECAL_INDEX(logo_name);
	if (index < 0)
	  return;

	TraceResult pTrace;
	Vector v_src, v_dest;
	UTIL_MakeVectors(pEntity->v.v_angle);
	v_src = pEntity->v.origin + pEntity->v.view_ofs;
	v_dest = v_src + gpGlobals->v_forward * 80;
	UTIL_TraceLine(v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &pTrace);

	if (pTrace.pHit && (pTrace.flFraction < 1.0))
	{
		if (pTrace.pHit->v.solid != SOLID_BSP)
			return;

		if (g_bot_logo_custom.value > 0)
		{
			// XDM3035: I dont' really understand why we can't use this because it works
			MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
				WRITE_BYTE(TE_PLAYERDECAL);
				WRITE_BYTE(ENTINDEX(pEntity));
				WRITE_COORD(pTrace.vecEndPos.x);
				WRITE_COORD(pTrace.vecEndPos.y);
				WRITE_COORD(pTrace.vecEndPos.z);
				WRITE_SHORT((short)ENTINDEX(pTrace.pHit));
				WRITE_BYTE(index);
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			if (index > 255)
			{
				WRITE_BYTE(TE_WORLDDECALHIGH);
				index -= 256;
			}
			else
			{
				WRITE_BYTE(TE_WORLDDECAL);
			}
				WRITE_COORD(pTrace.vecEndPos.x);
				WRITE_COORD(pTrace.vecEndPos.y);
				WRITE_COORD(pTrace.vecEndPos.z);
				WRITE_BYTE(index);
			MESSAGE_END();
		}
		EMIT_SOUND_DYN2(pEntity, CHAN_STATIC, "player/sprayer.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);// XDM: was CHAN_VOICE
	}
}
