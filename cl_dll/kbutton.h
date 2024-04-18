//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined( KBUTTONH )
#define KBUTTONH
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */

#define BUTTONSTATE_DOWN			1
#define BUTTONSTATE_IMPULSEDOWN		2
#define BUTTONSTATE_IMPULSEUP		4

typedef struct kbutton_s
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

#endif // !KBUTTONH
