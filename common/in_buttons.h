/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef IN_BUTTONS_H
#define IN_BUTTONS_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

// Button bits that are sent to server
#define IN_ATTACK		(1 << 0)
#define IN_JUMP			(1 << 1)
#define IN_DUCK			(1 << 2)
#define IN_FORWARD		(1 << 3)// +cmd->forwardmove
#define IN_BACK			(1 << 4)// -cmd->forwardmove
#define IN_USE			(1 << 5)
//#define IN_CANCEL		(1 << 6) unused
#define IN_LEFT			(1 << 7)
#define IN_RIGHT		(1 << 8)
#define IN_MOVELEFT		(1 << 9)// -cmd->sidemove
#define IN_MOVERIGHT	(1 << 10)// +cmd->sidemove
#define IN_ATTACK2		(1 << 11)
#define IN_SPEED		(1 << 12)
#define IN_RELOAD		(1 << 13)
//#define IN_ALT1		(1 << 14) unused
#define IN_SCORE		(1 << 15)// scoreboard is held down
// Won't fit in 2 bytes
#define IN_MOVEUP		(1 << 16)// XDM3038: 131072 +cmd->upmove
#define IN_MOVEDOWN		(1 << 17)// XDM3038: 262144 -cmd->upmove

// Common masks
#define BUTTONS_FIRE	(IN_ATTACK | IN_ATTACK2)
#define BUTTONS_READY	(IN_ATTACK | IN_ATTACK2 | IN_USE)
//#define BUTTONS_RESPAWN	(IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP)

#endif // IN_BUTTONS_H
