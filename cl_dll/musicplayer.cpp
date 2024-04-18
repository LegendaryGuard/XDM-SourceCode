//=========== Copyright © 1999-2017, xawari, All rights reserved. =============
//
// Client-side dynamically loaded background music player module
// NOTE: FMOD version is old, but it works just fine. If you want different backend, do it.
//
//=============================================================================
#include "hud.h"
#include "cl_util.h"
#include "cdll_int.h"
#include "musicplayer.h"
#include "vgui_Viewport.h"
#include "vgui_CustomObjects.h"
#include "vgui_MusicPlayer.h"
#include "parsemsg.h"
#include "../external/fmod/fmoddyn.h"
#include "../external/fmod/fmod_errors.h"

#if defined(_WINDOWS)
#if defined(_WIN64)
#define MUSICPLAYER_LIBNAME	"fmod64.dll"
#else // _WIN64
#define MUSICPLAYER_LIBNAME	"fmod.dll"
#endif
#else // _WINDOWS
#define MUSICPLAYER_LIBNAME	"libfmod.so"
#endif

#define MAX_SOFTWARE_CHANNELS 2

static FMOD_INSTANCE *gpFMOD = NULL;
static FSOUND_STREAM *g_pMusicStream = NULL;

cvar_t *mp3player = NULL;
cvar_t *bgm_driver = NULL;
cvar_t *bgm_quality = NULL;
cvar_t *bgm_dir = NULL;
cvar_t *bgm_volume = NULL;
cvar_t *bgm_pausable = NULL;
cvar_t *bgm_pls_loop = NULL;// XDM3035
cvar_t *bgm_playmaplist = NULL;
cvar_t *bgm_playcustomlist = NULL;
cvar_t *bgm_defplaylist = NULL;

char g_szPlayListFileName[32];
char g_PlayList[MAX_XPL_ENTRIES][MAX_PATH];// XDM3038c: fixed
int g_iPlayListPosiiton = 0;
int g_iPlayListNumItems = 0;
int g_iPlayStartOffsetSec = 0;// in seconds

short gMusicStatePlaying = MST_NONE;
short gMusicStateLast = MST_NONE;
byte g_MusicPlayerAsyncCmd = MPAC_NONE;
byte g_MusicPlayerLoopTrack = false;

bool g_iBGMInitialized = false;
//bool g_bXPLLoaded = false;


//-----------------------------------------------------------------------------
// music player console commands
//-----------------------------------------------------------------------------
void __CmdFunc_BGM_Play(void)
{
	if (CMD_ARGC() < 2)
	{
		conprintf(0, "Usage: %s <name> [loop 0/1]\n", CMD_ARGV(0));
		return;
	}
	char path[MAX_PATH];
	_snprintf(path, MAX_PATH, "%s/%s/%s.mp3", GET_GAME_DIR(), bgm_dir->string, CMD_ARGV(1));
	gMusicStateLast = gMusicStatePlaying;
	BGM_Play(path, atoi(CMD_ARGV(2)));
	if (g_pMusicStream)
		gMusicStatePlaying = MST_TRACK;
}

void __CmdFunc_BGM_PlayPath(void)
{
	if (CMD_ARGC() < 2)
	{
		conprintf(0, "Usage: %s <full path> [loop 0/1]\n", CMD_ARGV(0));
		return;
	}
	BGM_Play(CMD_ARGV(1), atoi(CMD_ARGV(2)));
}

void __CmdFunc_BGM_Stop(void)
{
	BGM_Stop();
}

void __CmdFunc_BGM_Pause(void)
{
	BGM_Pause();
}

void __CmdFunc_BGM_Info(void)
{
	BGM_Info();
}

void __CmdFunc_BGM_SetVolume(void)
{
	if (CMD_ARGC() < 2)
	{
		if (gpFMOD)
			conprintf(0, "music volume: %d\n", gpFMOD->FSOUND_GetVolume(0));
	}
	else
		BGM_SetVolume(atoi(CMD_ARGV(1)));
}

void __CmdFunc_BGM_Position(void)
{
	if (strlen(CMD_ARGV(1)) <= 0)
	{
		//if (stream != NULL)//( && FSOUND_IsPlaying(FSOUND_ALL))
		//	conprintf(0, "Current track position %f%%\n", gpFMOD->FSOUND_Stream_GetTime(stream)/gpFMOD->FSOUND_Stream_GetLengthMs(stream)*100);
		//else
			conprintf(0, "Usage: %s <1-100>\n", CMD_ARGV(0));
	}
	else
		BGM_SetPosition(atof(CMD_ARGV(1)));
}



//-----------------------------------------------------------------------------
// playlist-related console commands
//-----------------------------------------------------------------------------
void __CmdFunc_BGM_PLS_Load(void)
{
	if (CMD_ARGC() < 2)
	{
		/*if (g_bXPLLoaded)
			BGM_PLS_Play(-1, -1);
		else*/
			conprintf(0, "Usage: %s <filename> [loop 0/1]\n", CMD_ARGV(0));
	}
	else
		BGM_PLS_Load(CMD_ARGV(1));
}

void __CmdFunc_BGM_PLS_Play(void)
{
	if (CMD_ARGC() < 2)
	{
		if (g_iPlayListNumItems > 0)//g_bXPLLoaded)
		{
			conprintf(0, "Playing current list (%s)\n", g_szPlayListFileName);
			BGM_PLS_Play(-1, -1);
		}
		else
			conprintf(0, "Usage: %s <filename> [loop 0/1]\n", CMD_ARGV(0));
	}
	else
	{
		BGM_PLS_Load(CMD_ARGV(1));
		BGM_PLS_Play(0, atoi(CMD_ARGV(2)));
	}
}

void __CmdFunc_BGM_PLS_Next(void)
{
	BGM_PLS_Next();
}

void __CmdFunc_BGM_PLS_Prev(void)
{
	BGM_PLS_Prev();
}

void __CmdFunc_BGM_PLS_List(void)
{
	BGM_PLS_List();
}

void __CmdFunc_BGM_PLS_Track(void)
{
	if (CMD_ARGC() < 2)
	{
		conprintf(0, "Usage: %s <track#>. Current track: %d\n", CMD_ARGV(0), g_iPlayListPosiiton);
		return;
	}
	BGM_PLS_Play(atoi(CMD_ARGV(1)), -1);
}

void __CmdFunc_BGM_Shutdown(void)
{
	BGM_Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Server message: map loaded, trigger fired - play background music
//-----------------------------------------------------------------------------
int __MsgFunc_AudioTrack(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
		int iCommand = READ_BYTE();
		int iOffset = READ_SHORT();
		const char *pTrackName = READ_STRING();
	END_READ();
	DBG_PRINTF("__MsgFunc_%s: %d \"%s\" ofs %d\n", pszName, iCommand, pTrackName, iOffset);

	if (iCommand == MPSVCMD_RESET)
	{
		DBG_PRINTF("MPSVCMD_RESET\n");
	}
	else if (iCommand == MPSVCMD_PLAYFILE || iCommand == MPSVCMD_PLAYFILELOOP)
	{
		int loaded = 0;
		if (*pTrackName == '\0')// play track from map playlist
		{
			if (bgm_playmaplist && (bgm_playmaplist->value >= 1.0f))// play map playlist at the start of the game
			{
				if (bgm_playcustomlist && (bgm_playcustomlist->value > 0.0f) && bgm_defplaylist && bgm_defplaylist->string)// force use client's custom list?
				{
					if (g_szPlayListFileName)// a different playlist
					{
						if (strcmp(g_szPlayListFileName, bgm_defplaylist->string) == 0)// XDM3038c: already loaded
							loaded  = 1;
						else
							loaded = BGM_PLS_Load(bgm_defplaylist->string);
					}
				}
				else// load map playlist
					loaded = BGM_PLS_LoadForMap();

				if (loaded == 0 && IsMultiplayer())// XDM3038c: try loading gamerules playlist
				{
					for (uint32 px = 0; gGameTypePrefixes[px].prefix; ++px)// find text prefix of current game rules
					{
						if (gGameTypePrefixes[px].gametype == gHUD.m_iGameType)
						{
							char gameruleslist[32];
							_snprintf(gameruleslist, 32, "%s.xpl\0", gGameTypePrefixes[px].prefix);
							loaded = BGM_PLS_Load(gameruleslist);
							break;
						}
					}
				}
			}
		}
		else// filename was explicitly specified
		{
			loaded = BGM_PLS_Load(pTrackName);
			g_iPlayStartOffsetSec = iOffset;
			//if (gpFMOD) int iLengthMs = gpFMOD->FSOUND_Stream_GetLengthMs(g_pMusicStream))
			// while (iOffset * 1000 > iLengthMs) SkipTrack; fail because we would have to LOAD each track, measure length and continue. But what if player was looping the list of 200 songs for 2 hours?
		}
		if (loaded > 0)
			BGM_PLS_Play(0, (iCommand == MPSVCMD_PLAYFILELOOP)?1:0);
		else
			conprintf(1, "%s: unable to load any tracks or lists\n", pszName);
	}
	else if (iCommand == MPSVCMD_PAUSE)
	{
		if (mp3player)
		{
			if (mp3player->value > 1)
				BGM_Pause();
			else if (mp3player->value > 0)
				CLIENT_COMMAND("cd pause\n");
		}
	}
	else if (iCommand == MPSVCMD_STOP)
	{
		if (mp3player)
		{
			if (mp3player->value > 1)
				BGM_Stop();
			else if (mp3player->value > 0)
				CLIENT_COMMAND("cd stop\n");
		}
	}
	else if (iCommand == MPSVCMD_PLAYTRACK || iCommand == MPSVCMD_PLAYTRACKLOOP)
	{
		if (mp3player)
		{
			if (mp3player->value > 1)
			{
				char path[MAX_PATH];
				_snprintf(path, MAX_PATH, "%s/%s/%s.mp3", GET_GAME_DIR(), bgm_dir->string, pTrackName);
				gMusicStateLast = gMusicStatePlaying;
				BGM_Play(path, (iCommand == MPSVCMD_PLAYTRACKLOOP)?1:0);
				if (iOffset > 0)
					BGM_SetPositionMillisec(iOffset * 1000);// XDM3038: resume
			}
			else if (mp3player->value > 0)
			{
				char str[32];
				if (iCommand == MPSVCMD_PLAYTRACKLOOP)
					_snprintf(str, 32, "cd loop %s\n", pTrackName);
				else
					_snprintf(str, 32, "cd play %s\n", pTrackName);

				CLIENT_COMMAND(str);
			}
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Validate and correct user setting
// Output : Updated driver for FMOD
//-----------------------------------------------------------------------------
int BGM_UpdateDriver(void)
{
	int driver = (int)bgm_driver->value;
	if (driver > 6 || driver < 0)
	{
#if defined (_WIN32)
		driver = FSOUND_OUTPUT_DSOUND;// default for windows
#else
		driver = FSOUND_OUTPUT_OSS;// default for x
#endif // _WIN32
		conprintf(0, "BGM_UpdateDriver(): Incorrect driver! Resetting to default: %d.\n", driver);
	}
	CVAR_SET_FLOAT("bgm_driver", driver);
	return driver;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize music player (load DLL, set parameters, etc.). Once.
// Warning: Every pointer may be NULL and avery function may fail!
//-----------------------------------------------------------------------------
void BGM_Init(void)
{
	DBG_PRINTF("BGM_Init()\n");
	// These CVars and MSGs MUST be created!
	mp3player			= CVAR_CREATE("mp3player",			"2",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_playmaplist		= CVAR_CREATE("bgm_playmaplist",	"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_playcustomlist	= CVAR_CREATE("bgm_playcustomlist",	"0",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_defplaylist		= CVAR_CREATE("bgm_defplaylist",	"default.xpl",	FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_driver			= CVAR_CREATE("bgm_driver",
#ifdef _WIN32
		"2",
#else
		"6",	
#endif
			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_quality			= CVAR_CREATE("bgm_quality",		"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_dir				= CVAR_CREATE("bgm_dir",			"music",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_volume			= CVAR_CREATE("bgm_volume",			"255",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_pausable		= CVAR_CREATE("bgm_pausable",		"0",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);
	bgm_pls_loop		= CVAR_CREATE("bgm_pls_loop",		"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_UNLOGGED);

	HOOK_MESSAGE(AudioTrack);

	if (mp3player->value < 2)
		return;

	char path[MAX_PATH];
	_snprintf(path, MAX_PATH, "%s/cl_dlls/%s", GET_GAME_DIR(), MUSICPLAYER_LIBNAME);
	gpFMOD = FMOD_CreateInstance(path);
	// XDM3038b: modified FMOD API allows missing pointers so check everything we may need!
	if (gpFMOD == NULL ||
		gpFMOD->FSOUND_Init == NULL ||
		gpFMOD->FSOUND_Close == NULL ||
		gpFMOD->FSOUND_GetDriverName == NULL ||
		gpFMOD->FSOUND_GetDriver == NULL ||
		gpFMOD->FSOUND_SetDriver == NULL ||
		gpFMOD->FSOUND_GetVersion == NULL ||
		gpFMOD->FSOUND_GetError == NULL ||
		gpFMOD->FSOUND_SetBufferSize == NULL ||
		gpFMOD->FSOUND_SetOutput == NULL ||
		gpFMOD->FSOUND_SetPan == NULL ||
		gpFMOD->FSOUND_GetVolume == NULL ||
		gpFMOD->FSOUND_SetVolume == NULL ||
		gpFMOD->FSOUND_SetVolumeAbsolute == NULL ||
		gpFMOD->FSOUND_GetPaused == NULL ||
		gpFMOD->FSOUND_SetPaused == NULL ||
		gpFMOD->FSOUND_Stream_Open == NULL ||
		gpFMOD->FSOUND_Stream_Close == NULL ||
		gpFMOD->FSOUND_Stream_Play == NULL ||
		gpFMOD->FSOUND_Stream_Stop == NULL ||
		gpFMOD->FSOUND_Stream_Stop == NULL ||
		gpFMOD->FSOUND_Stream_GetOpenState == NULL ||
		gpFMOD->FSOUND_Stream_GetTime == NULL ||
		gpFMOD->FSOUND_Stream_SetTime == NULL ||
		gpFMOD->FSOUND_Stream_GetLengthMs == NULL ||
		gpFMOD->FSOUND_Stream_GetLength == NULL ||
		gpFMOD->FSOUND_Stream_SetEndCallback == NULL)
	{
		conprintf(0, "CL_DLL: ERROR! Unable to load '%s' or some functions missing! (check stderr)\n", path);
		mp3player->value = 1.0f;
		CVAR_SET_FLOAT("mp3player", 1.0f);
		return;
	}

	float ver = gpFMOD->FSOUND_GetVersion();
	if (ver != FMOD_VERSION)
		conprintf(0, "CL_DLL: WARNING! Incorrect fmod.dll version %f, expected %f!\n", ver, FMOD_VERSION);

	HOOK_COMMAND("bgm_play",		BGM_Play);
	HOOK_COMMAND("bgm_playpath",	BGM_PlayPath);
	HOOK_COMMAND("bgm_stop",		BGM_Stop);
	HOOK_COMMAND("bgm_pause",		BGM_Pause);
	HOOK_COMMAND("bgm_info",		BGM_Info);
	HOOK_COMMAND("bgm_setvolume",	BGM_SetVolume);
	HOOK_COMMAND("bgm_position",	BGM_Position);
	HOOK_COMMAND("bgm_pls_next",	BGM_PLS_Next);
	HOOK_COMMAND("bgm_pls_prev",	BGM_PLS_Prev);
	HOOK_COMMAND("bgm_pls_list",	BGM_PLS_List);
	HOOK_COMMAND("bgm_pls_track",	BGM_PLS_Track);
	HOOK_COMMAND("bgm_pls_play",	BGM_PLS_Play);
	HOOK_COMMAND("bgm_pls_load",	BGM_PLS_Load);
	HOOK_COMMAND("bgm_shutdown",	BGM_Shutdown);

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		gpFMOD->FSOUND_SetDriver(BGM_UpdateDriver());
		gpFMOD->FSOUND_SetBufferSize(200);
		g_iBGMInitialized = TRUE;
		conprintf(0, "CL_DLL: BGM_Init() <%s>\n", FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		g_iBGMInitialized = FALSE;
		printf("*** CL_DLL BGM_Init() exception!\n");
		conprintf(0, "*** CL_DLL BGM_Init() exception!\n");
//		DBG_FORCEBREAK
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: For external use
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_Initialized(void)
{
	return g_iBGMInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown
//-----------------------------------------------------------------------------
void BGM_Shutdown(void)
{
	DBG_PRINTF("BGM_Shutdown()\n");
	if (gpFMOD)
	{
		if (g_pMusicStream)
		{
			gpFMOD->FSOUND_Stream_Close(g_pMusicStream);
			g_pMusicStream = NULL;
		}

		gpFMOD->FSOUND_Close();
		FMOD_FreeInstance(gpFMOD);
		gpFMOD = NULL;
	}
	g_iBGMInitialized = FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: A stream has ended
// Input  : *stream - A pointer to the stream that ended.
//			*buff - This is NULL for end of stream callbacks, or a string for synch callbacks.
//			len - This is reserved and is always 0 for end and synch callbacks. ignore.
//			userdata - A user data value specified from FSOUND_Stream_Create.
// Output : The return value can be TRUE or FALSE it is ignored.
//-----------------------------------------------------------------------------
signed char F_CALLBACKAPI BGM_EndCallback(FSOUND_STREAM *stream, void *buff, int len, void *userdata)// FMOD375
{
	if (gpFMOD == NULL || stream == NULL)
		return 0;

	//int off = gpFMOD->FSOUND_Stream_GetTime(stream);
	//if (buff == NULL || (off >= gpFMOD->FSOUND_Stream_GetLengthMs(stream)))// end of stream callback doesnt have a 'buff' value
	{
		if ((gMusicStatePlaying == MST_PLAYLIST)/* && (gpFMOD->FSOUND_GetLoopMode(FSOUND_ALL) == 0)*/)
		{
			// Optimization: don't unload and reload the same track
			if ((g_iPlayListNumItems <= 1 || (g_iPlayListPosiiton < g_iPlayListNumItems-1 && strcmp(g_PlayList[g_iPlayListPosiiton], g_PlayList[g_iPlayListPosiiton+1]) == 0))
				&& bgm_pls_loop->value > 0.0f)// just loop the track
			{
				if (g_iPlayListNumItems > 1)// found duplicate entry, next track is the same!
					++g_iPlayListPosiiton;// advance to prevent eternal loop

				//conprintf(1, "CL: BGM_EndCallback() looping track\n");
				gpFMOD->FSOUND_Stream_Play(FSOUND_FREE, g_pMusicStream);
				return TRUE;
			}

			//gpFMOD->FSOUND_Stream_Stop(stream);
			BGM_Stop();
			gpFMOD->FSOUND_Stream_Close(stream);// XDM3035a TESTME
			if (g_pMusicStream == stream)
				g_pMusicStream = NULL;
			else
				conprintf(1, "CL: BGM_EndCallback() stream differs from g_pMusicStream!\n");

			BGM_PLS_Next();
		}
		else if (gMusicStatePlaying != MST_NONE)
		{
			if (g_MusicPlayerLoopTrack)// XDM3037
				g_MusicPlayerAsyncCmd = MPAC_LOADING;// this will eventually force player to PlayStart()
			else
			{
				gMusicStatePlaying = MST_NONE;
				conprintf(0, "CL: Music state cleared.\n");

				if (g_iPlayListNumItems > 0 /*g_bXPLLoaded*/ && gMusicStateLast == MST_PLAYLIST)// a playlist was loaded before a music event occurred
					BGM_PLS_Play(-1, -1);// resume
			}
		}
	}
	return 1;// ignored
}

//-----------------------------------------------------------------------------
// Purpose: Play audio file
// Input  : *path - absolute path (for normal user playlist support)
//			loop - 0/1 or -1 == "don't change last loop setting"
// Note   : Should not change or use gMusicStatePlaying/last
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_Play(const char *path, int loop)
{
	if (path == NULL)
		return false;

#if defined (CLDLL_NEWFUNCTIONS)
	if (mp3player->value == 1.0f)
	{
		gEngfuncs.pfnPrimeMusicStream(path, loop);// XDM3038: UNTESTED
		CLIENT_COMMAND("mp3 play\n");
		return true;
	}
#endif

	if (!g_iBGMInitialized)
		BGM_Init();

	if (!g_iBGMInitialized)// still have problems?
		return false;

	if (gpFMOD == NULL)
		return false;

	//ASSERT(g_MusicPlayerAsyncCmd == MPAC_NONE);

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		unsigned int hz = 22050;
		if (bgm_quality->value > 0.0f)
			hz = 44100;

		if (!gpFMOD->FSOUND_Init(hz, MAX_SOFTWARE_CHANNELS, 0/*FSOUND_INIT_GLOBALFOCUS*/))
		{
			conprintf(0, "CL: Unable to initialize fmod player! %s\n", FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
			return false;
		}
		gpFMOD->FSOUND_SetOutput(BGM_UpdateDriver());
		gpFMOD->FSOUND_SetPan(FSOUND_ALL, FSOUND_STEREOPAN);
		//Pathname_Convert(path);// works fine without this
		g_pMusicStream = gpFMOD->FSOUND_Stream_Open(path, FSOUND_LOOP_OFF|FSOUND_NONBLOCKING, 0, 0);// FMOD375

		if (g_pMusicStream)
		{
			int vol = (int)bgm_volume->value;
			if (vol > 0)
			{
				if (vol > BGM_VOLUME_MAX)// no FMOD constant??
					vol = BGM_VOLUME_MAX;
				gpFMOD->FSOUND_SetVolume(FSOUND_ALL, vol);
			}

			if (loop != -1)
				g_MusicPlayerLoopTrack = loop;

			conprintf(0, "BGM: loading \"%s\" <loop: track %d list %d> <%s>\n", path, loop, (int)bgm_pls_loop->value, FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
			g_MusicPlayerAsyncCmd = MPAC_LOADING;
			return true;
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		conprintf(0, "*** CL_DLL BGM_Play() exception!\n");
		//DBG_FORCEBREAK
	}
#endif
	g_MusicPlayerAsyncCmd = MPAC_NONE;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: asynchronous start function. Does not work really.
// Warning: All parameters are stored in global variables.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_PlayStart(void)
{
	ASSERT(g_MusicPlayerAsyncCmd == MPAC_LOADING);
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (gpFMOD == NULL)
		{
			conprintf(0, "BGM_PlayStart(): error: null gpFMOD!\n");
			return false;
		}
		if (g_pMusicStream == NULL)
		{
			conprintf(0, "BGM_PlayStart(): error: null stream! (%s)\n", FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
			return false;
		}
		int vol = (int)bgm_volume->value;
		if (vol > 0)
		{
			if (vol > BGM_VOLUME_MAX)// no FMOD constant??
				vol = BGM_VOLUME_MAX;

			gpFMOD->FSOUND_SetVolume(FSOUND_ALL, vol);
			gpFMOD->FSOUND_SetVolumeAbsolute(FSOUND_ALL, vol);// XDM3037a: ?
		}
		CLIENT_COMMAND("mp3 stop\n");// stop soundtrack/title song played by HL engine
		if (gpFMOD->FSOUND_Stream_Play(FSOUND_FREE, g_pMusicStream) != -1)
		{
			g_MusicPlayerAsyncCmd = MPAC_PLAYING;
			conprintf(0, "BGM: playing <loop list %d>\n", (int)bgm_pls_loop->value);
			if (gpFMOD->FSOUND_Stream_SetEndCallback(g_pMusicStream, BGM_EndCallback, 0) == FALSE)
				conprintf(1, "CL: BGM: SetEndCallback() failed!\n");

			if (gViewPort && gViewPort->GetMusicPlayer())
				gViewPort->GetMusicPlayer()->Update();

			gpFMOD->FSOUND_SetVolume(FSOUND_ALL, vol);
			gpFMOD->FSOUND_SetVolumeAbsolute(FSOUND_ALL, vol);// XDM3037a: ?
			if (g_iPlayStartOffsetSec > 0)// initial offset was specified
			{
				if (!BGM_SetPositionMillisec(g_iPlayStartOffsetSec * 1000))
					conprintf(0, "BGM_PlayStart(): warning: unable to set position. (%s)\n", FMOD_ErrorString(gpFMOD->FSOUND_GetError()));

				g_iPlayStartOffsetSec = 0;// reset right away!
			}
			return true;
		}
		else
			conprintf(0, "BGM_PlayStart(): error: unable to play stream! (%s)\n", FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		conprintf(0, "*** CL_DLL BGM_PlayStart() exception!\n");
//		DBG_FORCEBREAK
	}
#endif
	//g_MusicPlayerAsyncCmd = MPAC_NONE;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called by the game every frame
//-----------------------------------------------------------------------------
void BGM_StartFrame(void)
{
	if (gpFMOD == NULL)
		return;

	// Read previously issued asynchronous commands and execute them
	if (g_MusicPlayerAsyncCmd == MPAC_LOADING)
	{
		if (gpFMOD->FSOUND_Stream_GetOpenState(g_pMusicStream) == 0)// success
			if (BGM_PlayStart() == false)
				g_MusicPlayerAsyncCmd = MPAC_NONE;
	}
	else if (g_MusicPlayerAsyncCmd == MPAC_PLAYING)
	{
		// no	g_MusicPlayerAsyncCmd = MPAC_NONE;
	}
	else if (g_MusicPlayerAsyncCmd == MPAC_STOPPING)
	{
		//if (BGM_StopDo() == true)// UNDONE: async
		g_MusicPlayerAsyncCmd = MPAC_NONE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Toggle pause
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_Pause(void)
{
	/*if (mp3player->value == 1.0f)
	{
		there's no such command		CLIENT_COMMAND("mp3 pause\n");
	}*/
	if (gpFMOD == NULL)
		return false;

	if (g_pMusicStream)
	{
		signed char state = gpFMOD->FSOUND_GetPaused(0);// XDM3037a: does not eat FSOUND_ALL
		if (state == 0)
			state = 1;
		else
			state = 0;

		gpFMOD->FSOUND_SetPaused(FSOUND_ALL, state);
		conprintf(0, "music pause set to %d <%s>\n", (state==0)?(int)0:(int)1, FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Stop completely without remembering position
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_Stop(void)
{
	if (mp3player && mp3player->value == 1.0f)
		CLIENT_COMMAND("mp3 stop\n");

	g_MusicPlayerAsyncCmd = MPAC_STOPPING;// UNDONE: async
	gMusicStatePlaying = MST_NONE;

	if (gpFMOD == NULL)
		return false;

	if (g_pMusicStream)
	{
		if (gpFMOD->FSOUND_Stream_Stop(g_pMusicStream) == TRUE)
		{
			conprintf(0, "music stopped <%s>\n", FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Print info to console
//-----------------------------------------------------------------------------
void BGM_Info(void)
{
	if (gpFMOD && g_pMusicStream)
	{
		int ms = BGM_GetTimeMs();
		int min = ms/60000;
		float sec = (float)ms/1000.0f - min*60;
		conprintf(0, "Music player information\n Playlist: \"%s\", track: \"%s\" (%d/%d) @ %d:%.2f\n driver: %s, version: %f, last error: %s\n",
			g_szPlayListFileName, BGM_PLS_GetTrackName(), g_iPlayListPosiiton, g_iPlayListNumItems, min, sec,
			gpFMOD->FSOUND_GetDriverName(gpFMOD->FSOUND_GetDriver()), gpFMOD->FSOUND_GetVersion(), FMOD_ErrorString(gpFMOD->FSOUND_GetError()));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : volume - 0...BGM_VOLUME_MAX
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_SetVolume(int volume)
{
	if (volume > BGM_VOLUME_MAX)
		volume = BGM_VOLUME_MAX;
	else if (volume < 0)
		volume = 0;

	CVAR_SET_FLOAT(bgm_volume->name, volume);
	bgm_volume->value = (float)volume;
	//CVAR_SET_FLOAT("MP3Volume", volume);

	if (gpFMOD)
	{
		gpFMOD->FSOUND_SetVolume(FSOUND_ALL, volume);
		gpFMOD->FSOUND_SetVolumeAbsolute(FSOUND_ALL, volume);// XDM3037a: ?
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Set L/R panning
// Input  : balance - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_SetBalance(int balance)
{
	if (gpFMOD)
	{
		if (gpFMOD->FSOUND_SetPan(FSOUND_ALL, balance) == TRUE)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Set playing position
// Input  : percent - 0...100
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_SetPosition(float percent)
{
	if (gpFMOD)
	{
		if (percent < 0)
			percent = 0;
		else if (percent > 100.0f)
			percent = 100.0f;

		if (gpFMOD->FSOUND_Stream_SetTime(g_pMusicStream, (int)((float)gpFMOD->FSOUND_Stream_GetLengthMs(g_pMusicStream)*(float)percent/100.0f)) == TRUE)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Set playing position
// Input  : mseconds - used by server message
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool BGM_SetPositionMillisec(int mseconds)
{
	if (gpFMOD)
	{
		if (gpFMOD->FSOUND_Stream_SetTime(g_pMusicStream, mseconds) == TRUE)
			return true;
	}
	return false;
}

int BGM_GetPosition(void)
{
	if (gpFMOD && gpFMOD->FSOUND_Stream_GetLength(g_pMusicStream)>0)
		return (int)(((float)gpFMOD->FSOUND_Stream_GetTime(g_pMusicStream)/(float)gpFMOD->FSOUND_Stream_GetLengthMs(g_pMusicStream))*100.0f);

	return 0;
}

bool BGM_IsPlaying(void)
{
	if (gpFMOD && g_pMusicStream && g_MusicPlayerAsyncCmd == MPAC_PLAYING)
		return (gpFMOD->FSOUND_Stream_GetTime(g_pMusicStream) > 0);// does not work gpFMOD->FSOUND_IsPlaying(FSOUND_ALL);

	return false;
}

int BGM_GetTimeMs(void)
{
	if (gpFMOD && g_pMusicStream)
		return gpFMOD->FSOUND_Stream_GetTime(g_pMusicStream);

	return 0;
}

int BGM_GetLengthMs(void)
{
	if (gpFMOD && g_pMusicStream)
		return gpFMOD->FSOUND_Stream_GetLengthMs(g_pMusicStream);

	return 0;
}

int BGM_GetVolume(void)
{
	//if (gpFMOD && g_pMusicStream && g_MusicPlayerAsyncCmd == MPAC_PLAYING)
	int extvol;
	if (bgm_volume)
		extvol = (int)bgm_volume->value;
	else
		extvol = 0;

	if (gpFMOD && BGM_IsPlaying())
	{
		int intvol = gpFMOD->FSOUND_GetVolume(0);
		if (extvol != intvol)// XDM3037a: HACK HAAAAAAAAAACK!!! FMOD IS BUGGED! It resets to 255 after Play() no matter what!!
		{
			conprintf(0, "gpFMOD->FSOUND_GetVolume(0) returned %d\n", intvol);
			BGM_SetVolume(extvol);// validator inside
			//gpFMOD->FSOUND_SetVolume(FSOUND_ALL, extvol);
		}
	}
	return extvol;
}

void BGM_GamePaused(int paused)
{
	if (!g_iBGMInitialized)
		return;

	if (gpFMOD == NULL || g_pMusicStream == NULL || (bgm_pausable && bgm_pausable->value <= 0))
		return;

	if (paused > 0)// player has paused the game
	{
		if (!gpFMOD->FSOUND_GetPaused(FSOUND_ALL))// playing
			gpFMOD->FSOUND_SetPaused(FSOUND_ALL, 1);
	}
	else// player has unpaused the game
	{
		if (gpFMOD->FSOUND_GetPaused(FSOUND_ALL))// paused
			BGM_Pause();
	}
}


//-----------------------------------------------------------------------------
// playlist-related functions
//-----------------------------------------------------------------------------

// filename - relative with extension
int BGM_PLS_Load(const char *filename)
{
	if (!g_iBGMInitialized)
		return 0;

	if (filename)
	{
		if (gMusicStatePlaying == MST_PLAYLIST && (strcmp(g_szPlayListFileName, filename) == 0))// XDM3038: user must be able to reload the list by stopping, so ignore only while playing
		{
			conprintf(0, "BGM: %s is already playing - stop before reloading.\n", filename);
			return 1;
		}
		// XDM: allow reloading of a playlist	if (g_szPlayListFileName != NULL && strcmp(g_szPlayListFileName, filename) == 0)
		//return 1;// already loaded

		char path[MAX_PATH];
		_snprintf(path, MAX_PATH, "%s/%s/%s\0", GET_GAME_DIR(), bgm_dir->string, filename);
		path[MAX_PATH-1] = 0;

		FILE *f = fopen(path, "r");
		if (!f)
		{
			conprintf(0, "BGM: Unable to load %s!\n", filename);
			return 0;
		}

		g_iPlayListPosiiton = 0;
		g_iPlayListNumItems = 0;
		while (fgets(g_PlayList[g_iPlayListNumItems], MAX_PATH, f) != NULL)
		{
			if (!strncmp(g_PlayList[g_iPlayListNumItems], "//", 2) || g_PlayList[0][g_iPlayListNumItems] == '\n')
			{
				continue;// skip comments or blank lines
			}
			else if (!_stricmp(g_PlayList[g_iPlayListNumItems], "#EXTM3U\n"))// skip m3u tags
			{
				conprintf(1, "M3U playlist format recognized\n");
				continue;
			}
			else if (g_PlayList[g_iPlayListNumItems][0] == '#')//(!strnicmp(playlist[g_iPlayListNumItems], "#EXTINF", 7))
				continue;

			char *ch = strchr(g_PlayList[g_iPlayListNumItems], '\n');
			if (ch == NULL)// try windows-style
				ch = strchr(g_PlayList[g_iPlayListNumItems], '\r');

			if (ch)// force all strings to end only with file extension
				*ch = '\0';// not \n or something else

			++g_iPlayListNumItems;

			if (g_iPlayListNumItems >= MAX_XPL_ENTRIES)
				break;
		}
		fclose(f);
		conprintf(0, "BGM: %s loaded: %d(max %d) entries.\n", filename, g_iPlayListNumItems, MAX_XPL_ENTRIES);
		//g_bXPLLoaded = true;
		strncpy(g_szPlayListFileName, filename, 32);
		return g_iPlayListNumItems;
	}
	return 0;
}

// XDM3037a
int BGM_PLS_LoadForMap(void)
{
	const char *level = GET_LEVEL_NAME();
	if (level)
	{
		char s[64];
		//_splitpath(level, NULL, NULL, fname, NULL);
		strncpy(s, level, 64);
		s[strlen(level)-4] = 0;// '.bsp'
		strcat(s, ".xpl");
		return BGM_PLS_Load(s+5);// skip maps/
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : track - Number or "-1" which means "play curently selected track"
//			loop - "-1" means "don't change current setting"
//-----------------------------------------------------------------------------
bool BGM_PLS_Play(int track, int loop)
{
	if (!g_iBGMInitialized)
		return false;

	if (g_iPlayListNumItems <= 0)//!g_bXPLLoaded)
	{
		conprintf(0, "BGM: playlist not loaded!\n");
		return false;// XDM3038c
	}

	if (track < 0)// play curently selected track
		track = g_iPlayListPosiiton;

	// now it's safe to validate track ID
	if (track < 0 || track >= g_iPlayListNumItems)
	{
		conprintf(0, "BGM: Track number is incorrect!\n");
		return false;
	}

	if (g_PlayList[track] == NULL)
	{
		conprintf(0, "BGM: Track %d is NULL!\n", track);
		return false;
	}

	g_iPlayListPosiiton = track;

	BGM_PLS_SetLoopMode(loop);

	conprintf(0, " BGM track %d: ", g_iPlayListPosiiton);
	bool success = false;
	if (_strnicmp(g_PlayList[g_iPlayListPosiiton], "%musicdir%", 10) == 0)// a special tag for XDM
	{
		char path[MAX_PATH];
		_snprintf(path, MAX_PATH, "%s/%s%s", GET_GAME_DIR(), bgm_dir->string, g_PlayList[g_iPlayListPosiiton]+10);
		success = BGM_Play(path, loop);
	}
	else
	{
		bool isAbsolute;
#if defined (_WIN32)
		if (g_PlayList[g_iPlayListPosiiton][0] == '\\' && g_PlayList[g_iPlayListPosiiton][1] == '\\')// "\\mount\dir\"
		{
			isAbsolute = true;
		}
		else
		{
			char drive[_MAX_DRIVE];
			_splitpath(g_PlayList[g_iPlayListPosiiton], drive, NULL, NULL, NULL);
			isAbsolute = (drive[0] != '\0');//if (drive[0] != '\0')// this is an absolute path
		}
#else
		isAbsolute = (g_PlayList[g_iPlayListPosiiton][0] == '/');
#endif
		if (isAbsolute)// this is an absolute path
		{
			success = BGM_Play(g_PlayList[g_iPlayListPosiiton], loop);
		}
		else// relative path (just like winamp understands this)
		{
			char path[MAX_PATH];
			_snprintf(path, MAX_PATH, "%s/%s/%s", GET_GAME_DIR(), bgm_dir->string, g_PlayList[g_iPlayListPosiiton]);
			success = BGM_Play(path, loop);
		}
	}
	gMusicStatePlaying = MST_PLAYLIST;
	return success;
}

bool BGM_PLS_Next(void)
{
	if (g_iPlayListNumItems <= 0)//!g_bXPLLoaded)
	{
		conprintf(0, "BGM: Playlist is not loaded!\n");
		return false;
	}
	if (!g_iBGMInitialized)
		return false;

	++g_iPlayListPosiiton;

	if (g_iPlayListPosiiton >= g_iPlayListNumItems)
	{
		if (bgm_pls_loop->value > 0.0f)
			g_iPlayListPosiiton = 0;
		else
		{
			g_iPlayListPosiiton = g_iPlayListNumItems-1;// don't advance
			return true;// don't play
		}
	}
	return BGM_PLS_Play(-1, -1);
}

bool BGM_PLS_Prev(void)
{
	if (g_iPlayListNumItems <= 0)//!g_bXPLLoaded)
	{
		conprintf(0, "BGM: Playlist is not loaded!\n");
		return false;
	}
	if (!g_iBGMInitialized)
		return false;

	--g_iPlayListPosiiton;

	if (g_iPlayListPosiiton < 0)
	{
		if (bgm_pls_loop->value > 0.0f)
			g_iPlayListPosiiton = g_iPlayListNumItems-1;
		else
		{
			g_iPlayListPosiiton = 0;// don't advance
			return true;// don't play
		}
	}
	return BGM_PLS_Play(-1, -1);
}

void BGM_PLS_List(void)
{
	if (g_iPlayListNumItems > 0)//g_bXPLLoaded)
	{
		for (int i = 0; i < g_iPlayListNumItems; ++i)
			conprintf(0, " %d: %s\n", i, g_PlayList[i]);
	}
	else
		conprintf(0, "BGM: Playlist is not loaded\n");
}

void BGM_PLS_SetLoopMode(short loop)
{
	if (loop >= 0)// -1 means no change
		bgm_pls_loop->value = (float)loop;
}

char *BGM_PLS_GetTrackName(void)
{
	return g_PlayList[g_iPlayListPosiiton];
}
