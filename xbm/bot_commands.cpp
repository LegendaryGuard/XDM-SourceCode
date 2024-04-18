#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "bot.h"
#include "bot_func.h"
#include "bot_client.h"
#include "bot_cvar.h"
#include "bot_weapons.h"

bool BotUseCommand(bot_t *pBot, edict_t *pPlayer, short command)
{
	if (g_bot_allow_commands.value <= 0)
		return false;
	if (pBot == NULL || pPlayer == NULL)
		return false;
	if (UTIL_GetBotPointer(pPlayer))// XDM3038c: disallow use by bots
		return false;

	DBG_BOT_PRINT("BotUseCommand(%s, %d, %hd)\n", pBot->name, ENTINDEX(pPlayer), command);
	if (command >= BOT_USE_LASTTYPE)
		command = BOT_USE_NONE;

	bool result = false;
	char msg[64];// should be enough
	if (pBot)
	{
		// check if user is a teammate...
		if (!IsTeamplay() || pBot->pEdict->v.team >= 0 && pBot->pEdict->v.team == pPlayer->v.team)
		{
			if (pBot->pBotUser == NULL || pBot->pBotUser == pPlayer || UTIL_GetBotIndex(pBot->pBotUser) >= 0)// XDM3037: allow if used by another bot
			{
				result = true;
				switch(command)
				{
				default:
					result = false;
					break;
				case BOT_USE_NONE:
					pBot->use_type = BOT_USE_NONE;
					pBot->pBotUser = NULL;
					//_snprintf(msg, 64, "#BOT_CMD_USEFREE\n");//"%s is now free\n", pBot->name);
					break;
				case BOT_USE_FOLLOW:
					pBot->use_type = BOT_USE_FOLLOW;
					pBot->pBotUser = pPlayer;
					if (pBot->pBotEnemy == pPlayer)
						pBot->pBotEnemy = NULL;

					//_snprintf(msg, 64, "#BOT_CMD_USEFOLLOW\n");//"%s will follow you\n", pBot->name);
					break;
				case BOT_USE_HOLD:
					pBot->use_type = BOT_USE_HOLD;
					pBot->pBotUser = pPlayer;
					pBot->posHold.flags = pBot->pEdict->v.flags;
					pBot->posHold.origin = pBot->pEdict->v.origin;// XDM3038a: don't copy pPlayer position, remember own
					pBot->posHold.angles = pBot->pEdict->v.angles;
					if (pBot->pBotEnemy == pPlayer)
						pBot->pBotEnemy = NULL;

					//_snprintf(msg, 64, "#BOT_CMD_USEHOLD\n");//"%s will hold position\n", pBot->name);
					break;
				}
				if (result)
					_snprintf(msg, 64, "#BOT_CMD_USE%d\n", command);
			}
			else
				_snprintf(msg, 64, "#BOT_CMD_BUSY\n");//"%s is used by another player\n", pBot->name);
		}
		else
			_snprintf(msg, 64, "#BOT_CMD_NOTAFRIEND\n");//"%s is not a teammate\n");

		ClientPrint(&pPlayer->v, HUD_PRINTTALK, msg, pBot->name);
	}
	return result;
}
