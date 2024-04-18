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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "gamerules.h"// XDM
#include "game.h"

// XDM3037 LINK_ENTITY_TO_CLASS( soundent, CSoundEnt );

DLL_GLOBAL CSoundEnt gSoundEnt;// XDM3037: create when DLL is loaded, pSoundEnt is assigned in the constructor
DLL_GLOBAL CSoundEnt *pSoundEnt = NULL;


//=========================================================
// CSound - Clear - zeros all fields for a sound
//=========================================================
void CSound :: Clear (void)
{
	Reset();
	//m_vecOrigin		= g_vecZero;
	//m_iType			= 0;
	//m_iVolume		= 0;
	m_flExpireTime	= 0;
	//m_iNext			= SOUNDLIST_EMPTY;
	m_iNextAudible	= 0;
}

//=========================================================
// Reset - clears the volume, origin, and type for a sound, but doesn't expire or unlink it. 
//=========================================================
void CSound :: Reset (void)
{
	m_vecOrigin		= g_vecZero;
	m_iType			= 0;
	m_iVolume		= 0;
	m_iNext			= SOUNDLIST_EMPTY;
}

//=========================================================
// FIsSound - returns TRUE if the sound is an Audible sound
//=========================================================
bool CSound :: FIsSound (void)
{
	if ( m_iType & ( bits_SOUND_COMBAT | bits_SOUND_WORLD | bits_SOUND_PLAYER | bits_SOUND_DANGER ) )
		return true;

	return false;
}

//=========================================================
// FIsScent - returns TRUE if the sound is actually a scent
//=========================================================
bool CSound :: FIsScent (void)
{
	if ( m_iType & ( bits_SOUND_CARCASS | bits_SOUND_MEAT | bits_SOUND_GARBAGE ) )
		return true;

	return false;
}







//-----------------------------------------------------------------------------
// Purpose: XDM3037: Constructor
// WARNING! No world, entities or APIs here!!
//-----------------------------------------------------------------------------
CSoundEnt::CSoundEnt()
{
	m_iFreeSound = SOUNDLIST_EMPTY;
	m_iActiveSound = SOUNDLIST_EMPTY;
	m_cLastActiveSounds = 0;
// NO!	Initialize();
	ASSERT(pSoundEnt == NULL);
	pSoundEnt = this;
	m_fNextUpdate = 1.0;// XDM3038a
}

CSoundEnt::~CSoundEnt()
{
	ASSERT(pSoundEnt == this);
	pSoundEnt = NULL;
}

//=========================================================
// Think - at interval, the entire active sound list is checked
// for sounds that have ExpireTimes less than or equal
// to the current world time, and these sounds are deallocated.
//=========================================================
void CSoundEnt::Think(void)
{
// XDM3037	SetNextThink(0.3);// how often to check the sound list.
	m_fNextUpdate = gpGlobals->time + 0.3;

	int iPreviousSound = SOUNDLIST_EMPTY;
	int iSound = m_iActiveSound; 

	while (iSound != SOUNDLIST_EMPTY)
	{
		if (m_SoundPool[iSound].m_flExpireTime <= gpGlobals->time && m_SoundPool[iSound].m_flExpireTime != SOUND_NEVER_EXPIRE)
		{
			int iNext = m_SoundPool[iSound].m_iNext;
			// move this sound back into the free list
			FreeSound(iSound, iPreviousSound);
			iSound = iNext;
		}
		else
		{
			iPreviousSound = iSound;
			iSound = m_SoundPool[iSound].m_iNext;
		}
	}

	if (displaysoundlist.value > 0.0f)
	{
		conprintf(2, "CSoundEnt::Think(): Soundlist: %d / %d (%d)\n", ISoundsInList(SOUNDLISTTYPE_ACTIVE), ISoundsInList(SOUNDLISTTYPE_FREE), ISoundsInList(SOUNDLISTTYPE_ACTIVE) - m_cLastActiveSounds);
		m_cLastActiveSounds = ISoundsInList(SOUNDLISTTYPE_ACTIVE);
	}
}

//=========================================================
// Clears all sounds and moves them into the free sound list.
// WARNING: XDM3037: DON'T call from constructor!
//=========================================================
void CSoundEnt :: Initialize (void)
{
	int i;
	int iSound;

	m_cLastActiveSounds = 0;
	m_iFreeSound = 0;
	m_iActiveSound = SOUNDLIST_EMPTY;
	m_fNextUpdate = gpGlobals->time + 1.0f;// XDM3038a: important!

	for (i = 0 ; i < MAX_WORLD_SOUNDS ; ++i)
	{// clear all sounds, and link them into the free sound list.
		m_SoundPool[i].Clear();
		m_SoundPool[i].m_iNext = i + 1;
	}

	m_SoundPool[i-1].m_iNext = SOUNDLIST_EMPTY;// terminate the list here.
	
	// now reserve enough sounds for each client
	for (i = 0 ; i < gpGlobals->maxClients ; ++i)
	{
		iSound = pSoundEnt->IAllocSound();
		if (iSound == SOUNDLIST_EMPTY)
		{
			conprintf(1, "DLL: Could not AllocSound() for Client Reserve %d!\n", i);
			return;
		}
		pSoundEnt->m_SoundPool[iSound].m_flExpireTime = SOUND_NEVER_EXPIRE;
	}
}

//=========================================================
// Returns the number of sounds in the desired sound list.
//=========================================================
int CSoundEnt::ISoundsInList(int iListType)
{
	int iThisSound = 0;

	if ( iListType == SOUNDLISTTYPE_FREE )
	{
		iThisSound = m_iFreeSound;
	}
	else if ( iListType == SOUNDLISTTYPE_ACTIVE )
	{
		iThisSound = m_iActiveSound;
	}
	else
		conprintf(1, "Unknown Sound List Type %d!\n", iListType);

	if ( iThisSound == SOUNDLIST_EMPTY )
		return 0;

	int i = 0;
	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		++i;
		iThisSound = m_SoundPool[ iThisSound ].m_iNext;
	}

	return i;
}

//=========================================================
// Moves a sound from the Free list to the Active list returns the index of the alloc'd sound
//=========================================================
int CSoundEnt::IAllocSound(void)
{
	if ( m_iFreeSound == SOUNDLIST_EMPTY )
	{
		// no free sound!
		conprintf(2, "Free Sound List is full!\n");
		return SOUNDLIST_EMPTY;
	}

	// there is at least one sound available, so move it to the
	// Active sound list, and return its SoundPool index.
	int iNewSound = m_iFreeSound;// copy the index of the next free sound
	m_iFreeSound = m_SoundPool[ m_iFreeSound ].m_iNext;// move the index down into the free list. 
	m_SoundPool[ iNewSound ].m_iNext = m_iActiveSound;// point the new sound at the top of the active list.
	m_iActiveSound = iNewSound;// now make the new sound the top of the active list. You're done.
	return iNewSound;
}



//=========================================================
// STATIC functions go here!
//=========================================================


//=========================================================
// FreeSound - clears the passed active sound and moves it 
// to the top of the free list. TAKE CARE to only call this
// function for sounds in the Active list!!
//=========================================================
void CSoundEnt::FreeSound(int iSound, int iPrevious)
{
	if (!pSoundEnt)
		return;// no sound ent!

	//DBG_PRINTF("CSoundEnt::FreeSound(iSound %d, iPrevious %d)\n", iSound, iPrevious);
	if (iPrevious != SOUNDLIST_EMPTY)
	{
		// iSound is not the head of the active list, so
		// must fix the index for the Previous sound
		//pSoundEnt->m_SoundPool[iPrevious].m_iNext = m_SoundPool[iSound].m_iNext;
		pSoundEnt->m_SoundPool[iPrevious].m_iNext = pSoundEnt->m_SoundPool[iSound].m_iNext;
	}
	else
	{
		// the sound we're freeing IS the head of the active list.
		pSoundEnt->m_iActiveSound = pSoundEnt->m_SoundPool[iSound].m_iNext;
	}

	// make iSound the head of the Free list.
	pSoundEnt->m_SoundPool[iSound].m_iNext = pSoundEnt->m_iFreeSound;
	pSoundEnt->m_iFreeSound = iSound;
}

//=========================================================
// Allocates a free sound and fills it with sound info.
//=========================================================
void CSoundEnt::InsertSound(int iType, const Vector &vecOrigin, int iVolume, float flDuration)
{
	if (!pSoundEnt)
		return;// no sound ent!

	if (g_pGameRules == NULL)// XDM3037
		return;
	if (g_pGameRules->IsGameOver() || !g_pGameRules->FAllowMonsters())// XDM3037 only monsters need this
		return;

	int iThisSound = pSoundEnt->IAllocSound();
	if (iThisSound == SOUNDLIST_EMPTY)
	{
		//if (!g_pGameRules->IsMultiplayer())// XDM3037
		//	conprintf(2, "Could not AllocSound() for InsertSound()\n");

		return;
	}
	//DBG_PRINTF("CSoundEnt::InsertSound(%d, %d, %g): index %d\n", iType, iVolume, flDuration, iThisSound);
	pSoundEnt->m_SoundPool[iThisSound].m_vecOrigin = vecOrigin;
	pSoundEnt->m_SoundPool[iThisSound].m_iType = iType;
	pSoundEnt->m_SoundPool[iThisSound].m_iVolume = iVolume;
	pSoundEnt->m_SoundPool[iThisSound].m_flExpireTime = gpGlobals->time + flDuration;
}

//=========================================================
// Returns the head of the active sound list
//=========================================================
int CSoundEnt :: ActiveList (void)
{
	if ( !pSoundEnt )
		return SOUNDLIST_EMPTY;

	return pSoundEnt->m_iActiveSound;
}

//=========================================================
// Returns the head of the free sound list
//=========================================================
int CSoundEnt :: FreeList (void)
{
	if ( !pSoundEnt )
		return SOUNDLIST_EMPTY;

	return pSoundEnt->m_iFreeSound;
}

//=========================================================
// Returns a pointer to the instance of CSound at index's position in the sound pool.
//=========================================================
CSound *CSoundEnt::SoundPointerForIndex(const int &iIndex)
{
	if ( !pSoundEnt )
		return NULL;

	if ( iIndex > ( MAX_WORLD_SOUNDS - 1 ) )
	{
		conprintf(1, "SoundPointerForIndex(%d) - Index too large!\n", iIndex);
		return NULL;
	}

	if ( iIndex < 0 )
	{
		conprintf(2, "SoundPointerForIndex(%d) - Index < 0!\n", iIndex);
		return NULL;
	}

	return &pSoundEnt->m_SoundPool[ iIndex ];
}

//=========================================================
// Clients are numbered from 1 to MAXCLIENTS, but the client
// reserved sounds in the soundlist are from 0 to MAXCLIENTS - 1,
// so this function ensures that a client gets the proper index
// to his reserved sound in the soundlist.
//=========================================================
int CSoundEnt::ClientSoundIndex(edict_t *pClient)
{
	int iReturn;
	if (pClient)
		iReturn = ENTINDEX(pClient)-1;
	else
		iReturn = 0;

#if defined (_DEBUG)
	if (iReturn < 0 || iReturn > gpGlobals->maxClients)
	{
		conprintf(2, "ClientSoundIndex(%d) returning a bogus value!\n", iReturn);
	}
#endif // _DEBUG

	// XDM3037: NOTE: it's okay to return 'invalid' index, because when server is shutting down, maxClients is 1, but we need to free sounds for ALL allocated players.
	return iReturn;
}
