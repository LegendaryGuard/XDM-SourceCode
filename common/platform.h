//====================================================================
// PLATFORM.H
// Purpose: absolutely global things like preprocessor directives,
// includes, types, defines, platform dependancy overrides
//====================================================================
#ifndef PLATFORM_H
#define PLATFORM_H
#if defined(_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* !__MINGW32__ */
#endif /* _WIN32 */

// Allow "DEBUG" in addition to default "_DEBUG"
#if defined (_DEBUG)
#define DEBUG 1
#define BUILD_DESC	"Debug"
#else
#define BUILD_DESC	"Release"
#endif

// Start ignoring stuff written by others
#if defined(_MSC_VER)
#pragma warning(disable: 4820)// 'n' bytes padding added after data member 'x'
#endif // _MSC_VER

// NULL, sprintf, sscanf, etc.
#include <stdio.h>
// min/max/bits
#include <limits.h>

#if !defined(UI32_MAX)
#if defined(_UI32_MAX)
#define UI32_MAX			_UI32_MAX
#else
#define UI32_MAX			0xffffffffu /* maximum unsigned 32 bit value */
#endif
#endif


// file information functions
#include <sys/stat.h>

#if defined(_WIN32)

// Prevent windows.h from defining unnecessary stuff
#define NOMINMAX
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME

#if defined(_MSC_VER)
// Disable some unimportant warnings
#pragma warning(disable: 4061)// enumerator '%s' in switch of enum '%s_e' is not explicitly handled by a case label
#pragma warning(disable: 4062)// enumerator '%s' in switch of enum '%s_e' is not handled
#pragma warning(disable: 4100)// unreferenced formal parameter
#pragma warning(disable: 4201)// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4244)// conversion from 'int' to 'unsigned char', possible loss of data
#pragma warning(disable: 4305)// truncation from 'const double' to 'float'
#pragma warning(disable: 4310)// cast truncates constant value
#pragma warning(disable: 4514)// unreferenced inline function has been removed
// Elevate some important warnings into errors
#pragma warning(error: 4264)// no override available for virtual member function from base 'CBaseEntity'; function is hidden
#pragma warning(error: 4715)// not all control paths return a value
#endif // _MSC_VER

#include <direct.h>// _mkdir

#else// Non-WIN32 platforms after this line
/*
#pragma warning(disable : ??? )

warning: deprecated conversion from string constant to ‘char*’
error: cast from ‘char*’ to ‘int’ loses precision
*/
//#include <sys/dir.h>// _mkdir
//#include <unistd.h>
#include <math.h>

// Prototype names conversion for ISO compatibility under POSIX environment
#ifndef _snprintf
#define _snprintf snprintf
#endif // !_snprintf
//#ifndef _alloca
//#define _alloca alloca
//#endif
#ifndef _strnicmp
#define _strnicmp strncasecmp
#endif
#ifndef _stricmp
#define _stricmp strcasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif
//#ifndef strcmpi
//#define strcmpi strcasecmp
//#endif
#ifndef _access
#define _access access
#endif
#ifndef _open
#define _open open
#endif
#ifndef _close
#define _close close
#endif

#ifndef _mkdir
#define _mkdir(path) mkdir(path, 0777)
#endif

#ifndef _strupr
char *X_strupr(char *_Str);// in common utils
#define _strupr X_strupr
#endif

#endif// _WIN32

// Start ignoring stuff written by others
#if defined(_MSC_VER)
#pragma warning(default: 4820)// 'n' bytes padding added after data member 'x'
#endif // _MSC_VER

// Makes these more explicit, and easier to find
#define FILE_GLOBAL static
#define DLL_GLOBAL
// Until we figure out why "const" gives the compiler problems, we'll just have to use
// this bogus "empty" define to mark things as constant.
//#define CONSTANT
// More explicit than "int"
typedef int EOFFSET;

#ifndef TRUE
#define FALSE 0
#define TRUE 1
//#define TRUE (!FALSE)
#endif// !TRUE

#ifndef nullptr
#define nullptr NULL
#endif // !nullptr

#define BADPOINTER (void *)0xBADBADBA

#include "port.h"

// SQH: hack dragged since quake
typedef int BOOL;

#ifndef byte
typedef unsigned char byte;
#endif
#ifndef word
typedef unsigned short word;
#endif
/*#ifndef WORD
typedef unsigned short WORD;
#endif
#ifndef DWORD
typedef unsigned long DWORD;
#endif*/
#ifndef ULONG
typedef unsigned long ULONG;
#endif
#ifndef uchar
typedef unsigned char uchar;
#endif
#ifndef ushort
typedef unsigned short ushort;
#endif
#ifndef uint
typedef unsigned int uint;
#endif
#ifndef ulong
typedef unsigned long ulong;
#endif

// XDM3037: for unicode routines
#if defined (_MSC_VER) || defined(_WIN32)
typedef wchar_t	uchar16;
typedef unsigned int uchar32;
#else
typedef unsigned short uchar16;
typedef wchar_t uchar32;
#endif // (_MSC_VER) || defined(_WIN32)

#define MASK16_4BIT			0x000F
#define MASK16_8BIT			0x00FF

#define MASK32_4BIT			0x0000000F
#define MASK32_8BIT			0x000000FF
#define MASK32_16BIT		0x0000FFFF
#define MASK32_24BIT		0x00FFFFFF

// Suitable for 24- and 32-bit colors
/* color.h
#define RGBA2INT(r,g,b,a)	((unsigned long)(((byte)(r)|((WORD)((byte)(g))<<CHAR_BIT))|(((DWORD)(byte)(b))<<(2*CHAR_BIT))|(((DWORD)(byte)(a))<<(3*CHAR_BIT))))
#define RGB2INT(r,g,b)		((unsigned long)(((byte)(r)|((WORD)((byte)(g))<<CHAR_BIT))|(((DWORD)(byte)(b))<<(2*CHAR_BIT))))
#define RGB2R(rgb)			((unsigned char)(rgb))
#define RGB2G(rgb)			((unsigned char)(((unsigned short)(rgb)) >> CHAR_BIT))
#define RGB2B(rgb)			((unsigned char)((rgb) >> (2*CHAR_BIT)))
#define RGB2A(rgba)			((unsigned char)((rgba) >> (3*CHAR_BIT)))
#define RGBA2A(rgba)		((unsigned char)((rgba) >> (3*CHAR_BIT)))*/

// too bad it doesn't fit binary bool
enum policy_values_e
{
	POLICY_UNDEFINED = 0,// must be the initial value of a policy
	POLICY_ALLOW,// explicit value
	POLICY_DENY,// explicit value
};


#if defined(_WIN32)
// Prevent tons of unused windows definitions
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRALEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOGDI
#if defined(_MSC_VER)
#define VC_EXTRALEAN
#if _MSC_VER > 1200
#include <crtdbg.h>
#endif// _MSC_VER > 1200
#endif// _MSC_VER
#include <windows.h>

#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif// MAX_PATH

#else // _WIN32

#define MAX_PATH PATH_MAX
#define _MAX_FNAME NAME_MAX
#include <limits.h>

#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)

#if !defined (HINSTANCE)
typedef void* HINSTANCE;
#endif

#endif //_WIN32

#define PATHSEPARATOR_DOS		'\\'
#define PATHSEPARATOR_UNIX		'/'

#ifndef PATHSEPARATOR
#if defined(_WINDOWS)
#define PATHSEPARATOR			PATHSEPARATOR_DOS
#define PATHSEPARATOR_FOREIGN	PATHSEPARATOR_UNIX
#else
#define PATHSEPARATOR			PATHSEPARATOR_UNIX
#define PATHSEPARATOR_FOREIGN	PATHSEPARATOR_DOS
#endif // _WINDOWS
#endif // PATHSEPARATOR


/* Used for standard calling conventions */
#if (_MSC_VER > 0)/* VC compiler used */
	#define  STDCALL				__stdcall
	#define  FASTCALL			   __fastcall
#ifndef FORCEINLINE// GNU C may define it
	#define  FORCEINLINE		   __forceinline
#endif
#else
	#define  STDCALL
	#define  FASTCALL
#ifndef FORCEINLINE
	#define  FORCEINLINE		   inline
#endif
#endif

#if defined(_WIN32)
#define DLLEXPORT	__declspec(dllexport)// XDM: right name, but conflicts with stupidity in eiface.h
#define EXPORT		__declspec(dllexport)// XDM3034: ANSI C++
#else
#define DLLEXPORT	__attribute__ ((visibility("default")))// HL20130901
#define EXPORT		__attribute__ ((visibility("default")))
#endif // _WIN32

// Keeps clutter down a bit, when declaring external entity/global method prototypes
//#define DECLARE_GLOBAL_METHOD(MethodName) extern void DLLEXPORT MethodName(void)
//#define GLOBAL_METHOD(funcname) void DLLEXPORT funcname(void)

void DBG_PrintF(const char *szFmt, ...);// Prints to debugger output. Don't use this directly, use DBG_PRINTF!

#if defined(_DEBUG)

bool DBG_AssertFunction(bool fExpr, const char *szExpr, const char *szFile, int iLine, const char *szMessage);
#define ASSERT(f)			DBG_AssertFunction(f, #f, __FILE__, __LINE__, NULL)
#define ASSERTSZ(f, sz)		DBG_AssertFunction(f, #f, __FILE__, __LINE__, sz)
#define ASSERTD(f)			ASSERT(f)// use this macro to place an assert only in debug build
#if defined(_MSC_VER)
#define DBG_FORCEBREAK		_asm {int 3};// XDM3035: should be _DbgBreak();
#else
#define DBG_FORCEBREAK		ASSERT(false);
#endif // _MSC_VER
#define DBG_PRINTF			DBG_PrintF
//#define PARM_CHK_NULL(p)	DBG_AssertFunction(p != NULL, #p, __FILE__, __LINE__, "Parameter cannot be NULL!\n")

#else	// _DEBUG

bool NDB_AssertFunction(bool fExpr, const char *szExpr, const char *szMessage);
#define ASSERT(f)			NDB_AssertFunction(f, #f, NULL)
#define ASSERTSZ(f, sz)		NDB_AssertFunction(f, #f, sz)
#define ASSERTD				// do nothing in release build
#define DBG_FORCEBREAK		ASSERT(false);
#define DBG_PRINTF			// do nothing in release build
//#define PARM_CHK_NULL(p)	NDB_AssertFunction(p != NULL, #p, "Parameter cannot be NULL!\n")

#endif	// _DEBUG

#ifndef Assert
#define Assert			ASSERT
#define Assertsz		ASSERTSZ
#endif // Assert

// XDM3038b: printf format to display entity information before actual text
#define ENTFMT_S "%s[%d]\"%s\": %s"
// Common printf-like parameters
#define ENTFMT_P STRING(pev->classname), entindex(), STRING(pev->targetname)
#define ENTFMT_PE(pEntity) STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->targetname)
//#define ENTFMT "%s[%d]\"%s\": %s\n", STRING(pev->classname), entindex(), STRING(pev->targetname)
// To be used like conprintf(1, ENTFMT, "TEST\n");
#define ENTFMT ENTFMT_S ENTFMT_P
// Debug macros to be used inside CBaseEntity-derived code.
// We could surely use the handy __FUNCTION__ macro, but it is Microsoft-specific and we have to use the same macro for all platforms.
#if defined(_DEBUG) && defined(_DEBUG_ENTITIES)
#define DBG_PRINT_ENT(text)				DBG_PrintF("%s[%d]\"%s\": %s\n", ENTFMT_P, text)
#define DBG_PRINT_ENT_THINK(func)		DBG_PrintF("%s[%d]\"%s\"::%s()\n", ENTFMT_P, #func)
#define DBG_PRINT_ENT_TOUCH(func)		DBG_PrintF("%s[%d]\"%s\"::%s(%s[%d])\n", ENTFMT_P, #func, pOther?STRING(pOther->pev->classname):"", pOther?pOther->entindex():0)
#define DBG_PRINT_ENT_BLOCKED(func)		DBG_PrintF("%s[%d]\"%s\"::%s(%s[%d])\n", ENTFMT_P, #func, pOther?STRING(pOther->pev->classname):"", pOther?pOther->entindex():0)
#define DBG_PRINT_ENT_USE(func)			DBG_PrintF("%s[%d]\"%s\"::%s(%s[%d], %s[%d], %d, %g)\n", ENTFMT_P, #func, pActivator?STRING(pActivator->pev->classname):"", pActivator?pActivator->entindex():0, pCaller?STRING(pCaller->pev->classname):"", pCaller?pCaller->entindex():0, useType, value)
#define DBG_PRINT_ENT_TAKEDAMAGE		DBG_PrintF("%s[%d]\"%s\"::TakeDamage(%s[%d], %s[%d], %g, %d)\n", ENTFMT_P, pInflictor?STRING(pInflictor->pev->classname):"", pInflictor?pInflictor->entindex():0, pAttacker?STRING(pAttacker->pev->classname):"", pAttacker?pAttacker->entindex():0, flDamage, bitsDamageType)
#else	// _DEBUG && _DEBUG_ENTITIES
#define DBG_PRINT_ENT(text)
#define DBG_PRINT_ENT_THINK(func)
#define DBG_PRINT_ENT_TOUCH(func)
#define DBG_PRINT_ENT_BLOCKED(func)
#define DBG_PRINT_ENT_USE(func)
#define DBG_PRINT_ENT_TAKEDAMAGE
#endif	// _DEBUG && _DEBUG_ENTITIES


// XDM3038: moved to a place AFTER possible system definitions
#ifndef ARRAYSIZE
// Size of a fixed array:
#define ARRAYSIZE(a)		(sizeof(a)/sizeof((a)[0]))
#endif


#ifndef min
#define min(a,b)			(((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)			(((a) > (b)) ? (a) : (b))
#endif

// little hack to avoid some conflicts
#ifndef __min
#define __min(a,b)			(((a) < (b)) ? (a) : (b))
#endif

#ifndef __max
#define __max(a,b)			(((a) > (b)) ? (a) : (b))
#endif

#ifndef clamp// XDM
#define clamp(a,min,max)	((a < min)?(min):((a > max)?(max):(a)))
#endif

#ifndef SWAP
#define SWAP(a,b,temp)		((temp)=(a),(a)=(b),(b)=(temp))
#endif

#endif // PLATFORM_H
