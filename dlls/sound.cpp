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
//=========================================================
// sound.cpp
//=========================================================
// XDM3037: warning! this code is old as %s!!!

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapondef.h"// XDM
#include "player.h"
#include "talkmonster.h"
#include "gamerules.h"
#include "sound.h"
#include "func_break.h"
#include "game.h"
#include "globals.h"
#include "pm_shared.h"


// ==================== GENERIC AMBIENT SOUND ======================================

// presets for runtime pitch and vol modulation of ambient sounds
dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] =
{
// pitch	pstart	spinup	spindwn	volrun	volstrt	fadein	fadeout	lfotype	lforate	modptch modvol	cspnup
{1,	255,	 75,	95,		95,		10,		1,		50,		95, 	0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{2,	255,	 85,	70,		88,		10,		1,		20,		88,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{3,	255,	100,	50,		75,		10,		1,		10,		75,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{4,	100,	100,	0,		0,		10,		1,		90,		90,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{5,	100,	100,	0,		0,		10,		1,		80,		80,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{6,	100,	100,	0,		0,		10,		1,		50,		70,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{7,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		50,		0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{8,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		150,	0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{9,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		750,	0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{10,128,	100,	50,		75,		10,		1,		30,		40,		2,		 8,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{11,128,	100,	50,		75,		10,		1,		30,		40,		2,		25,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{12,128,	100,	50,		75,		10,		1,		30,		40,		2,		70,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{13,50,		 50,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{14,70,		 70,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{15,90,		 90,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{16,120,	120,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{17,180,	180,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{18,255,	255,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{19,200,	 75,	90,		90,		10,		1,		50,		90,		2,		100,	20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{20,255,	 75,	97,		90,		10,		1,		50,		90,		1,		40,		50,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{21,100,	100,	0,		0,		10,		1,		30,		50,		3,		15,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{22,160,	160,	0,		0,		10,		1,		50,		50,		3,		500,	25,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{23,255,	 75,	88,		0,		10,		1,		40,		0,		0,		0,		0,		0,		5,		0,0,0,0,0,0,0,0,0,0},
{24,200,	 20,	95,	    70,		10,		1,		70,		70,		3,		20,		50,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{25,180,	100,	50,		60,		10,		1,		40,		60,		2,		90,		100,	100,	0,		0,0,0,0,0,0,0,0,0,0},
{26,60,		 60,	0,		0,		10,		1,		40,		70,		3,		80,		20,		50,		0,		0,0,0,0,0,0,0,0,0,0},
{27,128,	 90,	10,		10,		10,		1,		20,		40,		1,		5,		10,		20,		0,		0,0,0,0,0,0,0,0,0,0}
};

LINK_ENTITY_TO_CLASS( ambient_generic, CAmbientGeneric );

TYPEDESCRIPTION	CAmbientGeneric::m_SaveData[] =
{
	DEFINE_FIELD( CAmbientGeneric, m_flAttenuation, FIELD_FLOAT ),
	DEFINE_FIELD( CAmbientGeneric, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAmbientGeneric, m_fLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAmbientGeneric, m_iChannel, FIELD_INTEGER ), // SHL
	DEFINE_FIELD( CAmbientGeneric, m_hPlayFrom, FIELD_EHANDLE ), // XDM3038c

	// HACKHACK - This is not really in the spirit of the save/restore design, but save this
	// out as a binary data block.  If the dynpitchvol_t is changed, old saved games will NOT
	// load these correctly, so bump the save/restore version if you change the size of the struct
	// The right way to do this is to split the input parms (read in keyvalue) into members and re-init this
	// struct in Precache(), but it's unlikely that the struct will change, so it's not worth the time right now.
	DEFINE_ARRAY( CAmbientGeneric, m_dpv, FIELD_CHARACTER, sizeof(dynpitchvol_t) ),
};

IMPLEMENT_SAVERESTORE( CAmbientGeneric, CBaseEntity );

// KeyValue - load keyvalue pairs into member data of the
// ambient generic. NOTE: called BEFORE spawn!
void CAmbientGeneric :: KeyValue(KeyValueData *pkvd)
{
	// NOTE: changing any of the modifiers in this code
	// NOTE: also requires changing InitModulationParms code.
	if (FStrEq(pkvd->szKeyName, "preset"))
	{
		m_dpv.preset = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		m_dpv.pitchrun = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

		if (m_dpv.pitchrun > PITCH_MAX) m_dpv.pitchrun = PITCH_MAX;
		else if (m_dpv.pitchrun < PITCH_MIN) m_dpv.pitchrun = PITCH_MIN;// XDM3038c: TESTME: was 0
	}
	else if (FStrEq(pkvd->szKeyName, "attenuation"))// XDM3037: fixed: float
	{
		m_flAttenuation = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchstart"))
	{
		m_dpv.pitchstart = atoi(pkvd->szValue);
		if (m_dpv.pitchstart > PITCH_MAX) m_dpv.pitchstart = PITCH_MAX;
		else if (m_dpv.pitchstart < PITCH_MIN) m_dpv.pitchstart = PITCH_MIN;// XDM3038c: TESTME: was 0
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spinup"))
	{
		m_dpv.spinup = atoi(pkvd->szValue);
		if (m_dpv.spinup > 100) m_dpv.spinup = 100;
		else if (m_dpv.spinup < 0) m_dpv.spinup = 0;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;
		m_dpv.spinupsav = m_dpv.spinup;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spindown"))
	{
		m_dpv.spindown = atoi(pkvd->szValue);
		if (m_dpv.spindown > 100) m_dpv.spindown = 100;
		else if (m_dpv.spindown < 0) m_dpv.spindown = 0;
		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		m_dpv.spindownsav = m_dpv.spindown;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volstart"))
	{
		m_dpv.volstart = atoi(pkvd->szValue);
		if (m_dpv.volstart > 10) m_dpv.volstart = 10;
		else if (m_dpv.volstart < 0) m_dpv.volstart = 0;
		m_dpv.volstart *= 10;	// 0 - 100
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_dpv.fadein = atoi(pkvd->szValue);
		if (m_dpv.fadein > 100) m_dpv.fadein = 100;
		else if (m_dpv.fadein < 0) m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		m_dpv.fadeinsav = m_dpv.fadein;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_dpv.fadeout = atoi(pkvd->szValue);
		if (m_dpv.fadeout > 100) m_dpv.fadeout = 100;
		else if (m_dpv.fadeout < 0) m_dpv.fadeout = 0;
		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lfotype"))
	{
		m_dpv.lfotype = atoi(pkvd->szValue);
		if (m_dpv.lfotype > 4) m_dpv.lfotype = LFO_TRIANGLE;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lforate"))
	{
		m_dpv.lforate = atoi(pkvd->szValue);
		if (m_dpv.lforate > 1000) m_dpv.lforate = 1000;
		else if (m_dpv.lforate < 0) m_dpv.lforate = 0;
		m_dpv.lforate *= 256;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lfomodpitch"))
	{
		m_dpv.lfomodpitch = atoi(pkvd->szValue);
		if (m_dpv.lfomodpitch > 100) m_dpv.lfomodpitch = 100;
		else if (m_dpv.lfomodpitch < 0) m_dpv.lfomodpitch = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lfomodvol"))
	{
		m_dpv.lfomodvol = atoi(pkvd->szValue);
		if (m_dpv.lfomodvol > 100) m_dpv.lfomodvol = 100;
		else if (m_dpv.lfomodvol < 0) m_dpv.lfomodvol = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "cspinup"))
	{
		m_dpv.cspinup = atoi(pkvd->szValue);
		if (m_dpv.cspinup > 100) m_dpv.cspinup = 100;
		else if (m_dpv.cspinup < 0) m_dpv.cspinup = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "channel"))// XDM: channel (e.g. CHAN_STATIC, etc.)
	{
		m_iChannel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

//
// ambient_generic - general-purpose user-defined static sound
//
void CAmbientGeneric::Spawn(void)
{
	if (m_flAttenuation == 0.0f)// XDM3035c: custom value overrides presets
	{
		if (FBitSet(pev->spawnflags, AMBIENT_SOUND_EVERYWHERE))
			m_flAttenuation = ATTN_NONE;
		else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_SMALLRADIUS))
			m_flAttenuation = ATTN_IDLE;
		else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_MEDIUMRADIUS))
			m_flAttenuation = ATTN_STATIC;
		else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_LARGERADIUS))
			m_flAttenuation = ATTN_NORM;
		else// if the designer didn't set a sound attenuation, default to one.
			m_flAttenuation = ATTN_STATIC;
	}

	char *szSoundFile = STRINGV(pev->message);
	if (FStringNull(pev->message) || strlen(szSoundFile) < 1)
	{
		conprintf(1, "EMPTY AMBIENT AT: %f, %f, %f REMOVED!\n", pev->origin.x, pev->origin.y, pev->origin.z);
		SetNextThink(0.1);
		SetThink(&CBaseEntity::SUB_Remove);
		return;
	}
    pev->solid		= SOLID_NOT;
    pev->movetype	= MOVETYPE_NONE;

	// Set up think function for dynamic modification of ambient sound's pitch or volume. Don't start thinking yet.
	SetThink(&CAmbientGeneric::RampThink);
	DontThink();// XDM3038a

	// allow on/off switching via 'use' function.
	SetUse(&CAmbientGeneric::ToggleUse);

	m_fActive = FALSE;

	if (FBitSet(pev->spawnflags, AMBIENT_SOUND_NOT_LOOPING))
		m_fLooping = FALSE;
	else
		m_fLooping = TRUE;

	if (m_iChannel == 0)// BUGBUG: this excludes CHAN_AUTO
		m_iChannel = CHAN_STATIC;

	Precache();
}

void CAmbientGeneric::Precache(void)
{
	if (!FStringNull(pev->message) && strlen(STRING(pev->message)) > 1)// XDM3038
	{
		if ((STRING(pev->message))[0] != '!')
			PRECACHE_SOUND(STRINGV(pev->message));
	}

	// init all dynamic modulation parms
	InitModulationParms();

	if (!FBitSet(pev->spawnflags, AMBIENT_SOUND_START_SILENT))
	{
		// start the sound ASAP
		if (m_fLooping)
			m_fActive = TRUE;
	}

	if (!FStringNull(pev->target))// XDM
	{
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
		if (!pTarget)
		{
			conprintf(1, "WARNING: ambient_generic \"%s\" can't find \"%s\", its entity to play from.\n", STRING(pev->targetname), STRING(pev->target));
			m_hPlayFrom = NULL;
		}
		else
			m_hPlayFrom = pTarget;
	}

	if (m_fActive)
	{
		if (m_hPlayFrom.Get())// XDM
			EMIT_SOUND_DYN(m_hPlayFrom.Get(), m_iChannel, STRING(pev->message), (m_dpv.vol * 0.01f), m_flAttenuation, SND_SPAWNING, m_dpv.pitch);//SetThink(StartPlayFrom);
		else
			UTIL_EmitAmbientSound(edict(), pev->origin, STRING(pev->message), (m_dpv.vol * 0.01f), m_flAttenuation, SND_SPAWNING, m_dpv.pitch);

		SetNextThink(0.1);
	}
}

// SHL LRC - for some reason, I can't get other entities to start playing sounds during Activate;
// this function is used to delay the effect until the first Think, which seems to fix the problem.
/*void CAmbientGeneric::StartPlayFrom(void)
{
	EMIT_SOUND_DYN(m_hPlayFrom.Get(), m_iChannel, STRING(pev->message), (m_dpv.vol * 0.01), m_flAttenuation, SND_SPAWNING, m_dpv.pitch);
	SetThink(&CAmbientGeneric::RampThink);
	SetNextThink( 0.1 );
}*/

// RampThink - Think at 5hz if we are dynamically modifying
// pitch or volume of the playing sound.  This function will
// ramp pitch and/or volume up or down, modify pitch/volume
// with lfo if active.
void CAmbientGeneric :: RampThink(void)
{
	char *szSoundFile = STRINGV(pev->message);
	int pitch = m_dpv.pitch;
	int vol = m_dpv.vol;
	int flags = 0;
	int fChanged = 0;		// FALSE if pitch and vol remain unchanged this round
	int	prev;

	if (!m_dpv.spinup && !m_dpv.spindown && !m_dpv.fadein && !m_dpv.fadeout && !m_dpv.lfotype)
		return;						// no ramps or lfo, stop thinking

	// ==============
	// pitch envelope
	// ==============
	if (m_dpv.spinup || m_dpv.spindown)
	{
		prev = m_dpv.pitchfrac >> 8;

		if (m_dpv.spinup > 0)
			m_dpv.pitchfrac += m_dpv.spinup;
		else if (m_dpv.spindown > 0)
			m_dpv.pitchfrac -= m_dpv.spindown;

		pitch = m_dpv.pitchfrac >> 8;

		if (pitch > m_dpv.pitchrun)
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0;				// done with ramp up
		}

		if (pitch < m_dpv.pitchstart)
		{
			pitch = m_dpv.pitchstart;
			m_dpv.spindown = 0;				// done with ramp down

			// shut sound off
			if (m_hPlayFrom.Get())// XDM
				STOP_SOUND(m_hPlayFrom.Get(), m_iChannel, szSoundFile);// SHL
			else
				UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);

			// return without setting nextthink
			return;
		}

		if (pitch > 255) pitch = 255;
		if (pitch < 1) pitch = 1;
		m_dpv.pitch = pitch;
		fChanged |= (prev != pitch);
		flags |= SND_CHANGE_PITCH;
	}

	// ==================
	// amplitude envelope
	// ==================
	if (m_dpv.fadein || m_dpv.fadeout)
	{
		prev = m_dpv.volfrac >> 8;

		if (m_dpv.fadein > 0)
			m_dpv.volfrac += m_dpv.fadein;
		else if (m_dpv.fadeout > 0)
			m_dpv.volfrac -= m_dpv.fadeout;

		vol = m_dpv.volfrac >> 8;

		if (vol > m_dpv.volrun)
		{
			vol = m_dpv.volrun;
			m_dpv.fadein = 0;				// done with ramp up
		}

		if (vol < m_dpv.volstart)
		{
			vol = m_dpv.volstart;
			m_dpv.fadeout = 0;				// done with ramp down

			// shut sound off
			if (m_hPlayFrom.Get())// XDM
				STOP_SOUND(m_hPlayFrom.Get(), m_iChannel, szSoundFile);// SHL
			else
				UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);

			// return without setting nextthink
			return;
		}

		if (vol > 100) vol = 100;
		if (vol < 1) vol = 1;

		m_dpv.vol = vol;

		fChanged |= (prev != vol);
		flags |= SND_CHANGE_VOL;
	}

	// ===================
	// pitch/amplitude LFO
	// ===================
	if (m_dpv.lfotype)
	{
		int pos;

		if (m_dpv.lfofrac > 0x6fffffff)
			m_dpv.lfofrac = 0;

		// update lfo, lfofrac/255 makes a triangle wave 0-255
		m_dpv.lfofrac += m_dpv.lforate;
		pos = m_dpv.lfofrac >> 8;

		if (m_dpv.lfofrac < 0)
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs(m_dpv.lforate);
			pos = 0;
		}
		else if (pos > 255)
		{
			pos = 255;
			m_dpv.lfofrac = (255 << 8);
			m_dpv.lforate = -abs(m_dpv.lforate);
		}

		switch (m_dpv.lfotype)
		{
		case LFO_SQUARE:
			if (pos < 128)
				m_dpv.lfomult = 255;
			else
				m_dpv.lfomult = 0;

			break;
		case LFO_RANDOM:
			if (pos == 255)
				m_dpv.lfomult = RANDOM_LONG(0, 255);
			break;
		case LFO_TRIANGLE:
		default:
			m_dpv.lfomult = pos;
			break;
		}

		if (m_dpv.lfomodpitch)
		{
			prev = pitch;
			// pitch 0-255
			pitch += ((m_dpv.lfomult - 128) * m_dpv.lfomodpitch) / 100;
			if (pitch > 255) pitch = 255;
			if (pitch < 1) pitch = 1;
			fChanged |= (prev != pitch);
			flags |= SND_CHANGE_PITCH;
		}

		if (m_dpv.lfomodvol)
		{
			// vol 0-100
			prev = vol;
			vol += ((m_dpv.lfomult - 128) * m_dpv.lfomodvol) / 100;
			if (vol > 100) vol = 100;
			if (vol < 0) vol = 0;
			fChanged |= (prev != vol);
			flags |= SND_CHANGE_VOL;
		}

	}

	// Send update to playing sound only if we actually changed
	// pitch or volume in this routine.
	if (flags && fChanged)
	{
		if (pitch == PITCH_NORM)
			pitch = PITCH_NORM + 1; // don't send 'no pitch' !

		if (m_hPlayFrom.Get())// XDM
			EMIT_SOUND_DYN(m_hPlayFrom.Get(), m_iChannel, szSoundFile, (vol * 0.01), m_flAttenuation, flags, pitch);
		else
			UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, (vol * 0.01), m_flAttenuation, flags, pitch);
	}

	// update ramps at 5hz
	SetNextThink(0.2);
	return;
}

// Init all ramp params in preparation to
// play a new sound
void CAmbientGeneric :: InitModulationParms(void)
{
	int pitchinc;
	m_dpv.volrun = (int)(pev->health * 10.0f);// 0 - 10 -> 0 - 100
	if (m_dpv.volrun > 100) m_dpv.volrun = 100;
	if (m_dpv.volrun < 0) m_dpv.volrun = 0;

	// get presets
	if (m_dpv.preset != 0 && m_dpv.preset <= CDPVPRESETMAX)
	{
		// load preset values
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		// fixup preset values, just like
		// fixups in KeyValue routine.
		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.lforate *= 256;
		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}

	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0;

	if (m_dpv.fadein)
		m_dpv.vol = m_dpv.volstart;
	else
		m_dpv.vol = m_dpv.volrun;

	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0;

	if (m_dpv.spinup)
		m_dpv.pitch = m_dpv.pitchstart;
	else
		m_dpv.pitch = m_dpv.pitchrun;

	if (m_dpv.pitch == 0)
		m_dpv.pitch = PITCH_NORM;

	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;
	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs(m_dpv.lforate);
	m_dpv.cspincount = 1;

	if (m_dpv.cspinup)
	{
		pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

		m_dpv.pitchrun = m_dpv.pitchstart + pitchinc;
		if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;
	}

	if ((m_dpv.spinupsav || m_dpv.spindownsav || (m_dpv.lfotype && m_dpv.lfomodpitch))
		&& (m_dpv.pitch == PITCH_NORM))
		m_dpv.pitch = PITCH_NORM + 1; // must never send 'no pitch' as first pitch
									  // if we intend to pitch shift later!
}

//
// ToggleUse - turns an ambient sound on or off.  If the
// ambient is a looping sound, mark sound as active (m_fActive)
// if it's playing, innactive if not.  If the sound is not
// a looping sound, never mark it as active.
//
void CAmbientGeneric :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DBG_PRINT_ENT_USE(ToggleUse);
	if ( useType != USE_TOGGLE )
	{
		if ( (m_fActive && useType == USE_ON) || (!m_fActive && useType == USE_OFF) )
			return;
	}
	const char *szSoundFile = STRING(pev->message);
	// Directly change pitch if arg passed. Only works if sound is already playing.
	if (useType == USE_SET && m_fActive)		// Momentary buttons will pass down a float in here
	{
		float fraction = value;

		if (fraction > 1.0f)
			fraction = 1.0f;

		if (fraction < 0.0f)
			fraction = 0.01f;

		m_dpv.pitch = (int)(fraction * 255.0f);

		if (m_hPlayFrom.Get())// XDM
			EMIT_SOUND_DYN(m_hPlayFrom.Get(), m_iChannel, szSoundFile, 0, 0, SND_CHANGE_PITCH, m_dpv.pitch);
		else
			UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, 0, 0, SND_CHANGE_PITCH, m_dpv.pitch);

		return;
	}

	// Toggle
	// m_fActive is TRUE only if a looping sound is playing.
	if ( m_fActive )
	{// turn sound off

		if (m_dpv.cspinup)
		{
			// Don't actually shut off. Each toggle causes incremental spinup to max pitch
			if (m_dpv.cspincount <= m_dpv.cspinup)
			{
				// start a new spinup
				m_dpv.cspincount++;
				int pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;
				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;
				m_dpv.pitchrun = m_dpv.pitchstart + pitchinc * m_dpv.cspincount;
				if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;
				SetNextThink(0.1);
			}
		}
		else
		{
			m_fActive = FALSE;

			// HACKHACK - this makes the code in Precache() work properly after a save/restore
			SetBits(pev->spawnflags, AMBIENT_SOUND_START_SILENT);

			if (m_dpv.spindownsav || m_dpv.fadeoutsav)
			{
				// spin it down (or fade it) before shutoff if spindown is set
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;
				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				SetNextThink(0.1);
			}
			else
			{
				if (m_hPlayFrom.Get())// XDM
					STOP_SOUND(m_hPlayFrom.Get(), m_iChannel, szSoundFile);
				else
					UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);
			}
		}
	}
	else
	{// turn sound on
		// only toggle if this is a looping sound.  If not looping, each
		// trigger will cause the sound to play.  If the sound is still
		// playing from a previous trigger press, it will be shut off
		// and then restarted.

		if (m_fLooping)
		{
			m_fActive = TRUE;
		}
		else
		{// shut sound off now - may be interrupting a long non-looping sound
			if (m_hPlayFrom.Get())// XDM
				STOP_SOUND(m_hPlayFrom.Get(), m_iChannel, szSoundFile);// SHL
			else
				UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);
		}

		// init all ramp params for startup
		InitModulationParms();

		if (m_hPlayFrom.Get())// XDM
			EMIT_SOUND_DYN(m_hPlayFrom.Get(), m_iChannel, szSoundFile, (m_dpv.vol * 0.01), m_flAttenuation, 0, m_dpv.pitch);
		else
			UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, (m_dpv.vol * 0.01), m_flAttenuation, 0, m_dpv.pitch);

		SetNextThink(0.1);
	}
}

void CAmbientGeneric::OnFreePrivateData(void)// XDM3037: TESTME
{
	if (m_fActive)
	{
		if (m_hPlayFrom.Get())
			STOP_SOUND(m_hPlayFrom.Get(), m_iChannel, STRINGV(pev->message));
		else
			UTIL_EmitAmbientSound(edict(), pev->origin, STRINGV(pev->message), 0, 0, SND_STOP, 0);
	}
	CBaseEntity::OnFreePrivateData();// XDM3038c
}




// =================== ROOM SOUND FX ==========================================
LINK_ENTITY_TO_CLASS( env_sound, CEnvSound );

TYPEDESCRIPTION	CEnvSound::m_SaveData[] =
{
	DEFINE_FIELD( CEnvSound, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvSound, m_flRoomtype, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CEnvSound, CBaseDelay );

void CEnvSound::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_flRadius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "roomtype"))
	{
		m_flRoomtype = atof(pkvd->szValue);// FIXME: another stupid leftover. An integer, really.
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

// returns TRUE if the given sound entity (pev) is in range
// and can see the given player entity (pevTarget)
bool FEnvSoundInRange(CEnvSound *pSound, entvars_t *pevTarget, float *pflRange)
{
	//Vector vecSpot1(pev->origin + pev->view_ofs);
	Vector vecSpot2(pevTarget->origin + pevTarget->view_ofs);
	TraceResult tr;
	UTIL_TraceLine(pSound->pev->origin, vecSpot2, ignore_monsters, pSound->edict(), &tr);

	// check if line of sight crosses water boundary, or is blocked
	if ((tr.fInOpen && tr.fInWater) || tr.flFraction != 1)
		return false;

	// calc range from sound entity to player
	Vector vecRange(tr.vecEndPos);//vecSpot1;
	vecRange -= pSound->pev->origin;
	vec_t flRange = vecRange.Length();
	//CEnvSound *pSound = (CEnvSound *)CEnvSound::Instance(ENT(pev));//GetClassPtr((CEnvSound *)pev);
	if (pSound->m_flRadius < flRange)
		return false;

	if (pflRange)
		*pflRange = flRange;

	return true;
}

//
// A client that is visible and in range of a sound entity will
// have its room_type set by that sound entity.  If two or more
// sound entities are contending for a client, then the nearest
// sound entity to the client will set the client's room_type.
// A client's room_type will remain set to its prior value until
// a new in-range, visible sound entity resets a new room_type.
//
// CONSIDER: if player in water state, autoset roomtype to 14,15 or 16.
void CEnvSound::Think(void)
{
	// get pointer to client if visible; FIND_CLIENT_IN_PVS will
	// cycle through visible clients on consecutive calls.

	// XDM3037: TESTME!!!
	float flRange;
	edict_t *pentPlayer = NULL;
	edict_t *pFirst = NULL;
	CBaseEntity *pPlayerEntity = NULL;
	CBasePlayer *pPlayer = NULL;
	while ((pentPlayer = FIND_CLIENT_IN_PVS(edict())) != NULL)// XDM3037: tricky!
	{
		if (pFirst == NULL)
			pFirst = pentPlayer;
		else if (FNullEnt(pentPlayer) || pFirst == pentPlayer)// reached first again
			break;

		pPlayerEntity = CBaseEntity::Instance(pentPlayer);
		if (!pPlayerEntity->IsPlayer())// XDM3037: double-check
			break;

		pPlayer = (CBasePlayer *)pPlayerEntity;

		// if we got this far, we're looking at an entity that is contending
		// for current player sound. the closest entity to player wins.
		if (FEnvSoundInRange(this, pPlayerEntity->pev, &flRange))
		{
			if (flRange < pPlayer->m_flSndRange || pPlayer->m_flSndRange == 0)// if current radius was far or unset
				pPlayer->UpdateSoundEnvironment(this, m_flRoomtype, flRange);// XDM3038

			// crank up nextthink rate for new active sound entity by falling through to think_fast...
			// player is not closer to the contending sound entity,
			// just fall through to think_fast. this effectively
			// cranks up the think_rate of entities near the player.
		}
		else if (pPlayer->m_hEntSndLast.Get() == edict())// player is out of our range, disable affection
		{
			if (!FBitSet(pev->spawnflags, SF_ENVSOUND_PERSISTENT))// XDM3038c: changes room type permanently
				pPlayer->UpdateSoundEnvironment(NULL, 0, 0);
		}

	}// XDM3037
	// player is in pvs of sound entity, but either not visible or
	// not in range. do nothing, fall through to think_fast...

	CBaseEntity::Think();// XDM3037: allow SetThink()
	SetNextThink(0.25);
}

//
// env_sound - spawn a sound entity that will set player roomtype
// when player moves in range and sight.
//
//
void CEnvSound :: Spawn(void)
{
	// spread think times
	SetNextThink(RANDOM_FLOAT(0.0, 0.5));
}


// ==================== SENTENCE GROUPS, UTILITY FUNCTIONS  ======================================

// globals
SENTENCEG rgsentenceg[CSENTENCEG_MAX];
bool fSentencesInit = false;// XDM3037: bool

char gszallsentencenames[CVOXFILESENTENCEMAX][CBSENTENCENAME_MAX];
int gcallsentences = 0;

// randomize list of sentence name indices
void USENTENCEG_InitLRU(unsigned char *plru, int count)
{
	if (!fSentencesInit)
		return;

	if (count > CSENTENCE_LRU_MAX)
		count = CSENTENCE_LRU_MAX;

	int i, j, k;
	unsigned char temp;
	for (i = 0; i < count; ++i)
		plru[i] = (unsigned char)i;

	// randomize array
	for (i = 0; i < (count * 4); ++i)
	{
		j = RANDOM_LONG(0,count-1);
		k = RANDOM_LONG(0,count-1);
		temp = plru[j];
		plru[j] = plru[k];
		plru[k] = temp;
	}
}

// ignore lru. pick next sentence from sentence group. Go in order until we hit the last sentence,
// then repeat list if freset is true.  If freset is false, then repeat last sentence.
// ipick is passed in as the requested sentence ordinal.
// ipick 'next' is returned.
// return of -1 indicates an error.
int USENTENCEG_PickSequential(int isentenceg, char *szfound, int ipick, int freset)
{
	if (!fSentencesInit)
		return -1;

	if (isentenceg < 0)
		return -1;

	unsigned int count = rgsentenceg[isentenceg].count;// XDM3038c: INT

	if (count == 0)
		return -1;

	if (ipick >= count)
		ipick = count-1;

	char *szgroupname = rgsentenceg[isentenceg].szgroupname;
	sprintf(szfound, "!%s%d", szgroupname, ipick);// XDM3035

	if (ipick >= count)
	{
		if (freset)
			// reset at end of list
			return 0;
		else
			return count;
	}

	return ipick + 1;
}

// pick a random sentence from rootname0 to rootnameX.
// picks from the rgsentenceg[isentenceg] least
// recently used, modifies lru array. returns the sentencename.
// note, lru must be seeded with 0-n randomized sentence numbers, with the
// rest of the lru filled with -1. The first integer in the lru is
// actually the size of the list.  Returns ipick, the ordinal
// of the picked sentence within the group.
int USENTENCEG_Pick(int isentenceg, char *szfound)
{
	if (!fSentencesInit)
		return -1;

	if (isentenceg < 0)
		return -1;

	const char *szgroupname = rgsentenceg[isentenceg].szgroupname;
	unsigned char *plru;
	unsigned int count;// XDM3038c: INT
	unsigned char i;
	unsigned char ipick = 0;
	bool ffound = false;
	count = rgsentenceg[isentenceg].count;
	plru = rgsentenceg[isentenceg].rgblru;

	while (!ffound)
	{
		for (i = 0; i < count; ++i)
			if (plru[i] != 0xFF)
			{
				ipick = plru[i];
				plru[i] = 0xFF;
				ffound = true;
				break;
			}

		if (!ffound)
			USENTENCEG_InitLRU(plru, count);
		else
		{
			sprintf(szfound, "!%s%d", szgroupname, ipick);// XDM3035
			return ipick;
		}
	}
	return -1;
}

// ===================== SENTENCE GROUPS, MAIN ROUTINES ========================

// Given sentence group rootname (name without number suffix),
// get sentence group index (isentenceg). Returns -1 if no such name.
int SENTENCEG_GetIndex(const char *szgroupname)
{
	if (!fSentencesInit || !szgroupname)
		return -1;

	// search rgsentenceg for match on szgroupname
	int i = 0;
	while (rgsentenceg[i].count)
	{
		if (!strcmp(szgroupname, rgsentenceg[i].szgroupname))
			return i;
		++i;
	}
	return -1;
}

// given sentence group index, play random sentence for given entity.
// returns ipick - which sentence was picked to
// play from the group. Ipick is only needed if you plan on stopping
// the sound before playback is done (see SENTENCEG_Stop).
int SENTENCEG_PlayRndI(edict_t *entity, int isentenceg, float volume, float attenuation, int flags, int pitch)
{
	if (!fSentencesInit)
		return -1;

	char name[64];
	name[0] = 0;
	int ipick = USENTENCEG_Pick(isentenceg, name);
	if (ipick > 0 && name)
		EMIT_SOUND_DYN(entity, CHAN_VOICE, name, volume, attenuation, flags, pitch);
	return ipick;
}

// same as above, but takes sentence group name instead of index
int SENTENCEG_PlayRndSz(edict_t *entity, const char *szgroupname, float volume, float attenuation, int flags, int pitch)
{
	if (!fSentencesInit)
		return -1;

	int isentenceg = SENTENCEG_GetIndex(szgroupname);
	if (isentenceg < 0)
	{
		conprintf(1, "No such sentence group: \"%s\"\n", szgroupname);
		return -1;
	}

	char name[64];
	name[0] = 0;
	int ipick = USENTENCEG_Pick(isentenceg, name);

	if (ipick >= 0 && name[0])
		EMIT_SOUND_DYN(entity, CHAN_VOICE, name, volume, attenuation, flags, pitch);

	return ipick;
}

// play sentences in sequential order from sentence group. Reset after last sentence.
int SENTENCEG_PlaySequentialSz(edict_t *entity, const char *szgroupname, float volume, float attenuation, int flags, int pitch, int ipick, int freset)
{
	if (!fSentencesInit)
		return -1;

	int isentenceg;
	isentenceg = SENTENCEG_GetIndex(szgroupname);
	if (isentenceg < 0)
		return -1;

	char name[64];
	name[0] = 0;
	int ipicknext = USENTENCEG_PickSequential(isentenceg, name, ipick, freset);
	if (ipicknext >= 0 && name[0])
		EMIT_SOUND_DYN(entity, CHAN_VOICE, name, volume, attenuation, flags, pitch);

	return ipicknext;
}

// for this entity, for the given sentence within the sentence group, stop the sentence.
void SENTENCEG_Stop(edict_t *entity, int isentenceg, int ipick)
{
	if (!fSentencesInit)
		return;

	if (isentenceg < 0 || ipick < 0)
		return;

	char buffer[64];
	sprintf(buffer, "!%s%d", rgsentenceg[isentenceg].szgroupname, ipick);// XDM3035
	STOP_SOUND(entity, CHAN_VOICE, buffer);
}

// open sentences.txt, scan for groups, build rgsentenceg
// Should be called from world spawn, only works on the first call and is ignored subsequently.
void SENTENCEG_Init(void)
{
	SERVER_PRINT("SENTENCEG_Init()\n");
	char buffer[512];
	char szgroup[64];
	int i, j;
	int isentencegs;

	if (fSentencesInit)
		return;

	memset(gszallsentencenames, 0, CVOXFILESENTENCEMAX * CBSENTENCENAME_MAX);
	gcallsentences = 0;
	memset(rgsentenceg, 0, CSENTENCEG_MAX * sizeof(SENTENCEG));
	memset(buffer, 0, 512);
	memset(szgroup, 0, 64);
	isentencegs = -1;


	int filePos = 0, fileSize;
	byte *pMemFile = LOAD_FILE_FOR_ME(SENTENCES_FILE, &fileSize);
	if (!pMemFile)
		return;

	// for each line in the file...
	while (memfgets(pMemFile, fileSize, filePos, buffer, 511) != NULL)
	{
		// skip whitespace
		i = 0;
		while(buffer[i] && buffer[i] == ' ')
			++i;

		if (!buffer[i])
			continue;

		if (buffer[i] == '/' || !isalpha(buffer[i]))
			continue;

		// get sentence name
		j = i;
		while (buffer[j] && buffer[j] != ' ')
			++j;

		if (!buffer[j])
			continue;

		if (gcallsentences > CVOXFILESENTENCEMAX)
		{
			conprintf(1, "Too many sentences in %s!\n", SENTENCES_FILE);
			break;
		}

		// null-terminate name and save in sentences array
		buffer[j] = 0;
		const char *pString = buffer + i;

		if ( strlen( pString ) >= CBSENTENCENAME_MAX )
			conprintf(1, "Sentence %s longer than %d letters\n", pString, CBSENTENCENAME_MAX-1 );

		strcpy( gszallsentencenames[gcallsentences++], pString );

		--j;
		if (j <= i)
			continue;
		if (!isdigit(buffer[j]))
			continue;

		// cut out suffix numbers
		while (j > i && isdigit(buffer[j]))
			--j;

		if (j <= i)
			continue;

		buffer[j+1] = 0;

		// if new name doesn't match previous group name,
		// make a new group.

		if (strcmp(szgroup, &(buffer[i])))
		{
			// name doesn't match with prev name,
			// copy name into group, init count to 1
			++isentencegs;
			if (isentencegs >= CSENTENCEG_MAX)
			{
				conprintf(1, "Too many sentence groups in \"%s\"!\n", SENTENCES_FILE);
				break;
			}

			strcpy(rgsentenceg[isentencegs].szgroupname, &(buffer[i]));
			rgsentenceg[isentencegs].count = 1;
			strcpy(szgroup, &(buffer[i]));
			continue;
		}
		else
		{
			//name matches with previous, increment group count
			if (isentencegs >= 0)
				rgsentenceg[isentencegs].count++;
		}
	}

	g_engfuncs.pfnFreeFile( pMemFile );
	fSentencesInit = true;
	conprintf(1, "%d sentences loaded from \"%s\".\n", gcallsentences, SENTENCES_FILE);// XDM3035
	// init lru lists
	i = 0;
	while (rgsentenceg[i].count && i < CSENTENCEG_MAX)
	{
		USENTENCEG_InitLRU(&(rgsentenceg[i].rgblru[0]), rgsentenceg[i].count);
		++i;
	}
}

// convert sentence (sample) name to !sentencenum, return !sentencenum
int SENTENCEG_Lookup(const char *sample, char *sentencenum)
{
	int i;
	// this is a sentence name; lookup sentence number and give to engine as string.
	for (i = 0; i < gcallsentences; ++i)
	{
		if (!_stricmp(gszallsentencenames[i], sample+1))
		{
			if (sentencenum)
				sprintf(sentencenum, "!%d", i);// XDM3035

			return i;
		}
	}
	conprintf(2, "Sentence \"%s\" not found in \"%s\"\n", sample, SENTENCES_FILE);
	return -1;// sentence name not found!
}

//-----------------------------------------------------------------------------
// Purpose: Play entity sound
// NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100 is no pitch shift.
// Pitch > 100 up to 255 is a higher pitch, pitch < 100 down to 1 is a lower pitch. 150 to 70 is the realistic range.
// EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as fast as EMIT_SOUND (the pitchshift mixer is not native coded).
//-----------------------------------------------------------------------------
void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch)
{
	if (FNullEnt(entity))// XDM
	{
		conprintf(1, "EMIT_SOUND_DYN(%s) error: called with NULL entity!\n", sample);
		return;
	}
	if (!sample || *sample == '\0')
	{
		conprintf(2, "EMIT_SOUND_DYN(%s[%d]\"%s\") error: no sample!\n", STRING(entity->v.classname), ENTINDEX(entity), STRING(entity->v.targetname));
		return;
	}
	// Prevent stupid engine crashes
	if (volume <= 0.0f && !(flags & SND_STOP))
	{
#if defined (_DEBUG)
		conprintf(2, "EMIT_SOUND_DYN(%s[%d]\"%s\") warning: tried to play \"%s\" with volume %g! Stopping.\n", STRING(entity->v.classname), ENTINDEX(entity), STRING(entity->v.targetname), sample, volume);
#endif
		return;
	}
	else if (volume > VOL_NORM)
	{
#if defined (_DEBUG)
		conprintf(2, "EMIT_SOUND_DYN(%s[%d]\"%s\") warning: tried to play \"%s\" with volume %g!\n", STRING(entity->v.classname), ENTINDEX(entity), STRING(entity->v.targetname), sample, volume);
#endif
		volume = VOL_NORM;
	}
	if (pitch < PITCH_MIN)
	{
#if defined (_DEBUG)
		conprintf(2, "EMIT_SOUND_DYN(%s[%d]\"%s\") warning: tried to play \"%s\" with pitch %d too low!\n", STRING(entity->v.classname), ENTINDEX(entity), STRING(entity->v.targetname), sample, pitch);
#endif
		pitch = PITCH_MIN;
	}
	else if (pitch > PITCH_MAX)
	{
#if defined (_DEBUG)
		conprintf(2, "EMIT_SOUND_DYN(%s[%d]\"%s\") warning: tried to play \"%s\" with pitch %d too high!\n", STRING(entity->v.classname), ENTINDEX(entity), STRING(entity->v.targetname), sample, pitch);
#endif
		pitch = PITCH_MAX;
	}
	if (*sample == '!')// another valvehack: we don't send sentence names to client side, we parse their names here and send indexes in hope that client has those the same
	{
		char name[32];
		if (SENTENCEG_Lookup(sample, name) >= 0)
		{
			//if (sindex > 255) done in the engine
			//	flags |= SND_LARGE_INDEX;
			EMIT_SOUND_DYN2(entity, channel, name, volume, attenuation, flags, pitch);
			//if (channel == CHAN_VOICE && sentence_text.value > 0)// subtitles
			//	UTIL_ShowMessageRadius(sample, entity->v.origin, volume * msg_max_radius.value);
		}
		//else
		// inside	conprintf(2, "EMIT_SOUND_DYN(%s[%d]\"%s\") error: sentence \"%s\" not found.\n", STRING(entity->v.classname), ENTINDEX(entity), STRING(entity->v.targetname), sample);
	}
	else
		EMIT_SOUND_DYN2(entity, channel, sample, volume, attenuation, flags, pitch);
}

// play a specific sentence over the HEV suit speaker - just pass player entity, and !sentencename
void EMIT_SOUND_SUIT(edict_t *entity, const char *sample)
{
	//int pitch = PITCH_NORM;
	float fvol = CVAR_GET_FLOAT("suitvolume");
	// XDM3038c: if (RANDOM_LONG(0,1))
	//	pitch = RANDOM_LONG(0,6) + 98;
	if (fvol > 0.05f)
		EMIT_SOUND_DYN(entity, CHAN_STATIC, sample, fvol, ATTN_IDLE, 0, PITCH_NORM);// XDM3035c: ATTN_IDLE
}

// play a sentence, randomly selected from the passed in group id, over the HEV suit speaker
void EMIT_GROUPID_SUIT(edict_t *entity, int isentenceg)
{
	//int pitch = PITCH_NORM;
	float fvol = CVAR_GET_FLOAT("suitvolume");// XDM: UNDONE: individual value for each client?
	// XDM3038c: if (RANDOM_LONG(0,1))
	//	pitch = RANDOM_LONG(0,6) + 98;
	if (fvol > 0.05f)
		SENTENCEG_PlayRndI(entity, isentenceg, fvol, ATTN_IDLE, 0, PITCH_NORM);// XDM3035c: ATTN_IDLE
}

// play a sentence, randomly selected from the passed in groupname
void EMIT_GROUPNAME_SUIT(edict_t *entity, const char *groupname)
{
	float fvol = CVAR_GET_FLOAT("suitvolume");
	//int pitch = PITCH_NORM;
	// XDM3038c: if (RANDOM_LONG(0,1))
	//	pitch = RANDOM_LONG(0,6) + 98;
	if (fvol > 0.05f)
		SENTENCEG_PlayRndSz(entity, groupname, fvol, ATTN_NORM, 0, PITCH_NORM);//pitch);
}

// ===================== MATERIAL TYPE DETECTION, MAIN ROUTINES ========================

//-----------------------------------------------------------------------------
// Purpose: Trace material of an entity/by the trace.
// Warning: Bug inside engine!
// Input  : &pEntity - if this entity has material property, it has higher priority than just texture
//			&vecSrc - for tracing
//			&vecEnd - 
// Output : single character texture type
//-----------------------------------------------------------------------------
char TEXTURETYPE_Trace(CBaseEntity *pEntity, const Vector &vecSrc, const Vector &vecEnd)// XDM3038
{
	char chTextureType = 0;
// old	CBaseEntity *pEntity = CBaseEntity::Instance(ptr->pHit);
	if (pEntity && (pEntity->IsMonster() || pEntity->IsPlayer() || pEntity->Classify() == CLASS_ALIEN_BIOWEAPON)) // HACK to play xen_* hit sounds
	{
		if (pEntity->Classify() == CLASS_MACHINE)
			chTextureType = CHAR_TEX_METAL;
		else
			chTextureType = CHAR_TEX_FLESH;
	}
	else if (pEntity && pEntity->IsBreakable())// FClassnameIs(pEntity->pev, "func_breakable"))
	{
		CBreakable *pBreakable = (CBreakable *)pEntity;
		if (pBreakable)
		{
			switch (pBreakable->m_Material)
			{
			default:
			case matNone:
			// unfortunately, this will break compatibility	chTextureType = CHAR_TEX_NULL; break;
			case matCinderBlock:
			case matRocks:
				chTextureType = CHAR_TEX_CONCRETE; break;
			case matGlass:
			case matUnbreakableGlass:
				chTextureType = CHAR_TEX_GLASS; break;
			case matWood:
				chTextureType = CHAR_TEX_WOOD; break;
			case matMetal:
				chTextureType = CHAR_TEX_METAL; break;
			case matFlesh:
				chTextureType = CHAR_TEX_FLESH; break;
			case matCeilingTile:
				chTextureType = CHAR_TEX_TILE; break;
			case matComputer:
				chTextureType = CHAR_TEX_COMPUTER; break;
			}
		}
	}
	else// try getting material by texture name
	{
		const char *pTextureName = NULL;
		if (pEntity == NULL)
			pEntity = g_pWorld;

		pTextureName = TRACE_TEXTURE(pEntity->edict(), vecSrc, vecEnd);// BUGBUG: at some strange angles this shit fails and returns "sky"!!
		//else
		// same	pTextureName = TRACE_TEXTURE(ENT(0), vecSrc, vecEnd);

		if (pTextureName)
		{
			//conprintf(2, "TRACE_TEXTURE returned %s\n", pTextureName);
			if (_stricmp(pTextureName, "sky") == 0)// XDM: HACK to avoid playing 'concrete' sounds when missing
			{
				chTextureType = CHAR_TEX_SKY;// XDM3038: special
			}
			else
			{
				// strip leading '-0' or '+0~' or '{' or '!'
				/*if (*pTextureName == '-' || *pTextureName == '+')
					pTextureName += 2;

				if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
					pTextureName++;

				char szbuffer[32];
				strcpy(szbuffer, pTextureName);
				szbuffer[CBTEXTURENAMEMAX - 1] = 0;
				chTextureType = PM_FindTextureType(szbuffer);*/
				chTextureType = PM_FindTextureTypeFull(pTextureName);// XDM3038c
			}
		}
	}
	//conprintf(1, "TEXTURETYPE_Trace(): returning %c\n", (int)chTextureType);
	return chTextureType;
}




// ===================================================================================
//
// Speaker class. Used for announcements per level, for door lock/unlock spoken voice.
//
LINK_ENTITY_TO_CLASS( speaker, CSpeaker );

TYPEDESCRIPTION	CSpeaker::m_SaveData[] =
{
	DEFINE_FIELD( CSpeaker, m_preset, FIELD_INTEGER ),
	DEFINE_FIELD( CSpeaker, m_flAttenuation, FIELD_FLOAT ),// XDM3037a
	DEFINE_FIELD( CSpeaker, m_iPitch, FIELD_INTEGER ),
	DEFINE_FIELD( CSpeaker, m_iDelayMin, FIELD_INTEGER ),
	DEFINE_FIELD( CSpeaker, m_iDelayMax, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSpeaker, CBaseEntity );

// KeyValue - load keyvalue pairs into member data
// NOTE: called BEFORE spawn!
void CSpeaker::KeyValue(KeyValueData *pkvd)
{
	// preset
	if (FStrEq(pkvd->szKeyName, "preset"))
	{
		m_preset = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "attenuation"))// XDM3037a
	{
		m_flAttenuation = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))// XDM3037a
	{
		m_iPitch = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay_min"))// XDM3037a
	{
		m_iDelayMin = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay_max"))// XDM3037a
	{
		m_iDelayMax = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

//
// ambient_generic - general-purpose user-defined static sound
//
void CSpeaker :: Spawn(void)
{
	if (!m_preset && (FStringNull(pev->message) || *STRING(pev->message) == 0))
	{
		conprintf(0, "Design error: SPEAKER with no Level/Sentence at: %g %g %g!\n", pev->origin.x, pev->origin.y, pev->origin.z);
		SetBits(pev->flags, FL_KILLME);
		return;
	}
	Precache();

	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_NONE;
	pev->effects	= EF_NODRAW;// XDM3037a

	// allow everywhere	if (m_flAttenuation == 0.0f)// XDM3037a
	//		m_flAttenuation = 0.3;// default HL

	if (m_iPitch == 0)// XDM3037a
		m_iPitch = PITCH_NORM;
	if (m_iDelayMin == 0)
		m_iDelayMin = (int)(ANNOUNCE_MINUTES_MIN * 60);
	if (m_iDelayMax == 0)
		m_iDelayMax = (int)(ANNOUNCE_MINUTES_MAX * 60);

	// allow on/off switching via 'use' function.
	SetUse(&CSpeaker::ToggleUse);
	SetThink(&CSpeaker::SpeakerThink);

	// set first announcement time for random n second
	if (FBitSet(pev->spawnflags, SPEAKER_START_SILENT))
		DontThink();// XDM3038a
	else
		SetNextThink(RANDOM_FLOAT(m_iDelayMin, m_iDelayMax));
}

void CSpeaker :: SpeakerThink(void)
{
	// Wait for the talkmonster to finish first.
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
	{
		pev->nextthink = CTalkMonster::g_talkWaitTime + RANDOM_FLOAT( 5, 10 );
		return;
	}

	char *szSoundFile = NULL;
	if (m_preset)
	{
		// go lookup preset text, assign szSoundFile
		switch (m_preset)
		{
		case 1: szSoundFile =  "C1A0_"; break;
		case 2: szSoundFile =  "C1A1_"; break;
		case 3: szSoundFile =  "C1A2_"; break;
		case 4: szSoundFile =  "C1A3_"; break;
		case 5: szSoundFile =  "C1A4_"; break;
		case 6: szSoundFile =  "C2A1_"; break;
		case 7: szSoundFile =  "C2A2_"; break;
		case 8: szSoundFile =  "C2A3_"; break;
		case 9: szSoundFile =  "C2A4_"; break;
		case 10: szSoundFile = "C2A5_"; break;
		case 11: szSoundFile = "C3A1_"; break;
		case 12: szSoundFile = "C3A2_"; break;
		}
	}
	else
		szSoundFile = STRINGV(pev->message);

	if (szSoundFile && szSoundFile[0] == '!')// XDM3038c
	{
		// play single sentence, one shot
		UTIL_EmitAmbientSound(edict(), pev->origin, szSoundFile, pev->health*0.1f, m_flAttenuation, 0, m_iPitch);
		// shut off and reset
		DontThink();// XDM3038a
	}
	else
	{
		// make random announcement from sentence group
		if (SENTENCEG_PlayRndSz(edict(), szSoundFile, pev->health*0.1f, m_flAttenuation, 0, m_iPitch) < 0)
			conprintf(2, "Error: %s[%d] \"%s\" has bad sentence group name: \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->targetname), szSoundFile);

		// set next announcement time for random 5 to 10 minute delay
		SetNextThink(RANDOM_FLOAT(m_iDelayMin, m_iDelayMax));
		CTalkMonster::g_talkWaitTime = gpGlobals->time + 5;		// time delay until it's ok to speak: used so that two NPCs don't talk at once
	}
}

//
// ToggleUse - if an announcement is pending, cancel it.  If no announcement is pending, start one.
//
void CSpeaker :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	bool fActive = (pev->nextthink > 0.0);
	// fActive is TRUE only if an announcement is pending
	if (!ShouldToggle(useType, fActive))// XDM3037a
		return;

	if ( useType == USE_ON )
		SetNextThink(0.1);// turn on announcements
	else if ( useType == USE_OFF )
		DontThink();// XDM3038a // turn off announcements
	else
	{
		// Toggle announcements
		if ( fActive )
			DontThink();// XDM3038a // turn off announcements
		else
			SetNextThink(0.1);// turn on announcements
	}
}




// play door or button locked or unlocked sounds.
// pass in pointer to valid locksound struct.
// if flocked is true, play 'door is locked' sound,
// otherwise play 'door is unlocked' sound
// NOTE: this routine is shared by doors and buttons
void PlayLockSounds(entvars_t *pev, locksound_t *pls, int flocked, float flsoundwait)//int fbutton)
{
	// LOCKED SOUND

	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
/*	float flsoundwait;

	if (fbutton)
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;
*/
	if (flocked)
	{
		int fplaysound = (pls->sLockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sLockedSentence && !pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		if (fplaysound && fplaysentence)
			fvol = 0.25f;
		else
			fvol = 1.0f;

		// if there is a locked sound, and we've debounced, play sound
		if (fplaysound)
		{
			// play 'door locked' sound
			EMIT_SOUND(ENT(pev), SNDCHAN_LOCK, STRINGV(pls->sLockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if (fplaysentence)
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;
			pls->iLockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sLockedSentence), 0.85, ATTN_NORM, 0, 100, pls->iLockedSentence, FALSE);
			pls->iUnlockedSentence = 0;
			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = (iprev == pls->iLockedSentence);
			pls->flwaitSentence = gpGlobals->time + LOCK_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		int fplaysound = (pls->sUnlockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sUnlockedSentence && !pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if (fplaysound && fplaysentence)
			fvol = 0.25f;
		else
			fvol = 1.0f;

		// play 'door unlocked' sound if set
		if (fplaysound)
		{
			EMIT_SOUND(ENT(pev), SNDCHAN_LOCK, STRINGV(pls->sUnlockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if (fplaysentence)
		{
			int iprev = pls->iUnlockedSentence;
			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sUnlockedSentence), 0.85, ATTN_NORM, 0, 100, pls->iUnlockedSentence, FALSE);
			pls->iLockedSentence = 0;
			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = (iprev == pls->iUnlockedSentence);
			pls->flwaitSentence = gpGlobals->time + LOCK_SENTENCEWAIT;
		}
	}
}

// Button sound table.
// Also used by CBaseDoor to get 'touched' door lock/unlock sounds
char *ButtonSound(int sound)
{
	char *pszSound;
	switch (sound)
	{
		case 0: pszSound = "common/null.wav"; break;
		case 1: pszSound = "buttons/button1.wav";	break;
		case 2: pszSound = "buttons/button2.wav";	break;
		case 3: pszSound = "buttons/button3.wav";	break;
		case 4: pszSound = "buttons/button4.wav";	break;
		case 5: pszSound = "buttons/button5.wav";	break;
		case 6: pszSound = "buttons/button6.wav";	break;
		case 7: pszSound = "buttons/button7.wav";	break;
		case 8: pszSound = "buttons/button8.wav";	break;
		default:
		case 9: pszSound = "buttons/button9.wav";	break;
		case 10: pszSound = "buttons/button10.wav";	break;
		case 11: pszSound = "buttons/button11.wav";	break;
		case 12: pszSound = "buttons/latchlocked1.wav";	break;
		case 13: pszSound = "buttons/latchunlocked1.wav";	break;
		case 14: pszSound = "buttons/lightswitch2.wav";break;

// next 6 slots reserved for any additional sliding button sounds we may add

		case 21: pszSound = "buttons/lever1.wav";	break;
		case 22: pszSound = "buttons/lever2.wav";	break;
		case 23: pszSound = "buttons/lever3.wav";	break;
		case 24: pszSound = "buttons/lever4.wav";	break;
		case 25: pszSound = "buttons/lever5.wav";	break;
	}
	return pszSound;
}



// XDM3035a: HACK!
const char **MaterialSoundList(unsigned int soundMaterial, short &soundCount)
{
	if (soundMaterial == matNone || soundMaterial == matLastMaterial)
		return NULL;

	const char **pSoundList = NULL;
	for (short i = 0; i < NUM_MATERIALS; ++i)
	{
		if (gMaterials[i].breakmat_id == soundMaterial)
		{
			pSoundList = gMaterials[i].ShardSounds;
			soundCount = NUM_SHARD_SOUNDS;
			break;
		}
	}
	return pSoundList;
}

//-----------------------------------------------------------------------------
// Purpose: Precache SHARD SOUNDS ONLY
// Input  : soundMaterial - 
//-----------------------------------------------------------------------------
void MaterialSoundPrecache(unsigned int soundMaterial)
{
	short i, soundCount = 0;
	const char **pSoundList = MaterialSoundList(soundMaterial, soundCount);
	conprintf(2, "MaterialSoundPrecache(%d) %d\n", soundMaterial, soundCount);
	for(i = 0; i < soundCount; ++i)
	{
		// WARNING: strings must be stored safely somewhere in the memory before calling PRECACHE!
		//conprintf(2, " Precaching %s\n", pSoundList[i]);
		PRECACHE_SOUND((char *)pSoundList[i]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: play random shard sound
// Input  : *pEdict - 
//			soundMaterial - 
//			volume - 
//-----------------------------------------------------------------------------
void MaterialSoundRandom(edict_t *pEdict, Materials soundMaterial, float volume)
{
	short soundCount = 0;
	const char **pSoundList = MaterialSoundList(soundMaterial, soundCount);
	if (soundCount)
	{
		const char *sample = pSoundList[RANDOM_LONG(0,soundCount-1)];
		//conprintf(2, "MaterialSoundRandom(%d, %s)\n", soundMaterial, sample);
		EMIT_SOUND_DYN(pEdict, CHAN_BODY, sample, volume, ATTN_STATIC, 0, RANDOM_LONG(95, 105));
	}
}
