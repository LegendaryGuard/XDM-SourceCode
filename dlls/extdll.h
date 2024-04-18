#ifndef EXTDLL_H
#define EXTDLL_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

//
// Global header file for extension DLLs
//

// Silence certain warnings (see platform.h)
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>// itoa
#include <stdarg.h>
#include <string.h>
#include <ctype.h>//isspace

#include "vector.h"

// Shared engine/DLL constants
#include "const.h"
#include "progdefs.h"
#include "edict.h"

// Shared header describing protocol between engine and DLLs
#include "Sequence.h"
#include "eiface.h"

// Shared header between the client DLL and the game DLLs
#include "cdll_dll.h"

enum XDM_EntityIs_e
{
	ENTIS_INVALID = 0,
	ENTIS_PLAYER,
	ENTIS_MONSTER,
	ENTIS_HUMAN,
	ENTIS_GAMEGOAL,
	ENTIS_PROJECTILE,
	ENTIS_PLAYERWEAPON,
	ENTIS_PLAYERITEM,
	ENTIS_OTHERPICKUP,
	ENTIS_PUSHABLE,
	ENTIS_BREAKABLE,
	ENTIS_TRIGGER,
	ENTIS_OTHER
};

enum XDM_GetActiveGameRules_e
{
	AGR_NOTINSTALLED = 0,
	AGR_ACTIVE,
	AGR_GAMEOVER
};

extern const char *g_szNonEntityExports[];

/*
int EXPORT Classify(edict_t *entity);
int EXPORT EntityIs(edict_t *entity);
int EXPORT EntityRelationship(edict_t *entity1, edict_t *entity2);
int EXPORT EntityObjectCaps(edict_t *entity);
int EXPORT CanHaveItem(edict_t *entity, edict_t *item);
int EXPORT GetActiveGameRules(short *type, short *mode);
*/

//#ifndef __linux__ ? FAR?

typedef int (*pfnXHL_Classify_t)(edict_t *entity);// XDM3037
typedef int (*pfnXHL_EntityIs_t)(edict_t *entity);// XDM3035a
typedef int (*pfnXHL_EntityRelationship_t)(edict_t *entity1, edict_t *entity2);// XDM3035a
typedef int (*pfnXHL_EntityObjectCaps_t)(edict_t *entity);// XDM3037
typedef int (*pfnXHL_CanHaveItem_t)(edict_t *entity, edict_t *item);// XDM3035c
typedef int (*pfnXHL_GetActiveGameRules_t)(short *type, short *mode);// XDM3036

#if !defined (CLIENT_DLL)
extern "C" 
{
int EXPORT Classify(edict_t *entity);
int EXPORT EntityIs(edict_t *entity);
int EXPORT EntityRelationship(edict_t *entity1, edict_t *entity2);
int EXPORT EntityObjectCaps(edict_t *entity);
int EXPORT CanHaveItem(edict_t *entity, edict_t *item);
int EXPORT GetActiveGameRules(short *type, short *mode);
}
#endif // CLIENT_DLL

#endif // EXTDLL_H
