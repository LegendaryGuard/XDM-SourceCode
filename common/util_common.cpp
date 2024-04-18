//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
// IMPORTANT: this file is included by all DLLs!
// One function may use different DLL APIs for the same purpose depending on project it is used in
//-----------------------------------------------------------------------------
#if defined (CLIENT_DLL)
#include "hud.h"
#include "cl_util.h"
#else
#include "extdll.h"
#include "util.h"
#include "game.h"
#endif
#include "util_common.h"
#include "color.h"
#include "colors.h"
#include "pm_defs.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "decals.h"
#include "shared_resources.h"
#include <time.h>
#if defined (_WIN32)
#include <io.h>
#else
#include <dirent.h>
#endif

//-----------------------------------------------------------------------------
// Purpose: Print text into debugger trace window (if possible)
// Input  : szFmt - printf-like arguments
//-----------------------------------------------------------------------------
void DBG_PrintF(const char *szFmt, ...)
{
	if (szFmt == NULL)
		return;
	static char	string[1024];
	va_list argptr;
	va_start(argptr, szFmt);
	_vsnprintf(string, 1024, szFmt, argptr);
	va_end(argptr);
	//gEngfuncs.Con_Printf("CL DBG: ");
	//gEngfuncs.Con_Printf(string);
	//SERVER_PRINT("SV DBG: ");
	//SERVER_PRINT(string);
#if defined(_WIN32)
	OutputDebugString(string);
#else
	puts(string);
#endif
}


#if defined (_DEBUG)
//-----------------------------------------------------------------------------
// Purpose: Check validity of expression and raise an error message on failure
// Input  : fExpr - expression (result)
//			*szExpr - #expression (filled by the preprocessor)
//			*szFile - __FILE__
//			iLine - __LINE__
//			*szMessage - optional message
//-----------------------------------------------------------------------------
bool DBG_AssertFunction(bool fExpr, const char *szExpr, const char *szFile, int iLine, const char *szMessage)
{
	if (fExpr)
		return true;

	const char *szModule;
#if defined (CLIENT_DLL)
	szModule = "CL";
#elif defined (SERVER_DLL)
	szModule = "SV";
#else
	szModule = "DLL";
#endif

	if (szMessage != NULL)
		conprintf(1, "%s: ASSERT FAILED: %s (%s@%d)\n%s", szModule, szExpr, szFile, iLine, szMessage);
	else
		conprintf(1, "%s: ASSERT FAILED: %s (%s@%d)\n", szModule, szExpr, szFile, iLine);

	if (IS_DEDICATED_SERVER() == 0 && g_pCvarDeveloper && g_pCvarDeveloper->value > 2.0f)// only bring up the "abort retry ignore" dialog if in debug mode!
	{
#if _MSC_VER > 1200
	//_ASSERTE
	//_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
	if (_CrtDbgReport(_CRT_ASSERT, szFile, iLine, NULL, szExpr) == 1)// "retry"
		_CrtDbgBreak();

	//_wassert(CRT_WIDE(szExpr), _CRT_WIDE(szFile), iLine);
#else// older VS and other platforms
#if defined (_WIN32)
		_assert((void *)szExpr, (void *)szFile, iLine);
#else// if there's no POSIX/ISO equivalent for this:
		fprintf(stderr, "ASSERT FAILED: %s (%s@%d)\n", szExpr, szFile, iLine);
		//getch();// halt execuiton?
#endif
#endif
	}
	return fExpr;
}
#else// XDM: ASSERT function for release build
bool NDB_AssertFunction(bool fExpr, const char *szExpr, const char *szMessage)
{
	if (fExpr)
		return true;

	if (szMessage != NULL)
		conprintf(1, "SV: ASSERT FAILED: %s %s\n", szExpr, szMessage);
	else
		conprintf(1, "SV: ASSERT FAILED: %s\n", szExpr);

	return fExpr;
}
#endif	// DEBUG


//-----------------------------------------------------------------------------
// Purpose: Print text into console. Universal server/client version
// Note   : Lots of code just to use different engine funcitons so engine may do some level detection too.
// Input  : devlevel - 0,1,2... higher = more printed
//			format - printf-like arguments
//-----------------------------------------------------------------------------
void conprintf(int devlevel, const char *format, ...)
{
	if (format == NULL)
		return;
	if (g_pCvarDeveloper != NULL && g_pCvarDeveloper->value < devlevel)
		return;

	static char	string[1024];
	va_list argptr;
	va_start(argptr, format);
	_vsnprintf(string, 1024, format, argptr);
	va_end(argptr);
	if (devlevel >= 1)
	{
		if ((g_pCvarDeveloper == NULL || g_pCvarDeveloper->value >= devlevel))
#if defined(CLIENT_DLL)
			gEngfuncs.Con_DPrintf(string);
#else // CLIENT_DLL
		{
			if (devlevel > 1)
				ALERT(at_aiconsole, string);
			else
				ALERT(at_console, string);
		}
#endif
	}
	else// straight to console
	{
#if defined(CLIENT_DLL)
		gEngfuncs.Con_Printf(string);
#else // CLIENT_DLL
		ALERT(at_console, string);
#endif
	}

#if !defined(CLIENT_DLL) && defined (_DEBUG)
	if (IS_DEDICATED_SERVER() && g_pCvarDeveloper != NULL && g_pCvarDeveloper->value > 1)
		SERVER_PRINT(string);
#endif // _DEBUG
}

//-----------------------------------------------------------------------------
// Purpose: Convert sizeless arguments into a normal string
// Input  : format - printf-like arguments
// Output : char * - temporary string
//-----------------------------------------------------------------------------
char *UTIL_VarArgs(const char *format, ...)
{
	if (format == NULL)
		return NULL;

	va_list argptr;
	static char string[1024];
	va_start(argptr, format);
	_vsnprintf(string, 1024, format, argptr);
	va_end(argptr);
	return string;
}


//-----------------------------------------------------------------------------
// Purpose: Weapon shared code
// Output : float
//-----------------------------------------------------------------------------
float UTIL_WeaponTimeBase(void)
{
#if defined(CLIENT_WEAPONS)
	return 0.0f;
#else
#if defined(CLIENT_DLL)
	return gHUD.m_flTime;// gEngfuncs.GetClientTime()? is this better?
#else
	return gpGlobals->time;
#endif // CLIENT_DLL
#endif // CLIENT_WEAPONS
}

static unsigned int glSeed = 0; 

unsigned int seed_table[256] =
{
	28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103,
	27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315,
	26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823,
	10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223,
	10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031,
	18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630,
	18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439,
	28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241,
	31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744,
	21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208,
	617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320,
	18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668,
	12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761,
	9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203,
	29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409,
	25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847
};

unsigned int U_Random(void) 
{ 
	glSeed *= 69069; 
	glSeed += seed_table[glSeed & 0xff];
 	return (++glSeed & 0x0fffffff);
} 

void U_Srand(const unsigned int &seed)
{
	glSeed = seed_table[seed & 0xff];
}

int UTIL_SharedRandomLong(const unsigned int &seed, const int &low, const int &high)
{
	U_Srand((int)seed + low + high);
	unsigned int range = high - low + 1;
	if (!(range - 1))
	{
		return low;
	}
	else
	{
		int rnum = U_Random();
		int offset = rnum % range;
		return (low + offset);
	}
}

float UTIL_SharedRandomFloat(const unsigned int &seed, const float &low, const float &high)
{
	U_Srand((int)seed + *(int *)&low + *(int *)&high);
	U_Random();
	U_Random();
	float range = high - low;// XDM3035c: TESTME
	if (range == 0.0f)
	{
		return low;
	}
	else
	{
		int tensixrand = U_Random() & 65535;
		float offset = (float)tensixrand / 65536.0f;
		return (low + offset * range);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse 4 floats from string
// Note   : Resulting output values are undefined on failure.
// Input  : *str - "0.0 0.0 0.0 0.0"
//			*array4f - output float[4]
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringTo4F(const char *str, float *array4f)
{
	if (str && *str)
	{
		if (sscanf(str, "%f %f %f %f", &array4f[0], &array4f[1], &array4f[2], &array4f[3]) == 4)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse array or one element
// Note   : Uses StringTo4F()
// Input  : *name - "asdf" or "asdf[0]"
//			*value - "0.0" or "0.0 0.0 0.0 0.0"
//			*array4f - output float[]
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ParseArray4f(const char *name, const char *value, float *array4f)
{
	uint32 index = UINT_MAX;
	const char *pStart = strchr(name, '[');
	if (pStart)// successfully retrieved element index
	{
		if (sscanf(pStart, "[%u]", &index) == 1)
		{
			if (index >= 0 && index < 4)
			{
				array4f[index] = atof(value);// the value should be 1 float here
				return true;
			}
		}
	}
	else
		return StringTo4F(value, array4f);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get gear number for client HUD
// Input  : &iSpeed - current speed
//			&iMax - maximum speed
// Output : int - speed mode value (for client HUD). Lowest starts from 1
//-----------------------------------------------------------------------------
int TrainSpeed(const int &iSpeed, const int &iMax)
{
	float delta = iMax/TRAIN_NUMSPEEDMODES;
	//float fSpeed = (float)iSpeed/(float)iMax;
	//iRet = (int)(fSpeed*TRAIN_NUMSPEEDMODES);
	if (delta == 0.0f)
		return 0;

	int iRet = (int)((float)iSpeed/delta);// now it should be -4 to +4
	iRet += TRAIN_NUMSPEEDMODES+1;// now 1 to 9
	return iRet;
}

//-----------------------------------------------------------------------------
// Purpose: Universal point contents checker
// Input  : vPoint - point in world
// Output : int - CONTENTS_
//-----------------------------------------------------------------------------
int UTIL_PointContents(const Vector &vPoint)// XDM3035
{
#if defined(CLIENT_DLL)
	return gEngfuncs.PM_PointContents(vPoint, NULL);
#else
	return POINT_CONTENTS(vPoint);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Checks if point is in any liquid
// Input  : vPoint - point in world
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_LiquidContents(const Vector &vPoint)
{
	int pc = UTIL_PointContents(vPoint);//int pc = POINT_CONTENTS(vPoint);
	if (pc < CONTENTS_SOLID && pc > CONTENTS_SKY)
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			fMinZ, fMaxZ -
// Output : z
//-----------------------------------------------------------------------------
vec_t UTIL_WaterLevel(const Vector &position, vec_t fMinZ, vec_t fMaxZ)
{
	Vector midUp(position);
	midUp.z = fMinZ;

	if (UTIL_PointContents(midUp) != CONTENTS_WATER)// no water at bottom point means no water at all
		return fMinZ;

	midUp.z = fMaxZ;
	if (UTIL_PointContents(midUp) == CONTENTS_WATER)// top point is in water, everything is in water
		return fMaxZ;

	vec_t diff = fMaxZ - fMinZ;
	while (diff > 1.0f)
	{
		midUp.z = fMinZ + diff/2.0f;
		if (UTIL_PointContents(midUp) == CONTENTS_WATER)
			fMinZ = midUp.z;
		else
			fMaxZ = midUp.z;

		diff = fMaxZ - fMinZ;
	}

	return midUp.z;
}

//-----------------------------------------------------------------------------
// Purpose: Convert blood color to streak color (see "r_efx.h")
// Input  : color - index in palette.lmp
// Output : int - index in gTracerColors
//-----------------------------------------------------------------------------
int UTIL_BloodToStreak(int color)
{
	switch (color)
	{
	default:
	case BLOOD_COLOR_BLACK:		return 4; break;
	case BLOOD_COLOR_GRAY:		return 4; break;
	case BLOOD_COLOR_MAGENTA:	return 1; break;
	case BLOOD_COLOR_YELLOW:	return 6; break;
	case BLOOD_COLOR_BLUE:		return 3; break;
	case BLOOD_COLOR_GREEN:		return 2; break;
	case BLOOD_COLOR_HUM_SKN:	return 5; break;
	case BLOOD_COLOR_CYAN:		return 3; break;
	case BLOOD_COLOR_RED:		return 1; break;
	case BLOOD_COLOR_WHITE:		return 0; break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Select random decal by blood color
// Input  : color - index in palette.lmp
// Output : int - index in decal_e
//-----------------------------------------------------------------------------
int UTIL_BloodDecalIndex(const int &bloodColor)
{
	/*if (bloodColor >= BLOOD_COLOR_RED && bloodColor <= BLOOD_COLOR_RED4)
		return RANDOM_LONG(DECAL_BLOOD1, DECAL_BLOOD6);
	else if (bloodColor >= BLOOD_COLOR_BLUE && bloodColor <= BLOOD_COLOR_BLUE7)
		return RANDOM_LONG(DECAL_BBLOOD1, DECAL_BBLOOD3);
	else if (bloodColor >= BLOOD_COLOR_GREEN && bloodColor <= BLOOD_COLOR_GREEN7)
		return RANDOM_LONG(DECAL_GBLOOD1, DECAL_GBLOOD3);
	else if (bloodColor >= 32 && bloodColor <= 47)
		return RANDOM_LONG(DECAL_BBLOOD1, DECAL_BBLOOD3);
	else if (bloodColor >= 64 && bloodColor <= 79)
		return RANDOM_LONG(DECAL_BLOOD1, DECAL_BLOOD6);
	else
		return RANDOM_LONG(DECAL_YBLOOD1, DECAL_YBLOOD6);*/

	// We could convert RGB to HSL and then check range of a hue, but that's a lot of calculations
	if (g_Palette[bloodColor].r > g_Palette[bloodColor].g && g_Palette[bloodColor].r > g_Palette[bloodColor].b)
		return RANDOM_LONG(DECAL_BLOOD1, DECAL_BLOOD6);
	else if (g_Palette[bloodColor].g > g_Palette[bloodColor].r && g_Palette[bloodColor].g > g_Palette[bloodColor].b)
		return RANDOM_LONG(DECAL_GBLOOD1, DECAL_GBLOOD3);
	else if (g_Palette[bloodColor].b > g_Palette[bloodColor].r && g_Palette[bloodColor].b >= g_Palette[bloodColor].g)
		return RANDOM_LONG(DECAL_BBLOOD1, DECAL_BBLOOD3);

	return RANDOM_LONG(DECAL_YBLOOD1, DECAL_YBLOOD6);
}

//-----------------------------------------------------------------------------
// Purpose: Round to the nearest power-of-two value
// Input  : number - 
//			bits - ex.: 8 bits
// Output : Returns
//-----------------------------------------------------------------------------
/* UNDONE int froundto(float number, uint16 bits)
{
	int lesserinteger = (int)number;//round(number);??
	clamp(bits, 0, sizeof(int)*CHAR_BIT-1);// leave one bit for the sign
	unsigned int mask = UINT_MAX;// fill all FFs
	uint16 bit = 0;
	for (bit = 0; bit < bits; ++bit)
		ClearBits(mask, 1 << bit);

	lesserinteger &= mask;
	int largerineger = lesserinteger
	return ninteger;
}*/

//-----------------------------------------------------------------------------
// Purpose: UNDONE: Parse target names and pick one randomly
// Input  : szNameString - "targetname1|targetname2|targetname3"
// Output : single targetname
//-----------------------------------------------------------------------------
const char *ParseRandomString(const char *szSourceString)
{
	// thread-unsafe static char szTargetName[MAX_ENTITY_STRING_LENGTH]; <-
	return szSourceString;// szTargetName;
}

//-----------------------------------------------------------------------------
// Purpose: \ to /
// Input  : *pathname - 
//-----------------------------------------------------------------------------
void Pathname_Convert(char *pathname)
{
	size_t len = strlen(pathname);
	for (size_t i=0; i < len; ++i)
	{
		if (pathname[i] == PATHSEPARATOR_DOS)
			pathname[i] = PATHSEPARATOR_UNIX;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Extract file name to a new buffer
// Input  : *path - path to a file, "/dir/file.name.ext" // 5, 14, 14-5=9
// Output : buffer - "file.name"
//-----------------------------------------------------------------------------
void UTIL_StripFileName(const char *path, char *buffer, size_t bufferlength)
{
	if (path == NULL || buffer == NULL || bufferlength < 2)
		return;

	size_t len = 0;
	size_t last_name = 0;// start of a directory/file name
	size_t last_dot = 0;
	while (path[len] != '\0')// !EOF
	{
		if (path[len] == PATHSEPARATOR_UNIX || path[len] == PATHSEPARATOR_DOS)
			last_name = len+1;
		else if (path[len] == '.')
			last_dot = len;

		++len;
	}
	if (last_dot == 0)// there was no extension
		last_dot = len-1;

	strncpy(buffer, path+last_name, min(bufferlength, last_dot-last_name));// throw error?
	buffer[min(bufferlength-1, last_dot-last_name)] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to file name and extension, no modifications, same buffer
// Input  : *path - path to a file, "/dir/dir/file.name.ext"
// Output : const char * - pointer to "file.name.ext"
//-----------------------------------------------------------------------------
const char *UTIL_StripFileNameExt(const char *path)
{
	if (path == NULL)
		return NULL;

	size_t len = 0;
	size_t last_name = 0;// start of a directory/file name
	while (path[len] != '\0')// !EOF
	{
		if (path[len] == PATHSEPARATOR_UNIX || path[len] == PATHSEPARATOR_DOS)
			last_name = len+1;

		++len;
	}
	return path + last_name;
	/* slower size_t len = strlen(path);
	do
	{
		--len;
		if (len == 0 || path[len] == '/' || path[len] == '\\')
			break;
	}
	while (len > 0);
	return path + len + 1;*/
}

//-----------------------------------------------------------------------------
// Purpose: Does this file path end with specified extension?
// Input  : *path - absolute/relative path to a file
//			*checkext - ".ext" with dot!
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_FileExtensionIs(const char *path, const char *checkext)
{
	if (path == NULL || checkext == NULL)
		return false;

	size_t len = strlen(path);
	size_t extlen = strlen(checkext);
	if (len < 2 || extlen < 1 || len < extlen)
		return false;

	bool bFail = false;
	do
	{
		--len;
		--extlen;
		if (path[len] != checkext[extlen])
		{
			bFail = true;
			break;
		}
	}
	while (len > 0 && extlen > 0);

	if (bFail == false)
	{
// if there was no dot in *checkext		if (path[len-1] == '.')
			return true;
	}
	return false;
}

/*	char ext[_MAX_EXT];
	_splitpath(path, NULL, NULL, NULL, ext);
	if (_strnicmp(ext, checkext, _MAX_EXT) == 0)// we have to use case-insensitive function
		return true;*/

//-----------------------------------------------------------------------------
// Purpose: Expand game path into system path
// Warning: UNDONE!
// Input  : *relpath - "sprites/mysprite.spr"
//			*fullpath - output for "C:\games\HL\XDM\sprites\mysprite.spr"
//			fullpathmax - maximum length of output string
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_ExpandPathSecure(const char *relpath, char *fullpath, size_t fullpathmax, bool bToGameBaseDir)
{
	if (bToGameBaseDir)// XDM3038c: TODO: revisit?
		strncpy(fullpath, GAME_BASE_DIR, fullpathmax);
	else
	{
#if defined(CLIENT_DLL)
	strncpy(fullpath, gEngfuncs.pfnGetGameDirectory(), fullpathmax);// "XDM/dir/file.ext" does NOT work on client!
	/* does not work for masks! if (COM_EXPAND_FILENAME(relpath, fullpath, fullpathmax) == 0)// this creates full d:\games\hl\xdm\... path in lower case!
		return false;
	else
		return true;*/
#else
	GET_GAME_DIR(fullpath);// "XDM/dir/file.ext" <- this works on server
#endif
	}

	// XDM3038a: security goes here
	size_t i = 0;
	size_t dest_index=0;
	while (fullpath[dest_index] != '\0')// find the end of the string
		++dest_index;

	if (dest_index >= fullpathmax)
		return false;

	// append separator
	fullpath[dest_index] = PATHSEPARATOR;
	++dest_index;
	if (dest_index >= fullpathmax)
		return false;

	fullpath[dest_index] = '\0';
	// ================================
	// WARNING! SECURITY: UNDONE: TODO:
	// We should expand given unsafe string using _fullpath() or canonicalize_file_name() or realpath()
	// and then check strncmp(fullpath, GET_GAME_DIR(), n) so the user won't get outside mod directory
	// This is to be done. Right now we only erase path redirection symbols.
	// ================================

	// append search path
	for (i=0; i<fullpathmax && relpath[i]; ++i)
	{
		// Get rid of all path modifiers
		if (relpath[i] == '.')// warning! this may be extension or a dot inside file name!
		{
			if (i+1<fullpathmax && (relpath[i+1] == '.' || relpath[i+1] == PATHSEPARATOR || relpath[i+1] == PATHSEPARATOR_FOREIGN))// skip '..', './' or '.\'
			{
				++i;// two characters to skip
				continue;
			}
		}
		if (relpath[i] == PATHSEPARATOR_FOREIGN)// use the opportunity to replace non-native path separators
			fullpath[dest_index] = PATHSEPARATOR;
		else
			fullpath[dest_index] = relpath[i];

		++dest_index;
		if (dest_index >= fullpathmax)
			return false;
	}
	fullpath[dest_index] = '\0';
	//conprintf(0, ">>> %s\n", fullpath);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check if FILE exists, not directories!
// Input  : *relpath - "sprites/mysprite.spr"
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_FileExists(const char *relpath)
{
	/* METHOD 1: SYSTEM. Works, but we need to check base dir and .PAKs too! :(
	char fullpath[MAX_PATH];
	if (!UTIL_ExpandPathSecure(relpath, fullpath, MAX_PATH))
	{
		conprintf(0, "--- UTIL_FileExists() failed to expand path! ---\n");
		return false;
	}
	struct _stat buf;
	int result = _stat(fullpath, &buf);
	if (result == 0)
	{
		//conprintf(0, " file %s found, size: %ld\n", relpath, buf.st_size);
		//ctime_s(timestr, 32, &buf.st_mtime);
		return true;
	}*/

	// METHOD 3: COM_FileSize. Does not allocate memory, faster and better than method 2.
	if (pmove)
	{
		if (pmove->COM_FileSize((char *)relpath) > 0)
			return true;
	}
	else// in emergency, fall back to method 2
	{
		// METHOD 2: COM_LOAD_FILE. Works, but this call should be avoided because engine may get overloaded.
#if defined(CLIENT_DLL)
		byte *pFile = COM_LOAD_FILE((char *)relpath, 5, NULL);
#else
		byte *pFile = LOAD_FILE_FOR_ME((char *)relpath, NULL);// HACK BUGBUG: eventually this shit overflows the engine!! (try running overdrive server with 32 players for 5 hours, after 600*32 respawns engine will overflow)
#endif
		if (pFile)
		{
#if defined(CLIENT_DLL)
			COM_FREE_FILE(pFile);
#else
			FREE_FILE(pFile);
#endif
			return true;
		}
	}
	return false;
}

#if !defined (_WIN32)
static const char *g_pattern = '\0';

static bool match(const char *pattern, const char *candidate, int p, int c)
{
	if (pattern[p] == '\0')
	{
		return candidate[c] == '\0';
	}
	else if (pattern[p] == '*')
	{
		for (; candidate[c] != '\0'; c++)
		{
			if (match(pattern, candidate, p+1, c))
				return true;
		}
		return match(pattern, candidate, p+1, c);
	}
	else if (pattern[p] != '?' && pattern[p] != candidate[c])
	{
		return false;
	}
	else
		return match(pattern, candidate, p+1, c+1);
}

static int filter(const struct dirent *ent)
{
	return match(g_pattern, ent->d_name, 0, 0);
}
#endif // !_WIN32

static const int TMP_LEN = 256;

//-----------------------------------------------------------------------------
// Purpose: Restores incorrect fila name case
// Note   : Looks into game directory first, then into base directory.
// Warning: There may be "XDM/maps/MyMap.bsp" and "valve/maps/mymap.bsp": is so, fail!
// Input  : *relpath - "maps/mymap.bsp" - output ONLY NAME "MyMap.bsp"
//			length - input/output buffer length
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_GetTrueFileName(char *relpath, size_t length)
{
	size_t inputlen = strlen(relpath);
	char tmp[TMP_LEN];
	bool bExpanded = false;
	bool bTriedGameDir = false;
	bool bTriedBaseDir = false;

trydirectory:// we need this for two passes: 1 to try expanding directory, 2 to try finding the file
	if (!bExpanded && !bTriedGameDir)
	{
		DBG_PRINTF("UTIL_GetTrueFileName(%s): trying GAME directory...\n", relpath);
		bExpanded = UTIL_ExpandPathSecure(relpath, tmp, TMP_LEN, false);// expand to "XDM/path"
		bTriedGameDir = true;
		// expand may succeed here, but file may actually not exist
	}
	if (!bExpanded && !bTriedBaseDir)
	{
		DBG_PRINTF("UTIL_GetTrueFileName(%s): trying BASE directory...\n", relpath);
		bExpanded = UTIL_ExpandPathSecure(relpath, tmp, TMP_LEN, true);// expand to "valve/path"
		bTriedBaseDir = true;
	}
	if (!bExpanded)
	{
		conprintf(0, "UTIL_GetTrueFileName() failed to expand path \"%s\"!\n", relpath);
		return false;
	}

	//size_t fulllen = strlen(tmp);
#if defined (_WIN32)
	intptr_t hFile = 0;
	_finddata_t fdata;
	if ((hFile = _findfirst(tmp, &fdata)) == -1L)
	{
		bExpanded = false;
		if (bTriedBaseDir)
		{
			conprintf(0, "UTIL_GetTrueFileName(\"%s\"): not found in game or base dir. %d\n", tmp, errno);
			goto notfoundinfs;
		}
		else
			goto trydirectory;
	}
	strcpy(tmp, fdata.name);
	ASSERT(_findnext(hFile, &fdata) != 0);// 0 means MORE, there should never be more files!
	_findclose(hFile);
#else// the horror of linux code
	struct dirent **list;
	int n = scandir(tmp, &list, filter, alphasort);
	if (n < 0)// WTF!??
	{
		bExpanded = false;
		if (bTriedBaseDir)
		{
			conprintf(0, "UTIL_GetTrueFileName(\"%s\"): not found in game or base dir (error code: %s)\n", tmp, strerror(errno));
			goto notfoundinfs;
		}
		else
			goto trydirectory;
	}
	strcpy(tmp, list[0]->d_name);
	free(list);
#endif
	strcpy(relpath, tmp);// just return filename
	return true;

notfoundinfs:// BUGBUG: not found in file system, but may exist inside a PAK file, in which case we have no means of protection :(
	if (UTIL_FileExists(relpath))
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Restores incorrect fila name case
// Input  : *relpath - "maps/test/mymap.bsp" - output "maps/test/MyMap.bsp"
//			length - input/output buffer length
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_FixFileName(char *relpath, size_t length)
{
	size_t inputlen = strlen(relpath);
	char tmp[TMP_LEN];
	strncpy(tmp, relpath, min(length, TMP_LEN));
	if (UTIL_GetTrueFileName(tmp, length))
	{
		// Not we've got correct NAME, but need to revert it back to specified relative level (e.g. "maps/Name.bsp")
		size_t namelen = strlen(tmp);
		size_t diff = inputlen-namelen;
		if (inputlen >= namelen && diff + namelen < length)
		{
			strncpy(relpath+diff, tmp, namelen);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: List files by mask
// Input  : *search - mask ("maps/DM_*.bsp")
// Output : size_t - number of files
//-----------------------------------------------------------------------------
size_t UTIL_ListFiles(const char *search)
{
	if (search == NULL)
		return 0;

	char tmp[TMP_LEN];
	if (search == "")// empty string means "everything"
	{
		tmp[0] = '*';
		tmp[1] = '\0';
	}
	else if (!UTIL_ExpandPathSecure(search, tmp, TMP_LEN, false))
	{
		conprintf(0, "--- UTIL_ListFiles() failed to expand path! ---\n");
		return 0;
	}

	//if (strcmp(search, ".") == 0 || strcmp(search, "..") == 0)// UTIL_ExpandPathSecure() shoul've taken care of this
	//	return 0;

	conprintf(0, "--- Listing of \"%s\" ---\n NAME\t\t\tSIZE\n", tmp);
	size_t count = 0;

#if defined (_WIN32)
	intptr_t hFile = 0;
	_finddata_t fdata;
	if ((hFile = _findfirst(tmp, &fdata)) == -1L)
		conprintf(0, "Nothing found for \"%s\"\n", search);
	else
	{
		do// Find the rest of the files
		{
			conprintf(0, " %-32s %8d B\n", fdata.name, fdata.size);// nice format, but only useful with constant width fonts
			++count;
		}
		while (_findnext(hFile, &fdata) == 0);
		_findclose(hFile);
	}
#else// the horror of linux code
	struct dirent **list;
	char tmp2[TMP_LEN + 2];

	// find last path section
	char *ptmp = (char *)strrchr(search, '/');
	if (!ptmp)
		ptmp = search;
	else
		++ptmp;

	if (!*ptmp)
		ptmp = "*";

	g_pattern = ptmp;
#if defined (CLIENT_DLL)
	snprintf(tmp2, TMP_LEN + 2, "%s",tmp);
#else
	snprintf(tmp2, TMP_LEN + 2, "./%s",tmp);
#endif
	ptmp = strrchr(tmp2, '/');
	if (ptmp)
		*(ptmp + 1) = 0;

	//conprintf(0, "tmp2:  \"%s\"\n", tmp2);
	int n = scandir(tmp2, &list, filter, alphasort);
	//printf("%s\n", tmp);

	if (n < 0)// WTF!??
	{
		conprintf(0, "Cannot list files for \"%s\": %s\n", search, strerror(errno));
		return 0;
	}
	for (int i = 0; i < n; ++i)
	{
		conprintf(0, " %-32s\n", list[i]->d_name);
		free(list[i]);
	}
	free(list);
	count = n;
#endif
	conprintf(0, "--- %u items ---\n", count);
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: Same as fopen, but no need to include gamedir
// Warning: Does not load from PAK files!
// Input  : *name - 'filename.ext'
//			*mode - 'r'
// Output : FILE * - use fclose() to close it!
//-----------------------------------------------------------------------------
FILE *LoadFile(const char *name, const char *mode)
{
//	DBG_PRINTF("LoadFile(\"%s\", \"%s\")\n", name, mode);
	char fullpath[MAX_PATH];
	if (!UTIL_ExpandPathSecure(name, fullpath, MAX_PATH, false))
	{
		if (!UTIL_ExpandPathSecure(name, fullpath, MAX_PATH, true))// XDM3038c
		{
			conprintf(0, "--- LoadFile() failed to expand path! ---\n");
			return NULL;
		}
	}
	FILE *f = fopen(fullpath, mode);
	if (f == NULL)
	{
		conprintf(0, "LoadFile() error: Unable to load file \"%s\"!\n", fullpath);// XDM3038c: show expanded path
		return NULL;
	}
	return f;
}

#define STRINGINLIST_BUFFERLEN		256

//-----------------------------------------------------------------------------
// Purpose: Find given string in a list file
// Input  : *szString - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringInList(const char *szString, const char *szFileName)
{
	if (szString == NULL || szFileName == NULL)
		return false;

	size_t iSearchLen = strlen(szString);// !
	if (iSearchLen == 0 || iSearchLen > STRINGINLIST_BUFFERLEN)// we cannot reliably match against lesser buffer: fail
		return false;

	FILE *pFile = LoadFile(szFileName, "rt");
	if (!pFile)// error message already displayed
		return false;

	char str[STRINGINLIST_BUFFERLEN];
	//unsigned int line = 0;
	while (!feof(pFile))
	{
		if (fgets(str, STRINGINLIST_BUFFERLEN, pFile) == NULL)
			break;
		//line ++;

		if ((str[0] == '/' && str[1] == '/') || str[0] == ';' || str[0] == '#')// skip all kind of comments
			continue;

		//if (_stricmp(str, szString) == 0)//if (_strnicmp(str, szString, STRINGINLIST_BUFFERLEN) == 0)// THIS SHIT DOES NOT WORK because \n screws it up!!
		if ((str[iSearchLen] == '\r' || str[iSearchLen] == '\n') && _strnicmp(str, szString, iSearchLen) == 0)
		{
			return true;
			//bRet = true;
			//goto f_end;
		}
	}
//f_end:
	fclose(pFile);
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse 256 RGB bytes from a binary file
// Input  : *filename - palette.lmp
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_LoadRawPalette(const char *filename)
{
	FILE *pFile = LoadFile(filename, "rb");
	if (pFile == NULL)
		return false;

	// NO! fread(g_Palette, 3, 256, pFile);// Color is RGBA, thus 4 bytes, but we only have RGB in palette
	size_t n;
	byte rgb[3];
	for (size_t i = 0; i<256; ++i)
	{
		n = fread(rgb, 1, 3, pFile);
		if (n < 3)
		{
			conprintf(2, "LoadRawPalette(%s): failed to read RGB data at offset %d!\n", filename, i*3+n);
			return false;
		}
		g_Palette[i].Set3b(rgb);
		// debug conprintf(2, "LoadRawPalette(%s): %08X\n", filename, g_Palette[i].integer);
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Generate random vector: range [-1...+1]
// Output : Vector - valid until next call
//-----------------------------------------------------------------------------
const Vector &RandomVector(void)
{
	static Vector out;
	out.Set(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Generate random vector: range [-halfrange...+halfrange]
// Input  : halfrange
// Output : Vector - valid until next call
//-----------------------------------------------------------------------------
const Vector &RandomVector(const float &fHalfRange)
{
	static Vector out;
	out.Set(RANDOM_FLOAT(-fHalfRange, fHalfRange), RANDOM_FLOAT(-fHalfRange, fHalfRange), RANDOM_FLOAT(-fHalfRange, fHalfRange));
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Generate random vector: range [-halfrange...+halfrange] in each axis
// Input  : halfx, halfy, halfz
// Output : Vector - valid until next call
//-----------------------------------------------------------------------------
const Vector &RandomVector(const float &fHalfX, const float &fHalfY, const float &fHalfZ)
{
	static Vector out;
	out.Set(RANDOM_FLOAT(-fHalfX, fHalfX), RANDOM_FLOAT(-fHalfY, fHalfY), RANDOM_FLOAT(-fHalfZ, fHalfZ));
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Generate random vector: range [-halfrange...+halfrange] in each axis
// Input  : vhalfrange - Vector
// Output : Vector - valid until next call
//-----------------------------------------------------------------------------
const Vector &RandomVector(const Vector &vHalfRange)
{
	static Vector out;
	out.x = RANDOM_FLOAT(-vHalfRange.x, vHalfRange.x);
	out.y = RANDOM_FLOAT(-vHalfRange.y, vHalfRange.y);
	out.z = RANDOM_FLOAT(-vHalfRange.z, vHalfRange.z);
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Generate random vector: range [min...max]
// Input  : min, max
// Output : Vector - valid until next call
//-----------------------------------------------------------------------------
const Vector &RandomVector(const float &fMin, const float &fMax)
{
	static Vector out;
	out.Set(RANDOM_FLOAT(fMin, fMax), RANDOM_FLOAT(fMin, fMax), RANDOM_FLOAT(fMin, fMax));
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Generate random vector: range [min...max] in each axis
// Input  : min, max - Vectors
// Output : Vector - valid until next call
//-----------------------------------------------------------------------------
const Vector &RandomVector(const Vector &vMin, const Vector &vMax)
{
	static Vector out;
	out.x = RANDOM_FLOAT(vMin.x, vMax.x);
	out.y = RANDOM_FLOAT(vMin.y, vMax.y);
	out.z = RANDOM_FLOAT(vMin.z, vMax.z);
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Randomly return one of two values
// Input  : a, b - possible variants. Equal probability.
// Output : int - a or b
//-----------------------------------------------------------------------------
const int &RANDOM_INT2(const int &a, const int &b)
{
	if (RANDOM_LONG(0,1) == 1)
		return b;

	return a;
}

//-----------------------------------------------------------------------------
// Purpose: Search array for specified value
// Input  : *pArray - [] MUST BE TERMINATED WITH 0!
//			&value - value
// Output : int - index in array or -1 if not found
//-----------------------------------------------------------------------------
int Array_FindInt(const int *pArray, const int value)// const int terminator, const int badreturn)
{
	//DBG_PRINTF("Array_FindInt(%d)\n", value);
	if (pArray)
	{
		int index = 0;
		while (pArray[index] != 0)// terminator
		{
			if (pArray[index] == value)
				return index;

			++index;
		}
	}
	return -1;// badreturn
}

//-----------------------------------------------------------------------------
// Purpose: Test iValue to be one of specified "options" and set to iDefault if not.
// Input  : &iValue - value
//			iDefault - value will be reset to this if not equal to any of provided options
//			iCount - number of options:
//			... - options
// Output : bool - returns true if iValue was modified
//-----------------------------------------------------------------------------
bool RestrictInt(int &iValue, const int iDefault, const size_t iOptionsCount, ...)
//#if defined(UNIX)
//	va_dcl
//#endif
{
	//bool bChanged = false;
	va_list marker;
    va_start(marker, iOptionsCount);
	for (size_t i=0; i<iOptionsCount; ++i)
	{
		if (iValue == va_arg(marker, int))// found one
		{
			va_end(marker);
			return false;
		}
	}
	va_end(marker);
	iValue = iDefault;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//			&clampSize - 
// Output : Vector - 
//-----------------------------------------------------------------------------
Vector UTIL_ClampVectorToBox(const Vector &input, const Vector &clampSize)
{
	Vector sourceVector(input);

	if (sourceVector.x > clampSize.x)
		sourceVector.x -= clampSize.x;
	else if (sourceVector.x < -clampSize.x)
		sourceVector.x += clampSize.x;
	else
		sourceVector.x = 0;

	if (sourceVector.y > clampSize.y)
		sourceVector.y -= clampSize.y;
	else if (sourceVector.y < -clampSize.y)
		sourceVector.y += clampSize.y;
	else
		sourceVector.y = 0;
	
	if (sourceVector.z > clampSize.z)
		sourceVector.z -= clampSize.z;
	else if (sourceVector.z < -clampSize.z)
		sourceVector.z += clampSize.z;
	else
		sourceVector.z = 0;

	sourceVector.NormalizeSelf();
	return sourceVector;
}

//-----------------------------------------------------------------------------
// Purpose: Finds nearest box plane point                         o  x__.__x
// Input  : &vecPoint - outside point to check                     \ |     |
//			&vecCenter - center of the box                          '.  c  .
//			&vecVertex - corner of the box (relative coords)        x|__.__|x
// Output : Vector - nearest point
//-----------------------------------------------------------------------------
Vector GetNearestPointOfABoxMinsMaxs(const Vector &vecPoint, const Vector &vecMins, const Vector &vecMaxs)
{
	static Vector points[8];
	// all corners
	points[0] = Vector(vecMins.x, vecMaxs.y, vecMins.z);
	points[1] = Vector(vecMins.x, vecMins.y, vecMins.z);
	points[2] = Vector(vecMaxs.x, vecMaxs.y, vecMins.z);
	points[3] = Vector(vecMaxs.x, vecMins.y, vecMins.z);
	points[4] = Vector(vecMaxs.x, vecMaxs.y, vecMaxs.z);
	points[5] = Vector(vecMaxs.x, vecMins.y, vecMaxs.z);
	points[6] = Vector(vecMins.x, vecMaxs.y, vecMaxs.z);
	points[7] = Vector(vecMins.x, vecMins.y, vecMaxs.z);

	Vector *pvecNearest = NULL;
	vec_t mindist = MAX_ABS_ORIGIN*2;
	vec_t l;
	for (short i=0; i<8; ++i)
	{
		//UTIL_DebugBeam(vecPoint, points[i], 10.0f);
		l = (vecPoint - points[i]).Length();
		if (l < mindist)
		{
			mindist = l;
			pvecNearest = &points[i];
		}
	}
	return *pvecNearest;
}


#if defined(BOXTRACE_EXTRA_ACCURACY)
#define BOXTRACE_POINTS		14
#else
#define BOXTRACE_POINTS		6
#endif

//-----------------------------------------------------------------------------
// Purpose: Finds nearest box plane point                         o  .__x__.
// Input  : &vecPoint - outside point to check                     \ |     |
//			&vecCenter - center of the box                          'x  c  x
//			&vecVertex - corner of the box (relative coords)         |__x__|
// Output : Vector - nearest point
//-----------------------------------------------------------------------------
Vector GetNearestPointOfABox(const Vector &vecPoint, const Vector &vecCenter, const Vector &vecVertex)
{
	static Vector points[BOXTRACE_POINTS];
	// centers of all planes
	points[0] = vecCenter + Vector(vecVertex.x, 0.0f, 0.0f);
	points[1] = vecCenter + Vector(-vecVertex.x, 0.0f, 0.0f);
	points[2] = vecCenter + Vector(0.0f, vecVertex.y, 0.0f);
	points[3] = vecCenter + Vector(0.0f, -vecVertex.y, 0.0f);
	points[4] = vecCenter + Vector(0.0f, 0.0f, vecVertex.z);
	points[5] = vecCenter + Vector(0.0f, 0.0f, -vecVertex.z);
	// box vertexes
#if defined(BOXTRACE_EXTRA_ACCURACY)
	points[6] = vecCenter + Vector(vecVertex.x, vecVertex.y, vecVertex.z);
	points[7] = vecCenter + Vector(-vecVertex.x, vecVertex.y, vecVertex.z);
	points[8] = vecCenter + Vector(vecVertex.x, -vecVertex.y, vecVertex.z);
	points[9] = vecCenter + Vector(-vecVertex.x, -vecVertex.y, vecVertex.z);
	points[10] = vecCenter + Vector(vecVertex.x, vecVertex.y, -vecVertex.z);
	points[11] = vecCenter + Vector(-vecVertex.x, vecVertex.y, -vecVertex.z);
	points[12] = vecCenter + Vector(vecVertex.x, -vecVertex.y, -vecVertex.z);
	points[13] = vecCenter + Vector(-vecVertex.x, -vecVertex.y, -vecVertex.z);
#endif

	Vector *pvecNearest = NULL;
	vec_t mindist = MAX_ABS_ORIGIN*2;
	vec_t l;
	for (short i=0; i<BOXTRACE_POINTS; ++i)
	{
		//UTIL_DebugBeam(vecPoint, points[i], 10.0f);
		l = (vecPoint - points[i]).Length();
		if (l < mindist)
		{
			mindist = l;
			pvecNearest = &points[i];
		}
	}
	return *pvecNearest;
}

//-----------------------------------------------------------------------------
// Purpose: Is the point inside specified cone?
// Input  : &vecPoint - point to check
//			&vPeak - peak of the cone
//			&vBase - center point of the base
//			baseradius - maximal radius of the cone
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool PointInCone(const Vector &vPoint, const Vector &vPeak, const Vector &vBase, const float &baseradius)
{
/*   o  <- peak
    /|\
   / |_\o <- point
  /  |  \
 o___|___o  <- base */
	// 1) check that projection of the point onto height vector fits into height (1D)
	// 2) check that distance from the point to the height vector fits into current radius (1D)
	/* WORKS!
	Vector vConeAxis = vBase - vPeak;
	Vector vDotDirFromPeak = vPoint - vPeak;
	float fAngCos = AngleCosBetweenVectors(vConeAxis, vDotDirFromPeak);
	float fProjectionDistFromPeak = vDotDirFromPeak.Length()*fAngCos;
	vec_t fHeight = vConeAxis.Length();
	conprintf(1, "PointInCone(r = %g): fAngCos = %g, fProjectionDistFromPeak = %g, fHeight = %g\n", baseradius, fAngCos, fProjectionDistFromPeak, fHeight);
	if (fProjectionDistFromPeak > 0.0f && fProjectionDistFromPeak < fHeight)// projection offset does not exceed cone height/length
	{
		//float fDotDistFromAxis = fProjectionDistFromPeak * tan(fAng);
		float fDotDistFromAxis = fProjectionDistFromPeak * sqrt(1.0f/(fAngCos*fAngCos) - 1.0f);
		//vec_t fDotDistFromAxis = vDotDirFromPeak.Length() * fAngSin;
		//float fAng2Cos = AngleCosBetweenVectors(vConeAxis, vDotProjection);
		//vec_t fDotDistFromAxis = vDotDirFromPeak.Length() * fAng2Cos;
		conprintf(1, "PointInCone(r = %g): fDotDistFromAxis = %g, fCurrentRadius = %g\n", baseradius, fDotDistFromAxis, (fProjectionDistFromPeak/fHeight)*baseradius);
		if (fDotDistFromAxis <= (fProjectionDistFromPeak/fHeight)*baseradius)
			return true;
	}*/
	// OPTIMIZED VERSION
	Vector vConeAxis(vBase);
	vConeAxis -= vPeak;
	Vector vDotDirFromPeak(vPoint);
	vDotDirFromPeak -= vPeak;
	float fAngCos = AngleCosBetweenVectors(vConeAxis, vDotDirFromPeak);
	vec_t fProjectionDistFromPeak = vDotDirFromPeak.Length()*fAngCos;
	vec_t fHeight = vConeAxis.Length();
	if (fProjectionDistFromPeak > 0.0f && fProjectionDistFromPeak < fHeight)// projection offset does not exceed cone height/length
	{
		if (fProjectionDistFromPeak * sqrt(1.0f/(fAngCos*fAngCos) - 1.0f) <= (fProjectionDistFromPeak/fHeight)*baseradius)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Does string start with another string? (fast, better than strcmp)
// Input  : string - where to search
//			substring - what to search for
// Output : size_t - length of substring or 0 on failure
//-----------------------------------------------------------------------------
size_t strbegin(const char *string, const char *substring)
{
	if (string && *string && substring)
	{
		size_t i = 0;
		do
		{
			if (string[i] != substring[i])
			{
				if (substring[i] == '\0')// substring ended, success
					return i;
				else
					return 0;// chars differ, fail
			}
			++i;
		}
		while (string[i] != '\0');

		if (substring[i] == '\0')// both strings are equal
			return i;
	}
	return 0;// string ended first, fail
}

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32(uchar32 uVal)
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return (uVal < 0x110000u) && ((uVal - 0x00D800u) > 0x7FFu) && ((uVal & 0xFFFFu) < 0xFFFEu) && ((uVal - 0x00FDD0u) > 0x1Fu);
}

//-----------------------------------------------------------------------------
// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
//-----------------------------------------------------------------------------
uint16 Q_UTF8ToUChar32(const char *pUTF8_, uchar32 &uValueOut, bool &bErrorOut)
{
	const uint8 *pUTF8 = (const uint8 *)pUTF8_;
	uint16 nBytes = 1;
	uint32 uValue = pUTF8[0];
	uint32 uMinValue = 0;

	// 0....... single byte
	if (uValue < 0x80)
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if ((uValue - 0xC0u) > 0x37u || (pUTF8[1] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0xC0 << 6) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if (!(uValue & (0x20 << 6)))
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if ((pUTF8[2] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x20 << 12) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

	// 1110.... three-byte lead byte
	if (!(uValue & (0x10 << 12)))
		goto decodeFinishedMaybeCESU8;

	// Expecting a four-byte sequence, longest permissible in UTF-8
	if ((pUTF8[3] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x10 << 18) + pUTF8[3] - 0x80;
	nBytes = 4;
	uMinValue = 0x10000;

	// 11110... four-byte lead byte. fall through to finished.
decodeFinished:
	if (uValue >= uMinValue && Q_IsValidUChar32(uValue))
	{
decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;

decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if ((uValue - 0xD800u) < 0x400u && pUTF8[3] == 0xED && (uint8)(pUTF8[4] - 0xB0) < 0x10 && (pUTF8[5] & 0xC0) == 0x80)
	{
		uValue = 0x10000 + ((uValue - 0xD800u) << 10) + ((uint8)(pUTF8[4] - 0xB0) << 6) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-8 string contains invalid sequences.
// Input  : *pUTF8 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate(const char *pUTF8)
{
	bool bError = false;
	while (*pUTF8)
	{
		uchar32 uVal;
		// Our UTF-8 decoder silently fixes up 6-byte CESU-8 (improperly re-encoded UTF-16) sequences.
		// However, these are technically not valid UTF-8. So if we eat 6 bytes at once, it's an error.
		uint16 nCharSize = Q_UTF8ToUChar32(pUTF8, uVal, bError);
		if (bError || nCharSize == 6)
			return false;

		pUTF8 += nCharSize;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Another HL hack
// Input  : *pMemFile - 
//			fileSize - 
//			&filePos - 
//			*pBuffer - 
//			bufferSize - 
// Output : char
//-----------------------------------------------------------------------------
char *memfgets(byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize)
{
	if (!pMemFile || !pBuffer)
		return NULL;

	if (filePos >= fileSize)
		return NULL;

	int i = filePos;
	int last = fileSize;

	// fgets always NULL-terminates, so only read bufferSize-1 characters
	if (last - filePos > (bufferSize-1))
		last = filePos + (bufferSize-1);

	int size;
	bool stop = false;
	while (i < last && !stop)
	{
		if (pMemFile[i] == '\n')// Stop at the next newline (inclusive) or end of buffer
			stop = true;
		++i;
	}

	if (i != filePos)// If we actually advanced the pointer, copy it over
	{
		size = i - filePos;// We read in size bytes
		memcpy(pBuffer, pMemFile + filePos, sizeof(byte)*size);
		if (size < bufferSize)// If the buffer isn't full, terminate (this is always true)
			pBuffer[size] = 0;

		filePos = i;// Update file pointer
		return pBuffer;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Function missing from CRT: convert all characters to upper case.
// Input  : _Str - a null-terminated string
// Output : char * - _Str
//-----------------------------------------------------------------------------
char *X_strupr(char *_Str)
{
	while (*_Str != '\0')
	{
		*_Str = toupper(*_Str);
		++_Str;
	}
	return _Str;
}

const char g_ParseSingleCharacterTokens[] = "{}()',";

//-----------------------------------------------------------------------------
// Purpose: Parse a token out of a string
// UNDONE: returns only "//" with bParseComments
// Input  : pData - char * stream
//			pToken - output buffer pointer
//			bParseComments - include or skip comments
//			pSingleCharacterTokens - optional custom single-character tokens, like "{}()',"
// Output : char * - pointer inside pData where parsing stopped
//-----------------------------------------------------------------------------
char *COM_Parse(char *pData, char *pToken, bool bParseComments, const char *pSingleCharacterTokens)
{
	if (pData == NULL || pToken == NULL)
		return pData;

	if (pSingleCharacterTokens == NULL)
		pSingleCharacterTokens = g_ParseSingleCharacterTokens;

	size_t len = 0;
	char c;
	pToken[0] = 0;
// skip whitespace
skipwhite:
	while ((c = *pData) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		++pData;
	}

// skip // comments
	if (!bParseComments && c == '/' && pData[1] == '/')
	{
		while (*pData && *pData != '\n')
			++pData;
		goto skipwhite;
	}

// XDM3035c: skip /* */ comments
	/* UNDONE: TESTME: NEEDS TO BE TESTED!
	if (!bParseComments &&  c=='/' && pData[1] == '*')
	{
		while (*pData)
		{
			if (*pData == '*')
			{
				++pData;
				if (*pData == 0 || *pData == '/')
					break;
			}
			++pData;
		}
		goto skipwhite;
	}*/

	if (c == '\\')// XDM3038c: detect escape characters, replace '\"' with '"'
	{
		if (*(pData+1) == '\"')// quotes detected
			c = *pData++;// skip the backslash
	}

// handle quoted strings specially
	if (c == '\"')
	{
		++pData;
		while (1)
		{
			c = *pData++;
			if (c=='\"' || !c)
			{
				pToken[len] = 0;
				return pData;
			}
			pToken[len] = c;
			++len;
		}
	}

// parse single characters
	if (strchr(pSingleCharacterTokens, c))//if (c=='{' || c=='}' || c==')' || c=='(' || c=='\'' || c==',')
	{
		pToken[len] = c;
		++len;
		pToken[len] = 0;
		return pData+1;
	}

// parse a regular word
	do
	{
		pToken[len] = c;
		++pData;
		++len;
		c = *pData;
		if (strchr(pSingleCharacterTokens, c))//if (c=='{' || c=='}' || c==')' || c=='(' || c=='\'' || c==',')
			break;
	} while (c>32);

	pToken[len] = 0;
	return pData;
}

static char com_token[1536];

//-----------------------------------------------------------------------------
// Purpose: Get current/last token
// Output : char * - temporary buffer, valid until next COM_ call.
//-----------------------------------------------------------------------------
char *COM_Token(void)
{
	return com_token;
}

//-----------------------------------------------------------------------------
// Purpose: Parse a token out of a string (global buffer version)
// Input  : pData - 
// Output : char * - pointer inside pData where parsing stopped
//-----------------------------------------------------------------------------
char *COM_Parse(char *pData)
{
	return COM_Parse(pData, com_token, false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buffer - 
// Output : int 1 if additional data is waiting to be processed on this line
//-----------------------------------------------------------------------------
int COM_TokenWaiting(char *pBuffer)
{
	char *p = pBuffer;
	while (*p && *p!='\n')
	{
		if (!isspace(*p) || isalnum(*p))
			return 1;

		++p;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Generate config file by cvars in list (plain string-by-string fmt)
// Input  : *listfilename - profile.lst
//			*configfilename - profile.cfg
// Output : int - number of cvars written, 0 = failure
//-----------------------------------------------------------------------------
uint32 CONFIG_GenerateFromList(const char *listfilename, const char *configfilename)
{
	DBG_PRINTF("CONFIG_GenerateFromList(\"%s\", \"%s\")\n", listfilename, configfilename);
	FILE *pListFile = LoadFile(listfilename, "rt");
	if (pListFile == NULL)
	{
		conprintf(0, "CONFIG_GenerateFromList() error: Unable to load list file \"%s\"!\n", listfilename);
		return 0;
	}

	FILE *pConfigFile = LoadFile(configfilename, "wt");
	if (pConfigFile == NULL)
	{
		conprintf(0, "CONFIG_GenerateFromList() error: Unable to load config file \"%s\" for writing!\n", configfilename);
		fclose(pListFile);// don't forget to close previous file
		return 0;
	}

	//fseek(pConfigFile, 0L, SEEK_SET);
#ifdef _WIN32
	_tzset();
#endif
	//char szBufDate[16];
	//char szBufTime[16];
	char szCurrentString[MAX_PATH];
	//_strdate(szBufDate);
	//_strtime(szBufTime);
	//fprintf(pConfigFile, "// This file was generated %s @ %s from list \"%s\"\n// Format:\n// <variable> <current value>", szBufDate, szBufTime, listfilename);
	time_t crt_time;
	time(&crt_time);
	strftime(szCurrentString, MAX_PATH, "%Y%m%d @ %H%M", localtime(&crt_time));
	fprintf(pConfigFile, "// This file was generated %s from list \"%s\"\n// Format:\n// <variable> <current value>\n", szCurrentString , listfilename);
	uint32 iRet = 0;
	while(!feof(pListFile))
	{
		if (fgets(szCurrentString, MAX_PATH, pListFile) == NULL)
			break;

		if (szCurrentString[0] == '/' && szCurrentString[1] == '/')
		{
			fprintf(pConfigFile, "%s\n", szCurrentString);
			continue;// write original comment lines
		}
		if (szCurrentString[0] == '\0')
		{
			fprintf(pConfigFile, "\n");
			continue;// write original blank lines
		}
		fprintf(pConfigFile, "%s \"%s\"\n", szCurrentString, CVAR_GET_STRING(szCurrentString));
		++iRet;
	}
	fclose(pListFile);
	fclose(pConfigFile);
	conprintf(0, "CONFIG_GenerateFromList(): Configuration file saved: \"%s\"\n", configfilename);
	return iRet;
}

//-----------------------------------------------------------------------------
// "Valve script" parsing routines
//-----------------------------------------------------------------------------
#define SCRIPT_VERSION				1.0f
#if defined(CLIENT_DLL)
#define SCRIPT_DESCRIPTION_TYPE		"INFO_OPTIONS"
#else
#define SCRIPT_DESCRIPTION_TYPE		"SERVER_OPTIONS"
#endif

#define SCRIPT_DESCRIPTION_KEYNAME	"DESCRIPTION"
#define SCRIPT_VERSION_KEYNAME		"VERSION"

enum scriptentrytype_e
{
	SCRTYPE_UNKNOWN = 0,
	SCRTYPE_BOOL,
	SCRTYPE_NUMBER,
	SCRTYPE_LIST,
	SCRTYPE_STRING
};

typedef struct scriptentrytypedesc_s
{
	uint32 type;// scriptentrytype_e - explicitly define field size
	char szDescription[32];
} scriptentrytypedesc_t;

scriptentrytypedesc_t scriptentrytypes[] =
{
	{ SCRTYPE_BOOL,		"BOOL" }, 
	{ SCRTYPE_NUMBER,	"NUMBER" }, 
	{ SCRTYPE_LIST,		"LIST" }, 
	{ SCRTYPE_STRING,	"STRING" }
};

//-----------------------------------------------------------------------------
// Purpose: Recognize block type by its text description
// Input  : *pszType - "LIST"
// Output : scriptentrytype_e
//-----------------------------------------------------------------------------
uint32 SCRIPT_ParseEntryType(const char *pszType)
{
	size_t nTypes = ARRAYSIZE(scriptentrytypes);
	for (size_t i = 0; i < nTypes; ++i)
	{
		if (_stricmp(scriptentrytypes[i].szDescription, pszType) == 0)
			return scriptentrytypes[i].type;
	}
	return SCRTYPE_UNKNOWN;
}

//-----------------------------------------------------------------------------
// Purpose: Parse the key-value entry from a script
// Input  : *pBuffer - file buffer
//			*pName - output pointer
//			*pValue - output pointer
//			*pDescription - output pointer
//			&handled - output 1 = success, 0 = failure
// Output : char * - remaining file buffer
//-----------------------------------------------------------------------------
char *CONFIG_ParseScriptEntry(char *pBuffer, char *pName, char *pValue, char *pDescription, byte &handled)
{
	//pOutput->fHandled = false;
	handled = 0;
	pBuffer = COM_Parse(pBuffer);
	char *pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	//pOutput->szKeyName = pToken;// pointer must last outside
	strcpy(pName, pToken);

	// Parse the {
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	if (strcmp(pToken, "{") != 0)
	{
		conprintf(0, "CONFIG_ParseScriptEntry() start error: expecting \"{\", got \"%s\"!\n", pToken);
		return pBuffer;// NULL?
	}

	// Parse the Prompt
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	//pOutput->szClassName = pToken;
	strcpy(pDescription, pToken);

	// Parse the next {
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	if (strcmp(pToken, "{") != 0)
	{
		conprintf(0, "CONFIG_ParseScriptEntry() error: expecting \"{\", got \"%s\"!\n", pToken);
		return pBuffer;// NULL?
	}

	// Now parse the type:
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	uint32 iType = SCRIPT_ParseEntryType(pToken);
	if (iType == SCRTYPE_UNKNOWN)
	{
		conprintf(0, "CONFIG_ParseScriptEntry() error: type \"%s\" is unknown!\n", pToken);
		return NULL;
	}

	switch (iType)
	{
	case SCRTYPE_BOOL:
		// Parse the next {
		pBuffer = COM_Parse(pBuffer);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			return pBuffer;

		if (strcmp(pToken, "}") != 0)
		{
			conprintf(0, "CONFIG_ParseScriptEntry() error parsing BOOL: expecting \"}\", got \"%s\"!\n", pToken);
			return NULL;
		}
		break;
	case SCRTYPE_NUMBER:
		// parse Min
		pBuffer = COM_Parse(pBuffer);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			return pBuffer;

		//fMin = atof(pToken);
		// parse Max
		pBuffer = COM_Parse(pBuffer);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			return pBuffer;

		//fMax = atof(pToken);
		// parse next {
		pBuffer = COM_Parse(pBuffer);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			return pBuffer;

		if (strcmp(pToken, "}") != 0)
		{
			conprintf(0, "CONFIG_ParseScriptEntry() error parsing NUMBER: expecting \"}\", got \"%s\"!\n", pToken);
			return NULL;
		}
		break;
	case SCRTYPE_STRING:
		// Parse the next {
		pBuffer = COM_Parse(pBuffer);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			return pBuffer;

		if (strcmp(pToken, "}") != 0)
		{
			conprintf(0, "CONFIG_ParseScriptEntry() error parsing STRING: expecting \"}\", got \"%s\"!\n", pToken);
			return NULL;
		}
		break;
	case SCRTYPE_LIST:
		while (pBuffer)// parse items until '}'
		{
			// Parse the next {
			pBuffer = COM_Parse(pBuffer);
			pToken = COM_Token();
			if (strlen(pToken) == 0)
				return pBuffer;

			if (strcmp(pToken, "}") == 0)
				break;

			// Parse the value
			pBuffer = COM_Parse(pBuffer);
			pToken = COM_Token();
			if (strlen(pToken) == 0)
				return pBuffer;

			// TODO: here's the good chance to instert this as an entry to a list, menu or smth.
		}
		break;
	}

	// Read the default value

	// Parse the {
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	if (strcmp(pToken, "{") != 0)
	{
		conprintf(0, "CONFIG_ParseScriptEntry() value error: expecting \"{\", got \"%s\"!\n", pToken);
		return NULL;
	}

	// Parse the default
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	//if (strlen(pToken) == 0)
	//	return pBuffer;

	// Set the values
	//pOutput->szValue = pToken;// pointer lasts outside
	strcpy(pValue, pToken);
	//fdefValue = (float)atof(pToken);

	// Parse the }
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	if (strcmp(pToken, "}") != 0)
	{
		conprintf(0, "CONFIG_ParseScriptEntry() value error: expecting \"}\", got \"%s\"!\n", pToken);
		return NULL;
	}

	// Parse the final }
	pBuffer = COM_Parse(pBuffer);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		return pBuffer;

	if (_stricmp(pToken, "SetInfo") == 0)
	{
		//bSetInfo = true;
		// Parse the final }
		pBuffer = COM_Parse(pBuffer);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			return pBuffer;
	}

	if (strcmp(pToken, "}") != 0)
	{
		conprintf(0, "CONFIG_ParseScriptEntry() value error: expecting \"}\", got \"%s\"!\n", pToken);
		return pBuffer;
	}
	handled = 1;
	//pOutput->fHandled = true;
	return pBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: Generate config file by cvars in "script" (valve {{}} fmt)
// Input  : *templatefilename - settings.scr
//			*configfilename - settings.cfg
//			usecurrentvalues - use CVAR_GET_STRING values (NOTE: they're often out of date or bad) instead of those in the template
// Output : uint32 - number of cvars written, 0 = failure
//-----------------------------------------------------------------------------
uint32 CONFIG_GenerateFromTemplate(const char *templatefilename, const char *configfilename, bool usecurrentvalues)
{
	DBG_PRINTF("CONFIG_GenerateFromTemplate(\"%s\", \"%s\")\n", templatefilename, configfilename);
	uint32 iRet = 0;
	float fVersion;
	int iFileLength = 0;
	char *pData;
	char *pToken;
	char *aFileStart = NULL;
	FILE *pConfigFile = LoadFile(configfilename, "wt");
	if (pConfigFile == NULL)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: unable to open config file \"%s\" for writing!\n", templatefilename);
		goto finish;
	}
#if defined(CLIENT_DLL)
	pData = (char *)COM_LOAD_FILE((char *)templatefilename, 5, &iFileLength);
#else
	pData = (char *)LOAD_FILE_FOR_ME((char *)templatefilename, &iFileLength);
#endif
	aFileStart = pData;
	if (pData == NULL || iFileLength == 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: unable to load template file \"%s\"!\n", templatefilename);
		goto finish;
	}

	// Get the first token
	pData = COM_Parse(pData);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		goto finish;

	// Read VERSION #
	if (_stricmp(pToken, SCRIPT_VERSION_KEYNAME) != 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: expecting \"%s\", got \"%s\" in \"%s\"\n", SCRIPT_VERSION_KEYNAME, pToken, templatefilename);
		goto finish;
	}

	// Parse in the version number (float)
	// Get the first token
	pData = COM_Parse(pData);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: expecting version number in \"%s\"\n", templatefilename);
		goto finish;
	}

	fVersion = (float)atof(pToken);
	if (fVersion != SCRIPT_VERSION)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: version mismatch: expecting \"%g\", got \"%g\" in \"%s\"\n", SCRIPT_VERSION, fVersion, templatefilename);
		goto finish;
	}

	// Get the "DESCRIPTION"
	pData = COM_Parse(pData);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		goto finish;

	// Read DESCRIPTION
	if (_stricmp(pToken, SCRIPT_DESCRIPTION_KEYNAME) != 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: expecting \"%s\", got \"%s\" in \"%s\"\n", SCRIPT_DESCRIPTION_KEYNAME, pToken, templatefilename);
		goto finish;
	}

	// Parse in the description type
	pData = COM_Parse(pData);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: expecting \"%s\"!\n", SCRIPT_DESCRIPTION_TYPE);
		goto finish;
	}

	if (_stricmp(pToken, SCRIPT_DESCRIPTION_TYPE) != 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: expecting \"%s\", got \"%s\" in \"%s\"\n", SCRIPT_DESCRIPTION_TYPE, pToken, templatefilename);
		goto finish;
	}

	// Parse the {
	pData = COM_Parse(pData);
	pToken = COM_Token();
	if (strlen(pToken) == 0)
		goto finish;

	if (strcmp(pToken, "{") != 0)
	{
		conprintf(0, "CONFIG_GenerateFromTemplate() error: expecting \"{\", got \"%s\" in \"%s\"\n", pToken, templatefilename);
		goto finish;
	}

	//fseek(pConfigFile, 0L, SEEK_SET);
#if defined (_WIN32)
	_tzset();
#endif
	//char szBufDate[16];
	//char szBufTime[16];
	//_strdate(szBufDate);
	//_strtime(szBufTime);
	//fprintf(pConfigFile, "// This file was generated %s @ %s from template \"%s\"\n// Format:\n// <description> <value from template>\n// <variable> <current value>", szBufDate, szBufTime, templatefilename);

	char szTimeStr[32];
	time_t crt_time;
	time(&crt_time);
	strftime(szTimeStr, 32, "%Y%m%d @ %H%M", localtime(&crt_time));
	fprintf(pConfigFile, "// This file was generated %s from template \"%s\"\n", szTimeStr, templatefilename);
	if (usecurrentvalues)
		fprintf(pConfigFile, "// Format:\n// <description> <value from template>\n// <variable> <current value>\n");
	else
		fprintf(pConfigFile, "// Format:\n// <description>\n// <variable> <value from template>\n");

	char *pStart;
	char szCVarName[64];
	char szCVarValue[192];
	char szCVarDescription[256];
	const char *pCurrentValueString;
	byte bCVarHandled;
	while (pData)
	{
		pStart = pData;// remember start of the block
		pData = COM_Parse(pData);
		pToken = COM_Token();
		if (strlen(pToken) == 0)
			goto finish;

		if (_stricmp(pToken, "}") == 0)// EOF
			break;

		// Create a new entry
		pData = CONFIG_ParseScriptEntry(pStart, szCVarName, szCVarValue, szCVarDescription, bCVarHandled);
		// Get value and write it to the config file
		if (bCVarHandled)//KVD.fHandled)
		{
			if (usecurrentvalues)
			{
				pCurrentValueString = CVAR_GET_STRING(szCVarName);// XDM3038c: there are some cvars with no string, but with value (e.g. maxplayers)
				if (pCurrentValueString && pCurrentValueString[0] != '\0')
					fprintf(pConfigFile, "// %s (%s)\n%s \"%s\"\n", szCVarDescription, szCVarValue, szCVarName, pCurrentValueString);// XDM3037a: comments cannot be on the same line as data, moved to previous line
				else
					fprintf(pConfigFile, "// %s (%s)\n%s \"%g\"\n", szCVarDescription, szCVarValue, szCVarName, CVAR_GET_FLOAT(szCVarName));
			}
			else
				fprintf(pConfigFile, "// %s\n%s \"%s\"\n", szCVarDescription, szCVarName, szCVarValue);

			++iRet;
		}
		else// there was some error
		{
			conprintf(0, "Some cvar(s) left unread in \"%s\"!\n", templatefilename);
			goto finish;// since we cannot possibly find the end of this block or anythin else now
		}
#if defined (_DEBUG_INFILOOPS)
		if (ASSERT(ret < 4096) == false)
		{
			conprintf(0, "CONFIG_GenerateFromTemplate(): possible endless loop detected in \"%s\"!\n", templatefilename);
			break;
		}
#endif
	}

finish:
	if (aFileStart)
#if defined(CLIENT_DLL)
		COM_FREE_FILE(aFileStart);
#else
		FREE_FILE(aFileStart);
#endif
	if (pConfigFile)
		fclose(pConfigFile);// don't forget to close previous file

	return iRet;
}

// Indexes must correspond to user_rights_e
const char *g_UserRightsNames[USER_RIGHTS_COUNT+1] =
{
	"",
	"player",
	"mod",
	"admin",
	"dev",
	NULL// terminate
};

//-----------------------------------------------------------------------------
// Purpose: Get "user right" index
// Input  : *pName - "admin"
// Output : user_rights_e index
//-----------------------------------------------------------------------------
uint32 UserRightByName(const char *pName)
{
	uint32 iRightIndex;
	//while (g_UserRightsNames[iRightIndex] != NULL)
	for (iRightIndex = USER_RIGHTS_NONE+1; iRightIndex < USER_RIGHTS_COUNT; ++iRightIndex)
	{
		if (strcmp(g_UserRightsNames[iRightIndex], pName) == 0)
			return iRightIndex;

		//++iRightIndex;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Draw a simple debug beam (only in developer mode)
// Input  : vecSrc - start
//			vecEnd - end
//			life - in seconds
//			r,g,b - colors 0-255
//-----------------------------------------------------------------------------
void UTIL_DebugBeam(const Vector &vecSrc, const Vector &vecEnd, const float &life, byte r, byte g, byte b)
{
//#if defined (_DEBUG)
	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)
	{
#if defined(CLIENT_DLL)
		gEngfuncs.pEfxAPI->R_BeamPoints(vecSrc, vecEnd, g_iModelIndexLaser, life, 1.0, 0.0, 1.0f, 0.0f, 0, 20, r/255.0f,g/255.0f,b/255.0f);
#else
		//BeamEffect(TE_BEAMPOINTS, vecSrc, vecEnd, g_iModelIndexLaser, 0,20, (int)(life*10.0f), 16,0, Vector(r,g,b),255, 0);
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSrc);
			WRITE_BYTE(TE_BEAMPOINTS);
			WRITE_COORD(vecSrc.x);
			WRITE_COORD(vecSrc.y);
			WRITE_COORD(vecSrc.z);
			WRITE_COORD(vecEnd.x);
			WRITE_COORD(vecEnd.y);
			WRITE_COORD(vecEnd.z);
			WRITE_SHORT(g_iModelIndexLaser);
			WRITE_BYTE(0);
			WRITE_BYTE(20);
			WRITE_BYTE(min((int)(life*10.0f), 255));
			WRITE_BYTE(10);// width in 0.1's
			WRITE_BYTE(0);
			WRITE_BYTE(r);
			WRITE_BYTE(g);
			WRITE_BYTE(b);
			WRITE_BYTE(255);
			WRITE_BYTE(0);
		MESSAGE_END();
#endif
	}
//#endif // _DEBUG
}

//-----------------------------------------------------------------------------
// Purpose: Draw debug box with beams (only in developer mode)
// Input  : vecSrc - start
//			vecEnd - end
//			life - in seconds
//			r,g,b - colors 0-255
//-----------------------------------------------------------------------------
void UTIL_DebugBox(const Vector &vecMins, const Vector &vecMaxs, const float &life, byte r, byte g, byte b)
{
//#if defined (_DEBUG)
	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)
	{
		uint i;
		Vector vPoints[8];// sequential points
		for (i = 0; i < 8; ++i)// 0000 0001 0010 0011 0100 0101 0110 0111
		{
			vPoints[i].x = (i & 1) ? vecMaxs.x : vecMins.x;
			vPoints[i].y = (i & 2) ? vecMaxs.y : vecMins.y;
			vPoints[i].z = (i & 4) ? vecMaxs.z : vecMins.z;
		}
		// TEST for (i = 0; i < 7; ++i)
		//	UTIL_DebugBeam(vPoints[i],vPoints[i+1], life, r,g,b);
		int shift;
		for (i = 0; i < 8; ++i)//xx00 xx01 xx10 xx11
		{
			shift = (FBitSet(i, Bit(0)) == FBitSet(i, Bit(1)))?1:2;// 1 2 2 1
			if (FBitSet(i, Bit(1))) shift = -shift;
			UTIL_DebugBeam(vPoints[i],vPoints[i+shift], life, r,g,b);
		}
		for (i = 0; i < 4; ++i)
			UTIL_DebugBeam(vPoints[i],vPoints[i+4], life, r,g,b);// vertical bars*/
	}
//#endif // _DEBUG
}

//-----------------------------------------------------------------------------
// Purpose: Draw a simple debug dot (only in developer mode)
// Input  : vecPos - position
//			life - in seconds
//			r,g,b - colors 0-255
//-----------------------------------------------------------------------------
void UTIL_DebugPoint(const Vector &vecPos, const float &life, byte r, byte g, byte b)
{
//#if defined (_DEBUG)
	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)
	{
#if defined(CLIENT_DLL)
		TEMPENTITY *pSprite = gEngfuncs.pEfxAPI->R_TempSprite(vecPos, g_vecZero, 0.1f, g_iModelIndexAnimglow01, kRenderGlow, kRenderFxNoDissipation, 1.0f, life, FTENT_FADEOUT);
		if (pSprite)
		{
			pSprite->entity.curstate.rendercolor.r = r;
			pSprite->entity.curstate.rendercolor.g = g;
			pSprite->entity.curstate.rendercolor.b = b;
		}
#else
		GlowSprite(vecPos, g_iModelIndexAnimglow01, (int)(life*10.0f), 1, 10);// no color there :(
#endif
	}
//#endif // _DEBUG
}

//-----------------------------------------------------------------------------
// Purpose: Draw line forward, and two crossing lines displaying face parallel to viewport
// Input  : origin - 
//			angles - 
//			life - in seconds
//			radius - 
//-----------------------------------------------------------------------------
void UTIL_DebugAngles(const Vector &origin, const Vector &angles, const float &life, const float &radius)
{
//#if defined (_DEBUG)
	if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)
	{
		Vector vEnd, vForward, vRight, vUp;
		AngleVectors(angles, vForward, vRight, vUp);
		vEnd = vForward; vEnd *= radius; vEnd += origin;
//#if defined(CLIENT_DLL)
		UTIL_DebugBeam(origin, vEnd, life, 255,0,0);
		UTIL_DebugBeam(vEnd-vRight*radius, vEnd+vRight*radius, life, 0,255,0);
		UTIL_DebugBeam(vEnd-vUp*radius, vEnd+vUp*radius, life, 0,0,255);
/*#else
		UTIL_ShowLine(origin, vEnd, life, 255,0,0);
		UTIL_ShowLine(vEnd-vRight*radius, vEnd+vRight*radius, life, 0,255,0);
		UTIL_ShowLine(vEnd-vUp*radius, vEnd+vUp*radius, life, 0,0,255);
#endif*/
	}
//#endif // _DEBUG
}




// The absolute default palette. Normally, will be overwritten from a file.
DLL_GLOBAL Color g_Palette[256] =
{
0xFF000000u,
0xFF0F0F0Fu,
0xFF1F1F1Fu,
0xFF2F2F2Fu,
0xFF3F3F3Fu,
0xFF4B4B4Bu,
0xFF5B5B5Bu,
0xFF6B6B6Bu,
0xFF7B7B7Bu,
0xFF8B8B8Bu,
0xFF9B9B9Bu,
0xFFABABABu,
0xFFBBBBBBu,
0xFFCBCBCBu,
0xFFDBDBDBu,
0xFFEBEBEBu,
0xFF070A0Fu,
0xFF0B0F17u,
0xFF0B171Fu,
0xFF0F1B27u,
0xFF13232Fu,
0xFF172B37u,
0xFF172F3Fu,
0xFF1B374Bu,
0xFF1B3B53u,
0xFF1F435Bu,
0xFF1F4B63u,
0xFF1F536Bu,
0xFF1F5773u,
0xFF235F7Bu,
0xFF236783u,
0xFF236F8Fu,
0xFF0F0B0Bu,
0xFF1B1313u,
0xFF271B1Bu,
0xFF332727u,
0xFF3F2F2Fu,
0xFF4B3737u,
0xFF573F3Fu,
0xFF674747u,
0xFF734F4Fu,
0xFF7F5B5Bu,
0xFF8B6363u,
0xFF976B6Bu,
0xFFA37373u,
0xFFAF7B7Bu,
0xFFBB8383u,
0xFFCB8B8Bu,
0xFF000404u,
0xFF000707u,
0xFF000B0Bu,
0xFF001313u,
0xFF001B1Bu,
0xFF002323u,
0xFF072B2Bu,
0xFF072F2Fu,
0xFF073737u,
0xFF073F3Fu,
0xFF074747u,
0xFF0B4B4Bu,
0xFF0B5353u,
0xFF0B5B5Bu,
0xFF0B6363u,
0xFF0F6B6Bu,
0xFF000007u,
0xFF00000Fu,
0xFF000017u,
0xFF00001Fu,
0xFF000027u,
0xFF00002Fu,
0xFF000037u,
0xFF00003Fu,
0xFF000047u,
0xFF00004Fu,
0xFF000057u,
0xFF00005Fu,
0xFF000067u,
0xFF00006Fu,
0xFF000077u,
0xFF00007Fu,
0xFF000E0Fu,
0xFF00191Au,
0xFF002223u,
0xFF002B2Fu,
0xFF002F37u,
0xFF003743u,
0xFF073B4Bu,
0xFF074357u,
0xFF07475Fu,
0xFF0B4B6Bu,
0xFF0F5377u,
0xFF135783u,
0xFF135B8Bu,
0xFF1B5F97u,
0xFF1F63A3u,
0xFF2367AFu,
0xFF071323u,
0xFF0B172Fu,
0xFF0F1F3Bu,
0xFF13234Bu,
0xFF172B57u,
0xFF1F2F63u,
0xFF233773u,
0xFF2B3B7Fu,
0xFF33438Fu,
0xFF334F9Fu,
0xFF2F63AFu,
0xFF2F77BFu,
0xFF2B8FCFu,
0xFF27ABDFu,
0xFF1FCBEFu,
0xFF1BF3FFu,
0xFF00070Bu,
0xFF00131Bu,
0xFF0F232Bu,
0xFF132B37u,
0xFF1B3347u,
0xFF233753u,
0xFF2B3F63u,
0xFF33476Fu,
0xFF3F537Fu,
0xFF475F8Bu,
0xFF536B9Bu,
0xFF5F7BA7u,
0xFF6B87B7u,
0xFF7B93C3u,
0xFF8BA3D3u,
0xFF97B3E3u,
0xFFA38BABu,
0xFF977F9Fu,
0xFF877393u,
0xFF7B678Bu,
0xFF6F5B7Fu,
0xFF635377u,
0xFF574B6Bu,
0xFF4B3F5Fu,
0xFF433757u,
0xFF372F4Bu,
0xFF2F2743u,
0xFF231F37u,
0xFF1B172Bu,
0xFF131323u,
0xFF0B0B17u,
0xFF07070Fu,
0xFFA074BAu,
0xFF8F6BAFu,
0xFF835FA3u,
0xFF775797u,
0xFF6B4F8Bu,
0xFF5F4B7Fu,
0xFF534373u,
0xFF4B3B6Bu,
0xFF3F335Fu,
0xFF372B53u,
0xFF2B2347u,
0xFF231F3Bu,
0xFF1B172Fu,
0xFF121223u,
0xFF0A0A17u,
0xFF06060Fu,
0xFFBBC3DBu,
0xFFA7B3CBu,
0xFF9BA3BFu,
0xFF8B97AFu,
0xFF7B87A3u,
0xFF6F7B97u,
0xFF5F6F87u,
0xFF53637Bu,
0xFF47576Bu,
0xFF3B4B5Fu,
0xFF333F53u,
0xFF273343u,
0xFF1F2B37u,
0xFF171F27u,
0xFF0F131Bu,
0xFF080B0Fu,
0xFF7B836Fu,
0xFF6F7B67u,
0xFF67735Fu,
0xFF5F6B57u,
0xFF57634Fu,
0xFF4F5B47u,
0xFF47533Fu,
0xFF3F4B37u,
0xFF37432Fu,
0xFF2F3B2Bu,
0xFF273323u,
0xFF1F2B1Fu,
0xFF172317u,
0xFF131B0Fu,
0xFF0B130Bu,
0xFF070B07u,
0xFF00FFFFu,
0xFF00EAEEu,
0xFF00D2DCu,
0xFF00BCCCu,
0xFF00AABAu,
0xFF009CB0u,
0xFF0088A0u,
0xFF007488u,
0xFF006478u,
0xFF005068u,
0xFF004860u,
0xFF00384Cu,
0xFF002D3Cu,
0xFF00202Cu,
0xFF001018u,
0xFF00080Cu,
0xFFFF0000u,
0xFFEE0000u,
0xFFDC0000u,
0xFFCC0000u,
0xFFBA0000u,
0xFFAA0000u,
0xFF9A0000u,
0xFF880000u,
0xFF00FF00u,
0xFF00EE00u,
0xFF00DC00u,
0xFF00CC00u,
0xFF00BA00u,
0xFF00AA00u,
0xFF009A00u,
0xFF008800u,
0xFF00002Bu,
0xFF00003Bu,
0xFF00074Bu,
0xFF00075Fu,
0xFF000F6Fu,
0xFF07177Fu,
0xFF071F93u,
0xFF0B27A3u,
0xFF0F33B7u,
0xFF1B4BC3u,
0xFF2B63CFu,
0xFF3B7FDBu,
0xFF4F97E3u,
0xFF5FABE7u,
0xFF77BFEFu,
0xFF8CD2F6u,
0xFF3B7BA7u,
0xFF379BB7u,
0xFF37C3C7u,
0xFF57E3E7u,
0xFF00C800u,
0xFFFFECA0u,
0xFFFFFFD7u,
0xFF000068u,
0xFF00008Cu,
0xFF0000B4u,
0xFF0000D8u,
0xFF0000FFu,
0xFF93F3FFu,
0xFFC7F7FFu,
0xFFFFFFFFu,// yes. all the time.
0xFF535B9Fu,
};
