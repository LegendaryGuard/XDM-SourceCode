#ifndef BOT_CVAR_H
#define BOT_CVAR_H

extern cvar_t g_bot_allow_commands;
extern cvar_t g_bot_reaction;
extern cvar_t g_bot_skill;
extern cvar_t g_bot_strafe_percent;
extern cvar_t g_bot_chat_enable;
extern cvar_t g_bot_chat_percent;
/*extern cvar_t g_bot_chat_tag_percent;// XDM3038: useless
extern cvar_t g_bot_chat_drop_percent;
extern cvar_t g_bot_chat_lower_percent;
extern cvar_t g_bot_chat_swap_percent;*/
extern cvar_t g_bot_taunt_percent;
extern cvar_t g_bot_whine_percent;
extern cvar_t g_bot_grenade_time;
extern cvar_t g_bot_logo_percent;
extern cvar_t g_bot_logo_custom;
extern cvar_t g_bot_dont_shoot;
extern cvar_t g_bot_check_lightlevel;
extern cvar_t g_bot_use_flashlight;
extern cvar_t g_bot_use_entities;
extern cvar_t g_bot_follow_actions;
extern cvar_t g_bot_follow_distance;// XDM3038
extern cvar_t g_bot_default_model;
extern cvar_t g_bot_minbots;
extern cvar_t g_bot_maxbots;
extern cvar_t g_bot_debug;// XDM3038b
extern cvar_t g_botmatch;
#if defined (_DEBUG)
extern cvar_t g_bot_superdebug;
extern cvar_t g_bot_test1;
#endif
#ifdef DEBUG_USERMSG
extern cvar_t g_dbg_disable_usermsg;
#endif
extern cvar_t *g_pCvarDeveloper;
extern cvar_t *g_pmp_footsteps;
extern cvar_t *g_pmp_friendlyfire;
extern cvar_t *g_pmp_noshooting;
extern cvar_t *g_pmp_revengemode;
extern cvar_t *g_pmp_teammines;
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_maxspeed;

#endif // BOT_CVAR_H
